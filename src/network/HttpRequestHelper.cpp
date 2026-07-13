#include "network/HttpRequestHelper.hpp"

#include <QNetworkProxy>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QThread>

#include "profile/DataStore.hpp"

namespace NekoGui_network {

    void NetworkRequestHelper::HttpGet(const QUrl &url, std::function<void(const NekoHTTPResponse &)> onDone) {
        // networkManager lives on the main thread, so the actual request must
        // run there. If called off-thread (e.g. from the subscription worker),
        // bounce to the main thread and continue asynchronously.
        if (QThread::currentThread() != qApp->thread()) {
            QMetaObject::invokeMethod(qApp, [=] { HttpGet(url, std::move(onDone)); }, Qt::QueuedConnection);
            return;
        }

        // Set proxy
        if (NekoGui::dataStore->sub_use_proxy) {
            if (NekoGui::dataStore->started_id < 0) {
                onDone(NekoHTTPResponse{QObject::tr("Request with proxy but no profile started.")});
                return;
            }
            QNetworkProxy proxy;
            // Note: sing-box mixed socks5 protocol error
            proxy.setType(QNetworkProxy::HttpProxy);
            proxy.setHostName("127.0.0.1");
            proxy.setPort(NekoGui::dataStore->inbound_port);
            if (NekoGui::dataStore->inbound_auth->NeedAuth()) {
                proxy.setUser(NekoGui::dataStore->inbound_auth->username);
                proxy.setPassword(NekoGui::dataStore->inbound_auth->password);
            }
            networkManager->setProxy(proxy);
        }

        QNetworkRequest request(url);
        request.setTransferTimeout(10000);
        request.setHeader(QNetworkRequest::KnownHeaders::UserAgentHeader, NekoGui::dataStore->GetUserAgent());
        if (NekoGui::dataStore->sub_insecure) {
            QSslConfiguration ssl;
            ssl.setPeerVerifyMode(QSslSocket::PeerVerifyMode::VerifyNone);
            request.setSslConfiguration(ssl);
        }
        QNetworkReply *reply = networkManager->get(request);

        // Async: deliver the result through the callback when finished.
        QObject::connect(reply, &QNetworkReply::finished, [=] {
            NekoHTTPResponse result{reply->error() == QNetworkReply::NetworkError::NoError ? "" : reply->errorString(),
                                    reply->readAll(), reply->rawHeaderPairs()};
            reply->deleteLater();
            onDone(result);
        });
    }

    QByteArray NetworkRequestHelper::GetHeader(const QList<QPair<QByteArray, QByteArray>> &headers, const QByteArray &name) {
        for (const auto &[k, v]: headers) {
            if (k.toLower() == name.toLower()) return v;
        }
        return {};
    }

} // namespace NekoGui_network
