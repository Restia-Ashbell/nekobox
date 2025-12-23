#include "ExternalProcess.hpp"
#include "main/NekoGui.hpp"

namespace NekoGui_sys {

    ExternalProcess::ExternalProcess() : QProcess() {
        // qDebug() << "[Debug] ExternalProcess()" << this << running_ext;
        this->env = QProcessEnvironment::systemEnvironment().toStringList();
    }

    ExternalProcess::~ExternalProcess() {
        // qDebug() << "[Debug] ~ExternalProcess()" << this << running_ext;
    }

    void ExternalProcess::Start() {
        if (started) return;
        started = true;

        if (managed) {
            connect(this, &QProcess::readyReadStandardOutput, this, [&]() {
                MW_show_log_ext_vt100(readAllStandardOutput());
            });
            connect(this, &QProcess::readyReadStandardError, this, [&]() {
                MW_show_log_ext_vt100(readAllStandardError());
            });
            connect(this, &QProcess::errorOccurred, this, [&](QProcess::ProcessError error) {
                if (!killed) {
                    crashed = true;
                    MW_show_log_ext(tag, "errorOccurred:" + errorString());
                    MW_dialog_message("ExternalProcess", "Crashed");
                }
            });
            connect(this, &QProcess::stateChanged, this, [&](QProcess::ProcessState state) {
                if (state == QProcess::NotRunning) {
                    if (killed) { // 用户命令退出
                        MW_show_log_ext(tag, "External core stopped");
                    } else if (!crashed) { // 异常退出
                        crashed = true;
                        MW_show_log_ext(tag, "[Error] Program exited accidentally: " + errorString());
                        Kill();
                        MW_dialog_message("ExternalProcess", "Crashed");
                    }
                }
            });
            MW_show_log_ext(tag, "External core starting: " + env.join(" ") + " " + program + " " + arguments.join(" "));
        }

        QProcess::setEnvironment(env);
        QProcess::start(program, arguments);
    }

    void ExternalProcess::Kill() {
        if (killed) return;
        killed = true;

        if (!crashed) {
            QProcess::kill();
            QProcess::waitForFinished(500);
        }
    }

} // namespace NekoGui_sys
