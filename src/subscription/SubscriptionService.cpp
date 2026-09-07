#include "SubscriptionService.hpp"

#include <QDateTime>
#include <QUrl>

#include "common/Utils.hpp"
#include "network/HttpRequestHelper.hpp"
#include "profile/DataStore.hpp"
#include "profile/ProfileFilter.hpp"
#include "profile/ProfileManager.hpp"
#include "subscription/SubscriptionParser.hpp"

namespace NekoGui_sub {

    SubscriptionService *subService = new SubscriptionService;

    bool SubscriptionService::isUpdating(int gid) const {
        return m_inflight.contains(gid);
    }

    void SubscriptionService::updateGroup(int gid) {
        auto group = NekoGui::profileManager->GetGroup(gid);
        if (group == nullptr || group->url.isEmpty() || group->archive) return;
        if (isUpdating(gid)) {
            MW_show_log(QObject::tr("The subscription is already being updated."));
            return;
        }

        m_inflight.insert(gid);
        emit taskStarted(gid);
        MW_show_log(">>>>>>>> " + QObject::tr("Requesting subscription: %1")
                                  .arg(group->name.isEmpty() ? group->url : group->name));
        startUpdate(gid);
    }

    void SubscriptionService::updateAll(bool autoUpdate) {
        int eligible = 0;
        int started = 0;
        for (auto gid: NekoGui::profileManager->groupsTabOrder) {
            auto g = NekoGui::profileManager->GetGroup(gid);
            if (g == nullptr || g->url.isEmpty() || g->archive) continue;
            if (autoUpdate && g->skip_auto_update) continue;
            eligible++;
            if (isUpdating(gid)) continue;

            m_inflight.insert(gid);
            emit taskStarted(gid);
            MW_show_log(">>>>>>>> " + QObject::tr("Requesting subscription: %1")
                                      .arg(g->name.isEmpty() ? g->url : g->name));
            startUpdate(gid);
            started++;
        }
        if (started == 0) {
            // 有符合条件的组但全部已在更新中：无需额外处理
            if (eligible > 0) MW_show_log(QObject::tr("The last subscription update has not exited."));
        }
    }

    void SubscriptionService::importText(const QString &raw) {
        auto content = raw.trimmed();
        if (content.isEmpty()) return;

        QUrl url(content);
        if (url.isValid() && (url.scheme() == "http" || url.scheme() == "https") && url.userInfo().isEmpty()) {
            emit importUrlDetected(content);
            return;
        }

        // 纯导入：无网络请求，直接在主线程解析并应用
        applyImport(content);
    }

    void SubscriptionService::resolveImport(const QString &text, bool asSubscription) {
        auto content = text.trimmed();
        if (content.isEmpty()) return;

        if (!asSubscription) {
            // 作为链接：纯导入文本
            applyImport(content);
            return;
        }

        // 作为订阅：新建一个分组，用粘贴的 URL 发起请求
        auto group = NekoGui::ProfileManager::NewGroup();
        group->url = content;
        NekoGui::profileManager->AddGroup(group);
        updateGroup(group->id);
    }

    void SubscriptionService::startUpdate(int gid) {
        auto group = NekoGui::profileManager->GetGroup(gid);
        if (group == nullptr) {
            finish(gid, {TaskStatus::Skipped, 0, 0, 0, QObject::tr("Group was already removed.")});
            return;
        }

        QUrl url(group->url);
        NekoGui_network::NetworkRequestHelper::HttpGet(url, [gid, this](const NekoGui_network::NekoHTTPResponse &resp) {
            onNetworkResponse(gid, resp);
        });
    }

    void SubscriptionService::onNetworkResponse(int gid, const NekoGui_network::NekoHTTPResponse &resp) {
        if (!isUpdating(gid)) return; // 已被移除（理论上不会发生）

        auto group = NekoGui::profileManager->GetGroup(gid);
        if (group == nullptr) {
            finish(gid, {TaskStatus::Skipped, 0, 0, 0, QObject::tr("Group was already removed.")});
            return;
        }

        // 网络错误：保留旧节点并记录失败
        if (!resp.error.isEmpty()) {
            auto report = UpdateReport{
                .status = TaskStatus::FailedNetwork,
                .message = QObject::tr("Requesting subscription %1 error: %2")
                               .arg(group->name, resp.error + "\n" + resp.data),
            };
            MW_show_log("<<<<<<<< " + report.message);
            group->Save();
            finish(gid, report);
            return;
        }

        // 成功：更新分组元信息（名称来自 Content-Disposition / info 解析自 Subscription-UserInfo）
        MW_show_log("<<<<<<<< " + QObject::tr("Subscription request finished: %1").arg(group->name));
        if (group->name.isEmpty()) {
            QString parsedName = SubscriptionParser::parseSubName(NekoGui_network::NetworkRequestHelper::GetHeader(resp.headers, "Content-Disposition"));
            group->name = parsedName.isEmpty() ? QUrl(group->url).host() : parsedName;
        }
        group->info = NekoGui_network::NetworkRequestHelper::GetHeader(resp.headers, "Subscription-UserInfo");

        applySubscription(gid, resp.data.trimmed());
    }

    void SubscriptionService::applySubscription(int gid, const QString &content) {
        // 解析在主线程执行；普通订阅规模解析开销可忽略
        QStringList diag;
        auto nodes = SubscriptionParser::update(content, &diag);
        for (const auto &line: diag) {
            MW_show_log(QObject::tr("Subscription parse: %1").arg(line));
        }

        auto report = UpdateReport{.status = TaskStatus::Succeeded, .total = (int)nodes.count()};
        auto group = NekoGui::profileManager->GetGroup(gid);
        if (group == nullptr || group->archive) {
            report.status = TaskStatus::Skipped;
            report.message = QObject::tr("Group was already removed.");
            finish(gid, report);
            return;
        }

        auto oldProfiles = group->Profiles();

        // 空结果守卫：已有节点时绝不应用空结果（防止远端异常把订阅清空）
        if (nodes.isEmpty() && !oldProfiles.isEmpty()) {
            report.status = TaskStatus::FailedEmpty;
            report.message = QObject::tr("Empty subscription response; existing profiles are kept.");
            MW_show_log("<<<<<<<< " + report.message);
            finish(gid, report);
            return;
        }

        auto [oldCommon, newCommon, oldOnly, newOnly] = NekoGui::ProfileFilter::Diff(oldProfiles, nodes);

        QString notice_added, notice_deleted;
        for (const auto &ent: newOnly) {
            if (NekoGui::profileManager->AddProfile(ent, gid)) {
                report.added++;
                notice_added += "[+] " + ent->bean->DisplayTypeAndName() + "\n";
            }
        }
        for (const auto &ent: oldOnly) {
            NekoGui::profileManager->DeleteProfile(ent->id);
            report.deleted++;
            notice_deleted += "[-] " + ent->bean->DisplayTypeAndName() + "\n";
        }

        // 按远端顺序重建分组 order
        group->order.clear();
        for (const auto &ent: nodes) {
            auto idx = newCommon.indexOf(ent);
            if (idx >= 0) {
                group->order.append(oldCommon[idx]->id);
            } else {
                group->order.append(ent->id);
            }
        }

        group->sub_last_update = QDateTime::currentSecsSinceEpoch();
        group->Save();

        QString change_text = report.added + report.deleted == 0
                                  ? QObject::tr("Nothing")
                                  : QObject::tr("Added %1 profiles:\n%2\nDeleted %3 profiles:\n%4")
                                        .arg(report.added)
                                        .arg(notice_added)
                                        .arg(report.deleted)
                                        .arg(notice_deleted);
        report.message = QObject::tr("Change of %1:").arg(group->name) + "\n" + change_text;
        MW_show_log("<<<<<<<< " + report.message);
        report.status = report.added + report.deleted == 0 ? TaskStatus::NoChange : TaskStatus::Succeeded;
        finish(gid, report);
    }

    void SubscriptionService::applyImport(const QString &content) {
        QStringList diag;
        auto nodes = SubscriptionParser::update(content, &diag);
        for (const auto &line: diag) {
            MW_show_log(QObject::tr("Subscription parse: %1").arg(line));
        }

        UpdateReport report;

        if (nodes.isEmpty()) {
            report.status = TaskStatus::FailedParse;
            report.message = QObject::tr("No usable profile found in the input.");
            MW_show_log("<<<<<<<< " + report.message);
        } else {
            report.status = TaskStatus::Succeeded;
            for (const auto &ent: nodes) {
                if (NekoGui::profileManager->AddProfile(ent, -1)) report.added++;
            }
            report.message = QObject::tr("Imported %1 profile(s)").arg(report.added);
            MW_show_log("<<<<<<<< " + report.message);
        }
        finish(-1, report);
    }

    void SubscriptionService::finish(int gid, const UpdateReport &report) {
        m_inflight.remove(gid);
        emit taskFinished(gid, report);
    }

} // namespace NekoGui_sub
