#pragma once

#include "ProxyEntity.hpp"

namespace NekoGui {
    class Group : public JsonStore {
    public:
        int id = -1;
        bool archive = false;
        bool skip_auto_update = false;
        QString name = "";
        QString url = "";
        QString info = "";
        qint64 sub_last_update = 0;
        int front_proxy_id = -1;

        // list ui
        QList<int> order;

        Group() {
            _add("id", &id);
            _add("front_proxy_id", &front_proxy_id);
            _add("archive", &archive);
            _add("skip_auto_update", &skip_auto_update);
            _add("name", &name);
            _add("order", &order);
            _add("url", &url);
            _add("info", &info);
            _add("lastup", &sub_last_update);
        }

        // 按 id 顺序
        [[nodiscard]] QList<std::shared_ptr<ProxyEntity>> Profiles() const;

        // 按 显示 顺序
        [[nodiscard]] QList<std::shared_ptr<ProxyEntity>> ProfilesWithOrder() const;
    };
} // namespace NekoGui
