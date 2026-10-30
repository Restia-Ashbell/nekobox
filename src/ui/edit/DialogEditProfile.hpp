#pragma once

#include <QDialog>

#include "profile/ProfileManager.hpp"
#include "ui/edit/ProfileEditor.hpp"

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
        QString certificate;
        QString ech;
    } CACHE;

    void typeSelected();

    bool onEnd();

    void editor_cache_updated_impl();
};
