#pragma once

#include <QHostInfo>

#include "protocol/V2RayStreamSettings.hpp"
#include "profile/DataStore.hpp"

namespace NekoGui_fmt {
    struct CoreObjOutboundBuildResult {
    public:
        QJsonObject outbound;
        QString error;
    };

    struct ExternalBuildResult {
    public:
        QString program;
        QStringList env;
        QStringList arguments;
        //
        QString tag;
        //
        QString error;
        QString config_export;
    };

    class AbstractBean : public JsonStore {
        Q_OBJECT
        Q_PROPERTY(int _v MEMBER version)
        Q_PROPERTY(QString name MEMBER name)
        Q_PROPERTY(QString addr MEMBER serverAddress)
        Q_PROPERTY(int port MEMBER serverPort)
        Q_PROPERTY(QString c_cfg MEMBER custom_config)
        Q_PROPERTY(QString c_out MEMBER custom_outbound)
        Q_PROPERTY(bool external MEMBER external)

    public:
        int version;

        QString name = "";
        QString serverAddress = "127.0.0.1";
        int serverPort = 1080;

        QString custom_config = "";
        QString custom_outbound = "";

        bool external = false;

        explicit AbstractBean(int v, QObject *parent = nullptr) : JsonStore(parent), version(v) {}

        virtual ~AbstractBean() = default;

        //

        QString ToNekorayShareLink(const QString &type) {
            auto b = ToJson();
            QUrl url;
            url.setScheme("nekobox");
            url.setHost(type);
            url.setFragment(QJsonObject2QString(b, true).toUtf8().toBase64(QByteArray::Base64UrlEncoding));
            return url.toString();
        }

        void ResolveDomainToIP(const std::function<void()> &onFinished) {
            QHostInfo::lookupHost(serverAddress, [=, this](const QHostInfo &host) {
                if (!IsIpAddress(serverAddress)) {
                    auto addr = host.addresses();
                    if (!addr.isEmpty()) {
                        // replace ws tls
                        if (auto *stream = _get<V2rayStreamSettings>("stream")) {
                            if (stream->security == "tls" && stream->sni.isEmpty()) {
                                stream->sni = serverAddress;
                            }
                            if (stream->network == "ws" && stream->host.isEmpty()) {
                                stream->host = serverAddress;
                            }
                        }
                        // replace serverAddress
                        serverAddress = addr.first().toString();
                    }
                }
                onFinished();
            });
        }

        //

        virtual QString DisplayType() = 0;

        virtual QString DisplayCoreType() {
            return software_core_name;
        }

        [[nodiscard]] virtual QString DisplayAddress() {
            return MakeHostPort(serverAddress, serverPort);
        }

        [[nodiscard]] virtual QString DisplayName() {
            return name.isEmpty() ? DisplayAddress() : name;
        }

        virtual QString DisplayTypeAndName() {
            return QString("[%1] %2").arg(DisplayType(), DisplayName());
        }

        //

        virtual CoreObjOutboundBuildResult BuildCoreObjSingBox() { return {}; }

        virtual ExternalBuildResult BuildExternal(int mapping_port, int socks_port) { return {}; }

        virtual QString ToShareLink() { return {}; }

        virtual bool TryParseLink(const QString &link) { return false; }
    };

    class MultiplexSettings : public JsonStore {
        Q_OBJECT
        Q_PROPERTY(bool enabled MEMBER enabled)
        Q_PROPERTY(bool padding MEMBER padding)
        Q_PROPERTY(QString protocol MEMBER protocol)
        Q_PROPERTY(int max_streams MEMBER max_streams)
        Q_PROPERTY(int brutal_up MEMBER brutal_up)
        Q_PROPERTY(int brutal_down MEMBER brutal_down)

    public:
        bool enabled = false;
        bool padding = false;
        QString protocol = "h2mux";
        int max_streams = 0;
        int brutal_up = 0;
        int brutal_down = 0;

        explicit MultiplexSettings(QObject *parent) : JsonStore(parent) {}

        void BuildMultiplexSettingsSingBox(QJsonObject *outbound);
    };
} // namespace NekoGui_fmt
