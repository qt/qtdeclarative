// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#include "qqmldom_utils_p.h"
#include "qqmldomitem_p.h"
#include "qqmldompath_p.h"
#include "qqmldomscriptelements_p.h"
#include <memory>
#include <utility>
#include <variant>

using namespace QQmlJS::Dom::ScriptElements;
using QQmlJS::Dom::DomType;
using QQmlJS::Dom::ScriptElement;
using QQmlJS::Dom::ScriptElementVariant;

/*!
   \internal
   \class ScriptElementBase

   The base class for all script elements.

   Derived classes should implement createFileLocations, DomElement::updatePathFromOwner and
   DomBase::iterateDirectSubpaths. Furthermore, they need their own DomType enum.

   updatePathFromOwner and createFileLocations should be called on the script element root node
   after it was constructed for the DomItem-wrapping to work correctly. Without it, methods like
   iterateDirectSubpaths and all the stuff in DomItem will not work.

   createFileLocations does not work without having the pathFromOwner set
   first via updatePathFromOwner.

   In derived classes, the updatePathFromOwner-implementation should call the base implementation
   and also call recursively updatePathFromOwner on the derived class's children.

   See \l ScriptElementBase::createFileLocations for the createFileLocations implementation in
   derived classes.

   Derived classes need to implement iterateDirectSubpaths to comply with the DomItem interface.
*/

/*!
   \internal
   \fn ScriptElementBase::createFileLocations

   Usually, all the visits/recursive calls to DOM elements can be done using the DomItem interface,
   once all the DOM has been constructed.

   During construction, createFileLocations can be used to annotate the DOM representation with the
   corresponding source locations, which are needed, e.g., to find the corresponding DOM element
   from a certain text position. When called, createFileLocations sets an entry for itself in the
   FileLocationsTree.

   Derived classes should call the base implemenatation and recursively call createFileLocations on
   all their children.

   Usually, only the root of the script DOM element requires one createFileLocations call after
   construction \b{and} after a pathFromOwner was set using updatePathFromOwner.

*/

/*!
   \internal
   \class ScriptList

   A Helper class for writing script elements that contain lists, helps for implementing the
   recursive calls of iterateDirectSubpaths, updatePathFromOwner and createFileLocations.
*/

/*!
   \internal
    Helper for fields with elements in iterateDirectSubpaths.
 */
static bool wrap(const QQmlJS::Dom::DomItem &self, QQmlJS::Dom::DirectVisitor visitor, QQmlJS::Dom::FieldName field,
                 const ScriptElementVariant &value)
{
    if (!value)
        return true;

    const bool b = visitor(
            field, [&self, field, &value]() -> QQmlJS::Dom::DomItem {
                const QQmlJS::Dom::Path pathFromOwner{ self.pathFromOwner().withField(field) };
                return self.subScriptElementWrapperItem(value);
            });
    return b;
}

/*!
   \internal
    Helper for fields with lists in iterateDirectSubpaths.
 */
static bool wrap(const QQmlJS::Dom::DomItem &self, QQmlJS::Dom::DirectVisitor visitor, QQmlJS::Dom::FieldName field,
                 const ScriptList &value)
{
    const bool b = visitor(
            field, [&self, field, &value]() -> QQmlJS::Dom::DomItem {
                const QQmlJS::Dom::Path pathFromOwner{ self.pathFromOwner().withField(field) };
                return self.subListItem(value.asList(pathFromOwner));
            });
    return b;
}

bool GenericScriptElement::iterateDirectSubpaths(const DomItem &self, DirectVisitor visitor) const
{
    bool cont = true;
    for (auto it = m_children.begin(); it != m_children.end(); ++it) {
        cont &= std::visit(
                [&self, &visitor, &it](auto &&e) { return wrap(self, visitor, it->first, e); },
                it->second);
    }
    for (auto it = m_values.begin(); it != m_values.end(); ++it) {
        cont &= self.invokeVisitorOnValue(visitor, it->first, it->second);
    }
    return cont;
}

void GenericScriptElement::updatePathFromOwner(const Path &p)
{
    BaseT::updatePathFromOwner(p);
    for (auto it = m_children.begin(); it != m_children.end(); ++it) {
        std::visit(qOverloadedVisitor{ [&p, &it](ScriptElementVariant &e) {
                                          e.base()->updatePathFromOwner(p.withField(it->first));
                                      },
                                       [&p, &it](ScriptList &list) {
                                           list.updatePathFromOwner(p.withField(it->first));
                                       } },
                   it->second);
    }
}

void GenericScriptElement::createFileLocations(const FileLocations::Tree &base)
{
    BaseT::createFileLocations(base);
    for (auto it = m_children.begin(); it != m_children.end(); ++it) {
        std::visit(
                qOverloadedVisitor{
                        [&base](ScriptElementVariant &e) { e.base()->createFileLocations(base); },
                        [&base](ScriptList &list) { list.createFileLocations(base); } },
                it->second);
    }
}

bool BlockStatement::iterateDirectSubpaths(const DomItem &self, DirectVisitor visitor) const
{
    // TODO: test me
    bool cont = true;
    cont &= wrap(self, visitor, Fields::statements, m_statements);
    return cont;
}

void BlockStatement::updatePathFromOwner(const Path &p)
{
    BaseT::updatePathFromOwner(p);
    m_statements.updatePathFromOwner(p.withField(Fields::statements));
}

void BlockStatement::createFileLocations(const FileLocations::Tree &base)
{
    BaseT::createFileLocations(base);
    m_statements.createFileLocations(base);
}

bool IdentifierExpression::iterateDirectSubpaths(const DomItem &self, DirectVisitor visitor) const
{
    bool cont = true;
    cont &= self.invokeVisitorOnValue(visitor, Fields::identifier, m_name);
    return cont;
}

bool Literal::iterateDirectSubpaths(const DomItem &self, DirectVisitor visitor) const
{
    bool cont = true;
    std::visit(
            [&cont, &visitor, &self](auto &&e) {
                cont &= self.invokeVisitorOnValue(visitor, Fields::value, e);
            },
            m_value);
    return cont;
}

bool IfStatement::iterateDirectSubpaths(const DomItem &self, DirectVisitor visitor) const
{
    // TODO: test me
    bool cont = true;
    cont &= wrap(self, visitor, Fields::condition, m_condition);
    cont &= wrap(self, visitor, Fields::consequence, m_consequence);
    cont &= wrap(self, visitor, Fields::alternative, m_alternative);
    return cont;
}

void IfStatement::updatePathFromOwner(const Path &p)
{
    BaseT::updatePathFromOwner(p);
    if (auto ptr = m_condition.base())
        ptr->updatePathFromOwner(p.withField(Fields::condition));
    if (auto ptr = m_consequence.base())
        ptr->updatePathFromOwner(p.withField(Fields::consequence));
    if (auto ptr = m_alternative.base())
        ptr->updatePathFromOwner(p.withField(Fields::alternative));
}

void IfStatement::createFileLocations(const FileLocations::Tree &base)
{
    BaseT::createFileLocations(base);
    if (auto ptr = m_condition.base())
        ptr->createFileLocations(base);
    if (auto ptr = m_consequence.base())
        ptr->createFileLocations(base);
    if (auto ptr = m_alternative.base())
        ptr->createFileLocations(base);
}

bool ForStatement::iterateDirectSubpaths(const DomItem &self, DirectVisitor visitor) const
{
    bool cont = true;
    cont &= wrap(self, visitor, Fields::initializer, m_initializer);
    cont &= wrap(self, visitor, Fields::declarations, m_declarations);
    cont &= wrap(self, visitor, Fields::condition, m_condition);
    cont &= wrap(self, visitor, Fields::expression, m_expression);
    cont &= wrap(self, visitor, Fields::body, m_body);
    return cont;
}

void ForStatement::updatePathFromOwner(const Path &p)
{
    BaseT::updatePathFromOwner(p);
    if (auto ptr = m_initializer.base())
        ptr->updatePathFromOwner(p.withField(Fields::initializer));
    if (auto ptr = m_declarations.base())
        ptr->updatePathFromOwner(p.withField(Fields::declarations));
    if (auto ptr = m_condition.base())
        ptr->updatePathFromOwner(p.withField(Fields::condition));
    if (auto ptr = m_expression.base())
        ptr->updatePathFromOwner(p.withField(Fields::expression));
    if (auto ptr = m_body.base())
        ptr->updatePathFromOwner(p.withField(Fields::body));
}

void ForStatement::createFileLocations(const FileLocations::Tree &base)
{
    BaseT::createFileLocations(base);
    if (auto ptr = m_initializer.base())
        ptr->createFileLocations(base);
    if (auto ptr = m_declarations.base())
        ptr->createFileLocations(base);
    if (auto ptr = m_condition.base())
        ptr->createFileLocations(base);
    if (auto ptr = m_expression.base())
        ptr->createFileLocations(base);
    if (auto ptr = m_body.base())
        ptr->createFileLocations(base);
}

bool BinaryExpression::iterateDirectSubpaths(const DomItem &self, DirectVisitor visitor) const
{
    bool cont = true;
    cont &= wrap(self, visitor, Fields::left, m_left);
    cont &= self.invokeVisitorOnValue(visitor, Fields::operation, m_operator);
    cont &= wrap(self, visitor, Fields::right, m_right);
    return cont;
}

void BinaryExpression::updatePathFromOwner(const Path &p)
{
    BaseT::updatePathFromOwner(p);
    if (auto ptr = m_left.base())
        ptr->updatePathFromOwner(p.withField(Fields::left));
    if (auto ptr = m_right.base())
        ptr->updatePathFromOwner(p.withField(Fields::right));
}

void BinaryExpression::createFileLocations(const FileLocations::Tree &base)
{
    BaseT::createFileLocations(base);
    if (auto ptr = m_left.base())
        ptr->createFileLocations(base);
    if (auto ptr = m_right.base())
        ptr->createFileLocations(base);
}

bool VariableDeclarationEntry::iterateDirectSubpaths(const DomItem &self, DirectVisitor visitor) const
{
    bool cont = true;
    cont &= self.invokeVisitorOnValue(visitor, Fields::scopeType, m_scopeType);
    cont &= wrap(self, visitor, Fields::identifier, m_identifier);
    cont &= wrap(self, visitor, Fields::initializer, m_initializer);
    return cont;
}

void VariableDeclarationEntry::updatePathFromOwner(const Path &p)
{
    BaseT::updatePathFromOwner(p);
    if (auto ptr = m_identifier.base())
        ptr->updatePathFromOwner(p.withField(Fields::identifier));
    if (auto ptr = m_initializer.base())
        ptr->updatePathFromOwner(p.withField(Fields::initializer));
}

void VariableDeclarationEntry::createFileLocations(const FileLocations::Tree &base)
{
    BaseT::createFileLocations(base);
    if (auto ptr = m_identifier.base())
        ptr->createFileLocations(base);
    if (auto ptr = m_initializer.base())
        ptr->createFileLocations(base);
}

bool VariableDeclaration::iterateDirectSubpaths(const DomItem &self, DirectVisitor visitor) const
{
    bool cont = true;
    cont &= wrap(self, visitor, Fields::declarations, m_declarations);
    return cont;
}

void VariableDeclaration::updatePathFromOwner(const Path &p)
{
    BaseT::updatePathFromOwner(p);
    m_declarations.updatePathFromOwner(p.withField(Fields::declarations));
}

void VariableDeclaration::createFileLocations(const FileLocations::Tree &base)
{
    BaseT::createFileLocations(base);
    m_declarations.createFileLocations(base);
}

bool ReturnStatement::iterateDirectSubpaths(const DomItem &self, DirectVisitor visitor) const
{
    bool cont = true;
    cont &= wrap(self, visitor, Fields::expression, m_expression);
    return cont;
}

void ReturnStatement::updatePathFromOwner(const Path &p)
{
    BaseT::updatePathFromOwner(p);
    if (auto ptr = m_expression.base())
        ptr->updatePathFromOwner(p.withField(Fields::expression));
}

void ReturnStatement::createFileLocations(const FileLocations::Tree &base)
{
    BaseT::createFileLocations(base);
    if (auto ptr = m_expression.base())
        ptr->createFileLocations(base);
}

void ScriptList::replaceKindForGenericChildren(DomType oldType, DomType newType)
{
    for (auto &it : m_list) {
        if (auto current = it.data()) {
            if (auto genericElement =
                        std::get_if<std::shared_ptr<ScriptElements::GenericScriptElement>>(
                                &*current)) {
                if ((*genericElement)->kind() == oldType)
                    (*genericElement)->setKind(newType);
            }
        }
    }
}
