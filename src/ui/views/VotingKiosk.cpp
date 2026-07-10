#include "VotingKiosk.h"
#include "src/core/SystemManager.h"
#include "src/ui/components/ToastNotification.h" // For error messages
#include "src/modules/election/ElectionManager.h" // For election state
#include "src/modules/election/VoteManager.h"     // For casting votes
#include "src/modules/storage/TestAdmissionStorage.h" // For test mode
#include "src/core/Utils.h" // For IdGenerator
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QScrollArea>
#include <QLineEdit>
#include <QFrame>
#include <QGraphicsDropShadowEffect>
#include <QApplication>
#include <QDebug> // For debugging
#include <QCheckBox>
#include <QUuid> // Still included for QJsonDocument::Indented or if needed elsewhere
#include <QPixmap> // For candidate photos
#include <QBuffer> // For QImage to QByteArray conversion

namespace Ballot::UI {

VotingKiosk::VotingKiosk(QWidget *parent) : QWidget(parent) {
    setupUi();
    qDebug() << "VotingKiosk: Initialized.";

    m_stateTimer = new QTimer(this);
    m_stateTimer->setInterval(Core::Constants::VOTING_STATE_POLL_MS); // Use constant for polling interval
    connect(m_stateTimer, &QTimer::timeout, this, &VotingKiosk::updateVotingState);
    m_stateTimer->start(); // Start polling immediately

    updateVotingState(); // Initial check
}

VotingKiosk::~VotingKiosk() {
    qDebug() << "VotingKiosk: Destroyed.";
}

/**
 * @brief Sets the test mode for the kiosk.
 * @param enabled True to enable test mode, false to disable.
 */
void VotingKiosk::setTestMode(bool enabled) {
    if (m_testMode == enabled) return; // No change
    m_testMode = enabled;
    Storage::TestAdmissionStorage::instance().setTestMode(enabled);
    if (m_testModeCheck) {
        m_testModeCheck->setChecked(enabled);
    }
    qInfo() << "VotingKiosk: Test mode set to" << enabled;
}

/**
 * @brief Sets up the user interface for the voting kiosk.
 */
void VotingKiosk::setupUi() {
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    // --- Header bar ---
    auto *header = new QFrame(this);
    header->setFixedHeight(60);
    header->setStyleSheet("background-color: #1a1a2e; border-bottom: 1px solid #2d2d44;");
    auto *headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(16, 0, 8, 0);

    auto *title = new QLabel("🗳️  Campus Ballot - Voting Kiosk", header);
    title->setStyleSheet("font-size: 18px; font-weight: 600; color: #e0e0e0; background: transparent;");
    headerLayout->addWidget(title);

    headerLayout->addStretch();

    auto *closeBtn = new QPushButton("✕", header);
    closeBtn->setObjectName("iconButton");
    closeBtn->setStyleSheet("font-size: 20px; color: #9a9ab0; background: transparent; border: none; padding: 8px;");
    closeBtn->setCursor(Qt::PointingHandCursor);
    connect(closeBtn, &QPushButton::clicked, this, [this]() {
        qInfo() << "VotingKiosk: Close button clicked. Resetting kiosk and returning to dashboard.";
        resetKiosk(); // Reset kiosk state on close
        auto* mw = window(); // Get the parent QMainWindow
        if (mw) {
            mw->showNormal(); // Exit fullscreen
            QMetaObject::invokeMethod(mw, "switchToView", Qt::QueuedConnection, Q_ARG(QString, "dashboard"));
        }
    });
    headerLayout->addWidget(closeBtn);

    layout->addWidget(header);

    // --- Pages Stack ---
    m_pages = new QStackedWidget(this);
    m_pages->setStyleSheet("background-color: #1a1a2e;");
    m_pages->addWidget(createWaitingPage());        // Index 0
    m_pages->addWidget(createScanIdPage());         // Index 1
    m_pages->addWidget(createVerifyPhotoPage());    // Index 2
    m_pages->addWidget(createChooseCandidatePage());// Index 3
    m_pages->addWidget(createConfirmVotePage());    // Index 4
    m_pages->addWidget(createSuccessPage());        // Index 5

    layout->addWidget(m_pages, 1);
    qDebug() << "VotingKiosk: UI setup complete.";
}

/**
 * @brief Starts the kiosk process, resetting its state and moving to the first interactive step.
 */
void VotingKiosk::start() {
    qInfo() << "VotingKiosk: Starting kiosk.";
    resetKiosk(); // Ensure kiosk is reset before starting
    m_currentStep = 1; // Start at scan ID page
    m_pages->setCurrentIndex(1);
    updateVotingState(); // Re-evaluate state in case voting has already started
}

/**
 * @brief Periodically updates the voting state based on the active election.
 * This ensures the kiosk reacts to election start/end/pause events.
 */
void VotingKiosk::updateVotingState() {
    qDebug() << "VotingKiosk: Updating voting state...";
    auto* storage = Core::SystemManager::instance().storage();
    if (!storage || !storage->isConnected()) {
        qWarning() << "VotingKiosk: Storage not available. Cannot update voting state.";
        // Keep on waiting page with appropriate message
        m_pages->setCurrentIndex(0);
        auto *label = m_pages->widget(0)->findChild<QLabel*>("statusMsg");
        if (label) label->setText("System storage is not available.\nPlease contact an administrator.");
        return;
    }

    m_activeElection = Election::ElectionManager::instance().getActiveElection();

    if (m_activeElection && m_activeElection->state == Core::VotingState::Voting) {
        if (m_pages->currentIndex() == 0) { // If currently on waiting page, transition to scan ID
            qInfo() << "VotingKiosk: Election" << m_activeElection->title << "is now active. Moving to scan ID.";
            m_currentStep = 1;
            m_pages->setCurrentIndex(1);
            // loadCandidates(); // Candidates are loaded when createChooseCandidatePage is shown or nextStep is called
        }
    } else {
        // If voting is not in progress, always show the waiting page
        if (m_pages->currentIndex() != 0) {
            qInfo() << "VotingKiosk: Voting is not active. Returning to waiting page.";
            resetKiosk(); // Reset to waiting page
        }
        auto *label = m_pages->widget(0)->findChild<QLabel*>("statusMsg");
        if (label) {
            if (!m_activeElection) {
                label->setText("No election is currently configured or active.\nPlease contact an administrator.");
            } else if (m_activeElection->state == Core::VotingState::Idle) {
                label->setText("Voting has not started yet for '" + m_activeElection->title + "'.\nPlease wait for the session to begin.");
            } else if (m_activeElection->state == Core::VotingState::Ended) {
                label->setText("Voting for '" + m_activeElection->title + "' has ended.\nResults will be announced soon.");
            } else if (m_activeElection->state == Core::VotingState::Paused) {
                label->setText("Voting for '" + m_activeElection->title + "' is currently paused.\nPlease wait for it to resume.");
            }
        }
    }
}

/**
 * @brief Advances the kiosk to the next step/page.
 */
void VotingKiosk::nextStep() {
    if (m_currentStep < m_pages->count() - 1) {
        m_currentStep++;
        m_pages->setCurrentIndex(m_currentStep);
        qDebug() << "VotingKiosk: Moving to step" << m_currentStep;

        // Special handling for candidate page to ensure candidates are loaded
        if (m_currentStep == 3) {
            loadCandidates();
        }
        // Special handling for confirm vote page to update candidate name
        if (m_currentStep == 4 && m_candidateNameLabel && m_selectedCandidate) {
            m_candidateNameLabel->setText(m_selectedCandidate->name);
        }
    } else {
        qWarning() << "VotingKiosk: Attempted to go past last step.";
    }
}

/**
 * @brief Moves the kiosk to the previous step/page.
 */
void VotingKiosk::prevStep() {
    if (m_currentStep > 0) {
        m_currentStep--;
        m_pages->setCurrentIndex(m_currentStep);
        qDebug() << "VotingKiosk: Moving to previous step" << m_currentStep;
    } else {
        qWarning() << "VotingKiosk: Attempted to go before first step.";
    }
}

/**
 * @brief Resets the kiosk to its initial waiting state, clearing all session data.
 */
void VotingKiosk::resetKiosk() {
    qInfo() << "VotingKiosk: Resetting kiosk state.";
    m_currentStep = 0;
    m_currentVoter.reset();
    m_selectedCandidate.reset();
    m_availableCandidates.clear();
    m_activeElection.reset();
    if (m_idInputEdit) m_idInputEdit->clear(); // Clear ID input field
    m_pages->setCurrentIndex(0); // Go back to waiting page
    updateVotingState(); // Re-evaluate state in case voting has already started
}

/**
 * @brief Processes the entered admission number or scanned ID.
 * @param admissionNumber The admission number or ID scanned.
 */
void VotingKiosk::processScanId(const QString& admissionNumber) {
    qInfo() << "VotingKiosk: Processing scanned ID:" << admissionNumber;
    if (admissionNumber.isEmpty()) {
        ToastNotification::show(this, "Please enter an admission number.", ToastNotification::Warning);
        return;
    }

    auto* storage = Core::SystemManager::instance().storage();
    if (!storage || !storage->isConnected()) {
        ToastNotification::show(this, "System storage not available. Please contact an administrator.", ToastNotification::Error);
        qCritical() << "VotingKiosk: Storage not available during ID scan.";
        return;
    }

    if (!m_activeElection || m_activeElection->state != Core::VotingState::Voting) {
        ToastNotification::show(this, "No active election to vote in.", ToastNotification::Warning);
        qWarning() << "VotingKiosk: No active election during ID scan.";
        resetKiosk();
        return;
    }

    // --- Test Mode Handling ---
    if (m_testMode) {
        auto& testStorage = Storage::TestAdmissionStorage::instance();
        if (testStorage.hasVoted(admissionNumber)) {
            ToastNotification::show(this, "This admission number has already voted in test mode.", ToastNotification::Warning);
            qWarning() << "VotingKiosk: Duplicate test vote attempt for:" << admissionNumber;
            if (m_idInputEdit) m_idInputEdit->clear();
            return;
        }

        // Create a mock student for test mode
        Core::Student student;
        student.id = Core::IdGenerator::generateId();
        student.name = "Test Student (" + admissionNumber + ")";
        student.admissionNumber = admissionNumber;
        student.hasVoted = false;
        student.isVerified = true;

        m_currentVoter = student;
        ToastNotification::show(this, "Test student verified: " + student.name, ToastNotification::Success);
        qInfo() << "VotingKiosk: Test student" << student.name << "verified.";
        nextStep(); // Move to next step (Verify Photo or Choose Candidate)
        return;
    }

    // --- Production Mode Handling ---
    std::optional<Core::Student> studentOpt = storage->getStudentByAdmission(admissionNumber); // Assuming admission number is the primary ID for kiosk

    if (!studentOpt) {
        ToastNotification::show(this, "Invalid admission number or student not found.", ToastNotification::Error);
        qWarning() << "VotingKiosk: Student not found for admission number:" << admissionNumber;
        if (m_idInputEdit) m_idInputEdit->clear();
        return;
    }
    Core::Student student = *studentOpt;

    // Check if student has already voted in this election
    if (Election::VoteManager::instance().hasStudentVoted(student.id, m_activeElection->id)) {
        ToastNotification::show(this, "You have already voted in this election.", ToastNotification::Warning);
        qWarning() << "VotingKiosk: Student" << student.id << "already voted in election" << m_activeElection->id;
        if (m_idInputEdit) m_idInputEdit->clear();
        return;
    }

    // Check if student is eligible for this election (e.g., by class, department)
    // This logic should ideally be in ElectionManager or a dedicated eligibility service
    bool isEligible = true;
    if (!m_activeElection->eligibleClasses.isEmpty() && !m_activeElection->eligibleClasses.contains(student.className)) {
        isEligible = false;
    }
    // Add department check if student struct has department field
    // if (!m_activeElection->eligibleDepartments.isEmpty() && !m_activeElection->eligibleDepartments.contains(student.department)) {
    //     isEligible = false;
    // }

    if (!isEligible) {
        ToastNotification::show(this, "You are not eligible to vote in this election.", ToastNotification::Warning);
        qWarning() << "VotingKiosk: Student" << student.id << "not eligible for election" << m_activeElection->id;
        if (m_idInputEdit) m_idInputEdit->clear();
        return;
    }


    m_currentVoter = student;
    ToastNotification::show(this, "Student verified: " + student.name, ToastNotification::Success);
    qInfo() << "VotingKiosk: Student" << student.name << "(" << student.id << ") verified.";
    nextStep(); // Move to next step (Verify Photo or Choose Candidate)
}

/**
 * @brief Loads candidates for the active election and populates the candidate grid.
 */
void VotingKiosk::loadCandidates() {
    m_availableCandidates.clear();
    if (!m_activeElection) {
        qWarning() << "VotingKiosk: No active election to load candidates for.";
        ToastNotification::show(this, "No active election to load candidates.", ToastNotification::Error);
        return;
    }

    m_availableCandidates = Election::ElectionManager::instance().getCandidates(m_activeElection->id);
    qInfo() << "VotingKiosk: Loaded" << m_availableCandidates.size() << "candidates for election" << m_activeElection->id;

    // Clear existing candidate cards from the grid layout
    if (m_candidatesGrid) {
        QLayoutItem *item;
        while((item = m_candidatesGrid->takeAt(0)) != nullptr) {
            delete item->widget();
            delete item;
        }

        // Repopulate the grid with new candidates
        int row = 0;
        int col = 0;
        for (const auto& candidate : m_availableCandidates) {
            auto *card = new QFrame(); // Parent is the content widget of the scroll area
            card->setCursor(Qt::PointingHandCursor);
            card->setStyleSheet(R"(
                QFrame {
                    background-color: #25253a; border: 2px solid #3d3d5c; border-radius: 16px;
                }
                QFrame:hover {
                    border-color: #0078d4; background-color: rgba(0, 120, 212, 0.08);
                }
            )");

            auto *shadow = new QGraphicsDropShadowEffect(card);
            shadow->setBlurRadius(20);
            shadow->setColor(QColor(0, 0, 0, 40));
            shadow->setOffset(0, 4);
            card->setGraphicsEffect(shadow);

            auto *vbox = new QVBoxLayout(card);
            vbox->setContentsMargins(16, 16, 16, 16);
            vbox->setSpacing(8);

            // Candidate Photo
            auto* photoLabel = new QLabel(card);
            photoLabel->setFixedSize(120, 120);
            photoLabel->setAlignment(Qt::AlignCenter);
            photoLabel->setStyleSheet("background-color: #1a1a2e; border-radius: 60px;");
            if (!candidate.photoData.isEmpty()) {
                QPixmap pixmap;
                if (pixmap.loadFromData(candidate.photoData)) {
                    photoLabel->setPixmap(pixmap.scaled(120, 120, Qt::KeepAspectRatio, Qt::SmoothTransformation));
                } else {
                    photoLabel->setText("👤"); // Fallback icon
                    photoLabel->setStyleSheet("font-size: 48px; color: #9a9ab0; background-color: #1a1a2e; border-radius: 60px;");
                    qWarning() << "VotingKiosk: Failed to load photo for candidate" << candidate.name;
                }
            } else {
                photoLabel->setText("👤"); // Fallback icon
                photoLabel->setStyleSheet("font-size: 48px; color: #9a9ab0; background-color: #1a1a2e; border-radius: 60px;");
            }
            vbox->addWidget(photoLabel, 0, Qt::AlignCenter);


            auto *name = new QLabel(candidate.name, card);
            name->setStyleSheet("font-size: 20px; font-weight: 700; color: #ffffff; background: transparent;");
            name->setAlignment(Qt::AlignCenter);
            vbox->addWidget(name);

            auto *party = new QLabel(candidate.party, card);
            party->setStyleSheet("font-size: 13px; color: #9a9ab0; background: transparent;");
            party->setAlignment(Qt::AlignCenter);
            vbox->addWidget(party);

            vbox->addStretch();

            auto *selectBtn = new QPushButton("Select", card);
            selectBtn->setStyleSheet("background-color: #0078d4; color: white; font-size: 16px; font-weight: 600; border-radius: 8px; padding: 10px;");
            selectBtn->setCursor(Qt::PointingHandCursor);
            vbox->addWidget(selectBtn);

            connect(selectBtn, &QPushButton::clicked, this, [this, candidate]() {
                m_selectedCandidate = candidate;
                qInfo() << "VotingKiosk: Candidate" << candidate.name << "selected.";
                nextStep(); // Move to confirm vote page
            });

            m_candidatesGrid->addWidget(card, row, col);
            col++;
            if (col >= 3) { // 3 candidates per row
                col = 0;
                row++;
            }
        }
        m_candidatesGrid->addItem(new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding), row + 1, 0, 1, 3); // Add stretch
    }
}

/**
 * @brief Confirms and casts the vote.
 */
void VotingKiosk::confirmVote() {
    qInfo() << "VotingKiosk: Confirming vote.";
    if (!m_currentVoter || !m_selectedCandidate || !m_activeElection) {
        ToastNotification::show(this, "Error: Missing voter, candidate, or election information. Please restart.", ToastNotification::Error);
        qCritical() << "VotingKiosk: Critical error: Missing voter, candidate, or election info during vote confirmation.";
        resetKiosk();
        return;
    }

    // --- Test Mode Handling ---
    if (m_testMode) {
        Storage::TestAdmissionStorage::instance().markAsVoted(m_currentVoter->admissionNumber);
        ToastNotification::show(this, "Test vote successfully recorded!", ToastNotification::Success);
        qInfo() << "VotingKiosk: Test vote recorded for" << m_currentVoter->admissionNumber;
        nextStep(); // Go to success page
        return;
    }

    // --- Production Mode Handling ---
    bool success = Election::VoteManager::instance().castVote(
        m_activeElection->id,
        m_currentVoter->id,
        m_selectedCandidate->id
    );

    if (success) {
        ToastNotification::show(this, "Vote successfully recorded!", ToastNotification::Success);
        qInfo() << "VotingKiosk: Vote recorded for student" << m_currentVoter->id << "for candidate" << m_selectedCandidate->id;
        nextStep(); // Go to success page
    } else {
        ToastNotification::show(this, "Failed to record vote. Please try again or contact an administrator.", ToastNotification::Error);
        qCritical() << "VotingKiosk: Failed to record vote for student" << m_currentVoter->id << "for candidate" << m_selectedCandidate->id;
        resetKiosk(); // Go back to start on failure
    }
}

/**
 * @brief Creates the waiting page for the kiosk.
 * @return The waiting page widget.
 */
QWidget* VotingKiosk::createWaitingPage() {
    auto *page = new QWidget(this);
    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(60, 60, 60, 60);
    layout->setAlignment(Qt::AlignCenter);

    layout->addStretch(2);

    auto *icon = new QLabel("🗳️", page);
    icon->setStyleSheet("font-size: 80px; background: transparent;");
    icon->setAlignment(Qt::AlignCenter);
    layout->addWidget(icon);

    auto *title = new QLabel("Welcome to Campus Ballot", page);
    title->setStyleSheet("font-size: 48px; font-weight: 700; color: #ffffff; background: transparent;");
    title->setAlignment(Qt::AlignCenter);
    layout->addWidget(title);

    auto *desc = new QLabel("Please wait for the election session to start.", page);
    desc->setObjectName("statusMsg"); // Used to update text dynamically
    desc->setStyleSheet("font-size: 24px; color: #9a9ab0; margin: 20px; background: transparent;");
    desc->setAlignment(Qt::AlignCenter);
    layout->addWidget(desc);

    layout->addStretch(3);
    return page;
}

/**
 * @brief Creates the scan ID page for the kiosk.
 * @return The scan ID page widget.
 */
QWidget* VotingKiosk::createScanIdPage() {
    auto *page = new QWidget(this);
    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(60, 30, 60, 30);
    layout->setSpacing(20);
    layout->setAlignment(Qt::AlignCenter);

    layout->addWidget(createStepIndicator(1, 4));

    auto *icon = new QLabel("🆔", page);
    icon->setStyleSheet("font-size: 64px; background: transparent;");
    icon->setAlignment(Qt::AlignCenter);
    layout->addWidget(icon);

    auto *title = new QLabel("Step 1: Scan ID Card", page);
    title->setStyleSheet("font-size: 36px; font-weight: 700; color: #ffffff; background: transparent;");
    title->setAlignment(Qt::AlignCenter);
    layout->addWidget(title);

    auto *desc = new QLabel("Please place your student ID card on the scanner\nor enter your admission number below.", page);
    desc->setStyleSheet("font-size: 18px; color: #9a9ab0; line-height: 1.5; background: transparent;");
    desc->setAlignment(Qt::AlignCenter);
    layout->addWidget(desc);

    m_idInputEdit = new QLineEdit(page);
    m_idInputEdit->setPlaceholderText("Enter admission number or scan ID...");
    m_idInputEdit->setFixedHeight(50);
    m_idInputEdit->setStyleSheet("font-size: 18px; text-align: center;");
    m_idInputEdit->setAlignment(Qt::AlignCenter);
    layout->addWidget(m_idInputEdit, 0, Qt::AlignCenter);

    // Test mode checkbox
    m_testModeCheck = new QCheckBox("🧪 Test Mode (Local File Storage)", page);
    m_testModeCheck->setStyleSheet("font-size: 14px; color: #9a9ab0; background: transparent;");
    m_testModeCheck->setToolTip("Enable test mode to use local file storage for admission numbers instead of database");
    connect(m_testModeCheck, &QCheckBox::toggled, this, &VotingKiosk::setTestMode);
    layout->addWidget(m_testModeCheck, 0, Qt::AlignCenter);

    layout->addSpacing(20);

    auto *btn = new QPushButton("Continue →", page);
    btn->setFixedHeight(60);
    btn->setStyleSheet("font-size: 22px; font-weight: 600; border-radius: 12px;");
    layout->addWidget(btn, 0, Qt::AlignCenter);

    connect(btn, &QPushButton::clicked, this, [this]() {
        processScanId(m_idInputEdit->text().trimmed());
    });
    connect(m_idInputEdit, &QLineEdit::returnPressed, this, [this]() {
        processScanId(m_idInputEdit->text().trimmed());
    });

    layout->addStretch();
    return page;
}

/**
 * @brief Creates the verify photo page for the kiosk.
 * @return The verify photo page widget.
 */
QWidget* VotingKiosk::createVerifyPhotoPage() {
    auto *page = new QWidget(this);
    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(60, 30, 60, 30);
    layout->setSpacing(20);
    layout->setAlignment(Qt::AlignCenter);

    layout->addWidget(createStepIndicator(2, 4));

    auto *icon = new QLabel("📸", page);
    icon->setStyleSheet("font-size: 64px; background: transparent;");
    icon->setAlignment(Qt::AlignCenter);
    layout->addWidget(icon);

    auto *title = new QLabel("Step 2: Verify Identity", page);
    title->setStyleSheet("font-size: 36px; font-weight: 700; color: #ffffff; background: transparent;");
    title->setAlignment(Qt::AlignCenter);
    layout->addWidget(title);

    auto *desc = new QLabel("Look at the camera to verify your identity.", page);
    desc->setStyleSheet("font-size: 18px; color: #9a9ab0; background: transparent;");
    desc->setAlignment(Qt::AlignCenter);
    layout->addWidget(desc);

    // Camera placeholder (future: integrate actual camera feed)
    auto *cameraFrame = new QFrame(page);
    cameraFrame->setFixedSize(320, 240);
    cameraFrame->setStyleSheet("background-color: #000000; border: 2px solid #0078d4; border-radius: 12px;");
    auto *camLayout = new QVBoxLayout(cameraFrame);
    auto *camIcon = new QLabel("📷", cameraFrame);
    camIcon->setStyleSheet("font-size: 48px; color: #555; background: transparent;");
    camIcon->setAlignment(Qt::AlignCenter);
    camLayout->addWidget(camIcon);
    layout->addWidget(cameraFrame, 0, Qt::AlignCenter);

    layout->addSpacing(10);

    auto *btnLayout = new QHBoxLayout();
    auto *cancelBtn = new QPushButton("← Back", page);
    cancelBtn->setObjectName("ghostButton");
    cancelBtn->setFixedHeight(60);
    cancelBtn->setStyleSheet("font-size: 18px; border-radius: 12px;");
    auto *verifyBtn = new QPushButton("✓ Verify Identity", page);
    verifyBtn->setFixedHeight(60);
    verifyBtn->setStyleSheet("background-color: #2e7d32; font-size: 20px; font-weight: 600; border-radius: 12px; color: white;");

    btnLayout->addStretch();
    btnLayout->addWidget(cancelBtn);
    btnLayout->addWidget(verifyBtn);
    btnLayout->addStretch();
    layout->addLayout(btnLayout);

    connect(verifyBtn, &QPushButton::clicked, this, &VotingKiosk::nextStep);
    connect(cancelBtn, &QPushButton::clicked, this, &VotingKiosk::prevStep);

    layout->addStretch();
    return page;
}

/**
 * @brief Creates the choose candidate page for the kiosk.
 * @return The choose candidate page widget.
 */
QWidget* VotingKiosk::createChooseCandidatePage() {
    auto *page = new QWidget(this);
    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(40, 20, 40, 20);
    layout->setSpacing(16);
    layout->setAlignment(Qt::AlignCenter);

    layout->addWidget(createStepIndicator(3, 4));

    auto *title = new QLabel("Step 3: Choose Your Candidate", page);
    title->setStyleSheet("font-size: 32px; font-weight: 700; color: #ffffff; background: transparent;");
    title->setAlignment(Qt::AlignCenter);
    layout->addWidget(title);

    auto *scroll = new QScrollArea(page);
    scroll->setWidgetResizable(true);
    scroll->setStyleSheet("QScrollArea { background: transparent; border: none; }");

    auto *content = new QWidget();
    content->setStyleSheet("background: transparent;");
    m_candidatesGrid = new QGridLayout(content); // Assign to member variable
    m_candidatesGrid->setSpacing(16);
    m_candidatesGrid->setAlignment(Qt::AlignTop | Qt::AlignHCenter); // Align candidates to top and center

    scroll->setWidget(content);
    layout->addWidget(scroll, 1);

    return page;
}

/**
 * @brief Creates the confirm vote page for the kiosk.
 * @return The confirm vote page widget.
 */
QWidget* VotingKiosk::createConfirmVotePage() {
    auto *page = new QWidget(this);
    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(60, 30, 60, 30);
    layout->setSpacing(20);
    layout->setAlignment(Qt::AlignCenter);

    layout->addWidget(createStepIndicator(4, 4));

    auto *icon = new QLabel("✅", page);
    icon->setStyleSheet("font-size: 64px; background: transparent;");
    icon->setAlignment(Qt::AlignCenter);
    layout->addWidget(icon);

    auto *title = new QLabel("Step 4: Confirm Your Vote", page);
    title->setStyleSheet("font-size: 36px; font-weight: 700; color: #ffffff; background: transparent;");
    title->setAlignment(Qt::AlignCenter);
    layout->addWidget(title);

    auto *warning = new QLabel("You are about to vote for:", page);
    warning->setStyleSheet("font-size: 20px; color: #9a9ab0; background: transparent;");
    warning->setAlignment(Qt::AlignCenter);
    layout->addWidget(warning);

    m_candidateNameLabel = new QLabel("Candidate Name", page); // Assign to member variable
    m_candidateNameLabel->setStyleSheet("font-size: 40px; font-weight: 700; color: #0078d4; background: transparent;");
    m_candidateNameLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(m_candidateNameLabel);

    auto *notice = new QLabel("This action cannot be undone.\nYour vote will be encrypted and recorded securely.", page);
    notice->setStyleSheet("font-size: 16px; color: #ffb300; line-height: 1.5; background: transparent;");
    notice->setAlignment(Qt::AlignCenter);
    layout->addWidget(notice);

    layout->addSpacing(20);

    auto *btnLayout = new QHBoxLayout();
    auto *cancelBtn = new QPushButton("← Cancel", page);
    cancelBtn->setObjectName("ghostButton");
    cancelBtn->setFixedHeight(64);
    cancelBtn->setStyleSheet("font-size: 20px; border-radius: 12px;");
    auto *confirmBtn = new QPushButton("✓ Confirm Vote", page);
    confirmBtn->setFixedHeight(64);
    confirmBtn->setStyleSheet("background-color: #2e7d32; color: white; font-size: 22px; font-weight: 700; border-radius: 12px;");

    btnLayout->addStretch();
    btnLayout->addWidget(cancelBtn);
    btnLayout->addSpacing(20);
    btnLayout->addWidget(confirmBtn);
    btnLayout->addStretch();
    layout->addLayout(btnLayout);

    connect(confirmBtn, &QPushButton::clicked, this, &VotingKiosk::confirmVote);
    connect(cancelBtn, &QPushButton::clicked, this, &VotingKiosk::resetKiosk); // Reset kiosk on cancel

    layout->addStretch();
    return page;
}

/**
 * @brief Creates the success page for the kiosk.
 * @return The success page widget.
 */
QWidget* VotingKiosk::createSuccessPage() {
    auto *page = new QWidget(this);
    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(60, 60, 60, 60);
    layout->setAlignment(Qt::AlignCenter);

    layout->addStretch(2);

    auto *icon = new QLabel("🎉", page);
    icon->setStyleSheet("font-size: 80px; background: transparent;");
    icon->setAlignment(Qt::AlignCenter);
    layout->addWidget(icon);

    auto *title = new QLabel("Vote Successfully Recorded!", page);
    title->setStyleSheet("font-size: 48px; font-weight: 700; color: #4caf50; background: transparent;");
    title->setAlignment(Qt::AlignCenter);
    layout->addWidget(title);

    auto *desc = new QLabel("Thank you for participating in the election.\nYour voice matters!", page);
    desc->setStyleSheet("font-size: 22px; color: #9a9ab0; margin: 20px; line-height: 1.5; background: transparent;");
    desc->setAlignment(Qt::AlignCenter);
    layout->addWidget(desc);

    layout->addStretch(1);

    auto *btn = new QPushButton("Return to Dashboard", page);
    btn->setFixedHeight(64);
    btn->setStyleSheet("font-size: 20px; font-weight: 600; border-radius: 12px;");
    layout->addWidget(btn, 0, Qt::AlignCenter);
    connect(btn, &QPushButton::clicked, this, [this]() {
        qInfo() << "VotingKiosk: Return to Dashboard button clicked.";
        resetKiosk(); // Reset kiosk state
        auto* mw = window();
        if (mw) {
            mw->showNormal();
            QMetaObject::invokeMethod(mw, "switchToView", Qt::QueuedConnection, Q_ARG(QString, "dashboard"));
        }
    });

    layout->addStretch(2);
    return page;
}

/**
 * @brief Creates a step indicator widget.
 * @param current The current step number.
 * @param total The total number of steps.
 * @return The step indicator widget.
 */
QWidget* VotingKiosk::createStepIndicator(int current, int total) {
    auto *widget = new QWidget(this);
    auto *layout = new QHBoxLayout(widget);
    layout->setAlignment(Qt::AlignCenter);
    layout->setSpacing(8);
    layout->setContentsMargins(0,0,0,0);

    for (int i = 1; i <= total; ++i) {
        // Dot indicator
        auto *dot = new QFrame(widget);
        dot->setFixedSize(12, 12);
        dot->setStyleSheet(QString(
            "background-color: %1; border-radius: 6px;"
        ).arg(i <= current ? "#0078d4" : "#3d3d5c"));
        layout->addWidget(dot);

        // Line separator (except after the last dot)
        if (i < total) {
            auto *line = new QFrame(widget);
            line->setFixedSize(30, 2);
            line->setStyleSheet(QString("background-color: %1;").arg(i < current ? "#0078d4" : "#3d3d5c"));
            layout->addWidget(line);
        }
    }

    // Current step text label
    auto *label = new QLabel(QString("Step %1 of %2").arg(current).arg(total), widget);
    label->setStyleSheet("font-size: 14px; color: #0078d4; font-weight: 600; margin-left: 8px; background: transparent;");
    layout->addWidget(label);

    return widget;
}

} // namespace Ballot::UI