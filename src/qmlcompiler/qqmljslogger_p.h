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
class IssueLocationWithContext
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
    Log_Compiler,
    Log_ControlsSanity,
    Log_AttachedPropertyReuse,
    QQmlJSLoggerCategory_Last = Log_AttachedPropertyReuse
};

struct FixSuggestion
{
    struct Fix
    {
        QString message;
        QQmlJS::SourceLocation cutLocation = QQmlJS::SourceLocation();
        QString replacementString = QString();
    };
    QList<Fix> fixes;
};

struct Message : public QQmlJS::DiagnosticMessage
{
    std::optional<FixSuggestion> fixSuggestion;
};

class QQmlJSLogger
{
    Q_DISABLE_COPY_MOVE(QQmlJSLogger)
public:
    struct Option
    {
        Option() = default;
        Option(QQmlJSLoggerCategory category, QString settingsName, const QString &description,
               QtMsgType level, bool error = true)
            : m_category(category),
              m_settingsName(settingsName),
              m_description(description),
              m_level(level),
              m_error(error)
        {
        }
        QQmlJSLoggerCategory m_category;
        QString m_settingsName;
        QString m_description;
        QtMsgType m_level;
        bool m_error;

        QString levelToString() const {
            switch (m_level) {
            case QtInfoMsg:
                return m_error ? QStringLiteral("warning") : QStringLiteral("info");
            case QtWarningMsg:
                // TODO: This case doesn't cleanly map onto any warning level yet
                // this has to be handled in the option overhaul
                return m_error ? QStringLiteral("warning") : QStringLiteral("info");
            case QtCriticalMsg:
                return QStringLiteral("disable");
            default:
                Q_UNREACHABLE();
                break;
            }
        }

        bool setLevel(const QString &level) {
            if (level == QStringLiteral("disable")) {
                m_level = QtCriticalMsg;
                m_error = false;
            } else if (level == QStringLiteral("info")) {
                m_level = QtInfoMsg;
                m_error = false;
            } else if (level == QStringLiteral("warning")) {
                m_level = QtInfoMsg;
                m_error = true;
            } else {
                return false;
            }

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
    void setCategoryLevel(QQmlJSLoggerCategory category, QtMsgType Level) { m_categoryLevels[category] = Level; }

    bool isCategoryError(QQmlJSLoggerCategory category) const { return m_categoryError[category]; }
    void setCategoryError(QQmlJSLoggerCategory category, bool error)
    {
        m_categoryError[category] = error;
    }

    void logInfo(const QString &message, QQmlJSLoggerCategory category,
                 const QQmlJS::SourceLocation &srcLocation = QQmlJS::SourceLocation(),
                 bool showContext = true, bool showFileName = true,
                 const std::optional<FixSuggestion> &suggestion = {})
    {
        log(message, category, srcLocation, QtInfoMsg, showContext, showFileName, suggestion);
    }

    void logWarning(const QString &message, QQmlJSLoggerCategory category,
                    const QQmlJS::SourceLocation &srcLocation = QQmlJS::SourceLocation(),
                    bool showContext = true, bool showFileName = true,
                    const std::optional<FixSuggestion> &suggestion = {})
    {
        log(message, category, srcLocation, QtWarningMsg, showContext, showFileName, suggestion);
    }

    void logCritical(const QString &message, QQmlJSLoggerCategory category,
                     const QQmlJS::SourceLocation &srcLocation = QQmlJS::SourceLocation(),
                     bool showContext = true, bool showFileName = true,
                     const std::optional<FixSuggestion> &suggestion = {})
    {
        log(message, category, srcLocation, QtCriticalMsg, showContext, showFileName, suggestion);
    }

    void processMessages(const QList<QQmlJS::DiagnosticMessage> &messages, QtMsgType level,
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
    void printContext(const QQmlJS::SourceLocation &location);
    void printFix(const FixSuggestion &fix);

    void log(const QString &message, QQmlJSLoggerCategory category, const QQmlJS::SourceLocation &,
             QtMsgType type, bool showContext, bool showFileName,
             const std::optional<FixSuggestion> &suggestion);

    QString m_fileName;
    QString m_code;

    QColorOutput m_output;

    QtMsgType m_categoryLevels[QQmlJSLoggerCategory_Last + 1] = {};
    bool m_categoryError[QQmlJSLoggerCategory_Last + 1] = {};

    QList<Message> m_infos;
    QList<Message> m_warnings;
    QList<Message> m_errors;
    QHash<uint32_t, QSet<QQmlJSLoggerCategory>> m_ignoredWarnings;
};

QT_END_NAMESPACE

#endif // QQMLJSLOGGER_P_H
