#pragma once

#include "fmt/AbstractBean.hpp"

namespace NekoGui_fmt {
    class ChainBean : public AbstractBean {
    public:
        QList<int> list; // in to out

        ChainBean() : AbstractBean(0) {
            _add("list", &list);
        };

        QString DisplayType() override { return QObject::tr("Chain Proxy"); };

        QString DisplayAddress() override { return ""; };
    };
} // namespace NekoGui_fmt
