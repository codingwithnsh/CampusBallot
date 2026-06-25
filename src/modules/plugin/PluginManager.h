#pragma once

#include <QObject>
#include <QDir>
#include <QVector>
#include <QPluginLoader>
#include "IPlugin.h"

namespace Ballot::Plugin {

class PluginManager : public QObject {
    Q_OBJECT
public:
    static PluginManager& instance();

    bool loadAll(const QString& pluginDir = QString());
    bool load(const QString& path);
    bool unload(const QString& pluginId);
    bool unloadAll();

    IPlugin* getPlugin(const QString& pluginId) const;
    QList<IPlugin*> getPlugins() const;
    QList<IPlugin*> getPluginsByCapability(const QString& capability) const;
    QStringList loadedPluginIds() const;
    int pluginCount() const;

    bool hasPlugin(const QString& pluginId) const;

signals:
    void pluginLoaded(const QString& pluginId, const QString& name);
    void pluginUnloaded(const QString& pluginId);
    void pluginError(const QString& pluginId, const QString& error);

private:
    PluginManager();
    QHash<QString, IPlugin*> m_plugins;
    QHash<QString, QPluginLoader*> m_loaders;
    QDir m_pluginDir;
};

} // namespace Ballot::Plugin
