#pragma once

#include <QWidget>
#include <QStackedWidget>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QGridLayout>
#include <QScrollArea>
#include <QTimer>
#include <QCheckBox>
#include <optional>
#include "src/core/Models.h"
#include "src/modules/election/ElectionManager.h"
#include "src/modules/storage/TestAdmissionStorage.h"
#include "src/core/config/KioskConfiguration.h"
#include "src/ui/components/KioskPage.h"
#include "src/ui/components/CandidateCard.h"
#include "src/ui/components/StepIndicator.h"

namespace Ballot::UI {

class VotingKiosk : public QWidget {
    Q_OBJECT
public:
    explicit VotingKiosk(QWidget *parent = nullptr);
    ~VotingKiosk() override;
    
    void start();
    void updateVotingState();
    void setTestMode(bool enabled);
    void applyConfiguration(const Core::KioskConfiguration& config);
    void resetKiosk();

private:
    enum class KioskState {
        Waiting,
        ScanId,
        VerifyPhoto,
        ChooseCandidate,
        ConfirmVote,
        Success,
        Error
    };
    
    void setupUi();
    void createPages();
    void createHeader();
    void connectSignals();
    void applyTheme();
    void applyConfiguration();
    QWidget* createWaitingPage();
    QWidget* createScanIdPage();
    QWidget* createVerifyPhotoPage();
    QWidget* createChooseCandidatePage();
    QWidget* createConfirmVotePage();
    QWidget* createSuccessPage();
    QWidget* createStepIndicator(int current, int total);

    // State management
    void nextStep();
    void prevStep();
    void transitionToState(KioskState newState);
    void clearCandidateGrid();
    
    // Page handlers
    void onWaitingPageShown();
    void onScanIdPageShown();
    void onVerifyPhotoPageShown();
    void onChooseCandidatePageShown();
    void onConfirmVotePageShown();
    void onSuccessPageShown();
    
    // Kiosk logic
    void processScanId(const QString& admissionNumber);
    void loadCandidates();
    void onCandidatesLoaded(const QList<Core::Candidate>& candidates);
    void onCandidateSelected(const Core::Candidate& candidate);
    void confirmVote();
    void onVoteCast(bool success, const QString& errorMessage = QString());
    
    // UI helpers
    void showLoading(bool show, const QString& message = QString());
    void showError(const QString& message);
    void updateStepIndicator(int step);
    void updateCandidateNameLabel();
    
    // Accessibility
    void setupAccessibility();
    void announceForScreenReader(const QString& message);
    
    // Audit logging
    void logAuditEvent(const QString& action, const QString& details);
    
    // Timer for session timeout
    void startSessionTimer();
    void stopSessionTimer();
    void onSessionTimeout();

    // UI Components
    QStackedWidget* m_pages = nullptr;
    QWidget* m_header = nullptr;
    QLabel* m_headerTitle = nullptr;
    QPushButton* m_closeButton = nullptr;
    
    // Pages (using KioskPage subclasses)
    KioskPage* m_waitingPage = nullptr;
    KioskPage* m_scanIdPage = nullptr;
    KioskPage* m_verifyPhotoPage = nullptr;
    KioskPage* m_chooseCandidatePage = nullptr;
    KioskPage* m_confirmVotePage = nullptr;
    KioskPage* m_successPage = nullptr;
    
    // Specific UI elements
    QLineEdit* m_idInputEdit = nullptr;
    QCheckBox* m_testModeCheck = nullptr;
    QLabel* m_candidateNameLabel = nullptr;
    QLabel* m_statusLabel = nullptr;
    StepIndicator* m_stepIndicator = nullptr;
    QWidget* m_candidateGridContainer = nullptr;
    QGridLayout* m_candidatesGrid = nullptr;
    QScrollArea* m_candidateScrollArea = nullptr;
    QPushButton* m_loadingOverlay = nullptr;
    
    // State
    KioskState m_currentState = KioskState::Waiting;
    int m_currentStep = 0;
    QTimer* m_stateTimer = nullptr;
    QTimer* m_sessionTimer = nullptr;
    
    // Data
    std::optional<Core::Student> m_currentVoter;
    std::optional<Core::Candidate> m_selectedCandidate;
    QList<Core::Candidate> m_availableCandidates;
    std::optional<Core::Election> m_activeElection;
    bool m_testMode = false;
    bool m_isLoading = false;
    Core::KioskConfiguration m_config;
    
    // Candidate cards for reuse
    QList<CandidateCard*> m_candidateCards;
    
    // Prevent copying
    VotingKiosk(const VotingKiosk&) = delete;
    VotingKiosk& operator=(const VotingKiosk&) = delete;
};

} // namespace Ballot::UI