#include "TestAdmissionStorage.h"
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QMutexLocker>

namespace Ballot::Storage {

TestAdmissionStorage& TestAdmissionStorage::instance() {
    static TestAdmissionStorage inst;
    return inst;
}

bool TestAdmissionStorage::initialize() {
    QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dataDir);
    m_filePath = dataDir + "/test_admissions.json";
    load();
    return true;
}

bool TestAdmissionStorage::hasVoted(const QString& admissionNumber) {
    QMutexLocker locker(&m_mutex);
    return m_votedAdmissions.contains(admissionNumber);
}

void TestAdmissionStorage::markAsVoted(const QString& admissionNumber) {
    QMutexLocker locker(&m_mutex);
    m_votedAdmissions.insert(admissionNumber);
    save();
}

void TestAdmissionStorage::clear() {
    QMutexLocker locker(&m_mutex);
    m_votedAdmissions.clear();
    save();
}

bool TestAdmissionStorage::isTestMode() const {
    return m_testMode;
}

void TestAdmissionStorage::setTestMode(bool enabled) {
    m_testMode = enabled;
    if (!enabled) {
        clear();
    }
}

void TestAdmissionStorage::load() {
    QFile file(m_filePath);
    if (file.open(QIODevice::ReadOnly)) {
        QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        file.close();
        if (!doc.isNull() && doc.isObject()) {
            QJsonObject obj = doc.object();
            m_testMode = obj["testMode"].toBool(false);
            QJsonArray array = obj["votedAdmissions"].toArray();
            for (const auto& val : array) {
                m_votedAdmissions.insert(val.toString());
            }
        }
    }
}

void TestAdmissionStorage::save() {
    QJsonObject obj;
    obj["testMode"] = m_testMode;
    QJsonArray array;
    for (const auto& adm : m_votedAdmissions) {
        array.append(adm);
    }
    obj["votedAdmissions"] = array;

    QFile file(m_filePath);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(QJsonDocument(obj).toJson());
        file.close();
    }
}

} // namespace Ballot::Storage