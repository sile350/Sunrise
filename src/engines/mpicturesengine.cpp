#include "mpicturesengine.h"

#include <QFileInfo>
#include <QRandomGenerator>
#include <QTimer>

#include <algorithm>

namespace {

QVector<int> shuffled(QVector<int> v) {
    for (int i = v.size() - 1; i > 0; --i) {
        const int j = int(QRandomGenerator::global()->bounded(i + 1));
        std::swap(v[i], v[j]);
    }
    return v;
}

const char *kDoneName = "done";

// mpictures.pictureBox1.Location = (48, 14) внутри UserControl, который uprSpace
// ставит в Left/Top (см. uprSpace.initEx). Координаты спрайтов в оригинале — внутри pictureBox.
constexpr int kBoxX = 48;
constexpr int kBoxY = 14;

// Пересчёт координат докторской панели в координаты пациента: Convert.ToInt16(v) * 100 / scale.
inline int pc(int v, int scale = 70) { return v * 100 / scale; }

} // namespace

int MPicturesEngine::storyCount(const QString &folder) {
    int n = 0;
    while (QFileInfo::exists(ExAssets::exnPath(folder + QStringLiteral("/") + QString::number(n + 1) + QStringLiteral(".png")))) {
        ++n;
    }
    return n;
}

void MPicturesEngine::setHintVisible(bool on) {
    m_hintVisible = on;
    const int i = indexByName(QStringLiteral("rcolors"));
    if (i >= 0) {
        sprites[i].visible = on;
        emit changed();
    }
}

void MPicturesEngine::resetDoneState() {
    block = false;
    m_moving = -1;
    m_dragging = false;
}

void MPicturesEngine::rebuild(bool newPicks) {
    clear();
    resetDoneState();
    m_assign.clear();
    m_home.clear();
    m_ans112.clear();
    m_seriesDone = false;
    m_hint.clear();
    background = QColor(249, 249, 249);
    if (newPicks) {
        dSay.clear();
    }
    if (runtime == QLatin1String("1.1.2")) {
        if (newPicks) {
            m_pics.clear();
        }
        build112();
    } else if (runtime == QLatin1String("1.1.4")) {
        build114();
    } else if (runtime == QLatin1String("1.2.1")) {
        build121();
    } else if (runtime == QLatin1String("1.2.3")) {
        build123();
    } else if (runtime == QLatin1String("2.2.1")) {
        build221();
    }
    m_assign = QVector<int>(sprites.size(), -1);
    emit changed();
}

// ---------------- 1.1.2 / 1.1.3 ----------------
//
// Раскладка пациента = координаты докторской панели (mpictures d1.1.2) * 100 / 70,
// плюс смещение pictureBox (48, 14). Картинки 350x210, подписи (fN.png) 350x40.

void MPicturesEngine::build112() {
    const bool show = p1 != QLatin1String("2");
    const int level = qBound(1, p2.toInt(), 3);
    const int n = level + 1;
    const QString dir = folder + QLatin1Char('/');
    // doctorInit: rnd.Next(1, 30) без повторов -> номера только 1..29
    if (m_pics.size() != n) {
        const int total = qMin(29, qMax(n, storyCount(folder)));
        QVector<int> all;
        for (int i = 1; i <= total; ++i) {
            all.append(i);
        }
        all = shuffled(all);
        m_pics = all.mid(0, n);
    }

    // doctor x, y сюжетов и y подписей (уровни 1..3), режим 1 («показать»)
    static const int m1x[3] = {227, 127, 227};
    static const int m1y[3][4] = {{81, 381, 0, 0}, {10, 215, 420, 0}, {0, 180, 360, 540}};
    static const int m1c[3][4] = {{231, 531, 0, 0}, {157, 362, 567, 0}, {150, 330, 510, 690}};
    // режим 2 («подписать»): картинки слева, подписи-кандидаты справа (x = 490)
    static const int m2y[3][4] = {{81, 281, 0, 0}, {10, 215, 420, 0}, {10, 190, 370, 550}};
    static const int m2cy[3][4] = {{181, 381, 0, 0}, {100, 291, 491, 0}, {100, 261, 431, 641}};

    // подпись над экраном: label2 в uprSpace (750,10), Arial 20 Italic.
    // Спрайт есть всегда, чтобы индексы картинок (dSay) не менялись при переключении режима.
    {
        QString label;
        if (show) {
            label = QStringLiteral("Покажите сюжетные картинки");
        }
        if (folder == QLatin1String("1.1.3")) {
            label = QStringLiteral("Подпишите предметные картинки");
        }
        const int li = addSprite();
        sprites[li].setText(label.isEmpty() ? QStringLiteral(" ") : label, 20, Qt::black, 100, true);
        sprites[li].x = 750;
        sprites[li].y = 10;
        sprites[li].name = QStringLiteral("label");
        sprites[li].visible = !label.isEmpty();
    }

    for (int k = 0; k < n; ++k) {
        const int num = m_pics[k];
        const int docX = show ? m1x[level - 1] : 127;
        const int docY = show ? m1y[level - 1][k] : m2y[level - 1][k];
        const int capDocY = show ? m1c[level - 1][k] : docY + 147;
        int i = addSprite();
        sprites[i].setImage(ExAssets::exnPath(dir + QString::number(num) + QStringLiteral(".png")));
        sprites[i].x = kBoxX + pc(docX);
        sprites[i].y = kBoxY + pc(docY);
        sprites[i].name = QStringLiteral("pic");
        sprites[i].word = QString::number(num);
        i = addSprite();
        if (show) {
            sprites[i].setImage(ExAssets::exnPath(dir + QStringLiteral("f") + QString::number(num) + QStringLiteral(".png")));
            sprites[i].name = QStringLiteral("capshow");
        } else {
            sprites[i].setImage(ExAssets::exnPath(QStringLiteral("1.1.2/void.png")));
            sprites[i].name = QStringLiteral("slot");
            sprites[i].patientOnly = true;
        }
        sprites[i].x = kBoxX + pc(docX);
        sprites[i].y = kBoxY + pc(capDocY);
    }
    if (!show) {
        // У врача под картинками остаются правильные подписи (d1.1.2: fN.png, name "no"), у пациента — void.png.
        for (int k = 0; k < n; ++k) {
            const int i = addSprite();
            sprites[i].setImage(ExAssets::exnPath(dir + QStringLiteral("f") + QString::number(m_pics[k]) + QStringLiteral(".png")));
            sprites[i].x = kBoxX + pc(127);
            sprites[i].y = kBoxY + pc(m2y[level - 1][k] + 147);
            sprites[i].name = QStringLiteral("no");
            sprites[i].doctorOnly = true;
        }
    }
    if (!show) {
        const QVector<int> order = shuffled(m_pics);
        for (int k = 0; k < order.size(); ++k) {
            const int i = addSprite();
            sprites[i].setImage(ExAssets::exnPath(dir + QStringLiteral("f") + QString::number(order[k]) + QStringLiteral(".png")));
            sprites[i].x = kBoxX + pc(490);
            sprites[i].y = kBoxY + pc(m2cy[level - 1][k]);
            sprites[i].name = QStringLiteral("cap");
            sprites[i].word = QString::number(order[k]);
            sprites[i].movable = true;
        }
    }
    // кнопка «готово» (sysImages\done.png) в оригинале в (1500, 600)
    addDoneButton(kBoxX + 1500, kBoxY + 600);
    // восстановить подсветку выбора врача
    for (const QString &s : dSay) {
        const int idx = s.toInt();
        if (idx >= 0 && idx < sprites.size()) {
            sprites[idx].dborder = true;
        }
    }
    m_home.resize(sprites.size());
    for (int i = 0; i < sprites.size(); ++i) {
        m_home[i] = QPoint(sprites[i].x, sprites[i].y);
    }
}

void MPicturesEngine::pressDone112() {
    const bool show = p1 != QLatin1String("2");
    const int n = m_pics.size();
    // sprites[i].border == true для картинок или есть ответы в main.ans; иначе done игнорируется
    if (show) {
        int selected = -1;
        for (int k = 0; k < n; ++k) {
            if (sprites[2 * k + (label112Offset())].border) {
                selected = 2 * k + label112Offset();
                break; // оригинал проверяет sprites[0], затем [2], [4], [6] и берёт первый
            }
        }
        if (selected < 0) {
            return;
        }
        const bool wrong = !dSay.contains(QString::number(selected));
        (wrong ? sprites[selected].red : sprites[selected].green) = true;
        finishDone(wrong);
        return;
    }
    // режим 2: main.ans — записи "индекс картинки;1/0" по каждому броску подписи на картинку
    if (m_ans112.isEmpty()) {
        return;
    }
    bool wrong = false;
    for (const QPair<int, bool> &a : m_ans112) {
        if (a.first < 0 || a.first >= sprites.size()) {
            continue;
        }
        (a.second ? sprites[a.first].green : sprites[a.first].red) = true;
        if (!a.second) {
            wrong = true;
        }
    }
    finishDone(wrong);
}

int MPicturesEngine::label112Offset() const {
    // подпись «label» занимает первый спрайт (если есть)
    return (!sprites.isEmpty() && sprites[0].name == QLatin1String("label")) ? 1 : 0;
}

// ---------------- 1.1.4 ----------------

void MPicturesEngine::build114() {
    const QString dir = QStringLiteral("1.1.4/");
    const int q = qBound(1, currentEx, 6);
    static const char *hints[] = {
        "Спать хотите? (да? (киваем); нет? (качаем головой); просим выбрать картинку с кроватью)",
        "Пить хотите? (да? (киваем); нет? (качаем головой); даем ему стакан с водой и/или просим показать стакан с водой на картинке)",
        "Есть хотите? (да? (киваем); нет? (качаем головой); предлагаем ему что-нибудь съедобное и/или просим выбрать картинку с едой).",
        "За окном день или ночь? (просим выбрать нужную картинку)",
        "За окном дождь, снег или солнце? (просим выбрать нужную картинку)",
        "Сейчас зима, весна, лето или осень? (просим выбрать нужную картинку)"};
    m_hint = QString::fromUtf8(hints[q - 1]);

    if (p1 == QLatin1String("2")) {
        // uprSpace.showQuestion: Arial 28, Top = Height / 2
        if (!questionText.trimmed().isEmpty()) {
            const int i = addSprite();
            sprites[i].setText(questionText.trimmed(), 28);
            sprites[i].setCenterX(1920);
            sprites[i].y = 540;
            sprites[i].name = QStringLiteral("question");
        }
        return;
    }
    // uprSpace.ShowText114: Arial 24, Top = 11, по центру
    {
        const QStringList texts = {QStringLiteral("Спать хотите? "), QStringLiteral("Пить хотите? "),
                                   QStringLiteral(" Есть хотите? "), QStringLiteral("  За окном день или ночь?  "),
                                   QStringLiteral("  За окном дождь, снег или солнце? "),
                                   QStringLiteral("  Сейчас зима, весна, лето или осень?  ")};
        const int i = addSprite();
        sprites[i].setText(texts[q - 1].trimmed(), 24);
        sprites[i].setCenterX(1920);
        sprites[i].y = 11;
        sprites[i].name = QStringLiteral("question");
    }
    // Картинки: doctor (x,y) * 100 / 70, y - 50; uprSpace: Left = 400, Top = 50 (+ pictureBox 48,14).
    const int ox = 400 + kBoxX;
    const int oy = 50 + kBoxY;
    struct Item {
        const char *file;
        int dx;
        int dy;
    };
    QVector<Item> items;
    switch (q) {
    case 1: items = {{"11.png", 227, 81}, {"12.png", 227, 320 + 81}}; break;
    case 2:
    case 3: items = {{"21.png", 227, 81}, {"22.png", 227, 320 + 81}}; break;
    case 4: items = {{"41.png", 227, 81}, {"42.png", 227, 320 + 81}}; break;
    case 5: items = {{"51.png", 5, 81}, {"52.png", 5 + 480, 81}, {"53.png", 5, 320 + 81}}; break;
    default: items = {{"61.png", 5, 81}, {"62.png", 5 + 480, 81}, {"63.png", 5, 320 + 81}, {"64.png", 5 + 480, 320 + 81}}; break;
    }
    for (const Item &it : items) {
        const int i = addSprite();
        sprites[i].setImage(ExAssets::exnPath(dir + QString::fromLatin1(it.file)));
        sprites[i].x = ox + pc(it.dx);
        sprites[i].y = oy + pc(it.dy) - 50;
        sprites[i].name = QStringLiteral("pic");
        sprites[i].word = QString::fromLatin1(it.file);
    }
}

// ---------------- 1.2.1 ----------------
//
// SeriesInfo: docX0/docStep — координаты докторской панели, scale — 60 (месяцы 50),
// cellDocY / poolDocY — y ячеек и y пула; left — Left uprSpace.movingPictures.

MPicturesEngine::SeriesInfo MPicturesEngine::seriesInfo(const QString &material) const {
    SeriesInfo s;
    if (material == QStringLiteral("Дни недели")) {
        s = {QStringLiteral("days"), 7, QStringLiteral("void2.png"), QStringLiteral("bvoid2.png"), 133, 50, 140, 200, {6, 5, 2}};
        s.docX0 = 10;
        s.scale = 60;
    } else if (material == QStringLiteral("Месяцы")) {
        s = {QStringLiteral("month"), 12, QStringLiteral("void1.png"), QStringLiteral("bvoid1.png"), 77, 50, 140, 0, {10, 7, 2}};
        s.docX0 = 10;
        s.scale = 50;
    } else if (material == QStringLiteral("Цвета радуги")) {
        s = {QStringLiteral("colors"), 7, QStringLiteral("void2.png"), QStringLiteral("bvoid2.png"), 130, 130, 200, 250, {6, 5, 2}};
        s.docX0 = 10;
        s.scale = 60;
    } else {
        s = {QStringLiteral("numbers"), 10, QStringLiteral("void1.png"), QStringLiteral("bvoid1.png"), 93, 50, 140, 200, {9, 6, 2}};
        s.docX0 = 0;
        s.scale = 60;
    }
    return s;
}

void MPicturesEngine::build121() {
    // p1: функция, p2: "направленность;материал;сложность"
    const QStringList pr = p2.split(QLatin1Char(';'));
    const QString dirText = pr.value(0);
    const QString material = pr.value(1);
    const int level = qBound(1, pr.value(2).toInt(), 3);
    m_series = seriesInfo(material);
    const SeriesInfo &s = m_series;
    // uprSpace: Top = 100 (+14 pictureBox); Left по материалу (pictureBox1.Left = 0)
    const int top = 100 + kBoxY;
    const bool reproduce = p1 == QStringLiteral("Воспроизведение ряда");
    const bool backward = dirText == QStringLiteral("Обратная");
    const QString base = QStringLiteral("1.2.1/");
    m_seriesCount = s.count;

    auto cellX = [&](int n) { return s.left + pc(s.docX0 + (n - 1) * s.docStep, s.scale); };
    auto itemFile = [&](int n) { return ExAssets::exnPath(base + s.dir + QLatin1Char('/') + QString::number(n) + QStringLiteral(".png")); };
    const int cellY = top + pc(s.cellDocY, s.scale);
    const int poolY = top + pc(s.poolDocY, s.scale);

    if (reproduce) {
        // Воспроизведение: ряд показывается в одной строке (doctor y = 140), для «Обратная» — 10..1
        for (int k = 0; k < s.count; ++k) {
            const int n = backward ? s.count - k : k + 1;
            const int i = addSprite();
            sprites[i].setImage(itemFile(n));
            sprites[i].x = cellX(k + 1);
            sprites[i].y = top + pc(140, s.scale);
            sprites[i].name = QStringLiteral("show") + QString::number(n);
        }
        block = true;
        return;
    }
    m_dir = backward ? -1 : 1;
    const int from = s.levels[level - 1];
    const int prefilled = from - 1;
    // ячейки
    for (int n = 1; n <= s.count; ++n) {
        const int i = addSprite();
        sprites[i].setImage(ExAssets::exnPath(base + s.voidImg));
        sprites[i].x = cellX(n);
        sprites[i].y = cellY;
        sprites[i].name = QStringLiteral("cell") + QString::number(n);
    }
    // предзаполненные элементы
    QVector<int> rest;
    if (!backward) {
        for (int n = 1; n <= prefilled; ++n) {
            const int i = addSprite();
            sprites[i].setImage(itemFile(n));
            sprites[i].x = cellX(n);
            sprites[i].y = cellY;
            sprites[i].name = QStringLiteral("fix") + QString::number(n);
        }
        for (int n = from; n <= s.count; ++n) {
            rest.append(n);
        }
        m_pointLine = from;
    } else {
        for (int n = s.count; n > s.count - prefilled; --n) {
            const int i = addSprite();
            sprites[i].setImage(itemFile(n));
            sprites[i].x = cellX(n);
            sprites[i].y = cellY;
            sprites[i].name = QStringLiteral("fix") + QString::number(n);
        }
        for (int n = 1; n <= s.count - prefilled; ++n) {
            rest.append(n);
        }
        m_pointLine = s.count - prefilled;
    }
    rest = shuffled(rest);
    m_home.clear();
    for (int k = 0; k < rest.size(); ++k) {
        const int i = addSprite();
        sprites[i].setImage(itemFile(rest[k]));
        sprites[i].x = cellX(k + 1);
        sprites[i].y = poolY;
        // pclassSel.movable = false: элемент выбирается щелчком, а не перетаскиванием
        sprites[i].name = QStringLiteral("row") + QString::number(rest[k]);
    }
    {
        const int i = addSprite();
        if (material == QStringLiteral("Цвета радуги")) {
            // doctor (10, 50) -> patient scale 100/60
            sprites[i].setImage(ExAssets::exnPath(base + QStringLiteral("rcolors.png")));
            sprites[i].x = cellX(1);
            sprites[i].y = top + pc(50, s.scale);
            sprites[i].name = QStringLiteral("rcolors");
            sprites[i].visible = m_hintVisible;
        } else {
            sprites[i].visible = false;
            sprites[i].name = QStringLiteral("rcolors");
        }
    }
    // подсветка текущей ячейки: pclassSel.border = синяя рамка 8px (bvoid в оригинале закомментирован)
    const int ci = indexByName(QStringLiteral("cell") + QString::number(m_pointLine));
    if (ci >= 0) {
        sprites[ci].border = true;
    }
    m_home.resize(sprites.size());
    for (int i = 0; i < sprites.size(); ++i) {
        m_home[i] = QPoint(sprites[i].x, sprites[i].y);
    }
}

void MPicturesEngine::releaseSeries() {
    // Оригинал: проверяется только имя спрайта (положение отпускания не важно);
    // щелчок по любому другому спрайту (ячейка, уже стоящий элемент) — ошибка.
    const int mv = m_moving;
    if (mv < 0 || mv >= sprites.size()) {
        return;
    }
    if (sprites[mv].name == QStringLiteral("row") + QString::number(m_pointLine)) {
        const int n = m_pointLine;
        const int ci = indexByName(QStringLiteral("cell") + QString::number(n));
        if (ci >= 0) {
            sprites[mv].x = sprites[ci].x;
            sprites[mv].y = sprites[ci].y;
            sprites[ci].border = false;
        }
        ExAssets::playCorrect();
        m_pointLine += m_dir;
        if (m_pointLine < 1 || m_pointLine > m_series.count) {
            m_seriesDone = true;
            block = true;
            emit resultRecorded(false);
        } else {
            const int ni = indexByName(QStringLiteral("cell") + QString::number(m_pointLine));
            if (ni >= 0) {
                sprites[ni].border = true;
            }
        }
    } else {
        // redSelect + minuteTimer(1 c): красная подсветка снимается, спрайт остаётся на месте
        sprites[mv].red = true;
        ExAssets::playIncorrect();
        QTimer::singleShot(1000, this, [this]() {
            for (ExSprite &sp : sprites) {
                sp.red = false;
            }
            emit changed();
        });
    }
    emit changed();
}

// ---------------- 1.2.3 ----------------

void MPicturesEngine::build123() {
    // p1: Запоминание | Устно | С перемещением ; p2: "1" данные, "2" карточки
    if (p2 == QLatin1String("1")) {
        // Doctor: y = 450 / 510 / 570, текст Arial 40, по центру (setCenter(1920)) внутри pictureBox
        const QStringList lines = {fio, address, phone};
        static const int ys[3] = {450, 510, 570};
        for (int k = 0; k < 3; ++k) {
            const QString line = lines[k].trimmed();
            if (line.isEmpty()) {
                continue;
            }
            const int i = addSprite();
            sprites[i].setText(line, 40);
            sprites[i].setCenterX(1920);
            sprites[i].x += kBoxX;
            sprites[i].y = kBoxY + ys[k];
            sprites[i].name = QStringLiteral("info");
        }
        return;
    }
    if (p2 != QLatin1String("2")) {
        return;
    }
    const QString base = QStringLiteral("1.2.3/");
    const int ex1 = ((currentEx - 1) % 10) + 1;
    const int ex2 = (ex1 % 10) + 1;
    const int exs[2] = {ex1, ex2};
    const bool moving = p1 == QStringLiteral("С перемещением");
    const bool full = p1 == QStringLiteral("Запоминание");
    // dFx = 150, dFy = 5, dAx = 5, dAy = 40, dTx = 1, dTy = 70
    const int cardX = moving ? 50 : 200;
    const int cardY[2] = {240, 440};
    for (int c = 0; c < 2; ++c) {
        int i = addSprite();
        sprites[i].setImage(ExAssets::exnPath(base + QStringLiteral("tm") + QString::number(c + 1) + QStringLiteral(".png")));
        sprites[i].x = kBoxX + cardX;
        sprites[i].y = kBoxY + cardY[c];
        sprites[i].name = QStringLiteral("card") + QString::number(c + 1);
        i = addSprite();
        sprites[i].setImage(ExAssets::exnPath(base + QStringLiteral("f") + QString::number(exs[c]) + QStringLiteral(".png")), 85);
        sprites[i].x = kBoxX + cardX + 150;
        sprites[i].y = kBoxY + cardY[c] + 5;
        sprites[i].name = QStringLiteral("fio") + QString::number(c + 1);
        if (full) {
            // a: (f.x + 5, f.y + 40); t: (a.x + 1, a.y + 70). В оригинале порядок спрайтов: tm1,f1,a1,t1,tm2,f2,a2,t2
            int j = addSprite();
            sprites[j].setImage(ExAssets::exnPath(base + QStringLiteral("a") + QString::number(exs[c]) + QStringLiteral(".png")), 85);
            sprites[j].x = sprites[i].x + 5;
            sprites[j].y = sprites[i].y + 40;
            sprites[j].name = QStringLiteral("adress") + QString::number(c + 1);
            const int t = addSprite();
            sprites[t].setImage(ExAssets::exnPath(base + QStringLiteral("t") + QString::number(exs[c]) + QStringLiteral(".png")), 85);
            sprites[t].x = sprites[j].x + 1;
            sprites[t].y = sprites[j].y + 70;
            sprites[t].name = QStringLiteral("tel") + QString::number(c + 1);
        }
    }
    if (moving) {
        // Подписи справа в фиксированных позициях x = 530: adress1, adress2, tel1, tel2 (шаг 90)
        static const char *kinds[4] = {"a", "a", "t", "t"};
        static const char *names[4] = {"adress", "adress", "tel", "tel"};
        static const int which[4] = {0, 1, 0, 1};
        for (int k = 0; k < 4; ++k) {
            const int c = which[k];
            const int i = addSprite();
            sprites[i].setImage(ExAssets::exnPath(base + QString::fromLatin1(kinds[k]) + QString::number(exs[c]) + QStringLiteral(".png")), 85);
            sprites[i].x = kBoxX + 530;
            sprites[i].y = kBoxY + 240 + k * 90;
            sprites[i].name = QString::fromLatin1(names[k]) + QString::number(c + 1);
            sprites[i].movable = true;
        }
        // «hidden»: правильные адрес и телефон внутри карточек — ключ для врача, у пациента скрыты
        for (int c = 0; c < 2; ++c) {
            const int a = addSprite();
            sprites[a].setImage(ExAssets::exnPath(base + QStringLiteral("a") + QString::number(exs[c]) + QStringLiteral(".png")), 85);
            sprites[a].x = kBoxX + 50 + 150;
            sprites[a].y = kBoxY + 240 + 40 + c * 200;
            sprites[a].name = QStringLiteral("hidden");
            sprites[a].doctorOnly = true;
            const int t = addSprite();
            sprites[t].setImage(ExAssets::exnPath(base + QStringLiteral("t") + QString::number(exs[c]) + QStringLiteral(".png")), 85);
            sprites[t].x = kBoxX + 50 + 150;
            sprites[t].y = kBoxY + 240 + 115 + c * 200;
            sprites[t].name = QStringLiteral("hidden");
            sprites[t].doctorOnly = true;
        }
    }
}

// ---------------- 2.2.1 ----------------

void MPicturesEngine::build221() {
    // doctor: x = y = 0 -> patient 0 * 100 / 70, смещение pictureBox (48, 14)
    const int n = qBound(1, currentEx, 20);
    const int i = addSprite();
    sprites[i].setImage(ExAssets::exnPath(QStringLiteral("2.2.1/") + QString::number(n) + QStringLiteral(".png")));
    sprites[i].x = kBoxX;
    sprites[i].y = kBoxY;
    sprites[i].name = QStringLiteral("story");
}

// ---------------- ввод ----------------

void MPicturesEngine::patientPress(const QPoint &p) {
    ExEngine::patientPress(p);
    if (m_moving < 0 || m_moving >= sprites.size()) {
        return;
    }
    ExSprite &s = sprites[m_moving];
    if (s.name == QLatin1String(kDoneName) || selectableOnClick(s)) {
        m_dragging = false;
    }
}

bool MPicturesEngine::selectableOnClick(const ExSprite &s) const {
    return s.name == QLatin1String("pic") &&
           (runtime == QLatin1String("1.1.4") || (runtime == QLatin1String("1.1.2") && p1 != QLatin1String("2")));
}

void MPicturesEngine::patientRelease(const QPoint &p) {
    const int mv = m_moving;
    const bool wasDragging = m_dragging;
    ExEngine::patientRelease(p);
    if (mv < 0 || mv >= sprites.size() || block) {
        return;
    }
    // MouseUp → pressDone: по спрайту, на котором нажали
    if (sprites[mv].name == QLatin1String(kDoneName)) {
        if (runtime == QLatin1String("1.1.2")) {
            pressDone112();
        }
        emit changed();
        return;
    }
    // MouseUp: border переключается, если кнопку отпустили над той же картинкой (выделенных может быть несколько)
    if (selectableOnClick(sprites[mv])) {
        if (sprites[mv].hit(p.x(), p.y())) {
            sprites[mv].border = !sprites[mv].border;
            emit changed();
        }
        return;
    }
    if (!wasDragging) {
        return;
    }
    if (runtime == QLatin1String("1.2.1")) {
        releaseSeries();
        return;
    }
    if (runtime == QLatin1String("1.1.2") && p1 == QLatin1String("2") && sprites[mv].name == QLatin1String("cap")) {
        // подпись бросили на картинку: проверяются только прямоугольники картинок (индексы 0,2,4,6)
        const int off = label112Offset();
        const int n = m_pics.size();
        for (int k = 0; k < n; ++k) {
            const int pi = 2 * k + off;
            if (!sprites[pi].hit(p.x(), p.y())) {
                continue;
            }
            sprites[mv].x = sprites[pi].x;
            sprites[mv].y = sprites[pi].y + sprites[pi].h;
            const bool ok = sprites[pi].word == sprites[mv].word && dSay.contains(QString::number(pi));
            m_ans112.append(qMakePair(pi, ok));
            break;
        }
        emit changed();
        return;
    }
    if (runtime == QLatin1String("1.2.3") && sprites[mv].movable) {
        for (int c = 1; c <= 2; ++c) {
            const int ci = indexByName(QStringLiteral("card") + QString::number(c));
            if (ci < 0 || !sprites[ci].hit(p.x(), p.y())) {
                continue;
            }
            if (sprites[mv].name == QStringLiteral("tel") + QString::number(c)) {
                sprites[mv].x = sprites[ci].x + 150;
                sprites[mv].y = sprites[ci].y + 114;
            }
            if (sprites[mv].name == QStringLiteral("adress") + QString::number(c)) {
                sprites[mv].x = sprites[ci].x + 150;
                sprites[mv].y = sprites[ci].y + 45;
            }
        }
        emit changed();
    }
}

void MPicturesEngine::doctorClick(const QPoint &p) {
    const int i = topAt(p, false);
    if (i < 0) {
        return;
    }
    if (runtime == QLatin1String("1.2.1")) {
        // doctorMouseUp d1.2.1: щелчок по элементу ряда (row*, rcolors; у нас предзаполненные — fix*) переключает
        // синюю рамку, её видит и пациент
        const QString &n = sprites[i].name;
        if (n.startsWith(QLatin1String("row")) || n.startsWith(QLatin1String("fix")) || n == QLatin1String("rcolors")) {
            sprites[i].border = !sprites[i].border;
            emit changed();
        }
        return;
    }
    // В doctorMouseUp выбор картинок есть только у d1.1.2 (1.1.4 щелчок врача не обрабатывает)
    if (runtime != QLatin1String("1.1.2") || sprites[i].name != QLatin1String("pic")) {
        return;
    }
    // addSayIfEmpty / removeSayIfYes: выбор переключается по каждой картинке (допускается несколько)
    const QString key = QString::number(i);
    if (dSay.contains(key)) {
        dSay.removeAll(key);
        sprites[i].dborder = false;
    } else {
        dSay.append(key);
        sprites[i].dborder = true;
    }
    emit changed();
}
