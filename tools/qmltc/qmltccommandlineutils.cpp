/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the tools applications of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qmltccommandlineutils.h"

#include <QtCore/qstring.h>
#include <QtCore/qfileinfo.h>
#include <QtCore/qdir.h>
#include <QtCore/qurl.h>
#include <QtCore/qlibraryinfo.h>

QT_BEGIN_NAMESPACE

QString parseUrlArgument(const QString &arg)
{
    const QUrl url = QUrl::fromUserInput(arg, QDir::currentPath(), QUrl::AssumeLocalFile);
    if (!url.isValid()) {
        fprintf(stderr, "Invalid URL: \"%s\"\n", qPrintable(arg));
        return QString();
    }
    if (!url.isLocalFile()) {
        fprintf(stderr, "\"%s\" is not a local file\n", qPrintable(arg));
        return QString();
    }
    return url.toLocalFile();
}

QString loadUrl(const QString &url)
{
    const QFileInfo fi(url);
    if (!fi.exists()) {
        fprintf(stderr, "\"%s\" does not exist.\n",
                qPrintable(QDir::toNativeSeparators(fi.absoluteFilePath())));
        return QString();
    }

    QFile f(fi.absoluteFilePath());
    if (!f.open(QIODevice::ReadOnly)) {
        fprintf(stderr, "Unable to read \"%s\": %s.\n",
                qPrintable(QDir::toNativeSeparators(fi.absoluteFilePath())),
                qPrintable(f.errorString()));
        return QString();
    }

    QByteArray data(fi.size(), Qt::Uninitialized);
    if (f.read(data.data(), data.length()) != data.length()) {
        fprintf(stderr, "Unable to read \"%s\": %s.\n",
                qPrintable(QDir::toNativeSeparators(fi.absoluteFilePath())),
                qPrintable(f.errorString()));
        return QString();
    }
    return QString::fromUtf8(data);
}

QString getImplicitImportDirectory(const QString &url)
{
    const QFileInfo fi(url);
    Q_ASSERT(fi.exists());
    QDir dir = fi.dir();
    QString implicitImport = dir.canonicalPath(); // resolves symlinks, etc.
    if (implicitImport.isEmpty()) {
        fprintf(stderr, "Cannot resolve implicit import directory of \"%s\"",
                qPrintable(QDir::toNativeSeparators(fi.absoluteFilePath())));
        return QString();
    }
    return implicitImport;
}

QT_END_NAMESPACE
