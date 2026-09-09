#include "profile/ConfigItem.hpp"

namespace NekoGui_ConfigItem {

    QJsonObject JsonStore::ToJson(const QStringList &without) {
        QJsonObject object;
        const auto *metaobject = metaObject();
        for (int i = 0; i < metaobject->propertyCount(); ++i) {
            const auto property = metaobject->property(i);
            // 跳过 QObject 自带的 objectName 等内置属性
            if (property.enclosingMetaObject() == &QObject::staticMetaObject) continue;
            const auto name = QString::fromLatin1(property.name());
            if (without.contains(name)) continue;
            const auto variant = property.read(this);
            const int typeId = variant.typeId();

            // 空字符串不写
            if (typeId == QMetaType::QString && variant.toString().isEmpty()) continue;

            // 嵌套 JsonStore 递归
            if (property.metaType().metaObject() && property.metaType().metaObject()->inherits(&JsonStore::staticMetaObject)) {
                if (auto *obj = variant.value<QObject *>()) {
                    if (auto *store = qobject_cast<JsonStore *>(obj)) {
                        object.insert(name, store->ToJson());
                    }
                }
                continue;
            }

            // QList<int>:QJsonValue::fromVariant 不支持
            if (typeId == qMetaTypeId<QList<int>>()) {
                object.insert(name, QList2QJsonArray<int>(variant.value<QList<int>>()));
                continue;
            }

            const auto json = QJsonValue::fromVariant(variant);
            if (!json.isUndefined()) object.insert(name, json);
        }
        return object;
    }

    QByteArray JsonStore::ToJsonBytes() {
        QJsonDocument document;
        document.setObject(ToJson());
        return document.toJson(save_control_compact ? QJsonDocument::Compact : QJsonDocument::Indented);
    }

    void JsonStore::FromJson(const QJsonObject &object) {
        const auto *metaobject = metaObject();
        for (int i = 0; i < metaobject->propertyCount(); ++i) {
            const auto property = metaobject->property(i);
            // 跳过 QObject 自带的属性
            if (property.enclosingMetaObject() == &QObject::staticMetaObject) continue;
            const auto name = QString::fromLatin1(property.name());
            if (!object.contains(name)) continue;
            const auto &json = object.value(name);
            const int typeId = property.metaType().id();

            // 嵌套 JsonStore 递归
            if (property.metaType().metaObject() && property.metaType().metaObject()->inherits(&JsonStore::staticMetaObject)) {
                if (json.isObject()) {
                    if (auto *obj = property.read(this).value<QObject *>()) {
                        if (auto *store = qobject_cast<JsonStore *>(obj)) {
                            store->FromJson(json.toObject());
                        }
                    }
                }
                continue;
            }

            // QList<int>
            if (typeId == qMetaTypeId<QList<int>>() && json.isArray()) {
                property.write(this, QVariant::fromValue(QJsonArray2QList<int>(json.toArray())));
                continue;
            }

            property.write(this, json.toVariant());
        }
    }

    void JsonStore::FromJsonBytes(const QByteArray &data) {
        QJsonParseError error{};
        auto document = QJsonDocument::fromJson(data, &error);

        if (error.error != QJsonParseError::NoError) {
            qDebug() << "QJsonParseError" << error.errorString();
            return;
        }

        FromJson(document.object());
    }

    bool JsonStore::Save() {
        if (save_control_no_save) return false;

        auto save_content = ToJsonBytes();
        auto new_hash = qHash(save_content);
        bool changed = last_save_hash != new_hash;
        if (!changed && !fn.isEmpty() && QFile::exists(fn)) {
            return false;
        }

        QFile file(fn);
        if (!file.open(QIODevice::ReadWrite | QIODevice::Truncate)) {
            MessageBoxWarning("error", "can not open config " + fn + "\n" + file.errorString());
            return false;
        }
        file.write(save_content);
        file.close();
        last_save_hash = new_hash;

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
            auto data = file.readAll();
            last_save_hash = qHash(data);
            FromJsonBytes(data);
        }

        file.close();
        return ok;
    }

} // namespace NekoGui_ConfigItem
