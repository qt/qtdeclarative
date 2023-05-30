// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

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
static bool wrap(QQmlJS::Dom::DomItem &self, QQmlJS::Dom::DirectVisitor visitor, QStringView field,
                 const ScriptElementVariant &value)
{
    if (!value)
        return true;

    const bool b =
            self.dvItemField(visitor, field, [&self, field, &value]() -> QQmlJS::Dom::DomItem {
                const QQmlJS::Dom::Path pathFromOwner{ self.pathFromOwner().field(field) };
                return self.subScriptElementWrapperItem(value);
            });
    return b;
}

/*!
   \internal
    Helper for fields with lists in iterateDirectSubpaths.
 */
static bool wrap(QQmlJS::Dom::DomItem &self, QQmlJS::Dom::DirectVisitor visitor, QStringView field,
                 const ScriptList &value)
{
    const bool b =
            self.dvItemField(visitor, field, [&self, field, &value]() -> QQmlJS::Dom::DomItem {
                const QQmlJS::Dom::Path pathFromOwner{ self.pathFromOwner().field(field) };
                return self.subListItem(value.asList(pathFromOwner));
            });
    return b;
}

bool GenericScriptElement::iterateDirectSubpaths(DomItem &self, DirectVisitor visitor)
{
    bool cont = true;
    for (auto it = m_children.begin(); it != m_children.end(); ++it) {
        cont &= std::visit(
                [&self, &visitor, &it](auto &&e) { return wrap(self, visitor, it->first, e); },
                it->second);
    }
    return cont;
}

void GenericScriptElement::updatePathFromOwner(Path p)
{
    BaseT::updatePathFromOwner(p);
    for (auto it = m_children.begin(); it != m_children.end(); ++it) {
        std::visit(qOverloadedVisitor{ [&p, &it](ScriptElementVariant &e) {
                                          e.base()->updatePathFromOwner(p.field(it->first));
                                      },
                                       [&p, &it](ScriptList &list) {
                                           list.updatePathFromOwner(p.field(it->first));
                                       } },
                   it->second);
    }
}

void GenericScriptElement::createFileLocations(FileLocations::Tree base)
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

bool BlockStatement::iterateDirectSubpaths(DomItem &self, DirectVisitor visitor)
{
    // TODO: test me
    bool cont = true;
    cont &= wrap(self, visitor, Fields::statements, m_statements);
    return cont;
}

void BlockStatement::updatePathFromOwner(Path p)
{
    BaseT::updatePathFromOwner(p);
    m_statements.updatePathFromOwner(p.field(Fields::statements));
}

void BlockStatement::createFileLocations(FileLocations::Tree base)
{
    BaseT::createFileLocations(base);
    m_statements.createFileLocations(base);
}

bool IdentifierExpression::iterateDirectSubpaths(DomItem &self, DirectVisitor visitor)
{
    bool cont = true;
    cont &= self.dvValueField(visitor, Fields::identifier, m_name);
    return cont;
}

bool Literal::iterateDirectSubpaths(DomItem &self, DirectVisitor visitor)
{
    bool cont = true;
    std::visit([&cont, &visitor,
                &self](auto &&e) { cont &= self.dvValueField(visitor, Fields::value, e); },
               m_value);
    return cont;
}

bool IfStatement::iterateDirectSubpaths(DomItem &self, DirectVisitor visitor)
{
    // TODO: test me
    bool cont = true;
    cont &= wrap(self, visitor, Fields::condition, m_condition);
    cont &= wrap(self, visitor, Fields::consequence, m_consequence);
    cont &= wrap(self, visitor, Fields::alternative, m_alternative);
    return cont;
}

void IfStatement::updatePathFromOwner(Path p)
{
    BaseT::updatePathFromOwner(p);
    if (auto ptr = m_condition.base())
        ptr->updatePathFromOwner(p.field(Fields::condition));
    if (auto ptr = m_consequence.base())
        ptr->updatePathFromOwner(p.field(Fields::consequence));
    if (auto ptr = m_alternative.base())
        ptr->updatePathFromOwner(p.field(Fields::alternative));
}

void IfStatement::createFileLocations(FileLocations::Tree base)
{
    BaseT::createFileLocations(base);
    if (auto ptr = m_condition.base())
        ptr->createFileLocations(base);
    if (auto ptr = m_consequence.base())
        ptr->createFileLocations(base);
    if (auto ptr = m_alternative.base())
        ptr->createFileLocations(base);
}

bool ForStatement::iterateDirectSubpaths(DomItem &self, DirectVisitor visitor)
{
    bool cont = true;
    cont &= wrap(self, visitor, Fields::initializer, m_initializer);
    cont &= wrap(self, visitor, Fields::declarations, m_declarations);
    cont &= wrap(self, visitor, Fields::condition, m_condition);
    cont &= wrap(self, visitor, Fields::expression, m_expression);
    cont &= wrap(self, visitor, Fields::body, m_body);
    return cont;
}

void ForStatement::updatePathFromOwner(Path p)
{
    BaseT::updatePathFromOwner(p);
    if (auto ptr = m_initializer.base())
        ptr->updatePathFromOwner(p.field(Fields::initializer));
    if (auto ptr = m_declarations.base())
        ptr->updatePathFromOwner(p.field(Fields::declarations));
    if (auto ptr = m_condition.base())
        ptr->updatePathFromOwner(p.field(Fields::condition));
    if (auto ptr = m_expression.base())
        ptr->updatePathFromOwner(p.field(Fields::expression));
    if (auto ptr = m_body.base())
        ptr->updatePathFromOwner(p.field(Fields::body));
}

void ForStatement::createFileLocations(FileLocations::Tree base)
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

bool BinaryExpression::iterateDirectSubpaths(DomItem &self, DirectVisitor visitor)
{
    bool cont = true;
    cont &= wrap(self, visitor, Fields::left, m_left);
    cont &= self.dvValueField(visitor, Fields::operation, m_operator);
    cont &= wrap(self, visitor, Fields::right, m_right);
    return cont;
}

void BinaryExpression::updatePathFromOwner(Path p)
{
    BaseT::updatePathFromOwner(p);
    if (auto ptr = m_left.base())
        ptr->updatePathFromOwner(p.field(Fields::left));
    if (auto ptr = m_right.base())
        ptr->updatePathFromOwner(p.field(Fields::right));
}

void BinaryExpression::createFileLocations(FileLocations::Tree base)
{
    BaseT::createFileLocations(base);
    if (auto ptr = m_left.base())
        ptr->createFileLocations(base);
    if (auto ptr = m_right.base())
        ptr->createFileLocations(base);
}

bool VariableDeclarationEntry::iterateDirectSubpaths(DomItem &self, DirectVisitor visitor)
{
    bool cont = true;
    cont &= self.dvValueField(visitor, Fields::scopeType, m_scopeType);
    cont &= wrap(self, visitor, Fields::identifier, m_identifier);
    cont &= wrap(self, visitor, Fields::initializer, m_initializer);
    return cont;
}

void VariableDeclarationEntry::updatePathFromOwner(Path p)
{
    BaseT::updatePathFromOwner(p);
    if (auto ptr = m_identifier.base())
        ptr->updatePathFromOwner(p.field(Fields::identifier));
    if (auto ptr = m_initializer.base())
        ptr->updatePathFromOwner(p.field(Fields::initializer));
}

void VariableDeclarationEntry::createFileLocations(FileLocations::Tree base)
{
    BaseT::createFileLocations(base);
    if (auto ptr = m_identifier.base())
        ptr->createFileLocations(base);
    if (auto ptr = m_initializer.base())
        ptr->createFileLocations(base);
}

bool VariableDeclaration::iterateDirectSubpaths(DomItem &self, DirectVisitor visitor)
{
    bool cont = true;
    cont &= wrap(self, visitor, Fields::declarations, m_declarations);
    return cont;
}

void VariableDeclaration::updatePathFromOwner(Path p)
{
    BaseT::updatePathFromOwner(p);
    m_declarations.updatePathFromOwner(p.field(Fields::declarations));
}

void VariableDeclaration::createFileLocations(FileLocations::Tree base)
{
    BaseT::createFileLocations(base);
    m_declarations.createFileLocations(base);
}

bool ReturnStatement::iterateDirectSubpaths(DomItem &self, DirectVisitor visitor)
{
    bool cont = true;
    cont &= wrap(self, visitor, Fields::expression, m_expression);
    return cont;
}

void ReturnStatement::updatePathFromOwner(Path p)
{
    BaseT::updatePathFromOwner(p);
    m_expression.base()->updatePathFromOwner(p.field(Fields::expression));
}

void ReturnStatement::createFileLocations(FileLocations::Tree base)
{
    BaseT::createFileLocations(base);
    m_expression.base()->createFileLocations(base);
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
