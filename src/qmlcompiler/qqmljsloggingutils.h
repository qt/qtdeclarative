// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef QQMLJSLOGGER_H
#define QQMLJSLOGGER_H

#include <QtCore/QFileInfo>
#include <qtqmlcompilerexports.h>

QT_BEGIN_NAMESPACE

class QQmlJSLoggerPrivate;
class QQmlJSScope;

namespace QQmlSA {
class SourceLocation;
}

namespace QQmlJS {
class LoggerCategoryPrivate;

class Q_QMLCOMPILER_EXPORT LoggerWarningId
{
public:
    constexpr LoggerWarningId(QAnyStringView name) : m_name(name) { }

    const QAnyStringView name() const { return m_name; }

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

class Q_QMLCOMPILER_EXPORT LoggerCategory
{
    Q_DECLARE_PRIVATE(LoggerCategory)

public:
    LoggerCategory();
    LoggerCategory(QString name, QString settingsName, QString description, QtMsgType level,
                   bool ignored = false, bool isDefault = false);
    LoggerCategory(const LoggerCategory &);
    LoggerCategory(LoggerCategory &&) noexcept;
    LoggerCategory &operator=(const LoggerCategory &);
    LoggerCategory &operator=(LoggerCategory &&) noexcept;
    ~LoggerCategory();

    QString name() const;
    QString settingsName() const;
    QString description() const;
    QtMsgType level() const;
    bool isIgnored() const;
    bool isDefault() const;

    LoggerWarningId id() const;

    void setLevel(QtMsgType);
    void setIgnored(bool);

    friend bool operator==(const LoggerCategory &category, const LoggerWarningId &warningId)
    {
        return category.name() == warningId.name();
    }

private:
    std::unique_ptr<QQmlJS::LoggerCategoryPrivate> d_ptr;
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
