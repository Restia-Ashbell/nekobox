#pragma once

#include "fmt/AbstractBean.hpp"

namespace NekoGui_fmt {
    class VMessBean : public AbstractBean {
    public:
        QString uuid = "";
        int aid = 0;
        QString security = "auto";

        std::shared_ptr<V2rayStreamSettings> stream = std::make_shared<V2rayStreamSettings>();
        MultiplexSettings multiplex;

        VMessBean() : AbstractBean(0) {
            _add("id", &uuid);
            _add("aid", &aid);
            _add("sec", &security);
            _add("stream", dynamic_cast<JsonStore *>(stream.get()));
            _add("multiplex", dynamic_cast<JsonStore *>(&multiplex));
        };

        QString DisplayType() override { return "VMess"; };

        CoreObjOutboundBuildResult BuildCoreObjSingBox() override;

        bool TryParseLink(const QString &link) override;

        QString ToShareLink() override;
    };
} // namespace NekoGui_fmt
