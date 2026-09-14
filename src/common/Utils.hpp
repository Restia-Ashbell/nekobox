#pragma once

#include <QApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QHostAddress>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLocale>
#include <QMessageBox>
#include <QObject>
#include <QRandomGenerator>
#include <QRegularExpression>
#include <QString>
#include <QTcpServer>
#include <QTemporaryFile>
#include <QThread>
#include <QUrlQuery>

inline const QString software_name = "NekoBox";
inline const QString software_core_name = "sing-box";

// MainWindow functions
inline QWidget *mainwindow;
inline std::function<void(const QString &)> MW_show_log;
inline std::function<void(const QString &, const QString &)> MW_show_log_ext;
inline std::function<void(const QString &, const QString &)> MW_dialog_message;

// String

inline const QString UNICODE_LRO = QStringLiteral("\u202A");

#define Int2String(num) (QString::number(num))

inline QString firstOrSecond(const QString &a, const QString &b) {
    return a.isEmpty() ? b : a;
}

inline QString SubStrBefore(const QString &str, const QString &sub) {
    const int idx = str.indexOf(sub);
    return idx >= 0 ? str.left(idx) : str;
}

inline QString SubStrAfter(const QString &str, const QString &sub) {
    const int idx = str.indexOf(sub);
    return idx >= 0 ? str.sliced(idx + sub.size()) : str;
}

inline QStringList SplitLines(const QString &str) {
    return str.split(QRegularExpression("[\r\n]"), Qt::SkipEmptyParts);
}

inline QStringList SplitLinesSkipSharp(const QString &str, int maxLine = 0) {
    QStringList out;
    for (const auto &line: SplitLines(str)) {
        if (line.trimmed().startsWith("#")) continue;
        out << line;
        if (maxLine > 0 && out.size() >= maxLine) break;
    }
    return out;
}

inline QString cleanVT100String(QString str) {
    return str.remove(QRegularExpression("\x1B\\[[0-9;]*m"));
}

// Base64

inline QByteArray DecodeB64IfValid(const QString &input, QByteArray::Base64Options options = QByteArray::Base64Encoding) {
    return QByteArray::fromBase64(input.toLatin1(), options | QByteArray::AbortOnBase64DecodingErrors);
}

inline QByteArray DecodeBase64OrBase64Url(const QString &input) {
    return QByteArray::fromBase64(input.toLatin1(), input.contains('-') || input.contains('_') ? QByteArray::Base64UrlEncoding : QByteArray::Base64Encoding);
}

// URL

inline QString GetQueryValue(const QUrlQuery &query, const QString &key, const QString &def = {}) {
    return query.hasQueryItem(key) ? query.queryItemValue(key) : def;
}

inline QString FirstQueryValue(const QUrlQuery &query, const QStringList &keys) {
    for (const auto &key: keys) {
        if (query.hasQueryItem(key)) {
            return query.queryItemValue(key);
        }
    }
    return {};
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
        const QString t = item.trimmed();
        bool ok = false;
        if (const int i = t.toInt(&ok); ok) {
            jsonArray.append(i);
            continue;
        }
        if (const double d = t.toDouble(&ok); ok) {
            jsonArray.append(d);
            continue;
        }
        jsonArray.append(t);
    }
    return jsonArray;
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
    s.listen(QHostAddress::LocalHost);
    return s.serverPort();
}

inline bool IsValidPort(int port) {
    return port >= 0 && port <= 65535;
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
    if (QWidget *w = QApplication::activeWindow()) return w;
    return mainwindow != nullptr && mainwindow->isVisible() ? mainwindow : nullptr;
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
inline QThread *runOnNewThread(Function &&func, Args &&...args) {
    auto thread = QThread::create(std::forward<Function>(func), std::forward<Args>(args)...);
    QObject::connect(thread, &QThread::finished, thread, &QObject::deleteLater);
    thread->start();
    return thread;
}
