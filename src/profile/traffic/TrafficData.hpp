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

        explicit TrafficData(QObject *parent) : JsonStore(parent) {}

        void Reset() {
            downlink = 0;
            uplink = 0;
        }

        [[nodiscard]] QString DisplayTraffic() const {
            return downlink + uplink > 0 ? WrapLtr(QString("%1↑ %2↓").arg(ReadableSize(uplink), ReadableSize(downlink))) : QString();
        }
    };
} // namespace NekoGui_traffic
