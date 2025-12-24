#pragma once

#include <QHostInfo>

#include "fmt/V2RayStreamSettings.hpp"
#include "main/NekoGui_DataStore.hpp"

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
    public:
        int version;

        QString name = "";
        QString serverAddress = "127.0.0.1";
        int serverPort = 1080;

        QString custom_config = "";
        QString custom_outbound = "";

        explicit AbstractBean(int v) : version(v) {
            _add(new configItem("_v", &version, itemType::integer));
            _add(new configItem("name", &name, itemType::string));
            _add(new configItem("addr", &serverAddress, itemType::string));
            _add(new configItem("port", &serverPort, itemType::integer));
            _add(new configItem("c_cfg", &custom_config, itemType::string));
            _add(new configItem("c_out", &custom_outbound, itemType::string));
        }

        //

        template<typename T>
        T *GetConfigItemPtr(const QString &name) {
            auto item = this->_get(name);
            if (!item) return nullptr;
            return (T *) (item->ptr);
        }

        QString ToNekorayShareLink(const QString &type) {
            auto b = ToJson();
            QUrl url;
            url.setScheme("nekoray");
            url.setHost(type);
            url.setFragment(QJsonObject2QString(b, true).toUtf8().toBase64(QByteArray::Base64UrlEncoding));
            return url.toString();
        }

        void ResolveDomainToIP(const std::function<void()> &onFinished) {
            QHostInfo::lookupHost(serverAddress, QApplication::instance(), [=, this](const QHostInfo &host) {
                if (!IsIpAddress(serverAddress)) {
                    auto addr = host.addresses();
                    if (!addr.isEmpty()) {
                        // replace ws tls
                        if (auto *stream = GetConfigItemPtr<V2rayStreamSettings>("stream")) {
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
            return ::DisplayAddress(serverAddress, serverPort);
        }

        [[nodiscard]] virtual QString DisplayName() {
            return name.isEmpty() ? DisplayAddress() : name;
        }

        virtual QString DisplayTypeAndName() {
            return QString("[%1] %2").arg(DisplayType(), DisplayName());
        }

        //

        virtual int NeedExternal(bool isFirstProfile) { return 0; }

        virtual CoreObjOutboundBuildResult BuildCoreObjSingBox() { return {}; }

        virtual ExternalBuildResult BuildExternal(int mapping_port, int socks_port, int external_stat) { return {}; }

        virtual QString ToShareLink() { return {}; }

        virtual bool TryParseLink(const QString &link) { return false; }
    };

    class MultiplexSettings : public JsonStore {
    public:
        bool enabled = false;
        bool padding = false;
        QString protocol = "h2mux";
        int max_streams = 0;
        int brutal_up = 0;
        int brutal_down = 0;

        MultiplexSettings() : JsonStore() {
            _add(new configItem("enabled", &enabled, itemType::boolean));
            _add(new configItem("padding", &padding, itemType::boolean));
            _add(new configItem("protocol", &protocol, itemType::string));
            _add(new configItem("max_streams", &max_streams, itemType::integer));
            _add(new configItem("brutal_up", &brutal_up, itemType::integer));
            _add(new configItem("brutal_down", &brutal_down, itemType::integer));
        }

        void BuildMultiplexSettingsSingBox(QJsonObject *outbound);
    };
} // namespace NekoGui_fmt
