#pragma once

#include "protocol/AbstractBean.hpp"

namespace NekoGui_fmt {
    class VMessBean : public AbstractBean {
        Q_OBJECT
        Q_PROPERTY(QString id MEMBER uuid)
        Q_PROPERTY(int aid MEMBER aid)
        Q_PROPERTY(QString sec MEMBER security)
        Q_PROPERTY(V2rayStreamSettings *stream MEMBER stream)
        Q_PROPERTY(MultiplexSettings *multiplex MEMBER multiplex)

    public:
        QString uuid = "";
        int aid = 0;
        QString security = "auto";

        V2rayStreamSettings *stream = nullptr;
        MultiplexSettings *multiplex = nullptr;

        VMessBean() : AbstractBean(0) {
            stream = new V2rayStreamSettings(this);
            multiplex = new MultiplexSettings(this);
        }

        QString DisplayType() override { return "VMess"; };

        CoreObjOutboundBuildResult BuildCoreObjSingBox() override;

        bool TryParseLink(const QString &link) override;

        QString ToShareLink() override;
    };
} // namespace NekoGui_fmt
