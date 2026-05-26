#include "dialog_manage_routes.h"
#include "ui_dialog_manage_routes.h"

#include <QFile>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>

#include "3rdparty/qv2ray/v2/ui/widgets/editors/w_JsonEditor.hpp"
#include "fmt/Preset.hpp"
#include "main/GuiUtils.hpp"

#define REFRESH_ACTIVE_ROUTING(name, obj)           \
    this->active_routing = name;                    \
    setWindowTitle(title_base + " [" + name + "]"); \
    UpdateDisplayRouting(obj, false);

DialogManageRoutes::DialogManageRoutes(QWidget *parent) : QDialog(parent), ui(new Ui::DialogManageRoutes) {
    ui->setupUi(this);
    title_base = windowTitle();

    ui->outbound_domain_strategy->addItems(Preset::SingBox::DomainStrategy);
    ui->domainStrategyCombo->addItems(Preset::SingBox::DomainStrategy);
    ui->direct_dns_strategy->addItems(Preset::SingBox::DomainStrategy);
    ui->remote_dns_strategy->addItems(Preset::SingBox::DomainStrategy);
    //
    connect(ui->enable_custom, &QCheckBox::checkStateChanged, this, [=, this](int state) {
        auto useDNSObject = state == Qt::Checked;
        ui->custom_edit->setDisabled(!useDNSObject);
        ui->commonBox->setDisabled(useDNSObject);
        ui->simple_dns_box->setDisabled(useDNSObject);
        ui->simple_route_box->setDisabled(useDNSObject);
    });
    emit ui->enable_custom->checkStateChanged(Qt::Unchecked);
    //
    connect(ui->custom_edit, &QPushButton::clicked, this, [=, this] {
        C_EDIT_JSON_ALLOW_EMPTY(custom)
    });
    //
    builtInSchemesMenu = new QMenu(this);
    builtInSchemesMenu->addActions(this->getBuiltInSchemes());
    ui->preset->setMenu(builtInSchemesMenu);

    REFRESH_ACTIVE_ROUTING(NekoGui::dataStore->active_routing, NekoGui::dataStore->routing.get())

    ADD_ASTERISK(this)
}

DialogManageRoutes::~DialogManageRoutes() {
    delete ui;
}

void DialogManageRoutes::accept() {
    bool routeChanged = false;
    if (NekoGui::dataStore->active_routing != active_routing) routeChanged = true;
    SaveDisplayRouting(NekoGui::dataStore->routing.get());
    NekoGui::dataStore->active_routing = active_routing;
    NekoGui::dataStore->routing->fn = "routes/" + NekoGui::dataStore->active_routing;
    if (NekoGui::dataStore->routing->Save()) routeChanged = true;
    //
    QString info = "UpdateDataStore";
    if (routeChanged) info += "RouteChanged";
    MW_dialog_message(Dialog_DialogManageRoutes, info);
    QDialog::accept();
}

// built in settings

QList<QAction *> DialogManageRoutes::getBuiltInSchemes() {
    QList<QAction *> list;
    list.append(this->schemeToAction(tr("Bypass LAN and China"), routing_cn_lan));
    list.append(this->schemeToAction(tr("Global"), routing_global));
    return list;
}

QAction *DialogManageRoutes::schemeToAction(const QString &name, const NekoGui::Routing &scheme) {
    auto *action = new QAction(name, this);
    connect(action, &QAction::triggered, [this, &scheme] { this->UpdateDisplayRouting((NekoGui::Routing *) &scheme, true); });
    return action;
}

void DialogManageRoutes::UpdateDisplayRouting(NekoGui::Routing *conf, bool qv) {
    ui->blockTxt->setPlainText(conf->block_rules);
    ui->proxyTxt->setPlainText(conf->proxy_rules);
    ui->directTxt->setPlainText(conf->direct_rules);
    ui->def_outbound->setCurrentText(conf->def_outbound);
    //
    if (qv) return;
    //
    ui->rule_sets_provider->setText(conf->rule_sets_provider);
    ui->enable_custom->setChecked(conf->enable_custom);
    CACHE.custom = conf->custom;
    //
    ui->sniffing_mode->setCurrentIndex(conf->sniffing_mode);
    ui->outbound_domain_strategy->setCurrentText(conf->outbound_domain_strategy);
    ui->domainStrategyCombo->setCurrentText(conf->domain_strategy);
    ui->dns_routing->setChecked(conf->dns_routing);
    ui->fake_dns->setChecked(conf->fake_dns);
    ui->remote_dns->setCurrentText(conf->remote_dns);
    ui->remote_dns_strategy->setCurrentText(conf->remote_dns_strategy);
    ui->direct_dns->setCurrentText(conf->direct_dns);
    ui->direct_dns_strategy->setCurrentText(conf->direct_dns_strategy);
    ui->dns_final_out->setCurrentText(conf->dns_final_out);
}

void DialogManageRoutes::SaveDisplayRouting(NekoGui::Routing *conf) {
    conf->block_rules = ui->blockTxt->toPlainText();
    conf->proxy_rules = ui->proxyTxt->toPlainText();
    conf->direct_rules = ui->directTxt->toPlainText();
    conf->def_outbound = ui->def_outbound->currentText();
    conf->rule_sets_provider = ui->rule_sets_provider->text();
    conf->custom = CACHE.custom;
    //
    conf->sniffing_mode = ui->sniffing_mode->currentIndex();
    conf->domain_strategy = ui->domainStrategyCombo->currentText();
    conf->outbound_domain_strategy = ui->outbound_domain_strategy->currentText();
    conf->enable_custom = ui->enable_custom->isChecked();
    conf->dns_routing = ui->dns_routing->isChecked();
    conf->fake_dns = ui->fake_dns->isChecked();
    conf->remote_dns = ui->remote_dns->currentText();
    conf->remote_dns_strategy = ui->remote_dns_strategy->currentText();
    conf->direct_dns = ui->direct_dns->currentText();
    conf->direct_dns_strategy = ui->direct_dns_strategy->currentText();
    conf->dns_final_out = ui->dns_final_out->currentText();
}

void DialogManageRoutes::on_load_save_clicked() {
    auto w = new QDialog;
    auto layout = new QVBoxLayout;
    w->setLayout(layout);
    auto lineEdit = new QLineEdit;
    layout->addWidget(lineEdit);
    auto list = new QListWidget;
    layout->addWidget(list);
    for (const auto &name: NekoGui::Routing::List()) {
        list->addItem(name);
    }
    connect(list, &QListWidget::currentTextChanged, lineEdit, &QLineEdit::setText);
    auto bottom = new QHBoxLayout;
    layout->addLayout(bottom);
    auto load = new QPushButton;
    load->setText(tr("Load"));
    bottom->addWidget(load);
    auto save = new QPushButton;
    save->setText(tr("Save"));
    bottom->addWidget(save);
    auto remove = new QPushButton;
    remove->setText(tr("Remove"));
    bottom->addWidget(remove);
    auto cancel = new QPushButton;
    cancel->setText(tr("Cancel"));
    bottom->addWidget(cancel);
    connect(load, &QPushButton::clicked, w, [=, this] {
        auto fn = lineEdit->text();
        if (!fn.isEmpty()) {
            auto r = std::make_unique<NekoGui::Routing>();
            r->load_control_must = true;
            r->fn = "routes/" + fn;
            if (r->Load()) {
                if (QMessageBox::question(nullptr, software_name, tr("Load routing: %1").arg(fn) + "\n" + r->DisplayRouting()) == QMessageBox::Yes) {
                    REFRESH_ACTIVE_ROUTING(fn, r.get()) // temp save to the window
                    w->accept();
                }
            }
        }
    });
    connect(save, &QPushButton::clicked, w, [=, this] {
        auto fn = lineEdit->text();
        if (!fn.isEmpty()) {
            auto r = std::make_unique<NekoGui::Routing>();
            SaveDisplayRouting(r.get());
            r->fn = "routes/" + fn;
            if (QMessageBox::question(nullptr, software_name, tr("Save routing: %1").arg(fn) + "\n" + r->DisplayRouting()) == QMessageBox::Yes) {
                r->Save();
                REFRESH_ACTIVE_ROUTING(fn, r.get())
                w->accept();
            }
        }
    });
    connect(remove, &QPushButton::clicked, w, [=, this] {
        auto fn = lineEdit->text();
        if (!fn.isEmpty() && NekoGui::Routing::List().length() > 1) {
            if (QMessageBox::question(nullptr, software_name, tr("Remove routing: %1").arg(fn)) == QMessageBox::Yes) {
                QFile f("routes/" + fn);
                f.remove();
                if (NekoGui::dataStore->active_routing == fn) {
                    NekoGui::Routing::SetToActive(NekoGui::Routing::List().first());
                    REFRESH_ACTIVE_ROUTING(NekoGui::dataStore->active_routing, NekoGui::dataStore->routing.get())
                }
                w->accept();
            }
        }
    });
    connect(cancel, &QPushButton::clicked, w, &QDialog::accept);
    connect(list, &QListWidget::itemDoubleClicked, this, [=, this](QListWidgetItem *item) {
        lineEdit->setText(item->text());
        emit load->clicked();
    });
    w->exec();
    w->deleteLater();
}
