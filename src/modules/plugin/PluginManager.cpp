#include "PluginManager.h"
#include <QDir>
#include <QLibrary>
#include <QDebug>
#include <QCoreApplication>

namespace Ballot::Plugin {

PluginManager& PluginManager::instance() {
    static PluginManager inst;
    return inst;
}

PluginManager::PluginManager() {
    m_pluginDir = QDir(QCoreApplication::applicationDirPath() + "/plugins");
}

bool PluginManager::loadAll(const QString& pluginDir) {
    if (!pluginDir.isEmpty()) {
        m_pluginDir = QDir(pluginDir);
    }

    if (!m_pluginDir.exists()) {
        m_pluginDir.mkpath(".");
        return false;
    }

    bool anyLoaded = false;
    const auto entries = m_pluginDir.entryInfoList(QDir::Files | QDir::NoDotAndDotDot);
    for (const auto& entry : entries) {
        if (QLibrary::isLibrary(entry.fileName())) {
            if (load(entry.absoluteFilePath())) {
                anyLoaded = true;
            }
        }
    }
    return anyLoaded;
}

bool PluginManager::load(const QString& path) {
    auto* loader = new QPluginLoader(path, this);
    QObject* instance = loader->instance();
    if (!instance) {
        emit pluginError(path, loader->errorString());
        delete loader;
        return false;
    }

    auto* plugin = qobject_cast<IPlugin*>(instance);
    if (!plugin) {
        delete loader;
        return false;
    }

    if (!plugin->initialize({})) {
        delete loader;
        return false;
    }

    m_plugins[plugin->id()] = plugin;
    m_loaders[plugin->id()] = loader;
    emit pluginLoaded(plugin->id(), plugin->name());
    return true;
}

bool PluginManager::unload(const QString& pluginId) {
    if (!m_plugins.contains(pluginId)) return false;

    auto* plugin = m_plugins[pluginId];
    plugin->shutdown();

    auto* loader = m_loaders[pluginId];
    loader->unload();

    m_plugins.remove(pluginId);
    m_loaders.remove(pluginId);
    delete loader;

    emit pluginUnloaded(pluginId);
    return true;
}

bool PluginManager::unloadAll() {
    auto ids = m_plugins.keys();
    for (const auto& id : ids) {
        unload(id);
    }
    return true;
}

IPlugin* PluginManager::getPlugin(const QString& pluginId) const {
    return m_plugins.value(pluginId, nullptr);
}

QList<IPlugin*> PluginManager::getPlugins() const {
    return m_plugins.values();
}

QList<IPlugin*> PluginManager::getPluginsByCapability(const QString& capability) const {
    QList<IPlugin*> result;
    for (auto* plugin : m_plugins) {
        if (plugin->hasCapability(capability)) {
            result.append(plugin);
        }
    }
    return result;
}

QStringList PluginManager::loadedPluginIds() const {
    return m_plugins.keys();
}

int PluginManager::pluginCount() const {
    return m_plugins.size();
}

bool PluginManager::hasPlugin(const QString& pluginId) const {
    return m_plugins.contains(pluginId);
}

} // namespace Ballot::Plugin
