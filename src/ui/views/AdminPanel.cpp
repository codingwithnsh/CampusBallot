#include "AdminPanel.h"
#include "src/core/SystemManager.h"
#include "src/modules/election/ElectionManager.h"
#include "src/modules/auth/AuthManager.h"
#include "src/modules/backup/BackupManager.h"
#include "src/modules/plugin/PluginManager.h"
#include "src/ui/components/ToastNotification.h"
#include "src/ui/dialogs/CandidateFormDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QScrollArea>
#include <QHeaderView>
#include <QGroupBox>
#include <QUuid>
#include <QMessageBox>
#include <QLocale>
#include <QInputDialog>

namespace Ballot::UI {

AdminPanel::AdminPanel(QWidget *parent) : QWidget(parent) {
    setupUi();
}

void AdminPanel::setupUi() {
    auto *scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setStyleSheet("QScrollArea { background: transparent; border: none; }");

    auto *content = new QWidget();
    content->setStyleSheet("background: transparent;");
    auto *mainLayout = new QVBoxLayout(content);
    mainLayout->setContentsMargins(32, 24, 32, 24);
    mainLayout->setSpacing(20);

    // Title
    auto *title = new QLabel("Administration Panel");
    title->setObjectName("title");
    mainLayout->addWidget(title);

    // System info
    auto *sysGroup = new QGroupBox("System Information");
    sysGroup->setStyleSheet(R"(
        QGroupBox { background-color: #25253a; border: 1px solid #3d3d5c; border-radius: 12px; padding: 20px; margin-top: 16px; color: #e0e0e0; font-weight: 600; }
    )");
    auto *sysLayout = new QVBoxLayout(sysGroup);
    m_systemInfo = new QLabel(sysGroup);
    m_systemInfo->setWordWrap(true);
    m_systemInfo->setStyleSheet("font-size: 13px; color: #9a9ab0; line-height: 1.6; background: transparent; font-weight: normal;");
    sysLayout->addWidget(m_systemInfo);
    mainLayout->addWidget(sysGroup);

    // Elections section
    auto *electionGroup = new QGroupBox("Election Management");
    electionGroup->setStyleSheet(R"(
        QGroupBox { background-color: #25253a; border: 1px solid #3d3d5c; border-radius: 12px; padding: 20px; margin-top: 16px; color: #e0e0e0; font-weight: 600; }
    )");
    auto *electionLayout = new QVBoxLayout(electionGroup);

    auto *btnLayout = new QHBoxLayout();
    m_createElectionBtn = new QPushButton("+ Create Election");
    m_createElectionBtn->setObjectName("successButton");
    m_deleteElectionBtn = new QPushButton("Delete Selected");
    m_deleteElectionBtn->setObjectName("dangerButton");
    m_manageCandidatesBtn = new QPushButton("Manage Candidates");
    m_manageCandidatesBtn->setObjectName("primaryButton");
    btnLayout->addWidget(m_createElectionBtn);
    btnLayout->addWidget(m_manageCandidatesBtn);
    btnLayout->addWidget(m_deleteElectionBtn);
    btnLayout->addStretch();
    electionLayout->addLayout(btnLayout);

    m_electionsTable = new QTableWidget(this);
    m_electionsTable->setColumnCount(5);
    m_electionsTable->setHorizontalHeaderLabels({"Title", "Start Date", "End Date", "Status", "Votes"});
    m_electionsTable->horizontalHeader()->setStretchLastSection(true);
    m_electionsTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_electionsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_electionsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    electionLayout->addWidget(m_electionsTable);

    mainLayout->addWidget(electionGroup);

    mainLayout->addStretch();

    scrollArea->setWidget(content);

    auto *outerLayout = new QVBoxLayout(this);
    outerLayout->setContentsMargins(0, 0, 0, 0);
    outerLayout->addWidget(scrollArea);

    // Connect buttons
    connect(m_createElectionBtn, &QPushButton::clicked, this, [this]() {
        bool ok;
        QString text = QInputDialog::getText(this, tr("Create New Election"),
                                             tr("Election Title:"), QLineEdit::Normal,
                                             "New Election", &ok);
        if (ok && !text.isEmpty()) {
            Core::Election election;
            election.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
            election.title = text;
            election.description = "Description for " + text;
            election.startDate = QDateTime::currentDateTime();
            election.endDate = QDateTime::currentDateTime().addDays(7);
            election.state = Core::VotingState::Idle;
            election.isActive = false;
            election.createdBy = Auth::AuthManager::instance().currentUserId(); // Use current user ID

            if (Election::ElectionManager::instance().createElection(election)) {
                ToastNotification::show(this, "Election '" + election.title + "' created successfully.", ToastNotification::Success);
                refreshData();
            } else {
                ToastNotification::show(this, "Failed to create election.", ToastNotification::Error);
            }
        }
    });

    connect(m_deleteElectionBtn, &QPushButton::clicked, this, [this]() {
        auto selectedItems = m_electionsTable->selectedItems();
        if (selectedItems.isEmpty()) {
            ToastNotification::show(this, "Please select an election to delete.", ToastNotification::Warning);
            return;
        }
        int row = selectedItems.first()->row();
        QString electionId = m_electionsTable->item(row, 0)->data(Qt::UserRole).toString();
        QString electionTitle = m_electionsTable->item(row, 0)->text();

        QMessageBox::StandardButton reply;
        reply = QMessageBox::question(this, "Confirm Deletion",
                                      "Are you sure you want to delete election '" + electionTitle + "'?\nThis action cannot be undone.",
                                      QMessageBox::Yes | QMessageBox::No);
        if (reply == QMessageBox::Yes) {
            if (Election::ElectionManager::instance().deleteElection(electionId)) {
                ToastNotification::show(this, "Election '" + electionTitle + "' deleted successfully.", ToastNotification::Success);
                refreshData();
            } else {
                ToastNotification::show(this, "Failed to delete election '" + electionTitle + "'.", ToastNotification::Error);
            }
        }
    });

    connect(m_manageCandidatesBtn, &QPushButton::clicked, this, [this]() {
        auto selectedItems = m_electionsTable->selectedItems();
        if (selectedItems.isEmpty()) {
            ToastNotification::show(this, "Please select an election to manage candidates for.", ToastNotification::Warning);
            return;
        }
        int row = selectedItems.first()->row();
        QString electionId = m_electionsTable->item(row, 0)->data(Qt::UserRole).toString();
        QString electionTitle = m_electionsTable->item(row, 0)->text();

        auto candidates = Election::ElectionManager::instance().getCandidates(electionId);
        QStringList actions = {"Add candidate"};
        if (!candidates.isEmpty()) actions << "Edit candidate" << "Delete candidate";

        bool ok = false;
        const QString action = QInputDialog::getItem(this, "Manage Candidates - " + electionTitle,
                                                      "Action:", actions, 0, false, &ok);
        if (!ok) return;

        if (action == "Add candidate") {
            // Use the new CandidateFormDialog
            CandidateFormDialog dialog(this);
            dialog.setWindowTitle("Add Candidate - " + electionTitle);
            
            // Pre-set election ID
            connect(&dialog, &CandidateFormDialog::candidateSaved, this, [this, electionId](const Core::Candidate& candidate) {
                Core::Candidate c = candidate;
                c.electionId = electionId;
                if (Election::ElectionManager::instance().addCandidate(c)) {
                    ToastNotification::show(this, "Candidate added successfully.", ToastNotification::Success);
                } else {
                    ToastNotification::show(this, "Failed to add candidate.", ToastNotification::Error);
                }
            });
            
            dialog.exec();
            return;
        }

        QStringList candidateLabels;
        for (const auto& candidate : candidates) {
            candidateLabels << QString("%1 (%2)").arg(candidate.name, candidate.id.left(8));
        }
        const QString selected = QInputDialog::getItem(this, action, "Candidate:", candidateLabels, 0, false, &ok);
        if (!ok) return;
        const int candidateIndex = candidateLabels.indexOf(selected);
        if (candidateIndex < 0) return;
        Core::Candidate candidate = candidates.at(candidateIndex);

        if (action == "Edit candidate") {
            // Use the new CandidateFormDialog for editing
            CandidateFormDialog dialog(candidate, this);
            dialog.setWindowTitle("Edit Candidate - " + electionTitle);
            
            connect(&dialog, &CandidateFormDialog::candidateSaved, this, [this](const Core::Candidate& updatedCandidate) {
                if (Election::ElectionManager::instance().updateCandidate(updatedCandidate)) {
                    ToastNotification::show(this, "Candidate updated successfully.", ToastNotification::Success);
                } else {
                    ToastNotification::show(this, "Failed to update candidate.", ToastNotification::Error);
                }
            });
            
            dialog.exec();
        } else if (QMessageBox::question(this, "Delete Candidate",
                                          "Delete '" + candidate.name + "'?",
                                          QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) {
            if (Election::ElectionManager::instance().deleteCandidate(candidate.id)) {
                ToastNotification::show(this, "Candidate deleted successfully.", ToastNotification::Success);
            } else {
                ToastNotification::show(this, "Failed to delete candidate.", ToastNotification::Error);
            }
        }
    });

    refreshData();
}

void AdminPanel::refreshData() {
    auto& sys = Core::SystemManager::instance();
    auto* storage = sys.storage();

    QString info;
    info += "System Status: " + QString(sys.isInitialized() ? "Running" : "Stopped") + "\n";
    info += "Storage Provider: " + QString(storage ? storage->providerName() : "None") + "\n";
    info += "Machine ID: " + sys.machineId().left(16) + "...\n";
    info += "Master Node: " + QString(sys.isMaster() ? "Yes" : "No") + "\n";

    auto backups = Backup::BackupManager::instance().getBackups();
    info += "Backups Available: " + QString::number(backups.size()) + "\n";

    auto plugins = Plugin::PluginManager::instance().loadedPluginIds();
    info += "Active Plugins: " + QString::number(plugins.size());

    m_systemInfo->setText(info);

    // Refresh elections table
    auto elections = Election::ElectionManager::instance().getElections();
    m_electionsTable->setRowCount(elections.size());
    for (int i = 0; i < elections.size(); ++i) {
        const auto& e = elections[i];
        auto* titleItem = new QTableWidgetItem(e.title);
        titleItem->setData(Qt::UserRole, e.id); // Store election ID in UserRole
        m_electionsTable->setItem(i, 0, titleItem);
        m_electionsTable->setItem(i, 1, new QTableWidgetItem(e.startDate.toString("yyyy-MM-dd HH:mm")));
        m_electionsTable->setItem(i, 2, new QTableWidgetItem(e.endDate.toString("yyyy-MM-dd HH:mm")));

        QString status;
        switch (e.state) {
            case Core::VotingState::Idle: status = "Not Started"; break;
            case Core::VotingState::Voting: status = "In Progress"; break;
            case Core::VotingState::Ended: status = "Ended"; break;
            case Core::VotingState::Paused: status = "Paused"; break;
        }
        m_electionsTable->setItem(i, 3, new QTableWidgetItem(status));

        if (storage) {
            int votes = storage->getVoteCount(e.id);
            m_electionsTable->setItem(i, 4, new QTableWidgetItem(QLocale().toString(votes)));
        }
    }
}

} // namespace Ballot::UI
