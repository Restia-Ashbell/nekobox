#pragma once

#include <QDialog>
#include <QMenu>

#include "main/NekoGui.hpp"

QT_BEGIN_NAMESPACE
namespace Ui {
    class DialogManageRoutes;
}
QT_END_NAMESPACE

class DialogManageRoutes : public QDialog {
    Q_OBJECT

public:
    explicit DialogManageRoutes(QWidget *parent = nullptr);

    ~DialogManageRoutes() override;

private:
    Ui::DialogManageRoutes *ui;

    struct {
        QString custom;
    } CACHE;

    QMenu *builtInSchemesMenu;
    //
    NekoGui::Routing routing_cn_lan = NekoGui::Routing(1);
    NekoGui::Routing routing_global = NekoGui::Routing(0);
    //
    QString title_base;
    QString active_routing;

public slots:

    void accept() override;

    QList<QAction *> getBuiltInSchemes();

    QAction *schemeToAction(const QString &name, const NekoGui::Routing &scheme);

    void UpdateDisplayRouting(NekoGui::Routing *conf, bool qv);

    void SaveDisplayRouting(NekoGui::Routing *conf);

    void on_load_save_clicked();
};
