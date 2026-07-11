#pragma once

#include <QPushButton>

#include "profile/ProxyEntity.hpp"
#include "common/GuiUtils.hpp"

class ProfileEditor {
public:
    virtual void onStart(std::shared_ptr<NekoGui::ProxyEntity> ent) = 0;

    virtual bool onEnd() = 0;

    std::function<QWidget *()> get_edit_dialog;

    // cached editor

    std::function<void()> editor_cache_updated;

    virtual QList<QPair<QPushButton *, QString>> get_editor_cached() { return {}; };
};
