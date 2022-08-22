// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquicklabsplatformstandardpaths_p.h"

#if QT_DEPRECATED_SINCE(6, 4)

#include <QtQml/qqmlengine.h>

QT_BEGIN_NAMESPACE

/*!
    \qmltype StandardPaths
    \inherits QtObject
//!     \instantiates QQuickLabsPlatformStandardPaths
    \inqmlmodule Qt.labs.platform
    \since 5.8
    \deprecated [6.4] Use QtCore::StandardPaths instead.
    \brief Provides access to the standard system paths.

    The StandardPaths singleton type provides methods for querying the standard
    system paths. The standard paths are mostly useful in conjunction with the
    FileDialog and FolderDialog types.

    \qml
    FileDialog {
        folder: StandardPaths.writableLocation(StandardPaths.DocumentsLocation)
    }
    \endqml

    \labs

    \sa QtCore::StandardPaths, FileDialog, FolderDialog, QStandardPaths
*/

static QList<QUrl> toUrlList(const QStringList &paths)
{
    QList<QUrl> urls;
    urls.reserve(paths.size());
    for (const QString &path : paths)
        urls += QUrl::fromLocalFile(path);
    return urls;
}

QQuickLabsPlatformStandardPaths::QQuickLabsPlatformStandardPaths(QObject *parent)
    : QObject(parent)
{
}

QObject *QQuickLabsPlatformStandardPaths::create(QQmlEngine *engine, QJSEngine *scriptEngine)
{
    Q_UNUSED(scriptEngine);
    return new QQuickLabsPlatformStandardPaths(engine);
}

/*!
    \qmlmethod string Qt.labs.platform::StandardPaths::displayName(StandardLocation type)

    \include standardpath/functiondocs.qdocinc displayName

    \sa QStandardPaths::displayName()
*/
QString QQuickLabsPlatformStandardPaths::displayName(QStandardPaths::StandardLocation type)
{
    return QStandardPaths::displayName(type);
}

/*!
    \qmlmethod url Qt.labs.platform::StandardPaths::findExecutable(string executableName, list<string> paths)

    \include standardpath/functiondocs.qdocinc findExecutable

    \sa QStandardPaths::findExecutable()
*/
QUrl QQuickLabsPlatformStandardPaths::findExecutable(const QString &executableName, const QStringList &paths)
{
    return QUrl::fromLocalFile(QStandardPaths::findExecutable(executableName, paths));
}

/*!
    \qmlmethod url Qt.labs.platform::StandardPaths::locate(StandardLocation type, string fileName, LocateOptions options)

    \include standardpath/functiondocs.qdocinc locate

    \sa QStandardPaths::locate()
*/
QUrl QQuickLabsPlatformStandardPaths::locate(QStandardPaths::StandardLocation type, const QString &fileName, QStandardPaths::LocateOptions options)
{
    return QUrl::fromLocalFile(QStandardPaths::locate(type, fileName, options));
}

/*!
    \qmlmethod list<url> Qt.labs.platform::StandardPaths::locateAll(StandardLocation type, string fileName, LocateOptions options)

    \include standardpath/functiondocs.qdocinc locateAll

    \sa QStandardPaths::locateAll()
*/
QList<QUrl> QQuickLabsPlatformStandardPaths::locateAll(QStandardPaths::StandardLocation type, const QString &fileName, QStandardPaths::LocateOptions options)
{
    return toUrlList(QStandardPaths::locateAll(type, fileName, options));
}

/*!
    \qmlmethod void Qt.labs.platform::StandardPaths::setTestModeEnabled(bool testMode)

    \include standardpath/functiondocs.qdocinc setTestModeEnabled

    \sa QStandardPaths::setTestModeEnabled()
*/
void QQuickLabsPlatformStandardPaths::setTestModeEnabled(bool testMode)
{
    QStandardPaths::setTestModeEnabled(testMode);
}

/*!
    \qmlmethod list<url> Qt.labs.platform::StandardPaths::standardLocations(StandardLocation type)

    \include standardpath/functiondocs.qdocinc standardLocations

    \sa QStandardPaths::standardLocations()
*/
QList<QUrl> QQuickLabsPlatformStandardPaths::standardLocations(QStandardPaths::StandardLocation type)
{
    return toUrlList(QStandardPaths::standardLocations(type));
}

/*!
    \qmlmethod url Qt.labs.platform::StandardPaths::writableLocation(StandardLocation type)

    \include standardpath/functiondocs.qdocinc writableLocation

    \sa QStandardPaths::writableLocation()
*/
QUrl QQuickLabsPlatformStandardPaths::writableLocation(QStandardPaths::StandardLocation type)
{
    return QUrl::fromLocalFile(QStandardPaths::writableLocation(type));
}

QT_END_NAMESPACE

#include "moc_qquicklabsplatformstandardpaths_p.cpp"

#endif // QT_DEPRECATED_SINCE(6, 4)
