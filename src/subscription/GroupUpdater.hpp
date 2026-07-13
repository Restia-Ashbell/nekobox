#pragma once

#include "profile/ProfileManager.hpp"

namespace NekoGui_sub {
    class GroupUpdater {
    public:
        void AsyncUpdate(const QString &str, int _sub_gid = -1, const std::function<void()> &finish = nullptr);

        void Update(const QString &content, int _sub_gid = -1);

    private:
        QList<std::shared_ptr<NekoGui::ProxyEntity>> update(const QString &str);

        QList<std::shared_ptr<NekoGui::ProxyEntity>> updateJson(const QString &str, const QJsonObject &obj);

        QList<std::shared_ptr<NekoGui::ProxyEntity>> updateLink(const QString &str);

        QList<std::shared_ptr<NekoGui::ProxyEntity>> updateClash(const QString &str);

        void fixEnt(const std::shared_ptr<NekoGui::ProxyEntity> &ent);
    };

    extern GroupUpdater *groupUpdater;
} // namespace NekoGui_sub

// 更新所有订阅 关闭分组窗口时 更新动作继续执行
void UI_update_all_groups(bool isAutoUpdate);
