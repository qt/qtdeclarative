/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

#include "qv4compilationunitmapper_p.h"

#include <sys/mman.h>
#include <functional>
#include <private/qcore_unix_p.h>
#include <QScopeGuard>
#include <QDateTime>

#include "qv4executablecompilationunit_p.h"

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

    if (!ExecutableCompilationUnit::verifyHeader(&header, sourceTimeStamp, errorString))
        return nullptr;

    // Data structure and qt version matched, so now we can access the rest of the file safely.

    length = static_cast<size_t>(lseek(fd, 0, SEEK_END));

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
