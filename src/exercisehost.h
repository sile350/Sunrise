#ifndef EXERCISEHOST_H
#define EXERCISEHOST_H

#include "exerciseids.h"

#include <QPointer>
#include <QWidget>

class CoverPanel;
class ExEngine;
class MPicturesEngine;
class MovewordsEngine;
class StoriesEngine;
class WolfEngine;
class PatientWindow;
class QComboBox;
class QRadioButton;
class QScrollArea;
class QTextBrowser;
class QLabel;
class QPushButton;
class ImageButton;
class ProtocolEditor;
class Repository;
class ScoringForm;

struct ExerciseContext {
    QString patientId;
    QString patientFio;
    QString patientBirthDate;
    QString specialistFio;
    Repository *repository = nullptr;
};

// Аналог exbegin: левая часть — or.html и шаблон протокола, справа — обложка с зеркалом
// экрана пациента. Окно пациента (PatientWindow) открывается кнопкой «Начало».
class ExerciseHost : public QWidget {
    Q_OBJECT
public:
    ExerciseHost(const ExerciseRuntime &runtime, const ExerciseContext &context, QWidget *parent = nullptr);
    ~ExerciseHost() override;

    // Закрыть окно пациента, остановить метроном.
    void shutdown();
    // Режим экранов (Администрирование / шапка): два экрана — пациент на втором мониторе,
    // один экран — сцена пациента на весь монитор врача с панелью управления.
    void setDualScreenEnabled(bool enabled);

signals:
    void closeRequested();

protected:
    void resizeEvent(QResizeEvent *event) override;

private:
    // инициализация
    void buildLeftColumn();
    void buildEngines();
    void wireCover();
    void layoutCover();
    // левая часть
    void loadLeftHtml();
    QString composeOrHtml() const;
    void refreshLeftSizes();
    void updateScoreBlock();
    void showLeft(bool on);
    // протокол (exbegin.pictureBox1_Click / bsum_Click)
    struct FormedRow;
    bool buildProtocolRow(FormedRow *out) const;
    void onFormProtocol();
    void onItog();
    void showProtocol(const QString &protocolId);
    void saveProtocolEdits(const QList<int> &indexes);
    // логика
    void onBegin();
    void onStop();
    void onNext();
    void onAgain();
    void onParamChanged(QComboBox *source = nullptr);
    void onShowp();
    void onResult(bool wrong);
    void onDoctorMark(bool wrong);
    void applyParams(bool fresh);
    void syncDoctorLabels();
    void attachEngineToWindows();
    void setRunning(bool running);
    void updateCounters();
    void openExternal(const QString &relativeExnFile);
    void openLocalFile(const QString &file);
    bool isWrittenMode() const;
    bool usesPatientWindow() const;
    QString currentMaterialText(int combo) const;
    void updateVisibility();
    // экран пациента в выбранном режиме
    void presentPatient();
    void representPatient();
    bool singleScreenActive() const;
    void showDoctorPanel();
    void refreshPatientControls();
    void updatePatientScreenButton();

    ExerciseRuntime m_rt;
    ExerciseContext m_ctx;
    CoverPanel *m_cover = nullptr;
    // левая колонка
    QScrollArea *m_leftScroll = nullptr;
    QTextBrowser *m_or = nullptr;
    ScoringForm *m_form = nullptr;
    ImageButton *m_formBtn = nullptr;
    ProtocolEditor *m_protocolView = nullptr;
    ImageButton *m_itogBtn = nullptr;
    int m_scoreMode = -1;
    bool m_doing = false;
    bool m_partly = false;
    QString m_protocolId;
    QLabel *m_title = nullptr;
    ImageButton *m_back = nullptr;
    ImageButton *m_print = nullptr;
    QString m_orRaw;
    bool m_open[3] = {false, false, false};
    // движки
    MPicturesEngine *m_mp = nullptr;
    MovewordsEngine *m_mw = nullptr;
    StoriesEngine *m_st = nullptr;
    WolfEngine *m_wolf = nullptr;
    ExEngine *m_blank = nullptr;
    ExEngine *m_active = nullptr;
    QPointer<PatientWindow> m_patient;
    // состояние обложки
    int m_ex = 1;
    int m_ecounter = 0;
    int m_ecorrect = 0;
    bool m_running = false;
    bool m_begun = false; // «Начать» нажималась хотя бы раз (1.2.2: счётчики остаются после «Стоп»)
    bool m_ready = false;
    QString m_modeTag = QStringLiteral("1"); // 1.1.2: 1 показать / 2 подписать; 1.2.2: 1 / 2 варианты
    bool m_metroOn = false;
    bool m_hintShown = false;
    bool m_examplesOn = false;
    bool m_explainOn = false;
    bool m_dualScreen = false;
    bool m_doctorPanel = false; // один экран: упражнение идёт, врач временно вернулся к своей панели
    QPushButton *m_patientScreenBtn = nullptr;
};

#endif
