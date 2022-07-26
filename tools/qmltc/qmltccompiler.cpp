// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmltccompiler.h"
#include "qmltcoutputir.h"
#include "qmltccodewriter.h"
#include "qmltcpropertyutils.h"
#include "qmltccompilerpieces.h"

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

QString QmltcCompiler::newSymbol(const QString &base)
{
    QString symbol = base;
    symbol.replace(QLatin1String("."), QLatin1String("_"));
    while (symbol.startsWith(QLatin1Char('_')) && symbol.size() >= 2
           && (symbol[1].isUpper() || symbol[1] == QLatin1Char('_'))) {
        symbol.remove(0, 1);
    }
    if (!m_symbols.contains(symbol)) {
        m_symbols.insert(symbol, 1);
    } else {
        symbol += u"_" + QString::number(m_symbols[symbol]++);
    }
    return symbol;
}

void QmltcCompiler::compile(const QmltcCompilerInfo &info)
{
    m_info = info;
    Q_ASSERT(!m_info.outputCppFile.isEmpty());
    Q_ASSERT(!m_info.outputHFile.isEmpty());
    Q_ASSERT(!m_info.resourcePath.isEmpty());

    // Note: we only compile "pure" QML types. any component-wrapped type is
    // expected to appear through a binding
    auto pureTypes = m_visitor->pureQmlTypes();
    Q_ASSERT(m_visitor->result() == pureTypes.at(0));

    const auto isComponent = [](const QQmlJSScope::ConstPtr &type) {
        auto base = type->baseType();
        return base && base->internalName() == u"QQmlComponent"_s;
    };

    QmltcCodeGenerator generator { m_url, m_visitor };

    QmltcMethod urlMethod;
    compileUrlMethod(urlMethod, generator.urlMethodName());
    m_urlMethodName = urlMethod.name;

    QQmlJSScope::ConstPtr root = pureTypes.at(0);

    QList<QmltcType> compiledTypes;
    if (isComponent(root)) {
        compiledTypes.reserve(1);
        compiledTypes.emplaceBack(); // create empty type
        const auto compile = [&](QmltcType &current, const QQmlJSScope::ConstPtr &type) {
            generator.generate_initCodeForTopLevelComponent(current, type);
        };
        compileType(compiledTypes.back(), root, compile);
    } else {
        const auto compile = [this](QmltcType &current, const QQmlJSScope::ConstPtr &type) {
            compileTypeElements(current, type);
        };

        compiledTypes.reserve(pureTypes.size());
        for (const auto &type : pureTypes) {
            Q_ASSERT(type->scopeType() == QQmlJSScope::QMLScope);
            compiledTypes.emplaceBack(); // create empty type
            compileType(compiledTypes.back(), type, compile);
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
    program.includes = m_visitor->cppIncludeFiles();
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
    current.setComplexBindings.access = QQmlJSMetaMethod::Protected;
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
    current.setComplexBindings.name = u"QML_setComplexBindings"_s;
    current.setComplexBindings.returnType = u"void"_s;
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
    current.endInit.parameterList = { creator, engine };
    current.setComplexBindings.parameterList = { creator, engine };
    current.handleOnCompleted.parameterList = { creator };
    if (documentRoot) {
        current.externalCtor.parameterList = { engine, parent };
        current.init.parameterList = { creator, engine, ctxtdata, finalizeFlag };
        current.beginClass.parameterList = { creator, finalizeFlag };
        current.completeComponent.parameterList = { creator, finalizeFlag };
        current.finalizeComponent.parameterList = { creator, finalizeFlag };
    } else {
        current.externalCtor.parameterList = { creator, engine, parent };
        current.init.parameterList = { creator, engine, ctxtdata };
        current.beginClass.parameterList = { creator };
        current.completeComponent.parameterList = { creator };
        current.finalizeComponent.parameterList = { creator };
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
    } else {
        current.externalCtor.body << u"// not document root:"_s;
        // just call init, we don't do any setup here otherwise
        current.externalCtor.body << current.init.name
                        + u"(creator, engine, QQmlData::get(parent)->outerContext);";
    }

    auto postponedQmlContextSetup = generator.generate_initCode(current, type);
    generator.generate_endInitCode(current, type);
    generator.generate_setComplexBindingsCode(current, type);
    generator.generate_beginClassCode(current, type);
    generator.generate_completeComponentCode(current, type);
    generator.generate_finalizeComponentCode(current, type);
    generator.generate_handleOnCompletedCode(current, type);

    compileElements(current, type);
}

template<typename Iterator>
static Iterator partitionBindings(Iterator first, Iterator last)
{
    // NB: the code generator cares about script bindings being processed at a
    // later point, so we should sort or partition the range. we do a stable
    // partition since the relative order of binding evaluation affects the UI
    return std::stable_partition(first, last, [](const QQmlJSMetaPropertyBinding &b) {
        // we want complex bindings to be at the end, so do the negation
        return !QmltcCompiler::isComplexBinding(b);
    });
}

void QmltcCompiler::compileTypeElements(QmltcType &current, const QQmlJSScope::ConstPtr &type)
{
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

    auto bindings = type->ownPropertyBindingsInQmlIROrder();
    partitionBindings(bindings.begin(), bindings.end());

    for (auto it = bindings.begin(); it != bindings.end(); ++it)
        compileBinding(current, *it, type, { type });
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
    Q_ASSERT(names.size() == types.size());

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

static QString figureReturnType(const QQmlJSMetaMethod &m)
{
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
}

void QmltcCompiler::compileMethod(QmltcType &current, const QQmlJSMetaMethod &m,
                                  const QQmlJSScope::ConstPtr &owner)
{
    const auto returnType = figureReturnType(m);
    const auto paramNames = m.parameterNames();
    const auto paramTypes = m.parameterTypes();
    Q_ASSERT(paramNames.size() == paramTypes.size()); // assume verified
    const QList<QmltcVariable> compiledParams = compileMethodParameters(paramNames, paramTypes);
    const auto methodType = QQmlJSMetaMethod::Type(m.methodType());

    QStringList code;
    if (methodType != QQmlJSMetaMethod::Signal) {
        QmltcCodeGenerator urlGenerator { m_url, m_visitor };
        QmltcCodeGenerator::generate_callExecuteRuntimeFunction(
                &code, urlGenerator.urlMethodName() + u"()",
                owner->ownRuntimeFunctionIndex(m.jsFunctionIndex()), u"this"_s, returnType,
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
    QStringList epilogue;
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
        frames.push(
                AliasResolutionFrame { QStringList(), QStringList(), QStringList(), u"this"_s });
    };
    aliasVisitor.processResolvedId = [&](const QQmlJSScope::ConstPtr &type) {
        Q_ASSERT(type);
        if (owner != type) { // cannot start at `this`, need to fetch object through context
            const int id = m_visitor->runtimeId(type);
            Q_ASSERT(id >= 0); // since the type is found by id, it must have an id

            AliasResolutionFrame queryIdFrame {};
            Q_ASSERT(frames.top().outVar == u"this"_s); // so inVar would be "this" as well
            queryIdFrame.prologue << u"auto context = %1::q_qmltc_thisContext;"_s.arg(
                    owner->internalName());

            // doing the above allows us to lookup id object by index (fast)
            queryIdFrame.outVar = u"alias_objectById_" + aliasExprBits.front(); // unique enough
            const QString cppType = (m_visitor->qmlComponentIndex(type) == -1)
                    ? type->internalName()
                    : u"QQmlComponent"_s;
            queryIdFrame.prologue << u"auto " + queryIdFrame.outVar + u" = static_cast<" + cppType
                            + u"*>(context->idValue(" + QString::number(id) + u"));";
            queryIdFrame.prologue << u"Q_ASSERT(" + queryIdFrame.outVar + u");";

            frames.push(queryIdFrame);
        }
    };
    aliasVisitor.processResolvedProperty = [&](const QQmlJSMetaProperty &p,
                                               const QQmlJSScope::ConstPtr &owner) {
        AliasResolutionFrame queryPropertyFrame {};

        auto [extensionPrologue, extensionAccessor, extensionEpilogue] =
                QmltcCodeGenerator::wrap_extensionType(
                        owner, p,
                        QmltcCodeGenerator::wrap_privateClass(AliasResolutionFrame::inVar, p));
        QString inVar = extensionAccessor;
        queryPropertyFrame.prologue += extensionPrologue;
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
        queryPropertyFrame.epilogue += extensionEpilogue;

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
        auto [extensionPrologue, extensionAccessor, extensionEpilogue] =
                QmltcCodeGenerator::wrap_extensionType(
                        result.owner, result.property,
                        QmltcCodeGenerator::wrap_privateClass(frames.top().outVar,
                                                              result.property));
        customFinalFrame.prologue = extensionPrologue;
        customFinalFrame.outVar = extensionAccessor;
        customFinalFrame.epilogue = extensionEpilogue;
        frames.push(customFinalFrame);
    }

    const QString latestAccessor = frames.top().outVar;
    const QStringList prologue =
            joinFrames(frames, [](const AliasResolutionFrame &frame) { return frame.prologue; });
    const QStringList epilogue =
            joinFrames(frames, [](const AliasResolutionFrame &frame) { return frame.epilogue; });
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
    if (result.kind == QQmlJSUtils::AliasTarget_Property) {
        if (QString read = result.property.read(); !read.isEmpty()) {
            getter.body << u"return %1->%2();"_s.arg(latestAccessor, read);
        } else { // use QObject::property() as a fallback when read method is unknown
            getter.body << u"return qvariant_cast<%1>(%2->property(\"%3\"));"_s.arg(
                    underlyingType, latestAccessor, result.property.propertyName());
        }
    } else { // AliasTarget_Object
        getter.body << u"return " + latestAccessor + u";";
    }
    getter.body += epilogue;
    getter.userVisible = true;
    current.functions.emplaceBack(getter);
    mocLines << u"READ"_s << getter.name;

    if (result.property.isWritable()) {
        Q_ASSERT(result.kind == QQmlJSUtils::AliasTarget_Property); // property is invalid otherwise
        QmltcMethod setter {};
        setter.returnType = u"void"_s;
        setter.name = compilationData.write;

        const QString setName = result.property.write();
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
        if (!setName.isEmpty()) {
            setter.body << u"%1->%2(%3);"_s.arg(latestAccessor, setName,
                                                commaSeparatedParameterNames);
        } else { // use QObject::setProperty() as fallback when write method is unknown
            Q_ASSERT(parameterNames.size() == 1);
            const QString variantName = u"var_" + aliasName; // fairly unique
            setter.body << u"QVariant %1;"_s.arg(variantName);
            setter.body << u"%1.setValue(%2);"_s.arg(variantName, commaSeparatedParameterNames);
            setter.body << u"%1->setProperty(\"%2\", std::move(%3));"_s.arg(
                    latestAccessor, result.property.propertyName(), variantName);
        }
        setter.body += joinFrames(
                frames, [](const AliasResolutionFrame &frame) { return frame.epilogueForWrite; });
        setter.body += epilogue; // NB: *after* epilogueForWrite - see prologue construction
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
        bindable.body += epilogue;
        bindable.userVisible = true;
        current.functions.emplaceBack(bindable);
        mocLines << u"BINDABLE"_s << bindable.name;
    }
    // 3. add notify - which is pretty special
    if (QString notifyName = result.property.notify(); !notifyName.isEmpty()) {
        Q_ASSERT(result.kind == QQmlJSUtils::AliasTarget_Property); // property is invalid otherwise

        auto notifyFrames = frames;
        notifyFrames.pop(); // we don't need the last frame at all in this case

        const QStringList notifyPrologue = joinFrames(
                frames, [](const AliasResolutionFrame &frame) { return frame.prologue; });
        const QStringList notifyEpilogue = joinFrames(
                frames, [](const AliasResolutionFrame &frame) { return frame.epilogue; });

        // notify is very special
        current.endInit.body << u"{ // alias notify connection:"_s;
        current.endInit.body += notifyPrologue;
        // TODO: use non-private accessor since signals must exist on the public
        // type, not on the private one -- otherwise, you can't connect to a
        // private property signal in C++ and so it is useless (hence, use
        // public type)
        const QString cppType = (m_visitor->qmlComponentIndex(result.owner) == -1)
                ? result.owner->internalName()
                : u"QQmlComponent"_s;
        const QString latestAccessorNonPrivate = notifyFrames.top().outVar;
        current.endInit.body << u"QObject::connect(" + latestAccessorNonPrivate + u", &" + cppType
                        + u"::" + notifyName + u", this, &" + current.cppType + u"::"
                        + compilationData.notify + u");";
        current.endInit.body += notifyEpilogue;
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

static QString generate_callCompilationUnit(const QString &urlMethodName)
{
    return u"QQmlEnginePrivate::get(engine)->compilationUnitFromUrl(%1())"_s.arg(urlMethodName);
}

void QmltcCompiler::compileBinding(QmltcType &current, const QQmlJSMetaPropertyBinding &binding,
                                   const QQmlJSScope::ConstPtr &type,
                                   const BindingAccessorData &accessor)
{
    QString propertyName = binding.propertyName();
    Q_ASSERT(!propertyName.isEmpty());

    auto bindingType = binding.bindingType();

    // Note: unlike QQmlObjectCreator, we don't have to do a complicated
    // deferral logic for bindings: if a binding is deferred, it is not compiled
    // (potentially, with all the bindings inside of it), period.
    if (type->isNameDeferred(propertyName)) {
        const auto location = binding.sourceLocation();
        if (bindingType == QQmlJSMetaPropertyBinding::GroupProperty) {
            qCWarning(lcQmltcCompiler)
                    << QStringLiteral("Binding at line %1 column %2 is not deferred as it is a "
                                      "binding on a group property.")
                               .arg(QString::number(location.startLine),
                                    QString::number(location.startColumn));
            // we do not support PropertyChanges and other types with similar
            // behavior yet, so this binding is compiled
        } else {
            qCDebug(lcQmltcCompiler)
                    << QStringLiteral(
                               "Binding at line %1 column %2 is deferred and thus not compiled")
                               .arg(QString::number(location.startLine),
                                    QString::number(location.startColumn));
            return;
        }
    }

    const auto assignToProperty = [&](const QQmlJSMetaProperty &p, const QString &value,
                                      bool constructFromQObject = false) {
        QmltcCodeGenerator::generate_assignToProperty(&current.endInit.body, type, p, value,
                                                      accessor.name, constructFromQObject);
    };

    QQmlJSMetaProperty p = type->property(propertyName);
    QQmlJSScope::ConstPtr propertyType = p.type();

    const auto addObjectBinding = [&](const QString &value) {
        if (p.isList()) {
            const auto &listName =
                    m_uniques[UniqueStringId(current, propertyName)].qmlListVariableName;
            Q_ASSERT(!listName.isEmpty());
            current.endInit.body << u"%1.append(%2);"_s.arg(listName, value);
        } else {
            assignToProperty(p, value, /* constructFromQObject = */ true);
        }
    };

    // when property is list, create a local variable (unique per-scope &&
    // per-property) that would be used to append new elements
    if (p.isList()) {
        auto &listName = m_uniques[UniqueStringId(current, propertyName)].qmlListVariableName;
        if (listName.isEmpty()) { // not created yet, add extra instructions
            listName = u"listref_" + propertyName;
            current.endInit.body << QStringLiteral("QQmlListReference %1(%2, %3);")
                                            .arg(listName, accessor.name,
                                                 QQmlJSUtils::toLiteral(propertyName,
                                                                        u"QByteArrayLiteral"));
            current.endInit.body << QStringLiteral("Q_ASSERT(%1.canAppend());").arg(listName);
        }
    }

    switch (binding.bindingType()) {
    case QQmlJSMetaPropertyBinding::BoolLiteral: {
        const bool value = binding.boolValue();
        assignToProperty(p, value ? u"true"_s : u"false"_s);
        break;
    }
    case QQmlJSMetaPropertyBinding::NumberLiteral: {
        assignToProperty(p, QString::number(binding.numberValue()));
        break;
    }
    case QQmlJSMetaPropertyBinding::StringLiteral: {
        assignToProperty(p, QQmlJSUtils::toLiteral(binding.stringValue()));
        break;
    }
    case QQmlJSMetaPropertyBinding::RegExpLiteral: {
        const QString value =
                u"QRegularExpression(%1)"_s.arg(QQmlJSUtils::toLiteral(binding.regExpValue()));
        assignToProperty(p, value);
        break;
    }
    case QQmlJSMetaPropertyBinding::Null: {
        // poor check: null bindings are only supported for var and objects
        Q_ASSERT(propertyType->isSameType(m_typeResolver->varType())
                 || propertyType->accessSemantics() == QQmlJSScope::AccessSemantics::Reference);
        if (propertyType->accessSemantics() == QQmlJSScope::AccessSemantics::Reference)
            assignToProperty(p, u"nullptr"_s);
        else
            assignToProperty(p, u"QVariant::fromValue(nullptr)"_s);
        break;
    }
    case QQmlJSMetaPropertyBinding::Script: {
        QString bindingSymbolName = type->internalName() + u'_' + propertyName + u"_binding";
        bindingSymbolName.replace(u'.', u'_'); // can happen with group properties
        compileScriptBinding(current, binding, bindingSymbolName, type, propertyName, propertyType,
                             accessor);
        break;
    }
    case QQmlJSMetaPropertyBinding::Object: {
        // NB: object is compiled with compileType(), here just need to use it
        auto object = binding.objectType();

        // Note: despite a binding being set for `accessor`, we use "this" as a
        // parent of a created object. Both attached and grouped properties are
        // parented by "this", so lifetime-wise we should be fine
        const QString qobjectParent = u"this"_s;

        if (!propertyType) {
            recordError(binding.sourceLocation(),
                        u"Binding on property '" + propertyName + u"' of unknown type");
            return;
        }

        // special case of implicit or explicit component:
        if (qsizetype index = m_visitor->qmlComponentIndex(object); index >= 0) {
            const QString objectName = newSymbol(u"sc"_s);

            const qsizetype creationIndex = m_visitor->creationIndex(object);

            QStringList *block = (creationIndex == -1) ? &current.endInit.body : &current.init.body;
            *block << u"{"_s;
            *block << QStringLiteral("auto thisContext = QQmlData::get(%1)->outerContext;")
                              .arg(qobjectParent);
            *block << QStringLiteral("auto %1 = QQmlObjectCreator::createComponent(engine, "
                                     "%2, %3, %4, thisContext);")
                              .arg(objectName, generate_callCompilationUnit(m_urlMethodName),
                                   QString::number(index), qobjectParent);
            *block << QStringLiteral("thisContext->installContext(QQmlData::get(%1), "
                                     "QQmlContextData::OrdinaryObject);")
                              .arg(objectName);

            // objects wrapped in implicit components do not have visible ids,
            // however, explicit components can have an id and that one is going
            // to be visible in the common document context
            if (creationIndex != -1) {
                // explicit component
                Q_ASSERT(object->isComposite());
                Q_ASSERT(object->baseType()->internalName() == u"QQmlComponent"_s);

                if (int id = m_visitor->runtimeId(object); id >= 0) {
                    QString idString = m_visitor->addressableScopes().id(object);
                    if (idString.isEmpty())
                        idString = u"<unknown>"_s;
                    QmltcCodeGenerator::generate_setIdValue(block, u"thisContext"_s, id, objectName,
                                                            idString);
                }

                const QString creationIndexStr = QString::number(creationIndex);
                *block << QStringLiteral("creator->set(%1, %2);").arg(creationIndexStr, objectName);
                Q_ASSERT(block == &current.init.body);
                current.endInit.body
                        << QStringLiteral("auto %1 = creator->get<%2>(%3);")
                                   .arg(objectName, u"QQmlComponent"_s, creationIndexStr);
            }
            addObjectBinding(objectName);
            *block << u"}"_s;
            break;
        }

        const QString objectName = newSymbol(u"o"_s);
        current.init.body << u"auto %1 = new %2(creator, engine, %3);"_s.arg(
                objectName, object->internalName(), qobjectParent);
        current.init.body << u"creator->set(%1, %2);"_s.arg(
                QString::number(m_visitor->creationIndex(object)), objectName);

        // refetch the same object during endInit to set the bindings
        current.endInit.body << u"auto %1 = creator->get<%2>(%3);"_s.arg(
                objectName, object->internalName(),
                QString::number(m_visitor->creationIndex(object)));
        addObjectBinding(objectName);
        break;
    }
    case QQmlJSMetaPropertyBinding::Interceptor:
        Q_FALLTHROUGH();
    case QQmlJSMetaPropertyBinding::ValueSource: {
        // NB: object is compiled with compileType(), here just need to use it
        QSharedPointer<const QQmlJSScope> object;
        if (bindingType == QQmlJSMetaPropertyBinding::Interceptor)
            object = binding.interceptorType();
        else
            object = binding.valueSourceType();

        // Note: despite a binding being set for `accessor`, we use "this" as a
        // parent of a created object. Both attached and grouped properties are
        // parented by "this", so lifetime-wise we should be fine
        const QString qobjectParent = u"this"_s;

        if (!propertyType) {
            recordError(binding.sourceLocation(),
                        u"Binding on property '" + propertyName + u"' of unknown type");
            return;
        }

        auto &objectName = m_uniques[UniqueStringId(current, propertyName)].onAssignmentObjectName;
        if (objectName.isEmpty()) {
            objectName = u"onAssign_" + propertyName;

            current.init.body << u"auto %1 = new %2(creator, engine, %3);"_s.arg(
                    objectName, object->internalName(), qobjectParent);
            current.init.body << u"creator->set(%1, %2);"_s.arg(
                    QString::number(m_visitor->creationIndex(object)), objectName);

            current.endInit.body << u"auto %1 = creator->get<%2>(%3);"_s.arg(
                    objectName, object->internalName(),
                    QString::number(m_visitor->creationIndex(object)));
        }

        // NB: we expect one "on" assignment per property, so creating
        // QQmlProperty each time should be fine (unlike QQmlListReference)
        current.endInit.body << u"{"_s;
        current.endInit.body << u"QQmlProperty qmlprop(%1, %2);"_s.arg(
                accessor.name, QQmlJSUtils::toLiteral(propertyName));
        current.endInit.body
                << u"QT_PREPEND_NAMESPACE(QQmlCppOnAssignmentHelper)::set(%1, qmlprop);"_s.arg(
                           objectName);
        current.endInit.body << u"}"_s;
        break;
    }
    case QQmlJSMetaPropertyBinding::AttachedProperty: {
        Q_ASSERT(accessor.name == u"this"_s); // doesn't have to hold, in fact
        const auto attachedType = binding.attachingType();
        Q_ASSERT(attachedType);

        const QString attachingTypeName = propertyName; // acts as an identifier
        auto attachingType = m_typeResolver->typeForName(attachingTypeName);

        QString attachedTypeName = attachedType->baseTypeName();
        Q_ASSERT(!attachedTypeName.isEmpty());

        auto &attachedMemberName =
                m_uniques[UniqueStringId(current, propertyName)].attachedVariableName;
        if (attachedMemberName.isEmpty()) {
            attachedMemberName = u"m_" + attachingTypeName;
            // add attached type as a member variable to allow noop lookup
            current.variables.emplaceBack(attachedTypeName + u" *", attachedMemberName,
                                          u"nullptr"_s);

            if (propertyName == u"Component"_s) { // Component attached type is special
                current.endInit.body << u"Q_ASSERT(qmlEngine(this));"_s;
                current.endInit.body
                        << u"// attached Component must be added to the object's QQmlData"_s;
                current.endInit.body
                        << u"Q_ASSERT(!QQmlEnginePrivate::get(qmlEngine(this))->activeObjectCreator);"_s;
            }

            // Note: getting attached property is fairly expensive
            const QString getAttachedPropertyLine = u"qobject_cast<" + attachedTypeName
                    + u" *>(qmlAttachedPropertiesObject<" + attachingType->internalName()
                    + u">(this, /* create = */ true))";
            current.endInit.body << attachedMemberName + u" = " + getAttachedPropertyLine + u";";

            if (propertyName == u"Component"_s) {
                // call completed/destruction signals appropriately
                current.handleOnCompleted.body
                        << u"Q_EMIT " + attachedMemberName + u"->completed();";
                if (!current.dtor) {
                    current.dtor = QmltcDtor {};
                    current.dtor->name = u"~" + current.cppType;
                }
                current.dtor->body << u"Q_EMIT " + attachedMemberName + u"->destruction();";
            }
        }

        auto subbindings = attachedType->ownPropertyBindingsInQmlIROrder();
        // compile bindings of the attached property
        partitionBindings(subbindings.begin(), subbindings.end());
        for (const auto &b : qAsConst(subbindings)) {
            compileBinding(current, b, attachedType,
                           { type, attachedMemberName, propertyName, false });
        }
        break;
    }
    case QQmlJSMetaPropertyBinding::GroupProperty: {
        Q_ASSERT(accessor.name == u"this"_s); // doesn't have to hold, in fact
        if (p.read().isEmpty()) {
            recordError(binding.sourceLocation(),
                        u"READ function of group property '" + propertyName + u"' is unknown");
            return;
        }

        auto groupType = binding.groupType();
        Q_ASSERT(groupType);

        const bool isValueType =
                propertyType->accessSemantics() == QQmlJSScope::AccessSemantics::Value;
        if (!isValueType
            && propertyType->accessSemantics() != QQmlJSScope::AccessSemantics::Reference) {
            recordError(binding.sourceLocation(),
                        u"Group property '" + propertyName + u"' has unsupported access semantics");
            return;
        }

        auto subbindings = groupType->ownPropertyBindingsInQmlIROrder();
        auto firstScript = partitionBindings(subbindings.begin(), subbindings.end());

        // if we have no non-script bindings, we have no bindings that affect
        // the value type group, so no reason to generate the wrapping code
        const bool generateValueTypeCode = isValueType && (subbindings.begin() != firstScript);

        QString groupAccessor =
                QmltcCodeGenerator::wrap_privateClass(accessor.name, p) + u"->" + p.read() + u"()";
        // NB: used when isValueType == true
        const QString groupPropertyVarName = accessor.name + u"_group_" + propertyName;
        // value types are special
        if (generateValueTypeCode) {
            if (p.write().isEmpty()) { // just reject this
                recordError(binding.sourceLocation(),
                            u"Group property '" + propertyName
                                    + u"' is a value type without a setter");
                return;
            }

            current.endInit.body << u"auto " + groupPropertyVarName + u" = " + groupAccessor + u";";
            // addressof operator is to make the binding logic work, which
            // expects that `accessor.name` is a pointer type
            groupAccessor = QmltcCodeGenerator::wrap_addressof(groupPropertyVarName);
        }

        // compile bindings of the grouped property
        const auto compile = [&](const QQmlJSMetaPropertyBinding &b) {
            compileBinding(current, b, groupType,
                           { type, groupAccessor, propertyName, isValueType });
        };

        auto it = subbindings.begin();
        for (; it != firstScript; ++it) {
            Q_ASSERT(it->bindingType() != QQmlJSMetaPropertyBinding::Script);
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
            current.endInit.body << QmltcCodeGenerator::wrap_privateClass(accessor.name, p) + u"->"
                            + p.write() + u"(" + groupPropertyVarName + u");";
        }

        // once the value is written back, process the script bindings
        for (; it != subbindings.end(); ++it) {
            Q_ASSERT(it->bindingType() == QQmlJSMetaPropertyBinding::Script);
            compile(*it);
        }
        break;
    }
    case QQmlJSMetaPropertyBinding::Invalid: {
        recordError(binding.sourceLocation(), u"This binding is invalid"_s);
        break;
    }
    default: {
        recordError(binding.sourceLocation(), u"Binding is not supported"_s);
        break;
    }
    }
}

// returns compiled script binding for "property changed" handler in a form of object type
static QmltcType compileScriptBindingPropertyChangeHandler(const QQmlJSMetaPropertyBinding &binding,
                                                           const QQmlJSScope::ConstPtr &objectType,
                                                           const QString &urlMethodName,
                                                           const QString &functorCppType,
                                                           const QString &objectCppType)
{
    QmltcType bindingFunctor {};
    bindingFunctor.cppType = functorCppType;
    bindingFunctor.ignoreInit = true;

    // default member variable and ctor:
    const QString pointerToObject = objectCppType + u" *";
    bindingFunctor.variables.emplaceBack(
            QmltcVariable { pointerToObject, u"m_self"_s, u"nullptr"_s });
    bindingFunctor.baselineCtor.name = functorCppType;
    bindingFunctor.baselineCtor.parameterList.emplaceBack(
            QmltcVariable { pointerToObject, u"self"_s, QString() });
    bindingFunctor.baselineCtor.initializerList.emplaceBack(u"m_self(self)"_s);

    // call operator:
    QmltcMethod callOperator {};
    callOperator.returnType = u"void"_s;
    callOperator.name = u"operator()"_s;
    callOperator.modifiers << u"const"_s;
    QmltcCodeGenerator::generate_callExecuteRuntimeFunction(
            &callOperator.body, urlMethodName + u"()",
            objectType->ownRuntimeFunctionIndex(binding.scriptIndex()), u"m_self"_s, u"void"_s, {});

    bindingFunctor.functions.emplaceBack(std::move(callOperator));

    return bindingFunctor;
}

// finds property for given scope and returns it together with the absolute
// property index in the property array of the corresponding QMetaObject
static std::pair<QQmlJSMetaProperty, int> getMetaPropertyIndex(const QQmlJSScope::ConstPtr &scope,
                                                               const QString &propertyName)
{
    auto owner = QQmlJSScope::ownerOfProperty(scope, propertyName).scope;
    Q_ASSERT(owner);
    auto p = owner->ownProperty(propertyName);
    if (!p.isValid())
        return { p, -1 };
    int index = p.index();
    if (index < 0) // this property doesn't have index - comes from QML
        return { p, -1 };
    // owner is not included in absolute index

    // TODO: we also have to go over extensions here!
    for (QQmlJSScope::ConstPtr base = owner->baseType(); base; base = base->baseType())
        index += int(base->ownProperties().size());
    return { p, index };
}

void QmltcCompiler::compileScriptBinding(QmltcType &current,
                                         const QQmlJSMetaPropertyBinding &binding,
                                         const QString &bindingSymbolName,
                                         const QQmlJSScope::ConstPtr &objectType,
                                         const QString &propertyName,
                                         const QQmlJSScope::ConstPtr &propertyType,
                                         const QmltcCompiler::BindingAccessorData &accessor)
{
    const auto compileScriptSignal = [&](const QString &name) {
        QString This_signal = u"this"_s;
        QString This_slot = u"this"_s;
        QString objectClassName_signal = objectType->internalName();
        QString objectClassName_slot = objectType->internalName();

        // TODO: ugly crutch to make stuff work
        if (accessor.name != u"this"_s) { // e.g. if attached property
            This_signal = accessor.name;
            This_slot = u"this"_s; // still
            objectClassName_signal = objectType->baseTypeName();
            objectClassName_slot = current.cppType; // real base class where slot would go
        }
        Q_ASSERT(!objectClassName_signal.isEmpty());
        Q_ASSERT(!objectClassName_slot.isEmpty());

        const auto signalMethods = objectType->methods(name, QQmlJSMetaMethod::Signal);
        Q_ASSERT(!signalMethods.isEmpty()); // an error somewhere else
        QQmlJSMetaMethod signal = signalMethods.at(0);
        Q_ASSERT(signal.methodType() == QQmlJSMetaMethod::Signal);

        const QString signalName = signal.methodName();
        const QString slotName = newSymbol(signalName + u"_slot");

        const QString signalReturnType = figureReturnType(signal);
        const QList<QmltcVariable> slotParameters = compileMethodParameters(
                signal.parameterNames(), signal.parameterTypes(), /* allow unnamed = */ true);

        // SignalHander specific:
        QmltcMethod slotMethod {};
        slotMethod.returnType = signalReturnType;
        slotMethod.name = slotName;
        slotMethod.parameterList = slotParameters;

        QmltcCodeGenerator::generate_callExecuteRuntimeFunction(
                &slotMethod.body, m_urlMethodName + u"()",
                objectType->ownRuntimeFunctionIndex(binding.scriptIndex()),
                u"this"_s, // Note: because script bindings always use current QML object scope
                signalReturnType, slotParameters);
        slotMethod.type = QQmlJSMetaMethod::Slot;

        current.functions << std::move(slotMethod);
        current.setComplexBindings.body
                << u"QObject::connect(" + This_signal + u", "
                        + QmltcCodeGenerator::wrap_qOverload(
                                slotParameters, u"&" + objectClassName_signal + u"::" + signalName)
                        + u", " + This_slot + u", &" + objectClassName_slot + u"::" + slotName
                        + u");";
    };

    switch (binding.scriptKind()) {
    case QQmlJSMetaPropertyBinding::Script_PropertyBinding: {
        if (!propertyType) {
            recordError(binding.sourceLocation(),
                        u"Binding on property '" + propertyName + u"' of unknown type");
            return;
        }

        auto [property, absoluteIndex] = getMetaPropertyIndex(objectType, propertyName);
        if (absoluteIndex < 0) {
            recordError(binding.sourceLocation(),
                        u"Binding on unknown property '" + propertyName + u"'");
            return;
        }

        QString bindingTarget = accessor.name;

        int valueTypeIndex = -1;
        if (accessor.isValueType) {
            Q_ASSERT(accessor.scope != objectType);
            bindingTarget = u"this"_s; // TODO: not necessarily "this"?
            auto [groupProperty, groupPropertyIndex] =
                    getMetaPropertyIndex(accessor.scope, accessor.propertyName);
            if (groupPropertyIndex < 0) {
                recordError(binding.sourceLocation(),
                            u"Binding on group property '" + accessor.propertyName
                                    + u"' of unknown type");
                return;
            }
            valueTypeIndex = absoluteIndex;
            absoluteIndex = groupPropertyIndex; // e.g. index of accessor.name
        }

        QmltcCodeGenerator::generate_createBindingOnProperty(
                &current.setComplexBindings.body, generate_callCompilationUnit(m_urlMethodName),
                u"this"_s, // NB: always using enclosing object as a scope for the binding
                static_cast<qsizetype>(objectType->ownRuntimeFunctionIndex(binding.scriptIndex())),
                bindingTarget, // binding target
                absoluteIndex, property, valueTypeIndex, accessor.name);
        break;
    }
    case QQmlJSMetaPropertyBinding::Script_SignalHandler: {
        const auto name = QQmlJSUtils::signalName(propertyName);
        Q_ASSERT(name.has_value());
        compileScriptSignal(*name);
        break;
    }
    case QQmlJSMetaPropertyBinding::Script_ChangeHandler: {
        const QString objectClassName = objectType->internalName();
        const QString bindingFunctorName = newSymbol(bindingSymbolName + u"Functor");

        const auto signalName = QQmlJSUtils::signalName(propertyName);
        Q_ASSERT(signalName.has_value()); // an error somewhere else
        const auto actualProperty = QQmlJSUtils::changeHandlerProperty(objectType, *signalName);
        Q_ASSERT(actualProperty.has_value()); // an error somewhere else
        const auto actualPropertyType = actualProperty->type();
        if (!actualPropertyType) {
            recordError(binding.sourceLocation(),
                        u"Binding on property '" + actualProperty->propertyName()
                                + u"' of unknown type");
            return;
        }

        // due to historical reasons (QQmlObjectCreator), prefer NOTIFY over
        // BINDABLE when both are available. thus, test for notify first
        const QString notifyString = actualProperty->notify();
        if (!notifyString.isEmpty()) {
            compileScriptSignal(notifyString);
            break;
        }
        const QString bindableString = actualProperty->bindable();
        QString typeOfQmlBinding =
                u"std::unique_ptr<QPropertyChangeHandler<" + bindingFunctorName + u">>";

        current.children << compileScriptBindingPropertyChangeHandler(
                binding, objectType, m_urlMethodName, bindingFunctorName, objectClassName);

        // TODO: this could be dropped if QQmlEngine::setContextForObject() is
        // done before currently generated C++ object is constructed
        current.setComplexBindings.body << bindingSymbolName + u".reset(new QPropertyChangeHandler<"
                        + bindingFunctorName + u">("
                        + QmltcCodeGenerator::wrap_privateClass(accessor.name, *actualProperty)
                        + u"->" + bindableString + u"().onValueChanged(" + bindingFunctorName + u"("
                        + accessor.name + u"))));";

        current.variables.emplaceBack(
                QmltcVariable { typeOfQmlBinding, bindingSymbolName, QString() });
        break;
    }
    default:
        recordError(binding.sourceLocation(), u"Invalid script binding found"_s);
        break;
    }
}

QT_END_NAMESPACE
