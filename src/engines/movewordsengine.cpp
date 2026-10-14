#include "movewordsengine.h"

#include <QFont>
#include <QFontMetrics>
#include <QRandomGenerator>
#include <algorithm>

namespace {

const char *k211Sentences[] = {
    "птицы петь после переставали полуночи",
    "сосед помочь припарковать меня машину попросил",
    "на обещали завтра погоду хорошую",
    "на обещали завтра погоду хорошую",
    "птицы осень на юг каждую улетают",
    "изменилась нового директора в лучшую сторону обстановка с приходом",
    "деревьях на листочки весной каждой распускаются",
    "квакают в ночью лягушки пруду громко",
    "маленького мне папа щенка пушистого купил",
    "каждую в друзья играют пятницу футбол",
    "центре большом с гулял в друзьями я",
    "палатку и пришли лес развели туристы разбили в костер",
    "платформе гудок подъезжая длинный к дал поезд",
    "всех к приготовила столу и обед позвала мама",
    "в укрыла зима и снежным пришла одеялом городе улицы",
    "открыл с работы и двери пришёл своим папа ключом"};

// По одной строке из каждого списка по кругу: случайной (2.1.2 «Смешанные») или по порядку (3.2.3 «Смешанные»).
QStringList interleave(const QList<QStringList> &lists, bool randomPick) {
    QList<QStringList> src = lists;
    QStringList out;
    bool any = true;
    while (any) {
        any = false;
        for (QStringList &l : src) {
            if (!l.isEmpty()) {
                const int k = randomPick ? int(QRandomGenerator::global()->bounded(l.size())) : 0;
                out.append(l.takeAt(k));
                any = true;
            }
        }
    }
    return out;
}

int leadingNumber(const QString &s, int *digits) {
    int n = 0;
    int d = 0;
    while (d < s.size() && s[d].isDigit()) {
        n = n * 10 + s[d].digitValue();
        ++d;
    }
    *digits = d;
    return n;
}

} // namespace

QString MovewordsEngine::dataPath(const QString &file) const {
    return ExAssets::exnPath(runtime + QLatin1Char('/') + file);
}

int MovewordsEngine::addText(const QString &text, int x, int y, int fontSize, const QColor &c, const QString &name,
                             bool movable, int scale) {
    const int i = addSprite();
    sprites[i].setText(text, fontSize, c, scale);
    sprites[i].x = x;
    sprites[i].y = y;
    sprites[i].name = name;
    sprites[i].movable = movable;
    return i;
}

int MovewordsEngine::addImage(const QString &path, int x, int y, const QString &name, int scale) {
    const int i = addSprite();
    sprites[i].setImage(path, scale);
    sprites[i].x = x;
    sprites[i].y = y;
    sprites[i].name = name;
    return i;
}

void MovewordsEngine::centerDoc(int idx, int scale, int docWidth, int extra) {
    const int docW = sprites[idx].w * scale / 100;
    const int docX = (docWidth - docW) / 2;
    sprites[idx].x = docX * 100 / scale + extra;
}

void MovewordsEngine::snapshotHome() {
    m_home.resize(sprites.size());
    for (int i = 0; i < sprites.size(); ++i) {
        m_home[i] = QPoint(sprites[i].x, sprites[i].y);
    }
}

QStringList MovewordsEngine::wordList() const {
    auto &self = const_cast<MovewordsEngine &>(*this);
    const QString key = runtime + QLatin1Char('|') + p1 + QLatin1Char('|') + p2 + QLatin1Char('|') + QString::number(dataNumber);
    if (runtime == QLatin1String("2.1.2") && p1 == QStringLiteral("Смешанные")) {
        if (m_cacheKey != key) {
            self.m_cache = interleave({ExAssets::readLines(dataPath(QStringLiteral("gs.txt"))),
                                       ExAssets::readLines(dataPath(QStringLiteral("ss.txt")))},
                                      true);
            self.m_cacheKey = key;
        }
        return m_cache;
    }
    if (runtime == QLatin1String("3.2.3") && p2 == QStringLiteral("Смешанные")) {
        if (m_cacheKey != key) {
            self.m_cache = interleave({ExAssets::readLines(dataPath(QStringLiteral("s.txt"))),
                                       ExAssets::readLines(dataPath(QStringLiteral("p.txt"))),
                                       ExAssets::readLines(dataPath(QStringLiteral("n.txt"))),
                                       ExAssets::readLines(dataPath(QStringLiteral("g.txt")))},
                                      false);
            self.m_cacheKey = key;
        }
        return m_cache;
    }
    QString file;
    if (runtime == QLatin1String("1.2.2")) {
        if (p2 == QStringLiteral("Ожидаемые слова")) {
            file = QStringLiteral("os.txt");
        } else if (p2 == QStringLiteral("Устойчивые выражения")) {
            file = QStringLiteral("uv.txt");
        } else {
            file = QStringLiteral("posl.txt");
        }
    } else if (runtime == QLatin1String("2.1.2")) {
        file = p1 == QStringLiteral("Два существительных") ? QStringLiteral("ss.txt") : QStringLiteral("gs.txt");
    } else if (runtime == QLatin1String("2.1.3")) {
        file = QStringLiteral("file.txt");
    } else if (runtime == QLatin1String("3.1.2")) {
        file = QStringLiteral("data") + QString::number(dataNumber) + QStringLiteral(".txt");
    } else if (runtime == QLatin1String("3.2.3")) {
        if (p2 == QStringLiteral("Глаголы")) {
            file = QStringLiteral("g.txt");
        } else if (p2 == QStringLiteral("Прилагательные")) {
            file = QStringLiteral("p.txt");
        } else if (p2 == QStringLiteral("Наречия")) {
            file = QStringLiteral("n.txt");
        } else {
            file = QStringLiteral("s.txt");
        }
    } else {
        file = QStringLiteral("data.txt");
    }
    QStringList lines = ExAssets::readLines(dataPath(file));
    lines.removeAll(QString());
    return lines;
}

int MovewordsEngine::recordCount() const {
    return wordList().size();
}

void MovewordsEngine::rebuild(bool resetState) {
    clear();
    block = false;
    m_mem = -1;
    m_home.clear();
    doctorHint.clear();
    taskText.clear();
    explanationText.clear();
    m_sortAlt = false;
    for (int &v : m_slotLetter) {
        v = -1;
    }
    background = QColor(249, 249, 249);
    m_hostApplied = false;
    m_lineMax = 0;
    m_sortLeft = 0;
    m_sortY = 0;
    m_slotsY.clear();
    if (resetState) {
        explainOn = false;
    }
    // Смещения хоста из uprSpace.initEx (Left/Top) + внутренний pictureBox movewords (3,21),
    // у 2.2.6/3.1.x/3.2.x pictureBox1.Top = 0.
    struct HostOff { const char *rt; int left; int top; int pbTop; };
    static const HostOff offs[] = {
        {"1.2.2", 430, 100, 21}, {"2.1.1", 210, 90, 21}, {"2.1.2", 470, 100, 21}, {"2.1.3", 100, 110, 21},
        {"2.1.4", 300, 500, 21}, {"2.2.3", 215, -30, 21}, {"2.2.4", 250, 0, 21}, {"2.2.6", 0, -70, 0},
        {"3.1.1", 500, 100, 0},  {"3.1.2", 300, 150, 0},  {"3.2.3", 300, 100, 0}, {"3.2.4", 380, 200, 0}};
    m_hx = 3;
    m_hy = 21;
    if (runtime == QLatin1String("2.2.5")) {
        const bool graphic = p1 == QStringLiteral("Графические задания");
        m_hx += 0;
        m_hy = (graphic ? 0 : 200) + 21;
    } else {
        for (const HostOff &o : offs) {
            if (runtime == QLatin1String(o.rt)) {
                m_hx = o.left + 3;
                m_hy = o.top + o.pbTop;
            }
        }
    }
    if (runtime == QLatin1String("1.2.2")) build122();
    else if (runtime == QLatin1String("2.1.1")) build211();
    else if (runtime == QLatin1String("2.1.2")) build212();
    else if (runtime == QLatin1String("2.1.3")) build213();
    else if (runtime == QLatin1String("2.1.4")) build214();
    else if (runtime == QLatin1String("2.2.3")) build223();
    else if (runtime == QLatin1String("2.2.4")) build224();
    else if (runtime == QLatin1String("2.2.5")) build225();
    else if (runtime == QLatin1String("2.2.6")) build226();
    else if (runtime == QLatin1String("3.1.1")) build311();
    else if (runtime == QLatin1String("3.1.2")) build312();
    else if (runtime == QLatin1String("3.2.3")) build323();
    else if (runtime == QLatin1String("3.2.4")) build324();
    applyHost();
    addAbsolute();
    snapshotHome();
    emit changed();
}

void MovewordsEngine::applyHost() {
    for (ExSprite &s : sprites) {
        s.x += m_hx;
        s.y += m_hy;
    }
    m_lineMax += m_hy;
    m_sortLeft += m_hx;
    m_sortY += m_hy;
    for (qreal &y : m_slotsY) {
        y += m_hy;
    }
    m_hostApplied = true;
}

// Элементы, которые в оригинале лежат на форме пациента (uprSpace), а не в pictureBox движка.
void MovewordsEngine::addAbsolute() {
    if (runtime == QLatin1String("2.2.5") && p1 != QStringLiteral("Графические задания")) {
        // ltext: (1920-800)/2, 150, 800x200, Arial 18, FixedSingle
        const int t = addSprite();
        sprites[t].setBox(taskText, 800, 200, 18);
        sprites[t].x = (1920 - 800) / 2;
        sprites[t].y = 150;
        sprites[t].name = QStringLiteral("task");
        sprites[t].patientOnly = true;
        // lob2 «Объяснения:» (Top 750) и lob (Top 800) — по кнопке bob
        QFont f(QStringLiteral("Arial"));
        f.setBold(true);
        f.setPixelSize(24);
        const QString cap = QStringLiteral("Объяснения:");
        const int cw = QFontMetrics(f).horizontalAdvance(cap) + 8;
        const int c = addSprite();
        sprites[c].setBox(cap, cw, 32, 18, false, true);
        sprites[c].x = (1920 - cw) / 2;
        sprites[c].y = 750;
        sprites[c].name = QStringLiteral("lob2");
        sprites[c].patientOnly = true;
        sprites[c].visible = explainOn;
        const int e = addSprite();
        sprites[e].setBox(explanationText, 800, 200, 18);
        sprites[e].x = (1920 - 800) / 2;
        sprites[e].y = 800;
        sprites[e].name = QStringLiteral("lob");
        sprites[e].patientOnly = true;
        sprites[e].visible = explainOn;
    }
    if (runtime == QLatin1String("2.1.4") && showExamples) {
        // Обложки\examples (Left 410, Top 10): метки Arial 24 в (28,77/135/193/251/304), «Примеры» в (290,15)
        static const QStringList labels = {
            QStringLiteral("Оля пошла в кино, потому что хотела посмотреть этот фильм"),
            QStringLiteral("Оля пошла в кино, несмотря на то, что уже смотрела этот фильм"),
            QStringLiteral("Оля пошла в кино, так как хотела посмотреть этот фильм"),
            QStringLiteral("Оля пошла в кино, поэтому не успела сделать уроки"),
            QStringLiteral("Оля пошла в кино,  так что вечер удался")};
        static const QStringList keys = {QStringLiteral("потому что"), QStringLiteral("несмотря"),
                                         QStringLiteral("так как"), QStringLiteral("поэтому"), QStringLiteral("так что")};
        static const int ly[5] = {77, 135, 193, 251, 304};
        static const int pw[5] = {934, 989, 867, 801, 613};
        QString key = p1.trimmed();
        if (key.startsWith(QStringLiteral("несмотря"))) {
            key = QStringLiteral("несмотря");
        }
        const int sel = keys.indexOf(key);
        const int ex0 = 410;
        const int ey0 = 10;
        const int title = addText(QStringLiteral("Примеры"), ex0 + 290, ey0 + 15, 16);
        sprites[title].name = QStringLiteral("ex");
        for (int k = 0; k < 5; ++k) {
            if (k == sel) {
                const int i = addImage(ExAssets::exnPath(QStringLiteral("2.1.4/v%1.png").arg(k + 1)), ex0 + 28, ey0 + ly[k], QStringLiteral("ex"));
                if (!sprites[i].pix.isNull()) {
                    sprites[i].pix = sprites[i].pix.scaled(pw[k], 60);
                    sprites[i].w = pw[k];
                    sprites[i].h = 60;
                }
            } else {
                addText(labels[k], ex0 + 28, ey0 + ly[k], 24, Qt::black, QStringLiteral("ex"));
            }
        }
    }
}

void MovewordsEngine::setExplain(bool on) {
    explainOn = on;
    for (ExSprite &s : sprites) {
        if (s.name == QLatin1String("lob") || s.name == QLatin1String("lob2")) {
            s.visible = on;
        } else if (runtime == QLatin1String("2.2.5") && (s.name == QLatin1String("2") || s.name == QLatin1String("3") || s.name == QLatin1String("4"))) {
            const QString file = QStringLiteral("2.2.5/%1%2.png").arg(s.name, on ? QStringLiteral("o") : QString());
            s.setImage(ExAssets::exnPath(file));
        }
    }
    emit changed();
}

// ---------------- построители ----------------

void MovewordsEngine::build122() {
    const QStringList list = wordList();
    if (list.isEmpty()) {
        return;
    }
    const QStringList t = list[(currentEx - 1) % list.size()].split(QLatin1Char(';'));
    if (t.size() < 5) {
        return;
    }
    // Пациент: x,y = doc*100/70 (врач рисует в 80%, но делит на 70 — «растяжка» по X сохранена как в оригинале)
    const int phrase = addText(t[0], 220 * 100 / 70, 200 * 100 / 70);
    const int ans = qBound(1, t[1].toInt(), 3);
    const int otv = addText(t[1 + ans], 314 + sprites[phrase].w, 200 * 100 / 70, 26, QColor(0, 0, 255), QStringLiteral("otv"));
    sprites[otv].doctorOnly = true;
    doctorHint = t[1 + ans];
    if (p1 == QLatin1String("2")) {
        int docX = 220;
        for (int k = 0; k < 3; ++k) {
            // у пациента movable = false: вариант выбирается кликом (MouseUp), а не перетаскиванием
            const int i = addText(t[2 + k], docX * 100 / 70, 250 * 100 / 70, 26, Qt::black, QStringLiteral("otg"), false);
            docX += sprites[i].w * 80 / 100;
        }
    }
}

void MovewordsEngine::build211() {
    const int count = int(sizeof(k211Sentences) / sizeof(k211Sentences[0]));
    const QString words = QString::fromUtf8(k211Sentences[(currentEx - 1) % count]);
    addImage(ExAssets::exnPath(QStringLiteral("2.1.1/line.png")), 20, 200 * 100 / 60, QStringLiteral("line"));
    int bx = 25 * 100 / 60;
    for (const QString &w : words.split(QLatin1Char(' '), Qt::SkipEmptyParts)) {
        const int i = addText(w, bx, 250 * 100 / 60, 26, Qt::black, QString(), true);
        bx += sprites[i].w + 10 * 100 / 60;
    }
    m_lineMax = 200 * 100 / 60;
    m_sortLeft = 100; // sortWordsLeft по умолчанию в оригинале
    m_sortY = 309;     // sortWordsY(310)-1
    m_sortGap = 10;
}

void MovewordsEngine::build212() {
    const QStringList list = wordList();
    if (list.isEmpty()) {
        return;
    }
    const QStringList t = list[(currentEx - 1) % list.size()].split(QLatin1Char(';'));
    if (t.size() < 2) {
        return;
    }
    addImage(ExAssets::exnPath(QStringLiteral("2.1.2/line.png")), 33, 293, QStringLiteral("line"));
    addText(QStringLiteral("Задание"), 200, 250, 22);
    addText(QStringLiteral("Ответ"), 650, 250, 22);
    const int a = addText(t[0], 33, 298);
    sprites[a].x = 33 + (450 - sprites[a].w) / 2;
    const int b = addText(t[1], 467, 298, 26, Qt::black, QStringLiteral("otv"));
    sprites[b].x = 467 + (450 - sprites[b].w) / 2;
    sprites[b].doctorOnly = true;
    doctorHint = t[1];
}

void MovewordsEngine::layout213() {
    clear();
    block = false;
    addImage(ExAssets::exnPath(QStringLiteral("2.1.3/line.png")), 33, 333, QStringLiteral("line"));
    int x = 33;
    for (const QString &w : m_sentence) {
        const int i = addText(w, x, 190 * 100 / 60);
        x += sprites[i].w;
    }
    x = 33;
    for (const QString &r : m_replacements) {
        int digits = 0;
        const int n = leadingNumber(r, &digits);
        const int i = addText(r.mid(digits), x, 250, 26, QColor(0, 128, 0), QStringLiteral("repl%1").arg(n));
        sprites[i].doctorOnly = true;
        x += sprites[i].w;
    }
    if (m_hostApplied) {
        for (ExSprite &s : sprites) {
            s.x += m_hx;
            s.y += m_hy;
        }
    }
    emit changed();
}

void MovewordsEngine::build213() {
    const QStringList list = wordList();
    if (list.isEmpty()) {
        return;
    }
    const QStringList t = list[(currentEx - 1) % list.size()].split(QLatin1Char(';'));
    m_sentence = t[0].split(QLatin1Char(' '), Qt::SkipEmptyParts);
    m_replacements = t.mid(1);
    layout213();
}

void MovewordsEngine::build214() {
    const QStringList list = wordList();
    if (list.isEmpty()) {
        return;
    }
    const QString base = list[(currentEx - 1) % list.size()].trimmed();
    // doctorInit d2.1.4: (70,10) -> *100/60; примеры (control examples) — см. addAbsolute()
    addText(base + p1, 70 * 100 / 60, 10 * 100 / 60);
}

void MovewordsEngine::build223() {
    const QStringList list = wordList();
    if (list.isEmpty()) {
        return;
    }
    const QStringList t = list[(currentEx - 1) % list.size()].split(QLatin1Char(';'));
    if (t.size() < 4) {
        return;
    }
    const int q = addText(t[0], 0, 250 * 100 / 60, 26);
    centerDoc(q, 60);
    const int a = addText(t[1], 230 * 100 / 60, 350 * 100 / 60, 26, Qt::black, t[3].trimmed() == QLatin1String("1") ? QStringLiteral("ot") : QStringLiteral("not"));
    const int b = addText(t[2], 630 * 100 / 60, 350 * 100 / 60, 26, Qt::black, t[3].trimmed() == QLatin1String("2") ? QStringLiteral("ot") : QStringLiteral("not"));
    Q_UNUSED(a);
    Q_UNUSED(b);
    addDoneButton(1370, 760);
}

void MovewordsEngine::build224() {
    const QStringList list = wordList();
    if (list.isEmpty()) {
        return;
    }
    const QStringList t = list[(currentEx - 1) % list.size()].split(QLatin1Char(';'));
    if (t.size() < 5) {
        return;
    }
    // doctorInit d2.2.4: все спрайты setCenter(900) при 60%, y = 200/220/300/350/400 * 100/60
    const int ln = addImage(ExAssets::exnPath(QStringLiteral("2.2.4/line.png")), 0, 200 * 100 / 60, QStringLiteral("line"));
    centerDoc(ln, 60);
    const int q = addText(t[0], 0, 220 * 100 / 60, 26);
    centerDoc(q, 60);
    const int correct = t[4].trimmed().toInt();
    static const int docY[3] = {300, 350, 400};
    for (int k = 0; k < 3; ++k) {
        const int o = addText(t[1 + k], 0, docY[k] * 100 / 60, 26, Qt::black,
                              correct == k + 1 ? QStringLiteral("ot") : QStringLiteral("not"));
        centerDoc(o, 60);
    }
    addDoneButton(1364, 774);
}

void MovewordsEngine::build225() {
    const bool graphic = p1 == QStringLiteral("Графические задания");
    if (!graphic) {
        QString st = ExAssets::readAll(ExAssets::exnPath(QStringLiteral("2.2.5/data.txt")));
        st.replace(QStringLiteral("\r\n"), QString());
        st.replace(QLatin1Char('\n'), QString());
        const QStringList tmp = st.split(QLatin1Char(';'));
        const int records = tmp.size() / 6;
        if (records <= 0) {
            return;
        }
        const int r = (currentEx - 1) % records;
        const int base = r * 6;
        taskText = tmp.value(base).trimmed();
        explanationText = tmp.value(base + 5).trimmed();
        const int correct = tmp.value(base + 4).trimmed().toInt();
        // doctorInit d2.2.5: y = 100/150/200 (60%), центр 900, +200 при переносе на экран пациента
        for (int k = 0; k < 3; ++k) {
            const int i = addText(tmp.value(base + 1 + k).trimmed(), 0, (100 + 50 * k) * 100 / 60, 26, Qt::black,
                                  correct == k + 1 ? QStringLiteral("ot") : QString());
            centerDoc(i, 60, 900, 200);
        }
        addDoneButton(1590, 455);
        return;
    }
    // Графические задания: 4 задания
    struct Pic {
        const char *file;
        int x;
        int y;
        bool correct;
    };
    const int ex = ((currentEx - 1) % 4) + 1;
    QVector<Pic> pics;
    QString main;
    // d2.2.5 «Графические задания»: у врача верный вариант с подписью «Верно» (13/211/322/43);
    // uprSpace подменяет его пациенту той же картинкой без подписи (133/21/32/433).
    static const char *patientOt[4] = {"133.png", "21.png", "32.png", "433.png"};
    switch (ex) {
    case 1:
        main = QStringLiteral("11.png");
        pics = {{"12.png", 508, 583, false}, {"13.png", 842, 583, true}, {"14.png", 1175, 583, false}};
        break;
    case 2:
        main = QStringLiteral("2.png");
        pics = {{"211.png", 383, 583, true}, {"22.png", 717, 583, false}, {"23.png", 1050, 583, false}, {"24.png", 1333, 583, false}};
        break;
    case 3:
        main = QStringLiteral("3.png");
        pics = {{"31.png", 383, 583, false}, {"322.png", 717, 583, true}, {"33.png", 1050, 583, false}, {"34.png", 1333, 583, false}};
        break;
    default:
        main = QStringLiteral("4.png");
        pics = {{"41.png", 383, 583, false}, {"42.png", 717, 583, false}, {"43.png", 1050, 583, true}};
        break;
    }
    // главная картинка: smaller=60, x=110, y=100, setCenter(900), затем *100/60 + 200
    const int mainIdx = addImage(ExAssets::exnPath(QStringLiteral("2.2.5/") + main), 0, 100 * 100 / 60,
                                 main.left(main.indexOf(QLatin1Char('.'))));
    // у врача картинка рисуется в 60%: ширина для центрирования берётся из натурального размера
    centerDoc(mainIdx, 60, 900, 200);
    for (const Pic &p : pics) {
        const QString docFile = ExAssets::exnPath(QStringLiteral("2.2.5/") + QString::fromLatin1(p.file));
        if (!p.correct) {
            addImage(docFile, p.x, p.y, QString());
            continue;
        }
        const int i = addImage(ExAssets::exnPath(QStringLiteral("2.2.5/") + QString::fromLatin1(patientOt[ex - 1])), p.x, p.y,
                               QStringLiteral("ot"));
        sprites[i].doctorPix = ExAssets::pixmap(docFile);
    }
    addDoneButton(1600, 740);
}

void MovewordsEngine::build226() {
    if (p1 == QLatin1String("3")) {
        return; // uprSpace: вместо сцены — about.rtf (PatientWindow::setRtf)
    }
    if (p1 == QLatin1String("1") || p1.isEmpty()) {
        // _226.nextz_Click: при currentEx == 5 — снова 1 (5.png не показывается)
        const int n = ((currentEx - 1) % 4) + 1;
        // doctorInit d2.2.6/1: smaller=50, x=20, y=50, setCenter(900); пациент: *100/60
        const int st = addImage(ExAssets::exnPath(QStringLiteral("2.2.6/%1.png").arg(n)), 0, 50 * 100 / 60, QStringLiteral("story"));
        centerDoc(st, 50, 900);
        sprites[st].x = ((900 - sprites[st].w * 50 / 100) / 2) * 100 / 60;
        return;
    }
    // режим 2: расставить предложения в правильном порядке
    const QStringList lines = ExAssets::readLines(ExAssets::exnPath(QStringLiteral("2.2.6/data.txt")));
    QVector<Group> groups;
    Group cur;
    for (const QString &raw : lines) {
        const QString line = raw.trimmed();
        if (line.isEmpty()) {
            continue;
        }
        if (line[0].isDigit()) {
            cur.order = line;
            groups.append(cur);
            cur = Group();
        } else {
            cur.words.append(line);
        }
    }
    if (groups.isEmpty()) {
        return;
    }
    const Group &g = groups[(currentEx - 1) % groups.size()];
    m_order = g.order;
    addImage(ExAssets::exnPath(QStringLiteral("2.2.6/line2.png")), 400, 350, QStringLiteral("rm1"), 120);
    m_slotsY.clear();
    for (int k = 0; k < g.words.size(); ++k) {
        const int y = (400 + 30 * k) * 100 / 60 - 180;
        const int i = addText(g.words[k], 170 * 100 / 60 + 200, y, 26, Qt::black, QString::number(k), true, 80);
        sprites[i].word = g.words[k];
        m_slotsY.append(y);
    }
    // d2.2.6/2: врач видит рамку «Правильная последовательность» (rm2) с предложениями в верном порядке («up»).
    // В оригинале она над рамкой пациента; в общей сцене — справа сверху, свободная область.
    {
        const int rm2 = addImage(ExAssets::exnPath(QStringLiteral("2.2.6/line.png")), 1390 - m_hx, 10 - m_hy, QStringLiteral("rm2"), 60);
        sprites[rm2].doctorOnly = true;
        const QStringList order = m_order.split(QLatin1Char(';'), Qt::SkipEmptyParts);
        int y = 58;
        for (const QString &o : order) {
            const int k = o.trimmed().toInt();
            if (k < 0 || k >= g.words.size()) {
                continue;
            }
            const int i = addText(g.words[k], 1405 - m_hx, y - m_hy, 26, Qt::black, QStringLiteral("up"), false, 50);
            sprites[i].doctorOnly = true;
            y += 30;
        }
    }
    addDoneButton(1585, 750);
}

void MovewordsEngine::build311() {
    const QStringList list = wordList();
    if (list.isEmpty()) {
        return;
    }
    const QStringList t = list[(currentEx - 1) % list.size()].split(QLatin1Char(';'));
    if (t.size() < 3) {
        return;
    }
    const int a = addText(t[0], 262, 312);
    const int pts = addText(QStringLiteral("...."), sprites[a].x + sprites[a].w, 312, 26, Qt::black, QStringLiteral("points"));
    addText(t[1], sprites[pts].x + sprites[pts].w, 312);
    int x = 262;
    for (const QString &syl : {QStringLiteral("но"), QStringLiteral("ло"), QStringLiteral("би")}) {
        const int i = addText(syl, x, 375, 26, Qt::black,
                              t[2].trimmed() == syl ? QStringLiteral("cot") : QStringLiteral("ot"));
        x += sprites[i].w + 62;
    }
    addDoneButton(1500 + 90 - 500, 600 + 150 - 100);
}

void MovewordsEngine::build312() {
    const QStringList list = wordList();
    if (list.isEmpty()) {
        return;
    }
    const QStringList t = list[(currentEx - 1) % list.size()].split(QLatin1Char(';'));
    if (t.size() < 3) {
        return;
    }
    m_order = t[2].trimmed();
    const bool support = p1 == QLatin1String("1");
    const int ram = addImage(ExAssets::exnPath(QStringLiteral("3.1.2/line%1.png").arg(dataNumber)), 200 * 100 / 80, 240 * 100 / 80, QStringLiteral("ram"));
    int mx = 210 * 100 / 80;
    int first = -1;
    if (support) {
        const QStringList tok = t[0].split(QLatin1Char(' '), Qt::SkipEmptyParts);
        if (!tok.isEmpty()) {
            first = addText(tok[0], 210 * 100 / 80, 243 * 100 / 80, 35, Qt::black, QStringLiteral("first"));
        }
        for (int i = 1; i < tok.size(); ++i) {
            const int s = addText(tok[i], mx, 387, 35, Qt::black, QStringLiteral("var"), true);
            mx += sprites[s].w;
        }
    } else {
        for (const QString &w : t[1].split(QLatin1Char(' '), Qt::SkipEmptyParts)) {
            const int s = addText(w, mx, 387, 35, Qt::black, QStringLiteral("var"), true);
            mx += sprites[s].w;
        }
    }
    m_lineMax = sprites[ram].y + sprites[ram].h;
    if (first >= 0) {
        m_sortLeft = sprites[first].x + sprites[first].w;
        m_sortY = sprites[first].y - 1;
        m_sortAlt = false;
    } else {
        m_sortLeft = 250;
        m_sortY = sprites[ram].y + 5;
        m_sortAlt = true;
    }
    m_sortGap = 10;
    // подсказка врачу — ответ
    doctorHint = m_order;
    const int hint = addText(m_order, 250, 225, 25, Qt::black, QStringLiteral("pd"));
    sprites[hint].doctorOnly = true;
    addDoneButton(1290, 650);
}

void MovewordsEngine::build323() {
    const QStringList list = wordList();
    if (list.isEmpty()) {
        return;
    }
    const QStringList t = list[(currentEx - 1) % list.size()].split(QLatin1Char(';'));
    if (t.size() < 3) {
        return;
    }
    const bool syn = p1 != QStringLiteral("Антонимы");
    // doctorInit d3.2.3: line и подпись — setCenter(900) при 70%, пациент *100/70
    const int ram = addImage(ExAssets::exnPath(QStringLiteral("3.2.2/line.png")), 200 * 100 / 70, 240 * 100 / 70, QStringLiteral("ram"));
    centerDoc(ram, 70);
    const int cap = addText(syn ? QStringLiteral("Синонимы") : QStringLiteral("Антонимы"), 200 * 100 / 70, 210 * 100 / 70, 20);
    centerDoc(cap, 70);
    const int w0 = addText(t[0], 270 * 100 / 70, 250 * 100 / 70, 30);
    const int tr = addText(QStringLiteral("-"), sprites[w0].x + sprites[w0].w, 357, 30, Qt::black, QStringLiteral("tr"));
    const int cr = addText(syn ? t[1] : t[2], sprites[tr].x + sprites[tr].w, 357, 30, QColor(0, 0, 255), QStringLiteral("correct"));
    sprites[cr].doctorOnly = true;
    doctorHint = syn ? t[1] : t[2];
    if (showHints) {
        // два варианта в фиксированном порядке: синоним, затем антоним
        const QStringList vars = {t[1], t[2]};
        int x = 270 * 100 / 70;
        for (const QString &v : vars) {
            const int i = addText(v, x, 330 * 100 / 70, 30, Qt::black, QStringLiteral("var"));
            x += sprites[i].w + 71;
        }
        addDoneButton(1290, 650);
    }
}

void MovewordsEngine::layout324() {
    // реплика расчёта позиций (слоты + слова подряд)
    const int hx = m_hostApplied ? m_hx : 0;
    const int hy = m_hostApplied ? m_hy : 0;
    int x = 210 * 100 / 80 + hx;
    const int y = 150 * 100 / 80 + hy;
    const int gap = 50 * 100 / 80;
    auto place = [&](int idx) {
        if (idx < 0) {
            return;
        }
        sprites[idx].x = x;
        sprites[idx].y = y;
        x += sprites[idx].w;
    };
    auto placeSlot = [&](int k) {
        const int slot = indexByName(QStringLiteral("p%1").arg(k));
        const int cor = indexByName(QStringLiteral("correct%1").arg(k));
        int width = qMax(slot >= 0 ? sprites[slot].w : 0, cor >= 0 ? sprites[cor].w : 0);
        const int letter = m_slotLetter[k - 1];
        if (letter >= 0) {
            width = sprites[letter].w;
            sprites[letter].x = x;
            sprites[letter].y = y;
            if (slot >= 0) sprites[slot].visible = false;
            if (cor >= 0) sprites[cor].visible = false;
        } else {
            if (slot >= 0) sprites[slot].visible = true;
            if (cor >= 0) sprites[cor].visible = true;
        }
        if (slot >= 0) { sprites[slot].x = x; sprites[slot].y = y; }
        if (cor >= 0) { sprites[cor].x = x; sprites[cor].y = y; }
        x += width;
    };
    placeSlot(1);
    place(indexByName(QStringLiteral("word1")));
    x += gap;
    place(indexByName(QStringLiteral("word2")));
    placeSlot(2);
    place(indexByName(QStringLiteral("word22")));
    x += gap;
    place(indexByName(QStringLiteral("word3")));
    placeSlot(3);
}

void MovewordsEngine::build324() {
    const QStringList list = wordList();
    if (list.isEmpty()) {
        return;
    }
    const QStringList t = list[(currentEx - 1) % list.size()].split(QLatin1Char(';'));
    if (t.size() < 4) {
        return;
    }
    const QStringList p0 = t[0].split(QLatin1Char('|'));
    const QStringList p1l = t[1].split(QLatin1Char('|'));
    const QStringList p2l = t[2].split(QLatin1Char('|'));
    if (p0.size() < 2 || p1l.size() < 2 || p2l.size() < 2) {
        return;
    }
    const QColor blue(0, 0, 255);
    auto correct = [&](const QString &name, const QString &letter) {
        const int i = addText(letter, 0, 0, 40, blue, name);
        sprites[i].doctorOnly = true;
    };
    auto slot = [&](const QString &name) {
        const int i = addText(QStringLiteral(".."), 0, 0, 40, Qt::black, name);
        sprites[i].patientOnly = true;
    };
    correct(QStringLiteral("correct1"), p0[1].left(1));
    slot(QStringLiteral("p1"));
    addText(p0[1].mid(1), 0, 0, 40, Qt::black, QStringLiteral("word1"));
    addText(p1l[0], 0, 0, 40, Qt::black, QStringLiteral("word2"));
    correct(QStringLiteral("correct2"), p1l[1].left(1));
    slot(QStringLiteral("p2"));
    addText(p1l[1].mid(1), 0, 0, 40, Qt::black, QStringLiteral("word22"));
    addText(p2l[0], 0, 0, 40, Qt::black, QStringLiteral("word3"));
    correct(QStringLiteral("correct3"), p2l[1].left(1));
    slot(QStringLiteral("p3"));
    if (showHints) {
        const QStringList letters = t[3].split(QLatin1Char(' '), Qt::SkipEmptyParts);
        int x = 375;
        int n = 1;
        for (const QString &l : letters) {
            const int i = addText(l, x, 312, 40, Qt::black, QStringLiteral("b%1").arg(n++), true);
            x += sprites[i].w + 125;
        }
        addDoneButton(1200, 550);
    }
    layout324();
    doctorHint = p0[1].left(1) + QLatin1Char(' ') + p1l[1].left(1) + QLatin1Char(' ') + p2l[1].left(1);
}

// ---------------- ввод пациента ----------------

void MovewordsEngine::toggleExclusive(int idx, const QStringList &names) {
    const bool was = sprites[idx].border;
    for (ExSprite &s : sprites) {
        if (names.contains(s.name) || s.name.isEmpty()) {
            s.border = false;
        }
    }
    sprites[idx].border = !was;
}

void MovewordsEngine::pressDone() {
    if (runtime == QLatin1String("2.2.3")) {
        const int ot = indexByName(QStringLiteral("ot"));
        const int not_ = indexByName(QStringLiteral("not"));
        if (ot >= 0 && sprites[ot].border) {
            sprites[ot].green = true;
            finishDone(false);
        } else if (not_ >= 0 && sprites[not_].border) {
            sprites[not_].red = true;
            finishDone(true);
        }
    } else if (runtime == QLatin1String("2.2.4")) {
        for (int i = 0; i < sprites.size(); ++i) {
            if (sprites[i].border && sprites[i].name == QLatin1String("not")) {
                sprites[i].red = true;
                finishDone(true);
                return;
            }
        }
        for (int i = 0; i < sprites.size(); ++i) {
            if (sprites[i].border && sprites[i].name == QLatin1String("ot")) {
                sprites[i].green = true;
                finishDone(false);
                return;
            }
        }
    } else if (runtime == QLatin1String("2.2.5")) {
        bool wrong = false;
        bool any = false;
        for (const ExSprite &s : sprites) {
            if (s.border) {
                any = true;
                if (s.name != QLatin1String("ot")) {
                    wrong = true;
                }
            }
        }
        if (!any) {
            return;
        }
        for (ExSprite &s : sprites) {
            if (s.border && s.name != QLatin1String("ot")) {
                s.red = true;
            }
        }
        if (!wrong) {
            const int ot = indexByName(QStringLiteral("ot"));
            if (ot >= 0) {
                sprites[ot].green = true;
            }
        }
        finishDone(wrong);
    } else if (runtime == QLatin1String("2.2.6")) {
        // порядок по Y
        QVector<int> idx;
        for (int i = 0; i < sprites.size(); ++i) {
            if (sprites[i].movable) {
                idx.append(i);
            }
        }
        std::sort(idx.begin(), idx.end(), [&](int a, int b) { return sprites[a].y < sprites[b].y; });
        const QStringList order = m_order.split(QLatin1Char(';'), Qt::SkipEmptyParts);
        bool ok = idx.size() == order.size();
        for (int k = 0; ok && k < idx.size(); ++k) {
            ok = sprites[idx[k]].name.toInt() == order[k].trimmed().toInt();
        }
        const int rm = indexByName(QStringLiteral("rm1"));
        if (rm >= 0) {
            (ok ? sprites[rm].green : sprites[rm].red) = true;
        }
        finishDone(!ok);
    } else if (runtime == QLatin1String("3.1.1")) {
        if (m_mem < 0) {
            return;
        }
        const bool ok = sprites[m_mem].name == QLatin1String("cot");
        (ok ? sprites[m_mem].green : sprites[m_mem].red) = true;
        finishDone(!ok);
    } else if (runtime == QLatin1String("3.1.2")) {
        QVector<int> idx;
        for (int i = 0; i < sprites.size(); ++i) {
            if (sprites[i].movable && sprites[i].y <= m_lineMax) {
                idx.append(i);
            }
        }
        std::sort(idx.begin(), idx.end(), [&](int a, int b) { return sprites[a].x < sprites[b].x; });
        QString word;
        for (int i : idx) {
            word += sprites[i].word;
        }
        const int first = indexByName(QStringLiteral("first"));
        const int skip = first >= 0 ? sprites[first].word.size() : 2;
        const bool ok = word == m_order || (m_order.size() >= skip && word == m_order.mid(skip));
        const int ram = indexByName(QStringLiteral("ram"));
        if (ram >= 0) {
            (ok ? sprites[ram].green : sprites[ram].red) = true;
        }
        finishDone(!ok);
    } else if (runtime == QLatin1String("3.2.3")) {
        if (m_mem < 0) {
            return;
        }
        const int cor = indexByName(QStringLiteral("correct"));
        const bool ok = cor >= 0 && sprites[m_mem].word == sprites[cor].word;
        (ok ? sprites[m_mem].green : sprites[m_mem].red) = true;
        finishDone(!ok);
    } else if (runtime == QLatin1String("3.2.4")) {
        if (m_slotLetter[0] < 0 || m_slotLetter[1] < 0 || m_slotLetter[2] < 0) {
            return;
        }
        bool wrong = false;
        for (int k = 1; k <= 3; ++k) {
            const int cor = indexByName(QStringLiteral("correct%1").arg(k));
            const int let = m_slotLetter[k - 1];
            const bool ok = cor >= 0 && sprites[let].word == sprites[cor].word;
            (ok ? sprites[let].green : sprites[let].red) = true;
            if (!ok) {
                wrong = true;
            }
        }
        finishDone(wrong);
    }
}

void MovewordsEngine::patientPress(const QPoint &p) {
    ExEngine::patientPress(p);
    // 3.2.4: когда все три пропуска заполнены, буквы больше не двигаются (exname = "x"), работает только «Готово»
    if (runtime == QLatin1String("3.2.4") && m_moving >= 0 && sprites[m_moving].name.startsWith(QLatin1Char('b')) &&
        m_slotLetter[0] >= 0 && m_slotLetter[1] >= 0 && m_slotLetter[2] >= 0) {
        m_dragging = false;
    }
}

// MouseUp: pressDone и выбор варианта срабатывают по спрайту, на котором нажали (точка отпускания не проверяется)
bool MovewordsEngine::releaseClick(int mv) {
    ExSprite &s = sprites[mv];
    if (s.name == QLatin1String("done")) {
        pressDone();
        emit changed();
        return true;
    }
    if ((runtime == QLatin1String("2.2.3") || runtime == QLatin1String("2.2.4")) &&
        (s.name == QLatin1String("ot") || s.name == QLatin1String("not"))) {
        toggleExclusive(mv, {QStringLiteral("ot"), QStringLiteral("not")});
        emit changed();
        return true;
    }
    if (runtime == QLatin1String("2.2.5") && (s.name.isEmpty() || s.name == QLatin1String("ot"))) {
        toggleExclusive(mv, {QStringLiteral("ot")});
        emit changed();
        return true;
    }
    return false;
}

void MovewordsEngine::sortWordsInLine(int lineMax, int left, int y, int gap) {
    QVector<int> idx;
    for (int i = 0; i < sprites.size(); ++i) {
        if (sprites[i].movable && sprites[i].y <= lineMax) {
            idx.append(i);
        }
    }
    std::sort(idx.begin(), idx.end(), [&](int a, int b) { return sprites[a].x < sprites[b].x; });
    int x = left;
    for (int i : idx) {
        sprites[i].x = x;
        sprites[i].y = y;
        x += sprites[i].w + gap;
    }
}

void MovewordsEngine::patientRelease(const QPoint &p) {
    const int mv = m_moving;
    const bool wasDragging = m_dragging;
    ExEngine::patientRelease(p);
    if (mv < 0 || mv >= sprites.size() || block || !wasDragging) {
        return;
    }
    if (releaseClick(mv)) {
        return;
    }
    ExSprite &s = sprites[mv];
    const QPoint start = m_start;

    if (runtime == QLatin1String("1.2.2") && s.name == QLatin1String("otg")) {
        const int phrase = 0;
        if (m_mem >= 0 && m_mem != mv) {
            sprites[m_mem].x = m_home[m_mem].x();
            sprites[m_mem].y = m_home[m_mem].y();
        }
        m_mem = mv;
        s.x = sprites[phrase].x + sprites[phrase].w;
        s.y = sprites[phrase].y;
        const int otv = indexByName(QStringLiteral("otv"));
        if (otv >= 0) {
            sprites[otv].visible = false;
        }
    } else if (runtime == QLatin1String("2.1.1") && s.movable) {
        s.border = false;
        sortWordsInLine(m_lineMax, m_sortLeft, m_sortY, m_sortGap);
    } else if (runtime == QLatin1String("3.1.2") && s.movable) {
        sortWordsInLine(m_lineMax, m_sortLeft, m_sortY + (m_sortAlt ? 0 : 0), m_sortGap);
    } else if (runtime == QLatin1String("3.1.1") && (s.name == QLatin1String("ot") || s.name == QLatin1String("cot"))) {
        const int pts = indexByName(QStringLiteral("points"));
        if (m_mem >= 0 && m_mem != mv) {
            const QPoint blank(sprites[m_mem].x, sprites[m_mem].y);
            const QPoint buf = m_memPos;
            m_memPos = start;
            s.x = blank.x();
            s.y = blank.y();
            sprites[m_mem].x = buf.x();
            sprites[m_mem].y = buf.y();
            m_mem = mv;
        } else if (m_mem < 0 && pts >= 0) {
            m_memPos = start;
            m_mem = mv;
            s.x = sprites[pts].x;
            s.y = sprites[pts].y;
            sprites[pts].visible = false;
        } else if (m_mem == mv && pts >= 0) {
            s.x = sprites[pts].x;
            s.y = sprites[pts].y;
        }
    } else if (runtime == QLatin1String("3.2.3") && s.name == QLatin1String("var")) {
        const int tr = indexByName(QStringLiteral("tr"));
        if (tr < 0) {
            return;
        }
        const QPoint slot(sprites[tr].x + sprites[tr].w, sprites[tr].y);
        if (m_mem >= 0 && m_mem != mv) {
            const QPoint buf = m_memPos;
            m_memPos = start;
            s.x = slot.x();
            s.y = slot.y();
            sprites[m_mem].x = buf.x();
            sprites[m_mem].y = buf.y();
            m_mem = mv;
        } else if (m_mem < 0) {
            m_memPos = start;
            m_mem = mv;
            s.x = slot.x();
            s.y = slot.y();
            const int cor = indexByName(QStringLiteral("correct"));
            if (cor >= 0) {
                sprites[cor].visible = false;
            }
        } else {
            s.x = slot.x();
            s.y = slot.y();
        }
    } else if (runtime == QLatin1String("2.2.6") && s.movable) {
        QVector<int> idx;
        for (int i = 0; i < sprites.size(); ++i) {
            if (sprites[i].movable) {
                idx.append(i);
            }
        }
        std::sort(idx.begin(), idx.end(), [&](int a, int b) { return sprites[a].y < sprites[b].y; });
        // saveSortIndexes: выравнивается только Y, X остаётся там, где предложение отпустили
        for (int k = 0; k < idx.size() && k < m_slotsY.size(); ++k) {
            sprites[idx[k]].y = int(m_slotsY[k]);
        }
    } else if (runtime == QLatin1String("3.2.4") && s.name.startsWith(QLatin1Char('b'))) {
        int from = -1;
        for (int k = 0; k < 3; ++k) {
            if (m_slotLetter[k] == mv) {
                from = k;
            }
        }
        auto hit = [&](const QString &name) {
            const int i = indexByName(name);
            return i >= 0 && sprites[i].rect().contains(p);
        };
        // цель: пропуск (или буква в нём) либо соседнее слово — word1, word2..word22, word3
        int target = -1;
        for (int k = 1; k <= 3; ++k) {
            const int slot = indexByName(QStringLiteral("p%1").arg(k));
            bool on = slot >= 0 && sprites[slot].rect().adjusted(-40, -30, 40, 30).contains(p);
            const int letter = m_slotLetter[k - 1];
            if (letter >= 0 && letter != mv && sprites[letter].rect().contains(p)) {
                on = true;
            }
            if (k == 1) {
                on = on || hit(QStringLiteral("word1"));
            } else if (k == 2) {
                const int w2 = indexByName(QStringLiteral("word2"));
                const int w22 = indexByName(QStringLiteral("word22"));
                if (w2 >= 0 && w22 >= 0) {
                    const QRect span(QPoint(sprites[w2].x, sprites[w2].y),
                                     QPoint(sprites[w22].x + sprites[w22].w, sprites[w2].y + sprites[w2].h));
                    on = on || span.contains(p);
                }
            } else {
                on = on || hit(QStringLiteral("word3"));
            }
            if (on) {
                target = k;
            }
        }
        if (target > 0) {
            const int prev = m_slotLetter[target - 1];
            if (from >= 0) {
                m_slotLetter[from] = -1;
            }
            if (prev >= 0 && prev != mv) {
                // вытесненная буква уходит туда, откуда взяли новую (memX/memY)
                if (from >= 0) {
                    m_slotLetter[from] = prev;
                } else {
                    sprites[prev].x = start.x();
                    sprites[prev].y = start.y();
                }
            }
            m_slotLetter[target - 1] = mv;
        } else if (from >= 0) {
            // брошена мимо пропусков — остаётся там, где отпустили
            m_slotLetter[from] = -1;
        }
        layout324();
    }
    emit changed();
}

// ---------------- действия врача ----------------

void MovewordsEngine::doctorClick(const QPoint &p) {
    const int i = topAt(p, false);
    if (i < 0) {
        return;
    }
    if (runtime == QLatin1String("2.1.1") && sprites[i].movable && sprites[i].word.size() > 2) {
        sprites[i].border = !sprites[i].border; // подсказка пациенту: рамка на слове
    } else if (runtime == QLatin1String("2.1.2") && sprites[i].name == QLatin1String("otv")) {
        sprites[i].name = QStringLiteral("showotv");
        sprites[i].doctorOnly = false;
    } else if (runtime == QLatin1String("2.1.3") && sprites[i].name.startsWith(QLatin1String("repl"))) {
        const int n = sprites[i].name.mid(4).toInt();
        const QString word = sprites[i].word;
        if (n >= 1 && n <= m_sentence.size()) {
            m_sentence[n - 1] = word;
        }
        for (int k = 0; k < m_replacements.size(); ++k) {
            int digits = 0;
            if (leadingNumber(m_replacements[k], &digits) == n && m_replacements[k].mid(digits) == word) {
                m_replacements.removeAt(k);
                break;
            }
        }
        layout213();
        return;
    }
    emit changed();
}
