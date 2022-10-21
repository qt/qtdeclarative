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

#ifndef FINDUNQUALIFIED_H
#define FINDUNQUALIFIED_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.

#include "checkidentifiers.h"

#include <QtQmlCompiler/private/qcoloroutput_p.h>
#include <QtQmlCompiler/private/qqmljstypedescriptionreader_p.h>
#include <QtQmlCompiler/private/qqmljsscope_p.h>
#include <QtQmlCompiler/private/qqmljsimporter_p.h>
#include <QtQmlCompiler/private/qqmljsimportvisitor_p.h>

#include <QtQml/private/qqmldirparser_p.h>
#include <QtQml/private/qqmljsastvisitor_p.h>
#include <QtQml/private/qqmljsast_p.h>

#include <QtCore/qscopedpointer.h>

class FindWarningVisitor : public QQmlJSImportVisitor
{
    Q_DISABLE_COPY_MOVE(FindWarningVisitor)
public:
    explicit FindWarningVisitor(QQmlJSImporter *importer, QStringList qmltypesFiles, QString code,
                                QList<QQmlJS::SourceLocation> comments, QString fileName,
                                bool silent);
    ~FindWarningVisitor() override = default;
    bool check();

private:
    MemberAccessChains m_memberAccessChains;

    QQmlJS::AST::ExpressionNode *m_fieldMemberBase = nullptr;

    void parseComments(const QList<QQmlJS::SourceLocation> &comments);

    // work around compiler error in clang11
    using QQmlJSImportVisitor::visit;
    using QQmlJSImportVisitor::endVisit;

    bool visit(QQmlJS::AST::UiObjectDefinition *uiod) override;
    void endVisit(QQmlJS::AST::UiObjectDefinition *uiod) override;

    // expression handling
    bool visit(QQmlJS::AST::IdentifierExpression *idexp) override;

    bool visit(QQmlJS::AST::PatternElement *) override;
    void endVisit(QQmlJS::AST::FieldMemberExpression *) override;

    void endVisit(QQmlJS::AST::BinaryExpression *) override;
};

#endif // FINDUNQUALIFIED_H
