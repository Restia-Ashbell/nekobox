#pragma once

#include "protocol/AbstractBean.hpp"

namespace NekoGui_fmt {
    class SSHBean : public AbstractBean {
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
            _add("user", &user);
            _add("password", &password);
            _add("private_key", &privateKey);
            _add("private_key_path", &privateKeyPath);
            _add("private_key_passphrase", &privateKeyPassphrase);
            _add("host_key", &hostKey);
            _add("host_key_algorithms", &hostKeyAlgorithms);
            _add("client_version", &clientVersion);
        };

        QString DisplayType() override { return "SSH"; };

        CoreObjOutboundBuildResult BuildCoreObjSingBox() override;

        bool TryParseLink(const QString &link) override;

        QString ToShareLink() override;
    };
} // namespace NekoGui_fmt
