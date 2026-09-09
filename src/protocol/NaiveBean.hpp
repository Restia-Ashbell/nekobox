#pragma once

#include "protocol/AbstractBean.hpp"

namespace NekoGui_fmt {
    class NaiveBean : public AbstractBean {
        Q_OBJECT
        Q_PROPERTY(QString username MEMBER username)
        Q_PROPERTY(QString password MEMBER password)
        Q_PROPERTY(QString protocol MEMBER protocol)
        Q_PROPERTY(QString extra_headers MEMBER extra_headers)
        Q_PROPERTY(QString sni MEMBER sni)
        Q_PROPERTY(QString certificate MEMBER certificate)
        Q_PROPERTY(int insecure_concurrency MEMBER insecure_concurrency)
        Q_PROPERTY(bool disable_log MEMBER disable_log)

    public:
        QString username = "";
        QString password = "";
        QString protocol = "https";
        QString extra_headers = "";
        QString sni = "";
        QString certificate = "";
        int insecure_concurrency = 0;

        bool disable_log = false;

        NaiveBean() : AbstractBean(0) {
            serverPort = 443;
            external = true;
        }

        QString DisplayCoreType() override { return "Naive"; };

        QString DisplayType() override { return "Naive"; };

        ExternalBuildResult BuildExternal(int mapping_port, int socks_port) override;

        bool TryParseLink(const QString &link) override;

        QString ToShareLink() override;
    };
} // namespace NekoGui_fmt