#pragma once

#include <QJsonObject>
#include <QList>
#include <QPair>
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
        QJsonObject queryStats();
        QString updateAll(const QJsonObject &stats);
        QPair<qint64, qint64> updateStats(TrafficData *item, const QJsonObject &ups, const QJsonObject &downs, const QJsonObject &lastUps, const QJsonObject &lastDowns);

        QTimer m_timer;
        QList<std::shared_ptr<NekoGui::ProxyEntity>> m_items;
        TrafficData *m_direct;
        QJsonObject m_lastStats;
    };

    extern TrafficLooper *trafficLooper;
} // namespace NekoGui_traffic