/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the tools applications of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qqmlirloader_p.h"
#include <private/qqmlirbuilder_p.h>

QT_BEGIN_NAMESPACE

QQmlIRLoader::QQmlIRLoader(const QV4::CompiledData::Unit *qmlData, QmlIR::Document *output)
    : unit(qmlData)
      , output(output)
{
    pool = output->jsParserEngine.pool();
}

void QQmlIRLoader::load()
{
    output->jsGenerator.stringTable.initializeFromBackingUnit(unit);

    const QV4::CompiledData::QmlUnit *qmlUnit = unit->qmlUnit();

    for (quint32 i = 0; i < qmlUnit->nImports; ++i)
        output->imports << qmlUnit->importAt(i);

    if (unit->flags & QV4::CompiledData::Unit::IsSingleton) {
        QmlIR::Pragma *p = New<QmlIR::Pragma>();
        p->location = QV4::CompiledData::Location();
        p->type = QmlIR::Pragma::PragmaSingleton;
        output->pragmas << p;
    }

    for (uint i = 0; i < qmlUnit->nObjects; ++i) {
        const QV4::CompiledData::Object *serializedObject = qmlUnit->objectAt(i);
        QmlIR::Object *object = loadObject(serializedObject);
        output->objects.append(object);
    }
}

struct FakeExpression : public QQmlJS::AST::NullExpression
{
    FakeExpression(int start, int length)
        : location(start, length)
    {}

    virtual QQmlJS::SourceLocation firstSourceLocation() const
    { return location; }

    virtual QQmlJS::SourceLocation lastSourceLocation() const
    { return location; }

private:
    QQmlJS::SourceLocation location;
};

QmlIR::Object *QQmlIRLoader::loadObject(const QV4::CompiledData::Object *serializedObject)
{
    QmlIR::Object *object = pool->New<QmlIR::Object>();
    object->init(pool, serializedObject->inheritedTypeNameIndex, serializedObject->idNameIndex);

    object->indexOfDefaultPropertyOrAlias = serializedObject->indexOfDefaultPropertyOrAlias;
    object->defaultPropertyIsAlias = serializedObject->defaultPropertyIsAlias;
    object->isInlineComponent = serializedObject->flags & QV4::CompiledData::Object::IsInlineComponentRoot;
    object->flags = serializedObject->flags;
    object->id = serializedObject->id;
    object->location = serializedObject->location;
    object->locationOfIdProperty = serializedObject->locationOfIdProperty;

    QVector<int> functionIndices;
    functionIndices.reserve(serializedObject->nFunctions + serializedObject->nBindings / 2);

    for (uint i = 0; i < serializedObject->nBindings; ++i) {
        QmlIR::Binding *b = pool->New<QmlIR::Binding>();
        *static_cast<QV4::CompiledData::Binding*>(b) = serializedObject->bindingTable()[i];
        object->bindings->append(b);
        if (b->type == QV4::CompiledData::Binding::Type_Script) {
            functionIndices.append(b->value.compiledScriptIndex);
            b->value.compiledScriptIndex = functionIndices.count() - 1;

            QmlIR::CompiledFunctionOrExpression *foe = pool->New<QmlIR::CompiledFunctionOrExpression>();
            foe->nameIndex = 0;

            QQmlJS::AST::ExpressionNode *expr;

            if (b->stringIndex != quint32(0)) {
                const int start = output->code.length();
                const QString script = output->stringAt(b->stringIndex);
                const int length = script.length();
                output->code.append(script);
                expr = new (pool) FakeExpression(start, length);
            } else
                expr = new (pool) QQmlJS::AST::NullExpression();
            foe->node = new (pool) QQmlJS::AST::ExpressionStatement(expr); // dummy
            object->functionsAndExpressions->append(foe);
        }
    }

    Q_ASSERT(object->functionsAndExpressions->count == functionIndices.count());

    for (uint i = 0; i < serializedObject->nSignals; ++i) {
        const QV4::CompiledData::Signal *serializedSignal = serializedObject->signalAt(i);
        QmlIR::Signal *s = pool->New<QmlIR::Signal>();
        s->nameIndex = serializedSignal->nameIndex;
        s->location = serializedSignal->location;
        s->parameters = pool->New<QmlIR::PoolList<QmlIR::Parameter> >();

        for (uint i = 0; i < serializedSignal->nParameters; ++i) {
            QmlIR::Parameter *p = pool->New<QmlIR::Parameter>();
            *static_cast<QV4::CompiledData::Parameter*>(p) = *serializedSignal->parameterAt(i);
            s->parameters->append(p);
        }

        object->qmlSignals->append(s);
    }

    for (uint i = 0; i < serializedObject->nEnums; ++i) {
        const QV4::CompiledData::Enum *serializedEnum = serializedObject->enumAt(i);
        QmlIR::Enum *e = pool->New<QmlIR::Enum>();
        e->nameIndex = serializedEnum->nameIndex;
        e->location = serializedEnum->location;
        e->enumValues = pool->New<QmlIR::PoolList<QmlIR::EnumValue> >();

        for (uint i = 0; i < serializedEnum->nEnumValues; ++i) {
            QmlIR::EnumValue *v = pool->New<QmlIR::EnumValue>();
            *static_cast<QV4::CompiledData::EnumValue*>(v) = *serializedEnum->enumValueAt(i);
            e->enumValues->append(v);
        }

        object->qmlEnums->append(e);
    }

    const QV4::CompiledData::Property *serializedProperty = serializedObject->propertyTable();
    for (uint i = 0; i < serializedObject->nProperties; ++i, ++serializedProperty) {
        QmlIR::Property *p = pool->New<QmlIR::Property>();
        *static_cast<QV4::CompiledData::Property*>(p) = *serializedProperty;
        object->properties->append(p);
    }

    {
        const QV4::CompiledData::Alias *serializedAlias = serializedObject->aliasTable();
        for (uint i = 0; i < serializedObject->nAliases; ++i, ++serializedAlias) {
            QmlIR::Alias *a = pool->New<QmlIR::Alias>();
            *static_cast<QV4::CompiledData::Alias*>(a) = *serializedAlias;
            object->aliases->append(a);
        }
    }

    const quint32_le *functionIdx = serializedObject->functionOffsetTable();
    for (uint i = 0; i < serializedObject->nFunctions; ++i, ++functionIdx) {
        QmlIR::Function *f = pool->New<QmlIR::Function>();
        const QV4::CompiledData::Function *compiledFunction = unit->functionAt(*functionIdx);

        functionIndices.append(*functionIdx);
        f->index = functionIndices.count() - 1;
        f->location = compiledFunction->location;
        f->nameIndex = compiledFunction->nameIndex;
        f->returnType = compiledFunction->returnType;

        f->formals.allocate(pool, int(compiledFunction->nFormals));
        const QV4::CompiledData::Parameter *formalNameIdx = compiledFunction->formalsTable();
        for (uint i = 0; i < compiledFunction->nFormals; ++i, ++formalNameIdx)
            *static_cast<QV4::CompiledData::Parameter*>(&f->formals[i]) = *formalNameIdx;

        object->functions->append(f);
    }

    object->runtimeFunctionIndices.allocate(pool, functionIndices);

    const QV4::CompiledData::InlineComponent *serializedInlineComponent = serializedObject->inlineComponentTable();
    for (uint i = 0; i < serializedObject->nInlineComponents; ++i, ++serializedInlineComponent) {
        QmlIR::InlineComponent *ic = pool->New<QmlIR::InlineComponent>();
        *static_cast<QV4::CompiledData::InlineComponent*>(ic) = *serializedInlineComponent;
        object->inlineComponents->append(ic);
    }

    return object;
}

QT_END_NAMESPACE
