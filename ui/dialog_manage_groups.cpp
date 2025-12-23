#include "dialog_manage_groups.h"
#include "ui_dialog_manage_groups.h"

#include "db/ProfileManager.hpp"
#include "sub/GroupUpdater.hpp"
#include "main/GuiUtils.hpp"
#include "ui/widget/GroupItem.h"
#include "ui/edit/dialog_edit_group.h"

#include <QInputDialog>
#include <QListWidgetItem>
#include <QMessageBox>

DialogManageGroups::DialogManageGroups(QWidget *parent, int index) : QDialog(parent), ui(new Ui::DialogManageGroups) {
    ui->setupUi(this);

    for (auto id: NekoGui::profileManager->groupsTabOrder) {
        addGroupToList(id);
    }

    updateWindowTitle();
    connect(ui->listWidget->model(), &QAbstractItemModel::rowsInserted, this, &DialogManageGroups::updateWindowTitle);
    connect(ui->listWidget->model(), &QAbstractItemModel::rowsRemoved, this, &DialogManageGroups::updateWindowTitle);

    connect(ui->listWidget, &QListWidget::itemDoubleClicked, this, [=, this](QListWidgetItem *wI) {
        auto w = dynamic_cast<GroupItem *>(ui->listWidget->itemWidget(wI));
        emit w->edit_clicked();
    });

    if (index >= 0 && index < ui->listWidget->count()) {
        ui->listWidget->scrollToItem(ui->listWidget->item(index));
    }
}

DialogManageGroups::~DialogManageGroups() {
    delete ui;
}

void DialogManageGroups::addGroupToList(int id) {
    if (auto ent = NekoGui::profileManager->GetGroup(id)) {
        auto item = new QListWidgetItem();
        auto w = new GroupItem(this, ent, item);
        item->setData(114514, id);
        ui->listWidget->addItem(item);
        ui->listWidget->setItemWidget(item, w);
    }
}

void DialogManageGroups::updateWindowTitle() {
    setWindowTitle(QString("%1 [%2]").arg(tr("Groups")).arg(ui->listWidget->count()));
}

void DialogManageGroups::on_add_clicked() {
    auto ent = NekoGui::ProfileManager::NewGroup();
    DialogEditGroup dialog(ent, this);
    if (dialog.exec() == QDialog::Accepted) {
        NekoGui::profileManager->AddGroup(ent);
        addGroupToList(ent->id);
        MW_dialog_message(Dialog_DialogManageGroups, "refresh-1");
    }
}

void DialogManageGroups::on_update_all_clicked() {
    if (QMessageBox::question(this, tr("Confirmation"), tr("Update all subscriptions?")) == QMessageBox::StandardButton::Yes) {
        UI_update_all_groups(false);
    }
}
