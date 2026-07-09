#include "ui/dialog/DialogHotkey.hpp"
#include "ui_DialogHotkey.h"

#include "ui/MainWindow.hpp"

DialogHotkey::DialogHotkey(QWidget *parent) : QDialog(parent), ui(new Ui::DialogHotkey) {
    ui->setupUi(this);
    ui->show_mainwindow->setKeySequence(NekoGui::dataStore->hotkey_mainwindow);
    ui->show_groups->setKeySequence(NekoGui::dataStore->hotkey_group);
    ui->show_routes->setKeySequence(NekoGui::dataStore->hotkey_route);
    ui->system_proxy->setKeySequence(NekoGui::dataStore->hotkey_system_proxy_menu);
    MainWindow::instance()->RegisterHotkey(true);
}

DialogHotkey::~DialogHotkey() {
    if (result() == QDialog::Accepted) {
        NekoGui::dataStore->hotkey_mainwindow = ui->show_mainwindow->keySequence().toString();
        NekoGui::dataStore->hotkey_group = ui->show_groups->keySequence().toString();
        NekoGui::dataStore->hotkey_route = ui->show_routes->keySequence().toString();
        NekoGui::dataStore->hotkey_system_proxy_menu = ui->system_proxy->keySequence().toString();
        NekoGui::dataStore->Save();
    }
    MainWindow::instance()->RegisterHotkey(false);
    delete ui;
}
