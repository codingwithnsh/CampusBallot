#include "VotingKiosk.h"
#include "src/core/SystemManager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QScrollArea>
#include <QLineEdit>
#include <QFrame>
#include <QGraphicsDropShadowEffect>
#include <QApplication>

namespace Ballot::UI {

VotingKiosk::VotingKiosk(QWidget *parent) : QWidget(parent) {
    setupUi();

    m_stateTimer = new QTimer(this);
    connect(m_stateTimer, &QTimer::timeout, this, &VotingKiosk::updateVotingState);
    m_stateTimer->start(2000);

    updateVotingState();
}

void VotingKiosk::setupUi() {
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    // Header bar
    auto *header = new QFrame(this);
    header->setFixedHeight(60);
    header->setStyleSheet("background-color: #1a1a2e; border-bottom: 1px solid #2d2d44;");
    auto *headerLayout = new QHBoxLayout(header);

    auto *title = new QLabel("🗳️  Campus Ballot - Voting Kiosk", header);
    title->setStyleSheet("font-size: 18px; font-weight: 600; color: #e0e0e0; background: transparent;");
    headerLayout->addWidget(title);

    headerLayout->addStretch();

    auto *closeBtn = new QPushButton("✕", header);
    closeBtn->setObjectName("iconButton");
    closeBtn->setStyleSheet("font-size: 20px; color: #9a9ab0; background: transparent; border: none; padding: 8px;");
    closeBtn->setCursor(Qt::PointingHandCursor);
    connect(closeBtn, &QPushButton::clicked, this, [this]() {
        auto* mw = window();
        if (mw) {
            mw->showNormal();
            QMetaObject::invokeMethod(mw, "switchToView", Qt::QueuedConnection, Q_ARG(QString, "dashboard"));
        }
    });
    headerLayout->addWidget(closeBtn);

    layout->addWidget(header);

    // Pages
    m_pages = new QStackedWidget(this);
    m_pages->setStyleSheet("background-color: #1a1a2e;");
    m_pages->addWidget(createWaitingPage());
    m_pages->addWidget(createScanIdPage());
    m_pages->addWidget(createVerifyPhotoPage());
    m_pages->addWidget(createChooseCandidatePage());
    m_pages->addWidget(createConfirmVotePage());
    m_pages->addWidget(createSuccessPage());

    layout->addWidget(m_pages, 1);
}

void VotingKiosk::updateVotingState() {
    auto* storage = Core::SystemManager::instance().storage();
    if (!storage) return;

    auto settings = storage->getSystemSettings();
    if (!settings) return;

    if (settings->votingStatus == Core::VotingState::Voting) {
        if (m_pages->currentIndex() == 0) {
            m_currentStep = 1;
            m_pages->setCurrentIndex(1);
        }
    } else {
        m_currentStep = 0;
        m_pages->setCurrentIndex(0);
        auto *label = m_pages->widget(0)->findChild<QLabel*>("statusMsg");
        if (label) {
            if (settings->votingStatus == Core::VotingState::Idle)
                label->setText("Voting has not started yet.\nPlease wait for the session to begin.");
            else
                label->setText("Voting has ended.\nResults will be announced soon.");
        }
    }
}

void VotingKiosk::nextStep() {
    if (m_currentStep < m_pages->count() - 1) {
        m_currentStep++;
        m_pages->setCurrentIndex(m_currentStep);
    }
}

QWidget* VotingKiosk::createWaitingPage() {
    auto *page = new QWidget(this);
    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(60, 60, 60, 60);

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
    desc->setObjectName("statusMsg");
    desc->setStyleSheet("font-size: 24px; color: #9a9ab0; margin: 20px; background: transparent;");
    desc->setAlignment(Qt::AlignCenter);
    layout->addWidget(desc);

    layout->addStretch(3);
    return page;
}

QWidget* VotingKiosk::createScanIdPage() {
    auto *page = new QWidget(this);
    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(60, 30, 60, 30);
    layout->setSpacing(20);

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

    auto *idInput = new QLineEdit(page);
    idInput->setPlaceholderText("Enter admission number or scan ID...");
    idInput->setFixedWidth(400);
    idInput->setFixedHeight(50);
    idInput->setStyleSheet("font-size: 18px; text-align: center;");
    layout->addWidget(idInput, 0, Qt::AlignCenter);

    layout->addSpacing(20);

    auto *btn = new QPushButton("Continue →", page);
    btn->setFixedSize(300, 60);
    btn->setStyleSheet("font-size: 22px; font-weight: 600; border-radius: 12px;");
    layout->addWidget(btn, 0, Qt::AlignCenter);

    connect(btn, &QPushButton::clicked, this, &VotingKiosk::nextStep);

    layout->addStretch();
    return page;
}

QWidget* VotingKiosk::createVerifyPhotoPage() {
    auto *page = new QWidget(this);
    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(60, 30, 60, 30);
    layout->setSpacing(20);

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

    // Camera placeholder
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
    cancelBtn->setFixedSize(200, 60);
    cancelBtn->setStyleSheet("font-size: 18px; border-radius: 12px;");
    auto *verifyBtn = new QPushButton("✓ Verify Identity", page);
    verifyBtn->setFixedSize(300, 60);
    verifyBtn->setStyleSheet("background-color: #2e7d32; font-size: 20px; font-weight: 600; border-radius: 12px; color: white;");

    btnLayout->addStretch();
    btnLayout->addWidget(cancelBtn);
    btnLayout->addWidget(verifyBtn);
    btnLayout->addStretch();
    layout->addLayout(btnLayout);

    connect(verifyBtn, &QPushButton::clicked, this, &VotingKiosk::nextStep);

    layout->addStretch();
    return page;
}

QWidget* VotingKiosk::createChooseCandidatePage() {
    auto *page = new QWidget(this);
    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(40, 20, 40, 20);
    layout->setSpacing(16);

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
    auto *grid = new QGridLayout(content);
    grid->setSpacing(16);

    QStringList names = {"Alice Johnson", "Bob Smith", "Carol Davis", "David Wilson", "Eve Martinez", "Frank Brown"};
    QStringList parties = {"Progressive Party", "Unity Coalition", "Forward Alliance", "People's Choice", "Reform Front", "Action Party"};

    for (int i = 0; i < 6; ++i) {
        auto *card = new QFrame(content);
        card->setFixedSize(280, 340);
        card->setCursor(Qt::PointingHandCursor);
        card->setStyleSheet(R"(
            QFrame {
                background-color: #25253a;
                border: 2px solid #3d3d5c;
                border-radius: 16px;
            }
            QFrame:hover {
                border-color: #0078d4;
                background-color: rgba(0, 120, 212, 0.08);
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

        // Photo placeholder
        auto *photo = new QFrame(card);
        photo->setFixedSize(120, 120);
        photo->setStyleSheet(QString("background-color: %1; border-radius: 60px;")
            .arg(i == 0 ? "#0078d4" : i == 1 ? "#2e7d32" : i == 2 ? "#7b1fa2" : i == 3 ? "#f57c00" : i == 4 ? "#c62828" : "#00695c"));
        vbox->addWidget(photo, 0, Qt::AlignCenter);

        auto *name = new QLabel(names[i], card);
        name->setStyleSheet("font-size: 20px; font-weight: 700; color: #ffffff; background: transparent;");
        name->setAlignment(Qt::AlignCenter);
        vbox->addWidget(name);

        auto *party = new QLabel(parties[i], card);
        party->setStyleSheet("font-size: 13px; color: #9a9ab0; background: transparent;");
        party->setAlignment(Qt::AlignCenter);
        vbox->addWidget(party);

        vbox->addStretch();

        auto *voteBtn = new QPushButton("Select", card);
        voteBtn->setStyleSheet("background-color: #0078d4; color: white; font-size: 16px; font-weight: 600; border-radius: 8px; padding: 10px;");
        voteBtn->setCursor(Qt::PointingHandCursor);
        vbox->addWidget(voteBtn);

        connect(voteBtn, &QPushButton::clicked, this, [this, names, i]() {
            m_selectedCandidate = names[i];
            nextStep();
        });

        grid->addWidget(card, i / 3, i % 3);
    }

    scroll->setWidget(content);
    layout->addWidget(scroll, 1);

    return page;
}

QWidget* VotingKiosk::createConfirmVotePage() {
    auto *page = new QWidget(this);
    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(60, 30, 60, 30);
    layout->setSpacing(20);

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

    auto *candidateLabel = new QLabel(m_selectedCandidate.isEmpty() ? "Candidate Name" : m_selectedCandidate, page);
    candidateLabel->setStyleSheet("font-size: 40px; font-weight: 700; color: #0078d4; background: transparent;");
    candidateLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(candidateLabel);

    auto *notice = new QLabel("This action cannot be undone.\nYour vote will be encrypted and recorded securely.", page);
    notice->setStyleSheet("font-size: 16px; color: #ffb300; line-height: 1.5; background: transparent;");
    notice->setAlignment(Qt::AlignCenter);
    layout->addWidget(notice);

    layout->addSpacing(20);

    auto *btnLayout = new QHBoxLayout();
    auto *cancelBtn = new QPushButton("← Cancel", page);
    cancelBtn->setObjectName("ghostButton");
    cancelBtn->setFixedSize(250, 64);
    cancelBtn->setStyleSheet("font-size: 20px; border-radius: 12px;");
    auto *confirmBtn = new QPushButton("✓ Confirm Vote", page);
    confirmBtn->setStyleSheet("background-color: #2e7d32; color: white; font-size: 22px; font-weight: 700; border-radius: 12px;");
    confirmBtn->setFixedSize(300, 64);

    btnLayout->addStretch();
    btnLayout->addWidget(cancelBtn);
    btnLayout->addSpacing(20);
    btnLayout->addWidget(confirmBtn);
    btnLayout->addStretch();
    layout->addLayout(btnLayout);

    connect(confirmBtn, &QPushButton::clicked, this, &VotingKiosk::nextStep);
    connect(cancelBtn, &QPushButton::clicked, this, [this]() {
        m_currentStep = 1;
        m_pages->setCurrentIndex(1);
    });

    layout->addStretch();
    return page;
}

QWidget* VotingKiosk::createSuccessPage() {
    auto *page = new QWidget(this);
    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(60, 60, 60, 60);

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
    btn->setFixedSize(350, 64);
    btn->setStyleSheet("font-size: 20px; font-weight: 600; border-radius: 12px;");
    layout->addWidget(btn, 0, Qt::AlignCenter);
    connect(btn, &QPushButton::clicked, this, [this]() {
        auto* mw = window();
        if (mw) {
            mw->showNormal();
            QMetaObject::invokeMethod(mw, "switchToView", Qt::QueuedConnection, Q_ARG(QString, "dashboard"));
        }
    });

    layout->addStretch(2);
    return page;
}

QWidget* VotingKiosk::createStepIndicator(int current, int total) {
    auto *widget = new QWidget(this);
    auto *layout = new QHBoxLayout(widget);
    layout->setAlignment(Qt::AlignCenter);
    layout->setSpacing(8);

    for (int i = 1; i <= total; ++i) {
        auto *step = new QFrame(widget);
        step->setFixedSize(i == current ? 32 : 12, 12);
        step->setStyleSheet(QString(
            "background-color: %1; border-radius: %2px;"
        ).arg(i <= current ? "#0078d4" : "#3d3d5c")
         .arg(i == current ? "16" : "6"));

        if (i == current) {
            auto *label = new QLabel(QString("Step %1 of %2").arg(current).arg(total), widget);
            label->setStyleSheet("font-size: 14px; color: #0078d4; font-weight: 600; margin-left: 8px; background: transparent;");
            layout->addWidget(label);
        }

        layout->addWidget(step);
    }

    return widget;
}

void VotingKiosk::start() {
    m_currentStep = 1;
    m_pages->setCurrentIndex(1);
}

} // namespace Ballot::UI
