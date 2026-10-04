#include "repository.h"

#include "fieldcrypto.h"

#include <QDateTime>
#include <QRegularExpression>
#include <QTimer>
#include <algorithm>

Repository::Repository(ApiClient *api, QObject *parent)
    : QObject(parent), m_api(api) {
    QString openError;
    m_local.open(&openError);
}
QString Repository::hashPassword(const QString &password, QString *errorText) {
    const QString sole = m_api->systemRequest("getsole", password);
    if (sole.isEmpty()) {
        if (errorText) {
            *errorText = "Не удалось хешировать пароль.";
        }
    }
    return sole;
}
std::optional<SessionUser> Repository::login(const QString &userLogin, const QString &password) {
    QString err;
    const QString sole = hashPassword(password, &err);
    if (sole.isEmpty()) {
        return std::nullopt;
    }
    const QString sql = "SELECT id,fio,login,pass,role,main FROM users WHERE login='"
        + LocalDatabase::escape(userLogin) + "' AND pass='" + LocalDatabase::escape(sole) + "'";
    const QList<QStringList> rows = m_local.queryRows(sql);
    if (rows.isEmpty() || rows.first().size() < 6) {
        return std::nullopt;
    }
    SessionUser user;
    user.id = rows.first().at(0);
    user.fio = rows.first().at(1);
    user.login = rows.first().at(2);
    user.role = rows.first().at(4);
    user.mainId = rows.first().at(5);
    return user;
}
QList<UserRecord> Repository::fetchUsers() {
    QList<UserRecord> records;
    const QList<QStringList> rows = m_local.queryRows("SELECT id,fio,login,role,main FROM users");
    for (const QStringList &row : rows) {
        if (row.size() < 5) {
            continue;
        }
        records.push_back({row.at(0), row.at(1), row.at(2), row.at(3), row.at(4)});
    }
    return records;
}
std::optional<UserRecord> Repository::fetchUserById(const QString &id) {
    const QList<QStringList> rows = m_local.queryRows(
        "SELECT id,fio,login,role,main FROM users WHERE id=" + id
    );
    if (rows.isEmpty() || rows.first().size() < 5) {
        return std::nullopt;
    }
    const QStringList &row = rows.first();
    return UserRecord{row.at(0), row.at(1), row.at(2), row.at(3), row.at(4)};
}
bool Repository::createUser(const QString &fio, const QString &login, const QString &password, const QString &role, const QString &mainId, QString *errorText, QString *createdId) {
    const QString localExists = m_local.queryScalar(
        "SELECT COUNT(*) FROM users WHERE login='" + LocalDatabase::escape(login) + "'");
    if (localExists.trimmed().toInt() > 0) {
        if (errorText) {
            *errorText = "Такой пользователь уже есть!";
        }
        return false;
    }
    const QString checkResponse = m_api->systemRequest("checkuser", "fio[" + login);
    if (checkResponse.trimmed() == QStringLiteral("уже есть")) {
        if (errorText) {
            *errorText = "Такой пользователь уже есть!";
        }
        return false;
    }
    const QString sole = hashPassword(password, errorText);
    if (sole.isEmpty()) {
        return false;
    }
    const QString duplicateAfterHash = m_local.queryScalar(
        "SELECT COUNT(*) FROM users WHERE login='" + LocalDatabase::escape(login) + "'");
    if (duplicateAfterHash.trimmed().toInt() > 0) {
        if (errorText) {
            *errorText = "Такой пользователь уже есть!";
        }
        return false;
    }
    const QString sql = "INSERT INTO users (fio,login,pass,role,main) VALUES('"
        + LocalDatabase::escape(fio) + "','"
        + LocalDatabase::escape(login) + "','"
        + LocalDatabase::escape(sole) + "','"
        + LocalDatabase::escape(role) + "','"
        + LocalDatabase::escape(mainId) + "')";
    if (!m_local.exec(sql)) {
        if (errorText) {
            *errorText = m_local.lastError();
        }
        return false;
    }
    if (createdId) {
        *createdId = QString::number(m_local.lastInsertId());
    }
    return true;
}
bool Repository::updateUser(const QString &id, const QString &fio, const QString &login, const QString &password, const QString &role, QString *errorText) {
    const QString duplicateLogin = m_local.queryScalar(
        "SELECT COUNT(*) FROM users WHERE login='" + LocalDatabase::escape(login) + "' AND id<>" + id
    );
    if (duplicateLogin.trimmed().toInt() > 0) {
        if (errorText) {
            *errorText = "Такой пользователь уже есть!";
        }
        return false;
    }

    QString sql;
    if (password.isEmpty()) {
        sql = "UPDATE users SET fio='" + LocalDatabase::escape(fio) + "',login='"
            + LocalDatabase::escape(login) + "',role='" + LocalDatabase::escape(role) + "' WHERE id=" + id;
    } else {
        const QString sole = hashPassword(password, errorText);
        if (sole.isEmpty()) {
            return false;
        }
        sql = "UPDATE users SET fio='" + LocalDatabase::escape(fio) + "',login='"
            + LocalDatabase::escape(login) + "',pass='" + LocalDatabase::escape(sole) + "',role='"
            + LocalDatabase::escape(role) + "' WHERE id=" + id;
    }
    if (!m_local.exec(sql)) {
        if (errorText) {
            *errorText = m_local.lastError();
        }
        return false;
    }
    return true;
}
bool Repository::deleteUser(const QString &id, QString *errorText) {
    if (!m_local.exec("DELETE FROM users WHERE id=" + id)) {
        if (errorText) {
            *errorText = m_local.lastError();
        }
        return false;
    }
    return true;
}
QList<PatientRecord> Repository::fetchPatients(const QString &search, bool useDateFilter, const QDate &from, const QDate &to) {
    QList<PatientRecord> patients;
    QString sql = "SELECT id,fio,dr,dt,dto FROM patients WHERE 1=1";
    if (useDateFilter) {
        sql += " AND dt>=" + QString::number(unixTime(from, false));
        sql += " AND dt<=" + QString::number(unixTime(to, true));
    }
    sql += " ORDER BY id";
    const QList<QStringList> rows = m_local.queryRows(sql);
    for (const QStringList &row : rows) {
        if (row.size() < 5) {
            continue;
        }
        PatientRecord patient;
        patient.id = row.at(0);
        patient.fio = decryptPatientFio(row.at(1));
        patient.birthDate = row.at(2);
        patient.createdAt = row.at(3).toLongLong();
        patient.visitDate = row.at(4);
        if (!patientMatchesSearch(patient.fio, search)) {
            continue;
        }
        patients.push_back(patient);
    }
    return patients;
}
QString Repository::loadPatientAnamnesis(const QString &patientId) {
    return m_local.queryScalar("SELECT an FROM patients WHERE id=" + patientId);
}
QString Repository::loadPatientProgram(const QString &patientId) {
    if (patientId.trimmed().isEmpty()) {
        return {};
    }
    return m_local.queryScalar("SELECT program FROM patients WHERE id=" + patientId.trimmed());
}
QString Repository::loadPatientDiag(const QString &patientId) {
    if (patientId.trimmed().isEmpty()) {
        return {};
    }
    return m_local.queryScalar("SELECT diag FROM patients WHERE id=" + patientId.trimmed());
}
bool Repository::savePatientProgram(const QString &patientId, const QString &program, QString *errorText) {
    if (patientId.trimmed().isEmpty()) {
        if (errorText) {
            *errorText = QStringLiteral("Пациент не выбран.");
        }
        return false;
    }
    const QString sql = "UPDATE patients SET program='" + LocalDatabase::escape(program)
        + "' WHERE id=" + patientId.trimmed();
    if (!m_local.exec(sql)) {
        if (errorText) {
            *errorText = m_local.lastError();
        }
        return false;
    }
    return true;
}
bool Repository::verifyPatientAccess(const QString &patientId, const QString &licenseKey, QString *errorText) {
    const QString ownerId = m_api->loadOneData("SELECT id FROM org WHERE ky='" + ApiClient::escapeSql(licenseKey) + "'");
    if (ownerId.trimmed().isEmpty()) {
        return true;
    }
    const QString acceptedId = m_api->loadOneData(
        "SELECT id FROM accepted WHERE pid='" + patientId + "' AND owner='" + ownerId.trimmed() + "'");
    if (acceptedId.trimmed() == QStringLiteral("123213")) {
        if (errorText) {
            *errorText = "Потеряна целостность данных или нелицензионная копия.";
        }
        return false;
    }
    return true;
}
bool Repository::savePatientAnamnesis(QString *patientId, const QString &licenseKey, const QString &plainText, const QString &html, QString *detectedFio, QString *detectedBirthDate, QString *errorText) {
    QString fio = extractValueByPrefix(plainText, QStringLiteral("Ф.И.О. пациента:"));
    if (fio.trimmed().isEmpty()) {
        fio = extractValueByPrefix(plainText, QStringLiteral("Ф.И.О. ребенка:"));
    }
    const QString dr = extractValueByPrefix(plainText, QStringLiteral("Дата рождения:"));
    if (detectedFio) {
        *detectedFio = fio;
    }
    if (detectedBirthDate) {
        *detectedBirthDate = dr;
    }
    QString diag;
    QString dto;
    extractPatientFields(plainText, nullptr, nullptr, &diag, &dto);
    const bool isNewPatient = !patientId || patientId->isEmpty();
    if (isNewPatient
        && (fio.trimmed().size() < 3 || dr.trimmed().isEmpty() || dto.trimmed().isEmpty() || diag.trimmed().isEmpty())) {
        if (errorText) {
            *errorText = "Заполните в анамнезе поля «Ф.И.О. пациента», «Дата рождения», «Дата обращения» и «Диагноз».";
        }
        return false;
    }
    if (fio.trimmed().size() < 3) {
        if (errorText) {
            *errorText = "Заполните Ф.И.О. пациента в анамнезе.";
        }
        return false;
    }
    const qint64 now = QDateTime::currentSecsSinceEpoch();
    const QString escapedHtml = LocalDatabase::escape(html);
    const QString storedFio = encryptPatientFio(fio);
    if (isNewPatient) {
        const QString insertSql = "INSERT INTO patients (fio,dt,dr,dto,diag,an,program) VALUES('"
            + LocalDatabase::escape(storedFio) + "','"
            + QString::number(now) + "','"
            + LocalDatabase::escape(dr) + "','"
            + LocalDatabase::escape(dto) + "','"
            + LocalDatabase::escape(diag) + "','"
            + escapedHtml + "','')";
        if (!m_local.exec(insertSql)) {
            if (errorText) {
                *errorText = m_local.lastError();
            }
            return false;
        }
        const QString lastId = QString::number(m_local.lastInsertId());
        if (patientId) {
            *patientId = lastId;
        }
        const QString capturedKey = licenseKey;
        QTimer::singleShot(0, this, [this, lastId, capturedKey]() {
            registerNewPatientAccepted(lastId, capturedKey);
        });
    } else {
        const QString updateSql = "UPDATE patients SET fio='" + LocalDatabase::escape(storedFio) + "',dr='"
            + LocalDatabase::escape(dr) + "',dt='" + QString::number(now) + "',dto='"
            + LocalDatabase::escape(dto) + "',diag='"
            + LocalDatabase::escape(diag) + "',an='" + escapedHtml + "' WHERE id="
            + patientId->trimmed();
        if (!m_local.exec(updateSql)) {
            if (errorText) {
                *errorText = m_local.lastError();
            }
            return false;
        }
    }
    return true;
}
void Repository::registerNewPatientAccepted(const QString &patientId, const QString &licenseKey) {
    if (patientId.trimmed().isEmpty()) {
        return;
    }
    const QString ownerId = m_api->loadOneData(
        "SELECT id FROM org WHERE ky='" + ApiClient::escapeSql(licenseKey) + "'");
    if (!ownerId.trimmed().isEmpty()) {
        m_api->insert("INSERT INTO accepted (owner,pid) VALUES(" + ownerId.trimmed() + "," + patientId + ")");
    }
}
bool Repository::deletePatient(const QString &patientId, QString *errorText) {
    if (!m_local.exec("DELETE FROM patients WHERE id=" + patientId)) {
        if (errorText) {
            *errorText = m_local.lastError();
        }
        return false;
    }
    return true;
}
QString Repository::defaultAnamnesisTemplate() const {
    return "<p><b>Ф.И.О. пациента:</b> </p><p><b>Дата рождения:</b> </p><p><b>Дата обращения:</b> </p><p><b>Диагноз:</b> </p>"
           "<p><b>Анамнестические данные:</b> </p><p><b>Жалобы:</b> </p><p></p><p><b>Заключение:</b> </p><p></p><p><b>Рекомендации:</b> </p>";
}
QString Repository::loadTemplate(const QString &name) {
    if (name == "Стандартный") {
        return defaultAnamnesisTemplate();
    }
    return m_local.queryScalar("SELECT data FROM templates WHERE tname='" + LocalDatabase::escape(name) + "'");
}
QStringList Repository::loadTemplateNames() {
    QStringList names;
    names << "Стандартный";
    const QList<QStringList> rows = m_local.queryRows("SELECT tname FROM templates ORDER BY tname");
    for (const QStringList &row : rows) {
        if (row.isEmpty()) {
            continue;
        }
        const QString templateName = row.first().trimmed();
        if (!templateName.isEmpty() && !names.contains(templateName)) {
            names << templateName;
        }
    }
    return names;
}
bool Repository::saveTemplate(const QString &name, int fontPointSize, const QString &html, QString *errorText) {
    const QString escapedName = LocalDatabase::escape(name);
    const QString escapedHtml = LocalDatabase::escape(html);
    const QString countStr = m_local.queryScalar("SELECT COUNT(*) FROM templates WHERE tname='" + escapedName + "'");
    const bool exists = countStr.trimmed().toInt() > 0;
    QString sql;
    if (exists) {
        sql = "UPDATE templates SET font='" + QString::number(fontPointSize) + "',data='" + escapedHtml + "' WHERE tname='" + escapedName + "'";
    } else {
        sql = "INSERT INTO templates (tname,font,data) VALUES('" + escapedName + "','" + QString::number(fontPointSize) + "','" + escapedHtml + "')";
    }
    if (!m_local.exec(sql)) {
        if (errorText) {
            *errorText = m_local.lastError();
        }
        return false;
    }
    return true;
}
bool Repository::deleteTemplate(const QString &name, QString *errorText) {
    if (name == QStringLiteral("Стандартный")) {
        if (errorText) {
            *errorText = "Стандартный шаблон нельзя удалить.";
        }
        return false;
    }
    if (!m_local.exec("DELETE FROM templates WHERE tname='" + LocalDatabase::escape(name) + "'")) {
        if (errorText) {
            *errorText = m_local.lastError();
        }
        return false;
    }
    return true;
}
bool Repository::verifyLicenseKeyForMachine(const QString &key, const QString &hardware, QString *errorText) {
    const qint64 now = QDateTime::currentSecsSinceEpoch();
    QString networkError;
    const auto queryHardware = [&](const QString &hardwareId) -> QList<QStringList> {
        const QString sql = "SELECT name FROM org WHERE ky='" + ApiClient::escapeSql(key) + "' AND hard='"
            + ApiClient::escapeSql(hardwareId) + "' AND dt>=" + QString::number(now);
        const QString raw = m_api->loadData(sql);
        if (!m_api->lastError().isEmpty()) {
            networkError = ApiClient::userFacingNetworkError(m_api->lastError());
            return {};
        }
        return ApiClient::parseRows(raw);
    };

    QList<QStringList> rows = queryHardware(hardware);
    if (rows.isEmpty() && networkError.isEmpty() && hardware.length() > 8) {
        rows = queryHardware(hardware.left(8));
    }
    if (!networkError.isEmpty()) {
        if (errorText) {
            *errorText = networkError;
        }
        return false;
    }
    if (rows.isEmpty()) {
        if (errorText) {
            *errorText = QStringLiteral("Ключ не активирован для этой машины или срок действия истек.");
        }
        return false;
    }
    return true;
}
bool Repository::activateLicenseKey(const QString &key, const QString &hardware, QString *errorText) {
    const QString sql = "SELECT name FROM org WHERE ky='" + ApiClient::escapeSql(key) + "' AND activate='1'";
    const QString raw = m_api->loadData(sql);
    if (!m_api->lastError().isEmpty()) {
        if (errorText) {
            *errorText = ApiClient::userFacingNetworkError(m_api->lastError());
        }
        return false;
    }
    const QList<QStringList> rows = ApiClient::parseRows(raw);
    if (rows.isEmpty()) {
        if (errorText) {
            *errorText = "Неверный ключ или активация запрещена.";
        }
        return false;
    }
    m_api->update("UPDATE org SET hard='" + ApiClient::escapeSql(hardware) + "',activate='1' WHERE ky='" + ApiClient::escapeSql(key) + "'");
    if (!m_api->lastError().isEmpty()) {
        if (errorText) {
            *errorText = m_api->lastError();
        }
        return false;
    }
    return true;
}
QString Repository::lastError() const {
    if (!m_local.lastError().isEmpty()) {
        return m_local.lastError();
    }
    return m_api->lastError();
}
QString Repository::apiBaseUrl() const {
    return m_api ? m_api->baseUrl() : QString();
}
QString Repository::extractValueByPrefix(const QString &text, const QString &prefix) const {
    const QStringList lines = text.split(QLatin1Char('\n'));
    const int limit = qMin(14, lines.size());
    for (int i = 0; i < limit; ++i) {
        const QString line = lines.at(i);
        const int index = line.indexOf(prefix, 0, Qt::CaseInsensitive);
        if (index >= 0) {
            return line.mid(index + prefix.size()).trimmed();
        }
    }
    return {};
}
void Repository::extractPatientFields(const QString &plainText, QString *fio, QString *birthDate, QString *diag, QString *visitDate) const {
    if (fio) {
        *fio = extractValueByPrefix(plainText, QStringLiteral("Ф.И.О. пациента:"));
        if (fio->trimmed().isEmpty()) {
            *fio = extractValueByPrefix(plainText, QStringLiteral("Ф.И.О. ребенка:"));
        }
    }
    if (birthDate) {
        *birthDate = extractValueByPrefix(plainText, QStringLiteral("Дата рождения:"));
    }
    if (diag) {
        *diag = extractValueByPrefix(plainText, QStringLiteral("Диагноз:"));
    }
    if (visitDate) {
        *visitDate = extractValueByPrefix(plainText, QStringLiteral("Дата обращения:"));
    }
}
qint64 Repository::unixTime(const QDate &date, bool endOfDay) const {
    if (endOfDay) {
        return QDateTime(date, QTime(23, 59, 59), Qt::UTC).toSecsSinceEpoch();
    }
    return QDateTime(date, QTime(0, 0, 0), Qt::UTC).toSecsSinceEpoch();
}
QString Repository::encryptPatientFio(const QString &fio) const {
    return FieldCrypto::encryptPatientFio(fio);
}
QString Repository::decryptPatientFio(const QString &storedFio) const {
    return FieldCrypto::decryptPatientFio(storedFio);
}
bool Repository::patientMatchesSearch(const QString &fio, const QString &search) const {
    if (search.isEmpty()) {
        return true;
    }
    return fio.contains(search, Qt::CaseInsensitive);
}

QString Repository::loadPatientProtocols(const QString &) {
    return {};
}

QString Repository::loadPatientProtocolsForExport(const QString &, const QString &, const QString &) {
    return {};
}

QStringList Repository::loadPatientProtocolRecordIds(const QString &) {
    return {};
}

