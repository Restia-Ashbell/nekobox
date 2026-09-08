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

        void start(const QList<std::shared_ptr<TrafficData>> &items, TrafficData *proxy);
        void stop();
        void saveAll();

    signals:
        void speedUpdated(const QString &text);
        void profileUpdated(int id);

    private slots:
        void onTick();

    private:
        void applyInterval();
        void updateAll();
        void updateStats(TrafficData *item, const QJsonObject &stats);

        QTimer m_timer;
        QElapsedTimer m_elapsedTimer;
        QList<std::shared_ptr<TrafficData>> m_items;
        TrafficData *m_proxy = nullptr;
        TrafficData *m_direct = new TrafficData("direct");
    };

    extern TrafficLooper *trafficLooper;
} // namespace NekoGui_traffic