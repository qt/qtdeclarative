/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

#include "restructureastvisitor.h"

#include <QList>

RestructureAstVisitor::RestructureAstVisitor(Node *rootNode, bool sortImports) : m_sortImports(sortImports)
{
    rootNode->accept(this);
}

template<typename T>
static QList<T *> findKind(UiObjectMemberList *list)
{
    QList<T *> members;
    for (auto *item = list; item != nullptr; item = item->next) {
        if (cast<T *>(item->member) != nullptr)
            members.append(cast<T *>(item->member));
    }

    return members;
}

template<typename T>
static QList<T *> findKind(UiHeaderItemList *list)
{
    QList<T *> members;
    for (auto *item = list; item != nullptr; item = item->next) {
        if (cast<T *>(item->headerItem) != nullptr)
            members.append(cast<T *>(item->headerItem));
    }

    return members;
}

static QString parseUiQualifiedId(UiQualifiedId *id)
{
    QString name = id->name.toString();
    for (auto *item = id->next; item != nullptr; item = item->next) {
        name += "." + item->name;
    }

    return name;
}

void RestructureAstVisitor::endVisit(UiHeaderItemList *node)
{
    QList<Node *> correctOrder;

    auto imports = findKind<UiImport>(node);

    if (!m_sortImports)
        return;

    // Sort imports
    std::sort(imports.begin(), imports.end(), [](UiImport *a, UiImport *b)
    {
        auto nameA = a->fileName.isEmpty() ? parseUiQualifiedId(a->importUri)
                                           : a->fileName.toString();
        auto nameB = b->fileName.isEmpty() ? parseUiQualifiedId(b->importUri)
                                           : b->fileName.toString();

        return nameA < nameB;
    });

    // Add imports
    for (auto *import : imports)
        correctOrder.append(import);

    // Add all the other items
    for (auto *item = node; item != nullptr; item = item->next) {
        if (!correctOrder.contains(item->headerItem))
            correctOrder.append(item->headerItem);
    }

    // Rebuild member list from correctOrder
    for (auto *item = node; item != nullptr; item = item->next) {
        item->headerItem = correctOrder.front();
        correctOrder.pop_front();
    }
}

void RestructureAstVisitor::endVisit(UiObjectMemberList *node)
{
    QList<UiObjectMember*> correctOrder;

    auto enumDeclarations = findKind<UiEnumDeclaration>(node);
    auto scriptBindings = findKind<UiScriptBinding>(node);
    auto arrayBindings = findKind<UiArrayBinding>(node);
    auto publicMembers = findKind<UiPublicMember>(node);
    auto sourceElements = findKind<UiSourceElement>(node);
    auto objectDefinitions = findKind<UiObjectDefinition>(node);

    // This structure is based on https://doc.qt.io/qt-5/qml-codingconventions.html

    // 1st id
    for (auto *binding : scriptBindings) {
        if (binding->qualifiedId->name == "id") {
            correctOrder.append(binding);

            scriptBindings.removeOne(binding);
            break;
        }
    }

    // 2nd enums
    for (auto *enumDeclaration : enumDeclarations)
        correctOrder.append(enumDeclaration);

    // 3rd property declarations
    for (auto *publicMember : publicMembers) {
        if (publicMember->type != UiPublicMember::Property)
            continue;

        correctOrder.append(publicMember);
    }

    // 4th signals
    for (auto *publicMember : publicMembers) {
        if (publicMember->type != UiPublicMember::Signal)
            continue;

        correctOrder.append(publicMember);
    }

    // 5th functions
    for (auto *source : sourceElements)
        correctOrder.append(source);

    // 6th properties
    for (auto *binding : scriptBindings)
        correctOrder.append(binding);

    for (auto *binding : arrayBindings)
        correctOrder.append(binding);

    // 7th child objects
    for (auto *objectDefinition : objectDefinitions)
        correctOrder.append(objectDefinition);

    // 8th all the rest
    for (auto *item = node; item != nullptr; item = item->next) {
        if (!correctOrder.contains(item->member))
            correctOrder.append(item->member);
    }

    // Rebuild member list from correctOrder
    for (auto *item = node; item != nullptr; item = item->next) {
        item->member = correctOrder.front();
        correctOrder.pop_front();
    }
}
