#include "SubscriptionParser.hpp"

#include <QLocale>
#include <QRegularExpression>

#include "common/Utils.hpp"
#include "profile/ProfileManager.hpp"
#include "protocol/Includes.hpp"

#include <yaml-cpp/yaml.h>

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

namespace NekoGui_sub {

    // 解析 Subscription-UserInfo 响应头：total / upload / download / expire
    QString SubscriptionParser::parseSubInfo(const QString &info) {
        if (info.trimmed().isEmpty()) return {};

        long long used = 0, total = 0, expire = 0;

        QRegularExpressionMatch match;
        match = QRegularExpression("total=([0-9]+)").match(info);
        if (match.hasMatch()) {
            total = match.captured(1).toLongLong();
        }
        match = QRegularExpression("upload=([0-9]+)").match(info);
        if (match.hasMatch()) {
            used += match.captured(1).toLongLong();
        }
        match = QRegularExpression("download=([0-9]+)").match(info);
        if (match.hasMatch()) {
            used += match.captured(1).toLongLong();
        }
        match = QRegularExpression("expire=([0-9]+)").match(info);
        if (match.hasMatch()) {
            expire = match.captured(1).toLongLong();
        }
        if (used == 0 && total == 0 && expire == 0) return {};

        return QObject::tr("Used: %1 Remain: %2 Expire: %3").arg(ReadableSize(used), ReadableSize(total - used), DisplayTime(expire, QLocale::ShortFormat));
    }

    // 解析 Content-Disposition 响应头里的分组名（支持 filename* 与 filename）
    QString SubscriptionParser::parseSubName(const QString &contentDisposition) {
        if (contentDisposition.trimmed().isEmpty()) return {};
        QRegularExpressionMatch match;
        QRegularExpression reFilenameStar(R"(filename\*\s*=\s*UTF-8''([^;]+))", QRegularExpression::CaseInsensitiveOption);
        match = reFilenameStar.match(contentDisposition);
        if (match.hasMatch()) {
            return QUrl::fromPercentEncoding(match.captured(1).toUtf8());
        }
        QRegularExpression reFilename(R"REGEX(filename\s*=\s*(?:"([^"]+)"|([^;]+)))REGEX", QRegularExpression::CaseInsensitiveOption);
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
        // Clash YAML
        if (str.contains("proxies:")) {
            return updateClash(str, diag);
        }

        // Sing-Box JSON
        if (auto obj = QString2QJsonObject(str); !obj.isEmpty()) {
            return updateJson(str, obj);
        }

        // Base64
        QString decoded = DecodeB64IfValid(str);
        const QString &content = decoded.isEmpty() ? str : decoded;

        return updateLink(content, diag);
    }

    QList<std::shared_ptr<NekoGui::ProxyEntity>> SubscriptionParser::updateJson(const QString &str, const QJsonObject &obj) {
        auto ent = NekoGui::ProfileManager::NewProxyEntity("custom");
        auto bean = ent->Bean<NekoGui_fmt::CustomBean>();
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
        std::shared_ptr<NekoGui::ProxyEntity> ent;

        for (const auto &line: str.split('\n', Qt::SkipEmptyParts)) {
            QString str = line.trimmed();
            QUrl link(str);

            const QString scheme = link.scheme();
            bool ok = false;

            if (scheme == "nekobox") {
                if (!link.isValid()) continue;
                ent = NekoGui::ProfileManager::NewProxyEntity(link.host());
                if (!ent->bean) continue;
                auto j = DecodeB64IfValid(link.fragment(), QByteArray::Base64UrlEncoding);
                if (j.isEmpty()) continue;
                ent->bean->FromJsonBytes(j);
                ok = true;
            } else {
                ent = NekoGui::ProfileManager::NewProxyEntity(scheme);
                ok = ent->bean && ent->bean->TryParseLink(str);
            }

            if (ok) {
                // Fix
                fixEnt(ent);

                // End
                result += ent;
            } else {
                if (diag) diag->append(QObject::tr("Failed to parse: %1").arg(str));
            }
        }

        return result;
    }

    template<typename T>
    T Node2Value(const YAML::Node &node, const T &def = T{}) {
        try {
            return node.as<T>();
        } catch (const YAML::Exception &ex) {
            qDebug() << ex.what();
        }
        return def;
    }

    // NodeChild returns the first defined children or Null Node
    YAML::Node NodeChild(const YAML::Node &node, const std::list<std::string> &keys) {
        for (const auto &key: keys) {
            auto child = node[key];
            if (child.IsDefined()) return child;
        }
        return {};
    }

    // https://github.com/Dreamacro/clash/wiki/configuration
    QList<std::shared_ptr<NekoGui::ProxyEntity>> SubscriptionParser::updateClash(const QString &str, QStringList *diag) {
        QList<std::shared_ptr<NekoGui::ProxyEntity>> result;

        try {
            auto proxies = YAML::Load(str.toStdString())["proxies"];
            for (auto proxy: proxies) {
                auto type = Node2Value<QString>(proxy["type"]).toLower();

                if (type == "socks5") type = "socks";
                if (type == "ss") type = "shadowsocks";
                if (type == "ssr") type = "shadowsocksr";

                auto ent = NekoGui::ProfileManager::NewProxyEntity(type);
                if (!ent->bean) continue;
                bool needFix = false;

                // common
                ent->bean->name = Node2Value<QString>(proxy["name"]);
                ent->bean->serverAddress = Node2Value<QString>(proxy["server"]);
                ent->bean->serverPort = Node2Value<int>(proxy["port"]);

                if (type == "shadowsocks") {
                    auto bean = ent->Bean<NekoGui_fmt::ShadowSocksBean>();
                    bean->method = Node2Value<QString>(proxy["cipher"]).replace("dummy", "none");
                    bean->password = Node2Value<QString>(proxy["password"]);
                    auto plugin_n = proxy["plugin"];
                    auto pluginOpts_n = proxy["plugin-opts"];

                    // UDP over TCP
                    if (Node2Value<bool>(proxy["udp-over-tcp"])) {
                        bean->uot = Node2Value<int>(proxy["udp-over-tcp-version"]);
                        if (bean->uot == 0) bean->uot = 2;
                    }

                    if (plugin_n.IsDefined() && pluginOpts_n.IsDefined()) {
                        QStringList ssPlugin;
                        auto plugin = Node2Value<QString>(plugin_n);
                        if (plugin == "obfs") {
                            ssPlugin << "obfs-local";
                            ssPlugin << "obfs=" + Node2Value<QString>(pluginOpts_n["mode"]);
                            ssPlugin << "obfs-host=" + Node2Value<QString>(pluginOpts_n["host"]);
                        } else if (plugin == "v2ray-plugin") {
                            auto mode = Node2Value<QString>(pluginOpts_n["mode"]);
                            auto host = Node2Value<QString>(pluginOpts_n["host"]);
                            auto path = Node2Value<QString>(pluginOpts_n["path"]);
                            ssPlugin << "v2ray-plugin";
                            if (!mode.isEmpty() && mode != "websocket") ssPlugin << "mode=" + mode;
                            if (Node2Value<bool>(pluginOpts_n["tls"])) ssPlugin << "tls";
                            if (!host.isEmpty()) ssPlugin << "host=" + host;
                            if (!path.isEmpty()) ssPlugin << "path=" + path;
                            // clash only: skip-cert-verify
                            // clash only: headers
                            // clash: mux=?
                        }
                        bean->plugin = ssPlugin.join(";");
                    }

                    // sing-mux
                    auto smux = NodeChild(proxy, {"smux"});
                    bean->multiplex->enabled = Node2Value<bool>(smux["enabled"]);
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
                    needFix = true;
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

                    // sing-mux
                    auto smux = NodeChild(proxy, {"smux"});
                    bean->multiplex->enabled = Node2Value<bool>(smux["enabled"]);

                    // opts
                    auto ws = NodeChild(proxy, {"ws-opts", "ws-opt"});
                    if (ws.IsMap()) {
                        auto headers = ws["headers"];
                        for (auto header: headers) {
                            if (Node2Value<QString>(header.first).toLower() == "host") {
                                bean->stream->host = Node2Value<QString>(header.second);
                            }
                        }
                        bean->stream->path = Node2Value<QString>(ws["path"]);
                        bean->stream->ws_early_data_length = Node2Value<int>(ws["max-early-data"]);
                        bean->stream->ws_early_data_name = Node2Value<QString>(ws["early-data-header-name"]);
                        if (Node2Value<bool>(ws["v2ray-http-upgrade"])) bean->stream->network = "httpupgrade";
                    }

                    auto grpc = NodeChild(proxy, {"grpc-opts", "grpc-opt"});
                    if (grpc.IsMap()) {
                        bean->stream->path = Node2Value<QString>(grpc["grpc-service-name"]);
                    }

                    auto h2 = NodeChild(proxy, {"h2-opts", "h2-opt"});
                    if (h2.IsMap()) {
                        bean->stream->host = Node2Value<QList<QString>>(h2["host"]).join(",");
                        bean->stream->path = Node2Value<QString>(h2["path"]);
                    }

                    auto http = NodeChild(proxy, {"http-opts", "http-opt"});
                    if (http.IsMap()) {
                        auto headers = http["headers"];
                        for (auto header: headers) {
                            if (Node2Value<QString>(header.first).toLower() == "host") {
                                bean->stream->host = Node2Value<QList<QString>>(header.second).join(",");
                                break;
                            }
                        }
                        bean->stream->path = Node2Value<QString>(http["path"][0]);
                    }

                    auto reality = NodeChild(proxy, {"reality-opts"});
                    if (reality.IsMap()) {
                        bean->stream->reality_pbk = Node2Value<QString>(reality["public-key"]);
                        bean->stream->reality_sid = Node2Value<QString>(reality["short-id"]);
                    }
                } else if (type == "vmess") {
                    needFix = true;
                    auto bean = ent->Bean<NekoGui_fmt::VMessBean>();
                    bean->uuid = Node2Value<QString>(proxy["uuid"]);
                    bean->aid = Node2Value<int>(proxy["alterId"]);
                    bean->security = Node2Value<QString>(proxy["cipher"], bean->security);
                    bean->stream->network = Node2Value<QString>(proxy["network"]);
                    bean->stream->sni = firstOrSecond(Node2Value<QString>(proxy["sni"]), Node2Value<QString>(proxy["servername"]));
                    bean->stream->alpn = Node2Value<QList<QString>>(proxy["alpn"]).join(",");
                    if (Node2Value<bool>(proxy["tls"])) bean->stream->security = "tls";
                    if (Node2Value<bool>(proxy["skip-cert-verify"])) bean->stream->allow_insecure = true;
                    bean->stream->utlsFingerprint = Node2Value<QString>(proxy["client-fingerprint"]);

                    // sing-mux
                    auto smux = NodeChild(proxy, {"smux"});
                    bean->multiplex->enabled = Node2Value<bool>(smux["enabled"]);

                    // meta packet encoding
                    bean->stream->packet_encoding = Node2Value<QString>(proxy["packet-encoding"]);

                    // opts
                    auto ws = NodeChild(proxy, {"ws-opts", "ws-opt"});
                    if (ws.IsMap()) {
                        auto headers = ws["headers"];
                        for (auto header: headers) {
                            if (Node2Value<QString>(header.first).toLower() == "host") {
                                bean->stream->host = Node2Value<QString>(header.second);
                            }
                        }
                        bean->stream->path = Node2Value<QString>(ws["path"]);
                        bean->stream->ws_early_data_length = Node2Value<int>(ws["max-early-data"]);
                        bean->stream->ws_early_data_name = Node2Value<QString>(ws["early-data-header-name"]);
                        if (Node2Value<bool>(ws["v2ray-http-upgrade"])) bean->stream->network = "httpupgrade";
                    }

                    auto grpc = NodeChild(proxy, {"grpc-opts", "grpc-opt"});
                    if (grpc.IsMap()) {
                        bean->stream->path = Node2Value<QString>(grpc["grpc-service-name"]);
                    }

                    auto h2 = NodeChild(proxy, {"h2-opts", "h2-opt"});
                    if (h2.IsMap()) {
                        bean->stream->host = Node2Value<QList<QString>>(h2["host"]).join(",");
                        bean->stream->path = Node2Value<QString>(h2["path"]);
                    }

                    auto http = NodeChild(proxy, {"http-opts", "http-opt"});
                    if (http.IsMap()) {
                        auto headers = http["headers"];
                        for (auto header: headers) {
                            if (Node2Value<QString>(header.first).toLower() == "host") {
                                bean->stream->host = Node2Value<QList<QString>>(header.second).join(",");
                                break;
                            }
                        }
                        bean->stream->path = Node2Value<QString>(http["path"][0]);
                    }

                    auto reality = NodeChild(proxy, {"reality-opts"});
                    if (reality.IsMap()) {
                        bean->stream->reality_pbk = Node2Value<QString>(reality["public-key"]);
                        bean->stream->reality_sid = Node2Value<QString>(reality["short-id"]);
                    }
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
                    bean->streamReceiveWindow = Node2Value<int>(proxy["recv-window"]);
                    bean->connectionReceiveWindow = Node2Value<int>(proxy["recv-window-conn"]);

                    auto upMbps = Node2Value<QString>(proxy["up"]).split(" ")[0].toInt();
                    auto downMbps = Node2Value<QString>(proxy["down"]).split(" ")[0].toInt();
                    if (upMbps > 0) bean->uploadMbps = upMbps;
                    if (downMbps > 0) bean->downloadMbps = downMbps;
                } else if (type == "hysteria2") {
                    auto bean = ent->Bean<NekoGui_fmt::QUICBean>();

                    bean->hopPort = Node2Value<QString>(proxy["ports"]);

                    bean->allowInsecure = Node2Value<bool>(proxy["skip-cert-verify"]);
                    bean->caText = Node2Value<QString>(proxy["ca-str"]);
                    bean->sni = Node2Value<QString>(proxy["sni"]);

                    bean->obfsPassword = Node2Value<QString>(proxy["obfs-password"]);
                    bean->password = Node2Value<QString>(proxy["password"]);

                    bean->uploadMbps = Node2Value<QString>(proxy["up"]).split(" ")[0].toInt();
                    bean->downloadMbps = Node2Value<QString>(proxy["down"]).split(" ")[0].toInt();
                } else if (type == "tuic") {
                    auto bean = ent->Bean<NekoGui_fmt::QUICBean>();

                    bean->uuid = Node2Value<QString>(proxy["uuid"]);
                    bean->password = Node2Value<QString>(proxy["password"]);

                    if (Node2Value<int>(proxy["heartbeat-interval"]) != 0) {
                        bean->heartbeat = Int2String(Node2Value<int>(proxy["heartbeat-interval"])) + "ms";
                    }

                    bean->udpRelayMode = Node2Value<QString>(proxy["udp-relay-mode"], bean->udpRelayMode);
                    bean->congestionControl = Node2Value<QString>(proxy["congestion-controller"], bean->congestionControl);

                    bean->disableSni = Node2Value<bool>(proxy["disable-sni"]);
                    bean->zeroRttHandshake = Node2Value<bool>(proxy["reduce-rtt"]);
                    bean->allowInsecure = Node2Value<bool>(proxy["skip-cert-verify"]);
                    bean->alpn = Node2Value<QList<QString>>(proxy["alpn"]).join(",");
                    bean->caText = Node2Value<QString>(proxy["ca-str"]);
                    bean->sni = Node2Value<QString>(proxy["sni"]);

                    if (Node2Value<bool>(proxy["udp-over-stream"])) bean->uos = true;

                    if (!Node2Value<QString>(proxy["ip"]).isEmpty()) {
                        if (bean->sni.isEmpty()) bean->sni = bean->serverAddress;
                        bean->serverAddress = Node2Value<QString>(proxy["ip"]);
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
                    auto getFieldValue = [&](const auto &key) -> QString {
                        auto node = proxy[key] ? proxy[key] : proxy["peers"][0][key];
                        return node.IsSequence() ? Node2Value<QList<QString>>(node).join(",") : Node2Value<QString>(node);
                    };

                    auto bean = ent->Bean<NekoGui_fmt::WireGuardBean>();
                    bean->serverAddress = getFieldValue("server");
                    bean->serverPort = getFieldValue("port").toInt();
                    bean->publicKey = getFieldValue("public-key");
                    bean->preSharedKey = getFieldValue("pre-shared-key");
                    bean->reserved = getFieldValue("reserved");
                    bean->privateKey = Node2Value<QString>(proxy["private-key"]);
                    bean->MTU = Node2Value<int>(proxy["mtu"], 1408);

                    auto ip = Node2Value<QString>(proxy["ip"]);
                    auto ipv6 = Node2Value<QString>(proxy["ipv6"]);
                    bean->localAddress = ip.isEmpty() ? ipv6 : (ipv6.isEmpty() ? ip : ip + "," + ipv6);
                } else {
                    continue;
                }

                if (needFix) fixEnt(ent);
                result += ent;
            }
        } catch (const YAML::Exception &ex) {
            if (diag) diag->append(QString("YAML Exception: %1").arg(QString::fromUtf8(ex.what())));
        }

        return result;
    }

} // namespace NekoGui_sub