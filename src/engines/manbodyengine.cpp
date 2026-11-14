#include "manbodyengine.h"

#include <QStringList>

namespace {

struct Def {
    QString key;
    int x;
    int y;
};

// Ключи и координаты — man.doctorInit, barray.
QVector<Def> frontDefs() {
    return {{QStringLiteral("Голова1"), 205, 10},
            {QStringLiteral("Шея1"), 225, 131},
            {QStringLiteral("Грудь1"), 163, 194},
            {QStringLiteral("Живот1"), 170, 303},
            {QStringLiteral("левоеплечо1"), 325, 199},
            {QStringLiteral("правоеплечо1"), 122, 187},
            {QStringLiteral("левоеколено1"), 283, 681},
            {QStringLiteral("правоеколено1"), 167, 685},
            {QStringLiteral("леваянога1"), 254, 547},
            {QStringLiteral("праваянога1"), 140, 545},
            {QStringLiteral("леваярука1"), 347, 263},
            {QStringLiteral("праваярука1"), 100, 263}};
}

QVector<Def> backDefs() {
    return {{QStringLiteral("голова2"), 215, 14},
            {QStringLiteral("шея2"), 218, 133},
            {QStringLiteral("левыйлокоть2"), 104, 355},
            {QStringLiteral("правыйлокоть2"), 366, 358},
            {QStringLiteral("леваянога2"), 151, 551},
            {QStringLiteral("праваянога2"), 256, 550},
            {QStringLiteral("левоеплечо2"), 116, 185},
            {QStringLiteral("правоеплечо2"), 340, 185},
            {QStringLiteral("леваярука2"), 100, 250},
            {QStringLiteral("праваярука2"), 353, 256},
            {QStringLiteral("спина2"), 149, 166}};
}

// mpictures initEx "5.1.4": закомментированные части в список не входят.
QStringList labelNames() {
    return QStringList{QStringLiteral("голова"),     QStringLiteral("грудь"),      QStringLiteral("живот"),
                       QStringLiteral("леваянога"),  QStringLiteral("леваярука"),  QStringLiteral("левоеплечо"),
                       QStringLiteral("праваянога"), QStringLiteral("праваярука"), QStringLiteral("правоеколено"),
                       QStringLiteral("спина"),      QStringLiteral("шея")};
}

const QColor kLabelSelect(225, 249, 32, 150);
const QColor kLabelBlue(5, 181, 254, 150);
const QColor kClear(0, 0, 0, 0);

QString stripDigit(const QString &key) {
    if (!key.isEmpty() && key.back().isDigit()) {
        return key.left(key.size() - 1);
    }
    return key;
}

bool hitBox(const ExSprite &s, const QPoint &p) {
    return s.w > 0 && s.h > 0 && p.x() >= s.x && p.x() <= s.x + s.w && p.y() >= s.y && p.y() <= s.y + s.h;
}

} // namespace

void ManBodyEngine::rebuild() {
    clear();
    block = false;
    m_front.clear();
    m_back.clear();
    m_doctorFront = m_doctorBack = m_patientFront = m_patientBack = -1;
    m_selectedLabel.clear();
    m_chipFile.clear();
    m_graded = false;
    m_lastCorrect = false;
    background = QColor(245, 245, 245);

    buildFigure(true, 310, 50);
    buildFigure(false, 1140, 50);

    QString prompt = QStringLiteral("Покажите заданную часть тела");
    if (mode == QLatin1String("sub")) {
        prompt = QStringLiteral("Подпишите заданную часть тела");
        buildLabels();
    } else if (mode == QLatin1String("3")) {
        prompt = QStringLiteral("Покажите заданную часть тела на себе");
    }
    const int title = addSprite();
    sprites[title].setText(prompt, 22);
    sprites[title].name = QStringLiteral("prompt");
    sprites[title].y = 3;
    sprites[title].setCenterX(sceneSize.width());

    if (mode != QLatin1String("3")) {
        addDoneButton(1620 - 17, 844 - 75);
    }
    emit changed();
}

void ManBodyEngine::buildFigure(bool front, int hostX, int hostY) {
    constexpr int ratio = 100;
    const int ox = hostX + 3;
    const int oy = hostY + 3;
    const int body = addSprite();
    sprites[body].setImage(ExAssets::exnPath(front ? QStringLiteral("5.1.4/man1.png")
                                                    : QStringLiteral("5.1.4/man2.png")));
    sprites[body].x = ox + 100 * ratio / 100;
    sprites[body].y = oy + 10 * ratio / 100;
    sprites[body].name = QStringLiteral("fone");

    const QVector<Def> defs = front ? frontDefs() : backDefs();
    QVector<Part> &out = front ? m_front : m_back;
    for (int n = 0; n < defs.size(); ++n) {
        const QString key = defs[n].key;
        const QString name2 = stripDigit(key);
        const int x = ox + defs[n].x * ratio / 100;
        const int y = oy + defs[n].y * ratio / 100;
        auto addOverlay = [&](const QString &prefix, const QString &color) {
            const int s = addSprite();
            sprites[s].setImage(ExAssets::exnPath(QStringLiteral("5.1.4/") + prefix + key + QStringLiteral(".png")));
            sprites[s].x = x;
            sprites[s].y = y;
            sprites[s].name = color + QString::number(n);
            sprites[s].visible = false;
            if (prefix == QLatin1String("b")) {
                sprites[s].name2 = name2;
            }
            return s;
        };
        Part part;
        part.key = key;
        part.name2 = name2;
        part.blue = addOverlay(QStringLiteral("b"), QStringLiteral("blue"));
        part.red = addOverlay(QStringLiteral("r"), QStringLiteral("red"));
        part.green = addOverlay(QStringLiteral("g"), QStringLiteral("green"));
        out.append(part);
    }
}

void ManBodyEngine::buildLabels() {
    // movingPictures (-80, 50) + pictureBox (48, 14); спрайт x=50, y с 40, шаг 60.
    constexpr int ox = -80 + 48;
    constexpr int oy = 50 + 14;
    // Форма для размещения выбранной карточки-подписи (справа от колонки).
    const int slot = addSprite();
    sprites[slot].setImage(ExAssets::exnPath(QStringLiteral("5.1.4/chp.png")));
    sprites[slot].x = ox + 50 + 220;
    sprites[slot].y = oy + 40;
    sprites[slot].name = QStringLiteral("labelSlot");
    int y = 40;
    const QStringList labels = labelNames();
    for (const QString &name : labels) {
        const int i = addSprite();
        sprites[i].setImage(ExAssets::exnPath(QStringLiteral("5.1.4/") + name + QStringLiteral(".png")));
        sprites[i].x = ox + 50;
        sprites[i].y = oy + y;
        sprites[i].name = name;
        sprites[i].name2 = QStringLiteral("label");
        y += 60;
    }
}

void ManBodyEngine::applyFlag(int sprite, bool doctor, bool patient) {
    if (sprite < 0 || sprite >= sprites.size()) {
        return;
    }
    ExSprite &s = sprites[sprite];
    if (!doctor && !patient) {
        s.visible = false;
        s.doctorOnly = false;
        s.patientOnly = false;
        return;
    }
    s.visible = true;
    s.doctorOnly = doctor && !patient;
    s.patientOnly = patient && !doctor;
}

void ManBodyEngine::applyAll() {
    auto applyList = [this](QVector<Part> &list) {
        for (Part &p : list) {
            applyFlag(p.blue, p.bD, p.bP);
            applyFlag(p.red, p.rD, p.rP);
            applyFlag(p.green, p.gD, p.gP);
        }
    };
    applyList(m_front);
    applyList(m_back);
}

void ManBodyEngine::clearFlags(Part &part) {
    part.bD = part.bP = part.rD = part.rP = part.gD = part.gP = false;
}

void ManBodyEngine::clearDoctorSide(bool front) {
    QVector<Part> &list = front ? m_front : m_back;
    const bool shared = mode == QLatin1String("sub") || mode == QLatin1String("3");
    for (Part &p : list) {
        p.bD = p.rD = p.gD = false;
        if (shared) {
            p.bP = p.rP = p.gP = false;
        }
    }
    if (front) {
        m_doctorFront = -1;
        if (shared) {
            m_patientFront = -1;
        }
    } else {
        m_doctorBack = -1;
        if (shared) {
            m_patientBack = -1;
        }
    }
}

void ManBodyEngine::clearPatientColors() {
    for (Part &p : m_front) {
        p.bP = p.rP = p.gP = false;
    }
    for (Part &p : m_back) {
        p.bP = p.rP = p.gP = false;
    }
    m_patientFront = m_patientBack = -1;
}

void ManBodyEngine::showColor(bool front, int part, QChar color) {
    QVector<Part> &list = front ? m_front : m_back;
    if (part < 0 || part >= list.size()) {
        return;
    }
    Part &p = list[part];
    if (color == QLatin1Char('r')) {
        p.rD = p.rP = true;
    } else if (color == QLatin1Char('g')) {
        p.gD = p.gP = true;
    } else {
        p.bD = p.bP = true;
    }
}

QString ManBodyEngine::partName2(bool front, int part) const {
    const QVector<Part> &list = front ? m_front : m_back;
    if (part < 0 || part >= list.size()) {
        return {};
    }
    return list[part].name2;
}

int ManBodyEngine::partAt(const QPoint &p, bool *frontOut) const {
    // Сначала точное попадание в bbox оверлея (даже если он скрыт).
    int found = -1;
    bool foundFront = true;
    for (int i = 0; i < m_front.size(); ++i) {
        if (hitBox(sprites[m_front[i].blue], p)) {
            found = i;
            foundFront = true;
        }
    }
    for (int i = 0; i < m_back.size(); ++i) {
        if (hitBox(sprites[m_back[i].blue], p)) {
            found = i;
            foundFront = false;
        }
    }
    // Клик по силуэту / рядом: ближайший центр оверлея — иначе на зеркале врача
    // мелкие части почти не ловятся, и «Готово» молча ничего не делает (нет выбора врача).
    if (found < 0) {
        auto dist2 = [&](const ExSprite &b) -> qint64 {
            const qint64 cx = b.x + b.w / 2;
            const qint64 cy = b.y + b.h / 2;
            const qint64 dx = p.x() - cx;
            const qint64 dy = p.y() - cy;
            return dx * dx + dy * dy;
        };
        constexpr qint64 kMax = 200LL * 200LL;
        qint64 bestDist = kMax + 1;
        auto consider = [&](const QVector<Part> &list, bool frontSide) {
            for (int i = 0; i < list.size(); ++i) {
                const ExSprite &b = sprites[list[i].blue];
                if (b.w <= 0 || b.h <= 0) {
                    continue;
                }
                const qint64 d = dist2(b);
                if (d <= kMax && d < bestDist) {
                    bestDist = d;
                    found = i;
                    foundFront = frontSide;
                }
            }
        };
        consider(m_front, true);
        consider(m_back, false);
    }
    if (found >= 0 && frontOut) {
        *frontOut = foundFront;
    }
    return found;
}

int ManBodyEngine::labelIndex(const QString &name) const {
    for (int i = 0; i < sprites.size(); ++i) {
        if (sprites[i].name2 == QLatin1String("label") &&
            sprites[i].name.compare(name, Qt::CaseInsensitive) == 0) {
            return i;
        }
    }
    return -1;
}

int ManBodyEngine::labelAt(const QPoint &p) const {
    int found = -1;
    for (int i = 0; i < sprites.size(); ++i) {
        if (sprites[i].name2 == QLatin1String("label") && sprites[i].visible && hitBox(sprites[i], p)) {
            found = i;
        }
    }
    return found;
}

void ManBodyEngine::paintLabel(const QString &name, const QColor &tint, bool green, bool red) {
    const int i = labelIndex(name);
    if (i < 0) {
        return;
    }
    sprites[i].tint = tint;
    sprites[i].green = green;
    sprites[i].red = red;
}

void ManBodyEngine::doctorChoose(bool front, int part) {
    if (block) {
        return;
    }
    QVector<Part> &list = front ? m_front : m_back;
    if (part < 0 || part >= list.size()) {
        return;
    }
    // Повторный клик по той же части — снять выделение (руководство).
    int &sel = front ? m_doctorFront : m_doctorBack;
    if (sel == part) {
        list[part].bD = false;
        if (mode == QLatin1String("sub") || mode == QLatin1String("3")) {
            list[part].bP = false;
        }
        sel = -1;
        applyAll();
        emit doctorSelectionChanged();
        emit changed();
        return;
    }
    if (front && m_doctorBack >= 0) {
        clearDoctorSide(false);
    } else if (!front && m_doctorFront >= 0) {
        clearDoctorSide(true);
    }
    if (sel >= 0 && sel < list.size()) {
        list[sel].bD = false;
        if (mode == QLatin1String("sub") || mode == QLatin1String("3")) {
            list[sel].bP = false;
        }
    }
    sel = part;
    list[part].bD = true;
    // «Показать»: выделение врача только у специалиста. «Подписать»/«На себе»: и у пациента.
    if (mode == QLatin1String("sub")) {
        clearPatientColors();
        list[part].bP = true;
    } else if (mode == QLatin1String("3")) {
        list[part].bP = true;
    }
    applyAll();
    emit doctorSelectionChanged();
    emit changed();
}

void ManBodyEngine::patientChoose(bool front, int part) {
    if (block || mode != QLatin1String("")) {
        return;
    }
    QVector<Part> &list = front ? m_front : m_back;
    if (part < 0 || part >= list.size()) {
        return;
    }
    // Повторный клик — снять выбор пациента.
    const int cur = front ? m_patientFront : m_patientBack;
    if (cur == part) {
        list[part].bP = false;
        // Не трогаем bD, если это одновременно выбор врача.
        const int doc = front ? m_doctorFront : m_doctorBack;
        if (part != doc) {
            list[part].bD = false;
        }
        if (front) {
            m_patientFront = -1;
        } else {
            m_patientBack = -1;
        }
        applyAll();
        emit changed();
        return;
    }
    auto hidePrevPatient = [this](bool side, int idx) {
        if (idx < 0) {
            return;
        }
        QVector<Part> &ps = side ? m_front : m_back;
        if (idx >= ps.size()) {
            return;
        }
        ps[idx].bP = false;
        const int doc = side ? m_doctorFront : m_doctorBack;
        // Не сбрасывать голубой врача, если пациент кликал по той же части.
        if (idx != doc) {
            ps[idx].bD = false;
        }
    };
    hidePrevPatient(true, m_patientFront);
    hidePrevPatient(false, m_patientBack);
    m_patientFront = m_patientBack = -1;
    if (front) {
        m_patientFront = part;
    } else {
        m_patientBack = part;
    }
    // Выбор пациента виден на обоих экранах.
    list[part].bP = true;
    list[part].bD = true;
    applyAll();
    emit changed();
}

void ManBodyEngine::chooseLabel(const QString &name) {
    if (block || mode != QLatin1String("sub")) {
        return;
    }
    const int idx = labelIndex(name);
    if (idx < 0) {
        return;
    }
    const int slot = indexByName(QStringLiteral("labelSlot"));
    // Вернуть предыдущую карточку на место в колонке.
    if (!m_selectedLabel.isEmpty()) {
        const int prev = labelIndex(m_selectedLabel);
        if (prev >= 0) {
            // Восстановить Y по порядку в списке.
            const QStringList names = labelNames();
            const int ord = names.indexOf(m_selectedLabel);
            constexpr int ox = -80 + 48;
            constexpr int oy = 50 + 14;
            sprites[prev].x = ox + 50;
            sprites[prev].y = oy + 40 + qMax(0, ord) * 60;
            sprites[prev].tint = kClear;
            sprites[prev].green = false;
            sprites[prev].red = false;
        }
    }
    for (ExSprite &s : sprites) {
        if (s.name2 == QLatin1String("label")) {
            s.tint = kClear;
            s.green = false;
            s.red = false;
        }
    }
    // Перенести выбранную карточку в форму размещения.
    if (slot >= 0) {
        sprites[idx].x = sprites[slot].x;
        sprites[idx].y = sprites[slot].y;
    }
    sprites[idx].tint = kLabelSelect;
    m_selectedLabel = sprites[idx].name;
    emit labelSelectionChanged(m_selectedLabel);
    emit changed();
}

void ManBodyEngine::resetAllSelections() {
    m_doctorFront = m_doctorBack = m_patientFront = m_patientBack = -1;
    m_selectedLabel.clear();
    m_chipFile.clear();
    m_graded = false;
    m_lastCorrect = false;
    for (Part &p : m_front) {
        clearFlags(p);
    }
    for (Part &p : m_back) {
        clearFlags(p);
    }
    applyAll();
    for (ExSprite &s : sprites) {
        if (s.name2 == QLatin1String("label")) {
            s.tint = kClear;
            s.green = false;
            s.red = false;
        }
    }
    emit doctorSelectionChanged();
    emit labelSelectionChanged(QString());
    emit changed();
}

void ManBodyEngine::submit() {
    if (block || mode == QLatin1String("3")) {
        return;
    }
    const bool sub = mode == QLatin1String("sub");
    auto clearSelectionBlues = [this]() {
        for (Part &p : m_front) {
            p.bD = p.bP = false;
        }
        for (Part &p : m_back) {
            p.bD = p.bP = false;
        }
    };
    if (!sub) {
        if (m_patientFront < 0 && m_patientBack < 0) {
            return;
        }
        if (m_doctorFront < 0 && m_doctorBack < 0) {
            return;
        }
        clearSelectionBlues();
        if (m_doctorFront >= 0) {
            const bool ok = m_patientFront == m_doctorFront;
            if (ok) {
                showColor(true, m_doctorFront, QLatin1Char('g'));
            } else {
                showColor(true, m_doctorFront, QLatin1Char('b'));
                if (m_patientFront >= 0) {
                    showColor(true, m_patientFront, QLatin1Char('r'));
                }
                if (m_patientBack >= 0) {
                    showColor(false, m_patientBack, QLatin1Char('r'));
                }
            }
            applyAll();
            m_graded = true;
            m_lastCorrect = ok;
            finishDone(!ok);
            return;
        }
        const bool ok = m_patientBack == m_doctorBack;
        if (ok) {
            showColor(false, m_doctorBack, QLatin1Char('g'));
        } else {
            showColor(false, m_doctorBack, QLatin1Char('b'));
            if (m_patientBack >= 0) {
                showColor(false, m_patientBack, QLatin1Char('r'));
            }
            if (m_patientFront >= 0) {
                showColor(true, m_patientFront, QLatin1Char('r'));
            }
        }
        applyAll();
        m_graded = true;
        m_lastCorrect = ok;
        finishDone(!ok);
        return;
    }

    if (m_selectedLabel.isEmpty() || (m_doctorFront < 0 && m_doctorBack < 0)) {
        return;
    }
    const bool front = m_doctorFront >= 0;
    const int part = front ? m_doctorFront : m_doctorBack;
    const QString expect = partName2(front, part);
    // name2 с обложки — «Голова»/«голова»; карточки — «голова» (без регистра).
    const bool ok = m_selectedLabel.compare(expect, Qt::CaseInsensitive) == 0;
    clearSelectionBlues();
    if (ok) {
        paintLabel(m_selectedLabel, kClear, true, false);
        showColor(front, part, QLatin1Char('g'));
        m_chipFile = ExAssets::exnPath(QStringLiteral("5.1.4/") +
                                        (front ? QStringLiteral("greencolor/") : QString()) + m_selectedLabel +
                                        QStringLiteral(".png"));
    } else {
        paintLabel(m_selectedLabel, kClear, false, true);
        paintLabel(expect, kLabelBlue, false, false);
        showColor(front, part, QLatin1Char('b'));
        // Подсветить часть тела, соответствующую неверной карточке пациента.
        for (bool side : {true, false}) {
            const QVector<Part> &lst = side ? m_front : m_back;
            for (int i = 0; i < lst.size(); ++i) {
                if (lst[i].name2.compare(m_selectedLabel, Qt::CaseInsensitive) == 0) {
                    showColor(side, i, QLatin1Char('r'));
                }
            }
        }
        m_chipFile = ExAssets::exnPath(QStringLiteral("5.1.4/redcolor/") + m_selectedLabel + QStringLiteral(".png"));
    }
    applyAll();
    m_graded = true;
    m_lastCorrect = ok;
    finishDone(!ok);
}

void ManBodyEngine::markManual(bool wrong) {
    if (mode != QLatin1String("3")) {
        return;
    }
    if (wrong) {
        ExAssets::playIncorrect();
    } else {
        ExAssets::playCorrect();
    }
    emit resultRecorded(wrong);
}

QString ManBodyEngine::coverTaskImage() const {
    if (m_doctorFront >= 0) {
        return ExAssets::exnPath(QStringLiteral("5.1.4/") + partName2(true, m_doctorFront) + QStringLiteral(".png"));
    }
    if (m_doctorBack >= 0) {
        return ExAssets::exnPath(QStringLiteral("5.1.4/") + partName2(false, m_doctorBack) + QStringLiteral(".png"));
    }
    if (mode == QLatin1String("sub")) {
        return ExAssets::exnPath(QStringLiteral("5.1.4/show.png"));
    }
    return ExAssets::exnPath(QStringLiteral("5.1.4/sub.png"));
}

QString ManBodyEngine::coverChipImage() const {
    if (m_graded && !m_chipFile.isEmpty()) {
        return m_chipFile;
    }
    if (!m_selectedLabel.isEmpty()) {
        return ExAssets::exnPath(QStringLiteral("5.1.4/") + m_selectedLabel + QStringLiteral(".png"));
    }
    return ExAssets::exnPath(QStringLiteral("5.1.4/chp.png"));
}

void ManBodyEngine::patientPress(const QPoint &) {}

void ManBodyEngine::patientRelease(const QPoint &p) {
    if (block) {
        return;
    }
    const int done = indexByName(QStringLiteral("done"));
    // hit() требует visible; для «Готово» достаточно bbox (кнопка всегда в сцене в режимах 1–2).
    if (done >= 0 && sprites[done].visible && hitBox(sprites[done], p)) {
        submit();
        return;
    }
    if (mode == QLatin1String("sub")) {
        const int lab = labelAt(p);
        if (lab >= 0) {
            chooseLabel(sprites[lab].name);
        }
        return;
    }
    if (mode == QLatin1String("3")) {
        return;
    }
    bool front = true;
    const int part = partAt(p, &front);
    if (part >= 0) {
        patientChoose(front, part);
    }
}

void ManBodyEngine::doctorClick(const QPoint &p) {
    if (block) {
        return;
    }
    // В «Подписать» врач может кликнуть подпись на зеркале.
    if (mode == QLatin1String("sub")) {
        const int lab = labelAt(p);
        if (lab >= 0) {
            chooseLabel(sprites[lab].name);
            return;
        }
    }
    bool front = true;
    const int part = partAt(p, &front);
    if (part >= 0) {
        doctorChoose(front, part);
    }
}
