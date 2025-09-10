// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qqmljsresourcefilemapper_p.h"

#include <QFileInfo>
#include <QDir>
#include <QXmlStreamReader>

QT_BEGIN_NAMESPACE

QQmlJSResourceFileMapper::Filter QQmlJSResourceFileMapper::allQmlJSFilter() {
    return Filter {
        QString(),
        QStringList { QStringLiteral("qml"), QStringLiteral("js"), QStringLiteral("mjs") },
        Directory | Recurse
    };
}

QQmlJSResourceFileMapper::Filter QQmlJSResourceFileMapper::localFileFilter(const QString &file)
{
    return Filter {
        QDir::cleanPath(QFileInfo(file).absoluteFilePath()),
        QStringList(),
        File
    };
}

QQmlJSResourceFileMapper::Filter QQmlJSResourceFileMapper::resourceFileFilter(const QString &file)
{
    return Filter {
        file,
        QStringList(),
        Resource
    };
}

QQmlJSResourceFileMapper::Filter QQmlJSResourceFileMapper::resourceQmlDirectoryFilter(
        const QString &directory)
{
    return Filter {
        directory,
        QStringList { QStringLiteral("qml") },
        Directory | Resource
    };
}

QQmlJSResourceFileMapper::QQmlJSResourceFileMapper(const QStringList &resourceFiles)
{
    for (const QString &fileName: resourceFiles) {
        QFile f(fileName);
        if (!f.open(QIODevice::ReadOnly))
            continue;
        populateFromQrcFile(f);
    }
}

bool QQmlJSResourceFileMapper::isEmpty() const
{
    return qrcPathToFileSystemPath.isEmpty();
}

bool QQmlJSResourceFileMapper::isFile(const QString &resourcePath) const
{
    for (const auto &entry : qrcPathToFileSystemPath) {
        if (entry.resourcePath == resourcePath)
            return true;
    }
    return false;
}

static bool hasSuffix(const QString &qrcPath, const QStringList &suffixes)
{
    if (suffixes.isEmpty())
        return true;
    const QString suffix = QFileInfo(qrcPath).suffix();
    return suffixes.contains(suffix);
}

template<typename HandleMatch>
void doFilter(const QList<QQmlJSResourceFileMapper::Entry> &qrcPathToFileSystemPath,
              const QQmlJSResourceFileMapper::Filter &filter,
              const HandleMatch &handler)
{
    if (filter.flags & QQmlJSResourceFileMapper::Directory) {
        const QString terminatedDirectory = filter.path.endsWith(u'/')
                ? filter.path : (filter.path + u'/');

        for (auto it = qrcPathToFileSystemPath.constBegin(),
             end = qrcPathToFileSystemPath.constEnd(); it != end; ++it) {

            const QString candidate = (filter.flags & QQmlJSResourceFileMapper::Resource)
                    ? it->resourcePath
                    : it->filePath;

            if (!filter.path.isEmpty() && !candidate.startsWith(terminatedDirectory))
                continue;

            if (!hasSuffix(candidate, filter.suffixes))
                continue;

            if ((filter.flags & QQmlJSResourceFileMapper::Recurse)
                    // Crude. But shall we really allow slashes in QRC file names?
                    || !candidate.mid(terminatedDirectory.size()).contains(u'/')) {
                if (handler(*it))
                    return;
            }
        }
        return;
    }

    if (!hasSuffix(filter.path, filter.suffixes))
        return;

    for (auto it = qrcPathToFileSystemPath.constBegin(),
         end = qrcPathToFileSystemPath.constEnd(); it != end; ++it) {
        if (filter.flags & QQmlJSResourceFileMapper::Resource) {
            if (it->resourcePath == filter.path && handler(*it))
                return;
        } else if (it->filePath == filter.path && handler(*it)) {
            return;
        }
    }
}

QList<QQmlJSResourceFileMapper::Entry> QQmlJSResourceFileMapper::filter(
        const QQmlJSResourceFileMapper::Filter &filter) const
{
    QList<Entry> result;
    doFilter(qrcPathToFileSystemPath, filter, [&](const Entry &entry) {
        result.append(entry);
        return false;
    });
    return result;
}

QStringList QQmlJSResourceFileMapper::filePaths(
        const QQmlJSResourceFileMapper::Filter &filter) const
{
    QStringList result;
    doFilter(qrcPathToFileSystemPath, filter, [&](const Entry &entry) {
        result.append(entry.filePath);
        return false;
    });
    return result;
}

QStringList QQmlJSResourceFileMapper::resourcePaths(
        const QQmlJSResourceFileMapper::Filter &filter) const
{
    QStringList result;
    doFilter(qrcPathToFileSystemPath, filter, [&](const Entry &entry) {
        result.append(entry.resourcePath);
        return false;
    });
    return result;
}

QQmlJSResourceFileMapper::Entry QQmlJSResourceFileMapper::entry(
        const QQmlJSResourceFileMapper::Filter &filter) const
{
    Entry result;
    doFilter(qrcPathToFileSystemPath, filter, [&](const Entry &entry) {
        result = entry;
        return true;
    });
    return result;
}

void QQmlJSResourceFileMapper::populateFromQrcFile(QFile &file)
{
    enum State {
        InitialState,
        InRCC,
        InResource,
        InFile
    };
    State state = InitialState;

    QDir qrcDir = QFileInfo(file).absoluteDir();

    QString prefix;
    QString currentFileName;
    QXmlStreamAttributes currentFileAttributes;

    QXmlStreamReader reader(&file);
    while (!reader.atEnd()) {
        switch (reader.readNext()) {
        case QXmlStreamReader::StartElement:
            if (reader.name() == QStringLiteral("RCC")) {
                if (state != InitialState)
                    return;
                state = InRCC;
                continue;
            } else if (reader.name() == QStringLiteral("qresource")) {
                if (state != InRCC)
                    return;
                state = InResource;
                QXmlStreamAttributes attributes = reader.attributes();
                if (attributes.hasAttribute(QStringLiteral("prefix")))
                    prefix = attributes.value(QStringLiteral("prefix")).toString();
                if (!prefix.startsWith(QLatin1Char('/')))
                    prefix.prepend(QLatin1Char('/'));
                if (!prefix.endsWith(QLatin1Char('/')))
                    prefix.append(QLatin1Char('/'));
                continue;
            } else if (reader.name() == QStringLiteral("file")) {
                if (state != InResource)
                    return;
                state = InFile;
                currentFileAttributes = reader.attributes();
                continue;
            }
            return;

        case QXmlStreamReader::EndElement:
            if (reader.name() == QStringLiteral("file")) {
                if (state != InFile)
                    return;
                state = InResource;
                continue;
            } else if (reader.name() == QStringLiteral("qresource")) {
                if (state != InResource)
                    return;
                state = InRCC;
                continue;
            } else if (reader.name() == QStringLiteral("RCC")) {
                if (state != InRCC)
                    return;
                state = InitialState;
                continue;
            }
            return;

        case QXmlStreamReader::Characters: {
            if (reader.isWhitespace())
                break;
            if (state != InFile)
                return;
            currentFileName = reader.text().toString();
            if (currentFileName.isEmpty())
                continue;

            const QString fsPath = QDir::cleanPath(qrcDir.absoluteFilePath(currentFileName));

            if (currentFileAttributes.hasAttribute(QStringLiteral("alias")))
                currentFileName = currentFileAttributes.value(QStringLiteral("alias")).toString();

            currentFileName = QDir::cleanPath(currentFileName);
            while (currentFileName.startsWith(QLatin1String("../")))
                currentFileName.remove(0, 3);

            const QString qrcPath = prefix + currentFileName;
            if (QFile::exists(fsPath))
                qrcPathToFileSystemPath.append({qrcPath, fsPath});
            continue;
        }

        default: break;
    }
    }
}

QT_END_NAMESPACE
