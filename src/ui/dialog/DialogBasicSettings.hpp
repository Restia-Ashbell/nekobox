#pragma once

#include <QDialog>
#include <QJsonObject>

namespace Ui {
    class DialogBasicSettings;
}

class DialogBasicSettings : public QDialog {
    Q_OBJECT

public:
    explicit DialogBasicSettings(QWidget *parent = nullptr);

    ~DialogBasicSettings() override;

public slots:

    void accept() override;

private:
    Ui::DialogBasicSettings *ui;

    struct {
        QVariantMap extraCore;
        QString custom_inbound;
        bool needRestart = false;
    } CACHE;

private slots:

    void on_set_custom_icon_clicked();
};
