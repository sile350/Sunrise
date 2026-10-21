#ifndef REPOSITORY_H
#define REPOSITORY_H

#include "apiclient.h"
#include "exerciseprotocol.h"
#include "localdatabase.h"
#include "models.h"

#include <QDate>
#include <QList>
#include <QObject>
#include <optional>

struct StoredProtocol {
    QString id;
    QString uprid;
    QString header;
    QString body;
};

struct SessionUser {
    QString id;
    QString fio;
    QString login;
    QString role;
    QString mainId;
};

class Repository : public QObject {
    Q_OBJECT
public:
    explicit Repository(ApiClient *api, QObject *parent = nullptr);

    std::optional<SessionUser> login(const QString &userLogin, const QString &password);

    QList<UserRecord> fetchUsers();
    std::optional<UserRecord> fetchUserById(const QString &id);
    bool createUser(const QString &fio, const QString &login, const QString &password, const QString &role, const QString &mainId, QString *errorText, QString *createdId = nullptr);
    bool updateUser(const QString &id, const QString &fio, const QString &login, const QString &password, const QString &role, QString *errorText);
    bool deleteUser(const QString &id, QString *errorText);

    QList<PatientRecord> fetchPatients(const QString &search, bool useDateFilter, const QDate &from, const QDate &to);
    QString loadPatientAnamnesis(const QString &patientId);
    QString loadPatientProgram(const QString &patientId);
    QString loadPatientDiag(const QString &patientId);
    bool savePatientProgram(const QString &patientId, const QString &program, QString *errorText = nullptr);
    bool verifyPatientAccess(const QString &patientId, const QString &licenseKey, QString *errorText);
    bool savePatientAnamnesis(QString *patientId, const QString &licenseKey, const QString &plainText, const QString &html, QString *detectedFio, QString *detectedBirthDate, QString *errorText);
    // Обновить только HTML анамнеза у существующей карты (без проверки ФИО) — после смены/удаления шаблона.
    bool replacePatientAnamnesisHtml(const QString &patientId, const QString &html, QString *errorText = nullptr);
    void registerNewPatientAccepted(const QString &patientId, const QString &licenseKey);
    void extractPatientFields(const QString &plainText, QString *fio, QString *birthDate, QString *diag = nullptr, QString *visitDate = nullptr) const;
    bool deletePatient(const QString &patientId, QString *errorText);

    QString defaultAnamnesisTemplate() const;
    // upr: пусто — шаблоны анамнеза; «1.1.1» / «1.1.4» — шаблоны текста упражнения (как в оригинале).
    QString loadTemplate(const QString &name, const QString &upr = QString());
    QStringList loadTemplateNames(const QString &upr = QString());
    bool saveTemplate(const QString &name, int fontPointSize, const QString &html, QString *errorText, const QString &upr = QString());
    bool deleteTemplate(const QString &name, QString *errorText, const QString &upr = QString());

    // Протоколы упражнений (protocols.cs: saveProtocol / getLastP / getAll / getSall).
    QString insertProtocol(const QString &patientId, const QString &uprid, const QString &header, const QString &body, QString *errorText);
    bool updateProtocolBody(const QString &protocolId, const QString &body, QString *errorText = nullptr);
    std::optional<StoredProtocol> loadProtocol(const QString &protocolId);
    QString lastProtocolId(const QString &patientId, const QString &uprid);
    QList<StoredProtocol> loadPatientProtocolRecords(const QString &patientId);
    // Порядок getAll: ecurrent, uprid, id; шапка — у первой записи каждого упражнения.
    QList<ExProtocol::Entry> loadPatientProtocolEntries(const QString &patientId, bool withProcess = true);
    bool hasPatientProtocols(const QString &patientId);
    QString loadPatientProtocols(const QString &patientId);
    QString loadPatientProtocolsForExport(const QString &patientId, const QString &role, const QString &patientDataHeader);
    QStringList loadPatientProtocolRecordIds(const QString &patientId);

    bool verifyLicenseKeyForMachine(const QString &key, const QString &hardware, QString *errorText);
    bool activateLicenseKey(const QString &key, const QString &hardware, QString *errorText);

    QString lastError() const;
    QString apiBaseUrl() const;

private:
    QString hashPassword(const QString &password, QString *errorText);
    QString encryptPatientFio(const QString &fio) const;
    QString decryptPatientFio(const QString &storedFio) const;
    bool patientMatchesSearch(const QString &fio, const QString &search) const;
    QString extractValueByPrefix(const QString &text, const QString &prefix) const;
    qint64 unixTime(const QDate &date, bool endOfDay) const;

    ApiClient *m_api;
    LocalDatabase m_local;
};

#endif
