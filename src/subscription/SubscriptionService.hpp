#pragma once

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QSet>
#include <QString>
#include <QTimer>

#include "profile/Group.hpp"
#include "profile/ProxyEntity.hpp"

namespace NekoGui_sub {

    enum class TaskStatus {
        Succeeded,
        NoChange,
        FailedNetwork,
        FailedParse,
        FailedEmpty,
        Skipped
    };

    struct UpdateReport {
        TaskStatus status = TaskStatus::Succeeded;
        int added = 0;
        int deleted = 0;
        int total = 0;
        QString message;
    };

    class SubscriptionService : public QObject {
        Q_OBJECT

    public:
        SubscriptionService();

        void updateGroup(int gid);

        void updateAll();

        void startAutoUpdate();

        void importText(const QString &raw);

        void resolveImport(const QString &text, bool asSubscription);

        bool isUpdating(int gid) const;

        QNetworkAccessManager networkManager;

    signals:
        void updateStarted(int gid);

        void updateFinished(int gid, const NekoGui_sub::UpdateReport &report);

        void importUrlDetected(const QString &url); // 由 UI 弹对话框，再调 resolveImport

    private:
        QSet<int> m_inflight;
        QTimer m_autoUpdateTimer;

        void startUpdate(int gid, const std::shared_ptr<NekoGui::Group> &group);

        void checkAutoUpdate();

        void sendUpdateRequest(int gid);

        void onUpdateReply(int gid, QNetworkReply *reply);

        void applySubscription(int gid, const QString &content);

        void applyImport(const QString &content);

        void finish(int gid, const UpdateReport &report);
    };

    extern SubscriptionService *subService;

} // namespace NekoGui_sub
