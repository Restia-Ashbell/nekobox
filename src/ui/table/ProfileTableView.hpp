#pragma once

#include <QTableView>

class ProfileTableModel;
class ProfileSortFilterProxyModel;

// 代理列表视图:装配 model + 过滤/排序代理,并集中维护视觉配置。
// 每个分组 tab 对应一个实例,负责向外提供"按 id 刷新行""整体重载"等入口。
class ProfileTableView : public QTableView {
    Q_OBJECT

public:
    explicit ProfileTableView(int gid, QWidget *parent = nullptr);

    int groupId() const { return m_gid; }

    ProfileTableModel *groupModel() const { return m_model; }

    ProfileSortFilterProxyModel *filterProxy() const { return m_proxy; }

    // 整体重载(同步 group->order 后重建行)
    void reload();

    // 单行刷新,不影响选中/滚动/过滤
    void notifyProfile(int id);

    // 搜索过滤文本
    void setFilterText(const QString &text);

private:
    void setupView();

    int m_gid;
    ProfileTableModel *m_model;
    ProfileSortFilterProxyModel *m_proxy;
};