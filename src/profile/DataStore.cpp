#include "profile/DataStore.hpp"

#include "protocol/Preset.hpp"

namespace NekoGui {

    DataStore *dataStore = new DataStore;

    // datastore

    DataStore::DataStore() : JsonStore() {
        _add("extraCore", dynamic_cast<JsonStore *>(extraCore));
        _add("inbound_auth", dynamic_cast<JsonStore *>(inbound_auth));
        _add("user_agent", &user_agent);
        _add("test_url", &test_latency_url);
        _add("test_url_dl", &test_download_url);
        _add("test_dl_timeout", &test_download_timeout);
        _add("current_group", &current_group);
        _add("inbound_address", &inbound_address);
        _add("inbound_port", &inbound_port);
        _add("traffic_loop_interval", &traffic_loop_interval);
        _add("test_concurrent", &test_concurrent);
        _add("theme", &theme);
        _add("custom_inbound", &custom_inbound);
        _add("sub_use_proxy", &sub_use_proxy);
        _add("started_id", &started_id);
        _add("spmode_vpn", &spmode_vpn);
        _add("spmode_system_proxy", &spmode_system_proxy);
        _add("language", &language);
        _add("font", &font);
        _add("icon_path", &icon_path);
        _add("skip_cert", &skip_cert);
        _add("hk_mw", &hotkey_mainwindow);
        _add("hk_group", &hotkey_group);
        _add("hk_route", &hotkey_route);
        _add("hk_spmenu", &hotkey_system_proxy_menu);
        _add("active_routing", &active_routing);
        _add("mw_geometry", &mw_geometry);
        _add("tun_stack", &tun_stack);
        _add("tun_mtu", &tun_mtu);
        _add("tun_ipv6", &tun_ipv6);
        _add("tun_strict_route", &tun_strict_route);
        _add("check_include_pre", &check_include_pre);
        _add("sp_format", &system_proxy_format);
        _add("sub_insecure", &sub_insecure);
        _add("sub_auto_update", &sub_auto_update);
        _add("log_ignore", &log_ignore);
        _add("start_minimal", &start_minimal);
        _add("max_log_line", &max_log_line);
        _add("splitter_state", &splitter_state);
        _add("utlsFingerprint", &utlsFingerprint);
        _add("log_disabled", &log_disabled);
        _add("log_timestamp", &log_timestamp);
        _add("log_level", &log_level);
        _add("clash_api_external_controller", &clash_api_external_controller);
        _add("clash_api_dashboard", &clash_api_dashboard);
        _add("clash_api_secret", &clash_api_secret);
        _add("ntp_enabled", &ntp_enabled);
        _add("ntp_server", &ntp_server);
        _add("ntp_server_port", &ntp_server_port);
        _add("ntp_interval", &ntp_interval);
        _add("certificate_store", &certificate_store);
        _add("certificate", &certificate);
        _add("certificate_path", &certificate_path);
        _add("certificate_directory_path", &certificate_directory_path);
    }

    QString DataStore::GetUserAgent(bool isDefault) const {
        if (user_agent.isEmpty() || isDefault) {
            QString version = SubStrBefore(NKR_VERSION, "-");
            if (!version.contains(".")) version = "Unknown";
            return "NekoBox/PC/" + version + " (Prefer ClashMeta Format)";
        }
        return user_agent;
    }

    // preset routing
    Routing::Routing(int preset) : JsonStore() {
        if (preset == 1) {
            block_rules =
                "geosite:category-ads-all\n"
                "domain:appcenter.ms\n"
                "domain:firebase.io\n"
                "domain:crashlytics.com\n";
            proxy_rules = "";
            direct_rules =
                "geosite:cn\n"
                "ip_is_private:true\n"
                "geoip:cn\n";
        }

        if (!Preset::SingBox::DomainStrategy.contains(domain_strategy)) domain_strategy = "";
        if (!Preset::SingBox::DomainStrategy.contains(outbound_domain_strategy)) outbound_domain_strategy = "";

        _add("block_rules", &block_rules);
        _add("proxy_rules", &proxy_rules);
        _add("direct_rules", &direct_rules);
        _add("def_outbound", &def_outbound);
        _add("rule_sets_provider", &rule_sets_provider);
        _add("custom", &custom);
        //
        _add("remote_dns", &remote_dns);
        _add("remote_dns_strategy", &remote_dns_strategy);
        _add("direct_dns", &direct_dns);
        _add("direct_dns_strategy", &direct_dns_strategy);
        _add("dns_final_out", &dns_final_out);
        _add("dns_routing", &dns_routing);
        _add("fake_dns", &fake_dns);
        _add("sniffing_mode", &sniffing_mode);
        _add("domain_strategy", &domain_strategy);
        _add("outbound_domain_strategy", &outbound_domain_strategy);
        _add("enable_custom", &enable_custom);
    }

    QString Routing::DisplayRouting() const {
        return QString("[Proxy] %1\n[Direct] %2\n[Block] %3\n[Default Outbound] %4\n[DNS] %5")
            .arg(SplitLinesSkipSharp(block_rules).join(","), 10)
            .arg(SplitLinesSkipSharp(proxy_rules).join(","), 10)
            .arg(SplitLinesSkipSharp(direct_rules).join(","), 10)
            .arg(def_outbound)
            .arg(enable_custom ? "DNS Object" : "Simple DNS");
    }

    QStringList Routing::List() {
        QDir dr("routes");
        return dr.entryList(QDir::Files);
    }

    bool Routing::SetToActive(const QString &name) {
        NekoGui::dataStore->routing = std::make_unique<Routing>();
        NekoGui::dataStore->routing->load_control_must = true;
        NekoGui::dataStore->routing->fn = "routes/" + name;
        auto ok = NekoGui::dataStore->routing->Load();
        if (ok) {
            NekoGui::dataStore->active_routing = name;
            NekoGui::dataStore->Save();
        }
        return ok;
    }

    // NO default extra core

    ExtraCore::ExtraCore() : JsonStore() {
        _add("core_map", &this->core_map);
    }

    QString ExtraCore::Get(const QString &id) const {
        auto obj = QString2QJsonObject(core_map);
        for (const auto &c: obj.keys()) {
            if (c == id) return obj[id].toString();
        }
        return "";
    }

    void ExtraCore::Set(const QString &id, const QString &path) {
        auto obj = QString2QJsonObject(core_map);
        obj[id] = path;
        core_map = QJsonObject2QString(obj, true);
    }

    void ExtraCore::Delete(const QString &id) {
        auto obj = QString2QJsonObject(core_map);
        obj.remove(id);
        core_map = QJsonObject2QString(obj, true);
    }

    InboundAuthorization::InboundAuthorization() : JsonStore() {
        _add("user", &this->username);
        _add("pass", &this->password);
    }

    bool InboundAuthorization::NeedAuth() const {
        return !username.trimmed().isEmpty() && !password.trimmed().isEmpty();
    }

} // namespace NekoGui
