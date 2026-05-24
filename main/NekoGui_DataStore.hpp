#pragma once

#include "Const.hpp"
#include "NekoGui_ConfigItem.hpp"

namespace NekoGui {

    class Routing : public JsonStore {
    public:
        QString block_rules;
        QString proxy_rules;
        QString direct_rules;
        QString def_outbound = "proxy";
        QString rule_sets_provider = "https://raw.githubusercontent.com/SagerNet/sing-%type%/rule-set/%type%-%name%.srs";
        QString custom = R"({"dns":{},"outbounds":[],"route":{}})";

        // DNS
        QString remote_dns = "https://dns.google/dns-query";
        QString remote_dns_strategy = "";
        QString direct_dns = "local";
        QString direct_dns_strategy = "";
        bool dns_routing = true;
        bool fake_dns = false;
        bool enable_custom = false;
        QString dns_final_out = "proxy";

        // Misc
        QString domain_strategy = "";
        QString outbound_domain_strategy = "";
        int sniffing_mode = SniffingMode::FOR_ROUTING;

        explicit Routing(int preset = 0);

        [[nodiscard]] QString DisplayRouting() const;

        static QStringList List();

        static bool SetToActive(const QString &name);
    };

    class ExtraCore : public JsonStore {
    public:
        QString core_map;

        explicit ExtraCore();

        [[nodiscard]] QString Get(const QString &id) const;

        void Set(const QString &id, const QString &path);

        void Delete(const QString &id);
    };

    class InboundAuthorization : public JsonStore {
    public:
        QString username;
        QString password;

        InboundAuthorization();

        [[nodiscard]] bool NeedAuth() const;
    };

    class DataStore : public JsonStore {
    public:
        // Running

        int started_id = -1919;
        bool prepare_exit = false;
        bool spmode_vpn = false;
        bool spmode_system_proxy = false;
        bool need_keep_vpn_off = false;
        QString appdataDir = "";
        QStringList ignoreConnTag = {};

        std::unique_ptr<Routing> routing;
        int resolve_count = 0;

        // Flags
        QStringList argv = {};
        bool flag_use_appdata = false;
        bool flag_tray = false;
        bool flag_debug = false;
        bool flag_restart_tun_on = false;
        bool flag_reorder = false;

        // Saved

        // Misc
        QString test_latency_url = "https://www.gstatic.com/generate_204";
        QString test_download_url = "http://speed.cloudflare.com/__down?bytes=10485760";
        int test_download_timeout = 30;
        int test_concurrent = 5;
        bool old_share_link_format = true;
        int traffic_loop_interval = 1000;
        int current_group = 0; // group id
        QString theme = "";
        QString language = "";
        QString font = "";
        bool check_include_pre = false;
        QString system_proxy_format = "";
        QStringList log_ignore = {};
        bool start_minimal = false;
        int max_log_line = 200;
        QString splitter_state = "";
        QString mw_geometry = "";

        // Subscription
        QString user_agent = "";
        bool sub_use_proxy = false;
        bool sub_insecure = false;
        int sub_auto_update = -30;

        // Security
        bool skip_cert = false;
        QString utlsFingerprint = "";

        // Mixed Inbound
        QString inbound_address = "127.0.0.1";
        int inbound_port = 2080;
        InboundAuthorization *inbound_auth = new InboundAuthorization;
        QString custom_inbound = R"({"inbounds":[]})";

        // Tun Inbound
        QString tun_stack;
        int tun_mtu = 9000;
        bool tun_ipv6 = false;
        bool tun_strict_route = false;

        // Routing
        QString active_routing = "Default";

        // Log
        bool log_disabled = false;
        bool log_timestamp = false;
        QString log_level = "info";

        // Clash API
        QString clash_api_external_controller;
        QString clash_api_dashboard = "zashboard";
        QString clash_api_secret;

        // NTP
        bool ntp_enabled = false;
        QString ntp_server;
        int ntp_server_port = 0;
        QString ntp_interval = "30m";

        // Certificate
        QString certificate_store = "system";
        QString certificate;
        QString certificate_path;
        QString certificate_directory_path;

        // Hotkey
        QString hotkey_mainwindow = "";
        QString hotkey_group = "";
        QString hotkey_route = "";
        QString hotkey_system_proxy_menu = "";

        // Other Core
        ExtraCore *extraCore = new ExtraCore;

        // Methods

        DataStore();

        QString GetUserAgent(bool isDefault = false) const;
    };

    extern DataStore *dataStore;

} // namespace NekoGui
