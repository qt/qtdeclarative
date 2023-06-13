// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef QMLTCCOMPILER_H
#define QMLTCCOMPILER_H

#include "qmltctyperesolver.h"
#include "qmltcvisitor.h"
#include "qmltcoutputir.h"

#include <QtCore/qcommandlineparser.h>
#include <QtCore/qcoreapplication.h>
#include <QtCore/qstring.h>
#include <QtCore/qhash.h>

#include <private/qqmljslogger_p.h>

#include <memory>

QT_BEGIN_NAMESPACE

struct QmltcCompilerInfo
{
    QString outputCppFile;
    QString outputHFile;
    QString outputNamespace;
    QString resourcePath;
    QString exportMacro;
    QString exportInclude;
};

class QmltcCompiler
{
    using InlineComponentOrDocumentRootName = QQmlJSScope::InlineComponentOrDocumentRootName;
    using InlineComponentNameType = QQmlJSScope::InlineComponentNameType;
    using RootDocumentNameType = QQmlJSScope::RootDocumentNameType;

public:
    QmltcCompiler(const QString &url, QmltcTypeResolver *resolver, QmltcVisitor *visitor,
                  QQmlJSLogger *logger);
    void compile(const QmltcCompilerInfo &info);

    ~QmltcCompiler();

    /*! \internal

        Returns \c true if \a binding is considered complex by the compiler
        (requires special code generation)
    */
    static bool isComplexBinding(const QQmlJSMetaPropertyBinding &binding)
    {
        // TODO: translation bindings (once supported) are also complex?
        return binding.bindingType() == QQmlSA::BindingType::Script;
    }

private:
    QString m_url; // QML input file url
    QmltcTypeResolver *m_typeResolver = nullptr;
    QmltcVisitor *m_visitor = nullptr;
    QQmlJSLogger *m_logger = nullptr;
    QmltcCompilerInfo m_info {}; // miscellaneous input/output information
    QString m_urlMethodName;

    struct UniqueStringId;
    struct QmltcTypeLocalData;
    // per-type, per-property code generation cache of created symbols
    QHash<UniqueStringId, QmltcTypeLocalData> m_uniques;

    void compileUrlMethod(QmltcMethod &urlMethod, const QString &urlMethodName);
    void
    compileType(QmltcType &current, const QQmlJSScope::ConstPtr &type,
                std::function<void(QmltcType &, const QQmlJSScope::ConstPtr &)> compileElements);
    void compileTypeElements(QmltcType &current, const QQmlJSScope::ConstPtr &type);
    void compileEnum(QmltcType &current, const QQmlJSMetaEnum &e);
    void compileMethod(QmltcType &current, const QQmlJSMetaMethod &m,
                       const QQmlJSScope::ConstPtr &owner);
    void compileProperty(QmltcType &current, const QQmlJSMetaProperty &p,
                         const QQmlJSScope::ConstPtr &owner);
    void compileAlias(QmltcType &current, const QQmlJSMetaProperty &alias,
                      const QQmlJSScope::ConstPtr &owner);
    void compileExtraListMethods(QmltcType &current, const QQmlJSMetaProperty &p);

    /*!
        \internal

        Helper structure that holds the information necessary for most bindings,
        such as accessor name, which is used to reference the properties. For
        example:
        > (accessor.name)->(propertyName) results in "this->myProperty"

        This data is also used in more advanced scenarios by attached and
        grouped properties
    */
    struct BindingAccessorData
    {
        QQmlJSScope::ConstPtr scope; // usually the current type
        QString name = QStringLiteral("this");
        QString propertyName = QString();
        bool isValueType = false;
    };

    QStringList unprocessedListBindings;
    QQmlJSMetaProperty unprocessedListProperty;

    void processLastListBindings(QmltcType &current, const QQmlJSScope::ConstPtr &type,
                                 const BindingAccessorData &accessor);

    void compileBinding(QmltcType &current, QList<QQmlJSMetaPropertyBinding>::iterator bindingStart,
                        QList<QQmlJSMetaPropertyBinding>::iterator bindingEnd,
                        const QQmlJSScope::ConstPtr &type, const BindingAccessorData &accessor);

    void compileBindingByType(QmltcType &current, const QQmlJSMetaPropertyBinding &binding,
                              const QQmlJSScope::ConstPtr &type,
                              const BindingAccessorData &accessor);

    void compileObjectBinding(QmltcType &current, const QQmlJSMetaPropertyBinding &binding,
                              const QQmlJSScope::ConstPtr &type,
                              const BindingAccessorData &accessor);

    void compileValueSourceOrInterceptorBinding(QmltcType &current,
                                                const QQmlJSMetaPropertyBinding &binding,
                                                const QQmlJSScope::ConstPtr &type,
                                                const BindingAccessorData &accessor);

    void compileAttachedPropertyBinding(QmltcType &current,
                                        const QQmlJSMetaPropertyBinding &binding,
                                        const QQmlJSScope::ConstPtr &type,
                                        const BindingAccessorData &accessor);

    void compileGroupPropertyBinding(QmltcType &current, const QQmlJSMetaPropertyBinding &binding,
                                     const QQmlJSScope::ConstPtr &type,
                                     const BindingAccessorData &accessor);

    void compileTranslationBinding(QmltcType &current, const QQmlJSMetaPropertyBinding &binding,
                                   const QQmlJSScope::ConstPtr &type,
                                   const BindingAccessorData &accessor);

    // special case (for simplicity)
    void compileScriptBinding(QmltcType &current, const QQmlJSMetaPropertyBinding &binding,
                              const QString &bindingSymbolName, const QQmlJSScope::ConstPtr &type,
                              const QString &propertyName,
                              const QQmlJSScope::ConstPtr &propertyType,
                              const BindingAccessorData &accessor);

    /*!
        \internal
        Helper structure that acts as a key in a hash-table of
        QmltcType-specific data (such as local variable names). Using a
        hash-table allows to avoid creating the same variables multiple times
        during binding compilation, which leads to better code generation and
        faster object creation. This is really something that the QML optimizer
        should do, but we have only this home-grown alternative at the moment
    */
    struct UniqueStringId
    {
        QString unique;
        UniqueStringId(const QmltcType &context, const QString &property)
            : unique(context.cppType + u"_" + property) // this is unique enough
        {
            Q_ASSERT(!context.cppType.isEmpty());
            Q_ASSERT(!property.isEmpty());
        }
        friend bool operator==(const UniqueStringId &x, const UniqueStringId &y)
        {
            return x.unique == y.unique;
        }
        friend bool operator!=(const UniqueStringId &x, const UniqueStringId &y)
        {
            return !(x == y);
        }
        friend size_t qHash(const UniqueStringId &x, size_t seed = 0)
        {
            return qHash(x.unique, seed);
        }
    };

    struct QmltcTypeLocalData
    {
        // empty QString() means that the local data is not present (yet)
        QString qmlListVariableName;
        QString onAssignmentObjectName;
        QString attachedVariableName;
    };

    QHash<QString, qsizetype> m_symbols;
    QString newSymbol(const QString &base);

    bool hasErrors() const { return m_logger->hasErrors(); }
    void recordError(const QQmlJS::SourceLocation &location, const QString &message,
                     QQmlJS::LoggerWarningId id = qmlCompiler)
    {
        // pretty much any compiler error is a critical error (we cannot
        // generate code - compilation fails)
        m_logger->log(message, id, location);
    }
    void recordError(const QV4::CompiledData::Location &location, const QString &message,
                     QQmlJS::LoggerWarningId id = qmlCompiler)
    {
        recordError(QQmlJS::SourceLocation { 0, 0, location.line(), location.column() }, message,
                    id);
    }
};

QT_END_NAMESPACE

#endif // QMLTCCOMPILER_H
