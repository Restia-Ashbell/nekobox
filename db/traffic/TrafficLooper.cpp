#include "TrafficLooper.hpp"

#include <QJsonObject>
#include <QThread>

#include "libbox.h"

#include "db/ProfileManager.hpp"
#include "ui/mainwindow.h"

namespace NekoGui_traffic {

    void TrafficLooper::update_stats(TrafficData *item, QJsonObject &stats) {
        // last update
        auto now = elapsedTimer.elapsed();
        auto interval = now - item->last_update;
        item->last_update = now;
        if (interval <= 0) return;

        // query
        QJsonObject ups = stats["ups"].toObject();
        QJsonObject downs = stats["downs"].toObject();
        auto uplink = ups[item->tag].toDouble();
        auto downlink = downs[item->tag].toDouble();

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

    void TrafficLooper::Loop() {
        elapsedTimer.start();
        while (true) {
            QThread::msleep(qBound(500, NekoGui::dataStore->traffic_loop_interval, 5000));
            if (NekoGui::dataStore->traffic_loop_interval == 0) continue; // user disabled

            // profile start and stop
            if (!loop_enabled) continue;

            // do update
            UpdateAll();

            // post to UI
            runOnUiThread([=, this] {
                auto m = MainWindow::instance();
                if (proxy != nullptr) {
                    m->refresh_status(QObject::tr("Proxy: %1\nDirect: %2").arg(proxy->DisplaySpeed(), direct->DisplaySpeed()));
                }
                for (const auto &item: items) {
                    if (item->id >= 0)
                        m->refresh_proxy(item->id);
                }
            });
        }
    }

} // namespace NekoGui_traffic
