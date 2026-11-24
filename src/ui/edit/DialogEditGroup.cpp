#include "ui/edit/DialogEditGroup.hpp"
#include "ui_DialogEditGroup.h"

#include <QClipboard>

#include "common/GuiUtils.hpp"
#include "profile/ProfileManager.hpp"
#include "ui/MainWindow.hpp"

DialogEditGroup::DialogEditGroup(const std::shared_ptr<NekoGui::Group> &ent, QWidget *parent) : QDialog(parent), ui(new Ui::DialogEditGroup) {
    ui->setupUi(this);
    this->ent = ent;

    connect(ui->type, &QComboBox::currentIndexChanged, this, [=, this](int index) {
        ui->cat_sub->setHidden(index == 0);
        ADJUST_SIZE
    });

    ui->name->setText(ent->name);
    ui->auto_update_enable->setChecked(ent->auto_update_interval > 0);
    ui->auto_update_interval->setValue(qAbs(ent->auto_update_interval));
    ui->url->setText(ent->url);
    ui->type->setCurrentIndex(ent->url.isEmpty() ? 0 : 1);
    ui->type->currentIndexChanged(ui->type->currentIndex());
    ui->cat_share->setVisible(false);

    if (ent->id >= 0) { // already a group
        ui->type->setDisabled(true);
        if (!NekoGui::profileManager->Profiles(ent->id).isEmpty()) {
            ui->cat_share->setVisible(true);
        }
    } else { // new group
        ui->front_proxy->hide();
        ui->front_proxy_l->hide();
        ui->front_proxy_clear->hide();
    }

    CACHE.front_proxy = ent->front_proxy_id;
    refresh_front_proxy();
    connect(ui->front_proxy_clear, &QPushButton::clicked, this, [=, this] {
        CACHE.front_proxy = -1;
        refresh_front_proxy();
    });

    connect(ui->copy_links, &QPushButton::clicked, this, [=, this] {
        QStringList links;
        for (const auto &[_, profile]: NekoGui::profileManager->profiles) {
            if (profile->gid != ent->id) continue;
            links += profile->bean->ToShareLink();
        }
        QApplication::clipboard()->setText(links.join("\n"));
        MessageBoxInfo(software_name, tr("Copied"));
    });
    connect(ui->copy_links_nkr, &QPushButton::clicked, this, [=, this] {
        QStringList links;
        for (const auto &[_, profile]: NekoGui::profileManager->profiles) {
            if (profile->gid != ent->id) continue;
            links += profile->bean->ToNekorayShareLink(profile->type);
        }
        QApplication::clipboard()->setText(links.join("\n"));
        MessageBoxInfo(software_name, tr("Copied"));
    });

    ADJUST_SIZE
}

DialogEditGroup::~DialogEditGroup() {
    delete ui;
}

void DialogEditGroup::accept() {
    if (ent->id >= 0) { // already a group
        if (!ent->url.isEmpty() && ui->url->text().isEmpty()) {
            MessageBoxWarning(tr("Warning"), tr("Please input URL"));
            return;
        }
    }
    ent->name = ui->name->text();
    ent->url = ui->url->text();
    const int hours = ui->auto_update_interval->value();
    ent->auto_update_interval = ui->auto_update_enable->isChecked() ? hours : -hours;
    ent->front_proxy_id = CACHE.front_proxy;
    QDialog::accept();
}

void DialogEditGroup::refresh_front_proxy() {
    auto fEnt = NekoGui::profileManager->GetProfile(CACHE.front_proxy);
    ui->front_proxy->setText(fEnt == nullptr ? tr("None") : fEnt->bean->DisplayTypeAndName());
}

void DialogEditGroup::on_front_proxy_clicked() {
    MainWindow::instance()->start_select_mode(this, [=, this](int id) {
        if (id >= 0) CACHE.front_proxy = id;
        refresh_front_proxy();
    });
}
