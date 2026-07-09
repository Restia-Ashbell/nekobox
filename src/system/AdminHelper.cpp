#include "AdminHelper.hpp"

#include <QApplication>
#include <QProcess>

#ifdef Q_OS_WIN
#include <shlobj.h>

#include <shellapi.h>
#else
#include <unistd.h>
#endif

bool isRunAsAdmin() {
    bool admin = false;

#ifdef Q_OS_WIN
    admin = IsUserAnAdmin();
#else
#ifdef Q_OS_LINUX
    QProcess process;
    process.start("getcap", {QApplication::applicationFilePath()});
    admin |= process.waitForFinished() && process.readAllStandardOutput().contains("cap_net_admin");
#endif
    admin |= geteuid() == 0;
#endif

    return admin;
}

bool runAsAdmin(const QString &program, const QStringList &arguments) {
#ifdef Q_OS_WIN
    QString params = arguments.join(" ");
    HINSTANCE hInstance = ShellExecuteW(NULL, L"runas", (LPCWSTR) program.utf16(), (LPCWSTR) params.utf16(), NULL, SW_NORMAL);
    return (INT_PTR) hInstance > 32;
#endif
#ifdef Q_OS_MACOS
    QStringList cmdParts;
    cmdParts << QString("\"%1\"").arg(program);
    for (const QString &arg: arguments) {
        cmdParts << QString("\"%1\"").arg(arg);
    }
    QString cmd = QString("do shell script \"%1\" with administrator privileges").arg(cmdParts.join(" "));
    return QProcess::startDetached("osascript", QStringList() << "-e" << cmd);
#endif
#ifdef Q_OS_LINUX
    if (QProcess::execute("pkexec", {"setcap", "cap_net_admin=ep", QApplication::applicationFilePath()}) == 0) {
        return QProcess::startDetached(program, arguments);
    }
#endif
    return false;
}
