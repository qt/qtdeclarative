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
#include <private/qv4function_p.h>
#include <private/qv4lookup_p.h>
#include <private/qv4regexpobject_p.h>

namespace QV4 {

namespace CompiledData {

CompilationUnit::~CompilationUnit()
{
    engine->compilationUnits.erase(engine->compilationUnits.find(this));
    free(data);
    free(runtimeStrings);
    delete [] runtimeLookups;
    delete [] runtimeRegularExpressions;
    free(runtimeClasses);
}

QV4::Function *CompilationUnit::linkToEngine(ExecutionEngine *engine)
{
    this->engine = engine;
    engine->compilationUnits.insert(this);

    assert(!runtimeStrings);
    assert(data);
    runtimeStrings = (QV4::String**)malloc(data->stringTableSize * sizeof(QV4::String*));
    for (int i = 0; i < data->stringTableSize; ++i)
        runtimeStrings[i] = engine->newIdentifier(data->stringAt(i)->qString());

    runtimeRegularExpressions = new QV4::Value[data->regexpTableSize];
    for (int i = 0; i < data->regexpTableSize; ++i) {
        const CompiledData::RegExp *re = data->regexpAt(i);
        int flags = 0;
        if (re->flags & CompiledData::RegExp::RegExp_Global)
            flags |= QQmlJS::V4IR::RegExp::RegExp_Global;
        if (re->flags & CompiledData::RegExp::RegExp_IgnoreCase)
            flags |= QQmlJS::V4IR::RegExp::RegExp_IgnoreCase;
        if (re->flags & CompiledData::RegExp::RegExp_Multiline)
            flags |= QQmlJS::V4IR::RegExp::RegExp_Multiline;
        QV4::RegExpObject *obj = engine->newRegExpObject(data->stringAt(re->stringIndex)->qString(), flags);
        runtimeRegularExpressions[i] = QV4::Value::fromObject(obj);
    }

    if (data->lookupTableSize) {
        runtimeLookups = new QV4::Lookup[data->lookupTableSize];
        const CompiledData::Lookup *compiledLookups = data->lookupTable();
        for (uint i = 0; i < data->lookupTableSize; ++i) {
            QV4::Lookup *l = runtimeLookups + i;

            if (compiledLookups[i].type_and_flags == CompiledData::Lookup::Type_Getter)
                l->getter = QV4::Lookup::getterGeneric;
            else if (compiledLookups[i].type_and_flags == CompiledData::Lookup::Type_Setter)
                l->setter = QV4::Lookup::setterGeneric;
            else if (compiledLookups[i].type_and_flags == CompiledData::Lookup::Type_GlobalGetter)
                l->globalGetter = QV4::Lookup::globalGetterGeneric;

            for (int i = 0; i < QV4::Lookup::Size; ++i)
                l->classList[i] = 0;
            l->level = -1;
            l->index = UINT_MAX;
            l->name = runtimeStrings[compiledLookups[i].nameIndex];
        }
    }

    if (data->jsClassTableSize) {
        runtimeClasses = (QV4::InternalClass**)malloc(data->jsClassTableSize * sizeof(QV4::InternalClass*));

        for (int i = 0; i < data->jsClassTableSize; ++i) {
            int memberCount = 0;
            const CompiledData::JSClassMember *member = data->jsClassAt(i, &memberCount);
            QV4::InternalClass *klass = engine->emptyClass;
            for (int j = 0; j < memberCount; ++j, ++member)
                klass = klass->addMember(runtimeStrings[member->nameOffset], member->isAccessor ? QV4::Attr_Accessor : QV4::Attr_Data);

            runtimeClasses[i] = klass;
        }
    }

    return linkBackendToEngine(engine);
}

void CompilationUnit::markObjects()
{
    for (int i = 0; i < data->stringTableSize; ++i)
        runtimeStrings[i]->mark();
    for (int i = 0; i < data->regexpTableSize; ++i)
        runtimeRegularExpressions[i].mark();
}

}

}
