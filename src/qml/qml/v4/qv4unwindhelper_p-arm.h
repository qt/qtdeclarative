/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the test suite of the Qt Toolkit.
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

#ifndef QV4UNWINDHELPER_PDW2_H
#define QV4UNWINDHELPER_PDW2_H

#include "qv4unwindhelper_p.h"
#include "qv4functionobject_p.h"
#include "qv4function_p.h"
#include <wtf/Platform.h>

#include <QMap>
#include <QMutex>

#define __USE_GNU
#include <dlfcn.h>

#if USE(LIBUNWIND_DEBUG)
#include <libunwind.h>
#include <execinfo.h>
#endif

namespace QV4 {

static void *removeThumbBit(void *addr)
{
    return reinterpret_cast<void*>(reinterpret_cast<intptr_t>(addr) & ~1u);
}

static QMutex functionProtector;
static QMap<quintptr, Function*> allFunctions;

static Function *lookupFunction(void *pc)
{
    quintptr key = reinterpret_cast<quintptr>(pc);
    QMap<quintptr, Function*>::ConstIterator it = allFunctions.lowerBound(key);
    if (it != allFunctions.begin() && allFunctions.count() > 0)
        --it;
    if (it == allFunctions.end())
        return 0;

    quintptr codeStart = reinterpret_cast<quintptr>(removeThumbBit((*it)->codeRef.code().executableAddress()));
    if (key < codeStart || key >= codeStart + (*it)->codeSize)
        return 0;
    return *it;
}


/* Program:
vsp = r4 (REG_TO_SP r4)
vsp -= 8 * 4 -- > vsp = vsp - (7 << 2) - 4
pop r12, r10, r9, r8, r7, r6, r5, r4
pop r4
pop lr
pop r0, r1, r2, r3
*/

#define REG_TO_SP 0b10010000
#define VSP_MINUS 0b01000000
#define POP_REG_MULTI 0b10000000
#define POP_R4_MULTI     0b10100000
#define POP_R4_R14_MULTI 0b10101000
#define POP_R0_TO_R3 0b10110001
#define FINISH 0b10110000

#define MK_UW_WORD(first, second, third, fourth) \
           (((first) << 24) | \
            ((second) << 16) | \
            ((third) << 8) | \
            (fourth))

static unsigned int extbl[] = {
    MK_UW_WORD(0x80 |                          // High bit set to indicate that this isn't a PREL31
               2,                              // Choose personality routine #2
               2,                              // Number of 4 byte words used to encode remaining unwind instructions
               REG_TO_SP | 4,                  // Encoded program from above.
               VSP_MINUS | 7),
    MK_UW_WORD(POP_REG_MULTI | 1, 0b01111111,
               POP_R4_R14_MULTI,
               POP_R0_TO_R3),
    MK_UW_WORD(0b00001111,
               FINISH,
               FINISH,
               FINISH)
};

static unsigned write_prel31(unsigned *addr, void *ptr)
{
    int delta = (char *)ptr - (char*)addr;
    if (delta < 0)
        delta |= (1 << 30);
    else
        delta &= ~(1 << 30);
    *addr = ((unsigned)delta) & 0x7fffffffU;
}

void UnwindHelper::deregisterFunction(Function *function)
{
    QMutexLocker locker(&functionProtector);
    allFunctions.remove(reinterpret_cast<quintptr>(function->code));
}

void UnwindHelper::deregisterFunctions(const QVector<Function *> &functions)
{
    QMutexLocker locker(&functionProtector);
    foreach (Function *f, functions)
        allFunctions.remove(reinterpret_cast<quintptr>(f->code));
}

void UnwindHelper::registerFunction(Function *function)
{
    QMutexLocker locker(&functionProtector);
    allFunctions.insert(reinterpret_cast<quintptr>(function->code), function);
}

void UnwindHelper::registerFunctions(const QVector<Function *> &functions)
{
    QMutexLocker locker(&functionProtector);
    foreach (Function *f, functions)
        allFunctions.insert(reinterpret_cast<quintptr>(f->code), f);
}

void UnwindHelper::prepareForUnwind(ExecutionContext *)
{
}

int UnwindHelper::unwindInfoSize()
{
    return 2 * sizeof(unsigned int) // 2 extbl entries
           + sizeof(extbl);
}

void UnwindHelper::writeARMUnwindInfo(void *codeAddr, int codeSize)
{
    unsigned int *exidx = (unsigned int *)((char *)codeAddr + codeSize);

    unsigned char *exprog = (unsigned char *)((unsigned char *)codeAddr + codeSize + 8);

    write_prel31(exidx, codeAddr);
    exidx[1] = 4; // PREL31 offset to extbl, which follows right afterwards

    memcpy(exprog, extbl, sizeof(extbl));

#if USE(LIBUNWIND_DEBUG)
    unw_dyn_info_t *info = (unw_dyn_info_t*)malloc(sizeof(unw_dyn_info_t));
    info->start_ip = (unw_word_t)codeAddr;
    info->end_ip = info->start_ip + codeSize;
    info->gp = 0;
    info->format = UNW_INFO_FORMAT_ARM_EXIDX;
    info->u.rti.name_ptr = 0;
    info->u.rti.segbase = 0;
    info->u.rti.table_len = 8;
    info->u.rti.table_data = (unw_word_t)exidx;
    _U_dyn_register(info);
#endif
}

}

extern "C" Q_DECL_EXPORT void *__gnu_Unwind_Find_exidx(void *pc, int *entryCount)
{
    typedef void *(*Old_Unwind_Find_exidx)(void*, int*);
    static Old_Unwind_Find_exidx oldFunction = 0;
    static ptrdiff_t *exidx = (ptrdiff_t*)malloc(2 * sizeof(uintptr_t));
    if (!oldFunction)
        oldFunction = (Old_Unwind_Find_exidx)dlsym(RTLD_NEXT, "__gnu_Unwind_Find_exidx");

    {
        QMutexLocker locker(&QV4::functionProtector);
        QV4::Function *function = QV4::lookupFunction(pc);
        if (function) {
            *entryCount = 1;
            void * codeStart = QV4::removeThumbBit(function->codeRef.code().executableAddress());
            // At the end of the function we store our synthetic exception table entry.
            return (char *)codeStart + function->codeSize;
        }
    }

    return oldFunction(pc, entryCount);
}

#endif // QV4UNWINDHELPER_PDW2_H
