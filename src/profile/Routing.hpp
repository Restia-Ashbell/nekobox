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
} // namespace NekoGui
