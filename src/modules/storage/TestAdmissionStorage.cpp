#include "TestAdmissionStorage.h"
#include "src/core/Constants.h" // For application name or other constants
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QMutexLocker>
#include <QDebug> // For logging

namespace Ballot::Storage {

/**
 * @brief Returns the singleton instance of the TestAdmissionStorage.
 * @return Reference to the TestAdmissionStorage instance.
 */
TestAdmissionStorage& TestAdmissionStorage::instance() {
    static TestAdmissionStorage inst;
    return inst;
}

/**
 * @brief Initializes the TestAdmissionStorage, setting up the file path and loading existing data.
 * This storage is intended for a "test mode" or simplified voting scenario where student
 * admission numbers are tracked locally to prevent duplicate votes without a full student database.
 * @return True if initialization is successful, false otherwise.
 */
bool TestAdmissionStorage::initialize() {
    qDebug() << "TestAdmissionStorage: Initializing...";
    // Use AppDataLocation for platform-independent storage of application data
    QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (dataDir.isEmpty()) {
        qCritical() << "TestAdmissionStorage: Failed to find writable AppDataLocation.";
        return false;
    }

    QDir dir(dataDir);
    if (!dir.mkpath(".")) { // Ensure the directory exists
        qCritical() << "TestAdmissionStorage: Failed to create data directory:" << dataDir;
        return false;
    }

    // The file name for storing test admission data
    m_filePath = dir.filePath("test_admissions.json");
    qDebug() << "TestAdmissionStorage: Data file path set to:" << m_filePath;

    load(); // Load any previously saved test data
    qInfo() << "TestAdmissionStorage: Initialization complete. Test mode is" << (m_testMode ? "enabled" : "disabled");
    return true;
}

/**
 * @brief Checks if a student with the given admission number has already voted in test mode.
 * @param admissionNumber The admission number to check.
 * @return True if the student has voted, false otherwise.
 */
bool TestAdmissionStorage::hasVoted(const QString& admissionNumber) {
    QMutexLocker locker(&m_mutex); // Protect shared data
    bool voted = m_votedAdmissions.contains(admissionNumber);
    qDebug() << "TestAdmissionStorage: Checking if" << admissionNumber << "has voted. Result:" << voted;
    return voted;
}

/**
 * @brief Marks a student with the given admission number as having voted in test mode.
 * The change is persisted to the JSON file.
 * @param admissionNumber The admission number to mark.
 */
void TestAdmissionStorage::markAsVoted(const QString& admissionNumber) {
    QMutexLocker locker(&m_mutex); // Protect shared data
    if (!m_votedAdmissions.contains(admissionNumber)) {
        m_votedAdmissions.insert(admissionNumber);
        qInfo() << "TestAdmissionStorage: Marked" << admissionNumber << "as voted.";
        save(); // Persist the change
    } else {
        qDebug() << "TestAdmissionStorage: Admission number" << admissionNumber << "was already marked as voted.";
    }
}

/**
 * @brief Clears all recorded voted admission numbers.
 * This effectively resets the test voting state. The change is persisted.
 */
void TestAdmissionStorage::clear() {
    QMutexLocker locker(&m_mutex); // Protect shared data
    m_votedAdmissions.clear();
    qInfo() << "TestAdmissionStorage: Cleared all voted admissions.";
    save(); // Persist the cleared state
}

/**
 * @brief Returns the current test mode status.
 * @return True if test mode is enabled, false otherwise.
 */
bool TestAdmissionStorage::isTestMode() const {
    qDebug() << "TestAdmissionStorage: Current test mode status is" << m_testMode;
    return m_testMode;
}

/**
 * @brief Sets the test mode status. If test mode is disabled, all recorded votes are cleared.
 * @param enabled True to enable test mode, false to disable.
 */
void TestAdmissionStorage::setTestMode(bool enabled) {
    if (m_testMode == enabled) {
        qDebug() << "TestAdmissionStorage: Test mode already" << (enabled ? "enabled" : "disabled") << ". No change.";
        return;
    }
    m_testMode = enabled;
    qInfo() << "TestAdmissionStorage: Test mode set to" << (enabled ? "enabled" : "disabled");
    if (!enabled) {
        qDebug() << "TestAdmissionStorage: Test mode disabled, clearing voted admissions.";
        clear(); // Clear data when test mode is turned off
    }
    save(); // Persist the test mode status
}

/**
 * @brief Loads voted admission numbers and test mode status from the JSON file.
 */
void TestAdmissionStorage::load() {
    QMutexLocker locker(&m_mutex); // Protect shared data during load
    QFile file(m_filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "TestAdmissionStorage: Could not open file for reading:" << m_filePath << "-" << file.errorString();
        return;
    }

    QByteArray jsonData = file.readAll();
    file.close();

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(jsonData, &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        qWarning() << "TestAdmissionStorage: Failed to parse JSON from" << m_filePath << ":" << parseError.errorString();
        return;
    }

    if (!doc.isObject()) {
        qWarning() << "TestAdmissionStorage: JSON document is not an object in" << m_filePath;
        return;
    }

    QJsonObject obj = doc.object();
    m_testMode = obj["testMode"].toBool(false);
    QJsonArray array = obj["votedAdmissions"].toArray();
    m_votedAdmissions.clear(); // Clear existing data before loading
    for (const auto& val : array) {
        if (val.isString()) {
            m_votedAdmissions.insert(val.toString());
        } else {
            qWarning() << "TestAdmissionStorage: Non-string value found in votedAdmissions array. Skipping.";
        }
    }
    qInfo() << "TestAdmissionStorage: Loaded" << m_votedAdmissions.size() << "voted admissions from" << m_filePath;
}

/**
 * @brief Saves the current voted admission numbers and test mode status to the JSON file.
 */
void TestAdmissionStorage::save() {
    QMutexLocker locker(&m_mutex); // Protect shared data during save

    QJsonObject obj;
    obj["testMode"] = m_testMode;
    QJsonArray array;
    for (const auto& adm : m_votedAdmissions) {
        array.append(adm);
    }
    obj["votedAdmissions"] = array;

    QFile file(m_filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        qCritical() << "TestAdmissionStorage: Could not open file for writing:" << m_filePath << "-" << file.errorString();
        return;
    }

    qint64 bytesWritten = file.write(QJsonDocument(obj).toJson(QJsonDocument::Indented)); // Use Indented for readability
    if (bytesWritten == -1) {
        qCritical() << "TestAdmissionStorage: Failed to write data to file:" << m_filePath << "-" << file.errorString();
    } else {
        qDebug() << "TestAdmissionStorage: Saved" << m_votedAdmissions.size() << "voted admissions to" << m_filePath << "(" << bytesWritten << "bytes)";
    }
    file.close();
}

} // namespace Ballot::Storage