// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#include "qqmltypeloaderdata_p.h"

#include <private/qqmldirdata_p.h>
#include <private/qqmlprofiler_p.h>
#include <private/qqmlscriptblob_p.h>
#include <private/qqmltypedata_p.h>

#include <QtCore/qdirlisting.h>
#include <QtCore/qdir.h>

QT_BEGIN_NAMESPACE

static constexpr QDirListing::IteratorFlags dirListingFlags()
{
    return QDirListing::IteratorFlag::CaseSensitive | QDirListing::IteratorFlag::IncludeHidden;
}

enum class FileSetPopulateResult { NotFound, Found, Overflow };
static FileSetPopulateResult populateFileSet(QCache<QString, bool> *fileSet, const QString &path,
                                             const QString &file)
{
    const QDirListing listing(path, dirListingFlags());
    bool seen = false;
    for (const auto &entry : listing) {
        const QString next = entry.fileName();
        if (next == file)
            seen = true;
        fileSet->insert(next, new bool(true));
        if (fileSet->totalCost() >= fileSet->maxCost())
            break;
    }

    if (seen)
        return FileSetPopulateResult::Found;
    if (fileSet->totalCost() < fileSet->maxCost())
        return FileSetPopulateResult::NotFound;
    return FileSetPopulateResult::Overflow;
}

bool QQmlTypeLoaderSharedData::ImportDirCache::fileExists(const QString &path, const QString &file)
{
    QCache<QString, bool> *fileSet = content.object(path);
    if (fileSet) {
        if (const bool *exists = fileSet->object(file))
            return *exists;

        // If the cache isn't full, we know that we've scanned the whole directory.
        // The file not being in the cache then means it doesn't exist.
        if (fileSet->totalCost() < fileSet->maxCost())
            return false;

    } else if (content.contains(path)) {
        // explicit nullptr in cache
        return false;
    }

    const QDir dir(path);

    if (!fileSet) {
        // First try to cache the whole directory, but only up to the maxCost of the cache.

        fileSet = dir.exists() ? new QCache<QString, bool> : nullptr;
        const bool inserted = content.insert(path, fileSet);
        Q_ASSERT(inserted);
        if (!fileSet)
            return false;

        switch (populateFileSet(fileSet, dir.path(), file)) {
        case FileSetPopulateResult::NotFound:
            return false;
        case FileSetPopulateResult::Found:
            return true;
        case FileSetPopulateResult::Overflow:
            break;
        }

        // Cache overflow. Look up files individually
    } else {
        // If the directory was completely cached, we'd have returned early above.
        Q_ASSERT(fileSet->totalCost() == fileSet->maxCost());
    }

    const QDirListing singleFile(dir.path(), { file }, dirListingFlags());
    const bool exists = singleFile.begin() != singleFile.end();
    fileSet->insert(file, new bool(exists));
    Q_ASSERT(fileSet->totalCost() == fileSet->maxCost());
    return exists;
}

bool QQmlTypeLoaderSharedData::ImportDirCache::directoryExists(const QString &dirPath)
{
    if (!content.contains(dirPath)) {
        if (QDir(dirPath).exists()) {
            QCache<QString, bool> *files = new QCache<QString, bool>;
            populateFileSet(files, dirPath, QString());
            content.insert(dirPath, files);
            return true;
        }

        content.insert(dirPath, nullptr);
        return false;
    }

    return content.object(dirPath) != nullptr;
}

QQmlTypeLoaderLockedData::QQmlTypeLoaderLockedData(QV4::ExecutionEngine *engine) : m_engine(engine)
{
}

QT_END_NAMESPACE
