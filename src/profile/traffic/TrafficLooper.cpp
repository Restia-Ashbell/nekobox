#include "TrafficLooper.hpp"

#include "libbox.h"

#include "profile/DataStore.hpp"

namespace NekoGui_traffic {

    TrafficLooper *trafficLooper = nullptr;

    static QString formatSpeed(qint64 up, qint64 down) {
        return WrapLtr(QString("%1↑ %2↓").arg(ReadableSize(up), ReadableSize(down)));
    }

    TrafficLooper::TrafficLooper(QObject *parent) : QObject(parent) {
        m_direct = new TrafficData(this);
        m_timer.setInterval(1000);
        connect(&m_timer, &QTimer::timeout, this, &TrafficLooper::onTick);
    }

    void TrafficLooper::start(const QList<QPair<QString, std::shared_ptr<NekoGui::ProxyEntity>>> &items) {
        m_items = items;
        if (!m_items.isEmpty()) m_timer.start();
    }

    void TrafficLooper::stop() {
        m_timer.stop();
        m_items.clear();
        m_lastStats = {};
    }

    void TrafficLooper::onTick() {
        if (m_items.isEmpty()) return;

        auto stats = queryStats();
        auto speedText = updateAll(stats);
        m_lastStats = stats;

        if (NekoGui::dataStore->traffic_show_speed) {
            emit speedUpdated(speedText);
        }

        if (NekoGui::dataStore->traffic_count_stats) {
            for (const auto &[_, ent]: m_items) {
                if (ent->id >= 0) emit profileUpdated(ent->id);
            }
        }
    }

    QJsonObject TrafficLooper::queryStats() {
        auto result = BoxStats();
        if (result == nullptr) return {};
        auto stats = QJsonDocument::fromJson(result).object();
        free(result);
        return stats;
    }

    QPair<qint64, qint64> TrafficLooper::updateStats(TrafficData *item, const QString &tag, const QJsonObject &ups, const QJsonObject &downs, const QJsonObject &lastUps, const QJsonObject &lastDowns) {
        auto current_up = ups.value(tag).toInteger();
        auto current_down = downs.value(tag).toInteger();
        auto last_up = lastUps.value(tag).toInteger();
        auto last_down = lastDowns.value(tag).toInteger();

        // A decrease means the core restarted its counters.
        auto delta_up = current_up >= last_up ? current_up - last_up : current_up;
        auto delta_down = current_down >= last_down ? current_down - last_down : current_down;

        if (NekoGui::dataStore->traffic_count_stats) {
            item->uplink += delta_up;
            item->downlink += delta_down;
        }

        return {delta_up, delta_down};
    }

    QString TrafficLooper::updateAll(const QJsonObject &stats) {
        auto ups = stats.value("ups").toObject();
        auto downs = stats.value("downs").toObject();
        auto lastUps = m_lastStats.value("ups").toObject();
        auto lastDowns = m_lastStats.value("downs").toObject();

        qint64 proxyUp = 0, proxyDown = 0, directUp = 0, directDown = 0;
        for (const auto &[tag, ent]: m_items) {
            auto delta = updateStats(ent->traffic_data, tag, ups, downs, lastUps, lastDowns);
            if (tag == "proxy") {
                proxyUp = delta.first;
                proxyDown = delta.second;
            }
        }
        auto directDelta = updateStats(m_direct, "direct", ups, downs, lastUps, lastDowns);
        directUp = directDelta.first;
        directDown = directDelta.second;

        return QObject::tr("Proxy: %1 | Direct: %2").arg(formatSpeed(proxyUp, proxyDown), formatSpeed(directUp, directDown));
    }

    void TrafficLooper::saveAll() {
        for (const auto &[_, ent]: m_items) {
            ent->Save();
        }
    }

} // namespace NekoGui_traffic
