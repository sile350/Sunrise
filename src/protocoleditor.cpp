#include "protocoleditor.h"

#include <QContextMenuEvent>
#include <QKeyEvent>
#include <QMenu>
#include <QMimeData>
#include <QMouseEvent>
#include <QTextFrame>
#include <QTextTable>
#include <QTimer>

ProtocolEditor::ProtocolEditor(QWidget *parent) : QTextEdit(parent) {
    setAcceptRichText(false);
    setAcceptDrops(false);
    setTabChangesFocus(true);
    setUndoRedoEnabled(false);
    setFrameShape(QFrame::NoFrame);
    setStyleSheet(QStringLiteral("QTextEdit { background: #f8f8f8; border: none; }"));
    m_saveTimer = new QTimer(this);
    m_saveTimer->setSingleShot(true);
    m_saveTimer->setInterval(700);
    connect(m_saveTimer, &QTimer::timeout, this, &ProtocolEditor::flushEdits);
    connect(document(), &QTextDocument::contentsChanged, this, &ProtocolEditor::onContentsChanged);
    connect(this, &QTextEdit::cursorPositionChanged, this, &ProtocolEditor::keepCursorInCells);
    viewport()->setMouseTracking(true);
}

void ProtocolEditor::setInteractive(bool editable) {
    setReadOnly(!editable);
    if (editable) {
        setFocusPolicy(Qt::StrongFocus);
    } else {
        setTextInteractionFlags(Qt::NoTextInteraction);
        setFocusPolicy(Qt::NoFocus);
        clearFocus();
    }
    viewport()->setCursor(Qt::ArrowCursor);
}

void ProtocolEditor::showEntries(const QString &preambleHtml, const QList<ExProtocol::Entry> &entries, bool editable) {
    m_saveTimer->stop();
    m_loading = true;
    m_entries = entries;
    setInteractive(editable);
    setHtml(ExProtocol::documentHtml(preambleHtml, m_entries));
    ExProtocol::tightenTables(document());
    setUndoRedoEnabled(false);
    m_cells = editable ? ExProtocol::editableCells(m_entries) : QList<ExProtocol::EditableCell>();
    m_lastGood = -1;
    m_loading = false;
    m_dirty = false;
    keepCursorInCells();
}

void ProtocolEditor::showMessage(const QString &text) {
    m_saveTimer->stop();
    m_loading = true;
    m_entries.clear();
    m_cells.clear();
    setInteractive(false);
    setHtml(QStringLiteral("<html><body style=\"font-family:'Times New Roman'; font-size:12pt\">%1</body></html>")
                .arg(text.toHtmlEscaped()));
    m_loading = false;
    m_dirty = false;
}

void ProtocolEditor::keepCursorInCells() {
    if (m_loading || m_restoring || isReadOnly()) {
        return;
    }
    const QTextCursor cursor = textCursor();
    if (isEditable(cursor)) {
        m_lastGood = cursor.position();
        setCursorWidth(1);
        return;
    }
    m_restoring = true;
    QTextCursor back(document());
    if (m_lastGood >= 0 && m_lastGood < document()->characterCount()) {
        back.setPosition(m_lastGood);
    }
    if (m_lastGood >= 0 && isEditable(back)) {
        setTextCursor(back);
        setCursorWidth(1);
    } else {
        m_lastGood = -1;
        setCursorWidth(0);
    }
    m_restoring = false;
}

void ProtocolEditor::mousePressEvent(QMouseEvent *event) {
    if (isReadOnly() || !isEditable(cursorForPosition(event->pos()))) {
        event->accept();
        return;
    }
    QTextEdit::mousePressEvent(event);
}

void ProtocolEditor::mouseDoubleClickEvent(QMouseEvent *event) {
    if (isReadOnly() || !isEditable(cursorForPosition(event->pos()))) {
        event->accept();
        return;
    }
    QTextEdit::mouseDoubleClickEvent(event);
}

void ProtocolEditor::mouseMoveEvent(QMouseEvent *event) {
    if (isReadOnly()) {
        event->accept();
        return;
    }
    QTextEdit::mouseMoveEvent(event);
    viewport()->setCursor(isEditable(cursorForPosition(event->pos())) ? Qt::IBeamCursor : Qt::ArrowCursor);
}

void ProtocolEditor::flushEdits() {
    m_saveTimer->stop();
    if (!m_dirty) {
        return;
    }
    m_dirty = false;
    const QList<int> changed = ExProtocol::applyDocumentEdits(document(), m_entries);
    if (!changed.isEmpty()) {
        emit entriesEdited(changed);
    }
}

int ProtocolEditor::contentHeight() const {
    return int(document()->size().height()) + 2 * frameWidth() + 4;
}

void ProtocolEditor::onContentsChanged() {
    if (m_loading) {
        return;
    }
    m_dirty = true;
    m_saveTimer->start();
}

bool ProtocolEditor::isEditable(const QTextCursor &cursor) const {
    QTextTable *table = cursor.currentTable();
    if (!table) {
        return false;
    }
    const QTextTableCell cell = table->cellAt(cursor);
    if (cursor.hasSelection()) {
        QTextCursor a(document());
        a.setPosition(cursor.selectionStart());
        QTextCursor b(document());
        b.setPosition(cursor.selectionEnd());
        if (a.currentTable() != table || b.currentTable() != table || !(table->cellAt(a) == cell)
            || !(table->cellAt(b) == cell)) {
            return false;
        }
    }
    int index = -1;
    int n = 0;
    for (QTextFrame *frame : document()->rootFrame()->childFrames()) {
        if (qobject_cast<QTextTable *>(frame)) {
            if (frame == table) {
                index = n;
                break;
            }
            ++n;
        }
    }
    for (const ExProtocol::EditableCell &c : m_cells) {
        if (c.table == index && c.row == cell.row() && c.column == cell.column()) {
            return true;
        }
    }
    return false;
}

void ProtocolEditor::keyPressEvent(QKeyEvent *event) {
    if (event->matches(QKeySequence::Undo) || event->matches(QKeySequence::Redo)) {
        event->accept();
        return;
    }
    const int key = event->key();
    const bool erasing = key == Qt::Key_Backspace || key == Qt::Key_Delete || event->matches(QKeySequence::Cut)
        || event->matches(QKeySequence::DeleteStartOfWord) || event->matches(QKeySequence::DeleteEndOfWord)
        || event->matches(QKeySequence::DeleteEndOfLine) || event->matches(QKeySequence::DeleteCompleteLine);
    const bool newline = key == Qt::Key_Return || key == Qt::Key_Enter;
    const QString text = event->text();
    const bool typing = !text.isEmpty() && text.at(0).isPrint() && !(event->modifiers() & Qt::ControlModifier);
    if (!erasing && !newline && !typing && !event->matches(QKeySequence::Paste)) {
        QTextEdit::keyPressEvent(event);
        return;
    }
    QTextCursor cursor = textCursor();
    if (!isEditable(cursor)) {
        event->accept();
        return;
    }
    if (!cursor.hasSelection()) {
        const QTextTableCell cell = cursor.currentTable()->cellAt(cursor);
        const bool atStart = cursor.position() <= cell.firstCursorPosition().position();
        const bool atEnd = cursor.position() >= cell.lastCursorPosition().position();
        const bool backward = key == Qt::Key_Backspace || event->matches(QKeySequence::DeleteStartOfWord);
        const bool forward = key == Qt::Key_Delete || event->matches(QKeySequence::DeleteEndOfWord)
            || event->matches(QKeySequence::DeleteEndOfLine);
        if ((backward && atStart) || (forward && atEnd)) {
            event->accept();
            return;
        }
    }
    if (newline) {
        cursor.insertText(QString(QChar(QChar::LineSeparator)));
        event->accept();
        return;
    }
    QTextEdit::keyPressEvent(event);
}

void ProtocolEditor::inputMethodEvent(QInputMethodEvent *event) {
    if ((!event->commitString().isEmpty() || !event->preeditString().isEmpty()) && !isEditable(textCursor())) {
        event->accept();
        return;
    }
    QTextEdit::inputMethodEvent(event);
}

bool ProtocolEditor::canInsertFromMimeData(const QMimeData *source) const {
    return source && source->hasText() && isEditable(textCursor());
}

void ProtocolEditor::insertFromMimeData(const QMimeData *source) {
    if (!canInsertFromMimeData(source)) {
        return;
    }
    QString text = source->text();
    text.replace(QStringLiteral("\r\n"), QStringLiteral("\n"));
    text.replace(QLatin1Char('\n'), QChar(QChar::LineSeparator));
    textCursor().insertText(text);
}

void ProtocolEditor::contextMenuEvent(QContextMenuEvent *event) {
    event->accept();
    if (isReadOnly() || !isEditable(textCursor())) {
        return;
    }
    QMenu menu(this);
    QAction *copy = menu.addAction(QStringLiteral("Копировать"), this, &QTextEdit::copy);
    copy->setEnabled(textCursor().hasSelection());
    QAction *paste = menu.addAction(QStringLiteral("Вставить"), this, &QTextEdit::paste);
    paste->setEnabled(canPaste());
    menu.exec(event->globalPos());
}

void ProtocolEditor::focusOutEvent(QFocusEvent *event) {
    flushEdits();
    QTextEdit::focusOutEvent(event);
}
