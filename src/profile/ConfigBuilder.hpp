#pragma once

#include "ProxyEntity.hpp"

namespace NekoGui {
    class BuildConfigResult {
    public:
        QString error;
        QJsonObject coreConfig;

        QList<std::shared_ptr<ProxyEntity>> outboundStats; // all, but not including "bypass" "block"

        QList<std::shared_ptr<NekoGui_fmt::ExternalBuildResult>> extRs;
    };

    class BuildConfigStatus {
    public:
        std::shared_ptr<BuildConfigResult> result;
        std::shared_ptr<ProxyEntity> ent;
        bool forTest;
        bool forExport;

        // config format
        QJsonArray routingRules;
        QJsonArray inbounds;
        QJsonArray outbounds;
    };

    std::shared_ptr<BuildConfigResult> BuildConfig(const std::shared_ptr<ProxyEntity> &ent, bool forTest, bool forExport);

    void BuildConfigSingBox(const std::shared_ptr<BuildConfigStatus> &status);

    void BuildChain(const QString &proxyTag, const std::shared_ptr<BuildConfigStatus> &status);

    void BuildChainInternal(const QString &proxyTag, const QList<std::shared_ptr<ProxyEntity>> &ents, const std::shared_ptr<BuildConfigStatus> &status);
} // namespace NekoGui
