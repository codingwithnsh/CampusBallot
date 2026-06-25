#pragma once

#include <QFrame>
#include <QLabel>
#include <QPropertyAnimation>
#include <QVBoxLayout>

namespace Ballot::UI {

class StatCard : public QFrame {
    Q_OBJECT
    Q_PROPERTY(int displayValue READ displayValue WRITE setDisplayValue)

public:
    explicit StatCard(const QString& title, const QString& value,
                      const QString& accentColor = "#0078d4",
                      const QString& icon = "",
                      QWidget* parent = nullptr);

    void setValue(const QString& value);
    void setAccentColor(const QString& color);
    void animateValue(int targetValue, int duration = 500);
    void setIcon(const QString& iconPath);

    int displayValue() const { return m_displayValue; }
    void setDisplayValue(int value);

private:
    QLabel* m_titleLabel;
    QLabel* m_valueLabel;
    QLabel* m_iconLabel;
    QFrame* m_accentBar;
    QString m_accentColor;
    int m_displayValue = 0;
    QPropertyAnimation* m_valueAnim;
};

} // namespace Ballot::UI
