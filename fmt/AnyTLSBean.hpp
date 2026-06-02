#pragma once

#include "fmt/AbstractBean.hpp"

namespace NekoGui_fmt {
    class AnyTLSBean : public AbstractBean {
    public:
        QString password = "";
        QString idleSessionCheckInterval = "30s";
        QString idleSessionTimeout = "30s";
        int minIdleSession = 0;

        std::shared_ptr<V2rayStreamSettings> stream = std::make_shared<V2rayStreamSettings>();

        AnyTLSBean() : AbstractBean(0) {
            serverPort = 443;
            stream->security = "tls";
            _add("password", &password);
            _add("idleSessionCheckInterval", &idleSessionCheckInterval);
            _add("idleSessionTimeout", &idleSessionTimeout);
            _add("minIdleSession", &minIdleSession);
            _add("stream", dynamic_cast<JsonStore *>(stream.get()));
        };

        QString DisplayType() override { return "AnyTLS"; };

        CoreObjOutboundBuildResult BuildCoreObjSingBox() override;

        bool TryParseLink(const QString &link) override;

        QString ToShareLink() override;
    };
} // namespace NekoGui_fmt
