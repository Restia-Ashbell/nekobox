#pragma once

#include <QElapsedTimer>
#include <QList>
#include <QString>

#include "TrafficData.hpp"

namespace NekoGui_traffic {
    class TrafficLooper {
    public:
        bool loop_enabled = false;

        QList<std::shared_ptr<TrafficData>> items;
        TrafficData *proxy = nullptr;

        void UpdateAll();

        void Loop();

    private:
        QElapsedTimer elapsedTimer;

        TrafficData *direct = new TrafficData("direct");

        void update_stats(TrafficData *item, QJsonObject &stats);
    };

    inline TrafficLooper *trafficLooper = new TrafficLooper;
} // namespace NekoGui_traffic
