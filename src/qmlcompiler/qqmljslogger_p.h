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

#include "qcoloroutput_p.h"

#include <private/qqmljsdiagnosticmessage_p.h>

#include <QtCore/qhash.h>
#include <QtCore/qstring.h>
#include <QtCore/qlist.h>

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

enum QQmlJSLoggerCategory
{
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
    QQmlJSLoggerCategory_Last = Log_UnusedImport
};

class QQmlJSLogger
{
    Q_DISABLE_MOVE(QQmlJSLogger)
public:
    QQmlJSLogger(const QString &fileName, const QString &code, bool silent = false);

    bool hasWarnings() const { return !m_warnings.isEmpty(); }
    bool hasErrors() const { return !m_errors.isEmpty(); }

    const QList<QQmlJS::DiagnosticMessage> &infos() const { return m_infos; }
    const QList<QQmlJS::DiagnosticMessage> &warnings() const { return m_warnings; }
    const QList<QQmlJS::DiagnosticMessage> &errors() const { return m_errors; }

    QtMsgType categoryLevel(QQmlJSLoggerCategory category) const { return m_categoryLevels[category]; }
    void setCategoryLevel(QQmlJSLoggerCategory category, QtMsgType Level) { m_categoryLevels[category] = Level; }

    bool isCategorySilent(QQmlJSLoggerCategory category) const { return m_categorySilent[category]; }
    void setCategorySilent(QQmlJSLoggerCategory category, bool silent) { m_categorySilent[category] = silent; }


    void log(const QString &message, QQmlJSLoggerCategory category,
             const QQmlJS::SourceLocation& = QQmlJS::SourceLocation(), bool showContext = true);
    void processMessages(const QList<QQmlJS::DiagnosticMessage> &messages, QQmlJSLoggerCategory category);

    QColorOutput &colorOutput() { return m_output; }

private:
    void printContext(const QQmlJS::SourceLocation &location);


    const QString &m_fileName;
    const QString &m_code;

    QColorOutput m_output;

    QtMsgType m_categoryLevels[QQmlJSLoggerCategory_Last + 1] = {};
    bool m_categorySilent[QQmlJSLoggerCategory_Last + 1] = {};

    QList<QQmlJS::DiagnosticMessage> m_infos;
    QList<QQmlJS::DiagnosticMessage> m_warnings;
    QList<QQmlJS::DiagnosticMessage> m_errors;
};

#endif // QLOGGER_H
