#include "ui/table/ProfileSortFilterProxyModel.hpp"

#include "ui/table/ProfileIdRole.hpp"
#include "ui/table/ProfileTableModel.hpp"

ProfileSortFilterProxyModel::ProfileSortFilterProxyModel(QObject *parent) : QSortFilterProxyModel(parent) {
    m_collator.setNumericMode(true);
}

void ProfileSortFilterProxyModel::setFilterText(const QString &text) {
    const auto newFilter = text.trimmed().toLower();
    if (newFilter == m_filter) return;
    beginFilterChange(); // Qt6.6+ 推荐,替换弃用的 invalidateFilter()
    m_filter = newFilter;
    endFilterChange();
}

bool ProfileSortFilterProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const {
    if (m_filter.isEmpty()) return true;
    const auto *source = sourceModel();
    if (source == nullptr) return true;
    for (int col = 0; col < ProfileTableModel::ColumnCount; ++col) {
        const auto text = source->data(source->index(sourceRow, col), Qt::DisplayRole).toString().toLower();
        if (text.contains(m_filter)) return true;
    }
    return false;
}

bool ProfileSortFilterProxyModel::lessThan(const QModelIndex &left, const QModelIndex &right) const {
    const auto l = sourceModel()->data(left, ProfileUi::ProfileSortRole);
    const auto r = sourceModel()->data(right, ProfileUi::ProfileSortRole);
    return m_collator.compare(l.toString(), r.toString()) < 0;
}