#pragma once

#include <QApplication>
#include <QObject>
#include <QString>
#include <QThread>
#include <QUrlQuery>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QTcpServer>
#include <QMessageBox>
#include <QDir>
#include <QFile>
#include <QRegularExpression>
#include <QDateTime>
#include <QLocale>
#include <QHostAddress>
#include <QRandomGenerator>
#include <QTemporaryFile>

inline const QString software_name = "NekoBox";
inline const QString software_core_name = "sing-box";

// MainWindow functions
inline QWidget *mainwindow;
inline std::function<void(const QString &)> MW_show_log;
inline std::function<void(const QString &, const QString &)> MW_show_log_ext;
inline std::function<void(const QString &)> MW_show_log_ext_vt100;
inline std::function<void(const QString &, const QString &)> MW_dialog_message;

// String

inline const QString UNICODE_LRO = QString::fromUtf8(QByteArray::fromHex("E280AD"));

inline QString Int2String(int num) {
    return QString::number(num);
}

inline QString firstOrSecond(const QString &a, const QString &b) {
    return a.isEmpty() ? b : a;
}

inline QString SubStrBefore(const QString &str, const QString &sub) {
    int idx = str.indexOf(sub);
    return (idx >= 0) ? str.left(idx) : str;
}

inline QString SubStrAfter(const QString &str, const QString &sub) {
    int idx = str.indexOf(sub);
    return (idx >= 0) ? str.sliced(idx + sub.size()) : str;
}

inline QStringList SplitLines(const QString &str) {
    return str.split(QRegularExpression("[\r\n]"), Qt::SkipEmptyParts);
}

inline QStringList SplitLinesSkipSharp(const QString &str, int maxLine = 0) {
    QStringList res;
    int count = 0;
    for (const auto &line: SplitLines(str)) {
        if (line.trimmed().startsWith("#")) continue;
        res << line;
        if (maxLine > 0 && ++count >= maxLine) break;
    }
    return res;
}

inline QString cleanVT100String(QString str) {
    return str.remove(QRegularExpression("\x1B\\[[0-9;]*m"));
}

// Base64

inline QByteArray DecodeB64IfValid(const QString &input, QByteArray::Base64Options options = QByteArray::Base64Encoding) {
    return QByteArray::fromBase64(input.toUtf8(), options | QByteArray::AbortOnBase64DecodingErrors);
}

inline QByteArray DecodeBase64OrBase64Url(const QString &input) {
    return QByteArray::fromBase64(input.toUtf8(), input.contains('-') || input.contains('_') ? QByteArray::Base64UrlEncoding : QByteArray::Base64Encoding);
}

// URL

inline QString GetQueryValue(const QUrlQuery &q, const QString &key, const QString &def = {}) {
    return q.hasQueryItem(key) ? q.queryItemValue(key) : def;
}

// Random

inline quint64 GetRandomUint64() {
    return QRandomGenerator::global()->generate64();
}

inline QString GetRandomHexString() {
    return QUuid::createUuid().toString(QUuid::Id128);
}

// JSON

inline QJsonObject QString2QJsonObject(const QString &jsonString) {
    return QJsonDocument::fromJson(jsonString.toUtf8()).object();
}

inline QString QJsonObject2QString(const QJsonObject &jsonObject, bool compact) {
    return QJsonDocument(jsonObject).toJson(compact ? QJsonDocument::Compact : QJsonDocument::Indented);
}

template<typename T>
inline QJsonArray QList2QJsonArray(const QList<T> &list) {
    QJsonArray arr;
    for (const auto &v: list) arr.append(v);
    return arr;
}

template<typename T>
inline QList<T> QJsonArray2QList(const QJsonArray &arr) {
    QList<T> list;
    for (const auto &v: arr) list.append(v.toVariant().value<T>());
    return list;
}

inline QJsonArray QString2QJsonArray(const QString &str) {
    QJsonArray jsonArray;
    for (const QString &item: str.split(",", Qt::SkipEmptyParts)) {
        QString trimmedItem = item.trimmed();

        bool isInt, isDouble;
        int intValue = trimmedItem.toInt(&isInt);
        double doubleValue = trimmedItem.toDouble(&isDouble);

        if (isInt) {
            jsonArray.append(intValue);
        } else if (isDouble) {
            jsonArray.append(doubleValue);
        } else {
            jsonArray.append(trimmedItem);
        }
    }
    return jsonArray;
}

inline QJsonArray mergeJsonArray(const QJsonArray &arr1, const QJsonArray &arr2) {
    QJsonArray result = arr1;
    for (const QJsonValue &v: arr2) {
        result.append(v);
    }
    return result;
}

// Files

inline QString WriteTempFile(const QString &fileName, const QString &content, QString &error) {
    QDir tempDir("temp");
    tempDir.mkpath(".");
    QTemporaryFile tempFile(tempDir.absoluteFilePath(fileName));
    tempFile.setAutoRemove(false);
    if (tempFile.open()) {
        tempFile.write(content.toUtf8());
    } else {
        error = tempFile.errorString();
    }
    return tempFile.fileName();
}

// Network

inline quint16 MkPort() {
    QTcpServer s;
    s.listen();
    quint16 port = s.serverPort();
    s.close();
    return port;
}

inline bool IsValidPort(int port) {
    return 0 <= port && port <= 65535;
}

inline bool IsIpAddress(const QString &str) {
    return QHostAddress(str).protocol() != QAbstractSocket::UnknownNetworkLayerProtocol;
}

inline bool IsIpAddressV4(const QString &str) {
    return QHostAddress(str).protocol() == QAbstractSocket::IPv4Protocol;
}

inline bool IsIpAddressV6(const QString &str) {
    return QHostAddress(str).protocol() == QAbstractSocket::IPv6Protocol;
}

inline QString WrapIPV6Host(const QString &str) {
    return IsIpAddressV6(str) ? QString("[%1]").arg(str) : str;
}

inline QString MakeHostPort(const QString &host, int port) {
    return host.isEmpty() && !IsValidPort(port) ? host : QString("%1:%2").arg(WrapIPV6Host(host)).arg(port);
};

inline QString DisplayTime(qint64 time, QLocale::FormatType format = QLocale::LongFormat) {
    return QLocale().toString(QDateTime::fromSecsSinceEpoch(time), format);
}

inline QString ReadableSize(qint64 bytes) {
    static const QStringList units{"B", "KiB", "MiB", "GiB", "TiB", "PiB", "EiB", "ZiB", "YiB"};
    double s = bytes;
    int i = 0;
    while (s >= 1024.0 && i < units.size() - 1) {
        s /= 1024.0;
        ++i;
    }
    return QString("%1 %2").arg(s, 0, 'f', 2).arg(units[i]);
}

// UI

inline QWidget *GetMessageBoxParent() {
    auto activeWindow = QApplication::activeWindow();
    if (activeWindow == nullptr && mainwindow != nullptr && mainwindow->isVisible()) {
        return mainwindow;
    }
    return activeWindow;
}

inline int MessageBoxWarning(const QString &title, const QString &text) {
    return QMessageBox::warning(GetMessageBoxParent(), title, text);
}

inline int MessageBoxInfo(const QString &title, const QString &text) {
    return QMessageBox::information(GetMessageBoxParent(), title, text);
}

inline void ActivateWindow(QWidget *w) {
    w->showNormal();
    w->raise();
    w->activateWindow();
}

// Thread

inline void runOnUiThread(const std::function<void()> &callback, QObject *context = qApp) {
    QMetaObject::invokeMethod(context, callback, Qt::AutoConnection);
}

template<typename Function, typename... Args>
inline void runOnNewThread(Function &&func, Args &&...args) {
    auto thread = QThread::create(std::forward<Function>(func), std::forward<Args>(args)...);
    QObject::connect(thread, &QThread::finished, thread, &QObject::deleteLater);
    thread->start();
}

//

template<typename EMITTER, typename SIGNAL, typename RECEIVER, typename ReceiverFunc>
inline void connectOnce(EMITTER *emitter, SIGNAL signal, RECEIVER *receiver, ReceiverFunc f,
                        Qt::ConnectionType connectionType = Qt::AutoConnection) {
    auto connection = std::make_shared<QMetaObject::Connection>();
    auto onTriggered = [connection, f](auto... arguments) {
        std::invoke(f, arguments...);
        QObject::disconnect(*connection);
    };

    *connection = QObject::connect(emitter, signal, receiver, onTriggered, connectionType);
}
