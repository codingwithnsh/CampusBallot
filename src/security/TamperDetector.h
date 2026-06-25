#pragma once

#include <QString>
#include <QByteArray>
#include <QDateTime>
#include <QObject>

namespace Ballot::Security {

class TamperDetector : public QObject {
    Q_OBJECT
public:
    explicit TamperDetector(QObject* parent = nullptr);
    ~TamperDetector() override;

    bool verifyDatabaseIntegrity(const QString& dbPath);
    bool verifyFileIntegrity(const QString& filePath, const QByteArray& expectedChecksum);
    bool verifyChecksumChain(const QStringList& logFiles);
    QByteArray calculateChecksum(const QString& filePath);

    bool startIntegrityMonitoring(const QString& directory);
    void stopIntegrityMonitoring();
    bool isMonitoring() const;

    static bool detectDebugger();
    static bool detectSandbox();
    static bool detectVirtualMachine();

signals:
    void tamperDetected(const QString& details);
    void integrityCheckPassed();
    void integrityCheckFailed(const QString& reason);

private:
    class Impl;
    std::unique_ptr<Impl> d;
};

} // namespace Ballot::Security
