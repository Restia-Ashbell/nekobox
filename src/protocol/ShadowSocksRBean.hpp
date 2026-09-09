#pragma once

#include "protocol/AbstractBean.hpp"

namespace NekoGui_fmt {
    class ShadowSocksRBean : public AbstractBean {
        Q_OBJECT
        Q_PROPERTY(QString method MEMBER method)
        Q_PROPERTY(QString pass MEMBER password)
        Q_PROPERTY(QString obfs MEMBER obfs)
        Q_PROPERTY(QString obfs_param MEMBER obfsParam)
        Q_PROPERTY(QString protocol MEMBER protocol)
        Q_PROPERTY(QString protocol_param MEMBER protocolParam)

    public:
        QString method = "none";
        QString password = "";
        QString obfs = "plain";
        QString obfsParam = "";
        QString protocol = "origin";
        QString protocolParam = "";

        ShadowSocksRBean() : AbstractBean(0) {}

        QString DisplayType() override { return "ShadowsocksR"; };

        CoreObjOutboundBuildResult BuildCoreObjSingBox() override;

        bool TryParseLink(const QString &link) override;

        QString ToShareLink() override;
    };
} // namespace NekoGui_fmt
