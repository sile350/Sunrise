#include "exengine.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFontMetrics>
#include <QHash>
#include <QMediaPlayer>
#include <QMouseEvent>
#include <QPainter>
#include <QSoundEffect>
#include <QTextStream>
#include <QTimer>
#include <QUrl>

namespace ExAssets {

QString assetsDir() {
    static QString cached;
    if (!cached.isEmpty()) {
        return cached;
    }
    const QString app = QCoreApplication::applicationDirPath();
    const QStringList roots = {
        app + QStringLiteral("/assets"),
        app + QStringLiteral("/../assets"),
        app + QStringLiteral("/../../assets"),
        app + QStringLiteral("/../../../assets"),
        QDir::currentPath() + QStringLiteral("/assets"),
    };
    for (const QString &root : roots) {
        if (QFileInfo::exists(root + QStringLiteral("/exn")) || QFileInfo::exists(root + QStringLiteral("/sysImages"))) {
            cached = QDir::cleanPath(root);
            return cached;
        }
    }
    cached = QDir::cleanPath(app + QStringLiteral("/assets"));
    return cached;
}

QString sysImage(const QString &name) {
    return assetsDir() + QStringLiteral("/sysImages/") + name;
}

QString exnPath(const QString &relative) {
    return assetsDir() + QStringLiteral("/exn/") + relative;
}

QString resourceImage(const QString &name) {
    return assetsDir() + QStringLiteral("/resources/") + name;
}

QPixmap pixmap(const QString &path) {
    static QHash<QString, QPixmap> cache;
    auto it = cache.constFind(path);
    if (it != cache.constEnd()) {
        return it.value();
    }
    QPixmap pm(path);
    cache.insert(path, pm);
    return pm;
}

QString readAll(const QString &path) {
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) {
        return {};
    }
    QByteArray data = f.readAll();
    if (data.startsWith("\xEF\xBB\xBF")) {
        data.remove(0, 3);
    }
    return QString::fromUtf8(data);
}

QStringList readLines(const QString &path) {
    QString text = readAll(path);
    text.replace(QStringLiteral("\r\n"), QStringLiteral("\n"));
    text.replace(QLatin1Char('\r'), QLatin1Char('\n'));
    QStringList lines = text.split(QLatin1Char('\n'));
    while (!lines.isEmpty() && lines.last().trimmed().isEmpty()) {
        lines.removeLast();
    }
    return lines;
}

static QMediaPlayer *player() {
    static QMediaPlayer *p = new QMediaPlayer(qApp);
    return p;
}

static void playFile(const QString &file) {
    if (!QFileInfo::exists(file)) {
        return;
    }
    QMediaPlayer *p = player();
    p->stop();
    p->setMedia(QUrl::fromLocalFile(file));
    p->setVolume(100);
    p->play();
}

void playCorrect() { playFile(sysImage(QStringLiteral("correct.mp3"))); }
void playIncorrect() { playFile(sysImage(QStringLiteral("incorrect.mp3"))); }

void playMetronome(bool start, int bpm) {
    static QTimer *timer = nullptr;
    static QSoundEffect *effect = nullptr;
    if (!timer) {
        timer = new QTimer(qApp);
        effect = new QSoundEffect(qApp);
        effect->setSource(QUrl::fromLocalFile(exnPath(QStringLiteral("1.2.1/sound.wav"))));
        QObject::connect(timer, &QTimer::timeout, qApp, []() { effect->play(); });
    }
    if (!start) {
        timer->stop();
        return;
    }
    // _121.trackBar1_Scroll: interval = 60000 / (30 + (v-1)*22); первый звук — по первому тику таймера
    timer->setInterval(60000 / qMax(1, bpm));
    if (!timer->isActive()) {
        timer->start();
    }
}

} // namespace ExAssets

// ---------------- ExSprite ----------------

void ExSprite::setImage(const QString &file, int scalePercent) {
    path = file;
    isText = false;
    QPixmap src = ExAssets::pixmap(file);
    if (src.isNull()) {
        pix = QPixmap();
        w = h = 0;
        return;
    }
    if (scalePercent != 100) {
        pix = src.scaled(src.width() * scalePercent / 100, src.height() * scalePercent / 100,
                         Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    } else {
        pix = src;
    }
    w = pix.width();
    h = pix.height();
}

void ExSprite::setText(const QString &text, int fontSize, const QColor &c, int scalePercent, bool italic) {
    word = text;
    color = c;
    isText = true;
    QFont font(QStringLiteral("Arial"));
    font.setItalic(italic);
    font.setPixelSize(qMax(6, qRound(fontSize * 96.0 / 72.0 * scalePercent / 100.0)));
    QFontMetrics fm(font);
    const int px = font.pixelSize();
    const int pad = qRound(px * 0.16);
    const int tw = fm.horizontalAdvance(text) + pad * 2;
    const int th = fm.height() + qRound(px * 0.1);
    pix = QPixmap(qMax(1, tw), qMax(1, th));
    pix.fill(Qt::transparent);
    QPainter p(&pix);
    p.setRenderHint(QPainter::TextAntialiasing, true);
    p.setFont(font);
    p.setPen(c);
    p.drawText(QRect(pad, 0, tw, th), Qt::AlignLeft | Qt::AlignVCenter | Qt::TextSingleLine, text);
    p.end();
    w = pix.width();
    h = pix.height();
}

void ExSprite::setBox(const QString &text, int width, int height, int pointSize, bool bordered, bool bold) {
    word = text;
    isText = true;
    QFont font(QStringLiteral("Arial"));
    font.setBold(bold);
    font.setPixelSize(qMax(6, qRound(pointSize * 96.0 / 72.0)));
    pix = QPixmap(qMax(1, width), qMax(1, height));
    pix.fill(Qt::transparent);
    QPainter p(&pix);
    p.setRenderHint(QPainter::TextAntialiasing, true);
    p.setFont(font);
    p.setPen(Qt::black);
    p.drawText(QRect(3, 1, width - 6, height - 2), Qt::AlignLeft | Qt::AlignTop | Qt::TextWordWrap, text);
    if (bordered) {
        p.setPen(QPen(QColor(0, 0, 0), 1));
        p.drawRect(0, 0, width - 1, height - 1);
    }
    p.end();
    w = pix.width();
    h = pix.height();
}

// ---------------- ExEngine ----------------

int ExEngine::addSprite() {
    sprites.append(ExSprite());
    return sprites.size() - 1;
}

int ExEngine::indexByName(const QString &name) const {
    for (int i = 0; i < sprites.size(); ++i) {
        if (sprites[i].name == name) {
            return i;
        }
    }
    return -1;
}

int ExEngine::topAt(const QPoint &p, bool patientView) const {
    int found = -1;
    for (int i = 0; i < sprites.size(); ++i) {
        const ExSprite &s = sprites[i];
        if (patientView && s.doctorOnly) {
            continue;
        }
        if (!patientView && s.patientOnly) {
            continue;
        }
        if (s.hit(p.x(), p.y())) {
            found = i;
        }
    }
    return found;
}

void ExEngine::patientPress(const QPoint &p) {
    m_moving = -1;
    m_dragging = false;
    if (block) {
        return;
    }
    const int i = topAt(p, true);
    if (i < 0) {
        return;
    }
    m_moving = i;
    m_off = QPoint(p.x() - sprites[i].x, p.y() - sprites[i].y);
    m_start = QPoint(sprites[i].x, sprites[i].y);
    m_dragging = true;
}

void ExEngine::patientMove(const QPoint &p) {
    if (m_moving < 0 || !m_dragging || m_moving >= sprites.size()) {
        return;
    }
    ExSprite &s = sprites[m_moving];
    if (!s.movable) {
        return;
    }
    s.x = p.x() - m_off.x();
    s.y = p.y() - m_off.y();
    emit changed();
}

void ExEngine::patientRelease(const QPoint &) {
    m_dragging = false;
}

void ExEngine::doctorClick(const QPoint &) {}

void ExEngine::addDoneButton(int x, int y) {
    const int i = addSprite();
    sprites[i].setImage(ExAssets::sysImage(QStringLiteral("done.png")));
    sprites[i].x = x;
    sprites[i].y = y;
    sprites[i].name = QStringLiteral("done");
}

void ExEngine::clearMarks() {
    for (ExSprite &s : sprites) {
        s.red = s.green = false;
    }
}

void ExEngine::finishDone(bool wrong) {
    block = true;
    const int idx = indexByName(QStringLiteral("done"));
    if (idx >= 0) {
        const QString file = ExAssets::sysImage(wrong ? QStringLiteral("incorrect.png") : QStringLiteral("correct.png"));
        const int ox = sprites[idx].x;
        const int oy = sprites[idx].y;
        sprites[idx].setImage(file);
        sprites[idx].x = ox;
        sprites[idx].y = oy;
    }
    if (wrong) {
        ExAssets::playIncorrect();
    } else {
        ExAssets::playCorrect();
    }
    emit resultRecorded(wrong);
    emit changed();
}

// ---------------- ExCanvas ----------------

ExCanvas::ExCanvas(bool patient, QWidget *parent) : QWidget(parent), m_patient(patient) {
    setMouseTracking(false);
}

void ExCanvas::setEngine(ExEngine *engine) {
    if (m_engine) {
        disconnect(m_engine, nullptr, this, nullptr);
    }
    m_engine = engine;
    if (m_engine) {
        connect(m_engine, &ExEngine::changed, this, QOverload<>::of(&QWidget::update));
    }
    update();
}

double ExCanvas::scaleFactor() const {
    if (!m_engine) {
        return 1.0;
    }
    const double sx = double(width()) / m_engine->sceneSize.width();
    const double sy = double(height()) / m_engine->sceneSize.height();
    return qMin(sx, sy);
}

QPointF ExCanvas::originOffset() const {
    if (!m_engine) {
        return {};
    }
    const double s = scaleFactor();
    return QPointF((width() - m_engine->sceneSize.width() * s) / 2.0,
                   (height() - m_engine->sceneSize.height() * s) / 2.0);
}

QPointF ExCanvas::toScene(const QPoint &p) const {
    const double s = scaleFactor();
    const QPointF o = originOffset();
    return QPointF((p.x() - o.x()) / s, (p.y() - o.y()) / s);
}

void ExCanvas::paintEvent(QPaintEvent *) {
    QPainter p(this);
    p.fillRect(rect(), m_patient ? QColor(249, 249, 249) : QColor(235, 235, 235));
    if (!m_engine) {
        return;
    }
    const double s = scaleFactor();
    const QPointF o = originOffset();
    p.setRenderHint(QPainter::SmoothPixmapTransform, true);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.translate(o);
    p.scale(s, s);
    p.fillRect(QRect(QPoint(0, 0), m_engine->sceneSize), m_engine->background);
    if (!m_patient) {
        p.setPen(QPen(QColor(180, 180, 180), 2 / s));
        p.drawRect(QRect(QPoint(0, 0), m_engine->sceneSize));
    }
    for (const ExSprite &sp : m_engine->sprites) {
        if (!sp.visible) {
            continue;
        }
        if (m_patient && sp.doctorOnly) {
            continue;
        }
        if (!m_patient && sp.patientOnly) {
            continue;
        }
        const QPixmap &pm = (!m_patient && !sp.doctorPix.isNull()) ? sp.doctorPix : sp.pix;
        if (!pm.isNull()) {
            p.drawPixmap(sp.x, sp.y, pm);
        }
        const QRect r(sp.x, sp.y, sp.w, sp.h);
        if (sp.green) {
            p.fillRect(r, QColor(51, 204, 0, 125));
        }
        if (sp.red) {
            p.fillRect(r, QColor(255, 0, 0, 125));
        }
        if (sp.border) {
            QPen pen(QColor(0, 102, 255), 8);
            p.setPen(pen);
            p.setBrush(Qt::NoBrush);
            p.drawRect(r.adjusted(3, 3, -3, -3));
        }
        if (!m_patient && sp.dborder) {
            QPen pen(QColor(255, 140, 0), 8, Qt::DashLine);
            p.setPen(pen);
            p.setBrush(Qt::NoBrush);
            p.drawRect(r.adjusted(3, 3, -3, -3));
        }
    }
}

void ExCanvas::mousePressEvent(QMouseEvent *e) {
    if (!m_engine || e->button() != Qt::LeftButton) {
        return;
    }
    if (m_patient) {
        m_engine->patientPress(toScene(e->pos()).toPoint());
    }
}

void ExCanvas::mouseMoveEvent(QMouseEvent *e) {
    if (m_engine && m_patient && (e->buttons() & Qt::LeftButton)) {
        m_engine->patientMove(toScene(e->pos()).toPoint());
    }
}

void ExCanvas::mouseReleaseEvent(QMouseEvent *e) {
    if (!m_engine || e->button() != Qt::LeftButton) {
        return;
    }
    const QPoint sp = toScene(e->pos()).toPoint();
    if (m_patient) {
        m_engine->patientRelease(sp);
    } else {
        m_engine->doctorClick(sp);
    }
    update();
}
