// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef QMLTCVISITOR_H
#define QMLTCVISITOR_H

#include <QtCore/qstring.h>
#include <QtCore/qstringlist.h>
#include <QtCore/qlist.h>

#include <QtQml/private/qqmlirbuilder_p.h>
#include <private/qqmljsimportvisitor_p.h>
#include <private/qqmljslogger_p.h>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

class QmltcVisitor : public QQmlJSImportVisitor
{
    void findCppIncludes();
    void postVisitResolve(const QHash<QQmlJSScope::ConstPtr, QList<QQmlJSMetaPropertyBinding>>
                                  &qmlIrOrderedBindings);
    void setupAliases();
    void checkForNamingCollisionsWithCpp(const QQmlJSScope::ConstPtr &type);
    void setRootFilePath();

    QString sourceDirectoryPath(const QString &path);

    using InlineComponentOrDocumentRootName = QQmlJSScope::InlineComponentOrDocumentRootName;

public:
    QmltcVisitor(const QQmlJSScope::Ptr &target, QQmlJSImporter *importer, QQmlJSLogger *logger,
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

    QList<QQmlJSScope::ConstPtr>
    qmlTypesWithQmlBases(const InlineComponentOrDocumentRootName &inlinedComponentName) const
    {
        return m_qmlTypesWithQmlBases.value(inlinedComponentName);
    }
    QSet<QString> cppIncludeFiles() const { return m_cppIncludes; }

    qsizetype creationIndex(const QQmlJSScope::ConstPtr &type) const
    {
        Q_ASSERT(type->scopeType() == QQmlSA::ScopeType::QMLScope);
        return m_creationIndices.value(type, -1);
    }

    qsizetype typeCount(const InlineComponentOrDocumentRootName &inlineComponent) const
    {
        return m_inlineComponentTypeCount.value(inlineComponent);
    }

    qsizetype qmlComponentIndex(const QQmlJSScope::ConstPtr &type) const
    {
        Q_ASSERT(type->scopeType() == QQmlSA::ScopeType::QMLScope);
        return m_syntheticTypeIndices.value(type, -1);
    }

    qsizetype qmlIrObjectIndex(const QQmlJSScope::ConstPtr &type) const
    {
        Q_ASSERT(type->scopeType() == QQmlSA::ScopeType::QMLScope);
        Q_ASSERT(m_qmlIrObjectIndices.contains(type));
        return m_qmlIrObjectIndices.value(type, -1);
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
        Returns QML types which return \c false in
        \c{isComponentRootElement()}. The QHash key are the enclosing inline component
        or the root document name when not beloning to any inline component.
        Called "pure", because these are the ones
        that are not wrapped into QQmlComponent. Pure QML types can be created
        through direct constructor invocation.
    */
    QList<QQmlJSScope::ConstPtr>
    pureQmlTypes(const InlineComponentOrDocumentRootName &inlineComponent) const
    {
        return m_pureQmlTypes[inlineComponent];
    }

    /*!
     * \internal
     * Returns a list of the inline components. This list ends with the document root.
     */
    QList<InlineComponentOrDocumentRootName> inlineComponentNames() const
    {
        return m_inlineComponentNames;
    }
    QQmlJSScope::ConstPtr
    inlineComponent(const InlineComponentOrDocumentRootName &inlineComponentName) const
    {
        return m_inlineComponents.value(inlineComponentName);
    }

    /*! \internal
        Returns \c true when \a type has deferred bindings. Returns \c false
        otherwise.
    */
    bool hasDeferredBindings(const QQmlJSScope::ConstPtr &type) const
    {
        return m_typesWithDeferredBindings.contains(type);
    }

    enum Mode { Import, Compile };
    void setMode(Mode mode) { m_mode = mode; }

protected:
    QStringList m_qmlTypeNames; // names of QML types arranged as a stack
    QHash<QString, int> m_qmlTypeNameCounts;
    /*!
     * \internal
     *  QML types with composite/QML base types, mapped from inline component name to types
     */
    QHash<InlineComponentOrDocumentRootName, QList<QQmlJSScope::ConstPtr>> m_qmlTypesWithQmlBases;
    QSet<QString> m_cppIncludes; // all C++ includes found from QQmlJSScope hierarchy
    QHash<InlineComponentOrDocumentRootName, QList<QQmlJSScope::ConstPtr>>
            m_pureQmlTypes; // the ones not under QQmlComponent
    /*!
     * \internal
     *  List of the names of the inline components, useful when iterating over QHash that
     *  uses those names as keys. Ends with the the document root.
     */
    QList<InlineComponentOrDocumentRootName> m_inlineComponentNames;
    /*!
     * \internal
     *  Map inline component names to the corresponding type, and the document root
     *  name to all types not belonging to any inline component.
     */
    QHash<InlineComponentOrDocumentRootName, QQmlJSScope::Ptr> m_inlineComponents;
    /*!
     * \internal
     *  Map types to their creation indices. Childrens are stored at their creation index in
     *  a QObject* array either in the document root or in the inline component they belong to.
     *  Therefore two types in the same file might have the same creation index, if they belong
     *  to different inline components.
     */
    QHash<QQmlJSScope::ConstPtr, qsizetype> m_creationIndices;
    /*!
     * \internal
     *  Counts the types (pure qml types and explicit/implicit components) per inline component.
     *  Needed to set the size of the QObject* array in the document root or the inline component
     *  they belong to.
     */
    QHash<InlineComponentOrDocumentRootName, qsizetype> m_inlineComponentTypeCount;
    QHash<QQmlJSScope::ConstPtr, qsizetype> m_syntheticTypeIndices;
    QHash<QQmlJSScope::ConstPtr, qsizetype> m_qmlIrObjectIndices;

    QSet<QQmlJSScope::ConstPtr> m_typesWithDeferredBindings;

    // prefer allQmlTypes or pureQmlTypes. this function is misleading in qmltc
    QList<QQmlJSScope::ConstPtr> qmlTypes() const { return QQmlJSImportVisitor::qmlTypes(); }

    QHash<QQmlJSScope::ConstPtr, int> m_typesWithId;

    Mode m_mode = Mode::Import;
};

QT_END_NAMESPACE

#endif // QMLTCVISITOR_H
