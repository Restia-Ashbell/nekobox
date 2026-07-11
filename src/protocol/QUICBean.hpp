#pragma once

#include "protocol/AbstractBean.hpp"

namespace NekoGui_fmt {
    class QUICBean : public AbstractBean {
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
            if (proxy_type == proxy_Hysteria || proxy_type == proxy_Hysteria2) {
                _add("obfsPassword", &obfsPassword);
                _add("uploadMbps", &uploadMbps);
                _add("downloadMbps", &downloadMbps);
                _add("streamReceiveWindow", &streamReceiveWindow);
                _add("connectionReceiveWindow", &connectionReceiveWindow);
                _add("disableMtuDiscovery", &disableMtuDiscovery);
                _add("hopInterval", &hopInterval);
                _add("hopPort", &hopPort);
                if (proxy_type == proxy_Hysteria) { // hy1
                    _add("auth_str", &auth_str);
                    _add("protocol", &protocol);
                } else { // hy2
                    uploadMbps = 0;
                    downloadMbps = 0;
                    _add("password", &password);
                }
            } else if (proxy_type == proxy_TUIC) {
                _add("uuid", &uuid);
                _add("password", &password);
                _add("congestionControl", &congestionControl);
                _add("udpRelayMode", &udpRelayMode);
                _add("zeroRttHandshake", &zeroRttHandshake);
                _add("heartbeat", &heartbeat);
                _add("uos", &uos);
            }
            // TLS
            _add("allowInsecure", &allowInsecure);
            _add("sni", &sni);
            _add("alpn", &alpn);
            _add("caText", &caText);
            _add("disableSni", &disableSni);
        };

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