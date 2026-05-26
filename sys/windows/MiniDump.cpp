#include "MiniDump.h"

#include <windows.h>

#include <dbghelp.h>

#include <QApplication>
#include <QDateTime>
#include <QDir>
#include <QMessageBox>

LONG WINAPI CrashHandler(EXCEPTION_POINTERS *pException) {
    QDir::setCurrent(QApplication::applicationDirPath());

    QString timestamp = QDateTime::currentDateTime().toString("yyyy_MM_dd_hh_mm_ss");
    QString dumpFile = QString("crash_%1.dmp").arg(timestamp);
    EXCEPTION_RECORD *record = pException->ExceptionRecord;
    QString message = QString("ErrorCode: 0x%1 ErrorAddr:0x%2 ErrorFlag: 0x%3 ErrorPara: 0x%4\nVersion: %5\n")
                          .arg(record->ExceptionCode, 0, 16)
                          .arg((quintptr) record->ExceptionAddress, 0, 16)
                          .arg(record->ExceptionFlags, 0, 16)
                          .arg(record->NumberParameters, 0, 16)
                          .arg(NKR_VERSION);

    HANDLE hFile = CreateFile((LPCWSTR) dumpFile.utf16(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile != INVALID_HANDLE_VALUE) {
        MINIDUMP_EXCEPTION_INFORMATION info;
        info.ThreadId = GetCurrentThreadId();
        info.ExceptionPointers = pException;
        info.ClientPointers = FALSE;
        // 将dump信息写入dmp文件
        if (MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), hFile, MiniDumpNormal, &info, NULL, NULL)) {
            message += QString("Dump file saved at: %1").arg(dumpFile);
        }
        CloseHandle(hFile);
    }
    // 创建消息提示
    QMessageBox::critical(NULL, "Application crashed", message);
    return EXCEPTION_EXECUTE_HANDLER;
}

void Windows_SetCrashHandler() {
    SetErrorMode(SEM_FAILCRITICALERRORS);
    SetUnhandledExceptionFilter(CrashHandler);
}
