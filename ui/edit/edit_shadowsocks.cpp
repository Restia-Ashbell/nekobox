#include "edit_shadowsocks.h"
#include "ui_edit_shadowsocks.h"

#include "fmt/Preset.hpp"
#include "fmt/ShadowSocksBean.hpp"

EditShadowSocks::EditShadowSocks(QWidget *parent) : QWidget(parent),
                                                    ui(new Ui::EditShadowSocks) {
    ui->setupUi(this);
    ui->method->addItems(Preset::SingBox::shadownone + Preset::SingBox::shadowaead2022 + Preset::SingBox::shadowaead + Preset::SingBox::shadowstream);
}

EditShadowSocks::~EditShadowSocks() {
    delete ui;
}

void EditShadowSocks::onStart(std::shared_ptr<NekoGui::ProxyEntity> _ent) {
    this->ent = _ent;
    auto bean = this->ent->Bean<NekoGui_fmt::ShadowSocksBean>();

    ui->method->setCurrentText(bean->method);
    ui->uot->setCurrentIndex(bean->uot);
    ui->password->setText(bean->password);
    auto ssPlugin = bean->plugin.split(";");
    if (!ssPlugin.empty()) {
        ui->plugin->setCurrentText(ssPlugin[0]);
        ui->plugin_opts->setText(SubStrAfter(bean->plugin, ";"));
    }
}

bool EditShadowSocks::onEnd() {
    auto bean = this->ent->Bean<NekoGui_fmt::ShadowSocksBean>();

    bean->method = ui->method->currentText();
    bean->password = ui->password->text();
    bean->uot = ui->uot->currentIndex();
    bean->plugin = ui->plugin->currentText();
    if (!bean->plugin.isEmpty()) {
        bean->plugin += ";" + ui->plugin_opts->text();
    }

    return true;
}
