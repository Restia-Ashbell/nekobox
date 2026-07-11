#include "ui/edit/EditCustom.hpp"
#include "ui_EditCustom.h"

#include "3rdparty/qv2ray/v2/ui/widgets/editors/w_JsonEditor.hpp"
#include "profile/ConfigBuilder.hpp"
#include "profile/ProfileManager.hpp"
#include "protocol/CustomBean.hpp"
#include "protocol/Preset.hpp"

EditCustom::EditCustom(QWidget *parent) : QWidget(parent), ui(new Ui::EditCustom) {
    ui->setupUi(this);
    ui->config_simple->setPlaceholderText(
        "example:\n"
        "  server-address: \"%server_addr%:%server_port%\"\n"
        "  listen-address: \"127.0.0.1\"\n"
        "  listen-port: %socks_port%\n"
        "  host: your-domain.com\n"
        "  sni: your-domain.com\n");
}

EditCustom::~EditCustom() {
    delete ui;
}

void EditCustom::onStart(std::shared_ptr<NekoGui::ProxyEntity> _ent) {
    this->ent = _ent;
    auto bean = this->ent->Bean<NekoGui_fmt::CustomBean>();

    // load known core
    auto core_map = QString2QJsonObject(NekoGui::dataStore->extraCore->core_map);
    for (const auto &key: core_map.keys()) {
        if (key == "naive" || key == "hysteria") continue;
        ui->core->addItem(key);
    }
    if (preset_core == "internal") {
        preset_command = preset_config = "";
        ui->config_simple->setPlaceholderText(
            "{\n"
            "    \"type\": \"socks\",\n"
            "    // ...\n"
            "}");
    } else if (preset_core == "internal-full") {
        preset_command = preset_config = "";
        ui->config_simple->setPlaceholderText(
            "{\n"
            "    \"inbounds\": [],\n"
            "    \"outbounds\": []\n"
            "}");
    }

    // load core ui
    P_LOAD_COMBO_STRING(core)
    ui->command->setText(bean->command.join(" "));
    ui->config_simple->setPlainText(bean->config_simple);
    P_LOAD_COMBO_STRING(config_suffix)

    // custom external
    if (!bean->core.isEmpty()) {
        ui->core->setDisabled(true);
    } else if (!preset_core.isEmpty()) {
        bean->core = preset_core;
        ui->core->setDisabled(true);
        ui->core->setCurrentText(preset_core);
        ui->command->setText(preset_command);
        ui->config_simple->setPlainText(preset_config);
    }

    // custom internal
    if (preset_core == "internal" || preset_core == "internal-full") {
        ui->core->hide();
        if (preset_core == "internal") {
            ui->core_l->setText(tr("Outbound JSON, please read the documentation."));
        } else {
            ui->core_l->setText(tr("Please fill the complete config."));
        }
        ui->w_ext1->hide();
        ui->w_ext2->hide();
    }
}

bool EditCustom::onEnd() {
    if (ui->core->currentText().isEmpty()) {
        MessageBoxWarning(software_name, tr("Please pick a core."));
        return false;
    }

    auto bean = this->ent->Bean<NekoGui_fmt::CustomBean>();

    P_SAVE_COMBO_STRING(core)
    bean->command = ui->command->text().split(" ");
    P_SAVE_STRING_PLAIN(config_simple)
    P_SAVE_COMBO_STRING(config_suffix)

    bean->external = !(preset_core == "internal" || preset_core == "internal-full");

    return true;
}

void EditCustom::on_as_json_clicked() {
    auto editor = new JsonEditor(ui->config_simple->toPlainText(), this);
    auto result = editor->OpenEditor();
    if (!result.isEmpty()) {
        ui->config_simple->setPlainText(QJsonObject2QString(result, false));
    }
}
