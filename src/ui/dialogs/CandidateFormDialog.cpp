#include "CandidateFormDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QMessageBox>
#include <QBuffer>
#include <QImageWriter>
#include <QCheckBox>
#include <QScrollArea>
#include <QUuid>
#include <QDateTime>
#include <QPixmap>

namespace Ballot::UI {

CandidateFormDialog::CandidateFormDialog(QWidget* parent) : QDialog(parent) {
    setWindowTitle("Add Candidate");
    setModal(true);
    resize(600, 800);
    setupUi();
}

CandidateFormDialog::CandidateFormDialog(const Core::Candidate& candidate, QWidget* parent) : QDialog(parent), m_isEditing(true), m_originalCandidate(candidate) {
    setWindowTitle("Edit Candidate");
    setModal(true);
    resize(600, 800);
    setupUi();
    loadCandidateData();
}

void CandidateFormDialog::setupUi() {
    setStyleSheet(R"(
        QDialog {
            background-color: #1e1e34;
        }
        QLabel {
            color: #e0e0e0;
            background: transparent;
        }
        QLineEdit, QTextEdit, QComboBox {
            background-color: #25253a;
            border: 1px solid #3d3d5c;
            border-radius: 8px;
            padding: 10px;
            color: #ffffff;
            font-size: 14px;
        }
        QLineEdit:focus, QTextEdit:focus {
            border-color: #0078d4;
        }
        QPushButton {
            background-color: #0078d4;
            color: white;
            border: none;
            border-radius: 8px;
            padding: 12px 24px;
            font-size: 15px;
            font-weight: 600;
        }
        QPushButton:hover {
            background-color: #1a8ae8;
        }
        QPushButton:pressed {
            background-color: #006cbd;
        }
        QPushButton#ghostButton {
            background-color: transparent;
            color: #9a9ab0;
            border: 1px solid #3d3d5c;
        }
        QPushButton#ghostButton:hover {
            background-color: #2a2a42;
            color: #e0e0e0;
        }
        QPushButton#dangerButton {
            background-color: #d32f2f;
        }
        QPushButton#dangerButton:hover {
            background-color: #ef5350;
        }
        QGroupBox {
            background-color: #25253a;
            border: 1px solid #3d3d5c;
            border-radius: 12px;
            padding: 20px;
            margin-top: 16px;
            color: #e0e0e0;
            font-weight: 600;
            font-size: 14px;
        }
        QGroupBox::title {
            subcontrol-origin: margin;
            left: 16px;
            padding: 0 8px;
        }
        QCheckBox {
            color: #e0e0e0;
            font-size: 14px;
        }
        QCheckBox::indicator {
            width: 20px;
            height: 20px;
            border: 2px solid #3d3d5c;
            border-radius: 4px;
            background-color: #1e1e34;
        }
        QCheckBox::indicator:checked {
            background-color: #0078d4;
            border-color: #0078d4;
            image: url(data:image/svg+xml;base64,PHN2ZyB3aWR0aD0iMTQiIGhlaWdodD0iMTQiIHZpZXdCb3g9IjAgMCAxNCAxNCIgZmlsbD0ibm9uZSIgeG1sbnM9Imh0dHA6Ly93d3cudzMub3JnLzIwMDAvc3ZnIj4KPHBhdGggZD0iTTQuNSAxMS41TDExIDVMMTEuNSA0LjVMNC41IDExTDIgOC41IDIuNSA4TDQuNSAxMXoiIGZpbGw9IndoaXRlIi8+Cjwvc3ZnPg==);
        }
        QScrollArea {
            background: transparent;
            border: none;
        }
    )");

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // Header
    auto* header = new QFrame(this);
    header->setFixedHeight(70);
    header->setStyleSheet("background-color: #1a1a2e; border-bottom: 1px solid #2d2d44;");
    auto* headerLayout = new QHBoxLayout(header);
    
    m_isEditing ? headerLayout->addWidget(new QLabel("✏️  Edit Candidate", header)) : headerLayout->addWidget(new QLabel("➕  Add New Candidate", header));
    header->findChild<QLabel*>()->setStyleSheet("font-size: 22px; font-weight: 700; color: #ffffff; background: transparent;");
    headerLayout->addStretch();
    
    mainLayout->addWidget(header);

    // Scroll area for form
    auto* scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setStyleSheet("QScrollArea { background: transparent; border: none; }");
    
    auto* scrollContent = new QWidget();
    scrollContent->setStyleSheet("background: transparent;");
    auto* formLayout = new QVBoxLayout(scrollContent);
    formLayout->setContentsMargins(32, 24, 32, 24);
    formLayout->setSpacing(20);

    // Photo Section
    auto* photoGroup = new QGroupBox("Candidate Photo", scrollContent);
    auto* photoLayout = new QVBoxLayout(photoGroup);
    photoLayout->setSpacing(16);

    auto* photoContainer = new QWidget(photoGroup);
    photoContainer->setStyleSheet("background: transparent;");
    auto* photoContainerLayout = new QHBoxLayout(photoContainer);
    photoContainerLayout->setAlignment(Qt::AlignLeft);

    m_photoLabel = new QLabel(photoContainer);
    m_photoLabel->setFixedSize(150, 150);
    m_photoLabel->setAlignment(Qt::AlignCenter);
    m_photoLabel->setStyleSheet(R"(
        QLabel {
            background-color: #1a1a2e;
            border: 2px dashed #3d3d5c;
            border-radius: 12px;
            color: #555;
            font-size: 48px;
        }
    )");
    m_photoLabel->setText("📷");
    m_photoLabel->setCursor(Qt::PointingHandCursor);
    m_photoLabel->installEventFilter(this);
    photoContainerLayout->addWidget(m_photoLabel);

    auto* photoInfo = new QVBoxLayout();
    photoInfo->setSpacing(8);
    m_photoBtn = new QPushButton("Choose Photo", photoContainer);
    m_photoBtn->setFixedWidth(160);
    m_photoBtn->setStyleSheet("background-color: #0078d4; color: white; font-size: 14px; padding: 10px; border-radius: 8px;");
    connect(m_photoBtn, &QPushButton::clicked, this, &CandidateFormDialog::onPhotoClicked);
    photoInfo->addWidget(m_photoBtn);

    auto* photoHint = new QLabel("Recommended: 300x300px, JPG/PNG", photoContainer);
    photoHint->setStyleSheet("font-size: 12px; color: #7a7a90; background: transparent;");
    photoInfo->addWidget(photoHint);
    photoInfo->addStretch();
    photoContainerLayout->addLayout(photoInfo);
    photoContainerLayout->addStretch();

    photoLayout->addWidget(photoContainer);
    formLayout->addWidget(photoGroup);

    // Basic Information
    auto* basicGroup = new QGroupBox("Basic Information", scrollContent);
    auto* basicLayout = new QFormLayout(basicGroup);
    basicLayout->setSpacing(16);
    basicLayout->setLabelAlignment(Qt::AlignLeft);
    basicLayout->setFormAlignment(Qt::AlignHCenter | Qt::AlignTop);
    basicLayout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);

    auto styleLabel = [](QLabel* label) {
        label->setStyleSheet("font-size: 14px; font-weight: 500; color: #e0e0e0; background: transparent;");
    };

    m_nameEdit = new QLineEdit(basicGroup);
    m_nameEdit->setPlaceholderText("Enter candidate full name");
    m_nameEdit->setFixedHeight(44);
    auto* nameLabel = new QLabel("Full Name *");
    styleLabel(nameLabel);
    basicLayout->addRow(nameLabel, m_nameEdit);

    m_partyEdit = new QLineEdit(basicGroup);
    m_partyEdit->setPlaceholderText("Enter party or group name");
    m_partyEdit->setFixedHeight(44);
    auto* partyLabel = new QLabel("Party / Group *");
    styleLabel(partyLabel);
    basicLayout->addRow(partyLabel, m_partyEdit);

    auto* classSectionLayout = new QHBoxLayout();
    classSectionLayout->setSpacing(12);
    
    m_classEdit = new QLineEdit(basicGroup);
    m_classEdit->setPlaceholderText("e.g., 12th Grade");
    m_classEdit->setFixedHeight(44);
    auto* classLabel = new QLabel("Class / Grade *");
    styleLabel(classLabel);
    classSectionLayout->addWidget(classLabel, 0, Qt::AlignVCenter);
    classSectionLayout->addWidget(m_classEdit, 1);
    
    m_sectionEdit = new QLineEdit(basicGroup);
    m_sectionEdit->setPlaceholderText("e.g., A, B, C");
    m_sectionEdit->setFixedHeight(44);
    m_sectionEdit->setFixedWidth(100);
    auto* sectionLabel = new QLabel("Section");
    styleLabel(sectionLabel);
    classSectionLayout->addWidget(sectionLabel, 0, Qt::AlignVCenter);
    classSectionLayout->addWidget(m_sectionEdit, 0);
    classSectionLayout->addStretch();
    
    basicLayout->addRow(classSectionLayout);

    m_symbolEdit = new QLineEdit(basicGroup);
    m_symbolEdit->setPlaceholderText("e.g., 🌟, ✊, 📚, 🌱");
    m_symbolEdit->setFixedHeight(44);
    auto* symbolLabel = new QLabel("Symbol / Emoji");
    styleLabel(symbolLabel);
    basicLayout->addRow(symbolLabel, m_symbolEdit);

    m_videoUrlEdit = new QLineEdit(basicGroup);
    m_videoUrlEdit->setPlaceholderText("https://youtube.com/... or https://drive.google.com/...");
    m_videoUrlEdit->setFixedHeight(44);
    auto* videoLabel = new QLabel("Video URL");
    styleLabel(videoLabel);
    basicLayout->addRow(videoLabel, m_videoUrlEdit);

    formLayout->addWidget(basicGroup);

    // Manifesto
    auto* manifestoGroup = new QGroupBox("Manifesto / Vision Statement", scrollContent);
    auto* manifestoLayout = new QVBoxLayout(manifestoGroup);
    manifestoLayout->setSpacing(12);

    m_manifestoEdit = new QTextEdit(manifestoGroup);
    m_manifestoEdit->setPlaceholderText("Describe your vision, goals, and what you plan to achieve if elected...");
    m_manifestoEdit->setFixedHeight(180);
    m_manifestoEdit->setStyleSheet("font-size: 14px; line-height: 1.5;");
    manifestoLayout->addWidget(m_manifestoEdit);

    auto* manifestoHint = new QLabel("Markdown supported. Keep it concise but impactful.", manifestoGroup);
    manifestoHint->setStyleSheet("font-size: 12px; color: #7a7a90; background: transparent;");
    manifestoLayout->addWidget(manifestoHint);

    formLayout->addWidget(manifestoGroup);

    // Status
    auto* statusGroup = new QGroupBox("Status", scrollContent);
    auto* statusLayout = new QHBoxLayout(statusGroup);
    statusLayout->setSpacing(20);

    m_approvedCheck = new QCheckBox("✓ Approved for Ballot", statusGroup);
    m_approvedCheck->setChecked(true);
    m_approvedCheck->setStyleSheet("font-size: 14px; font-weight: 500;");
    statusLayout->addWidget(m_approvedCheck);

    statusLayout->addStretch();
    formLayout->addWidget(statusGroup);

    formLayout->addStretch();

    scrollArea->setWidget(scrollContent);
    mainLayout->addWidget(scrollArea, 1);

    // Footer buttons
    auto* footer = new QFrame(this);
    footer->setFixedHeight(80);
    footer->setStyleSheet("background-color: #1a1a2e; border-top: 1px solid #2d2d44;");
    auto* footerLayout = new QHBoxLayout(footer);
    footerLayout->setContentsMargins(32, 10, 32, 10);

    m_cancelBtn = new QPushButton("Cancel", footer);
    m_cancelBtn->setObjectName("ghostButton");
    m_cancelBtn->setFixedWidth(120);
    connect(m_cancelBtn, &QPushButton::clicked, this, &CandidateFormDialog::onCancelClicked);

    footerLayout->addStretch();
    footerLayout->addWidget(m_cancelBtn);

    m_saveBtn = new QPushButton(m_isEditing ? "Update Candidate" : "Add Candidate", footer);
    m_saveBtn->setFixedWidth(180);
    connect(m_saveBtn, &QPushButton::clicked, this, &CandidateFormDialog::onSaveClicked);

    footerLayout->addWidget(m_saveBtn);
    mainLayout->addWidget(footer);
}

void CandidateFormDialog::loadCandidateData() {
    if (m_originalCandidate.id.isEmpty()) return;

    m_nameEdit->setText(m_originalCandidate.name);
    m_partyEdit->setText(m_originalCandidate.party);
    m_classEdit->setText(m_originalCandidate.className);
    m_sectionEdit->setText(m_originalCandidate.section);
    m_symbolEdit->setText(m_originalCandidate.symbol);
    m_videoUrlEdit->setText(m_originalCandidate.videoUrl);
    m_manifestoEdit->setPlainText(m_originalCandidate.manifesto);
    m_approvedCheck->setChecked(m_originalCandidate.isApproved);

    if (!m_originalCandidate.photo.isNull()) {
        m_candidatePhoto = m_originalCandidate.photo;
        QPixmap pixmap = QPixmap::fromImage(m_candidatePhoto.scaled(150, 150, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        m_photoLabel->setPixmap(pixmap);
        m_photoLabel->setText("");
    }
}

void CandidateFormDialog::onPhotoClicked() {
    QString filePath = QFileDialog::getOpenFileName(this, "Select Candidate Photo", "", "Images (*.png *.jpg *.jpeg *.bmp *.webp)");
    if (!filePath.isEmpty()) {
        QImage image(filePath);
        if (!image.isNull()) {
            // Scale to max 300x300 keeping aspect ratio
            if (image.width() > 300 || image.height() > 300) {
                image = image.scaled(300, 300, Qt::KeepAspectRatio, Qt::SmoothTransformation);
            }
            m_candidatePhoto = image;
            QPixmap pixmap = QPixmap::fromImage(m_candidatePhoto);
            m_photoLabel->setPixmap(pixmap.scaled(150, 150, Qt::KeepAspectRatio, Qt::SmoothTransformation));
            m_photoLabel->setText("");
        } else {
            QMessageBox::warning(this, "Invalid Image", "Could not load the selected image file.");
        }
    }
}

void CandidateFormDialog::onSaveClicked() {
    QString name = m_nameEdit->text().trimmed();
    QString party = m_partyEdit->text().trimmed();
    QString className = m_classEdit->text().trimmed();

    if (name.isEmpty()) {
        QMessageBox::warning(this, "Validation Error", "Candidate name is required.");
        m_nameEdit->setFocus();
        return;
    }
    if (party.isEmpty()) {
        QMessageBox::warning(this, "Validation Error", "Party/Group name is required.");
        m_partyEdit->setFocus();
        return;
    }
    if (className.isEmpty()) {
        QMessageBox::warning(this, "Validation Error", "Class/Grade is required.");
        m_classEdit->setFocus();
        return;
    }

    Core::Candidate candidate;
    if (m_isEditing) {
        candidate = m_originalCandidate;
    } else {
        candidate.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
        candidate.registeredAt = QDateTime::currentDateTime();
    }

    candidate.name = name;
    candidate.party = party;
    candidate.className = className;
    candidate.section = m_sectionEdit->text().trimmed();
    candidate.symbol = m_symbolEdit->text().trimmed();
    candidate.videoUrl = m_videoUrlEdit->text().trimmed();
    candidate.manifesto = m_manifestoEdit->toPlainText().trimmed();
    candidate.isApproved = m_approvedCheck->isChecked();
    candidate.photo = m_candidatePhoto;

    emit candidateSaved(candidate);
    accept();
}

void CandidateFormDialog::onCancelClicked() {
    reject();
}

Core::Candidate CandidateFormDialog::getCandidate() const {
    Core::Candidate candidate;
    if (m_isEditing) {
        candidate = m_originalCandidate;
    } else {
        candidate.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
        candidate.registeredAt = QDateTime::currentDateTime();
    }

    candidate.name = m_nameEdit->text().trimmed();
    candidate.party = m_partyEdit->text().trimmed();
    candidate.className = m_classEdit->text().trimmed();
    candidate.section = m_sectionEdit->text().trimmed();
    candidate.symbol = m_symbolEdit->text().trimmed();
    candidate.videoUrl = m_videoUrlEdit->text().trimmed();
    candidate.manifesto = m_manifestoEdit->toPlainText().trimmed();
    candidate.isApproved = m_approvedCheck->isChecked();
    candidate.photo = m_candidatePhoto;

    return candidate;
}

void CandidateFormDialog::setCandidate(const Core::Candidate& candidate) {
    m_originalCandidate = candidate;
    m_isEditing = !candidate.id.isEmpty();
    setWindowTitle(m_isEditing ? "Edit Candidate" : "Add Candidate");
    m_saveBtn->setText(m_isEditing ? "Update Candidate" : "Add Candidate");
    loadCandidateData();
}

bool CandidateFormDialog::eventFilter(QObject* watched, QEvent* event) {
    if (watched == m_photoLabel && event->type() == QEvent::MouseButtonPress) {
        onPhotoClicked();
        return true;
    }
    return QDialog::eventFilter(watched, event);
}

} // namespace Ballot::UI