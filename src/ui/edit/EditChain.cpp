#include "ui/edit/EditChain.hpp"
#include "ui_EditChain.h"

#include "ui/MainWindow.hpp"
#include "ui/widget/ProxyItem.hpp"

#include "profile/ProfileManager.hpp"
#include "protocol/ChainBean.hpp"

EditChain::EditChain(QWidget *parent) : QWidget(parent), ui(new Ui::EditChain) {
    ui->setupUi(this);
}

EditChain::~EditChain() {
    delete ui;
}

void EditChain::onStart(std::shared_ptr<NekoGui::ProxyEntity> _ent) {
    this->ent = _ent;
    auto bean = this->ent->Bean<NekoGui_fmt::ChainBean>();

    for (auto id: bean->list) {
        AddProfileToListIfExist(id);
    }
}

bool EditChain::onEnd() {
    auto bean = this->ent->Bean<NekoGui_fmt::ChainBean>();

    QList<int> idList;
    for (int i = 0; i < ui->listWidget->count(); i++) {
        idList << ui->listWidget->item(i)->data(114514).toInt();
    }
    bean->list = idList;

    return true;
}

void EditChain::on_select_profile_clicked() {
    get_edit_dialog()->hide();
    MainWindow::instance()->start_select_mode(this, [=, this](int id) {
        get_edit_dialog()->show();
        AddProfileToListIfExist(id);
    });
}

void EditChain::AddProfileToListIfExist(int profileId) {
    auto _ent = NekoGui::profileManager->GetProfile(profileId);
    if (_ent != nullptr && _ent->type != "chain") {
        auto wI = new QListWidgetItem();
        wI->setData(114514, profileId);
        auto w = new ProxyItem(this, _ent, wI);
        ui->listWidget->addItem(wI);
        ui->listWidget->setItemWidget(wI, w);
        // change button
        connect(w->get_change_button(), &QPushButton::clicked, w, [=, this] {
            get_edit_dialog()->hide();
            MainWindow::instance()->start_select_mode(w, [=, this](int newId) {
                get_edit_dialog()->show();
                ReplaceProfile(w, newId);
            });
        });
    }
}

void EditChain::ReplaceProfile(ProxyItem *w, int profileId) {
    auto _ent = NekoGui::profileManager->GetProfile(profileId);
    if (_ent != nullptr && _ent->type != "chain") {
        w->item->setData(114514, profileId);
        w->ent = _ent;
        w->refresh_data();
    }
}
