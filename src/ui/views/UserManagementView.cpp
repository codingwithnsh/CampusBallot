#include "UserManagementView.h"
#include "src/core/SystemManager.h"
#include "src/modules/auth/RBACManager.h"
#include "src/security/HashProvider.h" // Include HashProvider for password hashing
#include "src/ui/components/ToastNotification.h" // Include ToastNotification
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QScrollArea>
#include <QGroupBox>
#include <QInputDialog> // For user input dialogs
#include <QMessageBox>  // For confirmation dialogs
#include <QUuid>        // For generating user IDs

namespace Ballot::UI {

UserManagementView::UserManagementView(QWidget *parent) : QWidget(parent) {
    setupUi();
}

void UserManagementView::setupUi() {
    auto *scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setStyleSheet("QScrollArea { background: transparent; border: none; }");

    auto *content = new QWidget();
    content->setStyleSheet("background: transparent;");
    auto *mainLayout = new QVBoxLayout(content);
    mainLayout->setContentsMargins(32, 24, 32, 24);
    mainLayout->setSpacing(20);

    auto *title = new QLabel("User Management");
    title->setObjectName("title");
    mainLayout->addWidget(title);

    // Toolbar
    auto *toolbar = new QHBoxLayout();

    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setPlaceholderText("Search users...");
    m_searchEdit->setObjectName("searchField");
    m_searchEdit->setFixedWidth(300);
    toolbar->addWidget(m_searchEdit);

    m_roleFilter = new QComboBox(this);
    m_roleFilter->addItems({"All Roles", "Super Administrator", "Election Administrator", "Teacher", "Student Volunteer", "Observer", "Result Auditor"});
    m_roleFilter->setFixedWidth(200);
    toolbar->addWidget(m_roleFilter);

    toolbar->addStretch();

    m_addUserBtn = new QPushButton("+ Add User");
    m_addUserBtn->setObjectName("successButton");
    m_editUserBtn = new QPushButton("Edit");
    m_editUserBtn->setObjectName("primaryButton");
    m_deleteUserBtn = new QPushButton("Delete");
    m_deleteUserBtn->setObjectName("dangerButton");

    toolbar->addWidget(m_addUserBtn);
    toolbar->addWidget(m_editUserBtn);
    toolbar->addWidget(m_deleteUserBtn);
    mainLayout->addLayout(toolbar);

    // Users table
    m_usersTable = new QTableWidget(this);
    m_usersTable->setColumnCount(6);
    m_usersTable->setHorizontalHeaderLabels({"Name", "Email", "Department", "Role", "Status", "Last Login"});
    m_usersTable->horizontalHeader()->setStretchLastSection(true);
    m_usersTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_usersTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_usersTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    mainLayout->addWidget(m_usersTable);

    mainLayout->addStretch();

    scrollArea->setWidget(content);

    auto *outerLayout = new QVBoxLayout(this);
    outerLayout->setContentsMargins(0, 0, 0, 0);
    outerLayout->addWidget(scrollArea);

    connect(m_searchEdit, &QLineEdit::textChanged, this, &UserManagementView::refreshData);
    connect(m_roleFilter, &QComboBox::currentIndexChanged, this, &UserManagementView::refreshData);

    // Connect buttons
    connect(m_addUserBtn, &QPushButton::clicked, this, [this]() {
        auto* storage = Core::SystemManager::instance().storage();
        if (!storage) {
            ToastNotification::show(this, "Storage not available.", ToastNotification::Error);
            return;
        }

        bool ok;
        QString name = QInputDialog::getText(this, tr("Add New User"), tr("Name:"), QLineEdit::Normal, "", &ok);
        if (!ok || name.isEmpty()) return;

        QString email = QInputDialog::getText(this, tr("Add New User"), tr("Email:"), QLineEdit::Normal, "", &ok);
        if (!ok || email.isEmpty()) return;

        QString password = QInputDialog::getText(this, tr("Add New User"), tr("Password:"), QLineEdit::Password, "", &ok);
        if (!ok || password.isEmpty()) return;

        if (password.length() < 8) {
            ToastNotification::show(this, "Password must be at least 8 characters long.", ToastNotification::Warning);
            return;
        }

        // Select role
        QStringList roles;
        for (int i = 0; i < static_cast<int>(Core::UserRole::Count); ++i) {
            roles << Auth::RBACManager::instance().roleToString(static_cast<Core::UserRole>(i));
        }
        QString roleString = QInputDialog::getItem(this, tr("Add New User"), tr("Role:"), roles, 0, false, &ok);
        if (!ok || roleString.isEmpty()) return;
        Core::UserRole role = Auth::RBACManager::instance().roleFromString(roleString);

        Core::User newUser;
        newUser.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
        newUser.name = name;
        newUser.email = email;
        newUser.role = role;
        newUser.isActive = true;
        newUser.createdAt = QDateTime::currentDateTime();

        QByteArray salt = Security::HashProvider::generateSalt();
        QByteArray hashedPassword = Security::HashProvider::argon2Hash(password, salt);
        newUser.digitalSignature = salt + hashedPassword; // Store combined salt and hash

        if (storage->createUser(newUser)) {
            ToastNotification::show(this, "User '" + newUser.name + "' added successfully.", ToastNotification::Success);
            refreshData();
        } else {
            ToastNotification::show(this, "Failed to add user. Email might already exist.", ToastNotification::Error);
        }
    });

    connect(m_editUserBtn, &QPushButton::clicked, this, [this]() {
        auto selectedItems = m_usersTable->selectedItems();
        if (selectedItems.isEmpty()) {
            ToastNotification::show(this, "Please select a user to edit.", ToastNotification::Warning);
            return;
        }
        int row = selectedItems.first()->row();
        QString userId = m_usersTable->item(row, 0)->data(Qt::UserRole).toString();

        auto* storage = Core::SystemManager::instance().storage();
        if (!storage) {
            ToastNotification::show(this, "Storage not available.", ToastNotification::Error);
            return;
        }

        std::optional<Core::User> userOpt = storage->getUser(userId);
        if (!userOpt) {
            ToastNotification::show(this, "User not found.", ToastNotification::Error);
            return;
        }
        Core::User user = *userOpt;

        bool ok;
        QString name = QInputDialog::getText(this, tr("Edit User"), tr("Name:"), QLineEdit::Normal, user.name, &ok);
        if (!ok) return;
        user.name = name;

        QString email = QInputDialog::getText(this, tr("Edit User"), tr("Email:"), QLineEdit::Normal, user.email, &ok);
        if (!ok) return;
        user.email = email;

        // Select role
        QStringList roles;
        for (int i = 0; i < static_cast<int>(Core::UserRole::Count); ++i) {
            roles << Auth::RBACManager::instance().roleToString(static_cast<Core::UserRole>(i));
        }
        QString currentRoleString = Auth::RBACManager::instance().roleToString(user.role);
        int currentRoleIndex = roles.indexOf(currentRoleString);

        QString roleString = QInputDialog::getItem(this, tr("Edit User"), tr("Role:"), roles, currentRoleIndex, false, &ok);
        if (!ok) return;
        user.role = Auth::RBACManager::instance().roleFromString(roleString);

        // Option to change password
        int changePass = QMessageBox::question(this, tr("Edit User"), tr("Do you want to change the password?"), QMessageBox::Yes | QMessageBox::No);
        if (changePass == QMessageBox::Yes) {
            QString newPassword = QInputDialog::getText(this, tr("Change Password"), tr("New Password:"), QLineEdit::Password, "", &ok);
            if (!ok || newPassword.isEmpty()) return;
            if (newPassword.length() < 8) {
                ToastNotification::show(this, "Password must be at least 8 characters long.", ToastNotification::Warning);
                return;
            }
            QByteArray salt = Security::HashProvider::generateSalt();
            QByteArray hashedPassword = Security::HashProvider::argon2Hash(newPassword, salt);
            user.digitalSignature = salt + hashedPassword;
        }

        if (storage->updateUser(user)) {
            ToastNotification::show(this, "User '" + user.name + "' updated successfully.", ToastNotification::Success);
            refreshData();
        } else {
            ToastNotification::show(this, "Failed to update user.", ToastNotification::Error);
        }
    });

    connect(m_deleteUserBtn, &QPushButton::clicked, this, [this]() {
        auto selectedItems = m_usersTable->selectedItems();
        if (selectedItems.isEmpty()) {
            ToastNotification::show(this, "Please select a user to delete.", ToastNotification::Warning);
            return;
        }
        int row = selectedItems.first()->row();
        QString userId = m_usersTable->item(row, 0)->data(Qt::UserRole).toString();
        QString userName = m_usersTable->item(row, 0)->text();

        QMessageBox::StandardButton reply;
        reply = QMessageBox::question(this, "Confirm Deletion",
                                      "Are you sure you want to delete user '" + userName + "'?\nThis action cannot be undone.",
                                      QMessageBox::Yes | QMessageBox::No);
        if (reply == QMessageBox::Yes) {
            auto* storage = Core::SystemManager::instance().storage();
            if (!storage) {
                ToastNotification::show(this, "Storage not available.", ToastNotification::Error);
                return;
            }
            if (storage->deleteUser(userId)) {
                ToastNotification::show(this, "User '" + userName + "' deleted successfully.", ToastNotification::Success);
                refreshData();
            } else {
                ToastNotification::show(this, "Failed to delete user '" + userName + "'.", ToastNotification::Error);
            }
        }
    });

    refreshData();
}

void UserManagementView::refreshData() {
    auto* storage = Core::SystemManager::instance().storage();
    if (!storage) return;

    auto users = storage->getUsers();
    QString search = m_searchEdit->text().toLower();
    int roleFilterIndex = m_roleFilter->currentIndex(); // 0 for "All Roles", 1 for "Super Admin", etc.

    QList<Core::User> filtered;
    for (const auto& u : users) {
        if (!search.isEmpty() && !u.name.toLower().contains(search) && !u.email.toLower().contains(search))
            continue;
        // Adjust role filtering: roleFilterIndex 0 means all roles, otherwise map to Core::UserRole enum
        if (roleFilterIndex > 0 && static_cast<int>(u.role) != (roleFilterIndex - 1))
            continue;
        filtered.append(u);
    }

    m_usersTable->setRowCount(filtered.size());
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
}

} // namespace Ballot::UI
