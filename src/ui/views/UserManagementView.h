#pragma once

#include <QWidget>
#include <QTableWidget>
#include <QPushButton>
#include <QLineEdit>
#include <QComboBox>

namespace Ballot::UI {

class UserManagementView : public QWidget {
    Q_OBJECT
public:
    explicit UserManagementView(QWidget *parent = nullptr);

private:
    void setupUi();
    void refreshData();

    QTableWidget* m_usersTable;
    QLineEdit* m_searchEdit;
    QComboBox* m_roleFilter;
    QPushButton* m_addUserBtn;
    QPushButton* m_editUserBtn;
    QPushButton* m_deleteUserBtn;
};

} // namespace Ballot::UI
