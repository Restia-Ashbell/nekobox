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
            _add(new configItem("id", &id, itemType::integer));
            _add(new configItem("front_proxy_id", &front_proxy_id, itemType::integer));
            _add(new configItem("archive", &archive, itemType::boolean));
            _add(new configItem("skip_auto_update", &skip_auto_update, itemType::boolean));
            _add(new configItem("name", &name, itemType::string));
            _add(new configItem("order", &order, itemType::integerList));
            _add(new configItem("url", &url, itemType::string));
            _add(new configItem("info", &info, itemType::string));
            _add(new configItem("lastup", &sub_last_update, itemType::integer64));
        }

        // 按 id 顺序
        [[nodiscard]] QList<std::shared_ptr<ProxyEntity>> Profiles() const;

        // 按 显示 顺序
        [[nodiscard]] QList<std::shared_ptr<ProxyEntity>> ProfilesWithOrder() const;
    };
} // namespace NekoGui
