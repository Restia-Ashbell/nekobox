#pragma once

#include <QPixmap>

namespace Icon {

    enum TrayIconStatus {
        NONE,
        RUNNING,
        SYSTEM_PROXY,
        VPN,
    };

    QPixmap GetTrayIcon(TrayIconStatus status);

} // namespace Icon
