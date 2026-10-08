// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#include "qv4compilationunitmapper_p.h"

#include <private/qv4compileddata_p.h>

#include <QtCore/qdatetime.h>
#include <QtCore/qfileinfo.h>
#include <QtCore/qscopeguard.h>

#include <qt_windows.h>

QT_BEGIN_NAMESPACE

using namespace QV4;

CompiledData::Unit *CompilationUnitMapper::open(const QString &cacheFileName, const QDateTime &sourceTimeStamp, QString *errorString)
{
    close();

    // ### TODO: fix up file encoding/normalization/unc handling once QFileSystemEntry
    // is exported from QtCore.
    HANDLE handle =
        CreateFile(reinterpret_cast<const wchar_t*>(cacheFileName.constData()),
                   GENERIC_READ | GENERIC_EXECUTE, FILE_SHARE_READ,
                   nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,
                   nullptr);
    if (handle == INVALID_HANDLE_VALUE) {
        *errorString = qt_error_string(GetLastError());
        return nullptr;
    }

    auto fileHandleCleanup = qScopeGuard([handle]{
        CloseHandle(handle);
    });

    CompiledData::Unit header;
    DWORD bytesRead;
    if (!ReadFile(handle, reinterpret_cast<char *>(&header), sizeof(header), &bytesRead, nullptr)) {
        *errorString = qt_error_string(GetLastError());
        return nullptr;
    }

    if (bytesRead != sizeof(header)) {
        *errorString = QStringLiteral("File too small for the header fields");
        return nullptr;
    }

    if (!header.verifyHeader(sourceTimeStamp, errorString))
        return nullptr;

    // Data structure and qt version matched, so now we can access the rest of the file safely.

    /* Error out early on file corruption. We assume we can read header.unitSize bytes
       later (even before verifying the checksum), potentially causing out-of-bound
       reads
       Also, no need to wait until checksum verification if we know beforehand
       that the cached unit is bogus
    */
    LARGE_INTEGER fileSize;
    if (!GetFileSizeEx(handle, &fileSize)) {
        *errorString = QStringLiteral("Could not determine file size");
        return nullptr;
    }
    if (header.unitSize != fileSize.QuadPart) {
        *errorString = QStringLiteral("Potential file corruption, file too small");
        return nullptr;
    }


    HANDLE fileMappingHandle = CreateFileMapping(handle, 0, PAGE_READONLY, 0, 0, 0);
    if (!fileMappingHandle) {
        *errorString = qt_error_string(GetLastError());
        return nullptr;
    }

    auto mappingCleanup = qScopeGuard([fileMappingHandle]{
        CloseHandle(fileMappingHandle);
    });

    dataPtr = MapViewOfFile(fileMappingHandle, FILE_MAP_READ, 0, 0, 0);
    if (!dataPtr) {
        *errorString = qt_error_string(GetLastError());
        return nullptr;
    }

    return reinterpret_cast<CompiledData::Unit*>(dataPtr);
}

void CompilationUnitMapper::close()
{
    if (dataPtr != nullptr) {
        // Do not unmap cache files that are built with the StaticData flag. That's the majority of
        // them and it's necessary to benefit from the QString literal optimization. There might
        // still be QString instances around that point into that memory area. The memory is backed
        // on the disk, so the kernel is free to release the pages and all that remains is the
        // address space allocation.
        if (!(reinterpret_cast<CompiledData::Unit*>(dataPtr)->flags & CompiledData::Unit::StaticData))
            UnmapViewOfFile(dataPtr);
    }
    dataPtr = nullptr;
}

QT_END_NAMESPACE
