#include "ExternalProcess.hpp"

#include "main/NekoGui_DataStore.hpp"

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

} // namespace NekoGui_sys
