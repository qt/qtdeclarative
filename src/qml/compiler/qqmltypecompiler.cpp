/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the tools applications of the Qt Toolkit.
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

#include "qqmltypecompiler_p.h"

#include <private/qqmlcompiler_p.h>
#include <private/qqmlobjectcreator_p.h>

QT_BEGIN_NAMESPACE

QQmlTypeCompiler::QQmlTypeCompiler(QQmlEnginePrivate *engine, QQmlCompiledData *compiledData, QQmlTypeData *typeData, QtQml::ParsedQML *parsedQML)
    : engine(engine)
    , compiledData(compiledData)
    , typeData(typeData)
    , parsedQML(parsedQML)
{
}

bool QQmlTypeCompiler::compile()
{
    compiledData->importCache = new QQmlTypeNameCache;

    foreach (const QString &ns, typeData->namespaces())
        compiledData->importCache->add(ns);

    // Add any Composite Singletons that were used to the import cache
    foreach (const QQmlTypeData::TypeReference &singleton, typeData->compositeSingletons())
        compiledData->importCache->add(singleton.type->qmlTypeName(), singleton.type->sourceUrl(), singleton.prefix);

    typeData->imports().populateCache(compiledData->importCache);
    compiledData->importCache->addref();

    const QHash<int, QQmlTypeData::TypeReference> &resolvedTypes = typeData->resolvedTypeRefs();
    for (QHash<int, QQmlTypeData::TypeReference>::ConstIterator resolvedType = resolvedTypes.constBegin(), end = resolvedTypes.constEnd();
         resolvedType != end; ++resolvedType) {
        QQmlCompiledData::TypeReference ref;
        if (resolvedType->typeData) {
            ref.component = resolvedType->typeData->compiledData();
            ref.component->addref();
        } else {
            ref.type = resolvedType->type;
            Q_ASSERT(ref.type);
        }
        ref.majorVersion = resolvedType->majorVersion;
        ref.minorVersion = resolvedType->minorVersion;
        compiledData->resolvedTypes.insert(resolvedType.key(), ref);
    }

    // Build property caches and VME meta object data

    const int objectCount = parsedQML->objects.count();
    compiledData->datas.reserve(objectCount);
    compiledData->propertyCaches.reserve(objectCount);

    QQmlPropertyCacheCreator propertyCacheBuilder(this);

    for (int i = 0; i < objectCount; ++i) {
        const QtQml::QmlObject *obj = parsedQML->objects.at(i);

        QByteArray vmeMetaObjectData;
        QQmlPropertyCache *propertyCache = 0;

        // If the object has no type, then it's probably a nested object definition as part
        // of a group property.
        const bool objectHasType = !propertyCacheBuilder.stringAt(obj->inheritedTypeNameIndex).isEmpty();
        if (objectHasType) {
            if (!propertyCacheBuilder.create(obj, &propertyCache, &vmeMetaObjectData)) {
                errors << propertyCacheBuilder.errors;
                return false;
            }
        }

        compiledData->datas << vmeMetaObjectData;
        if (propertyCache)
            propertyCache->addref();
        compiledData->propertyCaches << propertyCache;

        if (i == parsedQML->indexOfRootObject) {
            Q_ASSERT(propertyCache);
            compiledData->rootPropertyCache = propertyCache;
            propertyCache->addref();
        }
    }

    {
        SignalHandlerConverter converter(engine, parsedQML, compiledData);
        if (!converter.convertSignalHandlerExpressionsToFunctionDeclarations()) {
            errors << converter.errors;
            return false;
        }
    }

    // Collect imported scripts
    const QList<QQmlTypeData::ScriptReference> &scripts = typeData->resolvedScripts();
    compiledData->scripts.reserve(scripts.count());
    for (int scriptIndex = 0; scriptIndex < scripts.count(); ++scriptIndex) {
        const QQmlTypeData::ScriptReference &script = scripts.at(scriptIndex);

        QString qualifier = script.qualifier;
        QString enclosingNamespace;

        const int lastDotIndex = qualifier.lastIndexOf(QLatin1Char('.'));
        if (lastDotIndex != -1) {
            enclosingNamespace = qualifier.left(lastDotIndex);
            qualifier = qualifier.mid(lastDotIndex+1);
        }

        compiledData->importCache->add(qualifier, scriptIndex, enclosingNamespace);
        QQmlScriptData *scriptData = script.script->scriptData();
        scriptData->addref();
        compiledData->scripts << scriptData;
    }

    // Resolve component boundaries and aliases

    {
        // Scan for components, determine their scopes and resolve aliases within the scope.
        QQmlComponentAndAliasResolver resolver(this);
        if (!resolver.resolve()) {
            errors << resolver.errors;
            return false;
        }
    }

    // Compile JS binding expressions and signal handlers

    JSCodeGen jsCodeGen(typeData->finalUrlString(), parsedQML->code, &parsedQML->jsModule, &parsedQML->jsParserEngine, parsedQML->program, compiledData->importCache);
    const QVector<int> runtimeFunctionIndices = jsCodeGen.generateJSCodeForFunctionsAndBindings(parsedQML->functions);

    QV4::ExecutionEngine *v4 = engine->v4engine();

    QScopedPointer<QQmlJS::EvalInstructionSelection> isel(v4->iselFactory->create(engine, v4->executableAllocator, &parsedQML->jsModule, &parsedQML->jsGenerator));
    isel->setUseFastLookups(false);
    QV4::CompiledData::CompilationUnit *jsUnit = isel->compile(/*generated unit data*/false);

    // Generate QML compiled type data structures

    QmlUnitGenerator qmlGenerator;
    QV4::CompiledData::QmlUnit *qmlUnit = qmlGenerator.generate(*parsedQML, runtimeFunctionIndices);

    if (jsUnit) {
        Q_ASSERT(!jsUnit->data);
        jsUnit->ownsData = false;
        jsUnit->data = &qmlUnit->header;
    }

    compiledData->compilationUnit = jsUnit;
    if (compiledData->compilationUnit)
        compiledData->compilationUnit->ref();
    compiledData->qmlUnit = qmlUnit; // ownership transferred to m_compiledData

    // Add to type registry of composites
    if (compiledData->isCompositeType())
        engine->registerInternalCompositeType(compiledData);
    else {
        const QV4::CompiledData::Object *obj = qmlUnit->objectAt(qmlUnit->indexOfRootObject);
        QQmlCompiledData::TypeReference typeRef = compiledData->resolvedTypes.value(obj->inheritedTypeNameIndex);
        if (typeRef.component) {
            compiledData->metaTypeId = typeRef.component->metaTypeId;
            compiledData->listMetaTypeId = typeRef.component->listMetaTypeId;
        } else {
            compiledData->metaTypeId = typeRef.type->typeId();
            compiledData->listMetaTypeId = typeRef.type->qListTypeId();
        }
    }

    // Sanity check property bindings
    QQmlPropertyValidator validator(this);
    if (!validator.validate()) {
        errors << validator.errors;
        return false;
    }

    return errors.isEmpty();
}

void QQmlTypeCompiler::recordError(const QQmlError &error)
{
    QQmlError e = error;
    e.setUrl(compiledData->url);
    errors << e;
}

QString QQmlTypeCompiler::stringAt(int idx) const
{
    return parsedQML->stringAt(idx);
}

int QQmlTypeCompiler::registerString(const QString &str)
{
    return parsedQML->jsGenerator.registerString(str);
}

const QV4::CompiledData::QmlUnit *QQmlTypeCompiler::qmlUnit() const
{
    return compiledData->qmlUnit;
}

const QQmlImports *QQmlTypeCompiler::imports() const
{
    return &typeData->imports();
}

QHash<int, QQmlCompiledData::TypeReference> *QQmlTypeCompiler::resolvedTypes()
{
    return &compiledData->resolvedTypes;
}

QList<QmlObject *> *QQmlTypeCompiler::qmlObjects()
{
    return &parsedQML->objects;
}

int QQmlTypeCompiler::rootObjectIndex() const
{
    return parsedQML->indexOfRootObject;
}

const QList<QQmlPropertyCache *> &QQmlTypeCompiler::propertyCaches() const
{
    return compiledData->propertyCaches;
}

QList<QByteArray> *QQmlTypeCompiler::vmeMetaObjects() const
{
    return &compiledData->datas;
}

QHash<int, int> *QQmlTypeCompiler::objectIndexToIdForRoot()
{
    return &compiledData->objectIndexToIdForRoot;
}

QHash<int, QHash<int, int> > *QQmlTypeCompiler::objectIndexToIdPerComponent()
{
    return &compiledData->objectIndexToIdPerComponent;
}

QHash<int, QByteArray> *QQmlTypeCompiler::customParserData()
{
    return &compiledData->customParserData;
}

MemoryPool *QQmlTypeCompiler::memoryPool()
{
    return parsedQML->jsParserEngine.pool();
}

QT_END_NAMESPACE
