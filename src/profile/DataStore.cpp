#include "profile/DataStore.hpp"

#include "protocol/Preset.hpp"

namespace NekoGui {

    DataStore *dataStore = new DataStore;

    // datastore

    DataStore::DataStore() : JsonStore() {}

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

    ExtraCore::ExtraCore() : JsonStore() {}

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

    InboundAuthorization::InboundAuthorization() : JsonStore() {}

    bool InboundAuthorization::NeedAuth() const {
        return !username.trimmed().isEmpty() && !password.trimmed().isEmpty();
    }

} // namespace NekoGui
