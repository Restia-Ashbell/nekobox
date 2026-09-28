#include "SubscriptionParser.hpp"

#include <QLocale>
#include <QRegularExpression>

#include "common/Utils.hpp"
#include "profile/ProfileManager.hpp"
#include "protocol/Includes.hpp"

namespace YAML {
    template<>
    struct convert<QString> {
        static bool decode(const Node &node, QString &rhs) {
            if (!node.IsScalar()) return false;
            rhs = QString::fromStdString(node.as<std::string>());
            return true;
        }
    };
    template<typename T>
    struct convert<QList<T>> {
        static bool decode(const Node &node, QList<T> &rhs) {
            if (!node.IsSequence()) return false;
            rhs.clear();
            for (const auto &n: node) {
                rhs.append(n.as<T>());
            }
            return true;
        }
    };
} // namespace YAML

YAML::Node QString2YAMLNode(const QString &str) {
    try {
        return YAML::Load(str.toStdString());
    } catch (const YAML::Exception &ex) {
        qDebug() << ex.what();
    }
    return {};
}

template<typename T>
T Node2Value(const YAML::Node &node, const T &def = T{}) {
    try {
        return node.as<T>(def);
    } catch (const YAML::Exception &ex) {
        qDebug() << ex.what();
    }
    return def;
}

namespace NekoGui_sub {

    // 解析 Subscription-UserInfo 响应头：total / upload / download / expire
    QString SubscriptionParser::parseSubInfo(const QString &info) {
        if (info.trimmed().isEmpty()) return {};

        long long used = 0, total = 0, expire = 0;
        static const QRegularExpression re(R"(\b(total|upload|download|expire)=([0-9]+))");
        auto it = re.globalMatch(info);
        while (it.hasNext()) {
            const auto match = it.next();
            const auto value = match.captured(2).toLongLong();
            const auto key = match.captured(1);
            if (key == "total")
                total = value;
            else if (key == "upload")
                used += value;
            else if (key == "download")
                used += value;
            else if (key == "expire")
                expire = value;
        }
        if (used == 0 && total == 0 && expire == 0) return {};

        return QObject::tr("Used: %1 Remain: %2 Expire: %3").arg(ReadableSize(used), ReadableSize(total - used), DisplayTime(expire, QLocale::ShortFormat));
    }

    // 解析 Content-Disposition 响应头里的分组名（支持 filename* 与 filename）
    QString SubscriptionParser::parseSubName(const QString &contentDisposition) {
        if (contentDisposition.trimmed().isEmpty()) return {};
        QRegularExpressionMatch match;
        static const QRegularExpression reFilenameStar(R"(filename\*\s*=\s*UTF-8''([^;]+))", QRegularExpression::CaseInsensitiveOption);
        match = reFilenameStar.match(contentDisposition);
        if (match.hasMatch()) {
            return QUrl::fromPercentEncoding(match.captured(1).toUtf8());
        }
        static const QRegularExpression reFilename(R"REGEX(filename\s*=\s*(?:"([^"]+)"|([^;]+)))REGEX", QRegularExpression::CaseInsensitiveOption);
        match = reFilename.match(contentDisposition);
        if (match.hasMatch()) {
            QString filename = match.captured(1);
            if (filename.isEmpty()) filename = match.captured(2);
            return filename.trimmed();
        }
        return {};
    }

    void SubscriptionParser::fixEnt(const std::shared_ptr<NekoGui::ProxyEntity> &ent) {
        if (ent == nullptr) return;
        auto stream = ent->bean->_get<NekoGui_fmt::V2rayStreamSettings>("stream");
        if (stream == nullptr) return;
        // 1. "security"
        if (stream->security == "none" || stream->security == "0" || stream->security == "false") {
            stream->security = "";
        } else if (stream->security == "1" || stream->security == "true" || stream->security == "reality") {
            stream->security = "tls";
        }
        // 2. TLS SNI: v2rayN config builder generate sni like this, so set sni here for their format.
        if (stream->security == "tls" && IsIpAddress(ent->bean->serverAddress) && (!stream->host.isEmpty()) && stream->sni.isEmpty()) {
            stream->sni = stream->host;
        }
        // 3. transport
        if (stream->network == "none" || stream->network == "tcp") {
            stream->network = "";
        } else if (stream->network == "h2") {
            stream->network = "http";
        } else if (stream->network == "websocket") {
            stream->network = "ws";
        }
    }

    QList<std::shared_ptr<NekoGui::ProxyEntity>> SubscriptionParser::update(const QString &str, QStringList *diag) {
        // Clash && Sing-Box
        if (auto root = QString2YAMLNode(str); root.IsMap()) {
            if (auto proxies = root["proxies"]; proxies.IsDefined() && proxies.IsSequence()) {
                return updateClash(proxies, diag);
            } else {
                return updateJson(str);
            }
        }

        const auto decoded = DecodeB64IfValid(str);
        return updateLink(decoded.isEmpty() ? str : decoded, diag);
    }

    QList<std::shared_ptr<NekoGui::ProxyEntity>> SubscriptionParser::updateJson(const QString &str) {
        auto ent = NekoGui::ProfileManager::NewProxyEntity("custom");
        auto bean = ent->Bean<NekoGui_fmt::CustomBean>();
        auto obj = QString2QJsonObject(str);
        if (obj.contains("outbounds")) {
            bean->core = "internal-full";
            bean->config_simple = str;
        } else if (obj.contains("server")) {
            bean->core = "internal";
            bean->config_simple = str;
        } else {
            return {};
        }
        return {ent};
    }

    QList<std::shared_ptr<NekoGui::ProxyEntity>> SubscriptionParser::updateLink(const QString &str, QStringList *diag) {
        QList<std::shared_ptr<NekoGui::ProxyEntity>> result;

        for (const auto &line: str.split('\n', Qt::SkipEmptyParts)) {
            const QString link_str = line.trimmed();
            const QString scheme = SubStrBefore(link_str, "://").toLower();
            std::shared_ptr<NekoGui::ProxyEntity> ent;

            if (scheme == "nekobox") {
                QUrl link(link_str);
                if (!link.isValid()) continue;
                ent = NekoGui::ProfileManager::NewProxyEntity(link.host());
                if (!ent->bean) continue;
                auto j = DecodeB64IfValid(link.fragment(), QByteArray::Base64UrlEncoding);
                if (j.isEmpty()) continue;
                ent->bean->FromJsonBytes(j);
            } else {
                ent = NekoGui::ProfileManager::NewProxyEntity(scheme);
                if (!ent->bean || !ent->bean->TryParseLink(link_str)) ent = nullptr;
            }

            if (ent) {
                // Fix
                fixEnt(ent);
                // End
                result += ent;
            } else {
                if (diag) diag->append(QObject::tr("Failed to parse: %1").arg(link_str));
            }
        }

        return result;
    }

    // ws / grpc / h2 / http / reality transport options; shared by vmess and trojan/vless
    void applyStreamOpts(NekoGui_fmt::V2rayStreamSettings *stream, const YAML::Node &proxy) {
        if (auto ws = proxy["ws-opts"]) {
            auto headers = ws["headers"];
            for (auto header: headers) {
                if (Node2Value<QString>(header.first).toLower() == "host") {
                    stream->host = Node2Value<QString>(header.second);
                }
            }
            stream->path = Node2Value<QString>(ws["path"]);
            stream->ws_early_data_length = Node2Value<int>(ws["max-early-data"]);
            stream->ws_early_data_name = Node2Value<QString>(ws["early-data-header-name"]);
            if (Node2Value<bool>(ws["v2ray-http-upgrade"])) stream->network = "httpupgrade";
        }

        if (auto grpc = proxy["grpc-opts"]) {
            stream->path = Node2Value<QString>(grpc["grpc-service-name"]);
        }

        if (auto h2 = proxy["h2-opts"]) {
            stream->host = Node2Value<QList<QString>>(h2["host"]).join(",");
            stream->path = Node2Value<QString>(h2["path"]);
        }

        if (auto http = proxy["http-opts"]) {
            auto headers = http["headers"];
            for (auto header: headers) {
                if (Node2Value<QString>(header.first).toLower() == "host") {
                    stream->host = Node2Value<QList<QString>>(header.second).join(",");
                    break;
                }
            }
            stream->path = Node2Value<QList<QString>>(http["path"]).value(0);
        }

        if (auto reality = proxy["reality-opts"]) {
            stream->reality_pbk = Node2Value<QString>(reality["public-key"]);
            stream->reality_sid = Node2Value<QString>(reality["short-id"]);
        }
    }

    // https://github.com/Dreamacro/clash/wiki/configuration
    QList<std::shared_ptr<NekoGui::ProxyEntity>> SubscriptionParser::updateClash(const YAML::Node &proxies, QStringList *diag) {
        QList<std::shared_ptr<NekoGui::ProxyEntity>> result;

        for (const auto &proxy: proxies) {
            auto type = Node2Value<QString>(proxy["type"]).toLower();

            if (type == "socks5") type = "socks";
            if (type == "ss") type = "shadowsocks";
            if (type == "ssr") type = "shadowsocksr";

            auto ent = NekoGui::ProfileManager::NewProxyEntity(type);
            if (!ent->bean) continue;

            // common
            ent->bean->name = Node2Value<QString>(proxy["name"]);
            ent->bean->serverAddress = Node2Value<QString>(proxy["server"]);
            ent->bean->serverPort = Node2Value<int>(proxy["port"]);

            if (type == "shadowsocks") {
                auto bean = ent->Bean<NekoGui_fmt::ShadowSocksBean>();
                bean->method = Node2Value<QString>(proxy["cipher"]).replace("dummy", "none");
                bean->password = Node2Value<QString>(proxy["password"]);

                // UDP over TCP
                if (Node2Value<bool>(proxy["udp-over-tcp"])) {
                    bean->uot = Node2Value<int>(proxy["udp-over-tcp-version"]);
                    if (bean->uot == 0) bean->uot = 2;
                }

                if (auto pluginOpts = proxy["plugin-opts"]) {
                    QStringList ssPlugin;
                    auto plugin = Node2Value<QString>(proxy["plugin"]);
                    if (plugin == "obfs") {
                        ssPlugin << "obfs-local";
                        ssPlugin << "obfs=" + Node2Value<QString>(pluginOpts["mode"]);
                        ssPlugin << "obfs-host=" + Node2Value<QString>(pluginOpts["host"]);
                    } else if (plugin == "v2ray-plugin") {
                        auto mode = Node2Value<QString>(pluginOpts["mode"]);
                        auto host = Node2Value<QString>(pluginOpts["host"]);
                        auto path = Node2Value<QString>(pluginOpts["path"]);
                        ssPlugin << "v2ray-plugin";
                        if (!mode.isEmpty() && mode != "websocket") ssPlugin << "mode=" + mode;
                        if (Node2Value<bool>(pluginOpts["tls"])) ssPlugin << "tls";
                        if (!host.isEmpty()) ssPlugin << "host=" + host;
                        if (!path.isEmpty()) ssPlugin << "path=" + path;
                        // clash only: skip-cert-verify
                        // clash only: headers
                        // clash: mux=?
                    }
                    bean->plugin = ssPlugin.join(";");
                }

                if (auto smux = proxy["smux"]) {
                    bean->multiplex->enabled = Node2Value<bool>(smux["enabled"]);
                }
            } else if (type == "shadowsocksr") {
                auto bean = ent->Bean<NekoGui_fmt::ShadowSocksRBean>();
                bean->method = Node2Value<QString>(proxy["cipher"]).replace("dummy", "none");
                bean->password = Node2Value<QString>(proxy["password"]);
                bean->obfs = Node2Value<QString>(proxy["obfs"]);
                bean->obfsParam = Node2Value<QString>(proxy["obfs-param"]);
                bean->protocol = Node2Value<QString>(proxy["protocol"]);
                bean->protocolParam = Node2Value<QString>(proxy["protocol-param"]);
            } else if (type == "socks" || type == "http") {
                auto bean = ent->Bean<NekoGui_fmt::SocksHttpBean>();
                bean->username = Node2Value<QString>(proxy["username"]);
                bean->password = Node2Value<QString>(proxy["password"]);
                if (type == "http") {
                    if (Node2Value<bool>(proxy["tls"])) bean->stream->security = "tls";
                    if (Node2Value<bool>(proxy["skip-cert-verify"])) bean->stream->allow_insecure = true;
                }
            } else if (type == "trojan" || type == "vless") {
                auto bean = ent->Bean<NekoGui_fmt::TrojanVLESSBean>();
                if (type == "vless") {
                    bean->flow = Node2Value<QString>(proxy["flow"]);
                    bean->password = Node2Value<QString>(proxy["uuid"]);
                    bean->stream->packet_encoding = Node2Value<QString>(proxy["packet-encoding"]);
                    if (Node2Value<bool>(proxy["tls"])) bean->stream->security = "tls";
                } else {
                    bean->password = Node2Value<QString>(proxy["password"]);
                    bean->stream->security = "tls";
                }
                bean->stream->network = Node2Value<QString>(proxy["network"]);
                bean->stream->sni = firstOrSecond(Node2Value<QString>(proxy["sni"]), Node2Value<QString>(proxy["servername"]));
                bean->stream->alpn = Node2Value<QList<QString>>(proxy["alpn"]).join(",");
                bean->stream->allow_insecure = Node2Value<bool>(proxy["skip-cert-verify"]);
                bean->stream->utlsFingerprint = Node2Value<QString>(proxy["client-fingerprint"]);

                if (auto smux = proxy["smux"]) {
                    bean->multiplex->enabled = Node2Value<bool>(smux["enabled"]);
                }
                applyStreamOpts(bean->stream, proxy);
            } else if (type == "vmess") {
                auto bean = ent->Bean<NekoGui_fmt::VMessBean>();
                bean->uuid = Node2Value<QString>(proxy["uuid"]);
                bean->aid = Node2Value<int>(proxy["alterId"]);
                bean->security = Node2Value<QString>(proxy["cipher"], bean->security);
                bean->stream->network = Node2Value<QString>(proxy["network"]);
                bean->stream->sni = firstOrSecond(Node2Value<QString>(proxy["sni"]), Node2Value<QString>(proxy["servername"]));
                bean->stream->alpn = Node2Value<QList<QString>>(proxy["alpn"]).join(",");
                if (Node2Value<bool>(proxy["tls"])) bean->stream->security = "tls";
                bean->stream->allow_insecure = Node2Value<bool>(proxy["skip-cert-verify"]);
                bean->stream->utlsFingerprint = Node2Value<QString>(proxy["client-fingerprint"]);
                bean->stream->packet_encoding = Node2Value<QString>(proxy["packet-encoding"]);

                if (auto smux = proxy["smux"]) {
                    bean->multiplex->enabled = Node2Value<bool>(smux["enabled"]);
                }
                applyStreamOpts(bean->stream, proxy);
            } else if (type == "hysteria") {
                auto bean = ent->Bean<NekoGui_fmt::QUICBean>();

                bean->hopPort = Node2Value<QString>(proxy["ports"]);

                bean->allowInsecure = Node2Value<bool>(proxy["skip-cert-verify"]);
                bean->caText = Node2Value<QString>(proxy["ca-str"]);
                bean->alpn = Node2Value<QList<QString>>(proxy["alpn"]).join(",");
                bean->sni = Node2Value<QString>(proxy["sni"]);

                bean->auth_str = Node2Value<QString>(proxy["auth-str"]);
                bean->protocol = Node2Value<QString>(proxy["protocol"]);
                bean->obfsPassword = Node2Value<QString>(proxy["obfs"]);

                bean->disableMtuDiscovery = Node2Value<bool>(proxy["disable-mtu-discovery"]);
                bean->streamReceiveWindow = Node2Value<qint64>(proxy["recv-window"]);
                bean->connectionReceiveWindow = Node2Value<qint64>(proxy["recv-window-conn"]);

                bean->uploadMbps = Node2Value<int>(proxy["up"]);
                bean->downloadMbps = Node2Value<int>(proxy["down"]);
            } else if (type == "hysteria2") {
                auto bean = ent->Bean<NekoGui_fmt::QUICBean>();

                bean->hopPort = Node2Value<QString>(proxy["ports"]);

                bean->allowInsecure = Node2Value<bool>(proxy["skip-cert-verify"]);
                bean->caText = Node2Value<QString>(proxy["ca-str"]);
                bean->sni = Node2Value<QString>(proxy["sni"]);

                bean->obfsPassword = Node2Value<QString>(proxy["obfs-password"]);
                bean->password = Node2Value<QString>(proxy["password"]);

                bean->uploadMbps = Node2Value<int>(proxy["up"]);
                bean->downloadMbps = Node2Value<int>(proxy["down"]);
            } else if (type == "tuic") {
                auto bean = ent->Bean<NekoGui_fmt::QUICBean>();

                bean->uuid = Node2Value<QString>(proxy["uuid"]);
                bean->password = Node2Value<QString>(proxy["password"]);

                if (auto heartbeat = Node2Value<int>(proxy["heartbeat-interval"]); heartbeat > 0) {
                    bean->heartbeat = Int2String(heartbeat) + "ms";
                }

                bean->udpRelayMode = Node2Value<QString>(proxy["udp-relay-mode"], bean->udpRelayMode);
                bean->congestionControl = Node2Value<QString>(proxy["congestion-controller"], bean->congestionControl);

                bean->disableSni = Node2Value<bool>(proxy["disable-sni"]);
                bean->zeroRttHandshake = Node2Value<bool>(proxy["reduce-rtt"]);
                bean->allowInsecure = Node2Value<bool>(proxy["skip-cert-verify"]);
                bean->alpn = Node2Value<QList<QString>>(proxy["alpn"]).join(",");
                bean->caText = Node2Value<QString>(proxy["ca-str"]);
                bean->sni = Node2Value<QString>(proxy["sni"]);

                bean->uos = Node2Value<bool>(proxy["udp-over-stream"]);

                if (auto ip = Node2Value<QString>(proxy["ip"]); !ip.isEmpty()) {
                    if (bean->sni.isEmpty()) bean->sni = bean->serverAddress;
                    bean->serverAddress = ip;
                }
            } else if (type == "anytls") {
                auto bean = ent->Bean<NekoGui_fmt::AnyTLSBean>();
                bean->password = Node2Value<QString>(proxy["password"]);
                bean->stream->sni = Node2Value<QString>(proxy["sni"]);
                bean->stream->allow_insecure = Node2Value<bool>(proxy["skip-cert-verify"]);
            } else if (type == "ssh") {
                auto bean = ent->Bean<NekoGui_fmt::SSHBean>();
                bean->user = Node2Value<QString>(proxy["username"]);
                bean->password = Node2Value<QString>(proxy["password"]);
                bean->privateKey = Node2Value<QString>(proxy["private-key"]);
                bean->privateKeyPassphrase = Node2Value<QString>(proxy["private-key-passphrase"]);
                bean->hostKey = Node2Value<QList<QString>>(proxy["host-key"]).join(",");
                bean->hostKeyAlgorithms = Node2Value<QList<QString>>(proxy["host-key-algorithms"]).join(",");
            } else if (type == "wireguard") {
                auto bean = ent->Bean<NekoGui_fmt::WireGuardBean>();
                bean->publicKey = Node2Value<QString>(proxy["public-key"]);
                bean->preSharedKey = Node2Value<QString>(proxy["pre-shared-key"]);
                bean->reserved = Node2Value<QList<QString>>(proxy["reserved"]).join(",");
                bean->privateKey = Node2Value<QString>(proxy["private-key"]);
                bean->MTU = Node2Value<int>(proxy["mtu"], 1408);

                auto ip = Node2Value<QString>(proxy["ip"]);
                auto ipv6 = Node2Value<QString>(proxy["ipv6"]);
                bean->localAddress = ip.isEmpty() ? ipv6 : (ipv6.isEmpty() ? ip : ip + "," + ipv6);
            } else {
                if (diag) diag->append(QObject::tr("Unsupported proxy type: %1").arg(type));
                continue;
            }

            fixEnt(ent);
            result += ent;
        }

        return result;
    }

} // namespace NekoGui_sub