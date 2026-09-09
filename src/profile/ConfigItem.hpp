#pragma once

#include <QObject>
#include <QJsonArray>
#include <QJsonObject>
#include <QMetaProperty>
#include <QVariant>

#include "common/Utils.hpp"

namespace NekoGui_ConfigItem {
    // 可格式化对象
    // 子类通过 Q_PROPERTY(<Type> <jsonKey> MEMBER <member>|<READ> ...) 声明可序列化字段
    // JsonStore 通过 QMetaObject 反射遍历属性完成 (反) 序列化,支持嵌套 JsonStore 递归
    class JsonStore : public QObject {
        Q_OBJECT
    public:
        QString fn;
        bool load_control_must = false; // must load from file
        bool save_control_compact = false;
        bool save_control_no_save = false;
        size_t last_save_hash = 0;

        explicit JsonStore(QObject *parent = nullptr) : QObject(parent) {}

        // 按 JSON key 取嵌套子对象(反射查找属性并向下转型)
        template<typename T>
        T *_get(const QString &name) const {
            auto v = property(name.toUtf8().constData());
            if (auto *obj = v.value<QObject *>()) {
                return qobject_cast<T *>(obj);
            }
            return nullptr;
        }

        QJsonObject ToJson(const QStringList &without = {});

        QByteArray ToJsonBytes();

        void FromJson(const QJsonObject &object);

        void FromJsonBytes(const QByteArray &data);

        bool Save();

        bool Load();
    };
} // namespace NekoGui_ConfigItem

using namespace NekoGui_ConfigItem;
