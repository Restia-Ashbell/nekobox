#include "TrafficLooper.hpp"

#include <QJsonObject>

#include "libbox.h"

#include "profile/DataStore.hpp"
#include "profile/ProfileManager.hpp"
#include "ui/MainWindow.hpp"

namespace NekoGui_traffic {

    TrafficLooper *trafficLooper = nullptr;

    TrafficLooper::TrafficLooper(QObject *parent) : QObject(parent) {
        connect(&m_timer, &QTimer::timeout, this, &TrafficLooper::onTick);
    }

    void TrafficLooper::start() {
        if (NekoGui::dataStore->traffic_loop_interval == 0) return; // user disabled

        auto interval = qBound(500, NekoGui::dataStore->traffic_loop_interval, 5000);
        if (!elapsedTimer.isValid()) elapsedTimer.start();
        m_timer.start(interval);
    }

    void TrafficLooper::stop() {
        m_timer.stop();
    }

    void TrafficLooper::onTick() {
        if (NekoGui::dataStore->traffic_loop_interval == 0) return; // user disabled

        // follow runtime interval change
        auto interval = qBound(500, NekoGui::dataStore->traffic_loop_interval, 5000);
        if (m_timer.interval() != interval) {
            m_timer.setInterval(interval);
        }

        UpdateAll();

        auto m = MainWindow::instance();
        if (proxy != nullptr) {
            m->refresh_status(QObject::tr("Proxy: %1\nDirect: %2").arg(proxy->DisplaySpeed(), direct->DisplaySpeed()));
        }
        for (const auto &item: items) {
            if (item->id >= 0)
                m->refresh_proxy(item->id);
        }
    }

    void TrafficLooper::update_stats(TrafficData *item, QJsonObject &stats) {
        // last update
        auto now = elapsedTimer.elapsed();
        auto interval = now - item->last_update;
        item->last_update = now;
        if (interval <= 0) return;

        // query
        QJsonObject ups = stats["ups"].toObject();
        QJsonObject downs = stats["downs"].toObject();
        auto uplink = ups[item->tag].toInteger();
        auto downlink = downs[item->tag].toInteger();

        // add diff
        item->uplink += uplink;
        item->downlink += downlink;
        item->uplink_rate = uplink * 1000 / interval;
        item->downlink_rate = downlink * 1000 / interval;
    }

    void TrafficLooper::UpdateAll() {
        auto boxStatsResult = BoxStats();
        auto stats = QString2QJsonObject(boxStatsResult);
        free(boxStatsResult);

        for (const auto &item: items) {
            update_stats(item.get(), stats);
        }
        update_stats(direct, stats);
    }

    void TrafficLooper::SaveAll() {
        for (const auto &item: items) {
            if (auto profile = NekoGui::profileManager->GetProfile(item->id)) profile->Save();
        }
    }

} // namespace NekoGui_traffic