#include "edit_anytls.h"
#include "ui_edit_anytls.h"

#include "fmt/AnyTLSBean.hpp"

EditAnyTLS::EditAnyTLS(QWidget *parent) : QWidget(parent), ui(new Ui::EditAnyTLS) {
    ui->setupUi(this);
}

EditAnyTLS::~EditAnyTLS() {
    delete ui;
}

void EditAnyTLS::onStart(std::shared_ptr<NekoGui::ProxyEntity> _ent) {
    this->ent = _ent;
    auto bean = this->ent->Bean<NekoGui_fmt::AnyTLSBean>();

    ui->password->setText(bean->password);
    ui->idleSessionCheckInterval->setText(bean->idleSessionCheckInterval);
    ui->idleSessionTimeout->setText(bean->idleSessionTimeout);
    ui->minIdleSession->setText(Int2String(bean->minIdleSession));
}

bool EditAnyTLS::onEnd() {
    auto bean = this->ent->Bean<NekoGui_fmt::AnyTLSBean>();

    bean->password = ui->password->text();
    bean->idleSessionCheckInterval = ui->idleSessionCheckInterval->text();
    bean->idleSessionTimeout = ui->idleSessionTimeout->text();
    bean->minIdleSession = ui->minIdleSession->text().toInt();

    return true;
}
