#pragma once

#include <QUrl>

namespace NekoGui_network {
    struct NekoHTTPResponse {
        QString error;
        QByteArray data;
        QList<QPair<QByteArray, QByteArray>> headers;
    };

    class NetworkRequestHelper {
    public:
        static NekoHTTPResponse HttpGet(const QUrl &url);

        static QByteArray GetHeader(const QList<QPair<QByteArray, QByteArray>> &header, const QByteArray &name);
    };
} // namespace NekoGui_network
