#include <QStyle>
#include <QApplication>

#include "ThemeManager.hpp"

ThemeManager *themeManager = new ThemeManager;

extern QString ReadFileText(const QString &path);

void ThemeManager::ApplyTheme(const QString &theme) {
    if (system_style_name.isEmpty()) {
        system_style_name = qApp->style()->name();
    }
    if (current_theme == theme) {
        return;
    }

    bool ok;
    auto themeId = theme.toInt(&ok);

    if (ok) {
        // System & Built-in
        QString qss;

        if (themeId == 0) {
            // system theme
            qApp->setStyleSheet("");
            qApp->setPalette(QPalette());
            qApp->setStyle(system_style_name);
        } else {
            QString path;
            switch (themeId) {
                case 1:
                    path = ":/themes/feiyangqingyun/qss/flatgray.css";
                    break;
                case 2:
                    path = ":/themes/feiyangqingyun/qss/lightblue.css";
                    break;
                case 3:
                    path = ":/themes/feiyangqingyun/qss/blacksoft.css";
                    break;
                default:
                    return;
            }
            qss = ReadFileText(path);
            qss.replace(":/qss/", ":/themes/feiyangqingyun/qss/");

            QString paletteColor = qss.mid(20, 7);
            qApp->setPalette(QPalette(paletteColor));
            qApp->setStyleSheet(qss);
        }
    } else {
        qApp->setStyleSheet("");
        qApp->setPalette(QPalette());
        qApp->setStyle(theme);
    }

    current_theme = theme;

    auto nekoray_css = ReadFileText(":/neko/neko.css");
    qApp->setStyleSheet(qApp->styleSheet().append("\n").append(nekoray_css));
}
