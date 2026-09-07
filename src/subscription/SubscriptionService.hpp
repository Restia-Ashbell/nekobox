#pragma once

#include <QSet>
#include <QString>

#include "profile/ProxyEntity.hpp"

namespace NekoGui_network {
    struct NekoHTTPResponse;
}

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
        void updateGroup(int gid);

        void updateAll(bool autoUpdate);

        void importText(const QString &raw);

        void resolveImport(const QString &text, bool asSubscription);

        bool isUpdating(int gid) const;

    signals:
        void taskStarted(int gid);

        void taskFinished(int gid, const NekoGui_sub::UpdateReport &report);

        void importUrlDetected(const QString &url); // 由 UI 弹对话框，再调 resolveImport

    private:
        QSet<int> m_inflight;

        void startUpdate(int gid);

        void onNetworkResponse(int gid, const NekoGui_network::NekoHTTPResponse &resp);

        void applySubscription(int gid, const QString &content);

        void applyImport(const QString &content);

        void finish(int gid, const UpdateReport &report);
    };

    extern SubscriptionService *subService;

} // namespace NekoGui_sub
