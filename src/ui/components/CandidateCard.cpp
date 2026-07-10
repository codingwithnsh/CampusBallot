#include "CandidateCard.h"
#include <QPainter>
#include <QPixmap>
#include <QBuffer>
#include <QMouseEvent>
#include <QEnterEvent>

namespace Ballot::UI {

CandidateCard::CandidateCard(const Core::Candidate& candidate, QWidget* parent)
    : QWidget(parent)
    , m_candidate(candidate)
    , m_cardFrame(nullptr)
    , m_shadowEffect(nullptr)
{
    setupUi();
    updateUi();
    createShadowEffect();
    setCursor(Qt::PointingHandCursor);
    setAttribute(Qt::WA_Hover, true);
}

void CandidateCard::setupUi() {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    m_cardFrame = new QFrame(this);
    m_cardFrame->setObjectName("candidateCardFrame");
    auto* frameLayout = new QVBoxLayout(m_cardFrame);
    frameLayout->setContentsMargins(20, 20, 20, 20);
    frameLayout->setSpacing(12);

    // Photo
    m_photoLabel = new QLabel(m_cardFrame);
    m_photoLabel->setFixedSize(120, 120);
    m_photoLabel->setAlignment(Qt::AlignCenter);
    m_photoLabel->setStyleSheet("border-radius: 60px; background-color: #1a1a2e;");
    m_photoLabel->setScaledContents(false);
    frameLayout->addWidget(m_photoLabel, 0, Qt::AlignCenter);

    // Name
    m_nameLabel = new QLabel(m_cardFrame);
    m_nameLabel->setAlignment(Qt::AlignCenter);
    m_nameLabel->setWordWrap(true);
    m_nameLabel->setStyleSheet("background: transparent;");
    frameLayout->addWidget(m_nameLabel);

    // Party
    m_partyLabel = new QLabel(m_cardFrame);
    m_partyLabel->setAlignment(Qt::AlignCenter);
    m_partyLabel->setWordWrap(true);
    m_partyLabel->setStyleSheet("background: transparent;");
    frameLayout->addWidget(m_partyLabel);

    // Manifesto (optional, shown on hover or when enabled)
    m_manifestoLabel = new QLabel(m_cardFrame);
    m_manifestoLabel->setAlignment(Qt::AlignCenter);
    m_manifestoLabel->setWordWrap(true);
    m_manifestoLabel->setVisible(false);
    m_manifestoLabel->setStyleSheet("background: transparent;");
    frameLayout->addWidget(m_manifestoLabel);

    frameLayout->addStretch();

    // Select button
    m_selectButton = new QPushButton("Select", m_cardFrame);
    m_selectButton->setCursor(Qt::PointingHandCursor);
    m_selectButton->setFixedHeight(44);
    connect(m_selectButton, &QPushButton::clicked, this, [this]() {
        if (m_selectable) {
            m_selected = true;
            updateUi();
            emit selected(m_candidate);
        }
    });
    frameLayout->addWidget(m_selectButton);

    layout->addWidget(m_cardFrame);
}

void CandidateCard::createShadowEffect() {
    m_shadowEffect = new QGraphicsDropShadowEffect(this);
    m_shadowEffect->setBlurRadius(20);
    m_shadowEffect->setColor(QColor(0, 0, 0, 40));
    m_shadowEffect->setOffset(0, 4);
    m_cardFrame->setGraphicsEffect(m_shadowEffect);
}

void CandidateCard::setCandidate(const Core::Candidate& candidate) {
    m_candidate = candidate;
    updateUi();
}

void CandidateCard::setSelected(bool selected) {
    m_selected = selected;
    updateUi();
}

void CandidateCard::setSelectable(bool selectable) {
    m_selectable = selectable;
    m_selectButton->setEnabled(selectable);
    m_selectButton->setVisible(selectable);
    setCursor(selectable ? Qt::PointingHandCursor : Qt::ArrowCursor);
}

void CandidateCard::applyTheme(const Core::Models::ThemeColors& colors) {
    m_colors = colors;
    
    QString frameStyle = QString(
        "QFrame#candidateCardFrame {"
        "    background-color: %1;"
        "    border: 2px solid %2;"
        "    border-radius: 16px;"
        "}"
        "QFrame#candidateCardFrame:hover {"
        "    border-color: %3;"
        "    background-color: %4;"
        "}"
        "QFrame#candidateCardFrame[selected=\"true\"] {"
        "    border-color: %3;"
        "    background-color: %4;"
        "}"
    ).arg(
        colors.surface,
        colors.outline,
        colors.accent,
        QString("rgba(%1, %2, %3, 0.08)").arg(colors.accent.mid(1,2).toInt(nullptr,16))
                                        .arg(colors.accent.mid(3,2).toInt(nullptr,16))
                                        .arg(colors.accent.mid(5,2).toInt(nullptr,16))
    );
    
    m_cardFrame->setProperty("selected", m_selected);
    m_cardFrame->setStyleSheet(frameStyle);
    m_cardFrame->style()->unpolish(m_cardFrame);
    m_cardFrame->style()->polish(m_cardFrame);

    m_nameLabel->setStyleSheet(QString("color: %1; background: transparent;").arg(colors.onSurface));
    m_partyLabel->setStyleSheet(QString("color: %1; background: transparent;").arg(colors.onSurface));
    m_manifestoLabel->setStyleSheet(QString("color: %1; background: transparent;").arg(colors.onSurfaceVariant));
    
    m_photoLabel->setStyleSheet(QString("border-radius: 60px; background-color: %1;").arg(colors.surfaceVariant));
    
    QString btnStyle = QString(
        "QPushButton {"
        "    background-color: %1;"
        "    color: %2;"
        "    font-size: %3px;"
        "    font-weight: 600;"
        "    border-radius: 8px;"
        "    padding: 10px;"
        "}"
        "QPushButton:hover {"
        "    background-color: %4;"
        "}"
        "QPushButton:disabled {"
        "    background-color: %5;"
        "    color: %6;"
        "}"
    ).arg(colors.accent, colors.onPrimary, QString::number(m_baseFontSize + 2))
     .arg(colors.secondary)
     .arg(colors.outline)
     .arg(colors.onSurfaceVariant);
    
    m_selectButton->setStyleSheet(btnStyle);
}

void CandidateCard::applyFont(const QString& family, int baseSize) {
    m_fontFamily = family;
    m_baseFontSize = baseSize;
    
    QFont nameFont(family, baseSize + 6, QFont::DemiBold);
    QFont partyFont(family, baseSize - 1);
    QFont manifestoFont(family, baseSize - 2);
    QFont btnFont(family, baseSize + 2, QFont::DemiBold);
    
    m_nameLabel->setFont(nameFont);
    m_partyLabel->setFont(partyFont);
    m_manifestoLabel->setFont(manifestoFont);
    m_selectButton->setFont(btnFont);
}

void CandidateCard::updateUi() {
    // Photo
    if (!m_candidate.photoData.isEmpty()) {
        QPixmap pixmap;
        if (pixmap.loadFromData(m_candidate.photoData)) {
            m_photoLabel->setPixmap(pixmap.scaled(120, 120, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        } else {
            m_photoLabel->setText("👤");
            m_photoLabel->setStyleSheet("font-size: 48px; color: #9a9ab0; border-radius: 60px; background-color: #1a1a2e;");
        }
    } else {
        m_photoLabel->setText("👤");
        m_photoLabel->setStyleSheet("font-size: 48px; color: #9a9ab0; border-radius: 60px; background-color: #1a1a2e;");
    }

    m_nameLabel->setText(m_candidate.name);
    m_partyLabel->setText(m_candidate.party);
    m_partyLabel->setVisible(m_showParty && !m_candidate.party.isEmpty());
    
    updateManifesto();
    
    m_cardFrame->setProperty("selected", m_selected);
    m_cardFrame->style()->unpolish(m_cardFrame);
    m_cardFrame->style()->polish(m_cardFrame);
}

void CandidateCard::updateManifesto() {
    if (m_showManifesto && !m_candidate.manifesto.isEmpty()) {
        QString text = m_candidate.manifesto;
        if (text.length() > 150) {
            text = text.left(147) + "...";
        }
        m_manifestoLabel->setText(text);
        m_manifestoLabel->setVisible(true);
    } else {
        m_manifestoLabel->setVisible(false);
    }
}

void CandidateCard::enterEvent(QEnterEvent* event) {
    QWidget::enterEvent(event);
    if (m_selectable && !m_showManifesto && !m_candidate.manifesto.isEmpty()) {
        m_showManifesto = true;
        updateManifesto();
    }
}

void CandidateCard::leaveEvent(QEvent* event) {
    QWidget::leaveEvent(event);
    if (m_selectable && m_showManifesto) {
        m_showManifesto = false;
        updateManifesto();
    }
}

void CandidateCard::mousePressEvent(QMouseEvent* event) {
    if (m_selectable && event->button() == Qt::LeftButton) {
        m_selected = true;
        updateUi();
        emit selected(m_candidate);
    }
    QWidget::mousePressEvent(event);
}

} // namespace Ballot::UI