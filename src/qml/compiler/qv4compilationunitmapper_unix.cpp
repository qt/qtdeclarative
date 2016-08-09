/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qv4compilationunitmapper_p.h"

#include <sys/mman.h>
#include <functional>
#include <private/qcore_unix_p.h>

#include "qv4compileddata_p.h"

QT_BEGIN_NAMESPACE

using namespace QV4;

namespace {
    struct Defer
    {
        std::function<void()> callback;
        template <typename Callback>
        Defer(Callback &&cb)
            : callback(cb)
        {}
        ~Defer() { callback(); }
        Defer(const Defer &) = delete;
        Defer &operator=(const Defer &) = delete;
    };
}

CompiledData::Unit *CompilationUnitMapper::open(const QString &sourcePath, QString *errorString)
{
    close();

    QByteArray cacheFileName = QFile::encodeName(sourcePath);
    cacheFileName.append('c');
    int fd = qt_safe_open(cacheFileName.constData(), O_RDONLY);
    if (fd == -1) {
        *errorString = qt_error_string(errno);
        return nullptr;
    }

    Defer cleanup([fd]{
       qt_safe_close(fd) ;
    });

    CompiledData::Unit header;
    qint64 bytesRead = qt_safe_read(fd, reinterpret_cast<char *>(&header), sizeof(header));

    if (bytesRead != sizeof(header)) {
        *errorString = QStringLiteral("File too small for the header fields");
        return nullptr;
    }

    if (!verifyHeader(&header, sourcePath, errorString))
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
    if (dataPtr != nullptr)
        munmap(dataPtr, length);
    dataPtr = nullptr;
}

QT_END_NAMESPACE
