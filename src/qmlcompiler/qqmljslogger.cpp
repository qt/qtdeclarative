// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
// Qt-Security score:significant

#include <QtCore/qcompilerdetection.h>
// GCC 11 thinks diagMsg.fixSuggestion.fixes.d.ptr is somehow uninitialized in
// QList::emplaceBack(), probably called from QQmlJsLogger::log()
// Ditto for GCC 12, but it emits a different warning
QT_WARNING_PUSH
QT_WARNING_DISABLE_GCC("-Wuninitialized")
QT_WARNING_DISABLE_GCC("-Wmaybe-uninitialized")
#include <QtCore/qlist.h>
QT_WARNING_POP

#include <private/qqmljslogger_p.h>
#include <private/qqmlsa_p.h>

#include <QtQmlCompiler/qqmljsloggingutils.h>

#include <QtCore/qglobal.h>
#include <QtCore/qfile.h>


QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

// don't forget to forward-declare your logging category ID in qqmljsloggingutils.h!
#define QMLLINT_BUILTIN_CATEGORIES                                                                 \
    X(qmlAccessSingleton, "access-singleton-via-object", "AccessSingletonViaObject",               \
      "Warn if a singleton is accessed via an object", QtWarningMsg, false, false)                 \
    X(qmlAliasCycle, "alias-cycle", "AliasCycle", "Warn about alias cycles", QtWarningMsg, false,  \
      false)                                                                                       \
    X(qmlAssignmentInCondition, "assignment-in-condition", "AssignmentInCondition",                \
      "Warn about using assignment in conditions.", QtWarningMsg, false, false)                    \
    X(qmlAttachedPropertyReuse, "attached-property-reuse", "AttachedPropertyReuse",                \
      "Warn if attached types from parent components aren't reused. This is handled by the "       \
      "QtQuick lint plugin. Use Quick.AttachedPropertyReuse instead.",                             \
      QtCriticalMsg, true, false)                                                                  \
    X(qmlComma, "comma", "Comma", "Warn about using comma expressions.", QtWarningMsg, false,      \
      false)                                                                                       \
    X(qmlCompiler, "compiler", "CompilerWarnings", "Warn about compiler issues", QtWarningMsg,     \
      true, false)                                                                                 \
    X(qmlComponentChildrenCount, "component-children-count", "ComponentChildrenCount",             \
      "Warn about Components that don't have exactly one child", QtWarningMsg, false, false)       \
    X(qmlConfusingExpressionStatement, "confusing-expression-statement",                           \
      "ConfusingExpressionStatement",                                                              \
      "Warn about expression statement that has no obvious effect.", QtWarningMsg, false, false)   \
    X(qmlConfusingMinuses, "confusing-minuses", "ConfusingMinuses",                                \
      "Warn about confusing minuses.", QtWarningMsg, false, false)                                 \
    X(qmlConfusingPluses, "confusing-pluses", "ConfusingPluses",                                   \
      "Warn about confusing pluses.", QtWarningMsg, false, false)                                  \
    X(qmlContextProperties, "context-properties", "ContextProperties",                             \
      "Warn about using context properties.", QtWarningMsg, false, false)                          \
    X(qmlDeferredPropertyId, "deferred-property-id", "DeferredPropertyId",                         \
      "Warn about making deferred properties immediate by giving them an id.", QtInfoMsg, true,    \
      true)                                                                                        \
    X(qmlEnumsAreNotTypes, "enums-are-not-types", "EnumsAreNotTypes",                              \
      "Warn about the use of enumerations as types.", QtWarningMsg, false, false)                  \
    X(qmlEqualityTypeCoercion, "equality-type-coercion", "EqualityTypeCoercion",                   \
      "Warn about coercions due to usages of '==' and '!='", QtWarningMsg, false, false)           \
    X(qmlDeprecated, "deprecated", "Deprecated", "Warn about deprecated properties and types",     \
      QtWarningMsg, false, false)                                                                  \
    X(qmlDuplicateEnumEntries, "duplicate-enum-entries", "DuplicateEnumEntries",                   \
      "Warn about duplicate enum entries", QtWarningMsg, false, false)                             \
    X(qmlDuplicateImport, "duplicate-import", "DuplicateImport", "Warn about duplicate imports",   \
      QtWarningMsg, false, false)                                                                  \
    X(qmlDuplicateInlineComponent, "duplicate-inline-component", "DuplicateInlineComponent",       \
      "Warn about duplicate inline components", QtWarningMsg, false, false)                        \
    X(qmlDuplicatePropertyBinding, "duplicate-property-binding", "DuplicatePropertyBinding",       \
      "Warn about duplicate property bindings", QtWarningMsg, false, false)                        \
    X(qmlDuplicatedName, "duplicated-name", "DuplicatedName",                                      \
      "Warn about duplicated property/signal names", QtWarningMsg, false, false)                   \
    X(qmlEnumEntryMatchesEnum, "enum-entry-matches-enum", "EnumEntryMatchesEnum",                  \
      "Warn about enum entries named the same as the enum itself", QtWarningMsg, false, false)     \
    X(qmlEval, "eval", "Eval", "Warn about uses of eval()", QtWarningMsg, false, false)            \
    X(qmlFunctionUsedBeforeDeclaration, "function-used-before-declaration",                        \
      "FunctionUsedBeforeDeclaration", "Warn if a function is used before declaration",            \
      QtWarningMsg, true, false)                                                                   \
    X(qmlImport, "import", "ImportFailure", "Warn about failing imports and deprecated qmltypes",  \
      QtWarningMsg, false, false)                                                                  \
    X(qmlImportFileSelector, "import-file-selector", "ImportFileSelector",                         \
        "Warn about encountered file selectors during import", QtInfoMsg, true, false)             \
    X(qmlIncompatibleType, "incompatible-type", "IncompatibleType",                                \
      "Warn about incompatible types", QtWarningMsg, false, false)                                 \
    X(qmlInheritanceCycle, "inheritance-cycle", "InheritanceCycle",                                \
      "Warn about inheritance cycles", QtWarningMsg, false, false)                                 \
    X(qmlInvalidLintDirective, "invalid-lint-directive", "InvalidLintDirective",                   \
      "Warn if an invalid qmllint comment is found", QtWarningMsg, false, false)                   \
    X(qmlLiteralConstructor, "literal-constructor", "LiteralConstructor",                          \
      "Warn about using literal constructors, like Boolean or String for example.", QtWarningMsg,  \
      false, false)                                                                                \
    X(qmlMissingEnumEntry, "missing-enum-entry", "MissingEnumEntry",                               \
      "Warn about using missing enum values.", QtWarningMsg, false, false)                         \
    X(qmlMissingProperty, "missing-property", "MissingProperty", "Warn about missing properties",  \
      QtWarningMsg, false, false)                                                                  \
    X(qmlMissingType, "missing-type", "MissingType", "Warn about missing types", QtWarningMsg,     \
      false, false)                                                                                \
    X(qmlMultilineStrings, "multiline-strings", "MultilineStrings",                                \
      "Warn about multiline strings", QtInfoMsg, false, false)                                     \
    X(qmlNonListProperty, "non-list-property", "NonListProperty",                                  \
      "Warn about non-list properties", QtWarningMsg, false, false)                                \
    X(qmlNonRootEnums, "non-root-enum", "NonRootEnum",                                             \
      "Warn about enums defined outside the root component", QtWarningMsg, false, false)           \
    X(qmlPropertyOverride, "property-override", "PropertyOverride",                                \
      "Warn about wrongly overriding properties from a base class", QtWarningMsg, false, false)    \
    X(qmlUnterminatedCase, "unterminated-case", "UnterminatedCase",                                \
      "Warn about non-empty case "                                                                 \
      "blocks that are not terminated by control flow or by a fallthrough comment",                \
      QtWarningMsg, false, false)                                                                  \
    X(qmlPlugin, "plugin", "LintPluginWarnings", "Warn if a qmllint plugin finds an issue",        \
      QtWarningMsg, true, false)                                                                   \
    X(qmlPreferNonVarProperties, "prefer-non-var-properties", "PreferNonVarProperties",            \
      "Warn about var properties that could use a more specific type", QtWarningMsg, false, false) \
    X(qmlPrefixedImportType, "prefixed-import-type", "PrefixedImportType",                         \
      "Warn about prefixed import types", QtWarningMsg, false, false)                              \
    X(qmlReadOnlyProperty, "read-only-property", "ReadOnlyProperty",                               \
      "Warn about writing to read-only properties", QtWarningMsg, false, false)                    \
    X(qmlRecursionDepthErrors, "recursion-depth-errors", "", "", QtWarningMsg, false, true)        \
    X(qmlRedundantOptionalChaining, "redundant-optional-chaining", "RedundantOptionalChaining",    \
      "Warn about optional chaining on non-voidable and non-nullable base", QtWarningMsg, false,   \
      false)                                                                                       \
    X(qmlRequired, "required", "RequiredProperty", "Warn about required properties", QtWarningMsg, \
      false, false)                                                                                \
    X(qmlShadow, "shadow", "Shadow", "Warn about shadowing attributes from a base class",          \
      QtWarningMsg, true, false)                                                                   \
    X(qmlSignalParameters, "signal-handler-parameters", "BadSignalHandlerParameters",              \
      "Warn about bad signal handler parameters", QtWarningMsg, false, false)                      \
    X(qmlStalePropertyRead, "stale-property-read", "StalePropertyRead",                            \
      "Warn about bindings reading non-constant and non-notifiable properties", QtWarningMsg,      \
      false, false)                                                                                \
    X(qmlSyntax, "syntax", "", "Syntax errors", QtWarningMsg, false, true)                         \
    X(qmlSyntaxDuplicateIds, "syntax.duplicate-ids", "", "ID duplication", QtCriticalMsg, false,   \
      true)                                                                                        \
    X(qmlSyntaxIdQuotation, "syntax.id-quotation", "", "ID quotation", QtWarningMsg, false, true)  \
    X(qmlTopLevelComponent, "top-level-component", "TopLevelComponent",                            \
      "Warn if a top level Component is encountered", QtWarningMsg, false, false)                  \
    X(qmlTranslationFunctionMismatch, "translation-function-mismatch",                             \
      "TranslationFunctionMismatch",                                                               \
      "Warn about usages of ID and non-ID translation functions in the same file.", QtWarningMsg,  \
      false, false)                                                                                \
    X(qmlUncreatableType, "uncreatable-type", "UncreatableType",                                   \
      "Warn if uncreatable types are created", QtWarningMsg, false, false)                         \
    X(qmlUnintentionalEmptyBlock, "unintentional-empty-block", "UnintentionalEmptyBlock",          \
      "Warn about bindings that contain only an empty block", QtWarningMsg, false, false)          \
    X(qmlUnqualified, "unqualified", "UnqualifiedAccess",                                          \
      "Warn about unqualified identifiers and how to fix them", QtWarningMsg, false, false)        \
    X(qmlUnreachableCode, "unreachable-code", "UnreachableCode", "Warn about unreachable code.",   \
      QtWarningMsg, false, false)                                                                  \
    X(qmlUnresolvedAlias, "unresolved-alias", "UnresolvedAlias", "Warn about unresolved aliases",  \
      QtWarningMsg, false, false)                                                                  \
    X(qmlUnresolvedType, "unresolved-type", "UnresolvedType", "Warn about unresolved types",       \
      QtWarningMsg, false, false)                                                                  \
    X(qmlUnusedImports, "unused-imports", "UnusedImports", "Warn about unused imports", QtInfoMsg, \
      false, false)                                                                                \
    X(qmlUseProperFunction, "use-proper-function", "UseProperFunction",                            \
      "Warn if var is used for storing functions", QtWarningMsg, true, false)                      \
    X(qmlVarUsedBeforeDeclaration, "var-used-before-declaration", "VarUsedBeforeDeclaration",      \
      "Warn if a variable is used before declaration", QtWarningMsg, false, false)                 \
    X(qmlVoid, "void", "Void", "Warn about void expressions.", QtWarningMsg, true, false)          \
    X(qmlWith, "with", "WithStatement",                                                            \
      "Warn about with statements as they can cause false "                                        \
      "positives when checking for unqualified access",                                            \
      QtWarningMsg, false, false)

#define X(category, name, setting, description, level, ignored, isDefault) \
    const QQmlSA::LoggerWarningId category{ name };
QMLLINT_BUILTIN_CATEGORIES
#undef X


#define X(category, name, setting, description, level, ignored, isDefault) ++i;
constexpr size_t numCategories = [] { size_t i = 0; QMLLINT_BUILTIN_CATEGORIES return i; }();
#undef X

constexpr bool isUnique(const std::array<std::string_view, numCategories>& fields) {
    for (std::size_t i = 0; i < fields.size(); ++i) {
        for (std::size_t j = i + 1; j < fields.size(); ++j) {
            if (!fields[i].empty() && fields[i] == fields[j]) {
                return false;
            }
        }
    }
    return true;
}

#define X(category, name, setting, description, level, ignored, isDefault) std::string_view(name),
static_assert(isUnique(std::array{ QMLLINT_BUILTIN_CATEGORIES }), "Duplicate names found!");
#undef X

#define X(category, name, setting, description, level, ignored, isDefault) std::string_view(setting),
static_assert(isUnique(std::array{ QMLLINT_BUILTIN_CATEGORIES }), "Duplicate settings found!");
#undef X

#define X(category, name, setting, description, level, ignored, isDefault) std::string_view(description),
static_assert(isUnique(std::array{ QMLLINT_BUILTIN_CATEGORIES }), "Duplicate description found!");
#undef X


QQmlJSLogger::QQmlJSLogger()
{
    static const QList<QQmlJS::LoggerCategory> cats = builtinCategories();

    for (const QQmlJS::LoggerCategory &category : cats)
        registerCategory(category);

    // setup color output
    m_output.insertMapping(QtCriticalMsg, QColorOutput::RedForeground);
    m_output.insertMapping(QtWarningMsg, QColorOutput::PurpleForeground); // Yellow?
    m_output.insertMapping(QtInfoMsg, QColorOutput::BlueForeground);
    m_output.insertMapping(QtDebugMsg, QColorOutput::GreenForeground); // None?
}

const QList<QQmlJS::LoggerCategory> &QQmlJSLogger::builtinCategories()
{
    static const QList<QQmlJS::LoggerCategory> cats = {
#define X(category, name, setting, description, level, ignored, isDefault) \
    QQmlJS::LoggerCategory{ name##_L1, setting##_L1, description##_L1, level, ignored, isDefault },
        QMLLINT_BUILTIN_CATEGORIES
#undef X
    };

    return cats;
}

bool QQmlJSFixSuggestion::operator==(const QQmlJSFixSuggestion &other) const
{
    return m_location == other.m_location && m_fixDescription == other.m_fixDescription
            && m_replacement == other.m_replacement && m_filename == other.m_filename
            && m_hint == other.m_hint && m_autoApplicable == other.m_autoApplicable;
}

bool QQmlJSFixSuggestion::operator!=(const QQmlJSFixSuggestion &other) const
{
    return !(*this == other);
}

QList<QQmlJS::LoggerCategory> QQmlJSLogger::categories() const
{
    return m_categories.values();
}

void QQmlJSLogger::registerCategory(const QQmlJS::LoggerCategory &category)
{
    if (m_categories.contains(category.name())) {
        qWarning() << "Trying to re-register existing logger category" << category.name();
        return;
    }

    m_categoryLevels[category.name()] = category.level();
    m_categoryIgnored[category.name()] = category.isIgnored();
    m_categories.insert(category.name(), category);
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

void QQmlJSLogger::log(
        Message diagMsg, bool showContext, bool showFileName, const QString overrideFileName)
{
    Q_ASSERT(m_categoryLevels.contains(diagMsg.id.toString()));

    if (isCategoryIgnored(diagMsg.id) || isDisabled())
        return;

    // Note: assume \a type is the type we should prefer for logging

    if (diagMsg.loc.isValid()
        && m_ignoredWarnings[diagMsg.lineForDisabling()].contains(diagMsg.id.toString())) {
        return;
    }

    QString prefix;

    if ((!overrideFileName.isEmpty() || !m_filePath.isEmpty()) && showFileName) {
        prefix = (!overrideFileName.isEmpty() ? overrideFileName : m_filePath)
                + QStringLiteral(":");
    }

    if (diagMsg.loc.isValid())
        prefix += QStringLiteral("%1:%2: ").arg(diagMsg.loc.startLine).arg(diagMsg.loc.startColumn);
    else if (!prefix.isEmpty())
        prefix += QStringLiteral(": "); // produce double colon for Qt Creator's issues pane

    // Note: we do the clamping to [Info, Critical] range since our logger only
    // supports 3 categories
    diagMsg.type = std::clamp(diagMsg.type, QtInfoMsg, QtCriticalMsg, isMsgTypeLess);

    // Note: since we clamped our \a type, the output message is not printed
    // exactly like it was requested, bear with us
    m_output.writePrefixedMessage(
            u"%1%2 [%3]"_s.arg(prefix, diagMsg.message, diagMsg.id.toString()), diagMsg.type);

    if (diagMsg.loc.length > 0 && !m_code.isEmpty() && showContext)
        printContext(overrideFileName, diagMsg.loc);

    if (diagMsg.fixSuggestion.has_value())
        printFix(diagMsg.fixSuggestion.value());

    if (m_inTransaction) {
        m_pendingMessages.push_back(std::move(diagMsg));
    } else {
        countMessage(diagMsg);
        m_currentFunctionMessages.push_back(std::move(diagMsg));
    }

    if (!m_inTransaction)
        m_output.flushBuffer();
}

void QQmlJSLogger::countMessage(const Message &message)
{
    switch (message.type) {
    case QtWarningMsg:
        ++m_numWarnings;
        break;
    case QtCriticalMsg:
        ++m_numErrors;
        break;
    default:
        break;
    }
}

void QQmlJSLogger::processMessages(QSpan<const QQmlJS::DiagnosticMessage> messages,
                                   QQmlJS::LoggerWarningId id,
                                   const QQmlJS::SourceLocation &sourceLocation)
{
    if (messages.isEmpty() || isCategoryIgnored(id) || isDisabled())
        return;

    m_output.write(QStringLiteral("---\n"));

    // TODO: we should instead respect message's category here (potentially, it
    // should hold a category instead of type)
    for (const QQmlJS::DiagnosticMessage &message : messages)
        log(message.message, id, sourceLocation, false, false);

    m_output.write(QStringLiteral("---\n\n"));
}

void QQmlJSLogger::finalizeFuction()
{
    Q_ASSERT(!m_inTransaction);
    m_archivedMessages.append(std::exchange(m_currentFunctionMessages, {}));
    m_hasCompileError = false;
}

/*!
    \internal
    Starts a transaction for a compile pass. This buffers all messages until the
    transaction completes. If you commit the transaction, the messages are printed
    and added to the list of committed messages. If you roll it back, the logger
    reverts to the state before the start of the transaction.

    This is useful for compile passes that potentially have to be repeated, such
    as the type propagator. We don't want to see the same messages logged multiple
    times.
 */
void QQmlJSLogger::startTransaction()
{
    Q_ASSERT(!m_inTransaction);
    m_inTransaction = true;
}

/*!
    \internal
    Commit the current transaction. Print all pending messages, and add them to
    the list of committed messages. Then, clear the transaction flag.
 */
void QQmlJSLogger::commit()
{
    Q_ASSERT(m_inTransaction);
    for (const Message &message : std::as_const(m_pendingMessages))
        countMessage(message);

    m_currentFunctionMessages.append(std::exchange(m_pendingMessages, {}));
    m_hasCompileError = m_hasCompileError || std::exchange(m_hasPendingCompileError, false);
    m_output.flushBuffer();
    m_inTransaction = false;
}

/*!
    \internal
    Roll back the current transaction and revert the logger to the state before
    it was started.
 */
void QQmlJSLogger::rollback()
{
    Q_ASSERT(m_inTransaction);
    m_pendingMessages.clear();
    m_hasPendingCompileError = false;
    m_output.discardBuffer();
    m_inTransaction = false;
}

void QQmlJSLogger::printContext(const QString &overrideFileName,
                                const QQmlJS::SourceLocation &location)
{
    QString code = m_code;

    if (!overrideFileName.isEmpty() && overrideFileName != m_filePath) {
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
    const QString currentFileAbsPath = m_filePath;
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

    if (!replacementString.isEmpty())
        m_output.write(replacementString, QtDebugMsg);
    m_output.write(issueLocationWithContext.afterText().toString() + u'\n');

    int tabCount = issueLocationWithContext.beforeText().count(u'\t');

    // Do not draw location indicator for multiline replacement strings
    if (!replacementString.contains(u'\n')) {
        m_output.write(u" "_s.repeated(
                               issueLocationWithContext.beforeText().size() - tabCount)
                       + u"\t"_s.repeated(tabCount)
                       + u"^"_s.repeated(replacement.size()) + u'\n');
    }

    if (!fixItem.hint().isEmpty())
        m_output.write("      "_L1 + fixItem.hint());
}

QQmlJSFixSuggestion::QQmlJSFixSuggestion(const QString &fixDescription,
                                         const QQmlJS::SourceLocation &location,
                                         const QString &replacement)
    : m_location{ location }, m_fixDescription{ fixDescription }, m_replacement{ replacement }
{
}

QT_END_NAMESPACE
