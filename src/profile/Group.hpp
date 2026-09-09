#pragma once

#include "ProxyEntity.hpp"

namespace NekoGui {
    class Group : public JsonStore {
        Q_OBJECT
        Q_PROPERTY(int id MEMBER id)
        Q_PROPERTY(int front_proxy_id MEMBER front_proxy_id)
        Q_PROPERTY(bool archive MEMBER archive)
        Q_PROPERTY(bool skip_auto_update MEMBER skip_auto_update)
        Q_PROPERTY(QString name MEMBER name)
        Q_PROPERTY(QList<int> order MEMBER order)
        Q_PROPERTY(QString url MEMBER url)
        Q_PROPERTY(QString info MEMBER info)
        Q_PROPERTY(qint64 lastup MEMBER sub_last_update)

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

        Group() = default;

        // 按 id 顺序
        [[nodiscard]] QList<std::shared_ptr<ProxyEntity>> Profiles() const;

        // 按 显示 顺序
        [[nodiscard]] QList<std::shared_ptr<ProxyEntity>> ProfilesWithOrder() const;
    };
} // namespace NekoGui
