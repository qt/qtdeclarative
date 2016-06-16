/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "qqmltypecompiler_p.h"

#include <private/qqmlirbuilder_p.h>
#include <private/qqmlobjectcreator_p.h>
#include <private/qqmlcustomparser_p.h>
#include <private/qqmlvmemetaobject_p.h>
#include <private/qqmlcomponent_p.h>
#include <private/qv4ssa_p.h>

#include "qqmlpropertycachecreator_p.h"
#include "qqmlpropertyvalidator_p.h"

#define COMPILE_EXCEPTION(token, desc) \
    { \
        recordError((token)->location, desc); \
        return false; \
    }

QT_BEGIN_NAMESPACE

QQmlTypeCompiler::QQmlTypeCompiler(QQmlEnginePrivate *engine, QQmlTypeData *typeData, QmlIR::Document *parsedQML, const QQmlRefPointer<QQmlTypeNameCache> &importCache, const QV4::CompiledData::CompilationUnit::ResolvedTypeReferenceMap &resolvedTypeCache)
    : resolvedTypes(resolvedTypeCache)
    , engine(engine)
    , typeData(typeData)
    , importCache(importCache)
    , document(parsedQML)
{
}

QV4::CompiledData::CompilationUnit *QQmlTypeCompiler::compile()
{
    // Build property caches and VME meta object data

    for (auto it = resolvedTypes.constBegin(), end = resolvedTypes.constEnd();
         it != end; ++it) {
        QQmlCustomParser *customParser = (*it)->type ? (*it)->type->customParser() : 0;
        if (customParser)
            customParsers.insert(it.key(), customParser);
    }

    {
        QQmlPropertyCacheCreator<QQmlTypeCompiler> propertyCacheBuilder(&m_propertyCaches, engine, this, imports());
        QQmlCompileError error = propertyCacheBuilder.buildMetaObjects();
        if (error.isSet()) {
            recordError(error);
            return nullptr;
        }
    }

    {
        QQmlDefaultPropertyMerger merger(this);
        merger.mergeDefaultProperties();
    }

    {
        SignalHandlerConverter converter(this);
        if (!converter.convertSignalHandlerExpressionsToFunctionDeclarations())
            return nullptr;
    }

    {
        QQmlEnumTypeResolver enumResolver(this);
        if (!enumResolver.resolveEnumBindings())
            return nullptr;
    }

    {
        QQmlCustomParserScriptIndexer cpi(this);
        cpi.annotateBindingsWithScriptStrings();
    }

    {
        QQmlAliasAnnotator annotator(this);
        annotator.annotateBindingsToAliases();
    }

    // Collect imported scripts
    const QList<QQmlTypeData::ScriptReference> &scripts = typeData->resolvedScripts();
    QVector<QQmlScriptData *> dependentScripts;
    dependentScripts.reserve(scripts.count());
    for (int scriptIndex = 0; scriptIndex < scripts.count(); ++scriptIndex) {
        const QQmlTypeData::ScriptReference &script = scripts.at(scriptIndex);

        QStringRef qualifier(&script.qualifier);
        QString enclosingNamespace;

        const int lastDotIndex = qualifier.lastIndexOf(QLatin1Char('.'));
        if (lastDotIndex != -1) {
            enclosingNamespace = qualifier.left(lastDotIndex).toString();
            qualifier = qualifier.mid(lastDotIndex+1);
        }

        importCache->add(qualifier.toString(), scriptIndex, enclosingNamespace);
        QQmlScriptData *scriptData = script.script->scriptData();
        scriptData->addref();
        dependentScripts << scriptData;
    }

    // Resolve component boundaries and aliases

    {
        // Scan for components, determine their scopes and resolve aliases within the scope.
        QQmlComponentAndAliasResolver resolver(this);
        if (!resolver.resolve())
            return nullptr;
    }

    {
        QQmlDeferredAndCustomParserBindingScanner deferredAndCustomParserBindingScanner(this);
        if (!deferredAndCustomParserBindingScanner.scanObject())
            return nullptr;
    }

    // Compile JS binding expressions and signal handlers
    if (!document->javaScriptCompilationUnit) {
        {
            // We can compile script strings ahead of time, but they must be compiled
            // without type optimizations as their scope is always entirely dynamic.
            QQmlScriptStringScanner sss(this);
            sss.scan();
        }

        QmlIR::JSCodeGen v4CodeGenerator(typeData->finalUrlString(), document->code, &document->jsModule, &document->jsParserEngine, document->program, importCache, &document->jsGenerator.stringTable);
        QQmlJSCodeGenerator jsCodeGen(this, &v4CodeGenerator);
        if (!jsCodeGen.generateCodeForComponents())
            return nullptr;

        QQmlJavaScriptBindingExpressionSimplificationPass pass(this);
        pass.reduceTranslationBindings();

        QV4::ExecutionEngine *v4 = engine->v4engine();
        QScopedPointer<QV4::EvalInstructionSelection> isel(v4->iselFactory->create(engine, v4->executableAllocator, &document->jsModule, &document->jsGenerator));
        isel->setUseFastLookups(false);
        isel->setUseTypeInference(true);
        document->javaScriptCompilationUnit = isel->compile(/*generated unit data*/false);
    }

    // Generate QML compiled type data structures

    QmlIR::QmlUnitGenerator qmlGenerator;
    QV4::CompiledData::Unit *qmlUnit = qmlGenerator.generate(*document);

    Q_ASSERT(document->javaScriptCompilationUnit);
    // The js unit owns the data and will free the qml unit.
    document->javaScriptCompilationUnit->data = qmlUnit;

    QV4::CompiledData::CompilationUnit *compilationUnit = document->javaScriptCompilationUnit;
    compilationUnit = document->javaScriptCompilationUnit;
    compilationUnit->importCache = importCache;
    compilationUnit->dependentScripts = dependentScripts;
    compilationUnit->resolvedTypes = resolvedTypes;
    compilationUnit->propertyCaches = std::move(m_propertyCaches);
    Q_ASSERT(compilationUnit->propertyCaches.count() == static_cast<int>(compilationUnit->data->nObjects));

    // Add to type registry of composites
    if (compilationUnit->propertyCaches.needsVMEMetaObject(qmlUnit->indexOfRootObject))
        engine->registerInternalCompositeType(compilationUnit);
    else {
        const QV4::CompiledData::Object *obj = qmlUnit->objectAt(qmlUnit->indexOfRootObject);
        auto *typeRef = resolvedTypes.value(obj->inheritedTypeNameIndex);
        Q_ASSERT(typeRef);
        if (typeRef->compilationUnit) {
            compilationUnit->metaTypeId = typeRef->compilationUnit->metaTypeId;
            compilationUnit->listMetaTypeId = typeRef->compilationUnit->listMetaTypeId;
        } else {
            compilationUnit->metaTypeId = typeRef->type->typeId();
            compilationUnit->listMetaTypeId = typeRef->type->qListTypeId();
        }
    }

    {
    // Sanity check property bindings
        QQmlPropertyValidator validator(engine, *imports(), compilationUnit);
        QVector<QQmlCompileError> errors = validator.validate();
        if (!errors.isEmpty()) {
            for (const QQmlCompileError &error: qAsConst(errors))
                recordError(error);
            return nullptr;
        }
    }

    compilationUnit->updateBindingAndObjectCounters();

    if (errors.isEmpty())
        return compilationUnit;
    else
        return nullptr;
}

void QQmlTypeCompiler::recordError(QQmlError error)
{
    error.setUrl(url());
    errors << error;
}

void QQmlTypeCompiler::recordError(const QV4::CompiledData::Location &location, const QString &description)
{
    QQmlError error;
    error.setLine(location.line);
    error.setColumn(location.column);
    error.setDescription(description);
    error.setUrl(url());
    errors << error;
}

void QQmlTypeCompiler::recordError(const QQmlCompileError &error)
{
    recordError(error.location, error.description);
}

QString QQmlTypeCompiler::stringAt(int idx) const
{
    return document->stringAt(idx);
}

int QQmlTypeCompiler::registerString(const QString &str)
{
    return document->jsGenerator.registerString(str);
}

QV4::IR::Module *QQmlTypeCompiler::jsIRModule() const
{
    return &document->jsModule;
}

const QV4::CompiledData::Unit *QQmlTypeCompiler::qmlUnit() const
{
    return document->javaScriptCompilationUnit->data;
}

const QQmlImports *QQmlTypeCompiler::imports() const
{
    return &typeData->imports();
}

QVector<QmlIR::Object *> *QQmlTypeCompiler::qmlObjects() const
{
    return &document->objects;
}

int QQmlTypeCompiler::rootObjectIndex() const
{
    return document->indexOfRootObject;
}

void QQmlTypeCompiler::setPropertyCaches(QQmlPropertyCacheVector &&caches)
{
    m_propertyCaches = std::move(caches);
    Q_ASSERT(m_propertyCaches.count() >= document->indexOfRootObject);
}

const QQmlPropertyCacheVector *QQmlTypeCompiler::propertyCaches() const
{
    return &m_propertyCaches;
}

QQmlPropertyCacheVector &&QQmlTypeCompiler::takePropertyCaches()
{
    return std::move(m_propertyCaches);
}

QQmlJS::MemoryPool *QQmlTypeCompiler::memoryPool()
{
    return document->jsParserEngine.pool();
}

QStringRef QQmlTypeCompiler::newStringRef(const QString &string)
{
    return document->jsParserEngine.newStringRef(string);
}

const QV4::Compiler::StringTableGenerator *QQmlTypeCompiler::stringPool() const
{
    return &document->jsGenerator.stringTable;
}

void QQmlTypeCompiler::setBindingPropertyDataPerObject(const QVector<QV4::CompiledData::BindingPropertyData> &propertyData)
{
    document->javaScriptCompilationUnit->bindingPropertyDataPerObject = propertyData;
}

QString QQmlTypeCompiler::bindingAsString(const QmlIR::Object *object, int scriptIndex) const
{
    return object->bindingAsString(document, scriptIndex);
}

QQmlCompilePass::QQmlCompilePass(QQmlTypeCompiler *typeCompiler)
    : compiler(typeCompiler)
{
}



SignalHandlerConverter::SignalHandlerConverter(QQmlTypeCompiler *typeCompiler)
    : QQmlCompilePass(typeCompiler)
    , enginePrivate(typeCompiler->enginePrivate())
    , qmlObjects(*typeCompiler->qmlObjects())
    , imports(typeCompiler->imports())
    , customParsers(typeCompiler->customParserCache())
    , resolvedTypes(typeCompiler->resolvedTypes)
    , illegalNames(QV8Engine::get(QQmlEnginePrivate::get(typeCompiler->enginePrivate()))->illegalNames())
    , propertyCaches(typeCompiler->propertyCaches())
{
}

bool SignalHandlerConverter::convertSignalHandlerExpressionsToFunctionDeclarations()
{
    for (int objectIndex = 0; objectIndex < qmlObjects.count(); ++objectIndex) {
        const QmlIR::Object * const obj = qmlObjects.at(objectIndex);
        QQmlPropertyCache *cache = propertyCaches->at(objectIndex);
        if (!cache)
            continue;
        if (QQmlCustomParser *customParser = customParsers.value(obj->inheritedTypeNameIndex)) {
            if (!(customParser->flags() & QQmlCustomParser::AcceptsSignalHandlers))
                continue;
        }
        const QString elementName = stringAt(obj->inheritedTypeNameIndex);
        if (!convertSignalHandlerExpressionsToFunctionDeclarations(obj, elementName, cache))
            return false;
    }
    return true;
}

bool SignalHandlerConverter::convertSignalHandlerExpressionsToFunctionDeclarations(const QmlIR::Object *obj, const QString &typeName, QQmlPropertyCache *propertyCache)
{
    // map from signal name defined in qml itself to list of parameters
    QHash<QString, QStringList> customSignals;

    for (QmlIR::Binding *binding = obj->firstBinding(); binding; binding = binding->next) {
        QString propertyName = stringAt(binding->propertyNameIndex);
        // Attached property?
        if (binding->type == QV4::CompiledData::Binding::Type_AttachedProperty) {
            const QmlIR::Object *attachedObj = qmlObjects.at(binding->value.objectIndex);
            auto *typeRef = resolvedTypes.value(binding->propertyNameIndex);
            QQmlType *type = typeRef ? typeRef->type : 0;
            if (!type) {
                if (imports->resolveType(propertyName, &type, 0, 0, 0)) {
                    if (type->isComposite()) {
                        QQmlTypeData *tdata = enginePrivate->typeLoader.getType(type->sourceUrl());
                        Q_ASSERT(tdata);
                        Q_ASSERT(tdata->isComplete());

                        auto compilationUnit = tdata->compilationUnit();
                        type = QQmlMetaType::qmlType(compilationUnit->metaTypeId);

                        tdata->release();
                    }
                }
            }

            const QMetaObject *attachedType = type ? type->attachedPropertiesType(enginePrivate) : 0;
            if (!attachedType)
                COMPILE_EXCEPTION(binding, tr("Non-existent attached object"));
            QQmlPropertyCache *cache = compiler->enginePrivate()->cache(attachedType);
            if (!convertSignalHandlerExpressionsToFunctionDeclarations(attachedObj, propertyName, cache))
                return false;
            continue;
        }

        if (!QmlIR::IRBuilder::isSignalPropertyName(propertyName))
            continue;

        QmlIR::PropertyResolver resolver(propertyCache);

        Q_ASSERT(propertyName.startsWith(QLatin1String("on")));
        propertyName.remove(0, 2);

        // Note that the property name could start with any alpha or '_' or '$' character,
        // so we need to do the lower-casing of the first alpha character.
        for (int firstAlphaIndex = 0; firstAlphaIndex < propertyName.size(); ++firstAlphaIndex) {
            if (propertyName.at(firstAlphaIndex).isUpper()) {
                propertyName[firstAlphaIndex] = propertyName.at(firstAlphaIndex).toLower();
                break;
            }
        }

        QList<QString> parameters;

        bool notInRevision = false;
        QQmlPropertyData *signal = resolver.signal(propertyName, &notInRevision);
        if (signal) {
            int sigIndex = propertyCache->methodIndexToSignalIndex(signal->coreIndex);
            sigIndex = propertyCache->originalClone(sigIndex);

            bool unnamedParameter = false;

            QList<QByteArray> parameterNames = propertyCache->signalParameterNames(sigIndex);
            for (int i = 0; i < parameterNames.count(); ++i) {
                const QString param = QString::fromUtf8(parameterNames.at(i));
                if (param.isEmpty())
                    unnamedParameter = true;
                else if (unnamedParameter) {
                    COMPILE_EXCEPTION(binding, tr("Signal uses unnamed parameter followed by named parameter."));
                } else if (illegalNames.contains(param)) {
                    COMPILE_EXCEPTION(binding, tr("Signal parameter \"%1\" hides global variable.").arg(param));
                }
                parameters += param;
            }
        } else {
            if (notInRevision) {
                // Try assinging it as a property later
                if (resolver.property(propertyName, /*notInRevision ptr*/0))
                    continue;

                const QString &originalPropertyName = stringAt(binding->propertyNameIndex);

                auto *typeRef = resolvedTypes.value(obj->inheritedTypeNameIndex);
                const QQmlType *type = typeRef ? typeRef->type : 0;
                if (type) {
                    COMPILE_EXCEPTION(binding, tr("\"%1.%2\" is not available in %3 %4.%5.").arg(typeName).arg(originalPropertyName).arg(type->module()).arg(type->majorVersion()).arg(type->minorVersion()));
                } else {
                    COMPILE_EXCEPTION(binding, tr("\"%1.%2\" is not available due to component versioning.").arg(typeName).arg(originalPropertyName));
                }
            }

            // Try to look up the signal parameter names in the object itself

            // build cache if necessary
            if (customSignals.isEmpty()) {
                for (const QmlIR::Signal *signal = obj->firstSignal(); signal; signal = signal->next) {
                    const QString &signalName = stringAt(signal->nameIndex);
                    customSignals.insert(signalName, signal->parameterStringList(compiler->stringPool()));
                }

                for (const QmlIR::Property *property = obj->firstProperty(); property; property = property->next) {
                    const QString propName = stringAt(property->nameIndex);
                    customSignals.insert(propName, QStringList());
                }
            }

            QHash<QString, QStringList>::ConstIterator entry = customSignals.constFind(propertyName);
            if (entry == customSignals.constEnd() && propertyName.endsWith(QLatin1String("Changed"))) {
                QString alternateName = propertyName.mid(0, propertyName.length() - static_cast<int>(strlen("Changed")));
                entry = customSignals.constFind(alternateName);
            }

            if (entry == customSignals.constEnd()) {
                // Can't find even a custom signal, then just don't do anything and try
                // keeping the binding as a regular property assignment.
                continue;
            }

            parameters = entry.value();
        }

        // Binding object to signal means connect the signal to the object's default method.
        if (binding->type == QV4::CompiledData::Binding::Type_Object) {
            binding->flags |= QV4::CompiledData::Binding::IsSignalHandlerObject;
            continue;
        }

        if (binding->type != QV4::CompiledData::Binding::Type_Script) {
            if (binding->type < QV4::CompiledData::Binding::Type_Script) {
                COMPILE_EXCEPTION(binding, tr("Cannot assign a value to a signal (expecting a script to be run)"));
            } else {
                COMPILE_EXCEPTION(binding, tr("Incorrectly specified signal assignment"));
            }
        }

        QQmlJS::MemoryPool *pool = compiler->memoryPool();

        QQmlJS::AST::FormalParameterList *paramList = 0;
        foreach (const QString &param, parameters) {
            QStringRef paramNameRef = compiler->newStringRef(param);

            if (paramList)
                paramList = new (pool) QQmlJS::AST::FormalParameterList(paramList, paramNameRef);
            else
                paramList = new (pool) QQmlJS::AST::FormalParameterList(paramNameRef);
        }

        if (paramList)
            paramList = paramList->finish();

        QmlIR::CompiledFunctionOrExpression *foe = obj->functionsAndExpressions->slowAt(binding->value.compiledScriptIndex);
        QQmlJS::AST::FunctionDeclaration *functionDeclaration = 0;
        if (QQmlJS::AST::ExpressionStatement *es = QQmlJS::AST::cast<QQmlJS::AST::ExpressionStatement*>(foe->node)) {
            if (QQmlJS::AST::FunctionExpression *fe = QQmlJS::AST::cast<QQmlJS::AST::FunctionExpression*>(es->expression)) {
                functionDeclaration = new (pool) QQmlJS::AST::FunctionDeclaration(fe->name, fe->formals, fe->body);
                functionDeclaration->functionToken = fe->functionToken;
                functionDeclaration->identifierToken = fe->identifierToken;
                functionDeclaration->lparenToken = fe->lparenToken;
                functionDeclaration->rparenToken = fe->rparenToken;
                functionDeclaration->lbraceToken = fe->lbraceToken;
                functionDeclaration->rbraceToken = fe->rbraceToken;
            }
        }
        if (!functionDeclaration) {
            QQmlJS::AST::Statement *statement = static_cast<QQmlJS::AST::Statement*>(foe->node);
            QQmlJS::AST::SourceElement *sourceElement = new (pool) QQmlJS::AST::StatementSourceElement(statement);
            QQmlJS::AST::SourceElements *elements = new (pool) QQmlJS::AST::SourceElements(sourceElement);
            elements = elements->finish();

            QQmlJS::AST::FunctionBody *body = new (pool) QQmlJS::AST::FunctionBody(elements);

            functionDeclaration = new (pool) QQmlJS::AST::FunctionDeclaration(compiler->newStringRef(stringAt(binding->propertyNameIndex)), paramList, body);
            functionDeclaration->functionToken = foe->node->firstSourceLocation();
        }
        foe->node = functionDeclaration;
        binding->propertyNameIndex = compiler->registerString(propertyName);
        binding->flags |= QV4::CompiledData::Binding::IsSignalHandlerExpression;
    }
    return true;
}

QQmlEnumTypeResolver::QQmlEnumTypeResolver(QQmlTypeCompiler *typeCompiler)
    : QQmlCompilePass(typeCompiler)
    , qmlObjects(*typeCompiler->qmlObjects())
    , propertyCaches(typeCompiler->propertyCaches())
    , imports(typeCompiler->imports())
    , resolvedTypes(&typeCompiler->resolvedTypes)
{
}

bool QQmlEnumTypeResolver::resolveEnumBindings()
{
    for (int i = 0; i < qmlObjects.count(); ++i) {
        QQmlPropertyCache *propertyCache = propertyCaches->at(i);
        if (!propertyCache)
            continue;
        const QmlIR::Object *obj = qmlObjects.at(i);

        QmlIR::PropertyResolver resolver(propertyCache);

        for (QmlIR::Binding *binding = obj->firstBinding(); binding; binding = binding->next) {
            if (binding->flags & QV4::CompiledData::Binding::IsSignalHandlerExpression
                || binding->flags & QV4::CompiledData::Binding::IsSignalHandlerObject)
                continue;

            if (binding->type != QV4::CompiledData::Binding::Type_Script)
                continue;

            const QString propertyName = stringAt(binding->propertyNameIndex);
            bool notInRevision = false;
            QQmlPropertyData *pd = resolver.property(propertyName, &notInRevision);
            if (!pd)
                continue;

            if (!pd->isEnum() && pd->propType != QMetaType::Int)
                continue;

            if (!tryQualifiedEnumAssignment(obj, propertyCache, pd, binding))
                return false;
        }
    }

    return true;
}

struct StaticQtMetaObject : public QObject
{
    static const QMetaObject *get()
        { return &staticQtMetaObject; }
};

bool QQmlEnumTypeResolver::assignEnumToBinding(QmlIR::Binding *binding, const QStringRef &enumName, int enumValue, bool isQtObject)
{
    if (enumName.length() > 0 && enumName[0].isLower() && !isQtObject) {
        COMPILE_EXCEPTION(binding, tr("Invalid property assignment: Enum value \"%1\" cannot start with a lowercase letter").arg(enumName.toString()));
    }
    binding->type = QV4::CompiledData::Binding::Type_Number;
    binding->value.d = (double)enumValue;
    binding->flags |= QV4::CompiledData::Binding::IsResolvedEnum;
    return true;
}

bool QQmlEnumTypeResolver::tryQualifiedEnumAssignment(const QmlIR::Object *obj, const QQmlPropertyCache *propertyCache, const QQmlPropertyData *prop, QmlIR::Binding *binding)
{
    bool isIntProp = (prop->propType == QMetaType::Int) && !prop->isEnum();
    if (!prop->isEnum() && !isIntProp)
        return true;

    if (!prop->isWritable() && !(binding->flags & QV4::CompiledData::Binding::InitializerForReadOnlyDeclaration))
        COMPILE_EXCEPTION(binding, tr("Invalid property assignment: \"%1\" is a read-only property").arg(stringAt(binding->propertyNameIndex)));

    Q_ASSERT(binding->type == QV4::CompiledData::Binding::Type_Script);
    const QString string = compiler->bindingAsString(obj, binding->value.compiledScriptIndex);
    if (!string.constData()->isUpper())
        return true;

    int dot = string.indexOf(QLatin1Char('.'));
    if (dot == -1 || dot == string.length()-1)
        return true;

    if (string.indexOf(QLatin1Char('.'), dot+1) != -1)
        return true;

    QHashedStringRef typeName(string.constData(), dot);
    const bool isQtObject = (typeName == QLatin1String("Qt"));
    const QStringRef enumValue = string.midRef(dot + 1);

    if (isIntProp) {
        // Allow enum assignment to ints.
        bool ok;
        int enumval = evaluateEnum(typeName.toString(), enumValue.toUtf8(), &ok);
        if (ok) {
            if (!assignEnumToBinding(binding, enumValue, enumval, isQtObject))
                return false;
        }
        return true;
    }
    QQmlType *type = 0;
    imports->resolveType(typeName, &type, 0, 0, 0);

    if (!type && !isQtObject)
        return true;

    int value = 0;
    bool ok = false;

    auto *tr = resolvedTypes->value(obj->inheritedTypeNameIndex);
    if (type && tr && tr->type == type) {
        QMetaProperty mprop = propertyCache->firstCppMetaObject()->property(prop->coreIndex);

        // When these two match, we can short cut the search
        if (mprop.isFlagType()) {
            value = mprop.enumerator().keysToValue(enumValue.toUtf8().constData(), &ok);
        } else {
            value = mprop.enumerator().keyToValue(enumValue.toUtf8().constData(), &ok);
        }
    } else {
        // Otherwise we have to search the whole type
        if (type) {
            value = type->enumValue(compiler->enginePrivate(), QHashedStringRef(enumValue), &ok);
        } else {
            QByteArray enumName = enumValue.toUtf8();
            const QMetaObject *metaObject = StaticQtMetaObject::get();
            for (int ii = metaObject->enumeratorCount() - 1; !ok && ii >= 0; --ii) {
                QMetaEnum e = metaObject->enumerator(ii);
                value = e.keyToValue(enumName.constData(), &ok);
            }
        }
    }

    if (!ok)
        return true;

    return assignEnumToBinding(binding, enumValue, value, isQtObject);
}

int QQmlEnumTypeResolver::evaluateEnum(const QString &scope, const QByteArray &enumValue, bool *ok) const
{
    Q_ASSERT_X(ok, "QQmlEnumTypeResolver::evaluateEnum", "ok must not be a null pointer");
    *ok = false;

    if (scope != QLatin1String("Qt")) {
        QQmlType *type = 0;
        imports->resolveType(scope, &type, 0, 0, 0);
        if (!type)
            return -1;
        return type->enumValue(compiler->enginePrivate(), QHashedCStringRef(enumValue.constData(), enumValue.length()), ok);
    }

    const QMetaObject *mo = StaticQtMetaObject::get();
    int i = mo->enumeratorCount();
    while (i--) {
        int v = mo->enumerator(i).keyToValue(enumValue.constData(), ok);
        if (*ok)
            return v;
    }
    return -1;
}

QQmlCustomParserScriptIndexer::QQmlCustomParserScriptIndexer(QQmlTypeCompiler *typeCompiler)
    : QQmlCompilePass(typeCompiler)
    , qmlObjects(*typeCompiler->qmlObjects())
    , customParsers(typeCompiler->customParserCache())
{
}

void QQmlCustomParserScriptIndexer::annotateBindingsWithScriptStrings()
{
    scanObjectRecursively(compiler->rootObjectIndex());
}

void QQmlCustomParserScriptIndexer::scanObjectRecursively(int objectIndex, bool annotateScriptBindings)
{
    const QmlIR::Object * const obj = qmlObjects.at(objectIndex);
    if (!annotateScriptBindings)
        annotateScriptBindings = customParsers.contains(obj->inheritedTypeNameIndex);
    for (QmlIR::Binding *binding = obj->firstBinding(); binding; binding = binding->next) {
        if (binding->type >= QV4::CompiledData::Binding::Type_Object) {
            scanObjectRecursively(binding->value.objectIndex, annotateScriptBindings);
            continue;
        } else if (binding->type != QV4::CompiledData::Binding::Type_Script)
            continue;
        if (!annotateScriptBindings)
            continue;
        const QString script = compiler->bindingAsString(obj, binding->value.compiledScriptIndex);
        binding->stringIndex = compiler->registerString(script);
    }
}

QQmlAliasAnnotator::QQmlAliasAnnotator(QQmlTypeCompiler *typeCompiler)
    : QQmlCompilePass(typeCompiler)
    , qmlObjects(*typeCompiler->qmlObjects())
    , propertyCaches(typeCompiler->propertyCaches())
{
}

void QQmlAliasAnnotator::annotateBindingsToAliases()
{
    for (int i = 0; i < qmlObjects.count(); ++i) {
        QQmlPropertyCache *propertyCache = propertyCaches->at(i);
        if (!propertyCache)
            continue;

        const QmlIR::Object *obj = qmlObjects.at(i);

        QmlIR::PropertyResolver resolver(propertyCache);
        QQmlPropertyData *defaultProperty = obj->indexOfDefaultPropertyOrAlias != -1 ? propertyCache->parent()->defaultProperty() : propertyCache->defaultProperty();

        for (QmlIR::Binding *binding = obj->firstBinding(); binding; binding = binding->next) {
            if (!binding->isValueBinding())
                continue;
            bool notInRevision = false;
            QQmlPropertyData *pd = binding->propertyNameIndex != 0 ? resolver.property(stringAt(binding->propertyNameIndex), &notInRevision) : defaultProperty;
            if (pd && pd->isAlias())
                binding->flags |= QV4::CompiledData::Binding::IsBindingToAlias;
        }
    }
}

QQmlScriptStringScanner::QQmlScriptStringScanner(QQmlTypeCompiler *typeCompiler)
    : QQmlCompilePass(typeCompiler)
    , qmlObjects(*typeCompiler->qmlObjects())
    , propertyCaches(typeCompiler->propertyCaches())
{

}

void QQmlScriptStringScanner::scan()
{
    const int scriptStringMetaType = qMetaTypeId<QQmlScriptString>();
    for (int i = 0; i < qmlObjects.count(); ++i) {
        QQmlPropertyCache *propertyCache = propertyCaches->at(i);
        if (!propertyCache)
            continue;

        const QmlIR::Object *obj = qmlObjects.at(i);

        QmlIR::PropertyResolver resolver(propertyCache);
        QQmlPropertyData *defaultProperty = obj->indexOfDefaultPropertyOrAlias != -1 ? propertyCache->parent()->defaultProperty() : propertyCache->defaultProperty();

        for (QmlIR::Binding *binding = obj->firstBinding(); binding; binding = binding->next) {
            if (binding->type != QV4::CompiledData::Binding::Type_Script)
                continue;
            bool notInRevision = false;
            QQmlPropertyData *pd = binding->propertyNameIndex != 0 ? resolver.property(stringAt(binding->propertyNameIndex), &notInRevision) : defaultProperty;
            if (!pd || pd->propType != scriptStringMetaType)
                continue;

            QmlIR::CompiledFunctionOrExpression *foe = obj->functionsAndExpressions->slowAt(binding->value.compiledScriptIndex);
            if (foe)
                foe->disableAcceleratedLookups = true;

            QString script = compiler->bindingAsString(obj, binding->value.compiledScriptIndex);
            binding->stringIndex = compiler->registerString(script);
        }
    }
}

QQmlComponentAndAliasResolver::QQmlComponentAndAliasResolver(QQmlTypeCompiler *typeCompiler)
    : QQmlCompilePass(typeCompiler)
    , enginePrivate(typeCompiler->enginePrivate())
    , pool(typeCompiler->memoryPool())
    , qmlObjects(typeCompiler->qmlObjects())
    , indexOfRootObject(typeCompiler->rootObjectIndex())
    , _componentIndex(-1)
    , resolvedTypes(&typeCompiler->resolvedTypes)
    , propertyCaches(std::move(typeCompiler->takePropertyCaches()))
{
}

void QQmlComponentAndAliasResolver::findAndRegisterImplicitComponents(const QmlIR::Object *obj, QQmlPropertyCache *propertyCache)
{
    QmlIR::PropertyResolver propertyResolver(propertyCache);

    QQmlPropertyData *defaultProperty = obj->indexOfDefaultPropertyOrAlias != -1 ? propertyCache->parent()->defaultProperty() : propertyCache->defaultProperty();

    for (QmlIR::Binding *binding = obj->firstBinding(); binding; binding = binding->next) {
        if (binding->type != QV4::CompiledData::Binding::Type_Object)
            continue;
        if (binding->flags & QV4::CompiledData::Binding::IsSignalHandlerObject)
            continue;

        const QmlIR::Object *targetObject = qmlObjects->at(binding->value.objectIndex);
        auto *tr = resolvedTypes->value(targetObject->inheritedTypeNameIndex);
        Q_ASSERT(tr);
        if (QQmlType *targetType = tr->type) {
            if (targetType->metaObject() == &QQmlComponent::staticMetaObject)
                continue;
        } else if (tr->compilationUnit) {
            if (tr->compilationUnit->rootPropertyCache()->firstCppMetaObject() == &QQmlComponent::staticMetaObject)
                continue;
        }

        QQmlPropertyData *pd = 0;
        if (binding->propertyNameIndex != 0) {
            bool notInRevision = false;
            pd = propertyResolver.property(stringAt(binding->propertyNameIndex), &notInRevision);
        } else {
            pd = defaultProperty;
        }
        if (!pd || !pd->isQObject())
            continue;

        QQmlPropertyCache *pc = enginePrivate->rawPropertyCacheForType(pd->propType);
        const QMetaObject *mo = pc->firstCppMetaObject();
        while (mo) {
            if (mo == &QQmlComponent::staticMetaObject)
                break;
            mo = mo->superClass();
        }

        if (!mo)
            continue;

        QQmlType *componentType = QQmlMetaType::qmlType(&QQmlComponent::staticMetaObject);
        Q_ASSERT(componentType);

        QmlIR::Object *syntheticComponent = pool->New<QmlIR::Object>();
        syntheticComponent->init(pool, compiler->registerString(QString::fromUtf8(componentType->typeName())), compiler->registerString(QString()));
        syntheticComponent->location = binding->valueLocation;
        syntheticComponent->flags |= QV4::CompiledData::Object::IsComponent;

        if (!resolvedTypes->contains(syntheticComponent->inheritedTypeNameIndex)) {
            auto typeRef = new QV4::CompiledData::CompilationUnit::ResolvedTypeReference;
            typeRef->type = componentType;
            typeRef->majorVersion = componentType->majorVersion();
            typeRef->minorVersion = componentType->minorVersion();
            resolvedTypes->insert(syntheticComponent->inheritedTypeNameIndex, typeRef);
        }

        qmlObjects->append(syntheticComponent);
        const int componentIndex = qmlObjects->count() - 1;
        // Keep property caches symmetric
        QQmlPropertyCache *componentCache = enginePrivate->cache(&QQmlComponent::staticMetaObject);
        propertyCaches.append(componentCache);

        QmlIR::Binding *syntheticBinding = pool->New<QmlIR::Binding>();
        *syntheticBinding = *binding;
        syntheticBinding->type = QV4::CompiledData::Binding::Type_Object;
        QString error = syntheticComponent->appendBinding(syntheticBinding, /*isListBinding*/false);
        Q_ASSERT(error.isEmpty());
        Q_UNUSED(error);

        binding->value.objectIndex = componentIndex;

        componentRoots.append(componentIndex);
    }
}

bool QQmlComponentAndAliasResolver::resolve()
{
    // Detect real Component {} objects as well as implicitly defined components, such as
    //     someItemDelegate: Item {}
    // In the implicit case Item is surrounded by a synthetic Component {} because the property
    // on the left hand side is of QQmlComponent type.
    const int objCountWithoutSynthesizedComponents = qmlObjects->count();
    for (int i = 0; i < objCountWithoutSynthesizedComponents; ++i) {
        QmlIR::Object *obj = qmlObjects->at(i);
        QQmlPropertyCache *cache = propertyCaches.at(i);
        if (obj->inheritedTypeNameIndex == 0 && !cache)
            continue;

        bool isExplicitComponent = false;

        if (obj->inheritedTypeNameIndex) {
            auto *tref = resolvedTypes->value(obj->inheritedTypeNameIndex);
            Q_ASSERT(tref);
            if (tref->type && tref->type->metaObject() == &QQmlComponent::staticMetaObject)
                isExplicitComponent = true;
        }
        if (!isExplicitComponent) {
            if (cache)
                findAndRegisterImplicitComponents(obj, cache);
            continue;
        }

        obj->flags |= QV4::CompiledData::Object::IsComponent;

        if (obj->functionCount() > 0)
            COMPILE_EXCEPTION(obj, tr("Component objects cannot declare new functions."));
        if (obj->propertyCount() > 0 || obj->aliasCount() > 0)
            COMPILE_EXCEPTION(obj, tr("Component objects cannot declare new properties."));
        if (obj->signalCount() > 0)
            COMPILE_EXCEPTION(obj, tr("Component objects cannot declare new signals."));

        if (obj->bindingCount() == 0)
            COMPILE_EXCEPTION(obj, tr("Cannot create empty component specification"));

        const QmlIR::Binding *rootBinding = obj->firstBinding();

        for (const QmlIR::Binding *b = rootBinding; b; b = b->next) {
            if (b->propertyNameIndex != 0)
                COMPILE_EXCEPTION(rootBinding, tr("Component elements may not contain properties other than id"));
        }

        if (rootBinding->next || rootBinding->type != QV4::CompiledData::Binding::Type_Object)
            COMPILE_EXCEPTION(obj, tr("Invalid component body specification"));

        // We are going to collect ids/aliases and resolve them for the root object as a separate
        // last pass.
        if (i != indexOfRootObject)
            componentRoots.append(i);

    }

    for (int i = 0; i < componentRoots.count(); ++i) {
        QmlIR::Object *component  = qmlObjects->at(componentRoots.at(i));
        const QmlIR::Binding *rootBinding = component->firstBinding();

        _componentIndex = i;
        _idToObjectIndex.clear();

        _objectsWithAliases.clear();

        if (!collectIdsAndAliases(rootBinding->value.objectIndex))
            return false;

        if (!resolveAliases())
            return false;

        component->namedObjectsInComponent.allocate(pool, _idToObjectIndex);
    }

    // Collect ids and aliases for root
    _componentIndex = -1;
    _idToObjectIndex.clear();
    _objectsWithAliases.clear();

    collectIdsAndAliases(indexOfRootObject);

    resolveAliases();

    QmlIR::Object *rootComponent = qmlObjects->at(indexOfRootObject);
    rootComponent->namedObjectsInComponent.allocate(pool, _idToObjectIndex);

    // Implicit component insertion may have added objects and thus we also need
    // to extend the symmetric propertyCaches.
    compiler->setPropertyCaches(std::move(propertyCaches));
    compiler->setComponentRoots(componentRoots);

    return true;
}

bool QQmlComponentAndAliasResolver::collectIdsAndAliases(int objectIndex)
{
    QmlIR::Object *obj = qmlObjects->at(objectIndex);

    if (obj->idNameIndex != 0) {
        if (_idToObjectIndex.contains(obj->idNameIndex)) {
            recordError(obj->locationOfIdProperty, tr("id is not unique"));
            return false;
        }
        obj->id = _idToObjectIndex.count();
        _idToObjectIndex.insert(obj->idNameIndex, objectIndex);
    }

    if (obj->aliasCount() > 0)
        _objectsWithAliases.append(objectIndex);

    // Stop at Component boundary
    if (obj->flags & QV4::CompiledData::Object::IsComponent && objectIndex != compiler->rootObjectIndex())
        return true;

    for (const QmlIR::Binding *binding = obj->firstBinding(); binding; binding = binding->next) {
        if (binding->type != QV4::CompiledData::Binding::Type_Object
            && binding->type != QV4::CompiledData::Binding::Type_AttachedProperty
            && binding->type != QV4::CompiledData::Binding::Type_GroupProperty)
            continue;

        if (!collectIdsAndAliases(binding->value.objectIndex))
            return false;
    }

    return true;
}

bool QQmlComponentAndAliasResolver::resolveAliases()
{
    foreach (int objectIndex, _objectsWithAliases) {
        const QmlIR::Object *obj = qmlObjects->at(objectIndex);

        QQmlPropertyCache *propertyCache = propertyCaches.at(objectIndex);
        Q_ASSERT(propertyCache);

        int effectiveSignalIndex = propertyCache->signalHandlerIndexCacheStart + propertyCache->propertyIndexCache.count();
        int effectivePropertyIndex = propertyCache->propertyIndexCacheStart + propertyCache->propertyIndexCache.count();

        int aliasIndex = 0;
        for (QmlIR::Alias *alias = obj->firstAlias(); alias; alias = alias->next, ++aliasIndex) {
            const int idIndex = alias->idIndex;
            const int targetObjectIndex = _idToObjectIndex.value(idIndex, -1);
            if (targetObjectIndex == -1) {
                recordError(alias->referenceLocation, tr("Invalid alias reference. Unable to find id \"%1\"").arg(stringAt(idIndex)));
                return false;
            }
            Q_ASSERT(!(alias->flags & QV4::CompiledData::Alias::Resolved));
            alias->flags |= QV4::CompiledData::Alias::Resolved;
            const QmlIR::Object *targetObject = qmlObjects->at(targetObjectIndex);
            Q_ASSERT(targetObject->id >= 0);
            alias->targetObjectId = targetObject->id;

            const QString aliasPropertyValue = stringAt(alias->propertyNameIndex);

            QStringRef property;
            QStringRef subProperty;

            const int propertySeparator = aliasPropertyValue.indexOf(QLatin1Char('.'));
            if (propertySeparator != -1) {
                property = aliasPropertyValue.leftRef(propertySeparator);
                subProperty = aliasPropertyValue.midRef(propertySeparator + 1);
            } else
                property = QStringRef(&aliasPropertyValue, 0, aliasPropertyValue.length());

            int propIdx = -1;
            int type = 0;
            bool writable = false;
            bool resettable = false;

            quint32 propertyFlags = QQmlPropertyData::IsAlias;

            if (property.isEmpty()) {
                const QmlIR::Object *targetObject = qmlObjects->at(targetObjectIndex);
                auto *typeRef = resolvedTypes->value(targetObject->inheritedTypeNameIndex);
                Q_ASSERT(typeRef);

                if (typeRef->type)
                    type = typeRef->type->typeId();
                else
                    type = typeRef->compilationUnit->metaTypeId;

                alias->flags |= QV4::CompiledData::Alias::AliasPointsToPointerObject;
                propertyFlags |= QQmlPropertyData::IsQObjectDerived;
            } else {
                QQmlPropertyCache *targetCache = propertyCaches.at(targetObjectIndex);
                Q_ASSERT(targetCache);
                QmlIR::PropertyResolver resolver(targetCache);

                QQmlPropertyData *targetProperty = resolver.property(property.toString());
                if (!targetProperty || targetProperty->coreIndex > 0x0000FFFF) {
                    recordError(alias->referenceLocation, tr("Invalid alias target location: %1").arg(property.toString()));
                    return false;
                }

                propIdx = targetProperty->coreIndex;
                type = targetProperty->propType;

                writable = targetProperty->isWritable();
                resettable = targetProperty->isResettable();

                if (!subProperty.isEmpty()) {
                    const QMetaObject *valueTypeMetaObject = QQmlValueTypeFactory::metaObjectForMetaType(type);
                    if (!valueTypeMetaObject) {
                        recordError(alias->referenceLocation, tr("Invalid alias target location: %1").arg(subProperty.toString()));
                        return false;
                    }

                    int valueTypeIndex =
                        valueTypeMetaObject->indexOfProperty(subProperty.toString().toUtf8().constData());
                    if (valueTypeIndex == -1) {
                        recordError(alias->referenceLocation, tr("Invalid alias target location: %1").arg(subProperty.toString()));
                        return false;
                    }
                    Q_ASSERT(valueTypeIndex <= 0x0000FFFF);

                    propIdx = QQmlPropertyData::encodeValueTypePropertyIndex(propIdx, valueTypeIndex);
                    if (valueTypeMetaObject->property(valueTypeIndex).isEnumType())
                        type = QVariant::Int;
                    else
                        type = valueTypeMetaObject->property(valueTypeIndex).userType();

                } else {
                    if (targetProperty->isEnum()) {
                        type = QVariant::Int;
                    } else {
                        // Copy type flags
                        propertyFlags |= targetProperty->getFlags() & QQmlPropertyData::PropTypeFlagMask;

                        if (targetProperty->isVarProperty())
                            propertyFlags |= QQmlPropertyData::IsQVariant;

                        if (targetProperty->isQObject())
                            alias->flags |= QV4::CompiledData::Alias::AliasPointsToPointerObject;
                    }
                }
            }

            alias->encodedMetaPropertyIndex = propIdx;

            if (!(alias->flags & QV4::CompiledData::Property::IsReadOnly) && writable)
                propertyFlags |= QQmlPropertyData::IsWritable;
            else
                propertyFlags &= ~QQmlPropertyData::IsWritable;

            if (resettable)
                propertyFlags |= QQmlPropertyData::IsResettable;
            else
                propertyFlags &= ~QQmlPropertyData::IsResettable;

            QString propertyName = stringAt(alias->nameIndex);

            if (obj->defaultPropertyIsAlias && aliasIndex == obj->indexOfDefaultPropertyOrAlias)
                propertyCache->_defaultPropertyName = propertyName;

            propertyCache->appendProperty(propertyName, propertyFlags, effectivePropertyIndex++,
                                          type, effectiveSignalIndex++);

        }
    }
    return true;
}

QQmlDeferredAndCustomParserBindingScanner::QQmlDeferredAndCustomParserBindingScanner(QQmlTypeCompiler *typeCompiler)
    : QQmlCompilePass(typeCompiler)
    , qmlObjects(typeCompiler->qmlObjects())
    , propertyCaches(typeCompiler->propertyCaches())
    , customParsers(typeCompiler->customParserCache())
    , _seenObjectWithId(false)
{
}

bool QQmlDeferredAndCustomParserBindingScanner::scanObject()
{
    return scanObject(compiler->rootObjectIndex());
}

bool QQmlDeferredAndCustomParserBindingScanner::scanObject(int objectIndex)
{
    QmlIR::Object *obj = qmlObjects->at(objectIndex);
    if (obj->idNameIndex != 0)
        _seenObjectWithId = true;

    if (obj->flags & QV4::CompiledData::Object::IsComponent) {
        Q_ASSERT(obj->bindingCount() == 1);
        const QV4::CompiledData::Binding *componentBinding = obj->firstBinding();
        Q_ASSERT(componentBinding->type == QV4::CompiledData::Binding::Type_Object);
        return scanObject(componentBinding->value.objectIndex);
    }

    QQmlPropertyCache *propertyCache = propertyCaches->at(objectIndex);
    if (!propertyCache)
        return true;

    QString defaultPropertyName;
    QQmlPropertyData *defaultProperty = 0;
    if (obj->indexOfDefaultPropertyOrAlias != -1) {
        QQmlPropertyCache *cache = propertyCache->parent();
        defaultPropertyName = cache->defaultPropertyName();
        defaultProperty = cache->defaultProperty();
    } else {
        defaultPropertyName = propertyCache->defaultPropertyName();
        defaultProperty = propertyCache->defaultProperty();
    }

    QQmlCustomParser *customParser = customParsers.value(obj->inheritedTypeNameIndex);

    QmlIR::PropertyResolver propertyResolver(propertyCache);

    QStringList deferredPropertyNames;
    {
        const QMetaObject *mo = propertyCache->firstCppMetaObject();
        const int namesIndex = mo->indexOfClassInfo("DeferredPropertyNames");
        if (namesIndex != -1) {
            QMetaClassInfo classInfo = mo->classInfo(namesIndex);
            deferredPropertyNames = QString::fromUtf8(classInfo.value()).split(QLatin1Char(','));
        }
    }

    for (QmlIR::Binding *binding = obj->firstBinding(); binding; binding = binding->next) {
        QQmlPropertyData *pd = 0;
        QString name = stringAt(binding->propertyNameIndex);

        if (customParser) {
            if (binding->type == QV4::CompiledData::Binding::Type_AttachedProperty) {
                if (customParser->flags() & QQmlCustomParser::AcceptsAttachedProperties) {
                    binding->flags |= QV4::CompiledData::Binding::IsCustomParserBinding;
                    obj->flags |= QV4::CompiledData::Object::HasCustomParserBindings;
                    continue;
                }
            } else if (QmlIR::IRBuilder::isSignalPropertyName(name)
                       && !(customParser->flags() & QQmlCustomParser::AcceptsSignalHandlers)) {
                obj->flags |= QV4::CompiledData::Object::HasCustomParserBindings;
                binding->flags |= QV4::CompiledData::Binding::IsCustomParserBinding;
                continue;
            }
        }

        if (name.isEmpty()) {
            pd = defaultProperty;
            name = defaultPropertyName;
        } else {
            if (name.constData()->isUpper())
                continue;

            bool notInRevision = false;
            pd = propertyResolver.property(name, &notInRevision, QmlIR::PropertyResolver::CheckRevision);
        }

        bool seenSubObjectWithId = false;

        if (binding->type >= QV4::CompiledData::Binding::Type_Object && (pd || binding->isAttachedProperty())) {
            qSwap(_seenObjectWithId, seenSubObjectWithId);
            const bool subObjectValid = scanObject(binding->value.objectIndex);
            qSwap(_seenObjectWithId, seenSubObjectWithId);
            if (!subObjectValid)
                return false;
            _seenObjectWithId |= seenSubObjectWithId;
        }

        if (!seenSubObjectWithId
            && !deferredPropertyNames.isEmpty() && deferredPropertyNames.contains(name)) {

            binding->flags |= QV4::CompiledData::Binding::IsDeferredBinding;
            obj->flags |= QV4::CompiledData::Object::HasDeferredBindings;
        }

        if (binding->flags & QV4::CompiledData::Binding::IsSignalHandlerExpression
            || binding->flags & QV4::CompiledData::Binding::IsSignalHandlerObject)
            continue;

        if (!pd) {
            if (customParser) {
                obj->flags |= QV4::CompiledData::Object::HasCustomParserBindings;
                binding->flags |= QV4::CompiledData::Binding::IsCustomParserBinding;
            }
        }
    }

    return true;
}

QQmlJSCodeGenerator::QQmlJSCodeGenerator(QQmlTypeCompiler *typeCompiler, QmlIR::JSCodeGen *v4CodeGen)
    : QQmlCompilePass(typeCompiler)
    , resolvedTypes(typeCompiler->resolvedTypes)
    , customParsers(typeCompiler->customParserCache())
    , qmlObjects(*typeCompiler->qmlObjects())
    , propertyCaches(typeCompiler->propertyCaches())
    , v4CodeGen(v4CodeGen)
{
}

bool QQmlJSCodeGenerator::generateCodeForComponents()
{
    const QVector<quint32> &componentRoots = compiler->componentRoots();
    for (int i = 0; i < componentRoots.count(); ++i) {
        if (!compileComponent(componentRoots.at(i)))
            return false;
    }

    return compileComponent(compiler->rootObjectIndex());
}

bool QQmlJSCodeGenerator::compileComponent(int contextObject)
{
    const QmlIR::Object *obj = qmlObjects.at(contextObject);
    if (obj->flags & QV4::CompiledData::Object::IsComponent) {
        Q_ASSERT(obj->bindingCount() == 1);
        const QV4::CompiledData::Binding *componentBinding = obj->firstBinding();
        Q_ASSERT(componentBinding->type == QV4::CompiledData::Binding::Type_Object);
        contextObject = componentBinding->value.objectIndex;
    }

    QmlIR::JSCodeGen::ObjectIdMapping idMapping;
    idMapping.reserve(obj->namedObjectsInComponent.count);
    for (int i = 0; i < obj->namedObjectsInComponent.count; ++i) {
        const int objectIndex = obj->namedObjectsInComponent.at(i);
        QmlIR::JSCodeGen::IdMapping m;
        const QmlIR::Object *obj = qmlObjects.at(objectIndex);
        m.name = stringAt(obj->idNameIndex);
        m.idIndex = obj->id;
        m.type = propertyCaches->at(objectIndex);

        auto *tref = resolvedTypes.value(obj->inheritedTypeNameIndex);
        if (tref && tref->isFullyDynamicType)
            m.type = 0;

        idMapping << m;
    }
    v4CodeGen->beginContextScope(idMapping, propertyCaches->at(contextObject));

    if (!compileJavaScriptCodeInObjectsRecursively(contextObject, contextObject))
        return false;

    return true;
}

bool QQmlJSCodeGenerator::compileJavaScriptCodeInObjectsRecursively(int objectIndex, int scopeObjectIndex)
{
    QmlIR::Object *object = qmlObjects.at(objectIndex);
    if (object->flags & QV4::CompiledData::Object::IsComponent)
        return true;

    if (object->functionsAndExpressions->count > 0) {
        QQmlPropertyCache *scopeObject = propertyCaches->at(scopeObjectIndex);
        v4CodeGen->beginObjectScope(scopeObject);

        QList<QmlIR::CompiledFunctionOrExpression> functionsToCompile;
        for (QmlIR::CompiledFunctionOrExpression *foe = object->functionsAndExpressions->first; foe; foe = foe->next) {
            const bool haveCustomParser = customParsers.contains(object->inheritedTypeNameIndex);
            if (haveCustomParser)
                foe->disableAcceleratedLookups = true;
            functionsToCompile << *foe;
        }
        const QVector<int> runtimeFunctionIndices = v4CodeGen->generateJSCodeForFunctionsAndBindings(functionsToCompile);
        QList<QQmlError> jsErrors = v4CodeGen->qmlErrors();
        if (!jsErrors.isEmpty()) {
            foreach (const QQmlError &e, jsErrors)
                compiler->recordError(e);
            return false;
        }

        QQmlJS::MemoryPool *pool = compiler->memoryPool();
        object->runtimeFunctionIndices.allocate(pool, runtimeFunctionIndices);
    }

    for (const QmlIR::Binding *binding = object->firstBinding(); binding; binding = binding->next) {
        if (binding->type < QV4::CompiledData::Binding::Type_Object)
            continue;

        int target = binding->value.objectIndex;
        int scope = binding->type == QV4::CompiledData::Binding::Type_Object ? target : scopeObjectIndex;

        if (!compileJavaScriptCodeInObjectsRecursively(binding->value.objectIndex, scope))
            return false;
    }

    return true;
}

QQmlDefaultPropertyMerger::QQmlDefaultPropertyMerger(QQmlTypeCompiler *typeCompiler)
    : QQmlCompilePass(typeCompiler)
    , qmlObjects(*typeCompiler->qmlObjects())
    , propertyCaches(typeCompiler->propertyCaches())
{

}

void QQmlDefaultPropertyMerger::mergeDefaultProperties()
{
    for (int i = 0; i < qmlObjects.count(); ++i)
        mergeDefaultProperties(i);
}

void QQmlDefaultPropertyMerger::mergeDefaultProperties(int objectIndex)
{
    QQmlPropertyCache *propertyCache = propertyCaches->at(objectIndex);
    if (!propertyCache)
        return;

    QmlIR::Object *object = qmlObjects.at(objectIndex);

    QString defaultProperty = object->indexOfDefaultPropertyOrAlias != -1 ? propertyCache->parent()->defaultPropertyName() : propertyCache->defaultPropertyName();
    QmlIR::Binding *bindingsToReinsert = 0;
    QmlIR::Binding *tail = 0;

    QmlIR::Binding *previousBinding = 0;
    QmlIR::Binding *binding = object->firstBinding();
    while (binding) {
        if (binding->propertyNameIndex == 0 || stringAt(binding->propertyNameIndex) != defaultProperty) {
            previousBinding = binding;
            binding = binding->next;
            continue;
        }

        QmlIR::Binding *toReinsert = binding;
        binding = object->unlinkBinding(previousBinding, binding);

        if (!tail) {
            bindingsToReinsert = toReinsert;
            tail = toReinsert;
        } else {
            tail->next = toReinsert;
            tail = tail->next;
        }
        tail->next = 0;
    }

    binding = bindingsToReinsert;
    while (binding) {
        QmlIR::Binding *toReinsert = binding;
        binding = binding->next;
        object->insertSorted(toReinsert);
    }
}

QQmlJavaScriptBindingExpressionSimplificationPass::QQmlJavaScriptBindingExpressionSimplificationPass(QQmlTypeCompiler *typeCompiler)
    : QQmlCompilePass(typeCompiler)
    , qmlObjects(*typeCompiler->qmlObjects())
    , jsModule(typeCompiler->jsIRModule())
{

}

void QQmlJavaScriptBindingExpressionSimplificationPass::reduceTranslationBindings()
{
    for (int i = 0; i < qmlObjects.count(); ++i)
        reduceTranslationBindings(i);
    if (!irFunctionsToRemove.isEmpty()) {
        QQmlIRFunctionCleanser cleanser(compiler, irFunctionsToRemove);
        cleanser.clean();
    }
}

void QQmlJavaScriptBindingExpressionSimplificationPass::reduceTranslationBindings(int objectIndex)
{
    const QmlIR::Object *obj = qmlObjects.at(objectIndex);

    for (QmlIR::Binding *binding = obj->firstBinding(); binding; binding = binding->next) {
        if (binding->type != QV4::CompiledData::Binding::Type_Script)
            continue;

        const int irFunctionIndex = obj->runtimeFunctionIndices.at(binding->value.compiledScriptIndex);
        QV4::IR::Function *irFunction = jsModule->functions.at(irFunctionIndex);
        if (simplifyBinding(irFunction, binding)) {
            irFunctionsToRemove.append(irFunctionIndex);
            jsModule->functions[irFunctionIndex] = 0;
            delete irFunction;
        }
    }
}

void QQmlJavaScriptBindingExpressionSimplificationPass::visitMove(QV4::IR::Move *move)
{
    QV4::IR::Temp *target = move->target->asTemp();
    if (!target || target->kind != QV4::IR::Temp::VirtualRegister) {
        discard();
        return;
    }

    if (QV4::IR::Call *call = move->source->asCall()) {
        if (QV4::IR::Name *n = call->base->asName()) {
            if (n->builtin == QV4::IR::Name::builtin_invalid) {
                visitFunctionCall(n->id, call->args, target);
                return;
            }
        }
        discard();
        return;
    }

    if (QV4::IR::Name *n = move->source->asName()) {
        if (n->builtin == QV4::IR::Name::builtin_qml_context
            || n->builtin == QV4::IR::Name::builtin_qml_imported_scripts_object) {
            // these are free of side-effects
            return;
        }
        discard();
        return;
    }

    if (!move->source->asTemp() && !move->source->asString() && !move->source->asConst()) {
        discard();
        return;
    }

    _temps[target->index] = move->source;
}

void QQmlJavaScriptBindingExpressionSimplificationPass::visitFunctionCall(const QString *name, QV4::IR::ExprList *args, QV4::IR::Temp *target)
{
    // more than one function call?
    if (_nameOfFunctionCalled) {
        discard();
        return;
    }

    _nameOfFunctionCalled = name;

    _functionParameters.clear();
    while (args) {
        int slot;
        if (QV4::IR::Temp *param = args->expr->asTemp()) {
            if (param->kind != QV4::IR::Temp::VirtualRegister) {
                discard();
                return;
            }
            slot = param->index;
            _functionParameters.append(slot);
        } else if (QV4::IR::Const *param = args->expr->asConst()) {
            slot = --_synthesizedConsts;
            Q_ASSERT(!_temps.contains(slot));
            _temps[slot] = param;
            _functionParameters.append(slot);
        }
        args = args->next;
    }

    _functionCallReturnValue = target->index;
}

void QQmlJavaScriptBindingExpressionSimplificationPass::visitRet(QV4::IR::Ret *ret)
{
    // nothing initialized earlier?
    if (_returnValueOfBindingExpression != -1) {
        discard();
        return;
    }
    QV4::IR::Temp *target = ret->expr->asTemp();
    if (!target || target->kind != QV4::IR::Temp::VirtualRegister) {
        discard();
        return;
    }
    _returnValueOfBindingExpression = target->index;
}

bool QQmlJavaScriptBindingExpressionSimplificationPass::simplifyBinding(QV4::IR::Function *function, QmlIR::Binding *binding)
{
    _canSimplify = true;
    _nameOfFunctionCalled = 0;
    _functionParameters.clear();
    _functionCallReturnValue = -1;
    _temps.clear();
    _returnValueOfBindingExpression = -1;
    _synthesizedConsts = 0;

    // It would seem unlikely that function with some many basic blocks (after optimization)
    // consists merely of a qsTr call or a constant value return ;-)
    if (function->basicBlockCount() > 10)
        return false;

    for (QV4::IR::BasicBlock *bb : function->basicBlocks()) {
        for (QV4::IR::Stmt *s : bb->statements()) {
            visit(s);
            if (!_canSimplify)
                return false;
        }
        if (!_canSimplify)
            return false;
    }

    if (_returnValueOfBindingExpression == -1)
        return false;

    if (_canSimplify) {
        if (_nameOfFunctionCalled) {
            if (_functionCallReturnValue != _returnValueOfBindingExpression)
                return false;
            return detectTranslationCallAndConvertBinding(binding);
        }
    }

    return false;
}

bool QQmlJavaScriptBindingExpressionSimplificationPass::detectTranslationCallAndConvertBinding(QmlIR::Binding *binding)
{
    if (*_nameOfFunctionCalled == QLatin1String("qsTr")) {
        QString translation;
        QV4::CompiledData::TranslationData translationData;
        translationData.number = -1;
        translationData.commentIndex = 0; // empty string

        QVector<int>::ConstIterator param = _functionParameters.constBegin();
        QVector<int>::ConstIterator end = _functionParameters.constEnd();
        if (param == end)
            return false;

        QV4::IR::String *stringParam = _temps[*param]->asString();
        if (!stringParam)
            return false;

        translation = *stringParam->value;

        ++param;
        if (param != end) {
            stringParam = _temps[*param]->asString();
            if (!stringParam)
                return false;
            translationData.commentIndex = compiler->registerString(*stringParam->value);
            ++param;

            if (param != end) {
                QV4::IR::Const *constParam = _temps[*param]->asConst();
                if (!constParam || constParam->type != QV4::IR::SInt32Type)
                    return false;

                translationData.number = int(constParam->value);
                ++param;
            }
        }

        if (param != end)
            return false;

        binding->type = QV4::CompiledData::Binding::Type_Translation;
        binding->stringIndex = compiler->registerString(translation);
        binding->value.translationData = translationData;
        return true;
    } else if (*_nameOfFunctionCalled == QLatin1String("qsTrId")) {
        QString id;
        QV4::CompiledData::TranslationData translationData;
        translationData.number = -1;
        translationData.commentIndex = 0; // empty string, but unused

        QVector<int>::ConstIterator param = _functionParameters.constBegin();
        QVector<int>::ConstIterator end = _functionParameters.constEnd();
        if (param == end)
            return false;

        QV4::IR::String *stringParam = _temps[*param]->asString();
        if (!stringParam)
            return false;

        id = *stringParam->value;

        ++param;
        if (param != end) {
            QV4::IR::Const *constParam = _temps[*param]->asConst();
            if (!constParam || constParam->type != QV4::IR::SInt32Type)
                return false;

            translationData.number = int(constParam->value);
            ++param;
        }

        if (param != end)
            return false;

        binding->type = QV4::CompiledData::Binding::Type_TranslationById;
        binding->stringIndex = compiler->registerString(id);
        binding->value.translationData = translationData;
        return true;
    } else if (*_nameOfFunctionCalled == QLatin1String("QT_TR_NOOP") || *_nameOfFunctionCalled == QLatin1String("QT_TRID_NOOP")) {
        QVector<int>::ConstIterator param = _functionParameters.constBegin();
        QVector<int>::ConstIterator end = _functionParameters.constEnd();
        if (param == end)
            return false;

        QV4::IR::String *stringParam = _temps[*param]->asString();
        if (!stringParam)
            return false;

        ++param;
        if (param != end)
            return false;

        binding->type = QV4::CompiledData::Binding::Type_String;
        binding->stringIndex = compiler->registerString(*stringParam->value);
        return true;
    } else if (*_nameOfFunctionCalled == QLatin1String("QT_TRANSLATE_NOOP")) {
        QVector<int>::ConstIterator param = _functionParameters.constBegin();
        QVector<int>::ConstIterator end = _functionParameters.constEnd();
        if (param == end)
            return false;

        ++param;
        if (param == end)
            return false;

        QV4::IR::String *stringParam = _temps[*param]->asString();
        if (!stringParam)
            return false;

        ++param;
        if (param != end)
            return false;

        binding->type = QV4::CompiledData::Binding::Type_String;
        binding->stringIndex = compiler->registerString(*stringParam->value);
        return true;
    }
    return false;
}

QQmlIRFunctionCleanser::QQmlIRFunctionCleanser(QQmlTypeCompiler *typeCompiler, const QVector<int> &functionsToRemove)
    : QQmlCompilePass(typeCompiler)
    , module(typeCompiler->jsIRModule())
    , functionsToRemove(functionsToRemove)
{
}

void QQmlIRFunctionCleanser::clean()
{
    QVector<QV4::IR::Function*> newFunctions;
    newFunctions.reserve(module->functions.count() - functionsToRemove.count());

    newFunctionIndices.resize(module->functions.count());

    for (int i = 0; i < module->functions.count(); ++i) {
        QV4::IR::Function *f = module->functions.at(i);
        Q_ASSERT(f || functionsToRemove.contains(i));
        if (f) {
            newFunctionIndices[i] = newFunctions.count();
            newFunctions << f;
        }
    }

    module->functions = newFunctions;

    foreach (QV4::IR::Function *function, module->functions) {
        for (QV4::IR::BasicBlock *block : function->basicBlocks()) {
            for (QV4::IR::Stmt *s : block->statements()) {
                visit(s);
            }
        }
    }

    foreach (QmlIR::Object *obj, *compiler->qmlObjects()) {
        for (int i = 0; i < obj->runtimeFunctionIndices.count; ++i)
            obj->runtimeFunctionIndices[i] = newFunctionIndices[obj->runtimeFunctionIndices.at(i)];
    }
}

void QQmlIRFunctionCleanser::visit(QV4::IR::Stmt *s)
{

    switch (s->stmtKind) {
    case QV4::IR::Stmt::PhiStmt:
        // nothing to do
        break;
    default:
        STMT_VISIT_ALL_KINDS(s);
        break;
    }
}

void QQmlIRFunctionCleanser::visit(QV4::IR::Expr *e)
{
    switch (e->exprKind) {
    case QV4::IR::Expr::ClosureExpr: {
        auto closure = e->asClosure();
        closure->value = newFunctionIndices.at(closure->value);
    } break;
    default:
        EXPR_VISIT_ALL_KINDS(e);
        break;
    }
}

QT_END_NAMESPACE
