#pragma once

#include <QWidget>
#include <QStackedWidget>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QGridLayout>
#include <QTimer>
#include <QCheckBox>
#include <optional>
#include "src/core/Models.h"
#include "src/modules/election/ElectionManager.h"
#include "src/modules/storage/TestAdmissionStorage.h"

namespace Ballot::UI {

class VotingKiosk : public QWidget {
    Q_OBJECT
public:
    explicit VotingKiosk(QWidget *parent = nullptr);
    void start();
    void updateVotingState();
    void setTestMode(bool enabled);

private:
    void setupUi();
    void nextStep();
    void prevStep();
    void resetKiosk();

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

    // UI elements
    QLabel* m_candidateNameLabel = nullptr;
    QLineEdit* m_idInputEdit = nullptr;
    QGridLayout* m_candidatesGrid = nullptr;
    QCheckBox* m_testModeCheck = nullptr;
    bool m_testMode = false;
};

} // namespace Ballot::UI
