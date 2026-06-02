#pragma once

#include "fmt/AbstractBean.hpp"

namespace NekoGui_fmt {
    class CustomBean : public AbstractBean {
    public:
        QString core;
        QList<QString> command;
        QString config_suffix;
        QString config_simple;
        int mapping_port = 0;
        int socks_port = 0;

        CustomBean() : AbstractBean(0) {
            _add("core", &core);
            _add("cmd", &command);
            _add("cs", &config_simple);
            _add("cs_suffix", &config_suffix);
            _add("mapping_port", &mapping_port);
            _add("socks_port", &socks_port);
        };

        QString DisplayType() override {
            if (core == "internal") {
                auto obj = QString2QJsonObject(config_simple);
                return obj["type"].toString();
            } else if (core == "internal-full") {
                return software_core_name + " config";
            }
            return core;
        };

        QString DisplayCoreType() override { return NeedExternal(true) == 0 ? software_core_name : core; };

        QString DisplayAddress() override {
            if (core == "internal") {
                auto obj = QString2QJsonObject(config_simple);
                return MakeHostPort(obj["server"].toString(), obj["server_port"].toInt());
            } else if (core == "internal-full") {
                return {};
            }
            return AbstractBean::DisplayAddress();
        };

        int NeedExternal(bool isFirstProfile) override;

        ExternalBuildResult BuildExternal(int mapping_port, int socks_port, int external_stat) override;

        CoreObjOutboundBuildResult BuildCoreObjSingBox() override;
    };
} // namespace NekoGui_fmt