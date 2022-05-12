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
    void postVisitResolve(const QHash<QQmlJSScope::ConstPtr, QList<QQmlJSMetaPropertyBinding>>
                                  &qmlIrOrderedBindings);

public:
    QmltcVisitor(QQmlJSImporter *importer, QQmlJSLogger *logger,
                 const QString &implicitImportDirectory,
                 const QStringList &qmldirFiles = QStringList());

    bool visit(QQmlJS::AST::UiObjectDefinition *) override;
    void endVisit(QQmlJS::AST::UiObjectDefinition *) override;

    bool visit(QQmlJS::AST::UiObjectBinding *) override;
    void endVisit(QQmlJS::AST::UiObjectBinding *) override;

    bool visit(QQmlJS::AST::UiScriptBinding *) override;

    bool visit(QQmlJS::AST::UiPublicMember *) override;

    bool visit(QQmlJS::AST::UiInlineComponent *) override;

    void endVisit(QQmlJS::AST::UiProgram *) override;

    QList<QQmlJSScope::ConstPtr> qmlTypesWithQmlBases() const { return m_qmlTypesWithQmlBases; }
    QSet<QString> cppIncludeFiles() const { return m_cppIncludes; }

    qsizetype creationIndex(const QQmlJSScope::ConstPtr &type) const
    {
        Q_ASSERT(m_pureTypeIndices.contains(type));
        Q_ASSERT(type->scopeType() == QQmlJSScope::QMLScope);
        return m_pureTypeIndices[type];
    }

    qsizetype qmlComponentIndex(const QQmlJSScope::ConstPtr &type) const
    {
        Q_ASSERT(type->scopeType() == QQmlJSScope::QMLScope);
        return m_syntheticTypeIndices.value(type, -1);
    }

    qsizetype qmlIrObjectIndex(const QQmlJSScope::ConstPtr &type) const
    {
        Q_ASSERT(type->scopeType() == QQmlJSScope::QMLScope);
        Q_ASSERT(m_qmlIrObjectIndices.contains(type));
        return m_qmlIrObjectIndices[type];
    }

    /*! \internal
        Returns a runtime index counterpart of `id: foo` for \a type. Returns -1
        if \a type does not have an id.
    */
    int runtimeId(const QQmlJSScope::ConstPtr &type) const
    {
        // NB: this function is expected to be called for "pure" types
        Q_ASSERT(!m_typesWithId.contains(type) || m_typesWithId[type] != -1);
        return m_typesWithId.value(type, -1);
    }

    /*! \internal
        Returns all encountered QML types.
    */
    QList<QQmlJSScope::ConstPtr> allQmlTypes() const { return qmlTypes(); }

    /*! \internal
        Returns encountered QML types which return \c false in
        \c{isComponentRootElement()}. Called "pure", because these are the ones
        that are not wrapped into QQmlComponent. Pure QML types can be created
        through direct constructor invocation.
    */
    QList<QQmlJSScope::ConstPtr> pureQmlTypes() const { return m_pureQmlTypes; }

    /*! \internal
        Returns \c true when \a type has deferred bindings. Returns \c false
        otherwise.
    */
    bool hasDeferredBindings(const QQmlJSScope::ConstPtr &type) const
    {
        return m_typesWithDeferredBindings.contains(type);
    }

protected:
    QStringList m_qmlTypeNames; // names of QML types arranged as a stack
    QHash<QString, int> m_qmlTypeNameCounts;
    QList<QQmlJSScope::ConstPtr> m_qmlTypesWithQmlBases; // QML types with composite/QML base types
    QSet<QString> m_cppIncludes; // all C++ includes found from QQmlJSScope hierarchy
    QList<QQmlJSScope::ConstPtr> m_pureQmlTypes; // the ones not under QQmlComponent

    QHash<QQmlJSScope::ConstPtr, qsizetype> m_pureTypeIndices;
    QHash<QQmlJSScope::ConstPtr, qsizetype> m_syntheticTypeIndices;
    QHash<QQmlJSScope::ConstPtr, qsizetype> m_qmlIrObjectIndices;

    QSet<QQmlJSScope::ConstPtr> m_typesWithDeferredBindings;

    // prefer allQmlTypes or pureQmlTypes. this function is misleading in qmltc
    QList<QQmlJSScope::ConstPtr> qmlTypes() const { return QQmlJSImportVisitor::qmlTypes(); }

    QHash<QQmlJSScope::ConstPtr, int> m_typesWithId;
};

QT_END_NAMESPACE

#endif // QMLTCVISITOR_H
