#include "AdminPanel.h"
#include "src/core/SystemManager.h"
#include "src/modules/election/ElectionManager.h"
#include "src/modules/election/VoteManager.h" // For getting vote counts
#include "src/modules/auth/AuthManager.h"
#include "src/modules/backup/BackupManager.h"
#include "src/modules/plugin/PluginManager.h"
#include "src/modules/audit/AuditManager.h" // For audit logging
#include "src/ui/components/ToastNotification.h"
#include "src/ui/dialogs/CandidateFormDialog.h"
#include "src/core/Utils.h" // For IdGenerator
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QScrollArea>
#include <QHeaderView>
#include <QGroupBox>
#include <QUuid> // Still included for QJsonDocument::Indented or if needed elsewhere
#include <QMessageBox>
#include <QLocale>
#include <QInputDialog>
#include <QDateTime>
#include <QDebug> // For logging

namespace Ballot::UI {

AdminPanel::AdminPanel(QWidget *parent) : QWidget(parent) {
    setupUi();
    // Connect to signals that might require data refresh
    connect(&Election::ElectionManager::instance(), &Election::ElectionManager::electionCreated, this, &AdminPanel::refreshData);
    connect(&Election::ElectionManager::instance(), &Election::ElectionManager::electionUpdated, this, &AdminPanel::refreshData);
    connect(&Election::ElectionManager::instance(), &Election::ElectionManager::electionDeleted, this, &AdminPanel::refreshData);
    connect(&Election::ElectionManager::instance(), &Election::ElectionManager::electionStateChanged, this, &AdminPanel::refreshData);
    connect(&Election::VoteManager::instance(), &Election::VoteManager::voteCast, this, &AdminPanel::refreshData); // Vote cast might change vote count
    connect(&Election::VoteManager::instance(), &Election::VoteManager::resultsUpdated, this, &AdminPanel::refreshData); // Results updated might change vote count
    qDebug() << "AdminPanel: Initialized.";
}

/**
 * @brief Sets up the user interface for the administration panel.
 */
void AdminPanel::setupUi() {
    auto *scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setStyleSheet("QScrollArea { background: transparent; border: none; }");

    auto *content = new QWidget();
    content->setStyleSheet("background: transparent;");
    auto *mainLayout = new QVBoxLayout(content);
    mainLayout->setContentsMargins(32, 24, 32, 24);
    mainLayout->setSpacing(20);

    // --- Title ---
    auto *title = new QLabel("Administration Panel", this);
    title->setObjectName("title");
    title->setStyleSheet("font-size: 32px; font-weight: 700; color: #e0e0e0;");
    mainLayout->addWidget(title);

    // --- System Information Section ---
    auto *sysGroup = new QGroupBox("System Information", this);
    sysGroup->setStyleSheet(R"(
        QGroupBox { background-color: #25253a; border: 1px solid #3d3d5c; border-radius: 12px; padding: 20px; margin-top: 16px; color: #e0e0e0; font-weight: 600; }
        QGroupBox::title { subcontrol-origin: margin; left: 16px; padding: 0 8px; }
    )");
    auto *sysLayout = new QVBoxLayout(sysGroup);
    m_systemInfo = new QLabel(sysGroup);
    m_systemInfo->setWordWrap(true);
    m_systemInfo->setStyleSheet("font-size: 13px; color: #9a9ab0; line-height: 1.6; background: transparent; font-weight: normal;");
    sysLayout->addWidget(m_systemInfo);
    mainLayout->addWidget(sysGroup);

    // --- Election Management Section ---
    auto *electionGroup = new QGroupBox("Election Management", this);
    electionGroup->setStyleSheet(R"(
        QGroupBox { background-color: #25253a; border: 1px solid #3d3d5c; border-radius: 12px; padding: 20px; margin-top: 16px; color: #e0e0e0; font-weight: 600; }
        QGroupBox::title { subcontrol-origin: margin; left: 16px; padding: 0 8px; }
    )");
    auto *electionLayout = new QVBoxLayout(electionGroup);

    auto *btnLayout = new QHBoxLayout();
    m_createElectionBtn = new QPushButton("+ Create Election", this);
    m_createElectionBtn->setObjectName("successButton");
    m_createElectionBtn->setStyleSheet(R"(
        QPushButton#successButton { background-color: #4CAF50; color: white; border: none; border-radius: 8px; font-size: 15px; font-weight: 600; padding: 8px 16px; }
        QPushButton#successButton:hover { background-color: #66BB6A; }
        QPushButton#successButton:pressed { background-color: #388E3C; }
    )");

    m_deleteElectionBtn = new QPushButton("Delete Selected", this);
    m_deleteElectionBtn->setObjectName("dangerButton");
    m_deleteElectionBtn->setStyleSheet(R"(
        QPushButton#dangerButton { background-color: #d32f2f; color: white; border: none; border-radius: 8px; font-size: 15px; font-weight: 600; padding: 8px 16px; }
        QPushButton#dangerButton:hover { background-color: #ef5350; }
        QPushButton#dangerButton:pressed { background-color: #b71c1c; }
    )");

    m_manageCandidatesBtn = new QPushButton("Manage Candidates", this);
    m_manageCandidatesBtn->setObjectName("primaryButton");
    m_manageCandidatesBtn->setStyleSheet(R"(
        QPushButton#primaryButton { background-color: #0078d4; color: white; border: none; border-radius: 8px; font-size: 15px; font-weight: 600; padding: 8px 16px; }
        QPushButton#primaryButton:hover { background-color: #1a8ae8; }
        QPushButton#primaryButton:pressed { background-color: #006cbd; }
    )");

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
    m_electionsTable->setStyleSheet(R"(
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
    electionLayout->addWidget(m_electionsTable);

    mainLayout->addWidget(electionGroup);

    mainLayout->addStretch(); // Pushes content to the top

    scrollArea->setWidget(content);

    auto *outerLayout = new QVBoxLayout(this);
    outerLayout->setContentsMargins(0, 0, 0, 0);
    outerLayout->addWidget(scrollArea);

    // --- Connect Buttons ---
    connect(m_createElectionBtn, &QPushButton::clicked, this, [this]() {
        qInfo() << "AdminPanel: Create Election button clicked.";
        bool ok;
        QString title = QInputDialog::getText(this, tr("Create New Election"),
                                             tr("Election Title:"), QLineEdit::Normal,
                                             "New Election", &ok);
        if (ok && !title.isEmpty()) {
            Core::Election election;
            // ID and createdAt will be set by ElectionManager
            election.title = title;
            election.description = "Description for " + title;
            election.startDate = QDateTime::currentDateTime();
            election.endDate = QDateTime::currentDateTime().addDays(7); // Default to 7 days duration
            election.state = Core::VotingState::Idle;
            election.isActive = false;
            election.createdBy = Auth::AuthManager::instance().currentUserId();

            if (Election::ElectionManager::instance().createElection(election)) {
                ToastNotification::show(this, "Election '" + election.title + "' created successfully.", ToastNotification::Success);
                refreshData();
                Audit::AuditManager::instance().log(Core::AuditAction::ElectionCreated, QString("Election '%1' created.").arg(election.title), Auth::AuthManager::instance().currentUserId());
            } else {
                ToastNotification::show(this, "Failed to create election.", ToastNotification::Error);
                Audit::AuditManager::instance().log(Core::AuditAction::ElectionCreated, QString("Failed to create election '%1'.").arg(election.title), Auth::AuthManager::instance().currentUserId());
            }
        } else {
            qDebug() << "AdminPanel: Create Election cancelled or empty title provided.";
        }
    });

    connect(m_deleteElectionBtn, &QPushButton::clicked, this, [this]() {
        qInfo() << "AdminPanel: Delete Selected button clicked.";
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
                Audit::AuditManager::instance().log(Core::AuditAction::ElectionDeleted, QString("Election '%1' deleted.").arg(electionTitle), Auth::AuthManager::instance().currentUserId());
            } else {
                ToastNotification::show(this, "Failed to delete election '" + electionTitle + "'.", ToastNotification::Error);
                Audit::AuditManager::instance().log(Core::AuditAction::ElectionDeleted, QString("Failed to delete election '%1'.").arg(electionTitle), Auth::AuthManager::instance().currentUserId());
            }
        } else {
            qDebug() << "AdminPanel: Election deletion cancelled.";
        }
    });

    connect(m_manageCandidatesBtn, &QPushButton::clicked, this, [this]() {
        qInfo() << "AdminPanel: Manage Candidates button clicked.";
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
        if (!ok) {
            qDebug() << "AdminPanel: Manage Candidates action cancelled.";
            return;
        }

        if (action == "Add candidate") {
            CandidateFormDialog dialog(this);
            dialog.setWindowTitle("Add Candidate - " + electionTitle);

            connect(&dialog, &CandidateFormDialog::candidateSaved, this, [this, electionId, electionTitle](const Core::Candidate& candidate) {
                Core::Candidate c = candidate;
                c.electionId = electionId; // Ensure candidate is linked to the selected election
                if (Election::ElectionManager::instance().addCandidate(c)) {
                    ToastNotification::show(this, "Candidate added successfully.", ToastNotification::Success);
                    Audit::AuditManager::instance().log(Core::AuditAction::CandidateAdded, QString("Candidate '%1' added to election '%2'.").arg(c.name, electionTitle), Auth::AuthManager::instance().currentUserId());
                } else {
                    ToastNotification::show(this, "Failed to add candidate.", ToastNotification::Error);
                    Audit::AuditManager::instance().log(Core::AuditAction::CandidateAdded, QString("Failed to add candidate '%1' to election '%2'.").arg(c.name, electionTitle), Auth::AuthManager::instance().currentUserId());
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
        if (!ok) {
            qDebug() << "AdminPanel: Candidate selection cancelled.";
            return;
        }
        const int candidateIndex = candidateLabels.indexOf(selected);
        if (candidateIndex < 0) {
            qWarning() << "AdminPanel: Selected candidate not found in list.";
            return;
        }
        Core::Candidate candidate = candidates.at(candidateIndex);

        if (action == "Edit candidate") {
            CandidateFormDialog dialog(candidate, this);
            dialog.setWindowTitle("Edit Candidate - " + electionTitle);

            connect(&dialog, &CandidateFormDialog::candidateSaved, this, [this, electionTitle](const Core::Candidate& updatedCandidate) {
                if (Election::ElectionManager::instance().updateCandidate(updatedCandidate)) {
                    ToastNotification::show(this, "Candidate updated successfully.", ToastNotification::Success);
                    Audit::AuditManager::instance().log(Core::AuditAction::CandidateModified, QString("Candidate '%1' updated in election '%2'.").arg(updatedCandidate.name, electionTitle), Auth::AuthManager::instance().currentUserId());
                } else {
                    ToastNotification::show(this, "Failed to update candidate.", ToastNotification::Error);
                    Audit::AuditManager::instance().log(Core::AuditAction::CandidateModified, QString("Failed to update candidate '%1' in election '%2'.").arg(updatedCandidate.name, electionTitle), Auth::AuthManager::instance().currentUserId());
                }
            });

            dialog.exec();
        } else if (QMessageBox::question(this, "Delete Candidate",
                                          "Are you sure you want to delete '" + candidate.name + "' from '" + electionTitle + "'?",
                                          QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) {
            if (Election::ElectionManager::instance().deleteCandidate(candidate.id)) {
                ToastNotification::show(this, "Candidate deleted successfully.", ToastNotification::Success);
                Audit::AuditManager::instance().log(Core::AuditAction::CandidateDeleted, QString("Candidate '%1' deleted from election '%2'.").arg(candidate.name, electionTitle), Auth::AuthManager::instance().currentUserId());
            } else {
                ToastNotification::show(this, "Failed to delete candidate.", ToastNotification::Error);
                Audit::AuditManager::instance().log(Core::AuditAction::CandidateDeleted, QString("Failed to delete candidate '%1' from election '%2'.").arg(candidate.name, electionTitle), Auth::AuthManager::instance().currentUserId());
            }
        }
    });

    refreshData(); // Initial data load
    qDebug() << "AdminPanel: UI setup complete.";
}

/**
 * @brief Refreshes the data displayed in the administration panel.
 * This includes system information and the elections table.
 */
void AdminPanel::refreshData() {
    qDebug() << "AdminPanel: Refreshing data...";
    auto& sys = Core::SystemManager::instance();
    auto* storage = sys.storage();

    // --- Update System Information ---
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
    qDebug() << "AdminPanel: System info updated.";

    // --- Refresh Elections Table ---
    auto elections = Election::ElectionManager::instance().getElections();
    m_electionsTable->setRowCount(elections.size());
    m_electionsTable->setSortingEnabled(false); // Disable sorting during update

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
            case Core::VotingState::Unknown: status = "Unknown"; break;
        }
        m_electionsTable->setItem(i, 3, new QTableWidgetItem(status));

        int votes = Election::VoteManager::instance().getVoteCount(e.id);
        m_electionsTable->setItem(i, 4, new QTableWidgetItem(QLocale().toString(votes)));
    }
    m_electionsTable->setSortingEnabled(true); // Re-enable sorting
    qDebug() << "AdminPanel: Elections table updated with" << elections.size() << "entries.";
}

} // namespace Ballot::UI