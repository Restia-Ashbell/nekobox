#pragma once

#include <QProcess>

namespace NekoGui_fmt {
    struct ExternalBuildResult;
}

namespace NekoGui_sys {
    class ExternalProcess : public QProcess {
    public:
        QString tag;

        ExternalProcess();
        ~ExternalProcess();

    protected:
        bool killed = false;
    };

    // Convert build results to external core processes.
    // Note: run and start in the same thread.
    std::list<std::shared_ptr<ExternalProcess>> CreateExtCFromExtR(
        const std::list<std::shared_ptr<NekoGui_fmt::ExternalBuildResult>> &extRs);
} // namespace NekoGui_sys