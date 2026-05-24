#pragma once

#include "main/NekoGui_DataStore.hpp"
#include "db/traffic/TrafficData.hpp"
#include "fmt/includes.h"

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
            _add(new configItem("type", &type, itemType::string));
            _add(new configItem("id", &id, itemType::integer));
            _add(new configItem("gid", &gid, itemType::integer));
            _add(new configItem("yc", &latency, itemType::integer));
            _add(new configItem("report", &full_test_report, itemType::string));

            // 可以不关联 bean，只加载 ProxyEntity 的信息
            if (bean_) {
                bean = std::shared_ptr<NekoGui_fmt::AbstractBean>(bean_);
                traffic_data = std::make_shared<NekoGui_traffic::TrafficData>("");
                // 有虚函数就要在这里 dynamic_cast
                _add(new configItem("bean", dynamic_cast<JsonStore *>(bean.get()), itemType::jsonStore));
                _add(new configItem("traffic", dynamic_cast<JsonStore *>(traffic_data.get()), itemType::jsonStore));
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

        [[nodiscard]] NekoGui_fmt::ChainBean *ChainBean() const {
            return (NekoGui_fmt::ChainBean *) bean.get();
        };

        [[nodiscard]] NekoGui_fmt::SocksHttpBean *SocksHTTPBean() const {
            return (NekoGui_fmt::SocksHttpBean *) bean.get();
        };

        [[nodiscard]] NekoGui_fmt::ShadowSocksBean *ShadowSocksBean() const {
            return (NekoGui_fmt::ShadowSocksBean *) bean.get();
        };

        [[nodiscard]] NekoGui_fmt::ShadowSocksRBean *ShadowSocksRBean() const {
            return (NekoGui_fmt::ShadowSocksRBean *) bean.get();
        };

        [[nodiscard]] NekoGui_fmt::VMessBean *VMessBean() const {
            return (NekoGui_fmt::VMessBean *) bean.get();
        };

        [[nodiscard]] NekoGui_fmt::TrojanVLESSBean *TrojanVLESSBean() const {
            return (NekoGui_fmt::TrojanVLESSBean *) bean.get();
        };

        [[nodiscard]] NekoGui_fmt::NaiveBean *NaiveBean() const {
            return (NekoGui_fmt::NaiveBean *) bean.get();
        };

        [[nodiscard]] NekoGui_fmt::QUICBean *QUICBean() const {
            return (NekoGui_fmt::QUICBean *) bean.get();
        };

        [[nodiscard]] NekoGui_fmt::AnyTLSBean *AnyTLSBean() const {
            return (NekoGui_fmt::AnyTLSBean *) bean.get();
        };

        [[nodiscard]] NekoGui_fmt::SSHBean *SSHBean() const {
            return (NekoGui_fmt::SSHBean *) bean.get();
        };

        [[nodiscard]] NekoGui_fmt::WireGuardBean *WireGuardBean() const {
            return (NekoGui_fmt::WireGuardBean *) bean.get();
        };

        [[nodiscard]] NekoGui_fmt::CustomBean *CustomBean() const {
            return (NekoGui_fmt::CustomBean *) bean.get();
        };
    };
} // namespace NekoGui
