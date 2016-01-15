/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/
#ifndef MASM_EXECUTABLEALLOCATOR_H
#define MASM_EXECUTABLEALLOCATOR_H

#include <RefPtr.h>
#include <RefCounted.h>
#include <wtf/PageBlock.h>

#include <private/qv4executableallocator_p.h>

#if OS(WINDOWS)
#include <windows.h>
#else
#include <sys/mman.h>
#include <unistd.h>
#endif

namespace JSC {

class JSGlobalData;

struct ExecutableMemoryHandle : public RefCounted<ExecutableMemoryHandle> {
    ExecutableMemoryHandle(QV4::ExecutableAllocator *allocator, int size)
        : m_allocator(allocator)
        , m_size(size)
    {
        m_allocation = allocator->allocate(size);
    }
    ~ExecutableMemoryHandle()
    {
        m_allocation->deallocate(m_allocator);
    }

    inline void shrink(size_t) {
        // ### TODO.
    }

    inline bool isManaged() const { return true; }

    void* start() { return m_allocation->start(); }
    int sizeInBytes() { return m_size; }

    QV4::ExecutableAllocator::ChunkOfPages *chunk() const
    { return m_allocator->chunkForAllocation(m_allocation); }

    QV4::ExecutableAllocator *m_allocator;
    QV4::ExecutableAllocator::Allocation *m_allocation;
    int m_size;
};

struct ExecutableAllocator {
    ExecutableAllocator(QV4::ExecutableAllocator *alloc)
        : realAllocator(alloc)
    {}

    PassRefPtr<ExecutableMemoryHandle> allocate(JSGlobalData&, int size, void*, int)
    {
        return adoptRef(new ExecutableMemoryHandle(realAllocator, size));
    }

    static void makeWritable(void* addr, size_t size)
    {
        quintptr pageSize = WTF::pageSize();
        quintptr iaddr = reinterpret_cast<quintptr>(addr);
        quintptr roundAddr = iaddr & ~(pageSize - 1);
        size = size + (iaddr - roundAddr);
        addr = reinterpret_cast<void*>(roundAddr);

#if ENABLE(ASSEMBLER_WX_EXCLUSIVE)
#  if OS(WINDOWS)
        DWORD oldProtect;
#    if !OS(WINRT)
        VirtualProtect(addr, size, PAGE_READWRITE, &oldProtect);
#    elif _MSC_VER >= 1900
        bool hr = VirtualProtectFromApp(addr, size, PAGE_READWRITE, &oldProtect);
        if (!hr) {
            Q_UNREACHABLE();
        }
#    else
        (void)oldProtect;
#    endif
#  else
        int mode = PROT_READ | PROT_WRITE;
        if (mprotect(addr, size, mode) != 0) {
            perror("mprotect failed in ExecutableAllocator::makeWritable");
            Q_UNREACHABLE();
        }
#  endif
#else
        // We assume we already have RWX
        (void)addr; // suppress unused parameter warning
        (void)size; // suppress unused parameter warning
#endif
    }

    static void makeExecutable(void* addr, size_t size)
    {
        quintptr pageSize = WTF::pageSize();
        quintptr iaddr = reinterpret_cast<quintptr>(addr);
        quintptr roundAddr = iaddr & ~(pageSize - 1);
        size = size + (iaddr - roundAddr);
        addr = reinterpret_cast<void*>(roundAddr);

#if ENABLE(ASSEMBLER_WX_EXCLUSIVE)
#  if OS(WINDOWS)
        DWORD oldProtect;
#    if !OS(WINRT)
        VirtualProtect(addr, size, PAGE_EXECUTE_READ, &oldProtect);
#    elif _MSC_VER >= 1900
        bool hr = VirtualProtectFromApp(addr, size, PAGE_EXECUTE_READ, &oldProtect);
        if (!hr) {
            Q_UNREACHABLE();
        }
#    else
        (void)oldProtect;
#    endif
#  else
        int mode = PROT_READ | PROT_EXEC;
        if (mprotect(addr, size, mode) != 0) {
            perror("mprotect failed in ExecutableAllocator::makeExecutable");
            Q_UNREACHABLE();
        }
#  endif
#else
#  error "Only W^X is supported"
#endif
    }

    QV4::ExecutableAllocator *realAllocator;
};

}

#endif // MASM_EXECUTABLEALLOCATOR_H
