/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
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
#ifndef QQMLJSRESOURCEFILEMAPPER_P_H
#define QQMLJSRESOURCEFILEMAPPER_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.

#include <QStringList>
#include <QHash>
#include <QFile>

QT_BEGIN_NAMESPACE

struct QQmlJSResourceFileMapper
{
    struct Entry
    {
        QString resourcePath;
        QString filePath;
        bool isValid() const { return !resourcePath.isEmpty() && !filePath.isEmpty(); }
    };

    enum FilterMode {
        File      = 0x0, // Default is local (non-directory) file, without recursion
        Directory = 0x1, // Directory, either local or resource
        Resource  = 0x2, // Resource path, either to file or directory
        Recurse   = 0x4, // Recurse into subdirectories if Directory
    };
    Q_DECLARE_FLAGS(FilterFlags, FilterMode);

    struct Filter {
        QString path;
        QStringList suffixes;
        FilterFlags flags;
    };

    static Filter allQmlJSFilter();
    static Filter localFileFilter(const QString &file);
    static Filter resourceFileFilter(const QString &file);
    static Filter resourceQmlDirectoryFilter(const QString &directory);

    QQmlJSResourceFileMapper(const QStringList &resourceFiles);

    bool isEmpty() const;
    bool isFile(const QString &resourcePath) const;

    QList<Entry> filter(const Filter &filter) const;
    QStringList filePaths(const Filter &filter) const;
    QStringList resourcePaths(const Filter &filter) const;
    Entry entry(const Filter &filter) const;

private:
    void populateFromQrcFile(QFile &file);

    QList<Entry> qrcPathToFileSystemPath;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QQmlJSResourceFileMapper::FilterFlags);

QT_END_NAMESPACE

#endif // QMLJSRESOURCEFILEMAPPER_P_H
