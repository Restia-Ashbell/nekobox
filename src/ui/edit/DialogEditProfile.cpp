#include "ui/edit/DialogEditProfile.hpp"
#include "ui_DialogEditProfile.h"

#include <QInputDialog>

#include "3rdparty/qv2ray/v2/ui/widgets/editors/w_JsonEditor.hpp"
#include "common/GuiUtils.hpp"
#include "protocol/Includes.hpp"
#include "protocol/Preset.hpp"
#include "ui/edit/EditAnyTLS.hpp"
#include "ui/edit/EditChain.hpp"
#include "ui/edit/EditCustom.hpp"
#include "ui/edit/EditNaive.hpp"
#include "ui/edit/EditQUIC.hpp"
#include "ui/edit/EditSSH.hpp"
#include "ui/edit/EditShadowSocks.hpp"
#include "ui/edit/EditShadowSocksR.hpp"
#include "ui/edit/EditSocksHttp.hpp"
#include "ui/edit/EditTrojanVLESS.hpp"
#include "ui/edit/EditVMess.hpp"
#include "ui/edit/EditWireGuard.hpp"

#define LOAD_TYPE(a) ui->type->addItem(NekoGui::ProfileManager::NewProxyEntity(a)->bean->DisplayType(), a);

DialogEditProfile::DialogEditProfile(const QString &_type, int profileOrGroupId, QWidget *parent)
    : QDialog(parent), ui(new Ui::DialogEditProfile) {
    // setup UI
    ui->setupUi(this);
    ui->dialog_layout->setAlignment(ui->left, Qt::AlignTop);

    // network changed
    ui->network->addItems(Preset::SingBox::V2RayTransport);
    connect(ui->network, &QComboBox::currentTextChanged, this, [base = ui->network_box->title(), this](const QString &txt) {
        ui->network_box->setTitle(base.arg(txt));
        // 传输设置
        if (txt == "ws" || txt == "http" || txt == "httpupgrade") {
            ui->path->setVisible(true);
            ui->path_l->setVisible(true);
            ui->host->setVisible(true);
            ui->host_l->setVisible(true);
        } else if (txt == "grpc") {
            ui->path->setVisible(true);
            ui->path_l->setVisible(true);
            ui->host->setVisible(false);
            ui->host_l->setVisible(false);
        } else {
            ui->path->setVisible(false);
            ui->path_l->setVisible(false);
            ui->host->setVisible(false);
            ui->host_l->setVisible(false);
        }
        // 传输设置 ED
        if (txt == "ws") {
            ui->ws_early_data_length->setVisible(true);
            ui->ws_early_data_length_l->setVisible(true);
            ui->ws_early_data_name->setVisible(true);
            ui->ws_early_data_name_l->setVisible(true);
        } else {
            ui->ws_early_data_length->setVisible(false);
            ui->ws_early_data_length_l->setVisible(false);
            ui->ws_early_data_name->setVisible(false);
            ui->ws_early_data_name_l->setVisible(false);
        }
        // 传输设置 是否可见
        int networkBoxVisible = 0;
        for (auto label: ui->network_box->findChildren<QLabel *>()) {
            if (!label->isHidden()) networkBoxVisible++;
        }
        ui->network_box->setVisible(networkBoxVisible);
        ui->right_all_w->setVisible(!(ui->network_box->isHidden() && ui->security_box->isHidden() && ui->multiplex_box->isHidden()));
        ADJUST_SIZE
    });
    emit ui->network->currentTextChanged(ui->network->currentText());

    // security changed
    ui->utlsFingerprint->addItems(Preset::SingBox::UtlsFingerPrint);
    connect(ui->security, &QComboBox::currentTextChanged, this, [=, this](const QString &txt) {
        ui->security_box->setVisible(txt == "tls");
        ui->right_all_w->setVisible(!(ui->network_box->isHidden() && ui->security_box->isHidden() && ui->multiplex_box->isHidden()));
        ADJUST_SIZE
    });
    emit ui->security->currentTextChanged(ui->security->currentText());

    // multiplex changed
    ui->multiplex_protocol->addItems(Preset::SingBox::MultiplexProtocol);
    connect(ui->multiplex, &QComboBox::currentIndexChanged, this, [=, this](int index) {
        ui->multiplex_box->setVisible(index == 1);
        ui->right_all_w->setVisible(!(ui->network_box->isHidden() && ui->security_box->isHidden() && ui->multiplex_box->isHidden()));
        ADJUST_SIZE
    });
    emit ui->multiplex->currentIndexChanged(ui->multiplex->currentIndex());

    // 确定模式和 ent
    newEnt = _type != "";
    if (newEnt) {
        this->groupId = profileOrGroupId;
        this->type = _type;

        // load type to combo box
        LOAD_TYPE("socks")
        LOAD_TYPE("http")
        LOAD_TYPE("shadowsocks")
        LOAD_TYPE("shadowsocksr")
        LOAD_TYPE("trojan")
        LOAD_TYPE("vmess")
        LOAD_TYPE("vless")
        LOAD_TYPE("naive")
        LOAD_TYPE("hysteria")
        LOAD_TYPE("hysteria2")
        LOAD_TYPE("tuic")
        LOAD_TYPE("anytls")
        LOAD_TYPE("ssh")
        LOAD_TYPE("wireguard")
        ui->type->addItem(tr("Custom (%1 outbound)").arg(software_core_name), "internal");
        ui->type->addItem(tr("Custom (%1 config)").arg(software_core_name), "internal-full");
        ui->type->addItem(tr("Custom (Extra Core)"), "custom");
        LOAD_TYPE("chain")

        // type changed
        connect(ui->type, &QComboBox::currentIndexChanged, this, [this](int index) {
            this->type = ui->type->itemData(index).toString();
            typeSelected();
        });
    } else {
        this->ent = NekoGui::profileManager->GetProfile(profileOrGroupId);
        if (this->ent == nullptr) return;
        this->type = ent->type;
        ui->type->setVisible(false);
        ui->type_l->setVisible(false);
    }

    typeSelected();
    show();
}

DialogEditProfile::~DialogEditProfile() {
    delete ui;
}

void DialogEditProfile::typeSelected() {
    QString customType;

    if (type == "socks" || type == "http") {
        innerEditor = new EditSocksHttp(this);
    } else if (type == "shadowsocks") {
        innerEditor = new EditShadowSocks(this);
    } else if (type == "shadowsocksr") {
        innerEditor = new EditShadowSocksR(this);
    } else if (type == "chain") {
        innerEditor = new EditChain(this);
    } else if (type == "vmess") {
        innerEditor = new EditVMess(this);
    } else if (type == "trojan" || type == "vless") {
        innerEditor = new EditTrojanVLESS(this);
    } else if (type == "naive") {
        innerEditor = new EditNaive(this);
    } else if (type == "hysteria" || type == "hysteria2" || type == "tuic") {
        innerEditor = new EditQUIC(this);
    } else if (type == "anytls") {
        innerEditor = new EditAnyTLS(this);
    } else if (type == "ssh") {
        innerEditor = new EditSSH(this);
    } else if (type == "wireguard") {
        innerEditor = new EditWireGuard(this);
    } else if (type == "custom" || type == "internal" || type == "internal-full") {
        auto _innerWidget = new EditCustom(this);
        innerEditor = _innerWidget;
        customType = newEnt ? type : ent->Bean<NekoGui_fmt::CustomBean>()->core;
        if (customType != "custom") _innerWidget->preset_core = customType;
        type = "custom";
    } else {
        MessageBoxWarning(type, "Wrong type");
        return;
    }

    if (newEnt) {
        this->ent = NekoGui::ProfileManager::NewProxyEntity(type);
        this->ent->gid = groupId;
    }

    // hide some widget
    auto showAddressPort = type != "chain" && customType != "internal" && customType != "internal-full";
    ui->address->setVisible(showAddressPort);
    ui->address_l->setVisible(showAddressPort);
    ui->port->setVisible(showAddressPort);
    ui->port_l->setVisible(showAddressPort);

    // 右边 stream
    if (auto stream = ent->bean->_get<NekoGui_fmt::V2rayStreamSettings>("stream")) {
        ui->network->setCurrentText(stream->network);
        ui->security->setCurrentText(stream->security);
        ui->packet_encoding->setCurrentText(stream->packet_encoding);
        ui->path->setText(stream->path);
        ui->host->setText(stream->host);
        ui->sni->setText(stream->sni);
        ui->alpn->setText(stream->alpn);
        if (newEnt) {
            ui->utlsFingerprint->setCurrentText(NekoGui::dataStore->utlsFingerprint);
        } else {
            ui->utlsFingerprint->setCurrentText(stream->utlsFingerprint);
        }
        ui->insecure->setChecked(stream->allow_insecure);
        ui->ech_enabled->setChecked(stream->ech_enabled);
        ui->disable_sni->setChecked(stream->disable_sni);
        ui->ws_early_data_name->setText(stream->ws_early_data_name);
        ui->ws_early_data_length->setText(Int2String(stream->ws_early_data_length));
        ui->reality_pbk->setText(stream->reality_pbk);
        ui->reality_sid->setText(stream->reality_sid);
        ui->tls_fragment->setChecked(stream->tls_fragment);
        ui->tls_record_fragment->setChecked(stream->tls_record_fragment);
        CACHE.certificate = stream->certificate;
        CACHE.ech = stream->ech;
    } else {
        ui->network_box->setVisible(false);
        ui->security_box->setVisible(false);
    }

    if (auto multiplex = ent->bean->_get<NekoGui_fmt::MultiplexSettings>("multiplex")) {
        ui->multiplex->setCurrentIndex(multiplex->enabled ? 1 : 0);
        ui->multiplex_padding->setChecked(multiplex->padding);
        ui->multiplex_protocol->setCurrentText(multiplex->protocol);
        ui->multiplex_max_streams->setText(Int2String(multiplex->max_streams));
        ui->brutal_up->setText(Int2String(multiplex->brutal_up));
        ui->brutal_down->setText(Int2String(multiplex->brutal_down));
    } else {
        ui->multiplex_box->setVisible(false);
    }

    // left: custom
    CACHE.custom_config = ent->bean->custom_config;
    CACHE.custom_outbound = ent->bean->custom_outbound;
    bool show_custom_config = true;
    bool show_custom_outbound = true;
    if (type == "chain") {
        show_custom_outbound = false;
    } else if (type == "custom") {
        if (customType == "internal") {
            show_custom_outbound = false;
        } else if (customType == "internal-full") {
            show_custom_outbound = false;
            show_custom_config = false;
        }
    }
    ui->custom_box->setVisible(show_custom_outbound);
    ui->custom_global_box->setVisible(show_custom_config);

    // 左边 bean
    if (auto old = ui->bean->layout()->takeAt(0)) {
        delete old->widget();
        delete old;
    }
    auto innerWidget = dynamic_cast<QWidget *>(innerEditor);
    innerWidget->layout()->setContentsMargins(0, 0, 0, 0);
    ui->bean->layout()->addWidget(innerWidget);
    ui->bean->setTitle(ent->bean->DisplayType());

    // 左边 bean inner editor
    innerEditor->get_edit_dialog = [&]() { return this; };
    innerEditor->editor_cache_updated = [=, this] { editor_cache_updated_impl(); };
    innerEditor->onStart(ent);

    // 左边 common
    ui->name->setText(ent->bean->name);
    ui->address->setText(ent->bean->serverAddress);
    ui->port->setText(Int2String(ent->bean->serverPort));
    ui->port->setValidator(QRegExpValidator_Number);

    // 星号
    ADD_ASTERISK(this)

    // 设置 for NekoBox
    if (type == "vmess" || type == "vless") {
        ui->packet_encoding->setVisible(true);
        ui->packet_encoding_l->setVisible(true);
    } else {
        ui->packet_encoding->setVisible(false);
        ui->packet_encoding_l->setVisible(false);
    }
    if (type == "vmess" || type == "vless" || type == "trojan") {
        ui->network_l->setVisible(true);
        ui->network->setVisible(true);
    } else {
        ui->network_l->setVisible(false);
        ui->network->setVisible(false);
    }
    if (type == "vmess" || type == "vless" || type == "trojan" || type == "http") {
        ui->security->setVisible(true);
        ui->security_l->setVisible(true);
    } else {
        ui->security->setVisible(false);
        ui->security_l->setVisible(false);
    }
    if (type == "vmess" || type == "vless" || type == "trojan" || type == "shadowsocks") {
        ui->multiplex->setVisible(true);
        ui->multiplex_l->setVisible(true);
    } else {
        ui->multiplex->setVisible(false);
        ui->multiplex_l->setVisible(false);
    }
    // 设置 是否可见
    int streamBoxVisible = 0;
    for (auto label: ui->stream_box->findChildren<QLabel *>()) {
        if (!label->isHidden()) streamBoxVisible++;
    }
    ui->stream_box->setVisible(streamBoxVisible);

    ui->right_all_w->setVisible(!(ui->network_box->isHidden() && ui->security_box->isHidden() && ui->multiplex_box->isHidden()));

    editor_cache_updated_impl();
    ADJUST_SIZE
}

bool DialogEditProfile::onEnd() {
    // bean
    if (!innerEditor->onEnd()) {
        return false;
    }

    // 左边
    ent->bean->name = ui->name->text();
    ent->bean->serverAddress = ui->address->text().trimmed();
    ent->bean->serverPort = ui->port->text().toInt();

    // 右边 stream
    if (auto stream = ent->bean->_get<NekoGui_fmt::V2rayStreamSettings>("stream")) {
        stream->network = ui->network->currentText();
        stream->security = ui->security->currentText();
        stream->packet_encoding = ui->packet_encoding->currentText();
        stream->path = ui->path->text();
        stream->host = ui->host->text();
        stream->sni = ui->sni->text();
        stream->alpn = ui->alpn->text();
        stream->utlsFingerprint = ui->utlsFingerprint->currentText();
        stream->allow_insecure = ui->insecure->isChecked();
        stream->ech_enabled = ui->ech_enabled->isChecked();
        stream->disable_sni = ui->disable_sni->isChecked();
        stream->ws_early_data_name = ui->ws_early_data_name->text();
        stream->ws_early_data_length = ui->ws_early_data_length->text().toInt();
        stream->reality_pbk = ui->reality_pbk->text();
        stream->reality_sid = ui->reality_sid->text();
        stream->tls_fragment = ui->tls_fragment->isChecked();
        stream->tls_record_fragment = ui->tls_record_fragment->isChecked();
        stream->certificate = CACHE.certificate;
        stream->ech = CACHE.ech;
    }

    if (auto multiplex = ent->bean->_get<NekoGui_fmt::MultiplexSettings>("multiplex")) {
        multiplex->enabled = ui->multiplex->currentIndex() == 1;
        multiplex->padding = ui->multiplex_padding->isChecked();
        multiplex->protocol = ui->multiplex_protocol->currentText();
        multiplex->max_streams = ui->multiplex_max_streams->text().toInt();
        multiplex->brutal_up = ui->brutal_up->text().toInt();
        multiplex->brutal_down = ui->brutal_down->text().toInt();
    }

    // cached custom
    ent->bean->custom_outbound = CACHE.custom_outbound;
    ent->bean->custom_config = CACHE.custom_config;

    return true;
}

void DialogEditProfile::accept() {
    // save to ent
    if (!onEnd()) {
        return;
    }

    // finish
    QStringList msg = {"accept"};

    if (newEnt) {
        auto ok = NekoGui::profileManager->AddProfile(ent);
        if (!ok) {
            MessageBoxWarning("???", "id exists");
        }
    } else {
        auto changed = ent->Save();
        if (changed && NekoGui::dataStore->started_id == ent->id) msg << "restart";
    }

    MW_dialog_message(Dialog_DialogEditProfile, msg.join(","));
    QDialog::accept();
}

// cached editor (dialog)

void DialogEditProfile::editor_cache_updated_impl() {
    if (CACHE.certificate.isEmpty()) {
        ui->certificate_edit->setText(tr("Not set"));
    } else {
        ui->certificate_edit->setText(tr("Already set"));
    }
    if (CACHE.ech.isEmpty()) {
        ui->ech_edit->setText(tr("Not set"));
    } else {
        ui->ech_edit->setText(tr("Already set"));
    }
    if (CACHE.custom_outbound.isEmpty()) {
        ui->custom_outbound_edit->setText(tr("Not set"));
    } else {
        ui->custom_outbound_edit->setText(tr("Already set"));
    }
    if (CACHE.custom_config.isEmpty()) {
        ui->custom_config_edit->setText(tr("Not set"));
    } else {
        ui->custom_config_edit->setText(tr("Already set"));
    }

    // CACHE macro
    for (auto a: innerEditor->get_editor_cached()) {
        if (a.second.isEmpty()) {
            a.first->setText(tr("Not set"));
        } else {
            a.first->setText(tr("Already set"));
        }
    }
}

void DialogEditProfile::on_custom_outbound_edit_clicked() {
    C_EDIT_JSON_ALLOW_EMPTY(custom_outbound)
    editor_cache_updated_impl();
}

void DialogEditProfile::on_custom_config_edit_clicked() {
    C_EDIT_JSON_ALLOW_EMPTY(custom_config)
    editor_cache_updated_impl();
}

void DialogEditProfile::on_certificate_edit_clicked() {
    bool ok;
    auto txt = QInputDialog::getMultiLineText(this, tr("Certificate"), "", CACHE.certificate, &ok);
    if (ok) {
        CACHE.certificate = txt;
        editor_cache_updated_impl();
    }
}

void DialogEditProfile::on_ech_edit_clicked() {
    bool ok;
    auto txt = QInputDialog::getMultiLineText(this, "ECH", "", CACHE.ech, &ok);
    if (ok) {
        CACHE.ech = txt;
        editor_cache_updated_impl();
    }
}
