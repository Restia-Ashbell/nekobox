#include "ui/dialog/DialogManageGroups.hpp"
#include "ui_DialogManageGroups.h"

#include <QInputDialog>
#include <QListWidgetItem>
#include <QMessageBox>

#include "profile/ProfileManager.hpp"
#include "common/GuiUtils.hpp"
#include "subscription/GroupUpdater.hpp"
#include "ui/edit/DialogEditGroup.hpp"
#include "ui/MainWindow.hpp"
#include "ui/widget/GroupItem.hpp"

DialogManageGroups::DialogManageGroups(QWidget *parent, int index) : QDialog(parent), ui(new Ui::DialogManageGroups) {
    ui->setupUi(this);

    for (auto id: NekoGui::profileManager->groupsTabOrder) {
        addGroupToList(id);
    }

    updateWindowTitle();
    connect(ui->listWidget->model(), &QAbstractItemModel::rowsInserted, this, &DialogManageGroups::updateWindowTitle);
    connect(ui->listWidget->model(), &QAbstractItemModel::rowsRemoved, this, &DialogManageGroups::updateWindowTitle);

    connect(ui->listWidget, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem *wI) {
        auto w = dynamic_cast<GroupItem *>(ui->listWidget->itemWidget(wI));
        w->on_edit_clicked();
    });

    connect(MainWindow::instance(), &MainWindow::groupUpdated, this, [this](int gid) {
        for (int i = 0; i < ui->listWidget->count(); ++i) {
            auto item = ui->listWidget->item(i);
            if (item->data(114514).toInt() == gid) {
                if (!NekoGui::profileManager->GetGroup(gid)) {
                    delete ui->listWidget->takeItem(i);
                }
                return;
            }
        }
        addGroupToList(gid);
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
