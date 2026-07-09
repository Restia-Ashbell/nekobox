#pragma once

#include "ProxyEntity.hpp"

namespace NekoGui {
    class ProfileFilter {
    public:
        static std::tuple<QList<std::shared_ptr<ProxyEntity>>, QList<std::shared_ptr<ProxyEntity>>,
                          QList<std::shared_ptr<ProxyEntity>>, QList<std::shared_ptr<ProxyEntity>>>
        Diff(const QList<std::shared_ptr<ProxyEntity>> &list1, const QList<std::shared_ptr<ProxyEntity>> &list2);

        static QList<std::shared_ptr<ProxyEntity>> Dup(const QList<std::shared_ptr<ProxyEntity>> &list);

    private:
        static QString Key(const std::shared_ptr<ProxyEntity> &ent, bool by_address);
    };
} // namespace NekoGui
