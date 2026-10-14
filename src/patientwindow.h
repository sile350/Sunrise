#ifndef PATIENTWINDOW_H
#define PATIENTWINDOW_H

#include <QPixmap>
#include <QWidget>

class ExCanvas;
class ExEngine;
class QScreen;
class QTextEdit;

// Экран пациента (аналог Engines.uprSpace).
// Два экрана: отдельное окно без рамки на весь второй монитор, сверху шапка как у основного окна.
// Один экран: отдельного окна нет — встраивается в основное окно под его шапкой,
// внизу панель управления врача.
class PatientWindow final : public QWidget {
    Q_OBJECT
public:
    explicit PatientWindow(QWidget *parent = nullptr);

    static QScreen *secondaryScreen();

    void setEngine(ExEngine *engine);
    // uprSpace 2.2.6 режим 3: RichTextBox 800x900, Top 100, по центру. Пустой путь — скрыть.
    void setRtf(const QString &rtfFile);
    void showOnSecondaryScreen();
    void showEmbedded(QWidget *container, const QRect &rect);
    void hideDisplay();
    bool isSingleScreen() const { return m_single; }

    // Панель врача (только в режиме одного экрана): внизу слева, кнопки переносятся по строкам.
    QWidget *controlBar() const { return m_bar; }
    void setControlWidgets(const QList<QWidget *> &widgets);
    QList<QWidget *> controlWidgets() const { return m_barWidgets; }
    void layoutControlBar();

signals:
    void closedByUser();
    void doctorPanelRequested();

protected:
    void resizeEvent(QResizeEvent *event) override;
    void closeEvent(QCloseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private:
    void layoutContent();
    void layoutRtf();

    QWidget *m_header = nullptr;
    QWidget *m_content = nullptr;
    ExCanvas *m_canvas = nullptr;
    QTextEdit *m_rtf = nullptr;
    QWidget *m_bar = nullptr;
    QList<QWidget *> m_barWidgets;
    QString m_rtfFile;
    bool m_programmaticClose = false;
    bool m_single = false;
};

#endif
