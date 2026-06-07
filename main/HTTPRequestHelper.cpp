#include "HTTPRequestHelper.hpp"

#include <QEventLoop>
#include <QNetworkProxy>
#include <QNetworkReply>
#include <QNetworkRequest>

#include "main/NekoGui_DataStore.hpp"

namespace NekoGui_network {

    NekoHTTPResponse NetworkRequestHelper::HttpGet(const QUrl &url) {
        // Set proxy
        if (NekoGui::dataStore->sub_use_proxy) {
            if (NekoGui::dataStore->started_id < 0) {
                return NekoHTTPResponse{QObject::tr("Request with proxy but no profile started.")};
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

        QEventLoop loop;
        QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
        loop.exec();

        auto result = NekoHTTPResponse{reply->error() == QNetworkReply::NetworkError::NoError ? "" : reply->errorString(),
                                       reply->readAll(), reply->rawHeaderPairs()};
        reply->deleteLater();
        return result;
    }

    QByteArray NetworkRequestHelper::GetHeader(const QList<QPair<QByteArray, QByteArray>> &headers, const QByteArray &name) {
        for (const auto &[k, v]: headers) {
            if (k.toLower() == name.toLower()) return v;
        }
        return {};
    }

} // namespace NekoGui_network
