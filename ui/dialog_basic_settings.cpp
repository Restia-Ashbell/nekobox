#include "dialog_basic_settings.h"
#include "ui_dialog_basic_settings.h"

#include "3rdparty/qv2ray/v2/ui/widgets/editors/w_JsonEditor.hpp"
#include "fmt/Preset.hpp"
#include "ui/mainwindow.h"
#include "ui/Icon.hpp"
#include "main/GuiUtils.hpp"

#include <QStyleFactory>
#include <QFileDialog>
#include <QFontDatabase>
#include <QInputDialog>
#include <QMessageBox>
#include <QTimer>

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
    //
    if (NekoGui::dataStore->traffic_loop_interval == 500) {
        ui->rfsh_r->setCurrentIndex(0);
    } else if (NekoGui::dataStore->traffic_loop_interval == 1000) {
        ui->rfsh_r->setCurrentIndex(1);
    } else if (NekoGui::dataStore->traffic_loop_interval == 2000) {
        ui->rfsh_r->setCurrentIndex(2);
    } else if (NekoGui::dataStore->traffic_loop_interval == 3000) {
        ui->rfsh_r->setCurrentIndex(3);
    } else if (NekoGui::dataStore->traffic_loop_interval == 5000) {
        ui->rfsh_r->setCurrentIndex(4);
    } else {
        ui->rfsh_r->setCurrentIndex(5);
    }
    //
    if (NekoGui::dataStore->language == "zh_CN") {
        ui->language->setCurrentIndex(1);
    } else if (NekoGui::dataStore->language == "fa_IR") {
        ui->language->setCurrentIndex(2);
    } else if (NekoGui::dataStore->language == "ru_RU") {
        ui->language->setCurrentIndex(3);
    } else {
        ui->language->setCurrentIndex(0);
    }
    connect(ui->language, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, [=, this](int index) {
        CACHE.needRestart = true;
    });
    //
    ui->font->addItems(QFontDatabase::families());
    ui->font->setCurrentText(QApplication::font().family());
    connect(ui->font, static_cast<void (QComboBox::*)(const QString &)>(&QComboBox::currentTextChanged), this, [=, this](const QString &fontName) {
        QFont currentFont = QApplication::font();
        currentFont.setFamily(fontName);
        QApplication::setFont(currentFont);
    });
    //
    ui->theme->addItems(QStyleFactory::keys());
    ui->theme->setCurrentText(NekoGui::dataStore->theme);
    connect(ui->theme, &QComboBox::currentTextChanged, this, [=, this](const QString &theme) {
        QApplication::setStyle(theme);
    });

    // Subscription

    ui->user_agent->setText(NekoGui::dataStore->user_agent);
    ui->user_agent->setPlaceholderText(NekoGui::dataStore->GetUserAgent(true));
    D_LOAD_BOOL(sub_use_proxy)
    D_LOAD_BOOL(sub_clear)
    D_LOAD_BOOL(sub_insecure)
    D_LOAD_INT_ENABLE(sub_auto_update, sub_auto_update_enable)

    // Inbound

    refresh_auth();
    D_LOAD_STRING(inbound_address)
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
        auto s = QInputDialog::getText(nullptr, tr("Add"), tr("Please input the core name."), QLineEdit::Normal, "", &ok).trimmed();
        if (s.isEmpty() || !ok) return;
        if (CACHE.extraCore.contains(s)) return;
        extra_core_layout->addWidget(new ExtraCoreWidget(&CACHE.extraCore, s));
        CACHE.extraCore.insert(s, "");
    });
    connect(ui->extra_core_del, &QPushButton::clicked, this, [=, this] {
        bool ok;
        auto s = QInputDialog::getItem(nullptr, tr("Delete"), tr("Please select the core name."), CACHE.extraCore.keys(), 0, false, &ok);
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

    switch (ui->language->currentIndex()) {
        case 1: NekoGui::dataStore->language = "zh_CN"; break;
        case 2: NekoGui::dataStore->language = "fa_IR"; break;
        case 3: NekoGui::dataStore->language = "ru_RU"; break;
        default: NekoGui::dataStore->language = "en";
    }
    NekoGui::dataStore->font = ui->font->currentText();
    NekoGui::dataStore->theme = ui->theme->currentText();
    D_SAVE_BOOL(check_include_pre)
    D_SAVE_BOOL(start_minimal)

    int oldValue = NekoGui::dataStore->max_log_line;
    D_SAVE_INT(max_log_line)
    if (NekoGui::dataStore->max_log_line != oldValue) {
        MainWindow::instance()->updateLogMaxLines();
    }

    if (ui->rfsh_r->currentIndex() == 0) {
        NekoGui::dataStore->traffic_loop_interval = 500;
    } else if (ui->rfsh_r->currentIndex() == 1) {
        NekoGui::dataStore->traffic_loop_interval = 1000;
    } else if (ui->rfsh_r->currentIndex() == 2) {
        NekoGui::dataStore->traffic_loop_interval = 2000;
    } else if (ui->rfsh_r->currentIndex() == 3) {
        NekoGui::dataStore->traffic_loop_interval = 3000;
    } else if (ui->rfsh_r->currentIndex() == 4) {
        NekoGui::dataStore->traffic_loop_interval = 5000;
    } else {
        NekoGui::dataStore->traffic_loop_interval = 0;
    }

    // Subscription

    NekoGui::dataStore->user_agent = ui->user_agent->text();
    D_SAVE_BOOL(sub_use_proxy)
    D_SAVE_BOOL(sub_clear)
    D_SAVE_BOOL(sub_insecure)
    D_SAVE_INT_ENABLE(sub_auto_update, sub_auto_update_enable)
    MainWindow::instance()->resetAutoUpdateSubscription(NekoGui::dataStore->sub_auto_update);

    // Inbound
    D_SAVE_STRING(inbound_address)
    D_SAVE_INT(inbound_port)
    NekoGui::dataStore->custom_inbound = CACHE.custom_inbound;

    auto mtu = ui->tun_mtu->currentText().toInt();
    if (mtu > 10000 || mtu < 1000) mtu = 9000;
    NekoGui::dataStore->tun_stack = ui->tun_stack->currentText();
    NekoGui::dataStore->tun_mtu = mtu;
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
    QString user_icon_path = "./" + software_name.toLower() + ".png";
    auto c = QMessageBox::question(this, title, tr("Please select a PNG file."),
                                   QMessageBox::Open | QMessageBox::Reset | QMessageBox::Cancel, QMessageBox::Cancel);
    if (c == QMessageBox::Open) {
        auto fn = QFileDialog::getOpenFileName(this, QObject::tr("Select"), QDir::currentPath(),
                                               "*.png", nullptr, QFileDialog::Option::ReadOnly);
        QImage img(fn);
        if (img.isNull() || img.height() != img.width()) {
            MessageBoxWarning(title, tr("Please select a valid square image."));
            return;
        }
        QFile::remove(user_icon_path);
        QFile::copy(fn, user_icon_path);
    } else if (c == QMessageBox::Reset) {
        QFile::remove(user_icon_path);
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
