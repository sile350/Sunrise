#ifndef PROTOCOLEDITOR_H
#define PROTOCOLEDITOR_H

#include "exerciseprotocol.h"

#include <QTextEdit>

class QTimer;

// Протокол в упражнении: правятся только «Результат», «Примечание», «Характер деятельности»,
// «Виды помощи» и баллы; курсор ставится только в эти ячейки. Без editable — только просмотр.
class ProtocolEditor : public QTextEdit {
    Q_OBJECT
public:
    explicit ProtocolEditor(QWidget *parent = nullptr);

    void showEntries(const QString &preambleHtml, const QList<ExProtocol::Entry> &entries, bool editable = true);
    void showMessage(const QString &text);
    QList<ExProtocol::Entry> &entries() { return m_entries; }
    // Немедленно перенести отложенные правки в модели (перед печатью, уходом со страницы).
    void flushEdits();
    int contentHeight() const;

signals:
    void entriesEdited(const QList<int> &indexes);

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void inputMethodEvent(QInputMethodEvent *event) override;
    bool canInsertFromMimeData(const QMimeData *source) const override;
    void insertFromMimeData(const QMimeData *source) override;
    void contextMenuEvent(QContextMenuEvent *event) override;
    void focusOutEvent(QFocusEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;

private:
    bool isEditable(const QTextCursor &cursor) const;
    void setInteractive(bool editable);
    void onContentsChanged();
    // Курсор только в редактируемых ячейках: иначе возврат на прежнее место или скрытие.
    void keepCursorInCells();

    QList<ExProtocol::Entry> m_entries;
    QList<ExProtocol::EditableCell> m_cells;
    QTimer *m_saveTimer = nullptr;
    bool m_loading = false;
    bool m_dirty = false;
    bool m_restoring = false;
    int m_lastGood = -1;
};

#endif
