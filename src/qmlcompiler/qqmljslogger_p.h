// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef QQMLJSLOGGER_P_H
#define QQMLJSLOGGER_P_H

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

#include <private/qtqmlcompilerexports_p.h>

#include "qcoloroutput_p.h"

#include <private/qqmljsdiagnosticmessage_p.h>

#include <QtCore/qhash.h>
#include <QtCore/qmap.h>
#include <QtCore/qstring.h>
#include <QtCore/qlist.h>
#include <QtCore/qset.h>
#include <QtCore/QLoggingCategory>

#include <optional>

QT_BEGIN_NAMESPACE

/*!
    \internal
    Used to print the the line containing the location of a certain error
 */
class Q_QMLCOMPILER_PRIVATE_EXPORT IssueLocationWithContext
{
public:
    /*!
       \internal
       \param code: The whole text of a translation unit
       \param location: The location where an error occurred.
     */
    IssueLocationWithContext(QStringView code, const QQmlJS::SourceLocation &location) {
        quint32 before = qMax(0, code.lastIndexOf(QLatin1Char('\n'), location.offset));

        if (before != 0 && before < location.offset)
            before++;

        m_beforeText = code.mid(before, location.offset - before);
        m_issueText = code.mid(location.offset, location.length);
        int after = code.indexOf(QLatin1Char('\n'), location.offset + location.length);
        m_afterText = code.mid(location.offset + location.length,
                                  after - (location.offset+location.length));
    }

    // returns start of the line till first character of location
    QStringView beforeText() const { return m_beforeText; }
    // returns the text at location
    QStringView issueText() const { return m_issueText; }
    // returns any text after location until the end of the line is reached
    QStringView afterText() const { return m_afterText; }

private:
    QStringView m_beforeText;
    QStringView m_issueText;
    QStringView m_afterText;
};

class Q_QMLCOMPILER_PRIVATE_EXPORT QQmlJSFixSuggestion
{
public:
    QQmlJSFixSuggestion() = default;
    QQmlJSFixSuggestion(const QString &fixDescription, const QQmlJS::SourceLocation &location,
                        const QString &replacement = QString())
        : m_location(location)
        , m_fixDescription(fixDescription)
        , m_replacement(replacement)
    {}

    QString fixDescription() const { return m_fixDescription; }
    QQmlJS::SourceLocation location() const { return m_location; }
    QString replacement() const { return m_replacement; }

    void setFilename(const QString &filename) { m_filename = filename; }
    QString filename() const { return m_filename; }

    void setHint(const QString &hint) { m_hint = hint; }
    QString hint() const { return m_hint; }

    void setAutoApplicable(bool autoApply = true) { m_autoApplicable = autoApply; }
    bool isAutoApplicable() const { return m_autoApplicable; }

private:
    QQmlJS::SourceLocation m_location;
    QString m_fixDescription;
    QString m_replacement;
    QString m_filename;
    QString m_hint;
    bool m_autoApplicable = false;
};

class Q_QMLCOMPILER_PRIVATE_EXPORT LoggerWarningId
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

extern const Q_QMLCOMPILER_PRIVATE_EXPORT LoggerWarningId qmlRequired;
extern const Q_QMLCOMPILER_PRIVATE_EXPORT LoggerWarningId qmlUnresolvedAlias;
extern const Q_QMLCOMPILER_PRIVATE_EXPORT LoggerWarningId qmlAliasCycle;
extern const Q_QMLCOMPILER_PRIVATE_EXPORT LoggerWarningId qmlImport;
extern const Q_QMLCOMPILER_PRIVATE_EXPORT LoggerWarningId qmlRecursionDepthErrors;
extern const Q_QMLCOMPILER_PRIVATE_EXPORT LoggerWarningId qmlWith;
extern const Q_QMLCOMPILER_PRIVATE_EXPORT LoggerWarningId qmlInheritanceCycle;
extern const Q_QMLCOMPILER_PRIVATE_EXPORT LoggerWarningId qmlDeprecated;
extern const Q_QMLCOMPILER_PRIVATE_EXPORT LoggerWarningId qmlSignalParameters;
extern const Q_QMLCOMPILER_PRIVATE_EXPORT LoggerWarningId qmlMissingType;
extern const Q_QMLCOMPILER_PRIVATE_EXPORT LoggerWarningId qmlUnresolvedType;
extern const Q_QMLCOMPILER_PRIVATE_EXPORT LoggerWarningId qmlIncompatibleType;
extern const Q_QMLCOMPILER_PRIVATE_EXPORT LoggerWarningId qmlMissingProperty;
extern const Q_QMLCOMPILER_PRIVATE_EXPORT LoggerWarningId qmlRestrictedType;
extern const Q_QMLCOMPILER_PRIVATE_EXPORT LoggerWarningId qmlPrefixedImportType;
extern const Q_QMLCOMPILER_PRIVATE_EXPORT LoggerWarningId qmlNonListProperty;
extern const Q_QMLCOMPILER_PRIVATE_EXPORT LoggerWarningId qmlReadOnlyProperty;
extern const Q_QMLCOMPILER_PRIVATE_EXPORT LoggerWarningId qmlDuplicatePropertyBinding;
extern const Q_QMLCOMPILER_PRIVATE_EXPORT LoggerWarningId qmlDuplicatedName;
extern const Q_QMLCOMPILER_PRIVATE_EXPORT LoggerWarningId qmlDeferredPropertyId;
extern const Q_QMLCOMPILER_PRIVATE_EXPORT LoggerWarningId qmlUnqualified;
extern const Q_QMLCOMPILER_PRIVATE_EXPORT LoggerWarningId qmlUnusedImports;
extern const Q_QMLCOMPILER_PRIVATE_EXPORT LoggerWarningId qmlMultilineStrings;
extern const Q_QMLCOMPILER_PRIVATE_EXPORT LoggerWarningId qmlSyntax;
extern const Q_QMLCOMPILER_PRIVATE_EXPORT LoggerWarningId qmlSyntaxIdQuotation;
extern const Q_QMLCOMPILER_PRIVATE_EXPORT LoggerWarningId qmlSyntaxDuplicateIds;
extern const Q_QMLCOMPILER_PRIVATE_EXPORT LoggerWarningId qmlCompiler;
extern const Q_QMLCOMPILER_PRIVATE_EXPORT LoggerWarningId qmlControlsSanity;
extern const Q_QMLCOMPILER_PRIVATE_EXPORT LoggerWarningId qmlAttachedPropertyReuse;
extern const Q_QMLCOMPILER_PRIVATE_EXPORT LoggerWarningId qmlPlugin;
extern const Q_QMLCOMPILER_PRIVATE_EXPORT LoggerWarningId qmlVarUsedBeforeDeclaration;
extern const Q_QMLCOMPILER_PRIVATE_EXPORT LoggerWarningId qmlInvalidLintDirective;
extern const Q_QMLCOMPILER_PRIVATE_EXPORT LoggerWarningId qmlUseProperFunction;
extern const Q_QMLCOMPILER_PRIVATE_EXPORT LoggerWarningId qmlAccessSingleton;
extern const Q_QMLCOMPILER_PRIVATE_EXPORT LoggerWarningId qmlTopLevelComponent;
extern const Q_QMLCOMPILER_PRIVATE_EXPORT LoggerWarningId qmlUncreatableType;

struct Message : public QQmlJS::DiagnosticMessage
{
    // This doesn't need to be an owning-reference since the string is expected to outlive any
    // Message object by virtue of coming from a LoggerWarningId.
    QAnyStringView id;
    std::optional<QQmlJSFixSuggestion> fixSuggestion;
};

class Q_QMLCOMPILER_PRIVATE_EXPORT QQmlJSLogger
{
    Q_DISABLE_COPY_MOVE(QQmlJSLogger)
public:
    struct Category
    {
        QString name;
        QString settingsName;
        QString description;
        QtMsgType level;
        bool ignored = false;
        bool isDefault = false; // Whether or not the category can be disabled
        bool changed = false;

        bool operator==(const LoggerWarningId warningId) const { return warningId.name() == name; }

        LoggerWarningId id() const { return LoggerWarningId(name); }

        QString levelToString() const {
            // TODO: this only makes sense to qmllint
            Q_ASSERT(ignored || level != QtCriticalMsg);
            if (ignored)
                return QStringLiteral("disable");

            switch (level) {
            case QtInfoMsg:
                return QStringLiteral("info");
            case QtWarningMsg:
                return QStringLiteral("warning");
            default:
                Q_UNREACHABLE();
                break;
            }
        }

        bool setLevel(const QString &level) {
            if (level == QStringLiteral("disable")) {
                this->level = QtCriticalMsg; // TODO: only so for consistency with previous logic
                this->ignored = true;
            } else if (level == QStringLiteral("info")) {
                this->level = QtInfoMsg;
                this->ignored = false;
            } else if (level == QStringLiteral("warning")) {
                this->level = QtWarningMsg;
                this->ignored = false;
            } else {
                return false;
            }

            this->changed = true;
            return true;
        }
    };

    const QList<Category> categories() const;
    static const QList<Category> &defaultCategories();

    void registerCategory(const Category &category);

    QQmlJSLogger();
    ~QQmlJSLogger() = default;

    bool hasWarnings() const { return !m_warnings.isEmpty(); }
    bool hasErrors() const { return !m_errors.isEmpty(); }

    const QList<Message> &infos() const { return m_infos; }
    const QList<Message> &warnings() const { return m_warnings; }
    const QList<Message> &errors() const { return m_errors; }

    QtMsgType categoryLevel(LoggerWarningId id) const
    {
        return m_categoryLevels[id.name().toString()];
    }
    void setCategoryLevel(LoggerWarningId id, QtMsgType level)
    {
        m_categoryLevels[id.name().toString()] = level;
        m_categoryChanged[id.name().toString()] = true;
    }

    bool isCategoryIgnored(LoggerWarningId id) const
    {
        return m_categoryIgnored[id.name().toString()];
    }
    void setCategoryIgnored(LoggerWarningId id, bool error)
    {
        m_categoryIgnored[id.name().toString()] = error;
        m_categoryChanged[id.name().toString()] = true;
    }

    bool isCategoryFatal(LoggerWarningId id) const
    {
        return m_categoryFatal[id.name().toString()];
    }
    void setCategoryFatal(LoggerWarningId id, bool error)
    {
        m_categoryFatal[id.name().toString()] = error;
        m_categoryChanged[id.name().toString()] = true;
    }

    bool wasCategoryChanged(LoggerWarningId id) const
    {
        return m_categoryChanged[id.name().toString()];
    }

    /*! \internal

        Logs \a message with severity deduced from \a category. Prefer using
        this function in most cases.

        \sa setCategoryLevel
    */
    void log(const QString &message, LoggerWarningId id, const QQmlJS::SourceLocation &srcLocation,
             bool showContext = true, bool showFileName = true,
             const std::optional<QQmlJSFixSuggestion> &suggestion = {},
             const QString overrideFileName = QString())
    {
        log(message, id, srcLocation, m_categoryLevels[id.name().toString()], showContext,
            showFileName, suggestion, overrideFileName);
    }

    void processMessages(const QList<QQmlJS::DiagnosticMessage> &messages,
                         const LoggerWarningId id);

    void ignoreWarnings(uint32_t line, const QSet<QString> &categories)
    {
        m_ignoredWarnings[line] = categories;
    }

    void setSilent(bool silent) { m_output.setSilent(silent); }
    bool isSilent() const { return m_output.isSilent(); }

    void setCode(const QString &code) { m_code = code; }
    QString code() const { return m_code; }

    void setFileName(const QString &fileName) { m_fileName =  fileName; }
    QString fileName() const { return m_fileName; }

private:
    QMap<QString, Category> m_categories;

    void printContext(const QString &overrideFileName, const QQmlJS::SourceLocation &location);
    void printFix(const QQmlJSFixSuggestion &fix);

    void log(const QString &message, LoggerWarningId id, const QQmlJS::SourceLocation &srcLocation,
             QtMsgType type, bool showContext, bool showFileName,
             const std::optional<QQmlJSFixSuggestion> &suggestion, const QString overrideFileName);

    QString m_fileName;
    QString m_code;

    QColorOutput m_output;

    QHash<QString, QtMsgType> m_categoryLevels;
    QHash<QString, bool> m_categoryIgnored;

    // If true, triggers qFatal on documents with "pragma Strict"
    // TODO: Works only for qmlCompiler category so far.
    QHash<QString, bool> m_categoryFatal;

    QHash<QString, bool> m_categoryChanged;

    QList<Message> m_infos;
    QList<Message> m_warnings;
    QList<Message> m_errors;
    QHash<uint32_t, QSet<QString>> m_ignoredWarnings;

    // the compiler needs private log() function at the moment
    friend class QQmlJSAotCompiler;
};

QT_END_NAMESPACE

#endif // QQMLJSLOGGER_P_H
