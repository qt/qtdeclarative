/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QQMLBUNDLE_P_H
#define QQMLBUNDLE_P_H

#include <QtCore/qfile.h>
#include <QtCore/qstring.h>
#include <QtQml/qtqmlglobal.h>

#ifdef Q_CC_MSVC
// nonstandard extension used : zero-sized array in struct/union.
#  pragma warning( disable : 4200 )
#endif

QT_BEGIN_NAMESPACE

class Q_QML_EXPORT QQmlBundle
{
    Q_DISABLE_COPY(QQmlBundle)
public:
    struct Q_PACKED Q_QML_EXPORT Entry
    {
        enum Kind {
            File = 123, // Normal file
            Skip,       // Empty space
            Link        // A meta data linked file

            // ### add entries for qmldir, index, ...
        };

        int kind;
        quint64 size;
    };

    struct Q_PACKED Q_QML_EXPORT RawEntry : public Entry
    {
        char data[]; // trailing data
    };

    struct Q_PACKED Q_QML_EXPORT FileEntry : public Entry
    {
        quint64 link;
        int fileNameLength;
        char data[]; // trailing data

        QString fileName() const;
        bool isFileName(const QString &) const;

        quint64 fileSize() const;
        const char *contents() const;
    };

    QQmlBundle(const QString &fileName);
    ~QQmlBundle();

    bool open(QIODevice::OpenMode mode = QIODevice::ReadWrite);
    void close();

    QList<const FileEntry *> files() const;
    void remove(const FileEntry *entry);
    bool add(const QString &fileName);
    bool add(const QString &name, const QString &fileName);

    bool addMetaLink(const QString &fileName,
                     const QString &linkName,
                     const QByteArray &data);

    const FileEntry *find(const QString &fileName) const;
    const FileEntry *find(const QChar *fileName, int length) const;

    const FileEntry *link(const FileEntry *, const QString &linkName) const;

    static int bundleHeaderLength();
    static bool isBundleHeader(const char *, int size);
private:
    const Entry *findInsertPoint(quint64 size, qint64 *offset);

private:
    QFile file;
    uchar *buffer;
    quint64 bufferSize;
    bool opened:1;
    bool headerWritten:1;
};

QT_END_NAMESPACE

#endif // QQMLBUNDLE_P_H
