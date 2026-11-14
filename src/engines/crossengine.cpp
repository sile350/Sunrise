#include "crossengine.h"

#include <QFont>
#include <QFontMetrics>
#include <QPainter>
#include <QPen>

namespace {

// uprSpace 4.3.6: cword (10,200), pictureBox1 (19,14).
constexpr int kGridX = 10 + 19;
constexpr int kGridY = 200 + 14;
// initEx: panel1.Left = pictureBox.Left + Width + 10 = 987, Top = 5.
constexpr int kClueX = 10 + 987;
constexpr int kClueY = 200 + 5;

const QColor kLetter(102, 255, 153);
const QColor kSelect(238, 237, 133, 121);
const QColor kClear(0, 0, 0, 0);

constexpr int kCluePointSize = 20; // крупнее оригинальных 12pt — и у врача, и у пациента

QPixmap cluePixmap(const QString &text, bool selected) {
    QFont font(QStringLiteral("Microsoft Sans Serif"));
    font.setStyleHint(QFont::SansSerif);
    font.setPointSize(kCluePointSize);
    QFontMetrics fm(font);
    const int pad = 4;
    // Длинные формулировки — с переносом в пределах панели вопросов (~700px)
    const int maxW = 700;
    const QRect br = fm.boundingRect(QRect(0, 0, maxW - pad * 2, 2000), Qt::TextWordWrap, text);
    const int w = qMin(maxW, br.width() + pad * 2);
    const int h = br.height() + pad;
    QPixmap pm(qMax(1, w), qMax(1, h));
    pm.fill(selected ? QColor(255, 255, 0) : Qt::transparent);
    QPainter p(&pm);
    p.setRenderHint(QPainter::TextAntialiasing, true);
    p.setFont(font);
    p.setPen(Qt::black);
    p.drawText(QRect(pad, pad / 2, w - pad * 2, h - pad / 2), Qt::AlignLeft | Qt::AlignTop | Qt::TextWordWrap, text);
    return pm;
}

} // namespace

void CrossEngine::loadPuzzle(int exercise) {
    gQuest.clear();
    vQuest.clear();
    gClues.clear();
    vClues.clear();
    switch (exercise) {
    case 1:
        gQuest = QStringList{QStringLiteral("лимон"), QStringLiteral("марка"), QStringLiteral("юмор")};
        vQuest = QStringList{QStringLiteral("малина"), QStringLiteral("маска"), QStringLiteral("банан")};
        gClues = QStringList{QStringLiteral("1. Фрукт цитрусовый. Имеет кислый вкус, желтого цвета. "),
                  QStringLiteral("2. Наклеивается на конверт для отправления письма. "),
                  QStringLiteral("3. Интеллектуальная способность подмечать в явлениях их комичные, смешные стороны. ")};
        vClues = QStringList{QStringLiteral("1. Ягода кисло-сладкого вкуса. Растет на кустах. Считается, что медведь любит мед и… "),
                  QStringLiteral("2. На карнавале, чтобы никто не узнал, на лицо надевают... "),
                  QStringLiteral("3. Субтропический, желтый, сладкий, продолговатый съедобный плод. ")};
        break;
    case 2:
        gQuest = QStringList{QStringLiteral("соловей"), QStringLiteral("весна"), QStringLiteral("совет"), QStringLiteral("знамя")};
        vQuest = QStringList{QStringLiteral("подснежник"), QStringLiteral("фрегат"), QStringLiteral("рояль"), QStringLiteral("успех")};
        gClues = QStringList{QStringLiteral("1. Невзрачная птичка с очень красивым голосом (свистящий разбойник из русских былин)"),
                  QStringLiteral("2. Пора года."),
                  QStringLiteral("3. Дружеское наставление, мнение, высказанное кому-нибудь по поводу того, как ему поступить, что сделат"),
                  QStringLiteral("4. Широкое полотнище на древке, принадлежащее воинской части, государству (переходящий коллективный текстиль).")};
        vClues = QStringList{QStringLiteral("1. Первый весенний лесной цветок."),
                  QStringLiteral("2. Военный парусный трехмачтовый корабль или океаническая птица."),
                  QStringLiteral("3. Я стою на трех ногах, ноги в черных сапогах, зубы белые, педаль, а зовут меня ... (музыкальный инструмент)."),
                  QStringLiteral("4. Общественное признание (провал с точностью наоборот).")};
        break;
    case 3:
        gQuest = QStringList{QStringLiteral("кадет"), QStringLiteral("конфета"), QStringLiteral("жрица"), QStringLiteral("циклоп")};
        vQuest = QStringList{QStringLiteral("башня"), QStringLiteral("серебро"), QStringLiteral("заяц"), QStringLiteral("бинт")};
        // В cross.cs вторая строка записана в qg3 повторно; по номеру это вопрос 4.
        gClues = QStringList{QStringLiteral("1. Воспитанник кадетского корпуса. "),
                  QStringLiteral("2. Сладкое кондитерское изделие в фантике."),
                  QStringLiteral("3. Служительница божества, культа."),
                  QStringLiteral("4. Мифический великан с одним глазом во лбу.")};
        vClues = QStringList{QStringLiteral("1. Высокое строение, чья высота явно преобладает над длиной и шириной."),
                  QStringLiteral("2. Драгоценный металл, используемый в борьбе с бактериями. "),
                  QStringLiteral("3. Зверек отряда грызунов, с длинными ушами (безбилетный пассажир). "),
                  QStringLiteral("4. Длинная узкая лента из марли для перевязки ран. ")};
        break;
    case 4:
        gQuest = QStringList{QStringLiteral("дятел"), QStringLiteral("беседка"), QStringLiteral("армия"), QStringLiteral("европа")};
        vQuest = QStringList{QStringLiteral("колбаса"), QStringLiteral("хвост"), QStringLiteral("неделя"), QStringLiteral("январь")};
        gClues = QStringList{QStringLiteral("1. Деревообрабатывающая птица."),
                  QStringLiteral("2. Крытая лёгкая постройка в саду для беседы и отдыха."),
                  QStringLiteral("3. Совокупность вооруженных сил государства. "),
                  QStringLiteral("4. Континент (материк). ")};
        vClues = QStringList{QStringLiteral("1. Мясной пищевой продукт (вареная или копченая). "),
                  QStringLiteral("2. Задняя часть тела у некоторых животных."),
                  QStringLiteral("3. Единица исчисления времени (7 дней)."),
                  QStringLiteral("4. Зимний месяц.")};
        break;
    case 5:
        gQuest = QStringList{QStringLiteral("пограничник"), QStringLiteral("пингвин"), QStringLiteral("борщ"), QStringLiteral("банкомат")};
        vQuest = QStringList{QStringLiteral("тропинка"), QStringLiteral("берег"), QStringLiteral("монолог"), QStringLiteral("изюм")};
        gClues = QStringList{QStringLiteral("1. Военнослужащий пограничной заставы. "),
                  QStringLiteral("2. Полярная нелетающая птица. "),
                  QStringLiteral("3. Свекольный горячий суп."),
                  QStringLiteral("4. Автомат, выдающий наличные деньги по кредитной карточке.")};
        vClues = QStringList{QStringLiteral("1. Узкая дорожка в лесу, горах, поле."),
                  QStringLiteral("2. Край земли около воды."),
                  QStringLiteral("3. Речь одного человека, обращенная к слушателям или к себе. "),
                  QStringLiteral("4. Сушеные ягоды винограда. ")};
        break;
    case 6:
        gQuest = QStringList{QStringLiteral("среда"), QStringLiteral("тансовщица"), QStringLiteral("кругозор"), QStringLiteral("шаровары")};
        vQuest = QStringList{QStringLiteral("незабудка"), QStringLiteral("поезд"), QStringLiteral("пещера"), QStringLiteral("барабан")};
        gClues = QStringList{QStringLiteral("1. День недели. "),
                  QStringLiteral("2. Исполнительница танцев."),
                  QStringLiteral("3. Широта интересов, знаний."),
                  QStringLiteral("4. Широкие штаны.")};
        vClues = QStringList{QStringLiteral(" 1. Цветок с мелкими голубыми цветками (судя по названию, обладает 'хорошей памятью')."),
                  QStringLiteral("2. Состав сцепленных железнодорожных вагонов, приводимых в движение локомотивом."),
                  QStringLiteral("3. Полое пространство в гор\u0300е, имеющее выход наружу. "),
                  QStringLiteral("4. Ударный музыкальный инструмент.")};
        break;
    case 7:
        gQuest = QStringList{QStringLiteral("опасность"), QStringLiteral("какао"), QStringLiteral("тонометр"), QStringLiteral("окулист")};
        vQuest = QStringList{QStringLiteral("творчество"), QStringLiteral("танкист"), QStringLiteral("паника"), QStringLiteral("пьедестал")};
        gClues = QStringList{QStringLiteral("1.  Возможность, угроза чего-нибудь очень плохого (противоположность безопасности)."),
                  QStringLiteral("2.  Напиток из плодов шоколадного дерева. "),
                  QStringLiteral("3.  Прибор для измерения давления крови."),
                  QStringLiteral("4.  Врач по глазным болезням.")};
        vClues = QStringList{QStringLiteral(" 1.  Создание новых произведений искусства, науки. "),
                  QStringLiteral("2.  Водитель танка. "),
                  QStringLiteral("3.  Неудержимый страх, сразу охватывающий человека или многих людей "),
                  QStringLiteral("4.  Основание скульптуры, памятника (тумба для медалистов).")};
        break;
    case 8:
        gQuest = QStringList{QStringLiteral("миллиметр"), QStringLiteral("пилот"), QStringLiteral("анкета"), QStringLiteral("галстук")};
        vQuest = QStringList{QStringLiteral("гигиена"), QStringLiteral("диктант"), QStringLiteral("жезл"), QStringLiteral("трамвай")};
        gClues = QStringList{QStringLiteral("1.   Мера длины, одна тысячная метра."),
                  QStringLiteral("2.   Водитель самолета или гоночного болида. "),
                  QStringLiteral("3.   Сбор сведений путем получения ответов на определенные вопросы (опросный лист)."),
                  QStringLiteral("4.   Предмет мужской одежды, завязываемая узлом вокруг воротника. ")};
        vClues = QStringList{QStringLiteral(" 1.   Совокупность мер, направленных на поддержание чистоты и здоровья. "),
                  QStringLiteral("2.   Письменная работа под диктовку. "),
                  QStringLiteral("3.   Короткая палка, которой регулировщик движения дает указания транспорту, пешеходам. "),
                  QStringLiteral("4.   Городской общественный транспорт, который ездит по рельсам. ")};
        break;
    case 9:
        gQuest = QStringList{QStringLiteral("раствор"), QStringLiteral("купол"), QStringLiteral("груша"), QStringLiteral("финиш")};
        vQuest = QStringList{QStringLiteral("типография"), QStringLiteral("ромб"), QStringLiteral("школа"), QStringLiteral("алмаз")};
        gClues = QStringList{QStringLiteral("1. Однородная смесь из веществ и жидкости или нескольких жидкостей.  "),
                  QStringLiteral("2. Крыша здания в виде полушария. "),
                  QStringLiteral("3. И фрукт, и спортивный инвентарь. "),
                  QStringLiteral("4. Конечный пункт дистанции в состязаниях на скорость. ")};
        vClues = QStringList{QStringLiteral(" 1.   Полиграфическое предприятие, выпускающее печатную продукцию. "),
                  QStringLiteral("2.   Четырехугольник, все стороны которого равны, но не квадрат."),
                  QStringLiteral("3.   Заведение, где учатся дети. "),
                  QStringLiteral("4.   Самый твердый драгоценный камень. ")};
        break;
    default:
        gQuest = QStringList{QStringLiteral("официантка"), QStringLiteral("хобот"), QStringLiteral("локоть"), QStringLiteral("кольцо")};
        vQuest = QStringList{QStringLiteral("холод"), QStringLiteral("бинокль"), QStringLiteral("внучка"), QStringLiteral("экватор")};
        gClues = QStringList{QStringLiteral("1. Работница ресторана, разносящая еду.   "),
                  QStringLiteral("2. Нос слона. "),
                  QStringLiteral("3. Место сгиба руки. "),
                  QStringLiteral("4. Украшение, символ бесконечности, носимый на пальце.  ")};
        vClues = QStringList{QStringLiteral(" 1.    Низкая температура воздуха. "),
                  QStringLiteral("2.     Оптический прибор для рассматривания далёких предметов."),
                  QStringLiteral("3.    Дочь сына или дочери. "),
                  QStringLiteral("4.    Воображаемая линия, делящая земной шар на два полушария.")};
        break;
    }
}

void CrossEngine::rebuild(int exercise) {
    clear();
    block = false;
    userNowQuest.clear();
    m_wordState.clear();
    m_anyProgress = false;
    // Как фон экрана пациента — один цвет без «рамки» letterbox.
    background = QColor(249, 249, 249);
    int n = exercise;
    while (n > 10) {
        n -= 10;
    }
    while (n < 1) {
        n += 10;
    }
    currentEx = n;
    loadPuzzle(currentEx);
    buildGrid();
    placeHorizontal(gQuest, QStringLiteral("g"));
    placeVertical(vQuest, QStringLiteral("v"));
    addClues();
    emit changed();
}

void CrossEngine::buildGrid() {
    const QString dir = QStringLiteral("4.3.6/");
    const QPixmap sample = ExAssets::pixmap(ExAssets::exnPath(dir + QStringLiteral("cell.png")));
    const int cW = sample.width();
    const int cH = sample.height();
    if (cW < 2 || cH < 2) {
        return;
    }
    const QStringList rows = ExAssets::readLines(
        ExAssets::exnPath(dir + QStringLiteral("data") + QString::number(currentEx) + QStringLiteral(".txt")));
    int y = 10;
    for (const QString &row : rows) {
        const QStringList cols = row.split(QLatin1Char(';'));
        int x = 100;
        for (const QString &token : cols) {
            if (token != QLatin1String("_") && !token.isEmpty()) {
                const int i = addSprite();
                const bool blank = token == QLatin1String("0");
                sprites[i].setImage(ExAssets::exnPath(dir + (blank ? QStringLiteral("cell") : token) + QStringLiteral(".png")));
                sprites[i].x = kGridX + x;
                sprites[i].y = kGridY + y;
                sprites[i].name = QStringLiteral("cell");
                if (!blank) {
                    sprites[i].name2 = token;
                }
            }
            x += cW - 2;
        }
        y += cH - 2;
    }
}

void CrossEngine::placeHorizontal(const QStringList &words, const QString &prefix) {
    for (int i = 0; i < words.size(); ++i) {
        const int marker = indexByName2(prefix + QString::number(i + 1));
        if (marker < 0) {
            continue;
        }
        const int mx = sprites[marker].x;
        const int my = sprites[marker].y;
        const int mw = sprites[marker].w;
        const QString &word = words[i];
        for (int j = 0; j < word.size(); ++j) {
            const int cell = cellAt(mx + mw * j + 7, my + 10);
            if (cell < 0) {
                continue;
            }
            const int cx = sprites[cell].x;
            const int cy = sprites[cell].y;
            const int cw = sprites[cell].w;
            const int ch = sprites[cell].h;
            const int li = addSprite();
            sprites[li].setText(word.mid(j, 1), 24, kLetter);
            sprites[li].x = cx + (cw - sprites[li].w) / 2;
            sprites[li].y = cy + (ch - sprites[li].h) / 2;
            sprites[li].name = QStringLiteral("incell") + QString::number(cell);
            sprites[li].doctorOnly = true;
        }
    }
}

void CrossEngine::placeVertical(const QStringList &words, const QString &prefix) {
    for (int i = 0; i < words.size(); ++i) {
        const int marker = indexByName2(prefix + QString::number(i + 1));
        if (marker < 0) {
            continue;
        }
        const int mx = sprites[marker].x;
        const int my = sprites[marker].y;
        const int mh = sprites[marker].h;
        const QString &word = words[i];
        for (int j = 0; j < word.size(); ++j) {
            const int probeX = mx + 10;
            const int probeY = my + mh * j + 5;
            const int cell = cellAt(probeX, probeY);
            if (cell < 0) {
                continue;
            }
            if (indexByName(QStringLiteral("incell") + QString::number(cell)) >= 0) {
                continue;
            }
            const int cx = sprites[cell].x;
            const int cy = sprites[cell].y;
            const int cw = sprites[cell].w;
            const int ch = sprites[cell].h;
            const int li = addSprite();
            sprites[li].setText(word.mid(j, 1), 24, kLetter);
            sprites[li].x = cx + (cw - sprites[li].w) / 2;
            sprites[li].y = cy + (ch - sprites[li].h) / 2;
            sprites[li].name = QStringLiteral("incell") + QString::number(cell);
            sprites[li].doctorOnly = true;
        }
    }
}

void CrossEngine::addClue(const QString &name, const QString &text, int localX, int localY) {
    const int i = addSprite();
    sprites[i].word = text;
    sprites[i].pix = cluePixmap(text, false);
    sprites[i].w = sprites[i].pix.width();
    sprites[i].h = sprites[i].pix.height();
    sprites[i].isText = true;
    sprites[i].x = kClueX + localX;
    sprites[i].y = kClueY + localY;
    sprites[i].name = name;
}

void CrossEngine::addClues() {
    // В оригинале Labels qg/qv есть и у врача, и у пациента (fillTextQuestion) — клик пациента выбирает слово.
    const int h = addSprite();
    sprites[h].setBox(QStringLiteral("По горизонтали"), 360, 36, kCluePointSize, false, true);
    sprites[h].x = kClueX + 20;
    sprites[h].y = kClueY + 8;
    sprites[h].name = QStringLiteral("hor");

    int y = kClueY + sprites[h].h + 14;
    for (int i = 0; i < gClues.size() && i < 4; ++i) {
        addClue(QStringLiteral("qg") + QString::number(i + 1), gClues[i], 20, y - kClueY);
        y += sprites.last().h + 8;
    }
    y += 12;
    const int v = addSprite();
    sprites[v].setBox(QStringLiteral("По вертикали"), 340, 36, kCluePointSize, false, true);
    sprites[v].x = kClueX + 20;
    sprites[v].y = y;
    sprites[v].name = QStringLiteral("ver");
    y += sprites[v].h + 10;
    for (int i = 0; i < vClues.size() && i < 4; ++i) {
        addClue(QStringLiteral("qv") + QString::number(i + 1), vClues[i], 20, y - kClueY);
        y += sprites.last().h + 8;
    }
}

void CrossEngine::setClueSelected(int index, bool on) {
    if (index < 0 || index >= sprites.size()) {
        return;
    }
    ExSprite &s = sprites[index];
    const int x = s.x;
    const int y = s.y;
    const QString text = s.word;
    s.pix = cluePixmap(text, on);
    s.w = s.pix.width();
    s.h = s.pix.height();
    s.x = x;
    s.y = y;
}

void CrossEngine::clearWordOutline() {
    m_selectedCells.clear();
    const int oi = indexByName(QStringLiteral("wordOutline"));
    if (oi >= 0) {
        sprites[oi].visible = false;
        sprites[oi].w = 0;
        sprites[oi].h = 0;
        sprites[oi].pix = QPixmap();
    }
}

void CrossEngine::setWordOutline(const QRect &r) {
    if (r.width() < 2 || r.height() < 2) {
        clearWordOutline();
        return;
    }
    int oi = indexByName(QStringLiteral("wordOutline"));
    if (oi < 0) {
        oi = addSprite();
        sprites[oi].name = QStringLiteral("wordOutline");
    }
    ExSprite &o = sprites[oi];
    o.x = r.x();
    o.y = r.y();
    o.w = r.width();
    o.h = r.height();
    o.visible = true;
    o.isText = false;
    o.border = false;
    o.tint = kClear;
    QPixmap pm(qMax(1, r.width()), qMax(1, r.height()));
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing, true);
    QPen pen(QColor(0, 102, 255), 3);
    pen.setJoinStyle(Qt::MiterJoin);
    p.setPen(pen);
    p.setBrush(Qt::NoBrush);
    p.drawRect(QRectF(1.5, 1.5, r.width() - 3.0, r.height() - 3.0));
    p.end();
    o.pix = pm;
}

void CrossEngine::unselectAll() {
    clearWordOutline();
    for (int i = 0; i < sprites.size(); ++i) {
        ExSprite &s = sprites[i];
        if (s.name == QLatin1String("cell")) {
            s.tint = kClear;
            s.border = false;
        } else if (s.name.startsWith(QLatin1String("qg")) || s.name.startsWith(QLatin1String("qv"))) {
            setClueSelected(i, false);
        }
    }
}

QString CrossEngine::answer(const QString &code) const {
    bool ok = false;
    const int n = code.mid(1).toInt(&ok);
    if (!ok || n < 1) {
        return {};
    }
    const QStringList &list = code.startsWith(QLatin1Char('g')) ? gQuest : vQuest;
    if (n > list.size()) {
        return {};
    }
    return list.at(n - 1);
}

bool CrossEngine::cellInSelection(int cell) const {
    return m_selectedCells.contains(cell);
}

void CrossEngine::paintWord(const QString &code, bool on) {
    const int marker = indexByName2(code);
    const QString word = answer(code);
    if (marker < 0 || word.isEmpty()) {
        return;
    }
    if (!on) {
        clearWordOutline();
        return;
    }
    const bool horiz = code.startsWith(QLatin1Char('g'));
    int x = sprites[marker].x + 10;
    int y = sprites[marker].y + 10;
    m_selectedCells.clear();
    QRect bounds;
    for (int i = 0; i < word.size(); ++i) {
        const int cell = cellAt(x, y);
        if (cell < 0) {
            break;
        }
        m_selectedCells.append(cell);
        const QRect cr(sprites[cell].x, sprites[cell].y, sprites[cell].w, sprites[cell].h);
        bounds = bounds.isNull() ? cr : bounds.united(cr);
        if (horiz) {
            x += sprites[cell].w;
        } else {
            y += sprites[cell].h;
        }
    }
    setWordOutline(bounds.adjusted(-1, -1, 1, 1));
}

void CrossEngine::selectQuestion(const QString &code) {
    if (!(code.startsWith(QLatin1Char('g')) || code.startsWith(QLatin1Char('v')))) {
        return;
    }
    const QString spriteName = QLatin1Char('q') + code;
    if (indexByName(spriteName) < 0 || answer(code).isEmpty()) {
        return;
    }
    // Уже полностью открытое слово снова не выбираем.
    if (m_wordState.value(code, WordState::Untouched) == WordState::Opened) {
        return;
    }
    unselectAll();
    paintWord(code, true);
    setClueSelected(indexByName(spriteName), true);
    userNowQuest = code;
    emit changed();
}

bool CrossEngine::isLetterOpened(int cell) const {
    const int letter = indexByName(QStringLiteral("incell") + QString::number(cell));
    // Буква «открыта» пациенту, когда снято doctorOnly.
    return letter >= 0 && !sprites[letter].doctorOnly;
}

void CrossEngine::revealCell(int cell) {
    const int letter = indexByName(QStringLiteral("incell") + QString::number(cell));
    if (letter < 0) {
        return;
    }
    const QString ch = sprites[letter].word;
    const int x = sprites[letter].x;
    const int y = sprites[letter].y;
    sprites[letter].setText(ch, 24, Qt::black);
    sprites[letter].x = x;
    sprites[letter].y = y;
    sprites[letter].doctorOnly = false;
    sprites[letter].visible = true;
}

void CrossEngine::markLetterOpened(int cell) {
    if (userNowQuest.isEmpty()) {
        return;
    }
    if (m_wordState.value(userNowQuest, WordState::Untouched) == WordState::Opened) {
        return;
    }
    if (isLetterOpened(cell)) {
        return;
    }
    revealCell(cell);
    m_anyProgress = true;
    // Первая подсказка-буква по слову: Выполнено +1, Верно не меняется.
    if (m_wordState.value(userNowQuest, WordState::Untouched) == WordState::Untouched) {
        m_wordState[userNowQuest] = WordState::Hinted;
        emit resultRecorded(true);
    }
}

void CrossEngine::openWord(const QString &quest) {
    const int marker = indexByName2(quest);
    const QString word = answer(quest);
    if (marker < 0 || word.isEmpty()) {
        return;
    }
    const WordState prev = m_wordState.value(quest, WordState::Untouched);
    if (prev == WordState::Opened) {
        return;
    }
    const bool horiz = quest.startsWith(QLatin1Char('g'));
    int x = sprites[marker].x + 10;
    int y = sprites[marker].y + 10;
    for (int i = 0; i < word.size(); ++i) {
        const int cell = cellAt(x, y);
        if (cell < 0) {
            break;
        }
        revealCell(cell);
        if (horiz) {
            x += sprites[cell].w;
        } else {
            y += sprites[cell].h;
        }
    }
    m_anyProgress = true;
    m_wordState[quest] = WordState::Opened;
    // Без подсказок — верно; если уже были буквы-подсказки — только открываем, счётчик Верно не трогаем.
    if (prev == WordState::Untouched) {
        emit resultRecorded(false);
    }
    unselectAll();
    userNowQuest.clear();
}

QStringList CrossEngine::allQuestCodes() const {
    QStringList codes;
    for (int i = 0; i < gQuest.size(); ++i) {
        codes.append(QLatin1Char('g') + QString::number(i + 1));
    }
    for (int i = 0; i < vQuest.size(); ++i) {
        codes.append(QLatin1Char('v') + QString::number(i + 1));
    }
    return codes;
}

void CrossEngine::settleBeforeNext() {
    // Если в кроссворде ничего не открывали — при смене задания штрафов нет.
    if (!m_anyProgress) {
        return;
    }
    for (const QString &code : allQuestCodes()) {
        if (answer(code).isEmpty()) {
            continue;
        }
        if (m_wordState.value(code, WordState::Untouched) != WordState::Opened) {
            m_wordState[code] = WordState::Opened;
            emit resultRecorded(true); // неоткрытое слово = выполнено неверно
        }
    }
}

int CrossEngine::cellAt(int px, int py) const {
    for (int i = 0; i < sprites.size(); ++i) {
        const ExSprite &s = sprites[i];
        if (s.name == QLatin1String("cell") && s.x < px && s.x + s.w > px && s.y < py && s.y + s.h > py) {
            return i;
        }
    }
    return -1;
}

bool CrossEngine::trySelectClueAt(const QPoint &p) {
    const int i = topAt(p, true);
    if (i < 0) {
        return false;
    }
    const QString &name = sprites[i].name;
    if (name.startsWith(QLatin1String("qg")) || name.startsWith(QLatin1String("qv"))) {
        selectQuestion(name.mid(1));
        return true;
    }
    return false;
}

void CrossEngine::patientRelease(const QPoint &p) {
    ExEngine::patientRelease(p);
    trySelectClueAt(p);
}

void CrossEngine::doctorClick(const QPoint &p, Qt::MouseButton button) {
    // Выбор вопроса — ЛКМ по строке задания (и у специалиста, и у пациента).
    if (button == Qt::LeftButton && trySelectClueAt(p)) {
        return;
    }
    const int cell = cellAt(p.x(), p.y());
    // Открытие буквы/слова только если слово выбрано (клетка входит в обведённую область).
    if (cell < 0 || userNowQuest.isEmpty() || !cellInSelection(cell)) {
        return;
    }
    if (button == Qt::RightButton) {
        openWord(userNowQuest);
        emit changed();
        return;
    }
    if (button == Qt::LeftButton) {
        markLetterOpened(cell);
        emit changed();
    }
}
