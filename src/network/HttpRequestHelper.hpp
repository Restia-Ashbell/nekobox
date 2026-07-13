#pragma once

#include <QNetworkAccessManager>
#include <QUrl>

namespace NekoGui_network {
    inline QNetworkAccessManager *networkManager = nullptr;

    struct NekoHTTPResponse {
        QString error;
        QByteArray data;
        QList<QPair<QByteArray, QByteArray>> headers;
    };

    class NetworkRequestHelper {
    public:
        static void HttpGet(const QUrl &url, std::function<void(const NekoHTTPResponse &)> onDone);

        static QByteArray GetHeader(const QList<QPair<QByteArray, QByteArray>> &header, const QByteArray &name);
    };
} // namespace NekoGui_network
