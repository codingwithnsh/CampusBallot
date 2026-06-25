#pragma once

#include <QString>

namespace Ballot::Core::Constants {

constexpr int SPLASH_DURATION_MS = 3000;
constexpr int SPLASH_STEPS = 5;
constexpr int VOTE_ID_LENGTH = 32;
constexpr int TOKEN_LENGTH = 64;
constexpr int QR_CODE_SIZE = 200;
constexpr int PHOTO_MAX_SIZE_MB = 5;
constexpr int PHOTO_CROP_SIZE = 300;
constexpr int AUDIT_LOG_RETENTION_DAYS = 365;
constexpr int BACKUP_RETENTION_COUNT = 30;
constexpr int SESSION_CHECK_INTERVAL_MS = 60000;
constexpr int DASHBOARD_REFRESH_MS = 5000;
constexpr int VOTING_STATE_POLL_MS = 2000;

constexpr const char* APP_NAME = "TheRealCampusBallot";
constexpr const char* ORG_NAME = "CampusBallot";
constexpr const char* APP_VERSION = "1.0.0";
constexpr const char* DB_FILENAME = "ballot.db";
constexpr const char* SETTINGS_FILE = "settings.ini";
constexpr const char* LOG_FILE = "ballot.log";

constexpr const char* DEFAULT_ACCENT = "#0078d4";
constexpr const char* DEFAULT_THEME = "dark";

} // namespace Ballot::Core::Constants
