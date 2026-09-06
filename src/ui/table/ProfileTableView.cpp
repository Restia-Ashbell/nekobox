#include "ui/table/ProfileTableView.hpp"

#include <QHeaderView>

#include "ui/table/ProfileSortFilterProxyModel.hpp"
#include "ui/table/ProfileTableModel.hpp"

ProfileTableView::ProfileTableView(int gid, QWidget *parent) : QTableView(parent), m_gid(gid) {
    m_model = new ProfileTableModel(this);
    m_proxy = new ProfileSortFilterProxyModel(this);
    m_proxy->setSourceModel(m_model);

    setupView();

    m_model->setGroup(gid);
}

void ProfileTableView::setupView() {
    setModel(m_proxy);

    // 排序是视图态:启用表头排序,但初始指示器清空,显示顺序 = 手动顺序(order)
    setSortingEnabled(true);
    horizontalHeader()->setSortIndicator(-1, Qt::AscendingOrder);
    horizontalHeader()->setSortIndicatorClearable(true);
    horizontalHeader()->setSortIndicatorShown(true);
    horizontalHeader()->setHighlightSections(false);

    horizontalHeader()->setSectionResizeMode(ProfileTableModel::ColumnType, QHeaderView::ResizeToContents);
    horizontalHeader()->setSectionResizeMode(ProfileTableModel::ColumnAddress, QHeaderView::Stretch);
    horizontalHeader()->setSectionResizeMode(ProfileTableModel::ColumnName, QHeaderView::Stretch);
    horizontalHeader()->setSectionResizeMode(ProfileTableModel::ColumnTest, QHeaderView::ResizeToContents);
    horizontalHeader()->setSectionResizeMode(ProfileTableModel::ColumnTraffic, QHeaderView::ResizeToContents);

    verticalHeader()->setDefaultSectionSize(24);

    setSelectionBehavior(QAbstractItemView::SelectRows);
    setEditTriggers(QAbstractItemView::NoEditTriggers);
    setTabKeyNavigation(false);
    setWordWrap(false);
    setContextMenuPolicy(Qt::CustomContextMenu);
}

void ProfileTableView::reload() {
    m_model->setGroup(m_gid);
}

void ProfileTableView::notifyProfile(int id) {
    m_model->notifyProfileChanged(id);
}

void ProfileTableView::setFilterText(const QString &text) {
    m_proxy->setFilterText(text);
}