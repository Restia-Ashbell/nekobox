#pragma once

namespace Preset {
    namespace SingBox {
        inline QStringList LogLevel = {"trace", "debug", "info", "warn", "error", "fatal", "panic"};
        inline QStringList TunStack = {"mixed", "system", "gvisor"};
        inline QStringList DomainStrategy = {"", "prefer_ipv4", "prefer_ipv6", "ipv4_only", "ipv6_only"};
        inline QStringList UtlsFingerPrint = {"", "chrome", "firefox", "edge", "safari", "360", "qq", "ios", "android", "random", "randomized"};
        inline QStringList MultiplexProtocol = {"smux", "yamux", "h2mux"};
        inline QStringList CertificateStore = {"system", "mozilla", "none"};
        inline QStringList V2RayTransport = {"http", "ws", "quic", "grpc", "httpupgrade"};
        inline QStringList ShadowsocksMethods = {"2022-blake3-aes-128-gcm", "2022-blake3-aes-256-gcm", "2022-blake3-chacha20-poly1305", "none", "aes-128-gcm", "aes-192-gcm", "aes-256-gcm", "chacha20-ietf-poly1305", "xchacha20-ietf-poly1305", "aes-128-ctr", "aes-192-ctr", "aes-256-ctr", "aes-128-cfb", "aes-192-cfb", "aes-256-cfb", "rc4-md5", "chacha20-ietf", "xchacha20"};
        inline QStringList shadowaead2022 = {
            "2022-blake3-aes-128-gcm",
            "2022-blake3-aes-256-gcm",
            "2022-blake3-chacha20-poly1305",
            "2022-blake3-chacha8-poly1305",
            "2022-blake3-aes-128-ccm",
            "2022-blake3-aes-256-ccm",
        };
        inline QStringList shadowaead = {
            "aes-128-gcm",
            "aes-192-gcm",
            "aes-256-gcm",
            "chacha20-ietf-poly1305",
            "xchacha20-ietf-poly1305",
            "chacha8-ietf-poly1305",
            "xchacha8-ietf-poly1305",
            "rabbit128-poly1305",
            "aes-128-ccm",
            "aes-192-ccm",
            "aes-256-ccm",
            "aes-128-gcm-siv",
            "aes-256-gcm-siv",
            "aegis-128l",
            "aegis-256",
            "aez-384",
            "deoxys-ii-256-128",
            "lea-128-gcm",
            "lea-192-gcm",
            "lea-256-gcm",
            "sm4-gcm",
            "sm4-ccm",
        };
        inline QStringList shadowstream = {
            "aes-128-ctr",
            "aes-192-ctr",
            "aes-256-ctr",
            "aes-128-cfb",
            "aes-192-cfb",
            "aes-256-cfb",
            "aes-128-cfb8",
            "aes-192-cfb8",
            "aes-256-cfb8",
            "aes-128-ofb",
            "aes-192-ofb",
            "aes-256-ofb",
            "camellia-128-cfb",
            "camellia-192-cfb",
            "camellia-256-cfb",
            "camellia-128-cfb8",
            "camellia-192-cfb8",
            "camellia-256-cfb8",
            "rc4-md5",
            "rc4-md5-6",
            "rc4",
            "bf-cfb",
            "cast5-cfb",
            "des-cfb",
            "idea-cfb",
            "rc2-cfb",
            "seed-cfb",
            "chacha20-ietf",
            "xchacha20",
            "chacha20",
            "xsalsa20",
            "salsa20",
            "table",
            "rabbit",
            "hc128",
            "zuc128",
        };
        inline QStringList shadownone = {"none"};
        inline QStringList Flows = {"xtls-rprx-vision"};
    } // namespace SingBox

    namespace Windows {
        inline QStringList system_proxy_format{
            "{ip}:{http_port}",
            "socks={ip}:{socks_port}",
            "http={ip}:{http_port};https={ip}:{http_port};ftp={ip}:{http_port};socks={ip}:{socks_port}",
            "http=http://{ip}:{http_port};https=http://{ip}:{http_port}",
        };
    } // namespace Windows
} // namespace Preset
