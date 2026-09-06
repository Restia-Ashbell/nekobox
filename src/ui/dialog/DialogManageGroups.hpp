#pragma once

#include <QDialog>
#include <QMenu>
#include <QWidget>

#include "profile/Group.hpp"

QT_BEGIN_NAMESPACE
namespace Ui {
    class DialogManageGroups;
}
QT_END_NAMESPACE

class DialogManageGroups : public QDialog {
    Q_OBJECT

public:
    explicit DialogManageGroups(QWidget *parent = nullptr, int index = -1);

    ~DialogManageGroups() override;

private:
    Ui::DialogManageGroups *ui;

    void addGroupToList(int id);

    void updateWindowTitle();

private slots:

    void on_add_clicked();

    void on_update_all_clicked();
};
