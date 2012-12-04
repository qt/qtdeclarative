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

#include "qmljs_engine.h"
#include "qv4mm_moth.h"

#include <QList>

using namespace QQmlJS;
using namespace QQmlJS::Moth;

MemoryManager::MemoryManager()
{
    stackFrames.reserve(64);
}

MemoryManager::~MemoryManager()
{
}

VM::Value *MemoryManager::allocStackFrame(std::size_t frameSize)
{
    std::size_t size = frameSize * sizeof(VM::Value);
    MMObject *m = alloc(align(size));
    stackFrames.append(m);
    return reinterpret_cast<VM::Value *>(&m->data);
}

void MemoryManager::deallocStackFrame(VM::Value *stackFrame)
{
    MMObject *o = toObject(stackFrame);
    for (int i = stackFrames.size() - 1; i >= 0; --i) {
        if (stackFrames[i] == o) {
            stackFrames.remove(i);
            dealloc(o);
            return;
        }
    }

    Q_UNREACHABLE();
}

void MemoryManager::collectRootsOnStack(QVector<VM::Object *> &roots) const
{
    for (int i = 0, ei = stackFrames.size(); i < ei; ++i) {
        MMObject *m = stackFrames[i];
        VM::Value *frame = reinterpret_cast<VM::Value *>(&m->data);
        std::size_t frameSize = (m->info.size - align(sizeof(MMInfo))) / sizeof(VM::Value);
        for (std::size_t j = 0; j < frameSize; ++j) {
            if (VM::Object *o = frame[j].asObject()) {
                roots.append(o);
            }
        }
    }
}
