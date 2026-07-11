#include "profile/ConfigBuilder.hpp"
#include "profile/ProfileManager.hpp"
#include "protocol/Includes.hpp"
#include "protocol/Preset.hpp"
#include "system/AdminHelper.hpp"

namespace NekoGui {

    void MergeJson(const QJsonObject &custom, QJsonObject &outbound) {
        // 合并
        if (custom.isEmpty()) return;
        for (const auto &key: custom.keys()) {
            if (outbound.contains(key)) {
                auto v = custom[key];
                auto v_orig = outbound[key];
                if (v.isObject() && v_orig.isObject()) { // isObject 则合并？
                    QJsonObject vo = v.toObject();
                    QJsonObject vo_orig = v_orig.toObject();
                    MergeJson(vo, vo_orig);
                    outbound[key] = vo_orig;
                } else {
                    outbound[key] = v;
                }
            } else {
                outbound[key] = custom[key];
            }
        }
    }

    QJsonObject parseLinesToJson(const QString &text) {
        QJsonObject result;

        for (const auto &raw: text.split('\n', Qt::SkipEmptyParts)) {
            QString line = raw.trimmed();
            if (line.isEmpty() || line.startsWith('#')) continue;

            auto idx = line.indexOf(':');
            if (idx <= 0) continue;

            const QString key = line.left(idx).trimmed();
            const QString valueStr = line.mid(idx + 1).trimmed();

            QJsonValue value;
            const QString lower = valueStr.toLower();
            if (lower == "true")
                value = true;
            else if (lower == "false")
                value = false;
            else if (lower == "null")
                value = QJsonValue();
            else {
                bool okInt = false;
                int intVal = valueStr.toInt(&okInt);
                if (okInt)
                    value = intVal;
                else {
                    bool okDouble = false;
                    double dblVal = valueStr.toDouble(&okDouble);
                    if (okDouble)
                        value = dblVal;
                    else
                        value = valueStr;
                }
            }

            if (!result.contains(key)) {
                result.insert(key, value);
            } else {
                QJsonValue existing = result.value(key);
                QJsonArray arr;
                if (existing.isArray()) {
                    arr = existing.toArray();
                } else {
                    arr.append(existing);
                }
                arr.append(value);
                result[key] = arr;
            }
        }

        return result;
    }

    QPair<QString, QString> parseDnsAddress(const QString &input) {
        QUrl url(input);
        if (input == "local") {
            return {input, ""};
        } else if (url.scheme() == "dhcp") {
            return {url.scheme(), ""};
        } else if (!url.scheme().isEmpty()) {
            return {url.scheme(), url.host()};
        } else {
            return {"udp", input};
        }
    }

    // Common

    std::shared_ptr<BuildConfigResult> BuildConfig(const std::shared_ptr<ProxyEntity> &ent, bool forTest, bool forExport) {
        auto result = std::make_shared<BuildConfigResult>();
        auto status = std::make_shared<BuildConfigStatus>();
        status->ent = ent;
        status->result = result;
        status->forTest = forTest;
        status->forExport = forExport;

        auto customBean = dynamic_cast<NekoGui_fmt::CustomBean *>(ent->bean.get());
        if (customBean != nullptr && customBean->core == "internal-full") {
            result->coreConfig = QString2QJsonObject(customBean->config_simple);
        } else {
            BuildConfigSingBox(status);
        }

        // apply custom config
        MergeJson(QString2QJsonObject(ent->bean->custom_config), result->coreConfig);

        return result;
    }

    QString BuildChain(int chainId, const std::shared_ptr<BuildConfigStatus> &status) {
        auto group = profileManager->GetGroup(status->ent->gid);
        if (group == nullptr) {
            status->result->error = QString("This profile is not in any group, your data may be corrupted.");
            return {};
        }

        auto resolveChain = [=](const std::shared_ptr<ProxyEntity> &ent) {
            QList<std::shared_ptr<ProxyEntity>> resolved;
            if (ent->type == "chain") {
                auto list = ent->Bean<NekoGui_fmt::ChainBean>()->list;
                for (auto id: list) {
                    resolved += profileManager->GetProfile(id);
                    if (resolved.last() == nullptr) {
                        status->result->error = QString("chain missing ent: %1").arg(id);
                        break;
                    }
                    if (resolved.last()->type == "chain") {
                        status->result->error = QString("chain in chain is not allowed: %1").arg(id);
                        break;
                    }
                }
            } else {
                resolved += ent;
            };
            return resolved;
        };

        // Make list
        auto ents = resolveChain(status->ent);
        if (!status->result->error.isEmpty()) return {};

        if (group->front_proxy_id >= 0) {
            auto fEnt = profileManager->GetProfile(group->front_proxy_id);
            if (fEnt == nullptr) {
                status->result->error = QString("front proxy ent not found.");
                return {};
            }
            ents += resolveChain(fEnt);
            if (!status->result->error.isEmpty()) return {};
        }

        // BuildChain
        QString chainTagOut = BuildChainInternal(0, ents, status);

        // Chain ent traffic stat
        if (ents.length() > 1) {
            status->ent->traffic_data->id = status->ent->id;
            status->ent->traffic_data->tag = chainTagOut;
            status->result->outboundStats += status->ent->traffic_data;
        }
        status->result->outboundStat = ents[ents.length() - 1]->traffic_data;

        return chainTagOut;
    }

    QString BuildChainInternal(int chainId, const QList<std::shared_ptr<ProxyEntity>> &ents,
                               const std::shared_ptr<BuildConfigStatus> &status) {
        QString chainTag = "chain-" + Int2String(chainId);
        QString chainTagOut = "proxy";

        QString pastTag;

        for (int index = 0; index < ents.length(); index++) {
            // tagOut: v2ray outbound tag for a profile
            // profile2 (in) (global)   tag g-(id)
            // profile1                 tag (chainTag)-(id)
            // profile0 (out)           tag (chainTag)-(id) / single: chainTag=g-(id)
            auto ent = ents[index];
            auto tagOut = index == ents.length() - 1 ? chainTagOut : chainTag + "-" + Int2String(ent->id);

            // Outbound

            QJsonObject outbound;

            if (ent->bean->external) {
                auto ext_mapping_port = 0;
                auto ext_socks_port = MkPort();
                if (index > 0) {
                    // mapping
                    ext_mapping_port = MkPort();
                    status->inbounds += QJsonObject{
                        {"type", "direct"},
                        {"tag", tagOut + "-mapping"},
                        {"listen", "127.0.0.1"},
                        {"listen_port", ext_mapping_port},
                        {"override_address", ent->bean->serverAddress},
                        {"override_port", ent->bean->serverPort},
                    };
                    status->routingRules += QJsonObject{
                        {"inbound", tagOut + "-mapping"},
                        {"outbound", pastTag},
                    };
                }
                auto extR = ent->bean->BuildExternal(ext_mapping_port, ext_socks_port);
                if (extR.program.isEmpty()) {
                    status->result->error = QObject::tr("Core not found: %1").arg(ent->bean->DisplayCoreType());
                    return {};
                }
                if (!extR.error.isEmpty()) { // rejected
                    status->result->error = extR.error;
                    return {};
                }
                extR.tag = ent->bean->DisplayType();
                status->result->extRs.emplace_back(std::make_shared<NekoGui_fmt::ExternalBuildResult>(extR));

                // SOCKS OUTBOUND
                outbound["type"] = "socks";
                outbound["server"] = "127.0.0.1";
                outbound["server_port"] = ext_socks_port;
            } else {
                const auto coreR = ent->bean->BuildCoreObjSingBox();
                if (coreR.outbound.isEmpty()) {
                    status->result->error = "unsupported outbound";
                    return {};
                }
                if (!coreR.error.isEmpty()) { // rejected
                    status->result->error = coreR.error;
                    return {};
                }
                outbound = coreR.outbound;
                if (index > 0) {
                    outbound["detour"] = pastTag;
                }
            }

            // outbound misc
            outbound["tag"] = tagOut;
            ent->traffic_data->id = ent->id;
            ent->traffic_data->tag = tagOut;
            status->result->outboundStats += ent->traffic_data;

            // common
            // apply domain_strategy
            if (!dataStore->routing->outbound_domain_strategy.isEmpty())
                outbound["domain_strategy"] = dataStore->routing->outbound_domain_strategy;

            // apply custom outbound settings
            // MergeJson(QString2QJsonObject(ent->bean->custom_outbound), outbound);

            status->outbounds.prepend(outbound);
            if (!status->forTest) {
                QJsonObject customOutboundObj = QString2QJsonObject(ent->bean->custom_outbound);
                if (!customOutboundObj.isEmpty()) {
                    status->outbounds += customOutboundObj;
                }
            }
            pastTag = tagOut;
        }

        return chainTagOut;
    }

    // SingBox

    void BuildConfigSingBox(const std::shared_ptr<BuildConfigStatus> &status) {
        // Log
        status->result->coreConfig["log"] = QJsonObject{
            {"disabled", status->forTest ? true : dataStore->log_disabled},
            {"timestamp", dataStore->log_timestamp},
            {"level", dataStore->log_level},
        };

        if (!status->forTest) {
            // NTP
            if (dataStore->ntp_enabled) {
                status->result->coreConfig["ntp"] = QJsonObject{
                    {"enabled", true},
                    {"server", dataStore->ntp_server},
                    {"server_port", dataStore->ntp_server_port},
                    {"interval", dataStore->ntp_interval}};
            }

            // Certificate
            auto certificateObj = QJsonObject{{"store", dataStore->certificate_store}};
            if (!dataStore->certificate.isEmpty()) certificateObj["certificate"] = dataStore->certificate;
            if (!dataStore->certificate_path.isEmpty()) certificateObj["certificate_path"] = dataStore->certificate_path;
            if (!dataStore->certificate_directory_path.isEmpty()) certificateObj["certificate_directory_path"] = dataStore->certificate_directory_path;
            status->result->coreConfig["certificate"] = certificateObj;

            // Inbounds
            // mixed-in
            if (IsValidPort(dataStore->inbound_port)) {
                QJsonObject inboundObj;
                inboundObj["tag"] = "mixed-in";
                inboundObj["type"] = "mixed";
                inboundObj["listen"] = dataStore->inbound_address;
                inboundObj["listen_port"] = dataStore->inbound_port;
                if (dataStore->inbound_auth->NeedAuth()) {
                    inboundObj["users"] = QJsonArray{
                        QJsonObject{
                            {"username", dataStore->inbound_auth->username},
                            {"password", dataStore->inbound_auth->password},
                        },
                    };
                }
                status->inbounds += inboundObj;
            }

            // tun-in
            if (dataStore->spmode_vpn) {
#ifdef Q_OS_MACOS
                auto tun_name = "";
#else
                auto tun_name = "nekobox-tun";
#endif
                QJsonObject inboundObj;
                inboundObj["tag"] = "tun-in";
                inboundObj["type"] = "tun";
                inboundObj["interface_name"] = tun_name;
                inboundObj["auto_route"] = true;
                // inboundObj["endpoint_independent_nat"] = true;
                inboundObj["mtu"] = dataStore->tun_mtu;
                inboundObj["stack"] = dataStore->tun_stack;
                inboundObj["strict_route"] = dataStore->tun_strict_route;
                inboundObj["address"] = dataStore->tun_ipv6 ? QJsonArray{"172.18.0.1/30", "fdfe:dcba:9876::1/126"} : QJsonArray{"172.18.0.1/30"};
                status->inbounds += inboundObj;
            }

            // custom inbound
            for (const auto &inbound: QString2QJsonObject(dataStore->custom_inbound)["inbounds"].toArray()) status->inbounds.append(inbound);
        }

        // Outbounds
        auto tagProxy = BuildChain(0, status);
        if (!status->result->error.isEmpty()) return;

        if (!status->forTest) {
            status->outbounds += QJsonObject{
                {"type", "direct"},
                {"tag", "direct"},
            };
        }

        // user rule
        QJsonObject block_rules;
        QJsonObject proxy_rules;
        QJsonObject direct_rules;
        if (!status->forTest) {
            block_rules = parseLinesToJson(dataStore->routing->block_rules);
            proxy_rules = parseLinesToJson(dataStore->routing->proxy_rules);
            direct_rules = parseLinesToJson(dataStore->routing->direct_rules);
        }
        auto custom_routeObj = QString2QJsonObject(dataStore->routing->custom);

        // DNS
        QJsonObject dns;
        QJsonArray dnsServers;
        QJsonArray dnsRules;
        // Remote
        if (!status->forTest) {
            auto remoteDnsData = parseDnsAddress(dataStore->routing->remote_dns);
            QJsonObject dnsRemoteServer{
                {"tag", "dns-remote"},
                {"type", remoteDnsData.first},
                {"domain_resolver", "dns-local"},
            };
            if (tagProxy != "direct") dnsRemoteServer["detour"] = tagProxy;
            if (!remoteDnsData.second.isEmpty()) dnsRemoteServer["server"] = remoteDnsData.second;
            dnsServers.append(dnsRemoteServer);
        }
        // Direct
        auto directDnsData = parseDnsAddress(dataStore->routing->direct_dns);
        QJsonObject dnsDirectServer{
            {"tag", "dns-direct"},
            {"type", directDnsData.first},
            {"domain_resolver", "dns-local"},
        };
        if (!directDnsData.second.isEmpty()) dnsDirectServer["server"] = directDnsData.second;
        if (dataStore->routing->dns_final_out == "direct") {
            dnsServers.prepend(dnsDirectServer);
        } else {
            dnsServers.append(dnsDirectServer);
        }

        // Underlying 100% Working DNS
        dnsServers += QJsonObject{
            {"tag", "dns-local"},
            {"type", "local"},
        };

        if (!status->forTest) {
            // Fakedns
            if (dataStore->routing->fake_dns) {
                dnsServers += QJsonObject{
                    {"tag", "dns-fake"},
                    {"type", "fakeip"},
                    {"inet4_range", "198.18.0.0/15"},
                    {"inet6_range", "fc00::/18"},
                };
                dnsRules += QJsonObject{
                    {"query_type", QJsonArray{"A", "AAAA"}},
                    {"server", "dns-fake"}};
                dns["independent_cache"] = true;
            }

            // Dns Rules
            if (dataStore->routing->dns_routing) {
                const QSet<QString> directFields = {"domain", "domain_suffix", "domain_keyword", "domain_regex"};
                auto add_rule_dns = [&](const QJsonObject &input, const QString &server) {
                    for (auto it = input.constBegin(); it != input.constEnd(); ++it) {
                        const QString &key = it.key();
                        const QJsonValue &value = it.value();
                        QJsonObject obj;
                        if (key == "geosite") {
                            QJsonArray inputArray = value.isArray() ? value.toArray() : QJsonArray{value};
                            QJsonArray tagArray;
                            for (const auto &v: inputArray) {
                                QString tag = key + "-" + v.toString();
                                tagArray.append(tag);
                            }
                            obj["rule_set"] = tagArray;
                        } else if (directFields.contains(key)) {
                            obj[key] = value;
                        } else {
                            continue;
                        }
                        if (server == "dns-block")
                            obj["action"] = "reject";
                        else
                            obj["server"] = server;
                        dnsRules += obj;
                    }
                };
                add_rule_dns(block_rules, "dns-block");
                add_rule_dns(proxy_rules, "dns-remote");
                add_rule_dns(direct_rules, "dns-direct");
                dns["reverse_mapping"] = true;
            }
        }

        dns["servers"] = dnsServers;
        dns["rules"] = dnsRules;

        if (dataStore->routing->enable_custom && !status->forTest) {
            dns = custom_routeObj["dns"].toObject();
        }

        // Routing
        QJsonObject route;
        QJsonArray routeRules;
        QJsonArray rule_set;
        // Rules
        if (!status->forTest) {
            if (dataStore->routing->domain_strategy != "") {
                routeRules += QJsonObject{
                    {"action", "resolve"}};
            }
            if (dataStore->routing->sniffing_mode != SniffingMode::DISABLE) {
                routeRules += QJsonObject{
                    {"action", "sniff"}};
            }
            routeRules += QJsonObject{
                {"action", "hijack-dns"},
                {"mode", "or"},
                {"type", "logical"},
                {"rules", QJsonArray{
                              QJsonObject{{"port", 53}},
                              QJsonObject{{"protocol", "dns"}}}},
            };

            // final add user rule
            QSet<QString> addedRuleSet;
            auto add_rule_route = [&](const QJsonObject &input, const QString &outbound) {
                for (auto it = input.constBegin(); it != input.constEnd(); ++it) {
                    const QString &key = it.key();
                    const QJsonValue &value = it.value();
                    QJsonObject obj;
                    if (key == "geosite" || key == "geoip") {
                        QJsonArray inputArray = value.isArray() ? value.toArray() : QJsonArray{value};
                        QJsonArray tagArray;
                        for (const auto &v: inputArray) {
                            QString tag = key + "-" + v.toString();
                            tagArray.append(tag);
                            if (!addedRuleSet.contains(tag)) {
                                addedRuleSet.insert(tag);
                                QString url = dataStore->routing->rule_sets_provider;
                                rule_set.append(QJsonObject{
                                    {"type", "remote"},
                                    {"tag", tag},
                                    {"url", url.replace("%type%", key).replace("%name%", v.toString())},
                                });
                            }
                        }
                        obj["rule_set"] = tagArray;
                    } else {
                        obj[key] = value;
                    }
                    if (outbound == "block")
                        obj["action"] = "reject";
                    else
                        obj["outbound"] = outbound;
                    routeRules += obj;
                }
            };
            add_rule_route(block_rules, "block");
            add_rule_route(proxy_rules, tagProxy);
            add_rule_route(direct_rules, "direct");

            // tun rule
            if (dataStore->spmode_vpn) {
                QStringList externalProcessPaths;
                for (const auto &extR: status->result->extRs) {
                    auto path = extR->program;
                    if (!path.trimmed().isEmpty()) {
                        externalProcessPaths << QDir::toNativeSeparators(path);
                    }
                }
                if (!externalProcessPaths.isEmpty()) {
                    QJsonObject rule{
                        {"outbound", "direct"},
                        {"process_path", QList2QJsonArray(externalProcessPaths)},
                    };
                    routeRules += rule;
                }
            }
        }

        if (!status->forTest && dataStore->routing->enable_custom) {
            for (const auto &outbound: custom_routeObj["outbounds"].toArray()) status->outbounds.append(outbound);
            route = custom_routeObj["route"].toObject();
            for (const auto &rule: route["rules"].toArray()) status->routingRules.append(rule);
            route["rules"] = status->routingRules;
        } else {
            for (const auto &rule: routeRules) status->routingRules.append(rule);
            route = {
                {"rules", status->routingRules},
                {"rule_set", rule_set},
                {"final", dataStore->routing->def_outbound},
            };
            if (dataStore->spmode_vpn) {
                route["auto_detect_interface"] = true;
            }
            route["default_domain_resolver"] = "dns-direct";
        }

        // experimental
        if (!status->forTest) {
            QJsonObject experimentalObj;
            experimentalObj["cache_file"] = QJsonObject{{"enabled", true}};

            QJsonObject clash_api;
            if (!dataStore->clash_api_external_controller.isEmpty()) {
                clash_api["external_controller"] = dataStore->clash_api_external_controller;
                if (dataStore->clash_api_dashboard == "yacd-meta") {
                    clash_api["external_ui"] = "dashboard/yacd-meta";
                    clash_api["external_ui_download_url"] = "https://github.com/MetaCubeX/Yacd-meta/archive/gh-pages.zip";
                } else if (dataStore->clash_api_dashboard == "metacubexd") {
                    clash_api["external_ui"] = "dashboard/metacubexd";
                    clash_api["external_ui_download_url"] = "https://github.com/MetaCubeX/metacubexd/archive/gh-pages.zip";
                } else if (dataStore->clash_api_dashboard == "zashboard") {
                    clash_api["external_ui"] = "dashboard/zashboard";
                    clash_api["external_ui_download_url"] = "https://github.com/Zephyruso/zashboard/archive/gh-pages.zip";
                }
                if (!dataStore->clash_api_secret.isEmpty()) clash_api["secret"] = dataStore->clash_api_secret;
            } else if (dataStore->traffic_loop_interval > 0) {
                clash_api["default_mode"] = "";
            }
            if (!clash_api.isEmpty()) experimentalObj["clash_api"] = clash_api;

            status->result->coreConfig["experimental"] = experimentalObj;
        }

        status->result->coreConfig.insert("dns", dns);
        status->result->coreConfig.insert("inbounds", status->inbounds);
        status->result->coreConfig.insert("outbounds", status->outbounds);
        status->result->coreConfig.insert("route", route);
    }

} // namespace NekoGui