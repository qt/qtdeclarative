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

Q_LOGGING_CATEGORY(lcQmltcCompiler, "qml.qmltc.compiler", QtWarningMsg);

const QString QmltcCodeGenerator::privateEngineName = u"ePriv"_qs;
const QString QmltcCodeGenerator::urlMethodName = u"q_qmltc_docUrl"_qs;
const QString QmltcCodeGenerator::typeCountName = u"q_qmltc_typeCount"_qs;

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
        return base && base->internalName() == u"QQmlComponent"_qs;
    };

    // Note: we only compile "pure" QML types. any component-wrapped type is
    // expected to appear through a binding
    auto pureTypes = m_visitor->pureQmlTypes();
    const QSet<QQmlJSScope::ConstPtr> types(pureTypes.begin(), pureTypes.end());

    QmltcMethod urlMethod;
    compileUrlMethod(urlMethod);
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
        const auto compile = [](QmltcType &current, const QQmlJSScope::ConstPtr &type) {
            QmltcCodeGenerator::generate_initCodeForTopLevelComponent(current, type);
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

void QmltcCompiler::compileUrlMethod(QmltcMethod &urlMethod)
{
    urlMethod.name = QmltcCodeGenerator::urlMethodName;
    urlMethod.returnType = u"const QUrl&"_qs;
    urlMethod.body << u"static QUrl url {QStringLiteral(\"qrc:%1\")};"_qs.arg(m_info.resourcePath);
    urlMethod.body << u"return url;"_qs;
    urlMethod.declarationPrefixes << u"static"_qs;
    urlMethod.modifiers << u"noexcept"_qs;
}

void QmltcCompiler::compileType(
        QmltcType &current, const QQmlJSScope::ConstPtr &type,
        std::function<void(QmltcType &, const QQmlJSScope::ConstPtr &)> compileElements)
{
    if (type->isSingleton()) {
        recordError(type->sourceLocation(), u"Singleton types are not supported"_qs);
        return;
    }

    Q_ASSERT(!type->internalName().isEmpty());
    current.cppType = type->internalName();
    Q_ASSERT(!type->baseType()->internalName().isEmpty());
    const QString baseClass = type->baseType()->internalName();

    const auto rootType = m_visitor->result();
    const bool documentRoot = (type == rootType);
    const bool isAnonymous = !documentRoot || type->internalName().at(0).isLower();

    const auto hasQmlBase = [](const QQmlJSScope::ConstPtr &scope) {
        if (!scope)
            return false;
        const auto base = scope->baseType();
        if (!base)
            return false;
        return base->isComposite() && base->scopeType() == QQmlJSScope::QMLScope;
    };
    const bool baseTypeIsCompiledQml = hasQmlBase(type);

    QmltcCodeGenerator generator { m_visitor };

    current.baseClasses = { baseClass };
    if (!documentRoot) {
        // make document root a friend to allow it to access init and endInit
        current.otherCode << u"friend class %1;"_qs.arg(rootType->internalName());

        // additionally make an immediate parent a friend since that parent
        // would create the object through a non-public constructor
        const auto realQmlScope = [](const QQmlJSScope::ConstPtr &scope) {
            if (scope->isArrayScope())
                return scope->parentScope();
            return scope;
        };
        current.otherCode << u"friend class %1;"_qs.arg(
                realQmlScope(type->parentScope())->internalName());
    } else {
        // make QQmltcObjectCreationBase<DocumentRoot> a friend to allow it to
        // be created for the root object
        current.otherCode << u"friend class QQmltcObjectCreationBase<%1>;"_qs.arg(
                rootType->internalName());

        QmltcMethod typeCountMethod;
        typeCountMethod.name = QmltcCodeGenerator::typeCountName;
        typeCountMethod.returnType = u"uint"_qs;
        typeCountMethod.body << u"return " + generator.generate_typeCount() + u";";
        current.typeCount = typeCountMethod;
    }
    // make QQmltcObjectCreationHelper a friend of every type since it provides
    // useful helper methods for all types
    current.otherCode << u"friend class QT_PREPEND_NAMESPACE(QQmltcObjectCreationHelper);"_qs;

    current.mocCode = {
        u"Q_OBJECT"_qs,
        // Note: isAnonymous holds for non-root types in the document as well
        isAnonymous ? u"QML_ANONYMOUS"_qs : u"QML_ELEMENT"_qs,
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
    current.init.name = u"QML_init"_qs;
    current.init.returnType = u"QQmlRefPointer<QQmlContextData>"_qs;
    current.beginClass.name = u"QML_beginClass"_qs;
    current.beginClass.returnType = u"void"_qs;
    current.endInit.name = u"QML_endInit"_qs;
    current.endInit.returnType = u"void"_qs;
    current.completeComponent.name = u"QML_completeComponent"_qs;
    current.completeComponent.returnType = u"void"_qs;
    current.finalizeComponent.name = u"QML_finalizeComponent"_qs;
    current.finalizeComponent.returnType = u"void"_qs;
    current.handleOnCompleted.name = u"QML_handleOnCompleted"_qs;
    current.handleOnCompleted.returnType = u"void"_qs;
    QmltcVariable creator(u"QQmltcObjectCreationHelper*"_qs, u"creator"_qs);
    QmltcVariable engine(u"QQmlEngine*"_qs, u"engine"_qs);
    QmltcVariable parent(u"QObject*"_qs, u"parent"_qs, u"nullptr"_qs);
    QmltcVariable ctxtdata(u"const QQmlRefPointer<QQmlContextData>&"_qs, u"parentContext"_qs);
    QmltcVariable finalizeFlag(u"bool"_qs, u"canFinalize"_qs);
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
    if (baseTypeIsCompiledQml) {
        // call parent's (QML type's) basic ctor from this. that one will take
        // care about QObject::setParent()
        current.baselineCtor.initializerList = { baseClass + u"(" + parent.name + u")" };
    } else {
        // default call to ctor is enough, but QQml_setParent_noEvent() is
        // needed (note: faster? version of QObject::setParent())
        current.baselineCtor.body << u"QQml_setParent_noEvent(this, " + parent.name + u");";
    }

    // compilation stub:
    current.externalCtor.body << u"Q_UNUSED(engine);"_qs;
    current.endInit.body << u"Q_UNUSED(engine);"_qs;
    current.endInit.body << u"Q_UNUSED(creator);"_qs;
    if (documentRoot) {
        current.externalCtor.body << u"// document root:"_qs;
        // if it's document root, we want to create our QQmltcObjectCreationBase
        // that would store all the created objects
        current.externalCtor.body << u"QQmltcObjectCreationBase<%1> objectHolder;"_qs.arg(
                type->internalName());
        current.externalCtor.body
                << u"QQmltcObjectCreationHelper creator = objectHolder.view();"_qs;
        current.externalCtor.body << u"creator.set(0, this);"_qs; // special case
        // now call init
        current.externalCtor.body << current.init.name
                        + u"(&creator, engine, QQmlContextData::get(engine->rootContext()), /* "
                          u"endInit */ true);";

        current.endInit.body << u"Q_UNUSED(canFinalize);"_qs;
    } else {
        current.externalCtor.body << u"// not document root:"_qs;
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
                        u"Internal error: property '%1' has incomplete information"_qs.arg(
                                p.propertyName()));
            continue;
        }
        if (p.isAlias()) {
            m_prototypeCodegen->compileAlias(current, p, type);
        } else {
            compileProperty(current, p, type);
        }
    }

    QHash<QString, const QmlIR::Function *> irFunctionsByName;
    std::for_each(object.irObject->functionsBegin(), object.irObject->functionsEnd(),
                  [&](const QmlIR::Function &function) {
                      irFunctionsByName.insert(m_prototypeCodegen->stringAt(function.nameIndex),
                                               std::addressof(function));
                  });
    const auto methods = type->ownMethods();
    current.functions.reserve(methods.size() + properties.size() * 3); // sensible default
    for (auto it = methods.cbegin(); it != methods.cend(); ++it) {
        const QmlIR::Function *irFunction = irFunctionsByName.value(it.key(), nullptr);
        m_prototypeCodegen->compileMethod(current, it.value(), irFunction, object);
    }

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
                                               { object.type, u"this"_qs, u""_qs, false });
        }

        for (; it != sortedBindings.cend(); ++it) {
            m_prototypeCodegen->compileBinding(current, **it, object,
                                               { type, u"this"_qs, u""_qs, false });
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
                              u"Q_ENUM(%1)"_qs.arg(e.name()));
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

void QmltcCompiler::compileMethod(QmltcType &current, const QQmlJSMetaMethod &m)
{
    const auto figureReturnType = [](const QQmlJSMetaMethod &m) {
        const bool isVoidMethod =
                m.returnTypeName() == u"void" || m.methodType() == QQmlJSMetaMethod::Signal;
        Q_ASSERT(isVoidMethod || m.returnType());
        QString type;
        if (isVoidMethod) {
            type = u"void"_qs;
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
        // just put "unimplemented" for now
        for (const QmltcVariable &param : compiledParams)
            code << u"Q_UNUSED(%1);"_qs.arg(param.name);
        code << u"Q_UNIMPLEMENTED();"_qs;

        if (returnType != u"void"_qs) {
            code << u"return %1;"_qs.arg(m.returnType()->accessSemantics()
                                                         == QQmlJSScope::AccessSemantics::Reference
                                                 ? u"nullptr"_qs
                                                 : returnType + u"{}");
        }
    }

    QmltcMethod compiled {};
    compiled.returnType = returnType;
    compiled.name = m.methodName();
    compiled.parameterList = std::move(compiledParams);
    compiled.body = std::move(code);
    compiled.type = methodType;
    compiled.access = m.access();
    if (methodType == QQmlJSMetaMethod::Method)
        compiled.declarationPrefixes << u"Q_INVOKABLE"_qs;
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
        setter.returnType = u"void"_qs;
        setter.name = compilationData.write;
        // QmltcVariable
        setter.parameterList.emplaceBack(QQmlJSUtils::constRefify(underlyingType), name + u"_",
                                         u""_qs);
        setter.body << variableName + u".setValue(" + name + u"_);";
        setter.body << u"Q_EMIT " + compilationData.notify + u"();";
        setter.userVisible = true;
        current.functions.emplaceBack(setter);
        mocPieces << u"WRITE"_qs << setter.name;
    }

    QmltcMethod getter {};
    getter.returnType = underlyingType;
    getter.name = compilationData.read;
    getter.body << u"return " + variableName + u".value();";
    getter.userVisible = true;
    current.functions.emplaceBack(getter);
    mocPieces << u"READ"_qs << getter.name;

    // 2. add bindable
    if (!p.isList()) {
        QmltcMethod bindable {};
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
                                            value ? u"true"_qs : u"false"_qs, accessor.name);
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
                        u"Cannot assign null to incompatible property"_qs);
        } else if (propertyType->accessSemantics() == QQmlJSScope::AccessSemantics::Reference) {
            generator.generate_assignToProperty(&current.init.body, type, p, u"nullptr"_qs,
                                                accessor.name);
        } else {
            generator.generate_assignToProperty(&current.init.body, type, p,
                                                u"QVariant::fromValue(nullptr)"_qs, accessor.name);
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
        m_logger->log(u"This binding is invalid"_qs, Log_Compiler, binding.sourceLocation());
        break;
    }
    default: {
        m_logger->log(u"Binding type is not supported (yet)"_qs, Log_Compiler,
                      binding.sourceLocation());
        break;
    }
    }
}

QT_END_NAMESPACE
