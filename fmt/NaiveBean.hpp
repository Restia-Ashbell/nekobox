#pragma once

#include "fmt/AbstractBean.hpp"

namespace NekoGui_fmt {
    class NaiveBean : public AbstractBean {
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
            _add("username", &username);
            _add("password", &password);
            _add("protocol", &protocol);
            _add("extra_headers", &extra_headers);
            _add("sni", &sni);
            _add("certificate", &certificate);
            _add("insecure_concurrency", &insecure_concurrency);
            _add("disable_log", &disable_log);
        };

        QString DisplayCoreType() override { return "Naive"; };

        QString DisplayType() override { return "Naive"; };

        int NeedExternal(bool isFirstProfile) override;

        ExternalBuildResult BuildExternal(int mapping_port, int socks_port, int external_stat) override;

        bool TryParseLink(const QString &link) override;

        QString ToShareLink() override;
    };
} // namespace NekoGui_fmt