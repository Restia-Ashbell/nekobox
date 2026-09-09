#pragma once

#include "protocol/AbstractBean.hpp"

namespace NekoGui_fmt {
    class ChainBean : public AbstractBean {
        Q_OBJECT
        Q_PROPERTY(QList<int> list MEMBER list)

    public:
        QList<int> list; // in to out

        ChainBean() : AbstractBean(0) {}

        QString DisplayType() override { return QObject::tr("Chain Proxy"); };

        QString DisplayAddress() override { return ""; };
    };
} // namespace NekoGui_fmt
