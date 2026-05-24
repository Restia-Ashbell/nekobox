#pragma once

#include <QStringList>

bool isRunAsAdmin();

bool runAsAdmin(const QString &program, const QStringList &arguments);
