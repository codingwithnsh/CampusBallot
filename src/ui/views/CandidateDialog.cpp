#include "CandidateDialog.h"
#include "src/ui/components/ToastNotification.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QFileDialog>
#include <QPixmap>
#include <QMessageBox>
#include <QUuid>
#include <QDateTime>

namespace Ballot::UI {

CandidateDialog::CandidateDialog(const QString& electionId, QWidget *parent)
    : QDialog(parent), m_electionId(electionId) {
    m_candidate.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    m_candidate.electionId = electionId;
    m_candidate.isApproved = true;
    m_candidate.registeredAt = QDateTime::currentDateTime();
    setWindowTitle("Add Candidate");
    setModal(true);
    resize(600, 700);
    setupUi();
}

CandidateDialog::CandidateDialog(const Core::Candidate& candidate, const QString& electionId, QWidget *parent)
    : QDialog(parent), m_candidate(candidate), m_electionId(electionId) {
    m_isEditMode = true;
    setWindowTitle("Edit Candidate");
    setModal(true);
    resize(600, 700);
    setupUi();
    loadCandidateData();
}

void CandidateDialog::setupUi() {
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(24, 24, 24, 24);
    mainLayout->setSpacing(20);

    // Title
    auto *title = new QLabel(m_isEditMode ? "Edit Candidate" : "Add New Candidate", this);
    title->setStyleSheet("font-size: 24px; font-weight: 700; color: #ffffff; background: transparent;");
    mainLayout->addWidget(title);

    // Photo section
    auto *photoFrame = new QFrame(this);
    photoFrame->setObjectName("card");
    photoFrame->setStyleSheet("QFrame#card { background-color: #25253a; border: 1px solid #3d3d5c; border-radius: 12px; padding: 20px; }");
    auto *photoLayout = new QHBoxLayout(photoFrame);

    // Photo preview
    m_photoLabel = new QLabel(this);
    m_photoLabel->setFixedSize(150, 150);
    m_photoLabel->setStyleSheet("background-color: #374151; border: 2px dashed #4b5563; border-radius: 12px;");
    m_photoLabel->setAlignment(Qt::AlignCenter);
    m_photoLabel->setText("No Photo\nClick to add");
    m_photoLabel->setStyleSheet(m_photoLabel->styleSheet() + "color: #9a9ab0; font-size: 14px;");
    m_photoLabel->setCursor(Qt::PointingHandCursor);
    photoLayout->addWidget(m_photoLabel);

    // Photo controls
    auto *photoControls = new QVBoxLayout();
    m_selectPhotoBtn = new QPushButton("Select Photo", this);
    m_selectPhotoBtn->setObjectName("primaryButton");
    m_selectPhotoBtn->setFixedHeight(40);
    m_selectPhotoBtn->setCursor(Qt::PointingHandCursor);
    connect(m_selectPhotoBtn, &QPushButton::clicked, this, &CandidateDialog::selectPhoto);
    photoControls->addWidget(m_selectPhotoBtn);

    auto *photoHint = new QLabel("Recommended: 300x300px, PNG/JPG", this);
    photoHint->setStyleSheet("font-size: 12px; color: #9a9ab0; background: transparent;");
    photoControls->addWidget(photoHint);
    photoControls->addStretch();

    photoLayout->addLayout(photoControls);
    mainLayout->addWidget(photoFrame);

    // Form section
    auto *formFrame = new QFrame(this);
    formFrame->setObjectName("card");
    formFrame->setStyleSheet("QFrame#card { background-color: #25253a; border: 1px solid #3d3d5c; border-radius: 12px; padding: 20px; }");
    auto *formLayout = new QFormLayout(formFrame);
    formLayout->setSpacing(16);
    formLayout->setLabelAlignment(Qt::AlignRight);

    auto labelStyle = "font-size: 14px; font-weight: 600; color: #e0e0e0; background: transparent;";
    auto inputStyle = R"(
        QLineEdit, QTextEdit {
            background-color: #374151;
            border: 1px solid #4b5563;
            border-radius: 8px;
            padding: 10px 12px;
            color: #ffffff;
            font-size: 14px;
        }
        QLineEdit:focus, QTextEdit:focus {
            border-color: #0078d4;
        }
    )";

    // Name (required)
    m_nameEdit = new QLineEdit(this);
    m_nameEdit->setPlaceholderText("Enter candidate's full name");
    m_nameEdit->setStyleSheet(inputStyle);
    auto *nameLabel = new QLabel("Name *", this);
    nameLabel->setStyleSheet(labelStyle);
    formLayout->addRow(nameLabel, m_nameEdit);

    // Party
    m_partyEdit = new QLineEdit(this);
    m_partyEdit->setPlaceholderText("Enter party/group name");
    m_partyEdit->setStyleSheet(inputStyle);
    auto *partyLabel = new QLabel("Party / Group", this);
    partyLabel->setStyleSheet(labelStyle);
    formLayout->addRow(partyLabel, m_partyEdit);

    // Class
    m_classEdit = new QLineEdit(this);
    m_classEdit->setPlaceholderText("e.g., 12th Grade, Senior Year");
    m_classEdit->setStyleSheet(inputStyle);
    auto *classLabel = new QLabel("Class / Year", this);
    classLabel->setStyleSheet(labelStyle);
    formLayout->addRow(classLabel, m_classEdit);

    // Section
    m_sectionEdit = new QLineEdit(this);
    m_sectionEdit->setPlaceholderText("e.g., A, B, Section 1");
    m_sectionEdit->setStyleSheet(inputStyle);
    auto *sectionLabel = new QLabel("Section", this);
    sectionLabel->setStyleSheet(labelStyle);
    formLayout->addRow(sectionLabel, m_sectionEdit);

    // Symbol
    m_symbolEdit = new QLineEdit(this);
    m_symbolEdit->setPlaceholderText("e.g., ★, 🌟, Book, Tree");
    m_symbolEdit->setStyleSheet(inputStyle);
    auto *symbolLabel = new QLabel("Symbol", this);
    symbolLabel->setStyleSheet(labelStyle);
    formLayout->addRow(symbolLabel, m_symbolEdit);

    // Manifesto
    m_manifestoEdit = new QTextEdit(this);
    m_manifestoEdit->setPlaceholderText("Enter candidate's manifesto/vision...");
    m_manifestoEdit->setStyleSheet(inputStyle);
    m_manifestoEdit->setMaximumHeight(120);
    auto *manifestoLabel = new QLabel("Manifesto", this);
    manifestoLabel->setStyleSheet(labelStyle);
    formLayout->addRow(manifestoLabel, m_manifestoEdit);

    // Video URL
    m_videoUrlEdit = new QLineEdit(this);
    m_videoUrlEdit->setPlaceholderText("https://youtube.com/... or https://vimeo.com/...");
    m_videoUrlEdit->setStyleSheet(inputStyle);
    auto *videoLabel = new QLabel("Video URL", this);
    videoLabel->setStyleSheet(labelStyle);
    formLayout->addRow(videoLabel, m_videoUrlEdit);

    mainLayout->addWidget(formFrame, 1);

    // Buttons
    auto *btnLayout = new QHBoxLayout();
    btnLayout->addStretch();

    m_cancelBtn = new QPushButton("Cancel", this);
    m_cancelBtn->setObjectName("ghostButton");
    m_cancelBtn->setFixedSize(120, 44);
    m_cancelBtn->setCursor(Qt::PointingHandCursor);
    connect(m_cancelBtn, &QPushButton::clicked, this, &QDialog::reject);

    m_saveBtn = new QPushButton(m_isEditMode ? "Update" : "Add Candidate", this);
    m_saveBtn->setObjectName("successButton");
    m_saveBtn->setFixedSize(160, 44);
    m_saveBtn->setCursor(Qt::PointingHandCursor);
    connect(m_saveBtn, &QPushButton::clicked, this, [this]() {
        if (validateAndSave()) {
            accept();
        }
    });

    btnLayout->addWidget(m_cancelBtn);
    btnLayout->addWidget(m_saveBtn);
    mainLayout->addLayout(btnLayout);
}

void CandidateDialog::loadCandidateData() {
    m_nameEdit->setText(m_candidate.name);
    m_partyEdit->setText(m_candidate.party);
    m_classEdit->setText(m_candidate.className);
    m_sectionEdit->setText(m_candidate.section);
    m_symbolEdit->setText(m_candidate.symbol);
    m_manifestoEdit->setPlainText(m_candidate.manifesto);
    m_videoUrlEdit->setText(m_candidate.videoUrl);

    if (!m_candidate.photo.isNull()) {
        m_photoLabel->setPixmap(m_candidate.photo.scaled(150, 150, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        m_photoLabel->setText("");
        m_photoLabel->setStyleSheet("background-color: #374151; border: 2px solid #0078d4; border-radius: 12px;");
    }
}

void CandidateDialog::selectPhoto() {
    QString filePath = QFileDialog::getOpenFileName(this, "Select Candidate Photo", "",
                                                    "Images (*.png *.jpg *.jpeg *.bmp *.webp)");
    if (!filePath.isEmpty()) {
        QPixmap pixmap(filePath);
        if (!pixmap.isNull()) {
            // Scale to reasonable size
            if (pixmap.width() > 600 || pixmap.height() > 600) {
                pixmap = pixmap.scaled(600, 600, Qt::KeepAspectRatio, Qt::SmoothTransformation);
            }
            m_candidate.photo = pixmap.toImage();
            m_photoLabel->setPixmap(pixmap.scaled(150, 150, Qt::KeepAspectRatio, Qt::SmoothTransformation));
            m_photoLabel->setText("");
            m_photoLabel->setStyleSheet("background-color: #374151; border: 2px solid #0078d4; border-radius: 12px;");
        } else {
            ToastNotification::show(this, "Failed to load image.", ToastNotification::Error);
        }
    }
}

bool CandidateDialog::validateAndSave() {
    QString name = m_nameEdit->text().trimmed();
    if (name.isEmpty()) {
        ToastNotification::show(this, "Please enter the candidate's name.", ToastNotification::Warning);
        m_nameEdit->setFocus();
        return false;
    }

    m_candidate.name = name;
    m_candidate.party = m_partyEdit->text().trimmed();
    m_candidate.className = m_classEdit->text().trimmed();
    m_candidate.section = m_sectionEdit->text().trimmed();
    m_candidate.symbol = m_symbolEdit->text().trimmed();
    m_candidate.manifesto = m_manifestoEdit->toPlainText().trimmed();
    m_candidate.videoUrl = m_videoUrlEdit->text().trimmed();
    m_candidate.isApproved = true;

    m_isValid = true;
    return true;
}

} // namespace Ballot::UI