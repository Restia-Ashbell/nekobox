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
            _add(new configItem("net", &network, itemType::string));
            _add(new configItem("sec", &security, itemType::string));
            _add(new configItem("pac_enc", &packet_encoding, itemType::string));
            _add(new configItem("path", &path, itemType::string));
            _add(new configItem("host", &host, itemType::string));
            _add(new configItem("sni", &sni, itemType::string));
            _add(new configItem("alpn", &alpn, itemType::string));
            _add(new configItem("cert", &certificate, itemType::string));
            _add(new configItem("ech", &ech, itemType::string));
            _add(new configItem("insecure", &allow_insecure, itemType::boolean));
            _add(new configItem("ech_enabled", &ech_enabled, itemType::boolean));
            _add(new configItem("disable_sni", &disable_sni, itemType::boolean));
            _add(new configItem("ed_name", &ws_early_data_name, itemType::string));
            _add(new configItem("ed_len", &ws_early_data_length, itemType::integer));
            _add(new configItem("utls", &utlsFingerprint, itemType::string));
            _add(new configItem("pbk", &reality_pbk, itemType::string));
            _add(new configItem("sid", &reality_sid, itemType::string));
            _add(new configItem("spx", &reality_spx, itemType::string));
            _add(new configItem("tls_fragment", &tls_fragment, itemType::boolean));
            _add(new configItem("tls_record_fragment", &tls_record_fragment, itemType::boolean));
        }

        void BuildStreamSettingsSingBox(QJsonObject *outbound);
    };
} // namespace NekoGui_fmt
