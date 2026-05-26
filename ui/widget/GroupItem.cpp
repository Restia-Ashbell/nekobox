#include "GroupItem.h"
#include "ui_GroupItem.h"

#include <QMessageBox>

#include "main/GuiUtils.hpp"
#include "sub/GroupUpdater.hpp"
#include "ui/edit/dialog_edit_group.h"
#include "ui/mainwindow.h"

QString GroupItem::ParseSubInfo(const QString &info) {
    if (info.trimmed().isEmpty()) return {};

    long long used = 0, total = 0, expire = 0;

    QRegularExpressionMatch match;
    match = QRegularExpression("total=([0-9]+)").match(info);
    if (match.hasMatch()) {
        total = match.captured(1).toLongLong();
    }
    match = QRegularExpression("upload=([0-9]+)").match(info);
    if (match.hasMatch()) {
        used += match.captured(1).toLongLong();
    }
    match = QRegularExpression("download=([0-9]+)").match(info);
    if (match.hasMatch()) {
        used += match.captured(1).toLongLong();
    }
    match = QRegularExpression("expire=([0-9]+)").match(info);
    if (match.hasMatch()) {
        expire = match.captured(1).toLongLong();
    }
    if (used == 0 && total == 0 && expire == 0) return {};

    return QObject::tr("Used: %1 Remain: %2 Expire: %3").arg(ReadableSize(used), ReadableSize(total - used), DisplayTime(expire, QLocale::ShortFormat));
}

QString GroupItem::parseFileName(const QString &contentDisposition) {
    if (contentDisposition.isEmpty()) return {};
    QRegularExpressionMatch match;
    QRegularExpression reFilenameStar(R"(filename\*\s*=\s*UTF-8''([^;]+))", QRegularExpression::CaseInsensitiveOption);
    match = reFilenameStar.match(contentDisposition);
    if (match.hasMatch()) {
        return QUrl::fromPercentEncoding(match.captured(1).toUtf8());
    }
    QRegularExpression reFilename(R"REGEX(filename\s*=\s*(?:"([^"]+)"|([^;]+)))REGEX", QRegularExpression::CaseInsensitiveOption);
    match = reFilename.match(contentDisposition);
    if (match.hasMatch()) {
        QString filename = match.captured(1);
        if (filename.isEmpty()) filename = match.captured(2);
        return filename.trimmed();
    }
    return {};
}

GroupItem::GroupItem(QWidget *parent, const std::shared_ptr<NekoGui::Group> &ent, QListWidgetItem *item) : QWidget(parent), ui(new Ui::GroupItem) {
    ui->setupUi(this);
    this->setLayoutDirection(Qt::LeftToRight);

    this->ent = ent;
    this->item = item;
    if (ent == nullptr) return;

    connect(MainWindow::instance(), &MainWindow::groupUpdated, this, [this](int gid) {
        if (gid == this->ent->id) refresh_data();
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
        QStringList info;
        if (ent->sub_last_update != 0) {
            info << tr("Last update: %1").arg(DisplayTime(ent->sub_last_update, QLocale::ShortFormat));
        }
        if (!ent->info.isEmpty()) {
            info << ParseSubInfo(ent->info);
        }
        if (info.isEmpty()) {
            ui->subinfo->hide();
        } else {
            ui->subinfo->setText(info.join(" | "));
            ui->subinfo->show();
        }
    }
    item->setSizeHint(sizeHint());
}

void GroupItem::on_update_sub_clicked() {
    NekoGui_sub::groupUpdater->AsyncUpdate(ent->url, ent->id);
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
