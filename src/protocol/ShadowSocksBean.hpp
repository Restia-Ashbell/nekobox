#pragma once

#include "protocol/AbstractBean.hpp"

namespace NekoGui_fmt {
    class ShadowSocksBean : public AbstractBean {
        Q_OBJECT
        Q_PROPERTY(QString method MEMBER method)
        Q_PROPERTY(QString pass MEMBER password)
        Q_PROPERTY(QString plugin MEMBER plugin)
        Q_PROPERTY(int uot MEMBER uot)
        Q_PROPERTY(MultiplexSettings *multiplex MEMBER multiplex)

    public:
        QString method = "aes-128-gcm";
        QString password = "";
        QString plugin = "";
        int uot = 0;

        MultiplexSettings *multiplex = nullptr;

        ShadowSocksBean() : AbstractBean(0) { multiplex = new MultiplexSettings(this); }

        QString DisplayType() override { return "Shadowsocks"; };

        CoreObjOutboundBuildResult BuildCoreObjSingBox() override;

        bool TryParseLink(const QString &link) override;

        QString ToShareLink() override;
    };
} // namespace NekoGui_fmt
