#include "CandidateFormDialog.h"
#include "src/core/SystemManager.h" // For accessing storage if needed (though ElectionManager is preferred)
#include "src/modules/election/ElectionManager.h" // For adding/updating candidates
#include "src/modules/audit/AuditManager.h" // For audit logging
#include "src/core/Utils.h" // For IdGenerator
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QMessageBox>
#include <QBuffer>
#include <QImageWriter>
#include <QCheckBox>
#include <QScrollArea>
#include <QUuid> // Still included for QJsonDocument::Indented or if needed elsewhere
#include <QDateTime>
#include <QPixmap>
#include <QFileDialog>
#include <QDebug> // For logging

namespace Ballot::UI {

/**
 * @brief Constructs a CandidateFormDialog for adding a new candidate.
 * @param parent The parent widget.
 */
CandidateFormDialog::CandidateFormDialog(QWidget* parent) : QDialog(parent) {
    setWindowTitle("Add Candidate");
    setModal(true);
    resize(600, 800);
    setupUi();
    qDebug() << "CandidateFormDialog: Add mode initialized.";
}

/**
 * @brief Constructs a CandidateFormDialog for editing an existing candidate.
 * @param candidate The candidate object to edit.
 * @param parent The parent widget.
 */
CandidateFormDialog::CandidateFormDialog(const Core::Candidate& candidate, QWidget* parent)
    : QDialog(parent), m_isEditing(true), m_originalCandidate(candidate) {
    setWindowTitle("Edit Candidate");
    setModal(true);
    resize(600, 800);
    setupUi();
    loadCandidateData();
    qDebug() << "CandidateFormDialog: Edit mode initialized for candidate ID:" << candidate.id;
}

/**
 * @brief Sets up the user interface of the dialog.
 */
void CandidateFormDialog::setupUi() {
    // Consolidated stylesheet for better readability
    setStyleSheet(R"(
        QDialog { background-color: #1e1e34; }
        QLabel { color: #e0e0e0; background: transparent; border: none; }
        QLineEdit, QTextEdit, QComboBox {
            background-color: #25253a; border: none; border-radius: 8px;
            padding: 10px; color: #ffffff; font-size: 14px;
        }
        QLineEdit:focus, QTextEdit:focus { border: none; }
        QPushButton {
            background-color: #0078d4; color: white; border: none; border-radius: 8px;
            padding: 12px 24px; font-size: 15px; font-weight: 600;
        }
        QPushButton:hover { background-color: #1a8ae8; }
        QPushButton:pressed { background-color: #006cbd; }
        QPushButton#ghostButton {
            background-color: transparent; color: #9a9ab0; border: 1px solid #3d3d5c;
        }
        QPushButton#ghostButton:hover { background-color: #2a2a42; color: #e0e0e0; }
        QPushButton#dangerButton { background-color: #d32f2f; }
        QPushButton#dangerButton:hover { background-color: #ef5350; }
        QGroupBox {
            background-color: #25253a; border: 1px solid #3d3d5c; border-radius: 12px;
            padding: 20px; margin-top: 16px; color: #e0e0e0; font-weight: 600; font-size: 14px;
        }
        QGroupBox::title { subcontrol-origin: margin; left: 16px; padding: 0 8px; }
        QCheckBox { color: #e0e0e0; font-size: 14px; }
        QCheckBox::indicator {
            width: 20px; height: 20px; border: 2px solid #3d3d5c; border-radius: 4px;
            background-color: #1e1e34;
        }
        QCheckBox::indicator:checked {
            background-color: #0078d4; border-color: #0078d4;
            image: url(data:image/svg+xml;base64,PHN2ZyB3aWR0aD0iMTQiIGhlaWdodD0iMTQiIHZpZXdCb3g9IjAgMCAxNCAxNCIgZmlsbD0ibm9uZSIgeG1sbnM9Imh0dHA6Ly93d3cudzMub3JnLzIwMDAvc3ZnIj4KPHBhdGggZD0iTTQuNSAxMS41TDExIDVMMTEuNSA0LjVMNC41IDExTDIgOC41IDIuNSA4TDQuNSAxMXoiIGZpbGw9IndoaXRlIi8+Cjwvc3ZnPg==);
        }
        QScrollArea { background: transparent; border: none; }
    )");

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // Header
    auto* header = new QFrame(this);
    header->setFixedHeight(70);
    header->setStyleSheet("background-color: #1a1a2e; border-bottom: 1px solid #2d2d44;");
    auto* headerLayout = new QHBoxLayout(header);

    QLabel* headerLabel = m_isEditing ? new QLabel("✏️  Edit Candidate", header) : new QLabel("➕  Add New Candidate", header);
    headerLabel->setStyleSheet("font-size: 22px; font-weight: 700; color: #ffffff; background: transparent;");
    headerLayout->addWidget(headerLabel);
    headerLayout->addStretch();

    mainLayout->addWidget(header);

    // Scroll area for form content
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
            border: none;
            border-radius: 12px;
            color: #555;
            font-size: 48px;
        }
    )");
    m_photoLabel->setText("📷");
    m_photoLabel->setCursor(Qt::PointingHandCursor);
    m_photoLabel->installEventFilter(this); // Install event filter for click
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
    qDebug() << "CandidateFormDialog: UI setup complete.";
}

/**
 * @brief Loads candidate data into the form fields when editing an existing candidate.
 */
void CandidateFormDialog::loadCandidateData() {
    if (m_originalCandidate.id.isEmpty()) {
        qWarning() << "CandidateFormDialog: Attempted to load data for empty candidate.";
        return;
    }

    m_nameEdit->setText(m_originalCandidate.name);
    m_partyEdit->setText(m_originalCandidate.party);
    m_classEdit->setText(m_originalCandidate.className);
    m_sectionEdit->setText(m_originalCandidate.section);
    m_symbolEdit->setText(m_originalCandidate.symbol);
    m_videoUrlEdit->setText(m_originalCandidate.videoUrl);
    m_manifestoEdit->setPlainText(m_originalCandidate.manifesto);
    m_approvedCheck->setChecked(m_originalCandidate.isApproved);

    // Convert QByteArray photoData to QImage for display
    if (!m_originalCandidate.photoData.isEmpty()) {
        QImage image;
        if (image.loadFromData(m_originalCandidate.photoData)) {
            m_candidatePhotoData = m_originalCandidate.photoData; // Store the raw data
            QPixmap pixmap = QPixmap::fromImage(image.scaled(150, 150, Qt::KeepAspectRatio, Qt::SmoothTransformation));
            m_photoLabel->setPixmap(pixmap);
            m_photoLabel->setText("");
            qDebug() << "CandidateFormDialog: Loaded candidate photo for display.";
        } else {
            qWarning() << "CandidateFormDialog: Failed to load image from photoData for candidate" << m_originalCandidate.id;
            m_photoLabel->setText("📷"); // Fallback icon
        }
    } else {
        m_photoLabel->setText("📷"); // Fallback icon
    }
    qDebug() << "CandidateFormDialog: Candidate data loaded into form.";
}

/**
 * @brief Slot to handle photo selection when the photo label or button is clicked.
 */
void CandidateFormDialog::onPhotoClicked() {
    qDebug() << "CandidateFormDialog: Photo selection initiated.";
    QString filePath = QFileDialog::getOpenFileName(this, "Select Candidate Photo", "", "Images (*.png *.jpg *.jpeg *.bmp *.webp)");
    if (!filePath.isEmpty()) {
        QImage image;
        if (image.load(filePath)) {
            // Scale to max 300x300 keeping aspect ratio
            if (image.width() > Core::Constants::PHOTO_CROP_SIZE || image.height() > Core::Constants::PHOTO_CROP_SIZE) {
                image = image.scaled(Core::Constants::PHOTO_CROP_SIZE, Core::Constants::PHOTO_CROP_SIZE, Qt::KeepAspectRatio, Qt::SmoothTransformation);
            }

            // Convert QImage to QByteArray (PNG format for lossless storage)
            QByteArray imageData;
            QBuffer buffer(&imageData);
            buffer.open(QIODevice::WriteOnly);
            if (image.save(&buffer, "PNG")) {
                m_candidatePhotoData = imageData;
                QPixmap pixmap = QPixmap::fromImage(image);
                m_photoLabel->setPixmap(pixmap.scaled(150, 150, Qt::KeepAspectRatio, Qt::SmoothTransformation));
                m_photoLabel->setText("");
                qInfo() << "CandidateFormDialog: Photo selected and converted to QByteArray.";
            } else {
                qCritical() << "CandidateFormDialog: Failed to convert QImage to QByteArray (PNG).";
                QMessageBox::warning(this, "Image Error", "Could not process the selected image.");
            }
        } else {
            qWarning() << "CandidateFormDialog: Could not load the selected image file:" << filePath;
            QMessageBox::warning(this, "Invalid Image", "Could not load the selected image file.");
        }
    } else {
        qDebug() << "CandidateFormDialog: Photo selection cancelled.";
    }
}

/**
 * @brief Slot to handle the save button click. Validates input and emits candidateSaved signal.
 */
void CandidateFormDialog::onSaveClicked() {
    qInfo() << "CandidateFormDialog: Save button clicked.";
    QString name = m_nameEdit->text().trimmed();
    QString party = m_partyEdit->text().trimmed();
    QString className = m_classEdit->text().trimmed();

    if (name.isEmpty()) {
        QMessageBox::warning(this, "Validation Error", "Candidate name is required.");
        m_nameEdit->setFocus();
        Audit::AuditManager::instance().log(Core::AuditAction::CandidateModified, "Candidate save failed: Name missing.", "UI");
        return;
    }
    if (party.isEmpty()) {
        QMessageBox::warning(this, "Validation Error", "Party/Group name is required.");
        m_partyEdit->setFocus();
        Audit::AuditManager::instance().log(Core::AuditAction::CandidateModified, "Candidate save failed: Party missing.", "UI");
        return;
    }
    if (className.isEmpty()) {
        QMessageBox::warning(this, "Validation Error", "Class/Grade is required.");
        m_classEdit->setFocus();
        Audit::AuditManager::instance().log(Core::AuditAction::CandidateModified, "Candidate save failed: Class missing.", "UI");
        return;
    }

    Core::Candidate candidate;
    if (m_isEditing) {
        candidate = m_originalCandidate;
    } else {
        // ID and registeredAt will be set by ElectionManager when adding a new candidate
        // candidate.id = Core::IdGenerator::generateId();
        // candidate.registeredAt = QDateTime::currentDateTime();
    }

    candidate.name = name;
    candidate.party = party;
    candidate.className = className;
    candidate.section = m_sectionEdit->text().trimmed();
    candidate.symbol = m_symbolEdit->text().trimmed();
    candidate.videoUrl = m_videoUrlEdit->text().trimmed();
    candidate.manifesto = m_manifestoEdit->toPlainText().trimmed();
    candidate.isApproved = m_approvedCheck->isChecked();
    candidate.photoData = m_candidatePhotoData; // Use photoData

    emit candidateSaved(candidate); // Emit the candidate object
    accept(); // Close dialog with accept result
    qInfo() << "CandidateFormDialog: Candidate data emitted and dialog accepted.";
}

/**
 * @brief Slot to handle the cancel button click.
 */
void CandidateFormDialog::onCancelClicked() {
    qInfo() << "CandidateFormDialog: Cancel button clicked. Dialog rejected.";
    reject(); // Close dialog with reject result
}

/**
 * @brief Retrieves the candidate data from the form fields.
 * @return A Core::Candidate object populated with current form data.
 */
Core::Candidate CandidateFormDialog::getCandidate() const {
    Core::Candidate candidate;
    if (m_isEditing) {
        candidate = m_originalCandidate;
    } else {
        // ID and registeredAt will be set by ElectionManager when adding a new candidate
        // candidate.id = Core::IdGenerator::generateId();
        // candidate.registeredAt = QDateTime::currentDateTime();
    }

    candidate.name = m_nameEdit->text().trimmed();
    candidate.party = m_partyEdit->text().trimmed();
    candidate.className = m_classEdit->text().trimmed();
    candidate.section = m_sectionEdit->text().trimmed();
    candidate.symbol = m_symbolEdit->text().trimmed();
    candidate.videoUrl = m_videoUrlEdit->text().trimmed();
    candidate.manifesto = m_manifestoEdit->toPlainText().trimmed();
    candidate.isApproved = m_approvedCheck->isChecked();
    candidate.photoData = m_candidatePhotoData; // Use photoData

    return candidate;
}

/**
 * @brief Sets the candidate data to be displayed and edited in the form.
 * @param candidate The Core::Candidate object to display.
 */
void CandidateFormDialog::setCandidate(const Core::Candidate& candidate) {
    m_originalCandidate = candidate;
    m_isEditing = !candidate.id.isEmpty();
    setWindowTitle(m_isEditing ? "Edit Candidate" : "Add Candidate");
    // Update save button text
    if (m_saveBtn) { // Check if button is initialized
        m_saveBtn->setText(m_isEditing ? "Update Candidate" : "Add Candidate");
    }
    loadCandidateData();
    qDebug() << "CandidateFormDialog: Candidate set for form. Editing mode:" << m_isEditing;
}

/**
 * @brief Event filter to handle clicks on the photo label.
 * @param watched The object that received the event.
 * @param event The event that occurred.
 * @return True if the event was handled, false otherwise.
 */
bool CandidateFormDialog::eventFilter(QObject* watched, QEvent* event) {
    if (watched == m_photoLabel && event->type() == QEvent::MouseButtonPress) {
        onPhotoClicked();
        return true; // Event handled
    }
    return QDialog::eventFilter(watched, event); // Pass unhandled events to base class
}

} // namespace Ballot::UI