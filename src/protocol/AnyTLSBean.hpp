#pragma once

#include "protocol/AbstractBean.hpp"

namespace NekoGui_fmt {
    class AnyTLSBean : public AbstractBean {
        Q_OBJECT
        Q_PROPERTY(QString password MEMBER password)
        Q_PROPERTY(QString idleSessionCheckInterval MEMBER idleSessionCheckInterval)
        Q_PROPERTY(QString idleSessionTimeout MEMBER idleSessionTimeout)
        Q_PROPERTY(int minIdleSession MEMBER minIdleSession)
        Q_PROPERTY(V2rayStreamSettings *stream MEMBER stream)

    public:
        QString password = "";
        QString idleSessionCheckInterval = "30s";
        QString idleSessionTimeout = "30s";
        int minIdleSession = 0;

        V2rayStreamSettings *stream = nullptr;

        AnyTLSBean() : AbstractBean(0) {
            serverPort = 443;
            stream = new V2rayStreamSettings(this);
            stream->security = "tls";
        }

        QString DisplayType() override { return "AnyTLS"; };

        CoreObjOutboundBuildResult BuildCoreObjSingBox() override;

        bool TryParseLink(const QString &link) override;

        QString ToShareLink() override;
    };
} // namespace NekoGui_fmt
