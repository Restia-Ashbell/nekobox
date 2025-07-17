#pragma once

#include "fmt/AbstractBean.hpp"
#include "fmt/V2RayStreamSettings.hpp"

namespace NekoGui_fmt {
    class AnyTLSBean : public AbstractBean {
    public:
        QString password = "";
        QString idleSessionCheckInterval = "30s";
        QString idleSessionTimeout = "30s";
        int minIdleSession = 0;

        std::shared_ptr<V2rayStreamSettings> stream = std::make_shared<V2rayStreamSettings>();

        AnyTLSBean() : AbstractBean(0) {
            stream->security = "tls";
            _add(new configItem("password", &password, itemType::string));
            _add(new configItem("idleSessionCheckInterval", &idleSessionCheckInterval, itemType::string));
            _add(new configItem("idleSessionTimeout", &idleSessionTimeout, itemType::string));
            _add(new configItem("minIdleSession", &minIdleSession, itemType::integer));
            _add(new configItem("stream", dynamic_cast<JsonStore *>(stream.get()), itemType::jsonStore));
        };

        QString DisplayType() override { return "AnyTLS"; };

        CoreObjOutboundBuildResult BuildCoreObjSingBox() override;

        bool TryParseLink(const QString &link);

        QString ToShareLink() override;
    };
} // namespace NekoGui_fmt
