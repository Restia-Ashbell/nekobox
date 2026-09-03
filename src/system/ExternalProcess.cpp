#include "ExternalProcess.hpp"

#include "profile/DataStore.hpp"
#include "protocol/AbstractBean.hpp"

namespace NekoGui_sys {

    ExternalProcess::ExternalProcess() : QProcess() {
        // qDebug() << "[Debug] ExternalProcess()" << this << running_ext;
        setProcessChannelMode(QProcess::MergedChannels);
        connect(this, &QProcess::readyRead, this, [this] {
            MW_show_log_ext_vt100(readAll());
        });
        connect(this, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
            if (!killed) {
                MW_show_log_ext(tag, "errorOccurred:" + errorString());
                if (QThread::currentThread() == qApp->thread()) {
                    MW_dialog_message("ExternalProcess", "Crashed");
                }
            }
        });
        connect(this, &QProcess::stateChanged, this, [this](QProcess::ProcessState state) {
            if (state == QProcess::Starting) {
                MW_show_log_ext(tag, QString("External core starting: %1 %2").arg(program(), arguments().join(" ")));
            } else if (state == QProcess::NotRunning) {
                if (killed) { // 用户命令退出
                    MW_show_log_ext(tag, "External core stopped");
                } else { // 异常退出
                    MW_show_log_ext(tag, "Program exited accidentally");
                    if (QThread::currentThread() == qApp->thread()) {
                        MW_dialog_message("ExternalProcess", "Crashed");
                    }
                }
            }
        });
    }

    ExternalProcess::~ExternalProcess() {
        // qDebug() << "[Debug] ~ExternalProcess()" << this << running_ext;
        if (state() != QProcess::NotRunning) {
            killed = true;
            kill();
            waitForFinished();
        }
    }

    std::list<std::shared_ptr<ExternalProcess>> CreateExtCFromExtR(const std::list<std::shared_ptr<NekoGui_fmt::ExternalBuildResult>> &extRs) {
        std::list<std::shared_ptr<ExternalProcess>> processes;
        for (const auto &extR: extRs) {
            auto extC = std::make_shared<ExternalProcess>();
            extC->tag = extR->tag;
            extC->setProgram(extR->program);
            extC->setArguments(extR->arguments);
            extC->setEnvironment(QProcess::systemEnvironment() + extR->env);
            processes.emplace_back(extC);
        }
        return processes;
    }

} // namespace NekoGui_sys
