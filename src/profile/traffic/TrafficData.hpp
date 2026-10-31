#pragma once

#include "profile/ConfigItem.hpp"

namespace NekoGui_traffic {
    class TrafficData : public JsonStore {
        Q_OBJECT
        Q_PROPERTY(qlonglong dl MEMBER downlink)
        Q_PROPERTY(qlonglong ul MEMBER uplink)

    public:
        QString tag;

        long long downlink = 0;
        long long uplink = 0;
        long long downlink_rate = 0;
        long long uplink_rate = 0;

        long long last_update = 0;

        explicit TrafficData(QObject *parent) : JsonStore(parent) {}

        void Reset() {
            downlink = 0;
            uplink = 0;
            downlink_rate = 0;
            uplink_rate = 0;
        }

        [[nodiscard]] QString DisplaySpeed() const {
            return WrapLtr(QString("%1↑ %2↓").arg(ReadableSize(uplink_rate), ReadableSize(downlink_rate)));
        }

        [[nodiscard]] QString DisplayTraffic() const {
            if (downlink + uplink == 0) return "";
            return WrapLtr(QString("%1↑ %2↓").arg(ReadableSize(uplink), ReadableSize(downlink)));
        }
    };
} // namespace NekoGui_traffic
