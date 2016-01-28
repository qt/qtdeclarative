/***************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Copyright (C) 2013 BlackBerry Limited. All rights reserved.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Qt Labs Controls module of the Qt Toolkit.
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

#include <QtCore/QFile>
#include <QtCore/QDir>
#include <QtCore/QMutex>
#include <QtCore/QMutexLocker>
#include <QtCore/QUrl>
#include <QtCore/QFileInfo>
#include <QtCore/QLocale>
#include <QtCore/QDebug>
#include <QtCore/QSettings>

#include <QtGui/private/qguiapplication_p.h>
#include <QtLabsControls/private/qquickstyle_p.h>

QT_BEGIN_NAMESPACE

Q_GLOBAL_STATIC(QQuickStyleSelectorSharedData, sharedData);
static QBasicMutex sharedDataMutex;

QQuickStyleSelectorPrivate::QQuickStyleSelectorPrivate()
{
}

QQuickStyleSelector::QQuickStyleSelector() : d_ptr(new QQuickStyleSelectorPrivate)
{
    Q_D(QQuickStyleSelector);
    d->style = QGuiApplicationPrivate::styleOverride.toLower();
    if (d->style.isEmpty())
        d->style = QString::fromLatin1(qgetenv("QT_LABS_CONTROLS_STYLE")).toLower();
    if (d->style.isEmpty()) {
        QSharedPointer<QSettings> settings = QQuickStyle::settings(QStringLiteral("Controls"));
        if (settings)
            d->style = settings->value(QStringLiteral("Style")).toString().toLower();
    }
}

QQuickStyleSelector::~QQuickStyleSelector()
{
}

static bool isLocalScheme(const QString &file)
{
    bool local = file == QLatin1String("qrc");
#ifdef Q_OS_ANDROID
    local |= file == QLatin1String("assets");
#endif
    return local;
}

QString QQuickStyleSelector::select(const QString &filePath) const
{
    Q_D(const QQuickStyleSelector);
    QUrl url(d->baseUrl.toString() + filePath);
    if (isLocalScheme(url.scheme()) || url.isLocalFile()) {
        if (isLocalScheme(url.scheme())) {
            QString equivalentPath = QLatin1Char(':') + url.path();
            QString selectedPath = d->select(equivalentPath);
            url.setPath(selectedPath.remove(0, 1));
        } else {
            url = QUrl::fromLocalFile(d->select(url.toLocalFile()));
        }
    }
    return url.toString();
}

static QString selectionHelper(const QString &path, const QString &fileName, const QStringList &selectors)
{
    /* selectionHelper does a depth-first search of possible selected files. Because there is strict
       selector ordering in the API, we can stop checking as soon as we find the file in a directory
       which does not contain any other valid selector directories.
    */
    Q_ASSERT(path.isEmpty() || path.endsWith(QLatin1Char('/')));

    foreach (const QString &s, selectors) {
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
    if (!QFile::exists(path + fileName))
        return QString();
    return path + fileName;
}

QString QQuickStyleSelectorPrivate::select(const QString &filePath) const
{
    QFileInfo fi(filePath);
    // If file doesn't exist, don't select
    if (!fi.exists())
        return filePath;

    QString ret = selectionHelper(fi.path().isEmpty() ? QString() : fi.path() + QLatin1Char('/'),
            fi.fileName(), allSelectors());

    if (!ret.isEmpty())
        return ret;
    return filePath;
}

QString QQuickStyleSelector::style() const
{
    Q_D(const QQuickStyleSelector);
    return d->style;
}

void QQuickStyleSelector::setStyle(const QString &s)
{
    Q_D(QQuickStyleSelector);
    d->style = s;
}

QStringList QQuickStyleSelectorPrivate::allSelectors() const
{
    QMutexLocker locker(&sharedDataMutex);
    updateSelectors();
    QStringList selectors = sharedData->staticSelectors;
    if (!style.isEmpty())
        selectors.prepend(style);
    return selectors;
}

void QQuickStyleSelector::setBaseUrl(const QUrl &base)
{
    Q_D(QQuickStyleSelector);
    if (d->baseUrl != base)
        d->baseUrl = base;
}

QUrl QQuickStyleSelector::baseUrl() const
{
    Q_D(const QQuickStyleSelector);
    return d->baseUrl;
}

void QQuickStyleSelectorPrivate::updateSelectors()
{
    if (!sharedData->staticSelectors.isEmpty())
        return; //Already loaded

    sharedData->staticSelectors << sharedData->preloadedStatics; //Potential for static selectors from other modules

    // TODO: Update on locale changed?
    sharedData->staticSelectors << QLocale().name();

    sharedData->staticSelectors << platformSelectors();
}

QStringList QQuickStyleSelectorPrivate::platformSelectors()
{
    // similar, but not identical to QSysInfo::osType
    QStringList ret;
#if defined(Q_OS_WIN)
    // can't fall back to QSysInfo because we need both "winphone" and "winrt" for the Windows Phone case
    ret << QStringLiteral("windows");
    ret << QSysInfo::kernelType();  // "wince" and "winnt"
#  if defined(Q_OS_WINRT)
    ret << QStringLiteral("winrt");
#    if defined(Q_OS_WINPHONE)
    ret << QStringLiteral("winphone");
#    endif
#  endif
#elif defined(Q_OS_UNIX)
    ret << QStringLiteral("unix");
#  if !defined(Q_OS_ANDROID) && !defined(Q_OS_BLACKBERRY)
    // we don't want "linux" for Android or "qnx" for Blackberry here
    ret << QSysInfo::kernelType();
#     ifdef Q_OS_MAC
    ret << QStringLiteral("mac"); // compatibility, since kernelType() is "darwin"
#     endif
#  endif
    QString productName = QSysInfo::productType();
    if (productName != QLatin1String("unknown"))
        ret << productName; // "opensuse", "fedora", "osx", "ios", "blackberry", "android"
#endif
    return ret;
}

void QQuickStyleSelectorPrivate::addStatics(const QStringList &statics)
{
    QMutexLocker locker(&sharedDataMutex);
    sharedData->preloadedStatics << statics;
}

QT_END_NAMESPACE
