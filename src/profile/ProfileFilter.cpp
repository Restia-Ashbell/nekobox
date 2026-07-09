#include "ProfileFilter.hpp"

namespace NekoGui {

    QString ProfileFilter::Key(const std::shared_ptr<ProxyEntity> &ent, bool by_address) {
        return by_address && ent->type != "custom"
                   ? ent->bean->DisplayAddress() + ent->bean->DisplayType()
                   : QJsonObject2QString(ent->bean->ToJson({"c_cfg", "c_out"}), true) + ent->bean->DisplayType();
    }

    QList<std::shared_ptr<ProxyEntity>> ProfileFilter::Dup(const QList<std::shared_ptr<ProxyEntity>> &list) {
        QList<std::shared_ptr<ProxyEntity>> result;
        QSet<QString> seen;

        for (const auto &ent: list) {
            QString key = Key(ent, true);
            if (seen.contains(key)) {
                result.append(ent);
            } else {
                seen.insert(key);
            }
        }

        return result;
    }

    std::tuple<QList<std::shared_ptr<ProxyEntity>>, QList<std::shared_ptr<ProxyEntity>>,
               QList<std::shared_ptr<ProxyEntity>>, QList<std::shared_ptr<ProxyEntity>>>
    ProfileFilter::Diff(const QList<std::shared_ptr<ProxyEntity>> &list1, const QList<std::shared_ptr<ProxyEntity>> &list2) {
        QList<std::shared_ptr<ProxyEntity>> common1, common2, only1, only2;
        QHash<QString, std::shared_ptr<ProxyEntity>> map1, map2;

        for (const auto &ent: list1)
            map1.insert(Key(ent, false), ent);

        for (const auto &ent: list2) {
            QString key = Key(ent, false);
            map2.insert(key, ent);
            if (map1.contains(key)) {
                common1.append(map1[key]);
                common2.append(ent);
            } else {
                only2.append(ent);
            }
        }

        for (const auto &ent: list1) {
            QString key = Key(ent, false);
            if (!map2.contains(key))
                only1.append(ent);
        }

        return {common1, common2, only1, only2};
    }

} // namespace NekoGui