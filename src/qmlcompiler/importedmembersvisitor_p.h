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

#ifndef IMPORTEDMEMBERSVISITOR_H
#define IMPORTEDMEMBERSVISITOR_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.

#include "qqmljsscope_p.h"

#include <private/qqmljsast_p.h>

class ImportedMembersVisitor : public QQmlJS::AST::Visitor
{
public:
    QQmlJSScope::Ptr result(const QString &scopeName) const;
    QStringList errors() const { return m_errors; }

private:
    bool visit(QQmlJS::AST::UiObjectDefinition *) override;
    void endVisit(QQmlJS::AST::UiObjectDefinition *) override;
    bool visit(QQmlJS::AST::UiPublicMember *) override;
    bool visit(QQmlJS::AST::UiSourceElement *) override;
    bool visit(QQmlJS::AST::UiScriptBinding *) override;
    bool visit(QQmlJS::AST::UiEnumDeclaration *uied) override;
    void throwRecursionDepthError() override;

    QQmlJSScope::Ptr currentObject() const { return m_currentObjects.back(); }

    QVector<QQmlJSScope::Ptr> m_currentObjects;
    QQmlJSScope::ConstPtr m_rootObject;
    QHash<QString, QQmlJSScope::Ptr> m_objects;

    QStringList m_errors;
};

#endif // IMPORTEDMEMBERSVISITOR_H
