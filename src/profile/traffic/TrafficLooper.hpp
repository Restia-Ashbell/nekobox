#pragma once

#include <QElapsedTimer>
#include <QList>
#include <QString>
#include <QTimer>

#include "TrafficData.hpp"

namespace NekoGui_traffic {
    class TrafficLooper : public QObject {
        Q_OBJECT
    public:
        explicit TrafficLooper(QObject *parent = nullptr);

        QList<std::shared_ptr<TrafficData>> items;
        TrafficData *proxy = nullptr;

        void start();
        void stop();

        void SaveAll();
        void UpdateAll();

    private slots:
        void onTick();

    private:
        QTimer m_timer;
        QElapsedTimer elapsedTimer;
        TrafficData *direct = new TrafficData("direct");

        void update_stats(TrafficData *item, QJsonObject &stats);
    };

    extern TrafficLooper *trafficLooper;
} // namespace NekoGui_traffic