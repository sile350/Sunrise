#include "exercisehost.h"

#include "approotpaths.h"
#include "coverpanel.h"
#include "custommessagebox.h"
#include "engines/exengine.h"
#include "engines/movewordsengine.h"
#include "engines/mpicturesengine.h"
#include "engines/rtfcoverengine.h"
#include "engines/storiesengine.h"
#include "engines/wolfengine.h"
#include "imagebutton.h"
#include "patientwindow.h"
#include "protocoleditor.h"
#include "repository.h"
#include "scoringform.h"

#include <QAbstractTextDocumentLayout>
#include <QCheckBox>
#include <QComboBox>
#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QRadioButton>
#include <QScreen>
#include <QTimer>
#include <QWindow>
#include <QRegularExpression>
#include <QScrollArea>
#include <QSlider>
#include <QTextBrowser>
#include <QTextEdit>
#include <QTextStream>
#include <QUrl>
#include <QVBoxLayout>

namespace {

constexpr int kLeftWidth = 850;
constexpr int kCoverX = 860;
// Заголовок упражнения убран (название уже в ссылке описания) — контент от верхнего края.
constexpr int kTopBar = 0;

QString readTextFile(const QString &path) {
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

QString for123File(const QString &patientId) {
    return AppRootPaths::dataDir() + QStringLiteral("/for123/") + patientId + QStringLiteral(".txt");
}

} // namespace

ExerciseHost::ExerciseHost(const ExerciseRuntime &runtime, const ExerciseContext &context, QWidget *parent)
    : QWidget(parent), m_rt(runtime), m_ctx(context) {
    setAutoFillBackground(true);
    QPalette pal = palette();
    pal.setColor(QPalette::Window, QColor(249, 249, 249));
    setPalette(pal);
    setObjectName(QStringLiteral("exerciseHost"));

    m_title = nullptr;

    m_patientScreenBtn = new QPushButton(QStringLiteral("Экран пациента"), this);
    m_patientScreenBtn->setToolTip(QStringLiteral("Вернуться к выполнению упражнения на экране пациента"));
    m_patientScreenBtn->setCursor(Qt::PointingHandCursor);
    m_patientScreenBtn->setStyleSheet(QStringLiteral(
        "QPushButton { font-family: 'Microsoft Sans Serif'; font-size: 11pt; font-weight: bold; color: white;"
        " background: #c8641e; border: 1px solid #8a3f0c; border-radius: 4px; padding: 3px 14px; }"
        "QPushButton:hover { background: #d97a33; }"));
    m_patientScreenBtn->adjustSize();
    m_patientScreenBtn->move(1810 - m_patientScreenBtn->width(), 4);
    m_patientScreenBtn->hide();
    connect(m_patientScreenBtn, &QPushButton::clicked, this, &ExerciseHost::presentPatient);
    connect(qGuiApp, &QGuiApplication::screenAdded, this, [this]() { representPatient(); });
    connect(qGuiApp, &QGuiApplication::screenRemoved, this, [this]() {
        QTimer::singleShot(0, this, &ExerciseHost::representPatient);
    });

    if (m_rt.coverKind == ExCoverKind::C114) {
        m_print = new ImageButton(this);
        m_print->setImagePath(ExAssets::resourceImage(QStringLiteral("Печать (1).png")));
        m_print->setToolTip(QStringLiteral("Печать"));
        m_print->setGeometry(1821, 2, 36, 34);
        m_print->setCursor(Qt::PointingHandCursor);
        connect(m_print, &ImageButton::clicked, this, [this]() { openExternal(QStringLiteral("1.1.4/Ответы_на_письменные_вопросы.rtf")); });
    }

    m_cover = new CoverPanel(m_rt.coverKind, this);
    buildLeftColumn();
    buildEngines();
    layoutCover();
    wireCover();
    loadLeftHtml();
    updateVisibility();
    m_ready = true;
    applyParams(true);
    attachEngineToWindows();
}

ExerciseHost::~ExerciseHost() {
    shutdown();
    if (m_patient) {
        delete m_patient;
        m_patient = nullptr;
    }
}

void ExerciseHost::shutdown() {
    if (m_protocolView) {
        m_protocolView->flushEdits();
    }
    ExAssets::playMetronome(false, 0);
    m_metroOn = false;
    m_doctorPanel = false;
    if (m_patient) {
        m_patient->hideDisplay();
    }
}

// ---------------- левая колонка ----------------

void ExerciseHost::buildLeftColumn() {
    m_leftScroll = new QScrollArea(this);
    m_leftScroll->setGeometry(0, kTopBar, kLeftWidth, height() > 100 ? height() - kTopBar : 925);
    m_leftScroll->setFrameShape(QFrame::NoFrame);
    m_leftScroll->setWidgetResizable(true);
    m_leftScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_leftScroll->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    m_leftScroll->setStyleSheet(QStringLiteral("QScrollArea { background: #f9f9f9; border: none; }"));
    auto *inner = new QWidget();
    inner->setStyleSheet(QStringLiteral("background: #f9f9f9;"));
    auto *lay = new QVBoxLayout(inner);
    lay->setContentsMargins(24, 10, 10, 10);
    lay->setSpacing(10);
    // Фон как у остальной области упражнения (#f9f9f9), не отдельное «поле» #f8f8f8.
    const QString browserStyle = QStringLiteral("QTextBrowser { background: #f9f9f9; border: none; }");
    m_or = new QTextBrowser(inner);
    m_or->setOpenLinks(false);
    m_or->setOpenExternalLinks(false);
    m_or->setFrameShape(QFrame::NoFrame);
    m_or->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_or->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_or->setStyleSheet(browserStyle);
    m_or->setFont(ExProtocol::textFont());
    lay->addWidget(m_or);
    connect(m_or->document()->documentLayout(), &QAbstractTextDocumentLayout::documentSizeChanged, this,
            [this](const QSizeF &s) { m_or->setFixedHeight(int(s.height()) + 12); });

    m_form = new ScoringForm(inner);
    m_form->setObjectName(QStringLiteral("scoringForm"));
    lay->addWidget(m_form);

    // exbegin.Designer: pformp 196x35 по центру под формой оценки.
    m_formBtn = new ImageButton(inner);
    m_formBtn->setObjectName(QStringLiteral("pformp"));
    m_formBtn->setImagePath(ExAssets::sysImage(QStringLiteral("formp.png")));
    m_formBtn->setFixedSize(196, 35);
    m_formBtn->setCursor(Qt::PointingHandCursor);
    lay->addWidget(m_formBtn, 0, Qt::AlignHCenter);
    connect(m_formBtn, &ImageButton::clicked, this, &ExerciseHost::onFormProtocol);

    // До первого формирования здесь пустой протокол из template.html, затем сам протокол.
    m_protocolView = new ProtocolEditor(inner);
    m_protocolView->setObjectName(QStringLiteral("protocolView"));
    m_protocolView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_protocolView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    lay->addWidget(m_protocolView);
    connect(m_protocolView, &ProtocolEditor::entriesEdited, this, &ExerciseHost::saveProtocolEdits);
    connect(m_protocolView->document()->documentLayout(), &QAbstractTextDocumentLayout::documentSizeChanged, this,
            [this](const QSizeF &) { m_protocolView->setFixedHeight(m_protocolView->contentHeight()); });

    // bsum под правым краем таблицы протокола.
    auto *itogRow = new QHBoxLayout();
    itogRow->setContentsMargins(0, 0, 0, 0);
    m_itogBtn = new ImageButton(inner);
    m_itogBtn->setObjectName(QStringLiteral("bsum"));
    m_itogBtn->setImagePath(ExAssets::sysImage(QStringLiteral("itog.png")));
    m_itogBtn->setFixedSize(154, 34);
    m_itogBtn->setCursor(Qt::PointingHandCursor);
    m_itogBtn->hide();
    itogRow->addSpacing(ExProtocol::kTableWidth + 4 - m_itogBtn->width());
    itogRow->addWidget(m_itogBtn);
    itogRow->addStretch(1);
    lay->addLayout(itogRow);
    connect(m_itogBtn, &ImageButton::clicked, this, &ExerciseHost::onItog);

    // Запас прокрутки под панелью задач Windows, чтобы домотать до низа протокола / «Подвести итог».
    auto *bottomPad = new QWidget(inner);
    bottomPad->setFixedHeight(240);
    bottomPad->setStyleSheet(QStringLiteral("background: transparent;"));
    lay->addWidget(bottomPad);
    m_leftScroll->setWidget(inner);

    connect(m_or, &QTextBrowser::anchorClicked, this, [this](const QUrl &url) {
        const QString frag = url.fragment();
        if (frag == QLatin1String("method")) {
            m_open[0] = !m_open[0];
        } else if (frag == QLatin1String("procedure")) {
            m_open[1] = !m_open[1];
        } else if (frag == QLatin1String("analis")) {
            m_open[2] = !m_open[2];
        } else {
            return;
        }
        m_or->setHtml(composeOrHtml());
        refreshLeftSizes();
    });
}

void ExerciseHost::loadLeftHtml() {
    const QString dir = ExAssets::exnPath(m_rt.htmlFolder);
    // or.html: описание до <hr>, после — форма оценки (поля idch*/idd*/idp*).
    const QString orHtml = readTextFile(dir + QStringLiteral("/or.html"));
    static const QRegularExpression hrRe(QStringLiteral("<hr\\b[^>]*>"), QRegularExpression::CaseInsensitiveOption);
    const QRegularExpressionMatch hr = hrRe.match(orHtml);
    if (hr.hasMatch()) {
        m_orRaw = orHtml.left(hr.capturedEnd()) + QStringLiteral("</body></html>");
        m_form->load(orHtml.mid(hr.capturedEnd()));
    } else {
        m_orRaw = orHtml;
        m_form->load(QString());
    }
    m_or->setSearchPaths({dir});
    m_or->setHtml(composeOrHtml());
    m_protocolView->showEntries(QString(), {ExProtocol::templateEntry(m_rt.htmlFolder + QStringLiteral("/template.html"))},
                                false);
    refreshLeftSizes();
}

QString ExerciseHost::composeOrHtml() const {
    QString h = m_orRaw;
    // Фон body как у панели упражнения.
    h.replace(QRegularExpression(QStringLiteral("bgcolor\\s*=\\s*['\"][^'\"]*['\"]"),
                                 QRegularExpression::CaseInsensitiveOption),
              QStringLiteral("bgcolor=\"#f9f9f9\""));
    static const char *ids[3] = {"method", "procedure", "analis"};
    for (int i = 0; i < 3; ++i) {
        const QRegularExpression re(QStringLiteral("<a\\s+id=['\"]%1['\"][^>]*>").arg(QLatin1String(ids[i])),
                                    QRegularExpression::CaseInsensitiveOption);
        // Как в оригинале: заголовки ссылок подчёркнуты.
        h.replace(re, QStringLiteral("<a href=\"#%1\" style=\"text-decoration:underline;color:#000000;font-size:16px\">")
                          .arg(QLatin1String(ids[i])));
    }
    for (int i = 2; i >= 0; --i) {
        const QRegularExpression re(QStringLiteral("<div\\s+id=['\"]div%1['\"][^>]*>").arg(i + 1),
                                    QRegularExpression::CaseInsensitiveOption);
        const QRegularExpressionMatch m = re.match(h);
        if (!m.hasMatch()) {
            continue;
        }
        const int start = m.capturedStart();
        int end = -1;
        if (i == 0) {
            end = h.indexOf(QStringLiteral("<a href=\"#procedure\""), start);
        } else if (i == 1) {
            end = h.indexOf(QStringLiteral("<a href=\"#analis\""), start);
        } else {
            // Не забирать <hr> вместе с блоком «Оценка результата» — линия должна быть всегда.
            end = h.indexOf(QStringLiteral("<hr"), start, Qt::CaseInsensitive);
            if (end < 0) {
                end = h.lastIndexOf(QStringLiteral("</body>"), -1, Qt::CaseInsensitive);
            }
        }
        if (end < 0) {
            end = h.size();
        }
        if (m_open[i]) {
            h.replace(start, m.capturedLength(), QStringLiteral("<div style=\"margin:0\">"));
        } else {
            h.remove(start, end - start);
        }
    }
    // Каждая ссылка — отдельный абзац от левого края: в QTextBrowser свёрнутые div не дают переноса строки.
    for (int i = 0; i < 3; ++i) {
        const QString open = QStringLiteral("<a href=\"#%1\"").arg(QLatin1String(ids[i]));
        const int a = h.indexOf(open);
        if (a < 0) {
            continue;
        }
        const int close = h.indexOf(QStringLiteral("</a>"), a, Qt::CaseInsensitive);
        if (close < 0) {
            continue;
        }
        h.insert(close + 4, QStringLiteral("</p>"));
        h.insert(a, QStringLiteral("<p style=\"margin:0 0 6px 0;text-indent:0\">"));
    }
    // Гарантировать горизонтальную линию после ссылок (даже если все блоки свёрнуты).
    if (!h.contains(QStringLiteral("<hr"), Qt::CaseInsensitive)) {
        const int bodyEnd = h.lastIndexOf(QStringLiteral("</body>"), -1, Qt::CaseInsensitive);
        if (bodyEnd >= 0) {
            h.insert(bodyEnd, QStringLiteral("<hr />\n"));
        }
    }
    return h;
}

void ExerciseHost::refreshLeftSizes() {
    const int w = m_leftScroll->viewport()->width() - 34;
    m_or->document()->setTextWidth(qMax(300, w));
    m_or->setFixedHeight(int(m_or->document()->size().height()) + 12);
    if (m_protocolView) {
        m_protocolView->document()->setTextWidth(qMax(300, w));
        m_protocolView->setFixedHeight(m_protocolView->contentHeight());
    }
}

// _122/_226/_324 при смене режима переписывают idvz: стандартные баллы или пусто.
void ExerciseHost::updateScoreBlock() {
    if (!m_form->hasScoreBlock()) {
        return;
    }
    int filled = -1;
    switch (m_rt.coverKind) {
    case ExCoverKind::C122: filled = m_cover->combo1->currentIndex() != 0 ? 1 : 0; break;
    case ExCoverKind::C324: filled = m_cover->combo1->currentIndex() == 1 ? 1 : 0; break;
    case ExCoverKind::C226: filled = m_cover->combo1->currentIndex() == 1 ? 0 : 1; break;
    default: return;
    }
    if (filled == m_scoreMode) {
        return;
    }
    // 2.2.6: при первом открытии остаётся idvz из or.html.
    const bool keepInitial = m_scoreMode < 0 && filled == 1 && m_rt.coverKind == ExCoverKind::C226;
    m_scoreMode = filled;
    if (!keepInitial) {
        m_form->setScoreBlock(filled == 1);
    }
    refreshLeftSizes();
}

void ExerciseHost::showLeft(bool on) {
    m_leftScroll->setVisible(on);
}

// ---------------- движки ----------------

void ExerciseHost::buildEngines() {
    m_blank = new ExEngine(this);
    switch (m_rt.engineKind) {
    case ExEngineKind::Mpictures:
        m_mp = new MPicturesEngine(this);
        m_active = m_mp;
        connect(m_mp, &MPicturesEngine::resultRecorded, this, &ExerciseHost::onResult);
        break;
    case ExEngineKind::Movewords:
        m_mw = new MovewordsEngine(this);
        m_active = m_mw;
        connect(m_mw, &MovewordsEngine::resultRecorded, this, &ExerciseHost::onResult);
        if (m_rt.coverKind == ExCoverKind::C225) {
            m_wolf = new WolfEngine(this);
            connect(m_wolf, &WolfEngine::resultRecorded, this, &ExerciseHost::onResult);
        }
        break;
    case ExEngineKind::Stories:
        m_st = new StoriesEngine(this);
        m_active = m_st;
        break;
    default:
        break;
    }
    if (m_active && m_cover->mirror) {
        m_cover->mirror->setEngine(m_active);
    }
    m_patient = new PatientWindow();
    connect(m_patient, &PatientWindow::closedByUser, this, [this]() {
        if (m_running) {
            onStop();
        }
    });
    connect(m_patient, &PatientWindow::doctorPanelRequested, this, &ExerciseHost::showDoctorPanel);
}

void ExerciseHost::attachEngineToWindows() {
    if (m_cover->mirror) {
        m_cover->mirror->setEngine(m_active);
    }
    if (m_patient) {
        const bool show = !(m_rt.coverKind == ExCoverKind::C123 && m_cover->showCheck &&
                            !static_cast<QCheckBox *>(m_cover->showCheck)->isChecked());
        m_patient->setEngine(show ? m_active : m_blank);
        const bool about = m_rt.coverKind == ExCoverKind::C226 && m_cover->combo1->currentText() == QLatin1String("3");
        m_patient->setRtf(about ? ExAssets::exnPath(QStringLiteral("2.2.6/about.rtf")) : QString());
    }
}

// ---------------- раскладка ----------------

void ExerciseHost::layoutCover() {
    const int h = qMax(200, (height() > 100 ? height() : 965) - kTopBar);
    m_cover->setGeometry(kCoverX, kTopBar, 1040, h);
    int top = 150;
    int bottomPad = 10;
    switch (m_rt.coverKind) {
    case ExCoverKind::C112: top = 135; break;
    case ExCoverKind::C114: top = 100; break;
    case ExCoverKind::C121: top = 362; break; // под showp/updateR (Top 326, высота 34)
    case ExCoverKind::C122: top = 250; break;
    case ExCoverKind::C123: top = 70; break;
    case ExCoverKind::C211:
    case ExCoverKind::C212:
    case ExCoverKind::C221:
    case ExCoverKind::C222:
    case ExCoverKind::C226: top = 100; break;
    case ExCoverKind::C213: top = 100; break;
    case ExCoverKind::C214: top = 330; break;
    case ExCoverKind::C225: top = 205; bottomPad = 130; break;
    case ExCoverKind::C323: top = 200; break;
    case ExCoverKind::C324: top = 250; break;
    default: top = 150; break;
    }
    const int avail = h - top - bottomPad;
    const int w = qMin(1020, avail * 16 / 9);
    m_cover->setMirrorGeometry(QRect(10, top, w, w * 9 / 16));
    if (m_leftScroll) {
        m_leftScroll->setGeometry(0, kTopBar, kLeftWidth, h);
    }
    if (m_rt.engineKind == ExEngineKind::None) {
        m_cover->mirror->hide();
    }
}

void ExerciseHost::resizeEvent(QResizeEvent *event) {
    QWidget::resizeEvent(event);
    if (m_cover) {
        layoutCover();
        refreshLeftSizes();
    }
}

// ---------------- обложка ----------------

void ExerciseHost::wireCover() {
    CoverPanel *c = m_cover;
    auto bind = [this](ImageButton *b, void (ExerciseHost::*slot)()) {
        if (b) {
            connect(b, &ImageButton::clicked, this, slot);
        }
    };
    bind(c->beginu, &ExerciseHost::onBegin);
    bind(c->stopu, &ExerciseHost::onStop);
    bind(c->nextz, &ExerciseHost::onNext);
    bind(c->bagain, &ExerciseHost::onAgain);
    // _121.updateR_Click — пересобрать ряд с теми же параметрами.
    if (c->updateR) {
        connect(c->updateR, &ImageButton::clicked, this, [this]() { applyParams(true); });
    }
    for (ImageButton *b : {c->showp, c->explainBtn, c->showStory}) {
        bind(b, &ExerciseHost::onShowp);
    }
    if (c->dcorrect) {
        connect(c->dcorrect, &ImageButton::clicked, this, [this]() { onDoctorMark(false); });
    }
    if (c->dincorrect) {
        connect(c->dincorrect, &ImageButton::clicked, this, [this]() { onDoctorMark(true); });
    }
    for (QComboBox *cb : {c->combo1, c->combo2, c->combo3, c->combo4}) {
        if (cb) {
            connect(cb, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this, cb](int) {
                if (m_ready) {
                    onParamChanged(cb);
                }
            });
        }
    }
    // 1.1.2: переключатель показать/подписать
    if (c->modePic) {
        connect(c->modePic, &ImageButton::clicked, this, [this]() {
            m_modeTag = m_modeTag == QLatin1String("1") ? QStringLiteral("2") : QStringLiteral("1");
            m_cover->modePic->setImagePath(ExAssets::sysImage(m_modeTag == QLatin1String("1") ? QStringLiteral("ppod.png") : QStringLiteral("skrp.png")));
            // _112.pictureBox1_Click: те же картинки (doctorInit берёт имена из прежних спрайтов), выбор врача сохраняется
            applyParams(false);
            if (m_running) {
                attachEngineToWindows();
            }
        });
    }
    // 2.1.4: выбор союза
    for (int i = 0; i < 5; ++i) {
        if (c->conj[i]) {
            connect(c->conj[i], &ImageButton::clicked, this, [this, i]() {
                static const QStringList words = {QStringLiteral(" потому что"), QStringLiteral(" несмотря на то, что"),
                                                  QStringLiteral(" так как"), QStringLiteral(" поэтому"), QStringLiteral(" так что")};
                m_cover->selectConjunction(i);
                m_mw->p1 = words[i];
                m_mw->rebuild(true);
                syncDoctorLabels();
            });
        }
    }
    // 1.2.1: метроном
    if (c->metroBtn) {
        connect(c->metroBtn, &ImageButton::clicked, this, [this]() {
            m_metroOn = !m_metroOn;
            m_cover->metroBtn->setImagePath(ExAssets::exnPath(m_metroOn ? QStringLiteral("1.2.1/pause.png") : QStringLiteral("1.2.1/metro.png")));
            const int bpm = 30 + (m_cover->metroSlider->value() - 1) * 22;
            ExAssets::playMetronome(m_metroOn, bpm);
        });
        connect(c->metroSlider, &QSlider::valueChanged, this, [this](int v) {
            if (m_metroOn) {
                ExAssets::playMetronome(true, 30 + (v - 1) * 22);
            }
        });
    }
    // 1.1.4 / 1.1.1: форматирование и вопрос
    if (c->underlineBtn) {
        connect(c->underlineBtn, &ImageButton::clicked, this, [this]() { RtfCover::toggleUnderline(m_cover->rtf); });
    }
    if (c->boldBtn) {
        connect(c->boldBtn, &ImageButton::clicked, this, [this]() { RtfCover::toggleBold(m_cover->rtf); });
    }
    if (c->rtf && c->question) {
        connect(c->rtf, &QTextEdit::selectionChanged, this, [this]() {
            const QString sel = m_cover->rtf->textCursor().selectedText().trimmed();
            if (!sel.isEmpty()) {
                m_cover->question->setText(sel);
            }
        });
    }
    if (c->showQuestion) {
        connect(c->showQuestion, &ImageButton::clicked, this, [this]() {
            QString q = m_cover->question->text();
            if (q == QStringLiteral("Написать вопрос")) {
                q.clear();
            }
            m_mp->questionText = q;
            m_mp->rebuild(false);
        });
    }
    if (c->showCheck) {
        connect(static_cast<QCheckBox *>(c->showCheck), &QCheckBox::toggled, this, [this](bool) { attachEngineToWindows(); });
    }
    if (c->rtf && m_rt.coverKind == ExCoverKind::C1) {
        RtfCover::loadInto(c->rtf, ExAssets::exnPath(QStringLiteral("1.1.1/text.rtf")));
    }
    if (c->rtf && m_rt.coverKind == ExCoverKind::C114) {
        RtfCover::loadInto(c->rtf, ExAssets::exnPath(QStringLiteral("1.1.4/ans1.rtf")));
    }
    if (c->rtf && m_rt.coverKind == ExCoverKind::C226) {
        RtfCover::loadInto(c->rtf, ExAssets::exnPath(QStringLiteral("2.2.6/about.rtf")));
    }
}

// Видимость элементов обложки: Visible из *.Designer.cs, beginu_Click / stopu_Click и смена режимов.
void ExerciseHost::updateVisibility() {
    CoverPanel *c = m_cover;
    const bool run = m_running;
    const auto vis = [](QWidget *w, bool on) {
        if (w) {
            w->setVisible(on);
        }
    };
    switch (m_rt.coverKind) {
    case ExCoverKind::C112:
        vis(c->label1, !run);
        vis(c->combo1, !run);
        vis(c->bagain, run);
        break;
    case ExCoverKind::C114: {
        const bool written = c->combo1->currentText() == QLatin1String("2");
        c->rtf->setVisible(written);
        c->question->setVisible(written);
        c->showQuestion->setVisible(written);
        c->underlineBtn->setVisible(written);
        c->boldBtn->setVisible(written);
        c->hintLabel->setVisible(!written);
        c->mirror->setVisible(!written);
        c->nextz->setVisible(!written);
        m_print->setVisible(written);
        break;
    }
    case ExCoverKind::C211:
    case ExCoverKind::C223:
    case ExCoverKind::C224:
    case ExCoverKind::C311:
        vis(c->bagain, run);
        break;
    case ExCoverKind::C214:
        // _214.stopu_Click не прячет right/unright: оценивать можно и после «Стоп»
        vis(c->dcorrect, m_begun);
        vis(c->dincorrect, m_begun);
        break;
    case ExCoverKind::C221:
    case ExCoverKind::C222:
        vis(c->combo1, !run);
        break;
    case ExCoverKind::C312:
        // _312.beginu_Click: комбобокс прячется только для data1 (3.1.2), в 3.2.2 (data2) остаётся
        vis(c->combo1, !run || m_rt.dataNumber != 1);
        vis(c->bagain, run);
        break;
    case ExCoverKind::C226: {
        const QString mode = c->combo1->currentText();
        vis(c->combo1, !run);
        vis(c->lcounter, mode == QLatin1String("2"));
        vis(c->lcorrect, mode == QLatin1String("2"));
        vis(c->bagain, mode == QLatin1String("2"));
        vis(c->nextz, mode != QLatin1String("3"));
        vis(c->rtf, mode == QLatin1String("3"));
        c->mirror->setVisible(mode != QLatin1String("3"));
        break;
    }
    case ExCoverKind::C121: {
        const bool repro = c->combo1->currentIndex() == 1;
        c->combo2->setVisible(!repro);
        c->label2->setVisible(!repro);
        c->combo4->setVisible(!repro);
        c->label4->setVisible(!repro);
        c->metroPanel->setVisible(repro);
        c->updateR->setVisible(!repro);
        c->showp->setVisible(!repro && c->combo3->currentText() == QStringLiteral("Цвета радуги"));
        break;
    }
    case ExCoverKind::C123: {
        const QString mode = c->combo2->currentText();
        const bool cards = mode == QLatin1String("2");
        c->combo1->setVisible(cards);
        c->label1->setVisible(cards);
        c->combo2->setVisible(!run);
        c->nextz->setVisible(cards);
        c->dataBox->raise();
        c->showCheck->setVisible(mode != QLatin1String("3"));
        c->dataBox->setVisible(mode == QLatin1String("1"));
        c->mirror->setVisible(mode != QLatin1String("3"));
        if (mode == QLatin1String("1")) {
            c->fioLabel->setText(QStringLiteral("ФИО: ") + m_ctx.patientFio);
            const QStringList lines = QFileInfo::exists(for123File(m_ctx.patientId))
                                          ? readTextFile(for123File(m_ctx.patientId)).split(QLatin1Char('\n'))
                                          : QStringList();
            if (lines.size() > 2) {
                c->address->setText(lines[1].trimmed());
                c->phone->setText(lines[2].trimmed());
            }
        }
        break;
    }
    case ExCoverKind::C225: {
        // ltext/lob/label2 на обложке видны в «Текстовых заданиях»; bob (объяснение) — после «Начать».
        const bool text = c->combo1->currentText() == QStringLiteral("Текстовые задания");
        const bool wolfMode = c->combo1->currentText() == QStringLiteral("Головоломка");
        c->taskLabel->setVisible(text);
        c->explainCaption->setVisible(text);
        c->explainLabel->setVisible(text);
        // В графических заданиях bob скрыт на задании 1 (у 11.png нет версии с «o»): _225.nextz_Click / beginu_Click_1
        const bool graphicFirst = !text && !wolfMode && (m_ex - 1) % 4 == 0;
        c->explainBtn->setVisible(run && !wolfMode && !graphicFirst);
        c->nextz->setVisible(!wolfMode);
        c->bagain->setVisible(run);
        break;
    }
    case ExCoverKind::C323:
    case ExCoverKind::C324: {
        // showp_Click: right/unright/showp скрываются, bagain встаёт на место showp; nextz_Click — обратно.
        const bool written = m_rt.coverKind == ExCoverKind::C324 && c->combo1->currentIndex() == 1;
        const bool hints = m_mw && m_mw->showHints;
        if (m_rt.coverKind == ExCoverKind::C324) {
            vis(c->combo1, !run);
            vis(c->nextz, !written);
            vis(c->lcounter, !written);
            vis(c->lcorrect, !written);
            c->mirror->setVisible(!written);
        }
        vis(c->showp, run && !written && !hints);
        vis(c->dcorrect, run && !written && !hints);
        vis(c->dincorrect, run && !written && !hints);
        c->bagain->move(c->bagain->x(), c->showp->y());
        vis(c->bagain, run && !written && hints);
        break;
    }
    case ExCoverKind::C122: {
        // lcounter/lcorrect/dcorrect/dincorrect/bagain — Visible = false до «Начать» в режиме «Устно».
        const bool oral = c->combo1->currentIndex() == 0;
        vis(c->combo1, !run);
        vis(c->label1, !run);
        vis(c->combo2, oral);
        vis(c->label2, oral);
        vis(c->showp, oral);
        vis(c->nextz, oral);
        vis(c->dcorrect, run && oral);
        vis(c->dincorrect, run && oral);
        vis(c->bagain, run && oral);
        vis(c->lcounter, oral && m_begun);
        vis(c->lcorrect, oral && m_begun);
        c->mirror->setVisible(oral);
        break;
    }
    default:
        break;
    }
    updateScoreBlock();
    updatePatientScreenButton();
}

bool ExerciseHost::isWrittenMode() const {
    const CoverPanel *c = m_cover;
    switch (m_rt.coverKind) {
    case ExCoverKind::C122: return c->combo1->currentIndex() != 0;
    case ExCoverKind::C324: return c->combo1->currentIndex() == 1;
    case ExCoverKind::C123: return c->combo2->currentText() == QLatin1String("3");
    case ExCoverKind::C114: return c->combo1 && c->combo1->currentText() == QLatin1String("2");
    case ExCoverKind::C226: return c->combo1 && c->combo1->currentText() == QLatin1String("3");
    default: return false;
    }
}

bool ExerciseHost::usesPatientWindow() const {
    // Как в оригинале: отдельный экран пациента только там, где есть uprSpace / зеркало.
    // Текстовые/письменные режимы (инструкции 1.1.1, беседа ур.2, письменные варианты и т.п.) — только у врача.
    if (m_rt.coverKind == ExCoverKind::C1 || m_rt.engineKind == ExEngineKind::None) {
        return false;
    }
    if (isWrittenMode()) {
        return false;
    }
    return true;
}

// ---------------- параметры → движок ----------------

void ExerciseHost::applyParams(bool fresh) {
    CoverPanel *c = m_cover;
    switch (m_rt.coverKind) {
    case ExCoverKind::C112:
        m_mp->runtime = QStringLiteral("1.1.2");
        m_mp->folder = m_rt.assetFolder;
        m_mp->p1 = m_modeTag;
        m_mp->p2 = c->combo1->currentText();
        m_mp->rebuild(fresh);
        c->modeLabel->setText(m_rt.assetFolder == QLatin1String("1.1.3")
                                  ? QStringLiteral("Подпишите предметные картинки")
                                  : (m_modeTag == QLatin1String("1") ? QStringLiteral("Покажите сюжетные картинки")
                                                                    : QStringLiteral("Подпишите сюжетные картинки")));
        c->modeLabel->adjustSize();
        break;
    case ExCoverKind::C114:
        m_mp->runtime = QStringLiteral("1.1.4");
        m_mp->folder = QStringLiteral("1.1.4");
        m_mp->p1 = c->combo1->currentText();
        m_mp->currentEx = m_ex;
        m_mp->rebuild(fresh);
        break;
    case ExCoverKind::C121:
        m_mp->runtime = QStringLiteral("1.2.1");
        m_mp->folder = QStringLiteral("1.2.1");
        m_mp->p1 = c->combo1->currentText();
        m_mp->p2 = c->combo2->currentText() + QLatin1Char(';') + c->combo3->currentText() + QLatin1Char(';') + c->combo4->currentText();
        m_mp->rebuild(fresh);
        break;
    case ExCoverKind::C123:
        m_mp->runtime = QStringLiteral("1.2.3");
        m_mp->folder = QStringLiteral("1.2.3");
        m_mp->p1 = c->combo1->currentText();
        m_mp->p2 = c->combo2->currentText();
        m_mp->currentEx = m_ex;
        m_mp->fio = m_ctx.patientFio;
        m_mp->address = c->address->text();
        m_mp->phone = c->phone->text();
        m_mp->rebuild(fresh);
        break;
    case ExCoverKind::C221:
        m_mp->runtime = QStringLiteral("2.2.1");
        m_mp->folder = QStringLiteral("2.2.1");
        m_mp->currentEx = c->combo1->currentIndex() + 1;
        m_mp->rebuild(fresh);
        break;
    case ExCoverKind::C222:
        m_st->storyIndex = c->combo1->currentIndex() + 1;
        m_st->rebuild();
        break;
    default:
        if (m_mw) {
            m_mw->runtime = m_rt.runtimeId;
            m_mw->dataNumber = m_rt.dataNumber;
            m_mw->currentEx = m_ex;
            switch (m_rt.coverKind) {
            case ExCoverKind::C122:
                m_mw->p1 = m_modeTag;
                m_mw->p2 = c->combo2->currentText();
                break;
            case ExCoverKind::C212:
                m_mw->p1 = c->combo1->currentText();
                break;
            case ExCoverKind::C214:
                if (m_mw->p1.isEmpty()) {
                    m_mw->p1 = QStringLiteral(" потому что");
                }
                break;
            case ExCoverKind::C225:
                m_mw->p1 = c->combo1->currentText();
                break;
            case ExCoverKind::C226:
                m_mw->p1 = c->combo1->currentText();
                break;
            case ExCoverKind::C312:
                m_mw->p1 = c->combo1->currentIndex() == 0 ? QStringLiteral("1") : QStringLiteral("2");
                break;
            case ExCoverKind::C323:
                m_mw->p1 = c->combo1->currentText();
                m_mw->p2 = c->combo2->currentText();
                break;
            default:
                break;
            }
            if (m_rt.coverKind == ExCoverKind::C225 && c->combo1->currentText() == QStringLiteral("Головоломка")) {
                m_active = m_wolf;
                m_wolf->rebuild();
            } else {
                m_active = m_mw;
                m_mw->rebuild(fresh);
                if (m_rt.coverKind == ExCoverKind::C225) {
                    // _225.cs: nextz/bagain/смена режима возвращают кнопку bob в состояние exp.png
                    if (fresh) {
                        m_explainOn = false;
                        c->explainBtn->setImagePath(ExAssets::sysImage(QStringLiteral("exp.png")));
                    }
                    m_mw->setExplain(m_explainOn);
                }
            }
            attachEngineToWindows();
        }
        break;
    }
    syncDoctorLabels();
}

void ExerciseHost::syncDoctorLabels() {
    CoverPanel *c = m_cover;
    if (m_rt.coverKind == ExCoverKind::C114 && m_mp) {
        // _114: вопросы 1–3 — Bold 10, ширина 800 с переносом (3-й с Left = 45); 4–6 — Bold 11 в одну строку.
        const int q = qBound(1, m_mp->currentEx, 6);
        QFont f = c->hintLabel->font();
        f.setPointSize(q <= 3 ? 10 : 11);
        c->hintLabel->setFont(f);
        c->hintLabel->setFixedWidth(q <= 3 ? 800 : 940);
        c->hintLabel->move(q == 3 ? 45 : 40, 55);
        c->hintLabel->setText(m_mp->doctorHint());
    }
    if (m_rt.coverKind == ExCoverKind::C225 && m_mw) {
        c->taskLabel->setText(m_mw->taskText);
        c->explainLabel->setText(m_mw->explanationText);
    }
    updateCounters();
}

void ExerciseHost::updateCounters() {
    m_cover->setCounters(m_ecounter, m_ecorrect);
}

void ExerciseHost::onParamChanged(QComboBox *source) {
    CoverPanel *c = m_cover;
    switch (m_rt.coverKind) {
    case ExCoverKind::C122:
        // _122: номер задания сбрасывает только стимульный материал (comboBox2), «Выполнение» — нет
        if (source == c->combo2) {
            m_ex = 1;
        }
        m_modeTag = QStringLiteral("1");
        if (c->showp) {
            c->showp->setImagePath(ExAssets::sysImage(QStringLiteral("showp.png")));
        }
        break;
    case ExCoverKind::C225:
        m_ex = 1;
        break;
    case ExCoverKind::C123:
        break;
    case ExCoverKind::C121:
        m_hintShown = false;
        break;
    case ExCoverKind::C114:
        // _114.comboBox1_SelectedIndexChanged: us.showQuestion("") — экран пациента очищается
        if (m_mp) {
            m_mp->questionText.clear();
        }
        emit textSettingsAvailabilityChanged();
        break;
    case ExCoverKind::C112:
    case ExCoverKind::C312:
        // _112.param_SelectedIndexChanged / _312.comboBox1_SelectedIndexChanged обнуляют счётчики
        m_ecounter = 0;
        m_ecorrect = 0;
        break;
    default:
        break;
    }
    updateVisibility();
    if (isWrittenMode() && m_running == false) {
        return;
    }
    applyParams(true);
    if (m_rt.coverKind == ExCoverKind::C121 && m_mp) {
        m_mp->setHintVisible(false);
        if (c->showp) {
            c->showp->setImagePath(ExAssets::sysImage(QStringLiteral("showp.png")));
        }
    }
    if (m_running && usesPatientWindow()) {
        attachEngineToWindows();
        if (m_patient && !m_patient->isVisible() && !m_doctorPanel) {
            presentPatient();
        }
    } else if (m_running && !usesPatientWindow() && m_patient) {
        m_patient->hideDisplay();
    }
    updatePatientScreenButton();
}

// ---------------- кнопки ----------------

void ExerciseHost::setRunning(bool running) {
    const bool changed = (m_running != running);
    if (running && changed && m_form) {
        // При «Начать» сбрасываем галочки баллов / помощи / характера деятельности.
        m_form->clear();
    }
    m_running = running;
    if (m_cover->beginu) {
        m_cover->beginu->setVisible(!running);
    }
    if (m_cover->stopu) {
        m_cover->stopu->setVisible(running);
    }
    // 12.1/13.2: левая часть с описанием остаётся; скрывается только «Сформировать протокол».
    showLeft(true);
    if (m_formBtn) {
        m_formBtn->setVisible(!running);
    }
    if (running) {
        m_begun = true;
    }
    updateVisibility();
    if (changed) {
        emit runningChanged(running);
    }
}

void ExerciseHost::openLocalFile(const QString &file) {
    if (QFileInfo::exists(file)) {
        QDesktopServices::openUrl(QUrl::fromLocalFile(file));
    } else {
        CustomMessageBox::showWarning(this, QStringLiteral("Файл не найден:\n") + file);
    }
}

void ExerciseHost::openExternal(const QString &relativeExnFile) {
    openLocalFile(ExAssets::exnPath(relativeExnFile));
}

void ExerciseHost::onBegin() {
    CoverPanel *c = m_cover;
    if (m_rt.coverKind == ExCoverKind::C1) {
        setRunning(true);
        return;
    }
    // 13.1: сюжетные картинки — можно начать до выделения картинки.
    // Счётчики обнуляются при открытии упражнения; при «Начать» — только в _223/_224.beginu_Click_1
    if (m_rt.coverKind == ExCoverKind::C223 || m_rt.coverKind == ExCoverKind::C224) {
        m_ecounter = 0;
        m_ecorrect = 0;
    }
    updateCounters();
    if (m_rt.coverKind == ExCoverKind::C122 && c->combo1->currentIndex() == 1) {
        openExternal(QStringLiteral("1.2.2/С_опорой_на_образец.rtf"));
        setRunning(true);
        return;
    }
    if (m_rt.coverKind == ExCoverKind::C122 && c->combo1->currentIndex() == 2) {
        openExternal(QStringLiteral("1.2.2/Без_опоры_на_образец.rtf"));
        setRunning(true);
        return;
    }
    if (m_rt.coverKind == ExCoverKind::C123 && c->combo2->currentText() == QLatin1String("3")) {
        openExternal(QStringLiteral("1.2.3/data.rtf"));
        setRunning(true);
        return;
    }
    if (m_rt.coverKind == ExCoverKind::C123 && c->combo2->currentText() == QLatin1String("1")) {
        QDir().mkpath(QFileInfo(for123File(m_ctx.patientId)).absolutePath());
        QFile f(for123File(m_ctx.patientId));
        if (f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            QTextStream ts(&f);
            ts.setCodec("UTF-8");
            ts << QStringLiteral("ФИО: ") << m_ctx.patientFio << '\n' << c->address->text() << '\n' << c->phone->text() << '\n';
        }
    }
    if (m_rt.coverKind == ExCoverKind::C324 && c->combo1->currentIndex() == 1) {
        openExternal(QStringLiteral("3.2.4/Напиши_пропущеную_букву.docx"));
        setRunning(true);
        return;
    }
    // uprSpace.initEx копирует пациенту текущую сцену врача (requestData): то же задание, та же раскладка,
    // правки врача до старта (2.1.2 открытый ответ, 2.1.3 замены) сохраняются.
    if (m_rt.coverKind == ExCoverKind::C123) {
        applyParams(true);
    } else if (m_rt.coverKind == ExCoverKind::C225 || m_rt.coverKind == ExCoverKind::C114 ||
               (m_active && m_active->block)) {
        // _114.beginu_Click: us.initEx заново — выделения пациента сбрасываются
        applyParams(false);
    }
    if (m_rt.coverKind == ExCoverKind::C222 && m_st) {
        // _222.beginu_Click: новый uprSpace — рассказ у пациента скрыт до «Показать на экране пациента»
        m_st->contentVisible = false;
        c->showStory->setImagePath(ExAssets::exnPath(QStringLiteral("2.2.2/Показать на экране пациента.png")));
        m_st->rebuild();
    }
    if (m_rt.coverKind == ExCoverKind::C121 && m_mp) {
        // uprSpace 1.2.1 строит сцену со скрытой подсказкой rcolors
        m_hintShown = false;
        m_mp->setHintVisible(false);
        if (c->showp) {
            c->showp->setImagePath(ExAssets::sysImage(QStringLiteral("showp.png")));
        }
    }
    attachEngineToWindows();
    setRunning(true);
    presentPatient();
}

void ExerciseHost::onStop() {
    const bool wasRunning = m_running;
    ExAssets::playMetronome(false, 0);
    if (m_metroOn && m_cover->metroBtn) {
        m_metroOn = false;
        m_cover->metroBtn->setImagePath(ExAssets::exnPath(QStringLiteral("1.2.1/metro.png")));
    }
    m_doctorPanel = false;
    if (m_patient) {
        m_patient->hideDisplay();
    }
    setRunning(false);
    m_doing = m_doing || wasRunning;
    updateVisibility();
}

// ---------------- экран пациента ----------------

bool ExerciseHost::textSettingsAvailable() const {
    if (m_rt.coverKind == ExCoverKind::C1) {
        return true;
    }
    if (m_rt.coverKind == ExCoverKind::C114 && m_cover && m_cover->combo1) {
        return m_cover->combo1->currentText() == QLatin1String("2");
    }
    return false;
}

QString ExerciseHost::textSettingsScope() const {
    if (m_rt.coverKind == ExCoverKind::C1) {
        return QStringLiteral("1.1.1");
    }
    if (m_rt.coverKind == ExCoverKind::C114) {
        return QStringLiteral("1.1.4");
    }
    return {};
}

QTextEdit *ExerciseHost::textSettingsEdit() const {
    return (m_cover && textSettingsAvailable()) ? m_cover->rtf : nullptr;
}

void ExerciseHost::loadStandardTextDocument() {
    QTextEdit *edit = textSettingsEdit();
    if (!edit) {
        return;
    }
    if (m_rt.coverKind == ExCoverKind::C1) {
        RtfCover::loadInto(edit, ExAssets::exnPath(QStringLiteral("1.1.1/text.rtf")));
    } else if (m_rt.coverKind == ExCoverKind::C114) {
        RtfCover::loadInto(edit, ExAssets::exnPath(QStringLiteral("1.1.4/ans1.rtf")));
    }
}

void ExerciseHost::representPatient() {
    if (!m_running || !m_patient) {
        updatePatientScreenButton();
        return;
    }
    const bool wasShown = m_patient->isVisible() || m_doctorPanel;
    m_patient->hideDisplay();
    m_doctorPanel = false;
    if (wasShown && usesPatientWindow()) {
        presentPatient();
    }
    updatePatientScreenButton();
}

void ExerciseHost::presentPatient() {
    if (!m_patient || !m_running || !usesPatientWindow()) {
        updatePatientScreenButton();
        return;
    }
    m_doctorPanel = false;
    // Всегда отдельное окно: второй монитор, а если его нет — основной (Esc → панель врача).
    m_patient->showOnSecondaryScreen();
    updatePatientScreenButton();
}

void ExerciseHost::showDoctorPanel() {
    if (!m_patient || !m_running) {
        return;
    }
    m_doctorPanel = true;
    m_patient->hideDisplay();
    updatePatientScreenButton();
    window()->raise();
    window()->activateWindow();
}

void ExerciseHost::updatePatientScreenButton() {
    if (!m_patientScreenBtn) {
        return;
    }
    // Кнопка нужна, когда окно пациента скрыто (Esc / нет второго монитора).
    const bool show = m_running && usesPatientWindow() && m_patient && !m_patient->isVisible();
    m_patientScreenBtn->setVisible(show);
    if (show) {
        m_patientScreenBtn->raise();
    }
}

void ExerciseHost::onNext() {
    CoverPanel *c = m_cover;
    switch (m_rt.coverKind) {
    case ExCoverKind::C112:
        applyParams(true);
        return;
    case ExCoverKind::C114:
        m_ex = m_ex % 6 + 1;
        break;
    case ExCoverKind::C121:
        applyParams(true);
        return;
    case ExCoverKind::C123:
        m_ex += 2;
        if (m_ex > 10) {
            m_ex = 1;
        }
        break;
    case ExCoverKind::C221:
    case ExCoverKind::C222: {
        const int n = c->combo1->count();
        c->combo1->setCurrentIndex((c->combo1->currentIndex() + 1) % n); // сработает onParamChanged
        return;
    }
    case ExCoverKind::C122:
        m_modeTag = QStringLiteral("1");
        if (c->showp) {
            c->showp->setImagePath(ExAssets::sysImage(QStringLiteral("showp.png")));
        }
        ++m_ex;
        break;
    case ExCoverKind::C323:
    case ExCoverKind::C324:
        if (m_mw) {
            m_mw->showHints = false;
        }
        ++m_ex;
        break;
    default:
        ++m_ex;
        break;
    }
    applyParams(true);
    updateVisibility();
    attachEngineToWindows();
}

void ExerciseHost::onAgain() {
    if (m_rt.coverKind == ExCoverKind::C112 && m_mp) {
        // _112.bagain_Click: dSay и ответы сбрасываются, картинки остаются
        m_mp->dSay.clear();
        m_mp->rebuild(false);
    } else {
        if (m_rt.coverKind == ExCoverKind::C225) {
            // _225.bagain_Click: bob возвращается в exp.png, объяснение у пациента скрыто
            m_explainOn = false;
            m_cover->explainBtn->setImagePath(ExAssets::sysImage(QStringLiteral("exp.png")));
        }
        applyParams(false);
    }
    updateVisibility();
}

void ExerciseHost::onShowp() {
    CoverPanel *c = m_cover;
    ImageButton *sender = qobject_cast<ImageButton *>(QObject::sender());
    switch (m_rt.coverKind) {
    case ExCoverKind::C122:
        m_modeTag = m_modeTag == QLatin1String("1") ? QStringLiteral("2") : QStringLiteral("1");
        c->showp->setImagePath(ExAssets::sysImage(m_modeTag == QLatin1String("1") ? QStringLiteral("showp.png") : QStringLiteral("hidep.png")));
        applyParams(false);
        break;
    case ExCoverKind::C121:
        m_hintShown = !m_hintShown;
        m_mp->setHintVisible(m_hintShown);
        c->showp->setImagePath(ExAssets::sysImage(m_hintShown ? QStringLiteral("hidep.png") : QStringLiteral("showp.png")));
        break;
    case ExCoverKind::C214:
        m_examplesOn = !m_examplesOn;
        m_mw->showExamples = m_examplesOn;
        c->showp->setImagePath(ExAssets::sysImage(m_examplesOn ? QStringLiteral("hidep.png") : QStringLiteral("showp.png")));
        m_mw->rebuild(false);
        break;
    case ExCoverKind::C323:
    case ExCoverKind::C324:
        m_mw->showHints = true;
        applyParams(false);
        updateVisibility();
        break;
    case ExCoverKind::C225:
        if (sender == c->explainBtn) {
            m_explainOn = !m_explainOn;
            // оригинал _225: exp.png ↔ hideo.png
            c->explainBtn->setImagePath(ExAssets::sysImage(m_explainOn ? QStringLiteral("hideo.png") : QStringLiteral("exp.png")));
            if (m_mw) {
                m_mw->setExplain(m_explainOn); // lob/lob2 у пациента, 2o/3o/4o.png в графических заданиях
            }
            updateVisibility();
        }
        break;
    case ExCoverKind::C222: {
        m_st->contentVisible = !m_st->contentVisible;
        c->showStory->setImagePath(ExAssets::exnPath(m_st->contentVisible ? QStringLiteral("2.2.2/Скрыть  на экране пациента.png")
                                                                         : QStringLiteral("2.2.2/Показать на экране пациента.png")));
        m_st->rebuild();
        break;
    }
    default:
        break;
    }
}

void ExerciseHost::onResult(bool wrong) {
    if (!m_cover->lcounter) {
        return;
    }
    ++m_ecounter;
    if (!wrong) {
        ++m_ecorrect;
    }
    updateCounters();
}

void ExerciseHost::onDoctorMark(bool wrong) {
    onResult(wrong);
}

// ---------------- протокол ----------------

struct ExerciseHost::FormedRow {
    QString header;
    bool forceNew = false;
    ExProtocol::Record record; // шапка новой записи (partly == false)
    ExProtocol::Row row;
};

// protocols.createP: оценка, колонки и шапка по uname (runtimeId).
bool ExerciseHost::buildProtocolRow(FormedRow *out) const {
    const QString u = m_rt.runtimeId;
    const QString nb = QStringLiteral("&nbsp;");
    const QString nb2 = QStringLiteral("&nbsp;&nbsp;");
    const QString bandsMinus = QStringLiteral("81|100|3;61|80|2;41|60|1;21|40|0;0|21|-1");
    const QString bandsPlus = QStringLiteral("81|100|3;61|80|2;41|60|1;21|40|0");
    bool checked = false;
    const int radio = m_form->checkedScore(&checked);
    const int bls = checked ? radio : 0;
    const auto blsAbove = [&](int limit) { return bls > limit ? QString::number(bls) : nb; };
    const auto bands = [&](const QString &b) { return ExProtocol::calculateBls(b, m_ecorrect, m_ecounter); };
    const QString combo1 = m_cover->combo1 ? m_cover->combo1->currentText() : QString();

    ExProtocol::Record &r = out->record;
    ExProtocol::Row &row = out->row;
    r.dateLine = ExProtocol::moscowNow() + QLatin1Char(' ') + m_ctx.specialistFio;
    out->header = m_rt.htmlFolder + QStringLiteral("/header.html");
    QString score;
    bool resultFromScore = true;

    if (u == QLatin1String("1.1.1") || u == QLatin1String("1.1.4") || u == QLatin1String("1.2.1")) {
        score = blsAbove(-2);
        out->forceNew = true;
    } else if (u == QLatin1String("1.1.2")) {
        int v = bands(bandsMinus).toInt();
        if (combo1 == QLatin1String("2")) {
            v += 1;
        } else if (combo1 == QLatin1String("3")) {
            v += 2;
        }
        r.maxScore = 5;
        r.firstColumn = QStringLiteral("Уровень");
        r.hasMaxRow = true;
        r.resultIsCopy = true;
        row.first = combo1;
        resultFromScore = false;
        if (m_rt.assetFolder == QLatin1String("1.1.3")) {
            score = QString::number(v);
            r.result = nb2 + score;
        } else {
            score = v == 0 ? nb : QString::number(v);
        }
    } else if (u == QLatin1String("1.2.2")) {
        score = combo1 == QStringLiteral("Устно") ? bands(bandsPlus) : blsAbove(-2);
        r.firstColumn = QStringLiteral("Выполнение");
        row.first = combo1;
    } else if (u == QLatin1String("1.2.3")) {
        score = QString::number(bls);
        out->header = QStringLiteral("1.2.3/header%1.html").arg(m_cover->combo2 ? m_cover->combo2->currentText() : QStringLiteral("1"));
        out->forceNew = true;
    } else if (u == QLatin1String("2.1.1") || u == QLatin1String("2.1.2") || u == QLatin1String("2.1.3")) {
        const int b = checked ? radio : -10;
        score = b > -4 ? QString::number(b) : nb;
        out->forceNew = true;
    } else if (u == QLatin1String("2.1.4") || u == QLatin1String("3.2.3")) {
        score = bands(bandsMinus);
        out->forceNew = true;
    } else if (u == QLatin1String("2.2.1") || u == QLatin1String("2.2.2")) {
        score = blsAbove(-4);
        r.firstColumn = u == QLatin1String("2.2.1") ? QStringLiteral("Сюжет") : QStringLiteral("Рассказ");
        r.hasMaxRow = true;
        r.resultIsCopy = true;
        r.result = nb2;
        resultFromScore = false;
        row.first = ExProtocol::escape(combo1);
    } else if (u == QLatin1String("2.2.3") || u == QLatin1String("2.2.4") || u == QLatin1String("2.2.5")
               || u == QLatin1String("3.1.1")) {
        score = bands(bandsMinus);
    } else if (u == QLatin1String("2.2.6")) {
        score = combo1 == QLatin1String("2") ? bands(bandsMinus) : blsAbove(-4);
        r.firstColumn = QStringLiteral("Упражнение");
        r.hasMaxRow = true;
        r.resultIsCopy = true;
        r.result = nb2;
        resultFromScore = false;
        if (combo1 == QLatin1String("1")) {
            row.first = QStringLiteral("Составление рассказа с опорой на предметные картинки");
        } else if (combo1 == QLatin1String("2")) {
            row.first = QStringLiteral("Составление рассказа из заданных предложений");
        } else if (combo1 == QLatin1String("3")) {
            row.first = QStringLiteral("Самостоятельное составление рассказа");
        }
    } else if (u == QLatin1String("3.1.2") && m_rt.dataNumber != 2) {
        score = bands(bandsMinus);
        r.firstColumn = QStringLiteral("Выполнение");
        r.hasMaxRow = true;
        r.resultIsCopy = true;
        r.result = nb2;
        resultFromScore = false;
        row.first = ExProtocol::escape(combo1);
    } else if (u == QLatin1String("3.1.2")) {
        score = bands(bandsMinus);
        out->forceNew = true;
    } else if (u == QLatin1String("3.2.4")) {
        score = combo1 == QStringLiteral("Устно") ? bands(bandsMinus) : blsAbove(0);
        r.firstColumn = QStringLiteral("Выполнение");
        row.first = ExProtocol::escape(combo1);
        out->forceNew = true;
    } else {
        return false;
    }
    if (resultFromScore) {
        r.result = nb2 + score;
    }
    row.orHtml = m_form->activityHtml();
    row.hlpHtml = m_form->helpHtml();
    row.score = score;
    r.rows.append(row);
    return true;
}

void ExerciseHost::onFormProtocol() {
    if (!m_doing) {
        CustomMessageBox::showError(this, QStringLiteral("Формирование протокола невозможно без выполнения упражнения"));
        return;
    }
    if (!m_ctx.repository || m_ctx.patientId.isEmpty()) {
        CustomMessageBox::showError(this, QStringLiteral("Не выбран пациент"));
        return;
    }
    FormedRow formed;
    if (!buildProtocolRow(&formed)) {
        return;
    }
    m_protocolView->flushEdits();
    const QString uprid = m_rt.runtimeId;
    QString id;
    if (m_partly && !formed.forceNew) {
        if (m_protocolId.isEmpty()) {
            m_protocolId = m_ctx.repository->lastProtocolId(m_ctx.patientId, uprid);
        }
        const std::optional<StoredProtocol> last = m_protocolId.isEmpty()
                                                       ? std::nullopt
                                                       : m_ctx.repository->loadProtocol(m_protocolId);
        if (last) {
            ExProtocol::Record record = ExProtocol::parseStoredBody(last->body);
            if (record.hasMaxRow) {
                record.maxValue.clear();
            }
            record.rows.append(formed.row);
            if (m_ctx.repository->updateProtocolBody(last->id, ExProtocol::storedBody(record))) {
                id = last->id;
            }
        }
    }
    if (id.isEmpty()) {
        QString err;
        id = m_ctx.repository->insertProtocol(m_ctx.patientId, uprid, formed.header, ExProtocol::storedBody(formed.record), &err);
        if (id.isEmpty()) {
            CustomMessageBox::showError(this, QStringLiteral("Не удалось сохранить протокол.\n") + err);
            return;
        }
    }
    m_protocolId = id;
    showProtocol(id);
    m_itogBtn->setVisible(!m_protocolView->entries().isEmpty() && m_protocolView->entries().first().record.hasMaxRow);
    m_partly = true;
    m_form->clear();
    m_doing = false;
}

void ExerciseHost::showProtocol(const QString &protocolId) {
    const std::optional<StoredProtocol> p = m_ctx.repository->loadProtocol(protocolId);
    if (!p) {
        return;
    }
    ExProtocol::Entry e;
    e.id = p->id;
    e.uprid = p->uprid;
    e.record = ExProtocol::parseStoredBody(p->body);
    e.header = ExProtocol::loadHeader(p->header.isEmpty() ? ExProtocol::defaultHeaderPath(p->uprid) : p->header);
    // getProtocolById: заголовок, ФИО и дата рождения пациента над таблицами.
    const QString preamble = QStringLiteral("<div align='center' style='font-size:18px'><br><b>Протокол фиксации результатов исследования</b></div><br>"
                                            "ФИО: %1%2 Дата рождения:%3<br><br>")
                                 .arg(ExProtocol::escape(m_ctx.patientFio), QStringLiteral("&nbsp;").repeated(20),
                                      ExProtocol::escape(m_ctx.patientBirthDate));
    m_protocolView->showEntries(preamble, {e});
    refreshLeftSizes();
}

void ExerciseHost::saveProtocolEdits(const QList<int> &indexes) {
    if (!m_ctx.repository) {
        return;
    }
    const QList<ExProtocol::Entry> &entries = m_protocolView->entries();
    for (int i : indexes) {
        if (i >= 0 && i < entries.size()) {
            m_ctx.repository->updateProtocolBody(entries.at(i).id, ExProtocol::storedBody(entries.at(i).record));
        }
    }
}

// bsum_Click: «Максимальный балл» = максимум по строкам, копия в «Результат» (idcopy).
void ExerciseHost::onItog() {
    m_protocolView->flushEdits();
    QList<ExProtocol::Entry> &entries = m_protocolView->entries();
    if (entries.isEmpty() || !ExProtocol::applyItog(entries.first().record)) {
        return;
    }
    const QString id = entries.first().id;
    m_ctx.repository->updateProtocolBody(id, ExProtocol::storedBody(entries.first().record));
    showProtocol(id);
}
