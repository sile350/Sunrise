#include "sunrisewindow.h"
#include "appsettings.h"
#include "custommessagebox.h"
#include "rtfconverter.h"
#include "usagejournal.h"

#include <QClipboard>
#include <QTextCursor>
#include <QApplication>
#include <QCoreApplication>
#include <QEventLoop>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QRadioButton>
#include <QButtonGroup>
#include <QStandardPaths>
#include <QFileDialog>
#include <QFrame>
#include <QFont>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QIcon>
#include <QInputDialog>
#include <QGuiApplication>
#include <QMouseEvent>
#include <QMoveEvent>
#include <QResizeEvent>
#include <QPalette>
#include <QColor>
#include <QBrush>
#include <QPixmap>
#include <QStyledItemDelegate>
#include <QPainter>
#include <QPolygonF>
#include <QPushButton>
#include <QPrinter>
#include <QPrintDialog>
#include <QToolTip>
#include <QDesktopServices>
#include <QProcess>
#include <QTransform>
#include <QStandardPaths>
#include <QSignalBlocker>
#include <QStandardItemModel>
#include <QTextCharFormat>
#include <QTextBlock>
#include <QTextBlockFormat>
#include <QTextCodec>
#include <QTextDocumentWriter>
#include <QTextLayout>
#include <QTextOption>
#include <QSet>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QUrl>
#include <QVBoxLayout>
#include <QTimer>
#include <QtConcurrent>
#include <QFutureWatcher>
#include <QPropertyAnimation>
#include <QKeyEvent>
#include <QKeySequence>
#include <QShowEvent>
#include <QScreen>
#include <QScrollBar>
#include <QShortcut>
#include <QWindow>
#include <QSlider>
#include <memory>
#include <QRegularExpression>
#include <QMenu>
#include <QStyleFactory>
#include <QListView>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QCheckBox>
#include <QGraphicsOpacityEffect>
#include <QGroupBox>
#include <QTextDocument>
#include <algorithm>
#include <QCalendarWidget>
#include <QToolTip>
#include <QAbstractButton>
#include <QFileInfo>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPointer>
#include <functional>

namespace {

struct UserSaveResult {
    bool ok = false;
    QString error;
    QString userId;
    QString login;
    QString password;
};

QString journalUserName(const SessionUser &user) {
    QString name = user.fio.trimmed().isEmpty() ? user.login.trimmed() : user.fio.trimmed();
    if (!name.endsWith(QLatin1Char('.'))) {
        name += QLatin1Char('.');
    }
    return name;
}

QPixmap tintedPixmap(const QString &path, const QColor &color, const QSize &size) {
    QPixmap pixmap(path);
    if (pixmap.isNull()) {
        return {};
    }
    pixmap = pixmap.scaled(size, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    QImage image = pixmap.toImage().convertToFormat(QImage::Format_ARGB32);
    for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < image.width(); ++x) {
            const QRgb pixel = image.pixel(x, y);
            if (qAlpha(pixel) == 0) {
                continue;
            }
            image.setPixel(
                x,
                y,
                qRgba(color.red(), color.green(), color.blue(), qAlpha(pixel))
            );
        }
    }
    return QPixmap::fromImage(image);
}

QString dropdownArrowImagePath() {
    static QString cachedPath;
    if (!cachedPath.isEmpty()) {
        return cachedPath;
    }

    const QStringList roots = {
        QCoreApplication::applicationDirPath() + "/../assets/sysImages",
        QCoreApplication::applicationDirPath() + "/../../assets/sysImages",
        QDir::currentPath() + "/assets/sysImages"
    };
    for (const QString &root : roots) {
        const QString candidate = QDir(root).filePath("combo_arrow.png");
        if (QFile::exists(candidate)) {
            cachedPath = QDir::fromNativeSeparators(candidate);
            return cachedPath;
        }
    }

    const QString filePath = QDir(QStandardPaths::writableLocation(QStandardPaths::TempLocation))
                                 .filePath("infant_combo_arrow.png");
    if (!QFile::exists(filePath)) {
        QPixmap pixmap(10, 6);
        pixmap.fill(Qt::transparent);
        QPainter painter(&pixmap);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(0x33, 0x33, 0x33));
        QPolygonF arrow;
        arrow << QPointF(0.0, 0.0) << QPointF(10.0, 0.0) << QPointF(5.0, 6.0);
        painter.drawPolygon(arrow);
        pixmap.save(filePath, "PNG");
    }
    cachedPath = QDir::fromNativeSeparators(filePath);
    return cachedPath;
}

// Таблицы стилей родителей перестраивают палитру поля при каждой переполировке, поэтому цвет подсказки восстанавливается после неё.
class PlaceholderColorKeeper final : public QObject {
public:
    PlaceholderColorKeeper(QLineEdit *edit, const QColor &color) : QObject(edit), m_color(color) {
        edit->installEventFilter(this);
        apply(edit);
    }

protected:
    bool eventFilter(QObject *watched, QEvent *event) override {
        const QEvent::Type type = event->type();
        if (type == QEvent::PaletteChange || type == QEvent::StyleChange || type == QEvent::Polish) {
            apply(static_cast<QWidget *>(watched));
        }
        return QObject::eventFilter(watched, event);
    }

private:
    void apply(QWidget *widget) const {
        if (widget->palette().color(QPalette::PlaceholderText) == m_color) {
            return;
        }
        QPalette palette = widget->palette();
        palette.setColor(QPalette::PlaceholderText, m_color);
        widget->setPalette(palette);
    }

    QColor m_color;
};

void keepPlaceholderColor(QLineEdit *edit) {
    static const char *const kInstalled = "_legacyPlaceholderColorKeeper";
    if (!edit->property(kInstalled).toBool()) {
        edit->setProperty(kInstalled, true);
        new PlaceholderColorKeeper(edit, QColor(0xc0, 0xc0, 0xc0));
    }
}

QString dropdownArrowCss() {
    QString path = dropdownArrowImagePath();
    path.replace('\\', '/');
    return QStringLiteral(
        "  width: 10px;"
        "  height: 6px;"
        "  image: url(\"%1\");"
    ).arg(path);
}

QString panelCheckBoxStyleSheet() {
    return QStringLiteral(
        "QCheckBox, QRadioButton {"
        "  background: transparent;"
        "  color: #000000;"
        "  font-family: 'Microsoft Sans Serif';"
        "  font-size: 8.25pt;"
        "  spacing: 4px;"
        "}"
        "QCheckBox::indicator { width: 13px; height: 13px; }"
        "QCheckBox:disabled, QRadioButton:disabled { color: rgba(0, 0, 0, 128); }"
    );
}

void setPanelChildOpacity(QWidget *widget, bool enabled) {
    if (!widget) {
        return;
    }
    if (enabled) {
        widget->setGraphicsEffect(nullptr);
        return;
    }
    auto *effect = qobject_cast<QGraphicsOpacityEffect *>(widget->graphicsEffect());
    if (!effect) {
        effect = new QGraphicsOpacityEffect(widget);
        widget->setGraphicsEffect(effect);
    }
    effect->setOpacity(0.45);
}

QString whiteScrollBarCss(const QString &widgetSelector) {
    return QStringLiteral(
        "%1 QScrollBar:vertical {"
        "  background-color: #ffffff;"
        "  background-image: none;"
        "  border: none;"
        "  width: 14px;"
        "  margin: 0px;"
        "}"
        "%1 QScrollBar::handle:vertical {"
        "  background-color: #c1c1c1;"
        "  background-image: none;"
        "  min-height: 20px;"
        "  border: none;"
        "}"
        "%1 QScrollBar::add-line:vertical, %1 QScrollBar::sub-line:vertical {"
        "  background-color: #ffffff;"
        "  background-image: none;"
        "  border: none;"
        "  height: 14px;"
        "  subcontrol-origin: margin;"
        "}"
        "%1 QScrollBar::up-arrow:vertical, %1 QScrollBar::down-arrow:vertical {"
        "  background-color: #ffffff;"
        "  background-image: none;"
        "  border: none;"
        "  width: 14px;"
        "  height: 14px;"
        "}"
        "%1 QScrollBar::add-page:vertical, %1 QScrollBar::sub-page:vertical {"
        "  background-color: #ffffff;"
        "  background-image: none;"
        "}"
        "%1 QScrollBar:horizontal {"
        "  background-color: #ffffff;"
        "  background-image: none;"
        "  border: none;"
        "  height: 14px;"
        "  margin: 0px;"
        "}"
        "%1 QScrollBar::handle:horizontal {"
        "  background-color: #c1c1c1;"
        "  background-image: none;"
        "  min-width: 20px;"
        "  border: none;"
        "}"
        "%1 QScrollBar::add-line:horizontal, %1 QScrollBar::sub-line:horizontal {"
        "  background-color: #ffffff;"
        "  background-image: none;"
        "  border: none;"
        "  width: 14px;"
        "  subcontrol-origin: margin;"
        "}"
        "%1 QScrollBar::left-arrow:horizontal, %1 QScrollBar::right-arrow:horizontal {"
        "  background-color: #ffffff;"
        "  background-image: none;"
        "  border: none;"
        "  width: 14px;"
        "  height: 14px;"
        "}"
        "%1 QScrollBar::add-page:horizontal, %1 QScrollBar::sub-page:horizontal {"
        "  background-color: #ffffff;"
        "  background-image: none;"
        "}"
    ).arg(widgetSelector);
}

// Отрисовка таблиц в виде стандартного DataGridView (WinForms, визуальные стили Windows 10).
const QColor kGridLineColor(0xa0, 0xa0, 0xa0);
const QColor kGridHeaderSeparatorColor(0xe5, 0xe5, 0xe5);
const QColor kGridHeaderHighlightColor(0xf1, 0xf1, 0xf1);
const QColor kGridSelectionColor(0x00, 0x78, 0xd7);
constexpr int kGridHeaderTextLeft = 7;
constexpr int kGridHeaderTextRight = 5;
constexpr int kGridSortGlyphArea = 20;

QFont legacyGridFont() {
    QFont font(QStringLiteral("Microsoft Sans Serif"));
    font.setPointSizeF(8.25);
    return font;
}

QTextOption legacyHeaderTextOption() {
    QTextOption option(Qt::AlignLeft | Qt::AlignVCenter);
    option.setWrapMode(QTextOption::NoWrap);
    return option;
}

// Перенос как у GDI DrawText(DT_WORDBREAK) для кириллицы: по символам, без переноса перед знаком препинания.
QStringList legacyWrapLines(const QFont &font, const QString &text, int width) {
    static const QString noLineStart = QStringLiteral(".,;:!?)]}");
    const QFontMetricsF fm(font);
    QStringList lines;
    int start = 0;
    while (start < text.size()) {
        int end = start + 1;
        while (end < text.size() && fm.horizontalAdvance(text.mid(start, end + 1 - start).trimmed()) <= width) {
            ++end;
        }
        if (end < text.size()) {
            while (end - 1 > start && noLineStart.contains(text.at(end))) {
                --end;
            }
        }
        lines << text.mid(start, end - start).trimmed();
        start = end;
        while (start < text.size() && text.at(start).isSpace()) {
            ++start;
        }
    }
    if (lines.isEmpty()) {
        lines << QString();
    }
    return lines;
}

class LegacyGridHeader final : public QHeaderView {
public:
    explicit LegacyGridHeader(QWidget *parent = nullptr) : QHeaderView(Qt::Horizontal, parent) {
        setSectionsClickable(true);
        setHighlightSections(false);
        setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    }

    std::function<void(int)> onSectionClicked;

    void setSortableSections(const QSet<int> &sections) { m_sortable = sections; }
    void setSortGlyph(int section, Qt::SortOrder order) {
        m_sortSection = section;
        m_sortOrder = order;
        viewport()->update();
    }

    void updateHeight() {
        const QFontMetrics fm(font());
        int lines = 1;
        for (int i = 0; i < count(); ++i) {
            if (isSectionHidden(i)) {
                continue;
            }
            const QString text = model() ? model()->headerData(i, Qt::Horizontal).toString() : QString();
            lines = qMax(lines, int(legacyWrapLines(font(), text, textWidth(i)).size()));
        }
        setFixedHeight(lines * fm.lineSpacing() + 8);
    }

protected:
    void paintSection(QPainter *painter, const QRect &rect, int logicalIndex) const override {
        if (!rect.isValid()) {
            return;
        }
        painter->save();
        painter->fillRect(rect, Qt::white);
        painter->fillRect(QRect(rect.left(), rect.top(), rect.width(), 1), kGridLineColor);
        painter->fillRect(QRect(rect.left(), rect.top() + 1, rect.width(), 1), kGridHeaderHighlightColor);
        painter->fillRect(QRect(rect.right(), rect.top() + 2, 1, rect.height() - 3), kGridHeaderSeparatorColor);
        painter->fillRect(QRect(rect.left(), rect.bottom(), rect.width(), 1), kGridLineColor);

        const QString text = model() ? model()->headerData(logicalIndex, Qt::Horizontal).toString() : QString();
        const QRect textRect(rect.left() + kGridHeaderTextLeft, rect.top(), textWidth(logicalIndex), rect.height());
        painter->setFont(font());
        painter->setPen(Qt::black);
        painter->setClipRect(rect.adjusted(0, 0, -1, -1));
        const QString wrapped = legacyWrapLines(font(), text, textWidth(logicalIndex)).join(QChar::LineSeparator);
        painter->drawText(QRectF(textRect), wrapped, legacyHeaderTextOption());

        if (logicalIndex == m_sortSection) {
            const int glyphRight = rect.right() - 4;
            const int glyphTop = rect.center().y() - 1;
            for (int i = 0; i < 5; ++i) {
                const int half = m_sortOrder == Qt::AscendingOrder ? i : 4 - i;
                painter->fillRect(QRect(glyphRight - 4 - half, glyphTop + i, half * 2 + 1, 1), kGridLineColor);
            }
        }
        painter->restore();
    }

    void mousePressEvent(QMouseEvent *event) override {
        const int pos = event->pos().x();
        const int section = logicalIndexAt(pos);
        const int edge = section >= 0 ? sectionViewportPosition(section) + sectionSize(section) : -1;
        const int start = section >= 0 ? sectionViewportPosition(section) : -1;
        const bool nearEdge = section >= 0 && (qAbs(pos - edge) <= 3 || (qAbs(pos - start) <= 3 && start > 0));
        if (nearEdge || event->button() != Qt::LeftButton) {
            QHeaderView::mousePressEvent(event);
            return;
        }
        if (section >= 0 && onSectionClicked) {
            onSectionClicked(section);
        }
    }

private:
    int textWidth(int logicalIndex) const {
        int width = sectionSize(logicalIndex) - kGridHeaderTextLeft - kGridHeaderTextRight;
        if (m_sortable.contains(logicalIndex)) {
            width -= kGridSortGlyphArea;
        }
        return qMax(1, width);
    }

    QSet<int> m_sortable;
    int m_sortSection = -1;
    Qt::SortOrder m_sortOrder = Qt::AscendingOrder;
};

class LegacyRowHeader final : public QHeaderView {
public:
    explicit LegacyRowHeader(QTableView *table) : QHeaderView(Qt::Vertical, table), m_table(table) {
        setSectionsClickable(true);
        setHighlightSections(false);
        setFixedWidth(51);
    }

protected:
    void paintSection(QPainter *painter, const QRect &rect, int logicalIndex) const override {
        if (!rect.isValid()) {
            return;
        }
        painter->save();
        painter->fillRect(rect, Qt::white);
        painter->fillRect(QRect(rect.left(), rect.top(), 1, rect.height()), kGridLineColor);
        painter->fillRect(QRect(rect.left() + 1, rect.top(), 1, rect.height()), kGridHeaderHighlightColor);
        painter->fillRect(QRect(rect.left() + 2, rect.bottom(), rect.width() - 3, 1), kGridHeaderSeparatorColor);
        painter->fillRect(QRect(rect.right(), rect.top(), 1, rect.height()), kGridLineColor);
        if (m_table && m_table->currentIndex().isValid() && m_table->currentIndex().row() == logicalIndex) {
            static const unsigned char glyph[11][8] = {
                {246, 247, 253, 255, 255, 255, 255, 255},
                {246, 100, 235, 252, 255, 255, 255, 255},
                {246, 66, 100, 235, 252, 255, 255, 255},
                {246, 66, 66, 89, 212, 250, 255, 255},
                {246, 66, 66, 66, 77, 212, 249, 255},
                {246, 66, 66, 66, 66, 77, 235, 250},
                {246, 66, 66, 66, 77, 212, 249, 255},
                {246, 66, 66, 77, 212, 249, 255, 255},
                {246, 66, 100, 224, 251, 255, 255, 255},
                {246, 100, 235, 252, 255, 255, 255, 255},
                {246, 235, 252, 255, 255, 255, 255, 255},
            };
            const int x = rect.left() + 6;
            const int top = rect.top() + (rect.height() - 11) / 2 + 1;
            for (int row = 0; row < 11; ++row) {
                for (int col = 0; col < 8; ++col) {
                    const int v = glyph[row][col];
                    if (v < 255) {
                        painter->fillRect(QRect(x + col, top + row, 1, 1), QColor(v, v, v));
                    }
                }
            }
        }
        painter->restore();
    }

private:
    QTableView *m_table = nullptr;
};

class AdminUsersItemDelegate final : public QStyledItemDelegate {
public:
    explicit AdminUsersItemDelegate(QObject *parent = nullptr) : QStyledItemDelegate(parent) {}

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override {
        QStyledItemDelegate::paint(painter, option, index);
        if (!index.isValid() || index.column() == 0) {
            return;
        }

        painter->save();
        painter->setPen(QPen(Qt::white, 1));
        const QRect rect = option.rect;
        painter->drawLine(rect.bottomLeft(), rect.bottomRight());
        if (index.column() == 1 || index.column() == 2) {
            painter->drawLine(rect.topRight(), rect.bottomRight());
        }
        painter->restore();
    }
};

class LegacyGridDelegate final : public QStyledItemDelegate {
public:
    using QStyledItemDelegate::QStyledItemDelegate;

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override {
        painter->save();
        const bool selected = option.state & QStyle::State_Selected;
        painter->fillRect(option.rect, selected ? kGridSelectionColor : QColor(Qt::white));
        painter->setClipRect(option.rect);
        const QPixmap pixmap = index.data(Qt::DecorationRole).value<QPixmap>();
        if (!pixmap.isNull()) {
            const QSize size = pixmap.size();
            const QPoint topLeft(
                option.rect.left() + (option.rect.width() - size.width()) / 2,
                option.rect.top() + (option.rect.height() - size.height()) / 2
            );
            painter->drawPixmap(topLeft, pixmap);
        } else {
            const QRect textRect = option.rect.adjusted(2, 0, -2, 0);
            painter->setFont(option.font);
            painter->setPen(selected ? QColor(Qt::white) : QColor(Qt::black));
            const QString text = option.fontMetrics.elidedText(index.data(Qt::DisplayRole).toString(), Qt::ElideRight, textRect.width());
            painter->drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, text);
        }
        painter->restore();
    }
};

LegacyGridHeader *setupLegacyGrid(QTableWidget *table, const QFont &headerFont) {
    auto *header = new LegacyGridHeader(table);
    header->setFont(headerFont);
    table->setHorizontalHeader(header);
    auto *rowHeader = new LegacyRowHeader(table);
    table->setVerticalHeader(rowHeader);
    rowHeader->setDefaultSectionSize(27);
    rowHeader->setMinimumSectionSize(3);
    rowHeader->setSectionResizeMode(QHeaderView::Interactive);
    header->setSectionResizeMode(QHeaderView::Interactive);
    header->setStretchLastSection(false);
    header->setMinimumSectionSize(5);
    table->setFont(legacyGridFont());
    table->setItemDelegate(new LegacyGridDelegate(table));
    table->setShowGrid(true);
    table->setWordWrap(false);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setSelectionBehavior(QAbstractItemView::SelectItems);
    table->setSelectionMode(QAbstractItemView::ExtendedSelection);
    table->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    table->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    table->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    table->setFrameShape(QFrame::NoFrame);
    table->setStyleSheet(QStringLiteral(
        "QTableWidget {"
        "  background: white;"
        "  color: black;"
        "  border: 1px solid black;"
        "  gridline-color: #a0a0a0;"
        "  outline: 0;"
        "}"
        "QTableCornerButton::section {"
        "  background: white;"
        "  border: 1px solid #a0a0a0;"
        "}"
    ) + ImageButton::toolTipStyleSheet());
    QObject::connect(table->selectionModel(), &QItemSelectionModel::currentChanged, rowHeader, [rowHeader]() {
        rowHeader->viewport()->update();
    });
    return header;
}

void applyTruncatedTextTooltips(QTableWidget *table, const QList<int> &columns) {
    const QFontMetrics fm(table->font());
    for (int column : columns) {
        const int available = table->columnWidth(column) - 6;
        for (int row = 0; row < table->rowCount(); ++row) {
            if (QTableWidgetItem *item = table->item(row, column)) {
                item->setToolTip(fm.horizontalAdvance(item->text()) > available ? item->text() : QString());
            }
        }
    }
}

class WhitePanelWidget final : public QWidget {
public:
    explicit WhitePanelWidget(QWidget *parent = nullptr) : QWidget(parent) {
        setAttribute(Qt::WA_StyledBackground, true);
        setAttribute(Qt::WA_OpaquePaintEvent, true);
        setAutoFillBackground(true);
    }

protected:
    void paintEvent(QPaintEvent *event) override {
        QPainter painter(this);
        painter.fillRect(rect(), Qt::white);
        QWidget::paintEvent(event);
    }
};

class WorkPanelWidget final : public QWidget {
public:
    explicit WorkPanelWidget(QWidget *parent = nullptr, const QColor &background = QColor(0xf2, 0xf0, 0xf0))
        : QWidget(parent), m_background(background) {
        setAttribute(Qt::WA_StyledBackground, true);
        setAutoFillBackground(true);
    }

protected:
    void paintEvent(QPaintEvent *event) override {
        QPainter painter(this);
        painter.fillRect(rect(), m_background);
        QWidget::paintEvent(event);
    }

private:
    QColor m_background;
};

class ClickableLabel final : public QLabel {
public:
    using QLabel::QLabel;
    std::function<void()> onClick;

protected:
    void mousePressEvent(QMouseEvent *event) override {
        if (event->button() == Qt::LeftButton && onClick) {
            onClick();
        }
        QLabel::mousePressEvent(event);
    }
};

class GrayTitleLabel final : public QLabel {
public:
    explicit GrayTitleLabel(QWidget *parent = nullptr) : QLabel(parent) {
        setAttribute(Qt::WA_OpaquePaintEvent, true);
    }

protected:
    void paintEvent(QPaintEvent *event) override {
        QPainter painter(this);
        painter.fillRect(rect(), QColor(0xf2, 0xf0, 0xf0));
        QLabel::paintEvent(event);
    }
};

QString normalizeAnamnesisHtmlFonts(QString html) {
    html.replace(QStringLiteral("Calibri"), QStringLiteral("Times New Roman"));
    html.replace(QStringLiteral("Times New Roman CYR"), QStringLiteral("Times New Roman"));
    html.replace(
        QRegularExpression(QStringLiteral("font-size:\\s*\\d+(?:\\.\\d+)?pt")),
        QStringLiteral("font-size:12pt")
    );
    html.replace(
        QRegularExpression(QStringLiteral("line-height:\\s*[^;\"']+")),
        QStringLiteral("line-height:85%")
    );
    html.replace(
        QRegularExpression(QStringLiteral("margin-top:\\s*[^;\"']+")),
        QStringLiteral("margin-top:0pt")
    );
    html.replace(
        QRegularExpression(QStringLiteral("margin-bottom:\\s*[^;\"']+")),
        QStringLiteral("margin-bottom:0pt")
    );
    return html;
}

bool isRawRtfPlainText(const QString &plainText) {
    return plainText.trimmed().startsWith(QStringLiteral("{\\rtf"));
}

bool isWordExportHtml(const QString &html) {
    return html.contains(QStringLiteral("schemas-microsoft-com:office"))
        || html.contains(QStringLiteral("ProgId content=\"Word.Document\""))
        || html.contains(QStringLiteral("class=MsoNormal"));
}

QString extractHtmlBodyContent(const QString &html) {
    const int bodyStart = html.indexOf(QStringLiteral("<body"), 0, Qt::CaseInsensitive);
    if (bodyStart < 0) {
        return html;
    }
    const int contentStart = html.indexOf(QLatin1Char('>'), bodyStart);
    if (contentStart < 0) {
        return html;
    }
    const int bodyEnd = html.indexOf(QStringLiteral("</body>"), contentStart, Qt::CaseInsensitive);
    if (bodyEnd < 0) {
        return html;
    }
    return html.mid(contentStart + 1, bodyEnd - contentStart - 1);
}

QString stripWordArtifacts(QString html) {
    html.remove(QStringLiteral("<o:p></o:p>"));
    html.remove(QStringLiteral("<o:p/>"));
    while (true) {
        const int start = html.indexOf(QStringLiteral("<o:p"));
        if (start < 0) {
            break;
        }
        const int end = html.indexOf(QStringLiteral("</o:p>"), start);
        if (end < 0) {
            break;
        }
        html.remove(start, end + 6 - start);
    }
    return html;
}

QString prepareAnamnesisHtml(QString html) {
    if (html.size() > 50000 || isWordExportHtml(html)) {
        html = stripWordArtifacts(extractHtmlBodyContent(html));
    }

    if (html.size() > 150000) {
        return {};
    }

    html = normalizeAnamnesisHtmlFonts(html);

    if (!html.contains(QStringLiteral("<html"), Qt::CaseInsensitive)) {
        html = QStringLiteral(
            "<!DOCTYPE html><html><head><meta charset=\"utf-8\">"
            "<style>"
            "body { font-family: 'Times New Roman', serif; font-size: 12pt; color: #000000; background-color: #ffffff; }"
            "p { margin-top: 0pt; margin-bottom: 0pt; line-height: 85%; }"
            "</style></head><body>%1</body></html>"
        ).arg(html);
    }

    return html;
}

QString extractHelpStylesheet(const QString &html);
QString simplifyHelpInlineStyle(QString declarations);
QHash<QString, QString> parseHelpCssClassRules(const QString &stylesheet);
void appendStyleAttribute(QString *openTag, const QString &styleToAdd);
void applyHelpClassStylesToHtml(QString &html, const QHash<QString, QString> &classRules);
void reinforceHelpRichTextTags(QString &html);
QString buildHelpDefaultStylesheet(const QString &html);
void compactHelpSpacingStyles(QString &html);
void compactHelpDocumentSpacing(QTextDocument *doc);
QString normalizeHelpMarginsInCss(QString css);

QString prepareHelpHtml(QString html) {
    const QString stylesheet = extractHelpStylesheet(html);
    const QHash<QString, QString> classRules = parseHelpCssClassRules(stylesheet);
    applyHelpClassStylesToHtml(html, classRules);
    compactHelpSpacingStyles(html);
    reinforceHelpRichTextTags(html);
    return html;
}

QString extractHelpStylesheet(const QString &html) {
    const int styleStart = html.indexOf(QStringLiteral("<style"), 0, Qt::CaseInsensitive);
    if (styleStart < 0) {
        return {};
    }
    const int contentStart = html.indexOf(QLatin1Char('>'), styleStart);
    if (contentStart < 0) {
        return {};
    }
    const int styleEnd = html.indexOf(QStringLiteral("</style>"), contentStart, Qt::CaseInsensitive);
    if (styleEnd < 0) {
        return {};
    }
    QString css = html.mid(contentStart + 1, styleEnd - contentStart - 1);
    css.replace(QStringLiteral("<!--"), QString());
    css.replace(QStringLiteral("-->"), QString());
    return css;
}

QString normalizeHelpMarginsInCss(QString css) {
    css.replace(
        QRegularExpression(QStringLiteral("margin:\\s*0px\\s+0px\\s+(\\d+)px\\s+(\\d+)px\\s*;?")),
        QStringLiteral("margin-top:0;margin-bottom:\\1px;margin-left:\\2px;"));
    css.replace(
        QRegularExpression(QStringLiteral("margin:\\s*0px\\s+0px\\s+(\\d+)px\\s+0px\\s*;?")),
        QStringLiteral("margin-top:0;margin-bottom:\\1px;"));
    css.replace(
        QRegularExpression(QStringLiteral("margin:\\s*0px\\s+0px\\s+0px\\s+(\\d+)px\\s*;?")),
        QStringLiteral("margin-top:0;margin-left:\\1px;"));
    css.replace(
        QRegularExpression(QStringLiteral("margin:\\s*0px\\s+0px\\s+0px\\s+0px\\s*;?")),
        QStringLiteral("margin:0;"));
    css.replace(
        QRegularExpression(QStringLiteral("margin-top:\\s*\\d+px\\s*;?")),
        QStringLiteral("margin-top:0;"));
    css.replace(
        QRegularExpression(QStringLiteral("margin-bottom:\\s*11px\\s*;?")),
        QStringLiteral("margin-bottom:8px;"));
    return css;
}

QString simplifyHelpInlineStyle(QString declarations) {
    declarations = declarations.trimmed();
    if (declarations.isEmpty()) {
        return {};
    }

    static const QRegularExpression dropRule(
        QStringLiteral("(widows|orphans|text-justify|text-align-last)\\s*:[^;\"']*;?"),
        QRegularExpression::CaseInsensitiveOption);
    declarations.remove(dropRule);

    declarations = normalizeHelpMarginsInCss(declarations);
    declarations.replace(
        QRegularExpression(QStringLiteral("line-height:\\s*[^;\"']+\\s*;?")),
        QStringLiteral("line-height:100%;"));
    declarations.replace(
        QRegularExpression(QStringLiteral("(text-align|font-weight|font-style|color|text-decoration)\\s*:\\s*"),
                           QRegularExpression::CaseInsensitiveOption),
        QStringLiteral("\\1:"));

    declarations.replace(QStringLiteral("'Calibri Light'"), QStringLiteral("'DejaVu Sans','Liberation Sans',sans-serif"));
    declarations.replace(QStringLiteral("'Calibri'"), QStringLiteral("'DejaVu Sans','Liberation Sans',sans-serif"));
    declarations.replace(QStringLiteral("'Arial', 'Helvetica', sans-serif"), QStringLiteral("sans-serif"));

    while (declarations.contains(QStringLiteral(";;"))) {
        declarations.replace(QStringLiteral(";;"), QStringLiteral(";"));
    }
    return declarations.trimmed();
}

QHash<QString, QString> parseHelpCssClassRules(const QString &stylesheet) {
    QHash<QString, QString> rules;
    if (stylesheet.isEmpty()) {
        return rules;
    }

    QRegularExpression blockRe(
        QStringLiteral("([^{]+)\\{([^}]*)\\}"),
        QRegularExpression::CaseInsensitiveOption);
    auto it = blockRe.globalMatch(stylesheet);
    while (it.hasNext()) {
        const QRegularExpressionMatch match = it.next();
        const QStringList selectors = match.captured(1).split(QLatin1Char(','), Qt::SkipEmptyParts);
        const QString inlineStyle = simplifyHelpInlineStyle(match.captured(2));
        if (inlineStyle.isEmpty()) {
            continue;
        }
        for (QString selector : selectors) {
            selector = selector.trimmed();
            const QRegularExpression classRe(
                QStringLiteral("\\.(rv(?:ts|ps)\\d+)\\b"),
                QRegularExpression::CaseInsensitiveOption);
            const QRegularExpressionMatch classMatch = classRe.match(selector);
            if (!classMatch.hasMatch()) {
                continue;
            }
            const QString className = classMatch.captured(1);
            if (rules.contains(className)) {
                rules[className] = rules.value(className) + QLatin1Char(';') + inlineStyle;
            } else {
                rules.insert(className, inlineStyle);
            }
        }
    }
    return rules;
}

void appendStyleAttribute(QString *openTag, const QString &styleToAdd) {
    if (styleToAdd.isEmpty()) {
        return;
    }
    if (openTag->contains(QStringLiteral("style="), Qt::CaseInsensitive)) {
        static const QRegularExpression styleAttrRe(
            QStringLiteral(R"rx(style="([^"]*)")rx"),
            QRegularExpression::CaseInsensitiveOption);
        const QRegularExpressionMatch styleMatch = styleAttrRe.match(*openTag);
        if (styleMatch.hasMatch()) {
            QString merged = styleMatch.captured(1).trimmed();
            if (!merged.isEmpty() && !merged.endsWith(QLatin1Char(';'))) {
                merged += QLatin1Char(';');
            }
            merged += styleToAdd;
            openTag->replace(
                styleMatch.capturedStart(0),
                styleMatch.capturedLength(0),
                QStringLiteral("style=\"") + merged + QLatin1Char('"'));
        }
    } else {
        *openTag += QStringLiteral(" style=\"") + styleToAdd + QLatin1Char('"');
    }
}

void applyHelpClassStylesToHtml(QString &html, const QHash<QString, QString> &classRules) {
    if (classRules.isEmpty()) {
        return;
    }

    for (auto it = classRules.constBegin(); it != classRules.constEnd(); ++it) {
        const QString className = it.key();
        const QString inlineStyle = it.value();
        if (inlineStyle.isEmpty()) {
            continue;
        }

        const QRegularExpression tagRe(
            QString(
                R"re(<(?:(span|p|a|div|li|ul|ol|h[1-6]))([^>]*?\bclass=(?:%1|"%1"|'%1')\b)([^>]*?)(>))re")
                .arg(QRegularExpression::escape(className)),
            QRegularExpression::CaseInsensitiveOption);
        if (!tagRe.isValid()) {
            continue;
        }

        QString rebuilt;
        int offset = 0;
        auto tagIt = tagRe.globalMatch(html);
        while (tagIt.hasNext()) {
            const QRegularExpressionMatch match = tagIt.next();
            rebuilt += html.mid(offset, match.capturedStart(0) - offset);

            QString openTag = match.captured(1) + match.captured(2) + match.captured(3);
            appendStyleAttribute(&openTag, inlineStyle);

            if (match.captured(1).compare(QStringLiteral("p"), Qt::CaseInsensitive) == 0
                && inlineStyle.contains(QStringLiteral("text-align:center"), Qt::CaseInsensitive)
                && !openTag.contains(QStringLiteral("align="), Qt::CaseInsensitive)) {
                openTag += QStringLiteral(" align=\"center\"");
            }

            rebuilt += QLatin1Char('<') + openTag + match.captured(4);
            offset = match.capturedEnd(0);
        }
        if (offset > 0) {
            rebuilt += html.mid(offset);
            html = rebuilt;
        }
    }
}

void reinforceHelpRichTextTags(QString &html) {
    static const QRegularExpression styledInline(
        QStringLiteral(R"rx(<(span|a|p)([^>]*style="([^"]*)"[^>]*>([^<]+)</\1>)rx"),
        QRegularExpression::CaseInsensitiveOption);
    if (!styledInline.isValid()) {
        return;
    }

    auto styledIt = styledInline.globalMatch(html);
    if (!styledIt.hasNext()) {
        return;
    }

    int offset = 0;
    QString result;
    styledIt = styledInline.globalMatch(html);
    while (styledIt.hasNext()) {
        const QRegularExpressionMatch match = styledIt.next();
        result += html.mid(offset, match.capturedStart(0) - offset);

        const QString tag = match.captured(1);
        const QString attrs = match.captured(2);
        const QString style = match.captured(3);
        QString text = match.captured(4);

        static const QRegularExpression boldRe(
            QStringLiteral("font-weight\\s*:\\s*(?:bold|700)"),
            QRegularExpression::CaseInsensitiveOption);
        static const QRegularExpression italicRe(
            QStringLiteral("font-style\\s*:\\s*italic"),
            QRegularExpression::CaseInsensitiveOption);
        static const QRegularExpression underlineRe(
            QStringLiteral("text-decoration\\s*:\\s*underline"),
            QRegularExpression::CaseInsensitiveOption);
        static const QRegularExpression colorRe(
            QStringLiteral("color\\s*:\\s*(#[0-9a-fA-F]{3,6})"),
            QRegularExpression::CaseInsensitiveOption);

        if (boldRe.match(style).hasMatch()) {
            text = QStringLiteral("<b>") + text + QStringLiteral("</b>");
        }
        if (italicRe.match(style).hasMatch()) {
            text = QStringLiteral("<i>") + text + QStringLiteral("</i>");
        }
        if (underlineRe.match(style).hasMatch()) {
            text = QStringLiteral("<u>") + text + QStringLiteral("</u>");
        }
        const QRegularExpressionMatch colorMatch = colorRe.match(style);
        if (colorMatch.hasMatch()) {
            text = QStringLiteral("<font color=\"") + colorMatch.captured(1) + QStringLiteral("\">")
                + text + QStringLiteral("</font>");
        }

        result += QLatin1Char('<') + tag + attrs + QLatin1Char('>') + text + QStringLiteral("</") + tag
            + QLatin1Char('>');
        offset = match.capturedEnd(0);
    }
    result += html.mid(offset);
    html = result;
}

void compactHelpSpacingStyles(QString &html) {
    const int styleStart = html.indexOf(QStringLiteral("<style"), 0, Qt::CaseInsensitive);
    if (styleStart < 0) {
        return;
    }
    const int contentStart = html.indexOf(QLatin1Char('>'), styleStart);
    if (contentStart < 0) {
        return;
    }
    const int styleEnd = html.indexOf(QStringLiteral("</style>"), contentStart, Qt::CaseInsensitive);
    if (styleEnd < 0) {
        return;
    }

    QString css = html.mid(contentStart + 1, styleEnd - contentStart - 1);
    css.replace(
        QRegularExpression(QStringLiteral("line-height:\\s*[^;\"']+")),
        QStringLiteral("line-height:100%"));
    css = normalizeHelpMarginsInCss(css);
    css.replace(
        QStringLiteral("margin: 48px 48px 48px 48px"),
        QStringLiteral("margin: 8px"));

    html.replace(contentStart + 1, styleEnd - contentStart - 1, css);
}

QString buildHelpDefaultStylesheet(const QString &html) {
    QString css = extractHelpStylesheet(html);
    css.replace(QRegularExpression(QStringLiteral("<!--|-->")), QString());
    css.replace(
        QRegularExpression(
            QStringLiteral("(widows|orphans|text-justify|text-align-last)\\s*:[^;\"']*;?"),
            QRegularExpression::CaseInsensitiveOption),
        QString());
    css.replace(QStringLiteral("'Calibri Light'"), QStringLiteral("'DejaVu Sans','Liberation Sans',sans-serif"));
    css.replace(QStringLiteral("'Calibri'"), QStringLiteral("'DejaVu Sans','Liberation Sans',sans-serif"));
    css.replace(
        QRegularExpression(QStringLiteral("(span|a|p|div|li|body|table|ul|ol)\\.rv")),
        QStringLiteral(".rv"));
    css += QStringLiteral(
        "body { margin: 8px; line-height: 100%; }"
        "p, ul, ol { margin-top: 0; margin-bottom: 8px; line-height: 100%; }"
        "a { color: #0563c1; text-decoration: underline; }"
    );
    return css;
}

void compactHelpDocumentSpacing(QTextDocument *doc) {
    if (!doc || doc->isEmpty()) {
        return;
    }
    QTextCursor cursor(doc);
    cursor.beginEditBlock();
    for (QTextBlock block = doc->begin(); block.isValid(); block = block.next()) {
        QTextBlockFormat fmt = block.blockFormat();
        fmt.setLineHeight(100, QTextBlockFormat::ProportionalHeight);
        cursor.setPosition(block.position());
        cursor.mergeBlockFormat(fmt);
    }
    cursor.endEditBlock();
}

class HelpTextBrowser final : public QTextBrowser {
public:
    using LinkHandler = std::function<void(const QUrl &)>;

    explicit HelpTextBrowser(QWidget *parent = nullptr) : QTextBrowser(parent) {}

    void setLinkHandler(LinkHandler handler) {
        m_linkHandler = std::move(handler);
    }

protected:
    void setSource(const QUrl &name) override {
        if (m_linkHandler) {
            m_linkHandler(name);
            return;
        }
        QTextBrowser::setSource(name);
    }

private:
    LinkHandler m_linkHandler;
};

// Справка оригинала сохранена в windows-1251 (meta charset), остальные страницы — в UTF-8.
QString decodeHtmlBytes(const QByteArray &raw) {
    QTextCodec *utf8 = QTextCodec::codecForName("UTF-8");
    QTextCodec::ConverterState state;
    const QString text = utf8->toUnicode(raw.constData(), raw.size(), &state);
    if (state.invalidChars == 0) {
        return text;
    }
    QTextCodec *codec = QTextCodec::codecForHtml(raw, QTextCodec::codecForName("Windows-1251"));
    return codec->toUnicode(raw);
}

QString loadHelpHtmlFromFile(const QString &path, QString *rawSource = nullptr) {
    if (path.isEmpty()) {
        return {};
    }
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return {};
    }
    const QString source = decodeHtmlBytes(file.readAll());
    if (source.isEmpty()) {
        return {};
    }
    if (rawSource) {
        *rawSource = source;
    }
    return prepareHelpHtml(source);
}

class HelpWindowDragFilter final : public QObject {
public:
    explicit HelpWindowDragFilter(QDialog *dialog, int titleHeight, QObject *parent = nullptr)
        : QObject(parent), m_dialog(dialog), m_titleHeight(titleHeight) {
        dialog->installEventFilter(this);
    }

    bool eventFilter(QObject *watched, QEvent *event) override {
        if (watched != m_dialog || !m_dialog) {
            return QObject::eventFilter(watched, event);
        }

        switch (event->type()) {
        case QEvent::MouseButtonPress: {
            auto *mouseEvent = static_cast<QMouseEvent *>(event);
            if (mouseEvent->button() != Qt::LeftButton || mouseEvent->pos().y() >= m_titleHeight) {
                break;
            }
            QWidget *child = m_dialog->childAt(mouseEvent->pos());
            while (child && child != m_dialog) {
                if (qobject_cast<QAbstractButton *>(child)) {
                    return QObject::eventFilter(watched, event);
                }
                child = child->parentWidget();
            }
            m_dragging = true;
            m_dragOffset = mouseEvent->globalPos() - m_dialog->frameGeometry().topLeft();
            return true;
        }
        case QEvent::MouseMove:
            if (m_dragging) {
                auto *mouseEvent = static_cast<QMouseEvent *>(event);
                m_dialog->move(mouseEvent->globalPos() - m_dragOffset);
                return true;
            }
            break;
        case QEvent::MouseButtonRelease:
            m_dragging = false;
            break;
        default:
            break;
        }
        return QObject::eventFilter(watched, event);
    }

private:
    QDialog *m_dialog = nullptr;
    int m_titleHeight = 110;
    bool m_dragging = false;
    QPoint m_dragOffset;
};

QString loadAnamnesisTemplateFromFile(const QString &path) {
    if (path.isEmpty()) {
        return {};
    }
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return {};
    }
    const QByteArray raw = file.readAll();
    if (raw.isEmpty()) {
        return {};
    }

    const QString source = decodeHtmlBytes(raw);
    if (path.endsWith(QStringLiteral("anamnez_clean.html"), Qt::CaseInsensitive)) {
        return prepareAnamnesisHtml(source);
    }

#ifndef Q_OS_WIN
    if (raw.size() > 50000 || isWordExportHtml(source)) {
        return {};
    }
#endif

    return prepareAnamnesisHtml(source);
}

QString resolveHtmlAssetPath(const QString &name) {
    const QStringList roots = {
        QCoreApplication::applicationDirPath() + "/assets/htmls",
        QCoreApplication::applicationDirPath() + "/../assets/htmls",
        QCoreApplication::applicationDirPath() + "/../../assets/htmls",
        QCoreApplication::applicationDirPath() + "/../../../assets/htmls",
        QDir::currentPath() + "/assets/htmls",
        QDir::currentPath() + "/../old_project/serv9 2025/WindowsFormsApp1/bin/Debug/htmls"
    };
    for (const QString &root : roots) {
        const QString candidate = QDir(root).filePath(name);
        if (QFile::exists(candidate)) {
            return candidate;
        }
    }
    return {};
}

QString readAnamnesisTemplateHtml() {
    const QString preparedClean = loadAnamnesisTemplateFromFile(
        resolveHtmlAssetPath(QStringLiteral("anamnez_clean.html"))
    );
    if (!preparedClean.trimmed().isEmpty()) {
        return preparedClean;
    }

#ifdef Q_OS_WIN
    const QString preparedHtml = loadAnamnesisTemplateFromFile(resolveHtmlAssetPath(QStringLiteral("anamnez.html")));
    if (!preparedHtml.trimmed().isEmpty()) {
        return preparedHtml;
    }
#endif

    QFile rtfFile(resolveHtmlAssetPath(QStringLiteral("anamnez.rtf")));
    if (!rtfFile.open(QIODevice::ReadOnly)) {
        return {};
    }
    return rtfToHtml(rtfFile.readAll());
}

} // namespace

SunriseWindow::SunriseWindow(const QString &licenseKey, bool openAdminOnStart, QWidget *parent)
    : QMainWindow(parent), m_repository(&m_api, this), m_licenseKey(licenseKey) {
    buildUi();
    applyLegacyStyle();
    bindSignals();
    if (openAdminOnStart) {
        m_mainId.clear();
        m_userRole->setCurrentText("Администратор");
        setScreen(ScreenMode::Admin);
    } else {
        setScreen(ScreenMode::Enter);
    }
    m_root->setFocus();
}

void SunriseWindow::buildUi() {
    setWindowFlags(Qt::FramelessWindowHint | Qt::Window);
    resize(kDesignWidth, kDesignHeight);
    setMinimumSize(1024, 768);
    setWindowTitle(QStringLiteral("Санрайс"));
    if (!QApplication::windowIcon().isNull()) {
        setWindowIcon(QApplication::windowIcon());
    }

    m_root = new QWidget(this);
    setCentralWidget(m_root);

    m_bClose = new ImageButton(m_root);
    m_bClose->setGeometry(1878, 10, 36, 34);
    m_bLine = new ImageButton(m_root);
    m_bLine->setGeometry(1794, 10, 36, 34);
    m_bUp = new ImageButton(m_root);
    m_bUp->setGeometry(1836, 10, 36, 34);
    m_bBack = new ImageButton(m_root);
    m_bBack->setGeometry(8, 12, 36, 34);
    m_bList = new ImageButton(m_root);
    m_bList->setGeometry(50, 12, 166, 34);
    m_bExit = new ImageButton(m_root);
    m_bExit->setGeometry(1683, 10, 36, 34);
    m_bPicPrint = new ImageButton(m_root);
    m_bPicPrint->setGeometry(1440, 10, 36, 34);
    m_bUpload = new ImageButton(m_root);
    m_bUpload->setGeometry(1398, 10, 36, 34);
    m_bSave = new ImageButton(m_root);
    m_bSave->setGeometry(1490, 10, 36, 34);
    m_bPrint = new ImageButton(m_root);
    m_bPrint->setGeometry(1532, 10, 36, 34);
    m_bSettings = new ImageButton(m_root);
    m_bSettings->setGeometry(1574, 10, 36, 34);
    m_bInfo = new ImageButton(m_root);
    m_bInfo->setGeometry(1617, 10, 36, 34);
    m_bJournal = new ImageButton(m_root);
    m_bJournal->setGeometry(1600, 288, 264, 64);
    m_bJournal->hide();
    m_bUpdate = new ImageButton(m_root);
    m_bUpdate->setGeometry(1600, 380, 264, 64);
    m_bUpdate->hide();

    // Form1(): вкладки идут от x=945 с перекрытием в 1 px, y=64.
    {
        int tabX = 945;
        const auto makeTab = [this, &tabX](int width) {
            auto *tab = new ImageButton(m_root);
            tab->setGeometry(tabX, 64, width, 45);
            tab->setCursor(Qt::PointingHandCursor);
            tab->hide();
            tabX += width - 1;
            return tab;
        };
        m_pAna = makeTab(100);
        m_pRisk = makeTab(200);
        m_pCorr = makeTab(180);
        m_pUpr = makeTab(180);
        m_pDih = makeTab(130);
        m_pMp = makeTab(150);
    }

    m_logo1 = new QLabel(m_root);
    m_logo1->setAttribute(Qt::WA_TranslucentBackground, true);
    m_logo1->setStyleSheet("background: transparent;");
    m_logoTitle2 = new QLabel(m_root);
    m_logoTitle2->setAttribute(Qt::WA_TranslucentBackground, true);
    m_logoTitle2->setStyleSheet("background: transparent;");
    m_logo2 = new QLabel(m_root);
    m_logo2->setAttribute(Qt::WA_TranslucentBackground, true);
    m_logo2->setStyleSheet("background: transparent;");

    m_panelLogin = new QWidget(m_root);
    m_panelLogin->setGeometry((1920 - 500) / 2, (1080 - 176) / 2, 500, 176);
    m_panelLogin->setAttribute(Qt::WA_StyledBackground, true);
    m_panelLogin->setStyleSheet("background: transparent;");
    m_loginEdit = new QLineEdit(m_panelLogin);
    m_loginEdit->setGeometry(75, 22, 392, 29);
    m_passwordEdit = new QLineEdit(m_panelLogin);
    m_passwordEdit->setGeometry(75, 75, 257, 29);
    m_passwordEdit->setEchoMode(QLineEdit::Password);
    m_loginClear = new ImageButton(m_panelLogin);
    m_loginClear->setGeometry(440, 26, 24, 22);
    m_loginClear->setStyleSheet("background:white;");
    m_loginClear->hide();
    m_passwordClear = new ImageButton(m_panelLogin);
    m_passwordClear->setGeometry(305, 79, 23, 22);
    m_passwordClear->setStyleSheet("background:white;");
    m_passwordClear->hide();
    m_loginManIcon = new QLabel(m_panelLogin);
    m_loginManIcon->setGeometry(47, 23, 24, 26);
    m_loginManIcon->setScaledContents(true);
    m_loginKeyIcon = new QLabel(m_panelLogin);
    m_loginKeyIcon->setGeometry(47, 76, 24, 26);
    m_loginKeyIcon->setScaledContents(true);
    m_loginEye = new ImageButton(m_panelLogin);
    m_loginEye->setGeometry(342, 77, 35, 23);
    m_loginButton = new ImageButton(m_panelLogin);
    m_loginButton->setGeometry(387, 71, 80, 35);
    m_adminButton = new ImageButton(m_panelLogin);
    m_adminButton->setGeometry(125, 129, 247, 38);

    m_adminTitle = new QLabel(m_root);
    m_adminTitle->setAttribute(Qt::WA_TranslucentBackground, true);
    m_adminTitle->setGeometry((1920 - 526) / 2, 81, 526, 56);
    m_adminTitle->hide();

    m_panelAdmin = new QWidget(m_root);
    m_panelAdmin->setGeometry(80, 195, 1460, 423);
    m_panelAdmin->setAttribute(Qt::WA_StyledBackground, true);
    m_panelAdmin->setStyleSheet("background: transparent;");

    constexpr int kAdminTableW = 536;
    constexpr int kAdminFormW = 340;
    constexpr int kAdminFieldH = 37;
    constexpr int kAdminGap = 80;
    constexpr int kAdminBlockW = kAdminTableW + kAdminGap + kAdminFormW;
    constexpr int kAdminBlockX = (1760 - kAdminBlockW) / 2;
    constexpr int kAdminTableX = kAdminBlockX - 40;
    constexpr int kAdminFormX = kAdminBlockX + kAdminTableW + kAdminGap;
    constexpr int kAdminIconX = kAdminFormX - 36;
    constexpr int kAdminTitleY = 26;
    constexpr int kAdminFieldY[] = {92, 147, 202, 256};

    m_usersTable = new QTableWidget(m_panelAdmin);
    m_usersTable->setGeometry(kAdminTableX, kAdminTitleY + 5, kAdminTableW, 120);
    m_usersTable->setColumnCount(4);
    m_usersTable->setHorizontalHeaderLabels({"id", "ФИО", "Логин", "Уровень доступа"});
    m_usersTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_usersTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_usersTable->setFrameShape(QFrame::NoFrame);
    m_usersTable->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_usersTable->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_usersTable->hideColumn(0);
    m_usersTable->setItemDelegate(new AdminUsersItemDelegate(m_usersTable));

    m_adminLabel1 = new QLabel("Создать пользователя", m_panelAdmin);
    m_adminLabel1->setGeometry(kAdminFormX, kAdminTitleY, kAdminFormW, 24);
    m_adminLabel1->setAlignment(Qt::AlignHCenter | Qt::AlignTop);
    m_adminLabel2 = new QLabel("Уровень доступа", m_panelAdmin);
    m_adminLabel2->move(kAdminFormX + 2, 309);

    m_adminManIcon = new QLabel(m_panelAdmin);
    m_adminManIcon->setGeometry(kAdminIconX, kAdminFieldY[0] + 2, 32, 33);
    m_adminManIcon->setScaledContents(true);
    m_adminLoginIcon = new QLabel(m_panelAdmin);
    m_adminLoginIcon->setGeometry(kAdminIconX, kAdminFieldY[1] + 2, 32, 33);
    m_adminLoginIcon->setScaledContents(true);
    m_adminKeyIcon1 = new QLabel(m_panelAdmin);
    m_adminKeyIcon1->setGeometry(kAdminIconX, kAdminFieldY[2] + 2, 32, 33);
    m_adminKeyIcon1->setScaledContents(true);
    m_adminKeyIcon2 = new QLabel(m_panelAdmin);
    m_adminKeyIcon2->setGeometry(kAdminIconX, kAdminFieldY[3] + 2, 32, 33);
    m_adminKeyIcon2->setScaledContents(true);

    auto makeAdminField = [this](QWidget *&panel, QLineEdit *&edit, ImageButton *&clear, int y) {
        panel = new QWidget(m_panelAdmin);
        panel->setGeometry(kAdminFormX, y, kAdminFormW, kAdminFieldH);
        panel->setAttribute(Qt::WA_StyledBackground, true);
        edit = new QLineEdit(panel);
        edit->setGeometry(8, 7, kAdminFormW - 42, 22);
        edit->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        clear = new ImageButton(panel);
        clear->setGeometry(kAdminFormW - 30, 7, 23, 22);
        clear->hide();
    };
    makeAdminField(m_userFioPanel, m_userFio, m_userFioClear, kAdminFieldY[0]);
    makeAdminField(m_userLoginPanel, m_userLogin, m_userLoginClear, kAdminFieldY[1]);
    makeAdminField(m_userPassPanel, m_userPass, m_userPassClear, kAdminFieldY[2]);
    makeAdminField(m_userPass2Panel, m_userPass2, m_userPass2Clear, kAdminFieldY[3]);
    m_userPass->setEchoMode(QLineEdit::Password);
    m_userPass2->setEchoMode(QLineEdit::Password);
    m_adminEye1 = new ImageButton(m_panelAdmin);
    m_adminEye1->setGeometry(kAdminFormX + kAdminFormW + 4, kAdminFieldY[2] + 5, 35, 23);
    m_adminEye2 = new ImageButton(m_panelAdmin);
    m_adminEye2->setGeometry(kAdminFormX + kAdminFormW + 4, kAdminFieldY[3] + 7, 35, 23);

    m_userRole = new QComboBox(m_panelAdmin);
    m_userRole->setGeometry(kAdminFormX + kAdminFormW - 153, 308, 153, 28);
    m_userRole->addItems({"Специалист", "Администратор"});

    m_userSaveButton = new ImageButton(m_panelAdmin);
    m_userSaveButton->setGeometry(kAdminFormX + (kAdminFormW - 101) / 2, 354, 101, 30);
    m_userOpenPatients = new ImageButton(m_panelAdmin);
    m_userOpenPatients->setGeometry(kAdminFormX + kAdminFormW - 80, 350, 80, 36);

    m_panelPatients = new QWidget(m_root);
    m_panelPatients->setGeometry(1920 / 2 - 879 / 2, 80, 879, 950);
    m_panelPatients->setAttribute(Qt::WA_StyledBackground, true);
    m_panelPatients->setStyleSheet("background: transparent;");
    m_patientSearch = new QLineEdit(m_panelPatients);
    m_patientSearch->setGeometry(14, 21, 318, 29);
    m_patientSearchClear = new ImageButton(m_panelPatients);
    m_patientSearchClear->hide();
    m_addPatient = new ImageButton(m_panelPatients);
    m_addPatient->setGeometry(334, 21, 143, 29);
    m_dateFilter = new QCheckBox("Показать пациентов, добавленных", m_panelPatients);
    m_dateFilter->move(609, 64);
    m_dateFrom = new QDateEdit(QDate::currentDate(), m_panelPatients);
    m_dateFrom->setGeometry(647, 89, 199, 20);
    m_dateTo = new QDateEdit(QDate::currentDate(), m_panelPatients);
    m_dateTo->setGeometry(647, 115, 199, 20);
    m_dateFrom->setCalendarPopup(true);
    m_dateTo->setCalendarPopup(true);
    m_dateFrom->setLocale(QLocale(QLocale::Russian, QLocale::Russia));
    m_dateTo->setLocale(QLocale(QLocale::Russian, QLocale::Russia));
    m_dateFrom->setDisplayFormat(QStringLiteral("d  MMMM  yyyy 'г.'"));
    m_dateTo->setDisplayFormat(QStringLiteral("d  MMMM  yyyy 'г.'"));
    m_labelFrom = new QLabel("С", m_panelPatients);
    m_labelFrom->move(622, 95);
    m_labelTo = new QLabel("До", m_panelPatients);
    m_labelTo->move(622, 121);
    m_patientsTable = new QTableWidget(m_panelPatients);
    m_patientsTable->setGeometry(14, 63, 589, 800);
    m_patientsTable->setColumnCount(5);
    m_patientsTable->setHorizontalHeaderLabels({"id", "ФИО", "День рождения", "Дата обращения", "Уд."});
    {
        LegacyGridHeader *patientsHeader = setupLegacyGrid(m_patientsTable, legacyGridFont());
        patientsHeader->setSortableSections({1, 2, 3});
        patientsHeader->onSectionClicked = [this](int section) { handlePatientsHeaderClick(section); };
    }
    m_patientsTable->hideColumn(0);
    m_patientsTable->setFocusPolicy(Qt::StrongFocus);

    m_summaryPanel = new SummaryPanel(m_root);
    m_summaryPanel->setGeometry(10, 65, 920, 1005);
    m_summaryPanel->hide();

    m_workStack = new QStackedWidget(m_root);
    m_workStack->setObjectName(QStringLiteral("workStack"));
    m_workStack->setGeometry(945, 105, 965, 965);
    m_workStack->setAutoFillBackground(true);
    m_workStack->setAttribute(Qt::WA_StyledBackground, true);
    m_workStack->setAttribute(Qt::WA_OpaquePaintEvent, true);
    m_workStack->setStyleSheet(QStringLiteral(
        "QStackedWidget#workStack { background-color: #ffffff; background-image: none; }"
    ));

    m_panelClinical = new WorkPanelWidget(m_workStack, QColor(0xf0, 0xf0, 0xf0));
    m_panelClinical->setGeometry(0, 0, 966, 968);
    m_clinical = new ClinicalBrowserController(m_panelClinical, this);

    m_panelWork = new WorkPanelWidget(m_workStack);
    m_panelWork->setGeometry(0, 0, 965, 965);
    m_underlineButton = new ImageButton(m_panelWork);
    m_underlineButton->setGeometry(38, 15, 36, 34);
    m_boldButton = new ImageButton(m_panelWork);
    m_boldButton->setGeometry(80, 15, 36, 34);
    m_anamnesisLabel = new QLabel(QStringLiteral("Анамнез"), m_panelWork);
    {
        QFont labelFont(QStringLiteral("Microsoft Sans Serif"));
        labelFont.setPointSizeF(15);
        labelFont.setBold(true);
        m_anamnesisLabel->setFont(labelFont);
    }
    m_anamnesisLabel->setStyleSheet(QStringLiteral("background: transparent; color: #000000;"));
    m_anamnesisLabel->move(442, 18);
    m_anamnesisLabel->adjustSize();
    m_patientTitle = new GrayTitleLabel(m_root);
    m_patientTitle->setGeometry(287, 34, 237, 34);
    m_anamnesisEdit = new QTextEdit(m_panelWork);
    m_anamnesisEdit->setGeometry(29, 55, 900, 950);

    m_workStack->addWidget(m_panelWork);
    m_workStack->addWidget(m_panelClinical);
    m_workStack->setCurrentWidget(m_panelWork);
    m_workStack->hide();

    setupClinicalController();

    buildSlidePanels();

    const QWidgetList chromeWidgets = {
        m_pAna, m_pRisk, m_pCorr, m_pUpr, m_pDih, m_pMp, m_workStack,
        m_bBack, m_bList, m_bExit, m_bSave, m_bPrint, m_bSettings, m_bInfo, m_bJournal, m_bUpdate,
        m_bClose, m_bLine, m_bUp, m_patientTitle, m_userOpenPatients, m_summaryPanel
    };
    for (QWidget *widget : chromeWidgets) {
        if (widget) {
            widget->raise();
        }
    }
}

void SunriseWindow::buildSlidePanels() {
    const auto panelStyle = [this]() {
        const QString path = imagePath("popup3.png");
        if (!path.isEmpty()) {
            QString normalizedPath = path;
            normalizedPath.replace('\\', '/');
            return QStringLiteral(
                "QWidget#settingsPanel { background-image: url('%1'); background-repeat: no-repeat; }"
            ).arg(normalizedPath);
        }
        return QStringLiteral(
            "QWidget#settingsPanel { background-color: #ece9e9; border: 1px solid #808080; border-radius: 6px; }"
        );
    };
    const QString transparentPanelStyle = QStringLiteral("background: transparent;");
    const QString panelCheckBoxStyle = panelCheckBoxStyleSheet();
    const QString boldTitleStyle = QStringLiteral(
        "QLabel { background: transparent; color: #000000; font-family: 'Microsoft Sans Serif'; font-size: 8.25pt; font-weight: bold; }"
    );
    const QString plainLabelStyle = QStringLiteral(
        "QLabel { background: transparent; color: #000000; font-family: 'Microsoft Sans Serif'; font-size: 8.25pt; }"
    );

    m_settingsPanel = new QWidget(m_root);
    // Рабочий стек становится нативным окном из-за IE внутри; без своего HWND панель
    // оказывается под ним, и клики по полям шаблона уходят в анамнез.
    m_settingsPanel->setAttribute(Qt::WA_NativeWindow);
    m_settingsPanel->setObjectName(QStringLiteral("settingsPanel"));
    m_settingsPanel->setGeometry(1450, -200, 378, 190);
    m_settingsPanel->setStyleSheet(panelStyle());
    m_settingsPanel->hide();

    m_settingsMainView = new QWidget(m_settingsPanel);
    m_settingsMainView->setGeometry(0, 0, 378, 190);
    m_settingsMainView->setStyleSheet(transparentPanelStyle);
    m_settingsSaveView = new QWidget(m_settingsPanel);
    m_settingsSaveView->setGeometry(0, 0, 378, 190);
    m_settingsSaveView->setStyleSheet(transparentPanelStyle);
    m_settingsSaveView->hide();

    m_fontDownButton = new ImageButton(m_settingsMainView);
    m_fontDownButton->setGeometry(62, 13, 27, 23);
    setImage(m_fontDownButton, "fdown.png");
    m_fontUpButton = new ImageButton(m_settingsMainView);
    m_fontUpButton->setGeometry(275, 12, 27, 23);
    setImage(m_fontUpButton, "fup.png");
    m_fontSlider = new QSlider(Qt::Horizontal, m_settingsMainView);
    m_fontSlider->setGeometry(104, 12, 165, 30);
    m_fontSlider->setRange(21, 27);
    m_fontSlider->setValue(24);
    m_fontSlider->setStyleSheet(
        "QSlider::groove:horizontal { background: #ffffff; height: 8px; border: 1px solid #808080; }"
        "QSlider::handle:horizontal { background: #d4d0c8; width: 12px; margin: -4px 0; border: 1px solid #808080; }"
    );
    m_fontSizeLabel = new QLabel("24", m_settingsMainView);
    m_fontSizeLabel->setGeometry(178, 44, 30, 16);
    m_fontSizeLabel->setAlignment(Qt::AlignCenter);
    m_fontSizeLabel->setStyleSheet(
        "QLabel { background-color: #ffffff; color: #000000; font-family: 'Microsoft Sans Serif'; font-size: 8.25pt; }"
    );

    auto *templateLabel = new QLabel(QStringLiteral("Использовать как шаблон"), m_settingsMainView);
    templateLabel->setGeometry(104, 74, 180, 16);
    templateLabel->setStyleSheet(plainLabelStyle);
    m_templates = new QComboBox(m_settingsMainView);
    m_templates->setGeometry(82, 90, 197, 28);
    m_templates->setEditable(true);
    m_templates->setInsertPolicy(QComboBox::NoInsert);
    m_templateDeleteButton = new ImageButton(m_settingsMainView);
    m_templateDeleteButton->setGeometry(284, 91, 26, 26);
    m_templateDeleteButton->setImagePath(resourcePath("close.png"));
    m_templateSaveAsButton = new ImageButton(m_settingsMainView);
    m_templateSaveAsButton->setGeometry(119, 124, 127, 31);
    setImage(m_templateSaveAsButton, "save.png");

    auto *saveTemplateTitle = new QLabel(QStringLiteral("Сохранить как шаблон"), m_settingsSaveView);
    saveTemplateTitle->setGeometry(95, 20, 200, 20);
    saveTemplateTitle->setStyleSheet(plainLabelStyle);
    m_templateNameEdit = new QLineEdit(m_settingsSaveView);
    m_templateNameEdit->setGeometry(82, 55, 220, 28);
    m_templateNameEdit->setPlaceholderText(QStringLiteral("Название шаблона"));
    m_templateNameEdit->setStyleSheet(
        "QLineEdit { background: #ffffff; color: #000000; font-family: 'Microsoft Sans Serif'; font-size: 10pt; border: 1px solid #808080; padding: 2px 6px; }"
    );
    auto *saveTemplateBtn = new ImageButton(m_settingsSaveView);
    saveTemplateBtn->setGeometry(119, 100, 101, 30);
    setImage(saveTemplateBtn, "save.png");
    auto *cancelTemplateBtn = new ImageButton(m_settingsSaveView);
    cancelTemplateBtn->setGeometry(119, 140, 101, 30);
    setImage(cancelTemplateBtn, "cancel.png");

    // popupprint: 430x430, фон oPrn.png; один экземпляр для печати (makeLoad) и сохранения (makeSave).
    m_printPanel = new QWidget(m_root);
    // Выезжает поверх окна IE рабочей области — нужен собственный HWND.
    m_printPanel->setAttribute(Qt::WA_NativeWindow);
    m_printPanel->setObjectName(QStringLiteral("printPanel"));
    m_printPanel->setGeometry(1450, -450, 430, 430);
    {
        QString path = imagePath("oPrn.png");
        path.replace('\\', '/');
        m_printPanel->setStyleSheet(
            QStringLiteral("QWidget#printPanel { background-image: url('%1'); background-repeat: no-repeat; }").arg(path));
    }
    m_printPanel->hide();

    m_printPanelTitle = new QLabel(QStringLiteral("Отправить на печать"), m_printPanel);
    m_printPanelTitle->setStyleSheet(boldTitleStyle);
    m_printPanelTitle->move(162, 9);
    m_printPanelTitle->adjustSize();

    m_printProgramRb = new QRadioButton(
        QStringLiteral("Програм. индив. вторичн. профилакт. сосудист. события"), m_printPanel);
    m_printProgramRb->setAutoExclusive(false);
    m_printProgramRb->setStyleSheet(panelCheckBoxStyle);
    m_printProgramRb->move(34, 35);
    m_printProgramRb->adjustSize();

    const QStringList sectionTitles = {
        QStringLiteral("Оценка индивидуального риска повтор. сосудистого события (клинические шкалы)"),
        QStringLiteral("Коррекция факторов риска повтор. сосудистого события"),
        QStringLiteral("Программа восстановл. речевого мышления"),
        QStringLiteral("Дыхательная артикуляционная гимнастика"),
        QStringLiteral("Медикаментозная терапия"),
        QStringLiteral("Дополнительные данные")
    };
    const int sectionTops[] = {58, 93, 125, 158, 192, 224};
    for (int i = 0; i < sectionTitles.size(); ++i) {
        auto *check = new QCheckBox(m_printPanel);
        check->setStyleSheet(panelCheckBoxStyle);
        check->setChecked(true);
        check->setGeometry(57, sectionTops[i], 18, 43);
        auto *text = new ClickableLabel(m_printPanel);
        text->setText(sectionTitles.at(i));
        text->setWordWrap(true);
        text->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        text->setStyleSheet(plainLabelStyle + QStringLiteral("QLabel:disabled { color: rgba(0, 0, 0, 128); }"));
        text->setGeometry(57 + 18, sectionTops[i], 350 - 18, 43);
        text->onClick = [check]() {
            if (check->isEnabled()) {
                check->toggle();
            }
        };
        check->setProperty("sectionLabel", QVariant::fromValue<QObject *>(text));
        m_printSectionChecks.append(check);
    }

    auto *printKindPanel = new QWidget(m_printPanel);
    printKindPanel->setGeometry(34, 260, 102, 53);
    printKindPanel->setStyleSheet(transparentPanelStyle);
    m_printAnamnesisRb = new QRadioButton(QStringLiteral("Анамнез"), printKindPanel);
    m_printAnamnesisRb->setAutoExclusive(false);
    m_printAnamnesisRb->setStyleSheet(panelCheckBoxStyle);
    m_printAnamnesisRb->move(10, 10);
    m_printAnamnesisRb->adjustSize();
    m_printProtocolsRb = new QRadioButton(QStringLiteral("Протоколы"), printKindPanel);
    m_printProtocolsRb->setAutoExclusive(false);
    m_printProtocolsRb->setStyleSheet(panelCheckBoxStyle);
    m_printProtocolsRb->move(10, 33);
    m_printProtocolsRb->adjustSize();

    auto *audiencePanel = new QWidget(m_printPanel);
    audiencePanel->setGeometry(48, 319, 123, 60);
    audiencePanel->setStyleSheet(transparentPanelStyle);
    m_printForPatientRb = new QRadioButton(QStringLiteral("Для пациента"), audiencePanel);
    m_printForPatientRb->setStyleSheet(panelCheckBoxStyle);
    m_printForPatientRb->move(10, 10);
    m_printForPatientRb->adjustSize();
    m_printForSpecialistRb = new QRadioButton(QStringLiteral("Для специалиста"), audiencePanel);
    m_printForSpecialistRb->setStyleSheet(panelCheckBoxStyle);
    m_printForSpecialistRb->move(10, 33);
    m_printForSpecialistRb->adjustSize();

    m_printPanelPrintButton = new ImageButton(m_printPanel);
    {
        const QPixmap pixmap(resourcePath("Печать (текст).png"));
        m_printPanelPrintButton->setPixmap(pixmap);
        m_printPanelPrintButton->setGeometry(141, 385, pixmap.isNull() ? 81 : pixmap.width(),
                                             pixmap.isNull() ? 34 : pixmap.height());
    }
    m_printPanelSaveButton = new ImageButton(m_printPanel);
    {
        const QPixmap pixmap(resourcePath("Сохранить как.png"));
        m_printPanelSaveButton->setPixmap(pixmap);
        m_printPanelSaveButton->setGeometry(251, 385, pixmap.isNull() ? 144 : pixmap.width(),
                                            pixmap.isNull() ? 34 : pixmap.height());
    }
    for (QRadioButton *radio : {m_printProgramRb, m_printAnamnesisRb, m_printProtocolsRb}) {
        connect(radio, &QRadioButton::toggled, this, [this, radio]() { updatePrintPanelState(radio); });
    }

    connect(m_fontDownButton, &ImageButton::clicked, this, [this]() {
        if (m_fontSlider->value() > m_fontSlider->minimum()) {
            m_fontSlider->setValue(m_fontSlider->value() - 1);
        }
    });
    connect(m_fontUpButton, &ImageButton::clicked, this, [this]() {
        if (m_fontSlider->value() < m_fontSlider->maximum()) {
            m_fontSlider->setValue(m_fontSlider->value() + 1);
        }
    });
    connect(m_fontSlider, &QSlider::valueChanged, this, [this](int value) {
        m_fontSizeLabel->setText(QString::number(value));
        changeDocumentFontSize(value);
    });
    connect(m_templateDeleteButton, &ImageButton::clicked, this, [this]() {
        const QString name = m_templates->currentText();
        if (name == QStringLiteral("Стандартный")) {
            return;
        }
        if (!CustomMessageBox::askConfirm(this, "Внимание! Вы собираетесь удалить данные\nЭто действие необратимо!")) {
            return;
        }
        QString err;
        if (!m_repository.deleteTemplate(name, &err)) {
            CustomMessageBox::showError(this, err);
            return;
        }
        refreshTemplateNames();
        m_templates->setCurrentText(QStringLiteral("Стандартный"));
        writeProfileConfig(QStringLiteral("Стандартный"), m_fontSlider->value());
        loadDefaultAnamnesisTemplate();
    });
    connect(m_templateSaveAsButton, &ImageButton::clicked, this, [this]() { saveCurrentAnamnesisTemplate(); });
    connect(saveTemplateBtn, &ImageButton::clicked, this, [this]() {
        const QString name = m_templateNameEdit->text().trimmed();
        if (name.isEmpty()) {
            CustomMessageBox::showWarning(this, "Введите название шаблона.");
            return;
        }
        {
            QSignalBlocker blocker(m_templates);
            m_templates->setEditText(name);
        }
        saveCurrentAnamnesisTemplate();
        showSettingsSaveTemplateView(false);
    });
    connect(cancelTemplateBtn, &ImageButton::clicked, this, [this]() { showSettingsSaveTemplateView(false); });
    connect(m_printPanelSaveButton, &ImageButton::clicked, this, [this]() { exportDocument(); });
    connect(m_printPanelPrintButton, &ImageButton::clicked, this, [this]() { printSelectedContent(); });

    m_root->installEventFilter(this);
}

void SunriseWindow::applyLegacyStyle() {
    m_root->setObjectName(QStringLiteral("rootPanel"));
    // Только на сам root — иначе fone.jpg наследуется всеми дочерними виджетами (кнопки/радио/поля).
    m_root->setStyleSheet(
        QStringLiteral("QWidget#rootPanel { background-image: url('%1'); background-repeat: no-repeat; }")
            .arg(imagePath("fone.jpg")));
    QFont formFont("Microsoft Sans Serif", 10);
    setFont(formFont);

    auto setFromCandidates = [](ImageButton *button, const QStringList &candidates) {
        for (const QString &file : candidates) {
            if (!file.isEmpty()) {
                button->setImagePath(file);
                return;
            }
        }
    };

    // Form1.Designer: картинки верхнего бара из Properties.Resources, bUp/bplist/bexit — sysImages.
    setFromCandidates(m_bClose, {resourcePath("Закрыть.png")});
    setFromCandidates(m_bLine, {resourcePath("Свернуть.png")});
    setImage(m_bUp, "up.png");
    setFromCandidates(m_bBack, {resourcePath("Назад.png")});
    setImage(m_bList, "plist.png");
    setImage(m_bExit, "exit.png");
    setFromCandidates(m_bPicPrint, {resourcePath("Печать (1).png")});
    setFromCandidates(m_bUpload, {resourcePath("Загрузить файл.png")});
    setFromCandidates(m_bSave, {resourcePath("Сохранить.png")});
    setFromCandidates(m_bPrint, {resourcePath("Печать.png")});
    setFromCandidates(m_bSettings, {resourcePath("Настройки.png")});
    setFromCandidates(m_bInfo, {resourcePath("Информация.png")});
    setImage(m_bJournal, "journal.png");
    setImage(m_bUpdate, "updates.png");
    if (m_adminTitle) {
        const QPixmap title(imagePath("zadmin.png"));
        m_adminTitle->setPixmap(title);
        if (!title.isNull()) {
            m_adminTitle->setGeometry((1920 - title.width()) / 2, 81, title.width(), title.height());
        }
    }
    if (m_summaryPanel) {
        m_summaryPanel->setMarkerImage(resourcePath("Маркер.png"));
    }
    setImage(m_loginButton, "enter.png");
    setImage(m_adminButton, "admin.png");
    if (m_loginManIcon) {
        m_loginManIcon->setPixmap(QPixmap(imagePath("man.png")));
    }
    if (m_loginKeyIcon) {
        m_loginKeyIcon->setPixmap(QPixmap(imagePath("key.png")));
    }
    setImage(m_loginEye, "pon.png");
    setImage(m_adminEye1, "pon.png");
    setImage(m_adminEye2, "pon.png");
    setImage(m_userSaveButton, "save.png");
    setImage(m_userOpenPatients, "enter.png");
    if (m_adminManIcon) {
        m_adminManIcon->setPixmap(QPixmap(imagePath("man.png")));
    }
    if (m_adminLoginIcon) {
        m_adminLoginIcon->setPixmap(QPixmap(imagePath("Логин.png")));
    }
    if (m_adminKeyIcon1) {
        m_adminKeyIcon1->setPixmap(QPixmap(imagePath("key.png")));
    }
    if (m_adminKeyIcon2) {
        m_adminKeyIcon2->setPixmap(QPixmap(imagePath("key.png")));
    }
    setImage(m_addPatient, "addp.png");
    setFromCandidates(m_underlineButton, {resourcePath("Подчеркивание.png")});
    setFromCandidates(m_boldButton, {resourcePath("Жирный шрифт.png")});
    updatePatientTabIcons();

    applyEnterLogos();

    m_loginEdit->setPlaceholderText("Логин");
    m_passwordEdit->setPlaceholderText("Пароль");
    m_userFio->setPlaceholderText("ФИО");
    m_userLogin->setPlaceholderText("Логин");
    m_userPass->setPlaceholderText("Придумайте пароль");
    m_userPass2->setPlaceholderText("Подтвердите пароль");
    m_patientSearch->setPlaceholderText("Поиск");

    styleInputField(m_loginEdit);
    styleInputField(m_passwordEdit);
    styleAdminScreen();
    stylePatientsScreen();
    styleAnamnesisScreen();
    styleTemplateComboBox();
    installToolbarTooltips();

    const QString crossImage = resourcePath("крестик.png");
    if (!crossImage.isEmpty()) {
        m_loginClear->setImagePath(crossImage);
        m_passwordClear->setImagePath(crossImage);
        m_userFioClear->setImagePath(crossImage);
        m_userLoginClear->setImagePath(crossImage);
        m_userPassClear->setImagePath(crossImage);
        m_userPass2Clear->setImagePath(crossImage);
        m_patientSearchClear->setImagePath(crossImage);
    }
    m_loginClear->raise();
    m_passwordClear->raise();
    m_userFioClear->raise();
    m_userLoginClear->raise();
    m_userPassClear->raise();
    m_userPass2Clear->raise();
    m_adminEye1->raise();
    m_adminEye2->raise();
    m_adminManIcon->raise();
    m_adminLoginIcon->raise();
    m_adminKeyIcon1->raise();
    m_adminKeyIcon2->raise();
    m_adminTitle->setStyleSheet(QStringLiteral("background: transparent;"));
    m_patientTitle->setAlignment(Qt::AlignCenter);
}

void SunriseWindow::bindSignals() {
    connect(qApp, &QCoreApplication::aboutToQuit, this, [this]() { endUserSession(); });
    connect(m_bClose, &ImageButton::clicked, this, &SunriseWindow::close);
    connect(m_bLine, &ImageButton::clicked, this, [this]() {
        if (!m_isCustomMaximized) {
            m_savedWindowGeometry = geometry();
        }
        showMinimized();
    });
    connect(m_bUp, &ImageButton::clicked, this, [this]() { toggleWindowMaximize(); });
    connect(m_bExit, &ImageButton::clicked, this, [this]() {
        if (m_currentScreen == ScreenMode::Anamnesis) {
            tryAutoSaveAnamnesis();
            tryAutoSaveProgram(true);
        }
        endUserSession();
        m_mainId.clear();
        m_loginEdit->clear();
        m_passwordEdit->clear();
        m_loginClear->hide();
        m_passwordClear->hide();
        setScreen(ScreenMode::Enter);
    });
    connect(m_bBack, &ImageButton::clicked, this, [this]() { navigateBack(); });
    connect(m_bList, &ImageButton::clicked, this, [this]() { setScreen(ScreenMode::Patients); });
    connect(m_pAna, &ImageButton::clicked, this, [this]() { setScreen(ScreenMode::Anamnesis); });
    connect(m_pRisk, &ImageButton::clicked, this, [this]() { setScreen(ScreenMode::RiskAssessment); });
    connect(m_pCorr, &ImageButton::clicked, this, [this]() { setScreen(ScreenMode::Correction); });
    connect(m_pUpr, &ImageButton::clicked, this, [this]() { setScreen(ScreenMode::Speech); });
    connect(m_pDih, &ImageButton::clicked, this, [this]() { setScreen(ScreenMode::Breathing); });
    connect(m_pMp, &ImageButton::clicked, this, [this]() { setScreen(ScreenMode::Medication); });
    connect(m_bInfo, &ImageButton::clicked, this, [this]() { showInfoPopup(); });
    connect(m_bJournal, &ImageButton::clicked, this, [this]() { openJournal(); });
    connect(m_bUpdate, &ImageButton::clicked, this, [this]() { checkForUpdates(); });
    if (m_summaryPanel) {
        connect(m_summaryPanel, &SummaryPanel::programChanged, this, [this]() { tryAutoSaveProgram(); });
    }
    m_programSaveTimer = new QTimer(this);
    m_programSaveTimer->setSingleShot(true);
    m_programSaveTimer->setInterval(1200);
    connect(m_programSaveTimer, &QTimer::timeout, this, [this]() { tryAutoSaveProgram(true); });

    bindClearableField(m_loginEdit, m_loginClear);
    bindClearableField(m_passwordEdit, m_passwordClear);
    bindClearableField(m_userFio, m_userFioClear);
    bindClearableField(m_userLogin, m_userLoginClear);
    bindClearableField(m_userPass, m_userPassClear);
    bindClearableField(m_userPass2, m_userPass2Clear);

    connect(m_loginButton, &ImageButton::clicked, this, [this]() {
        ++m_loginAttempts;
        const auto user = m_repository.login(m_loginEdit->text().trimmed(), m_passwordEdit->text());
        if (!user.has_value()) {
            CustomMessageBox::showError(this, "Неверный логин или пароль!");
            return;
        }
        if (user->role != "Администратор" && user->role != "Специалист") {
            CustomMessageBox::showError(this, "Недостаточный уровень доступа.");
            return;
        }
        startUserSession(*user);
        m_mainId = user->id;
        rememberManagedUser(user->id, user->login, m_passwordEdit->text());
        refreshPatients();
        setScreen(ScreenMode::Patients);
    });
    connect(m_loginEye, &ImageButton::clicked, this, [this]() {
        if (m_passwordEdit->echoMode() == QLineEdit::Password) {
            m_passwordEdit->setEchoMode(QLineEdit::Normal);
            setImage(m_loginEye, "poff.png");
        } else {
            m_passwordEdit->setEchoMode(QLineEdit::Password);
            setImage(m_loginEye, "pon.png");
        }
    });

    connect(m_adminButton, &ImageButton::clicked, this, [this]() {
        ++m_loginAttempts;
        const auto user = m_repository.login(m_loginEdit->text().trimmed(), m_passwordEdit->text());
        if (!user.has_value()) {
            CustomMessageBox::showError(this, "Неверный логин или пароль!");
            return;
        }
        if (user->role != "Администратор") {
            CustomMessageBox::showError(this, "Ваш уровень доступа не позволяет осуществлять администрирование!");
            return;
        }
        startUserSession(*user);
        m_mainId = user->mainId.isEmpty() ? user->fio : user->mainId;
        rememberManagedUser(user->id, user->login, m_passwordEdit->text());
        refreshUsers();
        setScreen(ScreenMode::Admin);
    });

    connect(m_userSaveButton, &ImageButton::clicked, this, [this]() { saveUser(); });

    connect(m_userOpenPatients, &ImageButton::clicked, this, [this]() { enterAsManagedUser(); });
    connect(m_adminEye1, &ImageButton::clicked, this, [this]() {
        if (m_userPass->echoMode() == QLineEdit::Password) {
            m_userPass->setEchoMode(QLineEdit::Normal);
            setImage(m_adminEye1, "poff.png");
        } else {
            m_userPass->setEchoMode(QLineEdit::Password);
            setImage(m_adminEye1, "pon.png");
        }
    });
    connect(m_adminEye2, &ImageButton::clicked, this, [this]() {
        if (m_userPass2->echoMode() == QLineEdit::Password) {
            m_userPass2->setEchoMode(QLineEdit::Normal);
            setImage(m_adminEye2, "poff.png");
        } else {
            m_userPass2->setEchoMode(QLineEdit::Password);
            setImage(m_adminEye2, "pon.png");
        }
    });

    connect(m_patientSearch, &QLineEdit::textChanged, this, [this]() { refreshPatients(); });
    connect(m_dateFilter, &QCheckBox::stateChanged, this, [this]() { refreshPatients(); });
    connect(m_dateFrom, &QDateEdit::dateChanged, this, [this]() { refreshPatients(); });
    connect(m_dateTo, &QDateEdit::dateChanged, this, [this]() { refreshPatients(); });

    connect(m_addPatient, &ImageButton::clicked, this, [this]() {
        m_currentPatientId.clear();
        m_patientTitle->setText("Новая карта");
        loadDefaultAnamnesisTemplate();
        if (m_summaryPanel) {
            m_summaryPanel->resetDefaults();
            m_summaryPanel->setPatientHeader(QStringLiteral("Новая карта"), {}, {});
        }
        setScreen(ScreenMode::Anamnesis);
    });

    connect(m_patientsTable, &QTableWidget::cellClicked, this, &SunriseWindow::handlePatientsTableClick);
    connect(m_patientsTable, &QTableWidget::cellDoubleClicked, this, [this](int row, int column) {
        if (column == 1) {
            m_patientsTable->selectRow(row);
            openPatientFromTable();
        }
    });
    auto *patientEnter = new QShortcut(QKeySequence(Qt::Key_Return), m_patientsTable);
    connect(patientEnter, &QShortcut::activated, this, [this]() {
        if (m_currentScreen == ScreenMode::Patients && m_patientsTable->currentRow() >= 0) {
            openPatientFromTable();
        }
    });
    auto *patientEnter2 = new QShortcut(QKeySequence(Qt::Key_Enter), m_patientsTable);
    connect(patientEnter2, &QShortcut::activated, this, [this]() {
        if (m_currentScreen == ScreenMode::Patients && m_patientsTable->currentRow() >= 0) {
            openPatientFromTable();
        }
    });

    connect(m_bSave, &ImageButton::clicked, this, [this]() { showPrintPanel(true); });
    connect(m_bPrint, &ImageButton::clicked, this, [this]() { showPrintPanel(false); });
    connect(m_bSettings, &ImageButton::clicked, this, [this]() {
        refreshTemplateNames();
        syncTemplateSelectorFromProfile();
        toggleSlidePanel(m_settingsPanel);
    });
    connect(m_templates, QOverload<int>::of(&QComboBox::activated), this, [this](int index) {
        Q_UNUSED(index);
        if (m_settingsPanel == nullptr || !m_settingsPanel->isVisible() || m_suppressTemplateLoad) {
            return;
        }
        loadAnamnesisTemplateByName(m_templates->currentText().trimmed());
    });
    connect(m_underlineButton, &ImageButton::clicked, this, [this]() {
        m_underlineActive = !m_underlineActive;
        QTextCharFormat fmt;
        fmt.setFontUnderline(m_underlineActive);
        m_anamnesisEdit->mergeCurrentCharFormat(fmt);
        updateFormatButtonIcons();
    });
    connect(m_boldButton, &ImageButton::clicked, this, [this]() {
        m_boldActive = !m_boldActive;
        QTextCharFormat fmt;
        fmt.setFontWeight(m_boldActive ? QFont::Bold : QFont::Normal);
        m_anamnesisEdit->mergeCurrentCharFormat(fmt);
        updateFormatButtonIcons();
    });
    connect(m_anamnesisEdit, &QTextEdit::cursorPositionChanged, this, [this]() { updateFormatButtonIcons(); });
    connect(m_anamnesisEdit, &QTextEdit::selectionChanged, this, [this]() { updateFormatButtonIcons(); });
    m_anamnesisEdit->installEventFilter(this);
    auto *anamnesisAutoSaveTimer = new QTimer(this);
    anamnesisAutoSaveTimer->setSingleShot(true);
    anamnesisAutoSaveTimer->setInterval(1200);
    connect(m_anamnesisEdit, &QTextEdit::textChanged, this, [this, anamnesisAutoSaveTimer]() {
        updatePatientTitleFromDocument();
        anamnesisAutoSaveTimer->start();
    });
    connect(anamnesisAutoSaveTimer, &QTimer::timeout, this, [this]() { tryAutoSaveAnamnesis(); });
}

void SunriseWindow::setScreen(ScreenMode mode, bool pushHistory) {
    const ScreenMode previousScreen = m_currentScreen;
    m_screenTransitionGuard = true;
    struct ScreenTransitionGuard {
        bool *flag = nullptr;
        ~ScreenTransitionGuard() {
            if (flag) {
                *flag = false;
            }
        }
    } transitionGuard{&m_screenTransitionGuard};

    if (previousScreen == ScreenMode::Anamnesis && mode != ScreenMode::Anamnesis) {
        tryAutoSaveAnamnesis(true);
        tryAutoSaveProgram(true);
        refreshPatients();
    }
    if (isWorkScreen(previousScreen) && previousScreen != mode) {
        tryAutoSaveProgram(true);
    }

    if (pushHistory && !m_navigatingBack) {
        m_navHistory.append(mode);
    }

    m_currentScreen = mode;
    hideSlidePanels();
    if (m_infoPopup && m_infoPopup->isVisible()) {
        m_infoPopup->hide();
    }

    const bool enter = mode == ScreenMode::Enter;
    const bool patients = mode == ScreenMode::Patients;
    const bool admin = mode == ScreenMode::Admin;
    const bool anamnesis = mode == ScreenMode::Anamnesis;
    const bool workScreen = isWorkScreen(mode);

    m_logo1->setVisible(enter);
    if (m_logoTitle2) {
        m_logoTitle2->setVisible(enter);
    }
    m_logo2->setVisible(enter);
    m_panelLogin->setVisible(enter);

    m_panelPatients->setVisible(patients);
    m_panelAdmin->setVisible(admin);

    if (m_workStack) {
        m_workStack->setVisible(workScreen);
        if (anamnesis) {
            if (m_anamnesisEdit) {
                m_anamnesisEdit->clearFocus();
            }
            m_workStack->setCurrentWidget(m_panelWork);
        } else if (workScreen) {
            m_workStack->setCurrentWidget(m_panelClinical);
            using Section = ClinicalBrowserController::Section;
            Section section = Section::Risk;
            if (mode == ScreenMode::Correction) {
                section = Section::Correction;
            } else if (mode == ScreenMode::Speech) {
                section = Section::Speech;
            } else if (mode == ScreenMode::Breathing) {
                section = Section::Breathing;
            } else if (mode == ScreenMode::Medication) {
                section = Section::Medication;
            }
            m_clinical->openSection(section);
        }
        m_workStack->update();
    }
    layoutWorkAreaForMode(mode);
    // pSelector(): wsummary виден на анамнезе и остаётся видимым на вкладках пациента.
    if (m_summaryPanel) {
        m_summaryPanel->setVisible(workScreen);
        if (anamnesis && !isWorkScreen(previousScreen)) {
            loadSummaryForCurrentPatient();
        }
    }
    m_bJournal->setVisible(admin);
    m_bUpdate->setVisible(admin);
    m_adminTitle->setVisible(admin);
    m_userOpenPatients->setVisible(admin);
    m_userOpenPatients->raise();

    m_bBack->setVisible(false);
    m_bList->setVisible(workScreen);
    m_bExit->setVisible(!enter);
    m_bSave->setVisible(workScreen);
    m_bPrint->setVisible(workScreen);
    if (!workScreen && m_printPanel) {
        m_printPanel->hide();
    }
    m_bSettings->setVisible(anamnesis);
    if (!anamnesis && m_settingsPanel && m_settingsPanel->isVisible()) {
        m_settingsPanel->hide();
    }
    m_bPicPrint->setVisible(false);
    m_bUpload->setVisible(false);
    m_bInfo->setVisible(true);
    for (ImageButton *tab : {m_pAna, m_pRisk, m_pCorr, m_pUpr, m_pDih, m_pMp}) {
        tab->setVisible(workScreen);
    }
    m_patientTitle->setVisible(workScreen);
    m_underlineButton->setVisible(anamnesis);
    m_boldButton->setVisible(anamnesis);

    if (workScreen) {
        for (ImageButton *tab : {m_pAna, m_pRisk, m_pCorr, m_pUpr, m_pDih, m_pMp}) {
            tab->raise();
        }
        if (m_workStack) {
            m_workStack->raise();
        }
        m_summaryPanel->raise();
        m_patientTitle->raise();
        m_bSave->raise();
        m_bPrint->raise();
        if (anamnesis) {
            m_bSettings->raise();
        }
    }
    if (admin) {
        m_bJournal->raise();
        m_bUpdate->raise();
    }

    if (patients) {
        m_helpIndex = QStringLiteral("0.3.htm");
        refreshPatients();
    }
    if (admin) {
        m_helpIndex = QStringLiteral("0.2.1.htm");
        refreshUsers();
    }
    if (anamnesis) {
        m_helpIndex = QStringLiteral("0.4.htm");
        QTimer::singleShot(0, this, [this]() {
            if (m_currentScreen != ScreenMode::Anamnesis) {
                return;
            }
            refreshTemplateNames();
            if (m_currentPatientId.isEmpty() && m_anamnesisEdit->toPlainText().trimmed().isEmpty()) {
                loadDefaultAnamnesisTemplate();
            } else {
                int fontSize = 24;
                const QString configPath = profileConfigPath();
                if (!configPath.isEmpty()) {
                    QFile configFile(configPath);
                    if (configFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
                        const QStringList parts = QString::fromUtf8(configFile.readAll()).trimmed().split(';');
                        if (parts.size() > 1) {
                            fontSize = parts.at(1).toInt();
                        }
                    }
                }
                if (m_fontSlider) {
                    QSignalBlocker blocker(m_fontSlider);
                    m_fontSlider->setValue(fontSize);
                }
                if (m_fontSizeLabel) {
                    m_fontSizeLabel->setText(QString::number(fontSize));
                }
                changeDocumentFontSize(fontSize, false);
            }
        });
    } else if (workScreen) {
        m_helpIndex = m_clinical->helpIndex();
    }
    if (workScreen) {
        updatePatientTabIcons();
    }
    if (enter) {
        m_helpIndex = QStringLiteral("0.1.htm");
        applyEnterLogos();
    }
}

void SunriseWindow::navigateBack() {
    if (m_navHistory.size() <= 1) {
        setScreen(ScreenMode::Patients, false);
        return;
    }
    m_navigatingBack = true;
    m_navHistory.removeLast();
    setScreen(m_navHistory.last(), false);
    m_navigatingBack = false;
}

void SunriseWindow::toggleWindowMaximize() {
    QRect frame = m_savedWindowGeometry.isValid() ? m_savedWindowGeometry : geometry();
    if (m_isCustomMaximized) {
        frame.setHeight(kDesignHeight);
        m_isCustomMaximized = false;
    } else {
        frame.setHeight(kDesignHeight - kTaskbarReserve);
        m_isCustomMaximized = true;
    }
    applyWindowGeometry(frame);
    updateMaximizeButtonIcon();
}

void SunriseWindow::applyWindowGeometry(const QRect &rect) {
    m_programmaticGeometryChange = true;
    setGeometry(rect);
    m_savedWindowGeometry = rect;
    m_programmaticGeometryChange = false;
}

void SunriseWindow::moveEvent(QMoveEvent *event) {
#ifndef Q_OS_WIN
    if (!m_programmaticGeometryChange && m_savedWindowGeometry.isValid()
        && pos() != m_savedWindowGeometry.topLeft()) {
        m_programmaticGeometryChange = true;
        move(m_savedWindowGeometry.topLeft());
        m_programmaticGeometryChange = false;
        return;
    }
#endif
    QMainWindow::moveEvent(event);
}

void SunriseWindow::resizeEvent(QResizeEvent *event) {
#ifndef Q_OS_WIN
    if (!m_programmaticGeometryChange && m_savedWindowGeometry.isValid()
        && size() != m_savedWindowGeometry.size()) {
        m_programmaticGeometryChange = true;
        resize(m_savedWindowGeometry.size());
        m_programmaticGeometryChange = false;
        return;
    }
#endif
    QMainWindow::resizeEvent(event);
}

void SunriseWindow::updateMaximizeButtonIcon() {
    setImage(m_bUp, m_isCustomMaximized ? "down.png" : "up.png");
}

void SunriseWindow::changeEvent(QEvent *event) {
    if (event->type() == QEvent::WindowStateChange) {
        const auto *stateEvent = static_cast<QWindowStateChangeEvent *>(event);
        const Qt::WindowStates oldState = stateEvent->oldState();
        const Qt::WindowStates newState = windowState();

        if (!(oldState & Qt::WindowMinimized) && (newState & Qt::WindowMinimized)) {
            m_savedWindowGeometry = geometry();
        } else if ((oldState & Qt::WindowMinimized) && !(newState & Qt::WindowMinimized)) {
            QTimer::singleShot(0, this, [this]() {
                if (m_savedWindowGeometry.isValid()) {
                    applyWindowGeometry(m_savedWindowGeometry);
                }
            });
        }
    }
    QMainWindow::changeEvent(event);
}

void SunriseWindow::showEvent(QShowEvent *event) {
    QMainWindow::showEvent(event);
    if (!m_geometryInitialized) {
        m_geometryInitialized = true;
        applyNormalWindowGeometry();
    }
}

QRect SunriseWindow::calculateNormalWindowGeometry() const {
    QScreen *screen = windowHandle() ? windowHandle()->screen() : QGuiApplication::primaryScreen();
    if (!screen) {
        return QRect(0, 0, kDesignWidth, kDesignHeight);
    }

    const QRect screenGeometry = screen->geometry();
    const int width = qMin(kDesignWidth, screenGeometry.width());
    const int height = kDesignHeight;
    const int x = screenGeometry.x() + qMax(0, (screenGeometry.width() - width) / 2);
    const int y = screenGeometry.y();
    return QRect(x, y, width, height);
}

QRect SunriseWindow::calculateMaximizedWindowGeometry() const {
    return calculateNormalWindowGeometry();
}

void SunriseWindow::applyNormalWindowGeometry() {
    const QRect initialGeometry = calculateNormalWindowGeometry();
    applyWindowGeometry(initialGeometry);
    m_isCustomMaximized = false;
    updateMaximizeButtonIcon();
}

void SunriseWindow::updatePatientTabIcons() {
    // setTab(): t1/prot/upr/btab4/btab5/btab6 в состояниях On/Off.
    setImage(m_pAna, m_currentScreen == ScreenMode::Anamnesis ? "t1on.png" : "t1off.png");
    setImage(m_pRisk, m_currentScreen == ScreenMode::RiskAssessment ? "proton.png" : "protoff.png");
    setImage(m_pCorr, m_currentScreen == ScreenMode::Correction ? "upron.png" : "uproff.png");
    setImage(m_pUpr, m_currentScreen == ScreenMode::Speech ? "btab4On.png" : "btab4Off.png");
    setImage(m_pDih, m_currentScreen == ScreenMode::Breathing ? "btab5On.png" : "btab5Off.png");
    setImage(m_pMp, m_currentScreen == ScreenMode::Medication ? "btab6On.png" : "btab6Off.png");
}

void SunriseWindow::styleInputField(QLineEdit *edit) const {
    edit->setStyleSheet(
        "QLineEdit {"
        "  font-family: 'Microsoft Sans Serif';"
        "  font-size: 14.25pt;"
        "  background: white;"
        "  border: 1px solid #7a7a7a;"
        "  padding: 0px 1px 2px 0px;"
        "}"
        "QLineEdit:hover { border-color: #171717; }"
        "QLineEdit:focus { border-color: #0078d7; }"
    );
    keepPlaceholderColor(edit);
}

void SunriseWindow::styleAdminInputField(QLineEdit *edit) const {
    edit->setFrame(false);
    edit->setMaxLength(35);
    edit->setStyleSheet(
        "QLineEdit {"
        "  font-family: 'Microsoft Sans Serif';"
        "  font-size: 14.25pt;"
        "  background: white;"
        "  border: none;"
        "  padding: 0px;"
        "}"
    );
    keepPlaceholderColor(edit);
}

void SunriseWindow::styleAdminScreen() {
    for (QWidget *panel : {m_userFioPanel, m_userLoginPanel, m_userPassPanel, m_userPass2Panel}) {
        panel->setObjectName(QStringLiteral("adminFieldPanel"));
        panel->setStyleSheet(QStringLiteral("QWidget#adminFieldPanel { background: white; border: 1px solid black; }"));
    }

    styleAdminInputField(m_userFio);
    styleAdminInputField(m_userLogin);
    styleAdminInputField(m_userPass);
    styleAdminInputField(m_userPass2);

    m_userFioClear->setStyleSheet("background: white;");
    m_userLoginClear->setStyleSheet("background: white;");
    m_userPassClear->setStyleSheet("background: white;");
    m_userPass2Clear->setStyleSheet("background: white;");

    const QString labelStyle = QStringLiteral(
        "color: white; font-family: 'Microsoft Sans Serif'; font-size: 12pt; font-weight: bold; background: transparent;"
    );
    m_adminLabel1->setStyleSheet(labelStyle);
    m_adminLabel2->setStyleSheet(labelStyle);
    m_adminLabel2->ensurePolished();
    m_adminLabel2->adjustSize();
    m_userRole->setStyleSheet(QStringLiteral(
        "QComboBox {"
        "  font-family: 'Microsoft Sans Serif';"
        "  font-size: 12pt;"
        "  background: #e1e1e1;"
        "  color: black;"
        "  border: 1px solid #adadad;"
        "  border-radius: 0px;"
        "  padding: 0px 18px 0px 3px;"
        "}"
        "QComboBox::drop-down {"
        "  subcontrol-origin: border;"
        "  subcontrol-position: center right;"
        "  width: 18px;"
        "  border: none;"
        "  background: transparent;"
        "}"
        "QComboBox::down-arrow {"
        "  image: url(:/ui/combo_arrow.png);"
        "  width: 12px;"
        "  height: 8px;"
        "}"
        "QComboBox QAbstractItemView {"
        "  background: white;"
        "  color: black;"
        "  border: 1px solid #646464;"
        "  selection-background-color: #0078d7;"
        "  selection-color: white;"
        "  outline: 0;"
        "}"
    ));
}

void SunriseWindow::stylePatientsScreen() {
    styleInputField(m_patientSearch);

    m_dateFilter->setStyleSheet(
        QStringLiteral(
            "QCheckBox {"
            "  color: white;"
            "  font-family: 'Microsoft Sans Serif';"
            "  font-size: 9pt;"
            "  font-weight: bold;"
            "  spacing: 6px;"
            "  background: transparent;"
            "}"
            "QCheckBox::indicator { width: 13px; height: 13px; }"
        )
    );
    m_dateFilter->ensurePolished();
    m_dateFilter->adjustSize();

    const QString dateLabelStyle =
        "color: white; font-family: 'Microsoft Sans Serif'; font-size: 8.25pt; font-weight: bold; background: transparent;";
    for (QLabel *label : {m_labelFrom, m_labelTo}) {
        label->setStyleSheet(dateLabelStyle);
        label->ensurePolished();
        label->adjustSize();
    }

    const QString dateEditStyle = QStringLiteral(
        "QDateEdit {"
        "  font-family: 'Microsoft Sans Serif';"
        "  font-size: 8.25pt;"
        "  color: black;"
        "  background: white;"
        "  border: 1px solid #7a7a7a;"
        "  padding: 0px 30px 2px 5px;"
        "}"
        "QDateEdit::drop-down {"
        "  subcontrol-origin: border;"
        "  subcontrol-position: center right;"
        "  width: 31px;"
        "  border: none;"
        "  background: transparent;"
        "}"
        "QDateEdit::down-arrow {"
        "  image: url(:/ui/dtp_button.png);"
        "  width: 30px;"
        "  height: 18px;"
        "}"
    );
    m_dateFrom->setStyleSheet(dateEditStyle);
    m_dateTo->setStyleSheet(dateEditStyle);

    const QString calendarStyle = QStringLiteral(
        "QCalendarWidget { background-color: white; }"
        "QCalendarWidget QWidget#qt_calendar_navigationbar { background-color: white; }"
        "QCalendarWidget QToolButton {"
        "  color: black;"
        "  background-color: white;"
        "  font-family: 'Microsoft Sans Serif';"
        "  font-size: 9pt;"
        "  border: none;"
        "}"
        "QCalendarWidget QToolButton#qt_calendar_monthbutton {"
        "  min-width: 110px;"
        "  padding-right: 18px;"
        "}"
        "QCalendarWidget QToolButton#qt_calendar_yearbutton {"
        "  min-width: 58px;"
        "}"
        "QCalendarWidget QToolButton::menu-indicator {"
        "  subcontrol-position: right center;"
        "  width: 12px;"
        "  right: 2px;"
        "}"
        "QCalendarWidget QMenu { background-color: white; color: black; }"
        "QCalendarWidget QAbstractItemView:enabled {"
        "  background-color: white;"
        "  color: black;"
        "  selection-background-color: #316ac5;"
        "  selection-color: white;"
        "}"
        "QCalendarWidget QAbstractItemView:disabled { color: #a0a0a0; }"
    );
    if (m_dateFrom->calendarWidget()) {
        m_dateFrom->calendarWidget()->setStyleSheet(calendarStyle);
    }
    if (m_dateTo->calendarWidget()) {
        m_dateTo->calendarWidget()->setStyleSheet(calendarStyle);
    }

}

void SunriseWindow::styleAnamnesisScreen() {
    m_patientTitle->setStyleSheet(
        "QLabel {"
        "  background-color: #f2f0f0;"
        "  color: #000000;"
        "  font-family: 'Microsoft Sans Serif';"
        "  font-size: 11pt;"
        "}"
    );
    m_anamnesisEdit->setFrameShape(QFrame::StyledPanel);
    m_anamnesisEdit->setFrameShadow(QFrame::Sunken);
    m_anamnesisEdit->setLineWidth(1);
    m_anamnesisEdit->setAutoFillBackground(true);
    m_anamnesisEdit->setAttribute(Qt::WA_OpaquePaintEvent, true);
    m_anamnesisEdit->setStyleSheet(
        whiteScrollBarCss(QStringLiteral("QTextEdit")) +
        QStringLiteral(
            "QTextEdit {"
            "  background-color: #ffffff;"
            "  background-image: none;"
            "  color: #000000;"
            "  selection-background-color: #316ac5;"
            "  selection-color: #ffffff;"
            "}"
        )
    );
    applyAnamnesisDocumentFontDefaults();
    m_anamnesisEdit->document()->setDocumentMargin(6);
    QPalette editorPalette = m_anamnesisEdit->palette();
    editorPalette.setColor(QPalette::Base, Qt::white);
    editorPalette.setColor(QPalette::Text, Qt::black);
    editorPalette.setColor(QPalette::Window, Qt::white);
    m_anamnesisEdit->setPalette(editorPalette);
    if (m_anamnesisEdit->viewport()) {
        m_anamnesisEdit->viewport()->setAttribute(Qt::WA_OpaquePaintEvent, true);
        m_anamnesisEdit->viewport()->setAutoFillBackground(true);
        m_anamnesisEdit->viewport()->setStyleSheet(
            QStringLiteral("background-color: #ffffff; background-image: none;")
        );
    }
}

void SunriseWindow::applyAnamnesisDocumentFontDefaults() {
    if (!m_anamnesisEdit) {
        return;
    }
    QFont font(QStringLiteral("Times New Roman"), 12);
    m_anamnesisEdit->setFont(font);
    m_anamnesisEdit->document()->setDefaultFont(font);
    m_anamnesisEdit->document()->setDefaultStyleSheet(
        "body { font-family: 'Times New Roman'; font-size: 12pt; color: #000000; background-color: #ffffff; }"
        "p { margin-top: 0pt; margin-bottom: 0pt; line-height: 85%; }"
    );
}

void SunriseWindow::applyCompactAnamnesisLineSpacing() {
#ifndef Q_OS_WIN
    return;
#else
    if (!m_anamnesisEdit) {
        return;
    }
    QTextDocument *doc = m_anamnesisEdit->document();
    if (!doc || doc->isEmpty()) {
        return;
    }
    // Сохраняем курсор/скролл: mergeBlockFormat иначе может увести каретку в начало.
    const QTextCursor savedCursor = m_anamnesisEdit->textCursor();
    const int savedScroll = m_anamnesisEdit->verticalScrollBar()
        ? m_anamnesisEdit->verticalScrollBar()->value()
        : 0;

    QTextCursor cursor(doc);
    cursor.beginEditBlock();
    for (QTextBlock block = doc->begin(); block.isValid(); block = block.next()) {
        QTextBlockFormat fmt = block.blockFormat();
        fmt.setLineHeight(85, QTextBlockFormat::ProportionalHeight);
        fmt.setTopMargin(0);
        fmt.setBottomMargin(0);
        cursor.setPosition(block.position());
        cursor.mergeBlockFormat(fmt);
    }
    cursor.endEditBlock();

    m_anamnesisEdit->setTextCursor(savedCursor);
    if (m_anamnesisEdit->verticalScrollBar()) {
        m_anamnesisEdit->verticalScrollBar()->setValue(savedScroll);
    }
#endif
}

void SunriseWindow::styleTemplateComboBox() {
    if (!m_templates) {
        return;
    }
    m_templates->setStyleSheet(QString(
        "QComboBox {"
        "  background: #ffffff;"
        "  color: #000000;"
        "  font-family: 'Microsoft Sans Serif';"
        "  font-size: 12pt;"
        "  border: 1px solid #808080;"
        "  padding: 2px 28px 2px 6px;"
        "}"
        "QComboBox::drop-down {"
        "  subcontrol-origin: padding;"
        "  subcontrol-position: top right;"
        "  width: 22px;"
        "  border: none;"
        "  border-left: 1px solid #b0b0b0;"
        "  background: #ececec;"
        "}"
        "QComboBox::down-arrow {"
        "%1"
        "}"
        "QComboBox QAbstractItemView {"
        "  background: #ffffff;"
        "  color: #000000;"
        "  selection-background-color: #316ac5;"
        "}"
    ).arg(dropdownArrowCss()));
}

void SunriseWindow::fitPatientsTableToContent() {
    if (!m_patientsTable) {
        return;
    }

    constexpr int kTableHeight = 800;
    m_patientsTable->setFixedHeight(kTableHeight);
}

void SunriseWindow::bindClearableField(QLineEdit *edit, ImageButton *clearBtn) {
    connect(edit, &QLineEdit::textChanged, this, [clearBtn](const QString &text) {
        clearBtn->setVisible(!text.isEmpty());
    });
    connect(clearBtn, &ImageButton::clicked, this, [edit, clearBtn]() {
        edit->clear();
        clearBtn->hide();
    });
}

void SunriseWindow::applyEnterLogos() {
    auto placeLogo = [](QLabel *label, const QString &path, int x, int y, bool centerHorizontally, int canvasWidth) -> int {
        if (!label || path.isEmpty()) {
            return 0;
        }
        const QPixmap pixmap(path);
        if (pixmap.isNull()) {
            return 0;
        }
        label->setPixmap(pixmap);
        label->setScaledContents(false);
        const int posX = centerHorizontally ? (canvasWidth - pixmap.width()) / 2 : x;
        label->setGeometry(posX, y, pixmap.width(), pixmap.height());
        return pixmap.height();
    };

    // Как в old_sunrise Form1 pSelector(enter): infant2.png слева, infant.png — заголовок.
    placeLogo(m_logo2, imagePath("infant2.png"), 0, 7, false, width());
    placeLogo(m_logo1, imagePath("infant.png"), 160, 81, false, width());
    if (m_logoTitle2) {
        m_logoTitle2->clear();
        m_logoTitle2->hide();
    }
}

QString SunriseWindow::imagePath(const QString &name) const {
    const QStringList roots = {
        QCoreApplication::applicationDirPath() + "/assets/sysImages",
        QCoreApplication::applicationDirPath() + "/../assets/sysImages",
        QCoreApplication::applicationDirPath() + "/../../assets/sysImages",
        QCoreApplication::applicationDirPath() + "/../../../assets/sysImages",
        QDir::currentPath() + "/assets/sysImages",
        QDir::currentPath() + "/../old_project/serv9 2025/WindowsFormsApp1/bin/Debug/sysImages",
        QDir::currentPath() + "/../old_project/serv9 2025/WindowsFormsApp1/bin/maindata/sysImages"
    };
    for (const QString &root : roots) {
        const QString candidate = QDir(root).filePath(name);
        if (QFile::exists(candidate)) {
            return candidate;
        }
    }
    return {};
}

QString SunriseWindow::resourcePath(const QString &name) const {
    const QStringList roots = {
        QCoreApplication::applicationDirPath() + "/assets/resources",
        QCoreApplication::applicationDirPath() + "/../assets/resources",
        QCoreApplication::applicationDirPath() + "/../../assets/resources",
        QCoreApplication::applicationDirPath() + "/../../../assets/resources",
        QDir::currentPath() + "/assets/resources",
        QDir::currentPath() + "/../old_project/serv9 2025/WindowsFormsApp1/Resources",
        QDir::currentPath() + "/../old_project/serv9 2025/WindowsFormsApp1/bin/Debug/sysImages"
    };
    for (const QString &root : roots) {
        const QString candidate = QDir(root).filePath(name);
        if (QFile::exists(candidate)) {
            return candidate;
        }
    }
    return {};
}

QString SunriseWindow::htmlPath(const QString &name) const {
    const QStringList roots = {
        QCoreApplication::applicationDirPath() + "/assets/htmls",
        QCoreApplication::applicationDirPath() + "/../assets/htmls",
        QCoreApplication::applicationDirPath() + "/../../assets/htmls",
        QCoreApplication::applicationDirPath() + "/../../../assets/htmls",
        QDir::currentPath() + "/assets/htmls",
        QDir::currentPath() + "/../old_project/serv9 2025/WindowsFormsApp1/bin/Debug/htmls"
    };
    for (const QString &root : roots) {
        const QString candidate = QDir(root).filePath(name);
        if (QFile::exists(candidate)) {
            return candidate;
        }
    }
    return {};
}

QString SunriseWindow::profileConfigPath() const {
    const QStringList roots = {
        QCoreApplication::applicationDirPath() + "/../assets/ex/names",
        QCoreApplication::applicationDirPath() + "/../../assets/ex/names",
        QCoreApplication::applicationDirPath() + "/../../../assets/ex/names",
        QDir::currentPath() + "/assets/ex/names",
        QDir::currentPath() + "/../old_project/serv9 2025/WindowsFormsApp1/bin/Debug/ex/names",
        QDir::currentPath() + "/../old_project/serv9 2025/WindowsFormsApp1/bin/maindata/ex/names"
    };
    const QStringList names = {QStringLiteral("шаблон.txt"), QStringLiteral("Шаблон.txt")};
    for (const QString &root : roots) {
        for (const QString &name : names) {
            const QString candidate = QDir(root).filePath(name);
            if (QFile::exists(candidate)) {
                return candidate;
            }
        }
    }
    return {};
}

QString SunriseWindow::defaultAnamnesisHtml() const {
    const QString prepared = readAnamnesisTemplateHtml();
    if (!prepared.trimmed().isEmpty()) {
        return prepared;
    }
    return m_repository.defaultAnamnesisTemplate();
}

void SunriseWindow::setImage(ImageButton *button, const QString &name) {
    if (!button) {
        return;
    }
    const QString path = imagePath(name);
    if (!path.isEmpty()) {
        button->setImagePath(path);
    }
}

QString SunriseWindow::decodeDocument(const QString &raw) const {
    if (raw.trimmed().startsWith("{\\rtf")) {
        return QString();
    }
    if (raw.trimmed().isEmpty()) {
        return defaultAnamnesisHtml();
    }
    return raw;
}

void SunriseWindow::loadAnamnesisRtf(const QByteArray &rtf) {
    m_lastAnamnesisRtf.clear();
    const QString html = rtfToHtml(rtf);
    if (!m_anamnesisEdit || html.isEmpty()) {
        loadStandardAnamnesisHtml();
        return;
    }
    m_anamnesisEdit->setHtml(html);
    applyAnamnesisDocumentFontDefaults();
#ifdef Q_OS_WIN
    applyCompactAnamnesisLineSpacing();
#endif
}

void SunriseWindow::loadStandardAnamnesisHtml() {
    if (!m_anamnesisEdit) {
        return;
    }
    m_lastAnamnesisRtf.clear();
    const QString prepared = readAnamnesisTemplateHtml();
    if (prepared.trimmed().isEmpty()) {
        m_anamnesisEdit->setHtml(m_repository.defaultAnamnesisTemplate());
        applyAnamnesisDocumentFontDefaults();
        return;
    }
    m_anamnesisEdit->setHtml(prepared);
    applyAnamnesisDocumentFontDefaults();
#ifndef Q_OS_WIN
    return;
#else
    applyCompactAnamnesisLineSpacing();
#endif
}

void SunriseWindow::applyAnamnesisFont(int pointSize) {
    if (pointSize <= 0 || !m_anamnesisEdit) {
        return;
    }
    QTextCharFormat format;
    format.setFontFamily(QStringLiteral("Times New Roman"));
    format.setFontPointSize(pointSize / 2.0);
    m_anamnesisEdit->mergeCurrentCharFormat(format);
    if (m_fontSizeLabel) {
        m_fontSizeLabel->setText(QString::number(pointSize));
    }
}

void SunriseWindow::applyAnamnesisFontToEntireDocument(int pointSize) {
    if (pointSize <= 0 || !m_anamnesisEdit) {
        return;
    }

    const double fontPt = pointSize / 2.0;
    QTextDocument *doc = m_anamnesisEdit->document();
    if (!doc) {
        return;
    }

    QTextCursor cursor(doc);
    cursor.beginEditBlock();
    cursor.select(QTextCursor::Document);
    QTextCharFormat format;
    format.setFontFamily(QStringLiteral("Times New Roman"));
    format.setFontPointSize(fontPt);
    cursor.mergeCharFormat(format);
    cursor.clearSelection();
    cursor.endEditBlock();

    QFont defaultFont(QStringLiteral("Times New Roman"));
    defaultFont.setPointSizeF(fontPt);
    doc->setDefaultFont(defaultFont);
    doc->setDefaultStyleSheet(
        QStringLiteral(
            "body { font-family: 'Times New Roman'; font-size: %1pt; color: #000000; background-color: #ffffff; }"
            "p { margin-top: 0pt; margin-bottom: 0pt; line-height: 85%%; }"
        ).arg(fontPt, 0, 'f', 1)
    );
    m_anamnesisEdit->setFont(defaultFont);

    if (m_fontSizeLabel) {
        m_fontSizeLabel->setText(QString::number(pointSize));
    }
}

void SunriseWindow::applyAnamnesisDocument(const QString &raw) {
    if (raw.trimmed().startsWith("{\\rtf")) {
        loadAnamnesisRtf(raw.toLatin1());
        return;
    }
    m_lastAnamnesisRtf.clear();
    if (raw.trimmed().isEmpty()) {
        loadStandardAnamnesisHtml();
        return;
    }
    if (isRawRtfPlainText(raw)) {
        loadStandardAnamnesisHtml();
        return;
    }
    QString html = raw;
#ifndef Q_OS_WIN
    if (isWordExportHtml(html) || html.size() > 100000) {
        html = prepareAnamnesisHtml(html);
        if (html.trimmed().isEmpty()) {
            loadStandardAnamnesisHtml();
            return;
        }
    }
#endif
    m_anamnesisEdit->setHtml(html);
    applyAnamnesisDocumentFontDefaults();
#ifndef Q_OS_WIN
    return;
#else
    applyCompactAnamnesisLineSpacing();
#endif
}

void SunriseWindow::loadDefaultAnamnesisTemplate() {
    QString profileName = QStringLiteral("Стандартный");
    const QString configPath = profileConfigPath();
    if (!configPath.isEmpty()) {
        QFile configFile(configPath);
        if (configFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            const QStringList parts = QString::fromUtf8(configFile.readAll()).trimmed().split(';');
            if (!parts.isEmpty() && !parts.first().trimmed().isEmpty()) {
                profileName = parts.first().trimmed();
            }
        }
    }

    if (profileName == QStringLiteral("Стандартный")) {
        const QString rtfPath = htmlPath("anamnez.rtf");
        if (!rtfPath.isEmpty()) {
            QFile rtfFile(rtfPath);
            if (rtfFile.open(QIODevice::ReadOnly)) {
                loadAnamnesisRtf(rtfFile.readAll());
            } else {
                loadStandardAnamnesisHtml();
            }
        } else {
            loadStandardAnamnesisHtml();
        }
        int fontSize = 24;
        const QString configPath = profileConfigPath();
        if (!configPath.isEmpty()) {
            QFile configFile(configPath);
            if (configFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
                const QStringList parts = QString::fromUtf8(configFile.readAll()).trimmed().split(';');
                if (parts.size() > 1) {
                    fontSize = parts.at(1).toInt();
                }
            }
        }
        if (m_fontSlider) {
            QSignalBlocker blocker(m_fontSlider);
            m_fontSlider->setValue(fontSize);
        }
        changeDocumentFontSize(fontSize);
        return;
    }

    const QString templateData = m_repository.loadTemplate(profileName);
    if (templateData.trimmed().isEmpty()) {
        loadStandardAnamnesisHtml();
        int fontSize = 24;
        const QString configPath = profileConfigPath();
        if (!configPath.isEmpty()) {
            QFile configFile(configPath);
            if (configFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
                const QStringList parts = QString::fromUtf8(configFile.readAll()).trimmed().split(';');
                if (parts.size() > 1) {
                    fontSize = parts.at(1).toInt();
                }
            }
        }
        if (m_fontSlider) {
            QSignalBlocker blocker(m_fontSlider);
            m_fontSlider->setValue(fontSize);
        }
        changeDocumentFontSize(fontSize);
        return;
    }
    applyAnamnesisDocument(templateData);
}

void SunriseWindow::saveUser() {
    if (m_userSaveInProgress) {
        return;
    }

    const bool creating = m_adminLabel1->text() == QStringLiteral("Создать пользователя");
    if (creating) {
        if (m_userFio->text().trimmed().isEmpty() || m_userLogin->text().trimmed().isEmpty()
            || m_userPass->text().isEmpty() || m_userPass2->text().isEmpty()) {
            CustomMessageBox::showWarning(this, "Заполните все поля.");
            return;
        }
    } else if (m_userFio->text().trimmed().isEmpty() || m_userLogin->text().trimmed().isEmpty()) {
        CustomMessageBox::showWarning(this, "Заполните все поля.");
        return;
    }
    if (m_userPass->text() != m_userPass2->text()) {
        CustomMessageBox::showWarning(this, "Символы в полях 'придумайте пароль' и 'подтвердите пароль' должны быть полностью идентичны.");
        return;
    }

    const QString fio = m_userFio->text().trimmed();
    const QString login = m_userLogin->text().trimmed();
    const QString password = m_userPass->text();
    const QString role = m_userRole->currentText();
    const QString mainId = m_mainId;
    const QString editUserId = m_editUserId;

    m_userSaveInProgress = true;
    m_userSaveButton->setEnabled(false);
    QApplication::setOverrideCursor(Qt::WaitCursor);

    auto *watcher = new QFutureWatcher<UserSaveResult>(this);
    connect(watcher, &QFutureWatcher<UserSaveResult>::finished, this, [this, watcher]() {
        const UserSaveResult result = watcher->result();
        watcher->deleteLater();

        QApplication::restoreOverrideCursor();
        m_userSaveInProgress = false;
        m_userSaveButton->setEnabled(true);

        if (!result.ok) {
            CustomMessageBox::showError(this, result.error);
            return;
        }
        rememberManagedUser(result.userId, result.login, result.password);
        resetUserCreateForm();
        scheduleRefreshUsers();
    });

    watcher->setFuture(QtConcurrent::run([creating, fio, login, password, role, mainId, editUserId]() {
        ApiClient api;
        Repository repository(&api);
        QString error;
        UserSaveResult result;
        result.login = login;
        result.password = password;
        if (creating) {
            QString createdId;
            result.ok = repository.createUser(fio, login, password, role, mainId, &error, &createdId);
            result.userId = createdId;
        } else {
            result.ok = repository.updateUser(editUserId, fio, login, password, role, &error);
            result.userId = editUserId;
        }
        result.error = error;
        return result;
    }));
}

void SunriseWindow::rememberManagedUser(const QString &userId, const QString &login, const QString &password) {
    if (!userId.isEmpty()) {
        m_lastManagedUserId = userId;
    }
    if (!login.isEmpty()) {
        m_lastManagedUserLogin = login;
    }
    if (!password.isEmpty()) {
        m_lastManagedUserPassword = password;
    }
}

void SunriseWindow::startUserSession(const SessionUser &user) {
    if (m_session.has_value() && m_session->id != user.id) {
        endUserSession();
    }
    const bool alreadyLoggedIn = m_session.has_value();
    m_session = user;
    if (!alreadyLoggedIn) {
        UsageJournal::append(QStringLiteral("Вход пользователя %1 Количество попыток входа: %2.")
                                 .arg(journalUserName(user))
                                 .arg(qMax(1, m_loginAttempts)));
    }
    m_loginAttempts = 0;
}

void SunriseWindow::endUserSession() {
    if (!m_session.has_value()) {
        return;
    }
    UsageJournal::append(QStringLiteral("Выход пользователя %1").arg(journalUserName(*m_session)));
    m_session.reset();
}

void SunriseWindow::enterAsManagedUser() {
    const QList<UserRecord> users = m_repository.fetchUsers();
    if (users.isEmpty()) {
        CustomMessageBox::showWarning(this, QStringLiteral("Вначале добавьте пользователя"));
        return;
    }

    if (m_session.has_value()) {
        refreshPatients();
        setScreen(ScreenMode::Patients);
        return;
    }

    QString login = m_loginEdit ? m_loginEdit->text().trimmed() : QString();
    QString password = m_passwordEdit ? m_passwordEdit->text() : QString();
    const UserRecord &lastUser = users.last();

    if (login.isEmpty()) {
        login = lastUser.login;
    }
    if (password.isEmpty()) {
        if (m_lastManagedUserLogin == login) {
            password = m_lastManagedUserPassword;
        } else if (m_lastManagedUserId == lastUser.id) {
            login = m_lastManagedUserLogin.isEmpty() ? lastUser.login : m_lastManagedUserLogin;
            password = m_lastManagedUserPassword;
        }
    }

    if (login.isEmpty()) {
        login = lastUser.login;
    }
    if (password.isEmpty()) {
        CustomMessageBox::showWarning(this, QStringLiteral("Введите пароль пользователя для входа."));
        return;
    }

    ++m_loginAttempts;
    const auto user = m_repository.login(login, password);
    if (!user.has_value()) {
        CustomMessageBox::showError(this, QStringLiteral("Неверный логин или пароль!"));
        return;
    }
    if (user->role != QStringLiteral("Администратор") && user->role != QStringLiteral("Специалист")) {
        CustomMessageBox::showError(this, QStringLiteral("Недостаточный уровень доступа."));
        return;
    }

    startUserSession(*user);
    m_mainId = user->id;
    rememberManagedUser(user->id, user->login, password);
    refreshPatients();
    setScreen(ScreenMode::Patients);
}

void SunriseWindow::resetUserCreateForm() {
    m_editUserId.clear();
    m_adminLabel1->setText("Создать пользователя");
    m_userFio->clear();
    m_userLogin->clear();
    m_userPass->clear();
    m_userPass2->clear();
    m_userPass->setEchoMode(QLineEdit::Password);
    m_userPass2->setEchoMode(QLineEdit::Password);
    setImage(m_adminEye1, "pon.png");
    setImage(m_adminEye2, "pon.png");
    m_userFioClear->hide();
    m_userLoginClear->hide();
    m_userPassClear->hide();
    m_userPass2Clear->hide();
}

void SunriseWindow::loadUserForEdit(const QString &id) {
    if (!m_editUserId.isEmpty()) {
        m_editUserId.clear();
        m_adminLabel1->setText("Создать пользователя");
        m_userFio->clear();
        m_userPass->clear();
        m_userPass2->clear();
        return;
    }
    const auto user = m_repository.fetchUserById(id);
    if (!user.has_value()) {
        return;
    }
    m_editUserId = id;
    m_adminLabel1->setText("Изменить данные пользователя");
    if (m_lastManagedUserId != id) {
        m_lastManagedUserPassword.clear();
    }
    m_lastManagedUserId = id;
    m_lastManagedUserLogin = user->login;
    m_userFio->setText(user->fio);
    m_userLogin->setText(user->login);
    m_userRole->setCurrentText(user->role);
    m_userPass->clear();
    m_userPass2->clear();
    m_userPass->setEchoMode(QLineEdit::Password);
    m_userPass2->setEchoMode(QLineEdit::Password);
    setImage(m_adminEye1, "pon.png");
    setImage(m_adminEye2, "pon.png");
    m_userFioClear->hide();
    m_userLoginClear->hide();
    m_userPassClear->hide();
    m_userPass2Clear->hide();
}

void SunriseWindow::deleteUserById(const QString &userId) {
    if (!CustomMessageBox::askConfirm(this, "Внимание! Вы собираетесь удалить данные\nЭто действие необратимо!")) {
        return;
    }
    QString err;
    if (!m_repository.deleteUser(userId, &err)) {
        CustomMessageBox::showError(this, err);
        return;
    }
    if (m_editUserId == userId) {
        resetUserCreateForm();
    }
    scheduleRefreshUsers();
}

void SunriseWindow::handlePatientsTableClick(int row, int column) {
    if (row < 0 || !m_patientsTable->item(row, 0)) {
        return;
    }
    if (column == 1) {
        openPatientFromTable();
        return;
    }
    if (column == 4) {
        const QString patientId = m_patientsTable->item(row, 0)->text();
        if (!CustomMessageBox::askConfirm(this, "Внимание! Вы собираетесь удалить данные\nЭто действие необратимо!")) {
            return;
        }
        QString err;
        if (!m_repository.deletePatient(patientId, &err)) {
            CustomMessageBox::showError(this, err);
            return;
        }
        if (m_currentPatientId == patientId) {
            m_currentPatientId.clear();
        }
        refreshPatients();
    }
}

void SunriseWindow::scheduleRefreshUsers() {
    QTimer::singleShot(0, this, [this]() { refreshUsers(); });
}

void SunriseWindow::refreshUsers() {
    if (!m_usersTable) {
        return;
    }

    const QList<UserRecord> users = m_repository.fetchUsers();
    constexpr int kActionBtnSize = 30;
    const QSize actionIconSize(kActionBtnSize, kActionBtnSize);
    const QPixmap editPixmap = tintedPixmap(imagePath("edit.png"), Qt::white, actionIconSize);
    const QPixmap deletePixmap = tintedPixmap(imagePath("delete.png"), QColor(255, 70, 70), actionIconSize);

    const int oldRows = m_usersTable->rowCount();
    for (int i = 0; i < oldRows; ++i) {
        if (QWidget *widget = m_usersTable->cellWidget(i, 3)) {
            m_usersTable->removeCellWidget(i, 3);
            widget->deleteLater();
        }
    }
    m_usersTable->setRowCount(0);
    m_usersTable->setRowCount(users.size());
    for (int i = 0; i < users.size(); ++i) {
        const UserRecord &user = users.at(i);
        auto makeTextItem = [](const QString &text) {
            auto *item = new QTableWidgetItem(text);
            item->setForeground(QBrush(Qt::white));
            return item;
        };
        const QString userId = user.id;
        m_usersTable->setItem(i, 0, makeTextItem(userId));
        m_usersTable->setItem(i, 1, makeTextItem(user.fio));
        m_usersTable->setItem(i, 2, makeTextItem(user.login));

        auto *roleCell = new QWidget(m_usersTable);
        roleCell->setAttribute(Qt::WA_StyledBackground, true);
        roleCell->setStyleSheet("background: transparent;");
        auto *layout = new QHBoxLayout(roleCell);
        layout->setContentsMargins(8, 2, 8, 2);
        layout->setSpacing(6);

        auto *roleLabel = new QLabel(user.role, roleCell);
        roleLabel->setStyleSheet("color: white; background: transparent; font-family: 'Tahoma'; font-size: 12pt;");
        layout->addWidget(roleLabel, 1);

        auto *editBtn = new ImageButton(roleCell);
        editBtn->setFixedSize(kActionBtnSize, kActionBtnSize);
        editBtn->setCursor(Qt::PointingHandCursor);
        editBtn->setToolTip(QStringLiteral("Редактировать данные пользователя"));
        if (!editPixmap.isNull()) {
            editBtn->setPixmap(editPixmap);
        }
        connect(editBtn, &ImageButton::clicked, this, [this, userId]() {
            loadUserForEdit(userId);
        });
        layout->addWidget(editBtn, 0, Qt::AlignVCenter);

        auto *deleteBtn = new ImageButton(roleCell);
        deleteBtn->setFixedSize(kActionBtnSize, kActionBtnSize);
        deleteBtn->setCursor(Qt::PointingHandCursor);
        deleteBtn->setToolTip(QStringLiteral("Удалить пользователя"));
        if (!deletePixmap.isNull()) {
            deleteBtn->setPixmap(deletePixmap);
        }
        connect(deleteBtn, &ImageButton::clicked, this, [this, userId]() {
            deleteUserById(userId);
        });
        layout->addWidget(deleteBtn, 0, Qt::AlignVCenter);

        auto *placeholder = new QTableWidgetItem;
        placeholder->setFlags(Qt::NoItemFlags);
        m_usersTable->setItem(i, 3, placeholder);
        m_usersTable->setCellWidget(i, 3, roleCell);
    }
    m_usersTable->setColumnWidth(1, 180);
    m_usersTable->setColumnWidth(2, 120);
    m_usersTable->setColumnWidth(3, 236);
    styleUsersTable();
    fitUsersTableToContent();
    applyTruncatedTextTooltips(m_usersTable, {1, 2});
    m_usersTable->viewport()->update();
}

void SunriseWindow::styleUsersTable() {
    m_usersTable->setStyleSheet(
        "QTableWidget {"
        "  background: transparent;"
        "  alternate-background-color: transparent;"
        "  color: white;"
        "  border: 1px solid white;"
        "  gridline-color: white;"
        "  selection-background-color: rgba(255, 255, 255, 35);"
        "  selection-color: white;"
        "  font-family: 'Tahoma';"
        "  font-size: 12pt;"
        "  outline: none;"
        "}"
        "QTableWidget::item {"
        "  background: transparent;"
        "  color: white;"
        "  border: none;"
        "  padding: 4px 8px;"
        "}"
        "QHeaderView {"
        "  background: transparent;"
        "  border: none;"
        "}"
        "QHeaderView::section {"
        "  background-color: transparent;"
        "  color: white;"
        "  font-family: 'Tahoma';"
        "  font-size: 12pt;"
        "  font-style: italic;"
        "  font-weight: normal;"
        "  border: none;"
        "  border-right: 1px solid white;"
        "  border-bottom: 1px solid white;"
        "  padding: 6px 8px;"
        "}"
    );

    m_usersTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Fixed);
    m_usersTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Fixed);
    m_usersTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Fixed);

    QPalette tablePalette = m_usersTable->palette();
    tablePalette.setColor(QPalette::Base, Qt::transparent);
    tablePalette.setColor(QPalette::AlternateBase, Qt::transparent);
    tablePalette.setColor(QPalette::Text, Qt::white);
    tablePalette.setColor(QPalette::WindowText, Qt::white);
    m_usersTable->setPalette(tablePalette);
    if (m_usersTable->viewport()) {
        m_usersTable->viewport()->setAutoFillBackground(false);
    }

    m_usersTable->setShowGrid(false);
    m_usersTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_usersTable->setFocusPolicy(Qt::NoFocus);
    m_usersTable->verticalHeader()->setVisible(false);
    m_usersTable->verticalHeader()->setDefaultSectionSize(40);
    m_usersTable->horizontalHeader()->setVisible(true);
    m_usersTable->horizontalHeader()->setHighlightSections(false);
    m_usersTable->horizontalHeader()->setStretchLastSection(false);
    m_usersTable->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);
}

void SunriseWindow::fitUsersTableToContent() {
    if (!m_usersTable) {
        return;
    }
    m_usersTable->ensurePolished();
    m_usersTable->horizontalHeader()->ensurePolished();
    const int headerH = qMax(m_usersTable->horizontalHeader()->height(), m_usersTable->horizontalHeader()->sizeHint().height());
    const int rowH = m_usersTable->verticalHeader()->defaultSectionSize();
    const int rows = m_usersTable->rowCount();
    const int border = 2;
    const int contentH = headerH + rows * rowH + border;
    const int maxH = m_panelAdmin->height() - m_usersTable->y() - 10;
    m_usersTable->setVerticalScrollBarPolicy(contentH > maxH ? Qt::ScrollBarAsNeeded : Qt::ScrollBarAlwaysOff);
    m_usersTable->setFixedHeight(qMin(contentH, maxH));
}

void SunriseWindow::refreshPatients() {
    QList<PatientRecord> patients = m_repository.fetchPatients(
        m_patientSearch->text(),
        m_dateFilter->isChecked(),
        m_dateFrom->date(),
        m_dateTo->date()
    );

    if (m_patientSortColumn >= 1 && m_patientSortColumn <= 3) {
        const auto field = [this](const PatientRecord &p) {
            return m_patientSortColumn == 1 ? p.fio : (m_patientSortColumn == 2 ? p.birthDate : p.visitDate);
        };
        const auto compare = [this, field](const PatientRecord &a, const PatientRecord &b) {
            const QString va = field(a).trimmed();
            const QString vb = field(b).trimmed();
            if (m_patientSortColumn != 1) {
                const QDate da = QDate::fromString(va, "dd.MM.yyyy");
                const QDate db = QDate::fromString(vb, "dd.MM.yyyy");
                if (da.isValid() && db.isValid()) {
                    return da < db ? -1 : (db < da ? 1 : 0);
                }
            }
            return va.localeAwareCompare(vb);
        };
        std::stable_sort(patients.begin(), patients.end(), [this, compare](const PatientRecord &a, const PatientRecord &b) {
            const int cmp = compare(a, b);
            return m_patientSortAscending ? cmp < 0 : cmp > 0;
        });
    }

    const QPixmap deletePixmap(imagePath("delete.png"));
    int currentRow = patients.isEmpty() ? -1 : 0;
    m_patientsTable->clearContents();
    m_patientsTable->setRowCount(patients.size());
    for (int i = 0; i < patients.size(); ++i) {
        const PatientRecord &p = patients.at(i);
        m_patientsTable->setItem(i, 0, new QTableWidgetItem(p.id));
        m_patientsTable->setItem(i, 1, new QTableWidgetItem(p.fio));
        m_patientsTable->setItem(i, 2, new QTableWidgetItem(p.birthDate));
        m_patientsTable->setItem(i, 3, new QTableWidgetItem(p.visitDate));
        auto *deleteItem = new QTableWidgetItem;
        deleteItem->setData(Qt::DecorationRole, deletePixmap);
        deleteItem->setToolTip(QStringLiteral("Удалить пользователя"));
        m_patientsTable->setItem(i, 4, deleteItem);
        if (!m_selectedPatientRowId.isEmpty() && p.id == m_selectedPatientRowId) {
            currentRow = i;
        }
    }
    m_patientsTable->setColumnWidth(1, 280);
    m_patientsTable->setColumnWidth(2, 115);
    m_patientsTable->setColumnWidth(3, 100);
    m_patientsTable->setColumnWidth(4, 28);

    auto *header = static_cast<LegacyGridHeader *>(m_patientsTable->horizontalHeader());
    header->setSortGlyph(m_patientSortColumn, m_patientSortAscending ? Qt::AscendingOrder : Qt::DescendingOrder);
    header->updateHeight();
    applyTruncatedTextTooltips(m_patientsTable, {1, 2, 3});

    if (currentRow >= 0) {
        m_patientsTable->setCurrentCell(currentRow, 1);
    }
    fitPatientsTableToContent();
    m_patientsTable->viewport()->update();
}

void SunriseWindow::handlePatientsHeaderClick(int section) {
    if (section < 1 || section > 3) {
        return;
    }
    if (m_patientSortColumn == section) {
        m_patientSortAscending = !m_patientSortAscending;
    } else {
        m_patientSortColumn = section;
        m_patientSortAscending = true;
    }
    refreshPatients();
}

void SunriseWindow::refreshTemplateNames() {
    if (!m_templates) {
        return;
    }
    const QString current = m_templates->currentText().trimmed();
    const QSignalBlocker blocker(m_templates);
    m_templates->clear();
    m_templates->addItems(m_repository.loadTemplateNames());
    const int idx = m_templates->findText(current, Qt::MatchExactly);
    if (idx >= 0) {
        m_templates->setCurrentIndex(idx);
    } else if (!current.isEmpty()) {
        m_templates->setEditText(current);
    }
}

QString SunriseWindow::anamnesisTemplatePayload() const {
    if (!m_anamnesisEdit) {
        return {};
    }
    return m_anamnesisEdit->toHtml();
}

void SunriseWindow::loadAnamnesisTemplateByName(const QString &name) {
    if (name.isEmpty() || m_suppressTemplateLoad) {
        return;
    }
    writeProfileConfig(name, m_fontSlider ? m_fontSlider->value() : 24);
    if (name == QStringLiteral("Стандартный")) {
        loadDefaultAnamnesisTemplate();
        return;
    }
    const QString templateData = m_repository.loadTemplate(name);
    if (templateData.trimmed().isEmpty()) {
        return;
    }
    applyAnamnesisDocument(templateData);
}

void SunriseWindow::saveCurrentAnamnesisTemplate() {
    if (!m_templates || !m_anamnesisEdit) {
        return;
    }
    const QString name = m_templates->currentText().trimmed();
    if (name.isEmpty()) {
        CustomMessageBox::showWarning(this, QStringLiteral("Введите название шаблона."));
        return;
    }
    if (name == QStringLiteral("Стандартный")) {
        CustomMessageBox::showWarning(this, QStringLiteral("Шаблон «Стандартный» нельзя перезаписать."));
        return;
    }
    const int fontSize = m_fontSlider ? m_fontSlider->value() : 24;
    prepareAnamnesisDocumentForOutput();
    QString err;
    if (!m_repository.saveTemplate(name, fontSize, anamnesisTemplatePayload(), &err)) {
        CustomMessageBox::showError(this, err);
        return;
    }
    writeProfileConfig(name, fontSize);
    refreshTemplateNames();
    {
        QSignalBlocker blocker(m_templates);
        const int idx = m_templates->findText(name, Qt::MatchExactly);
        if (idx >= 0) {
            m_templates->setCurrentIndex(idx);
        } else {
            m_templates->addItem(name);
            m_templates->setCurrentIndex(m_templates->findText(name, Qt::MatchExactly));
        }
    }
    hideSlidePanels();
}

void SunriseWindow::syncTemplateSelectorFromProfile() {
    if (!m_templates) {
        return;
    }
    QString profileName = QStringLiteral("Стандартный");
    int fontSize = 24;
    const QString configPath = profileConfigPath();
    if (!configPath.isEmpty()) {
        QFile configFile(configPath);
        if (configFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            const QStringList parts = QString::fromUtf8(configFile.readAll()).trimmed().split(';');
            if (!parts.isEmpty() && !parts.first().trimmed().isEmpty()) {
                profileName = parts.first().trimmed();
            }
            if (parts.size() > 1) {
                fontSize = parts.at(1).toInt();
            }
        }
    }
    m_suppressTemplateLoad = true;
    const QSignalBlocker blocker(m_templates);
    const int idx = m_templates->findText(profileName, Qt::MatchExactly);
    if (idx >= 0) {
        m_templates->setCurrentIndex(idx);
    } else {
        m_templates->setEditText(profileName);
    }
    if (m_fontSlider) {
        QSignalBlocker sliderBlocker(m_fontSlider);
        m_fontSlider->setValue(fontSize);
    }
    if (m_fontSizeLabel) {
        m_fontSizeLabel->setText(QString::number(fontSize));
    }
    if (m_currentScreen == ScreenMode::Anamnesis && m_anamnesisEdit) {
        changeDocumentFontSize(fontSize, false);
    }
    m_suppressTemplateLoad = false;
}

void SunriseWindow::openPatientFromTable() {
    const int row = m_patientsTable->currentRow();
    if (row < 0) {
        return;
    }
    const QString patientId = m_patientsTable->item(row, 0)->text();
    QString err;
    if (!m_repository.verifyPatientAccess(patientId, m_licenseKey, &err)) {
        CustomMessageBox::showError(this, err);
        return;
    }
    const QString fio = m_patientsTable->item(row, 1)->text();
    const QString dr = m_patientsTable->item(row, 2)->text();
    m_currentPatientId = patientId;
    m_selectedPatientRowId = patientId;
    m_patientTitle->setText(fio);
    applyAnamnesisDocument(m_repository.loadPatientAnamnesis(patientId));
    if (m_fontSlider) {
        changeDocumentFontSize(m_fontSlider->value(), false);
    }
    setScreen(ScreenMode::Anamnesis);
    loadSummaryForCurrentPatient();
}

void SunriseWindow::updatePatientTitleFromDocument() {
    if (!m_anamnesisEdit || !m_patientTitle || m_currentScreen != ScreenMode::Anamnesis) {
        return;
    }
    QString fio;
    QString dr;
    m_repository.extractPatientFields(m_anamnesisEdit->toPlainText(), &fio, &dr);
    Q_UNUSED(dr);
    const QString trimmedFio = fio.trimmed();
    if (trimmedFio.size() > 3) {
        m_patientTitle->setText(trimmedFio);
    } else if (m_currentPatientId.isEmpty()) {
        m_patientTitle->setText(QStringLiteral("Новая карта"));
    }
}

void SunriseWindow::setAnamnesisDbControlsEnabled(bool enabled) {
    const bool allow = enabled && !m_anamnesisSaveInProgress;
    if (m_bList) {
        m_bList->setEnabled(allow);
    }
    if (m_bBack) {
        m_bBack->setEnabled(allow);
    }
    if (m_pAna) {
        m_pAna->setEnabled(allow);
    }
    if (m_pRisk) {
        m_pRisk->setEnabled(allow);
    }
    if (m_pCorr) {
        m_pCorr->setEnabled(allow);
    }
    if (m_pDih) {
        m_pDih->setEnabled(allow);
    }
    if (m_pMp) {
        m_pMp->setEnabled(allow);
    }
    if (m_addPatient) {
        m_addPatient->setEnabled(allow);
    }
    if (m_bExit) {
        m_bExit->setEnabled(allow);
    }
}

void SunriseWindow::tryAutoSaveAnamnesis(bool forceRefreshPatients) {
    if (m_anamnesisSaveInProgress) {
        return;
    }
    if (m_currentScreen != ScreenMode::Anamnesis || !m_anamnesisEdit) {
        return;
    }

    m_anamnesisSaveInProgress = true;
    setAnamnesisDbControlsEnabled(false);

#ifndef Q_OS_WIN
    prepareAnamnesisDocumentForOutput();
#else
    // На Windows prepareAnamnesisDocumentForOutput() делает setHtml/RTF-reload
    // и сбрасывает курсор в начало — при автосохранении во время редактирования нельзя.
#endif
    QString patientId = m_currentPatientId;
    QString fio;
    QString dr;
    QString err;
    const QString plainText = m_anamnesisEdit->toPlainText();
    const bool saved = m_repository.savePatientAnamnesis(
        &patientId,
        m_licenseKey,
        plainText,
        m_anamnesisEdit->toHtml(),
        &fio,
        &dr,
        &err);

    if (saved) {
        m_currentPatientId = patientId;
        if (!fio.trimmed().isEmpty()) {
            m_patientTitle->setText(fio.trimmed());
            m_selectedPatientRowId = patientId;
        }
        if (m_summaryPanel) {
            QString diag;
            QString visit;
            m_repository.extractPatientFields(plainText, nullptr, nullptr, &diag, &visit);
            Q_UNUSED(visit);
            m_summaryPanel->setPatientHeader(fio.trimmed(), dr.trimmed(), diag.trimmed());
        }
        tryAutoSaveProgram(true);
        if (forceRefreshPatients) {
            refreshPatients();
        }
    }

    m_anamnesisSaveInProgress = false;
    setAnamnesisDbControlsEnabled(true);
}

void SunriseWindow::saveAnamnesisToDb() {
    if (m_anamnesisSaveInProgress) {
        return;
    }
    m_anamnesisSaveInProgress = true;
    setAnamnesisDbControlsEnabled(false);

    prepareAnamnesisDocumentForOutput();
    QString patientId = m_currentPatientId;
    QString fio;
    QString dr;
    QString err;
    if (!m_repository.savePatientAnamnesis(
            &patientId,
            m_licenseKey,
            m_anamnesisEdit->toPlainText(),
            m_anamnesisEdit->toHtml(),
            &fio,
            &dr,
            &err)) {
        m_anamnesisSaveInProgress = false;
        setAnamnesisDbControlsEnabled(true);
        CustomMessageBox::showError(this, err);
        return;
    }
    m_currentPatientId = patientId;
    m_patientTitle->setText(fio.trimmed());
    if (m_summaryPanel) {
        QString diag;
        m_repository.extractPatientFields(m_anamnesisEdit->toPlainText(), nullptr, &dr, &diag, nullptr);
        m_summaryPanel->setPatientHeader(fio.trimmed(), dr.trimmed(), diag.trimmed());
    }
    tryAutoSaveProgram(true);
    refreshPatients();
    m_anamnesisSaveInProgress = false;
    setAnamnesisDbControlsEnabled(true);
    CustomMessageBox::showInfo(this, "Сохранено.");
}

bool SunriseWindow::eventFilter(QObject *watched, QEvent *event) {
    if (watched == m_anamnesisEdit && event->type() == QEvent::FocusOut && !m_screenTransitionGuard) {
        tryAutoSaveAnamnesis();
    }

    if (watched == m_root && event->type() == QEvent::MouseButtonPress) {
        const auto *mouseEvent = static_cast<QMouseEvent *>(event);
        const QPoint localPos = m_root->mapFromGlobal(mouseEvent->globalPos());
        auto outsidePanel = [&](QWidget *panel) {
            return panel && panel->isVisible() && !panel->geometry().contains(localPos);
        };
        if (outsidePanel(m_settingsPanel) && outsidePanel(m_printPanel)) {
            hideSlidePanels();
        }
        if (m_infoPopup && m_infoPopup->isVisible()) {
            const QPoint local = m_infoPopup->mapFromGlobal(mouseEvent->globalPos());
            if (!m_infoPopup->rect().contains(local)) {
                m_infoPopup->hide();
            }
        }
    }
    return QMainWindow::eventFilter(watched, event);
}

void SunriseWindow::toggleSlidePanel(QWidget *panel) {
    if (!panel) {
        return;
    }
    if (panel->isVisible() && panel->y() >= 60) {
        animateSlidePanel(panel, false);
        m_activeSlidePanel = nullptr;
        return;
    }
    hideSlidePanels();
    animateSlidePanel(panel, true);
    m_activeSlidePanel = panel;
}

void SunriseWindow::hideSlidePanels() {
    for (QWidget *panel : {m_settingsPanel, m_printPanel}) {
        if (panel && panel->isVisible()) {
            panel->hide();
        }
    }
    m_activeSlidePanel = nullptr;
    showSettingsSaveTemplateView(false);
}

void SunriseWindow::animateSlidePanel(QWidget *panel, bool showPanel) {
    if (!panel) {
        return;
    }
    if (!showPanel) {
        panel->hide();
        return;
    }
    panel->show();
    panel->raise();
    // tpopup_Tick: шаг 15 px от -150; pprofile останавливается на 60, pprint — на 75.
    const int finalTop = panel == m_settingsPanel ? 60 : 75;
    auto *animation = new QPropertyAnimation(panel, "pos", panel);
    animation->setDuration(150);
    animation->setStartValue(QPoint(panel->x(), -panel->height()));
    animation->setEndValue(QPoint(panel->x(), finalTop));
    animation->start(QAbstractAnimation::DeleteWhenStopped);
}

void SunriseWindow::showSettingsSaveTemplateView(bool show) {
    if (!m_settingsMainView || !m_settingsSaveView) {
        return;
    }
    m_settingsTemplateMode = show;
    m_settingsMainView->setVisible(!show);
    m_settingsSaveView->setVisible(show);
    if (show) {
        m_templateNameEdit->clear();
    }
}

void SunriseWindow::installToolbarTooltips() {
    const QList<QPair<QWidget *, QString>> tooltips = {
        {m_loginButton, QStringLiteral("Вход в программу")},
        {m_adminButton, QStringLiteral("Вход на страницу администрирования")},
        {m_bPicPrint, QStringLiteral("Печать")},
        {m_bUpload, QStringLiteral("Загрузить файл")},
        {m_bSettings, QStringLiteral("Настройки")},
        {m_bPrint, QStringLiteral("Печать")},
        {m_bSave, QStringLiteral("Сохранить")},
        {m_bExit, QStringLiteral("Выход")},
        {m_boldButton, QStringLiteral("Жирный")},
        {m_underlineButton, QStringLiteral("Подчеркивание")},
        {m_bInfo, QStringLiteral("Руководство пользователя и информация о программе")},
    };
    for (const auto &entry : tooltips) {
        if (entry.first) {
            entry.first->setToolTip(entry.second);
        }
    }
}

void SunriseWindow::updateFormatButtonIcons() {
    if (!m_anamnesisEdit) {
        return;
    }
    const bool underline = m_anamnesisEdit->currentCharFormat().fontUnderline();
    const bool bold = m_anamnesisEdit->currentCharFormat().fontWeight() >= QFont::Bold;
    m_underlineActive = underline;
    m_boldActive = bold;

    auto applyImage = [](ImageButton *button, const QStringList &candidates) {
        for (const QString &file : candidates) {
            if (!file.isEmpty() && QFile::exists(file)) {
                button->setImagePath(file);
                return;
            }
        }
    };
    applyImage(m_underlineButton, {
        resourcePath(underline ? QStringLiteral("Подчеркивание нажата.png") : QStringLiteral("Подчеркивание.png"))
    });
    applyImage(m_boldButton, {
        resourcePath(bold ? QStringLiteral("Жирный шрифт Нажата.png") : QStringLiteral("Жирный шрифт.png"))
    });
}

void SunriseWindow::changeDocumentFontSize(int pointSize, bool persistProfile) {
    if (!m_anamnesisEdit || pointSize <= 0) {
        return;
    }
    if (m_fontSizeLabel) {
        m_fontSizeLabel->setText(QString::number(pointSize));
    }

    bool reloadedFromRtf = false;
#ifndef Q_OS_WIN
    m_lastAnamnesisRtf.clear();
    applyAnamnesisFontToEntireDocument(pointSize);
#else
    const QTextCursor savedCursor = m_anamnesisEdit->textCursor();
    const int savedScroll = m_anamnesisEdit->verticalScrollBar()
        ? m_anamnesisEdit->verticalScrollBar()->value()
        : 0;
    const int savedPos = savedCursor.position();
    const int savedAnchor = savedCursor.anchor();

    if (!m_lastAnamnesisRtf.isEmpty()) {
        QString rtf = QString::fromLatin1(m_lastAnamnesisRtf);
        for (int size = 21; size <= 28; ++size) {
            rtf.replace(QStringLiteral("fs%1").arg(size), QStringLiteral("fs%1").arg(pointSize));
        }
        m_lastAnamnesisRtf = rtf.toLatin1();
        loadAnamnesisRtf(m_lastAnamnesisRtf);
        reloadedFromRtf = true;
    }
    if (!reloadedFromRtf) {
        QString html = m_anamnesisEdit->toHtml();
        for (int size = 21; size <= 28; ++size) {
            html.replace(QStringLiteral("fs%1").arg(size), QStringLiteral("fs%1").arg(pointSize));
            const QString oldPt = QString::number(size / 2.0, 'f', 1);
            const QString newPt = QString::number(pointSize / 2.0, 'f', 1);
            html.replace(
                QStringLiteral("font-size:%1pt").arg(oldPt),
                QStringLiteral("font-size:%1pt").arg(newPt)
            );
        }
        m_anamnesisEdit->setHtml(html);
    }

    applyAnamnesisFont(pointSize);
    applyCompactAnamnesisLineSpacing();

    // Восстановить позицию после setHtml / RTF paste (иначе каретка в начале).
    QTextCursor restored(m_anamnesisEdit->document());
    const int docEnd = m_anamnesisEdit->document()->characterCount() - 1;
    const int pos = qBound(0, savedPos, qMax(0, docEnd));
    const int anchor = qBound(0, savedAnchor, qMax(0, docEnd));
    restored.setPosition(anchor);
    restored.setPosition(pos, QTextCursor::KeepAnchor);
    m_anamnesisEdit->setTextCursor(restored);
    if (m_anamnesisEdit->verticalScrollBar()) {
        m_anamnesisEdit->verticalScrollBar()->setValue(savedScroll);
    }
#endif
    if (persistProfile && m_templates) {
        writeProfileConfig(m_templates->currentText(), pointSize);
    }
}

void SunriseWindow::prepareAnamnesisDocumentForOutput() {
    if (!m_anamnesisEdit) {
        return;
    }
    const int fontSize = m_fontSlider ? m_fontSlider->value() : 24;
    changeDocumentFontSize(fontSize, false);
}

void SunriseWindow::writeProfileConfig(const QString &profileName, int fontSize) {
    const QString path = profileConfigPath();
    if (path.isEmpty()) {
        return;
    }
    QFile file(path);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        file.write(QString("%1;%2\n").arg(profileName).arg(fontSize).toUtf8());
    }
}

void SunriseWindow::setWorkChromeVisible(bool visible) {
    const bool showSettings = visible && m_currentScreen == ScreenMode::Anamnesis;
    const bool showSavePrint = visible && isWorkScreen(m_currentScreen);
    const QWidgetList chromeWidgets = {
        m_bList, m_bExit, m_bInfo,
        m_pAna, m_pRisk, m_pCorr, m_pUpr, m_pDih, m_pMp, m_patientTitle, m_userOpenPatients
    };
    for (QWidget *widget : chromeWidgets) {
        if (widget) {
            widget->setVisible(visible);
        }
    }
    if (m_bSave) {
        m_bSave->setVisible(showSavePrint);
    }
    if (m_bPrint) {
        m_bPrint->setVisible(showSavePrint);
    }
    if (!showSavePrint && m_printPanel) {
        m_printPanel->hide();
    }
    if (m_bSettings) {
        m_bSettings->setVisible(showSettings);
    }
    if (m_bClose) {
        m_bClose->setVisible(visible);
    }
    if (m_bLine) {
        m_bLine->setVisible(visible);
    }
    if (m_bUp) {
        m_bUp->setVisible(visible);
    }
    if (visible) {
        raiseChromeWidgets();
    }
}

void SunriseWindow::raiseChromeWidgets() {
    const QWidgetList chromeWidgets = {
        m_pAna, m_pRisk, m_pCorr, m_pUpr, m_pDih, m_pMp, m_workStack,
        m_bBack, m_bList, m_bExit, m_bSave, m_bPrint, m_bPicPrint, m_bUpload, m_bSettings, m_bInfo, m_bJournal,
        m_bUpdate, m_bClose, m_bLine, m_bUp, m_patientTitle, m_userOpenPatients, m_summaryPanel
    };
    for (QWidget *widget : chromeWidgets) {
        if (widget && widget->isVisible()) {
            widget->raise();
        }
    }
}


bool SunriseWindow::isWorkScreen(ScreenMode mode) {
    switch (mode) {
    case ScreenMode::Anamnesis:
    case ScreenMode::RiskAssessment:
    case ScreenMode::Correction:
    case ScreenMode::Speech:
    case ScreenMode::Breathing:
    case ScreenMode::Medication:
        return true;
    default:
        return false;
    }
}

void SunriseWindow::setupClinicalController() {
    m_clinical->setHtmlRoot(clinicalHtmlRoot());
    m_clinical->setSummary(m_summaryPanel);
    m_clinical->setImageLoader([this](const QString &name) { return imagePath(name); });
    connect(m_clinical, &ClinicalBrowserController::helpIndexChanged, this, [this](const QString &help) {
        if (isWorkScreen(m_currentScreen) && m_currentScreen != ScreenMode::Anamnesis) {
            m_helpIndex = help;
        }
    });
    connect(m_clinical, &ClinicalBrowserController::programChanged, this, [this]() { tryAutoSaveProgram(); });
}

QString SunriseWindow::clinicalHtmlRoot() const {
    const QString viaHtml = htmlPath(QStringLiteral("interface/title.html"));
    if (!viaHtml.isEmpty()) {
        return QDir::cleanPath(QFileInfo(viaHtml).absolutePath() + QStringLiteral("/.."));
    }
    return QCoreApplication::applicationDirPath() + QStringLiteral("/assets/htmls");
}

void SunriseWindow::layoutWorkAreaForMode(ScreenMode mode) {
    if (!m_workStack) {
        return;
    }
    // panel3 (анамнез) — 945,105; wpageManager — 947,105, высота 965 (968 на «Упражнениях»).
    if (mode == ScreenMode::Anamnesis) {
        m_workStack->setGeometry(945, 105, 965, 965);
    } else if (mode == ScreenMode::Speech) {
        m_workStack->setGeometry(947, 105, 966, 968);
    } else if (isWorkScreen(mode)) {
        m_workStack->setGeometry(947, 105, 966, 965);
    }
}

void SunriseWindow::loadSummaryForCurrentPatient() {
    if (!m_summaryPanel) {
        return;
    }
    if (m_currentPatientId.isEmpty()) {
        m_summaryPanel->resetDefaults();
        m_summaryPanel->setPatientHeader(
            m_patientTitle ? m_patientTitle->text() : QString(),
            {},
            {});
        return;
    }
    m_summaryPanel->loadSerialized(m_repository.loadPatientProgram(m_currentPatientId));
    QString fio;
    QString dr;
    QString diag;
    if (m_anamnesisEdit) {
        m_repository.extractPatientFields(m_anamnesisEdit->toPlainText(), &fio, &dr, &diag, nullptr);
    }
    if (fio.trimmed().isEmpty() && m_patientTitle) {
        fio = m_patientTitle->text();
    }
    if (diag.trimmed().isEmpty()) {
        diag = m_repository.loadPatientDiag(m_currentPatientId);
    }
    m_summaryPanel->setPatientHeader(fio.trimmed(), dr.trimmed(), diag.trimmed());
}

void SunriseWindow::tryAutoSaveProgram(bool force) {
    if (m_programSaveInProgress) {
        return;
    }
    if (!force && m_programSaveTimer && !m_programSaveTimer->isActive()) {
        // called from debounce signal — timer already fired
    }
    if (m_currentPatientId.isEmpty() || !m_summaryPanel) {
        return;
    }
    m_programSaveInProgress = true;
    QString err;
    const bool ok = m_repository.savePatientProgram(m_currentPatientId, m_summaryPanel->serialize(), &err);
    m_programSaveInProgress = false;
    if (!ok && !err.isEmpty()) {
        // Silent for debounce; avoid modal spam.
        Q_UNUSED(err);
    }
}

void SunriseWindow::openJournal() {
    const QString path = UsageJournal::filePath();
    if (!QFile::exists(path)) {
        CustomMessageBox::showWarning(this, QStringLiteral("Файл журнала aJournal.rtf не найден."));
        return;
    }
#ifdef Q_OS_WIN
    for (const QString &root : {qEnvironmentVariable("ProgramW6432"), qEnvironmentVariable("ProgramFiles")}) {
        const QString wordpad = root + QStringLiteral("/Windows NT/Accessories/wordpad.exe");
        if (!root.isEmpty() && QFile::exists(wordpad)
            && QProcess::startDetached(wordpad, {QDir::toNativeSeparators(path)})) {
            return;
        }
    }
#endif
    if (!QDesktopServices::openUrl(QUrl::fromLocalFile(path))) {
        auto *dlg = new QDialog(this);
        dlg->setWindowTitle(QStringLiteral("Журнал"));
        dlg->resize(800, 600);
        auto *edit = new QTextEdit(dlg);
        auto *layout = new QVBoxLayout(dlg);
        layout->addWidget(edit);
        QFile file(path);
        if (file.open(QIODevice::ReadOnly)) {
            edit->setHtml(QString::fromUtf8(file.readAll())); // RTF may show raw; best-effort
            file.seek(0);
            edit->document()->setPlainText(QString::fromLocal8Bit(file.readAll()));
        }
        dlg->exec();
        dlg->deleteLater();
    }
}

QString SunriseWindow::currentPatientBirthDate() const {
    if (!m_currentPatientId.isEmpty() && m_patientsTable) {
        for (int row = 0; row < m_patientsTable->rowCount(); ++row) {
            const QTableWidgetItem *idItem = m_patientsTable->item(row, 0);
            const QTableWidgetItem *birthItem = m_patientsTable->item(row, 2);
            if (idItem && birthItem && idItem->text() == m_currentPatientId) {
                return birthItem->text().trimmed();
            }
        }
    }
    QString birthDate;
    if (m_anamnesisEdit) {
        m_repository.extractPatientFields(m_anamnesisEdit->toPlainText(), nullptr, &birthDate);
    }
    return birthDate.trimmed();
}

QString SunriseWindow::assembleExportHtml(const ExportSelection &selection) {
    if (selection.anamnesis) {
        return m_anamnesisEdit ? m_anamnesisEdit->toHtml() : QString();
    }
    if (selection.program) {
        return programDocumentHtml(selection);
    }
    return {};
}

void SunriseWindow::renderExportToPrinter(
    QPrinter &printer, const ExportSelection &selection, const QString &assembledHtml) {
    if (selection.anamnesis && m_anamnesisEdit) {
        std::unique_ptr<QTextDocument> doc(m_anamnesisEdit->document()->clone());
        doc->print(&printer);
        return;
    }

    QTextDocument doc;
    doc.setHtml(assembledHtml);
    doc.print(&printer);
}

SunriseWindow::ExportSelection SunriseWindow::currentExportSelection() const {
    ExportSelection selection;
    selection.anamnesis = m_printAnamnesisRb && m_printAnamnesisRb->isChecked();
    selection.program = m_printProgramRb && m_printProgramRb->isChecked();
    selection.protocols = m_printProtocolsRb && m_printProtocolsRb->isChecked();
    for (const QCheckBox *check : m_printSectionChecks) {
        selection.programSections.append(check->isChecked());
    }
    return selection;
}

bool SunriseWindow::validateExportSelection(
    const ExportSelection &selection,
    const QString &emptyMessage) {
    if (!selection.anamnesis && !selection.program && !selection.protocols) {
        CustomMessageBox::showWarning(this, emptyMessage);
        return false;
    }
    if (selection.protocols) {
        CustomMessageBox::showInfo(this, QStringLiteral("Пока ни одного протокола не сформировано"));
        return false;
    }
    return true;
}

namespace {

struct ProgramSection {
    QString title;
    QString body;
};

QList<ProgramSection> collectProgramSections(const SummaryPanel *summary, const QList<bool> &enabled) {
    QList<ProgramSection> sections;
    if (!summary) {
        return sections;
    }
    const SummaryPanel::Field fields[] = {
        SummaryPanel::Risk, SummaryPanel::Corr, SummaryPanel::Speech,
        SummaryPanel::Dih, SummaryPanel::Mp, SummaryPanel::ExtraBody
    };
    for (int i = 0; i < 6; ++i) {
        if (i < enabled.size() && !enabled.at(i)) {
            continue;
        }
        const SummaryPanel::Field field = fields[i];
        sections.append({summary->sectionTitle(field),
                         summary->isPlaceholder(field) ? QString() : summary->fieldText(field)});
    }
    return sections;
}

QString rtfEscape(const QString &text) {
    QString out;
    for (const QChar ch : text) {
        const ushort code = ch.unicode();
        if (ch == QLatin1Char('\\') || ch == QLatin1Char('{') || ch == QLatin1Char('}')) {
            out += QLatin1Char('\\');
            out += ch;
        } else if (ch == QLatin1Char('\n')) {
            out += QStringLiteral("\\par ");
        } else if (ch == QLatin1Char('\r')) {
            continue;
        } else if (code < 0x80) {
            out += ch;
        } else {
            out += QStringLiteral("\\u%1?").arg(static_cast<short>(code));
        }
    }
    return out;
}

} // namespace

QString SunriseWindow::programDocumentHtml(const ExportSelection &selection) const {
    // prepareText(): заголовок, пациент/диагноз/дата рождения, затем отмеченные разделы.
    const auto esc = [](const QString &text) { return text.toHtmlEscaped().replace(QLatin1Char('\n'), QStringLiteral("<br/>")); };
    QString html = QStringLiteral(
        "<html><body style=\"font-family:Arial; font-size:10pt;\">"
        "<p><b>Программа индивидуализированной вторичной профилактики повторного сосудистого события</b></p><br/>");
    const QString fio = m_summaryPanel ? m_summaryPanel->patientFio() : QString();
    const QString diag = m_summaryPanel ? m_summaryPanel->patientDiag() : QString();
    const QString birthDate = m_summaryPanel ? m_summaryPanel->patientBirthDate() : QString();
    html += QStringLiteral("<p><b>Пациент:</b> %1</p><br/>").arg(esc(fio));
    html += QStringLiteral("<p><b>Диагноз:</b> %1</p><br/>").arg(esc(diag));
    html += QStringLiteral("<p><b>Дата рождения:</b> %1</p><br/>").arg(esc(birthDate));
    for (const ProgramSection &section : collectProgramSections(m_summaryPanel, selection.programSections)) {
        html += QStringLiteral("<p><b>%1</b></p><br/>").arg(esc(section.title));
        html += QStringLiteral("<p>%1</p><br/>").arg(esc(section.body));
    }
    html += QStringLiteral("</body></html>");
    return html;
}

QByteArray SunriseWindow::programDocumentRtf(const ExportSelection &selection) const {
    QString rtf = QStringLiteral(
        "{\\rtf1\\ansi\\ansicpg1251\\deff0\\deflang1049{\\fonttbl{\\f0\\fnil\\fcharset204 Arial;}}"
        "\\viewkind4\\uc1\\pard\\f0\\fs20 ");
    rtf += QStringLiteral("\\b %1\\b0 \\par\\par ")
               .arg(rtfEscape(QStringLiteral("Программа индивидуализированной вторичной профилактики повторного сосудистого события")));
    const QString fio = m_summaryPanel ? m_summaryPanel->patientFio() : QString();
    const QString diag = m_summaryPanel ? m_summaryPanel->patientDiag() : QString();
    const QString birthDate = m_summaryPanel ? m_summaryPanel->patientBirthDate() : QString();
    rtf += QStringLiteral("\\b %1\\b0 %2\\par\\par ").arg(rtfEscape(QStringLiteral("Пациент:")), rtfEscape(fio));
    rtf += QStringLiteral("\\b %1\\b0 %2\\par\\par ").arg(rtfEscape(QStringLiteral("Диагноз:")), rtfEscape(diag));
    rtf += QStringLiteral("\\b %1\\b0 %2\\par\\par ").arg(rtfEscape(QStringLiteral("Дата рождения:")), rtfEscape(birthDate));
    for (const ProgramSection &section : collectProgramSections(m_summaryPanel, selection.programSections)) {
        rtf += QStringLiteral("\\b %1\\b0 \\par\\par ").arg(rtfEscape(section.title));
        rtf += rtfEscape(section.body) + QStringLiteral("\\par\\par ");
    }
    rtf += QLatin1Char('}');
    return rtf.toLatin1();
}

void SunriseWindow::showPrintPanel(bool saveMode) {
    if (!m_printPanel) {
        return;
    }
    const bool sameModeOpen = m_printPanel->isVisible() && m_printPanelSaveMode == saveMode;
    // makeSave()/makeLoad(): заголовок и кнопка по центру панели.
    m_printPanelSaveMode = saveMode;
    m_printPanelTitle->setText(saveMode ? QStringLiteral("Сохранить") : QStringLiteral("Отправить на печать"));
    m_printPanelTitle->adjustSize();
    m_printPanelTitle->move((m_printPanel->width() - m_printPanelTitle->width()) / 2, 9);
    m_printPanelPrintButton->setVisible(!saveMode);
    m_printPanelSaveButton->setVisible(saveMode);
    ImageButton *action = saveMode ? m_printPanelSaveButton : m_printPanelPrintButton;
    action->move((m_printPanel->width() - action->width()) / 2, action->y());
    if (sameModeOpen) {
        toggleSlidePanel(m_printPanel);
        return;
    }
    hideSlidePanels();
    animateSlidePanel(m_printPanel, true);
    m_activeSlidePanel = m_printPanel;
}

void SunriseWindow::updatePrintPanelState(QRadioButton *changed) {
    if (!changed) {
        return;
    }
    // Переключатели popupprint в разных контейнерах — взаимоисключение вручную, как в radioButtonN_CheckedChanged.
    if (!changed->isChecked()) {
        const QSignalBlocker blocker(changed);
        changed->setChecked(true);
        return;
    }
    const auto uncheck = [](QRadioButton *radio) {
        const QSignalBlocker blocker(radio);
        radio->setChecked(false);
    };
    const auto setSectionsEnabled = [this](bool enabled) {
        for (QCheckBox *check : qAsConst(m_printSectionChecks)) {
            check->setEnabled(enabled);
            if (auto *label = qobject_cast<QWidget *>(check->property("sectionLabel").value<QObject *>())) {
                label->setEnabled(enabled);
            }
        }
    };
    const auto setAudienceEnabled = [this](bool enabled) {
        m_printForPatientRb->setEnabled(enabled);
        m_printForSpecialistRb->setEnabled(enabled);
    };
    const bool anamnesisWasChecked = changed != m_printAnamnesisRb && m_printAnamnesisRb->isChecked();
    if (changed == m_printAnamnesisRb) {
        uncheck(m_printProgramRb);
        uncheck(m_printProtocolsRb);
        setSectionsEnabled(false);
        setAudienceEnabled(false);
        return;
    }
    uncheck(m_printAnamnesisRb);
    if (anamnesisWasChecked) {
        m_printForPatientRb->setChecked(true);
    }
    if (changed == m_printProgramRb) {
        uncheck(m_printProtocolsRb);
        setSectionsEnabled(true);
        setAudienceEnabled(false);
    } else if (changed == m_printProtocolsRb) {
        uncheck(m_printProgramRb);
        setSectionsEnabled(false);
        setAudienceEnabled(true);
        if (!m_printForPatientRb->isChecked() && !m_printForSpecialistRb->isChecked()) {
            m_printForPatientRb->setChecked(true);
        }
    }
}

void SunriseWindow::exportDocument() {
    const ExportSelection selection = currentExportSelection();
    if (!validateExportSelection(
            selection,
            QStringLiteral("Выберите содержимое для сохранения."))) {
        return;
    }
    hideSlidePanels();

    QString suggested = m_patientTitle->text().trimmed();
    if (suggested.isEmpty() || suggested == QStringLiteral("Новая карта")) {
        suggested = QStringLiteral("Документ");
    }

    const QString homeDir = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
    const QString suggestedPath = QDir(homeDir).filePath(suggested + QStringLiteral(".pdf"));

    const QString path = QFileDialog::getSaveFileName(
        this,
        QStringLiteral("Сохранить как"),
        suggestedPath,
        QStringLiteral("PDF (*.pdf);;Word (*.doc)")
    );
    if (path.isEmpty()) {
        return;
    }

    if (selection.anamnesis) {
        prepareAnamnesisDocumentForOutput();
    }

    const QString content = assembleExportHtml(selection);
    if (content.trimmed().isEmpty()) {
        CustomMessageBox::showWarning(this, "Нет данных для сохранения.");
        return;
    }

    const bool onlyAnamnesis = selection.anamnesis;

    if (path.endsWith(".pdf", Qt::CaseInsensitive)) {
        QPrinter printer(QPrinter::HighResolution);
        printer.setOutputFormat(QPrinter::PdfFormat);
        printer.setOutputFileName(path);
        renderExportToPrinter(printer, selection, content);
        return;
    }

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        CustomMessageBox::showError(this, "Не удалось сохранить файл.");
        return;
    }
    if (selection.program) {
        file.write(programDocumentRtf(selection));
        return;
    }
    if (onlyAnamnesis && !m_lastAnamnesisRtf.isEmpty()) {
        file.write(m_lastAnamnesisRtf);
        return;
    }
    QString exportContent = content;
    file.write(exportContent.toUtf8());
}

void SunriseWindow::printSelectedContent() {
    const ExportSelection selection = currentExportSelection();
    if (!validateExportSelection(
            selection,
            QStringLiteral("Выберите содержимое для печати."))) {
        return;
    }
    hideSlidePanels();

    if (selection.anamnesis) {
        prepareAnamnesisDocumentForOutput();
    }

    const QString content = assembleExportHtml(selection);
    if (content.trimmed().isEmpty()) {
        CustomMessageBox::showWarning(this, "Нет данных для печати.");
        return;
    }

    QPrinter printer(QPrinter::HighResolution);
    QPrintDialog dialog(&printer, this);
    dialog.setWindowTitle(QStringLiteral("Печать"));
    if (dialog.exec() != QDialog::Accepted) {
        // assembleExportHtml мог сохранить протоколы — вернуть кликабельные ссылки.
        return;
    }

    renderExportToPrinter(printer, selection, content);
}

void SunriseWindow::showInfoPopup() {
    if (!m_infoPopup) {
        m_infoPopup = new QDialog(this, Qt::FramelessWindowHint);
        m_infoPopup->setFixedSize(447, 133);
        m_infoPopup->setAttribute(Qt::WA_TranslucentBackground, false);
        const QString bg = imagePath("popup.png");
        m_infoPopup->setStyleSheet("QDialog { background-image: url('" + bg + "'); }");

        auto createLabel = [&](const QString &text, const QRect &rect, auto callback) {
            QLabel *label = new QLabel(text, m_infoPopup);
            label->setGeometry(rect);
            label->setStyleSheet("color:black;background:transparent;font: 11.25pt 'Microsoft Sans Serif';");
            label->setCursor(Qt::PointingHandCursor);
            class ClickLabel final : public QLabel {
            public:
                using QLabel::QLabel;
                std::function<void()> onClick;
            protected:
                void mousePressEvent(QMouseEvent *event) override {
                    if (onClick) {
                        onClick();
                    }
                    QLabel::mousePressEvent(event);
                }
            };
            ClickLabel *clickLabel = new ClickLabel(m_infoPopup);
            clickLabel->setText(text);
            clickLabel->setGeometry(rect);
            clickLabel->setStyleSheet(label->styleSheet());
            clickLabel->setCursor(label->cursor());
            clickLabel->onClick = callback;
            label->deleteLater();
            return clickLabel;
        };

        createLabel("Руководство пользователя к данной странице", QRect(23, 23, 380, 18), [this]() {
            m_infoPopup->hide();
            const QString path = htmlPath("spravka/" + m_helpIndex);
            if (!path.isEmpty()) {
                showHelpWindow(path);
            }
        });
        createLabel("Общее руководство пользователя", QRect(23, 56, 300, 18), [this]() {
            m_infoPopup->hide();
            const QString path = htmlPath("spravka/руководствопользователя.htm");
            if (!path.isEmpty()) {
                showHelpWindow(path);
            }
        });
        createLabel("О программе", QRect(23, 89, 140, 18), [this]() {
            m_infoPopup->hide();
            showAboutWindow();
        });
    }

    QPoint p = mapToGlobal(QPoint(1400, -150));
    m_infoPopup->move(p);
    m_infoPopup->show();
    m_infoPopup->raise();
    auto *animation = new QPropertyAnimation(m_infoPopup, "pos", m_infoPopup);
    animation->setDuration(150);
    animation->setStartValue(p);
    animation->setEndValue(mapToGlobal(QPoint(1400, 75)));
    animation->start(QAbstractAnimation::DeleteWhenStopped);
}

void SunriseWindow::showHelpWindow(const QString &address, bool aboveModalDialog) {
    const auto resolveHelpLinkTarget = [this](const QUrl &link) -> QString {
        if (m_currentHelpFilePath.isEmpty()) {
            return {};
        }
        const QUrl baseUrl = QUrl::fromLocalFile(m_currentHelpFilePath);
        const QString path = baseUrl.resolved(link).toLocalFile();
        if (path.isEmpty()) {
            return {};
        }
        if (QFile::exists(path)) {
            return path;
        }
        const QDir dir(QFileInfo(path).absolutePath());
        const QString fileName = QFileInfo(path).fileName();
        for (const QString &entry : dir.entryList(QDir::Files)) {
            if (entry.compare(fileName, Qt::CaseInsensitive) == 0) {
                return dir.filePath(entry);
            }
        }
        return htmlPath(QStringLiteral("spravka/") + fileName);
    };

    const auto loadHelpPage = [this, resolveHelpLinkTarget](const QString &path) {
        if (!m_helpBrowser || path.isEmpty()) {
            return;
        }
        QString rawSource;
        const QString html = loadHelpHtmlFromFile(path, &rawSource);
        if (html.isEmpty()) {
            return;
        }
        m_currentHelpFilePath = path;
        const QFileInfo info(path);
        m_helpBrowser->document()->setBaseUrl(QUrl::fromLocalFile(info.absolutePath() + QStringLiteral("/")));
        m_helpBrowser->document()->setDocumentMargin(8);
        m_helpBrowser->document()->setDefaultStyleSheet(buildHelpDefaultStylesheet(rawSource));
        m_helpBrowser->setHtml(html);
        compactHelpDocumentSpacing(m_helpBrowser->document());
        // webBrowser1_DocumentCompleted: на странице руководства кнопка перехода к нему скрыта.
        if (m_helpWindow) {
            if (auto *manualBtn = m_helpWindow->findChild<ImageButton *>(QStringLiteral("helpManualButton"))) {
                manualBtn->setVisible(!path.contains(QStringLiteral("руководство")));
            }
        }
    };

    const auto navigateHelpLink = [this, loadHelpPage, resolveHelpLinkTarget](const QUrl &url) {
        if (!m_helpBrowser) {
            return;
        }

        const QString fragment = url.fragment();
        const QUrl pageUrl = url.adjusted(QUrl::RemoveFragment);
        if (pageUrl.path().isEmpty() && !fragment.isEmpty()) {
            m_helpBrowser->scrollToAnchor(fragment);
            return;
        }

        const QString target = resolveHelpLinkTarget(pageUrl.isEmpty() ? url : pageUrl);
        if (target.isEmpty()) {
            return;
        }

        if (target != m_currentHelpFilePath) {
            loadHelpPage(target);
        }
        if (!fragment.isEmpty()) {
            m_helpBrowser->scrollToAnchor(fragment);
        }
    };

    // spravka: 800x900, фон spravka.png, браузер 23,111 757x767, закрытие d.png (735,16), toogl.png (657,74).
    if (!m_helpWindow) {
        m_helpWindow = new QDialog(this, Qt::FramelessWindowHint);
        m_helpWindow->setFixedSize(800, 900);
        m_helpWindow->setStyleSheet("QDialog { background-image: url('" + imagePath("spravka.png") + "'); }");

        auto *helpBrowser = new HelpTextBrowser(m_helpWindow);
        m_helpBrowser = helpBrowser;
        helpBrowser->setLinkHandler(navigateHelpLink);
        m_helpBrowser->setGeometry(23, 111, 757, 767);
        m_helpBrowser->setOpenExternalLinks(false);

        new HelpWindowDragFilter(m_helpWindow, 110, m_helpWindow);

        ImageButton *closeBtn = new ImageButton(m_helpWindow);
        {
            const QPixmap closePixmap(imagePath("d.png"));
            closeBtn->setPixmap(closePixmap);
            closeBtn->setGeometry(735, 16, closePixmap.isNull() ? 36 : closePixmap.width(),
                                  closePixmap.isNull() ? 31 : closePixmap.height());
        }
        connect(closeBtn, &ImageButton::clicked, m_helpWindow, &QDialog::hide);

        ImageButton *manualBtn = new ImageButton(m_helpWindow);
        manualBtn->setObjectName(QStringLiteral("helpManualButton"));
        manualBtn->setGeometry(657, 74, 100, 30);
        manualBtn->setImagePath(imagePath("toogl.png"));
        connect(manualBtn, &ImageButton::clicked, this, [this, loadHelpPage]() {
            loadHelpPage(htmlPath("spravka/руководствопользователя.htm"));
        });
    }

    loadHelpPage(address);
    const Qt::WindowModality modality = aboveModalDialog ? Qt::ApplicationModal : Qt::NonModal;
    if (m_helpWindow->windowModality() != modality) {
        m_helpWindow->hide();
        m_helpWindow->setWindowModality(modality);
    }
    m_helpWindow->show();
    m_helpWindow->raise();
    m_helpWindow->activateWindow();
}

void SunriseWindow::showAboutWindow() {
    if (!m_aboutWindow) {
        m_aboutWindow = new QDialog(this, Qt::FramelessWindowHint);
        m_aboutWindow->setFixedSize(478, 147);
        m_aboutWindow->setStyleSheet("QDialog { background-image: url('" + imagePath("about.png") + "'); }");

        auto addText = [&](const QString &text, int x, int y, qreal pointSize) {
            QLabel *l = new QLabel(text, m_aboutWindow);
            QFont font(QStringLiteral("Microsoft Sans Serif"));
            font.setPointSizeF(pointSize);
            l->setFont(font);
            l->setStyleSheet(QStringLiteral("color: black; background: transparent;"));
            l->move(x, y);
            l->adjustSize();
        };
        addText(QStringLiteral("О программе"), 170, 11, 11.25);
        addText(QStringLiteral("Программа для ЭВМ «Программа индивидуализированной "), 23, 50, 9.75);
        addText(QStringLiteral("вторичной профилактики (интерактивная программа «Санрайс»)» "), 12, 66, 9.75);
        addText(QStringLiteral("Версия 1.0. от 23.10.2021"), 135, 85, 9.75);

        ImageButton *closeBtn = new ImageButton(m_aboutWindow);
        closeBtn->setGeometry(423, 11, 31, 29);
        QString closeRes = resourcePath("Закрыть.png");
        if (!closeRes.isEmpty()) {
            closeBtn->setImagePath(closeRes);
        }
        connect(closeBtn, &ImageButton::clicked, m_aboutWindow, &QDialog::hide);
    }
    m_aboutWindow->show();
    m_aboutWindow->raise();
    m_aboutWindow->activateWindow();
}

namespace {

QString localUpdatesFilePath() {
    return QCoreApplication::applicationDirPath() + QStringLiteral("/updates.txt");
}

QString extractVersionText(const QString &raw) {
    QString data = raw;
    const int start = data.indexOf(QStringLiteral("<PRE>"), 0, Qt::CaseInsensitive);
    const int end = data.indexOf(QStringLiteral("</PRE>"), 0, Qt::CaseInsensitive);
    if (start >= 0 && end > start) {
        data = data.mid(start + 5, end - start - 5);
    }
    data.remove(QChar(0xfeff));
    return data.trimmed();
}

constexpr int kUpdateTransferTimeoutMs = 30000;

} // namespace

void SunriseWindow::checkForUpdates() {
    // bUpdate_Click: сравнение https://dokitlab.ru/sun/updates.txt с локальным updates.txt.
    m_helpIndex = QStringLiteral("0.2.2.htm");
    if (m_updateCheckInProgress) {
        return;
    }
    if (!m_updateNetwork) {
        m_updateNetwork = new QNetworkAccessManager(this);
    }
    m_updateCheckInProgress = true;
    QNetworkRequest request(QUrl(QStringLiteral("https://dokitlab.ru/sun/updates.txt")));
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    request.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::AlwaysNetwork);
    request.setTransferTimeout(kUpdateTransferTimeoutMs);
    QNetworkReply *reply = m_updateNetwork->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        m_updateCheckInProgress = false;
        if (reply->error() != QNetworkReply::NoError) {
            CustomMessageBox::showError(this, QStringLiteral("Не удалось проверить наличие обновлений.\n%1")
                                                  .arg(reply->errorString()));
            return;
        }
        bool remoteOk = false;
        const QString remoteText = extractVersionText(QString::fromUtf8(reply->readAll()));
        const int remoteVersion = remoteText.toInt(&remoteOk);
        if (!remoteOk) {
            CustomMessageBox::showError(this, QStringLiteral("Не удалось проверить наличие обновлений."));
            return;
        }
        int localVersion = 0;
        QFile localFile(localUpdatesFilePath());
        if (localFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            localVersion = extractVersionText(QString::fromUtf8(localFile.readAll())).toInt();
        }
        m_remoteUpdateVersion = remoteText;
        showUpdateResultDialog(remoteVersion > localVersion);
    });
}

void SunriseWindow::showUpdateResultDialog(bool updateAvailable) {
    // fupdate / noupdate: 458x297, фон updatef.png.
    QDialog dialog(this, Qt::FramelessWindowHint | Qt::Dialog);
    dialog.setObjectName(QStringLiteral("updateDialog"));
    dialog.setFixedSize(458, 297);
    {
        QString background = imagePath("updatef.png");
        background.replace('\\', '/');
        dialog.setStyleSheet(
            QStringLiteral("QDialog#updateDialog { background-image: url('%1'); }").arg(background));
    }
    dialog.setWindowModality(Qt::WindowModal);

    const auto addLabel = [&dialog](const QString &text, int x, int y, qreal pointSize, bool bold, bool italic,
                                    const QString &color = QStringLiteral("#000000")) {
        auto *label = new QLabel(text, &dialog);
        QFont font(QStringLiteral("Microsoft Sans Serif"));
        font.setPointSizeF(pointSize);
        font.setBold(bold);
        font.setItalic(italic);
        label->setFont(font);
        label->setStyleSheet(QStringLiteral("background: transparent; color: %1;").arg(color));
        label->move(x, y);
        label->adjustSize();
        return label;
    };
    const auto addImage = [this, &dialog](const QString &name, int x, int y, int width = -1, int height = -1) {
        auto *button = new ImageButton(&dialog);
        const QPixmap pixmap(imagePath(name));
        button->setPixmap(pixmap);
        button->setGeometry(x, y, width > 0 ? width : pixmap.width(), height > 0 ? height : pixmap.height());
        return button;
    };

    if (!updateAvailable) {
        addLabel(QStringLiteral("Вы используете актуальную версию ПО"), 42, 110, 12, true, true);
        ImageButton *okButton = addImage(QStringLiteral("ok.png"), 200, 239);
        connect(okButton, &ImageButton::clicked, &dialog, &QDialog::accept);
        dialog.exec();
        return;
    }

    QList<QWidget *> infoWidgets;
    infoWidgets << addLabel(QStringLiteral("Обнаружена новая версия ПО"), 128, 45, 11, true, true);
    infoWidgets << addLabel(QStringLiteral("При установке новой версии ознакомьтесь"), 42, 110, 12, true, true);
    infoWidgets << addLabel(QStringLiteral("с соответсующий разделом"), 99, 134, 12, true, true);
    infoWidgets << addLabel(QStringLiteral("руководства пользователя!"), 99, 159, 12, true, true);
    addLabel(QStringLiteral("ВНИМАНИЕ!"), 187, 68, 11, true, false);
    QLabel *progressLabel = addLabel(QStringLiteral("Обновление загружается. Дождитесь окончания загрузки!"),
                                     22, 92, 11, false, false, QStringLiteral("#ff0000"));
    progressLabel->hide();
    ImageButton *manualButton = addImage(QStringLiteral("br.png"), 121, 188, 225, 30);
    ImageButton *alertImage = addImage(QStringLiteral("alert.png"), 25, 45, 43, 41);
    ImageButton *downloadButton = addImage(QStringLiteral("download.png"), 36, 236);
    ImageButton *cancelButton = addImage(QStringLiteral("cancel.png"), 304, 236);
    infoWidgets << manualButton << alertImage << downloadButton;

    connect(manualButton, &ImageButton::clicked, this, [this]() {
        showHelpWindow(htmlPath(QStringLiteral("spravka/0.2.2.htm")), true);
    });
    QPointer<QDialog> dialogGuard(&dialog);
    QPointer<QNetworkReply> activeDownload;
    std::shared_ptr<QFile> partFile;
    connect(cancelButton, &ImageButton::clicked, &dialog, [&dialog, &activeDownload]() {
        if (activeDownload && activeDownload->isRunning()) {
            activeDownload->abort();
        }
        dialog.reject();
    });
    connect(downloadButton, &ImageButton::clicked, this,
            [this, &dialog, &activeDownload, &partFile, dialogGuard, infoWidgets, progressLabel]() {
        const QString downloadsDir = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
        const QString target = QFileDialog::getSaveFileName(
            &dialog, QString(), QDir(downloadsDir).filePath(QStringLiteral("SunriseSetup.exe")),
            QStringLiteral("Установочный файл (*.exe)"));
        if (target.isEmpty()) {
            return;
        }
        const QString setupPath = QFileInfo(target).absoluteFilePath();
        partFile = std::make_shared<QFile>(setupPath + QStringLiteral(".part"));
        if (!partFile->open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            CustomMessageBox::showError(this, QStringLiteral("Не удалось сохранить файл обновления в выбранную папку."));
            partFile.reset();
            return;
        }
        for (QWidget *widget : infoWidgets) {
            widget->hide();
        }
        progressLabel->show();
        QNetworkRequest request(QUrl(QStringLiteral("https://dokitlab.ru/sun/SunriseSetup.exe")));
        request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
        request.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::AlwaysNetwork);
        request.setTransferTimeout(kUpdateTransferTimeoutMs);
        QNetworkReply *reply = m_updateNetwork->get(request);
        activeDownload = reply;
        const std::shared_ptr<QFile> file = partFile;
        auto writeFailed = std::make_shared<bool>(false);
        connect(reply, &QNetworkReply::readyRead, this, [reply, file, writeFailed]() {
            const QByteArray chunk = reply->readAll();
            if (!*writeFailed && file->write(chunk) != chunk.size()) {
                *writeFailed = true;
                reply->abort();
            }
        });
        connect(reply, &QNetworkReply::finished, this, [this, reply, file, writeFailed, setupPath, dialogGuard]() {
            reply->deleteLater();
            const bool aborted = reply->error() == QNetworkReply::OperationCanceledError && !*writeFailed;
            if (!*writeFailed && reply->error() == QNetworkReply::NoError) {
                const QByteArray rest = reply->readAll();
                if (file->write(rest) != rest.size()) {
                    *writeFailed = true;
                }
            }
            const bool ok = !*writeFailed && reply->error() == QNetworkReply::NoError && file->flush();
            file->close();
            if (!ok) {
                file->remove();
                if (aborted) {
                    return;
                }
                CustomMessageBox::showError(this, *writeFailed
                    ? QStringLiteral("Не удалось сохранить файл обновления.")
                    : QStringLiteral("Не удалось загрузить обновление.\n%1").arg(reply->errorString()));
                if (dialogGuard) {
                    dialogGuard->reject();
                }
                return;
            }
            QFile::remove(setupPath);
            if (!file->rename(setupPath)) {
                file->remove();
                CustomMessageBox::showError(this, QStringLiteral("Не удалось сохранить файл обновления."));
                if (dialogGuard) {
                    dialogGuard->reject();
                }
                return;
            }
            QFile versionFile(localUpdatesFilePath());
            if (versionFile.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
                versionFile.write(m_remoteUpdateVersion.toUtf8());
            }
            CustomMessageBox::showInfo(this, QStringLiteral("Обновление загружено"));
            if (dialogGuard) {
                dialogGuard->accept();
            }
        });
    });
    dialog.exec();
    if (activeDownload && activeDownload->isRunning()) {
        activeDownload->abort();
    }
    if (m_helpWindow && m_helpWindow->windowModality() != Qt::NonModal) {
        const bool visible = m_helpWindow->isVisible();
        m_helpWindow->hide();
        m_helpWindow->setWindowModality(Qt::NonModal);
        if (visible) {
            m_helpWindow->show();
        }
    }
}
