#pragma once

#include <QElapsedTimer>
#include <QList>
#include <QString>
#include <QTimer>

#include "TrafficData.hpp"
#include "profile/ProxyEntity.hpp"

namespace NekoGui_traffic {
    class TrafficLooper : public QObject {
        Q_OBJECT
    public:
        explicit TrafficLooper(QObject *parent = nullptr);

        void start(const QList<std::shared_ptr<NekoGui::ProxyEntity>> &items);
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
        QList<std::shared_ptr<NekoGui::ProxyEntity>> m_items;
        TrafficData *m_direct;
    };

    extern TrafficLooper *trafficLooper;
} // namespace NekoGui_traffic