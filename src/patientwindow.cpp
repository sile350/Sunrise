#include "patientwindow.h"

#include "engines/exengine.h"
#include "engines/rtfcoverengine.h"

#include <QCloseEvent>
#include <QGuiApplication>
#include <QKeyEvent>
#include <QPainter>
#include <QScreen>
#include <QTextEdit>
#include <QWindow>

namespace {

constexpr int kBarMargin = 8;
// Шапка основного окна — верхние 62 строки fone.jpg (1920x1080), включая тёмную кромку.
constexpr int kHeaderHeight = 62;

class HeaderStrip final : public QWidget {
public:
    explicit HeaderStrip(QWidget *parent) : QWidget(parent) {
        const QPixmap fone(ExAssets::sysImage(QStringLiteral("fone.jpg")));
        if (!fone.isNull()) {
            m_strip = fone.copy(0, 0, fone.width(), qMin(kHeaderHeight, fone.height()));
        }
    }

protected:
    void paintEvent(QPaintEvent *) override {
        QPainter p(this);
        if (m_strip.isNull()) {
            p.fillRect(rect(), QColor(215, 143, 67));
            return;
        }
        p.setRenderHint(QPainter::SmoothPixmapTransform, true);
        p.drawPixmap(rect(), m_strip);
    }

private:
    QPixmap m_strip;
};

} // namespace

QScreen *PatientWindow::secondaryScreen() {
    QScreen *primary = QGuiApplication::primaryScreen();
    for (QScreen *screen : QGuiApplication::screens()) {
        if (screen != primary) {
            return screen;
        }
    }
    return nullptr;
}

PatientWindow::PatientWindow(QWidget *parent) : QWidget(parent, Qt::Window | Qt::FramelessWindowHint) {
    setWindowTitle(QStringLiteral("Экран пациента"));
    setAttribute(Qt::WA_DeleteOnClose, false);
    setFocusPolicy(Qt::StrongFocus);
    setAutoFillBackground(true);
    QPalette pal = palette();
    pal.setColor(QPalette::Window, QColor(249, 249, 249));
    setPalette(pal);
    m_header = new HeaderStrip(this);
    m_content = new QWidget(this);
    m_canvas = new ExCanvas(true, m_content);
    layoutContent();

    m_bar = new QWidget(this);
    m_bar->setObjectName(QStringLiteral("doctorBar"));
    m_bar->setAttribute(Qt::WA_StyledBackground, true);
    m_bar->setStyleSheet(QStringLiteral(
        "QWidget#doctorBar { background: rgba(255, 255, 255, 230); border: 1px solid #b0b0b0; border-radius: 6px; }"));
    m_bar->hide();

    resize(960, 540);
}

void PatientWindow::setEngine(ExEngine *engine) {
    m_canvas->setEngine(engine);
}

void PatientWindow::setRtf(const QString &rtfFile) {
    if (rtfFile.isEmpty()) {
        if (m_rtf) {
            m_rtf->hide();
        }
        m_rtfFile.clear();
        return;
    }
    if (!m_rtf) {
        m_rtf = new QTextEdit(m_content);
    }
    if (m_rtfFile != rtfFile) {
        RtfCover::loadInto(m_rtf, rtfFile);
        m_rtfFile = rtfFile;
    }
    layoutRtf();
    m_rtf->show();
    m_rtf->raise();
    if (m_single) {
        m_bar->raise();
    }
}

void PatientWindow::layoutRtf() {
    if (!m_rtf) {
        return;
    }
    // Тот же масштаб и центрирование, что у ExCanvas для сцены 1920x1080.
    const double s = qMin(m_content->width() / 1920.0, m_content->height() / 1080.0);
    const double ox = (m_content->width() - 1920 * s) / 2.0;
    const double oy = (m_content->height() - 1080 * s) / 2.0;
    m_rtf->setGeometry(qRound(ox + (1920 - 800) / 2 * s), qRound(oy + 100 * s), qRound(800 * s), qRound(900 * s));
}

void PatientWindow::layoutContent() {
    // Встроенный режим: над экраном пациента и так шапка основного окна.
    const int headerH = m_single ? 0 : qRound(kHeaderHeight * width() / 1920.0);
    m_header->setVisible(!m_single);
    m_header->setGeometry(0, 0, width(), headerH);
    m_content->setGeometry(0, headerH, width(), qMax(0, height() - headerH));
    m_canvas->setGeometry(0, 0, m_content->width(), m_content->height());
    layoutRtf();
}

void PatientWindow::setControlWidgets(const QList<QWidget *> &widgets) {
    for (QWidget *w : m_barWidgets) {
        if (!widgets.contains(w)) {
            w->hide();
            w->deleteLater();
        }
    }
    m_barWidgets = widgets;
    for (QWidget *w : m_barWidgets) {
        w->setParent(m_bar);
        w->show();
    }
    layoutControlBar();
}

void PatientWindow::layoutControlBar() {
    m_bar->setVisible(m_single);
    if (!m_single) {
        return;
    }
    // Кнопки слева направо; не помещается по ширине окна — перенос на следующую строку.
    constexpr int kPadX = 8;
    constexpr int kPadY = 6;
    constexpr int kGap = 10;
    const int maxWidth = qMax(200, width() - 2 * kBarMargin);
    int x = kPadX;
    int y = kPadY;
    int rowH = 0;
    int usedW = 0;
    for (QWidget *w : m_barWidgets) {
        const QSize s = w->minimumSize().isEmpty() ? w->sizeHint() : w->minimumSize();
        if (x > kPadX && x + s.width() + kPadX > maxWidth) {
            x = kPadX;
            y += rowH + kGap / 2;
            rowH = 0;
        }
        w->setGeometry(x, y, s.width(), s.height());
        x += s.width() + kGap;
        rowH = qMax(rowH, s.height());
        usedW = qMax(usedW, x - kGap + kPadX);
    }
    const QSize barSize(qMax(usedW, 2 * kPadX), y + rowH + kPadY);
    m_bar->resize(barSize);
    // Внизу слева: вверху сцены у многих упражнений подпись задания.
    m_bar->move(kBarMargin, height() - barSize.height() - kBarMargin);
    m_bar->raise();
}

void PatientWindow::resizeEvent(QResizeEvent *event) {
    QWidget::resizeEvent(event);
    layoutContent();
    layoutControlBar();
}

void PatientWindow::showOnSecondaryScreen() {
    m_single = false;
    m_bar->hide();
    if (!isWindow() || parentWidget()) {
        hideDisplay();
        setParent(nullptr, Qt::Window | Qt::FramelessWindowHint);
    }
    // Как uprSpace при wLeft=1920: весь второй монитор, без рамки.
    // Второго монитора нет — на весь основной (Esc скрывает, у врача появляется «Экран пациента»).
    QScreen *screen = secondaryScreen();
    if (!screen) {
        screen = QGuiApplication::primaryScreen();
    }
    // Окно уже показано на другом мониторе в полноэкранном режиме — перенос надёжнее через hide().
    if (isVisible() && windowHandle() && windowHandle()->screen() != screen) {
        hideDisplay();
    }
    if (!windowHandle()) {
        create();
    }
    if (windowHandle() && screen) {
        windowHandle()->setScreen(screen);
    }
    if (screen) {
        setGeometry(screen->geometry());
    }
    showFullScreen();
    layoutContent();
    raise();
    if (!secondaryScreen()) {
        activateWindow();
        setFocus();
    }
}

void PatientWindow::showEmbedded(QWidget *container, const QRect &rect) {
    m_single = true;
    if (parentWidget() != container) {
        hideDisplay();
        setParent(container, Qt::Widget);
    }
    setGeometry(rect);
    layoutContent();
    layoutControlBar();
    show();
    raise();
    setFocus();
}

void PatientWindow::hideDisplay() {
    m_programmaticClose = true;
    hide();
    m_programmaticClose = false;
}

void PatientWindow::closeEvent(QCloseEvent *event) {
    event->ignore();
    hide();
    if (!m_programmaticClose) {
        emit closedByUser();
    }
}

void PatientWindow::keyPressEvent(QKeyEvent *event) {
    if (event->key() == Qt::Key_Escape) {
        if (m_single || !secondaryScreen()) {
            emit doctorPanelRequested();
            return;
        }
    }
    QWidget::keyPressEvent(event);
}
