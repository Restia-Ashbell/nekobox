#pragma once

#include "fmt/AbstractBean.hpp"

namespace NekoGui_fmt {
    class TrojanVLESSBean : public AbstractBean {
    public:
        static constexpr int proxy_Trojan = 0;
        static constexpr int proxy_VLESS = 1;
        int proxy_type = proxy_Trojan;

        QString password = "";
        QString flow = "";

        std::shared_ptr<V2rayStreamSettings> stream = std::make_shared<V2rayStreamSettings>();
        MultiplexSettings multiplex;

        explicit TrojanVLESSBean(int _proxy_type) : AbstractBean(0) {
            proxy_type = _proxy_type;
            _add("pass", &password);
            _add("flow", &flow);
            _add("stream", dynamic_cast<JsonStore *>(stream.get()));
            _add("multiplex", dynamic_cast<JsonStore *>(&multiplex));
        };

        QString DisplayType() override { return proxy_type == proxy_VLESS ? "VLESS" : "Trojan"; };

        CoreObjOutboundBuildResult BuildCoreObjSingBox() override;

        bool TryParseLink(const QString &link) override;

        QString ToShareLink() override;
    };
} // namespace NekoGui_fmt