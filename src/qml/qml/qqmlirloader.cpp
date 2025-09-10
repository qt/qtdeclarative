// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

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

    using QmlIR::Pragma;
    const auto createPragma = [&](Pragma::PragmaType type) {
        Pragma *p = New<Pragma>();
        p->location = QV4::CompiledData::Location();
        p->type = type;
        output->pragmas << p;
        return p;
    };

    const auto createListPragma = [&](
            Pragma::PragmaType type,
            Pragma::ListPropertyAssignBehaviorValue value) {
        createPragma(type)->listPropertyAssignBehavior = value;
    };

    const auto createComponentPragma = [&](
            Pragma::PragmaType type,
            Pragma::ComponentBehaviorValue value) {
        createPragma(type)->componentBehavior = value;
    };

    const auto createFunctionSignaturePragma = [&](
            Pragma::PragmaType type,
            Pragma::FunctionSignatureBehaviorValue value) {
        createPragma(type)->functionSignatureBehavior = value;
    };

    const auto createNativeMethodPragma = [&](
            Pragma::PragmaType type,
            Pragma::NativeMethodBehaviorValue value) {
        createPragma(type)->nativeMethodBehavior = value;
    };

    const auto createValueTypePragma = [&](
            Pragma::PragmaType type,
            Pragma::ValueTypeBehaviorValues value) {
        createPragma(type)->valueTypeBehavior = value;
    };

    if (unit->flags & QV4::CompiledData::Unit::IsSingleton)
        createPragma(Pragma::Singleton);
    if (unit->flags & QV4::CompiledData::Unit::IsStrict)
        createPragma(Pragma::Strict);

    if (unit->flags & QV4::CompiledData::Unit::ListPropertyAssignReplace)
        createListPragma(Pragma::ListPropertyAssignBehavior, Pragma::Replace);
    else if (unit->flags & QV4::CompiledData::Unit::ListPropertyAssignReplaceIfNotDefault)
        createListPragma(Pragma::ListPropertyAssignBehavior, Pragma::ReplaceIfNotDefault);

    if (unit->flags & QV4::CompiledData::Unit::ComponentsBound)
        createComponentPragma(Pragma::ComponentBehavior, Pragma::Bound);

    if (unit->flags & QV4::CompiledData::Unit::FunctionSignaturesIgnored)
        createFunctionSignaturePragma(Pragma::FunctionSignatureBehavior, Pragma::Ignored);

    if (unit->flags & QV4::CompiledData::Unit::NativeMethodsAcceptThisObject)
        createNativeMethodPragma(Pragma::NativeMethodBehavior, Pragma::AcceptThisObject);

    Pragma::ValueTypeBehaviorValues valueTypeBehavior = {};
    if (unit->flags & QV4::CompiledData::Unit::ValueTypesCopied)
        valueTypeBehavior |= Pragma::Copy;
    if (unit->flags & QV4::CompiledData::Unit::ValueTypesAddressable)
        valueTypeBehavior |= Pragma::Addressable;
    if (unit->flags & QV4::CompiledData::Unit::ValueTypesAssertable)
        valueTypeBehavior |= Pragma::Assertable;
    if (valueTypeBehavior)
        createValueTypePragma(Pragma::ValueTypeBehavior, valueTypeBehavior);

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

    QQmlJS::SourceLocation firstSourceLocation() const override
    { return location; }

    QQmlJS::SourceLocation lastSourceLocation() const override
    { return location; }

private:
    QQmlJS::SourceLocation location;
};

QmlIR::Object *QQmlIRLoader::loadObject(const QV4::CompiledData::Object *serializedObject)
{
    QmlIR::Object *object = pool->New<QmlIR::Object>();
    object->init(pool, serializedObject->inheritedTypeNameIndex, serializedObject->idNameIndex,
                 serializedObject->location);

    object->indexOfDefaultPropertyOrAlias = serializedObject->indexOfDefaultPropertyOrAlias;
    object->defaultPropertyIsAlias = serializedObject->hasAliasAsDefaultProperty();
    object->flags = serializedObject->flags();
    object->id = serializedObject->objectId();
    object->locationOfIdProperty = serializedObject->locationOfIdProperty;

    QVector<int> functionIndices;
    functionIndices.reserve(serializedObject->nFunctions + serializedObject->nBindings / 2);

    for (uint i = 0; i < serializedObject->nBindings; ++i) {
        QmlIR::Binding *b = pool->New<QmlIR::Binding>();
        *static_cast<QV4::CompiledData::Binding*>(b) = serializedObject->bindingTable()[i];
        object->bindings->append(b);
        if (b->type() == QV4::CompiledData::Binding::Type_Script) {
            functionIndices.append(b->value.compiledScriptIndex);
            b->value.compiledScriptIndex = functionIndices.size() - 1;

            QmlIR::CompiledFunctionOrExpression *foe = pool->New<QmlIR::CompiledFunctionOrExpression>();
            foe->nameIndex = 0;

            QQmlJS::AST::ExpressionNode *expr;

            if (b->stringIndex != quint32(0)) {
                const int start = output->code.size();
                const QString script = output->stringAt(b->stringIndex);
                const int length = script.size();
                output->code.append(script);
                expr = new (pool) FakeExpression(start, length);
            } else
                expr = new (pool) QQmlJS::AST::NullExpression();
            foe->node = new (pool) QQmlJS::AST::ExpressionStatement(expr); // dummy
            object->functionsAndExpressions->append(foe);
        }
    }

    Q_ASSERT(object->functionsAndExpressions->count == functionIndices.size());

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
        f->index = functionIndices.size() - 1;
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

    const QV4::CompiledData::RequiredPropertyExtraData *serializedRequiredPropertyExtraData = serializedObject->requiredPropertyExtraDataTable();
    for (uint i = 0u; i < serializedObject->nRequiredPropertyExtraData; ++i, ++serializedRequiredPropertyExtraData) {
        QmlIR::RequiredPropertyExtraData *extra = pool->New<QmlIR::RequiredPropertyExtraData>();
        *static_cast<QV4::CompiledData::RequiredPropertyExtraData *>(extra) = *serializedRequiredPropertyExtraData;
        object->requiredPropertyExtraDatas->append(extra);
    }

    return object;
}

QT_END_NAMESPACE
