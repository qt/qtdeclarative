// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qqmllscompletion_p.h"

using namespace QLspSpecification;
using namespace QQmlJS::Dom;
using namespace Qt::StringLiterals;

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(QQmlLSCompletionLog, "qt.languageserver.completions")

/*!
\class QQmlLSCompletion
\internal
\brief QQmlLSCompletion provides completions for all kinds of QML and JS constructs.

Use the \l{completions} method to obtain completions at a certain DomItem.

All the other methods in this class are helper methods: some compute completions for specific QML
and JS constructs and some are shared between multiple QML or JS constructs to avoid code
duplication. Most of the helper methods add their completion items via a BackInsertIterator.

Some helper methods are called "suggest*" and will try to suggest code that does not exist yet. For
example, any JS statement can be expected inside a Blockstatement so suggestJSStatementCompletion()
is used to suggest JS statements inside of BlockStatements. Another example might be
suggestReachableTypes() that will suggest Types for type annotations, attached types or Qml Object
hierarchies, or suggestCaseAndDefaultStatementCompletion() that will only suggest "case" and
"default" clauses for switch statements.

Some helper methods are called "inside*" and will try to suggest code inside an existing structure.
For example, insideForStatementCompletion() will try to suggest completion for the different code
pieces initializer, condition, increment and statement that exist inside of:
\badcode
for(initializer; condition; increment)
    statement
\endcode
*/

CompletionItem QQmlLSCompletion::makeSnippet(QUtf8StringView qualifier, QUtf8StringView label,
                                             QUtf8StringView insertText)
{
    CompletionItem res;
    if (!qualifier.isEmpty()) {
        res.label = qualifier.data();
        res.label += '.';
    }
    res.label += label.data();
    res.insertTextFormat = InsertTextFormat::Snippet;
    if (!qualifier.isEmpty()) {
        res.insertText = qualifier.data();
        *res.insertText += '.';
        *res.insertText += insertText.data();
    } else {
        res.insertText = insertText.data();
    }
    res.kind = int(CompletionItemKind::Snippet);
    res.insertTextMode = InsertTextMode::AdjustIndentation;
    return res;
}

CompletionItem QQmlLSCompletion::makeSnippet(QUtf8StringView label, QUtf8StringView insertText)
{
    return makeSnippet(QByteArray(), label, insertText);
}

/*!
\internal
\brief Compare left and right locations to the position denoted by ctx, see special cases below.

Statements and expressions need to provide different completions depending on where the cursor is.
For example, lets take following for-statement:
\badcode
for (let i = 0; <here> ; ++i) {}
\endcode
We want to provide script expression completion (method names, property names, available JS
variables names, QML objects ids, and so on) at the place denoted by \c{<here>}.
The question is: how do we know that the cursor is really at \c{<here>}? In the case of the
for-loop, we can compare the position of the cursor with the first and the second semicolon of the
for loop.

If the first semicolon does not exist, it has an invalid sourcelocation and the cursor is
definitively \e{not} at \c{<here>}. Therefore, return false when \c{left} is invalid.

If the second semicolon does not exist, then just ignore it: it might not have been written yet.
*/
bool QQmlLSCompletion::betweenLocations(QQmlJS::SourceLocation left,
                                        const QQmlLSCompletionPosition &positionInfo,
                                        QQmlJS::SourceLocation right) const
{
    if (!left.isValid())
        return false;
    // note: left.end() == ctx.offset() means that the cursor lies exactly after left
    if (!(left.end() <= positionInfo.offset()))
        return false;
    if (!right.isValid())
        return true;

    // note: ctx.offset() == right.begin() means that the cursor lies exactly before right
    return positionInfo.offset() <= right.begin();
}

/*!
\internal
Returns true if ctx denotes an offset lying behind left.end(), and false otherwise.
*/
bool QQmlLSCompletion::afterLocation(QQmlJS::SourceLocation left,
                                     const QQmlLSCompletionPosition &positionInfo) const
{
    return betweenLocations(left, positionInfo, QQmlJS::SourceLocation{});
}

/*!
\internal
Returns true if ctx denotes an offset lying before right.begin(), and false otherwise.
*/
bool QQmlLSCompletion::beforeLocation(const QQmlLSCompletionPosition &ctx,
                                      QQmlJS::SourceLocation right) const
{
    if (!right.isValid())
        return true;

    // note: ctx.offset() == right.begin() means that the cursor lies exactly before right
    if (ctx.offset() <= right.begin())
        return true;

    return false;
}

bool QQmlLSCompletion::ctxBeforeStatement(const QQmlLSCompletionPosition &positionInfo,
                                          const DomItem &parentForContext,
                                          FileLocationRegion firstRegion) const
{
    const auto regions = FileLocations::treeOf(parentForContext)->info().regions;
    const bool result = beforeLocation(positionInfo, regions[firstRegion]);
    return result;
}

void
QQmlLSCompletion::suggestBindingCompletion(const DomItem &itemAtPosition, BackInsertIterator it) const
{
    suggestReachableTypes(itemAtPosition, LocalSymbolsType::AttachedType, CompletionItemKind::Class,
                          it);

    const auto scope = [&]() -> QQmlJSScope::ConstPtr {
        const DomItem owner = ownerOfQualifiedExpression(itemAtPosition);
        if (!owner)
            return itemAtPosition.qmlObject().semanticScope();

        const auto expressionType = QQmlLSUtils::resolveExpressionType(
                owner, QQmlLSUtils::ResolveActualTypeForFieldMemberExpression);

        // no properties nor signal handlers inside a qualified import
        if (!expressionType || expressionType->type == QQmlLSUtils::QualifiedModuleIdentifier)
            return {};

        return expressionType->semanticScope;
    }();

    if (!scope)
        return;

    propertyCompletion(scope, nullptr, it);
    signalHandlerCompletion(scope, nullptr, it);
}

void QQmlLSCompletion::insideImportCompletionHelper(const DomItem &file,
                                                    const QQmlLSCompletionPosition &positionInfo,
                                                    BackInsertIterator it) const
{
    // returns completions for import statements, ctx is supposed to be in an import statement
    const CompletionContextStrings &ctx = positionInfo.cursorPosition;
    ImportCompletionType importCompletionType = ImportCompletionType::None;
    QRegularExpression spaceRe(uR"(\s+)"_s);
    QList<QStringView> linePieces = ctx.preLine().split(spaceRe, Qt::SkipEmptyParts);
    qsizetype effectiveLength = linePieces.size()
            + ((!ctx.preLine().isEmpty() && ctx.preLine().last().isSpace()) ? 1 : 0);
    if (effectiveLength < 2) {
        CompletionItem comp;
        comp.label = "import";
        comp.kind = int(CompletionItemKind::Keyword);
        it = comp;
    }
    if (linePieces.isEmpty() || linePieces.first() != u"import")
        return;
    if (effectiveLength == 2) {
        // the cursor is after the import, possibly in a partial module name
        importCompletionType = ImportCompletionType::Module;
    } else if (effectiveLength == 3) {
        if (linePieces.last() != u"as") {
            // the cursor is after the module, possibly in a partial version token (or partial as)
            CompletionItem comp;
            comp.label = "as";
            comp.kind = int(CompletionItemKind::Keyword);
            it = comp;
            importCompletionType = ImportCompletionType::Version;
        }
    }
    DomItem env = file.environment();
    if (std::shared_ptr<DomEnvironment> envPtr = env.ownerAs<DomEnvironment>()) {
        switch (importCompletionType) {
        case ImportCompletionType::None:
            break;
        case ImportCompletionType::Module: {
            QDuplicateTracker<QString> modulesSeen;
            for (const QString &uri : envPtr->moduleIndexUris(env)) {
                QStringView base = ctx.base(); // if we allow spaces we should get rid of them
                if (uri.startsWith(base)) {
                    QStringList rest = uri.mid(base.size()).split(u'.');
                    if (rest.isEmpty())
                        continue;

                    const QString label = rest.first();
                    if (!modulesSeen.hasSeen(label)) {
                        CompletionItem comp;
                        comp.label = label.toUtf8();
                        comp.kind = int(CompletionItemKind::Module);
                        it = comp;
                    }
                }
            }
            break;
        }
        case ImportCompletionType::Version:
            if (ctx.base().isEmpty()) {
                for (int majorV :
                     envPtr->moduleIndexMajorVersions(env, linePieces.at(1).toString())) {
                    CompletionItem comp;
                    comp.label = QString::number(majorV).toUtf8();
                    comp.kind = int(CompletionItemKind::Constant);
                    it = comp;
                }
            } else {
                bool hasMajorVersion = ctx.base().endsWith(u'.');
                int majorV = -1;
                if (hasMajorVersion)
                    majorV = ctx.base().mid(0, ctx.base().size() - 1).toInt(&hasMajorVersion);
                if (!hasMajorVersion)
                    break;
                if (std::shared_ptr<ModuleIndex> mIndex =
                            envPtr->moduleIndexWithUri(env, linePieces.at(1).toString(), majorV)) {
                    for (int minorV : mIndex->minorVersions()) {
                        CompletionItem comp;
                        comp.label = QString::number(minorV).toUtf8();
                        comp.kind = int(CompletionItemKind::Constant);
                        it = comp;
                    }
                }
            }
            break;
        }
    }
}

void QQmlLSCompletion::idsCompletions(const DomItem &component, BackInsertIterator it) const
{
    qCDebug(QQmlLSCompletionLog) << "adding ids completions";
    for (const QString &k : component.field(Fields::ids).keys()) {
        CompletionItem comp;
        comp.label = k.toUtf8();
        comp.kind = int(CompletionItemKind::Value);
        it = comp;
    }
}

static bool testScopeSymbol(const QQmlJSScope::ConstPtr &scope, LocalSymbolsTypes options,
                            CompletionItemKind kind)
{
    const bool currentIsSingleton = scope->isSingleton();
    const bool currentIsAttached = !scope->attachedType().isNull();
    if ((options & LocalSymbolsType::Singleton) && currentIsSingleton) {
        return true;
    }
    if ((options & LocalSymbolsType::AttachedType) && currentIsAttached) {
        return true;
    }
    const bool isObjectType = scope->isReferenceType();
    if (options & LocalSymbolsType::ObjectType && !currentIsSingleton && isObjectType) {
        return kind != CompletionItemKind::Constructor || scope->isCreatable();
    }
    if (options & LocalSymbolsType::ValueType && !currentIsSingleton && !isObjectType) {
        return true;
    }
    return false;
}

/*!
\internal
Obtain the types reachable from \c{el} as a CompletionItems.
*/
void QQmlLSCompletion::suggestReachableTypes(const DomItem &el, LocalSymbolsTypes options,
                                             CompletionItemKind kind, BackInsertIterator it) const
{
    auto file = el.containingFile().as<QmlFile>();
    if (!file)
        return;
    auto resolver = file->typeResolver();
    if (!resolver)
        return;

    const QString requiredQualifiers = QQmlLSUtils::qualifiersFrom(el);
    const auto keyValueRange = resolver->importedTypes().asKeyValueRange();
    for (const auto &type : keyValueRange) {
        // ignore special QQmlJSImporterMarkers
        const bool isMarkerType = type.first.contains(u"$internal$.")
                || type.first.contains(u"$anonymous$.") || type.first.contains(u"$module$.");
        if (isMarkerType || !type.first.startsWith(requiredQualifiers))
            continue;

        auto &scope = type.second.scope;
        if (!scope)
            continue;

        if (!testScopeSymbol(scope, options, kind))
            continue;

        CompletionItem completion;
        completion.label = QStringView(type.first).sliced(requiredQualifiers.size()).toUtf8();
        completion.kind = int(kind);
        it = completion;
    }
}

void QQmlLSCompletion::jsIdentifierCompletion(const QQmlJSScope::ConstPtr &scope,
                                              QDuplicateTracker<QString> *usedNames,
                                              BackInsertIterator it) const
{
    for (const auto &[name, jsIdentifier] : scope->ownJSIdentifiers().asKeyValueRange()) {
        CompletionItem completion;
        if (usedNames && usedNames->hasSeen(name)) {
            continue;
        }
        completion.label = name.toUtf8();
        completion.kind = int(CompletionItemKind::Variable);
        QString detail = u"has type "_s;
        if (jsIdentifier.typeName) {
            if (jsIdentifier.isConst) {
                detail.append(u"const ");
            }
            detail.append(*jsIdentifier.typeName);
        } else {
            detail.append(jsIdentifier.isConst ? u"const"_s : u"var"_s);
        }
        completion.detail = detail.toUtf8();
        it = completion;
    }
}

void QQmlLSCompletion::methodCompletion(const QQmlJSScope::ConstPtr &scope,
                                        QDuplicateTracker<QString> *usedNames,
                                        BackInsertIterator it) const
{
    // JS functions in current and base scopes
    for (const auto &[name, method] : scope->methods().asKeyValueRange()) {
        if (method.access() != QQmlJSMetaMethod::Public)
            continue;
        if (usedNames && usedNames->hasSeen(name)) {
            continue;
        }
        CompletionItem completion;
        completion.label = name.toUtf8();
        completion.kind = int(CompletionItemKind::Method);
        it = completion;
        // TODO: QQmlLSUtils::reachableSymbols seems to be able to do documentation and detail
        // and co, it should also be done here if possible.
    }
}

void QQmlLSCompletion::propertyCompletion(const QQmlJSScope::ConstPtr &scope,
                                          QDuplicateTracker<QString> *usedNames,
                                          BackInsertIterator it) const
{
    for (const auto &[name, property] : scope->properties().asKeyValueRange()) {
        if (usedNames && usedNames->hasSeen(name)) {
            continue;
        }
        CompletionItem completion;
        completion.label = name.toUtf8();
        completion.kind = int(CompletionItemKind::Property);
        QString detail{ u"has type "_s };
        if (!property.isWritable())
            detail.append(u"readonly "_s);
        detail.append(property.typeName().isEmpty() ? u"var"_s : property.typeName());
        completion.detail = detail.toUtf8();
        it = completion;
    }
}

void QQmlLSCompletion::enumerationCompletion(const QQmlJSScope::ConstPtr &scope,
                                             QDuplicateTracker<QString> *usedNames,
                                             BackInsertIterator it) const
{
    for (const QQmlJSMetaEnum &enumerator : scope->enumerations()) {
        if (usedNames && usedNames->hasSeen(enumerator.name())) {
            continue;
        }
        CompletionItem completion;
        completion.label = enumerator.name().toUtf8();
        completion.kind = static_cast<int>(CompletionItemKind::Enum);
        it = completion;
    }
}

void QQmlLSCompletion::enumerationValueCompletionHelper(const QStringList &enumeratorKeys,
                                                        BackInsertIterator it) const
{
    for (const QString &enumeratorKey : enumeratorKeys) {
        CompletionItem completion;
        completion.label = enumeratorKey.toUtf8();
        completion.kind = static_cast<int>(CompletionItemKind::EnumMember);
        it = completion;
    }
}

/*!
\internal
Creates completion items for enumerationvalues.
If enumeratorName is a valid enumerator then only do completion for the requested enumerator, and
otherwise do completion for \b{all other possible} enumerators.

For example:
```
id: someItem
enum Hello { World }
enum MyEnum { ValueOne, ValueTwo }

// Hello does refer to a enumerator:
property var a: Hello.<complete only World here>

// someItem does not refer to a enumerator:
property var b: someItem.<complete World, ValueOne and ValueTwo here>
```
*/

void QQmlLSCompletion::enumerationValueCompletion(const QQmlJSScope::ConstPtr &scope,
                                                  const QString &enumeratorName,
                                                  BackInsertIterator result) const
{
    auto enumerator = scope->enumeration(enumeratorName);
    if (enumerator.isValid()) {
        enumerationValueCompletionHelper(enumerator.keys(), result);
        return;
    }

    for (const QQmlJSMetaEnum &enumerator : scope->enumerations()) {
        enumerationValueCompletionHelper(enumerator.keys(), result);
    }
}

/*!
\internal
Calls F on all JavaScript-parents of scope. For example, you can use this method to
collect all the JavaScript Identifiers from following code:
```
{ // this block statement contains only 'x'
    let x = 3;
    { // this block statement contains only 'y', and 'x' has to be retrieved via its parent.
        let y = 4;
    }
}
```
*/
template<typename F>
void collectFromAllJavaScriptParents(const F &&f, const QQmlJSScope::ConstPtr &scope)
{
    for (QQmlJSScope::ConstPtr current = scope; current; current = current->parentScope()) {
        f(current);
        if (current->scopeType() == QQmlSA::ScopeType::QMLScope)
            return;
    }
}

/*!
\internal
Suggest enumerations (if applicable) and enumeration values from \c scope, for example \c
Asynchronous from the \c CompilationMode enum:

\qml
property var xxx: Component.Asynchronous // Component contains the \c CompilationMode enum
property var xxx2: CompilationMode.Asynchronous
\endqml
*/
void QQmlLSCompletion::suggestEnumerationsAndEnumerationValues(
        const QQmlJSScope::ConstPtr &scope, const QString &enumName,
        QDuplicateTracker<QString> &usedNames, BackInsertIterator result) const
{
    enumerationValueCompletion(scope, enumName, result);

    // skip enumeration types if already inside an enumeration type
    if (auto enumerator = scope->enumeration(enumName); !enumerator.isValid()) {
        enumerationCompletion(scope, &usedNames, result);
    }
}

/*!
\internal

Returns the owner of a qualified expression for further resolving, for example:
1. \c owner from the \c member ScriptExpression in \c {owner.member}. This happens when completion
is requested on \c member.
2. \c owner from the ScriptBinaryExpression \c {owner.member}. This happens when completion is
requested on the dot between \c owner and \c member.
3. An empty DomItem otherwise.
*/
DomItem QQmlLSCompletion::ownerOfQualifiedExpression(const DomItem &qualifiedExpression) const
{
    // note: there is an edge case, where the user asks for completion right after the dot
    // of some qualified expression like `root.hello`. In this case, scriptIdentifier is actually
    // the BinaryExpression instead of the left-hand-side that has not be written down yet.
    const bool askForCompletionOnDot = QQmlLSUtils::isFieldMemberExpression(qualifiedExpression);
    const bool hasQualifier =
            QQmlLSUtils::isFieldMemberAccess(qualifiedExpression) || askForCompletionOnDot;

    if (!hasQualifier)
        return {};

    const DomItem owner =
            (askForCompletionOnDot ? qualifiedExpression : qualifiedExpression.directParent())
                    .field(Fields::left);
    return owner;
}

/*!
\internal
Generate autocompletions for JS expressions, suggest possible properties, methods, etc.

If scriptIdentifier is inside a Field Member Expression, like \c{onCompleted} in
\c{Component.onCompleted} for example, then this method will only suggest properties, methods, etc
from the correct type. For the previous example that would be properties, methods, etc. from the
Component attached type.
*/
void QQmlLSCompletion::suggestJSExpressionCompletion(const DomItem &scriptIdentifier,
                                                     BackInsertIterator result) const
{
    QDuplicateTracker<QString> usedNames;
    QQmlJSScope::ConstPtr nearestScope;

    const DomItem owner = ownerOfQualifiedExpression(scriptIdentifier);

    if (!owner) {
        for (QUtf8StringView view : std::array<QUtf8StringView, 3>{ "null", "false", "true" }) {
            CompletionItem completion;
            completion.label = view.data();
            completion.kind = int(CompletionItemKind::Value);
            result = completion;
        }
        idsCompletions(scriptIdentifier.component(), result);
        suggestReachableTypes(scriptIdentifier,
                              LocalSymbolsType::Singleton | LocalSymbolsType::AttachedType,
                              CompletionItemKind::Class, result);

        auto scope = scriptIdentifier.nearestSemanticScope();
        if (!scope)
            return;
        nearestScope = scope;

        enumerationCompletion(nearestScope, &usedNames, result);
    } else {
        auto ownerExpressionType = QQmlLSUtils::resolveExpressionType(
                owner, QQmlLSUtils::ResolveActualTypeForFieldMemberExpression);
        if (!ownerExpressionType || !ownerExpressionType->semanticScope)
            return;
        nearestScope = ownerExpressionType->semanticScope;

        switch (ownerExpressionType->type) {
        case QQmlLSUtils::EnumeratorValueIdentifier:
            return;
        case QQmlLSUtils::EnumeratorIdentifier:
            suggestEnumerationsAndEnumerationValues(nearestScope, *ownerExpressionType->name,
                                                    usedNames, result);
            return;
        case QQmlLSUtils::QmlComponentIdentifier:
            // Suggest members of the attached type, for example suggest `progress` in
            // `property real p: Component.progress`.
            if (QQmlJSScope::ConstPtr attachedType =
                ownerExpressionType->semanticScope->attachedType()) {
                methodCompletion(attachedType, &usedNames, result);
                propertyCompletion(attachedType, &usedNames, result);
                suggestEnumerationsAndEnumerationValues(
                        attachedType, *ownerExpressionType->name, usedNames, result);
            }
            Q_FALLTHROUGH();
        case QQmlLSUtils::SingletonIdentifier:
            if (ownerExpressionType->name)
                suggestEnumerationsAndEnumerationValues(nearestScope, *ownerExpressionType->name,
                                                        usedNames, result);
            break;
        default:
            break;
        }
    }

    Q_ASSERT(nearestScope);

    methodCompletion(nearestScope, &usedNames, result);
    propertyCompletion(nearestScope, &usedNames, result);

    if (!owner) {
        // collect all of the stuff from parents
        collectFromAllJavaScriptParents(
                [this, &usedNames, result](const QQmlJSScope::ConstPtr &scope) {
                    jsIdentifierCompletion(scope, &usedNames, result);
                },
                nearestScope);
        collectFromAllJavaScriptParents(
                [this, &usedNames, result](const QQmlJSScope::ConstPtr &scope) {
                    methodCompletion(scope, &usedNames, result);
                },
                nearestScope);
        collectFromAllJavaScriptParents(
                [this, &usedNames, result](const QQmlJSScope::ConstPtr &scope) {
                    propertyCompletion(scope, &usedNames, result);
                },
                nearestScope);

        auto file = scriptIdentifier.containingFile().as<QmlFile>();
        if (!file)
            return;
        auto resolver = file->typeResolver();
        if (!resolver)
            return;

        const auto globals = resolver->jsGlobalObject();
        methodCompletion(globals, &usedNames, result);
        propertyCompletion(globals, &usedNames, result);
    }
}

static const QQmlJSScope *resolve(const QQmlJSScope *current, const QStringList &names)
{
    for (const QString &name : names) {
        if (auto property = current->property(name); property.isValid()) {
            if (auto propertyType = property.type().get()) {
                current = propertyType;
                continue;
            }
        }
        return {};
    }
    return current;
}

bool QQmlLSCompletion::cursorInFrontOfItem(const DomItem &parentForContext,
                                           const QQmlLSCompletionPosition &positionInfo)
{
    auto fileLocations = FileLocations::treeOf(parentForContext)->info().fullRegion;
    return positionInfo.offset() <= fileLocations.offset;
}

bool QQmlLSCompletion::cursorAfterColon(const DomItem &currentItem,
                                        const QQmlLSCompletionPosition &positionInfo)
{
    auto location = FileLocations::treeOf(currentItem)->info();
    auto region = location.regions.constFind(ColonTokenRegion);

    if (region == location.regions.constEnd())
        return false;

    if (region.value().isValid() && region.value().offset < positionInfo.offset()) {
        return true;
    }
    return false;
}

/*!
\internal
\brief Mapping from pragma names to allowed pragma values.

This mapping of pragma names to pragma values is not complete. In fact, it only contains the
pragma names and values that one should see autocompletion for.
Some pragmas like FunctionSignatureBehavior or Strict or the Reference/Value of ValueTypeBehavior,
for example, should currently not be proposed as completion items by qmlls.

An empty QList-value in the QMap means that the pragma does not accept pragma values.
*/
static const QMap<QString, QList<QString>> valuesForPragmas{
    { u"ComponentBehavior"_s, { u"Unbound"_s, u"Bound"_s } },
    { u"NativeMethodBehavior"_s, { u"AcceptThisObject"_s, u"RejectThisObject"_s } },
    { u"ListPropertyAssignBehavior"_s, { u"Append"_s, u"Replace"_s, u"ReplaceIfNotDefault"_s } },
    { u"Singleton"_s, {} },
    { u"ValueTypeBehavior"_s, { u"Addressable"_s, u"Inaddressable"_s } },
};

void QQmlLSCompletion::insidePragmaCompletion(QQmlJS::Dom::DomItem currentItem,
                                              const QQmlLSCompletionPosition &positionInfo,
                                              BackInsertIterator result) const
{
    if (cursorAfterColon(currentItem, positionInfo)) {
        const QString name = currentItem.field(Fields::name).value().toString();
        auto values = valuesForPragmas.constFind(name);
        if (values == valuesForPragmas.constEnd())
            return;

        for (const auto &value : *values) {
            CompletionItem comp;
            comp.label = value.toUtf8();
            comp.kind = static_cast<int>(CompletionItemKind::Value);
            result = comp;
        }
        return;
    }

    for (const auto &pragma : valuesForPragmas.asKeyValueRange()) {
        CompletionItem comp;
        comp.label = pragma.first.toUtf8();
        if (!pragma.second.isEmpty()) {
            comp.insertText = QString(pragma.first).append(u": ").toUtf8();
        }
        comp.kind = static_cast<int>(CompletionItemKind::Value);
        result = comp;
    }
}

void QQmlLSCompletion::insideQmlObjectCompletion(const DomItem &parentForContext,
                                                 const QQmlLSCompletionPosition &positionInfo,
                                                 BackInsertIterator result) const
{

    const auto regions = FileLocations::treeOf(parentForContext)->info().regions;

    const QQmlJS::SourceLocation leftBrace = regions[LeftBraceRegion];
    const QQmlJS::SourceLocation rightBrace = regions[RightBraceRegion];

    if (beforeLocation(positionInfo, leftBrace)) {
        LocalSymbolsTypes options;
        options.setFlag(LocalSymbolsType::ObjectType);
        suggestReachableTypes(positionInfo.itemAtPosition, options, CompletionItemKind::Constructor,
                              result);
        if (parentForContext.directParent().internalKind() == DomType::Binding)
            suggestSnippetsForRightHandSideOfBinding(positionInfo.itemAtPosition, result);
        else
            suggestSnippetsForLeftHandSideOfBinding(positionInfo.itemAtPosition, result);

        if (QQmlLSUtils::isFieldMemberExpression(positionInfo.itemAtPosition)) {
            /*!
                \internal
                In the case that a missing identifier is followed by an assignment to the default
                property, the parser will create a QmlObject out of both binding and default
               binding. For example, in \code property int x: root. Item {} \endcode the parser will
               create one binding containing one QmlObject of type `root.Item`, instead of two
               bindings (one for `x` and one for the default property). For this special case, if
               completion is requested inside `root.Item`, then try to also suggest JS expressions.

                Note: suggestJSExpressionCompletion() will suggest nothing if the
               fieldMemberExpression starts with the name of a qualified module or a filename, so
               this only adds invalid suggestions in the case that there is something shadowing the
               qualified module name or filename, like a property name for example.

                Note 2: This does not happen for field member accesses. For example, in
                \code
                property int x: root.x
                Item {}
                \endcode
                The parser will create both bindings correctly.
            */
            suggestJSExpressionCompletion(positionInfo.itemAtPosition, result);
        }
        return;
    }

    if (betweenLocations(leftBrace, positionInfo, rightBrace)) {
        // default/required property completion
        for (QUtf8StringView view :
             std::array<QUtf8StringView, 6>{ "", "readonly ", "default ", "default required ",
                                             "required default ", "required " }) {
            // readonly properties require an initializer
            if (view != QUtf8StringView("readonly ")) {
                result = makeSnippet(
                        QByteArray(view.data()).append("property type name;"),
                        QByteArray(view.data()).append("property ${1:type} ${0:name};"));
            }

            result = makeSnippet(
                    QByteArray(view.data()).append("property type name: value;"),
                    QByteArray(view.data()).append("property ${1:type} ${2:name}: ${0:value};"));
        }

        // signal
        result = makeSnippet("signal name(arg1:type1, ...)", "signal ${1:name}($0)");

        // signal without parameters
        result = makeSnippet("signal name;", "signal ${0:name};");

        // make already existing property required
        result = makeSnippet("required name;", "required ${0:name};");

        // function
        result = makeSnippet("function name(args...): returnType { statements...}",
                             "function ${1:name}($2): ${3:returnType} {\n\t$0\n}");

        // enum
        result = makeSnippet("enum name { Values...}", "enum ${1:name} {\n\t${0:values}\n}");

        // inline component
        result = makeSnippet("component Name: BaseType { ... }",
                             "component ${1:name}: ${2:baseType} {\n\t$0\n}");

        suggestBindingCompletion(positionInfo.itemAtPosition, result);

        // add Qml Types for default binding
        const DomItem containingFile = parentForContext.containingFile();
        suggestReachableTypes(containingFile, LocalSymbolsType::ObjectType,
                              CompletionItemKind::Constructor, result);
        suggestSnippetsForLeftHandSideOfBinding(positionInfo.itemAtPosition, result);
        return;
    }
}

void QQmlLSCompletion::insidePropertyDefinitionCompletion(
        const DomItem &currentItem, const QQmlLSCompletionPosition &positionInfo,
        BackInsertIterator result) const
{
    auto info = FileLocations::treeOf(currentItem)->info();
    const QQmlJS::SourceLocation propertyKeyword = info.regions[PropertyKeywordRegion];

    // do completions for the keywords
    if (positionInfo.offset() < propertyKeyword.offset + propertyKeyword.length) {
        const QQmlJS::SourceLocation readonlyKeyword = info.regions[ReadonlyKeywordRegion];
        const QQmlJS::SourceLocation defaultKeyword = info.regions[DefaultKeywordRegion];
        const QQmlJS::SourceLocation requiredKeyword = info.regions[RequiredKeywordRegion];

        bool completeReadonly = true;
        bool completeRequired = true;
        bool completeDefault = true;

        // if there is already a readonly keyword before the cursor: do not auto complete it again
        if (readonlyKeyword.isValid() && readonlyKeyword.offset < positionInfo.offset()) {
            completeReadonly = false;
            // also, required keywords do not like readonly keywords
            completeRequired = false;
        }

        // same for required
        if (requiredKeyword.isValid() && requiredKeyword.offset < positionInfo.offset()) {
            completeRequired = false;
            // also, required keywords do not like readonly keywords
            completeReadonly = false;
        }

        // same for default
        if (defaultKeyword.isValid() && defaultKeyword.offset < positionInfo.offset()) {
            completeDefault = false;
        }
        auto addCompletionKeyword = [&result](QUtf8StringView view, bool complete) {
            if (!complete)
                return;
            CompletionItem item;
            item.label = view.data();
            item.kind = int(CompletionItemKind::Keyword);
            result = item;
        };
        addCompletionKeyword(u8"readonly", completeReadonly);
        addCompletionKeyword(u8"required", completeRequired);
        addCompletionKeyword(u8"default", completeDefault);
        addCompletionKeyword(u8"property", true);

        return;
    }

    const QQmlJS::SourceLocation propertyIdentifier = info.regions[IdentifierRegion];
    if (propertyKeyword.end() <= positionInfo.offset()
        && positionInfo.offset() < propertyIdentifier.offset) {
        suggestReachableTypes(currentItem,
                              LocalSymbolsType::ObjectType | LocalSymbolsType::ValueType,
                              CompletionItemKind::Class, result);
    }
    // do not autocomplete the rest
    return;
}

void QQmlLSCompletion::insideBindingCompletion(const DomItem &currentItem,
                                               const QQmlLSCompletionPosition &positionInfo,
                                               BackInsertIterator result) const
{
    const DomItem containingBinding = currentItem.filterUp(
            [](DomType type, const QQmlJS::Dom::DomItem &) { return type == DomType::Binding; },
            FilterUpOptions::ReturnOuter);

    // do scriptidentifiercompletion after the ':' of a binding
    if (cursorAfterColon(containingBinding, positionInfo)) {
        suggestJSExpressionCompletion(positionInfo.itemAtPosition, result);

        if (auto type = QQmlLSUtils::resolveExpressionType(currentItem,
                                                           QQmlLSUtils::ResolveOwnerType)) {
            const QStringList names = currentItem.field(Fields::name).toString().split(u'.');
            const QQmlJSScope *current = resolve(type->semanticScope.get(), names);
            // add type names when binding to an object type or a property with var type
            if (!current || current->accessSemantics() == QQmlSA::AccessSemantics::Reference) {
                LocalSymbolsTypes options;
                options.setFlag(LocalSymbolsType::ObjectType);
                suggestReachableTypes(positionInfo.itemAtPosition, options,
                                      CompletionItemKind::Constructor, result);
                suggestSnippetsForRightHandSideOfBinding(positionInfo.itemAtPosition, result);
            }
        }
        return;
    }

    // ignore the binding if asking for completion in front of the binding
    if (cursorInFrontOfItem(containingBinding, positionInfo)) {
        insideQmlObjectCompletion(currentItem.containingObject(), positionInfo, result);
        return;
    }

    const DomItem containingObject = currentItem.qmlObject();

    suggestBindingCompletion(positionInfo.itemAtPosition, result);

    // add Qml Types for default binding
    suggestReachableTypes(positionInfo.itemAtPosition, LocalSymbolsType::ObjectType,
                          CompletionItemKind::Constructor, result);
    suggestSnippetsForLeftHandSideOfBinding(positionInfo.itemAtPosition, result);
}

void QQmlLSCompletion::insideImportCompletion(const DomItem &currentItem,
                                              const QQmlLSCompletionPosition &positionInfo,
                                              BackInsertIterator result) const
{
    const DomItem containingFile = currentItem.containingFile();
    insideImportCompletionHelper(containingFile, positionInfo, result);

    // when in front of the import statement: propose types for root Qml Object completion
    if (cursorInFrontOfItem(currentItem, positionInfo)) {
        suggestReachableTypes(containingFile, LocalSymbolsType::ObjectType,
                              CompletionItemKind::Constructor, result);
    }
}

void QQmlLSCompletion::insideQmlFileCompletion(const DomItem &currentItem,
                                               const QQmlLSCompletionPosition &positionInfo,
                                               BackInsertIterator result) const
{
    const DomItem containingFile = currentItem.containingFile();
    // completions for code outside the root Qml Object
    // global completions
    if (positionInfo.cursorPosition.atLineStart()) {
        if (positionInfo.cursorPosition.base().isEmpty()) {
            for (const QStringView &s : std::array<QStringView, 2>({ u"pragma", u"import" })) {
                CompletionItem comp;
                comp.label = s.toUtf8();
                comp.kind = int(CompletionItemKind::Keyword);
                result = comp;
            }
        }
    }
    // Types for root Qml Object completion
    suggestReachableTypes(containingFile, LocalSymbolsType::ObjectType,
                          CompletionItemKind::Constructor, result);
}

/*!
\internal
Generate the snippets for let, var and const variable declarations.
*/
void QQmlLSCompletion::suggestVariableDeclarationStatementCompletion(
        BackInsertIterator result, AppendOption option) const
{
    // let/var/const statement
    for (auto view : std::array<QUtf8StringView, 3>{ "let", "var", "const" }) {
        auto snippet = makeSnippet(QByteArray(view.data()).append(" variable = value"),
                             QByteArray(view.data()).append(" ${1:variable} = $0"));
        if (option == AppendSemicolon) {
            snippet.insertText->append(";");
            snippet.label.append(";");
        }
        result = snippet;
    }
}

/*!
\internal
Generate the snippets for case and default statements.
*/
void QQmlLSCompletion::suggestCaseAndDefaultStatementCompletion(BackInsertIterator result) const
{
    // case snippet
    result = makeSnippet("case value: statements...", "case ${1:value}:\n\t$0");
    // case + brackets snippet
    result = makeSnippet("case value: { statements... }", "case ${1:value}: {\n\t$0\n}");

    // default snippet
    result = makeSnippet("default: statements...", "default:\n\t$0");
    // default + brackets snippet
    result = makeSnippet("default: { statements... }", "default: {\n\t$0\n}");
}

/*!
\internal
Break and continue can be inserted only in following situations:
\list
    \li Break and continue inside a loop.
    \li Break inside a (nested) LabelledStatement
    \li Break inside a (nested) SwitchStatement
\endlist
*/
void QQmlLSCompletion::suggestContinueAndBreakStatementIfNeeded(const DomItem &itemAtPosition,
                                                                BackInsertIterator result) const
{
    bool alreadyInLabel = false;
    bool alreadyInSwitch = false;
    for (DomItem current = itemAtPosition; current; current = current.directParent()) {
        switch (current.internalKind()) {
        case DomType::ScriptExpression:
            // reached end of script expression
            return;

        case DomType::ScriptForStatement:
        case DomType::ScriptForEachStatement:
        case DomType::ScriptWhileStatement:
        case DomType::ScriptDoWhileStatement: {
            CompletionItem continueKeyword;
            continueKeyword.label = "continue";
            continueKeyword.kind = int(CompletionItemKind::Keyword);
            result = continueKeyword;

            // do not add break twice
            if (!alreadyInSwitch && !alreadyInLabel) {
                CompletionItem breakKeyword;
                breakKeyword.label = "break";
                breakKeyword.kind = int(CompletionItemKind::Keyword);
                result = breakKeyword;
            }
            // early exit: cannot suggest more completions
            return;
        }
        case DomType::ScriptSwitchStatement: {
            // check if break was already inserted
            if (alreadyInSwitch || alreadyInLabel)
                break;
            alreadyInSwitch = true;

            CompletionItem breakKeyword;
            breakKeyword.label = "break";
            breakKeyword.kind = int(CompletionItemKind::Keyword);
            result = breakKeyword;
            break;
        }
        case DomType::ScriptLabelledStatement: {
            // check if break was already inserted because of switch or loop
            if (alreadyInSwitch || alreadyInLabel)
                break;
            alreadyInLabel = true;

            CompletionItem breakKeyword;
            breakKeyword.label = "break";
            breakKeyword.kind = int(CompletionItemKind::Keyword);
            result = breakKeyword;
            break;
        }
        default:
            break;
        }
    }
}

/*!
\internal
Generates snippets or keywords for all possible JS statements where it makes sense. To use whenever
any JS statement can be expected, but when no JS statement is there yet.

Only generates JS expression completions when itemAtPosition is a qualified name.

Here is a list of statements that do \e{not} get any snippets:
\list
    \li BlockStatement does not need a code snippet, editors automatically include the closing
bracket anyway. \li EmptyStatement completion would only generate a single \c{;} \li
ExpressionStatement completion cannot generate any snippet, only identifiers \li WithStatement
completion is not recommended: qmllint will warn about usage of with statements \li
LabelledStatement completion might need to propose labels (TODO?) \li DebuggerStatement completion
does not strike as being very useful \endlist
*/
void QQmlLSCompletion::suggestJSStatementCompletion(const DomItem &itemAtPosition,
                                                    BackInsertIterator result) const
{
    suggestJSExpressionCompletion(itemAtPosition, result);

    if (QQmlLSUtils::isFieldMemberAccess(itemAtPosition)
        || QQmlLSUtils::isFieldMemberExpression(itemAtPosition))
        return;

    // expression statements
    suggestVariableDeclarationStatementCompletion(result);
    // block statement
    result = makeSnippet("{ statements... }", "{\n\t$0\n}");

    // if + brackets statement
    result = makeSnippet("if (condition) { statements }", "if ($1) {\n\t$0\n}");

    // do statement
    result = makeSnippet("do { statements } while (condition);", "do {\n\t$1\n} while ($0);");

    // while + brackets statement
    result = makeSnippet("while (condition) { statements...}", "while ($1) {\n\t$0\n}");

    // for + brackets loop statement
    result = makeSnippet("for (initializer; condition; increment) { statements... }",
                         "for ($1;$2;$3) {\n\t$0\n}");

    // for ... in + brackets loop statement
    result = makeSnippet("for (property in object) { statements... }", "for ($1 in $2) {\n\t$0\n}");

    // for ... of + brackets loop statement
    result = makeSnippet("for (element of array) { statements... }", "for ($1 of $2) {\n\t$0\n}");

    // try + catch statement
    result = makeSnippet("try { statements... } catch(error) { statements... }",
                         "try {\n\t$1\n} catch($2) {\n\t$0\n}");

    // try + finally statement
    result = makeSnippet("try { statements... } finally { statements... }",
                         "try {\n\t$1\n} finally {\n\t$0\n}");

    // try + catch + finally statement
    result = makeSnippet(
            "try { statements... } catch(error) { statements... } finally { statements... }",
            "try {\n\t$1\n} catch($2) {\n\t$3\n} finally {\n\t$0\n}");

    // one can always assume that JS code in QML is inside a function, so always propose `return`
    for (auto &&view : { "return"_ba, "throw"_ba }) {
        CompletionItem item;
        item.label = std::move(view);
        item.kind = int(CompletionItemKind::Keyword);
        result = item;
    }

    // rules for case+default statements:
    // 1) when inside a CaseBlock, or
    // 2) inside a CaseClause, as an (non-nested) element of the CaseClause statementlist.
    // 3) inside a DefaultClause, as an (non-nested) element of the DefaultClause statementlist,
    //
    // switch (x) {
    // // (1)
    // case 1:
    //      myProperty = 5;
    //      // (2) -> could be another statement of current case, but also a new case or default!
    // default:
    //      myProperty = 5;
    //      // (3) -> could be another statement of current default, but also a new case or default!
    // }
    const DomType currentKind = itemAtPosition.internalKind();
    const DomType parentKind = itemAtPosition.directParent().internalKind();
    if (currentKind == DomType::ScriptCaseBlock || currentKind == DomType::ScriptCaseClause
        || currentKind == DomType::ScriptDefaultClause
        || (currentKind == DomType::List
            && (parentKind == DomType::ScriptCaseClause
                || parentKind == DomType::ScriptDefaultClause))) {
        suggestCaseAndDefaultStatementCompletion(result);
    }
    suggestContinueAndBreakStatementIfNeeded(itemAtPosition, result);
}

void QQmlLSCompletion::insideForStatementCompletion(const DomItem &parentForContext,
                                                    const QQmlLSCompletionPosition &positionInfo,
                                                    BackInsertIterator result) const
{
    const auto regions = FileLocations::treeOf(parentForContext)->info().regions;

    const QQmlJS::SourceLocation leftParenthesis = regions[LeftParenthesisRegion];
    const QQmlJS::SourceLocation firstSemicolon = regions[FirstSemicolonTokenRegion];
    const QQmlJS::SourceLocation secondSemicolon = regions[SecondSemicolonRegion];
    const QQmlJS::SourceLocation rightParenthesis = regions[RightParenthesisRegion];

    if (betweenLocations(leftParenthesis, positionInfo, firstSemicolon)) {
        suggestJSExpressionCompletion(positionInfo.itemAtPosition, result);
        suggestVariableDeclarationStatementCompletion(result,
                                                      AppendOption::AppendNothing);
        return;
    }
    if (betweenLocations(firstSemicolon, positionInfo, secondSemicolon)
        || betweenLocations(secondSemicolon, positionInfo, rightParenthesis)) {
        suggestJSExpressionCompletion(positionInfo.itemAtPosition, result);
        return;
    }

    if (afterLocation(rightParenthesis, positionInfo)) {
        suggestJSStatementCompletion(positionInfo.itemAtPosition, result);
        return;
    }
}

void QQmlLSCompletion::insideScriptLiteralCompletion(const DomItem &currentItem,
                                                     const QQmlLSCompletionPosition &positionInfo,
                                                     BackInsertIterator result) const
{
    Q_UNUSED(currentItem);
    if (positionInfo.cursorPosition.base().isEmpty()) {
        suggestJSExpressionCompletion(positionInfo.itemAtPosition, result);
        return;
    }
}

void QQmlLSCompletion::insideCallExpression(const DomItem &currentItem,
                                            const QQmlLSCompletionPosition &positionInfo,
                                            BackInsertIterator result) const
{
    const auto regions = FileLocations::treeOf(currentItem)->info().regions;
    const QQmlJS::SourceLocation leftParenthesis = regions[LeftParenthesisRegion];
    const QQmlJS::SourceLocation rightParenthesis = regions[RightParenthesisRegion];
    if (beforeLocation(positionInfo, leftParenthesis)) {
        suggestJSExpressionCompletion(positionInfo.itemAtPosition, result);
        return;
    }
    if (betweenLocations(leftParenthesis, positionInfo, rightParenthesis)) {
        suggestJSExpressionCompletion(positionInfo.itemAtPosition, result);
        return;
    }
}

void QQmlLSCompletion::insideIfStatement(const DomItem &currentItem,
                                         const QQmlLSCompletionPosition &positionInfo,
                                         BackInsertIterator result) const
{
    const auto regions = FileLocations::treeOf(currentItem)->info().regions;
    const QQmlJS::SourceLocation leftParenthesis = regions[LeftParenthesisRegion];
    const QQmlJS::SourceLocation rightParenthesis = regions[RightParenthesisRegion];
    const QQmlJS::SourceLocation elseKeyword = regions[ElseKeywordRegion];

    if (betweenLocations(leftParenthesis, positionInfo, rightParenthesis)) {
        suggestJSExpressionCompletion(positionInfo.itemAtPosition, result);
        return;
    }
    if (betweenLocations(rightParenthesis, positionInfo, elseKeyword)) {
        suggestJSStatementCompletion(positionInfo.itemAtPosition, result);
        return;
    }
    if (afterLocation(elseKeyword, positionInfo)) {
        suggestJSStatementCompletion(positionInfo.itemAtPosition, result);
        return;
    }
}

void QQmlLSCompletion::insideReturnStatement(const DomItem &currentItem,
                                             const QQmlLSCompletionPosition &positionInfo,
                                             BackInsertIterator result) const
{
    const auto regions = FileLocations::treeOf(currentItem)->info().regions;
    const QQmlJS::SourceLocation returnKeyword = regions[ReturnKeywordRegion];

    if (afterLocation(returnKeyword, positionInfo)) {
        suggestJSExpressionCompletion(positionInfo.itemAtPosition, result);
        return;
    }
}

void QQmlLSCompletion::insideWhileStatement(const DomItem &currentItem,
                                            const QQmlLSCompletionPosition &positionInfo,
                                            BackInsertIterator result) const
{
    const auto regions = FileLocations::treeOf(currentItem)->info().regions;
    const QQmlJS::SourceLocation leftParenthesis = regions[LeftParenthesisRegion];
    const QQmlJS::SourceLocation rightParenthesis = regions[RightParenthesisRegion];

    if (betweenLocations(leftParenthesis, positionInfo, rightParenthesis)) {
        suggestJSExpressionCompletion(positionInfo.itemAtPosition, result);
        return;
    }
    if (afterLocation(rightParenthesis, positionInfo)) {
        suggestJSStatementCompletion(positionInfo.itemAtPosition, result);
        return;
    }
}

void QQmlLSCompletion::insideDoWhileStatement(const DomItem &parentForContext,
                                              const QQmlLSCompletionPosition &positionInfo,
                                              BackInsertIterator result) const
{
    const auto regions = FileLocations::treeOf(parentForContext)->info().regions;
    const QQmlJS::SourceLocation doKeyword = regions[DoKeywordRegion];
    const QQmlJS::SourceLocation whileKeyword = regions[WhileKeywordRegion];
    const QQmlJS::SourceLocation leftParenthesis = regions[LeftParenthesisRegion];
    const QQmlJS::SourceLocation rightParenthesis = regions[RightParenthesisRegion];

    if (betweenLocations(doKeyword, positionInfo, whileKeyword)) {
        suggestJSStatementCompletion(positionInfo.itemAtPosition, result);
        return;
    }
    if (betweenLocations(leftParenthesis, positionInfo, rightParenthesis)) {
        suggestJSExpressionCompletion(positionInfo.itemAtPosition, result);
        return;
    }
}

void QQmlLSCompletion::insideForEachStatement(const DomItem &parentForContext,
                                              const QQmlLSCompletionPosition &positionInfo,
                                              BackInsertIterator result) const
{
    const auto regions = FileLocations::treeOf(parentForContext)->info().regions;

    const QQmlJS::SourceLocation inOf = regions[InOfTokenRegion];
    const QQmlJS::SourceLocation leftParenthesis = regions[LeftParenthesisRegion];
    const QQmlJS::SourceLocation rightParenthesis = regions[RightParenthesisRegion];

    if (betweenLocations(leftParenthesis, positionInfo, inOf)) {
        suggestJSExpressionCompletion(positionInfo.itemAtPosition, result);
        suggestVariableDeclarationStatementCompletion(result);
        return;
    }
    if (betweenLocations(inOf, positionInfo, rightParenthesis)) {
        suggestJSExpressionCompletion(positionInfo.itemAtPosition, result);
        return;
    }

    if (afterLocation(rightParenthesis, positionInfo)) {
        suggestJSStatementCompletion(positionInfo.itemAtPosition, result);
        return;
    }
}

void QQmlLSCompletion::insideSwitchStatement(const DomItem &parentForContext,
                                             const QQmlLSCompletionPosition positionInfo,
                                             BackInsertIterator result) const
{
    const auto regions = FileLocations::treeOf(parentForContext)->info().regions;

    const QQmlJS::SourceLocation leftParenthesis = regions[LeftParenthesisRegion];
    const QQmlJS::SourceLocation rightParenthesis = regions[RightParenthesisRegion];

    if (betweenLocations(leftParenthesis, positionInfo, rightParenthesis)) {
        suggestJSExpressionCompletion(positionInfo.itemAtPosition, result);
        return;
    }
}

void QQmlLSCompletion::insideCaseClause(const DomItem &parentForContext,
                                        const QQmlLSCompletionPosition &positionInfo,
                                        BackInsertIterator result) const
{
    const auto regions = FileLocations::treeOf(parentForContext)->info().regions;

    const QQmlJS::SourceLocation caseKeyword = regions[CaseKeywordRegion];
    const QQmlJS::SourceLocation colonToken = regions[ColonTokenRegion];

    if (betweenLocations(caseKeyword, positionInfo, colonToken)) {
        suggestJSExpressionCompletion(positionInfo.itemAtPosition, result);
        return;
    }
    if (afterLocation(colonToken, positionInfo)) {
        suggestJSStatementCompletion(positionInfo.itemAtPosition, result);
        return;
    }

}

/*!
\internal
Checks if a case or default clause does happen before ctx in the code.
*/
bool QQmlLSCompletion::isCaseOrDefaultBeforeCtx(const DomItem &currentClause,
                                                const QQmlLSCompletionPosition &positionInfo,
                                                FileLocationRegion keywordRegion) const
{
    Q_ASSERT(keywordRegion == QQmlJS::Dom::CaseKeywordRegion
             || keywordRegion == QQmlJS::Dom::DefaultKeywordRegion);

    if (!currentClause)
        return false;

    const auto token = FileLocations::treeOf(currentClause)->info().regions[keywordRegion];
    if (afterLocation(token, positionInfo))
        return true;

    return false;
}

/*!
\internal

Search for a `case ...:` or a `default: ` clause happening before ctx, and return the
corresponding DomItem of type DomType::CaseClauses or DomType::DefaultClause.

Return an empty DomItem if neither case nor default was found.
*/
DomItem
QQmlLSCompletion::previousCaseOfCaseBlock(const DomItem &parentForContext,
                                          const QQmlLSCompletionPosition &positionInfo) const
{
    const DomItem caseClauses = parentForContext.field(Fields::caseClauses);
    for (int i = 0; i < caseClauses.indexes(); ++i) {
        const DomItem currentClause = caseClauses.index(i);
        if (isCaseOrDefaultBeforeCtx(currentClause, positionInfo, QQmlJS::Dom::CaseKeywordRegion)) {
            return currentClause;
        }
    }

    const DomItem defaultClause = parentForContext.field(Fields::defaultClause);
    if (isCaseOrDefaultBeforeCtx(defaultClause, positionInfo, QQmlJS::Dom::DefaultKeywordRegion))
        return parentForContext.field(Fields::defaultClause);

    const DomItem moreCaseClauses = parentForContext.field(Fields::moreCaseClauses);
    for (int i = 0; i < moreCaseClauses.indexes(); ++i) {
        const DomItem currentClause = moreCaseClauses.index(i);
        if (isCaseOrDefaultBeforeCtx(currentClause, positionInfo, QQmlJS::Dom::CaseKeywordRegion)) {
            return currentClause;
        }
    }

    return {};
}

void QQmlLSCompletion::insideCaseBlock(const DomItem &parentForContext,
                                       const QQmlLSCompletionPosition &positionInfo,
                                       BackInsertIterator result) const
{
    const auto regions = FileLocations::treeOf(parentForContext)->info().regions;

    const QQmlJS::SourceLocation leftBrace = regions[LeftBraceRegion];
    const QQmlJS::SourceLocation rightBrace = regions[RightBraceRegion];

    if (!betweenLocations(leftBrace, positionInfo, rightBrace))
        return;

    // TODO: looks fishy
    // if there is a previous case or default clause, you can still add statements to it
    if (const auto previousCase = previousCaseOfCaseBlock(parentForContext, positionInfo)) {
        suggestJSStatementCompletion(previousCase, result);
        return;
    }

    // otherwise, only complete case and default
    suggestCaseAndDefaultStatementCompletion(result);
}

void QQmlLSCompletion::insideDefaultClause(const DomItem &parentForContext,
                                           const QQmlLSCompletionPosition &positionInfo,
                                           BackInsertIterator result) const
{
    const auto regions = FileLocations::treeOf(parentForContext)->info().regions;

    const QQmlJS::SourceLocation colonToken = regions[ColonTokenRegion];

    if (afterLocation(colonToken, positionInfo)) {
        suggestJSStatementCompletion(positionInfo.itemAtPosition, result);
        return ;
    }
}

void QQmlLSCompletion::insideBinaryExpressionCompletion(
        const DomItem &parentForContext, const QQmlLSCompletionPosition &positionInfo,
        BackInsertIterator result) const
{
    const auto regions = FileLocations::treeOf(parentForContext)->info().regions;

    const QQmlJS::SourceLocation operatorLocation = regions[OperatorTokenRegion];

    if (beforeLocation(positionInfo, operatorLocation)) {
        suggestJSExpressionCompletion(positionInfo.itemAtPosition, result);
        return;
    }
    if (afterLocation(operatorLocation, positionInfo)) {
        suggestJSExpressionCompletion(positionInfo.itemAtPosition, result);
        return;
    }
}

/*!
\internal
Doing completion in variable declarations requires taking a look at all different cases:

\list
    \li Normal variable names, like \c{let helloWorld = 123;}
        In this case, only autocomplete scriptexpressionidentifiers after the '=' token.
        Do not propose existing names for the variable name, because the variable name needs to be
        an identifier that is not used anywhere (to avoid shadowing and confusing code),

    \li Deconstructed arrays, like \c{let [ helloWorld, ] = [ 123, ];}
        In this case, only autocomplete scriptexpressionidentifiers after the '=' token.
        Do not propose already existing identifiers inside the left hand side array.

    \li Deconstructed arrays with initializers, like \c{let [ helloWorld = someVar, ] = [ 123, ];}
        Note: this assigns the value of someVar to helloWorld if the right hand side's first element
        is undefined or does not exist.

        In this case, only autocomplete scriptexpressionidentifiers after the '=' tokens.
        Only propose already existing identifiers inside the left hand side array when behind a '='
    token.

    \li Deconstructed Objects, like \c{let { helloWorld, } = { helloWorld: 123, };}
        In this case, only autocomplete scriptexpressionidentifiers after the '=' token.
        Do not propose already existing identifiers inside the left hand side object.

    \li Deconstructed Objects with initializers, like \c{let { helloWorld = someVar, } = {};}
        Note: this assigns the value of someVar to helloWorld if the right hand side's object does
        not have a property called 'helloWorld'.

        In this case, only autocomplete scriptexpressionidentifiers after the '=' token.
        Only propose already existing identifiers inside the left hand side object when behind a '='
        token.

    \li Finally, you are allowed to nest and combine all above possibilities together for all your
        deconstruction needs, so the exact same completion needs to be done for
        DomType::ScriptPatternElement too.

\endlist
*/
void QQmlLSCompletion::insideScriptPattern(const DomItem &parentForContext,
                                           const QQmlLSCompletionPosition &positionInfo,
                                           BackInsertIterator result) const
{
    const auto regions = FileLocations::treeOf(parentForContext)->info().regions;

    const QQmlJS::SourceLocation equal = regions[EqualTokenRegion];

    if (!afterLocation(equal, positionInfo))
        return;

    // otherwise, only complete case and default
    suggestJSExpressionCompletion(positionInfo.itemAtPosition, result);
}

/*!
\internal
See comment on insideScriptPattern().
*/
void QQmlLSCompletion::insideVariableDeclarationEntry(const DomItem &parentForContext,
                                                      const QQmlLSCompletionPosition &positionInfo,
                                                      BackInsertIterator result) const
{
    insideScriptPattern(parentForContext, positionInfo, result);
}

void QQmlLSCompletion::insideThrowStatement(const DomItem &parentForContext,
                                            const QQmlLSCompletionPosition &positionInfo,
                                            BackInsertIterator result) const
{
    const auto regions = FileLocations::treeOf(parentForContext)->info().regions;

    const QQmlJS::SourceLocation throwKeyword = regions[ThrowKeywordRegion];

    if (afterLocation(throwKeyword, positionInfo)) {
        suggestJSExpressionCompletion(positionInfo.itemAtPosition, result);
        return;
    }
}

void QQmlLSCompletion::insideLabelledStatement(const DomItem &parentForContext,
                                               const QQmlLSCompletionPosition &positionInfo,
                                               BackInsertIterator result) const
{
    const auto regions = FileLocations::treeOf(parentForContext)->info().regions;

    const QQmlJS::SourceLocation colon = regions[ColonTokenRegion];

    if (afterLocation(colon, positionInfo)) {
        suggestJSStatementCompletion(positionInfo.itemAtPosition, result);
        return;
    }
    // note: the case "beforeLocation(ctx, colon)" probably never happens:
    // this is because without the colon, the parser will probably not parse this as a
    // labelledstatement but as a normal expression statement.
    // So this case only happens when the colon already exists, and the user goes back to the
    // label name and requests completion for that label.
}

/*!
\internal
Collect the current set of labels that some DomItem can jump to.
*/
static void collectLabels(const DomItem &context, QQmlLSCompletion::BackInsertIterator result)
{
    for (DomItem current = context; current; current = current.directParent()) {
        if (current.internalKind() == DomType::ScriptLabelledStatement) {
            const QString label = current.field(Fields::label).value().toString();
            if (label.isEmpty())
                continue;
            CompletionItem item;
            item.label = label.toUtf8();
            item.kind = int(CompletionItemKind::Value); // variable?
            // TODO: more stuff here?
            result = item;
        } else if (current.internalKind() == DomType::ScriptExpression) {
            // quick exit when leaving the JS part
            return;
        }
    }
    return;
}

void QQmlLSCompletion::insideContinueStatement(const DomItem &parentForContext,
                                               const QQmlLSCompletionPosition &positionInfo,
                                               BackInsertIterator result) const
{
    const auto regions = FileLocations::treeOf(parentForContext)->info().regions;

    const QQmlJS::SourceLocation continueKeyword = regions[ContinueKeywordRegion];

    if (afterLocation(continueKeyword, positionInfo)) {
        collectLabels(parentForContext, result);
        return;
    }
}

void QQmlLSCompletion::insideBreakStatement(const DomItem &parentForContext,
                                            const QQmlLSCompletionPosition &positionInfo,
                                            BackInsertIterator result) const
{
    const auto regions = FileLocations::treeOf(parentForContext)->info().regions;

    const QQmlJS::SourceLocation breakKeyword = regions[BreakKeywordRegion];

    if (afterLocation(breakKeyword, positionInfo)) {
        collectLabels(parentForContext, result);
        return;
    }
}

void QQmlLSCompletion::insideConditionalExpression(const DomItem &parentForContext,
                                                   const QQmlLSCompletionPosition &positionInfo,
                                                   BackInsertIterator result) const
{
    const auto regions = FileLocations::treeOf(parentForContext)->info().regions;

    const QQmlJS::SourceLocation questionMark = regions[QuestionMarkTokenRegion];
    const QQmlJS::SourceLocation colon = regions[ColonTokenRegion];

    if (beforeLocation(positionInfo, questionMark)) {
        suggestJSExpressionCompletion(positionInfo.itemAtPosition, result);
        return;
    }
    if (betweenLocations(questionMark, positionInfo, colon)) {
        suggestJSExpressionCompletion(positionInfo.itemAtPosition, result);
        return;
    }
    if (afterLocation(colon, positionInfo)) {
        suggestJSExpressionCompletion(positionInfo.itemAtPosition, result);
        return;
    }
}

void QQmlLSCompletion::insideUnaryExpression(const DomItem &parentForContext,
                                             const QQmlLSCompletionPosition &positionInfo,
                                             BackInsertIterator result) const
{
    const auto regions = FileLocations::treeOf(parentForContext)->info().regions;

    const QQmlJS::SourceLocation operatorToken = regions[OperatorTokenRegion];

    if (afterLocation(operatorToken, positionInfo)) {
        suggestJSExpressionCompletion(positionInfo.itemAtPosition, result);
        return;
    }
}

void QQmlLSCompletion::insidePostExpression(const DomItem &parentForContext,
                                            const QQmlLSCompletionPosition &positionInfo,
                                            BackInsertIterator result) const
{
    const auto regions = FileLocations::treeOf(parentForContext)->info().regions;

    const QQmlJS::SourceLocation operatorToken = regions[OperatorTokenRegion];

    if (beforeLocation(positionInfo, operatorToken)) {
        suggestJSExpressionCompletion(positionInfo.itemAtPosition, result);
        return;
    }
}

void QQmlLSCompletion::insideParenthesizedExpression(const DomItem &parentForContext,
                                                     const QQmlLSCompletionPosition &positionInfo,
                                                     BackInsertIterator result) const
{
    const auto regions = FileLocations::treeOf(parentForContext)->info().regions;

    const QQmlJS::SourceLocation leftParenthesis = regions[LeftParenthesisRegion];
    const QQmlJS::SourceLocation rightParenthesis = regions[RightParenthesisRegion];

    if (betweenLocations(leftParenthesis, positionInfo, rightParenthesis)) {
        suggestJSExpressionCompletion(positionInfo.itemAtPosition, result);
        return;
    }
}

void QQmlLSCompletion::insideTemplateLiteral(const DomItem &parentForContext,
                                             const QQmlLSCompletionPosition &positionInfo,
                                             BackInsertIterator result) const
{
    Q_UNUSED(parentForContext);
    suggestJSExpressionCompletion(positionInfo.itemAtPosition, result);
}

void QQmlLSCompletion::insideNewExpression(const DomItem &parentForContext,
                                           const QQmlLSCompletionPosition &positionInfo,
                                           BackInsertIterator result) const
{
    const auto regions = FileLocations::treeOf(parentForContext)->info().regions;
    const QQmlJS::SourceLocation newKeyword = regions[NewKeywordRegion];

    if (afterLocation(newKeyword, positionInfo)) {
        suggestJSExpressionCompletion(positionInfo.itemAtPosition, result);
        return;
    }
}

void QQmlLSCompletion::insideNewMemberExpression(const DomItem &parentForContext,
                                                 const QQmlLSCompletionPosition &positionInfo,
                                                 BackInsertIterator result) const
{
    const auto regions = FileLocations::treeOf(parentForContext)->info().regions;
    const QQmlJS::SourceLocation newKeyword = regions[NewKeywordRegion];
    const QQmlJS::SourceLocation leftParenthesis = regions[LeftParenthesisRegion];
    const QQmlJS::SourceLocation rightParenthesis = regions[RightParenthesisRegion];

    if (betweenLocations(newKeyword, positionInfo, leftParenthesis)
        || betweenLocations(leftParenthesis, positionInfo, rightParenthesis)) {
        suggestJSExpressionCompletion(positionInfo.itemAtPosition, result);
        return;
    }
}

void QQmlLSCompletion::signalHandlerCompletion(const QQmlJSScope::ConstPtr &scope,
                                               QDuplicateTracker<QString> *usedNames,
                                               BackInsertIterator result) const
{
    const auto keyValues = scope->methods().asKeyValueRange();
    for (const auto &[name, method] : keyValues) {
        if (method.access() != QQmlJSMetaMethod::Public
            || method.methodType() != QQmlJSMetaMethodType::Signal) {
            continue;
        }
        if (usedNames && usedNames->hasSeen(name)) {
            continue;
        }

        CompletionItem completion;
        completion.label = QQmlSignalNames::signalNameToHandlerName(name).toUtf8();
        completion.kind = int(CompletionItemKind::Method);
        result = completion;
    }
}

/*!
\internal
Decide which completions can be used at currentItem and compute them.
*/
QList<CompletionItem>
QQmlLSCompletion::completions(const DomItem &currentItem,
                              const CompletionContextStrings &contextStrings) const
{
    QList<CompletionItem> result;
    collectCompletions(currentItem, contextStrings, std::back_inserter(result));
    return result;
}

void QQmlLSCompletion::collectCompletions(const DomItem &currentItem,
                                          const CompletionContextStrings &contextStrings,
                                          BackInsertIterator result) const
{
    /*!
    Completion is not provided on a script identifier expression because script identifier
    expressions lack context information. Instead, find the first parent that has enough
    context information and provide completion for this one.
    For example, a script identifier expression \c{le} in
    \badcode
        for (;le;) { ... }
    \endcode
    will get completion for a property called \c{leProperty}, while the same script identifier
    expression in
    \badcode
        for (le;;) { ... }
    \endcode
    will, in addition to \c{leProperty}, also get completion for the \c{let} statement snippet.
    In this example, the parent used for the completion is the for-statement, of type
    DomType::ScriptForStatement.

    In addition of the parent for the context, use positionInfo to have exact information on where
    the cursor is (to compare with the SourceLocations of tokens) and which item is at this position
    (required to provide completion at the correct position, for example for attached properties).
    */
    const QQmlLSCompletionPosition positionInfo{ currentItem, contextStrings };
    for (DomItem currentParent = currentItem; currentParent;
         currentParent = currentParent.directParent()) {
        const DomType currentType = currentParent.internalKind();

        switch (currentType) {
        case DomType::Id:
            // suppress completions for ids
            return;
        case DomType::Pragma:
            insidePragmaCompletion(currentParent, positionInfo, result);
            return;
        case DomType::ScriptType: {
            if (currentParent.directParent().internalKind() == DomType::QmlObject) {
                insideQmlObjectCompletion(currentParent.directParent(), positionInfo, result);
                return;
            }

            LocalSymbolsTypes options;
            options.setFlag(LocalSymbolsType::ObjectType);
            options.setFlag(LocalSymbolsType::ValueType);
            suggestReachableTypes(currentItem, options, CompletionItemKind::Class, result);
            return;
        }
        case DomType::ScriptFormalParameter:
            // no autocompletion inside of function parameter definition
            return;
        case DomType::Binding:
            insideBindingCompletion(currentParent, positionInfo, result);
            return;
        case DomType::Import:
            insideImportCompletion(currentParent, positionInfo, result);
            return;
        case DomType::ScriptForStatement:
            insideForStatementCompletion(currentParent, positionInfo, result);
            return;
        case DomType::ScriptBlockStatement:
            suggestJSStatementCompletion(positionInfo.itemAtPosition, result);
            return;
        case DomType::QmlFile:
            insideQmlFileCompletion(currentParent, positionInfo, result);
            return;
        case DomType::QmlObject:
            insideQmlObjectCompletion(currentParent, positionInfo, result);
            return;
        case DomType::MethodInfo:
            // suppress completions
            return;
        case DomType::PropertyDefinition:
            insidePropertyDefinitionCompletion(currentParent, positionInfo, result);
            return;
        case DomType::ScriptBinaryExpression:
            // ignore field member expressions: these need additional context from its parents
            if (QQmlLSUtils::isFieldMemberExpression(currentParent))
                continue;
            insideBinaryExpressionCompletion(currentParent, positionInfo, result);
            return;
        case DomType::ScriptLiteral:
            insideScriptLiteralCompletion(currentParent, positionInfo, result);
            return;
        case DomType::ScriptRegExpLiteral:
            // no completion inside of regexp literals
            return;
        case DomType::ScriptCallExpression:
            insideCallExpression(currentParent, positionInfo, result);
            return;
        case DomType::ScriptIfStatement:
            insideIfStatement(currentParent, positionInfo, result);
            return;
        case DomType::ScriptReturnStatement:
            insideReturnStatement(currentParent, positionInfo, result);
            return;
        case DomType::ScriptWhileStatement:
            insideWhileStatement(currentParent, positionInfo, result);
            return;
        case DomType::ScriptDoWhileStatement:
            insideDoWhileStatement(currentParent, positionInfo, result);
            return;
        case DomType::ScriptForEachStatement:
            insideForEachStatement(currentParent, positionInfo, result);
            return;
        case DomType::ScriptTryCatchStatement:
            /*!
            \internal
            The Ecmascript standard specifies that there can only be a block statement between \c
            try and \c catch(...), \c try and \c finally and \c catch(...) and \c finally, so all of
            these completions are already handled by the DomType::ScriptBlockStatement completion.
            The only place in the try statement where there is no BlockStatement and therefore needs
            its own completion is inside the catch parameter, but that is
            \quotation
            An optional identifier or pattern to hold the caught exception for the associated catch
            block.
            \endquotation
            citing
            https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Statements/try...catch?retiredLocale=de#exceptionvar.
            This means that no completion is needed inside a catch-expression, as it should contain
            an identifier that is not yet used anywhere.
            Therefore, no completion is required at all when inside a try-statement but outside a
            block-statement.
            */
            return;
        case DomType::ScriptSwitchStatement:
            insideSwitchStatement(currentParent, positionInfo, result);
            return;
        case DomType::ScriptCaseClause:
            insideCaseClause(currentParent, positionInfo, result);
            return;
        case DomType::ScriptDefaultClause:
            if (ctxBeforeStatement(positionInfo, currentParent, QQmlJS::Dom::DefaultKeywordRegion))
                continue;
            insideDefaultClause(currentParent, positionInfo, result);
            return;
        case DomType::ScriptCaseBlock:
            insideCaseBlock(currentParent, positionInfo, result);
            return;
        case DomType::ScriptVariableDeclaration:
            // not needed: thats a list of ScriptVariableDeclarationEntry, and those entries cannot
            // be suggested because they all start with `{`, `[` or an identifier that should not be
            // in use yet.
            return;
        case DomType::ScriptVariableDeclarationEntry:
            insideVariableDeclarationEntry(currentParent, positionInfo, result);
            return;
        case DomType::ScriptProperty:
            // fallthrough: a ScriptProperty is a ScriptPattern but inside a JS Object. It gets the
            // same completions as a ScriptPattern.
        case DomType::ScriptPattern:
            insideScriptPattern(currentParent, positionInfo, result);
            return;
        case DomType::ScriptThrowStatement:
            insideThrowStatement(currentParent, positionInfo, result);
            return;
        case DomType::ScriptLabelledStatement:
            insideLabelledStatement(currentParent, positionInfo, result);
            return;
        case DomType::ScriptContinueStatement:
            insideContinueStatement(currentParent, positionInfo, result);
            return;
        case DomType::ScriptBreakStatement:
            insideBreakStatement(currentParent, positionInfo, result);
            return;
        case DomType::ScriptConditionalExpression:
            insideConditionalExpression(currentParent, positionInfo, result);
            return;
        case DomType::ScriptUnaryExpression:
            insideUnaryExpression(currentParent, positionInfo, result);
            return;
        case DomType::ScriptPostExpression:
            insidePostExpression(currentParent, positionInfo, result);
            return;
        case DomType::ScriptParenthesizedExpression:
            insideParenthesizedExpression(currentParent, positionInfo, result);
            return;
        case DomType::ScriptTemplateLiteral:
            insideTemplateLiteral(currentParent, positionInfo, result);
            return;
        case DomType::ScriptTemplateStringPart:
            // no completion inside of the non-expression parts of template strings
            return;
        case DomType::ScriptNewExpression:
            insideNewExpression(currentParent, positionInfo, result);
            return;
        case DomType::ScriptNewMemberExpression:
            insideNewMemberExpression(currentParent, positionInfo, result);
            return;
        case DomType::ScriptThisExpression:
            // suppress completions on `this`
            return;
        case DomType::ScriptSuperLiteral:
            // suppress completions on `super`
            return;
        case DomType::Comment:
            // no completion inside of comments
            return;

        // TODO: Implement those statements.
        // In the meanwhile, suppress completions to avoid weird behaviors.
        case DomType::ScriptArray:
        case DomType::ScriptObject:
        case DomType::ScriptElision:
        case DomType::ScriptArrayEntry:
            return;

        default:
            continue;
        }
        Q_UNREACHABLE();
    }

    // no completion could be found
    qCDebug(QQmlLSUtilsLog) << "No completion was found for current request.";
    return;
}

QQmlLSCompletion::QQmlLSCompletion(const QFactoryLoader &pluginLoader)
{
    const auto keys = pluginLoader.metaDataKeys();
    for (qsizetype i = 0; i < keys.size(); ++i) {
        auto instance = std::unique_ptr<QQmlLSPlugin>(
                qobject_cast<QQmlLSPlugin *>(pluginLoader.instance(i)));
        if (!instance)
            continue;
        if (auto completionInstance = instance->createCompletionPlugin())
            m_plugins.push_back(std::move(completionInstance));
    }
}

/*!
\internal
Helper method to call a method on all loaded plugins.
*/
void QQmlLSCompletion::collectFromPlugins(qxp::function_ref<CompletionFromPluginFunction> f,
                                          BackInsertIterator result) const
{
    for (const auto &plugin : m_plugins) {
        Q_ASSERT(plugin);
        f(plugin.get(), result);
    }
}

void QQmlLSCompletion::suggestSnippetsForLeftHandSideOfBinding(const DomItem &itemAtPosition,
                                                               BackInsertIterator result) const
{
    collectFromPlugins(
            [&itemAtPosition](QQmlLSCompletionPlugin *p, BackInsertIterator result) {
                p->suggestSnippetsForLeftHandSideOfBinding(itemAtPosition, result);
            },
            result);
}

void QQmlLSCompletion::suggestSnippetsForRightHandSideOfBinding(const DomItem &itemAtPosition,
                                                                BackInsertIterator result) const
{
    collectFromPlugins(
            [&itemAtPosition](QQmlLSCompletionPlugin *p, BackInsertIterator result) {
                p->suggestSnippetsForRightHandSideOfBinding(itemAtPosition, result);
            },
            result);
}

QT_END_NAMESPACE
