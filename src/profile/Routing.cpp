#include "profile/Routing.hpp"

#include <QDir>

#include "common/Utils.hpp"
#include "profile/DataStore.hpp"
#include "protocol/Preset.hpp"

namespace NekoGui {

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

} // namespace NekoGui
