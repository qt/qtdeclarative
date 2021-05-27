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

#ifndef QLOGGER_H
#define QLOGGER_H

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
    Log_UnqualifiedAccess,
    Log_UnusedImport,
    Log_MultilineString,
    Log_Syntax,
    QQmlJSLoggerCategory_Last = Log_Syntax
};

class QQmlJSLogger
{
    Q_DISABLE_COPY_MOVE(QQmlJSLogger)
public:
    struct Option
    {
        Option() = default;
        Option(QQmlJSLoggerCategory category, QString settingsName, const QString &description,
               QtMsgType level, bool disabled = false)
            : m_category(category),
              m_settingsName(settingsName),
              m_description(description),
              m_level(level),
              m_disabled(disabled)
        {
        }
        QQmlJSLoggerCategory m_category;
        QString m_settingsName;
        QString m_description;
        QtMsgType m_level;
        bool m_disabled;

        QString levelToString() const {
            if (m_disabled)
                return QStringLiteral("silent");
            switch (m_level) {
            case QtInfoMsg:
                return QStringLiteral("info");
            case QtWarningMsg:
                return QStringLiteral("warning");
            case QtCriticalMsg:
                return QStringLiteral("error");
            default:
                Q_UNREACHABLE();
                break;
            }
        }

        bool setLevel(const QString &level) {
            if (level == QStringLiteral("disable")) {
                m_disabled = true;
            } else if (level == QStringLiteral("info")) {
                m_level = QtInfoMsg;
            } else if (level == QStringLiteral("warning")) {
                m_level = QtWarningMsg;
            } else {
                return false;
            }

            return true;
        }
    };

    static const QMap<QString, Option> &options();

    QQmlJSLogger(const QString &fileName, const QString &code, bool silent = false);
    ~QQmlJSLogger() = default;

    bool hasWarnings() const { return !m_warnings.isEmpty(); }
    bool hasErrors() const { return !m_errors.isEmpty(); }

    const QList<QQmlJS::DiagnosticMessage> &infos() const { return m_infos; }
    const QList<QQmlJS::DiagnosticMessage> &warnings() const { return m_warnings; }
    const QList<QQmlJS::DiagnosticMessage> &errors() const { return m_errors; }

    QtMsgType categoryLevel(QQmlJSLoggerCategory category) const { return m_categoryLevels[category]; }
    void setCategoryLevel(QQmlJSLoggerCategory category, QtMsgType Level) { m_categoryLevels[category] = Level; }

    bool isCategoryDisabled(QQmlJSLoggerCategory category) const { return m_categoryDisabled[category]; }
    void setCategoryDisabled(QQmlJSLoggerCategory category, bool disabled) { m_categoryDisabled[category] = disabled; }


    void log(const QString &message, QQmlJSLoggerCategory category,
             const QQmlJS::SourceLocation& = QQmlJS::SourceLocation(), bool showContext = true, bool showFileName = true);

    void processMessages(const QList<QQmlJS::DiagnosticMessage> &messages, QQmlJSLoggerCategory category);

    void ignoreWarnings(uint32_t line, const QSet<QQmlJSLoggerCategory> &categories)
    {
        m_ignoredWarnings[line] = categories;
    }

    QColorOutput &colorOutput() { return m_output; }

private:
    void printContext(const QQmlJS::SourceLocation &location);

    const QString m_fileName;
    const QString m_code;

    QColorOutput m_output;

    QtMsgType m_categoryLevels[QQmlJSLoggerCategory_Last + 1] = {};
    bool m_categoryDisabled[QQmlJSLoggerCategory_Last + 1] = {};

    QList<QQmlJS::DiagnosticMessage> m_infos;
    QList<QQmlJS::DiagnosticMessage> m_warnings;
    QList<QQmlJS::DiagnosticMessage> m_errors;
    QHash<uint32_t, QSet<QQmlJSLoggerCategory>> m_ignoredWarnings;
};

#endif // QLOGGER_H
