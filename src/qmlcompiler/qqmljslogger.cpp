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

/*
Note: users are in full control of the severity level of a category. It is not possible to
have a single category and use it as a warning in one context, and as an info message in
another context.
For that use case, create two distinct categories instead, and give them appropriate default
warning levels.
 */

/* The X macro provides (in order):
   category, name, setting, description, severity, essentiality
 - category is the C++ variable name of the category
 - name is the user visible category name, i.e. what what qmllint put in bewteen "[" and "]"
 - setting name is the name that is used in the qmllint.ini file to configure the category
 - description is a _short_ description of the category's use
 - severity is the default warning level of the category (can be overriden by the user though)
 - essentiality marks essential categories that are required for the proper function of qmllint and can't be disabled by the user
 */

// clang-format off
// don't forget to forward-declare your logging category ID in qqmljsloggingutils.h!
#define QMLLINT_BUILTIN_CATEGORIES                                                                 \
    X(qmlAccessSingleton, "access-singleton-via-object", "AccessSingletonViaObject",               \
      "Warn if a singleton is accessed via an object", Warning, NonEssential)                      \
    X(qmlAliasCycle, "alias-cycle", "AliasCycle", "Warn about alias cycles", Warning, NonEssential)\
    X(qmlAssignmentInCondition, "assignment-in-condition", "AssignmentInCondition",                \
      "Warn about using assignment in conditions.", Warning, NonEssential)                         \
    X(qmlAttachedPropertyReuse, "attached-property-reuse", "AttachedPropertyReuse",                \
      "Warn if attached types from parent components aren't reused. This is handled by the "       \
      "QtQuick lint plugin. Use Quick.AttachedPropertyReuse instead.",                             \
      Disable, NonEssential)                                                                       \
    X(qmlBlockScopeVarDeclaration, "block-scope-var-declaration", "BlockScopeVarDeclaration",      \
      "Warn if a variable is declared with var inside a block scope", Warning, NonEssential)       \
    X(qmlComma, "comma", "Comma", "Warn about using comma expressions.", Warning, NonEssential)    \
    X(qmlCompiler, "compiler", "CompilerWarnings", "Warn about compiler issues", Disable,          \
      NonEssential)                                                                                \
    X(qmlComponentChildrenCount, "component-children-count", "ComponentChildrenCount",             \
      "Warn about Components that don't have exactly one child", Warning, NonEssential)            \
    X(qmlConfusingExpressionStatement, "confusing-expression-statement",                           \
      "ConfusingExpressionStatement",                                                              \
      "Warn about expression statement that has no obvious effect.", Warning, NonEssential)        \
    X(qmlConfusingMinuses, "confusing-minuses", "ConfusingMinuses",                                \
      "Warn about confusing minuses.", Warning, NonEssential)                                      \
    X(qmlConfusingPluses, "confusing-pluses", "ConfusingPluses",                                   \
      "Warn about confusing pluses.", Warning, NonEssential)                                       \
    X(qmlContextProperties, "context-properties", "ContextProperties",                             \
      "Warn about using context properties.", Warning, NonEssential)                               \
    X(qmlDeferredPropertyId, "deferred-property-id", "DeferredPropertyId",                         \
      "Warn about making deferred properties immediate by giving them an id.", Disable,            \
      NonEssential)                                                                                \
    X(qmlEnumsAreNotTypes, "enums-are-not-types", "EnumsAreNotTypes",                              \
      "Warn about the use of enumerations as types.", Warning, NonEssential)                       \
    X(qmlEqualityTypeCoercion, "equality-type-coercion", "EqualityTypeCoercion",                   \
      "Warn about coercions due to usages of '==' and '!='", Warning, NonEssential)                \
    X(qmlDeprecated, "deprecated", "Deprecated", "Warn about deprecated properties and types",     \
      Warning, NonEssential)                                                                       \
    X(qmlDuplicateEnumEntries, "duplicate-enum-entries", "DuplicateEnumEntries",                   \
      "Warn about duplicate enum entries", Warning, NonEssential)                                  \
    X(qmlDuplicateImport, "duplicate-import", "DuplicateImport", "Warn about duplicate imports",   \
      Warning, NonEssential)                                                                       \
    X(qmlDuplicateInlineComponent, "duplicate-inline-component", "DuplicateInlineComponent",       \
      "Warn about duplicate inline components", Warning, NonEssential)                             \
    X(qmlDuplicatePropertyBinding, "duplicate-property-binding", "DuplicatePropertyBinding",       \
      "Warn about duplicate property bindings", Warning, NonEssential)                             \
    X(qmlDuplicatedName, "duplicated-name", "DuplicatedName",                                      \
      "Warn about duplicated property/signal names", Warning, NonEssential)                        \
    X(qmlEnumEntryMatchesEnum, "enum-entry-matches-enum", "EnumEntryMatchesEnum",                  \
      "Warn about enum entries named the same as the enum itself", Warning, NonEssential)          \
    X(qmlEnumKeyCase, "enum-key-case", "EnumKeyCase", "Warn about lowercase enum keys", Warning,   \
      NonEssential)                                                                                \
    X(qmlEval, "eval", "Eval", "Warn about uses of eval()", Warning, NonEssential)                 \
    X(qmlFunctionUsedBeforeDeclaration, "function-used-before-declaration",                        \
      "FunctionUsedBeforeDeclaration", "Warn if a function is used before declaration",            \
      Disable, NonEssential)                                                                       \
    X(qmlIdShadowsMember, "id-shadows-member", "IdShadowsMember",                                  \
      "Warn about ids potentially shadowing members", Warning, NonEssential)                       \
    X(qmlImport, "import", "ImportFailure", "Warn about failing imports and deprecated qmltypes",  \
      Warning, NonEssential)                                                                       \
    X(qmlImportFileSelector, "import-file-selector", "ImportFileSelector",                         \
        "Warn about encountered file selectors during import", Disable, NonEssential)              \
    X(qmlIncompatibleType, "incompatible-type", "IncompatibleType",                                \
      "Warn about incompatible types", Warning, NonEssential)                                      \
    X(qmlInheritanceCycle, "inheritance-cycle", "InheritanceCycle",                                \
      "Warn about inheritance cycles", Warning, NonEssential)                                      \
    X(qmlInlineComponentEnums, "inline-component-enums", "InlineComponentEnums",                   \
      "Warn about enum declarations inside inline components", Warning, NonEssential)              \
    X(qmlInvalidLintDirective, "invalid-lint-directive", "InvalidLintDirective",                   \
      "Warn if an invalid qmllint comment is found", Warning, NonEssential)                        \
    X(qmlLiteralConstructor, "literal-constructor", "LiteralConstructor",                          \
      "Warn about using literal constructors, like Boolean or String for example.", Warning,       \
      NonEssential)                                                                                \
    X(qmlMissingEnumEntry, "missing-enum-entry", "MissingEnumEntry",                               \
      "Warn about using missing enum values.", Warning, NonEssential)                              \
    X(qmlMissingProperty, "missing-property", "MissingProperty", "Warn about missing properties",  \
      Warning, NonEssential)                                                                       \
    X(qmlMissingType, "missing-type", "MissingType", "Warn about missing types", Warning,          \
      NonEssential)                                                                                \
    X(qmlMultilineStrings, "multiline-strings", "MultilineStrings",                                \
      "Warn about multiline strings", Info, NonEssential)                                          \
    X(qmlNonListProperty, "non-list-property", "NonListProperty",                                  \
      "Warn about non-list properties", Warning, NonEssential)                                     \
    X(qmlNonRootEnums, "non-root-enum", "NonRootEnum",                                             \
      "Warn about enums defined outside the root component", Warning, NonEssential)                \
    X(qmlPropertyOverride, "property-override", "PropertyOverride",                                \
      "Warn about wrongly overriding properties from a base class", Warning, NonEssential)         \
    X(qmlUnterminatedCase, "unterminated-case", "UnterminatedCase", "Warn about non-empty case "   \
      "blocks that are not terminated by control flow or by a fallthrough comment", Warning,       \
      NonEssential)                                                                                \
    X(qmlPreferNonVarProperties, "prefer-non-var-properties", "PreferNonVarProperties",            \
      "Warn about var properties that could use a more specific type", Warning, NonEssential)      \
    X(qmlPrefixedImportType, "prefixed-import-type", "PrefixedImportType",                         \
      "Warn about prefixed import types", Warning, NonEssential)                                   \
    X(qmlReadOnlyProperty, "read-only-property", "ReadOnlyProperty",                               \
      "Warn about writing to read-only properties", Warning, NonEssential)                         \
    X(qmlRecursionDepthErrors, "recursion-depth-errors", "", "", Warning, NonEssential)            \
    X(qmlRedundantOptionalChaining, "redundant-optional-chaining", "RedundantOptionalChaining",    \
      "Warn about optional chaining on non-voidable and non-nullable base", Warning, NonEssential) \
    X(qmlRenamedType, "renamed-type", "RenamedType",                                               \
      "Warn when renamed types refer to themselves using their unrenamed name", Warning,           \
      NonEssential)                                                                                \
    X(qmlRequired, "required", "RequiredProperty", "Warn about required properties", Warning,      \
      NonEssential)                                                                                \
    X(qmlShadow, "shadow", "Shadow", "Warn about shadowing attributes from a base class", Disable, \
      NonEssential)                                                                                \
    X(qmlSignalParameters, "signal-handler-parameters", "BadSignalHandlerParameters",              \
      "Warn about bad signal handler parameters", Warning, NonEssential)                           \
    X(qmlStalePropertyRead, "stale-property-read", "StalePropertyRead",                            \
      "Warn about bindings reading non-constant and non-notifiable properties", Warning,           \
      NonEssential)                                                                                \
    X(qmlSyntax, "syntax", "Syntax", "Syntax errors", Warning, Essential)                          \
    X(qmlSyntaxDuplicateIds, "syntax.duplicate-ids", "", "ID duplication", Error, NonEssential)    \
    X(qmlSyntaxIdQuotation, "syntax.id-quotation", "", "ID quotation", Warning, NonEssential)      \
    X(qmlTypeInstantiatedRecursively, "type-instantiated-recursively",                             \
      "TypeInstantiatedRecursively", "Warn when types are instantiated recursively", Warning,      \
      NonEssential)                                                                                \
    X(qmlTopLevelComponent, "top-level-component", "TopLevelComponent",                            \
      "Warn if a top level Component is encountered", Warning, NonEssential)                       \
    X(qmlUncreatableType, "uncreatable-type", "UncreatableType",                                   \
      "Warn if uncreatable types are created", Warning, NonEssential)                              \
    X(qmlUnintentionalEmptyBlock, "unintentional-empty-block", "UnintentionalEmptyBlock",          \
      "Warn about bindings that contain only an empty block", Warning, NonEssential)               \
    X(qmlUnqualified, "unqualified", "UnqualifiedAccess",                                          \
      "Warn about unqualified identifiers and how to fix them", Warning, NonEssential)             \
    X(qmlUnreachableCode, "unreachable-code", "UnreachableCode", "Warn about unreachable code.",   \
      Warning, NonEssential)                                                                       \
    X(qmlUnresolvedAlias, "unresolved-alias", "UnresolvedAlias", "Warn about unresolved aliases",  \
      Warning, NonEssential)                                                                       \
    X(qmlUnresolvedType, "unresolved-type", "UnresolvedType", "Warn about unresolved types",       \
      Warning, NonEssential)                                                                       \
    X(qmlUnusedImports, "unused-imports", "UnusedImports", "Warn about unused imports", Info,      \
      NonEssential)                                                                                \
    X(qmlUseProperFunction, "use-proper-function", "UseProperFunction",                            \
      "Warn if var is used for storing functions", Disable, NonEssential)                          \
    X(qmlVarUsedBeforeDeclaration, "var-used-before-declaration", "VarUsedBeforeDeclaration",      \
      "Warn if a variable is used before declaration", Warning, NonEssential)                      \
    X(qmlVoid, "void", "Void", "Warn about void expressions.", Disable, NonEssential)              \
    X(qmlWith, "with", "WithStatement",                                                            \
      "Warn about with statements as they can cause NonEssential "                                 \
      "positives when checking for unqualified access",  Warning, NonEssential)                    \

// clang-format on

#define X(category, name, setting, description, severity, essential) \
    const QQmlSA::LoggerWarningId category{ name };
QMLLINT_BUILTIN_CATEGORIES
#undef X


#define X(category, name, setting, description, severity, essential) ++i;
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

#define X(category, name, setting, description, severity, essential) std::string_view(name),
static_assert(isUnique(std::array{ QMLLINT_BUILTIN_CATEGORIES }), "Duplicate names found!");
#undef X

#define X(category, name, setting, description, severity, essential) std::string_view(setting),
static_assert(isUnique(std::array{ QMLLINT_BUILTIN_CATEGORIES }), "Duplicate settings found!");
#undef X

#define X(category, name, setting, description, severity, essential) std::string_view(description),
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
#define X(category, name, setting, description, severity, essential) \
    QQmlJS::LoggerCategory{ name##_L1, setting##_L1, description##_L1, QQmlJS::WarningSeverity::severity, QQmlJS::LoggerCategory::essential },
        QMLLINT_BUILTIN_CATEGORIES
#undef X
    };

    return cats;
}

bool QQmlJSFixSuggestion::operator==(const QQmlJSFixSuggestion &other) const
{
    return m_location == other.m_location && m_description == other.m_description
            && m_documentEdits == other.m_documentEdits && m_filename == other.m_filename
            && m_autoApplicable == other.m_autoApplicable;
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

    m_categorySeverities[category.name()] = category.severity();
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

void QQmlJSLogger::log(Message &&diagMsg, bool showContext, bool showFileName)
{
    Q_ASSERT(m_categorySeverities.contains(diagMsg.id.toString()));

    if (categorySeverity(diagMsg.id) == QQmlJS::WarningSeverity::Disable || isDisabled())
        return;

    // Note: assume \a type is the type we should prefer for logging

    if (diagMsg.loc.isValid()
        && m_ignoredWarnings[diagMsg.lineForDisabling()].contains(diagMsg.id.toString())) {
        return;
    }

    QString prefix;
    if (!m_filePath.isEmpty() && showFileName)
        prefix = m_filePath + QStringLiteral(":");

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
        printContext(diagMsg.loc);

    if (diagMsg.fixSuggestion.has_value())
        printFix(diagMsg.fixSuggestion.value());

    if (m_inTransaction) {
        m_pendingMessages.push_back(std::move(diagMsg));
    } else {
        countMessage(diagMsg);
        m_currentFunctionMessages.push_back(std::move(diagMsg));
    }

    if (!m_inTransaction && !m_manualFlush)
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
    if (messages.isEmpty() || categorySeverity(id) == QQmlJS::WarningSeverity::Disable || isDisabled())
        return;

    m_output.write(QStringLiteral("---\n"));

    // TODO: we should instead respect message's category here (potentially, it
    // should hold a category instead of type)
    for (const QQmlJS::DiagnosticMessage &message : messages)
        log(message.message, id, sourceLocation, false, false);

    m_output.write(QStringLiteral("---\n\n"));
}

void QQmlJSLogger::finalizeFunction()
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
    if (!m_manualFlush)
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

void QQmlJSLogger::printContext(const QQmlJS::SourceLocation &location)
{
    QString code = m_code;

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
    m_output.writePrefixedMessage(fixItem.description(), QtInfoMsg);

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

    const auto &documentEdits = fixItem.documentEdits();
    if (!documentEdits.empty()) {
        m_output.write(documentEdits.size() == 1 ? "Suggested change:\n"_L1
                                                 : "Suggested changes:\n"_L1);
        for (const auto &documentEdit: documentEdits) {
            if (!documentEdit.m_location.isValid())
                continue;

            IssueLocationWithContext issueLocationWithContext { code, documentEdit.m_location };

            if (const QStringView beforeText = issueLocationWithContext.beforeText();
                !beforeText.isEmpty()) {
                m_output.write(beforeText);
            }

            // The replacement string can be empty if we're only pointing something out to the user
            const QString replacement = documentEdit.m_replacement;
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
        }
    }
}

QQmlJSFixSuggestion::QQmlJSFixSuggestion(const QString &description,
                                         const QQmlJS::SourceLocation &location,
                                         const QQmlJSDocumentEdit &documentEdit)
    : QQmlJSFixSuggestion(description, location, QList{ documentEdit })
{
}

QQmlJSFixSuggestion::QQmlJSFixSuggestion(const QString &description,
                                         const QQmlJS::SourceLocation &location,
                                         const QList<QQmlJSDocumentEdit> &documentEdits)
    : m_description{ description }, m_location{ location }, m_documentEdits(documentEdits)
{
}

void QQmlJSFixSuggestion::addDocumentEdit(const QQmlJSDocumentEdit &documentEdit)
{
    m_documentEdits.append(documentEdit);
}

void QQmlJSFixSuggestion::setDocumentEdits(const QList<QQmlJSDocumentEdit> &documentEdits)
{
    m_documentEdits = documentEdits;
}

QT_END_NAMESPACE
