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
duplication. Most of the helper methods return lists of CompletionItems to simplify the collection
of suggestions from different helper methods into one list.

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

QList<CompletionItem>
QQmlLSCompletion::suggestBindingCompletion(const DomItem &itemAtPosition) const
{
    QList<CompletionItem> res;
    res << suggestReachableTypes(itemAtPosition, LocalSymbolsType::AttachedType,
                          CompletionItemKind::Class);

    const QQmlJSScope::ConstPtr scope = [&]() {
        if (!QQmlLSUtils::isFieldMemberAccess(itemAtPosition))
            return itemAtPosition.qmlObject().semanticScope();

        const DomItem owner = itemAtPosition.directParent().field(Fields::left);
        auto expressionType = QQmlLSUtils::resolveExpressionType(
                owner, ResolveActualTypeForFieldMemberExpression);
        return expressionType ? expressionType->semanticScope : QQmlJSScope::ConstPtr{};
    }();

    if (!scope)
        return res;

    res << propertyCompletion(scope, nullptr);
    res << signalHandlerCompletion(scope, nullptr);

    return res;
}

QList<CompletionItem>
QQmlLSCompletion::insideImportCompletionHelper(const DomItem &file,
                                               const QQmlLSCompletionPosition &positionInfo) const
{
    const CompletionContextStrings &ctx = positionInfo.cursorPosition;
    // returns completions for import statements, ctx is supposed to be in an import statement
    QList<CompletionItem> res;
    ImportCompletionType importCompletionType = ImportCompletionType::None;
    QRegularExpression spaceRe(uR"(\W+)"_s);
    QList<QStringView> linePieces = ctx.preLine().split(spaceRe, Qt::SkipEmptyParts);
    qsizetype effectiveLength = linePieces.size()
            + ((!ctx.preLine().isEmpty() && ctx.preLine().last().isSpace()) ? 1 : 0);
    if (effectiveLength < 2) {
        CompletionItem comp;
        comp.label = "import";
        comp.kind = int(CompletionItemKind::Keyword);
        res.append(comp);
    }
    if (linePieces.isEmpty() || linePieces.first() != u"import")
        return res;
    if (effectiveLength == 2) {
        // the cursor is after the import, possibly in a partial module name
        importCompletionType = ImportCompletionType::Module;
    } else if (effectiveLength == 3) {
        if (linePieces.last() != u"as") {
            // the cursor is after the module, possibly in a partial version token (or partial as)
            CompletionItem comp;
            comp.label = "as";
            comp.kind = int(CompletionItemKind::Keyword);
            res.append(comp);
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
                        res.append(comp);
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
                    res.append(comp);
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
                        res.append(comp);
                    }
                }
            }
            break;
        }
    }
    return res;
}

QList<CompletionItem> QQmlLSCompletion::idsCompletions(const DomItem &component) const
{
    qCDebug(QQmlLSCompletionLog) << "adding ids completions";
    QList<CompletionItem> res;
    for (const QString &k : component.field(Fields::ids).keys()) {
        CompletionItem comp;
        comp.label = k.toUtf8();
        comp.kind = int(CompletionItemKind::Value);
        res.append(comp);
    }
    return res;
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
QList<CompletionItem> QQmlLSCompletion::suggestReachableTypes(const DomItem &el,
                                                              LocalSymbolsTypes options,
                                                              CompletionItemKind kind) const
{
    auto file = el.containingFile().as<QmlFile>();
    if (!file)
        return {};
    auto resolver = file->typeResolver();
    if (!resolver)
        return {};

    const QString requiredQualifiers = QQmlLSUtils::qualifiersFrom(el);
    QList<CompletionItem> res;
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
        res << completion;
    }
    return res;
}

QList<CompletionItem>
QQmlLSCompletion::jsIdentifierCompletion(const QQmlJSScope::ConstPtr &scope,
                                         QDuplicateTracker<QString> *usedNames) const
{
    QList<CompletionItem> result;
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
        result.append(completion);
    }
    return result;
}

QList<CompletionItem>
QQmlLSCompletion::methodCompletion(const QQmlJSScope::ConstPtr &scope,
                                   QDuplicateTracker<QString> *usedNames) const
{
    QList<CompletionItem> result;
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
        result.append(completion);
        // TODO: QQmlLSUtils::reachableSymbols seems to be able to do documentation and detail
        // and co, it should also be done here if possible.
    }
    return result;
}

QList<CompletionItem>
QQmlLSCompletion::propertyCompletion(const QQmlJSScope::ConstPtr &scope,
                                     QDuplicateTracker<QString> *usedNames) const
{
    QList<CompletionItem> result;
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
        result.append(completion);
    }
    return result;
}

QList<CompletionItem>
QQmlLSCompletion::enumerationCompletion(const QQmlJSScope::ConstPtr &scope,
                                        QDuplicateTracker<QString> *usedNames) const
{
    QList<CompletionItem> result;
    for (const QQmlJSMetaEnum &enumerator : scope->enumerations()) {
        if (usedNames && usedNames->hasSeen(enumerator.name())) {
            continue;
        }
        CompletionItem completion;
        completion.label = enumerator.name().toUtf8();
        completion.kind = static_cast<int>(CompletionItemKind::Enum);
        result.append(completion);
    }
    return result;
}

QList<CompletionItem>
QQmlLSCompletion::enumerationValueCompletionHelper(const QStringList &enumeratorKeys) const
{
    QList<CompletionItem> result;
    for (const QString &enumeratorKey : enumeratorKeys) {
        CompletionItem completion;
        completion.label = enumeratorKey.toUtf8();
        completion.kind = static_cast<int>(CompletionItemKind::EnumMember);
        result.append(completion);
    }
    return result;
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

QList<CompletionItem>
QQmlLSCompletion::enumerationValueCompletion(const QQmlJSScope::ConstPtr &scope,
                                             const QString &enumeratorName) const
{
    auto enumerator = scope->enumeration(enumeratorName);
    if (enumerator.isValid()) {
        return enumerationValueCompletionHelper(enumerator.keys());
    }

    QList<CompletionItem> result;
    for (const QQmlJSMetaEnum &enumerator : scope->enumerations()) {
        result << enumerationValueCompletionHelper(enumerator.keys());
    }
    return result;
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
decltype(auto) collectFromAllJavaScriptParents(const F &&f, const QQmlJSScope::ConstPtr &scope)
{
    decltype(f(scope)) result;
    for (QQmlJSScope::ConstPtr current = scope; current; current = current->parentScope()) {
        result << f(current);
        if (current->scopeType() == QQmlSA::ScopeType::QMLScope)
            break;
    }
    return result;
}

/*!
\internal
Generate autocompletions for JS expressions, suggest possible properties, methods, etc.

If scriptIdentifier is inside a Field Member Expression, like \c{onCompleted} in
\c{Component.onCompleted} for example, then this method will only suggest properties, methods, etc
from the correct type. For the previous example that would be properties, methods, etc. from the
Component attached type.
*/
QList<CompletionItem>
QQmlLSCompletion::suggestJSExpressionCompletion(const DomItem &scriptIdentifier) const
{
    QList<CompletionItem> result;
    QDuplicateTracker<QString> usedNames;
    QQmlJSScope::ConstPtr nearestScope;

    // note: there is an edge case, where the user asks for completion right after the dot
    // of some qualified expression like `root.hello`. In this case, scriptIdentifier is actually
    // the BinaryExpression instead of the left-hand-side that has not be written down yet.
    const bool askForCompletionOnDot = QQmlLSUtils::isFieldMemberExpression(scriptIdentifier);
    const bool hasQualifier =
            QQmlLSUtils::isFieldMemberAccess(scriptIdentifier) || askForCompletionOnDot;

    if (!hasQualifier) {
        for (QUtf8StringView view : std::array<QUtf8StringView, 3>{ "null", "false", "true" }) {
            result.emplaceBack();
            result.back().label = view.data();
            result.back().kind = int(CompletionItemKind::Value);
        }
        result << idsCompletions(scriptIdentifier.component())
               << suggestReachableTypes(scriptIdentifier,
                                 LocalSymbolsType::Singleton | LocalSymbolsType::AttachedType,
                                 CompletionItemKind::Class);

        auto scope = scriptIdentifier.nearestSemanticScope();
        if (!scope)
            return result;
        nearestScope = scope;

        result << enumerationCompletion(nearestScope, &usedNames);
    } else {
        const DomItem owner =
                (askForCompletionOnDot ? scriptIdentifier : scriptIdentifier.directParent())
                        .field(Fields::left);
        auto expressionType = QQmlLSUtils::resolveExpressionType(
                owner, ResolveActualTypeForFieldMemberExpression);
        if (!expressionType || !expressionType->semanticScope)
            return result;
        nearestScope = expressionType->semanticScope;
        // Use root element scope to use find the enumerations
        // This should be changed when we support usages in external files
        if (expressionType->type == QmlComponentIdentifier)
            nearestScope = owner.rootQmlObject(GoTo::MostLikely).semanticScope();
        if (expressionType->name) {
            // note: you only get enumeration values in qualified expressions, never alone
            result << enumerationValueCompletion(nearestScope, *expressionType->name);

            // skip enumeration types if already inside an enumeration type
            if (auto enumerator = nearestScope->enumeration(*expressionType->name);
                !enumerator.isValid()) {
                result << enumerationCompletion(nearestScope, &usedNames);
            }

            if (expressionType->type == EnumeratorIdentifier)
                return result;
        }
    }

    Q_ASSERT(nearestScope);

    result << methodCompletion(nearestScope, &usedNames)
           << propertyCompletion(nearestScope, &usedNames);

    if (!hasQualifier) {
        // collect all of the stuff from parents
        result << collectFromAllJavaScriptParents(
                [this, &usedNames](const QQmlJSScope::ConstPtr &scope) {
                    return jsIdentifierCompletion(scope, &usedNames);
                },
                nearestScope)
               << collectFromAllJavaScriptParents(
                          [this, &usedNames](const QQmlJSScope::ConstPtr &scope) {
                              return methodCompletion(scope, &usedNames);
                          },
                          nearestScope)
               << collectFromAllJavaScriptParents(
                          [this, &usedNames](const QQmlJSScope::ConstPtr &scope) {
                              return propertyCompletion(scope, &usedNames);
                          },
                          nearestScope);

        auto file = scriptIdentifier.containingFile().as<QmlFile>();
        if (!file)
            return result;
        auto resolver = file->typeResolver();
        if (!resolver)
            return result;

        const auto globals = resolver->jsGlobalObject();
        result << methodCompletion(globals, &usedNames) << propertyCompletion(globals, &usedNames);
    }

    return result;
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

QList<CompletionItem>
QQmlLSCompletion::insidePragmaCompletion(QQmlJS::Dom::DomItem currentItem,
                                         const QQmlLSCompletionPosition &positionInfo) const
{
    if (cursorAfterColon(currentItem, positionInfo)) {
        const QString name = currentItem.field(Fields::name).value().toString();
        auto values = valuesForPragmas.constFind(name);
        if (values == valuesForPragmas.constEnd())
            return {};

        QList<CompletionItem> res;
        for (const auto &value : *values) {
            CompletionItem comp;
            comp.label = value.toUtf8();
            comp.kind = static_cast<int>(CompletionItemKind::Value);
            res.append(comp);
        }
        return res;
    }

    QList<CompletionItem> res;
    for (const auto &pragma : valuesForPragmas.asKeyValueRange()) {
        CompletionItem comp;
        comp.label = pragma.first.toUtf8();
        if (!pragma.second.isEmpty()) {
            comp.insertText = QString(pragma.first).append(u": ").toUtf8();
        }
        comp.kind = static_cast<int>(CompletionItemKind::Value);
        res.append(comp);
    }
    return res;
}

QList<CompletionItem>
QQmlLSCompletion::insideQmlObjectCompletion(const DomItem &parentForContext,
                                            const QQmlLSCompletionPosition &positionInfo) const
{

    const auto regions = FileLocations::treeOf(parentForContext)->info().regions;

    const QQmlJS::SourceLocation leftBrace = regions[LeftBraceRegion];
    const QQmlJS::SourceLocation rightBrace = regions[RightBraceRegion];

    if (beforeLocation(positionInfo, leftBrace)) {
        QList<CompletionItem> res;
        LocalSymbolsTypes options;
        options.setFlag(LocalSymbolsType::ObjectType);
        res << suggestReachableTypes(positionInfo.itemAtPosition, options,
                              CompletionItemKind::Constructor);
        res << suggestQuickSnippetsCompletion(positionInfo.itemAtPosition);

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
            res << suggestJSExpressionCompletion(positionInfo.itemAtPosition);
        }
        return res;
    }

    if (betweenLocations(leftBrace, positionInfo, rightBrace)) {
        QList<CompletionItem> res;
        // default/required property completion
        for (QUtf8StringView view :
             std::array<QUtf8StringView, 6>{ "", "readonly ", "default ", "default required ",
                                             "required default ", "required " }) {
            // readonly properties require an initializer
            if (view != QUtf8StringView("readonly ")) {
                res.append(makeSnippet(
                        QByteArray(view.data()).append("property type name;"),
                        QByteArray(view.data()).append("property ${1:type} ${0:name};")));
            }

            res.append(makeSnippet(
                    QByteArray(view.data()).append("property type name: value;"),
                    QByteArray(view.data()).append("property ${1:type} ${2:name}: ${0:value};")));
        }

        // signal
        res.append(makeSnippet("signal name(arg1:type1, ...)", "signal ${1:name}($0)"));

        // signal without parameters
        res.append(makeSnippet("signal name;", "signal ${0:name};"));

        // make already existing property required
        res.append(makeSnippet("required name;", "required ${0:name};"));

        // function
        res.append(makeSnippet("function name(args...): returnType { statements...}",
                               "function ${1:name}($2): ${3:returnType} {\n\t$0\n}"));

        // enum
        res.append(makeSnippet("enum name { Values...}", "enum ${1:name} {\n\t${0:values}\n}"));

        // inline component
        res.append(makeSnippet("component Name: BaseType { ... }",
                               "component ${1:name}: ${2:baseType} {\n\t$0\n}"));

        // add bindings
        const DomItem containingObject = parentForContext.qmlObject();
        res += suggestBindingCompletion(containingObject);

        // add Qml Types for default binding
        const DomItem containingFile = parentForContext.containingFile();
        res += suggestReachableTypes(containingFile, LocalSymbolsType::ObjectType,
                              CompletionItemKind::Constructor);
        res << suggestQuickSnippetsCompletion(positionInfo.itemAtPosition);
        return res;
    }
    return {};
}

QList<CompletionItem> QQmlLSCompletion::insidePropertyDefinitionCompletion(
        const DomItem &currentItem, const QQmlLSCompletionPosition &positionInfo) const
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
        QList<CompletionItem> items;
        auto addCompletionKeyword = [&items](QUtf8StringView view, bool complete) {
            if (!complete)
                return;
            CompletionItem item;
            item.label = view.data();
            item.kind = int(CompletionItemKind::Keyword);
            items.append(item);
        };
        addCompletionKeyword(u8"readonly", completeReadonly);
        addCompletionKeyword(u8"required", completeRequired);
        addCompletionKeyword(u8"default", completeDefault);
        addCompletionKeyword(u8"property", true);

        return items;
    }

    const QQmlJS::SourceLocation propertyIdentifier = info.regions[IdentifierRegion];
    if (propertyKeyword.end() <= positionInfo.offset()
        && positionInfo.offset() < propertyIdentifier.offset) {
        return suggestReachableTypes(currentItem,
                              LocalSymbolsType::ObjectType | LocalSymbolsType::ValueType,
                              CompletionItemKind::Class);
    }
    // do not autocomplete the rest
    return {};
}

QList<CompletionItem>
QQmlLSCompletion::insideBindingCompletion(const DomItem &currentItem,
                                          const QQmlLSCompletionPosition &positionInfo) const
{
    const DomItem containingBinding = currentItem.filterUp(
            [](DomType type, const QQmlJS::Dom::DomItem &) { return type == DomType::Binding; },
            FilterUpOptions::ReturnOuter);

    // do scriptidentifiercompletion after the ':' of a binding
    if (cursorAfterColon(containingBinding, positionInfo)) {
        QList<CompletionItem> res;
        res << suggestJSExpressionCompletion(positionInfo.itemAtPosition);

        if (auto type = QQmlLSUtils::resolveExpressionType(currentItem, ResolveOwnerType)) {
            const QStringList names = currentItem.field(Fields::name).toString().split(u'.');
            const QQmlJSScope *current = resolve(type->semanticScope.get(), names);
            // add type names when binding to an object type or a property with var type
            if (!current || current->accessSemantics() == QQmlSA::AccessSemantics::Reference) {
                LocalSymbolsTypes options;
                options.setFlag(LocalSymbolsType::ObjectType);
                res << suggestReachableTypes(positionInfo.itemAtPosition, options,
                                      CompletionItemKind::Constructor);
                res << suggestQuickSnippetsCompletion(positionInfo.itemAtPosition);
            }
        }
        return res;
    }

    // ignore the binding if asking for completion in front of the binding
    if (cursorInFrontOfItem(containingBinding, positionInfo)) {
        return insideQmlObjectCompletion(currentItem.containingObject(), positionInfo);
    }

    QList<CompletionItem> res;
    const DomItem containingObject = currentItem.qmlObject();

    res << suggestBindingCompletion(positionInfo.itemAtPosition);

    // add Qml Types for default binding
    res += suggestReachableTypes(positionInfo.itemAtPosition, LocalSymbolsType::ObjectType,
                          CompletionItemKind::Constructor);
    res << suggestQuickSnippetsCompletion(positionInfo.itemAtPosition);
    return res;
}

QList<CompletionItem>
QQmlLSCompletion::insideImportCompletion(const DomItem &currentItem,
                                         const QQmlLSCompletionPosition &positionInfo) const
{
    const DomItem containingFile = currentItem.containingFile();
    QList<CompletionItem> res;
    res += insideImportCompletionHelper(containingFile, positionInfo);

    // when in front of the import statement: propose types for root Qml Object completion
    if (cursorInFrontOfItem(currentItem, positionInfo)) {
        res += suggestReachableTypes(containingFile, LocalSymbolsType::ObjectType,
                              CompletionItemKind::Constructor);
    }

    return res;
}

QList<CompletionItem>
QQmlLSCompletion::insideQmlFileCompletion(const DomItem &currentItem,
                                          const QQmlLSCompletionPosition &positionInfo) const
{
    const DomItem containingFile = currentItem.containingFile();
    QList<CompletionItem> res;
    // completions for code outside the root Qml Object
    // global completions
    if (positionInfo.cursorPosition.atLineStart()) {
        if (positionInfo.cursorPosition.base().isEmpty()) {
            for (const QStringView &s : std::array<QStringView, 2>({ u"pragma", u"import" })) {
                CompletionItem comp;
                comp.label = s.toUtf8();
                comp.kind = int(CompletionItemKind::Keyword);
                res.append(comp);
            }
        }
    }
    // Types for root Qml Object completion
    res += suggestReachableTypes(containingFile, LocalSymbolsType::ObjectType,
                          CompletionItemKind::Constructor);
    return res;
}

/*!
\internal
Generate the snippets for let, var and const variable declarations.
*/
QList<CompletionItem> QQmlLSCompletion::suggestVariableDeclarationStatementCompletion(
        QQmlLSUtilsAppendOption option) const
{
    QList<CompletionItem> result;
    // let/var/const statement
    for (auto view : std::array<QUtf8StringView, 3>{ "let", "var", "const" }) {
        result.append(makeSnippet(QByteArray(view.data()).append(" variable = value"),
                                  QByteArray(view.data()).append(" ${1:variable} = $0")));
        if (option == AppendSemicolon) {
            result.back().insertText->append(";");
            result.back().label.append(";");
        }
    }
    return result;
}

/*!
\internal
Generate the snippets for case and default statements.
*/
QList<CompletionItem> QQmlLSCompletion::suggestCaseAndDefaultStatementCompletion() const
{
    QList<CompletionItem> result;

    // case snippet
    result.append(makeSnippet("case value: statements...", "case ${1:value}:\n\t$0"));
    // case + brackets snippet
    result.append(makeSnippet("case value: { statements... }", "case ${1:value}: {\n\t$0\n}"));

    // default snippet
    result.append(makeSnippet("default: statements...", "default:\n\t$0"));
    // default + brackets snippet
    result.append(makeSnippet("default: { statements... }", "default: {\n\t$0\n}"));

    return result;
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
QList<CompletionItem>
QQmlLSCompletion::suggestContinueAndBreakStatementIfNeeded(const DomItem &itemAtPosition) const
{
    QList<CompletionItem> result;

    bool alreadyInLabel = false;
    bool alreadyInSwitch = false;
    for (DomItem current = itemAtPosition; current; current = current.directParent()) {
        switch (current.internalKind()) {
        case DomType::ScriptExpression:
            // reached end of script expression
            return result;

        case DomType::ScriptForStatement:
        case DomType::ScriptForEachStatement:
        case DomType::ScriptWhileStatement:
        case DomType::ScriptDoWhileStatement:
            result.emplaceBack();
            result.back().label = "continue";
            result.back().kind = int(CompletionItemKind::Keyword);

            // do not add break twice
            if (!alreadyInSwitch && !alreadyInLabel) {
                result.emplaceBack();
                result.back().label = "break";
                result.back().kind = int(CompletionItemKind::Keyword);
            }
            // early exit: cannot suggest more completions
            return result;

        case DomType::ScriptSwitchStatement:
            // check if break was already inserted
            if (alreadyInSwitch || alreadyInLabel)
                break;
            alreadyInSwitch = true;

            result.emplaceBack();
            result.back().label = "break";
            result.back().kind = int(CompletionItemKind::Keyword);
            break;

        case DomType::ScriptLabelledStatement:
            // check if break was already inserted because of switch or loop
            if (alreadyInSwitch || alreadyInLabel)
                break;
            alreadyInLabel = true;

            result.emplaceBack();
            result.back().label = "break";
            result.back().kind = int(CompletionItemKind::Keyword);
            break;

        default:
            break;
        }
    }
    return result;
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
QList<CompletionItem>
QQmlLSCompletion::suggestJSStatementCompletion(const DomItem &itemAtPosition) const
{
    QList<CompletionItem> result = suggestJSExpressionCompletion(itemAtPosition);
    if (QQmlLSUtils::isFieldMemberAccess(itemAtPosition))
        return result;

    // expression statements
    result << suggestVariableDeclarationStatementCompletion();
    // block statement
    result.append(makeSnippet("{ statements... }", "{\n\t$0\n}"));

    // if + brackets statement
    result.append(makeSnippet("if (condition) { statements }", "if ($1) {\n\t$0\n}"));

    // do statement
    result.append(makeSnippet("do { statements } while (condition);", "do {\n\t$1\n} while ($0);"));

    // while + brackets statement
    result.append(makeSnippet("while (condition) { statements...}", "while ($1) {\n\t$0\n}"));

    // for + brackets loop statement
    result.append(makeSnippet("for (initializer; condition; increment) { statements... }",
                              "for ($1;$2;$3) {\n\t$0\n}"));

    // for ... in + brackets loop statement
    result.append(
            makeSnippet("for (property in object) { statements... }", "for ($1 in $2) {\n\t$0\n}"));

    // for ... of + brackets loop statement
    result.append(
            makeSnippet("for (element of array) { statements... }", "for ($1 of $2) {\n\t$0\n}"));

    // try + catch statement
    result.append(makeSnippet("try { statements... } catch(error) { statements... }",
                              "try {\n\t$1\n} catch($2) {\n\t$0\n}"));

    // try + finally statement
    result.append(makeSnippet("try { statements... } finally { statements... }",
                              "try {\n\t$1\n} finally {\n\t$0\n}"));

    // try + catch + finally statement
    result.append(makeSnippet(
            "try { statements... } catch(error) { statements... } finally { statements... }",
            "try {\n\t$1\n} catch($2) {\n\t$3\n} finally {\n\t$0\n}"));

    // one can always assume that JS code in QML is inside a function, so always propose `return`
    for (auto &&view : { "return"_ba, "throw"_ba }) {
        result.emplaceBack();
        result.back().label = std::move(view);
        result.back().kind = int(CompletionItemKind::Keyword);
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
        result << suggestCaseAndDefaultStatementCompletion();
    }

    result << suggestContinueAndBreakStatementIfNeeded(itemAtPosition);

    return result;
}

QList<CompletionItem>
QQmlLSCompletion::insideForStatementCompletion(const DomItem &parentForContext,
                                               const QQmlLSCompletionPosition &positionInfo) const
{
    const auto regions = FileLocations::treeOf(parentForContext)->info().regions;

    const QQmlJS::SourceLocation leftParenthesis = regions[LeftParenthesisRegion];
    const QQmlJS::SourceLocation firstSemicolon = regions[FirstSemicolonTokenRegion];
    const QQmlJS::SourceLocation secondSemicolon = regions[SecondSemicolonRegion];
    const QQmlJS::SourceLocation rightParenthesis = regions[RightParenthesisRegion];

    if (betweenLocations(leftParenthesis, positionInfo, firstSemicolon)) {
        QList<CompletionItem> res;
        res << suggestJSExpressionCompletion(positionInfo.itemAtPosition)
            << suggestVariableDeclarationStatementCompletion(
                       QQmlLSUtilsAppendOption::AppendNothing);
        return res;
    }
    if (betweenLocations(firstSemicolon, positionInfo, secondSemicolon)
        || betweenLocations(secondSemicolon, positionInfo, rightParenthesis)) {
        QList<CompletionItem> res;
        res << suggestJSExpressionCompletion(positionInfo.itemAtPosition);
        return res;
    }

    if (afterLocation(rightParenthesis, positionInfo)) {
        return suggestJSStatementCompletion(positionInfo.itemAtPosition);
    }

    return {};
}

QList<CompletionItem>
QQmlLSCompletion::insideScriptLiteralCompletion(const DomItem &currentItem,
                                                const QQmlLSCompletionPosition &positionInfo) const
{
    Q_UNUSED(currentItem);
    if (positionInfo.cursorPosition.base().isEmpty())
        return suggestJSExpressionCompletion(positionInfo.itemAtPosition);
    return {};
}

QList<CompletionItem>
QQmlLSCompletion::insideCallExpression(const DomItem &currentItem,
                                       const QQmlLSCompletionPosition &positionInfo) const
{
    const auto regions = FileLocations::treeOf(currentItem)->info().regions;
    const QQmlJS::SourceLocation leftParenthesis = regions[LeftParenthesisRegion];
    const QQmlJS::SourceLocation rightParenthesis = regions[RightParenthesisRegion];
    if (beforeLocation(positionInfo, leftParenthesis)) {
        return suggestJSExpressionCompletion(positionInfo.itemAtPosition);
    }
    if (betweenLocations(leftParenthesis, positionInfo, rightParenthesis)) {
        return suggestJSExpressionCompletion(positionInfo.itemAtPosition);
    }
    return {};
}

QList<CompletionItem>
QQmlLSCompletion::insideIfStatement(const DomItem &currentItem,
                                    const QQmlLSCompletionPosition &positionInfo) const
{
    const auto regions = FileLocations::treeOf(currentItem)->info().regions;
    const QQmlJS::SourceLocation leftParenthesis = regions[LeftParenthesisRegion];
    const QQmlJS::SourceLocation rightParenthesis = regions[RightParenthesisRegion];
    const QQmlJS::SourceLocation elseKeyword = regions[ElseKeywordRegion];

    if (betweenLocations(leftParenthesis, positionInfo, rightParenthesis)) {
        return suggestJSExpressionCompletion(positionInfo.itemAtPosition);
    }
    if (betweenLocations(rightParenthesis, positionInfo, elseKeyword)) {
        return suggestJSStatementCompletion(positionInfo.itemAtPosition);
    }
    if (afterLocation(elseKeyword, positionInfo)) {
        return suggestJSStatementCompletion(positionInfo.itemAtPosition);
    }
    return {};
}

QList<CompletionItem>
QQmlLSCompletion::insideReturnStatement(const DomItem &currentItem,
                                        const QQmlLSCompletionPosition &positionInfo) const
{
    const auto regions = FileLocations::treeOf(currentItem)->info().regions;
    const QQmlJS::SourceLocation returnKeyword = regions[ReturnKeywordRegion];

    if (afterLocation(returnKeyword, positionInfo)) {
        return suggestJSExpressionCompletion(positionInfo.itemAtPosition);
    }
    return {};
}

QList<CompletionItem>
QQmlLSCompletion::insideWhileStatement(const DomItem &currentItem,
                                       const QQmlLSCompletionPosition &positionInfo) const
{
    const auto regions = FileLocations::treeOf(currentItem)->info().regions;
    const QQmlJS::SourceLocation leftParenthesis = regions[LeftParenthesisRegion];
    const QQmlJS::SourceLocation rightParenthesis = regions[RightParenthesisRegion];

    if (betweenLocations(leftParenthesis, positionInfo, rightParenthesis)) {
        return suggestJSExpressionCompletion(positionInfo.itemAtPosition);
    }
    if (afterLocation(rightParenthesis, positionInfo)) {
        return suggestJSStatementCompletion(positionInfo.itemAtPosition);
    }
    return {};
}

QList<CompletionItem>
QQmlLSCompletion::insideDoWhileStatement(const DomItem &parentForContext,
                                         const QQmlLSCompletionPosition &positionInfo) const
{
    const auto regions = FileLocations::treeOf(parentForContext)->info().regions;
    const QQmlJS::SourceLocation doKeyword = regions[DoKeywordRegion];
    const QQmlJS::SourceLocation whileKeyword = regions[WhileKeywordRegion];
    const QQmlJS::SourceLocation leftParenthesis = regions[LeftParenthesisRegion];
    const QQmlJS::SourceLocation rightParenthesis = regions[RightParenthesisRegion];

    if (betweenLocations(doKeyword, positionInfo, whileKeyword)) {
        return suggestJSStatementCompletion(positionInfo.itemAtPosition);
    }
    if (betweenLocations(leftParenthesis, positionInfo, rightParenthesis)) {
        return suggestJSExpressionCompletion(positionInfo.itemAtPosition);
    }
    return {};
}

QList<CompletionItem>
QQmlLSCompletion::insideForEachStatement(const DomItem &parentForContext,
                                         const QQmlLSCompletionPosition &positionInfo) const
{
    const auto regions = FileLocations::treeOf(parentForContext)->info().regions;

    const QQmlJS::SourceLocation inOf = regions[InOfTokenRegion];
    const QQmlJS::SourceLocation leftParenthesis = regions[LeftParenthesisRegion];
    const QQmlJS::SourceLocation rightParenthesis = regions[RightParenthesisRegion];

    if (betweenLocations(leftParenthesis, positionInfo, inOf)) {
        QList<CompletionItem> res;
        res << suggestJSExpressionCompletion(positionInfo.itemAtPosition)
            << suggestVariableDeclarationStatementCompletion();
        return res;
    }
    if (betweenLocations(inOf, positionInfo, rightParenthesis)) {
        const QList<CompletionItem> res =
                suggestJSExpressionCompletion(positionInfo.itemAtPosition);
        return res;
    }

    if (afterLocation(rightParenthesis, positionInfo)) {
        return suggestJSStatementCompletion(positionInfo.itemAtPosition);
    }

    return {};
}

QList<CompletionItem>
QQmlLSCompletion::insideSwitchStatement(const DomItem &parentForContext,
                                        const QQmlLSCompletionPosition positionInfo) const
{
    const auto regions = FileLocations::treeOf(parentForContext)->info().regions;

    const QQmlJS::SourceLocation leftParenthesis = regions[LeftParenthesisRegion];
    const QQmlJS::SourceLocation rightParenthesis = regions[RightParenthesisRegion];

    if (betweenLocations(leftParenthesis, positionInfo, rightParenthesis)) {
        const QList<CompletionItem> res =
                suggestJSExpressionCompletion(positionInfo.itemAtPosition);
        return res;
    }

    return {};
}

QList<CompletionItem>
QQmlLSCompletion::insideCaseClause(const DomItem &parentForContext,
                                   const QQmlLSCompletionPosition &positionInfo) const
{
    const auto regions = FileLocations::treeOf(parentForContext)->info().regions;

    const QQmlJS::SourceLocation caseKeyword = regions[CaseKeywordRegion];
    const QQmlJS::SourceLocation colonToken = regions[ColonTokenRegion];

    if (betweenLocations(caseKeyword, positionInfo, colonToken)) {
        const QList<CompletionItem> res =
                suggestJSExpressionCompletion(positionInfo.itemAtPosition);
        return res;
    }
    if (afterLocation(colonToken, positionInfo)) {
        const QList<CompletionItem> res = suggestJSStatementCompletion(positionInfo.itemAtPosition);
        return res;
    }

    return {};
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

QList<CompletionItem>
QQmlLSCompletion::insideCaseBlock(const DomItem &parentForContext,
                                  const QQmlLSCompletionPosition &positionInfo) const
{
    const auto regions = FileLocations::treeOf(parentForContext)->info().regions;

    const QQmlJS::SourceLocation leftBrace = regions[LeftBraceRegion];
    const QQmlJS::SourceLocation rightBrace = regions[RightBraceRegion];

    if (!betweenLocations(leftBrace, positionInfo, rightBrace))
        return {};

    // TODO: looks fishy
    // if there is a previous case or default clause, you can still add statements to it
    if (const auto previousCase = previousCaseOfCaseBlock(parentForContext, positionInfo))
        return suggestJSStatementCompletion(previousCase);

    // otherwise, only complete case and default
    return suggestCaseAndDefaultStatementCompletion();
}

QList<CompletionItem>
QQmlLSCompletion::insideDefaultClause(const DomItem &parentForContext,
                                      const QQmlLSCompletionPosition &positionInfo) const
{
    const auto regions = FileLocations::treeOf(parentForContext)->info().regions;

    const QQmlJS::SourceLocation colonToken = regions[ColonTokenRegion];

    if (afterLocation(colonToken, positionInfo)) {
        const QList<CompletionItem> res = suggestJSStatementCompletion(positionInfo.itemAtPosition);
        return res;
    }

    return {};
}

QList<CompletionItem> QQmlLSCompletion::insideBinaryExpressionCompletion(
        const DomItem &parentForContext, const QQmlLSCompletionPosition &positionInfo) const
{
    const auto regions = FileLocations::treeOf(parentForContext)->info().regions;

    const QQmlJS::SourceLocation operatorLocation = regions[OperatorTokenRegion];

    if (beforeLocation(positionInfo, operatorLocation)) {
        return suggestJSExpressionCompletion(positionInfo.itemAtPosition);
    }
    if (afterLocation(operatorLocation, positionInfo)) {
        return suggestJSExpressionCompletion(positionInfo.itemAtPosition);
    }

    return {};
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
QList<CompletionItem>
QQmlLSCompletion::insideScriptPattern(const DomItem &parentForContext,
                                      const QQmlLSCompletionPosition &positionInfo) const
{
    const auto regions = FileLocations::treeOf(parentForContext)->info().regions;

    const QQmlJS::SourceLocation equal = regions[EqualTokenRegion];

    if (!afterLocation(equal, positionInfo))
        return {};

    // otherwise, only complete case and default
    return suggestJSExpressionCompletion(positionInfo.itemAtPosition);
}

/*!
\internal
See comment on insideScriptPattern().
*/
QList<CompletionItem>
QQmlLSCompletion::insideVariableDeclarationEntry(const DomItem &parentForContext,
                                                 const QQmlLSCompletionPosition &positionInfo) const
{
    return insideScriptPattern(parentForContext, positionInfo);
}

QList<CompletionItem>
QQmlLSCompletion::insideThrowStatement(const DomItem &parentForContext,
                                       const QQmlLSCompletionPosition &positionInfo) const
{
    const auto regions = FileLocations::treeOf(parentForContext)->info().regions;

    const QQmlJS::SourceLocation throwKeyword = regions[ThrowKeywordRegion];

    if (afterLocation(throwKeyword, positionInfo)) {
        return suggestJSExpressionCompletion(positionInfo.itemAtPosition);
    }
    return {};
}

QList<CompletionItem>
QQmlLSCompletion::insideLabelledStatement(const DomItem &parentForContext,
                                          const QQmlLSCompletionPosition &positionInfo) const
{
    const auto regions = FileLocations::treeOf(parentForContext)->info().regions;

    const QQmlJS::SourceLocation colon = regions[ColonTokenRegion];

    if (afterLocation(colon, positionInfo)) {
        return suggestJSStatementCompletion(positionInfo.itemAtPosition);
    }
    // note: the case "beforeLocation(ctx, colon)" probably never happens:
    // this is because without the colon, the parser will probably not parse this as a
    // labelledstatement but as a normal expression statement.
    // So this case only happens when the colon already exists, and the user goes back to the
    // label name and requests completion for that label.
    return {};
}

/*!
\internal
Collect the current set of labels that some DomItem can jump to.
*/
static QList<CompletionItem> collectLabels(const DomItem &context)
{
    QList<CompletionItem> labels;
    for (DomItem current = context; current; current = current.directParent()) {
        if (current.internalKind() == DomType::ScriptLabelledStatement) {
            const QString label = current.field(Fields::label).value().toString();
            if (label.isEmpty())
                continue;
            labels.emplaceBack();
            labels.back().label = label.toUtf8();
            labels.back().kind = int(CompletionItemKind::Value); // variable?
            // TODO: more stuff here?
        } else if (current.internalKind() == DomType::ScriptExpression) {
            // quick exit when leaving the JS part
            return labels;
        }
    }
    return labels;
}

QList<CompletionItem>
QQmlLSCompletion::insideContinueStatement(const DomItem &parentForContext,
                                          const QQmlLSCompletionPosition &positionInfo) const
{
    const auto regions = FileLocations::treeOf(parentForContext)->info().regions;

    const QQmlJS::SourceLocation continueKeyword = regions[ContinueKeywordRegion];

    if (afterLocation(continueKeyword, positionInfo)) {
        return collectLabels(parentForContext);
    }
    return {};
}

QList<CompletionItem>
QQmlLSCompletion::insideBreakStatement(const DomItem &parentForContext,
                                       const QQmlLSCompletionPosition &positionInfo) const
{
    const auto regions = FileLocations::treeOf(parentForContext)->info().regions;

    const QQmlJS::SourceLocation breakKeyword = regions[BreakKeywordRegion];

    if (afterLocation(breakKeyword, positionInfo)) {
        return collectLabels(parentForContext);
    }
    return {};
}

QList<CompletionItem>
QQmlLSCompletion::insideConditionalExpression(const DomItem &parentForContext,
                                              const QQmlLSCompletionPosition &positionInfo) const
{
    const auto regions = FileLocations::treeOf(parentForContext)->info().regions;

    const QQmlJS::SourceLocation questionMark = regions[QuestionMarkTokenRegion];
    const QQmlJS::SourceLocation colon = regions[ColonTokenRegion];

    if (beforeLocation(positionInfo, questionMark)) {
        return suggestJSExpressionCompletion(positionInfo.itemAtPosition);
    }
    if (betweenLocations(questionMark, positionInfo, colon)) {
        return suggestJSExpressionCompletion(positionInfo.itemAtPosition);
    }
    if (afterLocation(colon, positionInfo)) {
        return suggestJSExpressionCompletion(positionInfo.itemAtPosition);
    }
    return {};
}

QList<CompletionItem>
QQmlLSCompletion::insideUnaryExpression(const DomItem &parentForContext,
                                        const QQmlLSCompletionPosition &positionInfo) const
{
    const auto regions = FileLocations::treeOf(parentForContext)->info().regions;

    const QQmlJS::SourceLocation operatorToken = regions[OperatorTokenRegion];

    if (afterLocation(operatorToken, positionInfo)) {
        return suggestJSExpressionCompletion(positionInfo.itemAtPosition);
    }

    return {};
}

QList<CompletionItem>
QQmlLSCompletion::insidePostExpression(const DomItem &parentForContext,
                                       const QQmlLSCompletionPosition &positionInfo) const
{
    const auto regions = FileLocations::treeOf(parentForContext)->info().regions;

    const QQmlJS::SourceLocation operatorToken = regions[OperatorTokenRegion];

    if (beforeLocation(positionInfo, operatorToken)) {
        return suggestJSExpressionCompletion(positionInfo.itemAtPosition);
    }
    return {};
}

QList<CompletionItem>
QQmlLSCompletion::insideParenthesizedExpression(const DomItem &parentForContext,
                                                const QQmlLSCompletionPosition &positionInfo) const
{
    const auto regions = FileLocations::treeOf(parentForContext)->info().regions;

    const QQmlJS::SourceLocation leftParenthesis = regions[LeftParenthesisRegion];
    const QQmlJS::SourceLocation rightParenthesis = regions[RightParenthesisRegion];

    if (betweenLocations(leftParenthesis, positionInfo, rightParenthesis)) {
        return suggestJSExpressionCompletion(positionInfo.itemAtPosition);
    }
    return {};
}

QList<CompletionItem>
QQmlLSCompletion::signalHandlerCompletion(const QQmlJSScope::ConstPtr &scope,
                                          QDuplicateTracker<QString> *usedNames) const
{
    QList<CompletionItem> res;
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
        res << completion;
    }
    return res;
}

QList<CompletionItem>
QQmlLSCompletion::suggestQuickSnippetsCompletion(const DomItem &itemAtPosition) const
{
    QList<CompletionItem> res;
    auto file = itemAtPosition.containingFile().as<QmlFile>();
    if (!file)
        return {};

    // check if QtQuick has been imported
    const auto &imports = file->imports();
    auto it = std::find_if(imports.constBegin(), imports.constEnd(), [](const Import &import) {
        return import.uri.moduleUri() == u"QtQuick";
    });
    if (it == imports.constEnd()) {
        return res;
    }

    // check if the user already typed some qualifier, remove its dot and compare it to QtQuick's
    // qualified name
    const QString userTypedQualifier = QQmlLSUtils::qualifiersFrom(itemAtPosition);
    if (!userTypedQualifier.isEmpty()
        && !it->importId.startsWith(QStringView(userTypedQualifier).chopped(1))) {
        return res;
    }

    const QByteArray prefixForSnippet =
            userTypedQualifier.isEmpty() ? it->importId.toUtf8() : QByteArray();
    const QByteArray prefixWithDotForSnippet =
            prefixForSnippet.isEmpty() ? QByteArray() : QByteArray(prefixForSnippet).append(u'.');

    // Quick completions from Qt Creator's code model
    res << makeSnippet(prefixForSnippet, "BorderImage snippet",
                       "BorderImage {\n"
                       "\tid: ${1:name}\n"
                       "\tsource: \"${2:file}\"\n"
                       "\twidth: ${3:100}; height: ${4:100}\n"
                       "\tborder.left: ${5: 5}; border.top: ${5}\n"
                       "\tborder.right: ${5}; border.bottom: ${5}\n"
                       "}");
    res << makeSnippet(prefixForSnippet, "ColorAnimation snippet",
                       "ColorAnimation {\n"
                       "\tfrom: \"${1:white}\"\n"
                       "\tto: \"${2:black}\"\n"
                       "\tduration: ${3:200}\n"
                       "}");
    res << makeSnippet(prefixForSnippet, "Image snippet",
                       "Image {\n"
                       "\tid: ${1:name}\n"
                       "\tsource: \"${2:file}\"\n"
                       "}");
    res << makeSnippet(prefixForSnippet, "Item snippet",
                       "Item {\n"
                       "\tid: ${1:name}\n"
                       "}");
    res << makeSnippet(prefixForSnippet, "NumberAnimation snippet",
                       "NumberAnimation {\n"
                       "\ttarget: ${1:object}\n"
                       "\tproperty: \"${2:name}\"\n"
                       "\tduration: ${3:200}\n"
                       "\teasing.type: "_ba.append(prefixWithDotForSnippet)
                               .append("Easing.${4:InOutQuad}\n"
                                       "}"));
    res << makeSnippet(prefixForSnippet, "NumberAnimation with targets snippet",
                       "NumberAnimation {\n"
                       "\ttargets: [${1:object}]\n"
                       "\tproperties: \"${2:name}\"\n"
                       "\tduration: ${3:200}\n"
                       "}");
    res << makeSnippet(prefixForSnippet, "PauseAnimation snippet",
                       "PauseAnimation {\n"
                       "\tduration: ${1:200}\n"
                       "}");
    res << makeSnippet(prefixForSnippet, "PropertyAction snippet",
                       "PropertyAction {\n"
                       "\ttarget: ${1:object}\n"
                       "\tproperty: \"${2:name}\"\n"
                       "}");
    res << makeSnippet(prefixForSnippet, "PropertyAction with targets snippet",
                       "PropertyAction {\n"
                       "\ttargets: [${1:object}]\n"
                       "\tproperties: \"${2:name}\"\n"
                       "}");
    res << makeSnippet(prefixForSnippet, "PropertyChanges snippet",
                       "PropertyChanges {\n"
                       "\ttarget: ${1:object}\n"
                       "}");
    res << makeSnippet(prefixForSnippet, "State snippet",
                       "State {\n"
                       "\tname: ${1:name}\n"
                       "\t"_ba.append(prefixWithDotForSnippet)
                               .append("PropertyChanges {\n"
                                       "\t\ttarget: ${2:object}\n"
                                       "\t}\n"
                                       "}"));
    res << makeSnippet(prefixForSnippet, "Text snippet",
                       "Text {\n"
                       "\tid: ${1:name}\n"
                       "\ttext: qsTr(\"${2:text}\")\n"
                       "}");
    res << makeSnippet(prefixForSnippet, "Transition snippet",
                       "Transition {\n"
                       "\tfrom: \"${1:fromState}\"\n"
                       "\tto: \"${2:toState}\"\n"
                       "}");

    if (!userTypedQualifier.isEmpty())
        return res;

    auto resolver = file->typeResolver();
    if (!resolver)
        return res;
    const auto qquickItemScope = resolver->typeForName(prefixWithDotForSnippet + u"Item"_s);
    const QQmlJSScope::ConstPtr ownerScope = itemAtPosition.qmlObject().semanticScope();
    if (!ownerScope || !qquickItemScope)
        return res;

    if (ownerScope->inherits(qquickItemScope)) {
        res << makeSnippet("states binding with PropertyChanges in State",
                           "states: [\n"
                           "\t"_ba.append(prefixWithDotForSnippet)
                                   .append("State {\n"
                                           "\t\tname: \"${1:name}\"\n"
                                           "\t\t"_ba.append(prefixWithDotForSnippet)
                                                   .append("PropertyChanges {\n"
                                                           "\t\t\ttarget: ${2:object}\n"
                                                           "\t\t}\n"
                                                           "\t}\n"
                                                           "]")));
        res << makeSnippet("transitions binding with Transition",
                           "transitions: [\n"
                           "\t"_ba.append(prefixWithDotForSnippet)
                                   .append("Transition {\n"
                                           "\t\tfrom: \"${1:fromState}\"\n"
                                           "\t\tto: \"${2:fromState}\"\n"
                                           "\t}\n"
                                           "]"));
    }

    return res;
}

/*!
\internal
Decide which completions can be used at currentItem and compute them.
*/
QList<CompletionItem>
QQmlLSCompletion::completions(const DomItem &currentItem,
                              const CompletionContextStrings &contextStrings) const
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
            return {};
        case DomType::Pragma:
            return insidePragmaCompletion(currentItem, positionInfo);
        case DomType::ScriptType: {
            if (currentParent.directParent().internalKind() == DomType::QmlObject)
                return insideQmlObjectCompletion(currentParent.directParent(), positionInfo);

            LocalSymbolsTypes options;
            options.setFlag(LocalSymbolsType::ObjectType);
            options.setFlag(LocalSymbolsType::ValueType);
            return suggestReachableTypes(currentItem, options, CompletionItemKind::Class);
        }
        case DomType::ScriptFormalParameter:
            // no autocompletion inside of function parameter definition
            return {};
        case DomType::Binding:
            return insideBindingCompletion(currentParent, positionInfo);
        case DomType::Import:
            return insideImportCompletion(currentParent, positionInfo);
        case DomType::ScriptForStatement:
            return insideForStatementCompletion(currentParent, positionInfo);
        case DomType::ScriptBlockStatement:
            return suggestJSStatementCompletion(positionInfo.itemAtPosition);
        case DomType::QmlFile:
            return insideQmlFileCompletion(currentParent, positionInfo);
        case DomType::QmlObject:
            return insideQmlObjectCompletion(currentParent, positionInfo);
        case DomType::MethodInfo:
            // suppress completions
            return {};
        case DomType::PropertyDefinition:
            return insidePropertyDefinitionCompletion(currentParent, positionInfo);
        case DomType::ScriptBinaryExpression:
            // ignore field member expressions: these need additional context from its parents
            if (QQmlLSUtils::isFieldMemberExpression(currentParent))
                continue;
            return insideBinaryExpressionCompletion(currentParent, positionInfo);
        case DomType::ScriptLiteral:
            return insideScriptLiteralCompletion(currentParent, positionInfo);
        case DomType::ScriptCallExpression:
            return insideCallExpression(currentParent, positionInfo);
        case DomType::ScriptIfStatement:
            return insideIfStatement(currentParent, positionInfo);
        case DomType::ScriptReturnStatement:
            return insideReturnStatement(currentParent, positionInfo);
        case DomType::ScriptWhileStatement:
            return insideWhileStatement(currentParent, positionInfo);
        case DomType::ScriptDoWhileStatement:
            return insideDoWhileStatement(currentParent, positionInfo);
        case DomType::ScriptForEachStatement:
            return insideForEachStatement(currentParent, positionInfo);
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
            return {};
        case DomType::ScriptSwitchStatement:
            return insideSwitchStatement(currentParent, positionInfo);
        case DomType::ScriptCaseClause:
            return insideCaseClause(currentParent, positionInfo);
        case DomType::ScriptDefaultClause:
            if (ctxBeforeStatement(positionInfo, currentParent, QQmlJS::Dom::DefaultKeywordRegion))
                continue;
            return insideDefaultClause(currentParent, positionInfo);
        case DomType::ScriptCaseBlock:
            return insideCaseBlock(currentParent, positionInfo);
        case DomType::ScriptVariableDeclaration:
            // not needed: thats a list of ScriptVariableDeclarationEntry, and those entries cannot
            // be suggested because they all start with `{`, `[` or an identifier that should not be
            // in use yet.
            return {};
        case DomType::ScriptVariableDeclarationEntry:
            return insideVariableDeclarationEntry(currentParent, positionInfo);
        case DomType::ScriptProperty:
            // fallthrough: a ScriptProperty is a ScriptPattern but inside a JS Object. It gets the
            // same completions as a ScriptPattern.
        case DomType::ScriptPattern:
            return insideScriptPattern(currentParent, positionInfo);
        case DomType::ScriptThrowStatement:
            return insideThrowStatement(currentParent, positionInfo);
        case DomType::ScriptLabelledStatement:
            return insideLabelledStatement(currentParent, positionInfo);
        case DomType::ScriptContinueStatement:
            return insideContinueStatement(currentParent, positionInfo);
        case DomType::ScriptBreakStatement:
            return insideBreakStatement(currentParent, positionInfo);
        case DomType::ScriptConditionalExpression:
            return insideConditionalExpression(currentParent, positionInfo);
        case DomType::ScriptUnaryExpression:
            return insideUnaryExpression(currentParent, positionInfo);
        case DomType::ScriptPostExpression:
            return insidePostExpression(currentParent, positionInfo);
        case DomType::ScriptParenthesizedExpression:
            return insideParenthesizedExpression(currentParent, positionInfo);

        // TODO: Implement those statements.
        // In the meanwhile, suppress completions to avoid weird behaviors.
        case DomType::ScriptArray:
        case DomType::ScriptObject:
        case DomType::ScriptElision:
        case DomType::ScriptArrayEntry:
            return {};

        default:
            continue;
        }
        Q_UNREACHABLE();
    }

    // no completion could be found
    qCDebug(QQmlLSUtilsLog) << "No completion was found for current request.";
    return {};
}

QT_END_NAMESPACE
