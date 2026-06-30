#pragma once

#include <QDialog>
#include <QLineEdit>
#include <QTextEdit>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QImage>
#include <QFileDialog>
#include <QPixmap>
#include <QCheckBox>
#include "src/core/Models.h"

namespace Ballot::UI {

class CandidateFormDialog : public QDialog {
    Q_OBJECT
public:
    explicit CandidateFormDialog(QWidget* parent = nullptr);
    explicit CandidateFormDialog(const Core::Candidate& candidate, QWidget* parent = nullptr);

    Core::Candidate getCandidate() const;
    void setCandidate(const Core::Candidate& candidate);

signals:
    void candidateSaved(const Core::Candidate& candidate);

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    void setupUi();
    void loadCandidateData();
    void onPhotoClicked();
    void onSaveClicked();
    void onCancelClicked();

    QLineEdit* m_nameEdit = nullptr;
    QLineEdit* m_partyEdit = nullptr;
    QLineEdit* m_classEdit = nullptr;
    QLineEdit* m_sectionEdit = nullptr;
    QLineEdit* m_symbolEdit = nullptr;
    QLineEdit* m_videoUrlEdit = nullptr;
    QTextEdit* m_manifestoEdit = nullptr;
    QCheckBox* m_approvedCheck = nullptr;
    QLabel* m_photoLabel = nullptr;
    QPushButton* m_photoBtn = nullptr;
    QPushButton* m_saveBtn = nullptr;
    QPushButton* m_cancelBtn = nullptr;

    QImage m_candidatePhoto;
    bool m_isEditing = false;
    Core::Candidate m_originalCandidate;
};

} // namespace Ballot::UI