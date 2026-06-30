#pragma once

#include <QWidget>
#include <QStackedWidget>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QGridLayout>
#include <QTimer>
#include <optional> // For std::optional
#include "src/core/Models.h" // For Core::Student, Core::Candidate
#include "src/modules/election/ElectionManager.h" // For ElectionManager

namespace Ballot::UI {

class VotingKiosk : public QWidget {
    Q_OBJECT
public:
    explicit VotingKiosk(QWidget *parent = nullptr);
    void start();
    void updateVotingState();

private:
    void setupUi();
    void nextStep();
    void prevStep(); // Added for back navigation
    void resetKiosk(); // Added to reset state after voting or cancellation

    // Page creation methods
    QWidget* createWaitingPage();
    QWidget* createScanIdPage();
    QWidget* createVerifyPhotoPage();
    QWidget* createChooseCandidatePage();
    QWidget* createConfirmVotePage();
    QWidget* createSuccessPage();
    QWidget* createStepIndicator(int current, int total);

    // Kiosk logic methods
    void processScanId(const QString& admissionNumber);
    void loadCandidates();
    void confirmVote();

    QStackedWidget *m_pages;
    int m_currentStep = 0;
    QTimer* m_stateTimer;

    // Data for current voting session
    std::optional<Core::Student> m_currentVoter;
    std::optional<Core::Candidate> m_selectedCandidate;
    QList<Core::Candidate> m_availableCandidates;
    std::optional<Core::Election> m_activeElection;

    // UI elements that need to be updated dynamically
    QLabel* m_candidateNameLabel = nullptr; // For confirm vote page
    QLineEdit* m_idInputEdit = nullptr; // For scan ID page
    QGridLayout* m_candidatesGrid = nullptr; // For choose candidate page
};

} // namespace Ballot::UI
