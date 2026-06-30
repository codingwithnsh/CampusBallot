#include "SQLiteStorageProvider.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QSqlRecord>
#include <QDebug>
#include <QUuid>
#include <QDateTime>
#include <QSet>

namespace Ballot::Storage {

static Core::Student parseStudent(const QSqlQuery& query);
static Core::User parseUser(const QSqlQuery& query);
static Core::AuditLogEntry parseAuditLog(const QSqlQuery& query);

SQLiteStorageProvider::~SQLiteStorageProvider() { disconnect(); }

bool SQLiteStorageProvider::connect(const QVariantMap& config) {
    QString dbPath = config.value("db_path", config.value("path", "ballot.db")).toString();

    m_db = QSqlDatabase::addDatabase("QSQLITE", "main_connection");
    m_db.setDatabaseName(dbPath);

    if (!m_db.open()) {
        qCritical() << "Failed to open database:" << m_db.lastError().text();
        return false;
    }

    // Enable WAL mode and foreign keys
    QSqlQuery query(m_db);
    query.exec("PRAGMA journal_mode=WAL");
    query.exec("PRAGMA foreign_keys=ON");
    query.exec("PRAGMA busy_timeout=5000");

    return initSchema();
}

bool SQLiteStorageProvider::disconnect() {
    if (m_db.isOpen()) { m_db.close(); return true; }
    return false;
}

bool SQLiteStorageProvider::isConnected() const { return m_db.isOpen(); }
bool SQLiteStorageProvider::testConnection() { return m_db.isOpen(); }

bool SQLiteStorageProvider::initSchema() {
    QSqlQuery query(m_db);

    auto exec = [&](const QString& sql) {
        if (!query.exec(sql)) {
            qCritical() << "Schema error:" << query.lastError().text() << "\nSQL:" << sql;
            return false;
        }
        return true;
    };

    bool ok = true;
    ok &= exec(R"(CREATE TABLE IF NOT EXISTS elections (
        id TEXT PRIMARY KEY, title TEXT, description TEXT,
        start_date DATETIME, end_date DATETIME, state INTEGER DEFAULT 0,
        is_active INTEGER DEFAULT 0, created_by TEXT, created_at DATETIME,
        eligible_classes TEXT, eligible_departments TEXT,
        max_votes_per_student INTEGER DEFAULT 1,
        require_verification INTEGER DEFAULT 1
    ))");

    ok &= exec(R"(CREATE TABLE IF NOT EXISTS candidates (
        id TEXT PRIMARY KEY, election_id TEXT, name TEXT, manifesto TEXT,
        party TEXT, class_name TEXT, section TEXT, symbol TEXT,
        video_url TEXT, is_approved INTEGER DEFAULT 0, registered_at DATETIME,
        FOREIGN KEY(election_id) REFERENCES elections(id)
    ))");

    ok &= exec(R"(CREATE TABLE IF NOT EXISTS students (
        id TEXT PRIMARY KEY, name TEXT, admission_number TEXT UNIQUE,
        roll_number TEXT, class_name TEXT, section TEXT, age INTEGER,
        gender TEXT, email TEXT, phone TEXT, parent_name TEXT,
        unique_voting_id TEXT UNIQUE, has_voted INTEGER DEFAULT 0,
        is_verified INTEGER DEFAULT 0, verified_at DATETIME, verified_by TEXT,
        registered_at DATETIME
    ))");

    ok &= exec(R"(CREATE TABLE IF NOT EXISTS votes (
        id TEXT PRIMARY KEY, election_id TEXT, student_id TEXT,
        candidate_id_encrypted TEXT, vote_hash BLOB, digital_signature BLOB,
        timestamp DATETIME, machine_id TEXT, is_audited INTEGER DEFAULT 0,
        FOREIGN KEY(election_id) REFERENCES elections(id),
        UNIQUE(student_id, election_id)
    ))");

    ok &= exec(R"(CREATE TABLE IF NOT EXISTS users (
        id TEXT PRIMARY KEY, name TEXT, department TEXT, class_name TEXT,
        section TEXT, phone TEXT, email TEXT UNIQUE, role INTEGER DEFAULT 4,
        permissions TEXT, digital_signature BLOB, is_active INTEGER DEFAULT 1,
        created_at DATETIME, last_login DATETIME
    ))");

    ok &= exec(R"(CREATE TABLE IF NOT EXISTS audit_logs (
        id TEXT PRIMARY KEY, timestamp DATETIME, user_id TEXT, user_name TEXT,
        action INTEGER, details TEXT, ip_address TEXT, machine_id TEXT,
        hash BLOB, is_immutable INTEGER DEFAULT 1
    ))");

    ok &= exec(R"(CREATE TABLE IF NOT EXISTS machines (
        id TEXT PRIMARY KEY, name TEXT, is_master INTEGER DEFAULT 0,
        last_seen DATETIME, ip_address TEXT, os_version TEXT,
        app_version TEXT, is_online INTEGER DEFAULT 0
    ))");

    ok &= exec(R"(CREATE TABLE IF NOT EXISTS system_settings (
        key TEXT PRIMARY KEY, value TEXT
    ))");

    ok &= exec(R"(CREATE TABLE IF NOT EXISTS backups (
        id TEXT PRIMARY KEY, name TEXT, created_at DATETIME,
        size_bytes INTEGER, type TEXT, checksum BLOB,
        storage_path TEXT, is_encrypted INTEGER DEFAULT 1
    ))");

    // Migrate databases created by earlier releases.
    QSet<QString> electionColumns;
    if (query.exec("PRAGMA table_info(elections)")) {
        while (query.next()) electionColumns.insert(query.value("name").toString());
    }
    if (!electionColumns.contains("eligible_classes")) ok &= exec("ALTER TABLE elections ADD COLUMN eligible_classes TEXT");
    if (!electionColumns.contains("eligible_departments")) ok &= exec("ALTER TABLE elections ADD COLUMN eligible_departments TEXT");
    if (!electionColumns.contains("max_votes_per_student")) ok &= exec("ALTER TABLE elections ADD COLUMN max_votes_per_student INTEGER DEFAULT 1");

    ok &= exec("INSERT OR IGNORE INTO system_settings (key, value) VALUES ('voting_status', '0')");
    ok &= exec("INSERT OR IGNORE INTO system_settings (key, value) VALUES ('master_machine_id', '')");
    ok &= exec("INSERT OR IGNORE INTO system_settings (key, value) VALUES ('allow_results_preview', '0')");
    ok &= exec("INSERT OR IGNORE INTO system_settings (key, value) VALUES ('auto_backup_enabled', '1')");
    ok &= exec("INSERT OR IGNORE INTO system_settings (key, value) VALUES ('backup_interval_hours', '24')");
    ok &= exec("INSERT OR IGNORE INTO system_settings (key, value) VALUES ('session_timeout_minutes', '30')");
    ok &= exec("INSERT OR IGNORE INTO system_settings (key, value) VALUES ('audit_all_actions', '1')");
    ok &= exec("INSERT OR IGNORE INTO system_settings (key, value) VALUES ('encryption_enabled', '1')");
    ok &= exec("INSERT OR IGNORE INTO system_settings (key, value) VALUES ('tamper_detection', '1')");
    ok &= exec("INSERT OR IGNORE INTO system_settings (key, value) VALUES ('theme', 'dark')");
    ok &= exec("INSERT OR IGNORE INTO system_settings (key, value) VALUES ('accent_color', '#0078d4')");
    ok &= exec("INSERT OR IGNORE INTO system_settings (key, value) VALUES ('language', 'en')");

    return ok;
}

// ---- Election Management ----

bool SQLiteStorageProvider::createElection(const Core::Election& election) {
    QSqlQuery query(m_db);
    query.prepare(R"(INSERT INTO elections (id, title, description, start_date, end_date, state, is_active, created_by, created_at, eligible_classes, eligible_departments, max_votes_per_student, require_verification)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?))");
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
    return query.exec();
}

bool SQLiteStorageProvider::updateElection(const Core::Election& election) {
    QSqlQuery query(m_db);
    query.prepare(R"(UPDATE elections SET title=?, description=?, start_date=?, end_date=?, state=?, is_active=?, eligible_classes=?, eligible_departments=?, max_votes_per_student=?, require_verification=? WHERE id=?)");
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
    query.addBindValue(election.id);
    return query.exec();
}

bool SQLiteStorageProvider::deleteElection(const QString& id) {
    if (!m_db.transaction()) return false;
    QSqlQuery query(m_db);
    query.prepare("DELETE FROM votes WHERE election_id=?");
    query.addBindValue(id);
    if (!query.exec()) { m_db.rollback(); return false; }
    query.prepare("DELETE FROM candidates WHERE election_id=?");
    query.addBindValue(id);
    if (!query.exec()) { m_db.rollback(); return false; }
    query.prepare("DELETE FROM elections WHERE id=?");
    query.addBindValue(id);
    if (!query.exec()) { m_db.rollback(); return false; }
    return m_db.commit();
}

std::optional<Core::Election> SQLiteStorageProvider::getElection(const QString& id) {
    QSqlQuery query(m_db);
    query.prepare("SELECT * FROM elections WHERE id=?");
    query.addBindValue(id);
    if (query.exec() && query.next()) {
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
        return e;
    }
    return std::nullopt;
}

QList<Core::Election> SQLiteStorageProvider::getElections() {
    QList<Core::Election> list;
    QSqlQuery query("SELECT * FROM elections ORDER BY created_at DESC", m_db);
    while (query.next()) {
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
        list.append(e);
    }
    return list;
}

QList<Core::Election> SQLiteStorageProvider::getActiveElections() {
    QList<Core::Election> list;
    QSqlQuery query("SELECT * FROM elections WHERE is_active=1 ORDER BY start_date DESC", m_db);
    while (query.next()) {
        Core::Election e;
        e.id = query.value("id").toString();
        e.title = query.value("title").toString();
        e.isActive = true;
        list.append(e);
    }
    return list;
}

// ---- Candidate Management ----

bool SQLiteStorageProvider::addCandidate(const Core::Candidate& candidate) {
    QSqlQuery query(m_db);
    query.prepare(R"(INSERT INTO candidates (id, election_id, name, manifesto, party, class_name, section, symbol, video_url, is_approved, registered_at)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?))");
    query.addBindValue(candidate.id);
    query.addBindValue(candidate.electionId);
    query.addBindValue(candidate.name);
    query.addBindValue(candidate.manifesto);
    query.addBindValue(candidate.party);
    query.addBindValue(candidate.className);
    query.addBindValue(candidate.section);
    query.addBindValue(candidate.symbol);
    query.addBindValue(candidate.videoUrl);
    query.addBindValue(candidate.isApproved ? 1 : 0);
    query.addBindValue(candidate.registeredAt);
    return query.exec();
}

bool SQLiteStorageProvider::updateCandidate(const Core::Candidate& candidate) {
    QSqlQuery query(m_db);
    query.prepare(R"(UPDATE candidates SET name=?, manifesto=?, party=?, class_name=?, section=?, symbol=?, video_url=?, is_approved=?
        WHERE id=?)");
    query.addBindValue(candidate.name);
    query.addBindValue(candidate.manifesto);
    query.addBindValue(candidate.party);
    query.addBindValue(candidate.className);
    query.addBindValue(candidate.section);
    query.addBindValue(candidate.symbol);
    query.addBindValue(candidate.videoUrl);
    query.addBindValue(candidate.isApproved ? 1 : 0);
    query.addBindValue(candidate.id);
    return query.exec();
}

bool SQLiteStorageProvider::deleteCandidate(const QString& id) {
    QSqlQuery query(m_db);
    query.prepare("DELETE FROM candidates WHERE id=?");
    query.addBindValue(id);
    return query.exec();
}

std::optional<Core::Candidate> SQLiteStorageProvider::getCandidate(const QString& id) {
    QSqlQuery query(m_db);
    query.prepare("SELECT * FROM candidates WHERE id=?");
    query.addBindValue(id);
    if (query.exec() && query.next()) {
        Core::Candidate c;
        c.id = query.value("id").toString();
        c.electionId = query.value("election_id").toString();
        c.name = query.value("name").toString();
        c.manifesto = query.value("manifesto").toString();
        c.party = query.value("party").toString();
        c.className = query.value("class_name").toString();
        c.section = query.value("section").toString();
        c.symbol = query.value("symbol").toString();
        c.videoUrl = query.value("video_url").toString();
        c.isApproved = query.value("is_approved").toBool();
        c.registeredAt = query.value("registered_at").toDateTime();
        return c;
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
            Core::Candidate c;
            c.id = query.value("id").toString();
            c.electionId = query.value("election_id").toString();
            c.name = query.value("name").toString();
            c.manifesto = query.value("manifesto").toString();
            c.party = query.value("party").toString();
            c.className = query.value("class_name").toString();
            c.section = query.value("section").toString();
            c.symbol = query.value("symbol").toString();
            c.videoUrl = query.value("video_url").toString();
            c.isApproved = query.value("is_approved").toBool();
            list.append(c);
        }
    }
    return list;
}

// ---- Student Management ----

bool SQLiteStorageProvider::addStudent(const Core::Student& student) {
    QSqlQuery query(m_db);
    query.prepare(R"(INSERT INTO students (id, name, admission_number, roll_number, class_name, section, age, gender, email, phone, parent_name, unique_voting_id, has_voted, is_verified, registered_at)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?))");
    query.addBindValue(student.id);
    query.addBindValue(student.name);
    query.addBindValue(student.admissionNumber);
    query.addBindValue(student.rollNumber);
    query.addBindValue(student.className);
    query.addBindValue(student.section);
    query.addBindValue(student.age);
    query.addBindValue(student.gender);
    query.addBindValue(student.email);
    query.addBindValue(student.phone);
    query.addBindValue(student.parentName);
    query.addBindValue(student.uniqueVotingId);
    query.addBindValue(student.hasVoted ? 1 : 0);
    query.addBindValue(student.isVerified ? 1 : 0);
    query.addBindValue(student.registeredAt.isValid() ? student.registeredAt : QDateTime::currentDateTime());
    return query.exec();
}

bool SQLiteStorageProvider::updateStudent(const Core::Student& student) {
    QSqlQuery query(m_db);
    query.prepare(R"(UPDATE students SET name=?, admission_number=?, roll_number=?, class_name=?, section=?, age=?, gender=?, email=?, phone=?, parent_name=?, has_voted=?, is_verified=? WHERE id=?)");
    query.addBindValue(student.name);
    query.addBindValue(student.admissionNumber);
    query.addBindValue(student.rollNumber);
    query.addBindValue(student.className);
    query.addBindValue(student.section);
    query.addBindValue(student.age);
    query.addBindValue(student.gender);
    query.addBindValue(student.email);
    query.addBindValue(student.phone);
    query.addBindValue(student.parentName);
    query.addBindValue(student.hasVoted ? 1 : 0);
    query.addBindValue(student.isVerified ? 1 : 0);
    query.addBindValue(student.id);
    return query.exec();
}

bool SQLiteStorageProvider::deleteStudent(const QString& id) {
    QSqlQuery query(m_db);
    query.prepare("DELETE FROM students WHERE id=?");
    query.addBindValue(id);
    return query.exec();
}

std::optional<Core::Student> SQLiteStorageProvider::getStudent(const QString& id) {
    QSqlQuery query(m_db);
    query.prepare("SELECT * FROM students WHERE id=?");
    query.addBindValue(id);
    if (query.exec() && query.next()) return std::optional<Core::Student>(parseStudent(query));
    return std::nullopt;
}

std::optional<Core::Student> SQLiteStorageProvider::getStudentByAdmission(const QString& admissionNumber) {
    QSqlQuery query(m_db);
    query.prepare("SELECT * FROM students WHERE admission_number=?");
    query.addBindValue(admissionNumber);
    if (query.exec() && query.next()) return std::optional<Core::Student>(parseStudent(query));
    return std::nullopt;
}

std::optional<Core::Student> SQLiteStorageProvider::getStudentByVotingId(const QString& votingId) {
    QSqlQuery query(m_db);
    query.prepare("SELECT * FROM students WHERE unique_voting_id=?");
    query.addBindValue(votingId);
    if (query.exec() && query.next()) return std::optional<Core::Student>(parseStudent(query));
    return std::nullopt;
}

QList<Core::Student> SQLiteStorageProvider::getStudents() {
    QList<Core::Student> list;
    QSqlQuery query("SELECT * FROM students ORDER BY name", m_db);
    while (query.next()) list.append(parseStudent(query));
    return list;
}

QList<Core::Student> SQLiteStorageProvider::getStudentsByClass(const QString& className) {
    QList<Core::Student> list;
    QSqlQuery query(m_db);
    query.prepare("SELECT * FROM students WHERE class_name=? ORDER BY name");
    query.addBindValue(className);
    if (query.exec()) {
        while (query.next()) list.append(parseStudent(query));
    }
    return list;
}

QList<Core::Student> SQLiteStorageProvider::getEligibleVoters(const QString& electionId) {
    QList<Core::Student> list;
    auto election = getElection(electionId);
    if (!election) return list;

    QSqlQuery query(m_db);
    QStringList conditions = {"NOT EXISTS (SELECT 1 FROM votes WHERE votes.student_id=students.id AND votes.election_id=?)"};
    if (!election->eligibleClasses.isEmpty()) conditions << "instr(',' || ? || ',', ',' || class_name || ',') > 0";
    query.prepare("SELECT * FROM students WHERE " + conditions.join(" AND ") + " ORDER BY name");
    query.addBindValue(electionId);
    if (!election->eligibleClasses.isEmpty()) query.addBindValue(election->eligibleClasses.join(","));
    if (query.exec()) {
        while (query.next()) list.append(parseStudent(query));
    }
    return list;
}

int SQLiteStorageProvider::getStudentCount() {
    QSqlQuery query("SELECT COUNT(*) FROM students", m_db);
    if (query.exec() && query.next()) return query.value(0).toInt();
    return 0;
}

int SQLiteStorageProvider::getVoterCount(const QString& electionId) {
    auto election = getElection(electionId);
    if (!election) return 0;
    return getStudentCount();
}

// ---- User Management ----

bool SQLiteStorageProvider::createUser(const Core::User& user) {
    QSqlQuery query(m_db);
    query.prepare(R"(INSERT INTO users (id, name, department, class_name, section, phone, email, role, permissions, digital_signature, is_active, created_at)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?))");
    query.addBindValue(user.id);
    query.addBindValue(user.name);
    query.addBindValue(user.department);
    query.addBindValue(user.className);
    query.addBindValue(user.section);
    query.addBindValue(user.phone);
    query.addBindValue(user.email);
    query.addBindValue(static_cast<int>(user.role));
    query.addBindValue(user.permissions.join(","));
    query.addBindValue(user.digitalSignature);
    query.addBindValue(user.isActive ? 1 : 0);
    query.addBindValue(user.createdAt.isValid() ? user.createdAt : QDateTime::currentDateTime());
    return query.exec();
}

bool SQLiteStorageProvider::updateUser(const Core::User& user) {
    QSqlQuery query(m_db);
    query.prepare(R"(UPDATE users SET name=?, department=?, class_name=?, section=?, phone=?, email=?, role=?, permissions=?, digital_signature=?, is_active=? WHERE id=?)");
    query.addBindValue(user.name);
    query.addBindValue(user.department);
    query.addBindValue(user.className);
    query.addBindValue(user.section);
    query.addBindValue(user.phone);
    query.addBindValue(user.email);
    query.addBindValue(static_cast<int>(user.role));
    query.addBindValue(user.permissions.join(","));
    query.addBindValue(user.digitalSignature);
    query.addBindValue(user.isActive ? 1 : 0);
    query.addBindValue(user.id);
    return query.exec();
}

bool SQLiteStorageProvider::deleteUser(const QString& id) {
    QSqlQuery query(m_db);
    query.prepare("DELETE FROM users WHERE id=?");
    query.addBindValue(id);
    return query.exec();
}

std::optional<Core::User> SQLiteStorageProvider::getUser(const QString& id) {
    QSqlQuery query(m_db);
    query.prepare("SELECT * FROM users WHERE id=?");
    query.addBindValue(id);
    if (query.exec() && query.next()) return std::optional<Core::User>(parseUser(query));
    return std::nullopt;
}

std::optional<Core::User> SQLiteStorageProvider::getUserByEmail(const QString& email) {
    QSqlQuery query(m_db);
    query.prepare("SELECT * FROM users WHERE email=?");
    query.addBindValue(email);
    if (query.exec() && query.next()) return std::optional<Core::User>(parseUser(query));
    return std::nullopt;
}

QList<Core::User> SQLiteStorageProvider::getUsers() {
    QList<Core::User> list;
    QSqlQuery query("SELECT * FROM users ORDER BY name", m_db);
    while (query.next()) list.append(parseUser(query));
    return list;
}

QList<Core::User> SQLiteStorageProvider::getUsersByRole(Core::UserRole role) {
    QList<Core::User> list;
    QSqlQuery query(m_db);
    query.prepare("SELECT * FROM users WHERE role=? ORDER BY name");
    query.addBindValue(static_cast<int>(role));
    if (query.exec()) {
        while (query.next()) list.append(parseUser(query));
    }
    return list;
}

// ---- Voting ----

bool SQLiteStorageProvider::castVote(const Core::Vote& vote) {
    QSqlQuery query(m_db);
    query.prepare(R"(INSERT INTO votes (id, election_id, student_id, candidate_id_encrypted, vote_hash, digital_signature, timestamp, machine_id)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?))");
    query.addBindValue(vote.id);
    query.addBindValue(vote.electionId);
    query.addBindValue(vote.studentId);
    query.addBindValue(vote.encryptedCandidateId);
    query.addBindValue(vote.voteHash);
    query.addBindValue(vote.digitalSignature);
    query.addBindValue(vote.timestamp);
    query.addBindValue(vote.machineId);
    return query.exec();
}

bool SQLiteStorageProvider::hasStudentVoted(const QString& studentId, const QString& electionId) {
    QSqlQuery query(m_db);
    query.prepare("SELECT COUNT(*) FROM votes WHERE student_id=? AND election_id=?");
    query.addBindValue(studentId);
    query.addBindValue(electionId);
    if (query.exec() && query.next()) return query.value(0).toInt() > 0;
    return false;
}

int SQLiteStorageProvider::getVoteCount(const QString& electionId) {
    QSqlQuery query(m_db);
    query.prepare("SELECT COUNT(*) FROM votes WHERE election_id=?");
    query.addBindValue(electionId);
    if (query.exec() && query.next()) return query.value(0).toInt();
    return 0;
}

int SQLiteStorageProvider::getTotalVotesCast(const QString& electionId) {
    return getVoteCount(electionId);
}

QList<Core::ElectionResult> SQLiteStorageProvider::getResults(const QString& electionId) {
    QList<Core::ElectionResult> results;
    int totalVotes = getVoteCount(electionId);
    if (totalVotes == 0) return results;

    QMap<QString, int> voteCounts;
    QSqlQuery query(m_db);
    query.prepare("SELECT candidate_id_encrypted FROM votes WHERE election_id=?");
    query.addBindValue(electionId);
    if (query.exec()) {
        while (query.next()) {
            QString enc = query.value(0).toString();
            voteCounts[enc]++;
        }
    }

    auto candidates = getCandidates(electionId);
    for (const auto& c : candidates) {
        Core::ElectionResult r;
        r.electionId = electionId;
        r.candidateId = c.id;
        r.candidateName = c.name;
        r.party = c.party;
        r.voteCount = voteCounts.value(c.id, 0);
        r.percentage = (static_cast<double>(r.voteCount) / totalVotes) * 100.0;
        results.append(r);
    }

    std::sort(results.begin(), results.end(), [](const Core::ElectionResult& a, const Core::ElectionResult& b) {
        return a.voteCount > b.voteCount;
    });

    return results;
}

QList<Core::ElectionResult> SQLiteStorageProvider::getResultsByClass(const QString& electionId, const QString& className) {
    return getResults(electionId);
}

QList<Core::ElectionResult> SQLiteStorageProvider::getResultsByDepartment(const QString& electionId, const QString& department) {
    return getResults(electionId);
}

QList<Core::ElectionResult> SQLiteStorageProvider::getResultsByGender(const QString& electionId, const QString& gender) {
    return getResults(electionId);
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
    return query.exec();
}

QList<Core::AuditLogEntry> SQLiteStorageProvider::getAuditLogs(const QDateTime& from, const QDateTime& to) {
    QList<Core::AuditLogEntry> list;
    QSqlQuery query(m_db);
    QString sql = "SELECT * FROM audit_logs";
    if (from.isValid() && to.isValid()) {
        query.prepare(sql + " WHERE timestamp BETWEEN ? AND ? ORDER BY timestamp DESC");
        query.addBindValue(from);
        query.addBindValue(to);
    } else {
        query.prepare(sql + " ORDER BY timestamp DESC LIMIT 1000");
    }
    if (query.exec()) {
        while (query.next()) list.append(parseAuditLog(query));
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
    }
    return list;
}

int SQLiteStorageProvider::getAuditLogCount() {
    QSqlQuery query("SELECT COUNT(*) FROM audit_logs", m_db);
    if (query.exec() && query.next()) return query.value(0).toInt();
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
    return query.exec();
}

bool SQLiteStorageProvider::updateMachine(const Core::MachineInfo& machine) {
    return registerMachine(machine);
}

QList<Core::MachineInfo> SQLiteStorageProvider::getMachines() {
    QList<Core::MachineInfo> list;
    QSqlQuery query("SELECT * FROM machines ORDER BY name", m_db);
    while (query.next()) {
        Core::MachineInfo m;
        m.id = query.value("id").toString();
        m.name = query.value("name").toString();
        m.isMaster = query.value("is_master").toBool();
        m.lastSeen = query.value("last_seen").toDateTime();
        m.ipAddress = query.value("ip_address").toString();
        m.osVersion = query.value("os_version").toString();
        m.appVersion = query.value("app_version").toString();
        m.isOnline = query.value("is_online").toBool();
        list.append(m);
    }
    return list;
}

std::optional<Core::MachineInfo> SQLiteStorageProvider::getMachine(const QString& id) {
    QSqlQuery query(m_db);
    query.prepare("SELECT * FROM machines WHERE id=?");
    query.addBindValue(id);
    if (query.exec() && query.next()) {
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
    return std::nullopt;
}

// ---- System Settings ----

std::optional<Core::SystemSettings> SQLiteStorageProvider::getSystemSettings() {
    Core::SystemSettings settings;
    QSqlQuery query("SELECT key, value FROM system_settings", m_db);
    while (query.next()) {
        QString key = query.value(0).toString();
        QString value = query.value(1).toString();
        if (key == "master_machine_id") settings.masterMachineId = value;
        else if (key == "voting_status") settings.votingStatus = static_cast<Core::VotingState>(value.toInt());
        else if (key == "allow_results_preview") settings.allowResultsPreview = (value == "1");
        else if (key == "auto_backup_enabled") settings.autoBackupEnabled = (value == "1");
        else if (key == "backup_interval_hours") settings.backupIntervalHours = value.toInt();
        else if (key == "session_timeout_minutes") settings.sessionTimeoutMinutes = value.toInt();
        else if (key == "audit_all_actions") settings.auditAllActions = (value == "1");
        else if (key == "encryption_enabled") settings.encryptionEnabled = (value == "1");
        else if (key == "tamper_detection") settings.tamperDetection = (value == "1");
        else if (key == "theme") settings.theme = value;
        else if (key == "accent_color") settings.accentColor = value;
        else if (key == "language") settings.language = value;
    }
    return settings;
}

bool SQLiteStorageProvider::updateSystemSettings(const Core::SystemSettings& settings) {
    auto set = [&](const QString& key, const QString& value) {
        QSqlQuery q(m_db);
        q.prepare("INSERT OR REPLACE INTO system_settings (key, value) VALUES (?, ?)");
        q.addBindValue(key);
        q.addBindValue(value);
        return q.exec();
    };
    bool ok = true;
    ok &= set("master_machine_id", settings.masterMachineId);
    ok &= set("voting_status", QString::number(static_cast<int>(settings.votingStatus)));
    ok &= set("allow_results_preview", settings.allowResultsPreview ? "1" : "0");
    ok &= set("auto_backup_enabled", settings.autoBackupEnabled ? "1" : "0");
    ok &= set("backup_interval_hours", QString::number(settings.backupIntervalHours));
    ok &= set("session_timeout_minutes", QString::number(settings.sessionTimeoutMinutes));
    ok &= set("audit_all_actions", settings.auditAllActions ? "1" : "0");
    ok &= set("encryption_enabled", settings.encryptionEnabled ? "1" : "0");
    ok &= set("tamper_detection", settings.tamperDetection ? "1" : "0");
    ok &= set("theme", settings.theme);
    ok &= set("accent_color", settings.accentColor);
    ok &= set("language", settings.language);
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
    return query.exec();
}

QList<Core::BackupEntry> SQLiteStorageProvider::getBackupHistory() {
    QList<Core::BackupEntry> list;
    QSqlQuery query("SELECT * FROM backups ORDER BY created_at DESC", m_db);
    while (query.next()) {
        Core::BackupEntry b;
        b.id = query.value("id").toString();
        b.name = query.value("name").toString();
        b.createdAt = query.value("created_at").toDateTime();
        b.sizeBytes = query.value("size_bytes").toLongLong();
        b.type = query.value("type").toString();
        b.checksum = query.value("checksum").toByteArray();
        b.storagePath = query.value("storage_path").toString();
        b.isEncrypted = query.value("is_encrypted").toBool();
        list.append(b);
    }
    return list;
}

bool SQLiteStorageProvider::deleteBackupRecord(const QString& id) {
    QSqlQuery query(m_db);
    query.prepare("DELETE FROM backups WHERE id=?");
    query.addBindValue(id);
    return query.exec();
}

// ---- Bulk Operations ----

bool SQLiteStorageProvider::bulkAddStudents(const QList<Core::Student>& students) {
    m_db.transaction();
    for (const auto& s : students) {
        if (!addStudent(s)) { m_db.rollback(); return false; }
    }
    m_db.commit();
    return true;
}

bool SQLiteStorageProvider::bulkAddCandidates(const QList<Core::Candidate>& candidates) {
    m_db.transaction();
    for (const auto& c : candidates) {
        if (!addCandidate(c)) { m_db.rollback(); return false; }
    }
    m_db.commit();
    return true;
}

bool SQLiteStorageProvider::clearElectionData(const QString& electionId) {
    QSqlQuery query(m_db);
    query.prepare("DELETE FROM votes WHERE election_id=?");
    query.addBindValue(electionId);
    query.exec();
    query.prepare("DELETE FROM candidates WHERE election_id=?");
    query.addBindValue(electionId);
    query.exec();
    return true;
}

// ---- Helpers ----

bool SQLiteStorageProvider::execQuery(const QString& sql) {
    QSqlQuery query(m_db);
    return query.exec(sql);
}

static Core::Student parseStudent(const QSqlQuery& query) {
    Core::Student s;
    s.id = query.value("id").toString();
    s.name = query.value("name").toString();
    s.admissionNumber = query.value("admission_number").toString();
    s.rollNumber = query.value("roll_number").toString();
    s.className = query.value("class_name").toString();
    s.section = query.value("section").toString();
    s.age = query.value("age").toInt();
    s.gender = query.value("gender").toString();
    s.email = query.value("email").toString();
    s.phone = query.value("phone").toString();
    s.parentName = query.value("parent_name").toString();
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
    u.department = query.value("department").toString();
    u.className = query.value("class_name").toString();
    u.section = query.value("section").toString();
    u.phone = query.value("phone").toString();
    u.email = query.value("email").toString();
    u.role = static_cast<Core::UserRole>(query.value("role").toInt());
    u.permissions = query.value("permissions").toString().split(",", Qt::SkipEmptyParts);
    u.digitalSignature = query.value("digital_signature").toByteArray();
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

} // namespace Ballot::Storage
