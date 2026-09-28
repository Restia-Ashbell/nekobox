#pragma once

#include <QList>
#include <QString>

#include <yaml-cpp/yaml.h>

#include "profile/ProxyEntity.hpp"

namespace NekoGui_sub {

    class SubscriptionParser {
    public:
        static QString parseSubInfo(const QString &info);

        static QString parseSubName(const QString &contentDisposition);

        static QList<std::shared_ptr<NekoGui::ProxyEntity>> update(const QString &str, QStringList *diag = nullptr);

    private:
        static QList<std::shared_ptr<NekoGui::ProxyEntity>> updateJson(const QString &str);

        static QList<std::shared_ptr<NekoGui::ProxyEntity>> updateLink(const QString &str, QStringList *diag);

        static QList<std::shared_ptr<NekoGui::ProxyEntity>> updateClash(const YAML::Node &proxies, QStringList *diag);

        static void fixEnt(const std::shared_ptr<NekoGui::ProxyEntity> &ent);
    };

} // namespace NekoGui_sub
