#pragma once

#include "fmt/AbstractBean.hpp"

namespace NekoGui_fmt {
    class WireGuardBean : public AbstractBean {
    public:
        QString privateKey;
        QString publicKey;
        QString preSharedKey;
        QString localAddress;
        QString reserved;
        int MTU = 1408;
        bool useSystemInterface = false;

        WireGuardBean() : AbstractBean(0) {
            _add("private_key", &privateKey);
            _add("public_key", &publicKey);
            _add("pre_shared_key", &preSharedKey);
            _add("local_address", &localAddress);
            _add("reserved", &reserved);
            _add("mtu", &MTU);
            _add("use_system_proxy", &useSystemInterface);
        };

        QString DisplayType() override { return "WireGuard"; };

        CoreObjOutboundBuildResult BuildCoreObjSingBox() override;

        bool TryParseLink(const QString &link) override;

        QString ToShareLink() override;
    };
} // namespace NekoGui_fmt
