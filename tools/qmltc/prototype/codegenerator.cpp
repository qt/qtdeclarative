/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the tools applications of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "prototype/codegenerator.h"
#include "prototype/qml2cppdefaultpasses.h"
#include "prototype/qml2cpppropertyutils.h"
#include "prototype/codegeneratorutil.h"
#include "prototype/codegeneratorwriter.h"

#include <QtCore/qfileinfo.h>
#include <QtCore/qhash.h>
#include <QtCore/qset.h>
#include <QtCore/qregularexpression.h>

#include <QtCore/qloggingcategory.h>

#include <private/qqmljsutils_p.h>

#include <optional>
#include <utility>
#include <numeric>

static constexpr char newLineLatin1[] =
#ifdef Q_OS_WIN32
        "\r\n";
#else
        "\n";
#endif

Q_LOGGING_CATEGORY(lcCodeGenerator, "qml.qmltc.compiler", QtWarningMsg);

static void writeToFile(const QString &path, const QByteArray &data)
{
    // When not using dependency files, changing a single qml invalidates all
    // qml files and would force the recompilation of everything. To avoid that,
    // we check if the data is equal to the existing file, if yes, don't touch
    // it so the build system will not recompile unnecessary things.
    //
    // If the build system use dependency file, we should anyway touch the file
    // so qmlcompiler is not re-run
    QFileInfo fi(path);
    if (fi.exists() && fi.size() == data.size()) {
        QFile oldFile(path);
        oldFile.open(QIODevice::ReadOnly);
        if (oldFile.readAll() == data)
            return;
    }
    QFile file(path);
    file.open(QIODevice::WriteOnly);
    file.write(data);
}

static QString figureReturnType(const QQmlJSMetaMethod &m)
{
    const bool isVoidMethod =
            m.returnTypeName() == u"void" || m.methodType() == QQmlJSMetaMethod::Signal;
    Q_ASSERT(isVoidMethod || m.returnType());
    // TODO: should really be m.returnTypeName(). for now, avoid inconsistencies
    // due to QML/C++ name collisions and unique C++ name generation
    QString type;
    if (isVoidMethod) {
        type = u"void"_qs;
    } else {
        type = m.returnType()->augmentedInternalName();
    }
    return type;
}

static QList<QQmlJSAotVariable>
compileMethodParameters(const QStringList &names,
                        const QList<QSharedPointer<const QQmlJSScope>> &types,
                        bool allowUnnamed = false)
{
    QList<QQmlJSAotVariable> paramList;
    const auto size = names.size();
    paramList.reserve(size);
    for (qsizetype i = 0; i < size; ++i) {
        Q_ASSERT(types[i]); // assume verified
        QString name = names[i];
        Q_ASSERT(allowUnnamed || !name.isEmpty()); // assume verified
        if (name.isEmpty() && allowUnnamed)
            name = u"unnamed_" + QString::number(i);
        paramList.emplaceBack(
                QQmlJSAotVariable { types[i]->augmentedInternalName(), name, QString() });
    }
    return paramList;
}

// this version just returns runtimeFunctionIndices[relative]. used in
// property assignments like `property var p: function() {}`
static qsizetype relativeToAbsoluteRuntimeIndex(const QmlIR::Object *irObject, qsizetype relative)
{
    // TODO: for some reason, this function is necessary. do we really need it?
    // why does it have to be a special case?
    return irObject->runtimeFunctionIndices.at(relative);
}

// this version expects that \a relative points to a to-be-executed function.
// for this, it needs to detect whether a case like `onSignal: function() {}` is
// present and return nested function index instead of the
// runtimeFunctionIndices[relative]
static qsizetype relativeToAbsoluteRuntimeIndex(const QmlIR::Document *doc,
                                                const QmlIR::Object *irObject, qsizetype relative)
{
    int absoluteIndex = irObject->runtimeFunctionIndices.at(relative);
    Q_ASSERT(absoluteIndex >= 0);
    Q_ASSERT(doc->javaScriptCompilationUnit.unitData());
    const QV4::CompiledData::Function *f =
            doc->javaScriptCompilationUnit.unitData()->functionAt(absoluteIndex);
    Q_ASSERT(f);
    if (f->nestedFunctionIndex != std::numeric_limits<uint32_t>::max())
        return f->nestedFunctionIndex;
    return absoluteIndex;
}

// finds property for given scope and returns it together with the absolute
// property index in the property array of the corresponding QMetaObject.
static std::pair<QQmlJSMetaProperty, int> getMetaPropertyIndex(const QQmlJSScope::ConstPtr &scope,
                                                               const QString &propertyName)
{
    auto owner = QQmlJSScope::ownerOfProperty(scope, propertyName);
    Q_ASSERT(owner);
    auto p = owner->ownProperty(propertyName);
    if (!p.isValid())
        return { p, -1 };
    int index = p.index();
    if (index < 0) // this property doesn't have index - comes from QML
        return { p, -1 };
    // owner is not included in absolute index
    for (QQmlJSScope::ConstPtr base = owner->baseType(); base; base = base->baseType())
        index += int(base->ownProperties().size());
    return { p, index };
}

struct QmlIrBindingCompare
{
private:
    using T = QmlIR::Binding;
    using I = typename QmlIR::PoolList<T>::Iterator;
    static QHash<uint, qsizetype> orderTable;

public:
    bool operator()(const I &x, const I &y) const
    {
        return orderTable[x->type()] < orderTable[y->type()];
    }
    bool operator()(const T &x, const T &y) const
    {
        return orderTable[x.type()] < orderTable[y.type()];
    }
};

QHash<uint, qsizetype> QmlIrBindingCompare::orderTable = {
    { QmlIR::Binding::Type_Invalid, 100 },
    // value assignments (and object bindings) are "equal"
    { QmlIR::Binding::Type_Boolean, 0 },
    { QmlIR::Binding::Type_Number, 0 },
    { QmlIR::Binding::Type_String, 0 },
    { QmlIR::Binding::Type_Object, 0 },
    // translations are also "equal" between themselves, but come after
    // assignments
    { QmlIR::Binding::Type_Translation, 1 },
    { QmlIR::Binding::Type_TranslationById, 1 },
    // attached properties and group properties might contain assignments
    // inside, so come next
    { QmlIR::Binding::Type_AttachedProperty, 2 },
    { QmlIR::Binding::Type_GroupProperty, 2 },
    // JS bindings come last because they can use values from other categories
    { QmlIR::Binding::Type_Script, 3 },
    // { QmlIR::Binding::Type_Null, 100 }, // TODO: what is this used for?
};

static QList<typename QmlIR::PoolList<QmlIR::Binding>::Iterator>
toOrderedSequence(typename QmlIR::PoolList<QmlIR::Binding>::Iterator first,
                  typename QmlIR::PoolList<QmlIR::Binding>::Iterator last, qsizetype n)
{
    // bindings actually have to sorted so that e.g. value assignments come
    // before script bindings. this is important for the code generator as it
    // would just emit instructions one by one, regardless of the ordering
    using I = typename QmlIR::PoolList<QmlIR::Binding>::Iterator;
    QList<I> sorted;
    sorted.reserve(n);
    for (auto it = first; it != last; ++it)
        sorted << it;
    // NB: use stable sort for bindings, because this matters: relative order of
    // bindings must be preserved - this might affect UI
    std::stable_sort(sorted.begin(), sorted.end(), QmlIrBindingCompare {});
    return sorted;
}

Q_LOGGING_CATEGORY(lcCodeGen, "qml.compiler.CodeGenerator", QtWarningMsg);

CodeGenerator::CodeGenerator(const QString &url, QQmlJSLogger *logger, QmlIR::Document *doc,
                             const Qmltc::TypeResolver *localResolver)
    : m_url(url),
      m_logger(logger),
      m_doc(doc),
      m_localTypeResolver(localResolver),
      m_qmlSource(doc->code.split(QLatin1String(newLineLatin1)))
{
}

void CodeGenerator::constructObjects(QSet<QString> &requiredCppIncludes)
{
    const auto &objects = m_doc->objects;
    m_objects.reserve(objects.size());

    m_typeToObjectIndex.reserve(objects.size());

    for (qsizetype objectIndex = 0; objectIndex != objects.size(); ++objectIndex) {
        QmlIR::Object *irObject = objects[objectIndex];
        if (!irObject) {
            recordError(QQmlJS::SourceLocation {},
                        u"Internal compiler error: IR object is null"_qs);
            return;
        }
        QQmlJSScope::Ptr object = m_localTypeResolver->scopeForLocation(irObject->location);
        if (!object) {
            recordError(irObject->location, u"Object of unknown type"_qs);
            return;
        }
        m_typeToObjectIndex.insert(object, objectIndex);
        m_objects.emplaceBack(CodeGenObject { irObject, object });
    }

    // objects are constructed, now we can run compiler passes to make sure the
    // objects are in good state
    Qml2CppCompilerPassExecutor executor(m_doc, m_localTypeResolver, m_url, m_objects,
                                         m_typeToObjectIndex);
    executor.addPass(&verifyTypes);
    executor.addPass(&checkForNamingCollisionsWithCpp);
    executor.addPass([this](const Qml2CppContext &context, QList<Qml2CppObject> &objects) {
        m_typeCounts = makeUniqueCppNames(context, objects);
    });
    const auto setupQmlBaseTypes = [&](const Qml2CppContext &context,
                                       QList<Qml2CppObject> &objects) {
        m_qmlCompiledBaseTypes = setupQmlCppTypes(context, objects);
    };
    executor.addPass(setupQmlBaseTypes);
    const auto resolveAliases = [&](const Qml2CppContext &context, QList<Qml2CppObject> &objects) {
        m_aliasesToIds = deferredResolveValidateAliases(context, objects);
    };
    executor.addPass(resolveAliases);
    const auto populateCppIncludes = [&](const Qml2CppContext &context,
                                         QList<Qml2CppObject> &objects) {
        requiredCppIncludes = findCppIncludes(context, objects);
    };
    executor.addPass(populateCppIncludes);
    const auto resolveExplicitComponents = [&](const Qml2CppContext &context,
                                               QList<Qml2CppObject> &objects) {
        m_componentIndices.insert(findAndResolveExplicitComponents(context, objects));
    };
    const auto resolveImplicitComponents = [&](const Qml2CppContext &context,
                                               QList<Qml2CppObject> &objects) {
        m_componentIndices.insert(findAndResolveImplicitComponents(context, objects));
    };
    executor.addPass(resolveExplicitComponents);
    executor.addPass(resolveImplicitComponents);
    executor.addPass(&setObjectIds); // NB: must be after Component resolution
    const auto setImmediateParents = [&](const Qml2CppContext &context,
                                         QList<Qml2CppObject> &objects) {
        m_immediateParents = findImmediateParents(context, objects);
    };
    executor.addPass(setImmediateParents);
    const auto setIgnoredTypes = [&](const Qml2CppContext &context, QList<Qml2CppObject> &objects) {
        m_ignoredTypes = collectIgnoredTypes(context, objects);
    };
    executor.addPass(setIgnoredTypes);
    executor.addPass(&setDeferredBindings);

    // run all passes:
    executor.run(m_logger);
}

void CodeGenerator::generate(const Options &options)
{
    m_options = options;
    GeneratedCode code;
    const QString rootClassName = QFileInfo(m_url).baseName();
    Q_ASSERT(!rootClassName.isEmpty());
    Q_ASSERT(!options.outputHFile.isEmpty());
    Q_ASSERT(!options.outputCppFile.isEmpty());
    const QString hPath = options.outputHFile;
    const QString cppPath = options.outputCppFile;
    m_isAnonymous = rootClassName.at(0).isLower();
    const QString url = QFileInfo(m_url).fileName();

    // generate url method straight away to be able to use it everywhere
    compileUrlMethod();

    QSet<QString> requiredCppIncludes;
    constructObjects(requiredCppIncludes); // this populates all the codegen objects
    // no point in compiling anything if there are errors
    if (m_logger->hasErrors() || m_logger->hasWarnings())
        return;

    if (m_objects.isEmpty()) {
        recordError(QQmlJS::SourceLocation(), u"No relevant QML types to compile."_qs);
        return;
    }

    const auto isComponent = [](const QQmlJSScope::ConstPtr &type) {
        auto base = type->baseType();
        return base && base->internalName() == u"QQmlComponent"_qs;
    };
    const auto &root = m_objects.at(0).type;

    QList<QQmlJSAotObject> compiledObjects;
    if (isComponent(root)) {
        compiledObjects.reserve(1);
        compiledObjects.emplaceBack(); // create new object
        const auto compile = [this](QQmlJSAotObject &current, const CodeGenObject &object) {
            this->compileQQmlComponentElements(current, object);
        };
        compileObject(compiledObjects.back(), m_objects.at(0), compile);
    } else {
        const auto compile = [this](QQmlJSAotObject &current, const CodeGenObject &object) {
            this->compileObjectElements(current, object);
        };

        // compile everything
        compiledObjects.reserve(m_objects.size());
        for (const auto &object : m_objects) {
            if (object.type->scopeType() != QQmlJSScope::QMLScope) {
                qCDebug(lcCodeGenerator) << u"Scope '" + object.type->internalName()
                                + u"' is not QMLScope, so it is skipped.";
                continue;
            }
            if (m_ignoredTypes.contains(object.type)) {
                // e.g. object.type is a view delegate
                qCDebug(lcCodeGenerator) << u"Scope '" + object.type->internalName()
                                + u"' is a QQmlComponent sub-component. It won't be compiled to "
                                  u"C++.";
                continue;
            }
            compiledObjects.emplaceBack(); // create new object
            compileObject(compiledObjects.back(), object, compile);
        }
    }
    // no point in generating anything if there are errors
    if (m_logger->hasErrors() || m_logger->hasWarnings())
        return;

    QQmlJSProgram program { compiledObjects,      m_urlMethod,        url, hPath, cppPath,
                            options.outNamespace, requiredCppIncludes };

    // write everything
    GeneratedCodeUtils codeUtils(code);
    CodeGeneratorWriter::write(codeUtils, program);

    writeToFile(hPath, code.header.toUtf8());
    writeToFile(cppPath, code.implementation.toUtf8());
}

QString buildCallSpecialMethodValue(bool documentRoot, const QString &outerFlagName,
                                    bool overridesInterface)
{
    const QString callInBase = overridesInterface ? u"false"_qs : u"true"_qs;
    if (documentRoot) {
        return outerFlagName + u" && " + callInBase;
    } else {
        return callInBase;
    }
}

void CodeGenerator::compileObject(
        QQmlJSAotObject &compiled, const CodeGenObject &object,
        std::function<void(QQmlJSAotObject &, const CodeGenObject &)> compileElements)
{
    if (object.type->isSingleton()) {
        recordError(object.type->sourceLocation(), u"Singleton types are not supported"_qs);
        return;
    }

    compiled.cppType = object.type->internalName();
    const QString baseClass = object.type->baseType()->internalName();

    const bool baseTypeIsCompiledQml = m_qmlCompiledBaseTypes.contains(object.type->baseTypeName());
    const qsizetype objectIndex = m_typeToObjectIndex[object.type];
    const bool documentRoot = objectIndex == 0;
    const bool hasParserStatusInterface = object.type->hasInterface(u"QQmlParserStatus"_qs);
    const bool hasFinalizerHookInterface = object.type->hasInterface(u"QQmlFinalizerHook"_qs);

    compiled.baseClasses = { baseClass };

    // add ctors code
    compiled.baselineCtor.access = QQmlJSMetaMethod::Protected;
    if (documentRoot) {
        compiled.externalCtor.access = QQmlJSMetaMethod::Public;
    } else {
        compiled.externalCtor.access = QQmlJSMetaMethod::Protected;
    }
    compiled.init.access = QQmlJSMetaMethod::Protected;
    compiled.endInit.access = QQmlJSMetaMethod::Protected;
    compiled.completeComponent.access = QQmlJSMetaMethod::Protected;
    compiled.finalizeComponent.access = QQmlJSMetaMethod::Protected;
    compiled.handleOnCompleted.access = QQmlJSMetaMethod::Protected;

    compiled.baselineCtor.name = compiled.cppType;
    compiled.externalCtor.name = compiled.cppType;
    compiled.init.name = u"QML_init"_qs;
    compiled.init.returnType = u"QQmlRefPointer<QQmlContextData>"_qs;
    compiled.endInit.name = u"QML_endInit"_qs;
    compiled.endInit.returnType = u"void"_qs;
    compiled.completeComponent.name = u"QML_completeComponent"_qs;
    compiled.completeComponent.returnType = u"void"_qs;
    compiled.finalizeComponent.name = u"QML_finalizeComponent"_qs;
    compiled.finalizeComponent.returnType = u"void"_qs;
    compiled.handleOnCompleted.name = u"QML_handleOnCompleted"_qs;
    compiled.handleOnCompleted.returnType = u"void"_qs;

    QQmlJSAotVariable engine(u"QQmlEngine *"_qs, u"engine"_qs, QString());
    QQmlJSAotVariable parent(u"QObject *"_qs, u"parent"_qs, u"nullptr"_qs);
    compiled.baselineCtor.parameterList = { parent };
    compiled.externalCtor.parameterList = { engine, parent };
    QQmlJSAotVariable ctxtdata(u"const QQmlRefPointer<QQmlContextData> &"_qs, u"parentContext"_qs,
                               QString());
    QQmlJSAotVariable finalizeFlag(u"bool"_qs, u"canFinalize"_qs, QString());
    QQmlJSAotVariable callSpecialMethodFlag(u"bool"_qs, u"callSpecialMethodNow"_qs, QString());
    if (documentRoot) {
        compiled.init.parameterList = { engine, ctxtdata, finalizeFlag, callSpecialMethodFlag };
        compiled.endInit.parameterList = { engine, finalizeFlag };
        compiled.completeComponent.parameterList = { callSpecialMethodFlag };
        compiled.finalizeComponent.parameterList = { callSpecialMethodFlag };
    } else {
        compiled.init.parameterList = { engine, ctxtdata };
        compiled.endInit.parameterList = { engine, CodeGeneratorUtility::compilationUnitVariable };
    }

    if (!documentRoot) {
        // make document root a friend to allow protected member function access
        Q_ASSERT(m_objects[0].type);
        compiled.otherCode << u"friend class %1;"_qs.arg(m_objects[0].type->internalName());
        // additionally, befriend the immediate parent of this type
        if (auto parent = m_immediateParents.value(object.type);
            parent && parent != m_objects[0].type) {
            compiled.otherCode << u"friend class %1;"_qs.arg(parent->internalName());
        }
    }

    if (baseTypeIsCompiledQml) {
        // call baseline ctor of the QML-originated base class. it also takes
        // care of QObject::setParent() call
        compiled.baselineCtor.initializerList = { baseClass + u"(parent)" };
    } else {
        // default call to ctor is enough, but QQml_setParent_noEvent() - is
        // needed (note, this is a faster version of QObject::setParent())
        compiled.baselineCtor.body << u"QQml_setParent_noEvent(this, " + parent.name + u");";
    }

    compiled.externalCtor.initializerList = { compiled.baselineCtor.name + u"(parent)" };
    if (documentRoot) {
        compiled.externalCtor.body << u"// document root:"_qs;
        compiled.endInit.body << u"auto " + CodeGeneratorUtility::compilationUnitVariable.name
                        + u" = " + u"QQmlEnginePrivate::get(engine)->compilationUnitFromUrl("
                        + m_urlMethod.name + u"());";

        // call init method of the document root
        compiled.externalCtor.body << compiled.init.name
                        + u"(engine, QQmlContextData::get(engine->rootContext()), /* finalize */ "
                          u"true, /* call special method */ true);";
    } else {
        compiled.externalCtor.body << u"// not document root:"_qs;
        compiled.externalCtor.body
                << compiled.init.name + u"(engine, QQmlData::get(parent)->outerContext);";
    }

    compiled.init.body << u"Q_UNUSED(engine);"_qs;
    if (documentRoot) {
        compiled.init.body << u"Q_UNUSED(" + callSpecialMethodFlag.name + u")";
        compiled.completeComponent.body << u"Q_UNUSED(" + callSpecialMethodFlag.name + u")";
        compiled.finalizeComponent.body << u"Q_UNUSED(" + callSpecialMethodFlag.name + u")";
    }

    // compiled.init.body << u"Q_UNUSED(" + finalizeFlag.name + u");";
    compiled.init.body << u"auto context = parentContext;"_qs;
    // TODO: context hierarchy is way-over-the-top complicated already

    // -1. if the parent scope of this type has base type as compiled qml and
    // this parent scope is not a root object, we have to go one level up in the
    // context. in a nutshell:
    // * parentScope->outerContext == parentContext of this type
    // * parentScope->outerContext != context of this document
    // * parentScope->outerContext is a child of context of this document
    // > to ensure correct context, we must use context->parent() instead of
    // > parentContext
    if (QQmlJSScope::ConstPtr parent = object.type->parentScope(); parent
        && m_qmlCompiledBaseTypes.contains(parent->baseTypeName())
        && m_typeToObjectIndex[parent] != 0) {
        compiled.init.body << u"// NB: context->parent() is the context of the root "_qs;
        compiled.init.body << u"context = context->parent();"_qs;
    }

    // 0. call parent's init if necessary
    if (baseTypeIsCompiledQml) {
        compiled.init.body << u"// 0. call parent's init"_qs;
        QString lhs;
        if (documentRoot)
            lhs = u"context = "_qs;
        const QString callParserStatusSpecialMethod = buildCallSpecialMethodValue(
                documentRoot, callSpecialMethodFlag.name, hasParserStatusInterface);
        compiled.init.body << lhs + baseClass + u"::" + compiled.init.name
                        + u"(engine, context, /* finalize */ false, /* call special method */ "
                        + callParserStatusSpecialMethod + u");";

        compiled.completeComponent.body << u"// call parent's completeComponent"_qs;
        compiled.completeComponent.body << baseClass + u"::" + compiled.completeComponent.name
                        + u"(" + callParserStatusSpecialMethod + u");";

        const QString callFinalizerHookSpecialMethod = buildCallSpecialMethodValue(
                documentRoot, callSpecialMethodFlag.name, hasFinalizerHookInterface);
        compiled.finalizeComponent.body << u"// call parent's finalizeComponent"_qs;
        compiled.finalizeComponent.body << baseClass + u"::" + compiled.finalizeComponent.name
                        + u"(" + callFinalizerHookSpecialMethod + u");";

        compiled.handleOnCompleted.body << u"// call parent's Component.onCompleted handler"_qs;
        compiled.handleOnCompleted.body
                << baseClass + u"::" + compiled.handleOnCompleted.name + u"();";
    }
    // 1. create new context through QQmlCppContextRegistrator
    if (documentRoot) {
        Q_ASSERT(objectIndex == 0);
        compiled.init.body << u"// 1. create context for this type (root)"_qs;
        compiled.init.body
                << QStringLiteral(
                           "context = %1->createInternalContext(%1->compilationUnitFromUrl(%2()), "
                           "context, 0, true);")
                           .arg(u"QQmlEnginePrivate::get(engine)"_qs, m_urlMethod.name);
    } else {
        // non-root objects adopt parent context and use that one instead of
        // creating own context
        compiled.init.body << u"// 1. use current as context of this type (non-root)"_qs;
        compiled.init.body << u"// context = context;"_qs;
    }

    // TODO: optimize step 2: do we need context = parentContext? simplify
    // QQmlCppContextRegistrator::set also

    // 2.
    if (baseTypeIsCompiledQml && !documentRoot) {
    } else { // !baseTypeIsCompiledQml || documentRoot
        // set up current context
        compiled.init.body << u"// 2. normal flow, set context for this object"_qs;
        const QString enumValue = documentRoot ? u"DocumentRoot"_qs : u"OrdinaryObject"_qs;
        compiled.init.body << QStringLiteral(
                                      "%1->setInternalContext(this, context, QQmlContextData::%2);")
                                      .arg(u"QQmlEnginePrivate::get(engine)"_qs, enumValue);
        if (documentRoot)
            compiled.init.body << u"context->setContextObject(this);"_qs;
    }
    // 3. set id if it's present in the QML document
    if (!m_doc->stringAt(object.irObject->idNameIndex).isEmpty()) {
        compiled.init.body << u"// 3. set id since it exists"_qs;
        compiled.init.body << CodeGeneratorUtility::generate_setIdValue(
                                      u"context"_qs, object.irObject->id, u"this"_qs,
                                      m_doc->stringAt(object.irObject->idNameIndex))
                        + u";";
    }

    // TODO: we might want to optimize storage space when there are no object
    // bindings, but this requires deep checking (e.g. basically go over all
    // bindings and all bindings of attached/grouped properties)
    compiled.init.body << u"// create objects for object bindings in advance:"_qs;
    // TODO: support private and protected variables
    compiled.variables.emplaceBack(CodeGeneratorUtility::childrenOffsetVariable);
    compiled.init.body << CodeGeneratorUtility::childrenOffsetVariable.name
                    + u" = QObject::children().size();";

    // magic step: if the type has a QQmlParserStatus interface, we should call
    // it's method here
    if (hasParserStatusInterface) {
        const QString indent = documentRoot ? u"    "_qs : QString();

        compiled.init.body << u"// this type has QQmlParserStatus interface:"_qs;
        compiled.init.body << u"Q_ASSERT(dynamic_cast<QQmlParserStatus *>(this) != nullptr);"_qs;
        if (documentRoot)
            compiled.init.body << u"if (" + callSpecialMethodFlag.name + u")";
        compiled.init.body << indent + u"this->classBegin();";

        compiled.completeComponent.lastLines << u"// this type has QQmlParserStatus interface:"_qs;
        compiled.completeComponent.lastLines
                << u"Q_ASSERT(dynamic_cast<QQmlParserStatus *>(this) != nullptr);"_qs;
        if (documentRoot)
            compiled.completeComponent.lastLines << u"if (" + callSpecialMethodFlag.name + u")";
        compiled.completeComponent.lastLines << indent + u"this->componentComplete();";
    }

    // magic step: if the type has a QQmlFinalizerHook interface, we should call
    // it's method here
    if (hasFinalizerHookInterface) {
        QString indent;
        compiled.finalizeComponent.lastLines << u"// this type has QQmlFinalizerHook interface:"_qs;
        compiled.finalizeComponent.lastLines
                << u"Q_ASSERT(dynamic_cast<QQmlFinalizerHook *>(this) != nullptr);"_qs;
        if (documentRoot) {
            compiled.finalizeComponent.lastLines << u"if (" + callSpecialMethodFlag.name + u")";
            indent = u"    "_qs;
        }
        compiled.finalizeComponent.lastLines << indent + u"this->componentFinalized();"_qs;
    }

    // NB: step 4 - and everything below that - is done at the very end of QML
    // init and so we use lastLines. we need to make sure that objects from
    // object bindings are created (and initialized) before we start the
    // finalization: we need fully-set context at the beginning of QML finalize.

    // 4. finalize if necessary
    if (documentRoot) {
        compiled.init.lastLines << u"// 4. document root, call finalize"_qs;
        compiled.init.lastLines << u"if (" + finalizeFlag.name + u") {";
        compiled.init.lastLines << u"    " + compiled.endInit.name
                        + u"(engine, /* finalize */ true);";
        compiled.init.lastLines << u"}"_qs;
    }
    // return
    compiled.init.lastLines << u"return context;"_qs;

    // TODO: is property update group needed?
    compiled.endInit.body << u"Q_UNUSED(engine);"_qs;
    compiled.endInit.body << u"Q_UNUSED(" + CodeGeneratorUtility::compilationUnitVariable.name
                    + u")"_qs;
    // compiled.endInit.body << u"Q_UNUSED(" + finalizeFlag.name + u");";
    if (baseTypeIsCompiledQml) {
        compiled.endInit.body << u"{ // call parent's finalize"_qs;
        compiled.endInit.body << baseClass + u"::" + compiled.endInit.name
                        + u"(engine, /* finalize */ false);";
        compiled.endInit.body << u"}"_qs;
    }

    if (object.irObject->flags & QV4::CompiledData::Object::HasDeferredBindings) {
        compiled.endInit.body << u"{ // defer bindings"_qs;
        compiled.endInit.body << u"auto ddata = QQmlData::get(this);"_qs;
        compiled.endInit.body << u"auto thisContext = ddata->outerContext;"_qs;
        compiled.endInit.body << u"Q_ASSERT(thisContext);"_qs;
        compiled.endInit.body << u"ddata->deferData(" + QString::number(objectIndex) + u", "
                        + CodeGeneratorUtility::compilationUnitVariable.name + u", thisContext);";
        compiled.endInit.body << u"}"_qs;
    }

    // TODO: decide whether begin/end property update group is needed
    // compiled.endInit.body << u"Qt::beginPropertyUpdateGroup(); // defer binding evaluation"_qs;

    // add basic MOC stuff
    compiled.mocCode = {
        u"Q_OBJECT"_qs,
        (m_isAnonymous ? u"QML_ANONYMOUS"_qs : u"QML_ELEMENT"_qs),
    };

    compileElements(compiled, object);

    // add finalization steps only to document root
    if (documentRoot) {
        compiled.endInit.body << u"if (" + finalizeFlag.name + u") {";

        // at this point, all bindings must've been finished, thus, we need:
        // 1. componentComplete()
        // 2. finalize callbacks / componentFinalized()
        // 3. Component.onCompleted()

        // 1.
        compiled.endInit.body << u"    this->" + compiled.completeComponent.name
                        + u"(/* complete component */ true);";

        // 2
        compiled.endInit.body << u"    this->" + compiled.finalizeComponent.name
                        + u"(/* finalize component */ true);";

        // 3.
        compiled.endInit.body << u"    this->" + compiled.handleOnCompleted.name + u"();";

        compiled.endInit.body << u"}"_qs;
    }

    // compiled.endInit.body << u"Qt::endPropertyUpdateGroup();"_qs;
}

void CodeGenerator::compileObjectElements(QQmlJSAotObject &compiled, const CodeGenObject &object)
{
    // compile enums
    const auto enums = object.type->ownEnumerations();
    compiled.enums.reserve(enums.size());
    for (auto it = enums.cbegin(); it != enums.cend(); ++it)
        compileEnum(compiled, it.value());

    // compile properties in order
    auto properties = object.type->ownProperties().values();
    compiled.variables.reserve(properties.size());
    std::sort(properties.begin(), properties.end(),
              [](const QQmlJSMetaProperty &x, const QQmlJSMetaProperty &y) {
                  return x.index() < y.index();
              });
    for (const QQmlJSMetaProperty &p : properties) {
        if (p.index() == -1) {
            recordError(object.type->sourceLocation(),
                        u"Property '" + p.propertyName()
                                + u"' has incomplete information (internal error)");
            continue;
        }
        if (p.isAlias()) {
            compileAlias(compiled, p, object.type);
        } else { // normal property
            compileProperty(compiled, p, object.type);
        }
    }

    // compile methods
    QHash<QString, const QmlIR::Function *> irFunctionsByName;
    std::for_each(object.irObject->functionsBegin(), object.irObject->functionsEnd(),
                  [&](const QmlIR::Function &function) {
                      irFunctionsByName.insert(m_doc->stringAt(function.nameIndex),
                                               std::addressof(function));
                  });
    const auto methods = object.type->ownMethods();
    compiled.functions.reserve(methods.size());
    for (auto it = methods.cbegin(); it != methods.cend(); ++it) {
        const QmlIR::Function *irFunction = irFunctionsByName.value(it.key(), nullptr);
        compileMethod(compiled, it.value(), irFunction, object);
    }

    // NB: just clearing is safe since we do not call this function recursively
    m_localChildrenToEndInit.clear();
    m_localChildrenToFinalize.clear();

    // compile bindings
    const auto sortedBindings =
            toOrderedSequence(object.irObject->bindingsBegin(), object.irObject->bindingsEnd(),
                              object.irObject->bindingCount());

    // for (auto it : sortedBindings)
    //     compileBinding(compiled, *it, object, u"this"_qs);

    // NB: can't use lower_bound since it only accepts a value, not a unary
    // predicate
    auto scriptBindingsBegin =
            std::find_if(sortedBindings.cbegin(), sortedBindings.cend(),
                         [](auto it) { return it->type() == QmlIR::Binding::Type_Script; });
    auto it = sortedBindings.cbegin();
    for (; it != scriptBindingsBegin; ++it)
        compileBinding(compiled, **it, object, { object.type, u"this"_qs, u""_qs, false });

    // NB: finalize children before creating/setting script bindings for `this`
    for (qsizetype i = 0; i < m_localChildrenToEndInit.size(); ++i) {
        compiled.endInit.body << m_localChildrenToEndInit.at(i) + u"->" + compiled.endInit.name
                        + u"(engine, " + CodeGeneratorUtility::compilationUnitVariable.name + u");";
    }

    const auto buildChildAtString = [](const QQmlJSScope::ConstPtr &type,
                                       const QString &i) -> QString {
        return u"static_cast<" + type->internalName() + u"* >(QObject::children().at("
                + CodeGeneratorUtility::childrenOffsetVariable.name + u" + " + i + u"))";
    };
    // TODO: there's exceptional redundancy (!) in this code generation part
    for (qsizetype i = 0; i < m_localChildrenToFinalize.size(); ++i) {
        QString index = QString::number(i);
        const QQmlJSScope::ConstPtr &child = m_localChildrenToFinalize.at(i);
        const QString childAt = buildChildAtString(child, index);
        // NB: children are not document roots, so all special methods are argument-less
        compiled.completeComponent.body
                << childAt + u"->" + compiled.completeComponent.name + u"();";
        compiled.finalizeComponent.body
                << childAt + u"->" + compiled.finalizeComponent.name + u"();";
        compiled.handleOnCompleted.body
                << childAt + u"->" + compiled.handleOnCompleted.name + u"();";
    }

    for (; it != sortedBindings.cend(); ++it)
        compileBinding(compiled, **it, object, { object.type, u"this"_qs, u""_qs, false });
}

void CodeGenerator::compileQQmlComponentElements(QQmlJSAotObject &compiled,
                                                 const CodeGenObject &object)
{
    Q_UNUSED(object);

    // since we create a document root as QQmlComponent, we only need to fake
    // QQmlComponent construction in init:
    compiled.init.body << u"// populate QQmlComponent bits"_qs;
    compiled.init.body << u"{"_qs;
    // we already called QQmlComponent(parent) constructor. now we need:
    // 1. QQmlComponent(engine, parent) logic:
    compiled.init.body << u"// QQmlComponent(engine, parent):"_qs;
    compiled.init.body << u"auto d = QQmlComponentPrivate::get(this);"_qs;
    compiled.init.body << u"Q_ASSERT(d);"_qs;
    compiled.init.body << u"d->engine = engine;"_qs;
    compiled.init.body << u"QObject::connect(engine, &QObject::destroyed, this, [d]() {"_qs;
    compiled.init.body << u"    d->state.creator.reset();"_qs;
    compiled.init.body << u"    d->engine = nullptr;"_qs;
    compiled.init.body << u"});"_qs;
    // 2. QQmlComponent(engine, compilationUnit, start, parent) logic:
    compiled.init.body << u"// QQmlComponent(engine, compilationUnit, start, parent):"_qs;
    compiled.init.body
            << u"auto compilationUnit = QQmlEnginePrivate::get(engine)->compilationUnitFromUrl("
                    + m_urlMethod.name + u"());";
    compiled.init.body << u"d->compilationUnit = compilationUnit;"_qs;
    compiled.init.body << u"d->start = 0;"_qs;
    compiled.init.body << u"d->url = compilationUnit->finalUrl();"_qs;
    compiled.init.body << u"d->progress = 1.0;"_qs;
    // 3. QQmlObjectCreator::createComponent() logic which is left:
    compiled.init.body << u"// QQmlObjectCreator::createComponent():"_qs;
    compiled.init.body << u"d->creationContext = context;"_qs;
    compiled.init.body << u"Q_ASSERT(QQmlData::get(this, /*create*/ false));"_qs;
    compiled.init.body << u"}"_qs;
}

void CodeGenerator::compileEnum(QQmlJSAotObject &current, const QQmlJSMetaEnum &e)
{
    const auto intValues = e.values();
    QStringList values;
    values.reserve(intValues.size());
    std::transform(intValues.cbegin(), intValues.cend(), std::back_inserter(values),
                   [](int x) { return QString::number(x); });

    // structure: (C++ type name, enum keys, enum values, MOC line)
    current.enums.emplaceBack(e.name(), e.keys(), std::move(values),
                              u"Q_ENUM(%1)"_qs.arg(e.name()));
}

void CodeGenerator::compileProperty(QQmlJSAotObject &current, const QQmlJSMetaProperty &p,
                                    const QQmlJSScope::ConstPtr &owner)
{
    Q_ASSERT(!p.isAlias()); // will be handled separately
    Q_ASSERT(p.type());

    const QString name = p.propertyName();
    const QString variableName = u"m_" + name;
    const QString underlyingType = getUnderlyingType(p);
    // only check for isList() here as it needs some special arrangements.
    // otherwise, getUnderlyingType() handles the specifics of a type in C++
    if (p.isList()) {
        const QString storageName = variableName + u"_storage";
        current.variables.emplaceBack(u"QList<" + p.type()->internalName() + u" *>", storageName,
                                      QString());
        current.baselineCtor.initializerList.emplaceBack(variableName + u"(" + underlyingType
                                                         + u"(this, std::addressof(" + storageName
                                                         + u")))");
    }

    // along with property, also add relevant moc code, so that we can use the
    // property in Qt/QML contexts
    QStringList mocPieces;
    mocPieces.reserve(10);
    mocPieces << underlyingType << name;

    Qml2CppPropertyData compilationData(p);

    // 1. add setter and getter
    // If p.isList(), it's a QQmlListProperty. Then you can write the underlying list through
    // the QQmlListProperty object retrieved with the getter. Setting it would make no sense.
    if (p.isWritable() && !p.isList()) {
        QQmlJSAotMethod setter {};
        setter.returnType = u"void"_qs;
        setter.name = compilationData.write;
        // QQmlJSAotVariable
        setter.parameterList.emplaceBack(QQmlJSUtils::constRefify(underlyingType), name + u"_",
                                         u""_qs);
        setter.body << variableName + u".setValue(" + name + u"_);";
        setter.body << u"emit " + compilationData.notify + u"();";
        setter.userVisible = true;
        current.functions.emplaceBack(setter);
        mocPieces << u"WRITE"_qs << setter.name;
    }

    QQmlJSAotMethod getter {};
    getter.returnType = underlyingType;
    getter.name = compilationData.read;
    getter.body << u"return " + variableName + u".value();";
    getter.userVisible = true;
    current.functions.emplaceBack(getter);
    mocPieces << u"READ"_qs << getter.name;

    // 2. add bindable
    if (!p.isList()) {
        QQmlJSAotMethod bindable {};
        bindable.returnType = u"QBindable<" + underlyingType + u">";
        bindable.name = compilationData.bindable;
        bindable.body << u"return QBindable<" + underlyingType + u">(std::addressof(" + variableName
                        + u"));";
        bindable.userVisible = true;
        current.functions.emplaceBack(bindable);
        mocPieces << u"BINDABLE"_qs << bindable.name;
    }

    // 3. add/check notify (actually, this is already done inside QmltcVisitor)

    if (owner->isPropertyRequired(name))
        mocPieces << u"REQUIRED"_qs;

    // 4. add moc entry
    // e.g. Q_PROPERTY(QString p READ getP WRITE setP BINDABLE bindableP)
    current.mocCode << u"Q_PROPERTY(" + mocPieces.join(u" "_qs) + u")";

    // 5. add extra moc entry if this property is marked default
    if (name == owner->defaultPropertyName())
        current.mocCode << u"Q_CLASSINFO(\"DefaultProperty\", \"%1\")"_qs.arg(name);

    // structure: (C++ type name, name, C++ class name, C++ signal name)
    current.properties.emplaceBack(underlyingType, variableName, current.cppType,
                                   compilationData.notify);
}

void CodeGenerator::compileAlias(QQmlJSAotObject &current, const QQmlJSMetaProperty &alias,
                                 const QQmlJSScope::ConstPtr &owner)
{
    const QString aliasName = alias.propertyName();
    Q_ASSERT(!aliasName.isEmpty());

    QStringList aliasExprBits = alias.aliasExpression().split(u'.');
    Q_ASSERT(!aliasExprBits.isEmpty());
    QString idString = aliasExprBits.front();
    Q_ASSERT(!idString.isEmpty());
    QQmlJSScope::ConstPtr type = m_localTypeResolver->scopeForId(idString, owner);
    Q_ASSERT(type);
    const bool aliasToProperty = aliasExprBits.size() > 1; // and not an object

    QString latestAccessor = u"this"_qs;
    QString latestAccessorNonPrivate; // TODO: crutch for notify
    QStringList prologue;
    QStringList writeSpecificEpilogue;

    if (owner != type) { // cannot start at `this`, need to fetch object through context at run time
        Q_ASSERT(m_typeToObjectIndex.contains(type)
                 && m_typeToObjectIndex[type] <= m_objects.size());
        const int id = m_objects[m_typeToObjectIndex[type]].irObject->id;
        Q_ASSERT(id >= 0); // since the type is found by id, it must have an id

        // TODO: the context fetching must be done once in the notify code
        // (across multiple alias properties). now there's a huge duplication
        prologue << u"auto context = QQmlData::get(this)->outerContext;"_qs;
        // there's a special case: when `this` type has compiled QML type as a base
        // type and it is not a root, it has a non-root first context, so we need to
        // step one level up
        if (m_qmlCompiledBaseTypes.contains(owner->baseTypeName())
            && m_typeToObjectIndex[owner] != 0) {
            Q_ASSERT(!owner->baseTypeName().isEmpty());
            prologue << u"// `this` is special: not a root and its base type is compiled"_qs;
            prologue << u"context = context->parent().data();"_qs;
        }
        // doing the above allows us to lookup id object by integer, which is fast
        latestAccessor = u"alias_objectById_" + idString; // unique enough
        if (aliasToProperty) {
            prologue << u"auto " + latestAccessor + u" = static_cast<" + type->internalName()
                            + u"*>(context->idValue(" + QString::number(id) + u"));";
        } else {
            // kind of a crutch for object aliases: expect that the user knows
            // what to do and so accept QObject*
            prologue << u"QObject *" + latestAccessor + u" = context->idValue("
                            + QString::number(id) + u");";
        }
        prologue << u"Q_ASSERT(" + latestAccessor + u");";
    }

    struct AliasData
    {
        QString underlyingType;
        QString readLine;
        QString writeLine;
        QString bindableLine;
    } info; // note, we only really need this crutch to cover the alias to object case
    QQmlJSMetaProperty resultingProperty;

    if (aliasToProperty) {
        // the following code goes to prologue section. the idea of that is to
        // construct the necessary code to fetch the resulting property
        for (qsizetype i = 1; i < aliasExprBits.size() - 1; ++i) {
            const QString &bit = qAsConst(aliasExprBits)[i];
            Q_ASSERT(!bit.isEmpty());
            Q_ASSERT(type);
            QQmlJSMetaProperty p = type->property(bit);

            QString currAccessor =
                    CodeGeneratorUtility::generate_getPrivateClass(latestAccessor, p);
            if (p.type()->accessSemantics() == QQmlJSScope::AccessSemantics::Value) {
                // we need to read the property to a local variable and then
                // write the updated value once the actual operation is done
                QString localVariableName = u"alias_" + bit; // should be fairly unique
                prologue << u"auto " + localVariableName + u" = " + currAccessor + u"->" + p.read()
                                + u"();";
                writeSpecificEpilogue
                        << currAccessor + u"->" + p.write() + u"(" + localVariableName + u");";
                // NB: since accessor becomes a value type, wrap it into an
                // addressof operator so that we could access it as a pointer
                currAccessor = CodeGeneratorUtility::generate_addressof(localVariableName); // reset
            } else {
                currAccessor += u"->" + p.read() + u"()"; // amend
            }

            type = p.type();
            latestAccessor = currAccessor;
        }

        resultingProperty = type->property(aliasExprBits.back());
        latestAccessorNonPrivate = latestAccessor;
        latestAccessor =
                CodeGeneratorUtility::generate_getPrivateClass(latestAccessor, resultingProperty);
        info.underlyingType = resultingProperty.type()->internalName();
        if (resultingProperty.isList()) {
            info.underlyingType = u"QQmlListProperty<" + info.underlyingType + u">";
        } else if (resultingProperty.type()->isReferenceType()) {
            info.underlyingType += u"*"_qs;
        }
        // reset to generic type when having alias to id:
        if (m_aliasesToIds.contains(resultingProperty))
            info.underlyingType = u"QObject*"_qs;

        // READ must always be present
        info.readLine = latestAccessor + u"->" + resultingProperty.read() + u"()";
        if (QString setName = resultingProperty.write(); !setName.isEmpty())
            info.writeLine = latestAccessor + u"->" + setName + u"(%1)";
        if (QString bindableName = resultingProperty.bindable(); !bindableName.isEmpty())
            info.bindableLine = latestAccessor + u"->" + bindableName + u"()";
    } else {
        info.underlyingType = u"QObject*"_qs;
        info.readLine = latestAccessor;
        // NB: id-pointing aliases are read-only, also alias to object is not
        // bindable since it's not a QProperty itself
    }

    QStringList mocLines;
    mocLines.reserve(10);
    mocLines << info.underlyingType << aliasName;

    Qml2CppPropertyData compilationData(aliasName);
    // 1. add setter and getter
    if (!info.readLine.isEmpty()) {
        QQmlJSAotMethod getter {};
        getter.returnType = info.underlyingType;
        getter.name = compilationData.read;
        getter.body += prologue;
        getter.body << u"return " + info.readLine + u";";
        // getter.body += writeSpecificEpilogue;
        getter.userVisible = true;
        current.functions.emplaceBack(getter);
        mocLines << u"READ"_qs << getter.name;
    } // else always an error?

    if (!info.writeLine.isEmpty()) {
        QQmlJSAotMethod setter {};
        setter.returnType = u"void"_qs;
        setter.name = compilationData.write;

        QList<QQmlJSMetaMethod> methods = type->methods(resultingProperty.write());
        if (methods.isEmpty()) {
            // QQmlJSAotVariable
            setter.parameterList.emplaceBack(QQmlJSUtils::constRefify(info.underlyingType),
                                             aliasName + u"_", u""_qs);
        } else {
            setter.parameterList = compileMethodParameters(methods.at(0).parameterNames(),
                                                           methods.at(0).parameterTypes(), true);
        }

        setter.body += prologue;
        if (aliasToProperty) {
            QStringList parameterNames;
            parameterNames.reserve(setter.parameterList.size());
            std::transform(setter.parameterList.cbegin(), setter.parameterList.cend(),
                           std::back_inserter(parameterNames),
                           [](const QQmlJSAotVariable &x) { return x.name; });
            QString commaSeparatedParameterNames = parameterNames.join(u", "_qs);
            setter.body << info.writeLine.arg(commaSeparatedParameterNames) + u";";
        } else {
            setter.body << info.writeLine + u";";
        }
        setter.body += writeSpecificEpilogue;
        setter.userVisible = true;
        current.functions.emplaceBack(setter);
        mocLines << u"WRITE"_qs << setter.name;
    }

    // 2. add bindable
    if (!info.bindableLine.isEmpty()) {
        QQmlJSAotMethod bindable {};
        bindable.returnType = u"QBindable<" + info.underlyingType + u">";
        bindable.name = compilationData.bindable;
        bindable.body += prologue;
        bindable.body << u"return " + info.bindableLine + u";";
        bindable.userVisible = true;
        current.functions.emplaceBack(bindable);
        mocLines << u"BINDABLE"_qs << bindable.name;
    }
    // 3. add notify - which is pretty special
    if (QString notifyName = resultingProperty.notify(); !notifyName.isEmpty()) {
        // notify is very special (even more than reasonable)
        current.endInit.body << u"{ // alias notify connection:"_qs;
        // TODO: part of the prologue (e.g. QQmlData::get(this)->outerContext)
        // must be shared across different notifies (to speed up finalize)
        current.endInit.body += prologue;
        // TODO: use non-private accessor since signals must exist on the public
        // type, not on the private one -- otherwise, they can't be used from
        // C++ which is not what's wanted (this is a mess)
        current.endInit.body << u"QObject::connect(" + latestAccessorNonPrivate + u", &"
                        + type->internalName() + u"::" + notifyName + u", this, &" + current.cppType
                        + u"::" + compilationData.notify + u");";
        current.endInit.body << u"}"_qs;
    }

    // 4. add moc entry
    // Q_PROPERTY(QString text READ text WRITE setText BINDABLE bindableText NOTIFY textChanged)
    current.mocCode << u"Q_PROPERTY(" + mocLines.join(u" "_qs) + u")";

    // 5. add extra moc entry if this alias is default one
    if (aliasName == owner->defaultPropertyName()) {
        // Q_CLASSINFO("DefaultProperty", propertyName)
        current.mocCode << u"Q_CLASSINFO(\"DefaultProperty\", \"%1\")"_qs.arg(aliasName);
    }
}

void CodeGenerator::compileMethod(QQmlJSAotObject &current, const QQmlJSMetaMethod &m,
                                  const QmlIR::Function *f, const CodeGenObject &object)
{
    Q_UNUSED(object);
    const QString returnType = figureReturnType(m);

    const auto paramNames = m.parameterNames();
    const auto paramTypes = m.parameterTypes();
    Q_ASSERT(paramNames.size() == paramTypes.size());
    const QList<QQmlJSAotVariable> paramList = compileMethodParameters(paramNames, paramTypes);

    const auto methodType = QQmlJSMetaMethod::Type(m.methodType());

    QStringList code;
    // the function body code only makes sense if the method is not a signal -
    // and there is a corresponding QmlIR::Function
    if (f) {
        Q_ASSERT(methodType != QQmlJSMetaMethod::Signal);
        code += CodeGeneratorUtility::generate_callExecuteRuntimeFunction(
                m_urlMethod.name + u"()",
                relativeToAbsoluteRuntimeIndex(m_doc, object.irObject, f->index), u"this"_qs,
                returnType, paramList);
    }

    QQmlJSAotMethod compiled {};
    compiled.returnType = returnType;
    compiled.name = m.methodName();
    compiled.parameterList = std::move(paramList);
    compiled.body = std::move(code);
    compiled.type = methodType;
    compiled.access = m.access();
    if (methodType != QQmlJSMetaMethod::Signal) {
        compiled.declPreambles << u"Q_INVOKABLE"_qs; // TODO: do we need this for signals as well?
        compiled.userVisible = m.access() == QQmlJSMetaMethod::Public;
    } else {
        compiled.userVisible = !m.isImplicitQmlPropertyChangeSignal();
    }
    current.functions.emplaceBack(compiled);
}

template<typename Iterator>
static QString getPropertyOrAliasNameFromIr(const QmlIR::Document *doc, Iterator first, int pos)
{
    std::advance(first, pos); // assume that first is correct - due to earlier check
    return doc->stringAt(first->nameIndex);
}

void CodeGenerator::compileBinding(QQmlJSAotObject &current, const QmlIR::Binding &binding,
                                   const CodeGenObject &object,
                                   const CodeGenerator::AccessorData &accessor)
{
    // Note: unlike QQmlObjectCreator, we don't have to do a complicated
    // deferral logic for bindings: if a binding is deferred, it is not compiled
    // (potentially, with all the bindings inside of it), period.
    if (binding.flags() & QV4::CompiledData::Binding::IsDeferredBinding) {
        if (binding.type() == QmlIR::Binding::Type_GroupProperty) {
            // TODO: we should warn about this in QmlCompiler library
            qCWarning(lcCodeGenerator)
                    << QStringLiteral("Binding at line %1 column %2 is not deferred as it is a "
                                      "binding on a group property.")
                               .arg(QString::number(binding.location.line()),
                                    QString::number(binding.location.column()));
            // we do not support PropertyChanges and other types with similar
            // behavior yet, so this binding is compiled
        } else {
            qCDebug(lcCodeGenerator)
                    << QStringLiteral(
                               "Binding at line %1 column %2 is deferred and thus not compiled")
                               .arg(QString::number(binding.location.line()),
                                    QString::number(binding.location.column()));
            return;
        }
    }

    // TODO: cache property name somehow, so we don't need to look it up again
    QString propertyName = m_doc->stringAt(binding.propertyNameIndex);
    if (propertyName.isEmpty()) {
        // if empty, try default property
        for (QQmlJSScope::ConstPtr t = object.type->baseType(); t && propertyName.isEmpty();
             t = t->baseType())
            propertyName = t->defaultPropertyName();
    }
    Q_ASSERT(!propertyName.isEmpty());
    QQmlJSMetaProperty p = object.type->property(propertyName);
    QQmlJSScope::ConstPtr propertyType = p.type();
    // Q_ASSERT(propertyType); // TODO: doesn't work with signals

    const auto addPropertyLine = [&](const QString &propertyName, const QQmlJSMetaProperty &p,
                                     const QString &value, bool constructFromQObject = false) {
        // TODO: there mustn't be this special case. instead, alias resolution
        // must be done in QQmlJSImportVisitor subclass, that would handle this
        // mess (see resolveValidateOrSkipAlias() in qml2cppdefaultpasses.cpp)
        if (p.isAlias() && m_qmlCompiledBaseTypes.contains(object.type->baseTypeName())) {
            qCDebug(lcCodeGenerator) << u"Property '" + propertyName + u"' is an alias on type '"
                            + object.type->internalName()
                            + u"' which is a QML type compiled to C++. The assignment is special "
                              u"in this case";
            current.endInit.body += CodeGeneratorUtility::generate_assignToSpecialAlias(
                    object.type, propertyName, p, value, accessor.name, constructFromQObject);
        } else {
            current.endInit.body += CodeGeneratorUtility::generate_assignToProperty(
                    object.type, propertyName, p, value, accessor.name, constructFromQObject);
        }
    };

    switch (binding.type()) {
    case QmlIR::Binding::Type_Boolean: {
        addPropertyLine(propertyName, p, binding.value.b ? u"true"_qs : u"false"_qs);
        break;
    }
    case QmlIR::Binding::Type_Number: {
        QString value = m_doc->javaScriptCompilationUnit.bindingValueAsString(&binding);
        addPropertyLine(propertyName, p, value);
        break;
    }
    case QmlIR::Binding::Type_String: {
        const QString str = m_doc->stringAt(binding.stringIndex);
        addPropertyLine(propertyName, p, QQmlJSUtils::toLiteral(str));
        break;
    }
    case QmlIR::Binding::Type_TranslationById: { // TODO: add test
        const QV4::CompiledData::TranslationData &translation =
                m_doc->javaScriptCompilationUnit.unitData()
                        ->translations()[binding.value.translationDataIndex];
        const QString id = m_doc->stringAt(binding.stringIndex);
        addPropertyLine(propertyName, p,
                        u"qsTrId(\"" + id + u"\", " + QString::number(translation.number) + u")");
        break;
    }
    case QmlIR::Binding::Type_Translation: { // TODO: add test
        const QV4::CompiledData::TranslationData &translation =
                m_doc->javaScriptCompilationUnit.unitData()
                        ->translations()[binding.value.translationDataIndex];
        int lastSlash = m_url.lastIndexOf(QChar(u'/'));
        const QStringView context = (lastSlash > -1)
                ? QStringView { m_url }.mid(lastSlash + 1, m_url.length() - lastSlash - 5)
                : QStringView();
        const QString comment = m_doc->stringAt(translation.commentIndex);
        const QString text = m_doc->stringAt(translation.stringIndex);

        addPropertyLine(propertyName, p,
                        u"QCoreApplication::translate(\"" + context + u"\", \"" + text + u"\", \""
                                + comment + u"\", " + QString::number(translation.number) + u")");
        break;
    }
    case QmlIR::Binding::Type_Script: {
        QString bindingSymbolName = object.type->internalName() + u'_' + propertyName + u"_binding";
        bindingSymbolName.replace(u'.', u'_'); // can happen with group properties
        compileScriptBinding(current, binding, bindingSymbolName, object, propertyName,
                             propertyType, accessor);
        break;
    }
    case QmlIR::Binding::Type_Object: {
        // NB: object is compiled with compileObject, here just need to use it
        const CodeGenObject &bindingObject = m_objects.at(binding.value.objectIndex);

        // Note: despite a binding being set for `accessor`, we use "this" as a
        // parent of a created object. Both attached and grouped properties are
        // parented by "this", so lifetime-wise we should be fine. If not, we're
        // in a fairly deep trouble, actually, since it's essential to use
        // `this` (at least at present it seems to be so) - e.g. the whole logic
        // of separating object binding into object creation and object binding
        // setup relies on the fact that we use `this`
        const QString qobjectParent = u"this"_qs;
        const QString objectAddr = accessor.name;

        if (binding.hasFlag(QmlIR::Binding::IsOnAssignment)) {
            const QString onAssignmentName = u"onAssign_" + propertyName;
            const auto uniqueId = UniqueStringId(current, onAssignmentName);
            if (!m_onAssignmentObjectsCreated.contains(uniqueId)) {
                m_onAssignmentObjectsCreated.insert(uniqueId);
                current.init.body << u"new " + bindingObject.type->internalName() + u"(engine, "
                                + qobjectParent + u");";

                // static_cast is fine, because we (must) know the exact type
                current.endInit.body << u"auto " + onAssignmentName + u" = static_cast<"
                                + bindingObject.type->internalName()
                                + u" *>(QObject::children().at("
                                + CodeGeneratorUtility::childrenOffsetVariable.name + u" + "
                                + QString::number(m_localChildrenToEndInit.size()) + u"));";

                m_localChildrenToEndInit.append(onAssignmentName);
                m_localChildrenToFinalize.append(bindingObject.type);
            }

            // NB: we expect one "on" assignment per property, so creating
            // QQmlProperty each time should be fine (unlike QQmlListReference)
            current.endInit.body << u"{"_qs;
            current.endInit.body << u"QQmlProperty qmlprop(" + objectAddr + u", QStringLiteral(\""
                            + propertyName + u"\"));";
            current.endInit.body << u"QT_PREPEND_NAMESPACE(QQmlCppOnAssignmentHelper)::set("
                            + onAssignmentName + u", qmlprop);";
            current.endInit.body << u"}"_qs;
            break;
        }

        if (!propertyType) {
            recordError(binding.location,
                        u"Binding on property '" + propertyName + u"' of unknown type");
            return;
        }

        if (p.isList() || binding.hasFlag(QmlIR::Binding::IsListItem)) {
            const QString refName = u"listref_" + propertyName;
            const auto uniqueId = UniqueStringId(current, refName);
            if (!m_listReferencesCreated.contains(uniqueId)) {
                m_listReferencesCreated.insert(uniqueId);
                // TODO: figure if Unicode support is needed here
                current.endInit.body << u"QQmlListReference " + refName + u"(" + objectAddr + u", "
                                + QQmlJSUtils::toLiteral(propertyName, u"QByteArrayLiteral")
                                + u");";
                current.endInit.body << u"Q_ASSERT(" + refName + u".canAppend());";
            }
        }

        const auto setObjectBinding = [&](const QString &value) {
            if (p.isList() || binding.hasFlag(QmlIR::Binding::IsListItem)) {
                const QString refName = u"listref_" + propertyName;
                current.endInit.body << refName + u".append(" + value + u");";
            } else {
                addPropertyLine(propertyName, p, value, /* fromQObject = */ true);
            }
        };

        if (bindingObject.irObject->flags & QV4::CompiledData::Object::IsComponent) {
            // NB: this one is special - and probably becomes a view delegate.
            // object binding separation also does not apply here
            const QString objectName = makeGensym(u"sc"_qs);
            Q_ASSERT(m_typeToObjectIndex.contains(bindingObject.type));
            const int objectIndex = int(m_typeToObjectIndex[bindingObject.type]);
            Q_ASSERT(m_componentIndices.contains(objectIndex));
            const int index = m_componentIndices[objectIndex];
            current.endInit.body << u"{"_qs;
            current.endInit.body << QStringLiteral(
                                            "auto thisContext = QQmlData::get(%1)->outerContext;")
                                            .arg(qobjectParent);
            current.endInit.body << QStringLiteral(
                                            "auto %1 = QQmlObjectCreator::createComponent(engine, "
                                            "%2, %3, %4, thisContext);")
                                            .arg(objectName,
                                                 CodeGeneratorUtility::compilationUnitVariable.name,
                                                 QString::number(index), qobjectParent);
            current.endInit.body << QStringLiteral("thisContext->installContext(QQmlData::get(%1), "
                                                   "QQmlContextData::OrdinaryObject);")
                                            .arg(objectName);
            const auto isComponentBased = [](const QQmlJSScope::ConstPtr &type) {
                auto base = QQmlJSScope::nonCompositeBaseType(type);
                return base && base->internalName() == u"QQmlComponent"_qs;
            };
            if (!m_doc->stringAt(bindingObject.irObject->idNameIndex).isEmpty()
                && isComponentBased(bindingObject.type)) {
                current.endInit.body
                        << CodeGeneratorUtility::generate_setIdValue(
                                   u"thisContext"_qs, bindingObject.irObject->id, objectName,
                                   m_doc->stringAt(bindingObject.irObject->idNameIndex))
                                + u";";
            }
            setObjectBinding(objectName);
            current.endInit.body << u"}"_qs;
            break;
        }

        current.init.body << u"new " + bindingObject.type->internalName() + u"(engine, "
                        + qobjectParent + u");";

        const QString objectName = makeGensym(u"o"_qs);

        // static_cast is fine, because we (must) know the exact type
        current.endInit.body << u"auto " + objectName + u" = static_cast<"
                        + bindingObject.type->internalName() + u" *>(QObject::children().at("
                        + CodeGeneratorUtility::childrenOffsetVariable.name + u" + "
                        + QString::number(m_localChildrenToEndInit.size()) + u"));";
        setObjectBinding(objectName);

        m_localChildrenToEndInit.append(objectName);
        m_localChildrenToFinalize.append(bindingObject.type);
        break;
    }
    case QmlIR::Binding::Type_AttachedProperty: {
        Q_ASSERT(accessor.name == u"this"_qs); // TODO: doesn't have to hold, in fact
        const auto attachedObject = m_objects.at(binding.value.objectIndex);
        const auto &irObject = attachedObject.irObject;
        const auto &type = attachedObject.type;
        Q_ASSERT(irObject && type);
        if (propertyName == u"Component"_qs) {
            // TODO: there's a special QQmlComponentAttached, which has to be
            // called? c.f. qqmlobjectcreator.cpp's finalize()
            const auto compileComponent = [&](const QmlIR::Binding &b) {
                Q_ASSERT(b.type() == QmlIR::Binding::Type_Script);
                compileScriptBindingOfComponent(current, irObject, type, b,
                                                m_doc->stringAt(b.propertyNameIndex));
            };
            std::for_each(irObject->bindingsBegin(), irObject->bindingsEnd(), compileComponent);
        } else {
            const QString attachingTypeName = propertyName; // acts as an identifier
            auto attachingType = m_localTypeResolver->typeForName(attachingTypeName);
            Q_ASSERT(attachingType); // an error somewhere else

            QString attachedTypeName = type->attachedTypeName(); // TODO: check if == internalName?
            if (attachedTypeName.isEmpty()) // TODO: shouldn't happen ideally
                attachedTypeName = type->baseTypeName();
            const QString attachedMemberName = u"m_" + attachingTypeName;
            const auto uniqueId = UniqueStringId(current, attachedMemberName);
            // add attached type as a member variable to allow noop lookup
            if (!m_attachedTypesAlreadyRegistered.contains(uniqueId)) {
                m_attachedTypesAlreadyRegistered.insert(uniqueId);
                current.variables.emplaceBack(attachedTypeName + u" *", attachedMemberName,
                                              u"nullptr"_qs);
                // Note: getting attached property is fairly expensive
                const QString getAttachedPropertyLine = u"qobject_cast<" + attachedTypeName
                        + u" *>(qmlAttachedPropertiesObject<" + attachingType->internalName()
                        + u">(this, /* create = */ true))";
                current.endInit.body
                        << attachedMemberName + u" = " + getAttachedPropertyLine + u";";
            }

            // compile bindings of the attached property
            auto sortedBindings = toOrderedSequence(
                    irObject->bindingsBegin(), irObject->bindingsEnd(), irObject->bindingCount());
            for (auto it : qAsConst(sortedBindings)) {
                compileBinding(current, *it, attachedObject,
                               { object.type, attachedMemberName, propertyName, false });
            }
        }
        break;
    }
    case QmlIR::Binding::Type_GroupProperty: {
        Q_ASSERT(accessor.name == u"this"_qs); // TODO: doesn't have to hold, in fact
        if (p.read().isEmpty()) {
            recordError(binding.location,
                        u"READ function of group property '" + propertyName + u"' is unknown");
            return;
        }

        const auto groupedObject = m_objects.at(binding.value.objectIndex);
        const auto &irObject = groupedObject.irObject;
        const auto &type = groupedObject.type;
        Q_ASSERT(irObject && type);

        const bool isValueType =
                propertyType->accessSemantics() == QQmlJSScope::AccessSemantics::Value;
        if (!isValueType
            && propertyType->accessSemantics() != QQmlJSScope::AccessSemantics::Reference) {
            recordError(binding.location,
                        u"Group property '" + propertyName + u"' has unsupported access semantics");
            return;
        }

        QString groupAccessor = CodeGeneratorUtility::generate_getPrivateClass(accessor.name, p)
                + u"->" + p.read() + u"()";
        // NB: used when isValueType == true
        QString groupPropertyVarName = accessor.name + u"_group_" + propertyName;

        const auto isGroupAffectingBinding = [](const QmlIR::Binding &b) {
            // script bindings do not require value type group property variable
            // to actually be present.
            switch (b.type()) {
            case QmlIR::Binding::Type_Invalid:
            case QmlIR::Binding::Type_Script:
                return false;
            default:
                return true;
            }
        };
        const bool generateValueTypeCode = isValueType
                && std::any_of(irObject->bindingsBegin(), irObject->bindingsEnd(),
                               isGroupAffectingBinding);

        // value types are special
        if (generateValueTypeCode) {
            if (p.write().isEmpty()) { // just reject this
                recordError(binding.location,
                            u"Group property '" + propertyName
                                    + u"' is a value type without a setter");
                return;
            }

            current.endInit.body << u"auto " + groupPropertyVarName + u" = " + groupAccessor + u";";
            // TODO: addressof operator is a crutch to make the binding logic
            // work, which expects that `accessor.name` is a pointer type.
            groupAccessor = CodeGeneratorUtility::generate_addressof(groupPropertyVarName);
        }

        // compile bindings of the grouped property
        auto sortedBindings = toOrderedSequence(irObject->bindingsBegin(), irObject->bindingsEnd(),
                                                irObject->bindingCount());
        const auto compile = [&](typename QmlIR::PoolList<QmlIR::Binding>::Iterator it) {
            compileBinding(current, *it, groupedObject,
                           { object.type, groupAccessor, propertyName, isValueType });
        };

        // NB: can't use lower_bound since it only accepts a value, not a unary
        // predicate
        auto scriptBindingsBegin =
                std::find_if(sortedBindings.cbegin(), sortedBindings.cend(),
                             [](auto it) { return it->type() == QmlIR::Binding::Type_Script; });
        auto it = sortedBindings.cbegin();
        for (; it != scriptBindingsBegin; ++it) {
            Q_ASSERT((*it)->type() != QmlIR::Binding::Type_Script);
            compile(*it);
        }

        // NB: script bindings are special on group properties. if our group is
        // a value type, the binding would be installed on the *object* that
        // holds the value type and not on the value type itself. this may cause
        // subtle side issues (esp. when script binding is actually a simple
        // enum value assignment - which is not recognized specially):
        //
        // auto valueTypeGroupProperty = getCopy();
        // installBinding(valueTypeGroupProperty, "subproperty1"); // changes subproperty1 value
        // setCopy(valueTypeGroupProperty); // oops, subproperty1 value changed to old again
        if (generateValueTypeCode) { // write the value type back
            current.endInit.body << CodeGeneratorUtility::generate_getPrivateClass(accessor.name, p)
                            + u"->" + p.write() + u"(" + groupPropertyVarName + u");";
        }

        // once the value is written back, process the script bindings
        for (; it != sortedBindings.cend(); ++it) {
            Q_ASSERT((*it)->type() == QmlIR::Binding::Type_Script);
            compile(*it);
        }
        break;
    }
    case QmlIR::Binding::Type_Null: {
        // poor check: null bindings are only supported for var and objects
        if (propertyType != m_localTypeResolver->varType()
            && propertyType->accessSemantics() != QQmlJSScope::AccessSemantics::Reference) {
            // TODO: this should really be done before the compiler here
            recordError(binding.location, u"Cannot assign null to incompatible property"_qs);
        } else if (propertyType->accessSemantics() == QQmlJSScope::AccessSemantics::Reference) {
            addPropertyLine(p.propertyName(), p, u"nullptr"_qs);
        } else {
            addPropertyLine(p.propertyName(), p, u"QVariant::fromValue(nullptr)"_qs);
        }
        break;
    }
    case QmlIR::Binding::Type_Invalid: {
        recordError(binding.valueLocation,
                    u"Binding on property '" + propertyName + u"' is invalid or unsupported");
        break;
    }
    }
}

QString CodeGenerator::makeGensym(const QString &base)
{
    QString gensym = base;
    gensym.replace(QLatin1String("."), QLatin1String("_"));
    while (gensym.startsWith(QLatin1Char('_')) && gensym.size() >= 2
           && (gensym[1].isUpper() || gensym[1] == QLatin1Char('_'))) {
        gensym.remove(0, 1);
    }

    if (!m_typeCounts.contains(gensym)) {
        m_typeCounts.insert(gensym, 1);
    } else {
        gensym += u"_" + QString::number(m_typeCounts[gensym]++);
    }

    return gensym;
}

// returns compiled script binding for "property changed" handler in a form of object type
static QQmlJSAotObject compileScriptBindingPropertyChangeHandler(
        const QmlIR::Document *doc, const QmlIR::Binding &binding, const QmlIR::Object *irObject,
        const QQmlJSAotMethod &urlMethod, const QString &functorCppType,
        const QString &objectCppType, const QList<QQmlJSAotVariable> &slotParameters)
{
    QQmlJSAotObject bindingFunctor {};
    bindingFunctor.cppType = functorCppType;
    bindingFunctor.ignoreInit = true;

    // default member variable and ctor:
    const QString pointerToObject = objectCppType + u" *";
    bindingFunctor.variables.emplaceBack(
            QQmlJSAotVariable { pointerToObject, u"m_self"_qs, u"nullptr"_qs });
    bindingFunctor.baselineCtor.name = functorCppType;
    bindingFunctor.baselineCtor.parameterList.emplaceBack(
            QQmlJSAotVariable { pointerToObject, u"self"_qs, QString() });
    bindingFunctor.baselineCtor.initializerList.emplaceBack(u"m_self(self)"_qs);

    // call operator:
    QQmlJSAotMethod callOperator {};
    callOperator.returnType = u"void"_qs;
    callOperator.name = u"operator()"_qs;
    callOperator.parameterList = slotParameters;
    callOperator.modifiers << u"const"_qs;
    callOperator.body += CodeGeneratorUtility::generate_callExecuteRuntimeFunction(
            urlMethod.name + u"()",
            relativeToAbsoluteRuntimeIndex(doc, irObject, binding.value.compiledScriptIndex),
            u"m_self"_qs, u"void"_qs, slotParameters);

    bindingFunctor.functions.emplaceBack(std::move(callOperator));

    return bindingFunctor;
}

static std::optional<QQmlJSMetaProperty>
propertyForChangeHandler(const QQmlJSScope::ConstPtr &scope, QString name)
{
    if (!name.endsWith(QLatin1String("Changed")))
        return {};
    constexpr int length = int(sizeof("Changed") / sizeof(char)) - 1;
    name.chop(length);
    auto p = scope->property(name);
    const bool isBindable = !p.bindable().isEmpty();
    const bool canNotify = !p.notify().isEmpty();
    if (p.isValid() && (isBindable || canNotify))
        return p;
    return {};
}

void CodeGenerator::compileScriptBinding(QQmlJSAotObject &current, const QmlIR::Binding &binding,
                                         const QString &bindingSymbolName,
                                         const CodeGenObject &object, const QString &propertyName,
                                         const QQmlJSScope::ConstPtr &propertyType,
                                         const CodeGenerator::AccessorData &accessor)
{
    auto objectType = object.type;

    // returns signal by name. signal exists if std::optional<> has value
    const auto signalByName = [&](const QString &name) -> std::optional<QQmlJSMetaMethod> {
        const auto signalMethods = objectType->methods(name, QQmlJSMetaMethod::Signal);
        if (signalMethods.isEmpty())
            return {};
        // if (signalMethods.size() != 1) return {}; // error somewhere else
        QQmlJSMetaMethod s = signalMethods.at(0);
        Q_ASSERT(s.methodType() == QQmlJSMetaMethod::Signal);
        return s;
    };

    const auto resolveSignal = [&](const QString &name) -> std::optional<QQmlJSMetaMethod> {
        auto signal = signalByName(name);
        if (signal) // found signal
            return signal;
        auto property = propertyForChangeHandler(objectType, name);
        if (!property) // nor signal nor property change handler
            return {}; // error somewhere else
        if (auto notify = property->notify(); !notify.isEmpty())
            return signalByName(notify);
        return {};
    };

    // TODO: add Invalid case which would reject garbage handlers
    enum BindingKind {
        JustProperty, // is a binding on property
        SignalHandler, // is a slot related to some signal
        BindablePropertyChangeHandler, // is a slot related to property's bindable
    };

    // these only make sense when binding is on signal handler
    QList<QQmlJSAotVariable> slotParameters;
    QString signalName;
    QString signalReturnType;

    const BindingKind kind = [&]() -> BindingKind {
        if (!QmlIR::IRBuilder::isSignalPropertyName(propertyName))
            return BindingKind::JustProperty;

        if (auto name = QQmlJSUtils::signalName(propertyName); name.has_value())
            signalName = *name;

        std::optional<QQmlJSMetaMethod> possibleSignal = resolveSignal(signalName);
        if (possibleSignal) { // signal with signalName exists
            QQmlJSMetaMethod s = possibleSignal.value();

            // Note: update signal name since it may be different from the one
            // used to resolve a signal
            signalName = s.methodName();

            const auto paramNames = s.parameterNames();
            const auto paramTypes = s.parameterTypes();
            Q_ASSERT(paramNames.size() == paramTypes.size());
            slotParameters = compileMethodParameters(paramNames, paramTypes, true);
            signalReturnType = figureReturnType(s);
            return BindingKind::SignalHandler;
        } else if (propertyName.endsWith(u"Changed"_qs)) {
            return BindingKind::BindablePropertyChangeHandler;
        }
        return BindingKind::JustProperty;
    }();

    switch (kind) {
    case BindingKind::JustProperty: {
        if (!propertyType) {
            recordError(binding.location,
                        u"Binding on property '" + propertyName + u"' of unknown type");
            return;
        }

        auto [property, absoluteIndex] = getMetaPropertyIndex(object.type, propertyName);
        if (absoluteIndex < 0) {
            recordError(binding.location, u"Binding on unknown property '" + propertyName + u"'");
            return;
        }

        QString bindingTarget = accessor.name;

        int valueTypeIndex = -1;
        if (accessor.isValueType) {
            Q_ASSERT(accessor.scope != object.type);
            bindingTarget = u"this"_qs; // TODO: not necessarily "this"?
            auto [groupProperty, groupPropertyIndex] =
                    getMetaPropertyIndex(accessor.scope, accessor.propertyName);
            if (groupPropertyIndex < 0) {
                recordError(binding.location,
                            u"Binding on group property '" + accessor.propertyName
                                    + u"' of unknown type");
                return;
            }
            valueTypeIndex = absoluteIndex;
            absoluteIndex = groupPropertyIndex; // e.g. index of accessor.name
        }

        current.endInit.body += CodeGeneratorUtility::generate_createBindingOnProperty(
                CodeGeneratorUtility::compilationUnitVariable.name,
                u"this"_qs, // NB: always using enclosing object as a scope for the binding
                relativeToAbsoluteRuntimeIndex(object.irObject, binding.value.compiledScriptIndex),
                bindingTarget, // binding target
                absoluteIndex, property, valueTypeIndex, accessor.name);
        break;
    }
    case BindingKind::SignalHandler: {
        QString This_signal = u"this"_qs;
        QString This_slot = u"this"_qs;
        QString objectClassName_signal = objectType->internalName();
        QString objectClassName_slot = objectType->internalName();
        // TODO: ugly crutch to make stuff work
        if (accessor.name != u"this"_qs) { // e.g. if attached property
            This_signal = accessor.name;
            This_slot = u"this"_qs; // still
            objectClassName_signal = objectType->baseTypeName();
            objectClassName_slot = current.cppType; // real base class where slot would go
        }
        Q_ASSERT(!objectClassName_signal.isEmpty());
        Q_ASSERT(!objectClassName_slot.isEmpty());
        const QString slotName = makeGensym(signalName + u"_slot");

        // SignalHander specific:
        QQmlJSAotMethod slotMethod {};
        slotMethod.returnType = signalReturnType;
        slotMethod.name = slotName;
        slotMethod.parameterList = slotParameters;

        slotMethod.body += CodeGeneratorUtility::generate_callExecuteRuntimeFunction(
                m_urlMethod.name + u"()",
                relativeToAbsoluteRuntimeIndex(m_doc, object.irObject,
                                               binding.value.compiledScriptIndex),
                u"this"_qs, // Note: because script bindings always use current QML object scope
                signalReturnType, slotParameters);
        slotMethod.type = QQmlJSMetaMethod::Slot;

        current.functions << std::move(slotMethod);
        current.endInit.body << u"QObject::connect(" + This_signal + u", "
                        + CodeGeneratorUtility::generate_qOverload(slotParameters,
                                                                   u"&" + objectClassName_signal
                                                                           + u"::" + signalName)
                        + u", " + This_slot + u", &" + objectClassName_slot + u"::" + slotName
                        + u");";
        break;
    }
    case BindingKind::BindablePropertyChangeHandler: {
        const QString objectClassName = objectType->internalName();
        const QString bindingFunctorName = makeGensym(bindingSymbolName + u"Functor");

        QString actualPropertyName;
        QRegularExpression onChangedRegExp(u"on(\\w+)Changed"_qs);
        QRegularExpressionMatch match = onChangedRegExp.match(propertyName);
        if (match.hasMatch()) {
            actualPropertyName = match.captured(1);
            actualPropertyName[0] = actualPropertyName.at(0).toLower();
        } else {
            // an error somewhere else
            return;
        }
        if (!objectType->hasProperty(actualPropertyName)) {
            recordError(binding.location, u"No property named " + actualPropertyName);
            return;
        }
        QQmlJSMetaProperty actualProperty = objectType->property(actualPropertyName);
        const auto actualPropertyType = actualProperty.type();
        if (!actualPropertyType) {
            recordError(binding.location,
                        u"Binding on property '" + actualPropertyName + u"' of unknown type");
            return;
        }

        QString bindableString = actualProperty.bindable();
        if (bindableString.isEmpty()) { // TODO: always should come from prop.bindalbe()
            recordError(binding.location,
                        u"Property '" + actualPropertyName
                                + u"' is non-bindable. Change handlers for such properties are not "
                                  u"supported");
            break;
        }

        QString typeOfQmlBinding =
                u"std::unique_ptr<QPropertyChangeHandler<" + bindingFunctorName + u">>";

        current.children << compileScriptBindingPropertyChangeHandler(
                m_doc, binding, object.irObject, m_urlMethod, bindingFunctorName, objectClassName,
                slotParameters);

        // TODO: the finalize.lastLines has to be used here -- somehow if property gets a binding,
        // the change handler is not updated?!

        // TODO: this could be dropped if QQmlEngine::setContextForObject() is
        // done before currently generated C++ object is constructed
        current.endInit.body << bindingSymbolName + u".reset(new QPropertyChangeHandler<"
                        + bindingFunctorName + u">("
                        + CodeGeneratorUtility::generate_getPrivateClass(accessor.name,
                                                                         actualProperty)
                        + u"->" + bindableString + u"().onValueChanged(" + bindingFunctorName + u"("
                        + accessor.name + u"))));";

        current.variables.emplaceBack(
                QQmlJSAotVariable { typeOfQmlBinding, bindingSymbolName, QString() });
        // current.ctor.initializerList << bindingSymbolName + u"()";
        break;
    }
    }
}

// TODO: should use "compileScriptBinding" instead of custom code
void CodeGenerator::compileScriptBindingOfComponent(QQmlJSAotObject &current,
                                                    const QmlIR::Object *irObject,
                                                    const QQmlJSScope::ConstPtr objectType,
                                                    const QmlIR::Binding &binding,
                                                    const QString &propertyName)
{
    // returns signal by name. signal exists if std::optional<> has value
    const auto signalByName = [&](const QString &name) -> std::optional<QQmlJSMetaMethod> {
        const QList<QQmlJSMetaMethod> signalMethods = objectType->methods(name);
        // TODO: no clue how to handle redefinition, so just record an error
        if (signalMethods.size() != 1) {
            recordError(binding.location, u"Binding on redefined signal '" + name + u"'");
            return {};
        }
        QQmlJSMetaMethod s = signalMethods.at(0);
        if (s.methodType() != QQmlJSMetaMethod::Signal)
            return {};
        return s;
    };

    QString signalName;
    QString signalReturnType;

    Q_ASSERT(QmlIR::IRBuilder::isSignalPropertyName(propertyName));
    const auto offset = static_cast<uint>(strlen("on"));
    signalName = propertyName[offset].toLower() + propertyName.mid(offset + 1);

    std::optional<QQmlJSMetaMethod> possibleSignal = signalByName(signalName);
    if (!possibleSignal) {
        recordError(binding.location, u"Component signal '" + signalName + u"' is not recognized");
        return;
    }
    Q_ASSERT(possibleSignal); // Component's script binding is always a signal handler
    QQmlJSMetaMethod s = possibleSignal.value();
    Q_ASSERT(s.parameterNames().isEmpty()); // Component signals do not have parameters
    signalReturnType = figureReturnType(s);

    const QString slotName = makeGensym(signalName + u"_slot");

    // SignalHander specific:
    QQmlJSAotMethod slotMethod {};
    slotMethod.returnType = signalReturnType;
    slotMethod.name = slotName;

    // Component is special:
    int runtimeIndex =
            relativeToAbsoluteRuntimeIndex(m_doc, irObject, binding.value.compiledScriptIndex);
    slotMethod.body += CodeGeneratorUtility::generate_callExecuteRuntimeFunction(
            m_urlMethod.name + u"()", runtimeIndex, u"this"_qs, signalReturnType);
    slotMethod.type = QQmlJSMetaMethod::Slot;

    // TODO: there's actually an attached type, which has completed/destruction
    // signals that are typically emitted -- do we care enough about supporting
    // that? see QQmlComponentAttached
    if (signalName == u"completed"_qs) {
        current.handleOnCompleted.body << slotName + u"();";
    } else if (signalName == u"destruction"_qs) {
        if (!current.dtor) {
            current.dtor = QQmlJSAotSpecialMethod {};
            current.dtor->name = u"~" + current.cppType;
        }
        current.dtor->firstLines << slotName + u"();";
    }
    current.functions << std::move(slotMethod);
}

void CodeGenerator::compileUrlMethod()
{
    QFileInfo fi(m_url);
    m_urlMethod.returnType = u"const QUrl &"_qs;
    m_urlMethod.name =
            u"q_qmltc_docUrl_" + fi.fileName().replace(u".qml"_qs, u""_qs).replace(u'.', u'_');
    m_urlMethod.body << u"static QUrl docUrl = %1;"_qs.arg(
            CodeGeneratorUtility::toResourcePath(m_options.resourcePath));
    m_urlMethod.body << u"return docUrl;"_qs;
    m_urlMethod.declPreambles << u"static"_qs;
    m_urlMethod.modifiers << u"noexcept"_qs;
}

void CodeGenerator::recordError(const QQmlJS::SourceLocation &location, const QString &message)
{
    m_logger->logCritical(message, Log_Compiler, location);
}

void CodeGenerator::recordError(const QV4::CompiledData::Location &location, const QString &message)
{
    recordError(QQmlJS::SourceLocation { 0, 0, location.line(), location.column() }, message);
}
