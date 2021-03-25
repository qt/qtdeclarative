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

#include "qqmljslogger_p.h"

QQmlJSLogger::QQmlJSLogger(const QString &fileName, const QString &code, bool silent) : m_fileName(fileName), m_code(code), m_output(silent)
{
    // Set up some sane default logging levels
    m_categoryLevels[Log_Required] = QtWarningMsg;
    m_categoryLevels[Log_Alias] = QtWarningMsg;
    m_categoryLevels[Log_Import] = QtWarningMsg;
    m_categoryLevels[Log_Deprecation] = QtWarningMsg;
    m_categoryLevels[Log_RecursionDepthError] = QtCriticalMsg;
    m_categoryLevels[Log_WithStatement] = QtWarningMsg;
    m_categoryLevels[Log_InheritanceCycle] = QtWarningMsg;
    m_categoryLevels[Log_Signal] = QtWarningMsg;
    m_categoryLevels[Log_Type] = QtWarningMsg;
    m_categoryLevels[Log_Property] = QtWarningMsg;
    m_categoryLevels[Log_UnqualifiedAccess] = QtWarningMsg;
    m_categoryLevels[Log_UnusedImport] = QtInfoMsg;

    // setup color output
    m_output.insertMapping(QtCriticalMsg, QColorOutput::RedForeground);
    m_output.insertMapping(QtWarningMsg, QColorOutput::PurpleForeground);
    m_output.insertMapping(QtInfoMsg, QColorOutput::BlueForeground);
    m_output.insertMapping(QtDebugMsg, QColorOutput::GreenForeground);
}

void QQmlJSLogger::log(const QString &message, QQmlJSLoggerCategory category, const QQmlJS::SourceLocation &srcLocation, bool showContext)
{
    if (isCategorySilent(category))
        return;

    const QtMsgType msgType = m_categoryLevels[category];

    QString prefix;

    if (!m_fileName.isEmpty())
        prefix = m_fileName + QStringLiteral(":");

    if (srcLocation.isValid())
        prefix += QStringLiteral("%1:%2:").arg(srcLocation.startLine).arg(srcLocation.startColumn);

    if (!prefix.isEmpty())
        prefix.append(QLatin1Char(' '));

    m_output.writePrefixedMessage(prefix + message, msgType);

    QQmlJS::DiagnosticMessage diagMsg;
    diagMsg.message = message;
    diagMsg.loc = srcLocation;
    diagMsg.type = msgType;

    switch (msgType) {
    case QtWarningMsg: m_warnings.push_back(diagMsg); break;
    case QtCriticalMsg: m_errors.push_back(diagMsg); break;
    case QtInfoMsg: m_infos.push_back(diagMsg); break;
    default: break;
    }

    if (srcLocation.isValid() && !m_code.isEmpty() && showContext)
        printContext(srcLocation);
}

void QQmlJSLogger::processMessages(const QList<QQmlJS::DiagnosticMessage> &messages, QQmlJSLoggerCategory category)
{
    for (const QQmlJS::DiagnosticMessage &message : messages)
        log(message.message, category, message.loc);
}

void QQmlJSLogger::printContext(const QQmlJS::SourceLocation &location)
{
    IssueLocationWithContext issueLocationWithContext { m_code, location };
    if (const QStringView beforeText = issueLocationWithContext.beforeText(); !beforeText.isEmpty())
        m_output.write(beforeText);
    m_output.write(issueLocationWithContext.issueText().toString(), QtCriticalMsg);
    m_output.write(issueLocationWithContext.afterText() + QLatin1Char('\n'));
    int tabCount = issueLocationWithContext.beforeText().count(QLatin1Char('\t'));
    m_output.write(QString::fromLatin1(" ").repeated(
                       issueLocationWithContext.beforeText().length() - tabCount)
                           + QString::fromLatin1("\t").repeated(tabCount)
                           + QString::fromLatin1("^").repeated(location.length)
                           + QLatin1Char('\n'));
}
