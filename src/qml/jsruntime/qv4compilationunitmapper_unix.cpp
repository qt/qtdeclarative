// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qv4compilationunitmapper_p.h"

#include <private/qcore_unix_p.h>
#include <private/qv4compileddata_p.h>

#include <QtCore/qscopeguard.h>
#include <QtCore/qdatetime.h>

#include <functional>
#include <sys/mman.h>

QT_BEGIN_NAMESPACE

using namespace QV4;

CompiledData::Unit *CompilationUnitMapper::open(const QString &cacheFileName, const QDateTime &sourceTimeStamp, QString *errorString)
{
    close();

    int fd = qt_safe_open(QFile::encodeName(cacheFileName).constData(), O_RDONLY);
    if (fd == -1) {
        *errorString = qt_error_string(errno);
        return nullptr;
    }

    auto cleanup = qScopeGuard([fd]{
       qt_safe_close(fd) ;
    });

    CompiledData::Unit header;
    qint64 bytesRead = qt_safe_read(fd, reinterpret_cast<char *>(&header), sizeof(header));

    if (bytesRead != sizeof(header)) {
        *errorString = QStringLiteral("File too small for the header fields");
        return nullptr;
    }

    if (!header.verifyHeader(sourceTimeStamp, errorString))
        return nullptr;

    // Data structure and qt version matched, so now we can access the rest of the file safely.

    length = static_cast<size_t>(lseek(fd, 0, SEEK_END));
    /* Error out early on file corruption. We assume we can read header.unitSize bytes
       later (even before verifying the checksum), potentially causing out-of-bound
       reads
       Also, no need to wait until checksum verification if we know beforehand
       that the cached unit is bogus
    */
    if (length != header.unitSize) {
        *errorString = QStringLiteral("Potential file corruption, file too small");
        return nullptr;
    }

    void *ptr = mmap(nullptr, length, PROT_READ, MAP_SHARED, fd, /*offset*/0);
    if (ptr == MAP_FAILED) {
        *errorString = qt_error_string(errno);
        return nullptr;
    }
    dataPtr = ptr;

    return reinterpret_cast<CompiledData::Unit*>(dataPtr);
}

void CompilationUnitMapper::close()
{
    // Do not unmap the data here.
    if (dataPtr != nullptr) {
        // Do not unmap cache files that are built with the StaticData flag. That's the majority of
        // them and it's necessary to benefit from the QString literal optimization. There might
        // still be QString instances around that point into that memory area. The memory is backed
        // on the disk, so the kernel is free to release the pages and all that remains is the
        // address space allocation.
        if (!(reinterpret_cast<CompiledData::Unit*>(dataPtr)->flags & CompiledData::Unit::StaticData))
            munmap(dataPtr, length);
    }
    dataPtr = nullptr;
}

QT_END_NAMESPACE
