#include "SQLiteStorageProvider.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QSqlRecord>
#include <QDebug>
#include <QUuid>
#include <QDateTime>
#include <QSet>
#include "src/core/Constants.h" // For DB_FILENAME

namespace Ballot::Storage {

// Helper functions to parse query results into Core models
static Core::Student parseStudent(const QSqlQuery& query);
static Core::User parseUser(const QSqlQuery& query);
static Core::AuditLogEntry parseAuditLog(const QSqlQuery& query);
static Core::Candidate parseCandidate(const QSqlQuery& query); // Added helper for Candidate
static Core::Election parseElection(const QSqlQuery& query); // Added helper for Election
static Core::MachineInfo parseMachineInfo(const QSqlQuery& query); // Added helper for MachineInfo
static Core::BackupEntry parseBackupEntry(const QSqlQuery& query); // Added helper for BackupEntry


SQLiteStorageProvider::~SQLiteStorageProvider() {
    disconnect();
}

bool SQLiteStorageProvider::connect(const QVariantMap& config) {
    QString dbPath = config.value("db_path", Core::Constants::DB_FILENAME).toString();

    // Use a unique connection name to avoid issues with multiple connections
    // or if QSqlDatabase::addDatabase is called multiple times.
    m_db = QSqlDatabase::addDatabase("QSQLITE", QUuid::createUuid().toString());
    m_db.setDatabaseName(dbPath);

    if (!m_db.open()) {
        qCritical() << "SQLiteStorageProvider: Failed to open database at" << dbPath << ":" << m_db.lastError().text();
        return false;
    }

    qInfo() << "SQLiteStorageProvider: Database opened successfully at" << dbPath;

    // Enable WAL mode and foreign keys for better performance and data integrity
    QSqlQuery query(m_db);
    if (!query.exec("PRAGMA journal_mode=WAL")) {
        qWarning() << "SQLiteStorageProvider: Failed to set WAL mode:" << query.lastError().text();
    }
    if (!query.exec("PRAGMA foreign_keys=ON")) {
        qWarning() << "SQLiteStorageProvider: Failed to enable foreign keys:" << query.lastError().text();
    }
    if (!query.exec("PRAGMA busy_timeout=5000")) {
        qWarning() << "SQLiteStorageProvider: Failed to set busy timeout:" << query.lastError().text();
    }

    return initSchema();
}

bool SQLiteStorageProvider::disconnect() {
    if (m_db.isOpen()) {
        QString connectionName = m_db.connectionName();
        m_db.close();
        m_db = QSqlDatabase();
        QSqlDatabase::removeDatabase(connectionName);
        qInfo() << "SQLiteStorageProvider: Database disconnected.";
        return true;
    }
    return false;
}

bool SQLiteStorageProvider::isConnected() const {
    return m_db.isOpen();
}

bool SQLiteStorageProvider::testConnection() {
    if (!m_db.isOpen()) return false;
    QSqlQuery query(m_db);
    return query.exec("SELECT 1");
}

// --- Transaction Management ---
bool SQLiteStorageProvider::beginTransaction() {
    if (!m_db.isOpen()) {
        qCritical() << "SQLiteStorageProvider: Cannot begin transaction, database not open.";
        return false;
    }
    if (!m_db.transaction()) {
        qCritical() << "SQLiteStorageProvider: Failed to begin transaction:" << m_db.lastError().text();
        return false;
    }
    qDebug() << "SQLiteStorageProvider: Transaction started.";
    return true;
}

bool SQLiteStorageProvider::commitTransaction() {
    if (!m_db.isOpen()) {
        qCritical() << "SQLiteStorageProvider: Cannot commit transaction, database not open.";
        return false;
    }
    if (!m_db.commit()) {
        qCritical() << "SQLiteStorageProvider: Failed to commit transaction:" << m_db.lastError().text();
        return false;
    }
    qDebug() << "SQLiteStorageProvider: Transaction committed.";
    return true;
}

bool SQLiteStorageProvider::rollbackTransaction() {
    if (!m_db.isOpen()) {
        qCritical() << "SQLiteStorageProvider: Cannot rollback transaction, database not open.";
        return false;
    }
    if (!m_db.rollback()) {
        qCritical() << "SQLiteStorageProvider: Failed to rollback transaction:" << m_db.lastError().text();
        return false;
    }
    qDebug() << "SQLiteStorageProvider: Transaction rolled back.";
    return true;
}

bool SQLiteStorageProvider::initSchema() {
    QSqlQuery query(m_db);

    auto exec = [&](const QString& sql, const QString& tableName = "N/A") {
        if (!query.exec(sql)) {
            qCritical() << "SQLiteStorageProvider: Schema error for table" << tableName << ":" << query.lastError().text() << "\nSQL:" << sql;
            return false;
        }
        return true;
    };

    bool ok = true;

    // Create tables
    ok &= exec(R"(CREATE TABLE IF NOT EXISTS elections (
        id TEXT PRIMARY KEY, title TEXT, description TEXT,
        start_date DATETIME, end_date DATETIME, state INTEGER DEFAULT 0,
        is_active INTEGER DEFAULT 0, created_by TEXT, created_at DATETIME,
        eligible_classes TEXT, eligible_departments TEXT,
        max_votes_per_student INTEGER DEFAULT 1,
        require_verification INTEGER DEFAULT 1,
        photo_optional INTEGER DEFAULT 0,
        verify_students INTEGER DEFAULT 1,
        student_verification_type TEXT,
        verification_file_path TEXT,
        verification_column TEXT
    ))", "elections");

    ok &= exec(R"(CREATE TABLE IF NOT EXISTS candidates (
        id TEXT PRIMARY KEY, election_id TEXT, name TEXT, photo_data BLOB, manifesto TEXT,
        party TEXT, class_name TEXT, section TEXT, symbol TEXT,
        video_url TEXT, campaign_poster_data BLOB, is_approved INTEGER DEFAULT 0, registered_at DATETIME,
        FOREIGN KEY(election_id) REFERENCES elections(id) ON DELETE CASCADE
    ))", "candidates");

    ok &= exec(R"(CREATE TABLE IF NOT EXISTS students (
        id TEXT PRIMARY KEY, name TEXT, photo_data BLOB, admission_number TEXT UNIQUE,
        roll_number TEXT, department TEXT, class_name TEXT, section TEXT, age INTEGER,
        gender TEXT, email TEXT, phone TEXT, parent_name TEXT,
        qr_code BLOB, rfid_tag TEXT, barcode BLOB, unique_voting_id TEXT UNIQUE, has_voted INTEGER DEFAULT 0,
        is_verified INTEGER DEFAULT 0, verified_at DATETIME, verified_by TEXT,
        registered_at DATETIME
    ))", "students");

    // --- CRITICAL CHANGE: votes table schema ---
    // Changed candidate_id_encrypted to candidate_id TEXT for foreign key integrity and querying.
    // The encryption logic will be handled in VoteManager.
    ok &= exec(R"(CREATE TABLE IF NOT EXISTS votes (
        id TEXT PRIMARY KEY, election_id TEXT, student_id TEXT,
        candidate_id TEXT, vote_hash BLOB, digital_signature BLOB,
        timestamp DATETIME, machine_id TEXT, is_audited INTEGER DEFAULT 0,
        FOREIGN KEY(election_id) REFERENCES elections(id) ON DELETE CASCADE,
        FOREIGN KEY(student_id) REFERENCES students(id) ON DELETE CASCADE,
        FOREIGN KEY(candidate_id) REFERENCES candidates(id) ON DELETE CASCADE,
        UNIQUE(student_id, election_id)
    ))", "votes");

    ok &= exec(R"(CREATE TABLE IF NOT EXISTS users (
        id TEXT PRIMARY KEY, name TEXT, photo_data BLOB, id_card_number TEXT, department TEXT, class_name TEXT,
        section TEXT, phone TEXT, email TEXT UNIQUE, password_hash_and_salt BLOB, role INTEGER DEFAULT 4,
        permissions TEXT, digital_signature BLOB, qr_code BLOB, is_active INTEGER DEFAULT 1,
        created_at DATETIME, last_login DATETIME
    ))", "users");

    ok &= exec(R"(CREATE TABLE IF NOT EXISTS audit_logs (
        id TEXT PRIMARY KEY, timestamp DATETIME, user_id TEXT, user_name TEXT,
        action INTEGER, details TEXT, ip_address TEXT, machine_id TEXT,
        hash BLOB, is_immutable INTEGER DEFAULT 1
    ))", "audit_logs");

    ok &= exec(R"(CREATE TABLE IF NOT EXISTS machines (
        id TEXT PRIMARY KEY, name TEXT, is_master INTEGER DEFAULT 0,
        last_seen DATETIME, ip_address TEXT, os_version TEXT,
        app_version TEXT, is_online INTEGER DEFAULT 0
    ))", "machines");

    ok &= exec(R"(CREATE TABLE IF NOT EXISTS system_settings (
        key TEXT PRIMARY KEY, value TEXT
    ))", "system_settings");

    ok &= exec(R"(CREATE TABLE IF NOT EXISTS backups (
        id TEXT PRIMARY KEY, name TEXT, created_at DATETIME,
        size_bytes INTEGER, type TEXT, checksum BLOB,
        storage_path TEXT, is_encrypted INTEGER DEFAULT 1
    ))", "backups");

    // These indexes keep the dashboard, eligibility checks, and result queries
    // responsive as elections and voter populations grow.
    ok &= exec("CREATE INDEX IF NOT EXISTS idx_candidates_election ON candidates(election_id)", "candidates");
    ok &= exec("CREATE INDEX IF NOT EXISTS idx_votes_election ON votes(election_id)", "votes");
    ok &= exec("CREATE INDEX IF NOT EXISTS idx_votes_student_election ON votes(student_id, election_id)", "votes");
    ok &= exec("CREATE INDEX IF NOT EXISTS idx_audit_logs_timestamp ON audit_logs(timestamp)", "audit_logs");

    // --- Schema Migrations ---
    // Helper to add column if it doesn't exist
    auto addColumn = [&](const QString& tableName, const QString& columnName, const QString& columnDef) {
        QSqlQuery checkQuery(m_db);
        checkQuery.prepare(QString("PRAGMA table_info(%1)").arg(tableName));
        if (!checkQuery.exec()) {
            qWarning() << "SQLiteStorageProvider: Failed to get table info for" << tableName << ":" << checkQuery.lastError().text();
            return false;
        }
        bool columnExists = false;
        while (checkQuery.next()) {
            if (checkQuery.value("name").toString() == columnName) {
                columnExists = true;
                break;
            }
        }

        if (!columnExists) {
            return exec(QString("ALTER TABLE %1 ADD COLUMN %2 %3").arg(tableName, columnName, columnDef), tableName);
        }
        return true;
    };

    // Migrate elections table
    ok &= addColumn("elections", "photo_optional", "INTEGER DEFAULT 0");
    ok &= addColumn("elections", "verify_students", "INTEGER DEFAULT 1");
    ok &= addColumn("elections", "student_verification_type", "TEXT");
    ok &= addColumn("elections", "verification_file_path", "TEXT");
    ok &= addColumn("elections", "verification_column", "TEXT");

    // Migrate candidates table
    ok &= addColumn("candidates", "photo_data", "BLOB");
    ok &= addColumn("candidates", "campaign_poster_data", "BLOB");

    // Migrate students table
    ok &= addColumn("students", "photo_data", "BLOB");
    ok &= addColumn("students", "department", "TEXT");
    ok &= addColumn("students", "qr_code", "BLOB");
    ok &= addColumn("students", "rfid_tag", "TEXT");
    ok &= addColumn("students", "barcode", "BLOB");

    // Migrate users table
    // Handle old digital_signature (password hash) to new password_hash_and_salt
    QSqlQuery checkUsersQuery(m_db);
    checkUsersQuery.prepare("PRAGMA table_info(users)");
    if (!checkUsersQuery.exec()) {
        qWarning() << "SQLiteStorageProvider: Failed to get table info for users during migration check.";
    } else {
        bool hasOldDigitalSignature = false;
        bool hasPasswordHashAndSalt = false;
        bool hasNewDigitalSignature = false;
        bool hasIdCardNumber = false;
        bool hasQrCode = false;
        bool hasPhotoData = false;

        while (checkUsersQuery.next()) {
            QString colName = checkUsersQuery.value("name").toString();
            if (colName == "digital_signature") hasOldDigitalSignature = true;
            if (colName == "password_hash_and_salt") hasPasswordHashAndSalt = true;
            if (colName == "new_digital_signature") hasNewDigitalSignature = true; // Placeholder for new column name
            if (colName == "id_card_number") hasIdCardNumber = true;
            if (colName == "qr_code") hasQrCode = true;
            if (colName == "photo_data") hasPhotoData = true;
        }

        if (hasOldDigitalSignature && !hasPasswordHashAndSalt) {
            qWarning() << "SQLiteStorageProvider: Migrating 'users' table: renaming 'digital_signature' to 'password_hash_and_salt'.";
            ok &= exec("ALTER TABLE users RENAME COLUMN digital_signature TO password_hash_and_salt", "users");
            hasPasswordHashAndSalt = true; // Mark as present after rename
        }
        // Now add the actual digital_signature column if it doesn't exist
        ok &= addColumn("users", "digital_signature", "BLOB");
        ok &= addColumn("users", "id_card_number", "TEXT");
        ok &= addColumn("users", "qr_code", "BLOB");
        ok &= addColumn("users", "photo_data", "BLOB");
    }

    // Migrate elections table (already handled in create, but ensure for older dbs)
    ok &= addColumn("elections", "eligible_classes", "TEXT");
    ok &= addColumn("elections", "eligible_departments", "TEXT");
    ok &= addColumn("elections", "max_votes_per_student", "INTEGER DEFAULT 1");
    ok &= addColumn("elections", "require_verification", "INTEGER DEFAULT 1");

    // Student eligibility filters depend on department; create this index only
    // after migrations have guaranteed the column exists on older databases.
    ok &= exec("CREATE INDEX IF NOT EXISTS idx_students_class_department ON students(class_name, department)", "students");


    // Migrate votes table if old schema exists (candidate_id_encrypted to candidate_id)
    QSqlQuery checkVotesQuery(m_db);
    checkVotesQuery.prepare("PRAGMA table_info(votes)");
    if (!checkVotesQuery.exec()) {
        qWarning() << "SQLiteStorageProvider: Failed to get table info for votes during migration check.";
    } else {
        bool hasEncryptedCandidateId = false;
        bool hasUnencryptedCandidateId = false;
        while (checkVotesQuery.next()) {
            if (checkVotesQuery.value("name").toString() == "candidate_id_encrypted") hasEncryptedCandidateId = true;
            if (checkVotesQuery.value("name").toString() == "candidate_id") hasUnencryptedCandidateId = true;
        }

        if (hasEncryptedCandidateId && !hasUnencryptedCandidateId) {
            qWarning() << "SQLiteStorageProvider: Migrating 'votes' table schema from 'candidate_id_encrypted' to 'candidate_id'. Old encrypted data will be lost.";
            // Rename old table, create new table with correct schema, copy data, drop old table
            ok &= exec("ALTER TABLE votes RENAME TO votes_old", "votes");
            ok &= exec(R"(CREATE TABLE votes (
                id TEXT PRIMARY KEY, election_id TEXT, student_id TEXT,
                candidate_id TEXT, vote_hash BLOB, digital_signature BLOB,
                timestamp DATETIME, machine_id TEXT, is_audited INTEGER DEFAULT 0,
                FOREIGN KEY(election_id) REFERENCES elections(id) ON DELETE CASCADE,
                FOREIGN KEY(student_id) REFERENCES students(id) ON DELETE CASCADE,
                FOREIGN KEY(candidate_id) REFERENCES candidates(id) ON DELETE CASCADE,
                UNIQUE(student_id, election_id)
            ))", "votes");
            // Copy data, but candidate_id will be NULL as we cannot decrypt old encrypted_candidate_id
            ok &= exec(R"(INSERT INTO votes (id, election_id, student_id, vote_hash, digital_signature, timestamp, machine_id, is_audited)
                        SELECT id, election_id, student_id, vote_hash, digital_signature, timestamp, machine_id, is_audited FROM votes_old)", "votes");
            ok &= exec("DROP TABLE votes_old", "votes");
            qInfo() << "SQLiteStorageProvider: 'votes' table migrated. Encrypted candidate IDs are now unrecoverable.";
        } else if (hasEncryptedCandidateId && hasUnencryptedCandidateId) {
            qWarning() << "SQLiteStorageProvider: 'votes' table has both 'candidate_id_encrypted' and 'candidate_id'. This is an ambiguous state. Attempting to drop 'candidate_id_encrypted'.";
            ok &= exec("ALTER TABLE votes DROP COLUMN candidate_id_encrypted", "votes");
        }
    }

    // Initial settings population if not present
    // Ensure all SystemSettings keys are present with default values if not already in DB
    auto insertDefaultSetting = [&](const QString& key, const QString& defaultValue) {
        QSqlQuery checkQuery(m_db);
        checkQuery.prepare("SELECT COUNT(*) FROM system_settings WHERE key = ?");
        checkQuery.addBindValue(key);
        if (checkQuery.exec() && checkQuery.next() && checkQuery.value(0).toInt() == 0) {
            QSqlQuery insertQuery(m_db);
            insertQuery.prepare("INSERT INTO system_settings (key, value) VALUES (?, ?)");
            insertQuery.addBindValue(key);
            insertQuery.addBindValue(defaultValue);
            if (!insertQuery.exec()) {
                qCritical() << "SQLiteStorageProvider: Failed to insert default setting for" << key << ":" << insertQuery.lastError().text();
                return false;
            }
        }
        return true;
    };

    ok &= insertDefaultSetting("master_machine_id", "");
    ok &= insertDefaultSetting("voting_status", QString::number(static_cast<int>(Core::VotingState::Idle)));
    ok &= insertDefaultSetting("allow_results_preview", "0");
    ok &= insertDefaultSetting("auto_backup_enabled", "1");
    ok &= insertDefaultSetting("backup_interval_hours", "24");
    ok &= insertDefaultSetting("session_timeout_minutes", "30");
    ok &= insertDefaultSetting("failed_login_attempts", "5");
    ok &= insertDefaultSetting("lockout_duration_minutes", "15");
    ok &= insertDefaultSetting("require_strong_password", "1");
    ok &= insertDefaultSetting("audit_all_actions", "1");
    ok &= insertDefaultSetting("encryption_enabled", "1");
    ok &= insertDefaultSetting("tamper_detection", "1");
    ok &= insertDefaultSetting("theme", "Modern");
    ok &= insertDefaultSetting("accent_color", "#0078d4");
    ok &= insertDefaultSetting("language", "en");
    ok &= insertDefaultSetting("storage_type", "sqlite");

    return ok;
}

// ---- Election Management ----

bool SQLiteStorageProvider::createElection(const Core::Election& election) {
    QSqlQuery query(m_db);
    query.prepare(R"(INSERT INTO elections (id, title, description, start_date, end_date, state, is_active, created_by, created_at, eligible_classes, eligible_departments, max_votes_per_student, require_verification, photo_optional, verify_students, student_verification_type, verification_file_path, verification_column)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?))");
    query.addBindValue(election.id);
    query.addBindValue(election.title);
    query.addBindValue(election.description);
    query.addBindValue(election.startDate);
    query.addBindValue(election.endDate);
    query.addBindValue(static_cast<int>(election.state));
    query.addBindValue(election.isActive ? 1 : 0);
    query.addBindValue(election.createdBy);
    query.addBindValue(election.createdAt);
    query.addBindValue(election.eligibleClasses.join(","));
    query.addBindValue(election.eligibleDepartments.join(","));
    query.addBindValue(election.maxVotesPerStudent);
    query.addBindValue(election.requireVerification ? 1 : 0);
    query.addBindValue(election.photoOptional ? 1 : 0);
    query.addBindValue(election.verifyStudents ? 1 : 0);
    query.addBindValue(election.studentVerificationType);
    query.addBindValue(election.verificationFilePath);
    query.addBindValue(election.verificationColumn);
    if (!query.exec()) {
        qCritical() << "SQLiteStorageProvider: Failed to create election:" << query.lastError().text();
        return false;
    }
    return true;
}

bool SQLiteStorageProvider::updateElection(const Core::Election& election) {
    QSqlQuery query(m_db);
    query.prepare(R"(UPDATE elections SET title=?, description=?, start_date=?, end_date=?, state=?, is_active=?, eligible_classes=?, eligible_departments=?, max_votes_per_student=?, require_verification=?, photo_optional=?, verify_students=?, student_verification_type=?, verification_file_path=?, verification_column=? WHERE id=?)");
    query.addBindValue(election.title);
    query.addBindValue(election.description);
    query.addBindValue(election.startDate);
    query.addBindValue(election.endDate);
    query.addBindValue(static_cast<int>(election.state));
    query.addBindValue(election.isActive ? 1 : 0);
    query.addBindValue(election.eligibleClasses.join(","));
    query.addBindValue(election.eligibleDepartments.join(","));
    query.addBindValue(election.maxVotesPerStudent);
    query.addBindValue(election.requireVerification ? 1 : 0);
    query.addBindValue(election.photoOptional ? 1 : 0);
    query.addBindValue(election.verifyStudents ? 1 : 0);
    query.addBindValue(election.studentVerificationType);
    query.addBindValue(election.verificationFilePath);
    query.addBindValue(election.verificationColumn);
    query.addBindValue(election.id);
    if (!query.exec()) {
        qCritical() << "SQLiteStorageProvider: Failed to update election:" << query.lastError().text();
        return false;
    }
    return true;
}

bool SQLiteStorageProvider::deleteElection(const QString& id) {
    // ON DELETE CASCADE on foreign keys handles deletion of associated candidates and votes
    QSqlQuery query(m_db);
    query.prepare("DELETE FROM elections WHERE id=?");
    query.addBindValue(id);

    if (!query.exec()) {
        qCritical() << "SQLiteStorageProvider: Failed to delete election:" << query.lastError().text();
        return false;
    }
    return true;
}

std::optional<Core::Election> SQLiteStorageProvider::getElection(const QString& id) {
    QSqlQuery query(m_db);
    query.prepare("SELECT * FROM elections WHERE id=?");
    query.addBindValue(id);
    if (query.exec()) {
        if (query.next()) {
            return parseElection(query);
        }
    } else {
        qCritical() << "SQLiteStorageProvider: Failed to get election:" << query.lastError().text();
    }
    return std::nullopt;
}

QList<Core::Election> SQLiteStorageProvider::getElections() {
    QList<Core::Election> list;
    QSqlQuery query("SELECT * FROM elections ORDER BY created_at DESC", m_db);
    if (query.exec()) {
        while (query.next()) {
            list.append(parseElection(query));
        }
    } else {
        qCritical() << "SQLiteStorageProvider: Failed to get elections:" << query.lastError().text();
    }
    return list;
}

QList<Core::Election> SQLiteStorageProvider::getActiveElections() {
    QList<Core::Election> list;
    QSqlQuery query("SELECT * FROM elections WHERE is_active=1 ORDER BY start_date DESC", m_db);
    if (query.exec()) {
        while (query.next()) {
            list.append(parseElection(query));
        }
    } else {
        qCritical() << "SQLiteStorageProvider: Failed to get active elections:" << query.lastError().text();
    }
    return list;
}

// ---- Candidate Management ----

bool SQLiteStorageProvider::addCandidate(const Core::Candidate& candidate) {
    QSqlQuery query(m_db);
    query.prepare(R"(INSERT INTO candidates (id, election_id, name, photo_data, manifesto, party, class_name, section, symbol, video_url, campaign_poster_data, is_approved, registered_at)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?))");
    query.addBindValue(candidate.id);
    query.addBindValue(candidate.electionId);
    query.addBindValue(candidate.name);
    query.addBindValue(candidate.photoData);
    query.addBindValue(candidate.manifesto);
    query.addBindValue(candidate.party);
    query.addBindValue(candidate.className);
    query.addBindValue(candidate.section);
    query.addBindValue(candidate.symbol);
    query.addBindValue(candidate.videoUrl);
    query.addBindValue(candidate.campaignPosterData);
    query.addBindValue(candidate.isApproved ? 1 : 0);
    query.addBindValue(candidate.registeredAt);
    if (!query.exec()) {
        qCritical() << "SQLiteStorageProvider: Failed to add candidate:" << query.lastError().text();
        return false;
    }
    return true;
}

bool SQLiteStorageProvider::updateCandidate(const Core::Candidate& candidate) {
    QSqlQuery query(m_db);
    query.prepare(R"(UPDATE candidates SET name=?, photo_data=?, manifesto=?, party=?, class_name=?, section=?, symbol=?, video_url=?, campaign_poster_data=?, is_approved=?
        WHERE id=?)");
    query.addBindValue(candidate.name);
    query.addBindValue(candidate.photoData);
    query.addBindValue(candidate.manifesto);
    query.addBindValue(candidate.party);
    query.addBindValue(candidate.className);
    query.addBindValue(candidate.section);
    query.addBindValue(candidate.symbol);
    query.addBindValue(candidate.videoUrl);
    query.addBindValue(candidate.campaignPosterData);
    query.addBindValue(candidate.isApproved ? 1 : 0);
    query.addBindValue(candidate.id);
    if (!query.exec()) {
        qCritical() << "SQLiteStorageProvider: Failed to update candidate:" << query.lastError().text();
        return false;
    }
    return true;
}

bool SQLiteStorageProvider::deleteCandidate(const QString& id) {
    QSqlQuery query(m_db);
    query.prepare("DELETE FROM candidates WHERE id=?");
    query.addBindValue(id);
    if (!query.exec()) {
        qCritical() << "SQLiteStorageProvider: Failed to delete candidate:" << query.lastError().text();
        return false;
    }
    return true;
}

std::optional<Core::Candidate> SQLiteStorageProvider::getCandidate(const QString& id) {
    QSqlQuery query(m_db);
    query.prepare("SELECT * FROM candidates WHERE id=?");
    query.addBindValue(id);
    if (query.exec()) {
        if (query.next()) {
            return parseCandidate(query);
        }
    } else {
        qCritical() << "SQLiteStorageProvider: Failed to get candidate:" << query.lastError().text();
    }
    return std::nullopt;
}

QList<Core::Candidate> SQLiteStorageProvider::getCandidates(const QString& electionId) {
    QList<Core::Candidate> list;
    QSqlQuery query(m_db);
    query.prepare("SELECT * FROM candidates WHERE election_id=? ORDER BY name");
    query.addBindValue(electionId);
    if (query.exec()) {
        while (query.next()) {
            list.append(parseCandidate(query));
        }
    } else {
        qCritical() << "SQLiteStorageProvider: Failed to get candidates for election" << electionId << ":" << query.lastError().text();
    }
    return list;
}

// ---- Student Management ----

bool SQLiteStorageProvider::addStudent(const Core::Student& student) {
    QSqlQuery query(m_db);
    query.prepare(R"(INSERT INTO students (id, name, photo_data, admission_number, roll_number, department, class_name, section, age, gender, email, phone, parent_name, qr_code, rfid_tag, barcode, unique_voting_id, has_voted, is_verified, verified_at, verified_by, registered_at)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?))");
    query.addBindValue(student.id);
    query.addBindValue(student.name);
    query.addBindValue(student.photoData);
    query.addBindValue(student.admissionNumber);
    query.addBindValue(student.rollNumber);
    query.addBindValue(student.department);
    query.addBindValue(student.className);
    query.addBindValue(student.section);
    query.addBindValue(student.age);
    query.addBindValue(student.gender);
    query.addBindValue(student.email);
    query.addBindValue(student.phone);
    query.addBindValue(student.parentName);
    query.addBindValue(student.qrCode);
    query.addBindValue(student.rfidTag);
    query.addBindValue(student.barcode);
    query.addBindValue(student.uniqueVotingId);
    query.addBindValue(student.hasVoted ? 1 : 0);
    query.addBindValue(student.isVerified ? 1 : 0);
    query.addBindValue(student.verifiedAt);
    query.addBindValue(student.verifiedBy);
    query.addBindValue(student.registeredAt.isValid() ? student.registeredAt : QDateTime::currentDateTime());
    if (!query.exec()) {
        qCritical() << "SQLiteStorageProvider: Failed to add student:" << query.lastError().text();
        return false;
    }
    return true;
}

bool SQLiteStorageProvider::updateStudent(const Core::Student& student) {
    QSqlQuery query(m_db);
    query.prepare(R"(UPDATE students SET name=?, photo_data=?, admission_number=?, roll_number=?, department=?, class_name=?, section=?, age=?, gender=?, email=?, phone=?, parent_name=?, qr_code=?, rfid_tag=?, barcode=?, unique_voting_id=?, has_voted=?, is_verified=?, verified_at=?, verified_by=? WHERE id=?)");
    query.addBindValue(student.name);
    query.addBindValue(student.photoData);
    query.addBindValue(student.admissionNumber);
    query.addBindValue(student.rollNumber);
    query.addBindValue(student.department);
    query.addBindValue(student.className);
    query.addBindValue(student.section);
    query.addBindValue(student.age);
    query.addBindValue(student.gender);
    query.addBindValue(student.email);
    query.addBindValue(student.phone);
    query.addBindValue(student.parentName);
    query.addBindValue(student.qrCode);
    query.addBindValue(student.rfidTag);
    query.addBindValue(student.barcode);
    query.addBindValue(student.uniqueVotingId);
    query.addBindValue(student.hasVoted ? 1 : 0);
    query.addBindValue(student.isVerified ? 1 : 0);
    query.addBindValue(student.verifiedAt);
    query.addBindValue(student.verifiedBy);
    query.addBindValue(student.id);
    if (!query.exec()) {
        qCritical() << "SQLiteStorageProvider: Failed to update student:" << query.lastError().text();
        return false;
    }
    return true;
}

bool SQLiteStorageProvider::deleteStudent(const QString& id) {
    QSqlQuery query(m_db);
    query.prepare("DELETE FROM students WHERE id=?");
    query.addBindValue(id);
    if (!query.exec()) {
        qCritical() << "SQLiteStorageProvider: Failed to delete student:" << query.lastError().text();
        return false;
    }
    return true;
}

std::optional<Core::Student> SQLiteStorageProvider::getStudent(const QString& id) {
    QSqlQuery query(m_db);
    query.prepare("SELECT * FROM students WHERE id=?");
    query.addBindValue(id);
    if (query.exec()) {
        if (query.next()) return std::optional<Core::Student>(parseStudent(query));
    } else {
        qCritical() << "SQLiteStorageProvider: Failed to get student by ID:" << query.lastError().text();
    }
    return std::nullopt;
}

std::optional<Core::Student> SQLiteStorageProvider::getStudentByAdmission(const QString& admissionNumber) {
    QSqlQuery query(m_db);
    query.prepare("SELECT * FROM students WHERE admission_number=?");
    query.addBindValue(admissionNumber);
    if (query.exec()) {
        if (query.next()) return std::optional<Core::Student>(parseStudent(query));
    } else {
        qCritical() << "SQLiteStorageProvider: Failed to get student by admission number:" << query.lastError().text();
    }
    return std::nullopt;
}

std::optional<Core::Student> SQLiteStorageProvider::getStudentByVotingId(const QString& votingId) {
    QSqlQuery query(m_db);
    query.prepare("SELECT * FROM students WHERE unique_voting_id=?");
    query.addBindValue(votingId);
    if (query.exec()) {
        if (query.next()) return std::optional<Core::Student>(parseStudent(query));
    } else {
        qCritical() << "SQLiteStorageProvider: Failed to get student by voting ID:" << query.lastError().text();
    }
    return std::nullopt;
}

QList<Core::Student> SQLiteStorageProvider::getStudents() {
    QList<Core::Student> list;
    QSqlQuery query("SELECT * FROM students ORDER BY name", m_db);
    if (query.exec()) {
        while (query.next()) list.append(parseStudent(query));
    } else {
        qCritical() << "SQLiteStorageProvider: Failed to get all students:" << query.lastError().text();
    }
    return list;
}

QList<Core::Student> SQLiteStorageProvider::getStudentsByClass(const QString& className) {
    QList<Core::Student> list;
    QSqlQuery query(m_db);
    query.prepare("SELECT * FROM students WHERE class_name=? ORDER BY name");
    query.addBindValue(className);
    if (query.exec()) {
        while (query.next()) list.append(parseStudent(query));
    } else {
        qCritical() << "SQLiteStorageProvider: Failed to get students by class:" << query.lastError().text();
    }
    return list;
}

QList<Core::Student> SQLiteStorageProvider::getEligibleVoters(const QString& electionId) {
    QList<Core::Student> list;
    auto election = getElection(electionId);
    if (!election) {
        qWarning() << "SQLiteStorageProvider: Election" << electionId << "not found for eligible voters query.";
        return list;
    }

    QSqlQuery query(m_db);
    QStringList conditions = {"NOT EXISTS (SELECT 1 FROM votes WHERE votes.student_id=students.id AND votes.election_id=?)"};
    QList<QVariant> bindValues;
    bindValues.append(electionId);

    auto addInCondition = [&](const QString& column, const QStringList& values) {
        if (values.isEmpty()) return;
        QStringList placeholders;
        for (const auto& value : values) {
            placeholders << "?";
            bindValues.append(value);
        }
        conditions << QString("%1 IN (%2)").arg(column, placeholders.join(","));
    };
    addInCondition("class_name", election->eligibleClasses);
    addInCondition("department", election->eligibleDepartments);

    QString sql = "SELECT * FROM students";
    if (!conditions.isEmpty()) {
        sql += " WHERE " + conditions.join(" AND ");
    }
    sql += " ORDER BY name";

    query.prepare(sql);
    for(const QVariant& val : bindValues) {
        query.addBindValue(val);
    }

    if (query.exec()) {
        while (query.next()) list.append(parseStudent(query));
    } else {
        qCritical() << "SQLiteStorageProvider: Failed to get eligible voters for election" << electionId << ":" << query.lastError().text();
    }
    return list;
}

int SQLiteStorageProvider::getStudentCount() {
    QSqlQuery query("SELECT COUNT(*) FROM students", m_db);
    if (query.exec() && query.next()) return query.value(0).toInt();
    qCritical() << "SQLiteStorageProvider: Failed to get student count:" << query.lastError().text();
    return 0;
}

int SQLiteStorageProvider::getVoterCount(const QString& electionId) {
    auto election = getElection(electionId);
    if (!election) {
        qWarning() << "SQLiteStorageProvider: Election" << electionId << "not found for voter count query.";
        return 0;
    }

    QSqlQuery query(m_db);
    QStringList conditions;
    QList<QVariant> bindValues;

    auto addInCondition = [&](const QString& column, const QStringList& values) {
        if (values.isEmpty()) return;
        QStringList placeholders;
        for (const auto& value : values) {
            placeholders << "?";
            bindValues.append(value);
        }
        conditions << QString("%1 IN (%2)").arg(column, placeholders.join(","));
    };
    addInCondition("class_name", election->eligibleClasses);
    addInCondition("department", election->eligibleDepartments);

    QString sql = "SELECT COUNT(*) FROM students";
    if (!conditions.isEmpty()) {
        sql += " WHERE " + conditions.join(" AND ");
    }

    query.prepare(sql);
    for(const QVariant& val : bindValues) {
        query.addBindValue(val);
    }

    if (query.exec() && query.next()) return query.value(0).toInt();
    qCritical() << "SQLiteStorageProvider: Failed to get voter count for election" << electionId << ":" << query.lastError().text();
    return 0;
}


// ---- User Management ----

bool SQLiteStorageProvider::createUser(const Core::User& user) {
    QSqlQuery query(m_db);
    query.prepare(R"(INSERT INTO users (id, name, photo_data, id_card_number, department, class_name, section,
        phone, email, password_hash_and_salt, role, permissions, digital_signature, qr_code, is_active, created_at, last_login)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?))");
    query.addBindValue(user.id);
    query.addBindValue(user.name);
    query.addBindValue(user.photoData);
    query.addBindValue(user.idCardNumber);
    query.addBindValue(user.department);
    query.addBindValue(user.className);
    query.addBindValue(user.section);
    query.addBindValue(user.phone);
    query.addBindValue(user.email);
    query.addBindValue(user.passwordHashAndSalt);
    query.addBindValue(static_cast<int>(user.role));
    query.addBindValue(user.permissions.join(","));
    query.addBindValue(user.digitalSignature);
    query.addBindValue(user.qrCode);
    query.addBindValue(user.isActive ? 1 : 0);
    query.addBindValue(user.createdAt.isValid() ? user.createdAt : QDateTime::currentDateTime());
    query.addBindValue(user.lastLogin);
    if (!query.exec()) {
        qCritical() << "SQLiteStorageProvider: Failed to create user:" << query.lastError().text();
        return false;
    }
    return true;
}

bool SQLiteStorageProvider::updateUser(const Core::User& user) {
    QSqlQuery query(m_db);
    query.prepare(R"(UPDATE users SET name=?, photo_data=?, id_card_number=?, department=?, class_name=?, section=?, phone=?, email=?, password_hash_and_salt=?, role=?, permissions=?, digital_signature=?, qr_code=?, is_active=?, last_login=? WHERE id=?)");
    query.addBindValue(user.name);
    query.addBindValue(user.photoData);
    query.addBindValue(user.idCardNumber);
    query.addBindValue(user.department);
    query.addBindValue(user.className);
    query.addBindValue(user.section);
    query.addBindValue(user.phone);
    query.addBindValue(user.email);
    query.addBindValue(user.passwordHashAndSalt);
    query.addBindValue(static_cast<int>(user.role));
    query.addBindValue(user.permissions.join(","));
    query.addBindValue(user.digitalSignature);
    query.addBindValue(user.qrCode);
    query.addBindValue(user.isActive ? 1 : 0);
    query.addBindValue(user.lastLogin);
    query.addBindValue(user.id);
    if (!query.exec()) {
        qCritical() << "SQLiteStorageProvider: Failed to update user:" << query.lastError().text();
        return false;
    }
    return true;
}

bool SQLiteStorageProvider::updateUserPassword(const QString& userId, const QByteArray& newPasswordHashAndSalt) {
    QSqlQuery query(m_db);
    query.prepare("UPDATE users SET password_hash_and_salt=? WHERE id=?");
    query.addBindValue(newPasswordHashAndSalt);
    query.addBindValue(userId);
    if (!query.exec()) {
        qCritical() << "SQLiteStorageProvider: Failed to update user password for ID" << userId << ":" << query.lastError().text();
        return false;
    }
    return true;
}

bool SQLiteStorageProvider::deleteUser(const QString& id) {
    QSqlQuery query(m_db);
    query.prepare("DELETE FROM users WHERE id=?");
    query.addBindValue(id);
    if (!query.exec()) {
        qCritical() << "SQLiteStorageProvider: Failed to delete user:" << query.lastError().text();
        return false;
    }
    return true;
}

std::optional<Core::User> SQLiteStorageProvider::getUser(const QString& id) {
    QSqlQuery query(m_db);
    query.prepare("SELECT * FROM users WHERE id=?");
    query.addBindValue(id);
    if (query.exec()) {
        if (query.next()) return std::optional<Core::User>(parseUser(query));
    } else {
        qCritical() << "SQLiteStorageProvider: Failed to get user by ID:" << query.lastError().text();
    }
    return std::nullopt;
}

std::optional<Core::User> SQLiteStorageProvider::getUserByEmail(const QString& email) {
    QSqlQuery query(m_db);
    query.prepare("SELECT * FROM users WHERE lower(trim(email))=lower(trim(?))");
    query.addBindValue(email.trimmed());
    if (query.exec()) {
        if (query.next()) return std::optional<Core::User>(parseUser(query));
    } else {
        qCritical() << "SQLiteStorageProvider: Failed to get user by email:" << query.lastError().text();
    }
    return std::nullopt;
}

QList<Core::User> SQLiteStorageProvider::getUsers() {
    QList<Core::User> list;
    QSqlQuery query("SELECT * FROM users ORDER BY name", m_db);
    if (query.exec()) {
        while (query.next()) list.append(parseUser(query));
    } else {
        qCritical() << "SQLiteStorageProvider: Failed to get all users:" << query.lastError().text();
    }
    return list;
}

QList<Core::User> SQLiteStorageProvider::getUsersByRole(Core::UserRole role) {
    QList<Core::User> list;
    QSqlQuery query(m_db);
    query.prepare("SELECT * FROM users WHERE role=? ORDER BY name");
    query.addBindValue(static_cast<int>(role));
    if (query.exec()) {
        while (query.next()) list.append(parseUser(query));
    } else {
        qCritical() << "SQLiteStorageProvider: Failed to get users by role:" << query.lastError().text();
    }
    return list;
}

// ---- Voting ----

bool SQLiteStorageProvider::castVote(const Core::Vote& vote) {
    QSqlQuery query(m_db);
    query.prepare(R"(INSERT INTO votes (id, election_id, student_id, candidate_id, vote_hash, digital_signature, timestamp, machine_id)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?))");
    query.addBindValue(vote.id);
    query.addBindValue(vote.electionId);
    query.addBindValue(vote.studentId);
    query.addBindValue(vote.candidateId); // Now using the unencrypted candidateId
    query.addBindValue(vote.voteHash);
    query.addBindValue(vote.digitalSignature);
    query.addBindValue(vote.timestamp);
    query.addBindValue(vote.machineId);
    if (!query.exec()) {
        qCritical() << "SQLiteStorageProvider: Failed to cast vote:" << query.lastError().text();
        return false;
    }
    return true;
}

bool SQLiteStorageProvider::hasStudentVoted(const QString& studentId, const QString& electionId) {
    QSqlQuery query(m_db);
    query.prepare("SELECT COUNT(*) FROM votes WHERE student_id=? AND election_id=?");
    query.addBindValue(studentId);
    query.addBindValue(electionId);
    if (query.exec()) {
        if (query.next()) return query.value(0).toInt() > 0;
    } else {
        qCritical() << "SQLiteStorageProvider: Failed to check if student voted:" << query.lastError().text();
    }
    return false;
}

int SQLiteStorageProvider::getVoteCount(const QString& electionId) {
    QSqlQuery query(m_db);
    query.prepare("SELECT COUNT(*) FROM votes WHERE election_id=?");
    query.addBindValue(electionId);
    if (query.exec()) {
        if (query.next()) return query.value(0).toInt();
    } else {
        qCritical() << "SQLiteStorageProvider: Failed to get vote count:" << query.lastError().text();
    }
    return 0;
}

int SQLiteStorageProvider::getTotalVotesCast(const QString& electionId) {
    return getVoteCount(electionId);
}

QList<Core::ElectionResult> SQLiteStorageProvider::getResults(const QString& electionId) {
    QList<Core::ElectionResult> results;
    int totalVotes = getVoteCount(electionId);
    if (totalVotes == 0) return results;

    QSqlQuery query(m_db);
    query.prepare(R"(
        SELECT c.id, c.name, c.party, COUNT(v.id) as vote_count
        FROM votes v
        JOIN candidates c ON v.candidate_id = c.id
        WHERE v.election_id = ?
        GROUP BY c.id, c.name, c.party
        ORDER BY vote_count DESC
    )");
    query.addBindValue(electionId);

    if (query.exec()) {
        while (query.next()) {
            Core::ElectionResult r;
            r.electionId = electionId;
            r.candidateId = query.value("id").toString();
            r.candidateName = query.value("name").toString();
            r.party = query.value("party").toString();
            r.voteCount = query.value("vote_count").toInt();
            r.percentage = (static_cast<double>(r.voteCount) / totalVotes) * 100.0;
            results.append(r);
        }
    } else {
        qCritical() << "SQLiteStorageProvider: Failed to get election results:" << query.lastError().text();
    }

    return results;
}

QList<Core::ElectionResult> SQLiteStorageProvider::getResultsByClass(const QString& electionId, const QString& className) {
    QList<Core::ElectionResult> results;
    int totalVotes = getVoteCount(electionId); // Total votes in the election, not just for the class
    if (totalVotes == 0) return results;

    QSqlQuery query(m_db);
    query.prepare(R"(
        SELECT c.id, c.name, c.party, COUNT(v.id) as vote_count
        FROM votes v
        JOIN candidates c ON v.candidate_id = c.id
        JOIN students s ON v.student_id = s.id
        WHERE v.election_id = ? AND s.class_name = ?
        GROUP BY c.id, c.name, c.party
        ORDER BY vote_count DESC
    )");
    query.addBindValue(electionId);
    query.addBindValue(className);

    if (query.exec()) {
        while (query.next()) {
            Core::ElectionResult r;
            r.electionId = electionId;
            r.candidateId = query.value("id").toString();
            r.candidateName = query.value("name").toString();
            r.party = query.value("party").toString();
            r.voteCount = query.value("vote_count").toInt();
            // Percentage should be relative to total votes in this class, or total votes overall?
            // For now, relative to total votes cast in the election.
            r.percentage = (static_cast<double>(r.voteCount) / totalVotes) * 100.0;
            results.append(r);
        }
    } else {
        qCritical() << "SQLiteStorageProvider: Failed to get election results by class:" << query.lastError().text();
    }
    return results;
}

QList<Core::ElectionResult> SQLiteStorageProvider::getResultsByDepartment(const QString& electionId, const QString& department) {
    QList<Core::ElectionResult> results;
    int totalVotes = getVoteCount(electionId);
    if (totalVotes == 0) return results;

    QSqlQuery query(m_db);
    query.prepare(R"(
        SELECT c.id, c.name, c.party, COUNT(v.id) as vote_count
        FROM votes v
        JOIN candidates c ON v.candidate_id = c.id
        JOIN students s ON v.student_id = s.id
        WHERE v.election_id = ? AND s.department = ? -- Assuming student has a department field
        GROUP BY c.id, c.name, c.party
        ORDER BY vote_count DESC
    )");
    query.addBindValue(electionId);
    query.addBindValue(department);

    if (query.exec()) {
        while (query.next()) {
            Core::ElectionResult r;
            r.electionId = electionId;
            r.candidateId = query.value("id").toString();
            r.candidateName = query.value("name").toString();
            r.party = query.value("party").toString();
            r.voteCount = query.value("vote_count").toInt();
            r.percentage = (static_cast<double>(r.voteCount) / totalVotes) * 100.0;
            results.append(r);
        }
    } else {
        qCritical() << "SQLiteStorageProvider: Failed to get election results by department:" << query.lastError().text();
    }
    return results;
}

QList<Core::ElectionResult> SQLiteStorageProvider::getResultsByGender(const QString& electionId, const QString& gender) {
    QList<Core::ElectionResult> results;
    int totalVotes = getVoteCount(electionId);
    if (totalVotes == 0) return results;

    QSqlQuery query(m_db);
    query.prepare(R"(
        SELECT c.id, c.name, c.party, COUNT(v.id) as vote_count
        FROM votes v
        JOIN candidates c ON v.candidate_id = c.id
        JOIN students s ON v.student_id = s.id
        WHERE v.election_id = ? AND s.gender = ?
        GROUP BY c.id, c.name, c.party
        ORDER BY vote_count DESC
    )");
    query.addBindValue(electionId);
    query.addBindValue(gender);

    if (query.exec()) {
        while (query.next()) {
            Core::ElectionResult r;
            r.electionId = electionId;
            r.candidateId = query.value("id").toString();
            r.candidateName = query.value("name").toString();
            r.party = query.value("party").toString();
            r.voteCount = query.value("vote_count").toInt();
            r.percentage = (static_cast<double>(r.voteCount) / totalVotes) * 100.0;
            results.append(r);
        }
    } else {
        qCritical() << "SQLiteStorageProvider: Failed to get election results by gender:" << query.lastError().text();
    }
    return results;
}

// ---- Audit ----

bool SQLiteStorageProvider::logAction(const Core::AuditLogEntry& log) {
    QSqlQuery query(m_db);
    query.prepare(R"(INSERT INTO audit_logs (id, timestamp, user_id, user_name, action, details, ip_address, machine_id, hash, is_immutable)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?))");
    query.addBindValue(log.id);
    query.addBindValue(log.timestamp);
    query.addBindValue(log.userId);
    query.addBindValue(log.userName);
    query.addBindValue(static_cast<int>(log.action));
    query.addBindValue(log.details);
    query.addBindValue(log.ipAddress);
    query.addBindValue(log.machineId);
    query.addBindValue(log.hash);
    query.addBindValue(log.isImmutable ? 1 : 0);
    if (!query.exec()) {
        qCritical() << "SQLiteStorageProvider: Failed to log audit action:" << query.lastError().text();
        return false;
    }
    return true;
}

QList<Core::AuditLogEntry> SQLiteStorageProvider::getAuditLogs(const QDateTime& from, const QDateTime& to) {
    QList<Core::AuditLogEntry> list;
    QSqlQuery query(m_db);
    QString sql = "SELECT * FROM audit_logs";
    if (from.isValid() && to.isValid()) {
        query.prepare(sql + " WHERE timestamp BETWEEN ? AND ? ORDER BY rowid DESC");
        query.addBindValue(from);
        query.addBindValue(to);
    } else {
        query.prepare(sql + " ORDER BY rowid DESC");
    }

    if (query.exec()) {
        while (query.next()) list.append(parseAuditLog(query));
    } else {
        qCritical() << "SQLiteStorageProvider: Failed to get audit logs:" << query.lastError().text();
    }
    return list;
}

QList<Core::AuditLogEntry> SQLiteStorageProvider::getAuditLogsByUser(const QString& userId) {
    QList<Core::AuditLogEntry> list;
    QSqlQuery query(m_db);
    query.prepare("SELECT * FROM audit_logs WHERE user_id=? ORDER BY timestamp DESC");
    query.addBindValue(userId);
    if (query.exec()) {
        while (query.next()) list.append(parseAuditLog(query));
    } else {
        qCritical() << "SQLiteStorageProvider: Failed to get audit logs by user:" << query.lastError().text();
    }
    return list;
}

QList<Core::AuditLogEntry> SQLiteStorageProvider::getAuditLogsByAction(Core::AuditAction action) {
    QList<Core::AuditLogEntry> list;
    QSqlQuery query(m_db);
    query.prepare("SELECT * FROM audit_logs WHERE action=? ORDER BY timestamp DESC");
    query.addBindValue(static_cast<int>(action));
    if (query.exec()) {
        while (query.next()) list.append(parseAuditLog(query));
    } else {
        qCritical() << "SQLiteStorageProvider: Failed to get audit logs by action:" << query.lastError().text();
    }
    return list;
}

int SQLiteStorageProvider::getAuditLogCount() {
    QSqlQuery query("SELECT COUNT(*) FROM audit_logs", m_db);
    if (query.exec() && query.next()) return query.value(0).toInt();
    qCritical() << "SQLiteStorageProvider: Failed to get audit log count:" << query.lastError().text();
    return 0;
}

// ---- Machine Management ----

bool SQLiteStorageProvider::registerMachine(const Core::MachineInfo& machine) {
    QSqlQuery query(m_db);
    query.prepare(R"(INSERT OR REPLACE INTO machines (id, name, is_master, last_seen, ip_address, os_version, app_version, is_online)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?))");
    query.addBindValue(machine.id);
    query.addBindValue(machine.name);
    query.addBindValue(machine.isMaster ? 1 : 0);
    query.addBindValue(machine.lastSeen);
    query.addBindValue(machine.ipAddress);
    query.addBindValue(machine.osVersion);
    query.addBindValue(machine.appVersion);
    query.addBindValue(machine.isOnline ? 1 : 0);
    if (!query.exec()) {
        qCritical() << "SQLiteStorageProvider: Failed to register machine:" << query.lastError().text();
        return false;
    }
    return true;
}

bool SQLiteStorageProvider::updateMachine(const Core::MachineInfo& machine) {
    return registerMachine(machine); // INSERT OR REPLACE handles update
}

QList<Core::MachineInfo> SQLiteStorageProvider::getMachines() {
    QList<Core::MachineInfo> list;
    QSqlQuery query("SELECT * FROM machines ORDER BY name", m_db);
    if (query.exec()) {
        while (query.next()) {
            list.append(parseMachineInfo(query));
        }
    } else {
        qCritical() << "SQLiteStorageProvider: Failed to get machines:" << query.lastError().text();
    }
    return list;
}

std::optional<Core::MachineInfo> SQLiteStorageProvider::getMachine(const QString& id) {
    QSqlQuery query(m_db);
    query.prepare("SELECT * FROM machines WHERE id=?");
    query.addBindValue(id);
    if (query.exec()) {
        if (query.next()) {
            return parseMachineInfo(query);
        }
    } else {
        qCritical() << "SQLiteStorageProvider: Failed to get machine by ID:" << query.lastError().text();
    }
    return std::nullopt;
}

// ---- System Settings ----

std::optional<Core::SystemSettings> SQLiteStorageProvider::getSystemSettings() {
    Core::SystemSettings settings;
    QSqlQuery query("SELECT key, value FROM system_settings", m_db);
    if (query.exec()) {
        while (query.next()) {
            QString key = query.value(0).toString();
            QString value = query.value(1).toString();
            if (key == "master_machine_id") settings.masterMachineId = value;
            else if (key == "voting_status") settings.votingStatus = static_cast<Core::VotingState>(value.toInt());
            else if (key == "allow_results_preview") settings.allowResultsPreview = (value == "1");
            else if (key == "auto_backup_enabled") settings.autoBackupEnabled = (value == "1");
            else if (key == "backup_interval_hours") settings.backupIntervalHours = value.toInt();
            else if (key == "session_timeout_minutes") settings.sessionTimeoutMinutes = value.toInt();
            else if (key == "failed_login_attempts") settings.failedLoginAttempts = value.toInt();
            else if (key == "lockout_duration_minutes") settings.lockoutDurationMinutes = value.toInt();
            else if (key == "require_strong_password") settings.requireStrongPassword = (value == "1");
            else if (key == "audit_all_actions") settings.auditAllActions = (value == "1");
            else if (key == "encryption_enabled") settings.encryptionEnabled = (value == "1");
            else if (key == "tamper_detection") settings.tamperDetection = (value == "1");
            else if (key == "theme") settings.theme = value;
            else if (key == "accent_color") settings.accentColor = value;
            else if (key == "language") settings.language = value;
            else if (key == "storage_type") settings.storageType = value;
        }
        return settings;
    } else {
        qCritical() << "SQLiteStorageProvider: Failed to get system settings:" << query.lastError().text();
    }
    return std::nullopt;
}

bool SQLiteStorageProvider::updateSystemSettings(const Core::SystemSettings& settings) {
    auto set = [&](const QString& key, const QString& value) {
        QSqlQuery q(m_db);
        q.prepare("INSERT OR REPLACE INTO system_settings (key, value) VALUES (?, ?)");
        q.addBindValue(key);
        q.addBindValue(value);
        if (!q.exec()) {
            qCritical() << "SQLiteStorageProvider: Failed to update setting" << key << ":" << q.lastError().text();
            return false;
        }
        return true;
    };
    bool ok = true;
    ok &= set("master_machine_id", settings.masterMachineId);
    ok &= set("voting_status", QString::number(static_cast<int>(settings.votingStatus)));
    ok &= set("allow_results_preview", settings.allowResultsPreview ? "1" : "0");
    ok &= set("auto_backup_enabled", settings.autoBackupEnabled ? "1" : "0");
    ok &= set("backup_interval_hours", QString::number(settings.backupIntervalHours));
    ok &= set("session_timeout_minutes", QString::number(settings.sessionTimeoutMinutes));
    ok &= set("failed_login_attempts", QString::number(settings.failedLoginAttempts));
    ok &= set("lockout_duration_minutes", QString::number(settings.lockoutDurationMinutes));
    ok &= set("require_strong_password", settings.requireStrongPassword ? "1" : "0");
    ok &= set("audit_all_actions", settings.auditAllActions ? "1" : "0");
    ok &= set("encryption_enabled", settings.encryptionEnabled ? "1" : "0");
    ok &= set("tamper_detection", settings.tamperDetection ? "1" : "0");
    ok &= set("theme", settings.theme);
    ok &= set("accent_color", settings.accentColor);
    ok &= set("language", settings.language);
    ok &= set("storage_type", settings.storageType);
    return ok;
}

// ---- Backup ----

bool SQLiteStorageProvider::saveBackupRecord(const Core::BackupEntry& backup) {
    QSqlQuery query(m_db);
    query.prepare(R"(INSERT INTO backups (id, name, created_at, size_bytes, type, checksum, storage_path, is_encrypted)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?))");
    query.addBindValue(backup.id);
    query.addBindValue(backup.name);
    query.addBindValue(backup.createdAt);
    query.addBindValue(backup.sizeBytes);
    query.addBindValue(backup.type);
    query.addBindValue(backup.checksum);
    query.addBindValue(backup.storagePath);
    query.addBindValue(backup.isEncrypted ? 1 : 0);
    if (!query.exec()) {
        qCritical() << "SQLiteStorageProvider: Failed to save backup record:" << query.lastError().text();
        return false;
    }
    return true;
}

QList<Core::BackupEntry> SQLiteStorageProvider::getBackupHistory() {
    QList<Core::BackupEntry> list;
    QSqlQuery query("SELECT * FROM backups ORDER BY created_at DESC", m_db);
    if (query.exec()) {
        while (query.next()) {
            list.append(parseBackupEntry(query));
        }
    } else {
        qCritical() << "SQLiteStorageProvider: Failed to get backup history:" << query.lastError().text();
    }
    return list;
}

std::optional<Core::BackupEntry> SQLiteStorageProvider::getBackup(const QString& id) {
    QSqlQuery query(m_db);
    query.prepare("SELECT * FROM backups WHERE id=?");
    query.addBindValue(id);
    if (query.exec()) {
        if (query.next()) return std::optional<Core::BackupEntry>(parseBackupEntry(query));
    } else {
        qCritical() << "SQLiteStorageProvider: Failed to get backup by ID:" << query.lastError().text();
    }
    return std::nullopt;
}

bool SQLiteStorageProvider::deleteBackupRecord(const QString& id) {
    QSqlQuery query(m_db);
    query.prepare("DELETE FROM backups WHERE id=?");
    query.addBindValue(id);
    if (!query.exec()) {
        qCritical() << "SQLiteStorageProvider: Failed to delete backup record:" << query.lastError().text();
        return false;
    }
    return true;
}

// ---- Bulk Operations ----

bool SQLiteStorageProvider::bulkAddStudents(const QList<Core::Student>& students) {
    if (!beginTransaction()) return false;
    for (const auto& s : students) {
        if (!addStudent(s)) {
            qCritical() << "SQLiteStorageProvider: Bulk add student failed for" << s.id;
            rollbackTransaction();
            return false;
        }
    }
    return commitTransaction();
}

bool SQLiteStorageProvider::bulkAddCandidates(const QList<Core::Candidate>& candidates) {
    if (!beginTransaction()) return false;
    for (const auto& c : candidates) {
        if (!addCandidate(c)) {
            qCritical() << "SQLiteStorageProvider: Bulk add candidate failed for" << c.id;
            rollbackTransaction();
            return false;
        }
    }
    return commitTransaction();
}

bool SQLiteStorageProvider::clearElectionData(const QString& electionId) {
    if (!beginTransaction()) return false;
    // ON DELETE CASCADE on elections table handles votes and candidates
    QSqlQuery query(m_db);
    query.prepare("DELETE FROM elections WHERE id=?");
    query.addBindValue(electionId);
    if (!query.exec()) {
        qCritical() << "SQLiteStorageProvider: Failed to clear election data for election" << electionId << ":" << query.lastError().text();
        rollbackTransaction();
        return false;
    }
    return commitTransaction();
}

// ---- Helper Functions to parse QSqlQuery to Core Models ----

static Core::Election parseElection(const QSqlQuery& query) {
    Core::Election e;
    e.id = query.value("id").toString();
    e.title = query.value("title").toString();
    e.description = query.value("description").toString();
    e.startDate = query.value("start_date").toDateTime();
    e.endDate = query.value("end_date").toDateTime();
    e.state = static_cast<Core::VotingState>(query.value("state").toInt());
    e.isActive = query.value("is_active").toBool();
    e.createdBy = query.value("created_by").toString();
    e.createdAt = query.value("created_at").toDateTime();
    e.eligibleClasses = query.value("eligible_classes").toString().split(",", Qt::SkipEmptyParts);
    e.eligibleDepartments = query.value("eligible_departments").toString().split(",", Qt::SkipEmptyParts);
    e.maxVotesPerStudent = query.value("max_votes_per_student").toInt();
    e.requireVerification = query.value("require_verification").toBool();
    e.photoOptional = query.value("photo_optional").toBool();
    e.verifyStudents = query.value("verify_students").toBool();
    e.studentVerificationType = query.value("student_verification_type").toString();
    e.verificationFilePath = query.value("verification_file_path").toString();
    e.verificationColumn = query.value("verification_column").toString();
    return e;
}

static Core::Candidate parseCandidate(const QSqlQuery& query) {
    Core::Candidate c;
    c.id = query.value("id").toString();
    c.electionId = query.value("election_id").toString();
    c.name = query.value("name").toString();
    c.photoData = query.value("photo_data").toByteArray();
    c.manifesto = query.value("manifesto").toString();
    c.party = query.value("party").toString();
    c.className = query.value("class_name").toString();
    c.section = query.value("section").toString();
    c.symbol = query.value("symbol").toString();
    c.videoUrl = query.value("video_url").toString();
    c.campaignPosterData = query.value("campaign_poster_data").toByteArray();
    c.isApproved = query.value("is_approved").toBool();
    c.registeredAt = query.value("registered_at").toDateTime();
    return c;
}

static Core::Student parseStudent(const QSqlQuery& query) {
    Core::Student s;
    s.id = query.value("id").toString();
    s.name = query.value("name").toString();
    s.photoData = query.value("photo_data").toByteArray();
    s.admissionNumber = query.value("admission_number").toString();
    s.rollNumber = query.value("roll_number").toString();
    s.department = query.value("department").toString();
    s.className = query.value("class_name").toString();
    s.section = query.value("section").toString();
    s.age = query.value("age").toInt();
    s.gender = query.value("gender").toString();
    s.email = query.value("email").toString();
    s.phone = query.value("phone").toString();
    s.parentName = query.value("parent_name").toString();
    s.qrCode = query.value("qr_code").toByteArray();
    s.rfidTag = query.value("rfid_tag").toString();
    s.barcode = query.value("barcode").toByteArray();
    s.uniqueVotingId = query.value("unique_voting_id").toString();
    s.hasVoted = query.value("has_voted").toBool();
    s.isVerified = query.value("is_verified").toBool();
    s.verifiedAt = query.value("verified_at").toDateTime();
    s.verifiedBy = query.value("verified_by").toString();
    s.registeredAt = query.value("registered_at").toDateTime();
    return s;
}

static Core::User parseUser(const QSqlQuery& query) {
    Core::User u;
    u.id = query.value("id").toString();
    u.name = query.value("name").toString();
    u.photoData = query.value("photo_data").toByteArray();
    u.idCardNumber = query.value("id_card_number").toString();
    u.department = query.value("department").toString();
    u.className = query.value("class_name").toString();
    u.section = query.value("section").toString();
    u.phone = query.value("phone").toString();
    u.email = query.value("email").toString();
    u.passwordHashAndSalt = query.value("password_hash_and_salt").toByteArray();
    u.role = static_cast<Core::UserRole>(query.value("role").toInt());
    u.permissions = query.value("permissions").toString().split(",", Qt::SkipEmptyParts);
    u.digitalSignature = query.value("digital_signature").toByteArray();
    u.qrCode = query.value("qr_code").toByteArray();
    u.isActive = query.value("is_active").toBool();
    u.createdAt = query.value("created_at").toDateTime();
    u.lastLogin = query.value("last_login").toDateTime();
    return u;
}

static Core::AuditLogEntry parseAuditLog(const QSqlQuery& query) {
    Core::AuditLogEntry l;
    l.id = query.value("id").toString();
    l.timestamp = query.value("timestamp").toDateTime();
    l.userId = query.value("user_id").toString();
    l.userName = query.value("user_name").toString();
    l.action = static_cast<Core::AuditAction>(query.value("action").toInt());
    l.details = query.value("details").toString();
    l.ipAddress = query.value("ip_address").toString();
    l.machineId = query.value("machine_id").toString();
    l.hash = query.value("hash").toByteArray();
    l.isImmutable = query.value("is_immutable").toBool();
    return l;
}

static Core::MachineInfo parseMachineInfo(const QSqlQuery& query) {
    Core::MachineInfo m;
    m.id = query.value("id").toString();
    m.name = query.value("name").toString();
    m.isMaster = query.value("is_master").toBool();
    m.lastSeen = query.value("last_seen").toDateTime();
    m.ipAddress = query.value("ip_address").toString();
    m.osVersion = query.value("os_version").toString();
    m.appVersion = query.value("app_version").toString();
    m.isOnline = query.value("is_online").toBool();
    return m;
}

static Core::BackupEntry parseBackupEntry(const QSqlQuery& query) {
    Core::BackupEntry b;
    b.id = query.value("id").toString();
    b.name = query.value("name").toString();
    b.createdAt = query.value("created_at").toDateTime();
    b.sizeBytes = query.value("size_bytes").toLongLong();
    b.type = query.value("type").toString();
    b.checksum = query.value("checksum").toByteArray();
    b.storagePath = query.value("storage_path").toString();
    b.isEncrypted = query.value("is_encrypted").toBool();
    return b;
}

} // namespace Ballot::Storage
