/***************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Copyright (C) 2013 BlackBerry Limited. All rights reserved.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Qt Quick Controls 2 module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qquickstyleselector_p.h"
#include "qquickstyleselector_p_p.h"
#include "qquickstyle.h"

#include <QtCore/qdir.h>
#include <QtCore/qfile.h>
#include <QtCore/qfileinfo.h>
#include <QtCore/qsysinfo.h>
#include <QtCore/qlocale.h>

#include <QtCore/private/qfileselector_p.h>
#include <QtGui/private/qguiapplication_p.h>

QT_BEGIN_NAMESPACE

static bool isLocalScheme(const QString &scheme)
{
    bool local = scheme == QLatin1String("qrc");
#ifdef Q_OS_ANDROID
    local |= scheme == QLatin1String("assets");
#endif
    return local;
}

static QStringList allSelectors(const QString &style = QString())
{
    static const QStringList platformSelectors = QFileSelectorPrivate::platformSelectors();
    QStringList selectors = platformSelectors;
    selectors += QLocale().name();
    if (!style.isEmpty())
        selectors.prepend(style);
    return selectors;
}

static QString selectionHelper(const QString &path, const QString &fileName, const QStringList &selectors)
{
    /* selectionHelper does a depth-first search of possible selected files. Because there is strict
       selector ordering in the API, we can stop checking as soon as we find the file in a directory
       which does not contain any other valid selector directories.
    */
    Q_ASSERT(path.isEmpty() || path.endsWith(QLatin1Char('/')));

    for (const QString &s : selectors) {
        QString prospectiveBase = path + s + QLatin1Char('/');
        QStringList remainingSelectors = selectors;
        remainingSelectors.removeAll(s);
        if (!QDir(prospectiveBase).exists())
            continue;
        QString prospectiveFile = selectionHelper(prospectiveBase, fileName, remainingSelectors);
        if (!prospectiveFile.isEmpty())
            return prospectiveFile;
    }

    // If we reach here there were no successful files found at a lower level in this branch, so we
    // should check this level as a potential result.
    const QString result = path + fileName;
    if (!QFileInfo::exists(result))
        return QString();
    return result;
}

QString QQuickStyleSelectorPrivate::select(const QString &filePath) const
{
    QFileInfo fi(filePath);
    // If file doesn't exist, don't select
    if (!fi.exists())
        return filePath;

    const QString path = fi.path();
    const QString ret = selectionHelper(path.isEmpty() ? QString() : path + QLatin1Char('/'),
                                        fi.fileName(), allSelectors(style));

    if (!ret.isEmpty())
        return ret;
    return filePath;
}

QQuickStyleSelector::QQuickStyleSelector() : d_ptr(new QQuickStyleSelectorPrivate)
{
    Q_D(QQuickStyleSelector);
    d->style = QQuickStyle::name();
}

QQuickStyleSelector::~QQuickStyleSelector()
{
}

QUrl QQuickStyleSelector::baseUrl() const
{
    Q_D(const QQuickStyleSelector);
    return d->baseUrl;
}

void QQuickStyleSelector::setBaseUrl(const QUrl &url)
{
    Q_D(QQuickStyleSelector);
    d->baseUrl = url;
}

QString QQuickStyleSelector::select(const QString &fileName) const
{
    Q_D(const QQuickStyleSelector);
    const QString overridePath = QQuickStyle::path();
    if (!overridePath.isEmpty()) {
        const QString stylePath = overridePath + d->style + QLatin1Char('/');
        if (QFile::exists(stylePath + fileName)) {
            // the style name is included to the path, so exclude it from the selectors.
            // the rest of the selectors (os, locale) are still valid, though.
            const QString selectedPath = selectionHelper(stylePath, fileName, allSelectors());
            if (selectedPath.startsWith(QLatin1Char(':')))
                return QLatin1String("qrc") + selectedPath;
            return QUrl::fromLocalFile(QFileInfo(selectedPath).absoluteFilePath()).toString();
        }
    }

    QString base = d->baseUrl.toString();
    if (!base.isEmpty() && !base.endsWith(QLatin1Char('/')))
        base += QLatin1Char('/');

    QUrl url(base + fileName);
    if (isLocalScheme(url.scheme())) {
        QString equivalentPath = QLatin1Char(':') + url.path();
        QString selectedPath = d->select(equivalentPath);
        url.setPath(selectedPath.remove(0, 1));
    } else if (url.isLocalFile()) {
        url = QUrl::fromLocalFile(d->select(url.toLocalFile()));
    }
    return url.toString();
}

QT_END_NAMESPACE
