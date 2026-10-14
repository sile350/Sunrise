#include "exerciseprotocol.h"

#include "engines/exengine.h"

#include <QDateTime>
#include <QFile>
#include <QHash>
#include <QRegularExpression>
#include <QTextBlock>
#include <QTextCursor>
#include <QTextDocument>
#include <QTextDocumentFragment>
#include <QTextFrame>
#include <QTextTable>

namespace ExProtocol {
namespace {

const QString kSplit = QStringLiteral("<!--s-->");
const QString kNbsp2 = QStringLiteral("&nbsp;&nbsp;");

// Ширины колонок: шапка 200/471, процесс 304/304/63 или 120/214/274/63 — сумма всегда 671,
// колонка «Бал.» одинакова во всех протоколах.
constexpr int kHeadLeft = 200;
constexpr int kHeadRight = kTableWidth - kHeadLeft;
constexpr int kScore = 63;
constexpr int kHelp3 = 304;
constexpr int kHelp4 = 274;
constexpr int kFirst = 120;
constexpr int kOr3 = kTableWidth - kHelp3 - kScore;
constexpr int kOr4 = kTableWidth - kFirst - kHelp4 - kScore;

const QString kViewTable = QStringLiteral(
    "<table width='%1' border='1' cellspacing='0' cellpadding='4' "
    "style='border-collapse:collapse; border-color:#000000; border-style:solid; margin:0px'>")
                               .arg(kTableWidth);
const QString kStoredTable = QStringLiteral(
    "<table style='table-layout:fixed' border='1' cellspacing='0' cellpadding='0' width='%1'>")
                                 .arg(kTableWidth);

const QRegularExpression &rowRe() {
    static const QRegularExpression re(QStringLiteral("<tr\\b[^>]*>(.*?)</tr>"),
                                       QRegularExpression::CaseInsensitiveOption
                                           | QRegularExpression::DotMatchesEverythingOption);
    return re;
}

const QRegularExpression &cellRe() {
    static const QRegularExpression re(QStringLiteral("<td\\b([^>]*)>(.*?)</td>"),
                                       QRegularExpression::CaseInsensitiveOption
                                           | QRegularExpression::DotMatchesEverythingOption);
    return re;
}

struct Cell {
    QString attrs;
    QString html;
};

// QString::trimmed() срезает и U+00A0, а ведущие неразрывные пробелы — часть содержимого ячеек.
QString asciiTrim(const QString &s) {
    int b = 0;
    int e = s.size();
    auto ws = [](QChar c) { return c == QLatin1Char(' ') || c == QLatin1Char('\t') || c == QLatin1Char('\r') || c == QLatin1Char('\n'); };
    while (b < e && ws(s.at(b))) {
        ++b;
    }
    while (e > b && ws(s.at(e - 1))) {
        --e;
    }
    return s.mid(b, e - b);
}

QList<QList<Cell>> parseRows(const QString &html) {
    QList<QList<Cell>> rows;
    auto it = rowRe().globalMatch(html);
    while (it.hasNext()) {
        const QString rowHtml = it.next().captured(1);
        QList<Cell> cells;
        auto cit = cellRe().globalMatch(rowHtml);
        while (cit.hasNext()) {
            const QRegularExpressionMatch m = cit.next();
            cells.append({m.captured(1), m.captured(2)});
        }
        if (!cells.isEmpty()) {
            rows.append(cells);
        }
    }
    return rows;
}

QString plain(const QString &html) {
    QString t = QTextDocumentFragment::fromHtml(html).toPlainText();
    t.replace(QChar(0x00A0), QLatin1Char(' '));
    return t.simplified();
}

// Содержимое ячейки без обёртки <div contenteditable …>.
QString inner(const QString &html) {
    static const QRegularExpression wrap(QStringLiteral("^<div\\b[^>]*>(.*)</div>$"),
                                         QRegularExpression::CaseInsensitiveOption
                                             | QRegularExpression::DotMatchesEverythingOption);
    QString s = asciiTrim(html);
    const QRegularExpressionMatch m = wrap.match(s);
    if (m.hasMatch()) {
        s = asciiTrim(m.captured(1));
    }
    return s;
}

QString comparable(QString text) {
    text.replace(QChar(QChar::ParagraphSeparator), QLatin1Char('\n'));
    text.replace(QChar(QChar::LineSeparator), QLatin1Char('\n'));
    text.replace(QChar(0x00A0), QLatin1Char(' '));
    int e = text.size();
    while (e > 0 && (text.at(e - 1) == QLatin1Char('\n') || text.at(e - 1) == QLatin1Char(' '))) {
        --e;
    }
    return text.left(e);
}

// Ячейка меняется только при реальной правке: иначе сохраняется исходная разметка (&nbsp;, <br>).
void readCell(QTextTable *table, int row, int column, QString *html) {
    const QTextTableCell cell = table->cellAt(row, column);
    if (!cell.isValid()) {
        return;
    }
    QTextCursor cursor = cell.firstCursorPosition();
    cursor.setPosition(cell.lastCursorPosition().position(), QTextCursor::KeepAnchor);
    const QString text = cursor.selectedText();
    if (comparable(text) == comparable(QTextDocumentFragment::fromHtml(*html).toPlainText())) {
        return;
    }
    QString out = text.toHtmlEscaped();
    out.replace(QChar(QChar::ParagraphSeparator), QStringLiteral("<br>"));
    out.replace(QChar(QChar::LineSeparator), QStringLiteral("<br>"));
    out.replace(QChar(0x00A0), QStringLiteral("&nbsp;"));
    *html = out;
}

QList<QTextTable *> topLevelTables(QTextDocument *document) {
    QList<QTextTable *> tables;
    for (QTextFrame *frame : document->rootFrame()->childFrames()) {
        if (auto *table = qobject_cast<QTextTable *>(frame)) {
            tables.append(table);
        }
    }
    return tables;
}

int processColumns(const Record &r) {
    return r.firstColumn.isEmpty() ? 3 : 4;
}

QString headerCellHtml(QString html) {
    html.replace(QRegularExpression(QStringLiteral("<p\\b[^>]*>"), QRegularExpression::CaseInsensitiveOption), QString());
    html.replace(QRegularExpression(QStringLiteral("</p\\s*>"), QRegularExpression::CaseInsensitiveOption), QStringLiteral("<br>"));
    html.replace(QRegularExpression(QStringLiteral("\\s+")), QStringLiteral(" "));
    html = asciiTrim(html);
    static const QRegularExpression tailBr(QStringLiteral("(\\s*<br\\s*/?>\\s*)+$"), QRegularExpression::CaseInsensitiveOption);
    html.remove(tailBr);
    return asciiTrim(html);
}

QString summaryTable(const Entry &e) {
    const Record &r = e.record;
    QString h = kViewTable;
    for (const HeaderRow &row : e.header) {
        const QString bg = row.background.isEmpty() ? QString() : QStringLiteral(" bgcolor='%1'").arg(row.background);
        h += QStringLiteral("<tr><td width='%1' valign='top'%3>%4</td><td width='%2' valign='top'%3>%5</td></tr>")
                 .arg(kHeadLeft)
                 .arg(kHeadRight)
                 .arg(bg, row.left, row.right);
    }
    const QString left = QStringLiteral("<tr><td width='%1' valign='top'>").arg(kHeadLeft);
    const QString right = QStringLiteral("</td><td width='%1' valign='top'>").arg(kHeadRight);
    h += left + QStringLiteral("Дата/специалист") + right + escape(r.dateLine) + QStringLiteral("</td></tr>");
    h += left + QStringLiteral("Результат: баллы (макс %1)").arg(r.maxScore) + right + r.result + QStringLiteral("</td></tr>");
    h += left + QStringLiteral("Примечание") + right + r.note + QStringLiteral("</td></tr>");
    h += QStringLiteral("<tr><td colspan='2' align='center'>Процесс выполнения упражнения</td></tr>");
    h += QStringLiteral("</table>");
    return h;
}

QString processTable(const Record &r) {
    const bool four = processColumns(r) == 4;
    QString h = kViewTable;
    const QString head = QStringLiteral("<td width='%1' align='center'><b>%2</b></td>");
    h += QStringLiteral("<tr>");
    if (four) {
        h += head.arg(kFirst).arg(escape(r.firstColumn));
    }
    h += head.arg(four ? kOr4 : kOr3).arg(QStringLiteral("Характер деятельности"));
    h += head.arg(four ? kHelp4 : kHelp3).arg(QStringLiteral("Виды помощи"));
    h += head.arg(kScore).arg(QStringLiteral("Бал."));
    h += QStringLiteral("</tr>");
    for (const Row &row : r.rows) {
        h += QStringLiteral("<tr>");
        if (four) {
            h += QStringLiteral("<td width='%1' valign='top' align='center'>%2</td>").arg(kFirst).arg(row.first);
        }
        h += QStringLiteral("<td width='%1' valign='top'>%2</td>").arg(four ? kOr4 : kOr3).arg(row.orHtml);
        h += QStringLiteral("<td width='%1' valign='top'>%2</td>").arg(four ? kHelp4 : kHelp3).arg(row.hlpHtml);
        h += QStringLiteral("<td width='%1' valign='top' align='center'>%2</td>").arg(kScore).arg(row.score);
        h += QStringLiteral("</tr>");
    }
    if (r.hasMaxRow) {
        h += QStringLiteral("<tr><td colspan='%1'><b>Максимальный балл</b></td><td width='%2' align='center'>%3</td></tr>")
                 .arg(processColumns(r) - 1)
                 .arg(kScore)
                 .arg(r.maxValue);
    }
    h += QStringLiteral("</table>");
    return h;
}

int scoreValue(const QString &html, bool *ok) {
    QString t = plain(html);
    t.remove(QLatin1Char(';'));
    return t.trimmed().toInt(ok);
}

} // namespace

QFont textFont() {
    QFont f(QStringLiteral("Times New Roman"));
    f.setPointSize(12);
    return f;
}

QString escape(const QString &text) {
    return text.toHtmlEscaped();
}

QString storedBody(const Record &r) {
    QString s;
    s += QStringLiteral("<tr><td align='left'>Дата/специалист</td><td>") + escape(r.dateLine) + QStringLiteral(" </td></tr>");
    s += QStringLiteral("<tr><td>Результат: баллы (макс %1) </td><td><div%2>%3</div></td></tr>")
             .arg(r.maxScore)
             .arg(r.resultIsCopy ? QStringLiteral(" id='idcopy'") : QString(), r.result);
    s += QStringLiteral("<tr><td>Примечание </td><td><div>") + r.note + QStringLiteral("</div></td></tr>");
    s += QStringLiteral("<tr><td align='center' colspan='2'>Процесс выполнения упражнения</td></tr>");
    s += QStringLiteral("</table>") + kSplit + kStoredTable;
    s += QStringLiteral("<tr style='font-weight:bold'>");
    if (!r.firstColumn.isEmpty()) {
        s += QStringLiteral("<td align='center'>") + escape(r.firstColumn) + QStringLiteral("</td>");
    }
    s += QStringLiteral("<td align='center'>Характер деятельности</td><td align='center'>Виды помощи</td><td align='center'>Бал.</td></tr>");
    int n = 0;
    for (const Row &row : r.rows) {
        ++n;
        s += QStringLiteral("<tr>");
        if (!r.firstColumn.isEmpty()) {
            s += QStringLiteral("<td valign='top'><div align='center'>") + row.first + QStringLiteral("</div></td>");
        }
        s += QStringLiteral("<td valign='top'><div>") + row.orHtml + QStringLiteral("</div></td>");
        s += QStringLiteral("<td valign='top'><div>") + row.hlpHtml + QStringLiteral("</div></td>");
        s += QStringLiteral("<td align='center'><div%1>%2</div></td></tr>")
                 .arg(r.hasMaxRow ? QStringLiteral(" id='idb%1'").arg(n) : QString(), row.score);
    }
    if (r.hasMaxRow) {
        s += QStringLiteral("<tr><td colspan='%1'><b>Максимальный балл</b></td><td align='center'><div id='idsum'>%2</div></td></tr>")
                 .arg(processColumns(r) - 1)
                 .arg(r.maxValue);
    }
    s += QStringLiteral("</table>");
    return s;
}

Record parseStoredBody(const QString &body) {
    Record r;
    const int split = body.indexOf(kSplit);
    const QString summary = split >= 0 ? body.left(split) : body;
    const QString process = split >= 0 ? body.mid(split + kSplit.size()) : QString();
    for (const QList<Cell> &cells : parseRows(summary)) {
        if (cells.size() < 2) {
            continue;
        }
        const QString label = plain(cells.at(0).html);
        if (label.startsWith(QStringLiteral("Дата/специалист"))) {
            r.dateLine = plain(cells.at(1).html);
        } else if (label.startsWith(QStringLiteral("Результат"))) {
            static const QRegularExpression maxRe(QStringLiteral("макс\\s*(\\d+)"));
            const QRegularExpressionMatch m = maxRe.match(label);
            if (m.hasMatch()) {
                r.maxScore = m.captured(1).toInt();
            }
            r.resultIsCopy = cells.at(1).html.contains(QStringLiteral("idcopy"));
            r.result = inner(cells.at(1).html);
        } else if (label.startsWith(QStringLiteral("Примечание"))) {
            r.note = inner(cells.at(1).html);
        }
    }
    const QList<QList<Cell>> rows = parseRows(process);
    for (int i = 0; i < rows.size(); ++i) {
        const QList<Cell> &cells = rows.at(i);
        if (i == 0) {
            if (cells.size() >= 4) {
                r.firstColumn = plain(cells.at(0).html);
            }
            continue;
        }
        if (plain(cells.at(0).html).contains(QStringLiteral("Максимальный балл"))) {
            r.hasMaxRow = true;
            r.maxValue = inner(cells.last().html);
            continue;
        }
        Row row;
        int c = 0;
        if (!r.firstColumn.isEmpty() && cells.size() >= 4) {
            row.first = inner(cells.at(c++).html);
        }
        if (cells.size() - c < 3) {
            continue;
        }
        row.orHtml = inner(cells.at(c).html);
        row.hlpHtml = inner(cells.at(c + 1).html);
        row.score = inner(cells.at(c + 2).html);
        r.rows.append(row);
    }
    return r;
}

QList<HeaderRow> loadHeader(const QString &headerPath) {
    static QHash<QString, QList<HeaderRow>> cache;
    const auto cached = cache.constFind(headerPath);
    if (cached != cache.constEnd()) {
        return cached.value();
    }
    QString content = ExAssets::readAll(ExAssets::exnPath(headerPath));
    const int body = content.indexOf(QStringLiteral("<body"), 0, Qt::CaseInsensitive);
    if (body >= 0) {
        content = content.mid(body);
    }
    // В части header.html у второй строки нет открывающего <tr>: строки режутся по </tr>.
    QList<HeaderRow> rows;
    static const QRegularExpression trEnd(QStringLiteral("</tr\\s*>"), QRegularExpression::CaseInsensitiveOption);
    for (const QString &segment : content.split(trEnd)) {
        QList<Cell> cells;
        auto cit = cellRe().globalMatch(segment);
        while (cit.hasNext()) {
            const QRegularExpressionMatch m = cit.next();
            cells.append({m.captured(1), m.captured(2)});
        }
        if (cells.size() < 2) {
            continue;
        }
        HeaderRow row;
        row.left = headerCellHtml(cells.at(0).html);
        row.right = headerCellHtml(cells.at(1).html);
        static const QRegularExpression bgRe(QStringLiteral("bgcolor\\s*=\\s*[\"']?(#?\\w+)"), QRegularExpression::CaseInsensitiveOption);
        const QRegularExpressionMatch m = bgRe.match(cells.at(0).attrs);
        if (m.hasMatch()) {
            row.background = m.captured(1);
        }
        rows.append(row);
    }
    cache.insert(headerPath, rows);
    return rows;
}

QString defaultHeaderPath(const QString &uprid) {
    QString folder = uprid.trimmed();
    folder.replace(QLatin1Char(','), QLatin1Char('.'));
    if (folder == QLatin1String("1.2.3")) {
        return folder + QStringLiteral("/header1.html");
    }
    return folder + QStringLiteral("/header.html");
}

Entry templateEntry(const QString &templatePath) {
    Entry e;
    for (const HeaderRow &row : loadHeader(templatePath)) {
        if (plain(row.left).startsWith(QStringLiteral("Дата"))) {
            break;
        }
        e.header.append(row);
    }
    const QString content = ExAssets::readAll(ExAssets::exnPath(templatePath));
    static const QRegularExpression maxRe(QStringLiteral("макс\\.?\\s*(\\d+)"));
    const QRegularExpressionMatch m = maxRe.match(content);
    if (m.hasMatch()) {
        e.record.maxScore = m.captured(1).toInt();
    }
    const int first = content.indexOf(QStringLiteral("<table"), 0, Qt::CaseInsensitive);
    const int second = first >= 0 ? content.indexOf(QStringLiteral("<table"), first + 1, Qt::CaseInsensitive) : -1;
    if (second >= 0) {
        const QList<QList<Cell>> rows = parseRows(content.mid(second));
        if (!rows.isEmpty() && rows.first().size() >= 4) {
            e.record.firstColumn = plain(rows.first().first().html);
        }
    }
    const QString nb = QStringLiteral("&nbsp;");
    e.record.rows.append({nb, nb, nb, nb});
    return e;
}

QString calculateBls(const QString &bands, int correct, int counter) {
    const int pr = (correct > 0 && counter > 0) ? correct * 100 / counter : 0;
    QString bls = QStringLiteral("0");
    for (const QString &band : bands.split(QLatin1Char(';'), Qt::SkipEmptyParts)) {
        const QStringList p = band.split(QLatin1Char('|'));
        if (p.size() == 3 && pr >= p.at(0).toInt() && pr <= p.at(1).toInt()) {
            bls = p.at(2);
        }
    }
    return bls;
}

QString moscowNow() {
    return QDateTime::currentDateTimeUtc().addSecs(3 * 3600).toString(QStringLiteral("dd.MM.yyyy H:mm:ss"));
}

QString entriesHtml(const QList<Entry> &entries) {
    QString h;
    for (const Entry &e : entries) {
        h += summaryTable(e);
        if (e.withProcess) {
            h += processTable(e.record);
        }
    }
    return h;
}

QString documentHtml(const QString &preambleHtml, const QList<Entry> &entries) {
    return QStringLiteral("<html><head><meta http-equiv='Content-Type' content='text/html; charset=utf-8'/></head>"
                          "<body style=\"font-family:'Times New Roman'; font-size:12pt; color:#000000\">")
        + preambleHtml + entriesHtml(entries) + QStringLiteral("</body></html>");
}

void tightenTables(QTextDocument *document) {
    if (!document) {
        return;
    }
    constexpr qreal kEntryGap = 14;
    const auto isSummary = [](QTextTable *table) {
        if (table->rows() == 0) {
            return false;
        }
        const QTextTableCell cell = table->cellAt(table->rows() - 1, 0);
        QTextCursor c = cell.firstCursorPosition();
        c.setPosition(cell.lastCursorPosition().position(), QTextCursor::KeepAnchor);
        return c.selectedText().trimmed() == QStringLiteral("Процесс выполнения упражнения");
    };
    bool afterSummary = false;
    bool seenTable = false;
    for (QTextFrame::iterator it = document->rootFrame()->begin(); !it.atEnd(); ++it) {
        if (auto *table = qobject_cast<QTextTable *>(it.currentFrame())) {
            const bool summary = isSummary(table);
            QTextTableFormat format = table->format();
            format.setMargin(0);
            if (summary && seenTable) {
                format.setTopMargin(kEntryGap);
            }
            table->setFormat(format);
            afterSummary = summary;
            seenTable = true;
            continue;
        }
        const QTextBlock block = it.currentBlock();
        if (afterSummary && block.isValid() && block.length() <= 1) {
            QTextCursor cursor(block);
            QTextBlockFormat bf = block.blockFormat();
            bf.setTopMargin(0);
            bf.setBottomMargin(0);
            bf.setLineHeight(0, QTextBlockFormat::FixedHeight);
            cursor.setBlockFormat(bf);
            QTextCharFormat cf = block.charFormat();
            cf.setFontPointSize(1);
            cursor.setBlockCharFormat(cf);
        }
        afterSummary = false;
    }
}

QList<EditableCell> editableCells(const QList<Entry> &entries) {
    QList<EditableCell> cells;
    int table = 0;
    for (const Entry &e : entries) {
        const int h = e.header.size();
        cells.append({table, h + 1, 1});
        cells.append({table, h + 2, 1});
        ++table;
        if (!e.withProcess) {
            continue;
        }
        // Первая колонка (Уровень / Выполнение / Сюжет / Рассказ / Упражнение) — только просмотр.
        const int cols = processColumns(e.record);
        const int firstCol = cols == 4 ? 1 : 0;
        for (int row = 1; row <= e.record.rows.size(); ++row) {
            for (int col = firstCol; col < cols; ++col) {
                cells.append({table, row, col});
            }
        }
        if (e.record.hasMaxRow) {
            cells.append({table, e.record.rows.size() + 1, cols - 1});
        }
        ++table;
    }
    return cells;
}

QList<int> applyDocumentEdits(QTextDocument *document, QList<Entry> &entries) {
    QList<int> changed;
    if (!document) {
        return changed;
    }
    const QList<QTextTable *> tables = topLevelTables(document);
    int t = 0;
    for (int i = 0; i < entries.size(); ++i) {
        Entry &e = entries[i];
        const QString before = storedBody(e.record);
        const int h = e.header.size();
        if (t < tables.size() && tables.at(t)->rows() >= h + 3) {
            readCell(tables.at(t), h + 1, 1, &e.record.result);
            readCell(tables.at(t), h + 2, 1, &e.record.note);
        }
        ++t;
        if (e.withProcess) {
            const int cols = processColumns(e.record);
            QTextTable *pt = t < tables.size() ? tables.at(t) : nullptr;
            if (pt && pt->rows() >= e.record.rows.size() + 1 && pt->columns() == cols) {
                for (int r = 0; r < e.record.rows.size(); ++r) {
                    Row &row = e.record.rows[r];
                    int c = 0;
                    if (cols == 4) {
                        readCell(pt, r + 1, c++, &row.first);
                    }
                    readCell(pt, r + 1, c, &row.orHtml);
                    readCell(pt, r + 1, c + 1, &row.hlpHtml);
                    readCell(pt, r + 1, c + 2, &row.score);
                }
                if (e.record.hasMaxRow && pt->rows() >= e.record.rows.size() + 2) {
                    readCell(pt, e.record.rows.size() + 1, cols - 1, &e.record.maxValue);
                }
            }
            ++t;
        }
        if (storedBody(e.record) != before) {
            changed.append(i);
        }
    }
    return changed;
}

bool applyItog(Record &record) {
    bool any = false;
    int best = -3;
    for (const Row &row : record.rows) {
        bool ok = false;
        const int v = scoreValue(row.score, &ok);
        if (ok) {
            best = any ? qMax(best, v) : v;
            any = true;
        }
    }
    if (!any) {
        return false;
    }
    record.maxValue = QString::number(best);
    if (record.resultIsCopy) {
        record.result = kNbsp2 + QString::number(best);
    }
    return true;
}

} // namespace ExProtocol
