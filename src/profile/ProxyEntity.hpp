#pragma once

#include "profile/DataStore.hpp"
#include "profile/traffic/TrafficData.hpp"
#include "protocol/Includes.hpp"

namespace NekoGui {
    class ProxyEntity : public JsonStore {
        Q_OBJECT
        Q_PROPERTY(QString type MEMBER type)
        Q_PROPERTY(int id MEMBER id)
        Q_PROPERTY(int gid MEMBER gid)
        Q_PROPERTY(int yc MEMBER latency)
        Q_PROPERTY(QString report MEMBER full_test_report)
        Q_PROPERTY(NekoGui_fmt::AbstractBean *bean MEMBER bean)
        Q_PROPERTY(NekoGui_traffic::TrafficData *traffic MEMBER traffic_data)

    public:
        QString type;

        int id = -1;
        int gid = 0;
        int latency = 0;
        NekoGui_fmt::AbstractBean *bean = nullptr;
        NekoGui_traffic::TrafficData *traffic_data = nullptr;

        QString full_test_report;

        ProxyEntity(NekoGui_fmt::AbstractBean *bean_, const QString &type_) : type(type_) {
            // 可以不关联 bean,只加载 ProxyEntity 的信息
            if (bean_) {
                bean = bean_;
                bean->setParent(this);
                traffic_data = new NekoGui_traffic::TrafficData("", this);
            }
        }

        [[nodiscard]] QVariant DisplayLatency() const {
            if (latency < 0) {
                return QObject::tr("Unavailable");
            } else if (latency > 0) {
                return latency;
            } else {
                return {};
            }
        }

        [[nodiscard]] QColor DisplayLatencyColor() const {
            if (latency < 0) {
                return Qt::red;
            } else if (latency > 0) {
                auto greenMs = dataStore->test_latency_url.startsWith("https://") ? 200 : 100;
                return latency < greenMs ? Qt::darkGreen : Qt::darkYellow;
            } else {
                return {};
            }
        }

        template<typename T>
        [[nodiscard]] T *Bean() const {
            return dynamic_cast<T *>(bean);
        }
    };
} // namespace NekoGui
