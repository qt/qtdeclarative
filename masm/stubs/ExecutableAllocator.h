/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the V4VM module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/
#ifndef MASM_EXECUTABLEALLOCATOR_H
#define MASM_EXECUTABLEALLOCATOR_H

#include <RefPtr.h>
#include <RefCounted.h>

#include <sys/mman.h>
#include <unistd.h>

namespace JSC {

class JSGlobalData;

struct ExecutableMemoryHandle : public RefCounted<ExecutableMemoryHandle> {
    ExecutableMemoryHandle(int size)
        : m_size(size)
    {
        static size_t pageSize = sysconf(_SC_PAGESIZE);
        m_size = (m_size + pageSize - 1) & ~(pageSize - 1);
#if OS(DARWIN)
#  define MAP_ANONYMOUS MAP_ANON
#endif
        m_data = mmap(0, m_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    }
    ~ExecutableMemoryHandle()
    {
        munmap(m_data, m_size);
    }

    inline void shrink(size_t) {
        // ### TODO.
    }

    inline bool isManaged() const { return true; }

    void* start() { return m_data; }
    int sizeInBytes() { return m_size; }

    void* m_data;
    int m_size;
};

struct ExecutableAllocator {
    PassRefPtr<ExecutableMemoryHandle> allocate(JSGlobalData&, int size, void*, int)
    {
        return adoptRef(new ExecutableMemoryHandle(size));
    }

    static void makeWritable(void*, int)
    {
    }

    static void makeExecutable(void* addr, int size)
    {
        static size_t pageSize = sysconf(_SC_PAGESIZE);
        size_t iaddr = reinterpret_cast<size_t>(addr);
        size_t roundAddr = iaddr & ~(pageSize - static_cast<size_t>(1));
        int mode = PROT_READ | PROT_WRITE | PROT_EXEC;
        mprotect(reinterpret_cast<void*>(roundAddr), size + (iaddr - roundAddr), mode);
    }
};

}

#endif // MASM_EXECUTABLEALLOCATOR_H
