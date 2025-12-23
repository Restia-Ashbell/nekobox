#pragma once

#include <QStringList>

bool isRunningAsAdmin();

void runAsAdmin(const QString &program, const QStringList &arguments);
