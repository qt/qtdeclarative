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

#include "qv4compileddata_p.h"
#include "qv4jsir_p.h"
#include <private/qv4engine_p.h>

namespace QV4 {

namespace CompiledData {

int Function::calculateSize(QQmlJS::V4IR::Function *f)
{
    return calculateSize(f->formals.size(), f->locals.size(), f->nestedFunctions.size());
}

CompilationUnit::~CompilationUnit()
{
    free(data);
    free(runtimeIdentifiers);
}

QV4::Function *CompilationUnit::linkToEngine(ExecutionEngine *engine)
{
    assert(!runtimeIdentifiers);
    assert(data);
    runtimeIdentifiers = (QV4::String**)malloc(data->stringTableSize * sizeof(QV4::String*));
    for (int i = 0; i < data->stringTableSize; ++i)
        runtimeIdentifiers[i] = engine->newIdentifier(data->stringAt(i)->qString());

    return linkBackendToEngine(engine);
}

// ### Move to masm
QV4::Function *MasmCompilationUnit::linkBackendToEngine(ExecutionEngine *engine)
{
    return rootFunction;
}

}

}
