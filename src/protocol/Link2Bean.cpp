#include "profile/ProxyEntity.hpp"
#include "protocol/Includes.hpp"

#include <QUrlQuery>

namespace NekoGui_fmt {
    bool SocksHttpBean::TryParseLink(const QString &link) {
        QUrl url(link);
        if (!url.isValid()) return false;
        QUrlQuery query(url);

        if (link.startsWith("socks4")) socks_http_type = type_Socks4;
        if (link.startsWith("http")) socks_http_type = type_HTTP;
        name = url.fragment(QUrl::FullyDecoded);
        serverAddress = url.host();
        serverPort = url.port();
        username = url.userName();
        password = url.password();

        // v2rayN fmt
        if (password.isEmpty() && !username.isEmpty()) {
            QString n = DecodeB64IfValid(username);
            if (!n.isEmpty()) {
                username = SubStrBefore(n, ":");
                password = SubStrAfter(n, ":");
            }
        }

        stream->security = GetQueryValue(query, "security", "");
        stream->sni = GetQueryValue(query, "sni");
        if (link.startsWith("https")) stream->security = "tls";

        return !serverAddress.isEmpty();
    }

    bool ShadowSocksBean::TryParseLink(const QString &link) {
        QUrl url;
        if (SubStrBefore(link, "#").contains("@")) {
            url = QUrl(link);
        } else if (SubStrBefore(link, "#").contains("?")) {
            url = QUrl("ss://" + DecodeBase64OrBase64Url(SubStrBefore(SubStrAfter(link, "://"), "?")) + "?" + SubStrAfter(link, "?"));
        } else if (link.contains("#")) {
            url = QUrl("ss://" + DecodeBase64OrBase64Url(SubStrBefore(SubStrAfter(link, "://"), "#")) + "#" + SubStrAfter(link, "#"));
        } else {
            url = QUrl("ss://" + DecodeBase64OrBase64Url(SubStrAfter(link, "://")));
        }
        if (!url.isValid()) return false;

        name = url.fragment(QUrl::FullyDecoded);
        serverAddress = url.host();
        serverPort = url.port();

        if (url.password().isEmpty()) {
            // traditional format
            auto method_password = DecodeBase64OrBase64Url(url.userName());
            if (method_password.isEmpty()) return false;
            method = SubStrBefore(method_password, ":");
            password = SubStrAfter(method_password, ":");
        } else {
            // 2022 format
            method = url.userName();
            password = url.password();
        }

        QUrlQuery query(url);
        if (!query.queryItemValue("plugin").startsWith("none")) {
            plugin = query.queryItemValue("plugin").replace("simple-obfs;", "obfs-local;");
        }

        // *ray misnomer
        if (method == "chacha20-poly1305")
            method = "chacha20-ietf-poly1305";
        else if (method == "xchacha20-poly1305")
            method = "xchacha20-ietf-poly1305";

        return !serverAddress.isEmpty();
    }

    bool ShadowSocksRBean::TryParseLink(const QString &link) {
        QString decoded = DecodeBase64OrBase64Url(SubStrAfter(link, "://"));
        QStringList parts = SubStrBefore(decoded, "/?").split(':');
        if (parts.size() != 6)
            return false;

        serverAddress = parts[0];
        serverPort = parts[1].toInt();
        protocol = parts[2];
        method = parts[3];
        obfs = parts[4];
        password = DecodeBase64OrBase64Url(parts[5]);

        QUrlQuery query(SubStrAfter(decoded, "/?"));
        obfsParam = DecodeBase64OrBase64Url(query.queryItemValue("obfsparam"));
        protocolParam = DecodeBase64OrBase64Url(query.queryItemValue("protoparam"));
        name = DecodeBase64OrBase64Url(query.queryItemValue("remarks"));

        return !serverAddress.isEmpty();
    }

    bool VMessBean::TryParseLink(const QString &link) {
        if (!link.contains("?")) {
            // V2RayN Format
            auto objN = QString2QJsonObject(DecodeBase64OrBase64Url(SubStrAfter(link, "://")));
            if (objN.isEmpty()) return false;
            // REQUIRED
            uuid = objN["id"].toString();
            serverAddress = objN["add"].toString();
            serverPort = objN["port"].toVariant().toInt();
            // OPTIONAL
            name = objN["ps"].toString();
            aid = objN["aid"].toVariant().toInt();
            security = objN["scy"].toString();
            stream->host = objN["host"].toString();
            stream->path = objN["path"].toString();
            stream->network = objN["net"].toString();
            if (objN["type"].toString() == "http") stream->network = "http";
            stream->security = objN["tls"].toString();
            stream->sni = objN["sni"].toString();
            stream->alpn = objN["alpn"].toString();
            stream->utlsFingerprint = objN["fp"].toString();
            stream->allow_insecure = objN["insecure"].toString() == "1";
        } else {
            QUrl url = SubStrBefore(link, "?").contains("@")
                           ? QUrl(link)
                           : QUrl("vmess://" + DecodeBase64OrBase64Url(SubStrBefore(SubStrAfter(link, "://"), "?")) + "?" + SubStrAfter(link, "?"));
            if (!url.isValid()) return false;
            QUrlQuery query(url);

            if (!url.password().isEmpty()) {
                name = query.queryItemValue("remarks");
                uuid = url.password();
                security = url.userName();
                if (GetQueryValue(query, "tls") == "1") stream->security = "tls";
            } else {
                // https://github.com/XTLS/Xray-core/discussions/716
                name = url.fragment(QUrl::FullyDecoded);
                uuid = url.userName();
                security = GetQueryValue(query, "encryption", "auto");
                stream->security = GetQueryValue(query, "security");
            }
            serverAddress = url.host();
            serverPort = url.port();
            aid = GetQueryValue(query, "alterId", "0").toInt();

            // security
            stream->sni = FirstQueryValue(query, {"sni", "peer"});
            stream->alpn = GetQueryValue(query, "alpn");
            if (!query.queryItemValue("allowInsecure").isEmpty()) stream->allow_insecure = true;
            stream->reality_pbk = GetQueryValue(query, "pbk", "");
            stream->reality_sid = GetQueryValue(query, "sid", "");
            stream->reality_spx = GetQueryValue(query, "spx", "");
            stream->utlsFingerprint = GetQueryValue(query, "fp", "");
            if (stream->utlsFingerprint.isEmpty()) {
                stream->utlsFingerprint = NekoGui::dataStore->utlsFingerprint;
            }

            // type
            stream->network = FirstQueryValue(query, {"type", "obfs"});
            if (GetQueryValue(query, "headerType") == "http") stream->network = "http";
            if (stream->network == "grpc") {
                stream->path = FirstQueryValue(query, {"serviceName", "path"});
            } else {
                stream->path = GetQueryValue(query, "path");
                stream->host = GetQueryValue(query, "host");
            }
        }
        return !(serverAddress.isEmpty() || uuid.isEmpty());
    }

    bool TrojanVLESSBean::TryParseLink(const QString &link) {
        QUrl url = SubStrBefore(link, "?").contains("@")
                       ? QUrl(link)
                       : QUrl("url://" + DecodeBase64OrBase64Url(SubStrBefore(SubStrAfter(link, "://"), "?")) + "?" + SubStrAfter(link, "?"));
        if (!url.isValid()) return false;
        QUrlQuery query(url);

        if (!url.password().isEmpty()) {
            name = query.queryItemValue("remarks");
            password = url.password();
            if (GetQueryValue(query, "tls") == "1") stream->security = "tls";
        } else {
            name = url.fragment(QUrl::FullyDecoded);
            password = url.userName();
            if (proxy_type == proxy_Trojan) {
                stream->security = GetQueryValue(query, "security", "tls");
            } else {
                stream->security = GetQueryValue(query, "security", "");
            }
        }
        serverAddress = url.host();
        serverPort = url.port();

        // security
        stream->sni = FirstQueryValue(query, {"sni", "peer"});
        stream->alpn = GetQueryValue(query, "alpn");
        if (!query.queryItemValue("allowInsecure").isEmpty()) stream->allow_insecure = true;
        stream->reality_pbk = GetQueryValue(query, "pbk", "");
        stream->reality_sid = GetQueryValue(query, "sid", "");
        stream->reality_spx = GetQueryValue(query, "spx", "");
        stream->utlsFingerprint = GetQueryValue(query, "fp", "");
        if (stream->utlsFingerprint.isEmpty()) {
            stream->utlsFingerprint = NekoGui::dataStore->utlsFingerprint;
        }

        // type
        stream->network = FirstQueryValue(query, {"type", "obfs"});
        if (GetQueryValue(query, "headerType") == "http") stream->network = "http";
        if (stream->network == "grpc") {
            stream->path = FirstQueryValue(query, {"serviceName", "path"});
        } else {
            stream->path = GetQueryValue(query, "path");
            stream->host = GetQueryValue(query, "host");
        }

        // protocol
        if (proxy_type == proxy_VLESS) {
            if (GetQueryValue(query, "flow").startsWith("xtls-rprx-vision") || GetQueryValue(query, "xtls") == "2")
                flow = "xtls-rprx-vision";
        }

        return !(serverAddress.isEmpty() || password.isEmpty());
    }

    bool NaiveBean::TryParseLink(const QString &link) {
        QUrl url(link);
        if (!url.isValid()) return false;

        protocol = url.scheme().replace("naive+", "");
        if (protocol != "https" && protocol != "quic") return false;

        name = url.fragment(QUrl::FullyDecoded);
        serverAddress = url.host();
        serverPort = url.port();
        username = url.userName();
        password = url.password();

        return !serverAddress.isEmpty();
    }

    bool QUICBean::TryParseLink(const QString &link) {
        QUrl url(link);
        if (!url.isValid()) return false;
        QUrlQuery query(url);
        name = url.fragment(QUrl::FullyDecoded);
        serverAddress = url.host();
        serverPort = url.port();
        alpn = query.queryItemValue("alpn");
        sni = FirstQueryValue(query, {"sni", "peer"});
        if (url.scheme() == "hysteria") {
            // https://hysteria.network/docs/uri-scheme/
            if (!query.hasQueryItem("upmbps") || !query.hasQueryItem("downmbps")) return false;

            hopPort = query.queryItemValue("mport");
            obfsPassword = query.queryItemValue("obfsParam");
            allowInsecure = QStringList{"1", "true"}.contains(query.queryItemValue("insecure"));
            uploadMbps = query.queryItemValue("upmbps").toInt();
            downloadMbps = query.queryItemValue("downmbps").toInt();

            auto protocolStr = GetQueryValue(query, "protocol", "udp").toLower();
            if (protocolStr == "faketcp") {
                hyProtocol = NekoGui_fmt::QUICBean::hysteria_protocol_facktcp;
            } else if (protocolStr.startsWith("wechat")) {
                hyProtocol = NekoGui_fmt::QUICBean::hysteria_protocol_wechat_video;
            }

            if (query.hasQueryItem("auth")) {
                authPayload = query.queryItemValue("auth");
                authPayloadType = NekoGui_fmt::QUICBean::hysteria_auth_string;
            }

            connectionReceiveWindow = query.queryItemValue("recv_window").toInt();
            streamReceiveWindow = query.queryItemValue("recv_window_conn").toInt();
        } else if (url.scheme() == "tuic") {
            // by daeuniverse
            // https://github.com/daeuniverse/dae/discussions/182

            uuid = url.userName();
            password = url.password();

            congestionControl = query.queryItemValue("congestion_control");
            udpRelayMode = query.queryItemValue("udp_relay_mode");
            allowInsecure = query.queryItemValue("allow_insecure") == "1";
            disableSni = query.queryItemValue("disable_sni") == "1";
        } else if (QStringList{"hy2", "hysteria2"}.contains(url.scheme())) {
            hopPort = query.queryItemValue("mport");
            obfsPassword = query.queryItemValue("obfs-password");
            allowInsecure = QStringList{"1", "true"}.contains(query.queryItemValue("insecure"));

            if (url.password().isEmpty()) {
                password = url.userName();
            } else {
                password = url.userName() + ":" + url.password();
            }
        }
        return !serverAddress.isEmpty();
    }

    bool AnyTLSBean::TryParseLink(const QString &link) {
        QUrl url(link);
        if (!url.isValid()) return false;
        QUrlQuery query(url);

        name = url.fragment(QUrl::FullyDecoded);
        serverAddress = url.host();
        serverPort = url.port();
        password = url.userName();
        stream->sni = query.queryItemValue("sni");
        stream->allow_insecure = query.queryItemValue("insecure") == "1";

        return !(serverAddress.isEmpty() || password.isEmpty());
    }

    bool SSHBean::TryParseLink(const QString &link) {
        QUrl url(link);
        if (!url.isValid()) return false;
        QUrlQuery query(url);

        name = url.fragment(QUrl::FullyDecoded);
        serverAddress = url.host();
        serverPort = url.port();
        user = url.userName();
        password = url.password();
        privateKey = DecodeBase64OrBase64Url(query.queryItemValue("private_key"));
        privateKeyPassphrase = query.queryItemValue("private_key_passphrase");

        return !serverAddress.isEmpty();
    }

    bool WireGuardBean::TryParseLink(const QString &link) {
        QUrl url(link);
        if (!url.isValid()) return false;
        QUrlQuery query(url);

        name = url.fragment(QUrl::FullyDecoded);
        serverAddress = url.host();
        serverPort = url.port();
        privateKey = query.queryItemValue("privateKey");
        publicKey = query.queryItemValue("publicKey");
        preSharedKey = query.queryItemValue("presharedKey");
        localAddress = FirstQueryValue(query, {"address", "ip"});
        reserved = query.queryItemValue("reserved");
        MTU = query.queryItemValue("mtu").toInt();

        return !serverAddress.isEmpty();
    }

} // namespace NekoGui_fmt