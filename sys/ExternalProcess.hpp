#pragma once

#include <QProcess>

namespace NekoGui_sys {
    class ExternalProcess : public QProcess {
    public:
        QString tag;

        ExternalProcess();
        ~ExternalProcess();

    protected:
        bool killed = false;
    };
} // namespace NekoGui_sys
