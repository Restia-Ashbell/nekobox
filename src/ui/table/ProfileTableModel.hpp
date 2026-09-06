#pragma once

#include <QAbstractTableModel>

#include "ui/table/ProfileIdRole.hpp"

namespace NekoGui {
    class ProxyEntity;
}

// 代理列表数据模型:把"显示顺序"与视图彻底解耦,行号由 m_ids 决定。
// 相比旧的 QTableWidget,不再需要魔法角色回填 id,刷新走 dataChanged 而不是重建单元格。
class ProfileTableModel : public QAbstractTableModel {
    Q_OBJECT

public:
    enum Column {
        ColumnType = 0,
        ColumnAddress,
        ColumnName,
        ColumnTest,
        ColumnTraffic,
        ColumnCount,
    };

    explicit ProfileTableModel(QObject *parent = nullptr);

    int groupId() const { return m_gid; }

    // 绑定组:同步 group->order(清理孤儿并补全)后重建行
    void setGroup(int gid);

    // 行号 → 实体
    std::shared_ptr<NekoGui::ProxyEntity> profileAtRow(int row) const;

    // 实体 id → 行号(未找到为 -1)
    int rowOfProfile(int id) const;

    // 单行整体刷新(流量/测速/启停高亮),不破坏选中、滚动与过滤
    void notifyProfileChanged(int id);

    // ---- QAbstractTableModel ----
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

private:
    int m_gid = -1;
    QList<int> m_ids; // 显示顺序(MainWindow 的 ProfileUi 视图态)
};