#pragma once

#include <QCollator>
#include <QSortFilterProxyModel>

// 搜索过滤 + 列排序,全部是视图态:
// - 过滤:匹配任意列文本(等价旧实现的手写 rowHidden)
// - 排序:不写回 group->order,只针对当前 setSortIndicator 生效
class ProfileSortFilterProxyModel : public QSortFilterProxyModel {
    Q_OBJECT

public:
    explicit ProfileSortFilterProxyModel(QObject *parent = nullptr);

    void setFilterText(const QString &text);

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;

    bool lessThan(const QModelIndex &left, const QModelIndex &right) const override;

private:
    QCollator m_collator;
    QString m_filter;
};