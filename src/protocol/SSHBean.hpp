#pragma once

#include "protocol/AbstractBean.hpp"

namespace NekoGui_fmt {
    class SSHBean : public AbstractBean {
        Q_OBJECT
        Q_PROPERTY(QString user MEMBER user)
        Q_PROPERTY(QString password MEMBER password)
        Q_PROPERTY(QString private_key MEMBER privateKey)
        Q_PROPERTY(QString private_key_path MEMBER privateKeyPath)
        Q_PROPERTY(QString private_key_passphrase MEMBER privateKeyPassphrase)
        Q_PROPERTY(QString host_key MEMBER hostKey)
        Q_PROPERTY(QString host_key_algorithms MEMBER hostKeyAlgorithms)
        Q_PROPERTY(QString client_version MEMBER clientVersion)

    public:
        QString user;
        QString password;
        QString privateKey;
        QString privateKeyPath;
        QString privateKeyPassphrase;
        QString hostKey;
        QString hostKeyAlgorithms;
        QString clientVersion;

        SSHBean() : AbstractBean(0) {
            serverPort = 22;
        }

        QString DisplayType() override { return "SSH"; };

        CoreObjOutboundBuildResult BuildCoreObjSingBox() override;

        bool TryParseLink(const QString &link) override;

        QString ToShareLink() override;
    };
} // namespace NekoGui_fmt
