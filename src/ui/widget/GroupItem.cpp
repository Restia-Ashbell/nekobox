#include "ui/widget/GroupItem.hpp"
#include "ui_GroupItem.h"

#include <QMessageBox>

#include "common/GuiUtils.hpp"
#include "subscription/SubscriptionParser.hpp"
#include "subscription/SubscriptionService.hpp"
#include "ui/edit/DialogEditGroup.hpp"
#include "ui/MainWindow.hpp"


GroupItem::GroupItem(QWidget *parent, const std::shared_ptr<NekoGui::Group> &ent, QListWidgetItem *item) : QWidget(parent), ui(new Ui::GroupItem) {
    ui->setupUi(this);
    this->setLayoutDirection(Qt::LeftToRight);

    this->ent = ent;
    this->item = item;
    if (ent == nullptr) return;

    connect(MainWindow::instance(), &MainWindow::groupUpdated, this, [this](int gid) {
        if (gid == this->ent->id) refresh_data();
    });

    connect(NekoGui_sub::subService, &NekoGui_sub::SubscriptionService::taskStarted, this, [this](int gid) {
        if (gid == this->ent->id) {
            m_updating = true;
            refresh_data();
        }
    });
    connect(NekoGui_sub::subService, &NekoGui_sub::SubscriptionService::taskFinished, this, [this](int gid, const NekoGui_sub::UpdateReport &) {
        if (gid == this->ent->id) {
            m_updating = false;
            refresh_data();
        }
    });

    refresh_data();
}

GroupItem::~GroupItem() {
    delete ui;
}

void GroupItem::refresh_data() {
    ui->name->setText(ent->name);

    auto type = ent->url.isEmpty() ? tr("Basic") : tr("Subscription");
    if (ent->archive) type = tr("Archive") + " " + type;
    type += " (" + Int2String(ent->Profiles().length()) + ")";
    ui->type->setText(type);

    if (ent->url.isEmpty()) {
        ui->url->hide();
        ui->subinfo->hide();
        ui->update_sub->hide();
    } else {
        ui->url->setText(ent->url);
        if (m_updating) {
            ui->subinfo->setText(tr("Updating…"));
            ui->subinfo->setStyleSheet("color: rgb(84, 130, 255);");
            ui->subinfo->show();
        } else {
            ui->subinfo->setStyleSheet("");
            QStringList info;
            if (ent->sub_last_update != 0) {
                info << tr("Last update: %1").arg(DisplayTime(ent->sub_last_update, QLocale::ShortFormat));
            }
            if (!ent->info.isEmpty()) {
                info << NekoGui_sub::SubscriptionParser::parseSubInfo(ent->info);
            }
            if (info.isEmpty()) {
                ui->subinfo->hide();
            } else {
                ui->subinfo->setText(info.join(" | "));
                ui->subinfo->show();
            }
        }
    }
    item->setSizeHint(sizeHint());
}

void GroupItem::on_update_sub_clicked() {
    NekoGui_sub::subService->updateGroup(ent->id);
}

void GroupItem::on_edit_clicked() {
    auto dialog = new DialogEditGroup(ent, parentWidget());
    connect(dialog, &QDialog::finished, this, [=, this] {
        if (dialog->result() == QDialog::Accepted) {
            ent->Save();
            refresh_data();
            MW_dialog_message(Dialog_DialogManageGroups, "refresh" + Int2String(ent->id));
        }
        dialog->deleteLater();
    });
    dialog->show();
}

void GroupItem::on_remove_clicked() {
    if (NekoGui::profileManager->groups.size() <= 1) return;
    if (QMessageBox::question(this, tr("Confirmation"), tr("Remove %1?").arg(ent->name)) ==
        QMessageBox::StandardButton::Yes) {
        NekoGui::profileManager->DeleteGroup(ent->id);
        MW_dialog_message(Dialog_DialogManageGroups, "refresh-1");
        delete item;
    }
}
