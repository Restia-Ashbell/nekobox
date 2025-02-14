#pragma once

#include "Const.hpp"
#include "NekoGui_Utils.hpp"
#include "NekoGui_ConfigItem.hpp"
#include "NekoGui_DataStore.hpp"

// Switch core support

namespace NekoGui {
    QString FindCoreAsset(const QString &name);

    QString FindNekoBoxCoreRealPath();

    bool IsAdmin();
} // namespace NekoGui
