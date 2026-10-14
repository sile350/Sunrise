#include "scoringform.h"

#include "exerciseprotocol.h"
#include "uistyles.h"

#include <QButtonGroup>
#include <QCheckBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QRadioButton>
#include <QRegularExpression>
#include <QResizeEvent>
#include <QTextDocumentFragment>
#include <QVBoxLayout>
#include <algorithm>
#include <functional>

namespace {

// _122.cs / _226.cs / _324.cs: idvz.InnerHtml для режимов с ручной оценкой.
const char *const kStandardScoreBlock =
    "<div align='center'><strong>Количество баллов:</strong></div>"
    "<input type='radio' id='idch1' name='n1' value='3'  /><strong>3 балла</strong> (пациент выполнил все задания уверенно и за адекватный период времени)<br>"
    "<input type='radio' id='idch2' name='n1'  value='2'  /><strong>2 балла </strong> (пациент выполнил все задания неуверенно и/или с задержками)<br>"
    "<input type='radio' id='idch3' name='n1'  value='1'  /><strong>1 балл</strong> (пациент выполнил большую часть заданий)<br>"
    "<input type='radio' id='idch4' name='n1'  value='0'  /><strong>0 баллов (низкий уровень)</strong> (пациент выполнил меньшую часть заданий, имеются неединичные ошибки, необходима посторонняя помощь)<br>"
    "<input type='radio' id='idch5' name='n1'  value='-1'  /><strong>-1 балл</strong> (пациент не справляется с заданиями, помощь малоэффективна)<br><br>   ";

// QLabel с переносом слов берёт высоту sizeHint для узкой ширины, и строки формы расползаются.
class WrapLabel : public QLabel {
public:
    explicit WrapLabel(QWidget *parent) : QLabel(parent) {
        setTextFormat(Qt::RichText);
        setWordWrap(true);
    }
    QSize sizeHint() const override {
        const QSize s = QLabel::sizeHint();
        return width() > 0 ? QSize(s.width(), heightForWidth(width())) : s;
    }
    QSize minimumSizeHint() const override {
        return QSize(0, sizeHint().height());
    }

protected:
    void resizeEvent(QResizeEvent *event) override {
        QLabel::resizeEvent(event);
        if (event->oldSize().width() != width()) {
            updateGeometry();
        }
    }
};

class ClickLabel final : public WrapLabel {
public:
    using WrapLabel::WrapLabel;
    std::function<void()> onClick;

protected:
    void mousePressEvent(QMouseEvent *event) override {
        if (event->button() == Qt::LeftButton && onClick) {
            onClick();
            event->accept();
            return;
        }
        WrapLabel::mousePressEvent(event);
    }
};

QString keepInlineFormatting(QString html) {
    static const QRegularExpression otherTags(QStringLiteral("<(?!/?(?:strong|b|em|i)\\b)[^>]*>"),
                                              QRegularExpression::CaseInsensitiveOption);
    html.remove(otherTags);
    html.replace(QRegularExpression(QStringLiteral("\\s+")), QStringLiteral(" "));
    return html.trimmed();
}

bool hasVisibleText(const QString &html) {
    QString t = QTextDocumentFragment::fromHtml(html).toPlainText();
    t.replace(QChar(0x00A0), QLatin1Char(' '));
    return !t.trimmed().isEmpty();
}

QString attribute(const QString &tag, const QString &name) {
    const QRegularExpression re(QStringLiteral("\\b%1\\s*=\\s*(?:'([^']*)'|\"([^\"]*)\")").arg(name),
                                QRegularExpression::CaseInsensitiveOption);
    const QRegularExpressionMatch m = re.match(tag);
    if (!m.hasMatch()) {
        return {};
    }
    return m.capturedStart(1) >= 0 ? m.captured(1) : m.captured(2);
}

void clearLayout(QLayout *layout) {
    while (QLayoutItem *item = layout->takeAt(0)) {
        if (QWidget *w = item->widget()) {
            delete w;
        }
        delete item;
    }
}

} // namespace

ScoringForm::ScoringForm(QWidget *parent) : QWidget(parent) {
    const QFont f = ExProtocol::textFont();
    setFont(f);
    setStyleSheet(panelCheckBoxStyleSheet()
                  + QStringLiteral("QLabel { font-family: '%1'; font-size: %2pt; color: #000000; }")
                        .arg(f.family())
                        .arg(f.pointSize()));
    m_layout = new QVBoxLayout(this);
    m_layout->setContentsMargins(0, 0, 0, 0);
    m_layout->setSpacing(3);
}

void ScoringForm::load(const QString &formHtml) {
    clearLayout(m_layout);
    m_inputs.clear();
    m_vz = nullptr;
    m_vzLayout = nullptr;

    static const QRegularExpression vzOpen(QStringLiteral("<div\\s+id\\s*=\\s*['\"]idvz['\"][^>]*>"),
                                           QRegularExpression::CaseInsensitiveOption);
    const QRegularExpressionMatch open = vzOpen.match(formHtml);
    if (!open.hasMatch()) {
        buildInto(m_layout, formHtml, false);
        return;
    }
    static const QRegularExpression divTag(QStringLiteral("<(/?)div\\b[^>]*>"), QRegularExpression::CaseInsensitiveOption);
    int depth = 1;
    int innerEnd = formHtml.size();
    int afterEnd = formHtml.size();
    auto it = divTag.globalMatch(formHtml, open.capturedEnd());
    while (it.hasNext()) {
        const QRegularExpressionMatch m = it.next();
        depth += m.captured(1).isEmpty() ? 1 : -1;
        if (depth == 0) {
            innerEnd = m.capturedStart();
            afterEnd = m.capturedEnd();
            break;
        }
    }
    buildInto(m_layout, formHtml.left(open.capturedStart()), false);
    m_vz = new QWidget(this);
    m_vzLayout = new QVBoxLayout(m_vz);
    m_vzLayout->setContentsMargins(0, 0, 0, 0);
    m_vzLayout->setSpacing(3);
    const QString vzHtml = formHtml.mid(open.capturedEnd(), innerEnd - open.capturedEnd());
    buildInto(m_vzLayout, vzHtml, true);
    m_vz->setVisible(m_vzLayout->count() > 0);
    m_layout->addWidget(m_vz);
    buildInto(m_layout, formHtml.mid(afterEnd), false);
}

void ScoringForm::setScoreBlock(bool filled) {
    if (!m_vz) {
        return;
    }
    clearLayout(m_vzLayout);
    for (int i = m_inputs.size() - 1; i >= 0; --i) {
        if (m_inputs.at(i).inScoreBlock) {
            m_inputs.removeAt(i);
        }
    }
    if (filled) {
        buildInto(m_vzLayout, QString::fromUtf8(kStandardScoreBlock), true);
    }
    m_vz->setVisible(filled);
}

void ScoringForm::buildInto(QVBoxLayout *layout, const QString &html, bool scoreBlock) {
    static const QRegularExpression inputRe(QStringLiteral("<input\\b[^>]*>"), QRegularExpression::CaseInsensitiveOption);
    static const QRegularExpression labelEnd(QStringLiteral("<br\\s*/?>|<input\\b|</?div\\b|</?p\\b|<hr\\b|</hr>"),
                                             QRegularExpression::CaseInsensitiveOption);
    static const QRegularExpression block(QStringLiteral("<br\\s*/?>|</?p\\b[^>]*>|</?div\\b[^>]*>|<hr\\b[^>]*>|</hr>|</?body\\b[^>]*>|</?html\\b[^>]*>"),
                                          QRegularExpression::CaseInsensitiveOption);
    static const QRegularExpression brRe(QStringLiteral("<br\\s*/?>"), QRegularExpression::CaseInsensitiveOption);

    const auto filler = [&](const QString &text) {
        if (text.count(brRe) >= 2 && layout->count() > 0) {
            layout->addSpacing(10);
        }
        bool centered = false;
        int pos = 0;
        auto it = block.globalMatch(text);
        const auto flush = [&](int end) {
            const QString chunk = text.mid(pos, end - pos);
            if (hasVisibleText(chunk)) {
                addHeading(layout, keepInlineFormatting(chunk), centered);
            }
        };
        while (it.hasNext()) {
            const QRegularExpressionMatch m = it.next();
            flush(m.capturedStart());
            const QString tag = m.captured(0);
            if (tag.startsWith(QStringLiteral("</"))) {
                centered = false;
            } else if (tag.contains(QStringLiteral("center"), Qt::CaseInsensitive)) {
                centered = true;
            }
            pos = m.capturedEnd();
        }
        flush(text.size());
    };

    int pos = 0;
    auto it = inputRe.globalMatch(html);
    while (it.hasNext()) {
        const QRegularExpressionMatch m = it.next();
        if (m.capturedStart() < pos) {
            continue;
        }
        filler(html.mid(pos, m.capturedStart() - pos));
        const QRegularExpressionMatch end = labelEnd.match(html, m.capturedEnd());
        const int stop = end.hasMatch() ? end.capturedStart() : html.size();
        addInput(layout, m.captured(0), html.mid(m.capturedEnd(), stop - m.capturedEnd()), scoreBlock);
        pos = stop;
    }
    filler(html.mid(pos));
}

void ScoringForm::addHeading(QVBoxLayout *layout, const QString &html, bool centered) {
    auto *label = new WrapLabel(this);
    label->setText(html);
    label->setAlignment((centered ? Qt::AlignHCenter : Qt::AlignLeft) | Qt::AlignTop);
    layout->addWidget(label);
}

void ScoringForm::addInput(QVBoxLayout *layout, const QString &tag, const QString &labelHtml, bool scoreBlock) {
    Input in;
    in.id = attribute(tag, QStringLiteral("id"));
    in.value = QTextDocumentFragment::fromHtml(attribute(tag, QStringLiteral("value"))).toPlainText();
    in.inScoreBlock = scoreBlock;
    const QString label = keepInlineFormatting(labelHtml);
    in.label = QTextDocumentFragment::fromHtml(label).toPlainText().simplified();

    auto *row = new QWidget(this);
    auto *h = new QHBoxLayout(row);
    h->setContentsMargins(0, 0, 0, 0);
    h->setSpacing(6);
    const bool radio = attribute(tag, QStringLiteral("type")).compare(QStringLiteral("radio"), Qt::CaseInsensitive) == 0;
    QAbstractButton *button = radio ? static_cast<QAbstractButton *>(new QRadioButton(row))
                                    : static_cast<QAbstractButton *>(new QCheckBox(row));
    button->setObjectName(in.id);
    button->setFixedHeight(QFontMetrics(ExProtocol::textFont()).height());
    button->setCursor(Qt::PointingHandCursor);
    button->setFocusPolicy(Qt::NoFocus);
    if (radio) {
        const QString key = attribute(tag, QStringLiteral("name")) + (scoreBlock ? QStringLiteral("#vz") : QString());
        QButtonGroup *group = m_groups.value(key);
        if (!group) {
            group = new QButtonGroup(this);
            m_groups.insert(key, group);
        }
        group->addButton(button);
    }
    auto *text = new ClickLabel(row);
    text->setText(label);
    text->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    text->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    text->setCursor(Qt::PointingHandCursor);
    text->onClick = [button]() { button->click(); };
    h->addWidget(button, 0, Qt::AlignTop);
    h->addWidget(text, 1);
    layout->addWidget(row);
    in.button = button;
    m_inputs.append(in);
}

bool ScoringForm::inputActive(const Input &in) const {
    return !in.inScoreBlock || (m_vz && !m_vz->isHidden());
}

QAbstractButton *ScoringForm::input(const QString &id) const {
    for (const Input &in : m_inputs) {
        if (in.id == id && inputActive(in)) {
            return in.button;
        }
    }
    return nullptr;
}

int ScoringForm::checkedScore(bool *ok) const {
    *ok = false;
    int score = 0;
    for (int i = 1; i <= 100; ++i) {
        const QString id = QStringLiteral("idch%1").arg(i);
        const auto found = std::find_if(m_inputs.cbegin(), m_inputs.cend(),
                                        [&](const Input &in) { return in.id == id && inputActive(in); });
        if (found == m_inputs.cend()) {
            break;
        }
        if (found->button->isChecked()) {
            score = found->value.toInt();
            *ok = true;
        }
    }
    return score;
}

// В порядке формы, а не по idd1, idd2…: в 1.2.3/or.html id idd3 повторяется дважды.
QString ScoringForm::joinChecked(const QString &prefix) const {
    static const QRegularExpression digits(QStringLiteral("^\\d+$"));
    QString out;
    for (const Input &in : m_inputs) {
        if (!in.id.startsWith(prefix) || !digits.match(in.id.mid(prefix.size())).hasMatch() || !inputActive(in)) {
            continue;
        }
        if (in.button->isChecked()) {
            out += QString(4, QChar(0x00A0)) + in.value.toHtmlEscaped() + QStringLiteral("<br>");
        }
    }
    return out;
}

QString ScoringForm::activityHtml() const {
    return joinChecked(QStringLiteral("idd"));
}

QString ScoringForm::helpHtml() const {
    return joinChecked(QStringLiteral("idp"));
}

void ScoringForm::clear() {
    for (QButtonGroup *group : qAsConst(m_groups)) {
        group->setExclusive(false);
    }
    for (const Input &in : qAsConst(m_inputs)) {
        in.button->setChecked(false);
    }
    for (QButtonGroup *group : qAsConst(m_groups)) {
        group->setExclusive(true);
    }
}

QStringList ScoringForm::inputIds() const {
    QStringList ids;
    for (const Input &in : m_inputs) {
        if (inputActive(in)) {
            ids << in.id;
        }
    }
    return ids;
}

QStringList ScoringForm::inputLabels() const {
    QStringList labels;
    for (const Input &in : m_inputs) {
        if (inputActive(in)) {
            labels << in.label;
        }
    }
    return labels;
}
