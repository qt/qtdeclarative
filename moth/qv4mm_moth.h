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

#ifndef QV4GC_MOTH_H
#define QV4GC_MOTH_H

#include "qv4mm.h"

#include <QVector>

namespace QQmlJS {
namespace Moth {

class MemoryManager: public QQmlJS::VM::MemoryManager
{
public:
    MemoryManager();
    ~MemoryManager();

    VM::Value *allocStackFrame(std::size_t frameSize);
    void deallocStackFrame(VM::Value *stackFrame);

protected:
    virtual void collectRootsOnStack(QVector<VM::Object *> &roots) const;

private:
    QVector<MMObject *> stackFrames;
};

} // namespace Moth
} // namespace QQmlJS

#endif // QV4GC_MOTH_H
