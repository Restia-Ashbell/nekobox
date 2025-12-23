#pragma once

#include <QProcess>

namespace NekoGui_sys {
    class ExternalProcess : public QProcess {
    public:
        QString tag;
        QString program;
        QStringList arguments;
        QStringList env;

        bool managed = true; // MW_dialog_message

        ExternalProcess();
        ~ExternalProcess();

        // start & kill is one time

        virtual void Start();

        void Kill();

    protected:
        bool started = false;
        bool killed = false;
        bool crashed = false;
    };

    // 手动管理
    inline std::list<std::shared_ptr<ExternalProcess>> running_ext;
} // namespace NekoGui_sys
