#pragma once

#include "protocol/AbstractBean.hpp"

namespace NekoGui_fmt {
    class TrojanVLESSBean : public AbstractBean {
        Q_OBJECT
        Q_PROPERTY(QString pass MEMBER password)
        Q_PROPERTY(QString flow MEMBER flow)
        Q_PROPERTY(V2rayStreamSettings *stream MEMBER stream)
        Q_PROPERTY(MultiplexSettings *multiplex MEMBER multiplex)

    public:
        static constexpr int proxy_Trojan = 0;
        static constexpr int proxy_VLESS = 1;
        int proxy_type = proxy_Trojan;

        QString password = "";
        QString flow = "";

        V2rayStreamSettings *stream = nullptr;
        MultiplexSettings *multiplex = nullptr;

        explicit TrojanVLESSBean(int _proxy_type) : AbstractBean(0), proxy_type(_proxy_type) {
            stream = new V2rayStreamSettings(this);
            multiplex = new MultiplexSettings(this);
        }

        QString DisplayType() override { return proxy_type == proxy_VLESS ? "VLESS" : "Trojan"; };

        CoreObjOutboundBuildResult BuildCoreObjSingBox() override;

        bool TryParseLink(const QString &link) override;

        QString ToShareLink() override;
    };
} // namespace NekoGui_fmt