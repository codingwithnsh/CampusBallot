#pragma once

#include <QWidget>
#include <QPushButton>
#include <QLabel>
#include <QTableWidget>
#include <QComboBox>

namespace Ballot::UI {

class AdminPanel : public QWidget {
    Q_OBJECT
public:
    explicit AdminPanel(QWidget *parent = nullptr);

private:
    void setupUi();
    void refreshData();

    QTableWidget* m_electionsTable;
    QPushButton* m_createElectionBtn;
    QPushButton* m_deleteElectionBtn;
    QPushButton* m_manageCandidatesBtn;
    QLabel* m_systemInfo;
};

} // namespace Ballot::UI
