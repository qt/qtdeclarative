// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qqmlstandardpaths_p.h"

#include <QtQml/qqmlengine.h>

QT_BEGIN_NAMESPACE

/*!
    \qmltype StandardPaths
    \inherits QtObject
    \inqmlmodule QtCore
    \since 6.2
    \brief Provides access to the standard system paths.

    The StandardPaths singleton type provides methods for querying the standard
    system paths.

    \qml
    property url documentsFolder: StandardPaths.writableLocation(StandardPaths.DocumentsLocation)
    \endqml

    \sa QStandardPaths
*/

static QList<QUrl> toUrlList(const QStringList &paths)
{
    QList<QUrl> urls;
    urls.reserve(paths.size());
    for (const QString &path : paths)
        urls += QUrl::fromLocalFile(path);
    return urls;
}

QQmlStandardPaths::QQmlStandardPaths(QObject *parent)
    : QObject(parent)
{
}

/*!
    \qmlmethod string QtCore::StandardPaths::displayName(StandardLocation type)

    \include standardpath/functiondocs.qdocinc displayName

    \sa QStandardPaths::displayName()
*/
QString QQmlStandardPaths::displayName(QStandardPaths::StandardLocation type) const
{
    return QStandardPaths::displayName(type);
}

/*!
    \qmlmethod url QtCore::StandardPaths::findExecutable(string executableName, list<string> paths) const

    \include standardpath/functiondocs.qdocinc findExecutable

    \sa QStandardPaths::findExecutable()
*/
QUrl QQmlStandardPaths::findExecutable(const QString &executableName, const QStringList &paths) const
{
    return QUrl::fromLocalFile(QStandardPaths::findExecutable(executableName, paths));
}

/*!
    \qmlmethod url QtCore::StandardPaths::locate(StandardLocation type, string fileName, LocateOptions options) const

    \include standardpath/functiondocs.qdocinc locate

    \sa QStandardPaths::locate()
*/
QUrl QQmlStandardPaths::locate(QStandardPaths::StandardLocation type, const QString &fileName,
    QStandardPaths::LocateOptions options) const
{
    return QUrl::fromLocalFile(QStandardPaths::locate(type, fileName, options));
}

/*!
    \qmlmethod list<url> QtCore::StandardPaths::locateAll(StandardLocation type, string fileName, LocateOptions options) const

    \include standardpath/functiondocs.qdocinc locateAll

    \sa QStandardPaths::locateAll()
*/
QList<QUrl> QQmlStandardPaths::locateAll(QStandardPaths::StandardLocation type, const QString &fileName,
    QStandardPaths::LocateOptions options) const
{
    return toUrlList(QStandardPaths::locateAll(type, fileName, options));
}

/*!
    \qmlmethod list<url> QtCore::StandardPaths::standardLocations(StandardLocation type)

    \include standardpath/functiondocs.qdocinc standardLocations

    \sa QStandardPaths::standardLocations()
*/
QList<QUrl> QQmlStandardPaths::standardLocations(QStandardPaths::StandardLocation type) const
{
    return toUrlList(QStandardPaths::standardLocations(type));
}

/*!
    \qmlmethod url QtCore::StandardPaths::writableLocation(StandardLocation type)

    \include standardpath/functiondocs.qdocinc writableLocation

    \sa QStandardPaths::writableLocation()
*/
QUrl QQmlStandardPaths::writableLocation(QStandardPaths::StandardLocation type) const
{
    return QUrl::fromLocalFile(QStandardPaths::writableLocation(type));
}

QT_END_NAMESPACE

#include "moc_qqmlstandardpaths_p.cpp"
