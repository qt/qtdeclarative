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

const LoggerWarningId qmlRequired { "required" };
const LoggerWarningId qmlAliasCycle { "alias-cycle" };
const LoggerWarningId qmlUnresolvedAlias { "unresolved-alias" };
const LoggerWarningId qmlImport { "import" };
const LoggerWarningId qmlRecursionDepthErrors { "recursion-depth-errors" };
const LoggerWarningId qmlWith { "with" };
const LoggerWarningId qmlInheritanceCycle { "inheritance-cycle" };
const LoggerWarningId qmlDeprecated { "deprecated" };
const LoggerWarningId qmlSignalParameters { "signal-handler-parameters" };
const LoggerWarningId qmlMissingType { "missing-type" };
const LoggerWarningId qmlUnresolvedType { "unresolved-type" };
const LoggerWarningId qmlRestrictedType { "restricted-type" };
const LoggerWarningId qmlPrefixedImportType { "prefixed-import-type" };
const LoggerWarningId qmlIncompatibleType { "incompatible-type" };
const LoggerWarningId qmlMissingProperty { "missing-property" };
const LoggerWarningId qmlNonListProperty { "non-list-property" };
const LoggerWarningId qmlReadOnlyProperty { "read-only-property" };
const LoggerWarningId qmlDuplicatePropertyBinding { "duplicate-property-binding" };
const LoggerWarningId qmlDuplicatedName { "duplicated-name" };
const LoggerWarningId qmlDeferredPropertyId { "deferred-property-id" };
const LoggerWarningId qmlUnqualified { "unqualified" };
const LoggerWarningId qmlUnusedImports { "unused-imports" };
const LoggerWarningId qmlMultilineStrings { "multiline-strings" };
const LoggerWarningId qmlSyntax { "syntax" };
const LoggerWarningId qmlSyntaxIdQuotation { "syntax.id-quotation" };
const LoggerWarningId qmlSyntaxDuplicateIds { "syntax.duplicate-ids" };
const LoggerWarningId qmlCompiler { "compiler" };
const LoggerWarningId qmlControlsSanity { "controls-sanity" };
const LoggerWarningId qmlAttachedPropertyReuse { "attached-property-reuse" };
const LoggerWarningId qmlPlugin { "plugin" };
const LoggerWarningId qmlVarUsedBeforeDeclaration { "var-used-before-declaration" };
const LoggerWarningId qmlInvalidLintDirective { "invalid-lint-directive" };
const LoggerWarningId qmlUseProperFunction { "use-proper-function" };
const LoggerWarningId qmlAccessSingleton { "access-singleton-via-object" };
const LoggerWarningId qmlTopLevelComponent { "top-level-component" };
const LoggerWarningId qmlUncreatableType { "uncreatable-type" };


const QList<QQmlJSLogger::Category> &QQmlJSLogger::defaultCategories()
{
    static const QList<QQmlJSLogger::Category> cats = {
        QQmlJSLogger::Category { qmlRequired.name().toString(), QStringLiteral("RequiredProperty"),
                                 QStringLiteral("Warn about required properties"), QtWarningMsg },
        QQmlJSLogger::Category { qmlAliasCycle.name().toString(),
                                 QStringLiteral("PropertyAliasCycles"),
                                 QStringLiteral("Warn about alias cycles"), QtWarningMsg },
        QQmlJSLogger::Category { qmlUnresolvedAlias.name().toString(),
                                 QStringLiteral("PropertyAliasCycles"),
                                 QStringLiteral("Warn about unresolved aliases"), QtWarningMsg },
        QQmlJSLogger::Category {
                qmlImport.name().toString(), QStringLiteral("ImportFailure"),
                QStringLiteral("Warn about failing imports and deprecated qmltypes"),
                QtWarningMsg },
        QQmlJSLogger::Category {
                qmlRecursionDepthErrors.name().toString(), QStringLiteral("ImportFailure"),
                QStringLiteral("Warn about failing imports and deprecated qmltypes"), QtWarningMsg,
                false, true },
        QQmlJSLogger::Category {
                qmlWith.name().toString(), QStringLiteral("WithStatement"),
                QStringLiteral("Warn about with statements as they can cause false "
                               "positives when checking for unqualified access"),
                QtWarningMsg },
        QQmlJSLogger::Category { qmlInheritanceCycle.name().toString(),
                                 QStringLiteral("InheritanceCycle"),
                                 QStringLiteral("Warn about inheritance cycles"), QtWarningMsg },
        QQmlJSLogger::Category { qmlDeprecated.name().toString(), QStringLiteral("Deprecated"),
                                 QStringLiteral("Warn about deprecated properties and types"),
                                 QtWarningMsg },
        QQmlJSLogger::Category {
                qmlSignalParameters.name().toString(), QStringLiteral("BadSignalHandlerParameters"),
                QStringLiteral("Warn about bad signal handler parameters"), QtWarningMsg },
        QQmlJSLogger::Category { qmlMissingType.name().toString(), QStringLiteral("MissingType"),
                                 QStringLiteral("Warn about missing types"), QtWarningMsg },
        QQmlJSLogger::Category { qmlUnresolvedType.name().toString(),
                                 QStringLiteral("UnresolvedType"),
                                 QStringLiteral("Warn about unresolved types"), QtWarningMsg },
        QQmlJSLogger::Category { qmlRestrictedType.name().toString(),
                                 QStringLiteral("RestrictedType"),
                                 QStringLiteral("Warn about restricted types"), QtWarningMsg },
        QQmlJSLogger::Category { qmlPrefixedImportType.name().toString(),
                                 QStringLiteral("PrefixedImportType"),
                                 QStringLiteral("Warn about prefixed import types"), QtWarningMsg },
        QQmlJSLogger::Category { qmlIncompatibleType.name().toString(),
                                 QStringLiteral("IncompatibleType"),
                                 QStringLiteral("Warn about missing types"), QtWarningMsg },
        QQmlJSLogger::Category { qmlMissingProperty.name().toString(),
                                 QStringLiteral("MissingProperty"),
                                 QStringLiteral("Warn about missing properties"), QtWarningMsg },
        QQmlJSLogger::Category { qmlNonListProperty.name().toString(),
                                 QStringLiteral("NonListProperty"),
                                 QStringLiteral("Warn about non-list properties"), QtWarningMsg },
        QQmlJSLogger::Category {
                qmlReadOnlyProperty.name().toString(), QStringLiteral("ReadOnlyProperty"),
                QStringLiteral("Warn about writing to read-only properties"), QtWarningMsg },
        QQmlJSLogger::Category { qmlDuplicatePropertyBinding.name().toString(),
                                 QStringLiteral("DuplicatePropertyBinding"),
                                 QStringLiteral("Warn about duplicate property bindings"),
                                 QtWarningMsg },
        QQmlJSLogger::Category { qmlDuplicatedName.name().toString(),
                            QStringLiteral("DuplicatedName"),
                            QStringLiteral("Warn about duplicated property/signal names"),
                            QtWarningMsg },
        QQmlJSLogger::Category {
                qmlDeferredPropertyId.name().toString(), QStringLiteral("DeferredPropertyId"),
                QStringLiteral(
                        "Warn about making deferred properties immediate by giving them an id."),
                QtInfoMsg, true, true },
        QQmlJSLogger::Category {
                qmlUnqualified.name().toString(), QStringLiteral("UnqualifiedAccess"),
                QStringLiteral("Warn about unqualified identifiers and how to fix them"),
                QtWarningMsg },
        QQmlJSLogger::Category { qmlUnusedImports.name().toString(),
                                 QStringLiteral("UnusedImports"),
                                 QStringLiteral("Warn about unused imports"), QtInfoMsg },
        QQmlJSLogger::Category { qmlMultilineStrings.name().toString(),
                                 QStringLiteral("MultilineStrings"),
                                 QStringLiteral("Warn about multiline strings"), QtInfoMsg },
        QQmlJSLogger::Category { qmlSyntax.name().toString(), QString(),
                                 QStringLiteral("Syntax errors"), QtWarningMsg, false, true },
        QQmlJSLogger::Category { qmlSyntaxIdQuotation.name().toString(), QString(),
                                 QStringLiteral("ID quotation"), QtWarningMsg, false, true },
        QQmlJSLogger::Category { qmlSyntaxDuplicateIds.name().toString(), QString(),
                                 QStringLiteral("ID duplication"), QtCriticalMsg, false, true },
        QQmlJSLogger::Category { qmlCompiler.name().toString(), QStringLiteral("CompilerWarnings"),
                                 QStringLiteral("Warn about compiler issues"), QtWarningMsg, true },

        QQmlJSLogger::Category {
                qmlControlsSanity.name().toString(), QStringLiteral("ControlsSanity"),
                QStringLiteral("Performance checks used for QuickControl's implementation"),
                QtCriticalMsg, true },

        QQmlJSLogger::Category { qmlAttachedPropertyReuse.name().toString(),
                                 QStringLiteral("AttachedPropertyReuse"),
                                 QStringLiteral("Warn if attached types from parent components "
                                                "aren't reused. This is handled by the QtQuick "
                                                "lint plugin. Use Quick.AttachedPropertyReuse instead."),
                                 QtCriticalMsg, true },
        QQmlJSLogger::Category { qmlPlugin.name().toString(), QStringLiteral("LintPluginWarnings"),
                                 QStringLiteral("Warn if a qmllint plugin finds an issue"),
                                 QtWarningMsg, true },
        QQmlJSLogger::Category { qmlVarUsedBeforeDeclaration.name().toString(),
                                 QStringLiteral("VarUsedBeforeDeclaration"),
                                 QStringLiteral("Warn if a variable is used before declaration"),
                                 QtWarningMsg },
        QQmlJSLogger::Category {
                qmlInvalidLintDirective.name().toString(), QStringLiteral("InvalidLintDirective"),
                QStringLiteral("Warn if an invalid qmllint comment is found"), QtWarningMsg },
        QQmlJSLogger::Category {
                qmlUseProperFunction.name().toString(), QStringLiteral("UseProperFunction"),
                QStringLiteral("Warn if var is used for storing functions"), QtWarningMsg },
        QQmlJSLogger::Category {
                qmlAccessSingleton.name().toString(), QStringLiteral("AccessSingletonViaObject"),
                QStringLiteral("Warn if a singleton is accessed via an object"), QtWarningMsg },
        QQmlJSLogger::Category {
            qmlTopLevelComponent.name().toString(), QStringLiteral("TopLevelComponent"),
            QStringLiteral("Fail when a top level Component are encountered"), QtWarningMsg },
        QQmlJSLogger::Category {
            qmlUncreatableType.name().toString(), QStringLiteral("UncreatableType"),
            QStringLiteral("Warn if uncreatable types are created"), QtWarningMsg }
    };

    return cats;
}

const QList<QQmlJSLogger::Category> QQmlJSLogger::categories() const
{
    return m_categories.values();
}

void QQmlJSLogger::registerCategory(const QQmlJSLogger::Category &category)
{
    if (m_categories.contains(category.name)) {
        qWarning() << "Trying to re-register existing logger category" << category.name;
        return;
    }

    m_categoryLevels[category.name] = category.level;
    m_categoryIgnored[category.name] = category.ignored;
    m_categories.insert(category.name, category);
}

QQmlJSLogger::QQmlJSLogger()
{
    static const QList<QQmlJSLogger::Category> cats = defaultCategories();

    for (const QQmlJSLogger::Category &category : cats)
        registerCategory(category);

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

void QQmlJSLogger::log(const QString &message, LoggerWarningId id,
                       const QQmlJS::SourceLocation &srcLocation, QtMsgType type, bool showContext,
                       bool showFileName, const std::optional<QQmlJSFixSuggestion> &suggestion,
                       const QString overrideFileName)
{
    Q_ASSERT(m_categoryLevels.contains(id.name().toString()));

    if (isCategoryIgnored(id))
        return;

    // Note: assume \a type is the type we should prefer for logging

    if (srcLocation.isValid()
        && m_ignoredWarnings[srcLocation.startLine].contains(id.name().toString()))
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
    m_output.writePrefixedMessage(u"%1%2 [%3]"_s.arg(prefix, message, id.name().toString()), type);

    Message diagMsg;
    diagMsg.message = message;
    diagMsg.id = id.name();
    diagMsg.loc = srcLocation;
    diagMsg.type = type;
    diagMsg.fixSuggestion = suggestion;

    switch (type) {
    case QtWarningMsg: m_warnings.push_back(diagMsg); break;
    case QtCriticalMsg: m_errors.push_back(diagMsg); break;
    case QtInfoMsg: m_infos.push_back(diagMsg); break;
    default: break;
    }

    if (srcLocation.length > 0 && !m_code.isEmpty() && showContext)
        printContext(overrideFileName, srcLocation);

    if (suggestion.has_value())
        printFix(suggestion.value());
}

void QQmlJSLogger::processMessages(const QList<QQmlJS::DiagnosticMessage> &messages,
                                   LoggerWarningId id)
{
    if (messages.isEmpty() || isCategoryIgnored(id))
        return;

    m_output.write(QStringLiteral("---\n"));

    // TODO: we should instead respect message's category here (potentially, it
    // should hold a category instead of type)
    for (const QQmlJS::DiagnosticMessage &message : messages)
        log(message.message, id, QQmlJS::SourceLocation(), false, false);

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
    m_output.write(QString::fromLatin1(" ").repeated(issueLocationWithContext.beforeText().size()
                                                     - tabCount)
                   + QString::fromLatin1("\t").repeated(tabCount)
                   + QString::fromLatin1("^").repeated(locationLength) + QLatin1Char('\n'));
}

void QQmlJSLogger::printFix(const QQmlJSFixSuggestion &fixItem)
{
    const QString currentFileAbsPath = QFileInfo(m_fileName).absolutePath();
    QString code = m_code;
    QString currentFile;
    m_output.writePrefixedMessage(fixItem.fixDescription(), QtInfoMsg);

    if (!fixItem.location().isValid())
        return;

    const QString filename = fixItem.filename();
    if (filename == currentFile) {
        // Nothing to do in this case, we've already read the code
    } else if (filename.isEmpty() || filename == currentFileAbsPath) {
        code = m_code;
    } else {
        QFile file(filename);
        const bool success = file.open(QFile::ReadOnly);
        Q_ASSERT(success);
        code = QString::fromUtf8(file.readAll());
        currentFile = filename;
    }

    IssueLocationWithContext issueLocationWithContext { code, fixItem.location() };

    if (const QStringView beforeText = issueLocationWithContext.beforeText();
        !beforeText.isEmpty()) {
        m_output.write(beforeText);
    }

    // The replacement string can be empty if we're only pointing something out to the user
    const QString replacement = fixItem.replacement();
    QStringView replacementString = replacement.isEmpty()
            ? issueLocationWithContext.issueText()
            : replacement;

    // But if there's nothing to change it cannot be auto-applied
    Q_ASSERT(!replacement.isEmpty() || !fixItem.isAutoApplicable());

    m_output.write(replacementString, QtDebugMsg);
    m_output.write(issueLocationWithContext.afterText().toString() + u'\n');

    int tabCount = issueLocationWithContext.beforeText().count(u'\t');

    // Do not draw location indicator for multiline replacement strings
    if (replacementString.contains(u'\n'))
        return;

    m_output.write(u" "_s.repeated(
                           issueLocationWithContext.beforeText().size() - tabCount)
                   + u"\t"_s.repeated(tabCount)
                   + u"^"_s.repeated(replacement.size()) + u'\n');
}

QT_END_NAMESPACE
