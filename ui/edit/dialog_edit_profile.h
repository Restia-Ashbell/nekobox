#pragma once

#include <QDialog>

#include "db/ProfileManager.hpp"
#include "profile_editor.h"

namespace Ui {
    class DialogEditProfile;
}

class DialogEditProfile : public QDialog {
    Q_OBJECT

public:
    explicit DialogEditProfile(const QString &_type, int profileOrGroupId, QWidget *parent = nullptr);

    ~DialogEditProfile() override;

public slots:

    void accept() override;

private slots:

    void on_custom_outbound_edit_clicked();

    void on_custom_config_edit_clicked();

    void on_certificate_edit_clicked();

    void on_ech_edit_clicked();

private:
    Ui::DialogEditProfile *ui;

    ProfileEditor *innerEditor{};

    QString type;
    int groupId;
    bool newEnt = false;
    std::shared_ptr<NekoGui::ProxyEntity> ent;

    struct {
        QString custom_outbound;
        QString custom_config;
        QString certificate;
        QString ech;
    } CACHE;

    void typeSelected();

    bool onEnd();

    void editor_cache_updated_impl();
};
