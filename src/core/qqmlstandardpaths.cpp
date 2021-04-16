/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Labs Platform module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

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
