#pragma once

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QGraphicsDropShadowEffect>
#include "src/core/Models.h" // ThemeColors is now defined here
// Removed: #include "src/core/models/ThemeConfig.h"

namespace Ballot::UI {

class CandidateCard : public QWidget {
    Q_OBJECT
public:
    explicit CandidateCard(const Core::Candidate& candidate, QWidget* parent = nullptr);
    ~CandidateCard() override = default;

    void setCandidate(const Core::Candidate& candidate);
    const Core::Candidate& candidate() const { return m_candidate; }
    void setSelected(bool selected);
    void setSelectable(bool selectable);
    void setShowParty(bool show) { m_showParty = show; m_partyLabel->setVisible(show); }
    void setShowManifesto(bool show) { m_showManifesto = show; updateManifesto(); }
    void applyTheme(const Core::ThemeColors& colors); // Changed to Core::ThemeColors
    void applyFont(const QString& family, int baseSize);

signals:
    void selected(const Core::Candidate& candidate);
    void previewRequested(const Core::Candidate& candidate);

protected:
    void enterEvent(QEnterEvent* event) override;
    void leaveEvent(QEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;

private:
    void setupUi();
    void updateUi();
    void updateManifesto();
    void createShadowEffect();

    Core::Candidate m_candidate;
    bool m_selected = false;
    bool m_selectable = true;
    bool m_showParty = true;
    bool m_showManifesto = false;

    QLabel* m_photoLabel = nullptr;
    QLabel* m_nameLabel = nullptr;
    QLabel* m_partyLabel = nullptr;
    QLabel* m_manifestoLabel = nullptr;
    QPushButton* m_selectButton = nullptr;
    QFrame* m_cardFrame = nullptr;
    QGraphicsDropShadowEffect* m_shadowEffect = nullptr;

    Core::ThemeColors m_colors; // Changed to Core::ThemeColors
    QString m_fontFamily = "Segoe UI";
    int m_baseFontSize = 14;
};

} // namespace Ballot::UI