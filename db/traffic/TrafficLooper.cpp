#include "TrafficLooper.hpp"

#include <QThread>
#include <QJsonObject>

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
            auto data = item.get();
            update_stats(data, stats);
        }
        update_stats(direct, stats);
    }

    void TrafficLooper::Loop() {
        elapsedTimer.start();
        while (true) {
            auto sleep_ms = NekoGui::dataStore->traffic_loop_interval;
            if (sleep_ms < 500 || sleep_ms > 5000) sleep_ms = 1000;
            QThread::msleep(sleep_ms);
            if (NekoGui::dataStore->traffic_loop_interval == 0) continue; // user disabled

            // profile start and stop
            if (!loop_enabled) {
                // 停止
                if (looping) {
                    looping = false;
                    runOnUiThread([=, this] {
                        auto m = MainWindow::instance();
                        m->refresh_status("STOP");
                        for (const auto &item: items) {
                            NekoGui::profileManager->GetProfile(item->id)->Save();
                            m->refresh_proxy_list(item->id);
                        }
                    });
                }
                continue;
            } else {
                // 开始
                if (!looping) {
                    looping = true;
                }
            }

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
                        m->refresh_proxy_list(item->id);
                }
            });
        }
    }

} // namespace NekoGui_traffic
