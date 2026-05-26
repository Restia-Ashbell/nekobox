#include <csignal>

#include <QApplication>
#include <QCryptographicHash>
#include <QDir>
#include <QFontDatabase>
#include <QLocalServer>
#include <QLocalSocket>
#include <QMessageBox>
#include <QStandardPaths>
#include <QTranslator>

#include "ui/mainwindow.h"

#ifdef Q_OS_WIN
#include "sys/windows/MiniDump.h"
#endif

void signal_handler(int signum) {
    if (qApp) {
        MainWindow::instance()->on_commitDataRequest();
        qApp->exit();
    }
}

int main(int argc, char *argv[]) {
    // Core dump
#ifdef Q_OS_WIN
    Windows_SetCrashHandler();
#endif

    // pre-init QApplication
    QApplication app(argc, argv);

    // RunGuard
    const QString APP_KEY = QCryptographicHash::hash(QApplication::applicationFilePath().toUtf8(), QCryptographicHash::Md5).toHex();
    QLocalSocket socket;
    socket.connectToServer(APP_KEY);
    if (socket.waitForConnected(100)) return 0;
    QLocalServer::removeServer(APP_KEY);
    QLocalServer server;
    server.listen(APP_KEY);
    QObject::connect(&server, &QLocalServer::newConnection, &app, [&] {
        auto socket = server.nextPendingConnection();
        socket->deleteLater();
        // raise main window
        MW_dialog_message("", "Raise");
    });

    // Flags
    NekoGui::dataStore->argv = QApplication::arguments();
    if (NekoGui::dataStore->argv.contains("-appdata")) {
        NekoGui::dataStore->flag_use_appdata = true;
        int appdataIndex = NekoGui::dataStore->argv.indexOf("-appdata");
        if (NekoGui::dataStore->argv.size() > appdataIndex + 1 && !NekoGui::dataStore->argv.at(appdataIndex + 1).startsWith("-")) {
            NekoGui::dataStore->appdataDir = NekoGui::dataStore->argv.at(appdataIndex + 1);
        }
    }
    if (NekoGui::dataStore->argv.contains("-tray")) NekoGui::dataStore->flag_tray = true;
    if (NekoGui::dataStore->argv.contains("-debug")) NekoGui::dataStore->flag_debug = true;
    if (NekoGui::dataStore->argv.contains("-flag_restart_tun_on")) NekoGui::dataStore->flag_restart_tun_on = true;
    if (NekoGui::dataStore->argv.contains("-flag_reorder")) NekoGui::dataStore->flag_reorder = true;
#ifdef NKR_CPP_USE_APPDATA
    NekoGui::dataStore->flag_use_appdata = true; // Example: Package & MacOS
#endif
#ifdef NKR_CPP_DEBUG
    NekoGui::dataStore->flag_debug = true;
#endif

    // dirs & clean
    QDir appDir(QApplication::applicationDirPath());
    if (NekoGui::dataStore->flag_use_appdata) {
        QApplication::setApplicationName("nekobox");
        if (!NekoGui::dataStore->appdataDir.isEmpty()) {
            appDir.setPath(NekoGui::dataStore->appdataDir);
        } else {
            appDir.setPath(QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation));
        }
    }
    if (!appDir.mkpath("config")) {
        QMessageBox::critical(nullptr, "Error", "Cannot create config directory:\n" + appDir.absoluteFilePath("config"));
        return 1;
    }
    QDir::setCurrent(appDir.absoluteFilePath("config"));
    QDir("temp").removeRecursively();
    for (const auto &dir: {"profiles", "groups", "routes"}) {
        QDir().mkdir(dir);
    }

    // Load dataStore
    NekoGui::dataStore->fn = "nekobox.json";
    if (!NekoGui::dataStore->Load()) NekoGui::dataStore->Save();

    // Datastore & Flags
    if (NekoGui::dataStore->start_minimal) NekoGui::dataStore->flag_tray = true;

    // load routing
    NekoGui::dataStore->routing = std::make_unique<NekoGui::Routing>();
    NekoGui::dataStore->routing->fn = "routes/" + NekoGui::dataStore->active_routing;
    if (!NekoGui::dataStore->routing->Load()) NekoGui::dataStore->routing->Save();

    // Translate
    if (NekoGui::dataStore->language.isEmpty()) NekoGui::dataStore->language = QLocale::system().name();
    QLocale::setDefault(QLocale(NekoGui::dataStore->language));
    QTranslator trans;
    if (trans.load(NekoGui::dataStore->language, ":/i18n")) QApplication::installTranslator(&trans);

    // Font
#ifdef Q_OS_WIN
    if (QSysInfo::productVersion().toInt() >= 11) { // win10似乎不支持该字体格式会将国旗显示为空白
        int fontId = QFontDatabase::addApplicationFont(":fonts/NotoColorEmoji-flagsonly.ttf");
        QFontDatabase::setApplicationEmojiFontFamilies(QFontDatabase::applicationFontFamilies(fontId));
    }
#endif
    if (!NekoGui::dataStore->font.isEmpty()) {
        QFont currentFont = QApplication::font();
        currentFont.setFamily(NekoGui::dataStore->font);
        QApplication::setFont(currentFont);
    }

    // Theme
    if (NekoGui::dataStore->theme.isEmpty()) NekoGui::dataStore->theme = QApplication::style()->name();
    QApplication::setStyle(NekoGui::dataStore->theme);

    // Signals
    signal(SIGTERM, signal_handler);
    signal(SIGINT, signal_handler);

    MainWindow w;
    return QApplication::exec();
}
