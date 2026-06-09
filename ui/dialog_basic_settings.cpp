#include "dialog_basic_settings.h"
#include "ui_dialog_basic_settings.h"

#include <QFileDialog>
#include <QFontDatabase>
#include <QInputDialog>
#include <QMessageBox>
#include <QStyleFactory>

#include "3rdparty/qv2ray/v2/ui/widgets/editors/w_JsonEditor.hpp"
#include "fmt/Preset.hpp"
#include "main/GuiUtils.hpp"
#include "sys/AutoRun.hpp"
#include "ui/mainwindow.h"

class ExtraCoreWidget : public QWidget {
public:
    QString coreName;

    QLabel *label_name;
    QLineEdit *lineEdit_path;
    QPushButton *pushButton_pick;

    explicit ExtraCoreWidget(QJsonObject *extraCore, const QString &coreName_,
                             QWidget *parent = nullptr)
        : QWidget(parent) {
        coreName = coreName_;
        label_name = new QLabel;
        label_name->setText(coreName);
        lineEdit_path = new QLineEdit;
        lineEdit_path->setText(extraCore->value(coreName).toString());
        pushButton_pick = new QPushButton;
        pushButton_pick->setText(QObject::tr("Select"));
        auto layout = new QHBoxLayout;
        layout->addWidget(label_name);
        layout->addWidget(lineEdit_path);
        layout->addWidget(pushButton_pick);
        setLayout(layout);
        setContentsMargins(0, 0, 0, 0);
        //
        connect(pushButton_pick, &QPushButton::clicked, this, [=, this] {
            auto fn = QFileDialog::getOpenFileName(this, QObject::tr("Select"), QDir::currentPath(),
                                                   "", nullptr, QFileDialog::Option::ReadOnly);
            if (!fn.isEmpty()) {
                lineEdit_path->setText(fn);
            }
        });
        connect(lineEdit_path, &QLineEdit::textChanged, this, [=, this](const QString &newTxt) {
            extraCore->insert(coreName, newTxt);
        });
    }
};

DialogBasicSettings::DialogBasicSettings(QWidget *parent)
    : QDialog(parent), ui(new Ui::DialogBasicSettings) {
    ui->setupUi(this);
    ADD_ASTERISK(this);

    // Common

    D_LOAD_INT(test_concurrent)
    D_LOAD_INT(test_download_timeout)
    D_LOAD_STRING(test_latency_url)
    D_LOAD_STRING(test_download_url)
    D_LOAD_BOOL(old_share_link_format)

    connect(ui->custom_inbound_edit, &QPushButton::clicked, this, [=, this] {
        C_EDIT_JSON_ALLOW_EMPTY(custom_inbound)
    });

#ifdef Q_OS_WIN
    ui->system_proxy_format->addItems(Preset::Windows::system_proxy_format);
    ui->system_proxy_format->setCurrentText(NekoGui::dataStore->system_proxy_format);
#else
    ui->systemProxyBox->hide();
#endif

    // Style
    D_LOAD_BOOL(check_include_pre)
    D_LOAD_BOOL(start_minimal)
    D_LOAD_INT(max_log_line)

    ui->launch_at_startup->setChecked(AutoRun_IsEnabled());
    //
    ui->rfsh_r->setItemData(0, 500);
    ui->rfsh_r->setItemData(1, 1000);
    ui->rfsh_r->setItemData(2, 2000);
    ui->rfsh_r->setItemData(3, 3000);
    ui->rfsh_r->setItemData(4, 5000);
    ui->rfsh_r->setItemData(5, 0);
    ui->rfsh_r->setCurrentIndex(ui->rfsh_r->findData(NekoGui::dataStore->traffic_loop_interval));
    //
    ui->language->setItemData(0, "en");
    ui->language->setItemData(1, "zh_CN");
    ui->language->setItemData(2, "fa_IR");
    ui->language->setItemData(3, "ru_RU");
    ui->language->setCurrentIndex(ui->language->findData(NekoGui::dataStore->language));
    connect(ui->language, &QComboBox::currentIndexChanged, this, [=, this](int index) {
        CACHE.needRestart = true;
    });
    //
    ui->font->addItems(QFontDatabase::families());
    ui->font->setCurrentText(QApplication::font().family());
    connect(ui->font, &QComboBox::currentTextChanged, this, [=, this](const QString &fontName) {
        QFont font = QApplication::font();
        font.setFamily(fontName);
        QApplication::setFont(font);
    });
    //
    ui->theme->addItems(QStyleFactory::keys());
    ui->theme->setCurrentText(NekoGui::dataStore->theme); // QApplication::style()->name() BUG?
    connect(ui->theme, &QComboBox::currentTextChanged, this, [=, this](const QString &theme) {
        QApplication::setStyle(theme);
    });

    // Subscription

    ui->user_agent->setText(NekoGui::dataStore->user_agent);
    ui->user_agent->setPlaceholderText(NekoGui::dataStore->GetUserAgent(true));
    D_LOAD_BOOL(sub_use_proxy)
    D_LOAD_BOOL(sub_insecure)
    D_LOAD_INT_ENABLE(sub_auto_update, sub_auto_update_enable)

    // Inbound

    refresh_auth();
    D_LOAD_COMBO_STRING(inbound_address)
    D_LOAD_INT(inbound_port)
    CACHE.custom_inbound = NekoGui::dataStore->custom_inbound;

    ui->tun_stack->addItems(Preset::SingBox::TunStack);
    ui->tun_stack->setCurrentText(NekoGui::dataStore->tun_stack);
    ui->tun_mtu->setCurrentText(Int2String(NekoGui::dataStore->tun_mtu));
    ui->tun_ipv6->setChecked(NekoGui::dataStore->tun_ipv6);
    ui->tun_strict_route->setChecked(NekoGui::dataStore->tun_strict_route);

    // Core

    CACHE.extraCore = QString2QJsonObject(NekoGui::dataStore->extraCore->core_map);
    if (!CACHE.extraCore.contains("naive")) CACHE.extraCore.insert("naive", "");
    if (!CACHE.extraCore.contains("hysteria")) CACHE.extraCore.insert("hysteria", "");
    if (!CACHE.extraCore.contains("hysteria2")) CACHE.extraCore.insert("hysteria2", "");
    if (!CACHE.extraCore.contains("tuic")) CACHE.extraCore.insert("tuic", "");
    //
    auto extra_core_layout = ui->extra_core_box_scrollAreaWidgetContents->layout();
    for (const auto &s: CACHE.extraCore.keys()) {
        extra_core_layout->addWidget(new ExtraCoreWidget(&CACHE.extraCore, s));
    }
    //
    connect(ui->extra_core_add, &QPushButton::clicked, this, [=, this] {
        bool ok;
        auto s = QInputDialog::getText(this, tr("Add"), tr("Please input the core name."), QLineEdit::Normal, "", &ok).trimmed();
        if (s.isEmpty() || !ok) return;
        if (CACHE.extraCore.contains(s)) return;
        extra_core_layout->addWidget(new ExtraCoreWidget(&CACHE.extraCore, s));
        CACHE.extraCore.insert(s, "");
    });
    connect(ui->extra_core_del, &QPushButton::clicked, this, [=, this] {
        bool ok;
        auto s = QInputDialog::getItem(this, tr("Delete"), tr("Please select the core name."), CACHE.extraCore.keys(), 0, false, &ok);
        if (s.isEmpty() || !ok) return;
        for (int i = 0; i < extra_core_layout->count(); i++) {
            auto item = extra_core_layout->itemAt(i);
            auto ecw = dynamic_cast<ExtraCoreWidget *>(item->widget());
            if (ecw != nullptr && ecw->coreName == s) {
                ecw->deleteLater();
                CACHE.extraCore.remove(s);
                return;
            }
        }
    });

    // Log
    ui->log_disabled->setChecked(NekoGui::dataStore->log_disabled);
    ui->log_timestamp->setChecked(NekoGui::dataStore->log_timestamp);
    ui->log_level->addItems(Preset::SingBox::LogLevel);
    ui->log_level->setCurrentText(NekoGui::dataStore->log_level);

    // Clash API
    ui->clash_api_external_controller->setText(NekoGui::dataStore->clash_api_external_controller);
    ui->clash_api_dashboard->setCurrentText(NekoGui::dataStore->clash_api_dashboard);
    ui->clash_api_secret->setText(NekoGui::dataStore->clash_api_secret);

    // NTP
    ui->ntp_enabled->setChecked(NekoGui::dataStore->ntp_enabled);
    ui->ntp_server->setText(NekoGui::dataStore->ntp_server);
    ui->ntp_server_port->setText(Int2String(NekoGui::dataStore->ntp_server_port));
    ui->ntp_interval->setCurrentText(NekoGui::dataStore->ntp_interval);

    // Certificate
    ui->certificate_store->addItems(Preset::SingBox::CertificateStore);
    ui->certificate_store->setCurrentText(NekoGui::dataStore->certificate_store);
    ui->certificate->setText(NekoGui::dataStore->certificate);
    ui->certificate_path->setText(NekoGui::dataStore->certificate_path);
    ui->certificate_directory_path->setText(NekoGui::dataStore->certificate_directory_path);

    // Security

    ui->utlsFingerprint->addItems(Preset::SingBox::UtlsFingerPrint);

    D_LOAD_BOOL(skip_cert)
    ui->utlsFingerprint->setCurrentText(NekoGui::dataStore->utlsFingerprint);
}

DialogBasicSettings::~DialogBasicSettings() {
    delete ui;
}

void DialogBasicSettings::accept() {
    // Common

    D_SAVE_INT(test_concurrent)
    D_SAVE_INT(test_download_timeout)
    D_SAVE_STRING(test_latency_url)
    D_SAVE_STRING(test_download_url)
    D_SAVE_BOOL(old_share_link_format)

#ifdef Q_OS_WIN
    NekoGui::dataStore->system_proxy_format = ui->system_proxy_format->currentText();
#endif

    // Style

    NekoGui::dataStore->language = ui->language->currentData().toString();
    NekoGui::dataStore->traffic_loop_interval = ui->rfsh_r->currentData().toInt();

    NekoGui::dataStore->font = ui->font->currentText();
    NekoGui::dataStore->theme = ui->theme->currentText();
    D_SAVE_BOOL(check_include_pre)
    D_SAVE_BOOL(start_minimal)

    D_SAVE_INT(max_log_line)
    MainWindow::instance()->updateLogMaxLines();
    AutoRun_SetEnabled(ui->launch_at_startup->isChecked());

    // Subscription

    NekoGui::dataStore->user_agent = ui->user_agent->text();
    D_SAVE_BOOL(sub_use_proxy)
    D_SAVE_BOOL(sub_insecure)
    D_SAVE_INT_ENABLE(sub_auto_update, sub_auto_update_enable)
    MainWindow::instance()->resetAutoUpdateSubscription(NekoGui::dataStore->sub_auto_update);

    // Inbound
    D_SAVE_COMBO_STRING(inbound_address)
    D_SAVE_INT(inbound_port)
    NekoGui::dataStore->custom_inbound = CACHE.custom_inbound;

    NekoGui::dataStore->tun_stack = ui->tun_stack->currentText();
    NekoGui::dataStore->tun_mtu = ui->tun_mtu->currentText().toInt();
    NekoGui::dataStore->tun_ipv6 = ui->tun_ipv6->isChecked();
    NekoGui::dataStore->tun_strict_route = ui->tun_strict_route->isChecked();

    // Core
    NekoGui::dataStore->extraCore->core_map = QJsonObject2QString(CACHE.extraCore, true);

    // Log
    NekoGui::dataStore->log_disabled = ui->log_disabled->isChecked();
    NekoGui::dataStore->log_timestamp = ui->log_timestamp->isChecked();
    NekoGui::dataStore->log_level = ui->log_level->currentText();

    // Clash API
    NekoGui::dataStore->clash_api_external_controller = ui->clash_api_external_controller->text();
    NekoGui::dataStore->clash_api_dashboard = ui->clash_api_dashboard->currentText();
    NekoGui::dataStore->clash_api_secret = ui->clash_api_secret->text();

    // NTP
    NekoGui::dataStore->ntp_enabled = ui->ntp_enabled->isChecked();
    NekoGui::dataStore->ntp_server = ui->ntp_server->text();
    NekoGui::dataStore->ntp_server_port = ui->ntp_server_port->text().toInt();
    NekoGui::dataStore->ntp_interval = ui->ntp_interval->currentText();

    // Certificate
    NekoGui::dataStore->certificate_store = ui->certificate_store->currentText();
    NekoGui::dataStore->certificate = ui->certificate->text();
    NekoGui::dataStore->certificate_path = ui->certificate_path->text();
    NekoGui::dataStore->certificate_directory_path = ui->certificate_directory_path->text();

    // Security

    D_SAVE_BOOL(skip_cert)
    NekoGui::dataStore->utlsFingerprint = ui->utlsFingerprint->currentText();

    QStringList str{"UpdateDataStore"};
    if (CACHE.needRestart) str << "NeedRestart";
    MW_dialog_message(Dialog_DialogBasicSettings, str.join(","));
    QDialog::accept();
}

// slots

void DialogBasicSettings::refresh_auth() {
    ui->inbound_auth->setText({});
    if (NekoGui::dataStore->inbound_auth->NeedAuth()) {
        ui->inbound_auth->setIcon(QIcon::fromTheme("system-lock-screen"));
    } else {
        ui->inbound_auth->setIcon({});
    }
}

void DialogBasicSettings::on_set_custom_icon_clicked() {
    auto title = ui->set_custom_icon->text();

    QDialog dlg(this);
    dlg.setWindowTitle(title);
    auto *gridLayout = new QGridLayout(&dlg);

    auto *btnIcon1 = new QPushButton(&dlg);
    btnIcon1->setIcon(QIcon(":/icons/nekobox.png"));
    btnIcon1->setIconSize(QSize(128, 128));

    auto *btnIcon2 = new QPushButton(&dlg);
    btnIcon2->setIcon(QIcon(":/icons/nekoray.png"));
    btnIcon2->setIconSize(QSize(128, 128));

    auto *btnSelect = new QPushButton(tr("Select"), &dlg);
    auto *btnCancel = new QPushButton(tr("Cancel"), &dlg);

    gridLayout->addWidget(btnIcon1, 0, 0);
    gridLayout->addWidget(btnIcon2, 0, 1);
    gridLayout->addWidget(btnSelect, 1, 0);
    gridLayout->addWidget(btnCancel, 1, 1);

    connect(btnIcon1, &QPushButton::clicked, &dlg, [&] { dlg.done(1); });
    connect(btnIcon2, &QPushButton::clicked, &dlg, [&] { dlg.done(2); });
    connect(btnCancel, &QPushButton::clicked, &dlg, [&] { dlg.done(0); });

    QString selectedFileName;
    connect(btnSelect, &QPushButton::clicked, &dlg, [&] {
        selectedFileName = QFileDialog::getOpenFileName(&dlg, QObject::tr("Select"), QDir::currentPath(),
                                                        "*.png", nullptr, QFileDialog::Option::ReadOnly);
        if (!selectedFileName.isEmpty()) {
            dlg.done(3);
        }
    });

    int result = dlg.exec();

    if (result == 1) {
        NekoGui::dataStore->icon_path = ":/icons/nekobox.png";
    } else if (result == 2) {
        NekoGui::dataStore->icon_path = ":/icons/nekoray.png";
    } else if (result == 3) {
        QImage img(selectedFileName);
        if (img.isNull() || img.height() != img.width()) {
            QMessageBox::warning(this, title, tr("Please select a valid square image."));
            return;
        }
        NekoGui::dataStore->icon_path = selectedFileName;
    } else {
        return;
    }
    MW_dialog_message(Dialog_DialogBasicSettings, "UpdateIcon");
}

void DialogBasicSettings::on_inbound_auth_clicked() {
    auto w = new QDialog(this);
    w->setWindowTitle(tr("Inbound Auth"));
    auto layout = new QGridLayout;
    w->setLayout(layout);
    //
    auto user_l = new QLabel(tr("Username"));
    auto pass_l = new QLabel(tr("Password"));
    auto user = new QLineEdit;
    auto pass = new QLineEdit;
    user->setText(NekoGui::dataStore->inbound_auth->username);
    pass->setText(NekoGui::dataStore->inbound_auth->password);
    //
    layout->addWidget(user_l, 0, 0);
    layout->addWidget(user, 0, 1);
    layout->addWidget(pass_l, 1, 0);
    layout->addWidget(pass, 1, 1);
    auto box = new QDialogButtonBox;
    box->setOrientation(Qt::Horizontal);
    box->setStandardButtons(QDialogButtonBox::Cancel | QDialogButtonBox::Ok);
    connect(box, &QDialogButtonBox::accepted, w, [=, this] {
        NekoGui::dataStore->inbound_auth->username = user->text();
        NekoGui::dataStore->inbound_auth->password = pass->text();
        MW_dialog_message(Dialog_DialogBasicSettings, "UpdateDataStore");
        w->accept();
    });
    connect(box, &QDialogButtonBox::rejected, w, &QDialog::reject);
    layout->addWidget(box, 2, 1);
    //
    w->exec();
    w->deleteLater();
    refresh_auth();
}
