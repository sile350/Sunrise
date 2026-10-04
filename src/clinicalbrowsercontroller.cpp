#include "clinicalbrowsercontroller.h"
#include "custommessagebox.h"
#include "rtfconverter.h"

#ifdef Q_OS_WIN
#include <ActiveQt/QAxObject>
#include <ActiveQt/QAxWidget>
#include <qt_windows.h>
#elif defined(SUNRISE_USE_WEBKIT)
#include <QWebFrame>
#include <QWebPage>
#include <QWebSettings>
#include <QWebView>
#else
#include <QEventLoop>
#include <QWebChannel>
#include <QWebEnginePage>
#include <QWebEngineSettings>
#include <QWebEngineView>
#endif
#include <QApplication>
#include <QColor>
#include <QDate>
#include <QDir>
#include <QEvent>
#include <QFile>
#include <QFileInfo>
#include <QHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QPixmap>
#include <QPointer>
#include <QPrintDialog>
#include <QPrinter>
#include <QRegularExpression>
#include <QSaveFile>
#include <QTextDocument>
#include <QTimer>
#include <QUrl>

namespace {

const QString kBrowserClsid = QStringLiteral("{8856F961-340A-11D0-A96B-00C04FD705A2}");
constexpr int kBrowserHeight = 866;

QString today() {
    return QDate::currentDate().toString(QStringLiteral("dd.MM.yyyy"));
}

QString normalizeText(QString text) {
    text.replace(QStringLiteral("\r\n"), QStringLiteral("\n"));
    text.replace(QLatin1Char('\r'), QLatin1Char('\n'));
    text.replace(QChar(0x00A0), QLatin1Char(' '));
    return text;
}

QString htmlToPlain(QString html) {
    html.replace(QRegularExpression(QStringLiteral("<br\\s*/?>"), QRegularExpression::CaseInsensitiveOption),
                 QStringLiteral("\n"));
    html.remove(QRegularExpression(QStringLiteral("<[^>]*>")));
    html.replace(QStringLiteral("&nbsp;"), QStringLiteral(" "));
    html.replace(QStringLiteral("&lt;"), QStringLiteral("<"));
    html.replace(QStringLiteral("&gt;"), QStringLiteral(">"));
    html.replace(QStringLiteral("&quot;"), QStringLiteral("\""));
    html.replace(QStringLiteral("&amp;"), QStringLiteral("&"));
    return normalizeText(html).trimmed();
}

QString pageFileName(const QString &url) {
    QString path = url;
    if (path.startsWith(QStringLiteral("file:"), Qt::CaseInsensitive)) {
        path = QUrl(path).toLocalFile();
    }
    path.replace(QLatin1Char('\\'), QLatin1Char('/'));
    return path.section(QLatin1Char('/'), -1).toLower();
}

struct TemplateRule {
    SummaryPanel::Field field;
    QString head;
    QString headId;
    QString bodyId;
    bool sameLine = false;
};

const QHash<QString, TemplateRule> &templateRules() {
    using F = SummaryPanel::Field;
    static const QHash<QString, TemplateRule> rules = {
        {QStringLiteral("tmp11"), {F::Mp, QStringLiteral("Различные классы антигипертензивных средств"), {}, QStringLiteral("idText1")}},
        {QStringLiteral("tmp12"), {F::Mp, QStringLiteral("Диуретики, комбинации диуретика и ингибитора ангиотензипревращающего фермента"), {}, QStringLiteral("idText2")}},
        {QStringLiteral("tmp13"), {F::Mp, QStringLiteral("Эпросартан"), {}, QStringLiteral("idText3")}},
        {QStringLiteral("tmp14"), {F::Mp, QStringLiteral("Нитрендипин"), {}, QStringLiteral("idText4")}},
        {QStringLiteral("tmp15"), {F::Mp, QStringLiteral("Тиазидоподобный диуретик индапамид"), {}, QStringLiteral("idText5")}},
        {QStringLiteral("tmp16"), {F::Mp, QStringLiteral("Комбинация ингибитора АПФ периндоприла с индапамидом"), {}, QStringLiteral("idText6")}},
        {QStringLiteral("tmp17"), {F::Mp, QStringLiteral("Ингибитор АПФ рамиприл"), {}, QStringLiteral("idText7")}},
        {QStringLiteral("tmp18"), {F::Mp, QStringLiteral("Кандесартан"), {}, QStringLiteral("idText8")}},
        {QStringLiteral("tmp19"), {F::Mp, {}, QStringLiteral("idText9"), QStringLiteral("idText10")}},
        {QStringLiteral("tmp21"), {F::Mp, QStringLiteral("Ацетилсалициловая кислота (аспирин)"), {}, QStringLiteral("idText1")}},
        {QStringLiteral("tmp22"), {F::Mp, QStringLiteral("Тиклопидин"), {}, QStringLiteral("idText2")}},
        {QStringLiteral("tmp23"), {F::Mp, QStringLiteral("Клопидогрел"), {}, QStringLiteral("idText3")}},
        {QStringLiteral("tmp24"), {F::Mp, QStringLiteral("Комбинация клопидогрела с ацетилсалициловой кислотой"), {}, QStringLiteral("idText4")}},
        {QStringLiteral("tmp25"), {F::Mp, QStringLiteral("Комбинация клопидогрела и 100 мг аспирина"), {}, QStringLiteral("idText5")}},
        {QStringLiteral("tmp26"), {F::Mp, QStringLiteral("Дипиридамол"), {}, QStringLiteral("idText6")}},
        {QStringLiteral("tmp27"), {F::Mp, QStringLiteral("Трифлюзал"), {}, QStringLiteral("idText7")}},
        {QStringLiteral("tmp28"), {F::Mp, QStringLiteral("Комбинация дипиридамола замедленного высвобождения и аспирина"), {}, QStringLiteral("idText8")}},
        {QStringLiteral("tmp29"), {F::Mp, QStringLiteral("Аспирин или клопидогрел"), {}, QStringLiteral("idText9")}},
        {QStringLiteral("tmp210"), {F::Mp, {}, QStringLiteral("idText10"), QStringLiteral("idText11")}},
        {QStringLiteral("tmp31"), {F::Mp, QStringLiteral("Варфарин"), {}, QStringLiteral("idText1")}},
        {QStringLiteral("tmp32"), {F::Mp, QStringLiteral("Дабигатран"), {}, QStringLiteral("idText2")}},
        {QStringLiteral("tmp33"), {F::Mp, QStringLiteral("Ривароксабан"), {}, QStringLiteral("idText3")}},
        {QStringLiteral("tmp34"), {F::Mp, QStringLiteral("Апиксабан"), {}, QStringLiteral("idText4")}},
        {QStringLiteral("tmp41"), {F::Mp, QStringLiteral("Статины"), {}, QStringLiteral("idText1")}},
        {QStringLiteral("tmp42"), {F::Mp, QStringLiteral("Аторвастатин"), {}, QStringLiteral("idText2")}},
        {QStringLiteral("tmp43"), {F::Mp, {}, QStringLiteral("idText3"), QStringLiteral("idText4")}},
        {QStringLiteral("tmp44"), {F::Mp, {}, QStringLiteral("idText5"), QStringLiteral("idText6")}},
        {QStringLiteral("idNazD1"), {F::Dih, QStringLiteral("Дыхательная гимнастика"), {}, QStringLiteral("idText11")}},
        {QStringLiteral("idNazD2"), {F::Dih, QStringLiteral("Активизация мускулатуры шеи и мышц гортани."), {}, QStringLiteral("idText12")}},
        {QStringLiteral("idNazD3"), {F::Dih, {}, QStringLiteral("idText13"), QStringLiteral("idText14")}},
        {QStringLiteral("idNazD21"), {F::Dih, QStringLiteral("Активизация мимической мускулатуры."), {}, QStringLiteral("idText1")}},
        {QStringLiteral("idNazD22"), {F::Dih, QStringLiteral("Упражнения для мышц нижней челюсти."), {}, QStringLiteral("idText2")}},
        {QStringLiteral("idNazD23"), {F::Dih, QStringLiteral("Артикуляционные упражнения."), {}, QStringLiteral("idText3")}},
        {QStringLiteral("idNazD24"), {F::Dih, {}, QStringLiteral("idText4"), QStringLiteral("idText5")}},
        {QStringLiteral("idNazD31"), {F::Dih, QStringLiteral("Артикуляционная гимнастика"), {}, QStringLiteral("idText1")}},
        {QStringLiteral("idNazD32"), {F::Dih, {}, QStringLiteral("idText2"), QStringLiteral("idText3")}},
        {QStringLiteral("idNazD33"), {F::Dih, {}, QStringLiteral("idText4"), QStringLiteral("idText5")}},
        {QStringLiteral("idNazD41"), {F::Dih, QStringLiteral("Упражнения для восстановления глотания"), {}, QStringLiteral("idText1")}},
        {QStringLiteral("idNazD42"), {F::Dih, {}, QStringLiteral("idText2"), QStringLiteral("idText3")}},
        {QStringLiteral("idNazD43"), {F::Dih, {}, QStringLiteral("idText4"), QStringLiteral("idText5")}},
        {QStringLiteral("idNaz21"), {F::Corr, QStringLiteral("Отказ от злоупотребления алкоголем."), {}, QStringLiteral("idText1"), true}},
        {QStringLiteral("idNaz31"), {F::Corr, QStringLiteral("Соблюдение диеты с ограничением поваренной соли и ненасыщенных жиров, обогащенной богатыми клетчаткой фруктами и овощами."), {}, QStringLiteral("idText1"), true}},
        {QStringLiteral("idNaz41"), {F::Corr, QStringLiteral("Снижение веса."), {}, QStringLiteral("idText1"), true}},
        {QStringLiteral("idNaz51"), {F::Corr, QStringLiteral("Физическая активность (ФА)."), {}, QStringLiteral("idText1"), true}},
    };
    return rules;
}

const QHash<QString, QString> &speechTitles() {
    static const QHash<QString, QString> titles = {
        {QStringLiteral("idz1"), QStringLiteral("Восстановление речи для пациентов с грубой сенсомоторной афазией")},
        {QStringLiteral("idz11"), QStringLiteral("Затормаживание обильной непродуктивной речи и привлечение слухового внимания")},
        {QStringLiteral("idz12"), QStringLiteral("Привлечение слухового внимания и растормаживание речи")},
        {QStringLiteral("idz2"), QStringLiteral("Восстановления речи для пациентов с динамической афазией")},
        {QStringLiteral("idz21"), QStringLiteral("Преодоление ошибок грамматического структурирования")},
        {QStringLiteral("idz22"), QStringLiteral("Преодоление трудностей планирования и программирования речевого высказывания")},
        {QStringLiteral("idz3"), QStringLiteral("Восстановления речи для пациентов с эфферентно-моторной афазией")},
        {QStringLiteral("idz31"), QStringLiteral("Подготовка к преодолению трудностей переключения")},
        {QStringLiteral("idz32"), QStringLiteral("Преодоление трудностей переключения")},
        {QStringLiteral("idz4"), QStringLiteral("Восстановления речи для пациентов с афферентно-моторной афазией")},
        {QStringLiteral("idz41"), QStringLiteral("Растормаживание речи при опоре на непроизвольный уровень")},
        {QStringLiteral("idz42"), QStringLiteral("Перевод на произвольный уровень")},
        {QStringLiteral("idz43"), QStringLiteral("Самостоятельные письмо и речь")},
        {QStringLiteral("idz5"), QStringLiteral("Восстановления речи для пациентов с акустико-гностической афазией")},
        {QStringLiteral("idz51"), QStringLiteral("Затормаживание обильной непродуктивной речи")},
        {QStringLiteral("idz52"), QStringLiteral("Восстановление фонематического восприятия")},
        {QStringLiteral("idz6"), QStringLiteral("Восстановления речи для пациентов с акустико-мнестической афазией")},
        {QStringLiteral("idz61"), QStringLiteral("Восстановление предметной отнесенности слова")},
        {QStringLiteral("idz62"), QStringLiteral("Восстановление ситуативно обусловленной речи")},
        {QStringLiteral("idz63"), QStringLiteral("Расширяем объем слухоречевой памяти и восстанавливаем фонематическое восприятие")},
        {QStringLiteral("idz7"), QStringLiteral("Восстановления речи для пациентов с семантической афазией")},
        {QStringLiteral("idz71"), QStringLiteral("Преодоление нарушений зрительного гнозиса и восстановление понимания математических знаков")},
        {QStringLiteral("idz72"), QStringLiteral("Преодоление трудностей понимания логико-грамматических оборотов")},
    };
    return titles;
}

const QHash<QString, QPair<QString, QString>> &pharmacologyRules() {
    static const QHash<QString, QPair<QString, QString>> rules = {
        {QStringLiteral("cr1Add"), {QStringLiteral("Трансдермальный пластырь с длительным высвобождением никотина."), QStringLiteral("idText1")}},
        {QStringLiteral("cr2Add"), {QStringLiteral("Жевательная резинка 2 и 4 мг."), QStringLiteral("idText2")}},
        {QStringLiteral("cr3Add"), {QStringLiteral("Подъязычная таблетка - 2 мг."), QStringLiteral("idText3")}},
        {QStringLiteral("cr4Add"), {QStringLiteral("Спрей для слизистой оболочки полости рта дозированный."), QStringLiteral("idText4")}},
        {QStringLiteral("cr5Add"), {QStringLiteral("Варениклин."), QStringLiteral("idText5")}},
        {QStringLiteral("cr6Add"), {QStringLiteral("Фабомотизол."), QStringLiteral("idText6")}},
        {QStringLiteral("cr7Add"), {QStringLiteral("Тетраметилтетраазабициклооктадион."), QStringLiteral("idText7")}},
        {QStringLiteral("cr8Add"), {QStringLiteral("Этифоксин гидрохлорид."), QStringLiteral("idText8")}},
        {QStringLiteral("cr9Add"), {QStringLiteral("Иглорефлексотерапии."), QStringLiteral("idText9")}},
        {QStringLiteral("cr10Add"), {QString(), QStringLiteral("idText10")}},
        {QStringLiteral("cr11Add"), {QString(), QStringLiteral("idText11")}},
    };
    return rules;
}

constexpr const char *kAlertOverrideJs =
    "window.alert = function (m) {"
    "  var q = document.getElementById('__appAlert');"
    "  if (!q) {"
    "    q = document.createElement('textarea');"
    "    q.id = '__appAlert';"
    "    q.style.display = 'none';"
    "    document.body.appendChild(q);"
    "  }"
    "  q.value = q.value ? q.value + '\\n' + m : String(m);"
    "};";

QString jsQuote(const QString &text) {
    return QString::fromUtf8(QJsonDocument(QJsonArray{text}).toJson(QJsonDocument::Compact))
        .mid(1)
        .chopped(1);
}

} // namespace

ClinicalBrowserController::ClinicalBrowserController(QWidget *panel, QObject *parent)
    : QObject(parent)
    , m_panel(panel)
{
#ifdef Q_OS_WIN
    m_web = new QAxWidget(m_panel);
    m_web->setControl(kBrowserClsid);
    m_web->setGeometry(16, 51, 937, kBrowserHeight);
    connect(m_web, SIGNAL(BeforeNavigate2(IDispatch*,QVariant&,QVariant&,QVariant&,QVariant&,QVariant&,bool&)),
            this, SLOT(onBeforeNavigate(IDispatch*,QVariant&,QVariant&,QVariant&,QVariant&,QVariant&,bool&)));
    connect(m_web, SIGNAL(NavigateComplete2(IDispatch*,QVariant&)),
            this, SLOT(onNavigateComplete(IDispatch*,QVariant&)));
    connect(m_web, SIGNAL(DocumentComplete(IDispatch*,QVariant&)),
            this, SLOT(onDocumentComplete(IDispatch*,QVariant&)));
#elif defined(SUNRISE_USE_WEBKIT)
    m_web = new QWebView(m_panel);
    m_web->setGeometry(16, 51, 937, kBrowserHeight);
    m_web->setAttribute(Qt::WA_NativeWindow);
    m_web->settings()->setAttribute(QWebSettings::JavascriptEnabled, true);
    m_web->settings()->setAttribute(QWebSettings::LocalContentCanAccessFileUrls, true);
    m_web->setStyleSheet(QStringLiteral("QWebView { background: #f0f0f0; }"));
    m_bridge = new ClinicalWebBridge(this, this);
    connect(m_web, &QWebView::loadStarted, this, &ClinicalBrowserController::onLoadStarted);
    connect(m_web, &QWebView::urlChanged, this, &ClinicalBrowserController::onUrlChanged);
    connect(m_web, &QWebView::loadFinished, this, &ClinicalBrowserController::onLoadFinished);
#else
    m_web = new QWebEngineView(m_panel);
    m_web->setGeometry(16, 51, 937, kBrowserHeight);
    m_web->setAttribute(Qt::WA_NativeWindow);
    m_web->page()->setBackgroundColor(QColor(0xf0, 0xf0, 0xf0));
    m_web->settings()->setAttribute(QWebEngineSettings::JavascriptEnabled, true);
    m_web->settings()->setAttribute(QWebEngineSettings::LocalContentCanAccessFileUrls, true);
    m_web->settings()->setAttribute(QWebEngineSettings::FocusOnNavigationEnabled, true);
    m_bridge = new ClinicalWebBridge(this, this);
    m_channel = new QWebChannel(m_web->page());
    m_channel->registerObject(QStringLiteral("bridge"), m_bridge);
    m_web->page()->setWebChannel(m_channel);
    connect(m_web, &QWebEngineView::loadStarted, this, &ClinicalBrowserController::onLoadStarted);
    connect(m_web, &QWebEngineView::urlChanged, this, &ClinicalBrowserController::onUrlChanged);
    connect(m_web, &QWebEngineView::loadFinished, this, &ClinicalBrowserController::onLoadFinished);
#endif

    const auto makeButton = [this](int x, int y, int w, int h) {
        auto *button = new ImageButton(m_panel);
        button->setAttribute(Qt::WA_NativeWindow);
        // Под собственным HWND поверх IE прозрачные края картинки иначе заливаются чёрным.
        button->setStyleSheet(QStringLiteral("ImageButton { background: #f0f0f0; border: none; }")
                              + ImageButton::toolTipStyleSheet());
        button->setGeometry(x, y, w, h);
        button->hide();
        return button;
    };
    m_itogBtn = makeButton(422, 909, 154, 34);
    m_plusBtn = makeButton(723, 909, 32, 34);
    m_backBtn = makeButton(803, 3, 100, 21);
    m_recBtn = makeButton(296, 3, 100, 21);

    connect(m_itogBtn, &ImageButton::clicked, this, [this]() {
        runItog();
        placeResultButtons();
    });
    connect(m_plusBtn, &ImageButton::clicked, this, &ClinicalBrowserController::runPlus);
    connect(m_backBtn, &ImageButton::clicked, this, [this]() {
        openPage(m_previousAddress);
        showButton(m_backBtn, false);
    });

    m_templateSaveTimer = new QTimer(this);
    m_templateSaveTimer->setSingleShot(true);
    m_templateSaveTimer->setInterval(800);
    connect(m_templateSaveTimer, &QTimer::timeout, this, &ClinicalBrowserController::saveTemplates);

    m_panel->installEventFilter(this);
    m_web->installEventFilter(this);
    qApp->installEventFilter(this);
}

ClinicalBrowserController::~ClinicalBrowserController() = default;

void ClinicalBrowserController::setSummary(SummaryPanel *summary) {
    m_summary = summary;
}

void ClinicalBrowserController::setHtmlRoot(const QString &htmlsDir) {
    m_htmlsRoot = htmlsDir;
}

void ClinicalBrowserController::setImageLoader(const std::function<QString(const QString &)> &loader) {
    m_imageLoader = loader;
    if (!m_imageLoader) {
        return;
    }
    m_itogBtn->setImagePath(m_imageLoader(QStringLiteral("itog.png")));
    m_plusBtn->setImagePath(m_imageLoader(QStringLiteral("plus.png")));
    setBackImage(QStringLiteral("backto.png"));
    const QPixmap rec(m_imageLoader(QStringLiteral("reqon.png")));
    m_recBtn->setPixmap(rec);
    m_recBtn->resize(rec.size());
}

void ClinicalBrowserController::setBackImage(const QString &name) {
    if (!m_imageLoader) {
        return;
    }
    const QPixmap pixmap(m_imageLoader(name));
    if (pixmap.isNull()) {
        return;
    }
    m_backBtn->setPixmap(pixmap);
    m_backBtn->resize(pixmap.size());
}

void ClinicalBrowserController::setBrowserHeight(int height) {
    m_web->resize(m_web->width(), height);
}

void ClinicalBrowserController::showButton(ImageButton *button, bool visible) {
    button->setVisible(visible);
    if (visible) {
        button->raise();
    }
}

void ClinicalBrowserController::setHelpIndex(const QString &help) {
    m_helpIndex = help;
    emit helpIndexChanged(m_helpIndex);
}

void ClinicalBrowserController::openSection(Section section) {
    saveTemplates();
    m_section = section;
    showButton(m_backBtn, false);
    showButton(m_itogBtn, false);
    showButton(m_plusBtn, false);
    const bool speech = section == Section::Speech;
    showButton(m_recBtn, speech);
    QString page;
    switch (section) {
    case Section::Risk:
        setHelpIndex(QStringLiteral("0.6.htm"));
        page = QStringLiteral("interface/title.html");
        break;
    case Section::Correction:
        setHelpIndex(QStringLiteral("0.7.htm"));
        setBrowserHeight(925);
        page = QStringLiteral("interface/corr0.html");
        break;
    case Section::Breathing:
        setHelpIndex(QStringLiteral("0.9.htm"));
        setBrowserHeight(965 - m_web->y());
        page = QStringLiteral("interface/dih0.html");
        break;
    case Section::Medication:
        setHelpIndex(QStringLiteral("0.10.htm"));
        page = QStringLiteral("interface/mp0.html");
        break;
    case Section::Speech:
        setHelpIndex(QStringLiteral("1.0.0.htm"));
        setBrowserHeight(968);
        page = QStringLiteral("upr/upr.html");
        break;
    }
    openPage(QDir(m_htmlsRoot).filePath(page));
}

void ClinicalBrowserController::openPage(const QString &path) {
    QString target = path;
    if (target.isEmpty()) {
        return;
    }
    if (!target.contains(QStringLiteral("://"))) {
        target = QUrl::fromLocalFile(target).toString();
    }
#ifdef Q_OS_WIN
    m_web->dynamicCall("Navigate(const QString&)", target);
#else
    m_pageReady = false;
    m_web->load(QUrl(target));
#endif
}

bool ClinicalBrowserController::eventFilter(QObject *watched, QEvent *event) {
    switch (event->type()) {
    case QEvent::KeyPress:
    case QEvent::KeyRelease:
    case QEvent::ShortcutOverride:
    case QEvent::InputMethod:
#ifdef Q_OS_WIN
        // ActiveQt дублирует в Qt каждую клавишу, набранную в IE (QAxClientSite::TranslateAccelerator),
        // и Qt отдаёт её последнему виджету с фокусом — скрытому полю анамнеза.
        if (browserHasFocus()) {
            return true;
        }
#endif
        break;
    default:
        break;
    }
    if ((watched == m_panel || watched == m_web)
        && (event->type() == QEvent::Leave || event->type() == QEvent::Hide)) {
        saveTemplates();
    }
    return QObject::eventFilter(watched, event);
}

#ifdef Q_OS_WIN
void ClinicalBrowserController::onBeforeNavigate(IDispatch *, QVariant &, QVariant &, QVariant &,
                                                 QVariant &, QVariant &, bool &) {
    saveTemplates();
}

void ClinicalBrowserController::onNavigateComplete(IDispatch *, QVariant &url) {
    applyPageAddress(url.toString());
}

void ClinicalBrowserController::onDocumentComplete(IDispatch *, QVariant &) {
    delete m_document;
    m_document = m_web->querySubObject("Document");
    if (!m_document) {
        return;
    }
    connect(m_document, SIGNAL(onmousedown(IDispatch*)), this, SLOT(onBodyMouseDown(IDispatch*)));
    connect(m_document, SIGNAL(onclick(IDispatch*)), this, SLOT(onDocumentClick(IDispatch*)));
    installPageAlert();
    connect(m_document, SIGNAL(onkeyup(IDispatch*)), this, SLOT(onDocumentEdited(IDispatch*)));
    connect(m_document, SIGNAL(onfocusout(IDispatch*)), this, SLOT(onDocumentEdited(IDispatch*)));
    finishDocumentLoad();
}
#endif

void ClinicalBrowserController::applyPageAddress(const QString &address) {
    const QString name = pageFileName(address);
    const QString base = name.section(QLatin1Char('.'), 0, 0);
    if (!base.isEmpty() && base.back() >= QLatin1Char('1') && base.back() <= QLatin1Char('5')) {
        m_previousAddress = m_address;
    }
    m_address = address;
    m_pageName.clear();
    m_fagerstromScore = -1;
    m_fagerstromVerdict.clear();
    m_prochaskaScore = -1;
    m_prochaskaVerdict.clear();

    const auto backAt = [this](int x) {
        m_backBtn->move(x, m_backBtn->y());
        showButton(m_backBtn, true);
    };
    if (base == QLatin1String("title")) {
        showButton(m_itogBtn, false);
        showButton(m_plusBtn, false);
        setHelpIndex(QStringLiteral("0.6.htm"));
    } else if (base == QLatin1String("next4")) {
        setHelpIndex(QStringLiteral("0.6.4.htm"));
        setBrowserHeight(kBrowserHeight);
        m_itogBtn->move(530, 530);
        m_plusBtn->move(m_itogBtn->x() + m_itogBtn->width() + 20, 530);
        m_pageName = QStringLiteral("4");
        showButton(m_itogBtn, true);
        showButton(m_plusBtn, true);
        showButton(m_backBtn, true);
    } else if (base == QLatin1String("next3")) {
        setHelpIndex(QStringLiteral("0.6.3.htm"));
        setBrowserHeight(900);
        m_itogBtn->move(390, 900);
        m_plusBtn->move(m_itogBtn->x() + m_itogBtn->width(), 900);
        m_pageName = QStringLiteral("3");
        showButton(m_plusBtn, false);
        backAt(140);
    } else if (base == QLatin1String("next2")) {
        setHelpIndex(QStringLiteral("0.6.2.htm"));
        setBrowserHeight(kBrowserHeight);
        m_itogBtn->move(530, 490);
        m_plusBtn->move(m_itogBtn->x() + m_itogBtn->width() + 20, 490);
        m_pageName = QStringLiteral("2");
        showButton(m_itogBtn, true);
        showButton(m_plusBtn, true);
        backAt(140);
    } else if (base == QLatin1String("next1")) {
        setHelpIndex(QStringLiteral("0.6.1.htm"));
        setBrowserHeight(kBrowserHeight);
        m_itogBtn->move(530, 570);
        m_plusBtn->move(m_itogBtn->x() + m_itogBtn->width() + 20, 570);
        m_pageName = QStringLiteral("1");
        showButton(m_itogBtn, true);
        showButton(m_plusBtn, true);
        setBackImage(QStringLiteral("back1.png"));
        backAt(140);
    } else if (base.startsWith(QLatin1String("corr")) && base.size() == 5 && base.back() != QLatin1Char('0')) {
        const QString n = base.right(1);
        setHelpIndex(QStringLiteral("0.7.%1.htm").arg(n));
        m_pageName = QStringLiteral("2") + n;
        if (n == QLatin1String("1")) {
            setBackImage(QStringLiteral("backto.png"));
        }
        backAt(330);
    } else if (base.startsWith(QLatin1String("dih")) && base.size() == 4 && base.back() != QLatin1Char('0')) {
        const QString n = base.right(1);
        setHelpIndex(QStringLiteral("0.9.%1.htm").arg(n));
        m_pageName = QStringLiteral("5") + n;
        backAt(660);
    } else if (base.startsWith(QLatin1String("mp")) && base.size() == 3 && base.back() != QLatin1Char('0')) {
        const QString n = base.right(1);
        setHelpIndex(QStringLiteral("0.10.%1.htm").arg(n));
        m_pageName = QStringLiteral("6") + n;
        backAt(785);
    } else if (base == QLatin1String("corr0")) {
        setHelpIndex(QStringLiteral("0.7.htm"));
    } else if (base == QLatin1String("dih0")) {
        setHelpIndex(QStringLiteral("0.9.htm"));
    } else if (base == QLatin1String("mp0")) {
        setHelpIndex(QStringLiteral("0.10.htm"));
    }
}

void ClinicalBrowserController::finishDocumentLoad() {
    installPageAlert();
    loadTemplates();
    placeResultButtons();
    // Скрипт страницы перестраивает выпадающие списки после onload — таблица меняет высоту.
    for (const int delay : {200, 700}) {
        QTimer::singleShot(delay, this, [this, address = m_address]() {
            if (m_address == address) {
                placeResultButtons();
            }
        });
    }
}

void ClinicalBrowserController::placeResultButtons() {
    if (m_pageName != QLatin1String("1") && m_pageName != QLatin1String("2") && m_pageName != QLatin1String("4")) {
        return;
    }
    const int bottom = elementBottom(QStringLiteral("idv"));
    if (bottom <= 0) {
        return;
    }
    const int y = qMin(m_web->y() + bottom + 18, m_panel->height() - m_itogBtn->height() - 12);
    const int gap = m_plusBtn->x() - m_itogBtn->x() - m_itogBtn->width();
    m_itogBtn->move(m_itogBtn->x(), y);
    m_plusBtn->move(m_itogBtn->x() + m_itogBtn->width() + gap, y);
}

void ClinicalBrowserController::installPageAlert() {
#ifdef Q_OS_WIN
    if (!m_document) {
        return;
    }
    std::unique_ptr<QAxObject> window(m_document->querySubObject("parentWindow"));
    if (!window) {
        return;
    }
    window->dynamicCall("execScript(QString,QString)", QString::fromLatin1(kAlertOverrideJs),
                        QStringLiteral("JavaScript"));
#else
    evalJs(QString::fromLatin1(kAlertOverrideJs));
#endif
}

void ClinicalBrowserController::flushPageAlerts() {
    if (!elementExists(QStringLiteral("__appAlert"))) {
        return;
    }
    const QString raw = normalizeText(valueOf(QStringLiteral("__appAlert"))).trimmed();
    if (raw.isEmpty()) {
        return;
    }
#ifdef Q_OS_WIN
    if (const auto queue = element(QStringLiteral("__appAlert"))) {
        queue->setProperty("value", QString());
    }
#else
    evalJs(QStringLiteral("(function(){ var q=document.getElementById('__appAlert'); if(q) q.value=''; })()"));
#endif
    QString text = raw;
    text.remove(QRegularExpression(QStringLiteral("^Ошибка!\\s*"), QRegularExpression::MultilineOption));
    QPointer<QWidget> panel = m_panel;
    QTimer::singleShot(0, this, [panel, text]() { CustomMessageBox::showWarning(panel, text); });
}

bool ClinicalBrowserController::documentReady() const {
#ifdef Q_OS_WIN
    return m_document != nullptr;
#else
    return m_pageReady;
#endif
}

bool ClinicalBrowserController::browserHasFocus() const {
    if (!m_web || !m_web->isVisible()) {
        return false;
    }
#ifdef Q_OS_WIN
    const HWND focus = ::GetFocus();
    const HWND browser = reinterpret_cast<HWND>(m_web->winId());
    return focus && focus != browser && ::IsChild(browser, focus);
#else
    return m_web->hasFocus();
#endif
}

#ifdef Q_OS_WIN
void ClinicalBrowserController::onDocumentClick(IDispatch *) {
    flushPageAlerts();
}

void ClinicalBrowserController::onDocumentEdited(IDispatch *) {
    m_templateSaveTimer->start();
}

std::unique_ptr<QAxObject> ClinicalBrowserController::element(const QString &id) const {
    if (!m_document || id.isEmpty()) {
        return nullptr;
    }
    return std::unique_ptr<QAxObject>(m_document->querySubObject("getElementById(QString)", id));
}
#endif

bool ClinicalBrowserController::elementExists(const QString &id) const {
    if (id.isEmpty() || !documentReady()) {
        return false;
    }
#ifdef Q_OS_WIN
    return static_cast<bool>(element(id));
#else
    return evalJs(QStringLiteral("document.getElementById(%1) ? '1' : '0'").arg(jsQuote(id))).toString()
        == QLatin1String("1");
#endif
}

int ClinicalBrowserController::elementBottom(const QString &id) const {
    if (!elementExists(id)) {
        return 0;
    }
#ifdef Q_OS_WIN
    const auto verdict = element(id);
    std::unique_ptr<QAxObject> rect(verdict->querySubObject("getBoundingClientRect()"));
    std::unique_ptr<QAxObject> root(m_document->querySubObject("documentElement"));
    const int scrollTop = root ? root->property("scrollTop").toInt() : 0;
    return rect ? qRound(rect->property("bottom").toDouble()) + scrollTop : 0;
#else
    return evalJs(QStringLiteral(
                      "(function(){ var e=document.getElementById(%1); if(!e) return 0;"
                      " var r=e.getBoundingClientRect();"
                      " var st=document.documentElement.scrollTop||document.body.scrollTop||0;"
                      " return Math.round(r.bottom+st); })()")
                      .arg(jsQuote(id)))
        .toInt();
#endif
}

QString ClinicalBrowserController::innerText(const QString &id) const {
#ifdef Q_OS_WIN
    const auto el = element(id);
    return el ? el->property("innerText").toString() : QString();
#else
    return evalJs(QStringLiteral(
                      "(function(){ var e=document.getElementById(%1); if(!e) return '';"
                      " return e.innerText != null ? e.innerText : (e.textContent || ''); })()")
                      .arg(jsQuote(id)))
        .toString();
#endif
}

QString ClinicalBrowserController::innerHtml(const QString &id) const {
#ifdef Q_OS_WIN
    const auto el = element(id);
    return el ? el->property("innerHTML").toString() : QString();
#else
    return evalJs(QStringLiteral(
                      "(function(){ var e=document.getElementById(%1); return e ? e.innerHTML : ''; })()")
                      .arg(jsQuote(id)))
        .toString();
#endif
}

void ClinicalBrowserController::setInnerHtml(const QString &id, const QString &html) {
#ifdef Q_OS_WIN
    if (const auto el = element(id)) {
        el->setProperty("innerHTML", html);
    }
#else
    evalJs(QStringLiteral("(function(){ var e=document.getElementById(%1); if(e) e.innerHTML=%2; })()")
               .arg(jsQuote(id), jsQuote(html)));
#endif
}

void ClinicalBrowserController::setInnerText(const QString &id, const QString &text) {
#ifdef Q_OS_WIN
    if (const auto el = element(id)) {
        el->setProperty("innerText", text);
    }
#else
    evalJs(QStringLiteral("(function(){ var e=document.getElementById(%1); if(e) e.innerText=%2; })()")
               .arg(jsQuote(id), jsQuote(text)));
#endif
}

QString ClinicalBrowserController::valueOf(const QString &id) const {
#ifdef Q_OS_WIN
    const auto el = element(id);
    if (!el) {
        return {};
    }
    static const QStringList formTags = {
        QStringLiteral("INPUT"), QStringLiteral("SELECT"), QStringLiteral("TEXTAREA"),
        QStringLiteral("BUTTON"), QStringLiteral("OPTION")
    };
    if (formTags.contains(el->property("tagName").toString().toUpper())) {
        return el->property("value").toString();
    }
    return el->dynamicCall("getAttribute(QString)", QStringLiteral("value")).toString();
#else
    return evalJs(QStringLiteral(
                      "(function(){ var e=document.getElementById(%1); if(!e) return '';"
                      " var t=(e.tagName||'').toUpperCase();"
                      " if(t==='INPUT'||t==='SELECT'||t==='TEXTAREA'||t==='BUTTON'||t==='OPTION'||t==='TEXTAREA')"
                      " return e.value != null ? String(e.value) : '';"
                      " var a=e.getAttribute('value'); return a != null ? String(a) : ''; })()")
                      .arg(jsQuote(id)))
        .toString();
#endif
}

bool ClinicalBrowserController::isChecked(const QString &id) const {
#ifdef Q_OS_WIN
    const auto el = element(id);
    return el && el->property("checked").toBool();
#else
    return evalJs(QStringLiteral(
                      "(function(){ var e=document.getElementById(%1); return (e && e.checked) ? '1' : '0'; })()")
                      .arg(jsQuote(id)))
               .toString()
        == QLatin1String("1");
#endif
}

void ClinicalBrowserController::setStyle(const QString &id, const QString &css) {
#ifdef Q_OS_WIN
    const auto el = element(id);
    if (!el) {
        return;
    }
    if (QAxObject *style = el->querySubObject("style")) {
        style->setProperty("cssText", css);
    }
#else
    evalJs(QStringLiteral("(function(){ var e=document.getElementById(%1); if(e) e.style.cssText=%2; })()")
               .arg(jsQuote(id), jsQuote(css)));
#endif
}

bool ClinicalBrowserController::templateSpec(TemplateSpec *spec) const {
    const QString defaultTitle = QStringLiteral("Шаблон назначения");
    const QString medicationTitle = QStringLiteral("Шаблон назначения ЛС");
    const QString extraTitle = QStringLiteral("Дополнительный раздел");
    const QString extraBody = QStringLiteral("При необходимости переименуйте раздел и введите данные по своему усмотрению");
    static const QHash<QString, int> counts = {
        {QStringLiteral("21"), 11}, {QStringLiteral("22"), 1}, {QStringLiteral("23"), 1}, {QStringLiteral("24"), 1},
        {QStringLiteral("51"), 4}, {QStringLiteral("52"), 5}, {QStringLiteral("53"), 5}, {QStringLiteral("54"), 5},
        {QStringLiteral("61"), 10}, {QStringLiteral("62"), 11}, {QStringLiteral("63"), 6}, {QStringLiteral("64"), 6},
    };
    if (!counts.contains(m_pageName)) {
        return false;
    }
    spec->count = counts.value(m_pageName);
    spec->fileName = QStringLiteral("шаблон%1.txt").arg(m_pageName.startsWith(QLatin1Char('2')) ? m_pageName.right(1) : m_pageName);
    spec->idPrefix = m_pageName == QLatin1String("51") ? QStringLiteral("idText1") : QStringLiteral("idText");
    spec->asHtml = m_pageName.startsWith(QLatin1Char('2'));
    if (m_pageName == QLatin1String("21")) {
        spec->placeholders = QStringList{defaultTitle, extraTitle, extraBody};
    } else if (m_pageName == QLatin1String("22")) {
        spec->placeholders = QStringList{defaultTitle, extraTitle, QStringLiteral("Шаблон рекомендаций по отказу от злоупотребления алкоголем")};
    } else if (m_pageName == QLatin1String("23")) {
        spec->placeholders = QStringList{defaultTitle, extraTitle, QStringLiteral("Шаблон рекомендаций по здоровому питанию")};
    } else if (m_pageName == QLatin1String("24")) {
        spec->placeholders = QStringList{defaultTitle, extraTitle, QStringLiteral("Шаблон рекомендаций по снижению веса")};
    } else if (m_pageName == QLatin1String("51")) {
        spec->placeholders = QStringList{defaultTitle, extraTitle, QStringLiteral("Введите данные по своему усмотрению")};
    } else if (m_pageName == QLatin1String("61")) {
        spec->placeholders = QStringList{medicationTitle, extraTitle, extraBody + QLatin1Char(' ')};
    } else if (m_pageName == QLatin1String("62") || m_pageName == QLatin1String("64")) {
        spec->placeholders = QStringList{medicationTitle, extraTitle, extraBody};
    } else {
        spec->placeholders = QStringList{defaultTitle, extraTitle, extraBody};
    }
    return true;
}

QString ClinicalBrowserController::templatePath(const QString &fileName) const {
    return QDir(m_htmlsRoot).filePath(QStringLiteral("interface/") + fileName);
}

void ClinicalBrowserController::loadTemplates() {
    TemplateSpec spec;
    if (!templateSpec(&spec)) {
        return;
    }
    QFile file(templatePath(spec.fileName));
    if (!file.open(QIODevice::ReadOnly)) {
        return;
    }
    QString text = QString::fromUtf8(file.readAll());
    if (text.startsWith(QChar(0xFEFF))) {
        text.remove(0, 1);
    }
    const QStringList parts = text.split(QLatin1Char(';'));
    for (int i = 1; i <= spec.count && i <= parts.size(); ++i) {
        const QString id = spec.idPrefix + QString::number(i);
        const QString part = parts.at(i - 1);
        if (part.trimmed().isEmpty()) {
            continue;
        }
        if (!spec.placeholders.contains(part) && part.trimmed() != innerText(id).trimmed()) {
            setStyle(id, QStringLiteral("color:black;"));
        }
        if (spec.asHtml) {
            setInnerHtml(id, part);
        } else {
            setInnerText(id, part);
        }
    }
}

void ClinicalBrowserController::saveTemplates() {
    m_templateSaveTimer->stop();
    TemplateSpec spec;
    if (!documentReady() || !templateSpec(&spec)) {
        return;
    }
    QString content;
    for (int i = 1; i <= spec.count; ++i) {
        const QString id = spec.idPrefix + QString::number(i);
        if (!elementExists(id)) {
            return;
        }
        content += innerText(id) + QLatin1Char(';');
    }
    const QByteArray bytes = content.toUtf8() + "\r\n";
    const QString path = templatePath(spec.fileName);
    QFile current(path);
    if (current.open(QIODevice::ReadOnly) && current.readAll() == bytes) {
        return;
    }
    current.close();
    QSaveFile out(path);
    if (out.open(QIODevice::WriteOnly)) {
        out.write(bytes);
        out.commit();
    }
}

void ClinicalBrowserController::showError(const QString &text) {
    QPointer<QWidget> panel = m_panel;
    QTimer::singleShot(0, this, [panel, text]() { CustomMessageBox::showError(panel, text); });
}

void ClinicalBrowserController::appendTo(SummaryPanel::Field field, const QString &chunk) {
    if (!m_summary) {
        return;
    }
    m_summary->appendToField(field, normalizeText(chunk), false);
    emit programChanged();
}

void ClinicalBrowserController::appendTemplate(SummaryPanel::Field field, const QString &head, const QString &bodyId) {
    appendTo(field, today() + QStringLiteral(". ") + head + QLatin1Char('\n') + normalizeText(innerText(bodyId)).trimmed());
}

void ClinicalBrowserController::printRtf(const QString &fileName) {
    QFile file(templatePath(fileName));
    if (!file.open(QIODevice::ReadOnly)) {
        showError(QStringLiteral("Файл %1 не найден.").arg(fileName));
        return;
    }
    const QString html = rtfToHtml(file.readAll());
    QPointer<QWidget> panel = m_panel;
    // Модальный диалог нельзя открывать внутри обработчика события IE.
    QTimer::singleShot(0, this, [panel, html]() {
        QPrinter printer(QPrinter::HighResolution);
        QPrintDialog dialog(&printer, panel);
        dialog.setWindowTitle(QStringLiteral("Печать"));
        if (dialog.exec() != QDialog::Accepted) {
            return;
        }
        QTextDocument doc;
        doc.setHtml(html);
        doc.print(&printer);
    });
}

#ifdef Q_OS_WIN
void ClinicalBrowserController::onBodyMouseDown(IDispatch *event) {
    if (!event || !m_document) {
        return;
    }
    QAxObject eventObject(reinterpret_cast<IUnknown *>(event));
    const int button = eventObject.property("button").toInt();
    if (button != 0 && button != 1) {
        return;
    }
    std::unique_ptr<QAxObject> source(eventObject.querySubObject("srcElement"));
    if (!source) {
        return;
    }
    const QString id = source->property("id").toString();
    std::unique_ptr<QAxObject> parent(source->querySubObject("parentElement"));
    const QString parentTag = parent ? parent->property("tagName").toString() : QString();
    const QString parentText = parent ? parent->property("innerText").toString() : QString();
    if (m_section == Section::Speech) {
        const QString src = source->dynamicCall("getAttribute(QString)", QStringLiteral("src")).toString();
        handleSpeechClick(id, src, parentText);
        return;
    }
    handleClick(id, parentTag);
}
#endif

void ClinicalBrowserController::handleSpeechClick(const QString &id, const QString &src, const QString &parentText) {
    if (src.contains(QStringLiteral("plust"))) {
        const QString title = normalizeText(parentText).trimmed();
        if (!title.isEmpty()) {
            appendTo(SummaryPanel::Speech, today() + QLatin1Char(' ') + title);
        }
        return;
    }
    const auto title = speechTitles().constFind(id);
    if (title != speechTitles().constEnd()) {
        appendTo(SummaryPanel::Speech, today() + QStringLiteral(". ") + title.value());
    }
}

void ClinicalBrowserController::handleClick(const QString &id, const QString &parentTag) {
    if (id.isEmpty()) {
        return;
    }
    const auto rule = templateRules().constFind(id);
    if (rule != templateRules().constEnd()) {
        const QString head = rule->headId.isEmpty() ? rule->head : normalizeText(innerText(rule->headId)).trimmed();
        if (rule->sameLine) {
            appendTo(rule->field, today() + QStringLiteral(". ") + head + QLatin1Char(' ')
                                      + normalizeText(innerText(rule->bodyId)).trimmed());
        } else {
            appendTemplate(rule->field, head, rule->bodyId);
        }
        return;
    }
    if (id == QLatin1String("tmp35")) {
        const bool extraSection = parentTag.compare(QStringLiteral("TD"), Qt::CaseInsensitive) == 0;
        if (extraSection) {
            appendTemplate(SummaryPanel::Mp, normalizeText(innerText(QStringLiteral("idText5"))).trimmed(), QStringLiteral("idText6"));
        } else {
            appendTemplate(SummaryPanel::Mp, QStringLiteral("Апиксабан"), QStringLiteral("idText4"));
        }
        return;
    }
    const auto pharmacology = pharmacologyRules().constFind(id);
    if (pharmacology != pharmacologyRules().constEnd()) {
        QString head = pharmacology->first;
        if (id == QLatin1String("cr11Add")) {
            head = normalizeText(innerText(QStringLiteral("idText10"))).trimmed();
        }
        QString chunk = today() + QStringLiteral(". Фармакологическая терапия. ");
        if (!head.isEmpty()) {
            chunk += head + QLatin1Char(' ');
        }
        chunk += normalizeText(innerText(pharmacology->second)).trimmed();
        appendTo(SummaryPanel::Corr, chunk);
        return;
    }
    if (id == QLatin1String("itog21")) {
        int score = 0;
        for (int i = 1; i <= 6; ++i) {
            const QString value = innerText(QStringLiteral("idb%1").arg(i)).trimmed();
            if (value != QLatin1String("0") && value != QLatin1String("1") && value != QLatin1String("2")
                && value != QLatin1String("3")) {
                showError(QStringLiteral("Ошибка! Пройдены не все пункты теста."));
                return;
            }
            score += value.toInt();
        }
        setInnerHtml(QStringLiteral("iditog"), QString::number(score));
        QString verdict;
        if (score <= 2) {
            verdict = QStringLiteral("Вывод: очень слабая зависимость");
        } else if (score <= 4) {
            verdict = QStringLiteral("Вывод: слабая зависимость");
        } else if (score == 5) {
            verdict = QStringLiteral("Вывод: средняя зависимость");
        } else if (score <= 7) {
            verdict = QStringLiteral("Вывод: высокая зависимость");
        } else {
            verdict = QStringLiteral("Вывод: очень высокая зависимость");
        }
        setInnerHtml(QStringLiteral("idv"), verdict);
        m_fagerstromScore = score;
        m_fagerstromVerdict = verdict;
        return;
    }
    if (id == QLatin1String("atog21")) {
        if (m_fagerstromScore < 0) {
            showError(QStringLiteral("Пройдены не все пункты теста"));
            return;
        }
        appendTo(SummaryPanel::Corr, today() + QStringLiteral(" Отказ от курения. Оценка степени никотиновой зависимости (тест Фагерстрема). Результат - %1. %2")
                                                   .arg(m_fagerstromScore).arg(m_fagerstromVerdict));
        return;
    }
    if (id == QLatin1String("itog22")) {
        int score = 0;
        for (int i = 1; i <= 2; ++i) {
            const QString value = innerText(QStringLiteral("idb2%1").arg(i)).trimmed();
            bool ok = false;
            const int points = value.toInt(&ok);
            if (!ok || points < 0 || points > 4) {
                showError(QStringLiteral("Ошибка! Пройдены не все пункты теста."));
                return;
            }
            score += points;
        }
        setInnerHtml(QStringLiteral("iditog2"), QString::number(score));
        QString verdict;
        if (score <= 2) {
            verdict = QStringLiteral("Вывод: отсутствие мотивации, и пациенту можно предложить программу снижения интенсивности курения");
        } else if (score <= 4) {
            verdict = QStringLiteral("Вывод: пациент имеет слабую мотивацию, и ему можно предложить короткую лечебную программу с целью снижения курения и усиления мотивации");
        } else {
            verdict = QStringLiteral("Вывод: пациент имеет высокую мотивацию к отказу от курения, и ему можно предложить длительную лечебную программу с целью полного отказа от курения");
        }
        setInnerHtml(QStringLiteral("idv2"), verdict);
        m_prochaskaScore = score;
        m_prochaskaVerdict = verdict;
        return;
    }
    if (id == QLatin1String("atog22")) {
        if (m_prochaskaScore < 0) {
            showError(QStringLiteral("Пройдены не все пункты теста"));
            return;
        }
        appendTo(SummaryPanel::Corr, today() + QStringLiteral(" Отказ от курения. Оценка мотивации бросить курить (тест Прохаска). Результат - %1. %2")
                                                   .arg(m_prochaskaScore).arg(m_prochaskaVerdict));
        return;
    }
    if (id == QLatin1String("ctest")) {
        const QString index = valueOf(QStringLiteral("idikr")).trimmed();
        if (index.isEmpty() || index == QLatin1String("?")) {
            showError(QStringLiteral("Ошибка! Не все данные внесены."));
            return;
        }
        appendTo(SummaryPanel::Corr, today() + QStringLiteral(" Отказ от курения. Индекс курения (ИК) - ") + index);
        return;
    }
    if (id == QLatin1String("idPrintDih0")) {
        printRtf(QStringLiteral("dih0.rtf"));
        return;
    }
    if (id == QLatin1String("idPrintCorr1")) {
        printRtf(QStringLiteral("cor1.rtf"));
        appendTo(SummaryPanel::Corr, today() + QStringLiteral(" Предоставлены рекомендации по мотивированию пациента к отказу от табака."));
        return;
    }
    if (id == QLatin1String("idnext3") && m_pageName == QLatin1String("3")) {
        QString verdict = valueOf(QStringLiteral("idv"));
        verdict.remove(QStringLiteral("Вывод:"));
        appendTo(SummaryPanel::Risk, today() + QStringLiteral("\nШкала SCORE: 10-летний риск фатальных сердечно - сосудистых осложнений.\n")
                                         + verdict.trimmed());
    }
}

void ClinicalBrowserController::runItog() {
    if (!documentReady()) {
        return;
    }
    if (m_pageName == QLatin1String("1") || m_pageName == QLatin1String("2")) {
        const bool essen = m_pageName == QLatin1String("1");
        const int count = essen ? 8 : 5;
        int score = 0;
        for (int i = 1; i <= count; ++i) {
            const QString value = valueOf(QStringLiteral("ids%1").arg(i));
            if (value.isEmpty()) {
                showError(QStringLiteral("Не все данные внесены"));
                return;
            }
            score += value.toInt();
            if (essen) {
                setInnerHtml(QStringLiteral("idbls%1").arg(i), value);
            }
        }
        setInnerHtml(QStringLiteral("idsum"), QString::number(score));
        QString verdict;
        if (essen) {
            if (score < 3) {
                verdict = QStringLiteral("Вывод: низкий риска повторного инсульта (менее 4% в год)");
            } else if (score <= 6) {
                verdict = QStringLiteral("Вывод: высокий риск повторного инсульта (более 4% в год)");
            } else {
                verdict = QStringLiteral("Вывод: очень высокий риск повторного инсульта.");
            }
        } else if (score <= 3) {
            verdict = QStringLiteral("Вывод: Риск развития инсульта в течение 2 дней: 1.0%<br>"
                                     "Риск развития инсульта в течение 7 дней: 1.2%<br>"
                                     "Риск развития инсульта в течение 90 дней: 3.1%");
        } else if (score <= 5) {
            verdict = QStringLiteral("Вывод: Риск развития инсульта в течение 2 дней: 4.1%<br>"
                                     "Риск развития инсульта в течение 7 дней: 5.9%<br>"
                                     "Риск развития инсульта в течение 90 дней: 9.8%");
        } else {
            verdict = QStringLiteral("Вывод: Риск развития инсульта в течение 2 дней: 8.1%<br>"
                                     "Риск развития инсульта в течение 7 дней: 11.7%<br>"
                                     "Риск развития инсульта в течение 90 дней: 17.8%");
        }
        const QString label = QStringLiteral("Вывод:");
        setInnerHtml(QStringLiteral("idv"), QStringLiteral("<b>%1</b>%2").arg(label, verdict.mid(label.size())));
        return;
    }
    if (m_pageName != QLatin1String("4")) {
        return;
    }
    const bool male = isChecked(QStringLiteral("idpm"));
    const bool female = isChecked(QStringLiteral("idpf"));
    if (!male && !female) {
        showError(QStringLiteral("Выберите пол"));
        return;
    }
    if (!isChecked(QStringLiteral("idbl")) && !isChecked(QStringLiteral("idfl"))) {
        showError(QStringLiteral("Выберите фон лечения"));
        return;
    }
    int score = 0;
    for (int i = 1; i <= 7; ++i) {
        bool ok = false;
        score += innerText(QStringLiteral("idbls%1").arg(i)).trimmed().toInt(&ok);
        if (!ok) {
            showError(QStringLiteral("Не все данные внесены"));
            return;
        }
    }
    setInnerHtml(QStringLiteral("idsum"), QString::number(score));
    const QStringList table = male
        ? QStringLiteral("|2,6|3,0|3,5|4,0|4,7|5,4|6,3|7,3|8,4|9,7|11,2|12,9|14,8|17,0|19,5|22,4|25,5|29,0|32,9|37,1|41,7|46,6|51,8|57,3|62,8|68,4|73,8|79,0|83,7|87,9").split(QLatin1Char('|'))
        : QStringLiteral("1,1|1,3|1,6|2,0|2,4|2,9|3,5|4,3|5,2|6,3|7,6|9,2|11,1|13,3|16,0|19,1|22,8|27,0|31,9|37,3|43,4|50,0|57,0|64,2|71,4|78,2|84,4").split(QLatin1Char('|'));
    const QString risk = table.value(qBound(0, score, table.size() - 1));
    setInnerHtml(QStringLiteral("idv"), QStringLiteral("<b>Вывод: </b> риск повторного инсульта ( %1% в год)").arg(risk));
}

void ClinicalBrowserController::runPlus() {
    if (!documentReady()) {
        return;
    }
    const QString sum = htmlToPlain(innerHtml(QStringLiteral("idsum")));
    if (sum.isEmpty()) {
        showError(QStringLiteral("Сначала нажмите «Итог»."));
        return;
    }
    const QString verdict = htmlToPlain(innerHtml(QStringLiteral("idv")));
    if (m_pageName == QLatin1String("1")) {
        appendTo(SummaryPanel::Risk, today() + QStringLiteral("\nШкала оценки риска повторного инсульта Essen Stroke Risk Score (ESRS)\n")
                                         + sum + QStringLiteral(" балл. ") + verdict);
    } else if (m_pageName == QLatin1String("2")) {
        appendTo(SummaryPanel::Risk, today() + QStringLiteral("\nШкала ABCD2: оценка риска развития инсульта после транзиторной ишемической атаки\n")
                                         + sum + QStringLiteral(" балл. ") + verdict);
    } else if (m_pageName == QLatin1String("4")) {
        appendTo(SummaryPanel::Risk, today() + QStringLiteral("\nФрамингемская шкала: оценка индивидуального риска развития инсульта\n")
                                         + sum + QStringLiteral(" баллов. ") + verdict);
    }
}

#ifndef Q_OS_WIN

ClinicalWebBridge::ClinicalWebBridge(ClinicalBrowserController *host, QObject *parent)
    : QObject(parent)
    , m_host(host) {}

void ClinicalWebBridge::mouseDown(const QString &id, const QString &src, const QString &parentTag,
                                  const QString &parentText, int button) {
    if (m_host) {
        m_host->onWebMouseDown(id, src, parentTag, parentText, button);
    }
}

void ClinicalWebBridge::clicked() {
    if (m_host) {
        m_host->onWebClicked();
    }
}

void ClinicalWebBridge::edited() {
    if (m_host) {
        m_host->onWebEdited();
    }
}

QVariant ClinicalBrowserController::evalJs(const QString &script) const {
    if (!m_web || !m_web->page()) {
        return {};
    }
#ifdef SUNRISE_USE_WEBKIT
    return const_cast<QWebView *>(m_web)->page()->mainFrame()->evaluateJavaScript(script);
#else
    QVariant result;
    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);
    QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    const_cast<QWebEngineView *>(m_web)->page()->runJavaScript(script, [&](const QVariant &value) {
        result = value;
        loop.quit();
    });
    timer.start(4000);
    loop.exec();
    return result;
#endif
}

void ClinicalBrowserController::injectLinuxBridge() {
#ifdef SUNRISE_USE_WEBKIT
    m_web->page()->mainFrame()->addToJavaScriptWindowObject(QStringLiteral("sunriseBridge"), m_bridge);
#endif
    static const QString listeners = QStringLiteral(
        "(function(){"
        "function info(el){"
        "  var t=el,id=(t&&t.id)?t.id:'';"
        "  if(!id){var p=t;while(p&&p!==document.body){if(p.id){id=p.id;break;}p=p.parentElement;}}"
        "  var src=(t&&t.getAttribute)?(t.getAttribute('src')||''):'';"
        "  var parent=t&&t.parentElement;"
        "  return {id:id,src:src,parentTag:parent?parent.tagName:'',"
        "          parentText:parent?(parent.innerText||parent.textContent||''):''};"
        "}"
        "if(!window.__sunriseBound){"
        "  window.__sunriseBound=true;"
        "  document.addEventListener('mousedown',function(e){"
        "    if(e.button!==0&&e.button!==1)return;"
        "    var x=info(e.target);"
        "    if(window.sunriseBridge)sunriseBridge.mouseDown(x.id,x.src,x.parentTag,x.parentText,e.button);"
        "  },true);"
        "  document.addEventListener('click',function(){if(window.sunriseBridge)sunriseBridge.clicked();},true);"
        "  document.addEventListener('keyup',function(){if(window.sunriseBridge)sunriseBridge.edited();},true);"
        "  document.addEventListener('focusout',function(){if(window.sunriseBridge)sunriseBridge.edited();},true);"
        "}"
        "})();");
#ifdef SUNRISE_USE_WEBKIT
    evalJs(listeners);
#else
    static const QString script = QStringLiteral(
        "(function(){"
        "function startChannel(){"
        "  if(!window.qt||!qt.webChannelTransport){setTimeout(startChannel,20);return;}"
        "  new QWebChannel(qt.webChannelTransport,function(channel){"
        "    window.sunriseBridge=channel.objects.bridge;"
        "  });"
        "}"
        "function loadChannel(){"
        "  if(typeof QWebChannel==='function'){startChannel();return;}"
        "  var s=document.createElement('script');"
        "  s.src='qrc:///qtwebchannel/qwebchannel.js';"
        "  s.onload=startChannel;"
        "  document.documentElement.appendChild(s);"
        "}"
        "loadChannel();"
        "})();");
    evalJs(listeners);
    evalJs(script);
#endif
}

void ClinicalBrowserController::onLoadStarted() {
    m_pageReady = false;
    saveTemplates();
}

void ClinicalBrowserController::onUrlChanged(const QUrl &url) {
    const QString address = url.toString();
    if (address.isEmpty() || address == QLatin1String("about:blank")) {
        return;
    }
    applyPageAddress(address);
}

void ClinicalBrowserController::onLoadFinished(bool ok) {
    m_pageReady = ok;
    if (!ok) {
        return;
    }
    injectLinuxBridge();
    finishDocumentLoad();
    QTimer::singleShot(150, this, [this, address = m_address]() {
        if (m_address == address) {
            injectLinuxBridge();
        }
    });
}

void ClinicalBrowserController::onWebMouseDown(const QString &id, const QString &src, const QString &parentTag,
                                               const QString &parentText, int button) {
    if (button != 0 && button != 1) {
        return;
    }
    if (m_section == Section::Speech) {
        handleSpeechClick(id, src, parentText);
        return;
    }
    handleClick(id, parentTag);
}

void ClinicalBrowserController::onWebClicked() {
    flushPageAlerts();
}

void ClinicalBrowserController::onWebEdited() {
    m_templateSaveTimer->start();
}

#endif
