/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
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
****************************************************************************/

#ifndef QV4GC_H
#define QV4GC_H

#include "qv4object.h"

#include <QScopedPointer>

#define DETAILED_MM_STATS

namespace QQmlJS {
namespace VM {

struct Managed;

class MemoryManager
{
    MemoryManager(const MemoryManager &);
    MemoryManager &operator=(const MemoryManager&);

public:
    struct Data;

    class GCBlocker
    {
    public:
        GCBlocker(MemoryManager *mm)
            : mm(mm)
            , wasBlocked(mm->isGCBlocked())
        {
            mm->setGCBlocked(true);
        }

        ~GCBlocker()
        {
            mm->setGCBlocked(wasBlocked);
        }

    private:
        MemoryManager *mm;
        bool wasBlocked;
    };

public:
    MemoryManager();
    ~MemoryManager();

    void protect(Managed *m);
    void unprotect(Managed *m);

    // TODO: this is only for 64bit (and x86 with SSE/AVX), so exend it for other architectures to be slightly more efficient (meaning, align on 8-byte boundaries).
    // Note: all occurances of "16" in alloc/dealloc are also due to the alignment.
    static inline std::size_t align(std::size_t size)
    { return (size + 15) & ~0xf; }

    inline Managed *allocManaged(std::size_t size)
    {
        size = align(size);
        Managed *o = alloc(size);
        return o;
    }

    bool isGCBlocked() const;
    void setGCBlocked(bool blockGC);
    void runGC();

    void setEnableGC(bool enableGC);
    void setExecutionEngine(ExecutionEngine *engine);

    void dumpStats() const;

protected:
    /// expects size to be aligned
    // TODO: try to inline
    Managed *alloc(std::size_t size);

    void scribble(Managed *obj, int c, int size) const;

    void collectRootsOnStack(QVector<Managed *> &roots) const;

    ExecutionEngine *engine() const;

#ifdef DETAILED_MM_STATS
    void willAllocate(std::size_t size);
#endif // DETAILED_MM_STATS

private:
    void collectRoots(QVector<VM::Managed *> &roots) const;
    static void mark(const QVector<Managed *> &objects);
    std::size_t sweep();
    std::size_t sweep(char *chunkStart, std::size_t chunkSize, size_t size);

protected:
    QScopedPointer<Data> m_d;
};


} // namespace VM
} // namespace QQmlJS

#endif // QV4GC_H
