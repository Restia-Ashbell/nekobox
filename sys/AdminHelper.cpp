#include "AdminHelper.hpp"

#include <QApplication>
#include <QProcess>

#ifdef Q_OS_WIN
#include <shlobj.h>
#include "3rdparty/WinCommander.hpp"
#else
#ifdef Q_OS_LINUX
#include <sys/linux/LinuxCap.h>
#endif
#include <unistd.h>
#endif

bool isRunningAsAdmin() {
    bool admin = false;

#ifdef Q_OS_WIN
    admin = IsUserAnAdmin();
#else
#ifdef Q_OS_LINUX
    admin |= Linux_GetCapString(QApplication::applicationFilePath()).contains("cap_net_admin");
#endif
    admin |= geteuid() == 0;
#endif

    return admin;
}

void runAsAdmin(const QString &program, const QStringList &arguments) {
#ifdef Q_OS_WIN
    WinCommander::runProcessElevated(program, arguments, "", SW_NORMAL, false);
#else
    QProcess::startDetached(program, arguments);
#endif
}
