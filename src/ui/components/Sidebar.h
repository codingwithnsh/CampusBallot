#pragma once

#include <QWidget>
#include <QVBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QPropertyAnimation>
#include <QHash>
#include <QScrollArea>

namespace Ballot::UI {

class SidebarButton : public QPushButton {
    Q_OBJECT
    Q_PROPERTY(bool active READ isActive WRITE setActive)
public:
    explicit SidebarButton(const QString& text, const QString& iconChar = "",
                           QWidget* parent = nullptr);
    bool isActive() const { return m_active; }
    void setActive(bool active);
    void setBadge(int count);

protected:
    void paintEvent(QPaintEvent* event) override;
private:
    bool m_active = false;
    QLabel* m_badge;
    QString m_iconChar;
};

class Sidebar : public QWidget {
    Q_OBJECT
public:
    explicit Sidebar(QWidget* parent = nullptr);

    void addItem(const QString& id, const QString& text, const QString& icon = "");
    void setActiveItem(const QString& id);
    void setItemBadge(const QString& id, int count);
    void setItemVisible(const QString& id, bool visible);
    void setUserInfo(const QString& name, const QString& role, const QPixmap& avatar = QPixmap());

signals:
    void itemClicked(const QString& id);

private:
    QVBoxLayout* m_itemsLayout;
    QHash<QString, SidebarButton*> m_buttons;
    QLabel* m_userName;
    QLabel* m_userRole;
    QLabel* m_avatar;
    QString m_activeId;
};

} // namespace Ballot::UI
