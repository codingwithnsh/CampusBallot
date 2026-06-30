#pragma once

#include <QString>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QStandardPaths>
#include <QDir>
#include <QMutex>
#include <optional>

namespace Ballot::Storage {

class TestAdmissionStorage {
public:
    static TestAdmissionStorage& instance();

    bool initialize();
    
    bool hasVoted(const QString& admissionNumber);
    void markAsVoted(const QString& admissionNumber);
    void clear();

    bool isTestMode() const;
    void setTestMode(bool enabled);

private:
    TestAdmissionStorage() = default;
    ~TestAdmissionStorage() = default;
    TestAdmissionStorage(const TestAdmissionStorage&) = delete;
    TestAdmissionStorage& operator=(const TestAdmissionStorage&) = delete;

    void load();
    void save();

    QString m_filePath;
    QSet<QString> m_votedAdmissions;
    bool m_testMode = false;
    QMutex m_mutex;
};

} // namespace Ballot::Storage