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

#ifndef QMLTCVISITOR_H
#define QMLTCVISITOR_H

#include <QtCore/qstring.h>
#include <QtCore/qstringlist.h>
#include <QtCore/qlist.h>

#include <QtQml/private/qqmlirbuilder_p.h>
#include <private/qqmljsimportvisitor_p.h>
#include <private/qqmljslogger_p.h>

QT_BEGIN_NAMESPACE

class QmltcVisitor : public QQmlJSImportVisitor
{
    void findCppIncludes();

public:
    QmltcVisitor(QQmlJSImporter *importer, QQmlJSLogger *logger,
                 const QString &implicitImportDirectory,
                 const QStringList &qmldirFiles = QStringList());

    bool visit(QQmlJS::AST::UiObjectDefinition *) override;
    void endVisit(QQmlJS::AST::UiObjectDefinition *) override;

    bool visit(QQmlJS::AST::UiObjectBinding *) override;
    void endVisit(QQmlJS::AST::UiObjectBinding *) override;

    bool visit(QQmlJS::AST::UiPublicMember *) override;

    void endVisit(QQmlJS::AST::UiProgram *) override;

    // NB: overwrite result() method to return ConstPtr
    QQmlJSScope::ConstPtr result() const { return QQmlJSImportVisitor::result(); }
    QList<QQmlJSScope::ConstPtr> qmlTypesWithQmlBases() const { return m_qmlTypesWithQmlBases; }
    QSet<QString> cppIncludeFiles() const { return m_cppIncludes; }

protected:
    QStringList m_qmlTypeNames; // names of QML types arranged as a stack
    QHash<QString, int> m_qmlTypeNameCounts;
    QList<QQmlJSScope::ConstPtr> m_qmlTypesWithQmlBases; // QML types with composite/QML base types
    QSet<QString> m_cppIncludes; // all C++ includes found from QQmlJSScope hierarchy
};

QT_END_NAMESPACE

#endif // QMLTCVISITOR_H
