#include "NekoGui_ConfigItem.hpp"

namespace NekoGui_ConfigItem {

    QJsonObject JsonStore::ToJson(const QStringList &without) {
        QJsonObject object;
        for (const auto &[name, v]: _map.asKeyValueRange()) {
            if (without.contains(name)) continue;
            QString type(v.typeName());
            if (type == "QString*") {
                if (auto s = *v.value<QString *>(); !s.isEmpty()) object.insert(name, s);
            } else if (type == "int*") {
                object.insert(name, *v.value<int *>());
            } else if (type == "qlonglong*") {
                object.insert(name, *v.value<qlonglong *>());
            } else if (type == "bool*") {
                object.insert(name, *v.value<bool *>());
            } else if (type == "QList<QString>*") {
                object.insert(name, QList2QJsonArray<QString>(*v.value<QList<QString> *>()));
            } else if (type == "QList<int>*") {
                object.insert(name, QList2QJsonArray<int>(*v.value<QList<int> *>()));
            } else if (type == "NekoGui_ConfigItem::JsonStore*") {
                object.insert(name, v.value<JsonStore *>()->ToJson());
            }
        }
        return object;
    }

    QByteArray JsonStore::ToJsonBytes() {
        QJsonDocument document;
        document.setObject(ToJson());
        return document.toJson(save_control_compact ? QJsonDocument::Compact : QJsonDocument::Indented);
    }

    void JsonStore::FromJson(const QJsonObject &object) {
        for (const auto &[key, value]: object.asKeyValueRange()) {
            auto v = _map.value(key.toString());
            if (v.isNull()) continue;
            QString type(v.typeName());
            if (type == "QString*") {
                if (value.isString()) *v.value<QString *>() = value.toString();
            } else if (type == "int*") {
                if (value.isDouble()) *v.value<int *>() = value.toInt();
            } else if (type == "qlonglong*") {
                if (value.isDouble()) *v.value<qlonglong *>() = value.toInteger();
            } else if (type == "bool*") {
                if (value.isBool()) *v.value<bool *>() = value.toBool();
            } else if (type == "QList<QString>*") {
                if (value.isArray()) *v.value<QList<QString> *>() = QJsonArray2QList<QString>(value.toArray());
            } else if (type == "QList<int>*") {
                if (value.isArray()) *v.value<QList<int> *>() = QJsonArray2QList<int>(value.toArray());
            } else if (type == "NekoGui_ConfigItem::JsonStore*") {
                if (value.isObject()) v.value<JsonStore *>()->FromJson(value.toObject());
            }
        }

        if (callback_after_load != nullptr) callback_after_load();
    }

    void JsonStore::FromJsonBytes(const QByteArray &data) {
        QJsonParseError error{};
        auto document = QJsonDocument::fromJson(data, &error);

        if (error.error != error.NoError) {
            qDebug() << "QJsonParseError" << error.errorString();
            return;
        }

        FromJson(document.object());
    }

    bool JsonStore::Save() {
        if (callback_before_save != nullptr) callback_before_save();
        if (save_control_no_save) return false;

        auto save_content = ToJsonBytes();
        auto changed = last_save_content != save_content;
        last_save_content = save_content;

        QFile file(fn);
        if (!file.open(QIODevice::ReadWrite | QIODevice::Truncate)) {
            MessageBoxWarning("error", "can not open config " + fn + "\n" + file.errorString());
            return false;
        }
        file.write(save_content);
        file.close();

        return changed;
    }

    bool JsonStore::Load() {
        QFile file(fn);
        if (!file.exists() && !load_control_must) {
            return false;
        }

        bool ok = file.open(QIODevice::ReadOnly);
        if (!ok) {
            MessageBoxWarning("error", "can not open config " + fn + "\n" + file.errorString());
        } else {
            last_save_content = file.readAll();
            FromJsonBytes(last_save_content);
        }

        file.close();
        return ok;
    }

} // namespace NekoGui_ConfigItem
