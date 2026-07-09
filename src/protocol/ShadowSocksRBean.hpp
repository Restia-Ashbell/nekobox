#pragma once

#include "protocol/AbstractBean.hpp"

namespace NekoGui_fmt {
    class ShadowSocksRBean : public AbstractBean {
    public:
        QString method = "none";
        QString password = "";
        QString obfs = "plain";
        QString obfsParam = "";
        QString protocol = "origin";
        QString protocolParam = "";

        ShadowSocksRBean() : AbstractBean(0) {
            _add("method", &method);
            _add("pass", &password);
            _add("obfs", &obfs);
            _add("obfs_param", &obfsParam);
            _add("protocol", &protocol);
            _add("protocol_param", &protocolParam);
        };

        QString DisplayType() override { return "ShadowsocksR"; };

        CoreObjOutboundBuildResult BuildCoreObjSingBox() override;

        bool TryParseLink(const QString &link) override;

        QString ToShareLink() override;
    };
} // namespace NekoGui_fmt
