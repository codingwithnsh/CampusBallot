#pragma once

#include <QString>
#include <QVariantMap>
#include <QJsonObject>

namespace Ballot::Plugin {

class IPlugin {
public:
    virtual ~IPlugin() = default;

    virtual QString id() const = 0;
    virtual QString name() const = 0;
    virtual QString version() const = 0;
    virtual QString description() const = 0;
    virtual QString vendor() const = 0;

    virtual bool initialize(const QVariantMap& config) = 0;
    virtual bool shutdown() = 0;
    virtual bool isInitialized() const = 0;

    virtual QJsonObject capabilities() const = 0;
    virtual bool hasCapability(const QString& capability) const = 0;

    virtual bool execute(const QString& command, const QVariantMap& params = {}) = 0;
};

#define BALLOT_PLUGIN_INTERFACE "Ballot.Plugin.Interface/1.0"

} // namespace Ballot::Plugin

Q_DECLARE_INTERFACE(Ballot::Plugin::IPlugin, "Ballot.Plugin.Interface/1.0")
