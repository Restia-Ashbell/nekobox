#pragma once

#include <QString>
#include <QList>
#include <QMutex>
#include <QElapsedTimer>

#include "TrafficData.hpp"

namespace NekoGui_traffic {
    class TrafficLooper {
    public:
        bool loop_enabled = false;
        bool looping = false;
        QMutex loop_mutex;

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
