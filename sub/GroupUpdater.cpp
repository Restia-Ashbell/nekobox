#include "GroupUpdater.hpp"

#include <QInputDialog>

#include "db/ProfileFilter.hpp"
#include "fmt/includes.h"
#include "fmt/Preset.hpp"
#include "main/HTTPRequestHelper.hpp"
#include "ui/widget/GroupItem.h"
#include "ui/mainwindow.h"

#ifndef NKR_NO_YAML
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
#endif

namespace NekoGui_sub {

    GroupUpdater *groupUpdater = new GroupUpdater;

    void GroupUpdater::fixEnt(const std::shared_ptr<NekoGui::ProxyEntity> &ent) {
        if (ent == nullptr) return;
        auto stream = ent->bean->GetConfigItemPtr<NekoGui_fmt::V2rayStreamSettings>("stream");
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

    QList<std::shared_ptr<NekoGui::ProxyEntity>> GroupUpdater::update(const QString &str) {
        // Clash YAML
        if (str.contains("proxies:")) {
            return updateClash(str);
        }

        // Sing-Box JSON
        if (auto obj = QString2QJsonObject(str); !obj.isEmpty()) {
            return updateJson(str, obj);
        }

        // Base64
        QString decoded = DecodeB64IfValid(str);
        const QString &content = decoded.isEmpty() ? str : decoded;

        return updateLink(content);
    }

    QList<std::shared_ptr<NekoGui::ProxyEntity>> GroupUpdater::updateJson(const QString &str, const QJsonObject &obj) {
        auto ent = NekoGui::ProfileManager::NewProxyEntity("custom");
        auto bean = ent->CustomBean();
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

    QList<std::shared_ptr<NekoGui::ProxyEntity>> GroupUpdater::updateLink(const QString &str) {
        QList<std::shared_ptr<NekoGui::ProxyEntity>> result;
        std::shared_ptr<NekoGui::ProxyEntity> ent;

        for (const auto &line: str.split('\n', Qt::SkipEmptyParts)) {
            QString str = line.trimmed();
            QUrl link(str);

            const QString scheme = link.scheme();
            bool ok = false;

            // Nekoray format
            if (scheme == "nekoray") {
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
                MW_show_log(QObject::tr("Failed to parse: %1").arg(str));
            }
        }

        return result;
    }

#ifndef NKR_NO_YAML

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

#endif

    // https://github.com/Dreamacro/clash/wiki/configuration
    QList<std::shared_ptr<NekoGui::ProxyEntity>> GroupUpdater::updateClash(const QString &str) {
#ifndef NKR_NO_YAML
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
                    auto bean = ent->ShadowSocksBean();
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
                    bean->multiplex.enabled = Node2Value<bool>(smux["enabled"]);
                } else if (type == "shadowsocksr") {
                    auto bean = ent->ShadowSocksRBean();
                    bean->method = Node2Value<QString>(proxy["cipher"]).replace("dummy", "none");
                    bean->password = Node2Value<QString>(proxy["password"]);
                    bean->obfs = Node2Value<QString>(proxy["obfs"]);
                    bean->obfsParam = Node2Value<QString>(proxy["obfs-param"]);
                    bean->protocol = Node2Value<QString>(proxy["protocol"]);
                    bean->protocolParam = Node2Value<QString>(proxy["protocol-param"]);
                } else if (type == "socks" || type == "http") {
                    auto bean = ent->SocksHTTPBean();
                    bean->username = Node2Value<QString>(proxy["username"]);
                    bean->password = Node2Value<QString>(proxy["password"]);
                    if (type == "http") {
                        if (Node2Value<bool>(proxy["tls"])) bean->stream->security = "tls";
                        if (Node2Value<bool>(proxy["skip-cert-verify"])) bean->stream->allow_insecure = true;
                    }
                } else if (type == "trojan" || type == "vless") {
                    needFix = true;
                    auto bean = ent->TrojanVLESSBean();
                    if (type == "vless") {
                        bean->flow = Node2Value<QString>(proxy["flow"]);
                        bean->password = Node2Value<QString>(proxy["uuid"]);
                        // meta packet encoding
                        if (Node2Value<bool>(proxy["packet-addr"])) {
                            bean->stream->packet_encoding = "packetaddr";
                        } else {
                            // For VLESS, default to use xudp
                            bean->stream->packet_encoding = "xudp";
                        }
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
                    if (bean->stream->utlsFingerprint.isEmpty()) {
                        bean->stream->utlsFingerprint = NekoGui::dataStore->utlsFingerprint;
                    }

                    // sing-mux
                    auto smux = NodeChild(proxy, {"smux"});
                    bean->multiplex.enabled = Node2Value<bool>(smux["enabled"]);

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
                    auto bean = ent->VMessBean();
                    bean->uuid = Node2Value<QString>(proxy["uuid"]);
                    bean->aid = Node2Value<int>(proxy["alterId"]);
                    bean->security = Node2Value<QString>(proxy["cipher"], bean->security);
                    bean->stream->network = Node2Value<QString>(proxy["network"]);
                    bean->stream->sni = firstOrSecond(Node2Value<QString>(proxy["sni"]), Node2Value<QString>(proxy["servername"]));
                    bean->stream->alpn = Node2Value<QList<QString>>(proxy["alpn"]).join(",");
                    if (Node2Value<bool>(proxy["tls"])) bean->stream->security = "tls";
                    if (Node2Value<bool>(proxy["skip-cert-verify"])) bean->stream->allow_insecure = true;
                    bean->stream->utlsFingerprint = Node2Value<QString>(proxy["client-fingerprint"]);
                    if (bean->stream->utlsFingerprint.isEmpty()) {
                        bean->stream->utlsFingerprint = NekoGui::dataStore->utlsFingerprint;
                    }

                    // sing-mux
                    auto smux = NodeChild(proxy, {"smux"});
                    bean->multiplex.enabled = Node2Value<bool>(smux["enabled"]);

                    // meta packet encoding
                    if (Node2Value<bool>(proxy["xudp"])) bean->stream->packet_encoding = "xudp";
                    if (Node2Value<bool>(proxy["packet-addr"])) bean->stream->packet_encoding = "packetaddr";

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
                    auto bean = ent->QUICBean();

                    bean->hopPort = Node2Value<QString>(proxy["ports"]);

                    bean->allowInsecure = Node2Value<bool>(proxy["skip-cert-verify"]);
                    bean->caText = Node2Value<QString>(proxy["ca-str"]);
                    bean->alpn = Node2Value<QList<QString>>(proxy["alpn"]).join(",");
                    bean->sni = Node2Value<QString>(proxy["sni"]);

                    auto auth_str = firstOrSecond(Node2Value<QString>(proxy["auth_str"]), Node2Value<QString>(proxy["auth-str"]));
                    auto auth = Node2Value<QString>(proxy["auth"]);
                    if (!auth_str.isEmpty()) {
                        bean->authPayloadType = NekoGui_fmt::QUICBean::hysteria_auth_string;
                        bean->authPayload = auth_str;
                    }
                    if (!auth.isEmpty()) {
                        bean->authPayloadType = NekoGui_fmt::QUICBean::hysteria_auth_base64;
                        bean->authPayload = auth;
                    }
                    bean->obfsPassword = Node2Value<QString>(proxy["obfs"]);

                    if (Node2Value<bool>(proxy["disable_mtu_discovery"]) || Node2Value<bool>(proxy["disable-mtu-discovery"])) bean->disableMtuDiscovery = true;
                    bean->streamReceiveWindow = Node2Value<int>(proxy["recv-window"]);
                    bean->connectionReceiveWindow = Node2Value<int>(proxy["recv-window-conn"]);

                    auto upMbps = Node2Value<QString>(proxy["up"]).split(" ")[0].toInt();
                    auto downMbps = Node2Value<QString>(proxy["down"]).split(" ")[0].toInt();
                    if (upMbps > 0) bean->uploadMbps = upMbps;
                    if (downMbps > 0) bean->downloadMbps = downMbps;
                } else if (type == "hysteria2") {
                    auto bean = ent->QUICBean();

                    bean->hopPort = Node2Value<QString>(proxy["ports"]);

                    bean->allowInsecure = Node2Value<bool>(proxy["skip-cert-verify"]);
                    bean->caText = Node2Value<QString>(proxy["ca-str"]);
                    bean->sni = Node2Value<QString>(proxy["sni"]);

                    bean->obfsPassword = Node2Value<QString>(proxy["obfs-password"]);
                    bean->password = Node2Value<QString>(proxy["password"]);

                    bean->uploadMbps = Node2Value<QString>(proxy["up"]).split(" ")[0].toInt();
                    bean->downloadMbps = Node2Value<QString>(proxy["down"]).split(" ")[0].toInt();
                } else if (type == "tuic") {
                    auto bean = ent->QUICBean();

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
                    auto bean = ent->AnyTLSBean();
                    bean->password = Node2Value<QString>(proxy["password"]);
                    bean->stream->sni = firstOrSecond(Node2Value<QString>(proxy["sni"]), Node2Value<QString>(proxy["servername"]));
                    bean->stream->allow_insecure = Node2Value<bool>(proxy["skip-cert-verify"]);
                } else if (type == "ssh") {
                    auto bean = ent->SSHBean();
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

                    auto bean = ent->WireGuardBean();
                    bean->serverAddress = getFieldValue("server");
                    bean->serverPort = getFieldValue("port").toInt();
                    bean->publicKey = getFieldValue("public-key");
                    bean->preSharedKey = getFieldValue("pre-shared-key");
                    bean->reserved = getFieldValue("reserved");
                    bean->privateKey = Node2Value<QString>(proxy["private-key"]);
                    bean->MTU = Node2Value<int>(proxy["mtu"], 1408);

                    QString ip = Node2Value<QString>(proxy["ip"]);
                    QString ipv6 = Node2Value<QString>(proxy["ipv6"]);
                    bean->localAddress = ip.isEmpty() ? ipv6 : (ipv6.isEmpty() ? ip : ip + "," + ipv6);
                } else {
                    continue;
                }

                if (needFix) fixEnt(ent);
                result += ent;
            }
        } catch (const YAML::Exception &ex) {
            runOnUiThread([=, this] {
                MessageBoxWarning("YAML Exception", ex.what());
            });
        }

        return result;
#endif
    }

    // 在新的 thread 运行
    void GroupUpdater::AsyncUpdate(const QString &str, int _sub_gid, const std::function<void()> &finish) {
        auto content = str.trimmed();
        bool asURL = _sub_gid >= 0;
        bool createNewGroup = false;
        QUrl url(content);

        if (_sub_gid < 0 && url.isValid() && (url.scheme() == "http" || url.scheme() == "https") && url.userInfo().isEmpty()) {
            auto items = QStringList{
                QObject::tr("As Subscription (create new group)"),
                QObject::tr("As Subscription (add to this group)"),
                QObject::tr("As link"),
            };
            bool ok;
            auto a = QInputDialog::getItem(GetMessageBoxParent(),
                                           QObject::tr("url detected"),
                                           QObject::tr("%1\nHow to update?").arg(content),
                                           items, 0, false, &ok);
            if (!ok) return;
            if (items.indexOf(a) <= 1) asURL = true;
            if (items.indexOf(a) == 0) {
                createNewGroup = true;
                auto group = NekoGui::ProfileManager::NewGroup();
                group->url = content;
                NekoGui::profileManager->AddGroup(group);
                _sub_gid = group->id;
            }
        }

        runOnNewThread([=, this] {
            Update(content, _sub_gid, asURL);
            runOnUiThread([=] {
                auto *mw = MainWindow::instance();
                createNewGroup ? mw->refresh_groups() : mw->refresh_group(_sub_gid);
                emit mw->groupUpdated(_sub_gid);
            });
            if (finish) finish();
        });
    }

    void GroupUpdater::Update(QString content, int _sub_gid, bool asURL) {
        // 准备
        QString sub_user_info, contentDisposition;
        auto group = NekoGui::profileManager->GetGroup(_sub_gid);
        if (group != nullptr && group->archive) return;

        // 网络请求
        if (asURL) {
            auto groupName = group && !group->name.isEmpty() ? group->name : content;
            MW_show_log(">>>>>>>> " + QObject::tr("Requesting subscription: %1").arg(groupName));

            auto resp = NekoGui_network::NetworkRequestHelper::HttpGet(content);
            if (!resp.error.isEmpty()) {
                MW_show_log("<<<<<<<< " + QObject::tr("Requesting subscription %1 error: %2").arg(groupName, resp.error + "\n" + resp.data));
                return;
            }

            content = resp.data.trimmed();
            sub_user_info = NekoGui_network::NetworkRequestHelper::GetHeader(resp.headers, "Subscription-UserInfo");
            contentDisposition = NekoGui_network::NetworkRequestHelper::GetHeader(resp.headers, "Content-Disposition");

            MW_show_log("<<<<<<<< " + QObject::tr("Subscription request fininshed: %1").arg(groupName));
        }

        auto newProfiles = update(content);

        if (group) {
            auto [oldCommon, newCommon, oldOnly, newOnly] = NekoGui::ProfileFilter::Diff(group->Profiles(), newProfiles);

            if (group->name.isEmpty()) {
                QString parsedName = GroupItem::parseFileName(contentDisposition);
                group->name = parsedName.isEmpty() ? QUrl(group->url).host() : parsedName;
            }
            group->sub_last_update = QDateTime::currentSecsSinceEpoch();
            group->info = sub_user_info;

            QString notice_added;
            QString notice_deleted;
            for (const auto &ent: newOnly) {
                NekoGui::profileManager->AddProfile(ent, _sub_gid);
                notice_added += "[+] " + ent->bean->DisplayTypeAndName() + "\n";
            }
            for (const auto &ent: oldOnly) {
                NekoGui::profileManager->DeleteProfile(ent->id);
                notice_deleted += "[-] " + ent->bean->DisplayTypeAndName() + "\n";
            }

            // sort according to order in remote
            group->order.clear();
            for (const auto &ent: newProfiles) {
                auto deleted_index = newCommon.indexOf(ent);
                if (deleted_index >= 0) {
                    group->order.append(oldCommon[deleted_index]->id);
                } else {
                    group->order.append(ent->id);
                }
            }
            group->Save();

            QString change_text = newOnly.length() + oldOnly.length() == 0
                                      ? QObject::tr("Nothing")
                                      : QObject::tr("Added %1 profiles:\n%2\nDeleted %3 Profiles:\n%4")
                                            .arg(newOnly.length())
                                            .arg(notice_added)
                                            .arg(oldOnly.length())
                                            .arg(notice_deleted);

            MW_show_log("<<<<<<<< " + QObject::tr("Change of %1:").arg(group->name) + "\n" + change_text);
        } else {
            for (const auto &ent: newProfiles) NekoGui::profileManager->AddProfile(ent, _sub_gid);
            MW_show_log(QObject::tr("Imported %1 profile(s)").arg(newProfiles.count()));
        }
    }
} // namespace NekoGui_sub

bool UI_update_all_groups_Updating = false;

void serialUpdateSubscription(const QList<int> &groupsTabOrder, int index, bool isAutoUpdate) {
    while (index < groupsTabOrder.size()) {
        auto g = NekoGui::profileManager->GetGroup(groupsTabOrder[index]);
        if (!(!g || g->url.isEmpty() || g->archive || (isAutoUpdate && g->skip_auto_update))) {
            UI_update_all_groups_Updating = true;
            NekoGui_sub::groupUpdater->AsyncUpdate(g->url, g->id, [=] {
                serialUpdateSubscription(groupsTabOrder, index + 1, isAutoUpdate);
            });
            return;
        }
        ++index;
    }

    UI_update_all_groups_Updating = false;
}

void UI_update_all_groups(bool isAutoUpdate) {
    if (UI_update_all_groups_Updating) {
        MW_show_log("The last subscription update has not exited.");
        return;
    }

    serialUpdateSubscription(NekoGui::profileManager->groupsTabOrder, 0, isAutoUpdate);
}
