#include "ui/table/ProfileTableModel.hpp"

#include <QApplication>

#include "profile/DataStore.hpp"
#include "profile/ProfileManager.hpp"

ProfileTableModel::ProfileTableModel(QObject *parent) : QAbstractTableModel(parent) {}

int ProfileTableModel::rowCount(const QModelIndex &parent) const {
    return parent.isValid() ? 0 : m_ids.size();
}

int ProfileTableModel::columnCount(const QModelIndex &parent) const {
    return parent.isValid() ? 0 : ColumnCount;
}

void ProfileTableModel::setGroup(int gid) {
    auto group = NekoGui::profileManager->GetGroup(gid);
    if (group == nullptr) {
        beginResetModel();
        m_gid = -1;
        m_ids.clear();
        endResetModel();
        return;
    }

    // 与旧实现 refresh_group 相同的顺序修复:剔除孤儿 id,兜底补全漏加的节点
    group->order.removeIf([](int k) { return !NekoGui::profileManager->profiles.contains(k); });
    for (const auto &[id, profile]: NekoGui::profileManager->profiles) {
        if (profile->gid == gid && !group->order.contains(id)) {
            group->order.append(id);
        }
    }
    group->Save();

    QList<int> ids;
    for (const auto &ent: group->ProfilesWithOrder()) {
        ids << ent->id; // ProfilesWithOrder 已跳过空实体
    }

    beginResetModel();
    m_gid = gid;
    m_ids = ids;
    endResetModel();
}

std::shared_ptr<NekoGui::ProxyEntity> ProfileTableModel::profileAtRow(int row) const {
    if (row < 0 || row >= m_ids.size()) return nullptr;
    return NekoGui::profileManager->GetProfile(m_ids.at(row));
}

int ProfileTableModel::rowOfProfile(int id) const {
    return m_ids.indexOf(id);
}

void ProfileTableModel::notifyProfileChanged(int id) {
    const auto row = rowOfProfile(id);
    if (row >= 0) {
        emit dataChanged(index(row, 0), index(row, ColumnCount - 1));
    }
}

QVariant ProfileTableModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= m_ids.size()) return {};
    auto ent = NekoGui::profileManager->GetProfile(m_ids.at(index.row()));
    if (ent == nullptr || ent->bean == nullptr) return {};

    const auto &bean = ent->bean;
    const bool isStarted = NekoGui::dataStore->started_id == ent->id;

    switch (role) {
        case ProfileUi::ProfileIdRole:
            return ent->id;
        case Qt::DisplayRole:
            switch (index.column()) {
                case ColumnType:
                    return bean->DisplayType();
                case ColumnAddress:
                    return bean->DisplayAddress();
                case ColumnName:
                    return bean->name;
                case ColumnTest:
                    // 完整测速报告优先,否则是普通延时
                    return !ent->full_test_report.isEmpty() ? ent->full_test_report : ent->DisplayLatency();
                case ColumnTraffic:
                    return ent->traffic_data->DisplayTraffic();
            }
            break;
        case ProfileUi::ProfileSortRole:
            switch (index.column()) {
                case ColumnType:
                    return bean->DisplayType();
                case ColumnAddress:
                    return bean->DisplayAddress();
                case ColumnName:
                    return bean->name;
                case ColumnTest: {
                    return !ent->full_test_report.isEmpty() ? ent->full_test_report : ent->DisplayLatency();
                }
                case ColumnTraffic:
                    return ent->traffic_data->downlink + ent->traffic_data->uplink;
            }
            break;
        case Qt::ForegroundRole:
            if (index.column() == ColumnTest && ent->full_test_report.isEmpty()) {
                if (const auto color = ent->DisplayLatencyColor(); color.isValid()) {
                    return color;
                }
            }
            break;
        case Qt::BackgroundRole:
            if (isStarted) {
                return QApplication::palette().highlight().color().lighter();
            }
            break;
        default:
            break;
    }
    return {};
}

QVariant ProfileTableModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        switch (section) {
            case ColumnType:
                return tr("Type");
            case ColumnAddress:
                return tr("Address");
            case ColumnName:
                return tr("Name");
            case ColumnTest:
                return tr("Test Result");
            case ColumnTraffic:
                return tr("Traffic");
        }
    }
    return QAbstractTableModel::headerData(section, orientation, role);
}