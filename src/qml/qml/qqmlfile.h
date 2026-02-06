// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QQMLFILE_H
#define QQMLFILE_H

#include <QtQml/qtqmlglobal.h>

QT_BEGIN_NAMESPACE

class QUrl;
class QString;
class QObject;
class QQmlEngine;
class QQmlFilePrivate;

// ### Qt7: Turn this into a namesapce
class Q_QML_EXPORT QQmlFile
{
public:

#if QT_DEPRECATED_SINCE(6, 11)
    QT_DEPRECATED_VERSION_X_6_11("Use QQmlComponent or the QML engine for loading files.")
    QQmlFile();

    QT_DEPRECATED_VERSION_X_6_11("Use QQmlComponent or the QML engine for loading files.")
    QQmlFile(QQmlEngine *engine, const QUrl &url);

    QT_DEPRECATED_VERSION_X_6_11("Use QQmlComponent or the QML engine for loading files.")
    QQmlFile(QQmlEngine *engine, const QString &url);

    QT_DEPRECATED_VERSION_X_6_11("Use QQmlComponent or the QML engine for loading files.")
    ~QQmlFile();

    enum Status { Null, Ready, Error, Loading };

    QT_DEPRECATED_VERSION_X_6_11("Use QQmlComponent or the QML engine for loading files.")
    bool isNull() const;

    QT_DEPRECATED_VERSION_X_6_11("Use QQmlComponent or the QML engine for loading files.")
    bool isReady() const;

    QT_DEPRECATED_VERSION_X_6_11("Use QQmlComponent or the QML engine for loading files.")
    bool isError() const;

    QT_DEPRECATED_VERSION_X_6_11("Use QQmlComponent or the QML engine for loading files.")
    bool isLoading() const;

    QT_DEPRECATED_VERSION_X_6_11("Use QQmlComponent or the QML engine for loading files.")
    QUrl url() const;

    QT_DEPRECATED_VERSION_X_6_11("Use QQmlComponent or the QML engine for loading files.")
    Status status() const;

    QT_DEPRECATED_VERSION_X_6_11("Use QQmlComponent or the QML engine for loading files.")
    QString error() const;

    QT_DEPRECATED_VERSION_X_6_11("Use QQmlComponent or the QML engine for loading files.")
    qint64 size() const;

    QT_DEPRECATED_VERSION_X_6_11("Use QQmlComponent or the QML engine for loading files.")
    const char *data() const;

    QT_DEPRECATED_VERSION_X_6_11("Use QQmlComponent or the QML engine for loading files.")
    QByteArray dataByteArray() const;

    QT_DEPRECATED_VERSION_X_6_11("Use QQmlComponent or the QML engine for loading files.")
    void load(QQmlEngine *, const QUrl &);

    QT_DEPRECATED_VERSION_X_6_11("Use QQmlComponent or the QML engine for loading files.")
    void load(QQmlEngine *, const QString &);

    QT_DEPRECATED_VERSION_X_6_11("Use QQmlComponent or the QML engine for loading files.")
    void clear();

    QT_DEPRECATED_VERSION_X_6_11("Use QQmlComponent or the QML engine for loading files.")
    void clear(QObject *object);

#if QT_CONFIG(qml_network)
    QT_DEPRECATED_VERSION_X_6_11("Use QQmlComponent or the QML engine for loading files.")
    bool connectFinished(QObject *, const char *);

    QT_DEPRECATED_VERSION_X_6_11("Use QQmlComponent or the QML engine for loading files.")
    bool connectFinished(QObject *, int);

    QT_DEPRECATED_VERSION_X_6_11("Use QQmlComponent or the QML engine for loading files.")
    bool connectDownloadProgress(QObject *, const char *);

    QT_DEPRECATED_VERSION_X_6_11("Use QQmlComponent or the QML engine for loading files.")
    bool connectDownloadProgress(QObject *, int);
#endif // QT_CONFIG(qml_network)

#endif // QT_DEPRECATED_SINCE(6, 11)

    static bool isSynchronous(const QString &url);
    static bool isSynchronous(const QUrl &url);

    static bool isLocalFile(const QString &url);
    static bool isLocalFile(const QUrl &url);

    static QString urlToLocalFileOrQrc(const QString &);
    static QString urlToLocalFileOrQrc(const QUrl &);
private:
    Q_DISABLE_COPY(QQmlFile)

#if QT_DEPRECATED_SINCE(6, 11)
    QQmlFilePrivate *d;
#else
    void *d = nullptr;
#endif
};

QT_END_NAMESPACE

#endif // QQMLFILE_H
