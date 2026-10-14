#include "wolfengine.h"

#include <QTimer>

WolfEngine::WolfEngine(QObject *parent) : ExEngine(parent) {
    m_timer = new QTimer(this);
    m_timer->setInterval(12);
    connect(m_timer, &QTimer::timeout, this, &WolfEngine::tick);
}

void WolfEngine::rebuild() {
    clear();
    m_timer->stop();
    m_animating = false;
    block = false;
    m_left.clear();
    m_right.clear();
    m_load.clear();
    m_shipSide = QStringLiteral("left");
    background = QColor(249, 249, 249);

    auto img = [&](const QString &file, int x, int y, const QString &name, bool movable) {
        const int i = addSprite();
        sprites[i].setImage(ExAssets::exnPath(QStringLiteral("2.2.5/") + file));
        sprites[i].x = x;
        sprites[i].y = y;
        sprites[i].name = name;
        sprites[i].movable = movable;
        return i;
    };
    img(QStringLiteral("water.png"), 210 * 100 / 60, 400 * 100 / 60, QStringLiteral("water"), false);
    img(QStringLiteral("side.png"), 40 * 100 / 60, 400 * 100 / 60, QStringLiteral("leftside"), false);
    img(QStringLiteral("side.png"), 540 * 100 / 60, 400 * 100 / 60, QStringLiteral("rightside"), false);
    int i = img(QStringLiteral("wolf.png"), 140 * 100 / 60, 0, QStringLiteral("wolf"), true);
    sprites[i].y = 400 * 100 / 60 - sprites[i].h;
    i = img(QStringLiteral("goat.png"), 40 * 100 / 60, 0, QStringLiteral("goat"), true);
    sprites[i].y = 400 * 100 / 60 - sprites[i].h;
    i = img(QStringLiteral("cab.png"), 200 * 100 / 60, 353 * 100 / 60, QStringLiteral("cab"), true);
    i = img(QStringLiteral("ship.png"), 260 * 100 / 60, 0, QStringLiteral("ship"), false);
    sprites[i].y = 400 * 100 / 60 - sprites[i].h;
    const int shipH = sprites[i].h;
    i = img(QStringLiteral("man.png"), 400 * 100 / 60, 0, QStringLiteral("man"), false);
    sprites[i].y = 400 * 100 / 60 - sprites[i].h - shipH;
    // uprSpace: wl.Left=240, wl.Top=-30; pictureBox1 внутри wolf — (39,32) => начало координат спрайтов (279,2)
    for (ExSprite &s : sprites) {
        s.x += 240 + 39;
        s.y += -30 + 32;
    }
    // arrayLeft/arrayRight — отдельные контролы wolf: Left 410/940, Top 900 (в координатах wl)
    img(QStringLiteral("arrayLeft.png"), 240 + 410, -30 + 900, QStringLiteral("arrayLeft"), false);
    img(QStringLiteral("arrayRight.png"), 240 + 940, -30 + 900, QStringLiteral("arrayRight"), false);
    // textBox1 (wolf.resx): Left 450, Top 170, 606x170, FixedSingle, 14pt — условие задачи; потом «Верно»
    {
        sprites.prepend(ExSprite()); // под остальными спрайтами, чтобы не перехватывать клики
        const int t = 0;
        sprites[t].setBox(QStringLiteral("Крестьянину нужно перевезти через реку волка, козу и капусту. \n"
                                         "В лодке, кроме крестьянина, может поместиться только один объект - или волк, "
                                         "или коза, или капуста.  Если крестьянин оставит без присмотра волка с козой, то волк "
                                         "съест козу; если крестьянин оставит без присмотра козу с капустой, коза съест капусту. \n"
                                         "Как ему перевезти всех на другой берег?"),
                          606, 170, 14);
        sprites[t].x = 240 + 450;
        sprites[t].y = -30 + 170;
        sprites[t].name = QStringLiteral("msg");
    }

    m_left = QStringList{QStringLiteral("goat"), QStringLiteral("wolf"), QStringLiteral("cab")};
    arrangeLeft();
    emit changed();
}

void WolfEngine::arrangeLeft() {
    const int side = idx(QStringLiteral("leftside"));
    int x = sprites[side].x;
    for (const QString &n : m_left) {
        const int i = idx(n);
        sprites[i].x = x;
        sprites[i].y = sprites[side].y - sprites[i].h;
        x += sprites[i].w;
    }
}

void WolfEngine::arrangeRight() {
    const int side = idx(QStringLiteral("rightside"));
    int x = sprites[side].x + sprites[side].w;
    for (const QString &n : m_right) {
        const int i = idx(n);
        sprites[i].x = x - sprites[i].w;
        sprites[i].y = sprites[side].y - sprites[i].h;
        x = sprites[i].x;
    }
}

void WolfEngine::showMessage(const QString &text) {
    // textBox1.Text = "Верно" (шрифт 26) в том же поле
    const int i = indexByName(QStringLiteral("msg"));
    if (i >= 0) {
        sprites[i].setBox(text, 606, 170, 26);
    }
}

bool WolfEngine::checkDie(const QStringList &bank) {
    const bool wolf = bank.contains(QStringLiteral("wolf"));
    const bool goat = bank.contains(QStringLiteral("goat"));
    const bool cab = bank.contains(QStringLiteral("cab"));
    bool die = false;
    if (wolf && goat) {
        sprites[idx(QStringLiteral("goat"))].red = true;
        sprites[idx(QStringLiteral("wolf"))].red = true;
        die = true;
    }
    if (goat && cab) {
        sprites[idx(QStringLiteral("goat"))].red = true;
        sprites[idx(QStringLiteral("cab"))].red = true;
        die = true;
    }
    if (die) {
        // wolf.cs checkDie*: красные спрайты, звук, setResults(true); сообщения нет, block не ставится
        ExAssets::playIncorrect();
        emit resultRecorded(true);
    }
    return die;
}

void WolfEngine::checkCorrect() {
    if (m_right.contains(QStringLiteral("wolf")) && m_right.contains(QStringLiteral("goat")) &&
        m_right.contains(QStringLiteral("cab"))) {
        ExAssets::playCorrect();
        showMessage(QStringLiteral("Верно"));
        block = true;
        emit resultRecorded(false);
    }
}

void WolfEngine::startCrossing() {
    if (block || m_animating) {
        return;
    }
    m_animating = true;
    m_timer->start();
}

void WolfEngine::tick() {
    const int ship = idx(QStringLiteral("ship"));
    const int man = idx(QStringLiteral("man"));
    const int load = m_load.isEmpty() ? -1 : idx(m_load);
    const int step = m_shipSide == QLatin1String("left") ? 5 : -5;
    sprites[ship].x += step;
    sprites[man].x += step;
    if (load >= 0) {
        sprites[load].x += step;
    }
    bool arrived = false;
    if (m_shipSide == QLatin1String("left") && sprites[ship].x > 580) {
        m_shipSide = QStringLiteral("right");
        arrived = true;
        m_timer->stop();
        m_animating = false;
        checkDie(m_left);
    } else if (m_shipSide == QLatin1String("right") && sprites[ship].x < 420) {
        m_shipSide = QStringLiteral("left");
        arrived = true;
        m_timer->stop();
        m_animating = false;
        checkDie(m_right);
    }
    Q_UNUSED(arrived);
    emit changed();
}

void WolfEngine::patientPress(const QPoint &p) {
    m_moving = -1;
    m_dragging = false;
    if (block || m_animating) {
        return;
    }
    const int i = topAt(p, true);
    if (i < 0) {
        return;
    }
    const QString n = sprites[i].name;
    if (n == QLatin1String("arrayRight")) {
        if (m_shipSide == QLatin1String("left")) {
            startCrossing();
        }
        return;
    }
    if (n == QLatin1String("arrayLeft")) {
        if (m_shipSide == QLatin1String("right")) {
            startCrossing();
        }
        return;
    }
    if (n == QLatin1String("wolf") || n == QLatin1String("cab") || n == QLatin1String("goat")) {
        m_sPos = QPoint(sprites[i].x, sprites[i].y);
        m_off = QPoint(p.x() - sprites[i].x, p.y() - sprites[i].y);
        m_moving = i;
        m_dragging = true;
    }
}

void WolfEngine::patientRelease(const QPoint &p) {
    const int mv = m_moving;
    const bool was = m_dragging;
    m_dragging = false;
    if (mv < 0 || !was || block || m_animating) {
        return;
    }
    Q_UNUSED(p);
    const QString name = sprites[mv].name;
    const bool left = m_shipSide == QLatin1String("left");
    QStringList &bank = left ? m_left : m_right;
    const int ship = idx(QStringLiteral("ship"));
    const int bankSide = idx(left ? QStringLiteral("leftside") : QStringLiteral("rightside"));
    bool action = false;
    const bool onShip = sprites[mv].x > sprites[ship].x - 40 && sprites[mv].x < sprites[ship].x + 200 &&
                        qAbs(sprites[mv].y - (sprites[ship].y - sprites[mv].h)) < 160;
    const bool onBank = sprites[mv].x > sprites[bankSide].x - 60 &&
                        sprites[mv].x < sprites[bankSide].x + sprites[bankSide].w;
    if (name == m_load) {
        // из лодки на берег (только на текущий берег)
        if (onBank && !onShip) {
            bank.append(name);
            m_load.clear();
            if (left) {
                arrangeLeft();
            } else {
                arrangeRight();
            }
            action = true;
            if (!left) {
                checkCorrect();
            }
        }
    } else if (bank.contains(name) && onShip) {
        if (!m_load.isEmpty()) {
            bank.append(m_load);
        }
        bank.removeAll(name);
        m_load = name;
        sprites[mv].x = sprites[ship].x + 50;
        sprites[mv].y = sprites[ship].y - sprites[mv].h;
        if (left) {
            arrangeLeft();
        } else {
            arrangeRight();
        }
        action = true;
    }
    if (!action) {
        sprites[mv].x = m_sPos.x();
        sprites[mv].y = m_sPos.y();
    }
    emit changed();
}
