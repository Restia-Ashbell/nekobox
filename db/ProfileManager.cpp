#include "ProfileManager.hpp"

#include <QDir>
#include <QFile>

#include "fmt/includes.h"

namespace NekoGui {

    ProfileManager *profileManager = new ProfileManager;

    ProfileManager::ProfileManager() : JsonStore("groups/pm.json") {
        _add("groups", &groupsTabOrder);
    }

    QList<int> filterIntJsonFile(const QString &path) {
        QList<int> result;
        QDir dir(path);
        for (auto e: dir.entryList(QDir::Files)) {
            if (!e.endsWith(".json", Qt::CaseInsensitive)) continue;
            e = e.remove(".json", Qt::CaseInsensitive);
            bool ok = false;
            auto id = e.toInt(&ok);
            if (ok) result << id;
        }
        return result;
    }

    void ProfileManager::LoadManager() {
        JsonStore::Load();
        // Load Groups
        groups.clear();
        for (auto id: filterIntJsonFile("groups")) {
            auto ent = LoadGroup(QString("groups/%1.json").arg(id));
            // Corrupted group?
            if (ent == nullptr) {
                DeleteGroup(id);
                continue;
            }
            groups[id] = ent;
            if (!groupsTabOrder.contains(id)) {
                groupsTabOrder << id;
            }
        }
        groupsTabOrder.removeIf([this](int k) { return !groups.contains(k); });
        // Load Proxys
        profiles.clear();
        for (auto id: filterIntJsonFile("profiles")) {
            auto ent = LoadProxyEntity(QString("profiles/%1.json").arg(id));
            // Corrupted profile?
            if (ent == nullptr || ent->bean == nullptr || !groups.contains(ent->gid)) {
                DeleteProfile(id);
                continue;
            }
            profiles[id] = ent;
        }
        // First setup
        if (groups.empty()) {
            auto defaultGroup = NekoGui::ProfileManager::NewGroup();
            defaultGroup->name = QObject::tr("Default");
            profileManager->AddGroup(defaultGroup);
        }
        //
        if (dataStore->flag_reorder) {
            {
                // remove all (contains orphan)
                for (const auto &profile: profiles) {
                    QFile::remove(profile.second->fn);
                }
            }
            std::map<int, int> gidOld2New;
            {
                int i = 0;
                int ii = 0;
                std::map<int, std::shared_ptr<ProxyEntity>> newProfiles;
                for (auto gid: groupsTabOrder) {
                    auto group = GetGroup(gid);
                    gidOld2New[gid] = ii++;
                    for (auto const &profile: group->ProfilesWithOrder()) {
                        auto oldId = profile->id;
                        auto newId = i++;
                        profile->id = newId;
                        profile->gid = gidOld2New[gid];
                        profile->fn = QString("profiles/%1.json").arg(newId);
                        profile->Save();
                        newProfiles[newId] = profile;
                    }
                    group->order = {};
                    group->Save();
                }
                profiles = newProfiles;
            }
            {
                QList<int> newGroupsIdOrder;
                std::map<int, std::shared_ptr<Group>> newGroups;
                for (auto oldGid: groupsTabOrder) {
                    auto newId = gidOld2New[oldGid];
                    auto group = groups[oldGid];
                    QFile::remove(group->fn);
                    group->id = newId;
                    group->fn = QString("groups/%1.json").arg(newId);
                    group->Save();
                    newGroups[newId] = group;
                    newGroupsIdOrder << newId;
                }
                groups = newGroups;
                groupsTabOrder = newGroupsIdOrder;
            }
            MessageBoxInfo(software_name, "Profiles and groups reorder complete.");
        }
    }

    void ProfileManager::SaveManager() {
        JsonStore::Save();
    }

    std::shared_ptr<ProxyEntity> ProfileManager::LoadProxyEntity(const QString &jsonPath) {
        // Load type
        QFile file(jsonPath);
        if (!file.open(QIODevice::ReadOnly)) return nullptr;
        QByteArray data = file.readAll();
        QJsonParseError err;
        QJsonObject obj = QJsonDocument::fromJson(data, &err).object();
        if (err.error != QJsonParseError::NoError) return nullptr;
        QString type = obj.value("type").toString();

        // Load content
        auto ent = NewProxyEntity(type);
        if (ent->bean) {
            ent->load_control_must = true;
            ent->fn = jsonPath;
            ent->last_save_content = data;
            ent->FromJson(obj);
        }

        return ent;
    }

    //  新建的不给 fn 和 id

    std::shared_ptr<ProxyEntity> ProfileManager::NewProxyEntity(const QString &type) {
        NekoGui_fmt::AbstractBean *bean;

        if (type == "socks5" || type == "socks4" || type == "socks4a" || type == "socks") {
            bean = new NekoGui_fmt::SocksHttpBean(NekoGui_fmt::SocksHttpBean::type_Socks5);
        } else if (type == "http" || type == "https") {
            bean = new NekoGui_fmt::SocksHttpBean(NekoGui_fmt::SocksHttpBean::type_HTTP);
        } else if (type == "shadowsocks" || type == "ss") {
            bean = new NekoGui_fmt::ShadowSocksBean();
        } else if (type == "shadowsocksr" || type == "ssr") {
            bean = new NekoGui_fmt::ShadowSocksRBean();
        } else if (type == "chain") {
            bean = new NekoGui_fmt::ChainBean();
        } else if (type == "vmess") {
            bean = new NekoGui_fmt::VMessBean();
        } else if (type == "trojan") {
            bean = new NekoGui_fmt::TrojanVLESSBean(NekoGui_fmt::TrojanVLESSBean::proxy_Trojan);
        } else if (type == "vless") {
            bean = new NekoGui_fmt::TrojanVLESSBean(NekoGui_fmt::TrojanVLESSBean::proxy_VLESS);
        } else if (type == "naive" || type == "naive+https" || type == "naive+quic") {
            bean = new NekoGui_fmt::NaiveBean();
        } else if (type == "hysteria" || type == "hy") {
            bean = new NekoGui_fmt::QUICBean(NekoGui_fmt::QUICBean::proxy_Hysteria);
        } else if (type == "hysteria2" || type == "hy2") {
            bean = new NekoGui_fmt::QUICBean(NekoGui_fmt::QUICBean::proxy_Hysteria2);
        } else if (type == "tuic") {
            bean = new NekoGui_fmt::QUICBean(NekoGui_fmt::QUICBean::proxy_TUIC);
        } else if (type == "anytls") {
            bean = new NekoGui_fmt::AnyTLSBean();
        } else if (type == "ssh") {
            bean = new NekoGui_fmt::SSHBean();
        } else if (type == "wireguard" || type == "wg") {
            bean = new NekoGui_fmt::WireGuardBean();
        } else if (type == "custom") {
            bean = new NekoGui_fmt::CustomBean();
        } else {
            bean = nullptr;
        }

        return std::make_shared<ProxyEntity>(bean, type);
    }

    std::shared_ptr<Group> ProfileManager::NewGroup() {
        return std::make_shared<Group>();
    }

    // Profile

    int ProfileManager::NewProfileID() const {
        return profiles.empty() ? 0 : profiles.rbegin()->first + 1;
    }

    bool ProfileManager::AddProfile(const std::shared_ptr<ProxyEntity> &ent, int gid) {
        if (ent->id >= 0) {
            return false;
        }

        ent->gid = gid < 0 ? dataStore->current_group : gid;
        ent->id = NewProfileID();
        profiles[ent->id] = ent;

        ent->fn = QString("profiles/%1.json").arg(ent->id);
        ent->Save();
        return true;
    }

    void ProfileManager::DeleteProfile(int id) {
        if (id < 0) return;
        if (dataStore->started_id == id) return;
        profiles.erase(id);
        QFile(QString("profiles/%1.json").arg(id)).remove();
    }

    void ProfileManager::MoveProfile(const std::shared_ptr<ProxyEntity> &ent, int gid) {
        if (gid == ent->gid || gid < 0) return;
        auto oldGroup = GetGroup(ent->gid);
        if (oldGroup != nullptr && !oldGroup->order.isEmpty()) {
            oldGroup->order.removeAll(ent->id);
            oldGroup->Save();
        }
        auto newGroup = GetGroup(gid);
        if (newGroup != nullptr && !newGroup->order.isEmpty()) {
            newGroup->order.push_back(ent->id);
            newGroup->Save();
        }
        ent->gid = gid;
        ent->Save();
    }

    std::shared_ptr<ProxyEntity> ProfileManager::GetProfile(int id) {
        auto it = profiles.find(id);
        return it != profiles.end() ? it->second : nullptr;
    }

    // Group

    std::shared_ptr<Group> ProfileManager::LoadGroup(const QString &jsonPath) {
        auto ent = std::make_shared<Group>();
        ent->fn = jsonPath;
        return ent->Load() ? ent : nullptr;
    }

    int ProfileManager::NewGroupID() const {
        return groups.empty() ? 0 : groups.rbegin()->first + 1;
    }

    bool ProfileManager::AddGroup(const std::shared_ptr<Group> &ent) {
        if (ent->id >= 0) {
            return false;
        }

        ent->id = NewGroupID();
        groups[ent->id] = ent;
        groupsTabOrder.push_back(ent->id);

        ent->fn = QString("groups/%1.json").arg(ent->id);
        ent->Save();
        return true;
    }

    void ProfileManager::DeleteGroup(int gid) {
        if (groups.size() <= 1) return;
        QList<int> toDelete;
        for (const auto &[id, profile]: profiles) {
            if (profile->gid == gid) toDelete += id; // map访问中，不能操作
        }
        for (const auto &id: toDelete) {
            DeleteProfile(id);
        }
        groups.erase(gid);
        groupsTabOrder.removeAll(gid);
        QFile(QString("groups/%1.json").arg(gid)).remove();
    }

    std::shared_ptr<Group> ProfileManager::GetGroup(int id) {
        auto it = groups.find(id);
        return it != groups.end() ? it->second : nullptr;
    }

    std::shared_ptr<Group> ProfileManager::CurrentGroup() {
        return GetGroup(dataStore->current_group);
    }

    QList<std::shared_ptr<ProxyEntity>> Group::Profiles() const {
        QList<std::shared_ptr<ProxyEntity>> ret;
        for (const auto &[_, profile]: profileManager->profiles) {
            if (id == profile->gid) ret += profile;
        }
        return ret;
    }

    QList<std::shared_ptr<ProxyEntity>> Group::ProfilesWithOrder() const {
        if (order.isEmpty()) {
            return Profiles();
        } else {
            QList<std::shared_ptr<ProxyEntity>> ret;
            for (auto _id: order) {
                if (auto ent = profileManager->GetProfile(_id)) ret += ent;
            }
            return ret;
        }
    }

} // namespace NekoGui