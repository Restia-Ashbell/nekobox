#pragma once

#include "common/Const.hpp"
#include "profile/ConfigItem.hpp"
#include "profile/Routing.hpp"

namespace NekoGui {

    class DataStore : public JsonStore {
        Q_OBJECT
        Q_PROPERTY(QVariantMap extra_core MEMBER extra_core)
        Q_PROPERTY(QString inbound_username MEMBER inbound_username)
        Q_PROPERTY(QString inbound_password MEMBER inbound_password)
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
        bool start_minimal = false;
        int max_log_line = 1000;
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
        QString inbound_username;
        QString inbound_password;
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
        QVariantMap extra_core;
    };

    inline DataStore *dataStore = new DataStore;

} // namespace NekoGui
