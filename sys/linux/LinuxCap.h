#pragma once

#include <QString>

QString Linux_GetCapString(const QString &path);

bool Linux_Pkexec_SetCapString(const QString &path, const QString &cap);
