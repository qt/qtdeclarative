// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmltcvisitor.h"
#include "qmltcpropertyutils.h"

#include <QtCore/qfileinfo.h>
#include <QtCore/qstack.h>
#include <QtCore/qdir.h>
#include <QtCore/qloggingcategory.h>

#include <private/qqmljsutils_p.h>

#include <algorithm>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

Q_DECLARE_LOGGING_CATEGORY(lcQmltcCompiler)

static QString uniqueNameFromPieces(const QStringList &pieces, QHash<QString, int> &repetitions)
{
    QString possibleName = pieces.join(u'_');
    const int count = repetitions[possibleName]++;
    if (count > 0)
        possibleName.append(u"_" + QString::number(count));
    return possibleName;
}

static bool isExplicitComponent(const QQmlJSScope::ConstPtr &type)
{
    if (!type->isComposite())
        return false;
    auto base = type->baseType();
    return base && base->internalName() == u"QQmlComponent";
}

static bool isImplicitComponent(const QQmlJSScope::ConstPtr &type)
{
    if (!type->isComposite())
        return false;
    const auto cppBase = QQmlJSScope::nonCompositeBaseType(type);
    const bool isComponentBased = (cppBase && cppBase->internalName() == u"QQmlComponent");
    return type->isComponentRootElement() && !isComponentBased;
}

static bool isOrUnderComponent(QQmlJSScope::ConstPtr type)
{
    Q_ASSERT(type->isComposite()); // we're dealing with composite types here
    for (; type; type = type->parentScope()) {
        if (isExplicitComponent(type) || isImplicitComponent(type))
            return true;
    }
    return false;
}

QmltcVisitor::QmltcVisitor(const QQmlJSScope::Ptr &target, QQmlJSImporter *importer,
                           QQmlJSLogger *logger, const QString &implicitImportDirectory,
                           const QStringList &qmldirFiles)
    : QQmlJSImportVisitor(target, importer, logger, implicitImportDirectory, qmldirFiles)
{
    m_qmlTypeNames.append(QFileInfo(logger->fileName()).baseName()); // put document root
}

void QmltcVisitor::findCppIncludes()
{
    // TODO: this pass is slow: we have to do exhaustive search because some C++
    // code could do forward declarations and they are hard to handle correctly
    QSet<const QQmlJSScope *> visitedTypes; // we can still improve by walking all types only once
    const auto visitType = [&visitedTypes](const QQmlJSScope::ConstPtr &type) -> bool {
        if (visitedTypes.contains(type.data()))
            return true;
        visitedTypes.insert(type.data());
        return false;
    };
    const auto addCppInclude = [this](const QQmlJSScope::ConstPtr &type) {
        if (QString includeFile = type->filePath(); includeFile.endsWith(u".h"))
            m_cppIncludes.insert(std::move(includeFile));
    };

    const auto findInType = [&](const QQmlJSScope::ConstPtr &type) {
        if (!type)
            return;
        if (visitType(type)) // optimization - don't call nonCompositeBaseType() needlessly
            return;

        // look in type
        addCppInclude(type);

        // look in type's base type
        auto base = type->baseType();
        Q_ASSERT(base || !type->isComposite());
        if (!base || visitType(base))
            return;
        addCppInclude(base);
    };

    const auto constructPrivateInclude = [](QStringView publicInclude) -> QString {
        if (publicInclude.isEmpty())
            return QString();
        Q_ASSERT(publicInclude.endsWith(u".h"_s) || publicInclude.endsWith(u".hpp"_s));
        const qsizetype dotLocation = publicInclude.lastIndexOf(u'.');
        QStringView extension = publicInclude.sliced(dotLocation);
        QStringView includeWithoutExtension = publicInclude.first(dotLocation);
        // check if "public" include is in fact already private
        if (publicInclude.startsWith(u"private"))
            return includeWithoutExtension + u"_p" + extension;
        return u"private/" + includeWithoutExtension + u"_p" + extension;
    };

    // walk the whole type hierarchy
    QStack<QQmlJSScope::ConstPtr> types;
    types.push(m_exportedRootScope);
    while (!types.isEmpty()) {
        auto type = types.pop();
        Q_ASSERT(type);

        const auto scopeType = type->scopeType();
        if (scopeType != QQmlJSScope::QMLScope && scopeType != QQmlJSScope::GroupedPropertyScope
            && scopeType != QQmlJSScope::AttachedPropertyScope) {
            continue;
        }

        for (auto t = type; !type->isArrayScope() && t; t = t->baseType()) {
            findInType(t);

            // look in properties
            const auto properties = t->ownProperties();
            for (const QQmlJSMetaProperty &p : properties) {
                findInType(p.type());

                if (p.isPrivate() && t->filePath().endsWith(u".h")) {
                    const QString ownersInclude = t->filePath();
                    QString privateInclude = constructPrivateInclude(ownersInclude);
                    if (!privateInclude.isEmpty())
                        m_cppIncludes.insert(std::move(privateInclude));
                }
            }

            // look in methods
            const auto methods = t->ownMethods();
            for (const QQmlJSMetaMethod &m : methods) {
                findInType(m.returnType());

                const auto parameters = m.parameterTypes();
                for (const auto &param : parameters)
                    findInType(param);
            }
        }

        types.append(type->childScopes());
    }

    // remove own include
    m_cppIncludes.remove(m_exportedRootScope->filePath());
}

static void addCleanQmlTypeName(QStringList *names, const QQmlJSScope::ConstPtr &scope)
{
    Q_ASSERT(scope->scopeType() == QQmlJSScope::QMLScope);
    Q_ASSERT(!scope->isArrayScope());
    Q_ASSERT(!scope->baseTypeName().isEmpty());
    // the scope is guaranteed to be a new QML type, so any prefixes (separated
    // by dots) should be import namespaces
    names->append(scope->baseTypeName().replace(u'.', u'_'));
}

bool QmltcVisitor::visit(QQmlJS::AST::UiObjectDefinition *object)
{
    const bool processingRoot = !rootScopeIsValid();

    if (!QQmlJSImportVisitor::visit(object))
        return false;

    if (processingRoot) {
        Q_ASSERT(rootScopeIsValid());
        setRootFilePath();
    }

    // we're not interested in non-QML scopes
    if (m_currentScope->scopeType() != QQmlJSScope::QMLScope)
        return true;

    if (m_currentScope != m_exportedRootScope) // not document root
        addCleanQmlTypeName(&m_qmlTypeNames, m_currentScope);
    // give C++-relevant internal names to QMLScopes, we can use them later in compiler
    m_currentScope->setInternalName(uniqueNameFromPieces(m_qmlTypeNames, m_qmlTypeNameCounts));

    if (auto base = m_currentScope->baseType();
        base && base->isComposite() && !isOrUnderComponent(m_currentScope)) {
        m_qmlTypesWithQmlBases.append(m_currentScope);
    }

    return true;
}

void QmltcVisitor::endVisit(QQmlJS::AST::UiObjectDefinition *object)
{
    if (m_currentScope->scopeType() == QQmlJSScope::QMLScope)
        m_qmlTypeNames.removeLast();
    QQmlJSImportVisitor::endVisit(object);
}

bool QmltcVisitor::visit(QQmlJS::AST::UiObjectBinding *uiob)
{
    if (!QQmlJSImportVisitor::visit(uiob))
        return false;

    if (m_currentScope != m_exportedRootScope) // not document root
        addCleanQmlTypeName(&m_qmlTypeNames, m_currentScope);
    // give C++-relevant internal names to QMLScopes, we can use them later in compiler
    m_currentScope->setInternalName(uniqueNameFromPieces(m_qmlTypeNames, m_qmlTypeNameCounts));

    if (auto base = m_currentScope->baseType();
        base && base->isComposite() && !isOrUnderComponent(m_currentScope)) {
        m_qmlTypesWithQmlBases.append(m_currentScope);
    }

    return true;
}

void QmltcVisitor::endVisit(QQmlJS::AST::UiObjectBinding *uiob)
{
    m_qmlTypeNames.removeLast();
    QQmlJSImportVisitor::endVisit(uiob);
}

bool QmltcVisitor::visit(QQmlJS::AST::UiPublicMember *publicMember)
{
    if (!QQmlJSImportVisitor::visit(publicMember))
        return false;

    // augment property: set its write/read/etc. methods
    if (publicMember->type == QQmlJS::AST::UiPublicMember::Property) {
        const auto name = publicMember->name.toString();

        QQmlJSScope::Ptr owner =
                m_savedBindingOuterScope ? m_savedBindingOuterScope : m_currentScope;
        QQmlJSMetaProperty property = owner->ownProperty(name);
        Q_ASSERT(property.isValid());
        if (!property.isAlias()) { // aliases are covered separately
            QmltcPropertyData compiledData(property);
            if (property.read().isEmpty())
                property.setRead(compiledData.read);
            if (!property.isList()) {
                if (property.write().isEmpty() && property.isWritable())
                    property.setWrite(compiledData.write);
                // Note: prefer BINDABLE to NOTIFY
                if (property.bindable().isEmpty())
                    property.setBindable(compiledData.bindable);
            }
            owner->addOwnProperty(property);
        }

        const QString notifyName = name + u"Changed"_s;
        // also check that notify is already a method of the scope
        {
            auto owningScope = m_savedBindingOuterScope ? m_savedBindingOuterScope : m_currentScope;
            const auto methods = owningScope->ownMethods(notifyName);
            if (methods.size() != 1) {
                const QString errorString =
                        methods.isEmpty() ? u"no signal"_s : u"too many signals"_s;
                m_logger->log(
                        u"internal error: %1 found for property '%2'"_s.arg(errorString, name),
                        Log_Compiler, publicMember->identifierToken);
                return false;
            } else if (methods[0].methodType() != QQmlJSMetaMethod::Signal) {
                m_logger->log(u"internal error: method %1 of property %2 must be a signal"_s.arg(
                                      notifyName, name),
                              Log_Compiler, publicMember->identifierToken);
                return false;
            }
        }
    }

    return true;
}

bool QmltcVisitor::visit(QQmlJS::AST::UiScriptBinding *scriptBinding)
{
    if (!QQmlJSImportVisitor::visit(scriptBinding))
        return false;

    {
        const auto id = scriptBinding->qualifiedId;
        if (!id->next && id->name == QLatin1String("id"))
            m_typesWithId[m_currentScope] = -1; // temporary value
    }

    return true;
}

bool QmltcVisitor::visit(QQmlJS::AST::UiInlineComponent *component)
{
    if (!QQmlJSImportVisitor::visit(component))
        return false;
    m_logger->log(u"Inline components are not supported"_s, Log_Compiler,
                  component->firstSourceLocation());
    // despite the failure, return true here so that we do not assert in
    // QQmlJSImportVisitor::endVisit(UiInlineComponent)
    return true;
}

void QmltcVisitor::endVisit(QQmlJS::AST::UiProgram *program)
{
    QQmlJSImportVisitor::endVisit(program);
    if (!rootScopeIsValid()) // in case we failed badly
        return;

    QHash<QQmlJSScope::ConstPtr, QList<QQmlJSMetaPropertyBinding>> bindings;
    for (const QQmlJSScope::ConstPtr &type : qAsConst(m_qmlTypes)) {
        if (isOrUnderComponent(type))
            continue;
        bindings.insert(type, type->ownPropertyBindingsInQmlIROrder());
    }

    postVisitResolve(bindings);
    setupAliases();

    if (m_mode != Mode::Compile)
        return;

    findCppIncludes();

    for (const QQmlJSScope::ConstPtr &type : m_pureQmlTypes)
        checkForNamingCollisionsWithCpp(type);
}

QQmlJSScope::ConstPtr fetchType(const QQmlJSMetaPropertyBinding &binding)
{
    switch (binding.bindingType()) {
    case QQmlJSMetaPropertyBinding::Object:
        return binding.objectType();
    case QQmlJSMetaPropertyBinding::Interceptor:
        return binding.interceptorType();
    case QQmlJSMetaPropertyBinding::ValueSource:
        return binding.valueSourceType();
    // TODO: AttachedProperty and GroupProperty are not supported yet,
    // but have to also be acknowledged here
    default:
        return {};
    }
    // coverity complains about the unreachable return below,
    // some compilers; some compilers complain without it
    Q_UNREACHABLE();
    return {};
}

template<typename Predicate>
void iterateTypes(
        const QQmlJSScope::ConstPtr &root,
        const QHash<QQmlJSScope::ConstPtr, QList<QQmlJSMetaPropertyBinding>> &qmlIrOrderedBindings,
        Predicate predicate)
{
    // NB: depth-first-search is used here to mimic various QmlIR passes
    QStack<QQmlJSScope::ConstPtr> types;
    types.push(root);
    while (!types.isEmpty()) {
        auto current = types.pop();

        if (predicate(current))
            continue;

        if (isOrUnderComponent(current)) // ignore implicit/explicit components
            continue;

        Q_ASSERT(qmlIrOrderedBindings.contains(current));
        const auto &bindings = qmlIrOrderedBindings[current];
        // reverse the binding order here, because stack processes right-most
        // child first and we need left-most first
        for (auto it = bindings.rbegin(); it != bindings.rend(); ++it) {
            const auto &binding = *it;
            if (auto type = fetchType(binding))
                types.push(type);
        }
    }
}

template<typename Predicate>
void iterateBindings(
        const QQmlJSScope::ConstPtr &root,
        const QHash<QQmlJSScope::ConstPtr, QList<QQmlJSMetaPropertyBinding>> &qmlIrOrderedBindings,
        Predicate predicate)
{
    // NB: depth-first-search is used here to mimic various QmlIR passes
    QStack<QQmlJSScope::ConstPtr> types;
    types.push(root);
    while (!types.isEmpty()) {
        auto current = types.pop();

        if (isOrUnderComponent(current)) // ignore implicit/explicit components
            continue;

        Q_ASSERT(qmlIrOrderedBindings.contains(current));
        const auto &bindings = qmlIrOrderedBindings[current];
        // reverse the binding order here, because stack processes right-most
        // child first and we need left-most first
        for (auto it = bindings.rbegin(); it != bindings.rend(); ++it) {
            const auto &binding = *it;

            if (predicate(current, binding))
                continue;

            if (auto type = fetchType(binding))
                types.push(type);
        }
    }
}

void QmltcVisitor::postVisitResolve(
        const QHash<QQmlJSScope::ConstPtr, QList<QQmlJSMetaPropertyBinding>> &qmlIrOrderedBindings)
{
    // This is a special function that must be called after
    // QQmlJSImportVisitor::endVisit(QQmlJS::AST::UiProgram *). It is used to
    // resolve things that couldn't be resolved during the AST traversal, such
    // as anything that is dependent on implicit or explicit components

    // match scopes to indices of QmlIR::Object from QmlIR::Document
    qsizetype count = 0;
    const auto setIndex = [&](const QQmlJSScope::Ptr &current) {
        if (current->scopeType() != QQmlJSScope::QMLScope || current->isArrayScope())
            return;
        Q_ASSERT(!m_qmlIrObjectIndices.contains(current));
        m_qmlIrObjectIndices[current] = count;
        ++count;
    };
    QQmlJSUtils::traverseFollowingQmlIrObjectStructure(m_exportedRootScope, setIndex);

    // find types that are part of the deferred bindings (we care about *types*
    // exclusively here)
    QSet<QQmlJSScope::ConstPtr> deferredTypes;
    const auto findDeferred = [&](const QQmlJSScope::ConstPtr &type,
                                  const QQmlJSMetaPropertyBinding &binding) {
        const QString propertyName = binding.propertyName();
        Q_ASSERT(!propertyName.isEmpty());
        if (type->isNameDeferred(propertyName)) {
            m_typesWithDeferredBindings.insert(type);

            if (binding.hasObject() || binding.hasInterceptor() || binding.hasValueSource()) {
                deferredTypes.insert(fetchType(binding));
                return true;
            }
        }
        return false;
    };
    iterateBindings(m_exportedRootScope, qmlIrOrderedBindings, findDeferred);

    const auto isOrUnderDeferred = [&deferredTypes](QQmlJSScope::ConstPtr type) {
        for (; type; type = type->parentScope()) {
            if (deferredTypes.contains(type))
                return true;
        }
        return false;
    };

    // find all "pure" QML types
    m_pureQmlTypes.reserve(m_qmlTypes.size());
    QList<QQmlJSScope::ConstPtr> explicitComponents;
    for (qsizetype i = 0; i < m_qmlTypes.size(); ++i) {
        const QQmlJSScope::ConstPtr &type = m_qmlTypes.at(i);

        if (isOrUnderComponent(type) || isOrUnderDeferred(type)) {
            // root is special: we compile Component roots. root is also never
            // deferred, so in case `isOrUnderDeferred(type)` returns true, we
            // always continue here
            if (type != m_exportedRootScope) {
                // if a type is an explicit component, its id "leaks" into the
                // document context
                if (isExplicitComponent(type))
                    explicitComponents.append(type);
                continue;
            }
        }

        m_creationIndices[type] = m_pureQmlTypes.size();
        m_pureQmlTypes.append(type);
    }

    // add explicit components to the object creation indices
    {
        qsizetype index = 0;
        for (const QQmlJSScope::ConstPtr &c : qAsConst(explicitComponents))
            m_creationIndices[c] = m_pureQmlTypes.size() + index++;
    }

    // filter out deferred types
    {
        QList<QQmlJSScope::ConstPtr> filteredQmlTypesWithQmlBases;
        filteredQmlTypesWithQmlBases.reserve(m_qmlTypesWithQmlBases.size());
        std::copy_if(m_qmlTypesWithQmlBases.cbegin(), m_qmlTypesWithQmlBases.cend(),
                     std::back_inserter(filteredQmlTypesWithQmlBases),
                     [&](const QQmlJSScope::ConstPtr &type) { return !isOrUnderDeferred(type); });
        qSwap(m_qmlTypesWithQmlBases, filteredQmlTypesWithQmlBases);
    }

    // count QmlIR::Objects in the document - the amount is used to calculate
    // object indices of implicit components
    int qmlScopeCount = 0;
    const auto countQmlScopes = [&](const QQmlJSScope::ConstPtr &scope) {
        if (scope->isArrayScope()) // special kind of QQmlJSScope::QMLScope
            return;
        switch (scope->scopeType()) {
        case QQmlJSScope::QMLScope:
        case QQmlJSScope::GroupedPropertyScope:
        case QQmlJSScope::AttachedPropertyScope: {
            ++qmlScopeCount;
            break;
        }
        default:
            return;
        }
        return;
    };
    // order doesn't matter (so re-use QQmlJSUtils)
    QQmlJSUtils::traverseFollowingQmlIrObjectStructure(m_exportedRootScope, countQmlScopes);
    Q_ASSERT(qmlScopeCount >= int(m_qmlTypes.size()));

    // figure synthetic indices of QQmlComponent-wrapped types
    int syntheticCreationIndex = 0;
    const auto addSyntheticIndex = [&](const QQmlJSScope::ConstPtr &type) {
        // explicit component
        if (isExplicitComponent(type)) {
            m_syntheticTypeIndices[type] = m_qmlIrObjectIndices.value(type, -1);
            return true;
        }
        // implicit component
        if (isImplicitComponent(type)) {
            const int index = int(qmlScopeCount) + syntheticCreationIndex++;
            m_syntheticTypeIndices[type] = index;
            return true;
        }
        return false;
    };
    iterateTypes(m_exportedRootScope, qmlIrOrderedBindings, addSyntheticIndex);

    // figure runtime object ids for non-component wrapped types
    int currentId = 0;
    const auto setRuntimeId = [&](const QQmlJSScope::ConstPtr &type) {
        // any type wrapped in an implicit component shouldn't be processed
        // here. even if it has id, it doesn't need to be set by qmltc
        if (type->isComponentRootElement())
            return true;

        if (m_typesWithId.contains(type))
            m_typesWithId[type] = currentId++;

        // otherwise we need to `return true` here
        Q_ASSERT(!type->isInlineComponent());
        return false;
    };
    iterateTypes(m_exportedRootScope, qmlIrOrderedBindings, setRuntimeId);
}

static void setAliasData(QQmlJSMetaProperty *alias, const QQmlJSUtils::ResolvedAlias &origin)
{
    Q_ASSERT(origin.kind != QQmlJSUtils::AliasTarget_Invalid);
    QmltcPropertyData compiledData(*alias);
    if (alias->read().isEmpty())
        alias->setRead(compiledData.read);
    if (origin.kind == QQmlJSUtils::AliasTarget_Object) // id-pointing aliases only have READ method
        return;
    if (origin.property.isWritable() && alias->write().isEmpty())
        alias->setWrite(compiledData.write);
    if (!origin.property.notify().isEmpty() && alias->notify().isEmpty())
        alias->setNotify(compiledData.notify);
    if (!origin.property.bindable().isEmpty() && alias->bindable().isEmpty())
        alias->setBindable(compiledData.bindable);
}

void QmltcVisitor::setupAliases()
{
    QStack<QQmlJSScope::Ptr> types;
    types.push(m_exportedRootScope);

    while (!types.isEmpty()) {
        QQmlJSScope::Ptr current = types.pop();
        auto properties = current->ownProperties();

        for (QQmlJSMetaProperty &p : properties) {
            if (!p.isAlias())
                continue;

            auto result = QQmlJSUtils::resolveAlias(m_scopesById, p, current,
                                                    QQmlJSUtils::AliasResolutionVisitor {});
            if (result.kind == QQmlJSUtils::AliasTarget_Invalid) {
                m_logger->log(QStringLiteral("Cannot resolve alias \"%1\"").arg(p.propertyName()),
                              Log_Alias, current->sourceLocation());
                continue;
            }
            setAliasData(&p, result);
            current->addOwnProperty(p);
        }
    }
}

void QmltcVisitor::checkForNamingCollisionsWithCpp(const QQmlJSScope::ConstPtr &type)
{
    static const QString cppKeywords[] = {
        u"alignas"_s,
        u"alignof"_s,
        u"and"_s,
        u"and_eq"_s,
        u"asm"_s,
        u"atomic_cancel"_s,
        u"atomic_commit"_s,
        u"atomic_noexcept"_s,
        u"auto"_s,
        u"bitand"_s,
        u"bitor"_s,
        u"bool"_s,
        u"break"_s,
        u"case"_s,
        u"catch"_s,
        u"char"_s,
        u"char8_t"_s,
        u"char16_t"_s,
        u"char32_t"_s,
        u"class"_s,
        u"compl"_s,
        u"concept"_s,
        u"const"_s,
        u"consteval"_s,
        u"constexpr"_s,
        u"const_cast"_s,
        u"continue"_s,
        u"co_await"_s,
        u"co_return"_s,
        u"co_yield"_s,
        u"decltype"_s,
        u"default"_s,
        u"delete"_s,
        u"do"_s,
        u"double"_s,
        u"dynamic_cast"_s,
        u"else"_s,
        u"enum"_s,
        u"explicit"_s,
        u"export"_s,
        u"extern"_s,
        u"false"_s,
        u"float"_s,
        u"for"_s,
        u"friend"_s,
        u"goto"_s,
        u"if"_s,
        u"inline"_s,
        u"int"_s,
        u"long"_s,
        u"mutable"_s,
        u"namespace"_s,
        u"new"_s,
        u"noexcept"_s,
        u"not"_s,
        u"not_eq"_s,
        u"nullptr"_s,
        u"operator"_s,
        u"or"_s,
        u"or_eq"_s,
        u"private"_s,
        u"protected"_s,
        u"public"_s,
        u"reflexpr"_s,
        u"register"_s,
        u"reinterpret_cast"_s,
        u"requires"_s,
        u"return"_s,
        u"short"_s,
        u"signed"_s,
        u"sizeof"_s,
        u"static"_s,
        u"static_assert"_s,
        u"static_cast"_s,
        u"struct"_s,
        u"switch"_s,
        u"synchronized"_s,
        u"template"_s,
        u"this"_s,
        u"thread_local"_s,
        u"throw"_s,
        u"true"_s,
        u"try"_s,
        u"typedef"_s,
        u"typeid"_s,
        u"typename"_s,
        u"union"_s,
        u"unsigned"_s,
        u"using"_s,
        u"virtual"_s,
        u"void"_s,
        u"volatile"_s,
        u"wchar_t"_s,
        u"while"_s,
        u"xor"_s,
        u"xor_eq"_s,
    };

    const auto isReserved = [&](QStringView word) {
        if (word.startsWith(QChar(u'_')) && word.size() >= 2
            && (word[1].isUpper() || word[1] == QChar(u'_'))) {
            return true; // Identifiers starting with underscore and uppercase are reserved in C++
        }
        return std::binary_search(std::begin(cppKeywords), std::end(cppKeywords), word);
    };

    const auto validate = [&](QStringView name, QStringView errorPrefix) {
        if (!isReserved(name))
            return;
        m_logger->log(errorPrefix + u" '" + name + u"' is a reserved C++ word, consider renaming",
                      Log_Compiler, type->sourceLocation());
    };

    const auto enums = type->ownEnumerations();
    for (auto it = enums.cbegin(); it != enums.cend(); ++it) {
        const QQmlJSMetaEnum e = it.value();
        validate(e.name(), u"Enumeration");

        const auto enumKeys = e.keys();
        for (const auto &key : enumKeys)
            validate(key, u"Enumeration '%1' key"_s.arg(e.name()));
    }

    const auto properties = type->ownProperties();
    for (auto it = properties.cbegin(); it != properties.cend(); ++it) {
        const QQmlJSMetaProperty &p = it.value();
        validate(p.propertyName(), u"Property");
    }

    const auto methods = type->ownMethods();
    for (auto it = methods.cbegin(); it != methods.cend(); ++it) {
        const QQmlJSMetaMethod &m = it.value();
        validate(m.methodName(), u"Method");

        const auto parameterNames = m.parameterNames();
        for (const auto &name : parameterNames)
            validate(name, u"Method '%1' parameter"_s.arg(m.methodName()));
    }

    // TODO: one could also test signal handlers' parameters but we do not store
    // this information in QQmlJSMetaPropertyBinding currently
}

void QmltcVisitor::setRootFilePath()
{
    const QString filePath = m_exportedRootScope->filePath();
    if (filePath.endsWith(u".h")) // assume the correct path is set
        return;
    Q_ASSERT(filePath.endsWith(u".qml"_s));

    const QString correctedFilePath = sourceDirectoryPath(filePath);
    const QStringList paths = m_importer->resourceFileMapper()->resourcePaths(
            QQmlJSResourceFileMapper::localFileFilter(correctedFilePath));
    auto firstHeader = std::find_if(paths.cbegin(), paths.cend(),
                                    [](const QString &x) { return x.endsWith(u".h"_s); });
    if (firstHeader == paths.cend()) {
        const QString matchedPaths = paths.isEmpty() ? u"<none>"_s : paths.join(u", ");
        qCDebug(lcQmltcCompiler,
                "Failed to find a header file name for path %s. Paths checked:\n%s",
                correctedFilePath.toUtf8().constData(), matchedPaths.toUtf8().constData());
        return;
    }
    // NB: get the file name to avoid prefixes
    m_exportedRootScope->setFilePath(QFileInfo(*firstHeader).fileName());
}

/*! \internal

    Returns a source directory path for a corresponding \a path which is either
    a source directory path (rarely - when processing currently-compiled file)
    or a build directory path (mostly - when importing a module/file).
*/
QString QmltcVisitor::sourceDirectoryPath(const QString &path)
{
    const QStringList resourcePaths = m_importer->resourceFileMapper()->resourcePaths(
            QQmlJSResourceFileMapper::localFileFilter(path));
    // path could be pointing to a source directory already
    const QString uniquePrefix = u"/qt_qml_module_dir_mapping/"_s;
    if (resourcePaths.size() != 1 || !resourcePaths[0].startsWith(uniquePrefix)) {
        // found nothing or path is a source directory path already. either way
        // return the input here and let the caller take care of it
        const QString matchedPaths =
                resourcePaths.isEmpty() ? u"<none>"_s : resourcePaths.join(u", ");
        qCDebug(lcQmltcCompiler,
                "Path %s is not a build directory path. Resource paths that matched:\n%s",
                path.toUtf8().constData(), matchedPaths.toUtf8().constData());
        return path;
    }
    QString sourceDirPath = resourcePaths[0];
    sourceDirPath.remove(0, uniquePrefix.size());
    Q_ASSERT(QFileInfo(sourceDirPath).exists());
    return sourceDirPath;
}

QT_END_NAMESPACE
