#ifndef SUNRISEWINDOW_H
#define SUNRISEWINDOW_H

#include "appsettings.h"
#include "clinicalbrowsercontroller.h"
#include "imagebutton.h"
#include "repository.h"
#include "summarypanel.h"

#include <QCheckBox>
#include <QRadioButton>
#include <QComboBox>
#include <QDateEdit>
#include <QLabel>
#include <QLineEdit>
#include <QMainWindow>
#include <QPushButton>
#include <QSlider>
#include <QStackedWidget>
#include <QStyledItemDelegate>
#include <QTableWidget>
#include <QTextEdit>
#include <QTextBrowser>
#include <QTreeWidget>
#include <QDialog>
#include <QTimer>
#include <optional>

class QNetworkAccessManager;
class QPrinter;

class SunriseWindow final : public QMainWindow {
    Q_OBJECT
public:
    explicit SunriseWindow(const QString &licenseKey, bool openAdminOnStart = false, QWidget *parent = nullptr);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;
    void changeEvent(QEvent *event) override;
    void showEvent(QShowEvent *event) override;
    void moveEvent(QMoveEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    enum class ScreenMode {
        Enter,
        Patients,
        Admin,
        Anamnesis,
        RiskAssessment,
        Correction,
        Speech,
        Breathing,
        Medication
    };

    void buildUi();
    void buildSlidePanels();
    void applyLegacyStyle();
    void bindSignals();
    void setScreen(ScreenMode mode, bool pushHistory = true);
    void navigateBack();
    void updatePatientTabIcons();
    void toggleWindowMaximize();
    void updateMaximizeButtonIcon();
    void applyNormalWindowGeometry();
    void applyWindowGeometry(const QRect &rect);
    QRect calculateNormalWindowGeometry() const;
    QRect calculateMaximizedWindowGeometry() const;

    QString imagePath(const QString &name) const;
    QString resourcePath(const QString &name) const;
    QString htmlPath(const QString &name) const;
    void setImage(ImageButton *button, const QString &name);
    void applyEnterLogos();
    void styleInputField(QLineEdit *edit) const;
    void styleAdminInputField(QLineEdit *edit) const;
    void styleAdminScreen();
    void stylePatientsScreen();
    void styleAnamnesisScreen();
    void setupClinicalController();
    void layoutWorkAreaForMode(ScreenMode mode);
    void loadSummaryForCurrentPatient();
    void tryAutoSaveProgram(bool force = false);
    void openJournal();
    QString clinicalHtmlRoot() const;
    void fitPatientsTableToContent();
    void loadDefaultAnamnesisTemplate();
    void loadStandardAnamnesisHtml();
    void applyAnamnesisDocument(const QString &raw);
    void loadAnamnesisRtf(const QByteArray &rtf);
    void applyAnamnesisFont(int pointSize);
    void applyAnamnesisFontToEntireDocument(int pointSize);
    void changeDocumentFontSize(int pointSize, bool persistProfile = true);
    void prepareAnamnesisDocumentForOutput();
    void applyAnamnesisDocumentFontDefaults();
    void applyCompactAnamnesisLineSpacing();
    void writeProfileConfig(const QString &profileName, int fontSize);
    QString profileConfigPath() const;
    void bindClearableField(QLineEdit *edit, ImageButton *clearBtn);
    QString defaultAnamnesisHtml() const;
    QString decodeDocument(const QString &raw) const;

    void refreshUsers();
    void scheduleRefreshUsers();
    void refreshPatients();
    void refreshTemplateNames();
    void loadAnamnesisTemplateByName(const QString &name);
    void saveCurrentAnamnesisTemplate();
    QString anamnesisTemplatePayload() const;
    void syncTemplateSelectorFromProfile();
    void setWorkChromeVisible(bool visible);
    void raiseChromeWidgets();
    void resetUserCreateForm();
    void loadUserForEdit(const QString &id);
    void deleteUserById(const QString &userId);
    void saveUser();
    void enterAsManagedUser();
    void rememberManagedUser(const QString &userId, const QString &login, const QString &password);
    void handlePatientsTableClick(int row, int column);
    void styleUsersTable();
    void fitUsersTableToContent();
    void startUserSession(const SessionUser &user);
    void endUserSession();
    void handlePatientsHeaderClick(int section);
    void openPatientFromTable();
    void tryAutoSaveAnamnesis(bool forceRefreshPatients = false);
    void setAnamnesisDbControlsEnabled(bool enabled);
    void updatePatientTitleFromDocument();
    void styleTemplateComboBox();
    void saveAnamnesisToDb();
    void exportDocument();
    void printSelectedContent();
    void showInfoPopup();
    void showHelpWindow(const QString &address, bool aboveModalDialog = false);
    void showAboutWindow();
    void toggleSlidePanel(QWidget *panel);
    void hideSlidePanels();
    void animateSlidePanel(QWidget *panel, bool showPanel);
    void updateFormatButtonIcons();
    void showSettingsSaveTemplateView(bool show);
    void installToolbarTooltips();
    QString currentPatientBirthDate() const;
    struct ExportSelection {
        bool anamnesis = false;
        bool program = false;
        bool protocols = false;
        QList<bool> programSections;
    };
    QString assembleExportHtml(const ExportSelection &selection);
    void renderExportToPrinter(QPrinter &printer, const ExportSelection &selection, const QString &assembledHtml);
    ExportSelection currentExportSelection() const;
    bool validateExportSelection(const ExportSelection &selection, const QString &emptyMessage);
    QString programDocumentHtml(const ExportSelection &selection) const;
    QByteArray programDocumentRtf(const ExportSelection &selection) const;
    void showPrintPanel(bool saveMode);
    void updatePrintPanelState(QRadioButton *changed);
    void checkForUpdates();
    void showUpdateResultDialog(bool updateAvailable);

    static bool isWorkScreen(ScreenMode mode);

    ApiClient m_api;
    Repository m_repository;
    QString m_licenseKey;
    QString m_helpIndex = "0.1.htm";
    std::optional<SessionUser> m_session;
    QString m_mainId;
    QString m_currentPatientId;
    QList<ScreenMode> m_navHistory;
    ScreenMode m_currentScreen = ScreenMode::Enter;
    bool m_navigatingBack = false;
    int m_patientSortColumn = -1;
    bool m_patientSortAscending = true;
    QString m_selectedPatientRowId;
    int m_loginAttempts = 0;
    bool m_updateCheckInProgress = false;
    bool m_underlineActive = false;
    bool m_boldActive = false;
    bool m_settingsTemplateMode = false;
    bool m_suppressTemplateLoad = false;
    QWidget *m_activeSlidePanel = nullptr;
    QWidget *m_root = nullptr;

    ImageButton *m_bClose = nullptr;
    ImageButton *m_bLine = nullptr;
    ImageButton *m_bUp = nullptr;
    ImageButton *m_bBack = nullptr;
    ImageButton *m_bList = nullptr;
    ImageButton *m_bExit = nullptr;
    ImageButton *m_bPicPrint = nullptr;
    ImageButton *m_bUpload = nullptr;
    ImageButton *m_bSave = nullptr;
    ImageButton *m_bPrint = nullptr;
    ImageButton *m_bSettings = nullptr;
    ImageButton *m_bInfo = nullptr;
    ImageButton *m_pAna = nullptr;
    ImageButton *m_pRisk = nullptr;
    ImageButton *m_pCorr = nullptr;
    ImageButton *m_pUpr = nullptr;
    ImageButton *m_pDih = nullptr;
    ImageButton *m_pMp = nullptr;
    ImageButton *m_bJournal = nullptr;
    ImageButton *m_bUpdate = nullptr;
    QLabel *m_logo1 = nullptr;
    QLabel *m_logoTitle2 = nullptr;
    QLabel *m_logo2 = nullptr;

    QWidget *m_panelLogin = nullptr;
    QLineEdit *m_loginEdit = nullptr;
    QLineEdit *m_passwordEdit = nullptr;
    ImageButton *m_loginClear = nullptr;
    ImageButton *m_passwordClear = nullptr;
    QLabel *m_loginManIcon = nullptr;
    QLabel *m_loginKeyIcon = nullptr;
    ImageButton *m_loginEye = nullptr;
    ImageButton *m_loginButton = nullptr;
    ImageButton *m_adminButton = nullptr;

    QWidget *m_panelAdmin = nullptr;
    QWidget *m_userFioPanel = nullptr;
    QWidget *m_userLoginPanel = nullptr;
    QWidget *m_userPassPanel = nullptr;
    QWidget *m_userPass2Panel = nullptr;
    QTableWidget *m_usersTable = nullptr;
    QLineEdit *m_userFio = nullptr;
    QLineEdit *m_userLogin = nullptr;
    QLineEdit *m_userPass = nullptr;
    QLineEdit *m_userPass2 = nullptr;
    ImageButton *m_userFioClear = nullptr;
    ImageButton *m_userLoginClear = nullptr;
    ImageButton *m_userPassClear = nullptr;
    ImageButton *m_userPass2Clear = nullptr;
    QLabel *m_adminLabel1 = nullptr;
    QLabel *m_adminLabel2 = nullptr;
    QLabel *m_adminManIcon = nullptr;
    QLabel *m_adminLoginIcon = nullptr;
    QLabel *m_adminKeyIcon1 = nullptr;
    QLabel *m_adminKeyIcon2 = nullptr;
    ImageButton *m_adminEye1 = nullptr;
    ImageButton *m_adminEye2 = nullptr;
    QComboBox *m_userRole = nullptr;
    ImageButton *m_userSaveButton = nullptr;
    ImageButton *m_userOpenPatients = nullptr;
    QString m_editUserId;
    bool m_userSaveInProgress = false;
    bool m_anamnesisSaveInProgress = false;
    QString m_lastManagedUserId;
    QString m_lastManagedUserLogin;
    QString m_lastManagedUserPassword;

    QWidget *m_panelPatients = nullptr;
    QLineEdit *m_patientSearch = nullptr;
    ImageButton *m_patientSearchClear = nullptr;
    QTableWidget *m_patientsTable = nullptr;
    QCheckBox *m_dateFilter = nullptr;
    QDateEdit *m_dateFrom = nullptr;
    QDateEdit *m_dateTo = nullptr;
    QLabel *m_labelFrom = nullptr;
    QLabel *m_labelTo = nullptr;
    ImageButton *m_addPatient = nullptr;

    QWidget *m_panelWork = nullptr;
    QWidget *m_panelClinical = nullptr;
    QStackedWidget *m_workStack = nullptr;
    SummaryPanel *m_summaryPanel = nullptr;
    QTextEdit *m_anamnesisEdit = nullptr;
    ClinicalBrowserController *m_clinical = nullptr;
    bool m_programSaveInProgress = false;
    QTimer *m_programSaveTimer = nullptr;
    QLabel *m_patientTitle = nullptr;
    QLabel *m_anamnesisLabel = nullptr;
    QLabel *m_adminTitle = nullptr;
    QComboBox *m_templates = nullptr;
    QSlider *m_fontSlider = nullptr;
    QLabel *m_fontSizeLabel = nullptr;
    ImageButton *m_fontDownButton = nullptr;
    ImageButton *m_fontUpButton = nullptr;
    ImageButton *m_underlineButton = nullptr;
    ImageButton *m_boldButton = nullptr;
    ImageButton *m_templateDeleteButton = nullptr;
    ImageButton *m_templateSaveAsButton = nullptr;
    QLineEdit *m_templateNameEdit = nullptr;
    QWidget *m_settingsMainView = nullptr;
    QWidget *m_settingsSaveView = nullptr;

    QWidget *m_settingsPanel = nullptr;
    QWidget *m_printPanel = nullptr;
    QLabel *m_printPanelTitle = nullptr;
    QRadioButton *m_printProgramRb = nullptr;
    QList<QCheckBox *> m_printSectionChecks;
    QRadioButton *m_printAnamnesisRb = nullptr;
    QRadioButton *m_printProtocolsRb = nullptr;
    QRadioButton *m_printForPatientRb = nullptr;
    QRadioButton *m_printForSpecialistRb = nullptr;
    ImageButton *m_printPanelPrintButton = nullptr;
    ImageButton *m_printPanelSaveButton = nullptr;
    bool m_printPanelSaveMode = false;
    QByteArray m_lastAnamnesisRtf;

    QNetworkAccessManager *m_updateNetwork = nullptr;
    QString m_remoteUpdateVersion;

    static constexpr int kDesignWidth = 1920;
    static constexpr int kDesignHeight = 1080;
    static constexpr int kTitleBarHeight = 57;
    static constexpr int kTaskbarReserve = 44;
    QRect m_savedWindowGeometry;
    QRect m_normalGeometryBeforeMaximize;
    bool m_isCustomMaximized = false;
    bool m_geometryInitialized = false;
    bool m_programmaticGeometryChange = false;
    bool m_screenTransitionGuard = false;

    QDialog *m_infoPopup = nullptr;
    QDialog *m_helpWindow = nullptr;
    QDialog *m_aboutWindow = nullptr;
    QTextBrowser *m_helpBrowser = nullptr;
    QString m_currentHelpFilePath;
};

#endif
