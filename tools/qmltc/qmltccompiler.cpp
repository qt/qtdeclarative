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

#include "qmltccompiler.h"
#include "qmltcoutputir.h"
#include "qmltccodewriter.h"
#include "qmltcpropertyutils.h"
#include "qmltccompilerpieces.h"

#include "prototype/codegenerator.h"

#include <QtCore/qloggingcategory.h>
#include <private/qqmljsutils_p.h>

#include <algorithm>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

Q_LOGGING_CATEGORY(lcQmltcCompiler, "qml.qmltc.compiler", QtWarningMsg);

const QString QmltcCodeGenerator::privateEngineName = u"ePriv"_s;
const QString QmltcCodeGenerator::typeCountName = u"q_qmltc_typeCount"_s;

QmltcCompiler::QmltcCompiler(const QString &url, QmltcTypeResolver *resolver, QmltcVisitor *visitor,
                             QQmlJSLogger *logger)
    : m_url(url), m_typeResolver(resolver), m_visitor(visitor), m_logger(logger)
{
    Q_UNUSED(m_typeResolver);
    Q_ASSERT(!hasErrors());
}

// needed due to std::unique_ptr<CodeGenerator> with CodeGenerator being
// incomplete type in the header (~std::unique_ptr<> fails with a static_assert)
QmltcCompiler::~QmltcCompiler() = default;

void QmltcCompiler::compile(const QmltcCompilerInfo &info, QmlIR::Document *doc)
{
    m_info = info;
    Q_ASSERT(!m_info.outputCppFile.isEmpty());
    Q_ASSERT(!m_info.outputHFile.isEmpty());
    Q_ASSERT(!m_info.resourcePath.isEmpty());

    m_prototypeCodegen =
            std::make_unique<CodeGenerator>(m_url, m_logger, doc, m_typeResolver, m_visitor, &info);

    QSet<QString> cppIncludesFromPrototype;
    m_prototypeCodegen->prepare(&cppIncludesFromPrototype);
    if (hasErrors())
        return;

    const auto isComponent = [](const QQmlJSScope::ConstPtr &type) {
        auto base = type->baseType();
        return base && base->internalName() == u"QQmlComponent"_s;
    };

    // Note: we only compile "pure" QML types. any component-wrapped type is
    // expected to appear through a binding
    auto pureTypes = m_visitor->pureQmlTypes();
    const QSet<QQmlJSScope::ConstPtr> types(pureTypes.begin(), pureTypes.end());

    QmltcCodeGenerator generator { m_url, m_visitor };

    QmltcMethod urlMethod;
    compileUrlMethod(urlMethod, generator.urlMethodName());
    m_prototypeCodegen->setUrlMethodName(urlMethod.name);

    const auto objects = m_prototypeCodegen->objects();
    QList<CodeGenerator::CodeGenObject> filteredObjects;
    filteredObjects.reserve(objects.size());
    std::copy_if(objects.cbegin(), objects.cend(), std::back_inserter(filteredObjects),
                 [&](const auto &object) { return types.contains(object.type); });

    QList<QmltcType> compiledTypes;
    QQmlJSScope::ConstPtr root = m_visitor->result();
    if (isComponent(root)) {
        compiledTypes.reserve(1);
        compiledTypes.emplaceBack(); // create empty type
        const auto compile = [&](QmltcType &current, const QQmlJSScope::ConstPtr &type) {
            generator.generate_initCodeForTopLevelComponent(current, type);
        };
        Q_ASSERT(root == filteredObjects.at(0).type);
        compileType(compiledTypes.back(), root, compile);
    } else {
        const auto compile = [this](QmltcType &current, const QQmlJSScope::ConstPtr &type) {
            compileTypeElements(current, type);
        };

        compiledTypes.reserve(filteredObjects.size());
        for (const auto &object : filteredObjects) {
            Q_ASSERT(object.type->scopeType() == QQmlJSScope::QMLScope);
            if (m_prototypeCodegen->ignoreObject(object))
                continue;
            compiledTypes.emplaceBack(); // create empty type
            compileType(compiledTypes.back(), object.type, compile);
        }
    }
    if (hasErrors())
        return;

    QmltcProgram program;
    program.url = m_url;
    program.cppPath = m_info.outputCppFile;
    program.hPath = m_info.outputHFile;
    program.outNamespace = m_info.outputNamespace;
    program.compiledTypes = compiledTypes;
    program.includes = m_visitor->cppIncludeFiles() | cppIncludesFromPrototype;
    program.urlMethod = urlMethod;

    QmltcOutput out;
    QmltcOutputWrapper code(out);
    QmltcCodeWriter::write(code, program);
}

void QmltcCompiler::compileUrlMethod(QmltcMethod &urlMethod, const QString &urlMethodName)
{
    urlMethod.name = urlMethodName;
    urlMethod.returnType = u"const QUrl&"_s;
    urlMethod.body << u"static QUrl url {QStringLiteral(\"qrc:%1\")};"_s.arg(m_info.resourcePath);
    urlMethod.body << u"return url;"_s;
    urlMethod.declarationPrefixes << u"static"_s;
    urlMethod.modifiers << u"noexcept"_s;
}

void QmltcCompiler::compileType(
        QmltcType &current, const QQmlJSScope::ConstPtr &type,
        std::function<void(QmltcType &, const QQmlJSScope::ConstPtr &)> compileElements)
{
    if (type->isSingleton()) {
        recordError(type->sourceLocation(), u"Singleton types are not supported"_s);
        return;
    }

    Q_ASSERT(!type->internalName().isEmpty());
    current.cppType = type->internalName();
    Q_ASSERT(!type->baseType()->internalName().isEmpty());
    const QString baseClass = type->baseType()->internalName();

    const auto rootType = m_visitor->result();
    const bool documentRoot = (type == rootType);
    const bool isAnonymous = !documentRoot || type->internalName().at(0).isLower();

    QmltcCodeGenerator generator { m_url, m_visitor };

    current.baseClasses = { baseClass };
    if (!documentRoot) {
        // make document root a friend to allow it to access init and endInit
        current.otherCode << u"friend class %1;"_s.arg(rootType->internalName());

        // additionally make an immediate parent a friend since that parent
        // would create the object through a non-public constructor
        const auto realQmlScope = [](const QQmlJSScope::ConstPtr &scope) {
            if (scope->isArrayScope())
                return scope->parentScope();
            return scope;
        };
        current.otherCode << u"friend class %1;"_s.arg(
                realQmlScope(type->parentScope())->internalName());
    } else {
        // make QQmltcObjectCreationBase<DocumentRoot> a friend to allow it to
        // be created for the root object
        current.otherCode << u"friend class QQmltcObjectCreationBase<%1>;"_s.arg(
                rootType->internalName());

        QmltcMethod typeCountMethod;
        typeCountMethod.name = QmltcCodeGenerator::typeCountName;
        typeCountMethod.returnType = u"uint"_s;
        typeCountMethod.body << u"return " + generator.generate_typeCount() + u";";
        current.typeCount = typeCountMethod;
    }
    // make QQmltcObjectCreationHelper a friend of every type since it provides
    // useful helper methods for all types
    current.otherCode << u"friend class QT_PREPEND_NAMESPACE(QQmltcObjectCreationHelper);"_s;

    current.mocCode = {
        u"Q_OBJECT"_s,
        // Note: isAnonymous holds for non-root types in the document as well
        isAnonymous ? u"QML_ANONYMOUS"_s : u"QML_ELEMENT"_s,
    };

    // add special member functions
    current.baselineCtor.access = QQmlJSMetaMethod::Protected;
    if (documentRoot) {
        current.externalCtor.access = QQmlJSMetaMethod::Public;
    } else {
        current.externalCtor.access = QQmlJSMetaMethod::Protected;
    }
    current.init.access = QQmlJSMetaMethod::Protected;
    current.beginClass.access = QQmlJSMetaMethod::Protected;
    current.endInit.access = QQmlJSMetaMethod::Protected;
    current.completeComponent.access = QQmlJSMetaMethod::Protected;
    current.finalizeComponent.access = QQmlJSMetaMethod::Protected;
    current.handleOnCompleted.access = QQmlJSMetaMethod::Protected;

    current.baselineCtor.name = current.cppType;
    current.externalCtor.name = current.cppType;
    current.init.name = u"QML_init"_s;
    current.init.returnType = u"QQmlRefPointer<QQmlContextData>"_s;
    current.beginClass.name = u"QML_beginClass"_s;
    current.beginClass.returnType = u"void"_s;
    current.endInit.name = u"QML_endInit"_s;
    current.endInit.returnType = u"void"_s;
    current.completeComponent.name = u"QML_completeComponent"_s;
    current.completeComponent.returnType = u"void"_s;
    current.finalizeComponent.name = u"QML_finalizeComponent"_s;
    current.finalizeComponent.returnType = u"void"_s;
    current.handleOnCompleted.name = u"QML_handleOnCompleted"_s;
    current.handleOnCompleted.returnType = u"void"_s;
    QmltcVariable creator(u"QQmltcObjectCreationHelper*"_s, u"creator"_s);
    QmltcVariable engine(u"QQmlEngine*"_s, u"engine"_s);
    QmltcVariable parent(u"QObject*"_s, u"parent"_s, u"nullptr"_s);
    QmltcVariable ctxtdata(u"const QQmlRefPointer<QQmlContextData>&"_s, u"parentContext"_s);
    QmltcVariable finalizeFlag(u"bool"_s, u"canFinalize"_s);
    current.baselineCtor.parameterList = { parent };
    if (documentRoot) {
        current.externalCtor.parameterList = { engine, parent };
        current.init.parameterList = { creator, engine, ctxtdata, finalizeFlag };
        current.beginClass.parameterList = { creator, finalizeFlag };
        current.endInit.parameterList = { creator, engine, finalizeFlag };
        current.completeComponent.parameterList = { creator, finalizeFlag };
        current.finalizeComponent.parameterList = { creator, finalizeFlag };
        current.handleOnCompleted.parameterList = { creator, finalizeFlag };
    } else {
        current.externalCtor.parameterList = { creator, engine, parent };
        current.init.parameterList = { creator, engine, ctxtdata };
        current.beginClass.parameterList = { creator };
        current.endInit.parameterList = { creator, engine };
        current.completeComponent.parameterList = { creator };
        current.finalizeComponent.parameterList = { creator };
        current.handleOnCompleted.parameterList = { creator };
    }

    current.externalCtor.initializerList = { current.baselineCtor.name + u"(" + parent.name
                                             + u")" };
    if (QQmlJSUtils::hasCompositeBase(type)) {
        // call parent's (QML type's) basic ctor from this. that one will take
        // care about QObject::setParent()
        current.baselineCtor.initializerList = { baseClass + u"(" + parent.name + u")" };
    } else {
        // default call to ctor is enough, but QQml_setParent_noEvent() is
        // needed (note: faster? version of QObject::setParent())
        current.baselineCtor.body << u"QQml_setParent_noEvent(this, " + parent.name + u");";
    }

    // compilation stub:
    current.externalCtor.body << u"Q_UNUSED(engine);"_s;
    current.endInit.body << u"Q_UNUSED(engine);"_s;
    current.endInit.body << u"Q_UNUSED(creator);"_s;
    if (documentRoot) {
        current.externalCtor.body << u"// document root:"_s;
        // if it's document root, we want to create our QQmltcObjectCreationBase
        // that would store all the created objects
        current.externalCtor.body << u"QQmltcObjectCreationBase<%1> objectHolder;"_s.arg(
                type->internalName());
        current.externalCtor.body
                << u"QQmltcObjectCreationHelper creator = objectHolder.view();"_s;
        current.externalCtor.body << u"creator.set(0, this);"_s; // special case
        // now call init
        current.externalCtor.body << current.init.name
                        + u"(&creator, engine, QQmlContextData::get(engine->rootContext()), /* "
                          u"endInit */ true);";

        current.endInit.body << u"Q_UNUSED(canFinalize);"_s;
    } else {
        current.externalCtor.body << u"// not document root:"_s;
        // just call init, we don't do any setup here otherwise
        current.externalCtor.body << current.init.name
                        + u"(creator, engine, QQmlData::get(parent)->outerContext);";
    }

    auto postponedQmlContextSetup = generator.generate_initCode(current, type);
    auto postponedFinalizeCode = generator.generate_endInitCode(current, type);
    generator.generate_beginClassCode(current, type);
    generator.generate_completeComponentCode(current, type);
    generator.generate_finalizeComponentCode(current, type);
    generator.generate_handleOnCompletedCode(current, type);

    compileElements(current, type);
}

void QmltcCompiler::compileTypeElements(QmltcType &current, const QQmlJSScope::ConstPtr &type)
{
    const CodeGenerator::CodeGenObject &object = m_prototypeCodegen->objectFromType(type);

    // compile components of a type:
    // - enums
    // - properties
    // - methods
    // - bindings

    const auto enums = type->ownEnumerations();
    current.enums.reserve(enums.size());
    for (auto it = enums.begin(); it != enums.end(); ++it)
        compileEnum(current, it.value());

    auto properties = type->ownProperties().values();
    current.properties.reserve(properties.size());
    // Note: index() is the (future) meta property index, so make sure given
    // properties are ordered by that index before compiling
    std::sort(properties.begin(), properties.end(),
              [](const QQmlJSMetaProperty &x, const QQmlJSMetaProperty &y) {
                  return x.index() < y.index();
              });
    for (const QQmlJSMetaProperty &p : qAsConst(properties)) {
        if (p.index() == -1) {
            recordError(type->sourceLocation(),
                        u"Internal error: property '%1' has incomplete information"_s.arg(
                                p.propertyName()));
            continue;
        }
        if (p.isAlias()) {
            compileAlias(current, p, type);
        } else {
            compileProperty(current, p, type);
        }
    }

    const auto methods = type->ownMethods();
    for (const QQmlJSMetaMethod &m : methods)
        compileMethod(current, m, type);

    {
        const auto sortedBindings = CodeGenerator::toOrderedSequence(
                object.irObject->bindingsBegin(), object.irObject->bindingsEnd(),
                object.irObject->bindingCount());

        auto scriptBindingsBegin =
                std::find_if(sortedBindings.cbegin(), sortedBindings.cend(),
                             [](auto it) { return it->type == QmlIR::Binding::Type_Script; });
        auto it = sortedBindings.cbegin();
        for (; it != scriptBindingsBegin; ++it) {
            m_prototypeCodegen->compileBinding(current, **it, object,
                                               { object.type, u"this"_s, u""_s, false });
        }

        for (; it != sortedBindings.cend(); ++it) {
            m_prototypeCodegen->compileBinding(current, **it, object,
                                               { type, u"this"_s, u""_s, false });
        }
    }
}

void QmltcCompiler::compileEnum(QmltcType &current, const QQmlJSMetaEnum &e)
{
    const auto intValues = e.values();
    QStringList values;
    values.reserve(intValues.size());
    std::transform(intValues.cbegin(), intValues.cend(), std::back_inserter(values),
                   [](int x) { return QString::number(x); });

    // structure: (C++ type name, enum keys, enum values, MOC line)
    current.enums.emplaceBack(e.name(), e.keys(), std::move(values),
                              u"Q_ENUM(%1)"_s.arg(e.name()));
}

static QList<QmltcVariable>
compileMethodParameters(const QStringList &names,
                        const QList<QSharedPointer<const QQmlJSScope>> &types,
                        bool allowUnnamed = false)
{
    QList<QmltcVariable> parameters;
    const auto size = names.size();
    parameters.reserve(size);
    for (qsizetype i = 0; i < size; ++i) {
        Q_ASSERT(types[i]); // assume verified
        QString name = names[i];
        Q_ASSERT(allowUnnamed || !name.isEmpty()); // assume verified
        if (name.isEmpty() && allowUnnamed)
            name = u"unnamed_" + QString::number(i);
        parameters.emplaceBack(types[i]->augmentedInternalName(), name, QString());
    }
    return parameters;
}

void QmltcCompiler::compileMethod(QmltcType &current, const QQmlJSMetaMethod &m,
                                  const QQmlJSScope::ConstPtr &owner)
{
    const auto figureReturnType = [](const QQmlJSMetaMethod &m) {
        const bool isVoidMethod =
                m.returnTypeName() == u"void" || m.methodType() == QQmlJSMetaMethod::Signal;
        Q_ASSERT(isVoidMethod || m.returnType());
        QString type;
        if (isVoidMethod) {
            type = u"void"_s;
        } else {
            type = m.returnType()->augmentedInternalName();
        }
        return type;
    };

    const auto returnType = figureReturnType(m);
    const auto paramNames = m.parameterNames();
    const auto paramTypes = m.parameterTypes();
    Q_ASSERT(paramNames.size() == paramTypes.size()); // assume verified
    const QList<QmltcVariable> compiledParams = compileMethodParameters(paramNames, paramTypes);
    const auto methodType = QQmlJSMetaMethod::Type(m.methodType());

    QStringList code;
    if (methodType != QQmlJSMetaMethod::Signal) {
        const qsizetype index =
                static_cast<qsizetype>(owner->ownRuntimeFunctionIndex(m.jsFunctionIndex()));
        Q_ASSERT(index >= 0);
        QmltcCodeGenerator urlGenerator { m_url, m_visitor };
        QmltcCodeGenerator::generate_callExecuteRuntimeFunction(
                &code, urlGenerator.urlMethodName() + u"()", index, u"this"_s, returnType,
                compiledParams);
    }

    QmltcMethod compiled {};
    compiled.returnType = returnType;
    compiled.name = m.methodName();
    compiled.parameterList = std::move(compiledParams);
    compiled.body = std::move(code);
    compiled.type = methodType;
    compiled.access = m.access();
    if (methodType != QQmlJSMetaMethod::Signal) {
        compiled.declarationPrefixes << u"Q_INVOKABLE"_s;
        compiled.userVisible = m.access() == QQmlJSMetaMethod::Public;
    } else {
        compiled.userVisible = !m.isImplicitQmlPropertyChangeSignal();
    }
    current.functions.emplaceBack(compiled);
}

void QmltcCompiler::compileProperty(QmltcType &current, const QQmlJSMetaProperty &p,
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

    QmltcPropertyData compilationData(p);

    // 1. add setter and getter
    // If p.isList(), it's a QQmlListProperty. Then you can write the underlying list through
    // the QQmlListProperty object retrieved with the getter. Setting it would make no sense.
    if (p.isWritable() && !p.isList()) {
        QmltcMethod setter {};
        setter.returnType = u"void"_s;
        setter.name = compilationData.write;
        // QmltcVariable
        setter.parameterList.emplaceBack(QQmlJSUtils::constRefify(underlyingType), name + u"_",
                                         u""_s);
        setter.body << variableName + u".setValue(" + name + u"_);";
        setter.body << u"Q_EMIT " + compilationData.notify + u"();";
        setter.userVisible = true;
        current.functions.emplaceBack(setter);
        mocPieces << u"WRITE"_s << setter.name;
    }

    QmltcMethod getter {};
    getter.returnType = underlyingType;
    getter.name = compilationData.read;
    getter.body << u"return " + variableName + u".value();";
    getter.userVisible = true;
    current.functions.emplaceBack(getter);
    mocPieces << u"READ"_s << getter.name;

    // 2. add bindable
    if (!p.isList()) {
        QmltcMethod bindable {};
        bindable.returnType = u"QBindable<" + underlyingType + u">";
        bindable.name = compilationData.bindable;
        bindable.body << u"return QBindable<" + underlyingType + u">(std::addressof(" + variableName
                        + u"));";
        bindable.userVisible = true;
        current.functions.emplaceBack(bindable);
        mocPieces << u"BINDABLE"_s << bindable.name;
    }

    // 3. add/check notify (actually, this is already done inside QmltcVisitor)

    if (owner->isPropertyRequired(name))
        mocPieces << u"REQUIRED"_s;

    // 4. add moc entry
    // e.g. Q_PROPERTY(QString p READ getP WRITE setP BINDABLE bindableP)
    current.mocCode << u"Q_PROPERTY(" + mocPieces.join(u" "_s) + u")";

    // 5. add extra moc entry if this property is marked default
    if (name == owner->defaultPropertyName())
        current.mocCode << u"Q_CLASSINFO(\"DefaultProperty\", \"%1\")"_s.arg(name);

    // structure: (C++ type name, name, C++ class name, C++ signal name)
    current.properties.emplaceBack(underlyingType, variableName, current.cppType,
                                   compilationData.notify);
}

struct AliasResolutionFrame
{
    static QString inVar;
    QStringList prologue;
    QStringList epilogueForWrite;
    QString outVar;
};
// special string replaced by outVar of the previous frame
QString AliasResolutionFrame::inVar = QStringLiteral("__QMLTC_ALIAS_FRAME_INPUT_VAR__");

static void unpackFrames(QStack<AliasResolutionFrame> &frames)
{
    if (frames.size() < 2)
        return;

    // assume first frame is fine
    auto prev = frames.begin();
    for (auto it = std::next(prev); it != frames.end(); ++it, ++prev) {
        for (QString &line : it->prologue)
            line.replace(AliasResolutionFrame::inVar, prev->outVar);
        for (QString &line : it->epilogueForWrite)
            line.replace(AliasResolutionFrame::inVar, prev->outVar);
        it->outVar.replace(AliasResolutionFrame::inVar, prev->outVar);
    }
}

template<typename Projection>
static QStringList joinFrames(const QStack<AliasResolutionFrame> &frames, Projection project)
{
    QStringList joined;
    for (const AliasResolutionFrame &frame : frames)
        joined += project(frame);
    return joined;
}

void QmltcCompiler::compileAlias(QmltcType &current, const QQmlJSMetaProperty &alias,
                                 const QQmlJSScope::ConstPtr &owner)
{
    const QString aliasName = alias.propertyName();
    Q_ASSERT(!aliasName.isEmpty());

    QStringList aliasExprBits = alias.aliasExpression().split(u'.');
    Q_ASSERT(!aliasExprBits.isEmpty());

    QStack<AliasResolutionFrame> frames;

    QQmlJSUtils::AliasResolutionVisitor aliasVisitor;
    qsizetype i = 0;
    aliasVisitor.reset = [&]() {
        frames.clear();
        i = 0; // we use it in property processing

        // first frame is a dummy one:
        frames.push(AliasResolutionFrame { QStringList(), QStringList(), u"this"_s });
    };
    aliasVisitor.processResolvedId = [&](const QQmlJSScope::ConstPtr &type) {
        Q_ASSERT(type);
        if (owner != type) { // cannot start at `this`, need to fetch object through context
            const int id = m_visitor->runtimeId(type);
            Q_ASSERT(id >= 0); // since the type is found by id, it must have an id

            AliasResolutionFrame queryIdFrame {};
            queryIdFrame.prologue << u"auto context = QQmlData::get(%1)->outerContext;"_s.arg(
                    AliasResolutionFrame::inVar);
            // there's a special case: when `this` type has compiled QML type as
            // a base type and it is not a root, it has a non-root first
            // context, so we need to step one level up
            if (QQmlJSUtils::hasCompositeBase(owner) && owner != m_visitor->result()) {
                Q_ASSERT(!owner->baseTypeName().isEmpty());
                queryIdFrame.prologue
                        << u"// `this` is special: not a root and its base type is compiled"_s;
                queryIdFrame.prologue << u"context = context->parent().data();"_s;
            }

            // doing the above allows us to lookup id object by index (fast)
            queryIdFrame.outVar = u"alias_objectById_" + aliasExprBits.front(); // unique enough
            queryIdFrame.prologue << u"auto " + queryIdFrame.outVar + u" = static_cast<"
                            + type->internalName() + u"*>(context->idValue(" + QString::number(id)
                            + u"));";
            queryIdFrame.prologue << u"Q_ASSERT(" + queryIdFrame.outVar + u");";

            frames.push(queryIdFrame);
        }
    };
    aliasVisitor.processResolvedProperty = [&](const QQmlJSMetaProperty &p,
                                               const QQmlJSScope::ConstPtr &) {
        AliasResolutionFrame queryPropertyFrame {};

        QString inVar = QmltcCodeGenerator::wrap_privateClass(AliasResolutionFrame::inVar, p);
        if (p.type()->accessSemantics() == QQmlJSScope::AccessSemantics::Value) {
            // we need to read the property to a local variable and then
            // write the updated value once the actual operation is done
            const QString aliasVar = u"alias_" + QString::number(i); // should be fairly unique
            ++i;
            queryPropertyFrame.prologue
                    << u"auto " + aliasVar + u" = " + inVar + u"->" + p.read() + u"();";
            queryPropertyFrame.epilogueForWrite
                    << inVar + u"->" + p.write() + u"(" + aliasVar + u");";
            // NB: since accessor becomes a value type, wrap it into an
            // addressof operator so that we could access it as a pointer
            inVar = QmltcCodeGenerator::wrap_addressof(aliasVar); // reset
        } else {
            inVar += u"->" + p.read() + u"()"; // update
        }
        queryPropertyFrame.outVar = inVar;

        frames.push(queryPropertyFrame);
    };

    QQmlJSUtils::ResolvedAlias result =
            QQmlJSUtils::resolveAlias(m_typeResolver, alias, owner, aliasVisitor);
    Q_ASSERT(result.kind != QQmlJSUtils::AliasTarget_Invalid);

    unpackFrames(frames);

    if (result.kind == QQmlJSUtils::AliasTarget_Property) {
        // we don't need the last frame here
        frames.pop();

        // instead, add a custom frame
        AliasResolutionFrame customFinalFrame {};
        customFinalFrame.outVar =
                QmltcCodeGenerator::wrap_privateClass(frames.top().outVar, result.property);
        frames.push(customFinalFrame);
    }

    const QString latestAccessor = frames.top().outVar;
    const QStringList prologue =
            joinFrames(frames, [](const AliasResolutionFrame &frame) { return frame.prologue; });
    const QString underlyingType = (result.kind == QQmlJSUtils::AliasTarget_Property)
            ? getUnderlyingType(result.property)
            : result.owner->internalName() + u" *";

    QStringList mocLines;
    mocLines.reserve(10);
    mocLines << underlyingType << aliasName;

    QmltcPropertyData compilationData(aliasName);
    // 1. add setter and getter
    QmltcMethod getter {};
    getter.returnType = underlyingType;
    getter.name = compilationData.read;
    getter.body += prologue;
    if (result.kind == QQmlJSUtils::AliasTarget_Property)
        getter.body << u"return " + latestAccessor + u"->" + result.property.read() + u"();";
    else // AliasTarget_Object
        getter.body << u"return " + latestAccessor + u";";
    getter.userVisible = true;
    current.functions.emplaceBack(getter);
    mocLines << u"READ"_s << getter.name;

    if (QString setName = result.property.write(); !setName.isEmpty()) {
        Q_ASSERT(result.kind == QQmlJSUtils::AliasTarget_Property); // property is invalid otherwise
        QmltcMethod setter {};
        setter.returnType = u"void"_s;
        setter.name = compilationData.write;

        QList<QQmlJSMetaMethod> methods = result.owner->methods(setName);
        if (methods.isEmpty()) { // when we are compiling the property as well
            // QmltcVariable
            setter.parameterList.emplaceBack(QQmlJSUtils::constRefify(underlyingType),
                                             aliasName + u"_", u""_s);
        } else {
            setter.parameterList = compileMethodParameters(methods.at(0).parameterNames(),
                                                           methods.at(0).parameterTypes(),
                                                           /* allow unnamed = */ true);
        }

        setter.body += prologue;
        QStringList parameterNames;
        parameterNames.reserve(setter.parameterList.size());
        std::transform(setter.parameterList.cbegin(), setter.parameterList.cend(),
                       std::back_inserter(parameterNames),
                       [](const QmltcVariable &x) { return x.name; });
        QString commaSeparatedParameterNames = parameterNames.join(u", "_s);
        setter.body << latestAccessor + u"->" + setName
                        + u"(%1)"_s.arg(commaSeparatedParameterNames) + u";";
        setter.body += joinFrames(
                frames, [](const AliasResolutionFrame &frame) { return frame.epilogueForWrite; });
        setter.userVisible = true;
        current.functions.emplaceBack(setter);
        mocLines << u"WRITE"_s << setter.name;
    }
    // 2. add bindable
    if (QString bindableName = result.property.bindable(); !bindableName.isEmpty()) {
        Q_ASSERT(result.kind == QQmlJSUtils::AliasTarget_Property); // property is invalid otherwise
        QmltcMethod bindable {};
        bindable.returnType = u"QBindable<" + underlyingType + u">";
        bindable.name = compilationData.bindable;
        bindable.body += prologue;
        bindable.body << u"return " + latestAccessor + u"->" + bindableName + u"()" + u";";
        bindable.userVisible = true;
        current.functions.emplaceBack(bindable);
        mocLines << u"BINDABLE"_s << bindable.name;
    }
    // 3. add notify - which is pretty special
    if (QString notifyName = result.property.notify(); !notifyName.isEmpty()) {
        Q_ASSERT(result.kind == QQmlJSUtils::AliasTarget_Property); // property is invalid otherwise

        // notify is very special
        current.endInit.body << u"{ // alias notify connection:"_s;
        current.endInit.body += prologue;
        // TODO: use non-private accessor since signals must exist on the public
        // type, not on the private one -- otherwise, you can't connect to a
        // private property signal in C++ and so it is useless (hence, use
        // public type)
        const QString latestAccessorNonPrivate = frames[frames.size() - 2].outVar;
        current.endInit.body << u"QObject::connect(" + latestAccessorNonPrivate + u", &"
                        + result.owner->internalName() + u"::" + notifyName + u", this, &"
                        + current.cppType + u"::" + compilationData.notify + u");";
        current.endInit.body << u"}"_s;
    }

    // 4. add moc entry
    // Q_PROPERTY(QString text READ text WRITE setText BINDABLE bindableText NOTIFY textChanged)
    current.mocCode << u"Q_PROPERTY(" + mocLines.join(u" "_s) + u")";

    // 5. add extra moc entry if this alias is default one
    if (aliasName == owner->defaultPropertyName()) {
        // Q_CLASSINFO("DefaultProperty", propertyName)
        current.mocCode << u"Q_CLASSINFO(\"DefaultProperty\", \"%1\")"_s.arg(aliasName);
    }
}

void QmltcCompiler::compileBinding(QmltcType &current, const QQmlJSMetaPropertyBinding &binding,
                                   const QQmlJSScope::ConstPtr &type,
                                   const BindingAccessorData &accessor)
{
    Q_UNUSED(current);
    Q_UNUSED(accessor);
    QString propertyName = binding.propertyName();
    if (propertyName.isEmpty()) {
        // if empty, try default property
        for (QQmlJSScope::ConstPtr t = type->baseType(); t && propertyName.isEmpty();
             t = t->baseType()) {
            propertyName = t->defaultPropertyName();
        }
    }
    Q_ASSERT(!propertyName.isEmpty());
    QQmlJSMetaProperty p = type->property(propertyName);
    Q_ASSERT(p.isValid());
    QQmlJSScope::ConstPtr propertyType = p.type();
    Q_ASSERT(propertyType);

    // NB: we assume here that QmltcVisitor took care of type mismatches and
    // other errors, so the compiler just needs to add correct instructions,
    // without if-checking every type

    QmltcCodeGenerator generator {};

    switch (binding.bindingType()) {
    case QQmlJSMetaPropertyBinding::BoolLiteral: {
        const bool value = binding.boolValue();
        generator.generate_assignToProperty(&current.init.body, type, p,
                                            value ? u"true"_s : u"false"_s, accessor.name);
        break;
    }
    case QQmlJSMetaPropertyBinding::NumberLiteral: {
        const QString value = QString::number(binding.numberValue());
        generator.generate_assignToProperty(&current.init.body, type, p, value, accessor.name);
        break;
    }
    case QQmlJSMetaPropertyBinding::StringLiteral: {
        const QString value = binding.stringValue();
        generator.generate_assignToProperty(&current.init.body, type, p,
                                            QQmlJSUtils::toLiteral(value), accessor.name);
        break;
    }
    case QQmlJSMetaPropertyBinding::Null: {
        // poor check: null bindings are only supported for var and objects
        if (propertyType != m_typeResolver->varType()
            && propertyType->accessSemantics() != QQmlJSScope::AccessSemantics::Reference) {
            // TODO: this should really be done before the compiler here
            recordError(binding.sourceLocation(),
                        u"Cannot assign null to incompatible property"_s);
        } else if (propertyType->accessSemantics() == QQmlJSScope::AccessSemantics::Reference) {
            generator.generate_assignToProperty(&current.init.body, type, p, u"nullptr"_s,
                                                accessor.name);
        } else {
            generator.generate_assignToProperty(&current.init.body, type, p,
                                                u"QVariant::fromValue(nullptr)"_s, accessor.name);
        }
        break;
    }
    // case QQmlJSMetaPropertyBinding::RegExpLiteral:
    // case QQmlJSMetaPropertyBinding::Translation:
    // case QQmlJSMetaPropertyBinding::TranslationById:
    // case QQmlJSMetaPropertyBinding::Script:
    // case QQmlJSMetaPropertyBinding::Object:
    // case QQmlJSMetaPropertyBinding::Interceptor:
    // case QQmlJSMetaPropertyBinding::ValueSource:
    // case QQmlJSMetaPropertyBinding::AttachedProperty:
    // case QQmlJSMetaPropertyBinding::GroupProperty:
    case QQmlJSMetaPropertyBinding::Invalid: {
        m_logger->log(u"This binding is invalid"_s, Log_Compiler, binding.sourceLocation());
        break;
    }
    default: {
        m_logger->log(u"Binding type is not supported (yet)"_s, Log_Compiler,
                      binding.sourceLocation());
        break;
    }
    }
}

QT_END_NAMESPACE
