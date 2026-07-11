#include "profile/ProxyEntity.hpp"
#include "protocol/Includes.hpp"

#include <QUrl>

namespace NekoGui_fmt {
    ExternalBuildResult NaiveBean::BuildExternal(int mapping_port, int socks_port) {
        ExternalBuildResult result{NekoGui::dataStore->extraCore->Get("naive")};

        auto domain_address = sni.isEmpty() ? serverAddress : sni;
        auto connect_address = mapping_port > 0 ? "127.0.0.1" : serverAddress;
        auto connect_port = mapping_port > 0 ? mapping_port : serverPort;
        domain_address = WrapIPV6Host(domain_address);
        connect_address = WrapIPV6Host(connect_address);

        auto proxy_url = QUrl();
        proxy_url.setScheme(protocol);
        proxy_url.setUserName(username);
        proxy_url.setPassword(password);
        proxy_url.setPort(connect_port);
        proxy_url.setHost(domain_address);

        if (!disable_log) result.arguments += "--log";
        result.arguments += "--listen=socks://127.0.0.1:" + Int2String(socks_port);
        result.arguments += "--proxy=" + proxy_url.toString(QUrl::FullyEncoded);
        if (domain_address != connect_address) result.arguments += "--host-resolver-rules=MAP " + domain_address + " " + connect_address;
        if (insecure_concurrency > 0) result.arguments += "--insecure-concurrency=" + Int2String(insecure_concurrency);
        if (!extra_headers.trimmed().isEmpty()) result.arguments += "--extra-headers=" + extra_headers;
        if (!certificate.trimmed().isEmpty()) result.env += "SSL_CERT_FILE=" + WriteTempFile("naive_XXXXXXXXXX.crt", certificate, result.error);

        return result;
    }

    ExternalBuildResult QUICBean::BuildExternal(int mapping_port, int socks_port) {
        if (proxy_type == proxy_TUIC) {
            ExternalBuildResult result{NekoGui::dataStore->extraCore->Get("tuic")};

            QJsonObject relay;

            relay["uuid"] = uuid;
            relay["password"] = password;
            relay["udp_relay_mode"] = udpRelayMode;
            relay["congestion_control"] = congestionControl;
            relay["zero_rtt_handshake"] = zeroRttHandshake;
            relay["disable_sni"] = disableSni;
            if (!heartbeat.trimmed().isEmpty()) relay["heartbeat"] = heartbeat;
            if (!alpn.trimmed().isEmpty()) relay["alpn"] = QList2QJsonArray(alpn.split(","));

            if (!caText.trimmed().isEmpty()) {
                QJsonArray certificate;
                certificate.append(WriteTempFile("tuic_XXXXXXXXXX.crt", caText, result.error));
                relay["certificates"] = certificate;
            }

            // The most confused part of TUIC......
            if (serverAddress == sni) {
                relay["server"] = MakeHostPort(serverAddress, serverPort);
            } else {
                relay["server"] = MakeHostPort(sni, serverPort);
                relay["ip"] = serverAddress;
            }

            QJsonObject local{
                {"server", "127.0.0.1:" + Int2String(socks_port)},
            };

            QJsonObject config{
                {"relay", relay},
                {"local", local},
            };

            //

            result.config_export = QJsonObject2QString(config, false);
            result.arguments = QStringList{"-c", WriteTempFile("tuic_XXXXXXXXXX.json", result.config_export, result.error)};

            return result;
        } else if (proxy_type == proxy_Hysteria2) {
            ExternalBuildResult result{NekoGui::dataStore->extraCore->Get("hysteria2")};

            QJsonObject config;

            auto server = serverAddress;
            if (!hopPort.trimmed().isEmpty()) {
                server = WrapIPV6Host(server) + ":" + hopPort;
            } else {
                server = WrapIPV6Host(server) + ":" + Int2String(serverPort);
            }

            QJsonObject transport;
            transport["type"] = "udp";
            transport["udp"] = QJsonObject{
                {"hopInterval", QString::number(hopInterval) + "s"},
            };
            config["transport"] = transport;

            config["server"] = server;
            config["socks5"] = QJsonObject{
                {"listen", "127.0.0.1:" + Int2String(socks_port)},
                {"disableUDP", false},
            };
            config["auth"] = password;

            QJsonObject bandwidth;
            if (uploadMbps > 0) bandwidth["up"] = Int2String(uploadMbps) + " mbps";
            if (downloadMbps > 0) bandwidth["down"] = Int2String(downloadMbps) + " mbps";
            config["bandwidth"] = bandwidth;

            QJsonObject quic;
            if (streamReceiveWindow > 0) quic["initStreamReceiveWindow"] = streamReceiveWindow;
            if (connectionReceiveWindow > 0) quic["initConnReceiveWindow"] = connectionReceiveWindow;
            if (disableMtuDiscovery) quic["disablePathMTUDiscovery"] = true;
            config["quic"] = quic;

            config["fastOpen"] = true;
            config["lazy"] = true;

            if (!obfsPassword.isEmpty()) {
                QJsonObject obfs;
                obfs["type"] = "salamander";
                obfs["salamander"] = QJsonObject{
                    {"password", obfsPassword},
                };

                config["obfs"] = obfs;
            }

            QJsonObject tls;
            auto sniGen = sni;
            if (sni.isEmpty() && !IsIpAddress(serverAddress)) sniGen = serverAddress;
            tls["sni"] = sniGen;
            if (allowInsecure) tls["insecure"] = true;
            if (!caText.trimmed().isEmpty()) {
                QJsonArray certificate;
                certificate.append(WriteTempFile("hysteria2_XXXXXXXXXX.crt", caText, result.error));
                tls["certificates"] = certificate;
            }
            config["tls"] = tls;

            result.config_export = QJsonObject2QString(config, false);
            result.arguments = QStringList{"-c", WriteTempFile("hysteria2_XXXXXXXXXX.json", result.config_export, result.error)};

            return result;

        } else { // Hysteria
            ExternalBuildResult result{NekoGui::dataStore->extraCore->Get("hysteria")};

            QJsonObject config;

            // determine server format
            auto sniGen = sni;
            if (sni.isEmpty() && !IsIpAddress(serverAddress)) sniGen = serverAddress;

            auto server = serverAddress;
            if (!hopPort.trimmed().isEmpty()) {
                server = WrapIPV6Host(server) + ":" + hopPort;
            } else {
                server = WrapIPV6Host(server) + ":" + Int2String(serverPort);
            }
            config["server"] = mapping_port > 0 ? "127.0.0.1:" + Int2String(mapping_port) : server;

            // listen
            config["socks5"] = QJsonObject{
                {"listen", "127.0.0.1:" + Int2String(socks_port)},
            };

            // misc

            config["retry"] = 5;
            config["fast_open"] = true;
            config["lazy_start"] = true;
            config["obfs"] = obfsPassword;
            config["up_mbps"] = uploadMbps;
            config["down_mbps"] = downloadMbps;
            config["auth_str"] = auth_str;
            config["protocol"] = protocol;

            if (!sniGen.isEmpty()) config["server_name"] = sniGen;
            if (!alpn.isEmpty()) config["alpn"] = alpn;

            if (!caText.trimmed().isEmpty()) {
                config["ca"] = WriteTempFile("hysteria_XXXXXXXXXX.crt", caText, result.error);
            }

            if (allowInsecure) config["insecure"] = true;
            if (streamReceiveWindow > 0) config["recv_window_conn"] = streamReceiveWindow;
            if (connectionReceiveWindow > 0) config["recv_window"] = connectionReceiveWindow;
            if (disableMtuDiscovery) config["disable_mtu_discovery"] = true;
            config["hop_interval"] = hopInterval;

            //

            result.config_export = QJsonObject2QString(config, false);
            result.arguments = QStringList{"--no-check", "-c", WriteTempFile("hysteria_XXXXXXXXXX.json", result.config_export, result.error)};

            return result;
        }
    }

    ExternalBuildResult CustomBean::BuildExternal(int mapping_port, int socks_port) {
        ExternalBuildResult result{NekoGui::dataStore->extraCore->Get(core)};

        result.arguments = command; // TODO split?

        for (int i = 0; i < result.arguments.length(); i++) {
            auto arg = result.arguments[i];
            arg = arg.replace("%socks_port%", Int2String(socks_port));
            arg = arg.replace("%server_addr%", mapping_port > 0 ? "127.0.0.1" : serverAddress);
            arg = arg.replace("%server_port%", Int2String(mapping_port > 0 ? mapping_port : serverPort));
            result.arguments[i] = arg;
        }

        if (!config_simple.trimmed().isEmpty()) {
            auto config = config_simple;
            config = config.replace("%socks_port%", Int2String(socks_port));
            config = config.replace("%server_addr%", mapping_port > 0 ? "127.0.0.1" : serverAddress);
            config = config.replace("%server_port%", Int2String(mapping_port > 0 ? mapping_port : serverPort));

            // suffix
            QString suffix;
            if (!config_suffix.isEmpty()) {
                suffix = "." + config_suffix;
            } else if (!QString2QJsonObject(config).isEmpty()) {
                // trojan-go: unsupported config format: xxx.tmp. use .yaml or .json instead.
                suffix = ".json";
            }

            // write config
            auto TempFile = WriteTempFile("custom_XXXXXXXXXX" + suffix, config, result.error);
            for (int i = 0; i < result.arguments.count(); i++) {
                result.arguments[i] = result.arguments[i].replace("%config%", TempFile);
            }

            result.config_export = config;
        }

        return result;
    }

} // namespace NekoGui_fmt
