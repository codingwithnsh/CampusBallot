#pragma once

#include <QWidget>
#include <QStackedWidget>
#include <QLabel>
#include <QPushButton>
#include <QTimer>

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

    QWidget* createWaitingPage();
    QWidget* createScanIdPage();
    QWidget* createVerifyPhotoPage();
    QWidget* createChooseCandidatePage();
    QWidget* createConfirmVotePage();
    QWidget* createSuccessPage();
    QWidget* createStepIndicator(int current, int total);

    QStackedWidget *m_pages;
    int m_currentStep = 0;
    QString m_selectedCandidate;
    QTimer* m_stateTimer;
};

} // namespace Ballot::UI
