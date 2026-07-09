#include "ui/edit/EditVMess.hpp"
#include "ui_EditVMess.h"

#include "protocol/VMessBean.hpp"

#include <QUuid>

EditVMess::EditVMess(QWidget *parent) : QWidget(parent), ui(new Ui::EditVMess) {
    ui->setupUi(this);
    connect(ui->uuidgen, &QPushButton::clicked, this, [=, this] { ui->uuid->setText(QUuid::createUuid().toString(QUuid::WithoutBraces)); });
}

EditVMess::~EditVMess() {
    delete ui;
}

void EditVMess::onStart(std::shared_ptr<NekoGui::ProxyEntity> _ent) {
    this->ent = _ent;
    auto bean = this->ent->Bean<NekoGui_fmt::VMessBean>();

    ui->uuid->setText(bean->uuid);
    ui->aid->setText(Int2String(bean->aid));
    ui->security->setCurrentText(bean->security);
}

bool EditVMess::onEnd() {
    auto bean = this->ent->Bean<NekoGui_fmt::VMessBean>();

    bean->uuid = ui->uuid->text();
    bean->aid = ui->aid->text().toInt();
    bean->security = ui->security->currentText();

    return true;
}
