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

#include "qmltcvisitor.h"

#include <QtCore/qfileinfo.h>
#include <QtCore/qstack.h>

#include <algorithm>

QT_BEGIN_NAMESPACE

static QString uniqueNameFromPieces(const QStringList &pieces, QHash<QString, int> &repetitions)
{
    QString possibleName = pieces.join(u'_');
    const int count = repetitions[possibleName]++;
    if (count > 0)
        possibleName.append(u"_" + QString::number(count));
    return possibleName;
}

static bool isOrUnderComponent(QQmlJSScope::ConstPtr type)
{
    Q_ASSERT(type->isComposite()); // we're dealing with composite types here
    for (; type; type = type->parentScope()) {
        if (type->isWrappedInImplicitComponent())
            return true;
        // for a composite type, its internalName() is guaranteed to not be a
        // QQmlComponent. we need to detect a case with `Component {}` QML type
        // where the *immediate* base type of the current type will be the
        // QQmlComponent. note that non-composite base is different: if our type
        // is not a direct child of QQmlComponent, then it is a good type that
        // we have to compile
        if (auto base = type->baseType(); base && base->internalName() == u"QQmlComponent")
            return true;
    }
    return false;
}

QmltcVisitor::QmltcVisitor(QQmlJSImporter *importer, QQmlJSLogger *logger,
                           const QString &implicitImportDirectory, const QStringList &qmldirFiles)
    : QQmlJSImportVisitor(
            QQmlJSScope::create(), importer, logger, implicitImportDirectory, qmldirFiles)
{
    m_qmlTypeNames.append(QFileInfo(logger->fileName()).baseName()); // put document root
}

void QmltcVisitor::findCppIncludes()
{
    // TODO: this pass is fairly slow: we have to do exhaustive search because
    // some C++ code could do forward declarations
    QSet<const QQmlJSScope *> visitedTypes; // we can still improve by walking all types only once
    const auto visitType = [&visitedTypes](const QQmlJSScope::ConstPtr &type) -> bool {
        if (visitedTypes.contains(type.data()))
            return true;
        visitedTypes.insert(type.data());
        return false;
    };

    const auto populateFromType = [&](const QQmlJSScope::ConstPtr &type) {
        if (!type) // TODO: it is a crutch
            return;
        if (visitType(type)) // optimization - don't call nonCompositeBaseType() needlessly
            return;
        auto t = QQmlJSScope::nonCompositeBaseType(type);
        if (t != type && visitType(t))
            return;

        QString includeFile = t->filePath();
        if (includeFile.endsWith(u".h"))
            m_cppIncludes.insert(std::move(includeFile));
    };

    const auto constructPrivateInclude = [](QStringView publicInclude) -> QString {
        if (publicInclude.isEmpty())
            return QString();
        Q_ASSERT(publicInclude.endsWith(u".h"_qs) || publicInclude.endsWith(u".hpp"_qs));
        const qsizetype dotLocation = publicInclude.lastIndexOf(u'.');
        QStringView extension = publicInclude.sliced(dotLocation);
        QStringView includeWithoutExtension = publicInclude.first(dotLocation);
        // check if "public" include is in fact already private
        if (publicInclude.startsWith(u"private"))
            return includeWithoutExtension + u"_p" + extension;
        return u"private/" + includeWithoutExtension + u"_p" + extension;
    };

    // walk the whole type hierarchy
    for (const QQmlJSScope::ConstPtr &type : qAsConst(m_pureQmlTypes)) {
        // TODO: figure how to NOT walk all the types. theoretically, we can
        // stop at first non-composite type
        for (auto t = type; t; t = t->baseType()) {
            if (visitType(t))
                break;
            // look in type
            if (auto includeFile = t->filePath(); includeFile.endsWith(u".h"))
                m_cppIncludes.insert(std::move(includeFile));

            // look in properties
            const auto properties = t->ownProperties();
            for (const QQmlJSMetaProperty &p : properties) {
                populateFromType(p.type());

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
                populateFromType(m.returnType());
                // TODO: debug Q_ASSERT(m.returnType())
                const auto parameters = m.parameterTypes();
                for (const auto &param : parameters)
                    populateFromType(param);
            }
        }
    }
}

void QmltcVisitor::findTypeIndicesInQmlDocument()
{
    qsizetype count = 0;

    // Perform DFS to align with the logic of discovering new QmlIR::Objects
    // during IR building: we should align with it here to get correct object
    // indices within the QmlIR::Document.
    QList<QQmlJSScope::Ptr> stack;
    stack.append(m_exportedRootScope);

    while (!stack.isEmpty()) {
        QQmlJSScope::Ptr current = stack.takeLast();

        if (current->scopeType() == QQmlJSScope::QMLScope) {
            Q_ASSERT(!m_qmlIrObjectIndices.contains(current));
            m_qmlIrObjectIndices[current] = count;
            ++count;
        }

        const auto &children = current->childScopes();
        std::copy(children.rbegin(), children.rend(), std::back_inserter(stack));
    }
}

bool QmltcVisitor::visit(QQmlJS::AST::UiObjectDefinition *object)
{
    if (!QQmlJSImportVisitor::visit(object))
        return false;

    // we're not interested in non-QML scopes
    if (m_currentScope->scopeType() != QQmlJSScope::QMLScope)
        return true;

    Q_ASSERT(m_currentScope->internalName().isEmpty());
    Q_ASSERT(!m_currentScope->baseTypeName().isEmpty());
    if (m_currentScope != m_exportedRootScope) // not document root
        m_qmlTypeNames.append(m_currentScope->baseTypeName());
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

    Q_ASSERT(m_currentScope->scopeType() == QQmlJSScope::QMLScope);
    Q_ASSERT(m_currentScope->internalName().isEmpty());
    Q_ASSERT(!m_currentScope->baseTypeName().isEmpty());
    if (m_currentScope != m_exportedRootScope) // not document root
        m_qmlTypeNames.append(m_currentScope->baseTypeName());
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

        // TODO: we should set the composite type property methods here, but as
        // of now this is done in the pass over the types after the ast
        // traversal

        const QString notifyName = name + u"Changed"_qs;
        // also check that notify is already a method of the scope
        {
            auto owningScope = m_savedBindingOuterScope ? m_savedBindingOuterScope : m_currentScope;
            const auto methods = owningScope->ownMethods(notifyName);
            if (methods.size() != 1) {
                const QString errorString =
                        methods.isEmpty() ? u"no signal"_qs : u"too many signals"_qs;
                m_logger->log(
                        u"internal error: %1 found for property '%2'"_qs.arg(errorString, name),
                        Log_Compiler, publicMember->identifierToken);
                return false;
            } else if (methods[0].methodType() != QQmlJSMetaMethod::Signal) {
                m_logger->log(u"internal error: method %1 of property %2 must be a signal"_qs.arg(
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
    m_logger->log(u"Inline components are not supported"_qs, Log_Compiler,
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
    findCppIncludes();
    findTypeIndicesInQmlDocument();
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

    // 1. find types that are part of the deferred bindings (we care about
    //    *types* exclusively here)
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

    // 2. find all "pure" QML types
    m_pureQmlTypes.reserve(m_qmlTypes.size());
    for (qsizetype i = 0; i < m_qmlTypes.size(); ++i) {
        const QQmlJSScope::ConstPtr &type = m_qmlTypes.at(i);

        if (isOrUnderComponent(type) || isOrUnderDeferred(type)) {
            // root is special: we compile Component roots. root is also never
            // deferred, so in case `isOrUnderDeferred(type)` returns true, we
            // always continue here
            if (type != m_exportedRootScope)
                continue;
        }

        m_pureTypeIndices[type] = m_pureQmlTypes.size();
        m_pureQmlTypes.append(type);
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

    // 3. figure synthetic indices of QQmlComponent-wrapped types
    int syntheticCreationIndex = -1;
    const auto addSyntheticIndex = [&](const QQmlJSScope::ConstPtr &type) mutable {
        if (type->isComponentRootElement()) {
            m_syntheticTypeIndices[type] = ++syntheticCreationIndex;
            return true;
        }
        return false;
    };
    iterateTypes(m_exportedRootScope, qmlIrOrderedBindings, addSyntheticIndex);

    // 4. figure runtime object ids for non-component wrapped types
    int currentId = 0;
    const auto setRuntimeId = [&](const QQmlJSScope::ConstPtr &type) mutable {
        // fancy way to call type->isComponentRootElement(). any type that is
        // considered synthetic shouldn't be processed here. even if it has id,
        // it doesn't need to be set by qmltc
        if (m_syntheticTypeIndices.contains(type))
            return true;

        if (m_typesWithId.contains(type))
            m_typesWithId[type] = currentId++;

        // otherwise we need to `return true` here
        Q_ASSERT(!type->isInlineComponent());
        return false;
    };
    iterateTypes(m_exportedRootScope, qmlIrOrderedBindings, setRuntimeId);
}

QT_END_NAMESPACE
