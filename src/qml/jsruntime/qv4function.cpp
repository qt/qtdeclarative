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

#include "qv4function_p.h"
#include "qv4managed_p.h"
#include "qv4string_p.h"
#include "qv4value_inl_p.h"
#include "qv4engine_p.h"
#include "qv4lookup_p.h"
#include "qv4mm_p.h"

QT_BEGIN_NAMESPACE

using namespace QV4;

Function::Function(ExecutionEngine *engine, CompiledData::CompilationUnit *unit, const CompiledData::Function *function,
                   ReturnedValue (*codePtr)(ExecutionEngine *, const uchar *))
        : compiledFunction(function)
        , compilationUnit(unit)
        , code(codePtr)
        , codeData(0)
{
    Q_UNUSED(engine);

    internalClass = engine->emptyClass;
    const quint32 *formalsIndices = compiledFunction->formalsTable();
    // iterate backwards, so we get the right ordering for duplicate names
    Scope scope(engine);
    ScopedString arg(scope);
    for (int i = static_cast<int>(compiledFunction->nFormals - 1); i >= 0; --i) {
        arg = compilationUnit->runtimeStrings[formalsIndices[i]];
        while (1) {
            InternalClass *newClass = internalClass->addMember(arg, Attr_NotConfigurable);
            if (newClass != internalClass) {
                internalClass = newClass;
                break;
            }
            // duplicate arguments, need some trick to store them
            MemoryManager *mm = engine->memoryManager;
            arg = mm->alloc<String>(mm, arg->d(), engine->newString(QString(0xfffe)));
        }
    }

    const quint32 *localsIndices = compiledFunction->localsTable();
    for (quint32 i = 0; i < compiledFunction->nLocals; ++i)
        internalClass = internalClass->addMember(compilationUnit->runtimeStrings[localsIndices[i]]->identifier, Attr_NotConfigurable);
}

Function::~Function()
{
}

QT_END_NAMESPACE
