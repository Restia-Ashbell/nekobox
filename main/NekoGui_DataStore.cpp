#include "NekoGui_DataStore.hpp"

#include "fmt/Preset.hpp"

namespace NekoGui {

    DataStore *dataStore = new DataStore;

    // datastore

    DataStore::DataStore() : JsonStore() {
        _add(new configItem("extraCore", dynamic_cast<JsonStore *>(extraCore), itemType::jsonStore));
        _add(new configItem("inbound_auth", dynamic_cast<JsonStore *>(inbound_auth), itemType::jsonStore));
        _add(new configItem("user_agent", &user_agent, itemType::string));
        _add(new configItem("test_url", &test_latency_url, itemType::string));
        _add(new configItem("test_url_dl", &test_download_url, itemType::string));
        _add(new configItem("test_dl_timeout", &test_download_timeout, itemType::integer));
        _add(new configItem("current_group", &current_group, itemType::integer));
        _add(new configItem("inbound_address", &inbound_address, itemType::string));
        _add(new configItem("inbound_port", &inbound_port, itemType::integer));
        _add(new configItem("traffic_loop_interval", &traffic_loop_interval, itemType::integer));
        _add(new configItem("test_concurrent", &test_concurrent, itemType::integer));
        _add(new configItem("theme", &theme, itemType::string));
        _add(new configItem("custom_inbound", &custom_inbound, itemType::string));
        _add(new configItem("sub_use_proxy", &sub_use_proxy, itemType::boolean));
        _add(new configItem("started_id", &started_id, itemType::integer));
        _add(new configItem("spmode_vpn", &spmode_vpn, itemType::boolean));
        _add(new configItem("spmode_system_proxy", &spmode_system_proxy, itemType::boolean));
        _add(new configItem("language", &language, itemType::string));
        _add(new configItem("font", &font, itemType::string));
        _add(new configItem("skip_cert", &skip_cert, itemType::boolean));
        _add(new configItem("hk_mw", &hotkey_mainwindow, itemType::string));
        _add(new configItem("hk_group", &hotkey_group, itemType::string));
        _add(new configItem("hk_route", &hotkey_route, itemType::string));
        _add(new configItem("hk_spmenu", &hotkey_system_proxy_menu, itemType::string));
        _add(new configItem("active_routing", &active_routing, itemType::string));
        _add(new configItem("mw_geometry", &mw_geometry, itemType::string));
        _add(new configItem("tun_stack", &tun_stack, itemType::string));
        _add(new configItem("tun_mtu", &tun_mtu, itemType::integer));
        _add(new configItem("tun_ipv6", &tun_ipv6, itemType::boolean));
        _add(new configItem("tun_strict_route", &tun_strict_route, itemType::boolean));
        _add(new configItem("check_include_pre", &check_include_pre, itemType::boolean));
        _add(new configItem("sp_format", &system_proxy_format, itemType::string));
        _add(new configItem("sub_insecure", &sub_insecure, itemType::boolean));
        _add(new configItem("sub_auto_update", &sub_auto_update, itemType::integer));
        _add(new configItem("log_ignore", &log_ignore, itemType::stringList));
        _add(new configItem("start_minimal", &start_minimal, itemType::boolean));
        _add(new configItem("max_log_line", &max_log_line, itemType::integer));
        _add(new configItem("splitter_state", &splitter_state, itemType::string));
        _add(new configItem("utlsFingerprint", &utlsFingerprint, itemType::string));
        _add(new configItem("log_disabled", &log_disabled, itemType::boolean));
        _add(new configItem("log_timestamp", &log_timestamp, itemType::boolean));
        _add(new configItem("log_level", &log_level, itemType::string));
        _add(new configItem("clash_api_external_controller", &clash_api_external_controller, itemType::string));
        _add(new configItem("clash_api_dashboard", &clash_api_dashboard, itemType::string));
        _add(new configItem("clash_api_secret", &clash_api_secret, itemType::string));
        _add(new configItem("ntp_enabled", &ntp_enabled, itemType::boolean));
        _add(new configItem("ntp_server", &ntp_server, itemType::string));
        _add(new configItem("ntp_server_port", &ntp_server_port, itemType::integer));
        _add(new configItem("ntp_interval", &ntp_interval, itemType::string));
        _add(new configItem("certificate_store", &certificate_store, itemType::string));
        _add(new configItem("certificate", &certificate, itemType::string));
        _add(new configItem("certificate_path", &certificate_path, itemType::string));
        _add(new configItem("certificate_directory_path", &certificate_directory_path, itemType::string));
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

        _add(new configItem("block_rules", &block_rules, itemType::string));
        _add(new configItem("proxy_rules", &proxy_rules, itemType::string));
        _add(new configItem("direct_rules", &direct_rules, itemType::string));
        _add(new configItem("def_outbound", &def_outbound, itemType::string));
        _add(new configItem("rule_sets_provider", &rule_sets_provider, itemType::string));
        _add(new configItem("custom", &custom, itemType::string));
        //
        _add(new configItem("remote_dns", &remote_dns, itemType::string));
        _add(new configItem("remote_dns_strategy", &remote_dns_strategy, itemType::string));
        _add(new configItem("direct_dns", &direct_dns, itemType::string));
        _add(new configItem("direct_dns_strategy", &direct_dns_strategy, itemType::string));
        _add(new configItem("dns_final_out", &dns_final_out, itemType::string));
        _add(new configItem("dns_routing", &dns_routing, itemType::boolean));
        _add(new configItem("fake_dns", &fake_dns, itemType::boolean));
        _add(new configItem("sniffing_mode", &sniffing_mode, itemType::integer));
        _add(new configItem("domain_strategy", &domain_strategy, itemType::string));
        _add(new configItem("outbound_domain_strategy", &outbound_domain_strategy, itemType::string));
        _add(new configItem("enable_custom", &enable_custom, itemType::boolean));
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
        _add(new configItem("core_map", &this->core_map, itemType::string));
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
        _add(new configItem("user", &this->username, itemType::string));
        _add(new configItem("pass", &this->password, itemType::string));
    }

    bool InboundAuthorization::NeedAuth() const {
        return !username.trimmed().isEmpty() && !password.trimmed().isEmpty();
    }

} // namespace NekoGui
