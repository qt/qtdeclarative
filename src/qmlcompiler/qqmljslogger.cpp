// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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

#include <QtCore/qfile.h>
#include <QtCore/qfileinfo.h>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

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
        { QStringLiteral("deferred-property-id"),
          QQmlJSLogger::Option(
                  Log_DeferredPropertyId, QStringLiteral("DeferredPropertyId"),
                  QStringLiteral(
                          "Warn about making deferred properties immediate by giving them an id."),
                  QtWarningMsg) },
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
                               QStringLiteral("Warn about multiline strings"), QtInfoMsg) },
        { QStringLiteral("compiler"),
          QQmlJSLogger::Option(Log_Compiler, QStringLiteral("CompilerWarnings"),
                               QStringLiteral("Warn about compiler issues"), QtCriticalMsg, true) },
        { QStringLiteral("controls-sanity"),
          QQmlJSLogger::Option(
                  Log_ControlsSanity, QStringLiteral("ControlsSanity"),
                  QStringLiteral("Performance checks used for QuickControl's implementation"),
                  QtCriticalMsg, true) },
        { QStringLiteral("multiple-attached-objects"),
          QQmlJSLogger::Option(
                  Log_AttachedPropertyReuse, QStringLiteral("AttachedPropertyReuse"),
                  QStringLiteral("Warn if attached types from parent components aren't reused"),
                  QtCriticalMsg, true) },
        { QStringLiteral("plugin"),
          QQmlJSLogger::Option(Log_Plugin, QStringLiteral("LintPluginWarnings"),
                               QStringLiteral("Warn if a qmllint plugin finds an issue"),
                               QtWarningMsg) }
    };

    return optionsMap;
}

QQmlJSLogger::QQmlJSLogger()
{
    const auto &opt = options();
    for (auto it = opt.cbegin(); it != opt.cend(); ++it) {
        m_categoryLevels[it.value().m_category] = it.value().m_level;
        m_categoryIgnored[it.value().m_category] = it.value().m_ignored;
    }

    // These have to be set up manually since we don't expose it as an option
    m_categoryLevels[Log_RecursionDepthError] = QtCriticalMsg;
    m_categoryLevels[Log_Syntax] = QtWarningMsg; // TODO: because we usually report it as a warning!
    m_categoryLevels[Log_SyntaxIdQuotation] = QtWarningMsg;
    m_categoryLevels[Log_SyntaxDuplicateIds] = QtCriticalMsg;

    // setup color output
    m_output.insertMapping(QtCriticalMsg, QColorOutput::RedForeground);
    m_output.insertMapping(QtWarningMsg, QColorOutput::PurpleForeground); // Yellow?
    m_output.insertMapping(QtInfoMsg, QColorOutput::BlueForeground);
    m_output.insertMapping(QtDebugMsg, QColorOutput::GreenForeground); // None?
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
                       bool showFileName, const std::optional<FixSuggestion> &suggestion,
                       const QString overrideFileName)
{
    if (isCategoryIgnored(category))
        return;

    // Note: assume \a type is the type we should prefer for logging

    if (srcLocation.isValid() && m_ignoredWarnings[srcLocation.startLine].contains(category))
        return;

    QString prefix;

    if ((!overrideFileName.isEmpty() || !m_fileName.isEmpty()) && showFileName)
        prefix =
                (!overrideFileName.isEmpty() ? overrideFileName : m_fileName) + QStringLiteral(":");

    if (srcLocation.isValid())
        prefix += QStringLiteral("%1:%2:").arg(srcLocation.startLine).arg(srcLocation.startColumn);

    if (!prefix.isEmpty())
        prefix.append(QLatin1Char(' '));

    // Note: we do the clamping to [Info, Critical] range since our logger only
    // supports 3 categories
    type = std::clamp(type, QtInfoMsg, QtCriticalMsg, isMsgTypeLess);

    // Note: since we clamped our \a type, the output message is not printed
    // exactly like it was requested, bear with us
    m_output.writePrefixedMessage(prefix + message, type);

    Message diagMsg;
    diagMsg.message = message;
    diagMsg.loc = srcLocation;
    diagMsg.type = type;
    diagMsg.fixSuggestion = suggestion;

    switch (type) {
    case QtWarningMsg: m_warnings.push_back(diagMsg); break;
    case QtCriticalMsg: m_errors.push_back(diagMsg); break;
    case QtInfoMsg: m_infos.push_back(diagMsg); break;
    default: break;
    }

    if (srcLocation.isValid() && !m_code.isEmpty() && showContext)
        printContext(overrideFileName, srcLocation);

    if (suggestion.has_value())
        printFix(suggestion.value());
}

void QQmlJSLogger::processMessages(const QList<QQmlJS::DiagnosticMessage> &messages,
                                   QQmlJSLoggerCategory category)
{
    if (messages.isEmpty() || isCategoryIgnored(category))
        return;

    m_output.write(QStringLiteral("---\n"));

    // TODO: we should instead respect message's category here (potentially, it
    // should hold a category instead of type)
    for (const QQmlJS::DiagnosticMessage &message : messages)
        log(message.message, category, QQmlJS::SourceLocation(), false, false);

    m_output.write(QStringLiteral("---\n\n"));
}

void QQmlJSLogger::printContext(const QString &overrideFileName,
                                const QQmlJS::SourceLocation &location)
{
    QString code = m_code;

    if (!overrideFileName.isEmpty() && overrideFileName != QFileInfo(m_fileName).absolutePath()) {
        QFile file(overrideFileName);
        const bool success = file.open(QFile::ReadOnly);
        Q_ASSERT(success);
        code = QString::fromUtf8(file.readAll());
    }

    IssueLocationWithContext issueLocationWithContext { code, location };
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
    const QString currentFileAbsPath = QFileInfo(m_fileName).absolutePath();
    QString code = m_code;
    QString currentFile;
    for (const auto &fixItem : fix.fixes) {
        m_output.writePrefixedMessage(fixItem.message, QtInfoMsg);

        if (!fixItem.cutLocation.isValid())
            continue;

        if (fixItem.fileName == currentFile) {
            // Nothing to do in this case, we've already read the code
        } else if (fixItem.fileName.isEmpty() || fixItem.fileName == currentFileAbsPath) {
            code = m_code;
        } else {
            QFile file(fixItem.fileName);
            const bool success = file.open(QFile::ReadOnly);
            Q_ASSERT(success);
            code = QString::fromUtf8(file.readAll());
            currentFile = fixItem.fileName;
        }

        IssueLocationWithContext issueLocationWithContext { code, fixItem.cutLocation };

        if (const QStringView beforeText = issueLocationWithContext.beforeText();
            !beforeText.isEmpty()) {
            m_output.write(beforeText);
        }

        // The replacement string can be empty if we're only pointing something out to the user
        QStringView replacementString = fixItem.replacementString.isEmpty()
                ? issueLocationWithContext.issueText()
                : fixItem.replacementString;

        // But if there's nothing to change it has to be a hint
        if (fixItem.replacementString.isEmpty())
            Q_ASSERT(fixItem.isHint);

        m_output.write(replacementString, QtDebugMsg);
        m_output.write(issueLocationWithContext.afterText().toString() + u'\n');

        int tabCount = issueLocationWithContext.beforeText().count(u'\t');

        // Do not draw location indicator for multiline replacement strings
        if (replacementString.contains(u'\n'))
            continue;

        m_output.write(u" "_s.repeated(
                               issueLocationWithContext.beforeText().length() - tabCount)
                       + u"\t"_s.repeated(tabCount)
                       + u"^"_s.repeated(fixItem.replacementString.length()) + u'\n');
    }
}

QT_END_NAMESPACE
