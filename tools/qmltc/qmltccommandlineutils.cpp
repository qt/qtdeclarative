// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
    if (f.read(data.data(), data.size()) != data.size()) {
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
