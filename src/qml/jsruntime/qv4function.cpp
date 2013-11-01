/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtQml module of the Qt Toolkit.
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

#include "qv4function_p.h"
#include "qv4managed_p.h"
#include "qv4string_p.h"
#include "qv4value_p.h"
#include "qv4engine_p.h"
#include "qv4lookup_p.h"

QT_BEGIN_NAMESPACE

using namespace QV4;

Function::Function(ExecutionEngine *engine, CompiledData::CompilationUnit *unit, const CompiledData::Function *function,
                   ReturnedValue (*codePtr)(ExecutionContext *, const uchar *), quint32 _codeSize)
        : compiledFunction(function)
        , compilationUnit(unit)
        , codePtr(codePtr)
        , codeData(0)
        , codeSize(_codeSize)
{
    Q_UNUSED(engine);

    name = compilationUnit->runtimeStrings[compiledFunction->nameIndex].asString();

    formals.resize(compiledFunction->nFormals);
    formals.fill(0);
    const quint32 *formalsIndices = compiledFunction->formalsTable();
    for (quint32 i = 0; i < compiledFunction->nFormals; ++i)
        formals[i] = compilationUnit->runtimeStrings[formalsIndices[i]].asString();


    locals.resize(compiledFunction->nLocals);
    locals.fill(0);
    const quint32 *localsIndices = compiledFunction->localsTable();
    for (quint32 i = 0; i < compiledFunction->nLocals; ++i)
        locals[i] = compilationUnit->runtimeStrings[localsIndices[i]].asString();
}

Function::~Function()
{
}


void Function::mark()
{
    name.mark();
    for (int i = 0; i < formals.size(); ++i)
        formals.at(i)->mark();
    for (int i = 0; i < locals.size(); ++i)
        locals.at(i)->mark();
}

namespace QV4 {
struct LineNumberMappingHelper
{
    const quint32 *table;
    int lowerBound(int begin, int end, quint32 offset) {
        int middle;
        int n = int(end - begin);
        int half;

        while (n > 0) {
            half = n >> 1;
            middle = begin + half;
            if (table[middle * 2] < offset) {
                begin = middle + 1;
                n -= half + 1;
            } else {
                n = half;
            }
        }
        return begin;
    }
};

}

int Function::lineNumberForProgramCounter(qptrdiff offset) const
{
    LineNumberMappingHelper helper;
    helper.table = compiledFunction->lineNumberMapping();
    const uint count = compiledFunction->nLineNumberMappingEntries;

    int pos = helper.lowerBound(0, count, offset);
    if (pos != 0 && count > 0)
        --pos;
    if (static_cast<uint>(pos) == count)
        return -1;
    return helper.table[pos * 2 + 1];
}

QT_END_NAMESPACE
