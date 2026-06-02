#pragma once

#include "db/traffic/TrafficData.hpp"
#include "fmt/includes.h"
#include "main/NekoGui_DataStore.hpp"

namespace NekoGui {
    class ProxyEntity : public JsonStore {
    public:
        QString type;

        int id = -1;
        int gid = 0;
        int latency = 0;
        std::shared_ptr<NekoGui_fmt::AbstractBean> bean;
        std::shared_ptr<NekoGui_traffic::TrafficData> traffic_data;

        QString full_test_report;

        ProxyEntity(NekoGui_fmt::AbstractBean *bean_, const QString &type_) : type(type_) {
            _add("type", &type);
            _add("id", &id);
            _add("gid", &gid);
            _add("yc", &latency);
            _add("report", &full_test_report);

            // 可以不关联 bean，只加载 ProxyEntity 的信息
            if (bean_) {
                bean = std::shared_ptr<NekoGui_fmt::AbstractBean>(bean_);
                traffic_data = std::make_shared<NekoGui_traffic::TrafficData>("");
                // 有虚函数就要在这里 dynamic_cast
                _add("bean", dynamic_cast<JsonStore *>(bean.get()));
                _add("traffic", dynamic_cast<JsonStore *>(traffic_data.get()));
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
            return dynamic_cast<T *>(bean.get());
        }
    };
} // namespace NekoGui
