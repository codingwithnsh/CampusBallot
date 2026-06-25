#include "UserManagementView.h"
#include "src/core/SystemManager.h"
#include "src/modules/auth/RBACManager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QScrollArea>
#include <QGroupBox>

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
    m_usersTable->setColumnCount(7);
    m_usersTable->setHorizontalHeaderLabels({"Name", "Email", "Department", "Role", "Status", "Last Login", "Actions"});
    m_usersTable->horizontalHeader()->setStretchLastSection(true);
    m_usersTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_usersTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_usersTable->setAlternatingRowColors(true);
    m_usersTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    mainLayout->addWidget(m_usersTable);

    mainLayout->addStretch();

    scrollArea->setWidget(content);

    auto *outerLayout = new QVBoxLayout(this);
    outerLayout->setContentsMargins(0, 0, 0, 0);
    outerLayout->addWidget(scrollArea);

    connect(m_searchEdit, &QLineEdit::textChanged, this, &UserManagementView::refreshData);
    connect(m_roleFilter, &QComboBox::currentIndexChanged, this, &UserManagementView::refreshData);

    refreshData();
}

void UserManagementView::refreshData() {
    auto* storage = Core::SystemManager::instance().storage();
    if (!storage) return;

    auto users = storage->getUsers();
    QString search = m_searchEdit->text().toLower();
    int roleFilter = m_roleFilter->currentIndex();

    QList<Core::User> filtered;
    for (const auto& u : users) {
        if (!search.isEmpty() && !u.name.toLower().contains(search) && !u.email.toLower().contains(search))
            continue;
        if (roleFilter > 0 && static_cast<int>(u.role) != roleFilter - 1)
            continue;
        filtered.append(u);
    }

    m_usersTable->setRowCount(filtered.size());
    auto& rbac = Auth::RBACManager::instance();
    for (int i = 0; i < filtered.size(); ++i) {
        const auto& u = filtered[i];
        m_usersTable->setItem(i, 0, new QTableWidgetItem(u.name));
        m_usersTable->setItem(i, 1, new QTableWidgetItem(u.email));
        m_usersTable->setItem(i, 2, new QTableWidgetItem(u.department));
        m_usersTable->setItem(i, 3, new QTableWidgetItem(rbac.roleToString(u.role)));
        auto *statusItem = new QTableWidgetItem(u.isActive ? "Active" : "Inactive");
        statusItem->setForeground(u.isActive ? QColor("#4caf50") : QColor("#f44336"));
        m_usersTable->setItem(i, 4, statusItem);
        m_usersTable->setItem(i, 5, new QTableWidgetItem(u.lastLogin.isValid() ? u.lastLogin.toString("yyyy-MM-dd HH:mm") : "Never"));
        m_usersTable->setItem(i, 6, new QTableWidgetItem("..."));
    }
}

} // namespace Ballot::UI
