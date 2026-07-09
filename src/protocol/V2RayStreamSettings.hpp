#pragma once

namespace NekoGui_fmt {
    class V2rayStreamSettings : public JsonStore {
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

        V2rayStreamSettings() : JsonStore() {
            _add("net", &network);
            _add("sec", &security);
            _add("pac_enc", &packet_encoding);
            _add("path", &path);
            _add("host", &host);
            _add("sni", &sni);
            _add("alpn", &alpn);
            _add("cert", &certificate);
            _add("ech", &ech);
            _add("insecure", &allow_insecure);
            _add("ech_enabled", &ech_enabled);
            _add("disable_sni", &disable_sni);
            _add("ed_name", &ws_early_data_name);
            _add("ed_len", &ws_early_data_length);
            _add("utls", &utlsFingerprint);
            _add("pbk", &reality_pbk);
            _add("sid", &reality_sid);
            _add("spx", &reality_spx);
            _add("tls_fragment", &tls_fragment);
            _add("tls_record_fragment", &tls_record_fragment);
        }

        void BuildStreamSettingsSingBox(QJsonObject *outbound);
    };
} // namespace NekoGui_fmt
