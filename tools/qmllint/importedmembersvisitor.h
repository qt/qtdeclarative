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

#include "scopetree.h"
#include "qcoloroutput.h"

#include <private/qqmljsast_p.h>

class ImportedMembersVisitor : public QQmlJS::AST::Visitor
{
public:
    ImportedMembersVisitor(ColorOutput *colorOut) :
        m_colorOut(colorOut)
    {}

    ScopeTree *result(const QString &scopeName) const;

private:
    bool visit(QQmlJS::AST::UiObjectDefinition *) override;
    void endVisit(QQmlJS::AST::UiObjectDefinition *) override;
    bool visit(QQmlJS::AST::UiPublicMember *) override;
    bool visit(QQmlJS::AST::UiSourceElement *) override;
    bool visit(QQmlJS::AST::UiScriptBinding *) override;
    void throwRecursionDepthError() override;

    ScopeTree::Ptr currentObject() const { return m_currentObjects.back(); }

    QVector<ScopeTree::Ptr> m_currentObjects;
    ScopeTree::ConstPtr m_rootObject;
    QHash<QString, ScopeTree::Ptr> m_objects;

    ColorOutput *m_colorOut = nullptr;
};

#endif // IMPORTEDMEMBERSVISITOR_H
