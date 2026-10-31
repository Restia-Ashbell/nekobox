#include "TrafficLooper.hpp"

#include <QJsonObject>

#include "libbox.h"

#include "profile/DataStore.hpp"
#include "profile/ProfileManager.hpp"

namespace NekoGui_traffic {

    TrafficLooper *trafficLooper = nullptr;

    TrafficLooper::TrafficLooper(QObject *parent) : QObject(parent) {
        m_direct = new TrafficData(this);
        m_direct->tag = "direct";
        connect(&m_timer, &QTimer::timeout, this, &TrafficLooper::onTick);
    }

    void TrafficLooper::start(const QList<std::shared_ptr<NekoGui::ProxyEntity>> &items) {
        m_items = items;
        if (NekoGui::dataStore->traffic_loop_interval == 0) return; // user disabled

        if (!m_elapsedTimer.isValid()) m_elapsedTimer.start();
        applyInterval();
        m_timer.start();
    }

    void TrafficLooper::stop() {
        m_timer.stop();
        m_items.clear();
    }

    void TrafficLooper::applyInterval() {
        auto interval = qBound(500, NekoGui::dataStore->traffic_loop_interval, 5000);
        if (m_timer.interval() != interval) m_timer.setInterval(interval);
    }

    void TrafficLooper::onTick() {
        if (NekoGui::dataStore->traffic_loop_interval == 0) return; // user disabled

        applyInterval(); // follow runtime interval change

        updateAll();

        if (!m_items.isEmpty()) {
            emit speedUpdated(QObject::tr("Proxy: %1 | Direct: %2").arg(m_items.last()->traffic_data->DisplaySpeed(), m_direct->DisplaySpeed()));
        }
        for (const auto &item: m_items) {
            if (item->id >= 0) emit profileUpdated(item->id);
        }
    }

    void TrafficLooper::updateStats(TrafficData *item, const QJsonObject &stats) {
        // last update
        auto now = m_elapsedTimer.elapsed();
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

    void TrafficLooper::updateAll() {
        auto boxStatsResult = BoxStats();
        auto stats = QString2QJsonObject(boxStatsResult);
        free(boxStatsResult);

        for (const auto &item: m_items) {
            updateStats(item->traffic_data, stats);
        }
        updateStats(m_direct, stats);
    }

    void TrafficLooper::saveAll() {
        for (const auto &item: m_items) {
            item->Save();
        }
    }

} // namespace NekoGui_traffic