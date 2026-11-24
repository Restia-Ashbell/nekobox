#pragma once

#include "ConfigItem.hpp"

namespace NekoGui {

    class Group : public JsonStore {
        Q_OBJECT
        Q_PROPERTY(int id MEMBER id)
        Q_PROPERTY(int front_proxy_id MEMBER front_proxy_id)
        Q_PROPERTY(int auto_update_interval MEMBER auto_update_interval)
        Q_PROPERTY(QString name MEMBER name)
        Q_PROPERTY(QList<int> order MEMBER order)
        Q_PROPERTY(QString url MEMBER url)
        Q_PROPERTY(QString info MEMBER info)
        Q_PROPERTY(qint64 lastup MEMBER sub_last_update)

    public:
        int id = -1;
        int auto_update_interval = -24;
        QString name = "";
        QString url = "";
        QString info = "";
        qint64 sub_last_update = 0;
        int front_proxy_id = -1;

        // list ui
        QList<int> order;
    };

} // namespace NekoGui