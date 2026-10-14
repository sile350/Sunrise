#include "coverpanel.h"

#include "engines/exengine.h"
#include "imagebutton.h"

#include <QCheckBox>
#include <QComboBox>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QPixmap>
#include <QSlider>
#include <QTextEdit>

namespace {

QFont msFont(double pt, bool bold = false) {
    QFont f(QStringLiteral("Microsoft Sans Serif"));
    f.setStyleHint(QFont::SansSerif);
    f.setPointSizeF(pt);
    f.setBold(bold);
    return f;
}

QSize fitSize(const QString &file, int w, int h) {
    const QPixmap pm(file);
    if (pm.isNull()) {
        return QSize(w, h);
    }
    QSize s = pm.size();
    s.scale(w, h, Qt::KeepAspectRatio);
    return s;
}

} // namespace

CoverPanel::CoverPanel(ExCoverKind kind, QWidget *parent) : QWidget(parent), m_kind(kind) {
    setAutoFillBackground(true);
    QPalette pal = palette();
    pal.setColor(QPalette::Window, QColor(249, 249, 249));
    setPalette(pal);
    setStyleSheet(QStringLiteral("QLabel { color: #000000; }"));
    mirror = new ExCanvas(false, this);
    buildFor(kind);
}

// В обложках оригинала кнопки — PictureBox с SizeMode AutoSize: картинка в натуральную величину.
ImageButton *CoverPanel::makeButton(const QString &sysImage, int x, int y) {
    const QString path = ExAssets::sysImage(sysImage);
    auto *b = new ImageButton(this);
    b->setImagePath(path);
    const QPixmap pm(path);
    b->setGeometry(x, y, pm.isNull() ? 100 : pm.width(), pm.isNull() ? 24 : pm.height());
    b->setCursor(Qt::PointingHandCursor);
    return b;
}

ImageButton *CoverPanel::makeExnButton(const QString &path, int x, int y, int w, int h) {
    auto *b = new ImageButton(this);
    b->setImagePath(path);
    const QSize s = fitSize(path, w, h);
    b->setGeometry(x + (w - s.width()) / 2, y + (h - s.height()) / 2, s.width(), s.height());
    b->setCursor(Qt::PointingHandCursor);
    return b;
}

QComboBox *CoverPanel::makeCombo(const QStringList &items, int x, int y, int w, int h, double pt) {
    auto *c = new QComboBox(this);
    c->setFont(msFont(pt));
    c->addItems(items);
    // Размер из дизайнера, но текст пункта не обрезается (шрифт может отличаться от Windows).
    c->setGeometry(x, y, qMax(w, c->sizeHint().width()), h);
    return c;
}

QLabel *CoverPanel::makeLabel(const QString &text, int x, int y, bool bold, double pt) {
    auto *l = new QLabel(text, this);
    l->setFont(msFont(pt, bold));
    l->adjustSize();
    l->move(x, y);
    return l;
}

void CoverPanel::addCounters(int x1, int y1, int x2, int y2, bool withColon) {
    lcounter = makeLabel(withColon ? QStringLiteral("Выполнено: 0") : QStringLiteral("Выполнено"), x1, y1, false, 12);
    lcorrect = makeLabel(withColon ? QStringLiteral("Верно: 0") : QStringLiteral("Верно"), x2, y2, false, 12);
}

void CoverPanel::selectConjunction(int index) {
    for (int k = 0; k < 5; ++k) {
        if (conj[k]) {
            conj[k]->setImagePath(ExAssets::exnPath(QStringLiteral("2.1.4/%1%2.png").arg(k + 1).arg(k == index ? QStringLiteral("on") : QString())));
        }
    }
    const bool causal = index < 3;
    if (raz1) {
        raz1->setPixmap(QPixmap(ExAssets::exnPath(causal ? QStringLiteral("2.1.4/ron.png") : QStringLiteral("2.1.4/r.png"))));
    }
    if (raz2) {
        raz2->setPixmap(QPixmap(ExAssets::exnPath(causal ? QStringLiteral("2.1.4/rr.png") : QStringLiteral("2.1.4/rron.png"))));
    }
}

void CoverPanel::setMirrorGeometry(const QRect &r) {
    mirror->setGeometry(r);
}

void CoverPanel::setCounters(int done, int correct) {
    if (lcounter) {
        lcounter->setText(QStringLiteral("Выполнено: ") + QString::number(done));
        lcounter->adjustSize();
    }
    if (lcorrect) {
        lcorrect->setText(QStringLiteral("Верно: ") + QString::number(correct));
        lcorrect->adjustSize();
    }
}

void CoverPanel::buildFor(ExCoverKind kind) {
    // В Load всех обложек: stopu.Top = beginu.Top — «Стоп» встаёт на место «Начать упражнение».
    auto begin = [&](int x, int y) {
        beginu = makeButton(QStringLiteral("beginu.png"), x, y);
        stopu = makeButton(QStringLiteral("stop.png"), x, y);
        stopu->hide();
    };
    auto next = [&](int x, int y) { nextz = makeButton(QStringLiteral("next.png"), x, y); };
    // bagain во всех дизайнерах Visible = false: показывается по «Начать» (см. ExerciseHost::updateVisibility).
    auto again = [&](int x, int y) {
        bagain = makeButton(QStringLiteral("again.png"), x, y);
        bagain->hide();
    };
    auto rightWrong = [&](int x1, int y1, int x2, int y2) {
        dcorrect = makeButton(QStringLiteral("right.png"), x1, y1);
        dincorrect = makeButton(QStringLiteral("unright.png"), x2, y2);
    };

    switch (kind) {
    case ExCoverKind::C1: {
        begin(21, 11);
        rtf = new QTextEdit(this);
        rtf->setGeometry(21, 57, 988, 890);
        QFont f = rtf->font();
        f.setPointSize(12);
        rtf->setFont(f);
        underlineBtn = makeExnButton(ExAssets::resourceImage(QStringLiteral("Подчеркивание.png")), 929, 11, 36, 34);
        boldBtn = makeExnButton(ExAssets::resourceImage(QStringLiteral("Жирный шрифт.png")), 971, 11, 36, 34);
        break;
    }
    case ExCoverKind::C112: {
        begin(3, 17);
        label1 = makeLabel(QStringLiteral("Уровень сложности"), 228, 1);
        combo1 = makeCombo({QStringLiteral("1"), QStringLiteral("2"), QStringLiteral("3")}, 240, 17, 75, 21);
        again(560, 17);
        next(774, 17);
        modePic = makeButton(QStringLiteral("ppod.png"), 3, 57);
        addCounters(319, 65, 455, 65, true);
        modeLabel = makeLabel(QStringLiteral("Покажите сюжетные картинки"), 251, 99, false, 16);
        break;
    }
    case ExCoverKind::C114: {
        combo1 = makeCombo({QStringLiteral("1"), QStringLiteral("2")}, 240, 18, 70, 21);
        next(737, 12);
        begin(0, 12);
        hintLabel = makeLabel(QString(), 40, 55, true, 10);
        hintLabel->setWordWrap(true);
        hintLabel->setFixedWidth(940);
        hintLabel->setFixedHeight(40);
        // panel1 (3,95) в режиме 2 поднимается на Top = 50: координаты детей + (3, 50)
        rtf = new QTextEdit(this);
        rtf->setGeometry(6, 104, 939, 620);
        QFont f = rtf->font();
        f.setPointSize(12);
        rtf->setFont(f);
        question = new QLineEdit(this);
        question->setFont(msFont(14));
        question->setGeometry(6, 743, 947, 29);
        question->setPlaceholderText(QStringLiteral("Написать вопрос"));
        showQuestion = makeExnButton(ExAssets::exnPath(QStringLiteral("1.1.4/showp.png")), 385, 788, 274, 34);
        underlineBtn = makeExnButton(ExAssets::resourceImage(QStringLiteral("Подчеркивание.png")), 867, 64, 36, 34);
        boldBtn = makeExnButton(ExAssets::resourceImage(QStringLiteral("Жирный шрифт.png")), 909, 64, 36, 34);
        break;
    }
    case ExCoverKind::C121: {
        begin(3, 11);
        label1 = makeLabel(QStringLiteral("Функция"), 138, 68, true, 10);
        combo1 = makeCombo({QStringLiteral("Продолжение ряда"), QStringLiteral("Воспроизведение ряда")}, 107, 88, 148, 21);
        combo1->setCurrentIndex(1); // оригинал _121.initEx: «Воспроизведение ряда»
        label2 = makeLabel(QStringLiteral("Направленность"), 293, 68, true, 10);
        combo2 = makeCombo({QStringLiteral("Прямая"), QStringLiteral("Обратная")}, 298, 88, 121, 21);
        label3 = makeLabel(QStringLiteral("Стимульный материал"), 462, 67, true, 10);
        combo3 = makeCombo({QStringLiteral("Числа"), QStringLiteral("Дни недели"), QStringLiteral("Месяцы"), QStringLiteral("Цвета радуги")}, 462, 88, 173, 21);
        label4 = makeLabel(QStringLiteral("Сложность"), 663, 68, true, 10);
        combo4 = makeCombo({QStringLiteral("1"), QStringLiteral("2"), QStringLiteral("3")}, 676, 88, 67, 21);
        // панель метронома (видна при «Воспроизведение ряда»)
        metroPanel = new QWidget(this);
        // _121.Designer: panel1 (204,140) 370x180; pictureBox1 (88,48) 44x41; trackBar1 (136,60) 104x45 [1..8]
        metroPanel->setGeometry(204, 140, 370, 180);
        metroPanel->setFont(msFont(8.25));
        auto *cap = new QLabel(QStringLiteral("Скорость метронома"), metroPanel);
        cap->move(84, 15);
        metroBtn = new ImageButton(metroPanel);
        metroBtn->setImagePath(ExAssets::exnPath(QStringLiteral("1.2.1/metro.png")));
        metroBtn->setGeometry(88, 48, 44, 41);
        metroBtn->setCursor(Qt::PointingHandCursor);
        auto *mlab = new QLabel(QStringLiteral("Метроном"), metroPanel);
        mlab->setFont(msFont(10, true));
        mlab->move(3, 60);
        auto *minL = new QLabel(QStringLiteral("min"), metroPanel);
        minL->move(128, 84);
        auto *maxL = new QLabel(QStringLiteral("max"), metroPanel);
        maxL->move(228, 84);
        metroSlider = new QSlider(Qt::Horizontal, metroPanel);
        metroSlider->setRange(1, 8);
        metroSlider->setValue(1);
        metroSlider->setGeometry(136, 60, 104, 24);
        metroPanel->hide();
        showp = makeButton(QStringLiteral("showp.png"), 225, 326);
        updateR = makeButton(QStringLiteral("updater.png"), 449, 326);
        showp->hide();
        break;
    }
    case ExCoverKind::C122: {
        // Сдвинуты вправо относительно оригинала (218/364), чтобы подписи не наезжали друг на друга.
        label1 = makeLabel(QStringLiteral("Выполнение"), 250, 10, true, 10);
        combo1 = makeCombo({QStringLiteral("Устно"), QStringLiteral("Письменно с опорой"), QStringLiteral("Письменно без опоры")}, 238, 30, 130, 21);
        label2 = makeLabel(QStringLiteral("Стимульный материал"), 430, 10, true, 10);
        combo2 = makeCombo({QStringLiteral("Пословицы"), QStringLiteral("Ожидаемые слова"), QStringLiteral("Устойчивые выражения")}, 427, 30, 188, 21);
        begin(6, 28);
        next(781, 27);
        showp = makeButton(QStringLiteral("showp.png"), 781, 71);
        again(613, 27);
        rightWrong(290, 102, 467, 102);
        addCounters(278, 157, 494, 157, false);
        hintLabel = makeLabel(QString(), 20, 215, true, 11);
        break;
    }
    case ExCoverKind::C123: {
        label1 = makeLabel(QStringLiteral("Выполнение"), 256, 0, true, 10);
        combo1 = makeCombo({QStringLiteral("Запоминание"), QStringLiteral("Устно"), QStringLiteral("С перемещением")}, 244, 19, 121, 23, 9);
        combo2 = makeCombo({QStringLiteral("1"), QStringLiteral("2"), QStringLiteral("3")}, 390, 19, 39, 24, 10);
        begin(18, 16);
        next(770, 16);
        auto *cb = new QCheckBox(QStringLiteral("Показать на экране пользователя"), this);
        cb->setFont(msFont(8.25));
        cb->move(446, 22);
        cb->adjustSize();
        cb->setChecked(true);
        showCheck = cb;
        // GroupBox в WinForms непрозрачен и перекрывает зеркало (mp.Top = 20).
        auto *box = new QGroupBox(QStringLiteral("Данные пациента"), this);
        box->setFont(msFont(12));
        box->setGeometry(175, 219, 627, 431);
        box->setAutoFillBackground(true);
        dataBox = box;
        const QFont f14 = msFont(14);
        fioLabel = new QLabel(QStringLiteral("ФИО: "), box);
        fioLabel->setFont(f14);
        fioLabel->setGeometry(61, 70, 540, 26);
        auto *la = new QLabel(QStringLiteral("Адрес:"), box);
        la->setFont(f14);
        la->move(48, 120);
        address = new QLineEdit(box);
        address->setFont(f14);
        address->setGeometry(149, 115, 444, 29);
        address->setPlaceholderText(QStringLiteral("Введите адрес пациента"));
        auto *lt = new QLabel(QStringLiteral("Телефон:"), box);
        lt->setFont(f14);
        lt->move(48, 165);
        phone = new QLineEdit(box);
        phone->setFont(f14);
        phone->setGeometry(149, 160, 306, 29);
        phone->setPlaceholderText(QStringLiteral("Введите телефон пациента"));
        break;
    }
    case ExCoverKind::C211:
        begin(21, 9);
        next(750, 9);
        again(573, 9);
        break;
    case ExCoverKind::C212:
        label1 = makeLabel(QStringLiteral("Стимульный материал"), 260, 15, true, 10);
        combo1 = makeCombo({QStringLiteral("Глагол и существительное"), QStringLiteral("Два существительных"), QStringLiteral("Смешанные")}, 250, 35, 213, 24, 10);
        begin(9, 15);
        next(745, 15);
        break;
    case ExCoverKind::C213:
        begin(3, 9);
        next(772, 9);
        break;
    case ExCoverKind::C214: {
        begin(3, 3);
        next(766, 3);
        // panel1 (326,112): label1 14pt, raz1/raz2, f1..f5 (191x32)
        label1 = makeLabel(QStringLiteral("Выбор союза"), 326 + 134, 112 + 7, false, 14);
        raz1 = new QLabel(this);
        raz1->setGeometry(326 + 23, 112 + 42, 171, 95);
        raz1->setScaledContents(true);
        raz2 = new QLabel(this);
        raz2->setGeometry(326 + 23, 112 + 137, 171, 64);
        raz2->setScaledContents(true);
        const int xs[5] = {195, 195, 195, 194, 194};
        const int ys[5] = {42, 74, 105, 137, 168};
        for (int i = 0; i < 5; ++i) {
            conj[i] = makeExnButton(ExAssets::exnPath(QStringLiteral("2.1.4/%1.png").arg(i + 1)), 326 + xs[i], 112 + ys[i], 191, 32);
        }
        selectConjunction(0); // _214.initEx: f1_Click
        rightWrong(380, 21, 557, 21);
        addCounters(366, 65, 557, 65, true);
        showp = makeButton(QStringLiteral("showp.png"), 766, 43);
        break;
    }
    case ExCoverKind::C221: {
        QStringList items = {
            QStringLiteral("1 Навязчивый гость"), QStringLiteral("2 Граммпластинка"), QStringLiteral("3 Мечта рыбака"),
            QStringLiteral("4 Вдохновение"), QStringLiteral("5 Недоразумение"), QStringLiteral("6 Дрессировка"),
            QStringLiteral("7 Знакомство"), QStringLiteral("8 Читатели одной книги"), QStringLiteral("9 Карикатурист"),
            QStringLiteral("10 Лыжник"), QStringLiteral("11 Муха"), QStringLiteral("12 Шляпа"),
            QStringLiteral("13 Семейная жизнь"), QStringLiteral("14 Кресло"), QStringLiteral("15 Картина"),
            QStringLiteral("16 Влюбленность"), QStringLiteral("17 Табуретка"), QStringLiteral("18 Находчивый папа"),
            QStringLiteral("19 На круги своя"), QStringLiteral("20 Почтальон")};
        combo1 = makeCombo(items, 351, 17, 215, 28, 12);
        begin(21, 12);
        next(774, 14);
        break;
    }
    case ExCoverKind::C222: {
        QStringList items = {
            QStringLiteral("1. Обмен любезностями"), QStringLiteral("2. Точный портрет"), QStringLiteral("3. Знание - сила"),
            QStringLiteral("4. Гениальный трубочист"), QStringLiteral("5. Вовремя сделать паузу"),
            QStringLiteral("6. Настоящая английская игра"), QStringLiteral("7. Главная причина"),
            QStringLiteral("8. Что проще изменить?"), QStringLiteral("9. Играй, музыкант..."),
            QStringLiteral("10. Справедливая цена"), QStringLiteral("11. Нашла коса на камень"),
            QStringLiteral("12. Попасть в историю"), QStringLiteral("13. Любите молодежь"),
            QStringLiteral("14. Бетховен и официант"), QStringLiteral("15. Запятые - это пустяк"),
            QStringLiteral("16. На собственных ногах"), QStringLiteral("17. Присутствие духа"),
            QStringLiteral("18. Портрет Тамерлана"), QStringLiteral("19. Знаток почерков")};
        combo1 = makeCombo(items, 350, 11, 242, 28, 12);
        begin(3, 11);
        next(777, 15);
        showStory = makeExnButton(ExAssets::exnPath(QStringLiteral("2.2.2/Показать на экране пациента.png")), 697, 65, 274, 34);
        break;
    }
    case ExCoverKind::C223:
        begin(13, 8);
        next(764, 8);
        again(764, 91);
        // dcorrect/dincorrect в 223.Designer без картинок и с пустыми обработчиками — на обложке их не видно
        addCounters(376, 93, 592, 93, false);
        break;
    case ExCoverKind::C224:
        begin(8, 4);
        next(765, 4);
        again(765, 74);
        addCounters(296, 74, 512, 74, false);
        break;
    case ExCoverKind::C225: {
        begin(7, 11);
        next(760, 11);
        combo1 = makeCombo({QStringLiteral("Текстовые задания"), QStringLiteral("Графические задания"), QStringLiteral("Головоломка")}, 250, 14, 180, 24, 10);
        again(760, 76);
        explainBtn = makeButton(QStringLiteral("exp.png"), 492, 11);
        explainBtn->hide();
        addCounters(330, 77, 466, 77, true);
        // ltext / lob — Microsoft Sans Serif 13pt, label2 «Объяснение:» — 12pt Bold
        taskLabel = makeLabel(QString(), 10, 110, false, 13);
        taskLabel->setWordWrap(true);
        taskLabel->setFixedSize(1020, 90);
        taskLabel->setAlignment(Qt::AlignLeft | Qt::AlignTop);
        explainCaption = makeLabel(QStringLiteral("Объяснение:"), 10, 790, true, 12);
        explainLabel = makeLabel(QString(), 10, 818, false, 13);
        explainLabel->setWordWrap(true);
        explainLabel->setFixedSize(1020, 100);
        explainLabel->setAlignment(Qt::AlignLeft | Qt::AlignTop);
        break;
    }
    case ExCoverKind::C226: {
        combo1 = makeCombo({QStringLiteral("1"), QStringLiteral("2"), QStringLiteral("3")}, 397, 12, 62, 21);
        begin(4, 9);
        next(754, 10);
        again(754, 63);
        addCounters(304, 51, 520, 51, false);
        // _226: rt (40,100) 800x850 — about.rtf в режиме «3»
        rtf = new QTextEdit(this);
        rtf->setGeometry(40, 100, 800, 850);
        rtf->hide();
        break;
    }
    case ExCoverKind::C311:
        begin(3, 16);
        next(724, 15);
        again(724, 83);
        addCounters(352, 58, 568, 58, false);
        break;
    case ExCoverKind::C312:
        // В оригинале латинская «C» («C опорой»), как в _312.Designer.
        combo1 = makeCombo({QStringLiteral("C опорой"), QStringLiteral("Без опоры")}, 408, 14, 136, 24, 10);
        begin(10, 11);
        next(766, 11);
        again(766, 71);
        addCounters(316, 73, 532, 73, false);
        break;
    case ExCoverKind::C323:
        label1 = makeLabel(QStringLiteral("Задание"), 246, 8, true, 10);
        label2 = makeLabel(QStringLiteral("Часть речи"), 386, 8, true, 10);
        combo1 = makeCombo({QStringLiteral("Синонимы"), QStringLiteral("Антонимы")}, 234, 28, 121, 24, 10);
        combo2 = makeCombo({QStringLiteral("Существительные"), QStringLiteral("Глаголы"), QStringLiteral("Прилагательные"),
                            QStringLiteral("Наречия"), QStringLiteral("Смешанные")}, 380, 28, 143, 24, 10);
        begin(3, 13);
        next(753, 13);
        again(753, 152);
        showp = makeButton(QStringLiteral("showp.png"), 753, 76);
        addCounters(354, 76, 570, 76, false);
        rightWrong(327, 120, 570, 120);
        break;
    case ExCoverKind::C324:
        combo1 = makeCombo({QStringLiteral("Устно"), QStringLiteral("Письменно")}, 272, 19, 121, 24, 10);
        begin(32, 16);
        next(783, 16);
        again(783, 141);
        showp = makeButton(QStringLiteral("showp.png"), 783, 73);
        addCounters(359, 80, 575, 80, false);
        rightWrong(342, 114, 585, 114);
        break;
    case ExCoverKind::None:
        break;
    }
}
