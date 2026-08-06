#include "AdminPanel.h"
#include "src/core/SystemManager.h"
#include "src/modules/election/ElectionManager.h"
#include "src/modules/election/VoteManager.h" // For getting vote counts
#include "src/modules/auth/AuthManager.h"
#include "src/modules/auth/RBACManager.h"
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
#include <QDateTime>
#include <QDebug> // For logging
#include <QDialog>
#include <QWizard>
#include <QWizardPage>
#include <QFormLayout>
#include <QLineEdit>
#include <QTextEdit>
#include <QDateTimeEdit>
#include <QSpinBox>
#include <QCheckBox>
#include <QRadioButton>
#include <QButtonGroup>
#include <QFileDialog>

namespace Ballot::UI {

namespace {

QStringList splitCsvValues(const QString& value) {
    QStringList result;
    const auto parts = value.split(',', Qt::SkipEmptyParts);
    for (const QString& part : parts) {
        const QString normalized = part.trimmed();
        if (!normalized.isEmpty() && !result.contains(normalized, Qt::CaseInsensitive)) {
            result.append(normalized);
        }
    }
    return result;
}

QString buildEligibilitySummary(const QStringList& classes, const QStringList& departments) {
    QStringList lines;
    lines << QString("Eligible classes: %1").arg(classes.isEmpty() ? "All classes" : classes.join(", "));
    lines << QString("Eligible departments: %1").arg(departments.isEmpty() ? "All departments" : departments.join(", "));
    return lines.join('\n');
}

} // namespace

AdminPanel::AdminPanel(QWidget *parent) : QWidget(parent) {
    setupUi();
    // Connect to signals that might require data refresh
    connect(&Auth::AuthManager::instance(), &Auth::AuthManager::authStateChanged, this, &AdminPanel::updateActionAvailability);
    connect(&Auth::AuthManager::instance(), &Auth::AuthManager::loginSuccessful, this, [this](const QString&) { updateActionAvailability(); });
    connect(&Auth::AuthManager::instance(), &Auth::AuthManager::logoutOccurred, this, &AdminPanel::updateActionAvailability);
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
    connect(m_createElectionBtn, &QPushButton::clicked, this, &AdminPanel::showCreateElectionWizard);

    connect(m_deleteElectionBtn, &QPushButton::clicked, this, [this]() {
        qInfo() << "AdminPanel: Delete Selected button clicked.";
        if (!Auth::AuthManager::instance().hasPermission(Auth::RBACManager::PERM_ELECTION_DELETE)) {
            ToastNotification::show(this, "You do not have permission to delete elections.", ToastNotification::Error);
            Audit::AuditManager::instance().log(Core::AuditAction::PermissionDenied, "Denied election deletion from admin panel.", Auth::AuthManager::instance().currentUserId());
            return;
        }

        auto selectedItems = m_electionsTable->selectedItems();
        if (selectedItems.isEmpty()) {
            ToastNotification::show(this, "Please select an election to delete.", ToastNotification::Warning);
            return;
        }
        int row = selectedItems.first()->row();
        QString electionId = m_electionsTable->item(row, 0)->data(Qt::UserRole).toString();
        QString electionTitle = m_electionsTable->item(row, 0)->text();

        const auto election = Election::ElectionManager::instance().getElection(electionId);
        if (!election) {
            ToastNotification::show(this, "Election not found. Refreshing list.", ToastNotification::Error);
            refreshData();
            return;
        }
        if (election->state == Core::VotingState::Voting) {
            ToastNotification::show(this, "End or pause the election before deleting it.", ToastNotification::Warning);
            return;
        }

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
        if (!Auth::AuthManager::instance().hasPermission(Auth::RBACManager::PERM_CANDIDATE_MANAGE)) {
            ToastNotification::show(this, "You do not have permission to manage candidates.", ToastNotification::Error);
            Audit::AuditManager::instance().log(Core::AuditAction::PermissionDenied, "Denied candidate management from admin panel.", Auth::AuthManager::instance().currentUserId());
            return;
        }

        auto selectedItems = m_electionsTable->selectedItems();
        if (selectedItems.isEmpty()) {
            ToastNotification::show(this, "Please select an election to manage candidates for.", ToastNotification::Warning);
            return;
        }
        int row = selectedItems.first()->row();
        QString electionId = m_electionsTable->item(row, 0)->data(Qt::UserRole).toString();
        QString electionTitle = m_electionsTable->item(row, 0)->text();

        QDialog dialog(this);
        dialog.setWindowTitle("Manage Candidates - " + electionTitle);
        dialog.setModal(true);
        dialog.resize(900, 560);
        dialog.setStyleSheet(R"(
            QDialog { background-color: #1e1e34; }
            QLabel { color: #e0e0e0; background: transparent; }
            QTableWidget {
                background-color: #1e1e34; alternate-background-color: #25253a;
                border: 1px solid #3d3d5c; border-radius: 10px; color: #e0e0e0;
                font-size: 14px; gridline-color: #3d3d5c;
            }
            QHeaderView::section {
                background-color: #2d2d44; color: #ffffff; padding: 8px;
                border: 1px solid #3d3d5c; font-weight: 600;
            }
            QTableWidget::item { padding: 8px; }
            QPushButton {
                background-color: #0078d4; color: white; border: none; border-radius: 8px;
                padding: 10px 18px; font-size: 14px; font-weight: 600;
            }
            QPushButton:hover { background-color: #1a8ae8; }
            QPushButton:disabled { background-color: #34344a; color: #77778c; }
            QPushButton#dangerButton { background-color: #d32f2f; }
            QPushButton#dangerButton:hover { background-color: #ef5350; }
            QPushButton#ghostButton {
                background-color: transparent; color: #c6c6d8; border: 1px solid #3d3d5c;
            }
            QPushButton#ghostButton:hover { background-color: #2a2a42; }
        )");

        auto* dialogLayout = new QVBoxLayout(&dialog);
        dialogLayout->setContentsMargins(24, 20, 24, 20);
        dialogLayout->setSpacing(14);

        auto* titleLabel = new QLabel(QString("Candidates for %1").arg(electionTitle), &dialog);
        titleLabel->setStyleSheet("font-size: 22px; font-weight: 700; color: #ffffff;");
        dialogLayout->addWidget(titleLabel);

        auto* helperLabel = new QLabel("Manage candidate profiles, approval state, party details, manifestos, media, and visibility from one place.", &dialog);
        helperLabel->setWordWrap(true);
        helperLabel->setStyleSheet("font-size: 13px; color: #a8a8bd;");
        dialogLayout->addWidget(helperLabel);

        auto* candidatesTable = new QTableWidget(&dialog);
        candidatesTable->setColumnCount(5);
        candidatesTable->setHorizontalHeaderLabels({"Candidate", "Party", "Class", "Section", "Status"});
        candidatesTable->horizontalHeader()->setStretchLastSection(true);
        candidatesTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        candidatesTable->setSelectionBehavior(QAbstractItemView::SelectRows);
        candidatesTable->setSelectionMode(QAbstractItemView::SingleSelection);
        candidatesTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
        candidatesTable->setAlternatingRowColors(true);
        candidatesTable->setAccessibleName("Candidates table");
        dialogLayout->addWidget(candidatesTable, 1);

        auto* footerLayout = new QHBoxLayout();
        auto* addBtn = new QPushButton("+ Add Candidate", &dialog);
        auto* editBtn = new QPushButton("Edit Selected", &dialog);
        auto* deleteBtn = new QPushButton("Delete Selected", &dialog);
        deleteBtn->setObjectName("dangerButton");
        auto* closeBtn = new QPushButton("Close", &dialog);
        closeBtn->setObjectName("ghostButton");

        footerLayout->addWidget(addBtn);
        footerLayout->addWidget(editBtn);
        footerLayout->addWidget(deleteBtn);
        footerLayout->addStretch();
        footerLayout->addWidget(closeBtn);
        dialogLayout->addLayout(footerLayout);

        QList<Core::Candidate> candidates;
        auto reloadCandidates = [&]() {
            candidates = Election::ElectionManager::instance().getCandidates(electionId);
            candidatesTable->setRowCount(candidates.size());
            for (int i = 0; i < candidates.size(); ++i) {
                const auto& c = candidates.at(i);
                auto* nameItem = new QTableWidgetItem(c.name);
                nameItem->setData(Qt::UserRole, c.id);
                candidatesTable->setItem(i, 0, nameItem);
                candidatesTable->setItem(i, 1, new QTableWidgetItem(c.party));
                candidatesTable->setItem(i, 2, new QTableWidgetItem(c.className));
                candidatesTable->setItem(i, 3, new QTableWidgetItem(c.section.isEmpty() ? "—" : c.section));
                candidatesTable->setItem(i, 4, new QTableWidgetItem(c.isApproved ? "Approved" : "Pending"));
            }
            candidatesTable->resizeRowsToContents();
            const bool hasSelection = candidatesTable->currentRow() >= 0 && candidatesTable->currentRow() < candidates.size();
            editBtn->setEnabled(hasSelection);
            deleteBtn->setEnabled(hasSelection);
        };

        auto selectedCandidate = [&]() -> std::optional<Core::Candidate> {
            const int selectedRow = candidatesTable->currentRow();
            if (selectedRow < 0 || selectedRow >= candidates.size()) {
                return std::nullopt;
            }
            return candidates.at(selectedRow);
        };

        auto addCandidate = [&]() {
            CandidateFormDialog dialog(this);
            dialog.setWindowTitle("Add Candidate - " + electionTitle);

            connect(&dialog, &CandidateFormDialog::candidateSaved, this, [&, this](const Core::Candidate& candidate) {
                Core::Candidate c = candidate;
                c.electionId = electionId; // Ensure candidate is linked to the selected election
                if (Election::ElectionManager::instance().addCandidate(c)) {
                    ToastNotification::show(this, "Candidate added successfully.", ToastNotification::Success);
                    Audit::AuditManager::instance().log(Core::AuditAction::CandidateAdded, QString("Candidate '%1' added to election '%2'.").arg(c.name, electionTitle), Auth::AuthManager::instance().currentUserId());
                    reloadCandidates();
                    refreshData();
                } else {
                    ToastNotification::show(this, "Failed to add candidate.", ToastNotification::Error);
                    Audit::AuditManager::instance().log(Core::AuditAction::CandidateAdded, QString("Failed to add candidate '%1' to election '%2'.").arg(c.name, electionTitle), Auth::AuthManager::instance().currentUserId());
                }
            });

            dialog.exec();
        };

        auto editCandidate = [&]() {
            const auto candidateOpt = selectedCandidate();
            if (!candidateOpt) {
                ToastNotification::show(this, "Select a candidate to edit.", ToastNotification::Warning);
                return;
            }
            const Core::Candidate candidate = *candidateOpt;
            CandidateFormDialog dialog(candidate, this);
            dialog.setWindowTitle("Edit Candidate - " + electionTitle);

            connect(&dialog, &CandidateFormDialog::candidateSaved, this, [&, this](const Core::Candidate& updatedCandidate) {
                if (Election::ElectionManager::instance().updateCandidate(updatedCandidate)) {
                    ToastNotification::show(this, "Candidate updated successfully.", ToastNotification::Success);
                    Audit::AuditManager::instance().log(Core::AuditAction::CandidateModified, QString("Candidate '%1' updated in election '%2'.").arg(updatedCandidate.name, electionTitle), Auth::AuthManager::instance().currentUserId());
                    reloadCandidates();
                    refreshData();
                } else {
                    ToastNotification::show(this, "Failed to update candidate.", ToastNotification::Error);
                    Audit::AuditManager::instance().log(Core::AuditAction::CandidateModified, QString("Failed to update candidate '%1' in election '%2'.").arg(updatedCandidate.name, electionTitle), Auth::AuthManager::instance().currentUserId());
                }
            });

            dialog.exec();
        };

        auto deleteCandidate = [&]() {
            const auto candidateOpt = selectedCandidate();
            if (!candidateOpt) {
                ToastNotification::show(this, "Select a candidate to delete.", ToastNotification::Warning);
                return;
            }
            const Core::Candidate candidate = *candidateOpt;
            if (QMessageBox::question(&dialog, "Delete Candidate",
                                      "Are you sure you want to delete '" + candidate.name + "' from '" + electionTitle + "'?",
                                      QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes) {
                return;
            }
            if (Election::ElectionManager::instance().deleteCandidate(candidate.id)) {
                ToastNotification::show(this, "Candidate deleted successfully.", ToastNotification::Success);
                Audit::AuditManager::instance().log(Core::AuditAction::CandidateDeleted, QString("Candidate '%1' deleted from election '%2'.").arg(candidate.name, electionTitle), Auth::AuthManager::instance().currentUserId());
                reloadCandidates();
                refreshData();
            } else {
                ToastNotification::show(this, "Failed to delete candidate.", ToastNotification::Error);
                Audit::AuditManager::instance().log(Core::AuditAction::CandidateDeleted, QString("Failed to delete candidate '%1' from election '%2'.").arg(candidate.name, electionTitle), Auth::AuthManager::instance().currentUserId());
            }
        };

        connect(addBtn, &QPushButton::clicked, &dialog, addCandidate);
        connect(editBtn, &QPushButton::clicked, &dialog, editCandidate);
        connect(deleteBtn, &QPushButton::clicked, &dialog, deleteCandidate);
        connect(closeBtn, &QPushButton::clicked, &dialog, &QDialog::accept);
        connect(candidatesTable, &QTableWidget::itemDoubleClicked, &dialog, [editCandidate](QTableWidgetItem*) { editCandidate(); });
        connect(candidatesTable->selectionModel(), &QItemSelectionModel::selectionChanged, &dialog, [&]() {
            const bool hasSelection = candidatesTable->currentRow() >= 0 && candidatesTable->currentRow() < candidates.size();
            editBtn->setEnabled(hasSelection);
            deleteBtn->setEnabled(hasSelection);
        });

        reloadCandidates();
        dialog.exec();
    });

    refreshData(); // Initial data load
    qDebug() << "AdminPanel: UI setup complete.";
}

void AdminPanel::showCreateElectionWizard() {
    qInfo() << "AdminPanel: Create Election wizard opened.";
    if (!Auth::AuthManager::instance().hasPermission(Auth::RBACManager::PERM_ELECTION_CREATE)) {
        ToastNotification::show(this, "You do not have permission to create elections.", ToastNotification::Error);
        Audit::AuditManager::instance().log(Core::AuditAction::PermissionDenied, "Denied election creation from admin panel.", Auth::AuthManager::instance().currentUserId());
        return;
    }

    QWizard wizard(this);
    wizard.setWindowTitle("Create Election");
    wizard.setWizardStyle(QWizard::ModernStyle);
    wizard.setOption(QWizard::NoBackButtonOnStartPage);
    wizard.setMinimumSize(720, 520);

    auto* basicPage = new QWizardPage(&wizard);
    basicPage->setTitle("Basic Information");
    basicPage->setSubTitle("Name the election and describe its purpose for voters and administrators.");

    auto* titleEdit = new QLineEdit(basicPage);
    titleEdit->setPlaceholderText("e.g. Student Council Election 2026");
    titleEdit->setMaxLength(120);
    titleEdit->setAccessibleName("Election name");

    auto* descriptionEdit = new QTextEdit(basicPage);
    descriptionEdit->setPlaceholderText("Describe eligibility, goals, offices, and any important instructions.");
    descriptionEdit->setAcceptRichText(false);
    descriptionEdit->setMinimumHeight(140);
    descriptionEdit->setAccessibleName("Election description");

    auto* classesEdit = new QLineEdit(basicPage);
    classesEdit->setPlaceholderText("Optional, comma-separated: Grade 10, Grade 11");
    classesEdit->setAccessibleName("Eligible classes");

    auto* departmentsEdit = new QLineEdit(basicPage);
    departmentsEdit->setPlaceholderText("Optional, comma-separated: Computer Science, Commerce");
    departmentsEdit->setAccessibleName("Eligible departments");

    auto* basicLayout = new QFormLayout(basicPage);
    basicLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    basicLayout->addRow("Election name *", titleEdit);
    basicLayout->addRow("Description", descriptionEdit);
    basicLayout->addRow("Eligible classes", classesEdit);
    basicLayout->addRow("Eligible departments", departmentsEdit);
    wizard.addPage(basicPage);

    auto* schedulePage = new QWizardPage(&wizard);
    schedulePage->setTitle("Schedule & Rules");
    schedulePage->setSubTitle("Define when voting is open and the core voting constraints.");

    const QDateTime now = QDateTime::currentDateTime();
    auto* startEdit = new QDateTimeEdit(now.addSecs(3600), schedulePage);
    startEdit->setCalendarPopup(true);
    startEdit->setDisplayFormat("yyyy-MM-dd HH:mm");
    startEdit->setAccessibleName("Voting start date and time");

    auto* endEdit = new QDateTimeEdit(now.addDays(7), schedulePage);
    endEdit->setCalendarPopup(true);
    endEdit->setDisplayFormat("yyyy-MM-dd HH:mm");
    endEdit->setAccessibleName("Voting end date and time");

    auto* maxVotesSpin = new QSpinBox(schedulePage);
    maxVotesSpin->setRange(1, 50);
    maxVotesSpin->setValue(1);
    maxVotesSpin->setAccessibleName("Maximum votes per student");

    auto* verificationCheck = new QCheckBox("Require student verification before voting", schedulePage);
    verificationCheck->setChecked(true);
    verificationCheck->setAccessibleName("Require student verification");
    verificationCheck->setStyleSheet("border: none;");

    auto* photoOptionalCheck = new QCheckBox("Make taking photo optional", schedulePage);
    photoOptionalCheck->setChecked(false);
    photoOptionalCheck->setAccessibleName("Photo optional");
    photoOptionalCheck->setStyleSheet("border: none;");

    auto* verificationTypeGroup = new QGroupBox("Verification Method", schedulePage);
    auto* verificationTypeLayout = new QVBoxLayout(verificationTypeGroup);
    auto* noneRadio = new QRadioButton("No verification (Bypass)", verificationTypeGroup);
    noneRadio->setStyleSheet("border: none;");
    auto* databaseRadio = new QRadioButton("Verify against Database", verificationTypeGroup);
    databaseRadio->setStyleSheet("border: none;");
    auto* fileRadio = new QRadioButton("Verify against File (Excel/CSV)", verificationTypeGroup);
    fileRadio->setStyleSheet("border: none;");
    verificationTypeLayout->addWidget(noneRadio);
    verificationTypeLayout->addWidget(databaseRadio);
    verificationTypeLayout->addWidget(fileRadio);
    databaseRadio->setChecked(true);

    auto* filePickerWidget = new QWidget(schedulePage);
    auto* filePickerLayout = new QHBoxLayout(filePickerWidget);
    auto* filePathEdit = new QLineEdit(filePickerWidget);
    filePathEdit->setReadOnly(true);
    auto* browseBtn = new QPushButton("Browse...", filePickerWidget);
    filePickerLayout->addWidget(filePathEdit);
    filePickerLayout->addWidget(browseBtn);
    filePickerWidget->setVisible(false);

    auto* columnEdit = new QLineEdit(schedulePage);
    columnEdit->setPlaceholderText("Column name to verify (e.g. AdmissionNumber)");
    columnEdit->setVisible(false);

    connect(verificationCheck, &QCheckBox::toggled, verificationTypeGroup, &QGroupBox::setEnabled);
    connect(fileRadio, &QRadioButton::toggled, filePickerWidget, &QWidget::setVisible);
    connect(fileRadio, &QRadioButton::toggled, columnEdit, &QWidget::setVisible);

    connect(browseBtn, &QPushButton::clicked, this, [=, this]() {
        QString path = QFileDialog::getOpenFileName(this, "Select Verification File", "", "Excel Files (*.xlsx *.xls);;CSV Files (*.csv)");
        if (!path.isEmpty()) {
            filePathEdit->setText(path);
        }
    });

    auto* scheduleLayout = new QFormLayout(schedulePage);
    scheduleLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    scheduleLayout->addRow("Voting starts", startEdit);
    scheduleLayout->addRow("Voting ends", endEdit);
    scheduleLayout->addRow("Max votes per student", maxVotesSpin);
    scheduleLayout->addRow("", verificationCheck);
    scheduleLayout->addRow("", photoOptionalCheck);
    scheduleLayout->addRow(verificationTypeGroup);
    scheduleLayout->addRow("Verification file", filePickerWidget);
    scheduleLayout->addRow("Verification column", columnEdit);
    wizard.addPage(schedulePage);

    auto* reviewPage = new QWizardPage(&wizard);
    reviewPage->setTitle("Review");
    reviewPage->setSubTitle("Confirm the election setup before it is created.");
    auto* reviewLabel = new QLabel(reviewPage);
    reviewLabel->setWordWrap(true);
    reviewLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    reviewLabel->setAccessibleName("Election creation summary");
    auto* reviewLayout = new QVBoxLayout(reviewPage);
    reviewLayout->addWidget(reviewLabel);
    wizard.addPage(reviewPage);

    connect(&wizard, &QWizard::currentIdChanged, &wizard, [&](int id) {
        if (wizard.page(id) != reviewPage) {
            return;
        }

        const QStringList classes = splitCsvValues(classesEdit->text());
        const QStringList departments = splitCsvValues(departmentsEdit->text());
        reviewLabel->setText(QString(
            "<b>%1</b><br><br>"
            "%2<br><br>"
            "<b>Schedule</b><br>"
            "Starts: %3<br>"
            "Ends: %4<br><br>"
            "<b>Rules</b><br>"
            "Max votes per student: %5<br>"
            "Verification required: %6<br><br>"
            "<b>Eligibility</b><br>%7")
            .arg(titleEdit->text().trimmed().toHtmlEscaped(),
                 descriptionEdit->toPlainText().trimmed().isEmpty()
                    ? QString("No public description provided.")
                    : descriptionEdit->toPlainText().trimmed().toHtmlEscaped(),
                 startEdit->dateTime().toString("yyyy-MM-dd HH:mm"),
                 endEdit->dateTime().toString("yyyy-MM-dd HH:mm"),
                 QString::number(maxVotesSpin->value()),
                 verificationCheck->isChecked() ? "Yes" : "No",
                 buildEligibilitySummary(classes, departments).toHtmlEscaped().replace("\n", "<br>")));
    });

    if (wizard.exec() != QDialog::Accepted) {
        qDebug() << "AdminPanel: Create Election wizard cancelled.";
        return;
    }

    const QString title = titleEdit->text().trimmed();
    if (title.isEmpty()) {
        ToastNotification::show(this, "Election name is required.", ToastNotification::Warning);
        return;
    }

    if (endEdit->dateTime() <= startEdit->dateTime()) {
        ToastNotification::show(this, "Voting end time must be after the start time.", ToastNotification::Warning);
        return;
    }

    Core::Election election;
    election.title = title;
    election.description = descriptionEdit->toPlainText().trimmed();
    election.startDate = startEdit->dateTime();
    election.endDate = endEdit->dateTime();
    election.state = Core::VotingState::Idle;
    election.isActive = false;
    election.createdBy = Auth::AuthManager::instance().currentUserId();
    election.eligibleClasses = splitCsvValues(classesEdit->text());
    election.eligibleDepartments = splitCsvValues(departmentsEdit->text());
    election.maxVotesPerStudent = maxVotesSpin->value();
    election.requireVerification = verificationCheck->isChecked();
    election.photoOptional = photoOptionalCheck->isChecked();
    election.verifyStudents = !noneRadio->isChecked() && verificationCheck->isChecked();
    if (fileRadio->isChecked()) {
        election.studentVerificationType = "File";
        election.verificationFilePath = filePathEdit->text();
        election.verificationColumn = columnEdit->text().trimmed();
    } else if (databaseRadio->isChecked()) {
        election.studentVerificationType = "Database";
    } else {
        election.studentVerificationType = "None";
    }

    if (Election::ElectionManager::instance().createElection(election)) {
        ToastNotification::show(this, "Election '" + election.title + "' created successfully.", ToastNotification::Success);
        refreshData();
        Audit::AuditManager::instance().log(Core::AuditAction::ElectionCreated, QString("Election '%1' created from wizard.").arg(election.title), Auth::AuthManager::instance().currentUserId());
        return;
    }

    ToastNotification::show(this, "Failed to create election.", ToastNotification::Error);
    Audit::AuditManager::instance().log(Core::AuditAction::ElectionCreated, QString("Failed to create election '%1' from wizard.").arg(election.title), Auth::AuthManager::instance().currentUserId());
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
    updateActionAvailability();
}

void AdminPanel::updateActionAvailability() {
    auto& auth = Auth::AuthManager::instance();
    const bool authenticated = auth.isAuthenticated();
    const bool canCreateElection = authenticated && Auth::RBACManager::instance().hasPermission(auth.currentRole(), Auth::RBACManager::PERM_ELECTION_CREATE);
    const bool canDeleteElection = authenticated && Auth::RBACManager::instance().hasPermission(auth.currentRole(), Auth::RBACManager::PERM_ELECTION_DELETE);
    const bool canManageCandidates = authenticated && Auth::RBACManager::instance().hasPermission(auth.currentRole(), Auth::RBACManager::PERM_CANDIDATE_MANAGE);

    m_createElectionBtn->setEnabled(canCreateElection);
    m_createElectionBtn->setToolTip(canCreateElection ? "Create a new election" : "Requires election creation permission");

    m_deleteElectionBtn->setEnabled(canDeleteElection);
    m_deleteElectionBtn->setToolTip(canDeleteElection ? "Delete the selected non-running election" : "Requires election deletion permission");

    m_manageCandidatesBtn->setEnabled(canManageCandidates);
    m_manageCandidatesBtn->setToolTip(canManageCandidates ? "Add, edit, or delete candidates" : "Requires candidate management permission");
}

} // namespace Ballot::UI
