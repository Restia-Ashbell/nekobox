#pragma once

#include <QUrlQuery>

#include "profile/ConfigItem.hpp"

namespace NekoGui_fmt {
    class V2rayStreamSettings : public JsonStore {
        Q_OBJECT
        Q_PROPERTY(QString net MEMBER network)
        Q_PROPERTY(QString sec MEMBER security)
        Q_PROPERTY(QString pac_enc MEMBER packet_encoding)
        // ws/http/grpc/httpupgrade
        Q_PROPERTY(QString path MEMBER path)
        Q_PROPERTY(QString host MEMBER host)
        // tls
        Q_PROPERTY(QString sni MEMBER sni)
        Q_PROPERTY(QString alpn MEMBER alpn)
        Q_PROPERTY(QString cert MEMBER certificate)
        Q_PROPERTY(QString ech MEMBER ech)
        Q_PROPERTY(bool insecure MEMBER allow_insecure)
        Q_PROPERTY(bool ech_enabled MEMBER ech_enabled)
        Q_PROPERTY(bool disable_sni MEMBER disable_sni)
        // ws early data
        Q_PROPERTY(QString ed_name MEMBER ws_early_data_name)
        Q_PROPERTY(int ed_len MEMBER ws_early_data_length)
        // reality
        Q_PROPERTY(QString utls MEMBER utlsFingerprint)
        Q_PROPERTY(QString pbk MEMBER reality_pbk)
        Q_PROPERTY(QString sid MEMBER reality_sid)
        Q_PROPERTY(QString spx MEMBER reality_spx)
        Q_PROPERTY(bool tls_fragment MEMBER tls_fragment)
        Q_PROPERTY(bool tls_record_fragment MEMBER tls_record_fragment)

    public:
        QString network = "";
        QString security = "";
        QString packet_encoding = "";
        // ws/http/grpc/httpupgrade
        QString path = "";
        QString host = "";
        // tls
        QString sni = "";
        QString alpn = "";
        QString certificate = "";
        QString ech = "";
        QString utlsFingerprint = "";
        bool allow_insecure = false;
        bool ech_enabled = false;
        bool disable_sni = false;
        // ws early data
        QString ws_early_data_name = "";
        int ws_early_data_length = 0;
        // reality
        QString reality_pbk = "";
        QString reality_sid = "";
        QString reality_spx = "";

        bool tls_fragment = false;
        bool tls_record_fragment = false;

        explicit V2rayStreamSettings(QObject *parent) : JsonStore(parent) {}

        void BuildStreamSettingsSingBox(QJsonObject *outbound);

        void BuildShareLinkQuery(QUrlQuery *query);

        void ParseShareLinkQuery(const QUrlQuery &query);
    };
} // namespace NekoGui_fmt
