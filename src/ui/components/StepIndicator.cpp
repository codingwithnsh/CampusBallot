#include "StepIndicator.h"
#include <QPropertyAnimation>
#include <QParallelAnimationGroup>

namespace Ballot::UI {

StepIndicator::StepIndicator(int totalSteps, QWidget* parent)
    : QWidget(parent)
    , m_totalSteps(totalSteps)
    , m_layout(nullptr)
{
    setupUi();
}

void StepIndicator::setupUi() {
    m_layout = new QHBoxLayout(this);
    m_layout->setAlignment(Qt::AlignCenter);
    m_layout->setSpacing(8);
    m_layout->setContentsMargins(0, 0, 0, 0);

    // Create dots and lines
    for (int i = 1; i <= m_totalSteps; ++i) {
        auto* dot = new QFrame(this);
        dot->setFixedSize(14, 14);
        dot->setObjectName(QString("stepDot%1").arg(i));
        m_dots.append(dot);
        m_layout->addWidget(dot);

        if (i < m_totalSteps) {
            auto* line = new QFrame(this);
            line->setFixedSize(40, 3);
            line->setObjectName(QString("stepLine%1").arg(i));
            m_lines.append(line);
            m_layout->addWidget(line);
        }

        auto* label = new QLabel(QString::number(i), this);
        label->setAlignment(Qt::AlignCenter);
        label->setFixedSize(24, 24);
        label->setVisible(false);
        label->setObjectName(QString("stepLabel%1").arg(i));
        m_labels.append(label);
    }

    m_stepLabel = new QLabel(this);
    m_stepLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    m_stepLabel->setVisible(m_showLabels);
    m_layout->addSpacing(12);
    m_layout->addWidget(m_stepLabel);

    updateUi();
}

void StepIndicator::setCurrentStep(int step) {
    if (step < 1 || step > m_totalSteps || step == m_currentStep) return;
    
    int oldStep = m_currentStep;
    m_currentStep = step;
    
    if (m_animated) {
        animateStepChange(oldStep, step);
    } else {
        updateUi();
    }
    
    emit stepChanged(step);
}

void StepIndicator::setTotalSteps(int total) {
    if (total < 1 || total == m_totalSteps) return;
    
    // Clear existing
    qDeleteAll(m_dots);
    qDeleteAll(m_lines);
    qDeleteAll(m_labels);
    m_dots.clear();
    m_lines.clear();
    m_labels.clear();
    
    m_totalSteps = total;
    m_currentStep = qMin(m_currentStep, m_totalSteps);
    
    // Recreate
    QLayoutItem* item;
    while ((item = m_layout->takeAt(0)) != nullptr) {
        delete item->widget();
        delete item;
    }
    
    setupUi();
}

void StepIndicator::applyTheme(const Core::Models::ThemeColors& colors) {
    m_colors = colors;
    updateUi();
}

void StepIndicator::applyFont(const QString& family, int baseSize) {
    m_fontFamily = family;
    m_baseFontSize = baseSize;
    
    QFont font(family, baseSize, QFont::Medium);
    m_stepLabel->setFont(font);
    
    for (auto* label : m_labels) {
        QFont labelFont(family, baseSize - 2, QFont::DemiBold);
        label->setFont(labelFont);
    }
}

void StepIndicator::updateUi() {
    for (int i = 0; i < m_totalSteps; ++i) {
        bool isActive = (i + 1) <= m_currentStep;
        bool isCurrent = (i + 1) == m_currentStep;
        
        // Update dot
        if (i < m_dots.size()) {
            auto* dot = m_dots[i];
            QString style = QString(
                "QFrame {"
                "    background-color: %1;"
                "    border-radius: 7px;"
                "    border: 3px solid %2;"
                "}"
            ).arg(
                isActive ? m_colors.accent : m_colors.surfaceVariant,
                isActive ? m_colors.accent : m_colors.outline
            );
            dot->setStyleSheet(style);
        }
        
        // Update line
        if (i < m_lines.size()) {
            auto* line = m_lines[i];
            QString style = QString(
                "QFrame {"
                "    background-color: %1;"
                "    border-radius: 1.5px;"
                "}"
            ).arg((i + 1) < m_currentStep ? m_colors.accent : m_colors.outline);
            line->setStyleSheet(style);
        }
        
        // Update label
        if (i < m_labels.size()) {
            auto* label = m_labels[i];
            label->setStyleSheet(QString(
                "QLabel {"
                "    color: %1;"
                "    background: transparent;"
                "    border-radius: 12px;"
                "}"
            ).arg(isActive ? m_colors.onPrimary : m_colors.onSurfaceVariant));
        }
    }
    
    updateLabels();
}

void StepIndicator::updateLabels() {
    if (m_stepLabel) {
        m_stepLabel->setVisible(m_showLabels);
        if (m_showLabels) {
            m_stepLabel->setText(QString("Step %1 of %2").arg(m_currentStep).arg(m_totalSteps));
            m_stepLabel->setStyleSheet(QString("color: %1; background: transparent;").arg(m_colors.accent));
        }
    }
    
    for (int i = 0; i < m_labels.size(); ++i) {
        m_labels[i]->setVisible(!m_showLabels && (i + 1) == m_currentStep);
    }
}

void StepIndicator::animateStepChange(int fromStep, int toStep) {
    // Simple color animation using property animation
    // For now, just update immediately with a subtle effect
    updateUi();
    
    // Could add QPropertyAnimation for dot scale/color transition here
    // This would require a custom property on the dot frames
}

} // namespace Ballot::UI