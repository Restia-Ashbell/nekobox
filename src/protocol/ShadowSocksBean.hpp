#pragma once

#include "protocol/AbstractBean.hpp"

namespace NekoGui_fmt {
    class ShadowSocksBean : public AbstractBean {
    public:
        QString method = "aes-128-gcm";
        QString password = "";
        QString plugin = "";
        int uot = 0;

        MultiplexSettings multiplex;

        ShadowSocksBean() : AbstractBean(0) {
            _add("method", &method);
            _add("pass", &password);
            _add("plugin", &plugin);
            _add("uot", &uot);
            _add("multiplex", dynamic_cast<JsonStore *>(&multiplex));
        };

        QString DisplayType() override { return "Shadowsocks"; };

        CoreObjOutboundBuildResult BuildCoreObjSingBox() override;

        bool TryParseLink(const QString &link) override;

        QString ToShareLink() override;
    };
} // namespace NekoGui_fmt
