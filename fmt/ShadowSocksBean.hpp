#pragma once

#include "fmt/AbstractBean.hpp"

namespace NekoGui_fmt {
    class ShadowSocksBean : public AbstractBean {
    public:
        QString method = "aes-128-gcm";
        QString password = "";
        QString plugin = "";
        int uot = 0;

        MultiplexSettings multiplex;

        ShadowSocksBean() : AbstractBean(0) {
            _add(new configItem("method", &method, itemType::string));
            _add(new configItem("pass", &password, itemType::string));
            _add(new configItem("plugin", &plugin, itemType::string));
            _add(new configItem("uot", &uot, itemType::integer));
            _add(new configItem("multiplex", &multiplex, itemType::jsonStore));
        };

        QString DisplayType() override { return "Shadowsocks"; };

        CoreObjOutboundBuildResult BuildCoreObjSingBox() override;

        bool TryParseLink(const QString &link) override;

        QString ToShareLink() override;
    };
} // namespace NekoGui_fmt
