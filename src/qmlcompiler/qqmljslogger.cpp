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

#include <qglobal.h>

// GCC 11 thinks diagMsg.fixSuggestion.fixes.d.ptr is somehow uninitialized in
// QList::emplaceBack(), probably called from QQmlJsLogger::log()
// Ditto for GCC 12, but it emits a different warning
QT_WARNING_PUSH
QT_WARNING_DISABLE_GCC("-Wuninitialized")
QT_WARNING_DISABLE_GCC("-Wmaybe-uninitialized")
#include <qlist.h>
QT_WARNING_POP

#include "qqmljslogger_p.h"

QT_BEGIN_NAMESPACE

const QMap<QString, QQmlJSLogger::Option> &QQmlJSLogger::options() {
    static QMap<QString, QQmlJSLogger::Option> optionsMap = {
        { QStringLiteral("required"),
          QQmlJSLogger::Option(Log_Required, QStringLiteral("RequiredProperty"),
                               QStringLiteral("Warn about required properties"), QtInfoMsg) },
        { QStringLiteral("alias"),
          QQmlJSLogger::Option(Log_Alias, QStringLiteral("PropertyAlias"),
                               QStringLiteral("Warn about alias errors"), QtInfoMsg) },
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
                               QStringLiteral("Warn about inheritance cycles"), QtInfoMsg) },
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
                               QStringLiteral("Warn about unknown properties"), QtInfoMsg) },
        { QStringLiteral("deferred-property-id"),
          QQmlJSLogger::Option(
                  Log_DeferredPropertyId, QStringLiteral("DeferredPropertyId"),
                  QStringLiteral(
                          "Warn about making deferred properties immediate by giving them an id."),
                  QtInfoMsg) },
        { QStringLiteral("unqualified"),
          QQmlJSLogger::Option(
                  Log_UnqualifiedAccess, QStringLiteral("UnqualifiedAccess"),
                  QStringLiteral("Warn about unqualified identifiers and how to fix them"),
                  QtWarningMsg) },
        { QStringLiteral("unused-imports"),
          QQmlJSLogger::Option(Log_UnusedImport, QStringLiteral("UnusedImports"),
                               QStringLiteral("Warn about unused imports"), QtInfoMsg, false) },
        { QStringLiteral("multiline-strings"),
          QQmlJSLogger::Option(Log_MultilineString, QStringLiteral("MultilineStrings"),
                               QStringLiteral("Warn about multiline strings"), QtInfoMsg, false) },
        { QStringLiteral("compiler"),
          QQmlJSLogger::Option(Log_Compiler, QStringLiteral("CompilerWarnings"),
                               QStringLiteral("Warn about compiler issues"), QtCriticalMsg,
                               false) },
        { QStringLiteral("controls-sanity"),
          QQmlJSLogger::Option(
                  Log_ControlsSanity, QStringLiteral("ControlsSanity"),
                  QStringLiteral("Performance checks used for QuickControl's implementation"),
                  QtCriticalMsg, false) },
        { QStringLiteral("multiple-attached-objects"),
          QQmlJSLogger::Option(
                  Log_AttachedPropertyReuse, QStringLiteral("AttachedPropertyReuse"),
                  QStringLiteral("Warn if attached types from parent components aren't reused"),
                  QtCriticalMsg, false) }
    };

    return optionsMap;
}

QQmlJSLogger::QQmlJSLogger()
{
    const auto &opt = options();
    for (auto it = opt.cbegin(); it != opt.cend(); ++it) {
        m_categoryLevels[it.value().m_category] = it.value().m_level;
        m_categoryError[it.value().m_category] = it.value().m_error;
    }

    // These have to be set up manually since we don't expose it as an option
    m_categoryLevels[Log_RecursionDepthError] = QtInfoMsg;
    m_categoryError[Log_RecursionDepthError] = true;
    m_categoryLevels[Log_Syntax] = QtInfoMsg;
    m_categoryError[Log_Syntax] = true;

    // setup color output
    m_output.insertMapping(QtCriticalMsg, QColorOutput::RedForeground);
    m_output.insertMapping(QtWarningMsg, QColorOutput::PurpleForeground);
    m_output.insertMapping(QtInfoMsg, QColorOutput::BlueForeground);
    m_output.insertMapping(QtDebugMsg, QColorOutput::GreenForeground);
}

static bool isMsgTypeLess(QtMsgType a, QtMsgType b)
{
    static QHash<QtMsgType, int> level = { { QtDebugMsg, 0 },
                                           { QtInfoMsg, 1 },
                                           { QtWarningMsg, 2 },
                                           { QtCriticalMsg, 3 },
                                           { QtFatalMsg, 4 } };
    return level[a] < level[b];
}

void QQmlJSLogger::log(const QString &message, QQmlJSLoggerCategory category,
                       const QQmlJS::SourceLocation &srcLocation, QtMsgType type, bool showContext,
                       bool showFileName, const std::optional<FixSuggestion> &suggestion)
{
    if (isMsgTypeLess(type, m_categoryLevels[category]))
        return;

    if (srcLocation.isValid() && m_ignoredWarnings[srcLocation.startLine].contains(category))
        return;

    QString prefix;

    if (!m_fileName.isEmpty() && showFileName)
        prefix = m_fileName + QStringLiteral(":");

    if (srcLocation.isValid())
        prefix += QStringLiteral("%1:%2:").arg(srcLocation.startLine).arg(srcLocation.startColumn);

    if (!prefix.isEmpty())
        prefix.append(QLatin1Char(' '));

    m_output.writePrefixedMessage(prefix + message, type);

    QtMsgType machineType = isMsgTypeLess(QtWarningMsg, type) ? QtCriticalMsg : QtInfoMsg;

    // If this is a category that produces error codes, we need to up all the messages to at least a
    // warning level
    if (isCategoryError(category)) {
        if (isMsgTypeLess(type, QtWarningMsg))
            machineType = QtWarningMsg;
        else
            machineType = type;
    }

    Message diagMsg;
    diagMsg.message = message;
    diagMsg.loc = srcLocation;
    diagMsg.type = machineType;
    diagMsg.fixSuggestion = suggestion;

    switch (machineType) {
    case QtWarningMsg: m_warnings.push_back(diagMsg); break;
    case QtCriticalMsg: m_errors.push_back(diagMsg); break;
    case QtInfoMsg: m_infos.push_back(diagMsg); break;
    default: break;
    }

    if (srcLocation.isValid() && !m_code.isEmpty() && showContext)
        printContext(srcLocation);

    if (suggestion.has_value())
        printFix(suggestion.value());
}

void QQmlJSLogger::processMessages(const QList<QQmlJS::DiagnosticMessage> &messages,
                                   QtMsgType level, QQmlJSLoggerCategory category)
{
    if (isMsgTypeLess(level, m_categoryLevels[category]) || messages.isEmpty())
        return;

    m_output.write(QStringLiteral("---\n"));

    for (const QQmlJS::DiagnosticMessage &message : messages)
        logWarning(message.message, category, QQmlJS::SourceLocation(), false, false);

    m_output.write(QStringLiteral("---\n\n"));
}

void QQmlJSLogger::printContext(const QQmlJS::SourceLocation &location)
{
    IssueLocationWithContext issueLocationWithContext { m_code, location };
    if (const QStringView beforeText = issueLocationWithContext.beforeText(); !beforeText.isEmpty())
        m_output.write(beforeText);

    bool locationMultiline = issueLocationWithContext.issueText().contains(QLatin1Char('\n'));

    if (!issueLocationWithContext.issueText().isEmpty())
        m_output.write(issueLocationWithContext.issueText().toString(), QtCriticalMsg);
    m_output.write(issueLocationWithContext.afterText().toString() + QLatin1Char('\n'));

    // Do not draw location indicator for multiline locations
    if (locationMultiline)
        return;

    int tabCount = issueLocationWithContext.beforeText().count(QLatin1Char('\t'));
    int locationLength = location.length == 0 ? 1 : location.length;
    m_output.write(QString::fromLatin1(" ").repeated(issueLocationWithContext.beforeText().length()
                                                     - tabCount)
                   + QString::fromLatin1("\t").repeated(tabCount)
                   + QString::fromLatin1("^").repeated(locationLength) + QLatin1Char('\n'));
}

void QQmlJSLogger::printFix(const FixSuggestion &fix)
{
    for (const auto &fixItem : fix.fixes) {
        m_output.writePrefixedMessage(fixItem.message, QtInfoMsg);

        if (!fixItem.cutLocation.isValid())
            continue;

        IssueLocationWithContext issueLocationWithContext { m_code, fixItem.cutLocation };

        if (const QStringView beforeText = issueLocationWithContext.beforeText();
            !beforeText.isEmpty()) {
            m_output.write(beforeText);
        }

        m_output.write(fixItem.replacementString, QtDebugMsg);
        m_output.write(issueLocationWithContext.afterText().toString() + u'\n');

        int tabCount = issueLocationWithContext.beforeText().count(u'\t');
        m_output.write(u" "_qs.repeated(
                               issueLocationWithContext.beforeText().length() - tabCount)
                       + u"\t"_qs.repeated(tabCount)
                       + u"^"_qs.repeated(fixItem.replacementString.length())
                       + u'\n');
    }
}

QT_END_NAMESPACE
