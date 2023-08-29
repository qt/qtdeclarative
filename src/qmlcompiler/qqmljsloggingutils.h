// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef QQMLJSLOGGER_H
#define QQMLJSLOGGER_H

#include <QtCore/qanystringview.h>
#include <QtQmlCompiler/qtqmlcompilerexports.h>

QT_BEGIN_NAMESPACE

class QQmlJSLoggerPrivate;
class QQmlJSScope;

namespace QQmlSA {
class SourceLocation;
}

namespace QQmlJS {

class Q_QMLCOMPILER_EXPORT LoggerWarningId
{
public:
    constexpr LoggerWarningId(QAnyStringView name) : m_name(name) { }

    QAnyStringView name() const { return m_name; }

private:
    friend bool operator==(const LoggerWarningId &a, const LoggerWarningId &b)
    {
        return a.m_name == b.m_name;
    }

    friend bool operator!=(const LoggerWarningId &a, const LoggerWarningId &b)
    {
        return a.m_name != b.m_name;
    }
    const QAnyStringView m_name;
};

} // namespace QQmlJS

extern const Q_QMLCOMPILER_EXPORT QQmlJS::LoggerWarningId qmlRequired;
extern const Q_QMLCOMPILER_EXPORT QQmlJS::LoggerWarningId qmlUnresolvedAlias;
extern const Q_QMLCOMPILER_EXPORT QQmlJS::LoggerWarningId qmlAliasCycle;
extern const Q_QMLCOMPILER_EXPORT QQmlJS::LoggerWarningId qmlImport;
extern const Q_QMLCOMPILER_EXPORT QQmlJS::LoggerWarningId qmlRecursionDepthErrors;
extern const Q_QMLCOMPILER_EXPORT QQmlJS::LoggerWarningId qmlWith;
extern const Q_QMLCOMPILER_EXPORT QQmlJS::LoggerWarningId qmlInheritanceCycle;
extern const Q_QMLCOMPILER_EXPORT QQmlJS::LoggerWarningId qmlDeprecated;
extern const Q_QMLCOMPILER_EXPORT QQmlJS::LoggerWarningId qmlSignalParameters;
extern const Q_QMLCOMPILER_EXPORT QQmlJS::LoggerWarningId qmlMissingType;
extern const Q_QMLCOMPILER_EXPORT QQmlJS::LoggerWarningId qmlUnresolvedType;
extern const Q_QMLCOMPILER_EXPORT QQmlJS::LoggerWarningId qmlIncompatibleType;
extern const Q_QMLCOMPILER_EXPORT QQmlJS::LoggerWarningId qmlMissingProperty;
extern const Q_QMLCOMPILER_EXPORT QQmlJS::LoggerWarningId qmlRestrictedType;
extern const Q_QMLCOMPILER_EXPORT QQmlJS::LoggerWarningId qmlPrefixedImportType;
extern const Q_QMLCOMPILER_EXPORT QQmlJS::LoggerWarningId qmlNonListProperty;
extern const Q_QMLCOMPILER_EXPORT QQmlJS::LoggerWarningId qmlReadOnlyProperty;
extern const Q_QMLCOMPILER_EXPORT QQmlJS::LoggerWarningId qmlDuplicatePropertyBinding;
extern const Q_QMLCOMPILER_EXPORT QQmlJS::LoggerWarningId qmlDuplicatedName;
extern const Q_QMLCOMPILER_EXPORT QQmlJS::LoggerWarningId qmlDeferredPropertyId;
extern const Q_QMLCOMPILER_EXPORT QQmlJS::LoggerWarningId qmlUnqualified;
extern const Q_QMLCOMPILER_EXPORT QQmlJS::LoggerWarningId qmlUnusedImports;
extern const Q_QMLCOMPILER_EXPORT QQmlJS::LoggerWarningId qmlMultilineStrings;
extern const Q_QMLCOMPILER_EXPORT QQmlJS::LoggerWarningId qmlSyntax;
extern const Q_QMLCOMPILER_EXPORT QQmlJS::LoggerWarningId qmlSyntaxIdQuotation;
extern const Q_QMLCOMPILER_EXPORT QQmlJS::LoggerWarningId qmlSyntaxDuplicateIds;
extern const Q_QMLCOMPILER_EXPORT QQmlJS::LoggerWarningId qmlCompiler;
extern const Q_QMLCOMPILER_EXPORT QQmlJS::LoggerWarningId qmlAttachedPropertyReuse;
extern const Q_QMLCOMPILER_EXPORT QQmlJS::LoggerWarningId qmlPlugin;
extern const Q_QMLCOMPILER_EXPORT QQmlJS::LoggerWarningId qmlVarUsedBeforeDeclaration;
extern const Q_QMLCOMPILER_EXPORT QQmlJS::LoggerWarningId qmlInvalidLintDirective;
extern const Q_QMLCOMPILER_EXPORT QQmlJS::LoggerWarningId qmlUseProperFunction;
extern const Q_QMLCOMPILER_EXPORT QQmlJS::LoggerWarningId qmlAccessSingleton;
extern const Q_QMLCOMPILER_EXPORT QQmlJS::LoggerWarningId qmlTopLevelComponent;
extern const Q_QMLCOMPILER_EXPORT QQmlJS::LoggerWarningId qmlUncreatableType;
extern const Q_QMLCOMPILER_EXPORT QQmlJS::LoggerWarningId qmlMissingEnumEntry;

QT_END_NAMESPACE

#endif // QQMLJSLOGGER_H
