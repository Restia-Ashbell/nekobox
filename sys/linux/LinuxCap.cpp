#include "LinuxCap.h"

#include <QProcess>

QString Linux_GetCapString(const QString &path) {
    QProcess p;
    p.setProgram("getcap");
    p.setArguments({path});
    p.start();
    p.waitForFinished(500);
    return p.readAllStandardOutput();
}

bool Linux_Pkexec_SetCapString(const QString &path, const QString &cap) {
    QProcess p;
    p.setProgram("pkexec");
    p.setArguments({"setcap", cap, path});
    p.start();
    return p.waitForFinished() && p.exitCode() == 0;
}
