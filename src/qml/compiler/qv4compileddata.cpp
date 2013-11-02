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
#include <private/qv4objectproto_p.h>
#include <private/qv4lookup_p.h>
#include <private/qv4regexpobject_p.h>

#include <algorithm>

QT_BEGIN_NAMESPACE

namespace QV4 {

namespace CompiledData {

CompilationUnit::~CompilationUnit()
{
    unlink();
}

QV4::Function *CompilationUnit::linkToEngine(ExecutionEngine *engine)
{
    this->engine = engine;
    engine->compilationUnits.insert(this);

    assert(!runtimeStrings);
    assert(data);
    runtimeStrings = (QV4::SafeString *)malloc(data->stringTableSize * sizeof(QV4::SafeString));
    // memset the strings to 0 in case a GC run happens while we're within the loop below
    memset(runtimeStrings, 0, data->stringTableSize * sizeof(QV4::SafeString));
    for (uint i = 0; i < data->stringTableSize; ++i)
        runtimeStrings[i] = engine->newIdentifier(data->stringAt(i));

    runtimeRegularExpressions = new QV4::SafeValue[data->regexpTableSize];
    // memset the regexps to 0 in case a GC run happens while we're within the loop below
    memset(runtimeRegularExpressions, 0, data->regexpTableSize * sizeof(QV4::SafeValue));
    for (uint i = 0; i < data->regexpTableSize; ++i) {
        const CompiledData::RegExp *re = data->regexpAt(i);
        int flags = 0;
        if (re->flags & CompiledData::RegExp::RegExp_Global)
            flags |= QQmlJS::V4IR::RegExp::RegExp_Global;
        if (re->flags & CompiledData::RegExp::RegExp_IgnoreCase)
            flags |= QQmlJS::V4IR::RegExp::RegExp_IgnoreCase;
        if (re->flags & CompiledData::RegExp::RegExp_Multiline)
            flags |= QQmlJS::V4IR::RegExp::RegExp_Multiline;
        runtimeRegularExpressions[i] = engine->newRegExpObject(data->stringAt(re->stringIndex), flags);
    }

    if (data->lookupTableSize) {
        runtimeLookups = new QV4::Lookup[data->lookupTableSize];
        memset(runtimeLookups, 0, data->lookupTableSize * sizeof(QV4::Lookup));
        const CompiledData::Lookup *compiledLookups = data->lookupTable();
        for (uint i = 0; i < data->lookupTableSize; ++i) {
            QV4::Lookup *l = runtimeLookups + i;

            if (compiledLookups[i].type_and_flags == CompiledData::Lookup::Type_Getter)
                l->getter = QV4::Lookup::getterGeneric;
            else if (compiledLookups[i].type_and_flags == CompiledData::Lookup::Type_Setter)
                l->setter = QV4::Lookup::setterGeneric;
            else if (compiledLookups[i].type_and_flags == CompiledData::Lookup::Type_GlobalGetter)
                l->globalGetter = QV4::Lookup::globalGetterGeneric;

            for (int j = 0; j < QV4::Lookup::Size; ++j)
                l->classList[j] = 0;
            l->level = -1;
            l->index = UINT_MAX;
            l->name = runtimeStrings[compiledLookups[i].nameIndex].asString();
        }
    }

    if (data->jsClassTableSize) {
        runtimeClasses = (QV4::InternalClass**)malloc(data->jsClassTableSize * sizeof(QV4::InternalClass*));

        for (uint i = 0; i < data->jsClassTableSize; ++i) {
            int memberCount = 0;
            const CompiledData::JSClassMember *member = data->jsClassAt(i, &memberCount);
            QV4::InternalClass *klass = engine->objectClass;
            for (int j = 0; j < memberCount; ++j, ++member)
                klass = klass->addMember(runtimeStrings[member->nameOffset].asString(), member->isAccessor ? QV4::Attr_Accessor : QV4::Attr_Data);

            runtimeClasses[i] = klass;
        }
    }

    linkBackendToEngine(engine);

#if 0
    runtimeFunctionsSortedByAddress.resize(runtimeFunctions.size());
    memcpy(runtimeFunctionsSortedByAddress.data(), runtimeFunctions.data(), runtimeFunctions.size() * sizeof(QV4::Function*));
    std::sort(runtimeFunctionsSortedByAddress.begin(), runtimeFunctionsSortedByAddress.end(), functionSortHelper);
#endif

    if (data->indexOfRootFunction != -1)
        return runtimeFunctions[data->indexOfRootFunction];
    else
        return 0;
}

void CompilationUnit::unlink()
{
    if (engine)
        engine->compilationUnits.erase(engine->compilationUnits.find(this));
    engine = 0;
    if (ownsData)
        free(data);
    data = 0;
    free(runtimeStrings);
    runtimeStrings = 0;
    delete [] runtimeLookups;
    runtimeLookups = 0;
    delete [] runtimeRegularExpressions;
    runtimeRegularExpressions = 0;
    free(runtimeClasses);
    runtimeClasses = 0;
    qDeleteAll(runtimeFunctions);
    runtimeFunctions.clear();
}

void CompilationUnit::markObjects(QV4::ExecutionEngine *e)
{
    for (uint i = 0; i < data->stringTableSize; ++i)
        runtimeStrings[i].mark(e);
    if (runtimeRegularExpressions) {
        for (uint i = 0; i < data->regexpTableSize; ++i)
            runtimeRegularExpressions[i].mark(e);
    }
    for (int i = 0; i < runtimeFunctions.count(); ++i)
        if (runtimeFunctions[i])
            runtimeFunctions[i]->mark(e);
    if (runtimeLookups) {
        for (uint i = 0; i < data->lookupTableSize; ++i)
            runtimeLookups[i].name->mark(e);
    }
}

QString Binding::valueAsString(const Unit *unit) const
{
    switch (type) {
    case Type_Script:
    case Type_String:
        return unit->stringAt(stringIndex);
    case Type_Boolean:
        return value.b ? QStringLiteral("true") : QStringLiteral("false");
    case Type_Number:
        return QString::number(value.d);
    case Type_Invalid:
        return QString();
    default:
        break;
    }
    return QString();
}

}

}

QT_END_NAMESPACE
