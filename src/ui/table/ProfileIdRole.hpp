#pragma once

#include <QObject>

namespace ProfileUi {
    // 表格行中携带代理节点 id 的自定义角色,取代历史魔法数字 114514
    inline constexpr int ProfileIdRole = Qt::UserRole + 233;
    // 排序键角色:每列返回一个可比较的原始值
    // (Type/Address/Name → QString,Test → 延时数值,Traffic → 累计流量字节数)
    inline constexpr int ProfileSortRole = Qt::UserRole + 234;
} // namespace ProfileUi