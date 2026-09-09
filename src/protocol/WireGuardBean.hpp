#pragma once

#include "protocol/AbstractBean.hpp"

namespace NekoGui_fmt {
    class WireGuardBean : public AbstractBean {
        Q_OBJECT
        Q_PROPERTY(QString private_key MEMBER privateKey)
        Q_PROPERTY(QString public_key MEMBER publicKey)
        Q_PROPERTY(QString pre_shared_key MEMBER preSharedKey)
        Q_PROPERTY(QString local_address MEMBER localAddress)
        Q_PROPERTY(QString reserved MEMBER reserved)
        Q_PROPERTY(int mtu MEMBER MTU)
        Q_PROPERTY(bool use_system_proxy MEMBER useSystemInterface)

    public:
        QString privateKey;
        QString publicKey;
        QString preSharedKey;
        QString localAddress;
        QString reserved;
        int MTU = 1408;
        bool useSystemInterface = false;

        WireGuardBean() : AbstractBean(0) {}

        QString DisplayType() override { return "WireGuard"; };

        CoreObjOutboundBuildResult BuildCoreObjSingBox() override;

        bool TryParseLink(const QString &link) override;

        QString ToShareLink() override;
    };
} // namespace NekoGui_fmt
