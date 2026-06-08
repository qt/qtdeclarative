// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
// Qt-Security score:significant

#ifndef QQMLTCCOMPILER_P_H
#define QQMLTCCOMPILER_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <private/qqmltctyperesolver_p.h>
#include <private/qqmltcvisitor_p.h>
#include <private/qqmltcoutputir_p.h>

#include <QtCore/qcommandlineparser.h>
#include <QtCore/qcoreapplication.h>
#include <QtCore/qstring.h>
#include <QtCore/qhash.h>

#include <private/qqmljslogger_p.h>

QT_BEGIN_NAMESPACE

namespace QQmltc {

struct CompilerInfo
{
    QString outputCppFile;
    QString outputHFile;
    QString outputNamespace;
    QString resourcePath;
    QString exportMacro;
    QString exportInclude;
};

class Compiler
{
    using InlineComponentOrDocumentRootName = QQmlJSScope::InlineComponentOrDocumentRootName;
    using InlineComponentNameType = QQmlJSScope::InlineComponentNameType;
    using RootDocumentNameType = QQmlJSScope::RootDocumentNameType;

public:
    Compiler(const QString &url, TypeResolver *resolver, Visitor *visitor,
             QQmlJSLogger *logger);
    void compile(const CompilerInfo &info);

    ~Compiler();

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
    TypeResolver *m_typeResolver = nullptr;
    Visitor *m_visitor = nullptr;
    QQmlJSLogger *m_logger = nullptr;
    CompilerInfo m_info {}; // miscellaneous input/output information
    QString m_urlMethodName;
    uint m_currentVariableNumber = 0;

    struct UniqueStringId;
    struct TypeLocalData;
    // per-type, per-property code generation cache of created symbols
    QHash<UniqueStringId, TypeLocalData> m_uniques;

    void compileUrlMethod(Method &urlMethod, const QString &urlMethodName);
    void
    compileType(Type &current, const QQmlJSScope::ConstPtr &type,
                std::function<void(Type &, const QQmlJSScope::ConstPtr &)> compileElements);
    void compileTypeElements(Type &current, const QQmlJSScope::ConstPtr &type);
    void compileEnum(Type &current, const QQmlJSMetaEnum &e);
    void compileMethod(Type &current, const QQmlJSMetaMethod &m,
                       const QQmlJSScope::ConstPtr &owner);
    void compileProperty(Type &current, const QQmlJSMetaProperty &p,
                         const QQmlJSScope::ConstPtr &owner);
    void compileAlias(Type &current, const QQmlJSMetaProperty &alias,
                      const QQmlJSScope::ConstPtr &owner);
    void compileExtraListMethods(Type &current, const QQmlJSMetaProperty &p);

    QString uniqueVariableName(const QString &qmlName)
    {
        QString result = u"m_"_s + QString::number(++m_currentVariableNumber) + qmlName;
        result.replace(u'.', u'_');
        return result;
    }

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

    void processLastListBindings(Type &current, const QQmlJSScope::ConstPtr &type,
                                 const BindingAccessorData &accessor);

    void compileBinding(Type &current, QList<QQmlJSMetaPropertyBinding>::iterator bindingStart,
                        QList<QQmlJSMetaPropertyBinding>::iterator bindingEnd,
                        const QQmlJSScope::ConstPtr &type, const BindingAccessorData &accessor);

    void compileBindingByType(Type &current, const QQmlJSMetaPropertyBinding &binding,
                              const QQmlJSScope::ConstPtr &type,
                              const BindingAccessorData &accessor);

    void compileObjectBinding(Type &current, const QQmlJSMetaPropertyBinding &binding,
                              const QQmlJSScope::ConstPtr &type,
                              const BindingAccessorData &accessor);

    void compileValueSourceOrInterceptorBinding(Type &current,
                                                const QQmlJSMetaPropertyBinding &binding,
                                                const QQmlJSScope::ConstPtr &type,
                                                const BindingAccessorData &accessor);

    void compileAttachedPropertyBinding(Type &current, const QQmlJSMetaPropertyBinding &binding,
                                        const QQmlJSScope::ConstPtr &type,
                                        const BindingAccessorData &accessor);

    void compileGroupPropertyBinding(Type &current, const QQmlJSMetaPropertyBinding &binding,
                                     const QQmlJSScope::ConstPtr &type,
                                     const BindingAccessorData &accessor);

    void compileTranslationBinding(Type &current, const QQmlJSMetaPropertyBinding &binding,
                                   const QQmlJSScope::ConstPtr &type,
                                   const BindingAccessorData &accessor);

    // special case (for simplicity)
    void compileScriptBinding(Type &current, const QQmlJSMetaPropertyBinding &binding,
                              const QString &bindingSymbolName, const QQmlJSScope::ConstPtr &type,
                              const QString &propertyName,
                              const QQmlJSScope::ConstPtr &propertyType,
                              const BindingAccessorData &accessor);

    void compilePropertyInitializer(Type &current, const QQmlJSScope::ConstPtr &type);

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
        UniqueStringId(const Type &context, const QString &property)
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

    struct TypeLocalData
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

} // namespace QQmltc

QT_END_NAMESPACE

#endif // QQMLTCCOMPILER_P_H
