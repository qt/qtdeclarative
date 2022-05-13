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
        int before = qMax(0,code.lastIndexOf(QLatin1Char('\n'), location.offset));

        if (before != 0) before++;

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

enum QQmlJSLoggerCategory {
    Log_Required,
    Log_Alias,
    Log_Import,
    Log_RecursionDepthError,
    Log_WithStatement,
    Log_InheritanceCycle,
    Log_Deprecation,
    Log_Signal,
    Log_Type,
    Log_Property,
    Log_DeferredPropertyId,
    Log_UnqualifiedAccess,
    Log_UnusedImport,
    Log_MultilineString,
    Log_Syntax,
    Log_SyntaxIdQuotation,
    Log_SyntaxDuplicateIds,
    Log_Compiler,
    Log_ControlsSanity,
    Log_AttachedPropertyReuse,
    Log_Plugin,
    QQmlJSLoggerCategory_Last = Log_Plugin
};

struct Q_QMLCOMPILER_PRIVATE_EXPORT FixSuggestion
{
    struct Fix
    {
        QString message;
        QQmlJS::SourceLocation cutLocation = QQmlJS::SourceLocation();
        QString replacementString = QString();
        QString fileName = QString();
        // A Fix is a hint if it can not be automatically applied to fix an issue or only points out
        // its origin
        bool isHint = true;
    };
    QList<Fix> fixes;
};

struct Message : public QQmlJS::DiagnosticMessage
{
    std::optional<FixSuggestion> fixSuggestion;
};

class Q_QMLCOMPILER_PRIVATE_EXPORT QQmlJSLogger
{
    Q_DISABLE_COPY_MOVE(QQmlJSLogger)
public:
    struct Option
    {
        Option() = default;
        Option(QQmlJSLoggerCategory category, QString settingsName, const QString &description,
               QtMsgType level, bool ignored = false)
            : m_category(category),
              m_settingsName(settingsName),
              m_description(description),
              m_level(level),
              m_ignored(ignored)
        {
        }
        QQmlJSLoggerCategory m_category;
        QString m_settingsName;
        QString m_description;
        QtMsgType m_level;
        bool m_ignored;
        bool m_changed = false;

        QString levelToString() const {
            // TODO:: this only makes sense to qmllint
            Q_ASSERT(m_ignored || m_level != QtCriticalMsg);
            if (m_ignored)
                return QStringLiteral("disable");

            switch (m_level) {
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
                m_level = QtCriticalMsg; // TODO: only so for consistency with previous logic
                m_ignored = true;
            } else if (level == QStringLiteral("info")) {
                m_level = QtInfoMsg;
                m_ignored = false;
            } else if (level == QStringLiteral("warning")) {
                m_level = QtWarningMsg;
                m_ignored = false;
            } else {
                return false;
            }

            m_changed = true;
            return true;
        }
    };

    static const QMap<QString, Option> &options();

    QQmlJSLogger();
    ~QQmlJSLogger() = default;

    bool hasWarnings() const { return !m_warnings.isEmpty(); }
    bool hasErrors() const { return !m_errors.isEmpty(); }

    const QList<Message> &infos() const { return m_infos; }
    const QList<Message> &warnings() const { return m_warnings; }
    const QList<Message> &errors() const { return m_errors; }

    QtMsgType categoryLevel(QQmlJSLoggerCategory category) const { return m_categoryLevels[category]; }
    void setCategoryLevel(QQmlJSLoggerCategory category, QtMsgType level)
    {
        m_categoryLevels[category] = level;
        m_categoryChanged[category] = true;
    }

    bool isCategoryIgnored(QQmlJSLoggerCategory category) const
    {
        return m_categoryIgnored[category];
    }
    void setCategoryIgnored(QQmlJSLoggerCategory category, bool error)
    {
        m_categoryIgnored[category] = error;
        m_categoryChanged[category] = true;
    }

    bool wasCategoryChanged(QQmlJSLoggerCategory category) const
    {
        return m_categoryChanged[category];
    }

    /*! \internal

        Logs \a message with severity deduced from \a category. Prefer using
        this function in most cases.

        \sa setCategoryLevel
    */
    void log(const QString &message, QQmlJSLoggerCategory category,
             const QQmlJS::SourceLocation &srcLocation, bool showContext = true,
             bool showFileName = true, const std::optional<FixSuggestion> &suggestion = {},
             const QString overrideFileName = QString())
    {
        log(message, category, srcLocation, m_categoryLevels[category], showContext, showFileName,
            suggestion, overrideFileName);
    }

    void processMessages(const QList<QQmlJS::DiagnosticMessage> &messages,
                         QQmlJSLoggerCategory category);

    void ignoreWarnings(uint32_t line, const QSet<QQmlJSLoggerCategory> &categories)
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
    void printContext(const QString &overrideFileName, const QQmlJS::SourceLocation &location);
    void printFix(const FixSuggestion &fix);

    void log(const QString &message, QQmlJSLoggerCategory category,
             const QQmlJS::SourceLocation &srcLocation, QtMsgType type, bool showContext,
             bool showFileName, const std::optional<FixSuggestion> &suggestion,
             const QString overrideFileName);

    QString m_fileName;
    QString m_code;

    QColorOutput m_output;

    QtMsgType m_categoryLevels[QQmlJSLoggerCategory_Last + 1] = {};
    bool m_categoryIgnored[QQmlJSLoggerCategory_Last + 1] = {};
    bool m_categoryChanged[QQmlJSLoggerCategory_Last + 1] = {};

    QList<Message> m_infos;
    QList<Message> m_warnings;
    QList<Message> m_errors;
    QHash<uint32_t, QSet<QQmlJSLoggerCategory>> m_ignoredWarnings;

    // the compiler needs private log() function at the moment
    friend class QQmlJSAotCompiler;
};

QT_END_NAMESPACE

#endif // QQMLJSLOGGER_P_H
