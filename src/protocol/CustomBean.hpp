#pragma once

#include "protocol/AbstractBean.hpp"

namespace NekoGui_fmt {
    class CustomBean : public AbstractBean {
        Q_OBJECT
        Q_PROPERTY(QString core MEMBER core)
        Q_PROPERTY(QStringList cmd MEMBER command)
        Q_PROPERTY(QString cs MEMBER config_simple)
        Q_PROPERTY(QString cs_suffix MEMBER config_suffix)

    public:
        QString core;
        QList<QString> command;
        QString config_suffix;
        QString config_simple;

        CustomBean() : AbstractBean(0) {}

        QString DisplayType() override {
            if (core == "internal") {
                auto obj = QString2QJsonObject(config_simple);
                return obj["type"].toString();
            } else if (core == "internal-full") {
                return software_core_name + " config";
            }
            return core;
        }

        QString DisplayCoreType() override {
            return core == "internal" || core == "internal-full" ? software_core_name : core;
        }

        QString DisplayAddress() override {
            if (core == "internal") {
                auto obj = QString2QJsonObject(config_simple);
                return MakeHostPort(obj["server"].toString(), obj["server_port"].toInt());
            } else if (core == "internal-full") {
                return {};
            }
            return AbstractBean::DisplayAddress();
        }

        ExternalBuildResult BuildExternal(int mapping_port, int socks_port) override;

        CoreObjOutboundBuildResult BuildCoreObjSingBox() override;
    };
} // namespace NekoGui_fmt