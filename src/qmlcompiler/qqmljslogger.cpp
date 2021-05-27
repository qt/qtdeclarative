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

const QMap<QString, QQmlJSLogger::Option> &QQmlJSLogger::options() {
    static QMap<QString, QQmlJSLogger::Option> optionsMap = {
        { QStringLiteral("required"),
          QQmlJSLogger::Option(Log_Required, QStringLiteral("RequiredProperty"),
                               QStringLiteral("Warn about required properties"), QtWarningMsg) },
        { QStringLiteral("alias"),
          QQmlJSLogger::Option(Log_Alias, QStringLiteral("PropertyAlias"),
                               QStringLiteral("Warn about alias errors"), QtWarningMsg) },
        { QStringLiteral("import"),
          QQmlJSLogger::Option(Log_Import, QStringLiteral("ImportFailure"),
                               QStringLiteral("Warn about failing imports and deprecated qmltypes"),
                               QtWarningMsg) },
        { QStringLiteral("with"),
          QQmlJSLogger::Option(Log_WithStatement, QStringLiteral("WithStatement"),
                               QStringLiteral("Warn about with statements as they can cause false "
                                              "positives when checking for unqualified access"),
                               QtWarningMsg) },
        { QStringLiteral("inheritance-cycle"),
          QQmlJSLogger::Option(Log_InheritanceCycle, QStringLiteral("InheritanceCycle"),
                               QStringLiteral("Warn about inheritance cycles"), QtWarningMsg) },
        { QStringLiteral("deprecated"),
          QQmlJSLogger::Option(Log_Deprecation, QStringLiteral("Deprecated"),
                               QStringLiteral("Warn about deprecated properties and types"),
                               QtWarningMsg) },
        { QStringLiteral("signal"),
          QQmlJSLogger::Option(Log_Signal, QStringLiteral("BadSignalHandler"),
                               QStringLiteral("Warn about bad signal handler parameters"),
                               QtWarningMsg) },
        { QStringLiteral("type"),
          QQmlJSLogger::Option(Log_Type, QStringLiteral("TypeError"),
                               QStringLiteral("Warn about unresolvable types and type mismatches"),
                               QtWarningMsg) },
        { QStringLiteral("property"),
          QQmlJSLogger::Option(Log_Property, QStringLiteral("UnknownProperty"),
                               QStringLiteral("Warn about unknown properties"), QtWarningMsg) },
        { QStringLiteral("unqualified"),
          QQmlJSLogger::Option(
                  Log_UnqualifiedAccess, QStringLiteral("UnqualifiedAccess"),
                  QStringLiteral("Warn about unqualified identifiers and how to fix them"),
                  QtWarningMsg) },
        { QStringLiteral("unused-imports"),
          QQmlJSLogger::Option(Log_UnusedImport, QStringLiteral("UnusedImports"),
                               QStringLiteral("Warn about unused imports"), QtInfoMsg) },
        { QStringLiteral("multiline-strings"),
          QQmlJSLogger::Option(Log_MultilineString, QStringLiteral("MultilineStrings"),
                               QStringLiteral("Warn about multiline strings"), QtInfoMsg) }
    };

    return optionsMap;
}

QQmlJSLogger::QQmlJSLogger(const QString &fileName, const QString &code, bool silent) : m_fileName(fileName), m_code(code), m_output(silent)
{
    const auto &opt = options();
    for (auto it = opt.cbegin(); it != opt.cend(); ++it) {
        m_categoryLevels[it.value().m_category] = it.value().m_level;
        m_categoryDisabled[it.value().m_category] = it.value().m_disabled;
    }

    // These have to be set up manually since we don't expose it as an option
    m_categoryLevels[Log_RecursionDepthError] = QtCriticalMsg;
    m_categoryLevels[Log_Syntax] = QtCriticalMsg;

    // setup color output
    m_output.insertMapping(QtCriticalMsg, QColorOutput::RedForeground);
    m_output.insertMapping(QtWarningMsg, QColorOutput::PurpleForeground);
    m_output.insertMapping(QtInfoMsg, QColorOutput::BlueForeground);
    m_output.insertMapping(QtDebugMsg, QColorOutput::GreenForeground);
}

void QQmlJSLogger::log(const QString &message, QQmlJSLoggerCategory category, const QQmlJS::SourceLocation &srcLocation, bool showContext, bool showFileName)
{
    if (isCategoryDisabled(category))
        return;

    if (srcLocation.isValid() && m_ignoredWarnings[srcLocation.startLine].contains(category))
        return;

    const QtMsgType msgType = m_categoryLevels[category];

    QString prefix;

    if (!m_fileName.isEmpty() && showFileName)
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
    if (isCategoryDisabled(category) || messages.isEmpty())
        return;

    m_output.write(QStringLiteral("---\n"));

    for (const QQmlJS::DiagnosticMessage &message : messages)
        log(message.message, category, QQmlJS::SourceLocation(), false, false);

    m_output.write(QStringLiteral("---\n\n"));
}

void QQmlJSLogger::printContext(const QQmlJS::SourceLocation &location)
{
    IssueLocationWithContext issueLocationWithContext { m_code, location };
    if (const QStringView beforeText = issueLocationWithContext.beforeText(); !beforeText.isEmpty())
        m_output.write(beforeText);

    bool locationMultiline = issueLocationWithContext.issueText().contains(QLatin1Char('\n'));

    m_output.write(issueLocationWithContext.issueText().toString(), QtCriticalMsg);
    m_output.write(issueLocationWithContext.afterText() + QLatin1Char('\n'));

    // Do not draw location indicator for multiline locations
    if (locationMultiline)
        return;

    int tabCount = issueLocationWithContext.beforeText().count(QLatin1Char('\t'));
    m_output.write(QString::fromLatin1(" ").repeated(
                       issueLocationWithContext.beforeText().length() - tabCount)
                           + QString::fromLatin1("\t").repeated(tabCount)
                           + QString::fromLatin1("^").repeated(location.length)
                           + QLatin1Char('\n'));
}
