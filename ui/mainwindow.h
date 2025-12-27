#pragma once

#include <QMainWindow>
#include <QFuture>
#include <QTableWidget>
#include <QKeyEvent>
#include <QSystemTrayIcon>
#include <QShortcut>
#include <QMutex>

#include "db/ProxyEntity.hpp"
#include "main/GuiUtils.hpp"
#include "main/NekoGui_DataStore.hpp"

QT_BEGIN_NAMESPACE
namespace Ui {
    class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

    ~MainWindow() override;

    void refresh_proxy(int id);

    void refresh_group(int gid = -1);

    void refresh_groups();

    void refresh_status(const QString &traffic_update = "");

    void neko_start(int _id = -1);

    void neko_stop(bool crash = false);

    void neko_set_spmode_vpn(bool enable);

    void neko_set_spmode_system_proxy(bool enable);

    bool get_elevated_permissions();

    void show_log_impl(const QString &log);

    void start_select_mode(QObject *context, const std::function<void(int)> &callback);

    void RegisterHotkey(bool unregister);

    void updateLogMaxLines();

    void resetAutoUpdateSubscription(int minutes);

    static MainWindow *instance();

signals:

    void profile_selected(int id);

public slots:

    void on_commitDataRequest();

    void on_menu_exit_triggered();

private slots:

    void on_masterLogBrowser_customContextMenuRequested(const QPoint &pos);

    void on_menu_basic_settings_triggered();

    void on_menu_routing_settings_triggered();

    void on_menu_hotkey_settings_triggered();

    void on_menu_add_from_input_triggered();

    void on_menu_add_from_clipboard_triggered();

    void on_menu_clone_triggered();

    void on_menu_move_triggered();

    void on_menu_delete_triggered();

    void on_menu_reset_traffic_triggered();

    void on_menu_profile_debug_info_triggered();

    void on_menu_copy_links_triggered();

    void on_menu_copy_links_nkr_triggered();

    void on_menu_export_config_triggered();

    void on_menu_scan_qr_triggered();

    void on_menu_clear_test_result_triggered();

    void on_menu_manage_groups_triggered();

    void on_menu_select_all_triggered();

    void on_menu_delete_repeat_triggered();

    void on_menu_remove_unavailable_triggered();

    void on_menu_update_subscription_triggered();

    void on_menu_resolve_domain_triggered();

    void on_tabWidget_currentChanged(int index);

    void on_tabWidget_customContextMenuRequested(const QPoint &p);

private:
    Ui::MainWindow *ui;
    QSystemTrayIcon *tray;
    QShortcut *shortcut_ctrl_f = new QShortcut(QKeySequence("Ctrl+F"), this);
    QShortcut *shortcut_esc = new QShortcut(QKeySequence("Esc"), this);
    //
    bool qvLogAutoScoll = true;
    //
    QString title_error;
    int icon_status = -1;
    std::shared_ptr<NekoGui::ProxyEntity> running;
    QString traffic_update_cache;
    qint64 last_test_time = 0;
    //
    bool select_mode = false;
    QMutex mu_state;
    int exit_reason = 0;
    //
    bool dialog_is_using = false;
    bool mw_sub_updating = false;
    QTimer *autoUpdateSubscriptionTimer;
    //
    enum TestMode {
        TcpPing = 1 << 0,
        UrlTest = 1 << 1,
        UdpTest = 1 << 2,
        SpeedTest = 1 << 3,
        IpTest = 1 << 4
    };
    QList<std::shared_ptr<NekoGui::ProxyEntity>> speedtestProfiles;
    QFuture<void> speedtestFuture;

    QList<std::shared_ptr<NekoGui::ProxyEntity>> get_now_selected_list();

    QList<std::shared_ptr<NekoGui::ProxyEntity>> get_selected_or_group();

    void dialog_message_impl(const QString &sender, const QString &info);

    void updateTableRow(int row, int id, QTableWidget *tableWidget);

    QTableWidget *createTable(int gid);

    void keyPressEvent(QKeyEvent *event) override;

    void closeEvent(QCloseEvent *event) override;

    template<typename DialogType, typename... Args>
    void openDialog(Args &&...args);

    void display_qr_link(bool nkrFormat = false);

    void speedtest_current_group(int mode);

    void speedtest_current();

    void CheckUpdate();

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;
};
