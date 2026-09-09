#pragma once

#include "protocol/AbstractBean.hpp"

namespace NekoGui_fmt {
    class SocksHttpBean : public AbstractBean {
        Q_OBJECT
        Q_PROPERTY(int v MEMBER socks_http_type)
        Q_PROPERTY(QString username MEMBER username)
        Q_PROPERTY(QString password MEMBER password)
        Q_PROPERTY(V2rayStreamSettings *stream MEMBER stream)

    public:
        static constexpr int type_HTTP = -80;
        static constexpr int type_Socks4 = 4;
        static constexpr int type_Socks5 = 5;
        int socks_http_type = type_Socks5;
        QString username = "";
        QString password = "";

        V2rayStreamSettings *stream = nullptr;

        explicit SocksHttpBean(int _socks_http_type) : AbstractBean(0), socks_http_type(_socks_http_type) {
            stream = new V2rayStreamSettings(this);
        }

        QString DisplayType() override { return socks_http_type == type_HTTP ? "HTTP" : "Socks"; };

        CoreObjOutboundBuildResult BuildCoreObjSingBox() override;

        bool TryParseLink(const QString &link) override;

        QString ToShareLink() override;
    };
} // namespace NekoGui_fmt
