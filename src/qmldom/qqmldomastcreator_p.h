// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQMLDOMASTCREATOR_P_H
#define QQMLDOMASTCREATOR_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "qqmldomelements_p.h"
#include "qqmldomitem_p.h"

#include <QtQmlCompiler/private/qqmljsimportvisitor_p.h>

#include <QtQml/private/qqmljsastvisitor_p.h>

QT_BEGIN_NAMESPACE

namespace QQmlJS {
namespace Dom {

class QQmlDomAstCreator final : public AST::Visitor
{
    Q_DECLARE_TR_FUNCTIONS(QQmlDomAstCreator)
    using AST::Visitor::endVisit;
    using AST::Visitor::visit;

    static constexpr const auto className = "QmlDomAstCreator";

    class DomValue
    {
    public:
        template<typename T>
        DomValue(const T &obj) : kind(T::kindValue), value(obj)
        {
        }
        DomType kind;
        std::variant<QmlObject, MethodInfo, QmlComponent, PropertyDefinition, Binding, EnumDecl,
                     EnumItem, ConstantData, Id>
                value;
    };

    class StackEl
    {
    public:
        Path path;
        DomValue item;
        FileLocations::Tree fileLocations;
    };

    MutableDomItem qmlFile;
    std::shared_ptr<QmlFile> qmlFilePtr;
    QVector<StackEl> nodeStack;
    QVector<int> arrayBindingLevels;
    FileLocations::Tree rootMap;

    template<typename T>
    StackEl &currentEl(int idx = 0)
    {
        Q_ASSERT_X(idx < nodeStack.size() && idx >= 0, "currentQmlObjectOrComponentEl",
                   "Stack does not contain enough elements!");
        int i = nodeStack.size() - idx;
        while (i-- > 0) {
            DomType k = nodeStack.at(i).item.kind;
            if (k == T::kindValue)
                return nodeStack[i];
        }
        Q_ASSERT_X(false, "currentEl", "Stack does not contan object of type ");
        return nodeStack.last();
    }

    template<typename T>
    T &current(int idx = 0)
    {
        return std::get<T>(currentEl<T>(idx).item.value);
    }

    index_type currentIndex() { return currentNodeEl().path.last().headIndex(); }

    StackEl &currentQmlObjectOrComponentEl(int idx = 0);

    StackEl &currentNodeEl(int i = 0);

    DomValue &currentNode(int i = 0);

    void removeCurrentNode(std::optional<DomType> expectedType);

    void pushEl(Path p, DomValue it, AST::Node *n)
    {
        nodeStack.append({ p, it, createMap(it.kind, p, n) });
    }

    FileLocations::Tree createMap(FileLocations::Tree base, Path p, AST::Node *n);

    FileLocations::Tree createMap(DomType k, Path p, AST::Node *n);

public:
    QQmlDomAstCreator(MutableDomItem qmlFile);

    bool visit(AST::UiProgram *program) override;
    void endVisit(AST::UiProgram *) override;

    bool visit(AST::UiPragma *el) override;

    bool visit(AST::UiImport *el) override;

    bool visit(AST::UiPublicMember *el) override;
    void endVisit(AST::UiPublicMember *el) override;

    bool visit(AST::UiSourceElement *el) override;
    void endVisit(AST::UiSourceElement *) override;

    void loadAnnotations(AST::UiObjectMember *el) { AST::Node::accept(el->annotations, this); }

    bool visit(AST::UiObjectDefinition *el) override;
    void endVisit(AST::UiObjectDefinition *) override;

    bool visit(AST::UiObjectBinding *el) override;
    void endVisit(AST::UiObjectBinding *) override;

    bool visit(AST::UiScriptBinding *el) override;
    void endVisit(AST::UiScriptBinding *) override;

    bool visit(AST::UiArrayBinding *el) override;
    void endVisit(AST::UiArrayBinding *) override;

    bool visit(AST::UiParameterList *el) override;
    void endVisit(AST::UiParameterList *el) override;

    bool visit(AST::UiQualifiedId *) override;

    bool visit(AST::UiEnumDeclaration *el) override;
    void endVisit(AST::UiEnumDeclaration *) override;

    bool visit(AST::UiEnumMemberList *el) override;
    void endVisit(AST::UiEnumMemberList *el) override;

    bool visit(AST::UiInlineComponent *el) override;
    void endVisit(AST::UiInlineComponent *) override;

    bool visit(AST::UiRequired *el) override;

    bool visit(AST::UiAnnotation *el) override;
    void endVisit(AST::UiAnnotation *) override;

    void throwRecursionDepthError() override;

public:
    friend class QQmlDomAstCreatorWithQQmlJSScope;
};

class QQmlDomAstCreatorWithQQmlJSScope : public AST::Visitor
{
public:
    QQmlDomAstCreatorWithQQmlJSScope(MutableDomItem &qmlFile, QQmlJSLogger *logger);

#define X(name)                       \
    bool visit(AST::name *) override; \
    void endVisit(AST::name *) override;
    QQmlJSASTClassListToVisit
#undef X

    virtual void throwRecursionDepthError() override;

private:
    void setScopeInDom()
    {
        QQmlJSScope::Ptr scope = m_scopeCreator.m_currentScope;
        if (!m_domCreator.nodeStack.isEmpty()) {
            std::visit(
                    [&scope](auto &&e) {
                        using U = std::remove_cv_t<std::remove_reference_t<decltype(e)>>;
                        if constexpr (std::is_same_v<U, QmlObject>) {
                            e.setSemanticScope(scope);
                        }
                    },
                    m_domCreator.currentNodeEl().item.value);
        }
    }

    template<typename T>
    bool visitT(T *t)
    {
        if (m_marker && m_marker->nodeKind == t->kind) {
            m_marker->count += 1;
        }

        // first case: no marker, both can visit
        if (!m_marker) {
            bool continueForDom = m_domCreator.visit(t);
            bool continueForScope = m_scopeCreator.visit(t);
            if (!continueForDom && !continueForScope)
                return false;
            else if (continueForDom ^ continueForScope) {
                m_marker.emplace();
                m_marker->inactiveVisitor = continueForDom ? ScopeCreator : DomCreator;
                m_marker->count = 1;
                m_marker->nodeKind = AST::Node::Kind(t->kind);
                return true;
            } else {
                Q_ASSERT(continueForDom && continueForScope);
                return true;
            }
            Q_UNREACHABLE();
        }

        // second case: a marker, just one visit
        switch (m_marker->inactiveVisitor) {
        case DomCreator: {
            const bool continueForScope = m_scopeCreator.visit(t);
            return continueForScope;
        }
        case ScopeCreator: {
            const bool continueForDom = m_domCreator.visit(t);
            return continueForDom;
        }
        };
        Q_UNREACHABLE();
    }

    template<typename T>
    void endVisitT(T *t)
    {
        if (m_marker && m_marker->nodeKind == t->kind) {
            m_marker->count -= 1;
            if (m_marker->count == 0)
                m_marker.reset();
        }

        if (m_marker) {
            switch (m_marker->inactiveVisitor) {
            case DomCreator: {
                m_scopeCreator.endVisit(t);
                return;
            }
            case ScopeCreator: {
                m_domCreator.endVisit(t);
                return;
            }
            };
            Q_UNREACHABLE();
        }

        m_domCreator.endVisit(t);
        setScopeInDom();
        m_scopeCreator.endVisit(t);
    }

    QQmlJSScope::Ptr m_root;
    QQmlJSLogger *m_logger;
    QQmlJSImporter m_importer;
    QString m_implicitImportDirectory;
    QQmlJSImportVisitor m_scopeCreator;
    QQmlDomAstCreator m_domCreator;

    enum InactiveVisitor : bool { DomCreator, ScopeCreator };
    struct Marker
    {
        qsizetype count;
        AST::Node::Kind nodeKind;
        InactiveVisitor inactiveVisitor;
    };
    std::optional<Marker> m_marker;
};

} // end namespace Dom
} // end namespace QQmlJS

QT_END_NAMESPACE
#endif // QQMLDOMASTCREATOR_P_H
