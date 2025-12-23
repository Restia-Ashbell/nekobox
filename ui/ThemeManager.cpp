#include <QStyle>
#include <QApplication>

#include "ThemeManager.hpp"
#include "main/NekoGui_Utils.hpp"

ThemeManager *themeManager = new ThemeManager;

void ThemeManager::ApplyTheme(const QString &theme) {
    bool ok = false;
    auto themeId = theme.toInt(&ok);

    if (ok) {
        QString path;
        switch (themeId) {
            case 0:
                path = ":/themes/feiyangqingyun/qss/flatgray.css";
                break;
            case 1:
                path = ":/themes/feiyangqingyun/qss/lightblue.css";
                break;
            case 2:
                path = ":/themes/feiyangqingyun/qss/blacksoft.css";
                break;
            default:
                return;
        }
        QString qss = ReadFileText(path);
        qss.replace(":/qss/", ":/themes/feiyangqingyun/qss/");

        qApp->setPalette(QPalette(qss.mid(20, 7)));
        qApp->setStyleSheet(qss);
    } else {
        qApp->setStyleSheet({});
        qApp->setPalette({});
        qApp->setStyle(theme);
    }
}
