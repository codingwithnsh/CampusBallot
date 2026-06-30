#pragma once

#include <QDialog>
#include <QLabel>
#include <QLineEdit>
#include <QTextEdit>
#include <QPushButton>
#include <QComboBox>
#include <QFileDialog>
#include <QPixmap>
#include "src/core/Models.h"

namespace Ballot::UI {

class CandidateDialog : public QDialog {
    Q_OBJECT
public:
    explicit CandidateDialog(const QString& electionId, QWidget *parent = nullptr);
    explicit CandidateDialog(const Core::Candidate& candidate, const QString& electionId, QWidget *parent = nullptr);
    ~CandidateDialog() override = default;

    Core::Candidate getCandidate() const { return m_candidate; }
    bool isValid() const { return m_isValid; }

signals:
    void candidateSaved(const Core::Candidate& candidate);

private:
    void setupUi();
    void loadCandidateData();
    void selectPhoto();

    QString m_electionId;
    Core::Candidate m_candidate;
    bool m_isEditMode = false;
    bool m_isValid = false;

    // UI elements
    QLabel* m_photoLabel = nullptr;
    QPushButton* m_selectPhotoBtn = nullptr;
    QLineEdit* m_nameEdit = nullptr;
    QLineEdit* m_partyEdit = nullptr;
    QLineEdit* m_classEdit = nullptr;
    QLineEdit* m_sectionEdit = nullptr;
    QLineEdit* m_symbolEdit = nullptr;
    QTextEdit* m_manifestoEdit = nullptr;
    QLineEdit* m_videoUrlEdit = nullptr;
    QPushButton* m_saveBtn = nullptr;
    QPushButton* m_cancelBtn = nullptr;
};

} // namespace Ballot::UI