#include "summarypanel.h"

#include <QAbstractTextDocumentLayout>
#include <QDate>
#include <QFocusEvent>
#include <QFrame>
#include <QLabel>
#include <QPixmap>
#include <QScrollBar>
#include <QSignalBlocker>
#include <QTextBlock>
#include <QTextDocument>
#include <QTextEdit>
#include <QTextLayout>
#include <QTimer>
#include <functional>

namespace {

constexpr int kContentWidth = 903;
constexpr int kFieldX = 24;
constexpr int kFieldWidth = 859;
constexpr int kMarkerX = 37;
constexpr int kSectionLabelX = 80;
constexpr int kSectionLabelHeight = 20;
constexpr int kPanelHeight = 1005;

const char *kPlaceholderColor = "#696969";

class PlaceholderTextEdit final : public QTextEdit {
public:
    using QTextEdit::QTextEdit;
    std::function<void()> onFocusIn;
    std::function<void()> onFocusOut;

protected:
    void focusInEvent(QFocusEvent *event) override {
        QTextEdit::focusInEvent(event);
        if (onFocusIn) {
            onFocusIn();
        }
    }
    void focusOutEvent(QFocusEvent *event) override {
        QTextEdit::focusOutEvent(event);
        if (onFocusOut) {
            onFocusOut();
        }
    }
};

QLabel *makeLabel(const QString &text, QWidget *parent, int x, int y, int pointSize, bool bold) {
    auto *label = new QLabel(text, parent);
    QFont font(QStringLiteral("Microsoft Sans Serif"));
    font.setPointSizeF(pointSize);
    font.setBold(bold);
    label->setFont(font);
    label->setStyleSheet(QStringLiteral("background: transparent; color: #000000;"));
    label->move(x, y);
    label->adjustSize();
    return label;
}

} // namespace

SummaryPanel::SummaryPanel(QWidget *parent)
    : QScrollArea(parent)
{
    setWidgetResizable(false);
    setFrameShape(QFrame::NoFrame);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    setStyleSheet(QStringLiteral(
        "QScrollArea { background: #f0f0f0; border: none; }"
        "QWidget#summaryContent { background: #f0f0f0; }"
    ));
    viewport()->setStyleSheet(QStringLiteral("background: #f0f0f0;"));
    buildUi();
    m_changeTimer = new QTimer(this);
    m_changeTimer->setSingleShot(true);
    m_changeTimer->setInterval(1200);
    connect(m_changeTimer, &QTimer::timeout, this, &SummaryPanel::programChanged);
    resetDefaults();
}

void SummaryPanel::buildUi() {
    m_content = new QWidget;
    m_content->setObjectName(QStringLiteral("summaryContent"));
    m_content->setFixedWidth(kContentWidth);

    m_title1 = makeLabel(QStringLiteral("Программа индивидуализированной вторичной профилактики"),
                         m_content, 158, 20, 14, false);
    m_title2 = makeLabel(QStringLiteral("повторного сосудистого события"), m_content, 273, 44, 14, false);

    m_pfio = makeLabel(QStringLiteral("Пациент:"), m_content, 34, 90, 10, true);
    m_lfio = makeLabel(QString(), m_content, 117, 90, 10, false);
    m_pdr = makeLabel(QStringLiteral("Дата рождения:"), m_content, 361, 90, 10, true);
    m_ldr = makeLabel(QString(), m_content, 496, 90, 10, false);
    m_pdiag = makeLabel(QStringLiteral("Диагноз:"), m_content, 34, 128, 10, true);
    m_ldiag = makeLabel(QString(), m_content, 115, 128, 10, false);

    const QStringList sectionTitles = {
        QStringLiteral("Оценка индивидуального риска повторного сосудистого события"),
        QStringLiteral("Коррекция факторов риска повторного сосудистого события"),
        QStringLiteral("Программа восстановления речевого мышления"),
        QStringLiteral("Дыхательная, артикуляционная гимнастика"),
        QStringLiteral("Медикаментозная терапия"),
        QString()
    };

    m_fields.resize(FieldCount);
    m_sectionLabels.resize(6);
    m_markers.resize(6);

    QFont fieldFont(QStringLiteral("Microsoft Sans Serif"));
    fieldFont.setPointSizeF(10);
    const QString fieldStyle = QStringLiteral(
        "QTextEdit { background-color: #f0f0f0; border: 1px solid #d3d3d3; }"
    );

    for (int i = 0; i < 6; ++i) {
        auto *marker = new QLabel(m_content);
        marker->setStyleSheet(QStringLiteral("background: transparent;"));
        marker->setGeometry(kMarkerX, 0, 22, 22);
        m_markers[i] = marker;

        if (i < 5) {
            m_sectionLabels[i] = makeLabel(sectionTitles.at(i), m_content, kSectionLabelX, 0, 12, true);
        }

        auto *edit = new PlaceholderTextEdit(m_content);
        edit->setAcceptRichText(false);
        edit->setFont(fieldFont);
        edit->setStyleSheet(fieldStyle);
        edit->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        edit->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        edit->setLineWrapMode(QTextEdit::WidgetWidth);
        edit->document()->setDocumentMargin(2);
        edit->setGeometry(kFieldX, 0, kFieldWidth, 74);
        const Field field = static_cast<Field>(i);
        m_fields[field] = edit;
        edit->onFocusIn = [this, field]() { clearPlaceholderOnFocus(field); };
        edit->onFocusOut = [this, field]() { restorePlaceholderIfEmpty(field); };
        connect(edit, &QTextEdit::textChanged, this, [this]() {
            scheduleEmitChanged();
            updateAllSize();
        });
    }

    m_riskSubtitle = makeLabel(QStringLiteral("(клинические шкалы)"), m_content, 666, 169, 12, true);

    auto *titleEdit = new PlaceholderTextEdit(m_content);
    QFont titleFont(QStringLiteral("Microsoft Sans Serif"));
    titleFont.setPointSizeF(12);
    titleFont.setBold(true);
    titleEdit->setFont(titleFont);
    titleEdit->setAcceptRichText(false);
    titleEdit->setStyleSheet(QStringLiteral("QTextEdit { background-color: #f0f0f0; border: none; }"));
    titleEdit->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    titleEdit->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    titleEdit->setLineWrapMode(QTextEdit::NoWrap);
    titleEdit->document()->setDocumentMargin(0);
    titleEdit->setGeometry(84, 0, 313, 21);
    m_fields[ExtraTitle] = titleEdit;
    titleEdit->onFocusIn = [this]() { clearPlaceholderOnFocus(ExtraTitle); };
    titleEdit->onFocusOut = [this]() { restorePlaceholderIfEmpty(ExtraTitle); };
    connect(titleEdit, &QTextEdit::textChanged, this, [this]() { scheduleEmitChanged(); });

    setWidget(m_content);
    updateAllSize();
}

void SummaryPanel::setMarkerImage(const QString &path) {
    const QPixmap pixmap(path);
    for (QLabel *marker : qAsConst(m_markers)) {
        if (marker) {
            marker->setPixmap(pixmap);
        }
    }
}

int SummaryPanel::fittedHeight(QTextEdit *edit, int padding) const {
    if (!edit) {
        return 0;
    }
    QTextDocument *doc = edit->document();
    doc->setTextWidth(edit->viewport()->width() > 0 ? edit->viewport()->width() : kFieldWidth - 2);
    doc->documentLayout()->documentSize();
    int lines = 0;
    for (QTextBlock block = doc->begin(); block.isValid(); block = block.next()) {
        const QTextLayout *layout = block.layout();
        lines += (layout && layout->lineCount() > 0) ? layout->lineCount() : 1;
    }
    lines = qMax(1, lines);
    return (edit->fontMetrics().height() + 1) * lines + padding;
}

void SummaryPanel::updateAllSize() {
    QTextEdit *risk = m_fields.value(Risk);
    if (!risk) {
        return;
    }
    // Как summary.updateAllSize(): поля растут по числу строк, разделы идут друг за другом.
    int top = 169;
    m_markers[0]->move(kMarkerX, top);
    m_sectionLabels[0]->move(kSectionLabelX, top);
    m_riskSubtitle->move(666, top);
    risk->move(kFieldX, 197);
    risk->resize(kFieldWidth, fittedHeight(risk, 10));

    int previousBottom = risk->y() + risk->height();
    for (int i = 1; i < 6; ++i) {
        top = previousBottom + 25;
        m_markers[i]->move(kMarkerX, top);
        int fieldTop = top + kSectionLabelHeight + 10;
        if (i < 5) {
            m_sectionLabels[i]->move(kSectionLabelX, top);
        } else {
            QTextEdit *title = m_fields.value(ExtraTitle);
            title->move(84, top);
            fieldTop = top + 19 + 10;
        }
        QTextEdit *edit = m_fields.value(i);
        edit->move(kFieldX, fieldTop);
        edit->resize(kFieldWidth, fittedHeight(edit, 20));
        previousBottom = edit->y() + edit->height();
    }
    m_content->setFixedHeight(qMax(kPanelHeight, previousBottom + 20));
}

QString SummaryPanel::defaultText(Field field) const {
    switch (field) {
    case Risk:
        return QStringLiteral("Введите результаты оценки индивидуального риска повторного сосудистого события");
    case Corr:
        return QStringLiteral("Введите \"не нуждается\" или данные по коррекции факторов риска");
    case Speech:
        return QStringLiteral("Введите \"не нуждается\" или данные по программе восстановления речевого мышления");
    case Dih:
        return QStringLiteral("Введите \"не нуждается\" или данные по дыхательной, артикуляционной гимнастике");
    case Mp:
        return QStringLiteral("Введите данные");
    case ExtraBody:
        return QStringLiteral("При необходимости переименуйте раздел и введите данные по своему усмотрению");
    case ExtraTitle:
        return QStringLiteral("Дополнительный раздел");
    default:
        return {};
    }
}

QString SummaryPanel::sectionTitle(Field field) const {
    switch (field) {
    case Risk:
        return m_sectionLabels.value(0)->text() + QLatin1Char(' ') + m_riskSubtitle->text();
    case Corr:
    case Speech:
    case Dih:
    case Mp:
        return m_sectionLabels.value(static_cast<int>(field))->text();
    case ExtraBody:
    case ExtraTitle:
        return fieldText(ExtraTitle);
    default:
        return {};
    }
}

void SummaryPanel::applyPlaceholderStyle(QTextEdit *edit, bool placeholder) {
    if (!edit) {
        return;
    }
    QPalette pal = edit->palette();
    pal.setColor(QPalette::Text, placeholder ? QColor(QString::fromLatin1(kPlaceholderColor)) : Qt::black);
    edit->setPalette(pal);
}

void SummaryPanel::resetDefaults() {
    for (int i = 0; i < FieldCount; ++i) {
        const Field field = static_cast<Field>(i);
        QTextEdit *edit = m_fields.value(i);
        if (!edit) {
            continue;
        }
        const QSignalBlocker blocker(edit);
        edit->setPlainText(defaultText(field));
        applyPlaceholderStyle(edit, true);
    }
    setPatientHeader({}, {}, {});
    updateAllSize();
}

void SummaryPanel::loadSerialized(const QString &program) {
    resetDefaults();
    const QStringList parts = program.split(QLatin1Char(';'));
    if (parts.size() < 7) {
        return;
    }
    for (int i = 0; i < 6; ++i) {
        const QString value = parts.at(i);
        if (value.isEmpty()) {
            continue;
        }
        setFieldText(static_cast<Field>(i), value, true);
    }
    const QString title = parts.at(6);
    if (!title.isEmpty()) {
        setFieldText(ExtraTitle, title, true);
    }
    updateAllSize();
}

QString SummaryPanel::serialize() const {
    QString out;
    for (int i = 0; i < 6; ++i) {
        if (isPlaceholder(static_cast<Field>(i))) {
            out += QLatin1Char(';');
        } else {
            out += fieldText(static_cast<Field>(i)) + QLatin1Char(';');
        }
    }
    out += fieldText(ExtraTitle) + QLatin1Char(';');
    return out;
}

void SummaryPanel::setPatientHeader(const QString &fio, const QString &birthDate, const QString &diag) {
    const auto setText = [](QLabel *label, const QString &text) {
        if (label) {
            label->setText(text);
            label->adjustSize();
        }
    };
    setText(m_lfio, fio);
    setText(m_ldr, birthDate);
    setText(m_ldiag, diag);
}

void SummaryPanel::setHeaderVisible(bool visible) {
    const QList<QWidget *> widgets = {m_pfio, m_lfio, m_pdr, m_ldr, m_pdiag, m_ldiag};
    for (QWidget *w : widgets) {
        if (w) {
            w->setVisible(visible);
        }
    }
}

QString SummaryPanel::patientFio() const {
    return m_lfio ? m_lfio->text() : QString();
}

QString SummaryPanel::patientBirthDate() const {
    return m_ldr ? m_ldr->text() : QString();
}

QString SummaryPanel::patientDiag() const {
    return m_ldiag ? m_ldiag->text() : QString();
}

void SummaryPanel::clearPlaceholderOnFocus(Field field) {
    QTextEdit *edit = m_fields.value(static_cast<int>(field));
    if (!edit || !isPlaceholder(field)) {
        return;
    }
    {
        const QSignalBlocker blocker(edit);
        edit->clear();
        applyPlaceholderStyle(edit, false);
    }
    updateAllSize();
}

void SummaryPanel::restorePlaceholderIfEmpty(Field field) {
    QTextEdit *edit = m_fields.value(static_cast<int>(field));
    if (!edit) {
        return;
    }
    const int minLength = field == ExtraTitle ? 1 : 2;
    if (edit->toPlainText().size() < minLength) {
        {
            const QSignalBlocker blocker(edit);
            edit->setPlainText(defaultText(field));
            applyPlaceholderStyle(edit, true);
        }
        updateAllSize();
    }
}

void SummaryPanel::appendToField(Field field, const QString &text, bool prependDate) {
    if (text.trimmed().isEmpty()) {
        return;
    }
    QTextEdit *edit = m_fields.value(static_cast<int>(field));
    if (!edit) {
        return;
    }
    if (isPlaceholder(field)) {
        setFieldText(field, {}, true);
    }
    QString chunk = text;
    if (prependDate) {
        chunk = QDate::currentDate().toString(QStringLiteral("dd.MM.yyyy")) + QLatin1Char(' ') + text;
    }
    QString current = edit->toPlainText();
    if (!current.trimmed().isEmpty()) {
        current += QLatin1Char('\n');
    }
    current += chunk;
    setFieldText(field, current, true);
    scheduleEmitChanged();
}

QString SummaryPanel::fieldText(Field field) const {
    QTextEdit *edit = m_fields.value(static_cast<int>(field));
    return edit ? edit->toPlainText() : QString();
}

bool SummaryPanel::isPlaceholder(Field field) const {
    QTextEdit *edit = m_fields.value(static_cast<int>(field));
    if (!edit) {
        return true;
    }
    return edit->palette().color(QPalette::Text) == QColor(QString::fromLatin1(kPlaceholderColor));
}

void SummaryPanel::setFieldText(Field field, const QString &text, bool asUserContent) {
    QTextEdit *edit = m_fields.value(static_cast<int>(field));
    if (!edit) {
        return;
    }
    {
        const QSignalBlocker blocker(edit);
        if (asUserContent) {
            edit->setPlainText(text);
            applyPlaceholderStyle(edit, false);
        } else {
            edit->setPlainText(text.isEmpty() ? defaultText(field) : text);
            applyPlaceholderStyle(edit, text.isEmpty() || text == defaultText(field));
        }
    }
    updateAllSize();
}

void SummaryPanel::scheduleEmitChanged() {
    if (m_changeTimer) {
        m_changeTimer->start();
    }
}
