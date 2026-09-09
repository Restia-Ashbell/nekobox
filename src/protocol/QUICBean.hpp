#pragma once

#include "protocol/AbstractBean.hpp"

namespace NekoGui_fmt {
    class QUICBean : public AbstractBean {
        Q_OBJECT
        // Hysteria 1
        Q_PROPERTY(QString protocol MEMBER protocol)
        Q_PROPERTY(QString auth_str MEMBER auth_str)

        // Hysteria 1&2
        Q_PROPERTY(QString obfsPassword MEMBER obfsPassword)
        Q_PROPERTY(int uploadMbps MEMBER uploadMbps)
        Q_PROPERTY(int downloadMbps MEMBER downloadMbps)
        Q_PROPERTY(qint64 streamReceiveWindow MEMBER streamReceiveWindow)
        Q_PROPERTY(qint64 connectionReceiveWindow MEMBER connectionReceiveWindow)
        Q_PROPERTY(bool disableMtuDiscovery MEMBER disableMtuDiscovery)
        Q_PROPERTY(int hopInterval MEMBER hopInterval)
        Q_PROPERTY(QString hopPort MEMBER hopPort)

        // TUIC
        Q_PROPERTY(QString uuid MEMBER uuid)
        Q_PROPERTY(QString congestionControl MEMBER congestionControl)
        Q_PROPERTY(QString udpRelayMode MEMBER udpRelayMode)
        Q_PROPERTY(bool zeroRttHandshake MEMBER zeroRttHandshake)
        Q_PROPERTY(QString heartbeat MEMBER heartbeat)
        Q_PROPERTY(bool uos MEMBER uos)

        // HY2&TUIC
        Q_PROPERTY(QString password MEMBER password)

        // TLS
        Q_PROPERTY(bool allowInsecure MEMBER allowInsecure)
        Q_PROPERTY(QString sni MEMBER sni)
        Q_PROPERTY(QString alpn MEMBER alpn)
        Q_PROPERTY(QString caText MEMBER caText)
        Q_PROPERTY(bool disableSni MEMBER disableSni)

    public:
        static constexpr int proxy_Hysteria = 0;
        static constexpr int proxy_TUIC = 1;
        static constexpr int proxy_Hysteria2 = 3;

        int proxy_type = proxy_Hysteria;

        // Hysteria 1

        QString protocol = "udp";
        QString auth_str = "";

        // Hysteria 1&2

        QString obfsPassword = "";

        int uploadMbps = 100;
        int downloadMbps = 100;

        qint64 streamReceiveWindow = 0;
        qint64 connectionReceiveWindow = 0;
        bool disableMtuDiscovery = false;

        int hopInterval = 30;
        QString hopPort = "";

        // TUIC

        QString uuid = "";
        QString congestionControl = "bbr";
        QString udpRelayMode = "native";
        bool zeroRttHandshake = false;
        QString heartbeat = "10s";
        bool uos = false;

        // HY2&TUIC

        QString password = "";

        // TLS

        bool allowInsecure = false;
        QString sni = "";
        QString alpn = "";
        QString caText = "";
        bool disableSni = false;

        explicit QUICBean(int _proxy_type) : AbstractBean(0), proxy_type(_proxy_type) {
            serverPort = 443;
            if (proxy_type == proxy_Hysteria2) {
                uploadMbps = 0;
                downloadMbps = 0;
            }
        }

        QString DisplayAddress() override {
            return !hopPort.trimmed().isEmpty() ? WrapIPV6Host(serverAddress) + ":" + hopPort : MakeHostPort(serverAddress, serverPort);
        }

        QString DisplayCoreType() override {
            if (!external) {
                return software_core_name;
            } else if (proxy_type == proxy_TUIC) {
                return "tuic";
            } else if (proxy_type == proxy_Hysteria) {
                return "hysteria";
            } else {
                return "hysteria2";
            }
        }

        QString DisplayType() override {
            if (proxy_type == proxy_TUIC) {
                return "TUIC";
            } else if (proxy_type == proxy_Hysteria) {
                return "Hysteria1";
            } else {
                return "Hysteria2";
            }
        }

        ExternalBuildResult BuildExternal(int mapping_port, int socks_port) override;

        CoreObjOutboundBuildResult BuildCoreObjSingBox() override;

        bool TryParseLink(const QString &link) override;

        QString ToShareLink() override;
    };
} // namespace NekoGui_fmt