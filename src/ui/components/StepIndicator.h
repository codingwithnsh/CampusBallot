#pragma once

#include <QWidget>
#include <QHBoxLayout>
#include <QFrame>
#include <QLabel>
#include <QPropertyAnimation>
#include <QParallelAnimationGroup>
#include "src/core/models/ElectionConfiguration.h"

namespace Ballot::UI {

class StepIndicator : public QWidget {
    Q_OBJECT
public:
    explicit StepIndicator(int totalSteps = 4, QWidget* parent = nullptr);
    ~StepIndicator() override = default;

    void setCurrentStep(int step);
    void setTotalSteps(int total);
    int currentStep() const { return m_currentStep; }
    int totalSteps() const { return m_totalSteps; }

    void applyTheme(const Core::Models::ThemeColors& colors);
    void applyFont(const QString& family, int baseSize);

    void setShowLabels(bool show) { m_showLabels = show; updateLabels(); }
    void setAnimated(bool animated) { m_animated = animated; }

signals:
    void stepChanged(int step);

private:
    void setupUi();
    void updateUi();
    void updateLabels();
    void animateStepChange(int fromStep, int toStep);

    int m_currentStep = 1;
    int m_totalSteps = 4;
    bool m_showLabels = true;
    bool m_animated = true;

    QHBoxLayout* m_layout = nullptr;
    QList<QFrame*> m_dots;
    QList<QFrame*> m_lines;
    QList<QLabel*> m_labels;
    QLabel* m_stepLabel = nullptr;

    Core::Models::ThemeColors m_colors;
    QString m_fontFamily = "Segoe UI";
    int m_baseFontSize = 14;
};

} // namespace Ballot::UI