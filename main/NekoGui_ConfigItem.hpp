#pragma once

#include "NekoGui_Utils.hpp"

namespace NekoGui_ConfigItem {
    // 可格式化对象
    class JsonStore {
    public:
        QMap<QString, QVariant> _map;

        std::function<void()> callback_after_load = nullptr;
        std::function<void()> callback_before_save = nullptr;

        QString fn;
        bool load_control_must = false; // must load from file
        bool save_control_compact = false;
        bool save_control_no_save = false;
        QByteArray last_save_content;

        JsonStore() = default;

        explicit JsonStore(const QString &fileName) : fn(fileName) {}

        template<typename T>
        void _add(const QString &name, T *ptr) {
            _map.insert(name, QVariant::fromValue(ptr));
        }

        template<typename T>
        T *_get(const QString &name) {
            return *(T **) _map.value(name).data();
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
