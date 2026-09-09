#pragma once

#include "common/Const.hpp"
#include "profile/ConfigItem.hpp"

namespace NekoGui {

    class Routing : public JsonStore {
        Q_OBJECT
        Q_PROPERTY(QString block_rules MEMBER block_rules)
        Q_PROPERTY(QString proxy_rules MEMBER proxy_rules)
        Q_PROPERTY(QString direct_rules MEMBER direct_rules)
        Q_PROPERTY(QString def_outbound MEMBER def_outbound)
        Q_PROPERTY(QString rule_sets_provider MEMBER rule_sets_provider)
        Q_PROPERTY(QString custom MEMBER custom)
        Q_PROPERTY(QString remote_dns MEMBER remote_dns)
        Q_PROPERTY(QString remote_dns_strategy MEMBER remote_dns_strategy)
        Q_PROPERTY(QString direct_dns MEMBER direct_dns)
        Q_PROPERTY(QString direct_dns_strategy MEMBER direct_dns_strategy)
        Q_PROPERTY(QString dns_final_out MEMBER dns_final_out)
        Q_PROPERTY(bool dns_routing MEMBER dns_routing)
        Q_PROPERTY(bool fake_dns MEMBER fake_dns)
        Q_PROPERTY(bool enable_custom MEMBER enable_custom)
        Q_PROPERTY(QString domain_strategy MEMBER domain_strategy)
        Q_PROPERTY(QString outbound_domain_strategy MEMBER outbound_domain_strategy)
        Q_PROPERTY(int sniffing_mode MEMBER sniffing_mode)

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
        Q_OBJECT
        Q_PROPERTY(QString core_map MEMBER core_map)

    public:
        QString core_map;

        explicit ExtraCore();

        [[nodiscard]] QString Get(const QString &id) const;

        void Set(const QString &id, const QString &path);

        void Delete(const QString &id);
    };

    class InboundAuthorization : public JsonStore {
        Q_OBJECT
        Q_PROPERTY(QString user MEMBER username)
        Q_PROPERTY(QString pass MEMBER password)

    public:
        QString username;
        QString password;

        InboundAuthorization();

        [[nodiscard]] bool NeedAuth() const;
    };

    class DataStore : public JsonStore {
        Q_OBJECT
        Q_PROPERTY(ExtraCore *extraCore MEMBER extraCore)
        Q_PROPERTY(InboundAuthorization *inbound_auth MEMBER inbound_auth)
        Q_PROPERTY(QString user_agent MEMBER user_agent)
        Q_PROPERTY(QString test_url MEMBER test_latency_url)
        Q_PROPERTY(QString test_url_dl MEMBER test_download_url)
        Q_PROPERTY(int test_dl_timeout MEMBER test_download_timeout)
        Q_PROPERTY(int current_group MEMBER current_group)
        Q_PROPERTY(QString inbound_address MEMBER inbound_address)
        Q_PROPERTY(int inbound_port MEMBER inbound_port)
        Q_PROPERTY(int traffic_loop_interval MEMBER traffic_loop_interval)
        Q_PROPERTY(int test_concurrent MEMBER test_concurrent)
        Q_PROPERTY(QString theme MEMBER theme)
        Q_PROPERTY(QString custom_inbound MEMBER custom_inbound)
        Q_PROPERTY(bool sub_use_proxy MEMBER sub_use_proxy)
        Q_PROPERTY(int started_id MEMBER started_id)
        Q_PROPERTY(bool spmode_vpn MEMBER spmode_vpn)
        Q_PROPERTY(bool spmode_system_proxy MEMBER spmode_system_proxy)
        Q_PROPERTY(QString language MEMBER language)
        Q_PROPERTY(QString font MEMBER font)
        Q_PROPERTY(QString icon_path MEMBER icon_path)
        Q_PROPERTY(bool skip_cert MEMBER skip_cert)
        Q_PROPERTY(QString hk_mw MEMBER hotkey_mainwindow)
        Q_PROPERTY(QString hk_group MEMBER hotkey_group)
        Q_PROPERTY(QString hk_route MEMBER hotkey_route)
        Q_PROPERTY(QString hk_spmenu MEMBER hotkey_system_proxy_menu)
        Q_PROPERTY(QString active_routing MEMBER active_routing)
        Q_PROPERTY(QString mw_geometry MEMBER mw_geometry)
        Q_PROPERTY(QString tun_stack MEMBER tun_stack)
        Q_PROPERTY(int tun_mtu MEMBER tun_mtu)
        Q_PROPERTY(bool tun_ipv6 MEMBER tun_ipv6)
        Q_PROPERTY(bool tun_strict_route MEMBER tun_strict_route)
        Q_PROPERTY(bool check_include_pre MEMBER check_include_pre)
        Q_PROPERTY(QString sp_format MEMBER system_proxy_format)
        Q_PROPERTY(bool sub_insecure MEMBER sub_insecure)
        Q_PROPERTY(int sub_auto_update MEMBER sub_auto_update)
        Q_PROPERTY(QStringList log_ignore MEMBER log_ignore)
        Q_PROPERTY(bool start_minimal MEMBER start_minimal)
        Q_PROPERTY(int max_log_line MEMBER max_log_line)
        Q_PROPERTY(QString splitter_state MEMBER splitter_state)
        Q_PROPERTY(QString utlsFingerprint MEMBER utlsFingerprint)
        Q_PROPERTY(bool log_disabled MEMBER log_disabled)
        Q_PROPERTY(bool log_timestamp MEMBER log_timestamp)
        Q_PROPERTY(QString log_level MEMBER log_level)
        Q_PROPERTY(QString clash_api_external_controller MEMBER clash_api_external_controller)
        Q_PROPERTY(QString clash_api_dashboard MEMBER clash_api_dashboard)
        Q_PROPERTY(QString clash_api_secret MEMBER clash_api_secret)
        Q_PROPERTY(bool ntp_enabled MEMBER ntp_enabled)
        Q_PROPERTY(QString ntp_server MEMBER ntp_server)
        Q_PROPERTY(int ntp_server_port MEMBER ntp_server_port)
        Q_PROPERTY(QString ntp_interval MEMBER ntp_interval)
        Q_PROPERTY(QString certificate_store MEMBER certificate_store)
        Q_PROPERTY(QString certificate MEMBER certificate)
        Q_PROPERTY(QString certificate_path MEMBER certificate_path)
        Q_PROPERTY(QString certificate_directory_path MEMBER certificate_directory_path)

    public:
        // Running

        int started_id = -1919;
        bool prepare_exit = false;
        bool spmode_vpn = false;
        bool spmode_system_proxy = false;

        std::unique_ptr<Routing> routing;

        // Flags
        QStringList argv = {};
        bool flag_use_appdata = false;
        bool flag_tray = false;
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
        QString icon_path = "";
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
