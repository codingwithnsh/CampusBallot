#include "UserManagementView.h"
#include "src/core/SystemManager.h"
#include "src/modules/auth/AuthManager.h"
#include "src/modules/auth/RBACManager.h"
#include "src/modules/audit/AuditManager.h" // For audit logging
#include "src/modules/security/HashProvider.h" // Include HashProvider for password hashing
#include "src/ui/components/ToastNotification.h" // Include ToastNotification
#include "src/core/Utils.h" // For IdGenerator
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QScrollArea>
#include <QGroupBox>
#include <QInputDialog> // For user input dialogs
#include <QMessageBox>  // For confirmation dialogs
#include <QUuid>        // Still included for QJsonDocument::Indented or if needed elsewhere
#include <QDateTime>
#include <QDebug> // For logging

namespace Ballot::UI {

UserManagementView::UserManagementView(QWidget *parent) : QWidget(parent) {
    setupUi();
    // Connect to signals that might require data refresh
    connect(&Auth::AuthManager::instance(), &Auth::AuthManager::loginSuccessful, this, &UserManagementView::refreshData);
    connect(&Auth::AuthManager::instance(), &Auth::AuthManager::logoutOccurred, this, &UserManagementView::refreshData);
    qDebug() << "UserManagementView: Initialized.";
}

/**
 * @brief Sets up the user interface for the user management view.
 */
void UserManagementView::setupUi() {
    auto *scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setStyleSheet("QScrollArea { background: transparent; border: none; }");

    auto *content = new QWidget();
    content->setStyleSheet("background: transparent;");
    auto *mainLayout = new QVBoxLayout(content);
    mainLayout->setContentsMargins(32, 24, 32, 24);
    mainLayout->setSpacing(20);

    // --- Title ---
    auto *title = new QLabel("User Management", this);
    title->setObjectName("title");
    title->setStyleSheet("font-size: 32px; font-weight: 700; color: #e0e0e0;");
    mainLayout->addWidget(title);

    // --- Toolbar ---
    auto *toolbar = new QHBoxLayout();

    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setPlaceholderText("Search users by name or email...");
    m_searchEdit->setObjectName("searchField");
    m_searchEdit->setFixedWidth(300);
    m_searchEdit->setStyleSheet(R"(
        QLineEdit#searchField {
            background-color: #25253a; border: 1px solid #3d3d5c; border-radius: 8px;
            padding: 8px 12px; color: #ffffff; font-size: 14px;
        }
        QLineEdit#searchField:focus { border-color: #0078d4; }
    )");
    toolbar->addWidget(m_searchEdit);

    m_roleFilter = new QComboBox(this);
    m_roleFilter->addItems({"All Roles", "Super Administrator", "Election Administrator", "Teacher", "Student Volunteer", "Observer", "Result Auditor"});
    m_roleFilter->setFixedWidth(200);
    m_roleFilter->setStyleSheet(R"(
        QComboBox {
            background-color: #25253a; color: #ffffff; border: 1px solid #3d3d5c;
            border-radius: 8px; padding: 8px 12px; font-size: 14px;
        }
        QComboBox::drop-down { border: 0px; }
        QComboBox::down-arrow { width: 16px; height: 16px; }
        QComboBox:hover { border: 1px solid #6b7280; }
        QComboBox QAbstractItemView {
            background-color: #25253a; color: #ffffff; selection-background-color: #0078d4;
            border: 1px solid #3d3d5c; border-radius: 8px;
        }
    )");
    toolbar->addWidget(m_roleFilter);

    toolbar->addStretch();

    m_addUserBtn = new QPushButton("+ Add User", this);
    m_addUserBtn->setObjectName("successButton");
    m_addUserBtn->setStyleSheet(R"(
        QPushButton#successButton { background-color: #4CAF50; color: white; border: none; border-radius: 8px; font-size: 15px; font-weight: 600; padding: 8px 16px; }
        QPushButton#successButton:hover { background-color: #66BB6A; }
        QPushButton#successButton:pressed { background-color: #388E3C; }
    )");

    m_editUserBtn = new QPushButton("Edit", this);
    m_editUserBtn->setObjectName("primaryButton");
    m_editUserBtn->setStyleSheet(R"(
        QPushButton#primaryButton { background-color: #0078d4; color: white; border: none; border-radius: 8px; font-size: 15px; font-weight: 600; padding: 8px 16px; }
        QPushButton#primaryButton:hover { background-color: #1a8ae8; }
        QPushButton#primaryButton:pressed { background-color: #006cbd; }
    )");

    m_deleteUserBtn = new QPushButton("Delete", this);
    m_deleteUserBtn->setObjectName("dangerButton");
    m_deleteUserBtn->setStyleSheet(R"(
        QPushButton#dangerButton { background-color: #d32f2f; color: white; border: none; border-radius: 8px; font-size: 15px; font-weight: 600; padding: 8px 16px; }
        QPushButton#dangerButton:hover { background-color: #ef5350; }
        QPushButton#dangerButton:pressed { background-color: #b71c1c; }
    )");

    toolbar->addWidget(m_addUserBtn);
    toolbar->addWidget(m_editUserBtn);
    toolbar->addWidget(m_deleteUserBtn);
    mainLayout->addLayout(toolbar);

    // --- Users table ---
    m_usersTable = new QTableWidget(this);
    m_usersTable->setColumnCount(6);
    m_usersTable->setHorizontalHeaderLabels({"Name", "Email", "Department", "Role", "Status", "Last Login"});
    m_usersTable->horizontalHeader()->setStretchLastSection(true);
    m_usersTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_usersTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_usersTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_usersTable->setStyleSheet(R"(
        QTableWidget {
            background-color: #1e1e34; alternate-background-color: #25253a;
            border: 1px solid #3d3d5c; border-radius: 8px; color: #e0e0e0;
            font-size: 14px;
        }
        QHeaderView::section {
            background-color: #2d2d44; color: #ffffff; padding: 8px;
            border: 1px solid #3d3d5c; font-weight: 600;
        }
        QTableWidget::item { padding: 8px; }
    )");
    mainLayout->addWidget(m_usersTable);

    mainLayout->addStretch(); // Pushes content to the top

    scrollArea->setWidget(content);

    auto *outerLayout = new QVBoxLayout(this);
    outerLayout->setContentsMargins(0, 0, 0, 0);
    outerLayout->addWidget(scrollArea);

    // --- Connect Signals ---
    connect(m_searchEdit, &QLineEdit::textChanged, this, &UserManagementView::refreshData);
    connect(m_roleFilter, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &UserManagementView::refreshData);

    connect(m_addUserBtn, &QPushButton::clicked, this, [this]() {
        qInfo() << "UserManagementView: Add User button clicked.";
        auto* storage = Core::SystemManager::instance().storage();
        if (!storage) {
            ToastNotification::show(this, "Storage not available. Cannot add user.", ToastNotification::Error);
            qCritical() << "UserManagementView: Storage not available for adding user.";
            return;
        }

        bool ok;
        QString name = QInputDialog::getText(this, tr("Add New User"), tr("Name:"), QLineEdit::Normal, "", &ok);
        if (!ok || name.isEmpty()) {
            qDebug() << "UserManagementView: Add User cancelled or empty name provided.";
            return;
        }

        QString email = QInputDialog::getText(this, tr("Add New User"), tr("Email:"), QLineEdit::Normal, "", &ok);
        if (!ok || email.isEmpty()) {
            qDebug() << "UserManagementView: Add User cancelled or empty email provided.";
            return;
        }
        // Basic email format validation
        if (!email.contains('@') || !email.contains('.')) {
            ToastNotification::show(this, "Invalid email format.", ToastNotification::Warning);
            qWarning() << "UserManagementView: Invalid email format provided:" << email;
            return;
        }

        QString password = QInputDialog::getText(this, tr("Add New User"), tr("Password:"), QLineEdit::Password, "", &ok);
        if (!ok || password.isEmpty()) {
            qDebug() << "UserManagementView: Add User cancelled or empty password provided.";
            return;
        }
        if (password.length() < 8) {
            ToastNotification::show(this, "Password must be at least 8 characters long.", ToastNotification::Warning);
            qWarning() << "UserManagementView: Password too short.";
            return;
        }

        // Select role
        QStringList roles;
        for (int i = 0; i < static_cast<int>(Core::UserRole::Count); ++i) {
            roles << Auth::RBACManager::instance().roleToString(static_cast<Core::UserRole>(i));
        }
        QString roleString = QInputDialog::getItem(this, tr("Add New User"), tr("Role:"), roles, 0, false, &ok);
        if (!ok || roleString.isEmpty()) {
            qDebug() << "UserManagementView: Add User cancelled or no role selected.";
            return;
        }
        Core::UserRole role = Auth::RBACManager::instance().roleFromString(roleString);

        Core::User newUser;
        newUser.id = Core::IdGenerator::generateId();
        newUser.name = name;
        newUser.email = email;
        newUser.role = role;
        newUser.isActive = true;
        newUser.createdAt = QDateTime::currentDateTime();

        QByteArray salt = Security::HashProvider::generateSalt();
        QByteArray hashedPassword = Security::HashProvider::argon2Hash(password, salt);
        newUser.passwordHashAndSalt = salt + hashedPassword; // Store combined salt and hash

        if (storage->createUser(newUser)) {
            ToastNotification::show(this, "User '" + newUser.name + "' added successfully.", ToastNotification::Success);
            refreshData();
            Audit::AuditManager::instance().log(Core::AuditAction::UserCreated, QString("User '%1' added.").arg(newUser.name), Auth::AuthManager::instance().currentUserId());
        } else {
            ToastNotification::show(this, "Failed to add user. Email might already exist.", ToastNotification::Error);
            qCritical() << "UserManagementView: Failed to add user" << newUser.email << ". Storage error or email exists.";
            Audit::AuditManager::instance().log(Core::AuditAction::UserCreated, QString("Failed to add user '%1'.").arg(newUser.name), Auth::AuthManager::instance().currentUserId());
        }
    });

    connect(m_editUserBtn, &QPushButton::clicked, this, [this]() {
        qInfo() << "UserManagementView: Edit User button clicked.";
        auto selectedItems = m_usersTable->selectedItems();
        if (selectedItems.isEmpty()) {
            ToastNotification::show(this, "Please select a user to edit.", ToastNotification::Warning);
            return;
        }
        int row = selectedItems.first()->row();
        QString userId = m_usersTable->item(row, 0)->data(Qt::UserRole).toString();

        auto* storage = Core::SystemManager::instance().storage();
        if (!storage) {
            ToastNotification::show(this, "Storage not available. Cannot edit user.", ToastNotification::Error);
            qCritical() << "UserManagementView: Storage not available for editing user.";
            return;
        }

        std::optional<Core::User> userOpt = storage->getUser(userId);
        if (!userOpt) {
            ToastNotification::show(this, "User not found.", ToastNotification::Error);
            qWarning() << "UserManagementView: User with ID" << userId << "not found for editing.";
            return;
        }
        Core::User user = *userOpt;

        bool ok;
        QString name = QInputDialog::getText(this, tr("Edit User"), tr("Name:"), QLineEdit::Normal, user.name, &ok);
        if (!ok) {
            qDebug() << "UserManagementView: Edit User cancelled (name).";
            return;
        }
        user.name = name;

        QString email = QInputDialog::getText(this, tr("Edit User"), tr("Email:"), QLineEdit::Normal, user.email, &ok);
        if (!ok) {
            qDebug() << "UserManagementView: Edit User cancelled (email).";
            return;
        }
        // Basic email format validation
        if (!email.contains('@') || !email.contains('.')) {
            ToastNotification::show(this, "Invalid email format.", ToastNotification::Warning);
            qWarning() << "UserManagementView: Invalid email format provided during edit:" << email;
            return;
        }
        user.email = email;

        // Select role
        QStringList roles;
        for (int i = 0; i < static_cast<int>(Core::UserRole::Count); ++i) {
            roles << Auth::RBACManager::instance().roleToString(static_cast<Core::UserRole>(i));
        }
        QString currentRoleString = Auth::RBACManager::instance().roleToString(user.role);
        int currentRoleIndex = roles.indexOf(currentRoleString);

        QString roleString = QInputDialog::getItem(this, tr("Edit User"), tr("Role:"), roles, currentRoleIndex, false, &ok);
        if (!ok || roleString.isEmpty()) {
            qDebug() << "UserManagementView: Edit User cancelled (role).";
            return;
        }
        user.role = Auth::RBACManager::instance().roleFromString(roleString);

        // Option to change password
        int changePass = QMessageBox::question(this, tr("Edit User"), tr("Do you want to change the password?"), QMessageBox::Yes | QMessageBox::No);
        if (changePass == QMessageBox::Yes) {
            QString newPassword = QInputDialog::getText(this, tr("Change Password"), tr("New Password:"), QLineEdit::Password, "", &ok);
            if (!ok || newPassword.isEmpty()) {
                qDebug() << "UserManagementView: Change password cancelled or empty password provided.";
                return;
            }
            if (newPassword.length() < 8) {
                ToastNotification::show(this, "Password must be at least 8 characters long.", ToastNotification::Warning);
                qWarning() << "UserManagementView: New password too short.";
                return;
            }
            QByteArray salt = Security::HashProvider::generateSalt();
            QByteArray hashedPassword = Security::HashProvider::argon2Hash(newPassword, salt);
            user.passwordHashAndSalt = salt + hashedPassword; // Store combined salt and hash
            qInfo() << "UserManagementView: Password change requested for user" << user.id;
        }

        if (storage->updateUser(user)) {
            ToastNotification::show(this, "User '" + user.name + "' updated successfully.", ToastNotification::Success);
            refreshData();
            Audit::AuditManager::instance().log(Core::AuditAction::UserModified, QString("User '%1' updated.").arg(user.name), Auth::AuthManager::instance().currentUserId());
        } else {
            ToastNotification::show(this, "Failed to update user.", ToastNotification::Error);
            qCritical() << "UserManagementView: Failed to update user" << user.email << ". Storage error.";
            Audit::AuditManager::instance().log(Core::AuditAction::UserModified, QString("Failed to update user '%1'.").arg(user.name), Auth::AuthManager::instance().currentUserId());
        }
    });

    connect(m_deleteUserBtn, &QPushButton::clicked, this, [this]() {
        qInfo() << "UserManagementView: Delete User button clicked.";
        auto selectedItems = m_usersTable->selectedItems();
        if (selectedItems.isEmpty()) {
            ToastNotification::show(this, "Please select a user to delete.", ToastNotification::Warning);
            return;
        }
        int row = selectedItems.first()->row();
        QString userId = m_usersTable->item(row, 0)->data(Qt::UserRole).toString();
        QString userName = m_usersTable->item(row, 0)->text();

        if (userId == Auth::AuthManager::instance().currentUserId()) {
            ToastNotification::show(this, "Cannot delete currently logged-in user.", ToastNotification::Error);
            qWarning() << "UserManagementView: Attempted to delete currently logged-in user:" << userId;
            return;
        }

        QMessageBox::StandardButton reply;
        reply = QMessageBox::question(this, "Confirm Deletion",
                                      "Are you sure you want to delete user '" + userName + "'?\nThis action cannot be undone.",
                                      QMessageBox::Yes | QMessageBox::No);
        if (reply == QMessageBox::Yes) {
            auto* storage = Core::SystemManager::instance().storage();
            if (!storage) {
                ToastNotification::show(this, "Storage not available. Cannot delete user.", ToastNotification::Error);
                qCritical() << "UserManagementView: Storage not available for deleting user.";
                return;
            }
            if (storage->deleteUser(userId)) {
                ToastNotification::show(this, "User '" + userName + "' deleted successfully.", ToastNotification::Success);
                refreshData();
                Audit::AuditManager::instance().log(Core::AuditAction::UserDeleted, QString("User '%1' deleted.").arg(userName), Auth::AuthManager::instance().currentUserId());
            } else {
                ToastNotification::show(this, "Failed to delete user '" + userName + "'.", ToastNotification::Error);
                qCritical() << "UserManagementView: Failed to delete user" << userName << ". Storage error.";
                Audit::AuditManager::instance().log(Core::AuditAction::UserDeleted, QString("Failed to delete user '%1'.").arg(userName), Auth::AuthManager::instance().currentUserId());
            }
        } else {
            qDebug() << "UserManagementView: User deletion cancelled.";
        }
    });

    refreshData(); // Initial data load
    qDebug() << "UserManagementView: UI setup complete.";
}

/**
 * @brief Refreshes the data displayed in the user table, applying search and role filters.
 */
void UserManagementView::refreshData() {
    qDebug() << "UserManagementView: Refreshing data...";
    auto* storage = Core::SystemManager::instance().storage();
    if (!storage) {
        qCritical() << "UserManagementView: Storage not available. Cannot refresh user data.";
        return;
    }

    auto users = storage->getUsers();
    QString search = m_searchEdit->text().toLower();
    int roleFilterIndex = m_roleFilter->currentIndex(); // 0 for "All Roles", 1 for "Super Admin", etc.

    QList<Core::User> filtered;
    for (const auto& u : users) {
        // Apply search filter
        if (!search.isEmpty() && !u.name.toLower().contains(search) && !u.email.toLower().contains(search)) {
            continue;
        }
        // Apply role filter
        // roleFilterIndex 0 means "All Roles", so no filtering needed
        if (roleFilterIndex > 0) {
            // Core::UserRole enum values start from 0, so roleFilterIndex - 1 matches the enum value
            if (static_cast<int>(u.role) != (roleFilterIndex - 1)) {
                continue;
            }
        }
        filtered.append(u);
    }

    m_usersTable->setRowCount(filtered.size());
    m_usersTable->setSortingEnabled(false); // Disable sorting during update

    auto& rbac = Auth::RBACManager::instance();
    for (int i = 0; i < filtered.size(); ++i) {
        const auto& u = filtered[i];
        auto* nameItem = new QTableWidgetItem(u.name);
        nameItem->setData(Qt::UserRole, u.id); // Store user ID in UserRole
        m_usersTable->setItem(i, 0, nameItem);
        m_usersTable->setItem(i, 1, new QTableWidgetItem(u.email));
        m_usersTable->setItem(i, 2, new QTableWidgetItem(u.department)); // Department is empty in Core::User, consider removing or adding to dialog
        m_usersTable->setItem(i, 3, new QTableWidgetItem(rbac.roleToString(u.role)));
        auto *statusItem = new QTableWidgetItem(u.isActive ? "Active" : "Inactive");
        statusItem->setForeground(u.isActive ? QColor("#4caf50") : QColor("#f44336"));
        m_usersTable->setItem(i, 4, statusItem);
        m_usersTable->setItem(i, 5, new QTableWidgetItem(u.lastLogin.isValid() ? u.lastLogin.toString("yyyy-MM-dd HH:mm") : "Never"));
    }
    m_usersTable->setSortingEnabled(true); // Re-enable sorting
    qDebug() << "UserManagementView: User data refreshed. Displaying" << filtered.size() << "users.";
}

} // namespace Ballot::UI