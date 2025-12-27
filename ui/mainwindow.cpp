#include "./ui_mainwindow.h"
#include "mainwindow.h"

#include <QClipboard>
#include <QLabel>
#include <QTextBlock>
#include <QScrollBar>
#include <QScreen>
#include <QDesktopServices>
#include <QInputDialog>
#include <QThread>
#include <QTimer>
#include <QMessageBox>
#include <QDir>
#include <QFileInfo>
#include <QPlainTextEdit>
#include <QHeaderView>
#include <QHotkey>

#include "ZxingQtReader.h"
#include "MultiFormatWriter.h"
#include "BarcodeFormat.h"
#include "BitMatrix.h"

#include "libbox.h"

#include "fmt/Preset.hpp"
#include "db/ProfileFilter.hpp"
#include "db/ConfigBuilder.hpp"
#include "db/traffic/TrafficLooper.hpp"
#include "sub/GroupUpdater.hpp"
#include "sys/ExternalProcess.hpp"
#include "sys/AutoRun.hpp"
#include "sys/AdminHelper.hpp"

#include "ui/ThemeManager.hpp"
#include "ui/Icon.hpp"
#include "ui/edit/dialog_edit_group.h"
#include "ui/edit/dialog_edit_profile.h"
#include "ui/dialog_basic_settings.h"
#include "ui/dialog_manage_groups.h"
#include "ui/dialog_manage_routes.h"
#include "ui/dialog_hotkey.h"

#include "3rdparty/VT100Parser.hpp"
#include "3rdparty/qv2ray/v2/components/proxy/QvProxyConfigurator.hpp"
#include "3rdparty/qv2ray/v2/ui/LogHighlighter.hpp"

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow) {
    mainwindow = this;
    MW_dialog_message = [this](const QString &a, const QString &b) {
        runOnUiThread([=, this] { dialog_message_impl(a, b); });
    };

    // Load Manager
    NekoGui::profileManager->LoadManager();

    // Setup misc UI
    if (NekoGui::dataStore->theme.isEmpty()) NekoGui::dataStore->theme = QApplication::style()->name();
    themeManager->ApplyTheme(NekoGui::dataStore->theme);
    ui->setupUi(this);
    //
    connect(ui->menu_start, &QAction::triggered, this, [=, this] { neko_start(); });
    connect(ui->menu_stop, &QAction::triggered, this, [=, this] { neko_stop(); });
    connect(ui->tabWidget->tabBar(), &QTabBar::tabMoved, this, [=, this](int from, int to) {
        // use tabData to track tab & gid
        NekoGui::profileManager->groupsTabOrder.clear();
        for (int i = 0; i < ui->tabWidget->tabBar()->count(); i++) {
            NekoGui::profileManager->groupsTabOrder += ui->tabWidget->tabBar()->tabData(i).toInt();
        }
        NekoGui::profileManager->SaveManager();
    });
    ui->label_running->installEventFilter(this);
    //
    RegisterHotkey(false);
    //
    auto last_size = NekoGui::dataStore->mw_size.split("x");
    if (last_size.length() == 2) {
        auto w = last_size[0].toInt();
        auto h = last_size[1].toInt();
        if (w > 0 && h > 0) {
            resize(w, h);
        }
    }

    // top bar
    ui->toolButton_program->setMenu(ui->menu_program);
    ui->toolButton_preferences->setMenu(ui->menu_preferences);
    ui->toolButton_server->setMenu(ui->menu_server);
    ui->menubar->setVisible(false);
    connect(ui->toolButton_dashboard, &QToolButton::clicked, this, [=, this] {
        if (!NekoGui::dataStore->clash_api_external_controller.isEmpty() && NekoGui::dataStore->started_id >= 0) {
            QDesktopServices::openUrl(QUrl("http://" + NekoGui::dataStore->clash_api_external_controller));
        } else {
            QMessageBox::warning(this, tr("Unable to Open Dashboard"), tr("Please configure the Clash API and start first."));
        }
    });
    connect(ui->toolButton_document, &QToolButton::clicked, this, [=, this] { QDesktopServices::openUrl(QUrl("https://matsuridayo.github.io/")); });
    connect(ui->toolButton_update, &QToolButton::clicked, this, [=, this] { runOnNewThread([=, this] { CheckUpdate(); }); });

    // Setup log UI
    ui->splitter->restoreState(DecodeB64IfValid(NekoGui::dataStore->splitter_state));
    auto *doc = ui->masterLogBrowser->document();
    doc->setMaximumBlockCount(NekoGui::dataStore->max_log_line);
    new SyntaxHighlighter(false, doc);
    ui->masterLogBrowser->setUndoRedoEnabled(false);
    ui->masterLogBrowser->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    auto *vBar = ui->masterLogBrowser->verticalScrollBar();
    connect(vBar, &QScrollBar::valueChanged, this, [vBar, this](int value) {
        qvLogAutoScoll = vBar->maximum() == value;
    });
    connect(ui->masterLogBrowser, &QTextBrowser::textChanged, this, [vBar, this]() {
        if (qvLogAutoScoll) vBar->setValue(vBar->maximum());
    });
    MW_show_log = [this](const QString &log) {
        runOnUiThread([=, this] { show_log_impl(log); });
    };
    MW_show_log_ext = [this](const QString &tag, const QString &log) {
        runOnUiThread([=, this] { show_log_impl("[" + tag + "] " + log); });
    };
    MW_show_log_ext_vt100 = [this](const QString &log) {
        runOnUiThread([=, this] { show_log_impl(cleanVT100String(log)); });
    };

    // search box
    ui->search->setVisible(false);
    connect(shortcut_ctrl_f, &QShortcut::activated, this, [=, this] {
        ui->search->setVisible(true);
        ui->search->setFocus();
    });
    connect(shortcut_esc, &QShortcut::activated, this, [=, this] {
        if (ui->search->isVisible()) {
            ui->search->setText("");
            ui->search->textChanged("");
            ui->search->setVisible(false);
        }
        if (select_mode) {
            emit profile_selected(-1);
            select_mode = false;
            refresh_status();
        }
    });
    connect(ui->search, &QLineEdit::textChanged, this, [this](const QString &text) {
        for (int index = 0; index < ui->tabWidget->count(); ++index) {
            auto proxyListTable = qobject_cast<QTableWidget *>(ui->tabWidget->widget(index));
            for (int row = 0; row < proxyListTable->rowCount(); ++row) {
                proxyListTable->setRowHidden(row, !text.isEmpty());
            }
            for (auto *item: proxyListTable->findItems(text, Qt::MatchContains)) {
                if (item) proxyListTable->setRowHidden(item->row(), false);
            }
        }
    });

    // refresh
    refresh_groups();

    // Setup Tray
    tray = new QSystemTrayIcon(this); // 初始化托盘对象tray
    tray->setIcon(Icon::GetTrayIcon(Icon::NONE));
    tray->setContextMenu(ui->menu_program); // 创建托盘菜单
    tray->show();                           // 让托盘图标显示在系统托盘上
    connect(tray, &QSystemTrayIcon::activated, this, [this](QSystemTrayIcon::ActivationReason reason) {
        if (reason == QSystemTrayIcon::Trigger) {
            !isVisible() || isMinimized() ? ActivateWindow(this) : hide();
        }
    });

    // Misc menu
    connect(ui->menu_open_config_folder, &QAction::triggered, this, [=, this] { QDesktopServices::openUrl(QUrl::fromLocalFile(QDir::currentPath())); });
    ui->menu_program_preference->addActions(ui->menu_preferences->actions());
    connect(ui->actionRestart_Proxy, &QAction::triggered, this, [=, this] { if (NekoGui::dataStore->started_id>=0) neko_start(NekoGui::dataStore->started_id); });
    connect(ui->actionRestart_Program, &QAction::triggered, this, [=, this] { MW_dialog_message("", "RestartProgram"); });
    connect(ui->actionShow_window, &QAction::triggered, this, [=, this] { tray->activated(QSystemTrayIcon::ActivationReason::Trigger); });
    //
    connect(ui->menu_program, &QMenu::aboutToShow, this, [=, this]() {
        ui->actionRemember_last_proxy->setChecked(NekoGui::dataStore->remember_enable);
        ui->actionStart_with_system->setChecked(AutoRun_IsEnabled());
        ui->actionAllow_LAN->setChecked(QStringList{"::", "0.0.0.0"}.contains(NekoGui::dataStore->inbound_address));
        // active server
        for (const auto &old: ui->menuActive_Server->actions()) {
            ui->menuActive_Server->removeAction(old);
            old->deleteLater();
        }
        int active_server_item_count = 0;
        for (const auto &pf: NekoGui::profileManager->CurrentGroup()->ProfilesWithOrder()) {
            auto a = new QAction(pf->bean->DisplayTypeAndName(), this);
            a->setProperty("id", pf->id);
            a->setCheckable(true);
            if (NekoGui::dataStore->started_id == pf->id) a->setChecked(true);
            ui->menuActive_Server->addAction(a);
            if (++active_server_item_count == 100) break;
        }
        // active routing
        for (const auto &old: ui->menuActive_Routing->actions()) {
            ui->menuActive_Routing->removeAction(old);
            old->deleteLater();
        }
        for (const auto &name: NekoGui::Routing::List()) {
            auto a = new QAction(name, this);
            a->setCheckable(true);
            a->setChecked(name == NekoGui::dataStore->active_routing);
            ui->menuActive_Routing->addAction(a);
        }
    });
    connect(ui->menuActive_Server, &QMenu::triggered, this, [=, this](QAction *a) {
        bool ok;
        auto id = a->property("id").toInt(&ok);
        if (!ok) return;
        if (NekoGui::dataStore->started_id == id) {
            neko_stop();
        } else {
            neko_start(id);
        }
    });
    connect(ui->menuActive_Routing, &QMenu::triggered, this, [=, this](QAction *a) {
        auto fn = a->text();
        if (!fn.isEmpty()) {
            NekoGui::Routing r;
            r.load_control_must = true;
            r.fn = "routes/" + fn;
            if (r.Load()) {
                if (QMessageBox::question(GetMessageBoxParent(), software_name, tr("Load routing and apply: %1").arg(fn) + "\n" + r.DisplayRouting()) == QMessageBox::Yes) {
                    NekoGui::Routing::SetToActive(fn);
                    if (NekoGui::dataStore->started_id >= 0) {
                        neko_start(NekoGui::dataStore->started_id);
                    } else {
                        refresh_status();
                    }
                }
            }
        }
    });
    connect(ui->actionRemember_last_proxy, &QAction::triggered, this, [=, this](bool checked) {
        NekoGui::dataStore->remember_enable = checked;
        NekoGui::dataStore->Save();
    });
    connect(ui->actionStart_with_system, &QAction::triggered, this, [=, this](bool checked) {
        AutoRun_SetEnabled(checked);
    });
    connect(ui->actionAllow_LAN, &QAction::triggered, this, [=, this](bool checked) {
        NekoGui::dataStore->inbound_address = checked ? "::" : "127.0.0.1";
        MW_dialog_message("", "UpdateDataStore");
    });
    //
    connect(ui->checkBox_VPN, &QCheckBox::clicked, this, [=, this](bool checked) { neko_set_spmode_vpn(checked); });
    connect(ui->checkBox_SystemProxy, &QCheckBox::clicked, this, [=, this](bool checked) { neko_set_spmode_system_proxy(checked); });
    connect(ui->menu_spmode, &QMenu::aboutToShow, this, [=, this]() {
        ui->menu_spmode_disabled->setChecked(!(NekoGui::dataStore->spmode_system_proxy || NekoGui::dataStore->spmode_vpn));
        ui->menu_spmode_system_proxy->setChecked(NekoGui::dataStore->spmode_system_proxy);
        ui->menu_spmode_vpn->setChecked(NekoGui::dataStore->spmode_vpn);
    });
    connect(ui->menu_spmode_system_proxy, &QAction::triggered, this, [=, this](bool checked) { neko_set_spmode_system_proxy(checked); });
    connect(ui->menu_spmode_vpn, &QAction::triggered, this, [=, this](bool checked) { neko_set_spmode_vpn(checked); });
    connect(ui->menu_spmode_disabled, &QAction::triggered, this, [=, this]() {
        neko_set_spmode_system_proxy(false);
        neko_set_spmode_vpn(false);
    });
    connect(ui->menu_qr, &QAction::triggered, this, [=, this]() { display_qr_link(false); });
    connect(ui->menu_tcp_ping, &QAction::triggered, this, [=, this]() { speedtest_current_group(0); });
    connect(ui->menu_url_test, &QAction::triggered, this, [=, this]() { speedtest_current_group(1); });
    connect(ui->menu_full_test, &QAction::triggered, this, [=, this]() { speedtest_current_group(999); });
    connect(ui->menu_stop_testing, &QAction::triggered, this, [=, this]() { speedtestFuture.cancel(); });
    //
    auto set_selected_or_group = [=, this](int mode) {
        // 0=group 1=select 2=unknown(menu is hide)
        ui->menu_server->setProperty("selected_or_group", mode);
    };
    auto move_tests_to_menu = [=, this](bool menuCurrent_Select) {
        return [=, this] {
            if (menuCurrent_Select) {
                ui->menuCurrent_Select->insertAction(ui->actionfake_4, ui->menu_tcp_ping);
                ui->menuCurrent_Select->insertAction(ui->actionfake_4, ui->menu_url_test);
                ui->menuCurrent_Select->insertAction(ui->actionfake_4, ui->menu_full_test);
                ui->menuCurrent_Select->insertAction(ui->actionfake_4, ui->menu_stop_testing);
                ui->menuCurrent_Select->insertAction(ui->actionfake_4, ui->menu_clear_test_result);
                ui->menuCurrent_Select->insertAction(ui->actionfake_4, ui->menu_resolve_domain);
            } else {
                ui->menuCurrent_Group->insertAction(ui->actionfake_5, ui->menu_tcp_ping);
                ui->menuCurrent_Group->insertAction(ui->actionfake_5, ui->menu_url_test);
                ui->menuCurrent_Group->insertAction(ui->actionfake_5, ui->menu_full_test);
                ui->menuCurrent_Group->insertAction(ui->actionfake_5, ui->menu_stop_testing);
                ui->menuCurrent_Group->insertAction(ui->actionfake_5, ui->menu_clear_test_result);
                ui->menuCurrent_Group->insertAction(ui->actionfake_5, ui->menu_resolve_domain);
            }
            set_selected_or_group(menuCurrent_Select ? 1 : 0);
        };
    };
    connect(ui->menuCurrent_Select, &QMenu::aboutToShow, this, move_tests_to_menu(true));
    connect(ui->menuCurrent_Group, &QMenu::aboutToShow, this, move_tests_to_menu(false));
    connect(ui->menu_server, &QMenu::aboutToHide, this, [=, this] {
        setTimeout([=, this] { set_selected_or_group(2); }, this, 200);
    });
    set_selected_or_group(2);
    //
    connect(ui->menu_share_item, &QMenu::aboutToShow, this, [=, this] {
        QString name;
        auto selected = get_now_selected_list();
        if (!selected.isEmpty()) {
            auto ent = selected.first();
            name = ent->bean->DisplayCoreType();
        }
        ui->menu_export_config->setVisible(name == software_core_name);
        ui->menu_export_config->setText(tr("Export %1 config").arg(name));
    });
    refresh_status();

    BoxMain([](const char *log) { MW_show_log(log); });
    runOnNewThread([=, this] { NekoGui_traffic::trafficLooper->Loop(); });

    // Remember system proxy
    if (NekoGui::dataStore->remember_enable || NekoGui::dataStore->flag_restart_tun_on) {
        if (NekoGui::dataStore->remember_spmode.contains("system_proxy")) {
            neko_set_spmode_system_proxy(true, false);
        }
        if (NekoGui::dataStore->remember_spmode.contains("vpn") || NekoGui::dataStore->flag_restart_tun_on) {
            neko_set_spmode_vpn(true, false);
        }
    }

    connect(qApp, &QGuiApplication::commitDataRequest, this, &MainWindow::on_commitDataRequest);

    auto t = new QTimer(this);
    connect(t, &QTimer::timeout, this, [=, this]() { refresh_status(); });
    t->start(2000);

    autoUpdateSubscriptionTimer = new QTimer(this);
    connect(autoUpdateSubscriptionTimer, &QTimer::timeout, this, [this] { UI_update_all_groups(true); });
    resetAutoUpdateSubscription(NekoGui::dataStore->sub_auto_update);

    if (!NekoGui::dataStore->flag_tray) show();
}

void MainWindow::closeEvent(QCloseEvent *event) {
    if (tray->isVisible()) {
        hide();          // 隐藏窗口
        event->ignore(); // 忽略事件
    }
}

MainWindow::~MainWindow() {
    delete ui;
}

// callback

void MainWindow::dialog_message_impl(const QString &sender, const QString &info) {
    // info
    if (info.contains("UpdateIcon")) {
        icon_status = -1;
        refresh_status();
    }
    if (info.contains("UpdateDataStore")) {
        auto suggestRestartProxy = NekoGui::dataStore->Save();
        if (info.contains("RouteChanged")) {
            suggestRestartProxy = true;
        }
        if (info.contains("NeedRestart")) {
            suggestRestartProxy = false;
        }
        if (info.contains("VPNChanged") && NekoGui::dataStore->spmode_vpn) {
            MessageBoxWarning(tr("Tun Settings changed"), tr("Restart Tun to take effect."));
        }
        if (suggestRestartProxy && NekoGui::dataStore->started_id >= 0 &&
            QMessageBox::question(GetMessageBoxParent(), tr("Confirmation"), tr("Settings changed, restart proxy?")) == QMessageBox::StandardButton::Yes) {
            neko_start(NekoGui::dataStore->started_id);
        }
        refresh_status();
    }
    if (info.contains("NeedRestart")) {
        auto n = QMessageBox::warning(GetMessageBoxParent(), tr("Settings changed"), tr("Restart the program to take effect."), QMessageBox::Yes | QMessageBox::No);
        if (n == QMessageBox::Yes) {
            this->exit_reason = 2;
            on_menu_exit_triggered();
        }
    }
    //
    if (info == "RestartProgram") {
        this->exit_reason = 2;
        on_menu_exit_triggered();
    } else if (info == "Raise") {
        ActivateWindow(this);
    }
    if (info == "NeedAdmin") {
        get_elevated_permissions();
    }
    // sender
    if (sender == Dialog_DialogEditProfile) {
        auto msg = info.split(",");
        if (msg.contains("accept")) {
            refresh_group();
            if (msg.contains("restart")) {
                if (QMessageBox::question(GetMessageBoxParent(), tr("Confirmation"), tr("Settings changed, restart proxy?")) == QMessageBox::StandardButton::Yes) {
                    neko_start(NekoGui::dataStore->started_id);
                }
            }
        }
    } else if (sender == Dialog_DialogManageGroups) {
        if (info.startsWith("refresh")) {
            refresh_groups();
        }
    } else if (sender == "SubUpdater") {
        if (info.startsWith("finish")) {
            if (!info.contains("dingyue")) {
                show_log_impl(tr("Imported %1 profile(s)").arg(NekoGui::dataStore->imported_count));
            }
        } else if (info == "NewGroup") {
            refresh_groups();
        }
    } else if (sender == "ExternalProcess") {
        if (info == "Crashed") {
            neko_stop();
        } else if (info == "CoreCrashed") {
            neko_stop(true);
        } else if (info.startsWith("CoreStarted")) {
            neko_start(info.split(",")[1].toInt());
        }
    }
}

// top bar & tray menu

template<typename DialogType, typename... Args>
void MainWindow::openDialog(Args &&...args) {
    if (dialog_is_using) return;
    dialog_is_using = true;
    auto dialog = new DialogType(this, std::forward<Args>(args)...);
    connect(dialog, &QDialog::finished, this, [this, dialog] {
        dialog->deleteLater();
        dialog_is_using = false;
    });
    dialog->show();
}

void MainWindow::on_menu_basic_settings_triggered() {
    openDialog<DialogBasicSettings>();
}

void MainWindow::on_menu_manage_groups_triggered() {
    openDialog<DialogManageGroups>(ui->tabWidget->currentIndex());
}

void MainWindow::on_menu_routing_settings_triggered() {
    openDialog<DialogManageRoutes>();
}

void MainWindow::on_menu_hotkey_settings_triggered() {
    openDialog<DialogHotkey>();
}

void MainWindow::on_commitDataRequest() {
    qDebug() << "Start of data save";
    //
    if (!isMaximized()) {
        auto olds = NekoGui::dataStore->mw_size;
        auto news = QString("%1x%2").arg(size().width()).arg(size().height());
        if (olds != news) {
            NekoGui::dataStore->mw_size = news;
        }
    }
    //
    NekoGui::dataStore->splitter_state = ui->splitter->saveState().toBase64();
    //
    auto last_id = NekoGui::dataStore->started_id;
    if (NekoGui::dataStore->remember_enable && last_id >= 0) {
        NekoGui::dataStore->remember_id = last_id;
    }
    //
    NekoGui::dataStore->Save();
    NekoGui::profileManager->SaveManager();
    qDebug() << "End of data save";
}

void MainWindow::on_menu_exit_triggered() {
    if (mu_exit.tryLock()) {
        NekoGui::dataStore->prepare_exit = true;
        //
        neko_set_spmode_system_proxy(false, false);
        neko_set_spmode_vpn(false, false);
        if (NekoGui::dataStore->spmode_vpn) {
            mu_exit.unlock(); // retry
            return;
        }
        RegisterHotkey(true);
        //
        on_commitDataRequest();
        //
        NekoGui::dataStore->save_control_no_save = true; // don't change datastore after this line
        neko_stop();
        //
        hide();
        runOnNewThread([=, this] {
            mu_state.lock();
            runOnUiThread([=, this] {
                on_menu_exit_triggered(); // continue exit progress
            });
        });
        return;
    }
    //
    if (exit_reason == 1) {
        QDir::setCurrent(QApplication::applicationDirPath());
        QProcess::startDetached("./updater", QStringList{});
    } else if (exit_reason == 2 || exit_reason == 3) {
        QDir::setCurrent(QApplication::applicationDirPath());

        auto arguments = NekoGui::dataStore->argv;
        if (arguments.length() > 0) {
            arguments.removeFirst();
            arguments.removeAll("-tray");
            arguments.removeAll("-flag_restart_tun_on");
            arguments.removeAll("-flag_reorder");
        }
        auto isLauncher = qEnvironmentVariable("NKR_FROM_LAUNCHER") == "1";
        if (isLauncher) arguments.prepend("--");
        auto program = isLauncher ? "./launcher" : QApplication::applicationFilePath();

        if (exit_reason == 3) {
            // Tun restart as admin
            arguments << "-flag_restart_tun_on";
            runAsAdmin(program, arguments);
        } else {
            QProcess::startDetached(program, arguments);
        }
    }
    tray->hide();
    QApplication::quit();
}

void MainWindow::neko_set_spmode_system_proxy(bool enable, bool save) {
    if (enable != NekoGui::dataStore->spmode_system_proxy) {
        if (enable) {
            auto socks_port = NekoGui::dataStore->inbound_port;
            SetSystemProxy(socks_port, socks_port);
        } else {
            ClearSystemProxy();
        }
    }

    if (save) {
        NekoGui::dataStore->remember_spmode.removeAll("system_proxy");
        if (enable && NekoGui::dataStore->remember_enable) {
            NekoGui::dataStore->remember_spmode.append("system_proxy");
        }
        NekoGui::dataStore->Save();
    }

    NekoGui::dataStore->spmode_system_proxy = enable;
    refresh_status();
}

bool MainWindow::get_elevated_permissions() {
    if (isRunningAsAdmin()) return true;
#ifdef Q_OS_LINUX
    if (!Linux_HavePkexec()) {
        MessageBoxWarning(software_name, "Please install \"pkexec\" first.");
    } else {
        auto ret = Linux_Pkexec_SetCapString(QApplication::applicationFilePath(), "cap_net_admin=ep");
        if (ret == 0) {
            this->exit_reason = 3;
            on_menu_exit_triggered();
        } else {
            MessageBoxWarning(software_name, "Setcap for Tun mode failed.\n\n1. You may canceled the dialog.\n2. You may be using an incompatible environment like AppImage.");
            if (QProcessEnvironment::systemEnvironment().contains("APPIMAGE")) {
                MW_show_log("If you are using AppImage, it's impossible to start a Tun. Please use other package instead.");
            }
        }
    }
#endif
#ifdef Q_OS_WIN
    auto n = QMessageBox::warning(GetMessageBoxParent(), software_name, tr("Please run Nekoray as admin"), QMessageBox::Yes | QMessageBox::No);
    if (n == QMessageBox::Yes) {
        this->exit_reason = 3;
        on_menu_exit_triggered();
    }
#endif
#ifdef Q_OS_MACOS
    MessageBoxWarning("Need administrator privilege", "Enabling TUN mode requires elevated privileges, please run Nekoray as root.");
#endif

    return false;
}

void MainWindow::neko_set_spmode_vpn(bool enable, bool save) {
    if (enable != NekoGui::dataStore->spmode_vpn) {
        if (enable) {
            bool requestPermission = !isRunningAsAdmin();
            if (requestPermission) {
                if (!get_elevated_permissions()) {
                    refresh_status();
                    return;
                }
            }
        }
        NekoGui::dataStore->spmode_vpn = enable;
        if (NekoGui::dataStore->started_id >= 0) neko_start(NekoGui::dataStore->started_id);
    }

    if (save) {
        NekoGui::dataStore->remember_spmode.removeAll("vpn");
        if (enable && NekoGui::dataStore->remember_enable) {
            NekoGui::dataStore->remember_spmode.append("vpn");
        }
        NekoGui::dataStore->Save();
    }

    refresh_status();
}

void MainWindow::refresh_status(const QString &traffic_update) {
    auto refresh_speed_label = [=, this] {
        if (NekoGui::dataStore->traffic_loop_interval == 0) {
            ui->label_speed->setText("");
        } else if (traffic_update_cache == "") {
            ui->label_speed->setText(QObject::tr("Proxy: %1\nDirect: %2").arg("", ""));
        } else {
            ui->label_speed->setText(traffic_update_cache);
        }
    };

    // From TrafficLooper
    if (!traffic_update.isEmpty() && NekoGui::dataStore->traffic_loop_interval > 0) {
        traffic_update_cache = traffic_update;
        if (traffic_update == "STOP") {
            traffic_update_cache = "";
        } else {
            refresh_speed_label();
            return;
        }
    }

    refresh_speed_label();

    // From UI
    QString group_name;
    if (running != nullptr) {
        auto group = NekoGui::profileManager->GetGroup(running->gid);
        if (group != nullptr) group_name = group->name;
    }

    if (QDateTime::currentSecsSinceEpoch() - last_test_time > 2) {
        ui->label_running->setText(running ? QString("[%1] %2").arg(group_name, running->bean->DisplayName()).left(30) : tr("Not Running"));
    }
    //
    auto display_socks = DisplayAddress(NekoGui::dataStore->inbound_address, NekoGui::dataStore->inbound_port);
    auto inbound_txt = QString("Mixed: %1").arg(display_socks);
    ui->label_inbound->setText(inbound_txt);
    //
    ui->checkBox_VPN->setChecked(NekoGui::dataStore->spmode_vpn);
    ui->checkBox_SystemProxy->setChecked(NekoGui::dataStore->spmode_system_proxy);
    if (select_mode) {
        ui->label_running->setText(tr("Select") + " *");
        ui->label_running->setToolTip(tr("Select mode, double-click or press Enter to select a profile, press ESC to exit."));
    } else {
        ui->label_running->setToolTip({});
    }

    auto make_title = [=, this](bool isTray) {
        QStringList tt;
        if (!isTray && isRunningAsAdmin()) tt << "[Admin]";
        if (select_mode) tt << "[" + tr("Select") + "]";
        if (!title_error.isEmpty()) tt << "[" + title_error + "]";
        if (NekoGui::dataStore->spmode_vpn && !NekoGui::dataStore->spmode_system_proxy) tt << "[Tun]";
        if (!NekoGui::dataStore->spmode_vpn && NekoGui::dataStore->spmode_system_proxy) tt << "[" + tr("System Proxy") + "]";
        if (NekoGui::dataStore->spmode_vpn && NekoGui::dataStore->spmode_system_proxy) tt << "[Tun+" + tr("System Proxy") + "]";
        tt << software_name;
        if (!isTray) tt << "(" + QString(NKR_VERSION) + ")";
        if (!NekoGui::dataStore->active_routing.isEmpty() && NekoGui::dataStore->active_routing != "Default") {
            tt << "[" + NekoGui::dataStore->active_routing + "]";
        }
        if (running != nullptr) tt << running->bean->DisplayTypeAndName() + "@" + group_name;
        return tt.join(isTray ? "\n" : " ");
    };

    auto icon_status_new = Icon::NONE;

    if (running != nullptr) {
        if (NekoGui::dataStore->spmode_vpn) {
            icon_status_new = Icon::VPN;
        } else if (NekoGui::dataStore->spmode_system_proxy) {
            icon_status_new = Icon::SYSTEM_PROXY;
        } else {
            icon_status_new = Icon::RUNNING;
        }
    }

    // refresh title & window icon
    setWindowTitle(make_title(false));
    if (icon_status_new != icon_status) QApplication::setWindowIcon(Icon::GetTrayIcon(Icon::NONE));

    // refresh tray
    if (tray != nullptr) {
        tray->setToolTip(make_title(true));
        if (icon_status_new != icon_status) tray->setIcon(Icon::GetTrayIcon(icon_status_new));
    }

    icon_status = icon_status_new;
}

// table显示

void MainWindow::updateTableRow(int row, int id, QTableWidget *tableWidget) {
    auto profile = NekoGui::profileManager->GetProfile(id);
    if (!profile) return;

    auto makeItem = [&](const QVariant &value, const QColor &color = QColor()) {
        auto item = new QTableWidgetItem();
        item->setData(Qt::DisplayRole, value);
        item->setData(114514, profile->id);
        if (color.isValid()) item->setForeground(color);
        if (id == NekoGui::dataStore->started_id) item->setBackground(palette().highlight().color().lighter());
        return item;
    };

    tableWidget->setItem(row, 0, makeItem(profile->bean->DisplayType()));

    tableWidget->setItem(row, 1, makeItem(profile->bean->DisplayAddress()));

    tableWidget->setItem(row, 2, makeItem(profile->bean->name));

    const QVariant testResult = profile->full_test_report.isEmpty() ? profile->DisplayLatency() : profile->full_test_report;
    const QColor testColor = profile->full_test_report.isEmpty() ? profile->DisplayLatencyColor() : QColor();
    tableWidget->setItem(row, 3, makeItem(testResult, testColor));

    tableWidget->setItem(row, 4, makeItem(profile->traffic_data->DisplayTraffic()));
}

QTableWidget *MainWindow::createTable(int gid) {
    auto group = NekoGui::profileManager->GetGroup(gid);
    auto tableWidget = new QTableWidget(group->order.size(), 5);
    tableWidget->setHorizontalHeaderLabels({tr("Type"), tr("Address"), tr("Name"), tr("Test Result"), tr("Traffic")});
    tableWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    // tableWidget->setAlternatingRowColors(true);
    tableWidget->setTabKeyNavigation(false);
    tableWidget->setWordWrap(false);
    tableWidget->verticalHeader()->setDefaultSectionSize(24);
    // tableWidget->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    tableWidget->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    tableWidget->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    tableWidget->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    tableWidget->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    tableWidget->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    tableWidget->horizontalHeader()->setHighlightSections(false);
    connect(tableWidget->horizontalHeader(), &QHeaderView::sortIndicatorChanged, this, [=, this](int logicalIndex, Qt::SortOrder order) {
        tableWidget->sortItems(logicalIndex, order);
        group->order.clear();
        for (int row = 0; row < tableWidget->rowCount(); ++row) {
            group->order.append(tableWidget->item(row, 0)->data(114514).toInt());
        }
        group->Save();
    });
    connect(tableWidget, &QTableWidget::itemDoubleClicked, this, [this](QTableWidgetItem *item) {
        auto id = item->data(114514).toInt();
        if (select_mode) {
            emit profile_selected(id);
            select_mode = false;
            refresh_status();
            return;
        }
        auto dialog = new DialogEditProfile("", id, this);
        connect(dialog, &QDialog::finished, dialog, &QDialog::deleteLater);
    });
    connect(tableWidget, &QWidget::customContextMenuRequested, this, [=, this](const QPoint &pos) {
        ui->menu_server->popup(tableWidget->viewport()->mapToGlobal(pos));
    });
    for (const auto &[row, id]: std::views::enumerate(group->order)) {
        updateTableRow(row, id, tableWidget);
    }
    return tableWidget;
}

void MainWindow::refresh_proxy(int id) {
    auto profile = NekoGui::profileManager->GetProfile(id);
    auto tableWidget = qobject_cast<QTableWidget *>(ui->tabWidget->widget(NekoGui::profileManager->groupsTabOrder.indexOf(profile->gid)));
    updateTableRow(NekoGui::profileManager->GetGroup(profile->gid)->order.indexOf(id), id, tableWidget);
}

void MainWindow::refresh_group(int gid) {
    QTableWidget *tableWidget;
    if (gid < 0) {
        gid = NekoGui::dataStore->current_group;
        tableWidget = qobject_cast<QTableWidget *>(ui->tabWidget->currentWidget());
    } else {
        tableWidget = qobject_cast<QTableWidget *>(ui->tabWidget->widget(NekoGui::profileManager->groupsTabOrder.indexOf(gid)));
    }

    auto group = NekoGui::profileManager->GetGroup(gid);
    group->order.removeIf([](int k) { return !NekoGui::profileManager->profiles.contains(k); });
    for (const auto &[id, profile]: NekoGui::profileManager->profiles)
        if (profile->gid == gid && !group->order.contains(id))
            group->order.append(id);
    group->Save();

    tableWidget->setRowCount(0);
    for (const auto &[row, id]: std::views::enumerate(group->order)) {
        tableWidget->insertRow(row);
        updateTableRow(row, id, tableWidget);
    }
    ui->tabWidget->setTabText(ui->tabWidget->indexOf(tableWidget), group->name);
}

void MainWindow::refresh_groups() {
    QList<int> validGids;
    for (int i = ui->tabWidget->count() - 1; i >= 0; --i) {
        int gid = ui->tabWidget->tabBar()->tabData(i).toInt();
        if (NekoGui::profileManager->groupsTabOrder.contains(gid)) {
            validGids.append(gid);
        } else {
            QWidget *page = ui->tabWidget->widget(i);
            ui->tabWidget->removeTab(i);
            delete page;
        }
    }
    int last_gid = NekoGui::dataStore->current_group;
    for (const auto &[index, gid]: std::views::enumerate(NekoGui::profileManager->groupsTabOrder)) {
        if (!validGids.contains(gid)) {
            auto group = NekoGui::profileManager->GetGroup(gid);
            ui->tabWidget->insertTab(index, createTable(gid), group->name);
            ui->tabWidget->tabBar()->setTabData(index, gid);
        }
    }
    ui->tabWidget->setCurrentIndex(NekoGui::profileManager->groupsTabOrder.indexOf(last_gid));
}

// table菜单相关

void MainWindow::on_menu_add_from_input_triggered() {
    auto dialog = new DialogEditProfile("socks", NekoGui::dataStore->current_group, this);
    connect(dialog, &QDialog::finished, dialog, &QDialog::deleteLater);
}

void MainWindow::on_menu_add_from_clipboard_triggered() {
    NekoGui_sub::groupUpdater->AsyncUpdate(QApplication::clipboard()->text());
}

void MainWindow::on_menu_clone_triggered() {
    auto ents = get_now_selected_list();
    if (ents.isEmpty()) return;

    auto btn = QMessageBox::question(this, tr("Clone"), tr("Clone %1 item(s)").arg(ents.count()));
    if (btn != QMessageBox::Yes) return;

    QStringList sls;
    for (const auto &ent: ents) {
        sls << ent->bean->ToNekorayShareLink(ent->type);
    }

    NekoGui_sub::groupUpdater->AsyncUpdate(sls.join("\n"));
}

void MainWindow::on_menu_move_triggered() {
    auto ents = get_now_selected_list();
    if (ents.isEmpty()) return;

    auto items = QStringList{};
    for (auto gid: NekoGui::profileManager->groupsTabOrder) {
        auto group = NekoGui::profileManager->GetGroup(gid);
        if (group == nullptr) continue;
        items += Int2String(gid) + " " + group->name;
    }

    bool ok;
    auto a = QInputDialog::getItem(nullptr,
                                   tr("Move"),
                                   tr("Move %1 item(s)").arg(ents.count()),
                                   items, 0, false, &ok);
    if (!ok) return;
    auto gid = SubStrBefore(a, " ").toInt();
    for (const auto &ent: ents) {
        NekoGui::profileManager->MoveProfile(ent, gid);
    }
    refresh_group();
    refresh_group(gid);
}

void MainWindow::on_menu_delete_triggered() {
    auto ents = get_now_selected_list();
    if (ents.count() == 0) return;
    if (QMessageBox::question(this, tr("Confirmation"), QString(tr("Remove %1 item(s) ?")).arg(ents.count())) ==
        QMessageBox::StandardButton::Yes) {
        for (const auto &ent: ents) {
            NekoGui::profileManager->DeleteProfile(ent->id);
        }
        refresh_group();
    }
}

void MainWindow::on_menu_reset_traffic_triggered() {
    auto ents = get_now_selected_list();
    if (ents.count() == 0) return;
    for (const auto &ent: ents) {
        ent->traffic_data->Reset();
        ent->Save();
        refresh_proxy(ent->id);
    }
}

void MainWindow::on_menu_profile_debug_info_triggered() {
    auto ents = get_now_selected_list();
    if (ents.count() != 1) return;
    QMessageBox msgBox(QMessageBox::NoIcon, software_name, ents.first()->ToJsonBytes(), QMessageBox::Ok, this);
    QPushButton *button_1 = msgBox.addButton("Edit", QMessageBox::ActionRole);
    QPushButton *button_2 = msgBox.addButton("Reload", QMessageBox::ResetRole);
    msgBox.setDefaultButton(QMessageBox::Ok);
    msgBox.setEscapeButton(QMessageBox::Ok);
    msgBox.exec();
    if (msgBox.clickedButton() == button_1) {
        QDesktopServices::openUrl(QUrl::fromLocalFile(QFileInfo(QString("profiles/%1.json").arg(ents.first()->id)).absoluteFilePath()));
    } else if (msgBox.clickedButton() == button_2) {
        NekoGui::dataStore->Load();
        NekoGui::profileManager->LoadManager();
        refresh_proxy(ents.first()->id);
    }
}

void MainWindow::on_menu_copy_links_triggered() {
    if (ui->masterLogBrowser->hasFocus()) {
        ui->masterLogBrowser->copy();
        return;
    }
    auto ents = get_now_selected_list();
    QStringList links;
    for (const auto &ent: ents) {
        links += ent->bean->ToShareLink();
    }
    if (links.length() == 0) return;
    QApplication::clipboard()->setText(links.join("\n"));
    show_log_impl(tr("Copied %1 item(s)").arg(links.length()));
}

void MainWindow::on_menu_copy_links_nkr_triggered() {
    auto ents = get_now_selected_list();
    QStringList links;
    for (const auto &ent: ents) {
        links += ent->bean->ToNekorayShareLink(ent->type);
    }
    if (links.length() == 0) return;
    QApplication::clipboard()->setText(links.join("\n"));
    show_log_impl(tr("Copied %1 item(s)").arg(links.length()));
}

void MainWindow::on_menu_export_config_triggered() {
    auto ents = get_now_selected_list();
    if (ents.count() != 1) return;
    auto ent = ents.first();
    if (ent->bean->DisplayCoreType() != software_core_name) return;

    auto result = BuildConfig(ent, false, true);
    QString config_core = QJsonObject2QString(result->coreConfig, true);
    QApplication::clipboard()->setText(config_core);

    QMessageBox msg(QMessageBox::NoIcon, tr("Config copied"), config_core, QMessageBox::Ok, this);
    QPushButton *button_1 = msg.addButton("Copy core config", QMessageBox::YesRole);
    QPushButton *button_2 = msg.addButton("Copy test config", QMessageBox::YesRole);
    msg.setEscapeButton(QMessageBox::Ok);
    msg.setDefaultButton(QMessageBox::Ok);
    msg.exec();
    if (msg.clickedButton() == button_1) {
        result = BuildConfig(ent, false, false);
        config_core = QJsonObject2QString(result->coreConfig, true);
        QApplication::clipboard()->setText(config_core);
    } else if (msg.clickedButton() == button_2) {
        result = BuildConfig(ent, true, false);
        config_core = QJsonObject2QString(result->coreConfig, true);
        QApplication::clipboard()->setText(config_core);
    }
}

void MainWindow::display_qr_link(bool nkrFormat) {
    auto ents = get_now_selected_list();
    if (ents.count() != 1) return;

    class W : public QDialog {
    public:
        QLabel *qrLabel = nullptr;
        QCheckBox *cb = nullptr;
        QPlainTextEdit *textEdit = nullptr;
        QImage qrImage;
        QString link, link_nk;

        W(const QString &link_, const QString &link_nk_, bool nkrFormat)
            : link(link_), link_nk(link_nk_) {
            auto *layout = new QVBoxLayout(this);

            qrLabel = new QLabel();
            qrLabel->setMinimumSize(256, 256);
            qrLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
            qrLabel->setAlignment(Qt::AlignCenter);
            layout->addWidget(qrLabel);

            cb = new QCheckBox("Neko Links");
            cb->setChecked(nkrFormat);
            layout->addWidget(cb);

            textEdit = new QPlainTextEdit();
            textEdit->setReadOnly(true);
            layout->addWidget(textEdit);

            connect(cb, &QCheckBox::toggled, this, &W::refresh);
            refresh();
        }

        void refresh() {
            const QString &link_display = cb->isChecked() ? link_nk : link;
            textEdit->setPlainText(link_display);

            try {
                auto writer = ZXing::MultiFormatWriter(ZXing::BarcodeFormat::QRCode);
                auto matrix = writer.encode(link_display.toStdString(), 0, 0);
                auto bitmap = ZXing::ToMatrix<uint8_t>(matrix);
                qrImage = QImage(bitmap.data(), bitmap.width(), bitmap.height(), bitmap.width(), QImage::Format_Grayscale8).copy();
                showQR();
            } catch (const std::exception &ex) {
                QMessageBox::warning(this, "QR generation error", ex.what());
            }
        }

        void showQR() {
            qrLabel->setPixmap(QPixmap::fromImage(qrImage.scaled(qrLabel->size(), Qt::KeepAspectRatio, Qt::FastTransformation)));
        }

        void resizeEvent(QResizeEvent *event) override {
            QDialog::resizeEvent(event);
            showQR();
        }
    };

    QString link = ents.first()->bean->ToShareLink();
    QString link_nk = ents.first()->bean->ToNekorayShareLink(ents.first()->type);
    W w(link, link_nk, nkrFormat);
    w.setWindowTitle(ents.first()->bean->DisplayTypeAndName());
    w.exec();
}

void MainWindow::on_menu_scan_qr_triggered() {
    using namespace ZXingQt;

    hide();

    QTimer::singleShot(200, this, [this] {
    auto screen = QGuiApplication::primaryScreen();
        auto qpx = screen->grabWindow();

    show();

    auto hints = ReaderOptions()
                     .setFormats(BarcodeFormat::QRCode)
                     .setTryRotate(false)
                     .setBinarizer(Binarizer::FixedThreshold);

    auto result = ReadBarcode(qpx.toImage(), hints);
    const auto &text = result.text();
    if (text.isEmpty()) {
        MessageBoxInfo(software_name, tr("QR Code not found"));
    } else {
        show_log_impl("QR Code Result:\n" + text);
        NekoGui_sub::groupUpdater->AsyncUpdate(text);
    }
    });
}

void MainWindow::on_menu_clear_test_result_triggered() {
    for (const auto &profile: get_selected_or_group()) {
        profile->latency = 0;
        profile->full_test_report = "";
        profile->Save();
    }
    refresh_group();
}

void MainWindow::on_menu_select_all_triggered() {
    if (ui->masterLogBrowser->hasFocus()) {
        ui->masterLogBrowser->selectAll();
        return;
    }
    qobject_cast<QTableWidget *>(ui->tabWidget->currentWidget())->selectAll();
}

void MainWindow::on_menu_delete_repeat_triggered() {
    QList<std::shared_ptr<NekoGui::ProxyEntity>> out;
    QList<std::shared_ptr<NekoGui::ProxyEntity>> out_del;

    NekoGui::ProfileFilter::Uniq(NekoGui::profileManager->CurrentGroup()->Profiles(), out, true, false);
    NekoGui::ProfileFilter::OnlyInSrc_ByPointer(NekoGui::profileManager->CurrentGroup()->Profiles(), out, out_del);

    int remove_display_count = 0;
    QString remove_display;
    for (const auto &ent: out_del) {
        remove_display += ent->bean->DisplayTypeAndName() + "\n";
        if (++remove_display_count == 20) {
            remove_display += "...";
            break;
        }
    }

    if (out_del.length() > 0 &&
        QMessageBox::question(this, tr("Confirmation"), tr("Remove %1 item(s) ?").arg(out_del.length()) + "\n" + remove_display) == QMessageBox::StandardButton::Yes) {
        for (const auto &ent: out_del) {
            NekoGui::profileManager->DeleteProfile(ent->id);
        }
        refresh_group();
    }
}

void MainWindow::on_menu_update_subscription_triggered() {
    auto group = NekoGui::profileManager->CurrentGroup();
    if (group->url.isEmpty()) return;
    if (mw_sub_updating) return;
    mw_sub_updating = true;
    NekoGui_sub::groupUpdater->AsyncUpdate(group->url, group->id, [&] { mw_sub_updating = false; });
}

void MainWindow::on_menu_remove_unavailable_triggered() {
    QList<std::shared_ptr<NekoGui::ProxyEntity>> out_del;

    for (const auto &[_, profile]: NekoGui::profileManager->profiles) {
        if (NekoGui::dataStore->current_group != profile->gid) continue;
        if (profile->latency < 0) out_del += profile;
    }

    int remove_display_count = 0;
    QString remove_display;
    for (const auto &ent: out_del) {
        remove_display += ent->bean->DisplayTypeAndName() + "\n";
        if (++remove_display_count == 20) {
            remove_display += "...";
            break;
        }
    }

    if (out_del.length() > 0 &&
        QMessageBox::question(this, tr("Confirmation"), tr("Remove %1 item(s) ?").arg(out_del.length()) + "\n" + remove_display) == QMessageBox::StandardButton::Yes) {
        for (const auto &ent: out_del) {
            NekoGui::profileManager->DeleteProfile(ent->id);
        }
        refresh_group();
    }
}

void MainWindow::on_menu_resolve_domain_triggered() {
    auto profiles = get_selected_or_group();
    if (profiles.isEmpty()) return;

    if (QMessageBox::question(this,
                              tr("Confirmation"),
                              tr("Resolving domain to IP, if support.")) != QMessageBox::StandardButton::Yes) {
        return;
    }
    if (mw_sub_updating) return;
    mw_sub_updating = true;
    NekoGui::dataStore->resolve_count = profiles.count();

    for (const auto &profile: profiles) {
        profile->bean->ResolveDomainToIP([=, this] {
            profile->Save();
            if (--NekoGui::dataStore->resolve_count != 0) return;
            refresh_group(profile->gid);
            mw_sub_updating = false;
        });
    }
}

QList<std::shared_ptr<NekoGui::ProxyEntity>> MainWindow::get_now_selected_list() {
    auto items = qobject_cast<QTableWidget *>(ui->tabWidget->currentWidget())->selectedItems();
    QList<std::shared_ptr<NekoGui::ProxyEntity>> list;
    for (auto item: items) {
        auto id = item->data(114514).toInt();
        auto ent = NekoGui::profileManager->GetProfile(id);
        if (ent != nullptr && !list.contains(ent)) list += ent;
    }
    return list;
}

QList<std::shared_ptr<NekoGui::ProxyEntity>> MainWindow::get_selected_or_group() {
    auto selected_or_group = ui->menu_server->property("selected_or_group").toInt();
    QList<std::shared_ptr<NekoGui::ProxyEntity>> profiles;
    if (selected_or_group > 0) {
        profiles = get_now_selected_list();
        if (profiles.isEmpty() && selected_or_group == 2) profiles = NekoGui::profileManager->CurrentGroup()->ProfilesWithOrder();
    } else {
        profiles = NekoGui::profileManager->CurrentGroup()->ProfilesWithOrder();
    }
    return profiles;
}

void MainWindow::keyPressEvent(QKeyEvent *event) {
    switch (event->key()) {
        case Qt::Key_Enter:
            neko_start();
            break;
        default:
            QMainWindow::keyPressEvent(event);
    }
}

// Log

void MainWindow::show_log_impl(const QString &log) {
    auto logText = log.trimmed();
    if (logText.isEmpty()) return;

    if (!NekoGui::dataStore->log_ignore.isEmpty()) {
        QStringList newLines;
        for (const auto &line: SplitLines(logText)) {
            bool showThisLine = true;
            for (const auto &str: NekoGui::dataStore->log_ignore) {
                if (line.contains(str)) {
                    showThisLine = false;
                    break;
                }
            }
            if (showThisLine) newLines << line;
        }
        if (newLines.isEmpty()) return;
        logText = newLines.join("\n");
    }

    ui->masterLogBrowser->append(logText);
}

void MainWindow::on_masterLogBrowser_customContextMenuRequested(const QPoint &pos) {
    QMenu *menu = ui->masterLogBrowser->createStandardContextMenu();

    menu->addSeparator();

    QAction *action_add_ignore = menu->addAction(tr("Set ignore keyword"));
    connect(action_add_ignore, &QAction::triggered, this, [this] {
        auto list = NekoGui::dataStore->log_ignore;
        auto newStr = ui->masterLogBrowser->textCursor().selectedText().trimmed();
        if (!newStr.isEmpty()) list << newStr;
        bool ok;
        newStr = QInputDialog::getMultiLineText(GetMessageBoxParent(), tr("Set ignore keyword"), tr("Set the following keywords to ignore?\nSplit by line."), list.join("\n"), &ok);
        if (ok) {
            NekoGui::dataStore->log_ignore = SplitLines(newStr);
            NekoGui::dataStore->Save();
        }
    });

    QAction *action_clear = menu->addAction(tr("Clear"));
    connect(action_clear, &QAction::triggered, ui->masterLogBrowser, &QTextBrowser::clear);

    menu->exec(ui->masterLogBrowser->viewport()->mapToGlobal(pos)); // 弹出菜单
    menu->deleteLater();
}

// Group tab manage

void MainWindow::on_tabWidget_currentChanged(int index) {
    int gid = NekoGui::profileManager->groupsTabOrder.value(index, -1);
    if (NekoGui::dataStore->current_group != gid) {
        NekoGui::dataStore->current_group = gid;
        NekoGui::dataStore->Save();
    }
}

void MainWindow::on_tabWidget_customContextMenuRequested(const QPoint &pos) {
    int clickedIndex = ui->tabWidget->tabBar()->tabAt(pos);
    QMenu menu(this);

    QAction *addAction = menu.addAction(tr("Add new Group"));
    connect(addAction, &QAction::triggered, this, [=, this] {
        auto ent = NekoGui::ProfileManager::NewGroup();
        DialogEditGroup dialog(ent, this);
        if (dialog.exec() == QDialog::Accepted) {
            NekoGui::profileManager->AddGroup(ent);
            refresh_groups();
        }
    });

    if (clickedIndex >= 0) {
        QAction *editAction = menu.addAction(tr("Edit selected Group"));
        connect(editAction, &QAction::triggered, this, [=, this] {
            auto id = NekoGui::profileManager->groupsTabOrder[clickedIndex];
            auto ent = NekoGui::profileManager->groups[id];
            auto dialog = new DialogEditGroup(ent, this);
            connect(dialog, &QDialog::finished, this, [=, this] {
                if (dialog->result() == QDialog::Accepted) {
                    ent->Save();
                    refresh_group(id);
                }
                dialog->deleteLater();
            });
            dialog->show();
        });
    }

    if (clickedIndex > 0) {
        QAction *deleteAction = menu.addAction(tr("Delete selected Group"));
        connect(deleteAction, &QAction::triggered, this, [=, this] {
            auto id = NekoGui::profileManager->groupsTabOrder[clickedIndex];
            if (QMessageBox::question(this, tr("Confirmation"), tr("Remove %1?").arg(NekoGui::profileManager->groups[id]->name)) ==
                QMessageBox::StandardButton::Yes) {
                NekoGui::profileManager->DeleteGroup(id);
                refresh_groups();
            }
        });
    }

    menu.exec(ui->tabWidget->tabBar()->mapToGlobal(pos));
}

// eventFilter

bool MainWindow::eventFilter(QObject *obj, QEvent *event) {
    if (event->type() == QEvent::MouseButtonPress) {
        auto mouseEvent = static_cast<QMouseEvent *>(event);
        if (obj == ui->label_running && mouseEvent->button() == Qt::LeftButton && running) {
            speedtest_current();
            return true;
        }
    }
    return QMainWindow::eventFilter(obj, event);
}

// profile selector

void MainWindow::start_select_mode(QObject *context, const std::function<void(int)> &callback) {
    select_mode = true;
    connectOnce(this, &MainWindow::profile_selected, context, callback);
    refresh_status();
}

// Hotkey

void MainWindow::RegisterHotkey(bool unregister) {
    static QList<QHotkey *> RegisteredHotkey;

    for (auto &hotkey: RegisteredHotkey) hotkey->deleteLater();
    RegisteredHotkey.clear();
    if (unregister) return;

    const QMap<QString, std::function<void()>> hotkeyActions = {
        {NekoGui::dataStore->hotkey_mainwindow, [this] { tray->activated(QSystemTrayIcon::Trigger); }},
        {NekoGui::dataStore->hotkey_group, [this] { on_menu_manage_groups_triggered(); }},
        {NekoGui::dataStore->hotkey_route, [this] { on_menu_routing_settings_triggered(); }},
        {NekoGui::dataStore->hotkey_system_proxy_menu, [this] { ui->menu_spmode->popup(QCursor::pos()); }},
    };

    for (auto it = hotkeyActions.constBegin(); it != hotkeyActions.constEnd(); ++it) {
        const QString &key = it.key();
        if (key.isEmpty()) continue;
        QHotkey *hotkey = new QHotkey(QKeySequence(key), true, this);
        if (hotkey->isRegistered()) {
            RegisteredHotkey.append(hotkey);
            connect(hotkey, &QHotkey::activated, this, it.value());
        } else {
            hotkey->deleteLater();
        }
    }
}

void MainWindow::updateLogMaxLines() {
    ui->masterLogBrowser->document()->setMaximumBlockCount(NekoGui::dataStore->max_log_line);
}

void MainWindow::resetAutoUpdateSubscription(int minutes) {
    if (minutes >= 30) autoUpdateSubscriptionTimer->start(minutes * 60 * 1000);
}

MainWindow *MainWindow::instance() {
    return (MainWindow *) mainwindow;
}