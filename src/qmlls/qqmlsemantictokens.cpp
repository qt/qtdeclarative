// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include <qqmlsemantictokens_p.h>
#include <qqmldiffer_p.h>

#include <QtQmlLS/private/qqmllsutils_p.h>
#include <QtQmlDom/private/qqmldomscriptelements_p.h>
#include <QtQmlDom/private/qqmldomfieldfilter_p.h>

#include <QtLanguageServer/private/qlanguageserverprotocol_p.h>

#include <QtCore/qregularexpression.h>

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(semanticTokens, "qt.languageserver.semanticTokens")

using namespace QQmlJS::Dom;
using namespace QLspSpecification;

namespace QmlHighlighting {

static int mapToProtocolForQtCreator(QmlHighlightKind highlightKind)
{
    switch (highlightKind) {
    case QmlHighlightKind::Comment:
        return int(SemanticTokenProtocolTypes::Comment);
    case QmlHighlightKind::QmlKeyword:
        return int(SemanticTokenProtocolTypes::Keyword);
    case QmlHighlightKind::QmlType:
        return int(SemanticTokenProtocolTypes::Type);
    case QmlHighlightKind::QmlImportId:
    case QmlHighlightKind::QmlNamespace:
        return int(SemanticTokenProtocolTypes::Namespace);
    case QmlHighlightKind::QmlLocalId:
        return int(SemanticTokenProtocolTypes::QmlLocalId);
    case QmlHighlightKind::QmlExternalId:
        return int(SemanticTokenProtocolTypes::QmlExternalId);
    case QmlHighlightKind::QmlProperty:
        return int(SemanticTokenProtocolTypes::Property);
    case QmlHighlightKind::QmlScopeObjectProperty:
        return int(SemanticTokenProtocolTypes::QmlScopeObjectProperty);
    case QmlHighlightKind::QmlRootObjectProperty:
        return int(SemanticTokenProtocolTypes::QmlRootObjectProperty);
    case QmlHighlightKind::QmlExternalObjectProperty:
        return int(SemanticTokenProtocolTypes::QmlExternalObjectProperty);
    case QmlHighlightKind::QmlMethod:
        return int(SemanticTokenProtocolTypes::Method);
    case QmlHighlightKind::QmlMethodParameter:
        return int(SemanticTokenProtocolTypes::Parameter);
    case QmlHighlightKind::QmlSignal:
        return int(SemanticTokenProtocolTypes::Method);
    case QmlHighlightKind::QmlSignalHandler:
        return int(SemanticTokenProtocolTypes::Property);
    case QmlHighlightKind::QmlEnumName:
        return int(SemanticTokenProtocolTypes::Enum);
    case QmlHighlightKind::QmlEnumMember:
        return int(SemanticTokenProtocolTypes::EnumMember);
    case QmlHighlightKind::QmlPragmaName:
    case QmlHighlightKind::QmlPragmaValue:
        return int(SemanticTokenProtocolTypes::Variable);
    case QmlHighlightKind::JsImport:
        return int(SemanticTokenProtocolTypes::Namespace);
    case QmlHighlightKind::JsGlobalVar:
        return int(SemanticTokenProtocolTypes::JsGlobalVar);
    case QmlHighlightKind::JsGlobalMethod:
        return int(SemanticTokenProtocolTypes::Method);
    case QmlHighlightKind::JsScopeVar:
        return int(SemanticTokenProtocolTypes::JsScopeVar);
    case QmlHighlightKind::JsLabel:
        return int(SemanticTokenProtocolTypes::Variable);
    case QmlHighlightKind::Number:
        return int(SemanticTokenProtocolTypes::Number);
    case QmlHighlightKind::String:
        return int(SemanticTokenProtocolTypes::String);
    case QmlHighlightKind::Operator:
        return int(SemanticTokenProtocolTypes::Operator);
    case QmlHighlightKind::QmlTypeModifier:
        return int(SemanticTokenProtocolTypes::Decorator);
    case QmlHighlightKind::Field:
        return int(SemanticTokenProtocolTypes::Field);
    case QmlHighlightKind::Unknown:
    default:
        return int(SemanticTokenProtocolTypes::Unknown);
    }
}

static int mapToProtocolDefault(QmlHighlightKind highlightKind)
{
    switch (highlightKind) {
    case QmlHighlightKind::Comment:
        return int(SemanticTokenProtocolTypes::Comment);
    case QmlHighlightKind::QmlKeyword:
        return int(SemanticTokenProtocolTypes::Keyword);
    case QmlHighlightKind::QmlType:
        return int(SemanticTokenProtocolTypes::Type);
    case QmlHighlightKind::QmlImportId:
    case QmlHighlightKind::QmlNamespace:
        return int(SemanticTokenProtocolTypes::Namespace);
    case QmlHighlightKind::QmlLocalId:
    case QmlHighlightKind::QmlExternalId:
        return int(SemanticTokenProtocolTypes::Variable);
    case QmlHighlightKind::QmlProperty:
    case QmlHighlightKind::QmlScopeObjectProperty:
    case QmlHighlightKind::QmlRootObjectProperty:
    case QmlHighlightKind::QmlExternalObjectProperty:
        return int(SemanticTokenProtocolTypes::Property);
    case QmlHighlightKind::QmlMethod:
        return int(SemanticTokenProtocolTypes::Method);
    case QmlHighlightKind::QmlMethodParameter:
        return int(SemanticTokenProtocolTypes::Parameter);
    case QmlHighlightKind::QmlSignal:
        return int(SemanticTokenProtocolTypes::Method);
    case QmlHighlightKind::QmlSignalHandler:
        return int(SemanticTokenProtocolTypes::Method);
    case QmlHighlightKind::QmlEnumName:
        return int(SemanticTokenProtocolTypes::Enum);
    case QmlHighlightKind::QmlEnumMember:
        return int(SemanticTokenProtocolTypes::EnumMember);
    case QmlHighlightKind::QmlPragmaName:
    case QmlHighlightKind::QmlPragmaValue:
        return int(SemanticTokenProtocolTypes::Variable);
    case QmlHighlightKind::JsImport:
        return int(SemanticTokenProtocolTypes::Namespace);
    case QmlHighlightKind::JsGlobalVar:
        return int(SemanticTokenProtocolTypes::Variable);
    case QmlHighlightKind::JsGlobalMethod:
        return int(SemanticTokenProtocolTypes::Method);
    case QmlHighlightKind::JsScopeVar:
        return int(SemanticTokenProtocolTypes::Variable);
    case QmlHighlightKind::JsLabel:
        return int(SemanticTokenProtocolTypes::Variable);
    case QmlHighlightKind::Number:
        return int(SemanticTokenProtocolTypes::Number);
    case QmlHighlightKind::String:
        return int(SemanticTokenProtocolTypes::String);
    case QmlHighlightKind::Operator:
        return int(SemanticTokenProtocolTypes::Operator);
    case QmlHighlightKind::QmlTypeModifier:
        return int(SemanticTokenProtocolTypes::Decorator);
    case QmlHighlightKind::Field:
        return int(SemanticTokenProtocolTypes::Property);
    case QmlHighlightKind::Unknown:
    default:
        return int(SemanticTokenProtocolTypes::Unknown);
    }
}

/*!
\internal
\brief Further resolves the type of a JavaScriptIdentifier
A global object can be in the object form or in the function form.
For example, Date can be used as a constructor function (like new Date())
or as a object (like Date.now()).
*/
static std::optional<QmlHighlightKind> resolveJsGlobalObjectKind(const DomItem &item,
                                                                 const QString &name)
{
    // Some objects are not constructable, they are always objects.
    static QSet<QString> noConstructorObjects = { u"Math"_s, u"JSON"_s, u"Atomics"_s, u"Reflect"_s,
                                                  u"console"_s };
    // if the method name is in the list of noConstructorObjects, then it is a global object. Do not
    // perform further checks.
    if (noConstructorObjects.contains(name))
        return QmlHighlightKind::JsGlobalVar;
    // Check if the method is called with new, then it is a constructor function
    if (item.directParent().internalKind() == DomType::ScriptNewMemberExpression) {
        return QmlHighlightKind::JsGlobalMethod;
    }
    if (DomItem containingCallExpression = item.filterUp(
                [](DomType k, const DomItem &) { return k == DomType::ScriptCallExpression; },
                FilterUpOptions::ReturnOuter)) {
        // Call expression
        // if callee is binary expression, then the rightest part is the method name
        const auto callee = containingCallExpression.field(Fields::callee);
        if (callee.internalKind() == DomType::ScriptBinaryExpression) {
            const auto right = callee.field(Fields::right);
            if (right.internalKind() == DomType::ScriptIdentifierExpression
                && right.field(Fields::identifier).value().toString() == name) {
                return QmlHighlightKind::JsGlobalMethod;
            } else {
                return QmlHighlightKind::JsGlobalVar;
            }
        } else {
            return QmlHighlightKind::JsGlobalVar;
        }
    }
    return std::nullopt;
}

static int fromQmlModifierKindToLspTokenType(QmlHighlightModifiers highlightModifier)
{
    using namespace QLspSpecification;
    using namespace Utils;
    int modifier = 0;

    if (highlightModifier.testFlag(QmlHighlightModifier::QmlPropertyDefinition))
        addModifier(SemanticTokenModifiers::Definition, &modifier);

    if (highlightModifier.testFlag(QmlHighlightModifier::QmlDefaultProperty))
        addModifier(SemanticTokenModifiers::DefaultLibrary, &modifier);

    if (highlightModifier.testFlag(QmlHighlightModifier::QmlVirtualProperty))
        addModifier(SemanticTokenModifiers::Static, &modifier);

    if (highlightModifier.testFlag(QmlHighlightModifier::QmlOverrideProperty))
        addModifier(SemanticTokenModifiers::Static, &modifier);

    if (highlightModifier.testFlag(QmlHighlightModifier::QmlFinalProperty))
        addModifier(SemanticTokenModifiers::Static, &modifier);

    if (highlightModifier.testFlag(QmlHighlightModifier::QmlRequiredProperty))
        addModifier(SemanticTokenModifiers::Abstract, &modifier);

    if (highlightModifier.testFlag(QmlHighlightModifier::QmlReadonlyProperty))
        addModifier(SemanticTokenModifiers::Readonly, &modifier);

    return modifier;
}

static FieldFilter highlightingFilter()
{
    QMultiMap<QString, QString> fieldFilterAdd{};
    QMultiMap<QString, QString> fieldFilterRemove{
        { QString(), Fields::propertyInfos.toString() },
        { QString(), Fields::fileLocationsTree.toString() },
        { QString(), Fields::importScope.toString() },
        { QString(), Fields::defaultPropertyName.toString() },
        { QString(), Fields::get.toString() },
    };
    return FieldFilter{ fieldFilterAdd, fieldFilterRemove };
}

HighlightToken::HighlightToken(const QQmlJS::SourceLocation &loc,
                               QmlHighlightKind kind,
                               QmlHighlightModifiers modifiers)
    : loc(loc), kind(kind), modifiers(modifiers)
{
}

HighlightingVisitor::HighlightingVisitor(const QQmlJS::Dom::DomItem &item,
                                         const std::optional<HighlightsRange> &range)
    : m_range(range)
{
    item.visitTree(
            Path(),
            [this](const Path &path, const DomItem &item, bool b) {
                return this->visitor(path, item, b);
            },
            VisitOption::Default | VisitOption::NoPath, emptyChildrenVisitor, emptyChildrenVisitor,
            highlightingFilter());
}

bool HighlightingVisitor::visitor(Path, const DomItem &item, bool)
{
    if (m_range.has_value()) {
        const auto fLocs = FileLocations::treeOf(item);
        if (!fLocs)
            return true;
        const auto regions = fLocs->info().regions;
        if (!Utils::rangeOverlapsWithSourceLocation(regions[MainRegion],
                                                                m_range.value()))
            return true;
    }
    switch (item.internalKind()) {
    case DomType::Comment: {
        highlightComment(item);
        return true;
    }
    case DomType::Import: {
        highlightImport(item);
        return true;
    }
    case DomType::Binding: {
        highlightBinding(item);
        return true;
    }
    case DomType::Pragma: {
        highlightPragma(item);
        return true;
    }
    case DomType::EnumDecl: {
        highlightEnumDecl(item);
        return true;
    }
    case DomType::EnumItem: {
        highlightEnumItem(item);
        return true;
    }
    case DomType::QmlObject: {
        highlightQmlObject(item);
        return true;
    }
    case DomType::QmlComponent: {
        highlightComponent(item);
        return true;
    }
    case DomType::PropertyDefinition: {
        highlightPropertyDefinition(item);
        return true;
    }
    case DomType::MethodInfo: {
        highlightMethod(item);
        return true;
    }
    case DomType::ScriptLiteral: {
        highlightScriptLiteral(item);
        return true;
    }
    case DomType::ScriptCallExpression: {
        highlightCallExpression(item);
        return true;
    }
    case DomType::ScriptIdentifierExpression: {
        highlightIdentifier(item);
        return true;
    }
    default:
        if (item.ownerAs<ScriptExpression>())
            highlightScriptExpressions(item);
        return true;
    }
    Q_UNREACHABLE_RETURN(false);
}

void HighlightingVisitor::highlightComment(const DomItem &item)
{
    const auto comment = item.as<Comment>();
    Q_ASSERT(comment);
    const auto locs = Utils::sourceLocationsFromMultiLineToken(
            comment->info().comment(), comment->info().sourceLocation());
    for (const auto &loc : locs)
        addHighlight(loc, QmlHighlightKind::Comment);
}

void HighlightingVisitor::highlightImport(const DomItem &item)
{
    const auto fLocs = FileLocations::treeOf(item);
    if (!fLocs)
        return;
    const auto regions = fLocs->info().regions;
    const auto import = item.as<Import>();
    Q_ASSERT(import);
    addHighlight(regions[ImportTokenRegion], QmlHighlightKind::QmlKeyword);
    if (import->uri.isModule())
        addHighlight(regions[ImportUriRegion], QmlHighlightKind::QmlImportId);
    else
        addHighlight(regions[ImportUriRegion], QmlHighlightKind::String);
    if (regions.contains(VersionRegion))
        addHighlight(regions[VersionRegion], QmlHighlightKind::Number);
    if (regions.contains(AsTokenRegion)) {
        addHighlight(regions[AsTokenRegion], QmlHighlightKind::QmlKeyword);
        addHighlight(regions[IdNameRegion], QmlHighlightKind::QmlNamespace);
    }
}

void HighlightingVisitor::highlightBinding(const DomItem &item)
{
    const auto binding = item.as<Binding>();
    Q_ASSERT(binding);
    const auto fLocs = FileLocations::treeOf(item);
    if (!fLocs) {
        qCDebug(semanticTokens) << "Can't find the locations for" << item.internalKind();
        return;
    }
    const auto regions = fLocs->info().regions;
    // If dotted name, then defer it to be handled in ScriptIdentifierExpression
    if (binding->name().contains("."_L1))
        return;

    if (binding->bindingType() != BindingType::Normal) {
        addHighlight(regions[OnTokenRegion], QmlHighlightKind::QmlKeyword);
        addHighlight(regions[IdentifierRegion], QmlHighlightKind::QmlProperty);
        return;
    }

    return addHighlight(regions[IdentifierRegion], QmlHighlightKind::QmlProperty);
}

void HighlightingVisitor::highlightPragma(const DomItem &item)
{
    const auto fLocs = FileLocations::treeOf(item);
    if (!fLocs)
        return;
    const auto regions = fLocs->info().regions;
    addHighlight(regions[PragmaKeywordRegion], QmlHighlightKind::QmlKeyword);
    addHighlight(regions[IdentifierRegion], QmlHighlightKind::QmlPragmaName );
    const auto pragma = item.as<Pragma>();
    for (auto i = 0; i < pragma->values.size(); ++i) {
        DomItem value = item.field(Fields::values).index(i);
        const auto valueRegions = FileLocations::treeOf(value)->info().regions;
        addHighlight(valueRegions[PragmaValuesRegion], QmlHighlightKind::QmlPragmaValue);
    }
    return;
}

void HighlightingVisitor::highlightEnumDecl(const DomItem &item)
{
    const auto fLocs = FileLocations::treeOf(item);
    if (!fLocs)
        return;
    const auto regions = fLocs->info().regions;
    addHighlight(regions[EnumKeywordRegion], QmlHighlightKind::QmlKeyword);
    addHighlight(regions[IdentifierRegion], QmlHighlightKind::QmlEnumName);
}

void HighlightingVisitor::highlightEnumItem(const DomItem &item)
{
    const auto fLocs = FileLocations::treeOf(item);
    if (!fLocs)
        return;
    const auto regions = fLocs->info().regions;
    addHighlight(regions[IdentifierRegion], QmlHighlightKind::QmlEnumMember);
    if (regions.contains(EnumValueRegion))
        addHighlight(regions[EnumValueRegion], QmlHighlightKind::Number);
}

void HighlightingVisitor::highlightQmlObject(const DomItem &item)
{
    const auto qmlObject = item.as<QmlObject>();
    Q_ASSERT(qmlObject);
    const auto fLocs = FileLocations::treeOf(item);
    if (!fLocs)
        return;
    const auto regions = fLocs->info().regions;
    // Handle ids here
    if (!qmlObject->idStr().isEmpty()) {
        addHighlight(regions[IdTokenRegion], QmlHighlightKind::QmlProperty);
        addHighlight(regions[IdNameRegion], QmlHighlightKind::QmlLocalId);
    }
    // If dotted name, then defer it to be handled in ScriptIdentifierExpression
    if (qmlObject->name().contains("."_L1))
        return;

    addHighlight(regions[IdentifierRegion], QmlHighlightKind::QmlType);
}

void HighlightingVisitor::highlightComponent(const DomItem &item)
{
    const auto fLocs = FileLocations::treeOf(item);
    if (!fLocs)
        return;
    const auto regions = fLocs->info().regions;
    const auto componentKeywordIt = regions.constFind(ComponentKeywordRegion);
    if (componentKeywordIt == regions.constEnd())
        return; // not an inline component, no need for highlighting
    addHighlight(*componentKeywordIt, QmlHighlightKind::QmlKeyword);
    addHighlight(regions[IdentifierRegion], QmlHighlightKind::QmlType);
}

void HighlightingVisitor::highlightPropertyDefinition(const DomItem &item)
{
    const auto propertyDef = item.as<PropertyDefinition>();
    Q_ASSERT(propertyDef);
    const auto fLocs = FileLocations::treeOf(item);
    if (!fLocs)
        return;
    const auto regions = fLocs->info().regions;
    QmlHighlightModifiers modifier = QmlHighlightModifier::QmlPropertyDefinition;
    if (propertyDef->isDefaultMember) {
        modifier |= QmlHighlightModifier::QmlDefaultProperty;
        addHighlight(regions[DefaultKeywordRegion], QmlHighlightKind::QmlKeyword);
    }
    if (propertyDef->isVirtual) {
        modifier |= QmlHighlightModifier::QmlVirtualProperty;
        addHighlight(regions[VirtualKeywordRegion], QmlHighlightKind::QmlKeyword);
    }
    if (propertyDef->isOverride) {
        modifier |= QmlHighlightModifier::QmlOverrideProperty;
        addHighlight(regions[OverrideKeywordRegion], QmlHighlightKind::QmlKeyword);
    }
    if (propertyDef->isFinal) {
        modifier |= QmlHighlightModifier::QmlFinalProperty;
        addHighlight(regions[FinalKeywordRegion], QmlHighlightKind::QmlKeyword);
    }
    if (propertyDef->isRequired) {
        modifier |= QmlHighlightModifier::QmlRequiredProperty;
        addHighlight(regions[RequiredKeywordRegion], QmlHighlightKind::QmlKeyword);
    }
    if (propertyDef->isReadonly) {
        modifier |= QmlHighlightModifier::QmlReadonlyProperty;
        addHighlight(regions[ReadonlyKeywordRegion], QmlHighlightKind::QmlKeyword);
    }
    addHighlight(regions[PropertyKeywordRegion], QmlHighlightKind::QmlKeyword);
    if (propertyDef->isAlias())
        addHighlight(regions[TypeIdentifierRegion], QmlHighlightKind::QmlKeyword);
    else
        addHighlight(regions[TypeIdentifierRegion], QmlHighlightKind::QmlType);

    addHighlight(regions[TypeModifierRegion], QmlHighlightKind::QmlTypeModifier);
    addHighlight(regions[IdentifierRegion], QmlHighlightKind::QmlProperty,
                                modifier);
}

void HighlightingVisitor::highlightMethod(const DomItem &item)
{
    const auto method = item.as<MethodInfo>();
    Q_ASSERT(method);
    const auto fLocs = FileLocations::treeOf(item);
    if (!fLocs)
        return;
    const auto regions = fLocs->info().regions;
    switch (method->methodType) {
    case MethodInfo::Signal: {
        addHighlight(regions[SignalKeywordRegion], QmlHighlightKind::QmlKeyword);
        addHighlight(regions[IdentifierRegion], QmlHighlightKind::QmlMethod);
        break;
    }
    case MethodInfo::Method: {
        addHighlight(regions[FunctionKeywordRegion], QmlHighlightKind::QmlKeyword);
        addHighlight(regions[IdentifierRegion], QmlHighlightKind::QmlMethod);
        addHighlight(regions[TypeIdentifierRegion], QmlHighlightKind::QmlType);
        break;
    }
    default:
        Q_UNREACHABLE();
    }

    for (auto i = 0; i < method->parameters.size(); ++i) {
        DomItem parameter = item.field(Fields::parameters).index(i);
        const auto paramRegions = FileLocations::treeOf(parameter)->info().regions;
        addHighlight(paramRegions[IdentifierRegion],
                                    QmlHighlightKind::QmlMethodParameter);
        addHighlight(paramRegions[TypeIdentifierRegion], QmlHighlightKind::QmlType);
    }
    return;
}

void HighlightingVisitor::highlightScriptLiteral(const DomItem &item)
{
    const auto literal = item.as<ScriptElements::Literal>();
    Q_ASSERT(literal);
    const auto fLocs = FileLocations::treeOf(item);
    if (!fLocs)
        return;
    const auto regions = fLocs->info().regions;
    if (std::holds_alternative<QString>(literal->literalValue())) {
        const auto file = item.containingFile().as<QmlFile>();
        if (!file)
            return;
        const auto &code = file->engine()->code();
        const auto offset = regions[MainRegion].offset;
        const auto length = regions[MainRegion].length;
        const QStringView literalCode = QStringView{code}.mid(offset, length);
        const auto &locs = Utils::sourceLocationsFromMultiLineToken(
                literalCode, regions[MainRegion]);
        for (const auto &loc : locs)
            addHighlight(loc, QmlHighlightKind::String);
    } else if (std::holds_alternative<double>(literal->literalValue()))
        addHighlight(regions[MainRegion], QmlHighlightKind::Number);
    else if (std::holds_alternative<bool>(literal->literalValue()))
        addHighlight(regions[MainRegion], QmlHighlightKind::QmlKeyword);
    else if (std::holds_alternative<std::nullptr_t>(literal->literalValue()))
        addHighlight(regions[MainRegion], QmlHighlightKind::QmlKeyword);
    else
        qCWarning(semanticTokens) << "Invalid literal variant";
}

void HighlightingVisitor::highlightIdentifier(const DomItem &item)
{
    using namespace QLspSpecification;
    const auto id = item.as<ScriptElements::IdentifierExpression>();
    Q_ASSERT(id);
    const auto loc = id->mainRegionLocation();
    // Many of the scriptIdentifiers expressions are already handled by
    // other cases. In those cases, if the location offset is already in the list
    // we don't need to perform expensive resolveExpressionType operation.
    if (m_highlights.contains(loc.offset))
        return;

    // If the item is a field member base, we need to resolve the expression type
    // If the item is a field member access, we don't need to resolve the expression type
    // because it is already resolved in the first element.
    if (QQmlLSUtils::isFieldMemberAccess(item))
        highlightFieldMemberAccess(item, loc);
    else
        highlightBySemanticAnalysis(item, loc);
}

void HighlightingVisitor::highlightCallExpression(const DomItem &item)
{
    const auto highlight = [this](const DomItem &item) {
        if (item.internalKind() == DomType::ScriptIdentifierExpression) {
            const auto id = item.as<ScriptElements::IdentifierExpression>();
            Q_ASSERT(id);
            const auto loc = id->mainRegionLocation();
            addHighlight(loc, QmlHighlightKind::QmlMethod);
        }
    };

    if (item.internalKind() == DomType::ScriptCallExpression) {
        // If the item is a call expression, we need to highlight the callee.
        const auto callee = item.field(Fields::callee);
        if (callee.internalKind() == DomType::ScriptIdentifierExpression) {
            highlight(callee);
            return;
        } else if (callee.internalKind() == DomType::ScriptBinaryExpression) {
            // If the callee is a binary expression, we need to highlight the right part.
            const auto right = callee.field(Fields::right);
            if (right.internalKind() == DomType::ScriptIdentifierExpression)
                highlight(right);
            return;
        }
    }
}

void HighlightingVisitor::highlightFieldMemberAccess(const DomItem &item,
                                                     QQmlJS::SourceLocation loc)
{
    // enum fields and qualified module identifiers are not just fields. Do semantic analysis if
    // the identifier name is an uppercase string.
    const auto name = item.field(Fields::identifier).value().toString();
    if (!name.isEmpty() && name.at(0).category() == QChar::Letter_Uppercase) {
        // maybe the identifier is an attached type or enum members, use semantic analysis to figure
        // out.
        return highlightBySemanticAnalysis(item, loc);
    }
    // Check if the name is a method
    const auto expression =
            QQmlLSUtils::resolveExpressionType(item, QQmlLSUtils::ResolveOptions::ResolveOwnerType);

    if (!expression) {
        addHighlight(loc, QmlHighlightKind::Field);
        return;
    }

    if (expression->type == QQmlLSUtils::MethodIdentifier
        || expression->type == QQmlLSUtils::LambdaMethodIdentifier) {
        addHighlight(loc, QmlHighlightKind::QmlMethod);
        return;
    } else {
        return addHighlight(loc, QmlHighlightKind::Field);
    }
}

void HighlightingVisitor::highlightBySemanticAnalysis(const DomItem &item, QQmlJS::SourceLocation loc)
{
    const auto expression = QQmlLSUtils::resolveExpressionType(
            item, QQmlLSUtils::ResolveOptions::ResolveOwnerType);

    if (!expression) {
        addHighlight(loc, QmlHighlightKind::Unknown);
        return;
    }
    switch (expression->type) {
    case QQmlLSUtils::QmlComponentIdentifier:
        addHighlight(loc, QmlHighlightKind::QmlType);
        return;
    case QQmlLSUtils::JavaScriptIdentifier: {
        QmlHighlightKind tokenType = QmlHighlightKind::JsScopeVar;
        QmlHighlightModifiers modifier = QmlHighlightModifier::None;
        if (const auto scope = expression->semanticScope) {
            if (const auto jsIdentifier = scope->jsIdentifier(*expression->name)) {
                if (jsIdentifier->kind == QQmlJSScope::JavaScriptIdentifier::Parameter)
                    tokenType = QmlHighlightKind::QmlMethodParameter;
                if (jsIdentifier->isConst) {
                    modifier |= QmlHighlightModifier::QmlReadonlyProperty;
                }
                addHighlight(loc, tokenType, modifier);
                return;
            }
        }
        if (const auto name = expression->name) {
            if (const auto highlightKind = resolveJsGlobalObjectKind(item, *name))
                return addHighlight(loc, *highlightKind);
        }
        return;
    }
    case QQmlLSUtils::PropertyIdentifier: {
        if (const auto scope = expression->semanticScope) {
            QmlHighlightKind tokenType = QmlHighlightKind::QmlProperty;
            if (scope == item.qmlObject().semanticScope()) {
                tokenType = QmlHighlightKind::QmlScopeObjectProperty;
            } else if (scope == item.rootQmlObject(GoTo::MostLikely).semanticScope()) {
                tokenType = QmlHighlightKind::QmlRootObjectProperty;
            } else {
                tokenType = QmlHighlightKind::QmlExternalObjectProperty;
            }
            const auto property = scope->property(expression->name.value());
            QmlHighlightModifiers modifier = QmlHighlightModifier::None;
            if (!property.isWritable())
                modifier |= QmlHighlightModifier::QmlReadonlyProperty;
            addHighlight(loc, tokenType, modifier);
        }
        return;
    }
    case QQmlLSUtils::PropertyChangedSignalIdentifier:
        addHighlight(loc, QmlHighlightKind::QmlSignal);
        return;
    case QQmlLSUtils::PropertyChangedHandlerIdentifier:
        addHighlight(loc, QmlHighlightKind::QmlSignalHandler);
        return;
    case QQmlLSUtils::SignalIdentifier:
        addHighlight(loc, QmlHighlightKind::QmlSignal);
        return;
    case QQmlLSUtils::SignalHandlerIdentifier:
        addHighlight(loc, QmlHighlightKind::QmlSignalHandler);
        return;
    case QQmlLSUtils::MethodIdentifier:
        addHighlight(loc, QmlHighlightKind::QmlMethod);
        return;
    case QQmlLSUtils::QmlObjectIdIdentifier: {
        if (!expression->semanticScope) {
            // In PropertyChanges and friends, this id looks like a generalized grouped property but
            // is actually custom parsed, so don't highlight it.
            addHighlight(loc, QmlHighlightKind::Unknown);
            return;
        }
        const auto qmlfile = item.fileObject().as<QmlFile>();
        if (!qmlfile) {
            addHighlight(loc, QmlHighlightKind::Unknown);
            return;
        }
        const auto resolver = qmlfile->typeResolver();
        if (!resolver) {
            addHighlight(loc, QmlHighlightKind::Unknown);
            return;
        }
        const auto &objects = resolver->objectsById();
        if (expression->name.has_value()) {
            const auto &name = expression->name.value();
            const auto boundName =
                    objects.id(expression->semanticScope, item.qmlObject().semanticScope());
            if (!boundName.isEmpty() && name == boundName) {
                // If the name is the same as the bound name, then it is a local id.
                addHighlight(loc, QmlHighlightKind::QmlLocalId);
                return;
            } else {
                addHighlight(loc, QmlHighlightKind::QmlExternalId);
                return;
            }
        } else {
            addHighlight(loc, QmlHighlightKind::QmlExternalId);
            return;
        }
    }
    case QQmlLSUtils::SingletonIdentifier:
        addHighlight(loc, QmlHighlightKind::QmlType);
        return;
    case QQmlLSUtils::EnumeratorIdentifier:
        addHighlight(loc, QmlHighlightKind::QmlEnumName);
        return;
    case QQmlLSUtils::EnumeratorValueIdentifier:
        addHighlight(loc, QmlHighlightKind::QmlEnumMember);
        return;
    case QQmlLSUtils::AttachedTypeIdentifier:
    case QQmlLSUtils::AttachedTypeIdentifierInBindingTarget:
        addHighlight(loc, QmlHighlightKind::QmlType);
        return;
    case QQmlLSUtils::GroupedPropertyIdentifier:
        addHighlight(loc, QmlHighlightKind::QmlProperty);
        return;
    case QQmlLSUtils::QualifiedModuleIdentifier:
        addHighlight(loc, QmlHighlightKind::QmlNamespace);
        return;
    default:
        qCWarning(semanticTokens)
                << QString::fromLatin1("Semantic token for %1 has not been implemented yet")
                            .arg(int(expression->type));
    }
}

void HighlightingVisitor::highlightScriptExpressions(const DomItem &item)
{
    const auto fLocs = FileLocations::treeOf(item);
    if (!fLocs)
        return;
    const auto regions = fLocs->info().regions;
    switch (item.internalKind()) {
    case DomType::ScriptLiteral:
        highlightScriptLiteral(item);
        return;
    case DomType::ScriptForStatement:
        addHighlight(regions[ForKeywordRegion], QmlHighlightKind::QmlKeyword);
        addHighlight(regions[TypeIdentifierRegion],
                                    QmlHighlightKind::QmlKeyword);
        return;

    case DomType::ScriptVariableDeclaration: {
        addHighlight(regions[TypeIdentifierRegion],
                                   QmlHighlightKind::QmlKeyword);
        return;
    }
    case DomType::ScriptReturnStatement:
        addHighlight(regions[ReturnKeywordRegion], QmlHighlightKind::QmlKeyword);
        return;
    case DomType::ScriptCaseClause:
        addHighlight(regions[CaseKeywordRegion], QmlHighlightKind::QmlKeyword);
        return;
    case DomType::ScriptDefaultClause:
        addHighlight(regions[DefaultKeywordRegion], QmlHighlightKind::QmlKeyword);
        return;
    case DomType::ScriptSwitchStatement:
        addHighlight(regions[SwitchKeywordRegion], QmlHighlightKind::QmlKeyword);
        return;
    case DomType::ScriptWhileStatement:
        addHighlight(regions[WhileKeywordRegion], QmlHighlightKind::QmlKeyword);
        return;
    case DomType::ScriptDoWhileStatement:
        addHighlight(regions[DoKeywordRegion], QmlHighlightKind::QmlKeyword);
        addHighlight(regions[WhileKeywordRegion], QmlHighlightKind::QmlKeyword);
        return;
    case DomType::ScriptTryCatchStatement:
        addHighlight(regions[TryKeywordRegion], QmlHighlightKind::QmlKeyword);
        addHighlight(regions[CatchKeywordRegion], QmlHighlightKind::QmlKeyword);
        addHighlight(regions[FinallyKeywordRegion], QmlHighlightKind::QmlKeyword);
        return;
    case DomType::ScriptForEachStatement:
        addHighlight(regions[TypeIdentifierRegion], QmlHighlightKind::QmlKeyword);
        addHighlight(regions[ForKeywordRegion], QmlHighlightKind::QmlKeyword);
        addHighlight(regions[InOfTokenRegion], QmlHighlightKind::QmlKeyword);
        return;
    case DomType::ScriptThrowStatement:
        addHighlight(regions[ThrowKeywordRegion], QmlHighlightKind::QmlKeyword);
        return;
    case DomType::ScriptBreakStatement:
        addHighlight(regions[BreakKeywordRegion], QmlHighlightKind::QmlKeyword);
        return;
    case DomType::ScriptContinueStatement:
        addHighlight(regions[ContinueKeywordRegion], QmlHighlightKind::QmlKeyword);
        return;
    case DomType::ScriptIfStatement:
        addHighlight(regions[IfKeywordRegion], QmlHighlightKind::QmlKeyword);
        addHighlight(regions[ElseKeywordRegion], QmlHighlightKind::QmlKeyword);
        return;
    case DomType::ScriptLabelledStatement:
        addHighlight(regions[IdentifierRegion], QmlHighlightKind::JsLabel);
        return;
    case DomType::ScriptConditionalExpression:
        addHighlight(regions[QuestionMarkTokenRegion], QmlHighlightKind::Operator);
        addHighlight(regions[ColonTokenRegion], QmlHighlightKind::Operator);
        return;
    case DomType::ScriptUnaryExpression:
    case DomType::ScriptPostExpression:
        addHighlight(regions[OperatorTokenRegion], QmlHighlightKind::Operator);
        return;
    case DomType::ScriptType:
        addHighlight(regions[IdentifierRegion], QmlHighlightKind::QmlType);
        addHighlight(regions[TypeIdentifierRegion], QmlHighlightKind::QmlType);
        return;
    case DomType::ScriptFunctionExpression: {
        addHighlight(regions[FunctionKeywordRegion], QmlHighlightKind::QmlKeyword);
        addHighlight(regions[IdentifierRegion], QmlHighlightKind::QmlMethod);
        return;
    }
    case DomType::ScriptYieldExpression:
        addHighlight(regions[YieldKeywordRegion], QmlHighlightKind::QmlKeyword);
        return;
    case DomType::ScriptThisExpression:
        addHighlight(regions[ThisKeywordRegion], QmlHighlightKind::QmlKeyword);
        return;
    case DomType::ScriptSuperLiteral:
        addHighlight(regions[SuperKeywordRegion], QmlHighlightKind::QmlKeyword);
        return;
    case DomType::ScriptNewMemberExpression:
    case DomType::ScriptNewExpression:
        addHighlight(regions[NewKeywordRegion], QmlHighlightKind::QmlKeyword);
        return;
    case DomType::ScriptTemplateExpressionPart:
        addHighlight(regions[DollarLeftBraceTokenRegion], QmlHighlightKind::Operator);
        visitor(Path(), item.field(Fields::expression), false);
        addHighlight(regions[RightBraceRegion], QmlHighlightKind::Operator);
        return;
    case DomType::ScriptTemplateLiteral:
        addHighlight(regions[LeftBacktickTokenRegion], QmlHighlightKind::String);
        addHighlight(regions[RightBacktickTokenRegion], QmlHighlightKind::String);
        return;
    case DomType::ScriptTemplateStringPart: {
        // handle multiline case
        QString code = item.field(Fields::value).value().toString();
        const auto &locs = Utils::sourceLocationsFromMultiLineToken(
            code, regions[MainRegion]);
        for (const auto &loc : locs)
            addHighlight(loc, QmlHighlightKind::String);
        return;
    }
    default:
        qCDebug(semanticTokens) << "Script Expressions with kind" << item.internalKind()
                                << "not implemented";
    }
}

void HighlightingVisitor::addHighlight(const QQmlJS::SourceLocation &loc, QmlHighlightKind highlightKind,
                              QmlHighlightModifiers modifierKind)
{
    return Utils::addHighlight(m_highlights, loc, highlightKind, modifierKind);
}

// A single physical line's worth of content within a multiline span, as found by
// splitIntoLineSegments(): offset is absolute (into the QStringView that was split), and length
// excludes whatever line-break characters follow it.
struct LineSegment
{
    qsizetype offset;
    qsizetype length;
};

// Splits [start, end) of `text` into per-physical-line segments, so a construct spanning
// multiple lines (a comment, a string literal, a template literal's literal portions, ...) can
// be reported one token per line. Treats both "\n" and "\r\n" as line breaks, excluding the break
// itself from the segment. The final (partial) line is included unless the span ends exactly on
// a line break, with nothing following it.
static std::vector<LineSegment> splitIntoLineSegments(QStringView text, qsizetype start,
                                                      qsizetype end)
{
    std::vector<LineSegment> segments;
    qsizetype segmentStart = start;
    qsizetype pos = text.indexOf(u'\n', start);
    while (pos != -1 && pos < end) {
        qsizetype contentEnd = pos;
        if (contentEnd > start + 1 && text[contentEnd - 1] == u'\r')
            --contentEnd;
        segments.push_back({ segmentStart, contentEnd - segmentStart });
        segmentStart = pos + 1;
        pos = text.indexOf(u'\n', segmentStart);
    }
    // Push the last line
    if (segmentStart < end)
        segments.push_back({ segmentStart, end - segmentStart });
    return segments;
}

/*!
\internal
\brief Returns multiple source locations for a given raw comment

Needed by semantic highlighting of comments. LSP clients usually don't support multiline
tokens. In QML, we can have multiline tokens like string literals and comments.
This method generates multiple source locations of sub-elements of token split by a newline
delimiter.
*/
QList<QQmlJS::SourceLocation>
Utils::sourceLocationsFromMultiLineToken(QStringView stringLiteral,
                                         const QQmlJS::SourceLocation &locationInDocument)
{
    QList<QQmlJS::SourceLocation> result;
    // First token location should start from the "stringLiteral"'s
    // location in the qml document.
    QQmlJS::SourceLocation lineLoc = locationInDocument;
    for (const auto &segment : splitIntoLineSegments(stringLiteral, 0, stringLiteral.size())) {
        lineLoc.offset = locationInDocument.offset + quint32(segment.offset);
        lineLoc.length = quint32(segment.length);
        result.push_back(lineLoc);

        ++lineLoc.startLine;
        lineLoc.startColumn = 1;
    }
    return result;
}

QList<unsigned> Utils::encodeSemanticTokens(const HighlightsContainer &highlights,
                                            HighlightingMode mode)
{
    QList<unsigned> result;
    constexpr auto tokenEncodingLength = 5;
    result.reserve(tokenEncodingLength * highlights.size());

    unsigned prevLine = 0;
    unsigned prevColumn = 0;
    const auto m_mapToProtocol = mode == HighlightingMode::Default
                                     ? mapToProtocolDefault
                                     : mapToProtocolForQtCreator;
    std::for_each(highlights.constBegin(), highlights.constEnd(), [&](const auto &token) {
        unsigned length = token.loc.length;
        unsigned line = token.loc.startLine - 1u; // protocol is 0-based
        unsigned col = token.loc.startColumn - 1u; // protocol is 0-based
        Q_ASSERT(line >= prevLine);
        if (line != prevLine)
            prevColumn = 0;
        result.emplace_back(line - prevLine);
        result.emplace_back(col - prevColumn);
        result.emplace_back(length);
        result.emplace_back(m_mapToProtocol(token.kind));
        result.emplace_back(fromQmlModifierKindToLspTokenType(token.modifiers));
        prevLine = line;
        prevColumn = col;
    });

    return result;
}

/*!
\internal
Computes the modifier value. Modifier is read as binary value in the protocol. The location
of the bits set are interpreted as the indices of the tokenModifiers list registered by the
server. Then, the client modifies the highlighting of the token.

tokenModifiersList: ["declaration", definition, readonly, static ,,,]

To set "definition" and "readonly", we need to send 0b00000110
*/
void Utils::addModifier(SemanticTokenModifiers modifier, int *baseModifier)
{
    if (!baseModifier)
        return;
    *baseModifier |= (1 << int(modifier));
}

/*!
\internal
Check if the ranges overlap by ensuring that one range starts before the other ends
*/
bool Utils::rangeOverlapsWithSourceLocation(const QQmlJS::SourceLocation &loc,
                                                        const HighlightsRange &r)
{
    int startOffsetItem = int(loc.offset);
    int endOffsetItem = startOffsetItem + int(loc.length);
    return (startOffsetItem <= r.endOffset) && (r.startOffset <= endOffsetItem);
}

/*
\internal
Increments the resultID by one.
*/
void Utils::updateResultID(QByteArray &resultID)
{
    int length = resultID.length();
    for (int i = length - 1; i >= 0; --i) {
        if (resultID[i] == '9') {
            resultID[i] = '0';
        } else {
            resultID[i] = resultID[i] + 1;
            return;
        }
    }
    resultID.prepend('1');
}

/*
\internal
A utility method that computes the difference of two list. The first argument is the encoded token data
of the file before edited. The second argument is the encoded token data after the file is edited. Returns
a list of SemanticTokensEdit as expected by the protocol.
*/
QList<SemanticTokensEdit> Utils::computeDiff(const QList<unsigned> &oldData,
                                             const QList<unsigned> &newData)
{
    // Find the iterators pointing the first mismatch, from the start
    const auto [oldStart, newStart] =
            std::mismatch(oldData.cbegin(), oldData.cend(), newData.cbegin(), newData.cend());

    // Find the iterators pointing the first mismatch, from the end
    // but the iterators shouldn't pass over the start iterators found above.
    const auto [r1, r2] = std::mismatch(oldData.crbegin(), std::make_reverse_iterator(oldStart),
                                        newData.crbegin(), std::make_reverse_iterator(newStart));
    const auto oldEnd = r1.base();
    const auto newEnd = r2.base();

    // no change
    if (oldStart == oldEnd && newStart == newEnd)
        return {};

    SemanticTokensEdit edit;
    edit.start = int(std::distance(newData.cbegin(), newStart));
    edit.deleteCount = int(std::distance(oldStart, oldEnd));

    if (newStart >= newData.cbegin() && newEnd <= newData.cend() && newStart < newEnd)
        edit.data.emplace(newStart, newEnd);

    return { std::move(edit) };
}

void Utils::addHighlight(HighlightsContainer &out,
                         const QQmlJS::SourceLocation &loc,
                         QmlHighlightKind highlightKind,
                         QmlHighlightModifiers modifierKind)
{
    if (!loc.isValid() || loc.length == 0) {
        qCDebug(semanticTokens)
            << "Invalid locations: Cannot add highlight to token";
        return;
    }
    if (!out.contains(loc.offset))
        out.insert(loc.offset, HighlightToken(loc, highlightKind, modifierKind));
}

HighlightsContainer Utils::visitTokens(const QQmlJS::Dom::DomItem &item,
                                     const std::optional<HighlightsRange> &range)
{
    using namespace QQmlJS::Dom;
    HighlightingVisitor highlightDomElements(item, range);
    return highlightDomElements.highlights();
}

HighlightsContainer Utils::shiftHighlights(const HighlightsContainer &cachedHighlights,
                                           const QString &lastValidCode, const QString &currentCode)
{
    using namespace QQmlLSUtils;
    Differ differ;
    const QList<Diff> diffs = differ.diff(lastValidCode, currentCode);
    HighlightsContainer shifts = cachedHighlights;
    applyDiffs(shifts, diffs);
    return shifts;
}

namespace {

struct LineAnchor
{
    qsizetype offset = 0;
    quint32 number = 1;
};

struct FallbackRule
{
    QRegularExpression regex;
    void (*action)(HighlightsContainer &, const QRegularExpressionMatch &, LineAnchor line);
};

static void addFallbackToken(HighlightsContainer &out, LineAnchor line, qsizetype offset,
                             qsizetype length, QmlHighlightKind kind,
                             QmlHighlightModifiers modifiers = QmlHighlightModifier::None)
{
    Q_ASSERT(length > 0);
    const auto loc = QQmlJS::SourceLocation::fromQSizeType(offset, length, line.number,
                                                           offset - line.offset + 1);
    Utils::addHighlight(out, loc, kind, modifiers);
}

static void addGroup(HighlightsContainer &out, const QRegularExpressionMatch &m, int group,
                     LineAnchor line, QmlHighlightKind kind,
                     QmlHighlightModifiers modifiers = QmlHighlightModifier::None)
{
    if (!m.hasCaptured(group))
        return;
    addFallbackToken(out, line, line.offset + m.capturedStart(group), m.capturedLength(group), kind,
                     modifiers);
}

static void highlightSignalParameters(HighlightsContainer &out, const QRegularExpressionMatch &m,
                                      LineAnchor line)
{
    static const QRegularExpression paramRe(
            uR"re((?:([\w.<>]+)\s+(\w+))|(?:(\w+)\s*:\s*([\w.<>]+)))re"_s);
    const qsizetype paramsStart = m.capturedStart(3);
    Q_ASSERT(paramsStart >= 0);
    auto it = paramRe.globalMatch(m.capturedView(3));
    while (it.hasNext()) {
        const auto pm = it.next();
        if (pm.capturedStart(1) >= 0) {
            addFallbackToken(out, line, line.offset + paramsStart + pm.capturedStart(1),
                             pm.capturedLength(1), QmlHighlightKind::QmlType);
            addFallbackToken(out, line, line.offset + paramsStart + pm.capturedStart(2),
                             pm.capturedLength(2), QmlHighlightKind::QmlMethodParameter);
        } else {
            addFallbackToken(out, line, line.offset + paramsStart + pm.capturedStart(3),
                             pm.capturedLength(3), QmlHighlightKind::QmlMethodParameter);
            addFallbackToken(out, line, line.offset + paramsStart + pm.capturedStart(4),
                             pm.capturedLength(4), QmlHighlightKind::QmlType);
        }
    }
}

static QmlHighlightModifiers propertyModifiers(QStringView modifierWord)
{
    QmlHighlightModifiers modifiers = QmlHighlightModifier::QmlPropertyDefinition;
    if (modifierWord == u"readonly")
        modifiers |= QmlHighlightModifier::QmlReadonlyProperty;
    else if (modifierWord == u"required")
        modifiers |= QmlHighlightModifier::QmlRequiredProperty;
    else if (modifierWord == u"default")
        modifiers |= QmlHighlightModifier::QmlDefaultProperty;
    else if (modifierWord == u"final")
        modifiers |= QmlHighlightModifier::QmlFinalProperty;
    else if (modifierWord == u"virtual")
        modifiers |= QmlHighlightModifier::QmlVirtualProperty;
    else if (modifierWord == u"override")
        modifiers |= QmlHighlightModifier::QmlOverrideProperty;
    return modifiers;
}

// Rules are tried in order at every position; the earliest match in the line wins, and ties are
// broken by priority (the rule listed first). This lets specific QML constructs (property
// definitions, signals, imports, ...) take precedence over the generic keyword/identifier rules
// that follow them.
const QList<FallbackRule> &fallbackRules()
{
    static const QList<FallbackRule> rules = [] {
        QList<FallbackRule> r;
        // Line comments.
        r.append({ QRegularExpression(uR"re(//.*)re"_s),
                   [](HighlightsContainer &out, const QRegularExpressionMatch &m, LineAnchor line) {
                       addGroup(out, m, 0, line, QmlHighlightKind::Comment);
                   } });
        // Single-line strings.
        r.append({ QRegularExpression(uR"re("(?:[^"\\]|\\.)*"|'(?:[^'\\]|\\.)*')re"_s),
                   [](HighlightsContainer &out, const QRegularExpressionMatch &m, LineAnchor line) {
                       addGroup(out, m, 0, line, QmlHighlightKind::String);
                   } });

        // import <ModuleOrPath> [version] [as Alias]
        r.append(
                { QRegularExpression(
                          uR"re(\b(import)\b\s+(?:("(?:[^"\\]|\\.)*")|([\w.]+))(?:\s+(\d+\.\d+))?(?:\s+(as)\s+(\w+))?)re"_s),
                  [](HighlightsContainer &out, const QRegularExpressionMatch &m, LineAnchor line) {
                      addGroup(out, m, 1, line, QmlHighlightKind::QmlKeyword);
                      addGroup(out, m, 2, line, QmlHighlightKind::String);
                      addGroup(out, m, 3, line, QmlHighlightKind::QmlImportId);
                      addGroup(out, m, 4, line, QmlHighlightKind::Number);
                      addGroup(out, m, 5, line, QmlHighlightKind::QmlKeyword);
                      addGroup(out, m, 6, line, QmlHighlightKind::QmlNamespace);
                  } });
        // [default|readonly|required|final|virtual|override] property <type|alias> <name>
        r.append(
                { QRegularExpression(
                          uR"re(\b(?:(default|readonly|required|final|virtual|override)\s+)*(property)\s+(alias|[A-Za-z_][\w.<>]*)\s+(\w+))re"_s),
                  [](HighlightsContainer &out, const QRegularExpressionMatch &m, LineAnchor line) {
                      addGroup(out, m, 1, line, QmlHighlightKind::QmlKeyword);
                      addGroup(out, m, 2, line, QmlHighlightKind::QmlKeyword);
                      const QStringView type = m.capturedView(3);
                      addGroup(out, m, 3, line,
                               type == u"alias" ? QmlHighlightKind::QmlKeyword
                                                : QmlHighlightKind::QmlType);
                      addGroup(out, m, 4, line, QmlHighlightKind::QmlProperty,
                               propertyModifiers(m.capturedView(1)));
                  } });
        // required <name>
        r.append({ QRegularExpression(uR"re((?:^|[{;])\s*(required)\s+(?!property\b)(\w+))re"_s),
                   [](HighlightsContainer &out, const QRegularExpressionMatch &m, LineAnchor line) {
                       addGroup(out, m, 1, line, QmlHighlightKind::QmlKeyword);
                       addGroup(out, m, 2, line, QmlHighlightKind::QmlProperty,
                                QmlHighlightModifier::QmlRequiredProperty);
                   } });
        // signal <name>(<type name>, ...)
        r.append({ QRegularExpression(uR"re(\b(signal)\s+(\w+)\s*\(([^)]*)\))re"_s),
                   [](HighlightsContainer &out, const QRegularExpressionMatch &m, LineAnchor line) {
                       addGroup(out, m, 1, line, QmlHighlightKind::QmlKeyword);
                       addGroup(out, m, 2, line, QmlHighlightKind::QmlSignal);
                       highlightSignalParameters(out, m, line);
                   } });
        // function <name>(
        r.append({ QRegularExpression(uR"re(\b(function)\s+(\w+)\s*(?=\())re"_s),
                   [](HighlightsContainer &out, const QRegularExpressionMatch &m, LineAnchor line) {
                       addGroup(out, m, 1, line, QmlHighlightKind::QmlKeyword);
                       addGroup(out, m, 2, line, QmlHighlightKind::QmlMethod);
                   } });
        // id: <identifier>
        r.append({ QRegularExpression(uR"re((?:^|[{;])\s*(id)\s*:\s*(\w+))re"_s),
                   [](HighlightsContainer &out, const QRegularExpressionMatch &m, LineAnchor line) {
                       addGroup(out, m, 1, line, QmlHighlightKind::QmlKeyword);
                       addGroup(out, m, 2, line, QmlHighlightKind::QmlLocalId);
                   } });
        // onSomething: <handler> signal handler properties.
        r.append({ QRegularExpression(uR"re(\bon[A-Z]\w*(?=\s*:))re"_s),
                   [](HighlightsContainer &out, const QRegularExpressionMatch &m, LineAnchor line) {
                       addGroup(out, m, 0, line, QmlHighlightKind::QmlSignalHandler);
                   } });
        // pragma <Name> [Value]
        r.append({ QRegularExpression(uR"re((?:^|[{;])\s*(pragma)\s+(\w+)(?:\s+(\w+))?)re"_s),
                   [](HighlightsContainer &out, const QRegularExpressionMatch &m, LineAnchor line) {
                       addGroup(out, m, 1, line, QmlHighlightKind::QmlKeyword);
                       addGroup(out, m, 2, line, QmlHighlightKind::QmlPragmaName);
                       addGroup(out, m, 3, line, QmlHighlightKind::QmlPragmaValue);
                   } });
        // <name>: property/binding assignment.
        r.append({ QRegularExpression(uR"re((?:^|[{;])\s*([a-z_]\w*(?:\.[a-z_]\w*)*)\s*(?=:))re"_s),
                   [](HighlightsContainer &out, const QRegularExpressionMatch &m, LineAnchor line) {
                       addGroup(out, m, 1, line, QmlHighlightKind::QmlProperty);
                   } });
        // QML/JS keywords.
        r.append(
                { QRegularExpression(
                          uR"re(\b(?:as|async|await|break|case|catch|class|component|const|continue|
                                   debugger|default|delete|do|else|enum|export|extends|false|final|finally|
                                   for|function|id|if|import|in|instanceof|let|new|null|on|override|pragma|
                                   property|readonly|required|return|signal|super|switch|this|throw|true|
                                   try|typeof|undefined|var|virtual|void|while|with|yield)\b)re"_s),
                  [](HighlightsContainer &out, const QRegularExpressionMatch &m, LineAnchor line) {
                      addGroup(out, m, 0, line, QmlHighlightKind::QmlKeyword);
                  } });
        // Numbers.
        r.append({ QRegularExpression(uR"re(\b\d+(?:\.\d+)?(?:[eE][+-]?\d+)?\b)re"_s),
                   [](HighlightsContainer &out, const QRegularExpressionMatch &m, LineAnchor line) {
                       addGroup(out, m, 0, line, QmlHighlightKind::Number);
                   } });
        // Property/enum access after a dot, e.g. mouse.x or Text.AlignHCenter.
        r.append({ QRegularExpression(uR"re((?<=\.)\w+)re"_s),
                   [](HighlightsContainer &out, const QRegularExpressionMatch &m, LineAnchor line) {
                       addGroup(out, m, 0, line, QmlHighlightKind::Field);
                   } });
        // Capitalized identifiers are QML type names by convention.
        r.append({ QRegularExpression(uR"re(\b[A-Z]\w*\b)re"_s),
                   [](HighlightsContainer &out, const QRegularExpressionMatch &m, LineAnchor line) {
                       addGroup(out, m, 0, line, QmlHighlightKind::QmlType);
                   } });
        // Function call sites, including the tag of a tagged template literal (tag`...`).
        r.append({ QRegularExpression(uR"re(\b[A-Za-z_]\w*(?=\s*[(`]))re"_s),
                   [](HighlightsContainer &out, const QRegularExpressionMatch &m, LineAnchor line) {
                       addGroup(out, m, 0, line, QmlHighlightKind::QmlMethod);
                   } });
        return r;
    }();
    return rules;
}

// Emits one token per physical line for the span [start, end), so a construct that runs across
// a line break (a block comment, or the literal text portions of a template literal) is still
// reported as one token per line, matching how every other multi-line construct is split.
static void addMultilineToken(HighlightsContainer &out, QStringView text, qsizetype start,
                              qsizetype end, LineAnchor &line, QmlHighlightKind kind)
{
    const auto segments = splitIntoLineSegments(text, start, end);
    for (size_t i = 0; i < segments.size(); ++i) {
        if (segments[i].length > 0)
            addFallbackToken(out, line, segments[i].offset, segments[i].length, kind);
        if (i + 1 < segments.size()) {
            ++line.number;
            line.offset = segments[i + 1].offset;
        }
    }
    if (!segments.empty() && segments.back().offset + segments.back().length < end) {
        // the span ends exactly on a line break, with nothing following it on that new line
        ++line.number;
        line.offset = end;
    }
}

// Highlights a "/* ... */" block comment starting at the opening slash. May span multiple
// lines; returns the offset just past the closing "*/" (or text.size() if left unterminated).
static qsizetype scanBlockComment(HighlightsContainer &out, QStringView text, qsizetype pos,
                                  LineAnchor &line)
{
    const qsizetype closeIdx = text.indexOf(u"*/", pos + 2);
    const qsizetype end = closeIdx == -1 ? text.size() : closeIdx + 2;
    addMultilineToken(out, text, pos, end, line, QmlHighlightKind::Comment);
    return end;
}

// Forward declared: scanTemplateLiteral() recurses into this to highlight the contents of a
// `${ expr }` interpolation
static qsizetype scanSpan(HighlightsContainer &out, QStringView text, qsizetype pos,
                          LineAnchor &line, bool stopAtUnbalancedBrace);

// Highlights a "`...`" template literal starting at the opening backtick, including any
// "${ expr }" interpolations, which may themselves span lines or contain further, nested
// template literals. Returns the offset just past the closing backtick (or text.size() if the
// literal is left unterminated).
static qsizetype scanTemplateLiteral(HighlightsContainer &out, QStringView text, qsizetype pos,
                                     LineAnchor &line)
{
    const qsizetype size = text.size();
    qsizetype segmentStart = pos;
    ++pos; // step over the opening backtick
    while (pos < size) {
        const QChar c = text[pos];
        if (c == u'\\') {
            pos += 2; // an escaped character (e.g. \` or \\) never terminates the literal
            continue;
        }
        if (c == u'`') {
            ++pos;
            addMultilineToken(out, text, segmentStart, pos, line, QmlHighlightKind::String);
            return pos;
        }
        if (c == u'$' && pos + 1 < size && text[pos + 1] == u'{') {
            addMultilineToken(out, text, segmentStart, pos, line, QmlHighlightKind::String);
            addFallbackToken(out, line, pos, 2, QmlHighlightKind::Operator);
            pos = scanSpan(out, text, pos + 2, line, /* stopAtUnbalancedBrace = */ true);
            if (pos < size) {
                addFallbackToken(out, line, pos, 1, QmlHighlightKind::Operator);
                ++pos;
            }
            segmentStart = pos;
            continue;
        }
        ++pos;
    }
    // Unterminated: whatever is left is highlighted as string content.
    addMultilineToken(out, text, segmentStart, size, line, QmlHighlightKind::String);
    return size;
}

enum class SpecialChar { None, BlockComment, TemplateLiteral, OpenBrace, CloseBrace };
struct SpecialCharMatch
{
    qsizetype col = -1;
    SpecialChar kind = SpecialChar::None;
};

static SpecialCharMatch nextSpecialChar(QStringView lineView, qsizetype col, bool trackBraces)
{
    for (qsizetype i = col; i < lineView.size(); ++i) {
        const QChar c = lineView[i];
        if (c == u'`')
            return { i, SpecialChar::TemplateLiteral };
        if (c == u'/' && i + 1 < lineView.size() && lineView[i + 1] == u'*')
            return { i, SpecialChar::BlockComment };
        if (trackBraces && c == u'{')
            return { i, SpecialChar::OpenBrace };
        if (trackBraces && c == u'}')
            return { i, SpecialChar::CloseBrace };
    }
    return {};
}

// Applies the fallback rule table to `text`, starting at `pos`.
// When stopAtUnbalancedBrace is true (used to scan the contents of a `${ expr }`
// interpolation), scanning stops at the first "}" that doesn't close a "{" seen since `pos`,
// returning its offset without consuming it. Returns the offset scanning stopped at (text.size()
// for a top-level/unbounded scan that ran off the end).
static qsizetype scanSpan(HighlightsContainer &out, QStringView text, qsizetype pos,
                          LineAnchor &line, bool stopAtUnbalancedBrace)
{
    const qsizetype size = text.size();
    const QList<FallbackRule> &rules = fallbackRules();
    int braceDepth = 0;

    while (pos <= size) {
        qsizetype physLineEnd = text.indexOf(u'\n', pos);
        if (physLineEnd == -1)
            physLineEnd = size;
        const QStringView lineView = text.mid(line.offset, physLineEnd - line.offset);
        const qsizetype col = pos - line.offset;

        const FallbackRule *bestRule = nullptr;
        QRegularExpressionMatch bestMatch;
        for (const auto &rule : rules) {
            const auto m = rule.regex.matchView(lineView, col);
            if (!m.hasMatch())
                continue;
            if (!bestRule || m.capturedStart(0) < bestMatch.capturedStart(0)) {
                bestRule = &rule;
                bestMatch = m;
            }
        }

        const SpecialCharMatch special = nextSpecialChar(lineView, col, stopAtUnbalancedBrace);
        const bool ruleWins = bestRule
                && (special.kind == SpecialChar::None || bestMatch.capturedStart(0) <= special.col);

        if (!bestRule && special.kind == SpecialChar::None) {
            if (physLineEnd >= size)
                return size;
            line.offset = physLineEnd + 1;
            pos = line.offset;
            ++line.number;
            continue;
        }

        if (ruleWins) {
            bestRule->action(out, bestMatch, line);
            pos = line.offset + qMax(bestMatch.capturedEnd(0), col + 1);
            continue;
        }

        const qsizetype absPos = line.offset + special.col;
        switch (special.kind) {
        case SpecialChar::BlockComment:
            pos = scanBlockComment(out, text, absPos, line);
            break;
        case SpecialChar::TemplateLiteral:
            pos = scanTemplateLiteral(out, text, absPos, line);
            break;
        case SpecialChar::OpenBrace:
            ++braceDepth;
            pos = absPos + 1;
            break;
        case SpecialChar::CloseBrace:
            if (braceDepth == 0)
                return absPos;
            --braceDepth;
            pos = absPos + 1;
            break;
        case SpecialChar::None:
            Q_UNREACHABLE();
        }
    }
    return pos;
}

} // namespace

// Highlighting for code that cannot be parsed into a DomItem at all
// Scans the document against a table of QML/JS regular expressions. Requires no
// semantic information, so it degrades gracefully on incomplete or invalid QML/JS.
HighlightsContainer Utils::regexFallbackHighlights(QStringView code,
                                                   const std::optional<HighlightsRange> &range)
{
    HighlightsContainer highlights;
    LineAnchor line;
    scanSpan(highlights, code, 0, line, /* stopAtUnbalancedBrace = */ false);

    if (range) {
        for (auto it = highlights.begin(); it != highlights.end();) {
            if (!rangeOverlapsWithSourceLocation(it->loc, *range))
                it = highlights.erase(it);
            else
                ++it;
        }
    }

    return highlights;
}

static std::pair<quint32, quint32> newlineCountAndLastLineLength(const QString &text)
{
    auto [row, col]  = QQmlJS::SourceLocation::rowAndColumnFrom(text, text.size());
    return { row - 1, col - 1 }; // rows are 1-based, so subtract 1 to get the number of newlines
}

static void updateCursorPositionByDiff(const QString &text, QQmlJS::SourceLocation &cursor)
{
    auto [newLines, lastLineLength] = newlineCountAndLastLineLength(text);
    if (newLines > 0) {
        cursor.startLine += newLines;
        cursor.startColumn = lastLineLength + 1; // +1 because columns are 1-based
    } else {
        cursor.startColumn += text.size();
    }
    cursor.offset += text.size();
};

//
// Utilities for insertion handling
//

static bool tokenBeforeOffset(const QQmlJS::SourceLocation &t, quint32 offset)
{
    return t.end() < offset;
}

static bool tokenAfterOffset(const QQmlJS::SourceLocation &t, quint32 offset)
{
    return t.begin() > offset;
}

static bool insertionInsideToken(const QQmlJS::SourceLocation &token,
                                 const QQmlJS::SourceLocation &cursor)
{
    return token.begin() < cursor.begin() && token.end() >= cursor.begin();
}

static bool insertionTouchesTokenLeft(const QQmlJS::SourceLocation &token,
                                      const QQmlJS::SourceLocation &cursor)
{
    return token.begin() >= cursor.begin() && token.begin() <= cursor.end();
}

static void shiftTokenAfterInsert(QQmlJS::SourceLocation &t, const QQmlJS::SourceLocation &cursor,
                                  int newlines, int lastLen, int diffLen)
{
    if (t.startLine == cursor.startLine) {
        if (newlines > 0) {
            t.startColumn = lastLen + t.startColumn - cursor.startColumn + 1;
        } else {
            t.startColumn += lastLen;
        }
    }
    t.startLine += newlines;
    t.offset += diffLen;
}

static void expandTokenForMiddleInsert(QQmlJS::SourceLocation &t, const QQmlLSUtils::Diff &diff,
                                       const QQmlJS::SourceLocation &cursor)
{
    auto begin = diff.text.cbegin();
    auto end = diff.text.cend();

    auto ptr = std::find_if(begin, end, [](QChar c) { return c.isSpace(); });

    if (ptr != end) {
        t.length = cursor.begin() - t.begin() + std::distance(begin, ptr);
    } else {
        t.length += diff.text.size();
    }
}

static void expandTokenForLeftOverlap(QQmlJS::SourceLocation &t, const QQmlLSUtils::Diff &diff,
                                      const QQmlJS::SourceLocation &cursor, int newlines,
                                      int lastLen)
{
    const int diffLen = diff.text.size();
    t.offset = cursor.begin();
    t.length += diffLen;
    t.startLine = cursor.startLine;
    t.startColumn = cursor.startColumn;

    // find last space inside diff text
    auto rbegin = diff.text.rbegin();
    auto rend = diff.text.rend();
    auto ptr = std::find_if(rbegin, rend, [](QChar c) { return c.isSpace(); });

    if (ptr != rend) {
        std::ptrdiff_t omitted = std::distance(ptr, rend);
        t.offset += omitted;
        t.length -= omitted;
        t.startColumn += omitted;
    }

    // adjust if diff contains newlines
    if (newlines > 0) {
        t.startLine += newlines;
        t.startColumn = lastLen - std::distance(ptr.base(), diff.text.end()) + 1;
    }
}

static void updateHighlightsOnInsert(HighlightsContainer &highlights,
                                     QQmlJS::SourceLocation &cursor, const QQmlLSUtils::Diff &diff)
{
    const auto [newlines, lastLen] = newlineCountAndLastLineLength(diff.text);
    const auto diffLen = diff.text.size();
    cursor.length = quint32(diffLen); // set length for insertion range, used in overlap checks

    HighlightsContainer shifted;

    for (auto item : highlights) {
        auto &token = item.loc;
        if (tokenBeforeOffset(token, cursor.begin())) {
            shifted.insert(token.offset, item);
            continue;
        }

        if (tokenAfterOffset(token, cursor.begin())) {
            shiftTokenAfterInsert(token, cursor, newlines, lastLen, diffLen);
            shifted.insert(token.offset, item);
            continue;
        }

        // Overlap cases
        if (insertionInsideToken(token, cursor)) {
            expandTokenForMiddleInsert(token, diff, cursor);
        } else if (insertionTouchesTokenLeft(token, cursor)) {
            expandTokenForLeftOverlap(token, diff, cursor, newlines, lastLen);
        }

        shifted.insert(token.offset, item);
    }

    highlights.swap(shifted);

    // Advance cursor for the next Diff
    updateCursorPositionByDiff(diff.text, cursor);
}

//
// Utilities for deletion handling
//
static bool spansAcrossDeletion(const QQmlJS::SourceLocation &t, quint32 delStart, quint32 delEnd)
{
    return t.begin() < delStart && t.end() > delEnd;
}

static bool leftFragmentRemains(const QQmlJS::SourceLocation &t, quint32 delStart, quint32 delEnd)
{
    return t.begin() < delStart && t.end() <= delEnd;
}

static bool rightFragmentRemains(const QQmlJS::SourceLocation &t, quint32 delStart, quint32 delEnd)
{
    return t.begin() >= delStart && t.end() > delEnd;
}

//
// Shift token after deletion
//
static void shiftTokenAfterDelete(QQmlJS::SourceLocation &t, int newlines, int lastLen,
                                  const QQmlJS::SourceLocation &cursor, int diffLen)
{
    t.offset -= diffLen;

    // Adjust column on deletion end line
    if (t.startLine == cursor.startLine + newlines) {
        if (newlines > 0) {
            t.startColumn = cursor.startColumn + (t.startColumn - lastLen) - 1;
        } else {
            t.startColumn -= lastLen;
        }
    }

    // Shift line upwards
    t.startLine -= newlines;
}

//
// Apply overlap logic
//
static void applyDeletionOverlap(QQmlJS::SourceLocation &t, quint32 delStart, quint32 delEnd,
                                 int newlines, quint32 delStartLine, quint32 delStartColumn)
{
    const quint32 deletedLen = delEnd - delStart;

    if (spansAcrossDeletion(t, delStart, delEnd)) {
        // Middle removed
        t.length -= deletedLen;
        return;
    }

    if (leftFragmentRemains(t, delStart, delEnd)) {
        // Left side remains
        t.length = delStart - t.begin();
        return;
    }

    if (rightFragmentRemains(t, delStart, delEnd)) {
        // Right side remains, shifted to the deletion start
        quint32 overlap = delEnd - t.begin();
        t.offset = delStart;
        t.length -= overlap;

        t.startColumn = delStartColumn;
        if (newlines > 0)
            t.startLine = delStartLine;

        return;
    }

    // Fully removed
    t.length = 0;
}

static void updateHighlightsOnDelete(HighlightsContainer &highlights,
                                     QQmlJS::SourceLocation &cursor, const QQmlLSUtils::Diff &diff)
{
    const auto [newlines, lastLen] = newlineCountAndLastLineLength(diff.text);
    const int diffLen = diff.text.size();

    cursor.length = diffLen;

    const quint32 delStart = cursor.offset;
    const quint32 delEnd = cursor.offset + diffLen;

    HighlightsContainer shifts;

    for (auto item : highlights) {
        auto &token = item.loc;

        //
        // Case A: token fully before deleted region
        //
        if (tokenBeforeOffset(token, delStart)) {
            shifts.insert(token.offset, item);
            continue;
        }

        //
        // Case B: token fully after deleted region
        //
        if (tokenAfterOffset(token, delEnd)) {
            shiftTokenAfterDelete(token, newlines, lastLen, cursor, diffLen);
            shifts.insert(token.offset, item);
            continue;
        }

        //
        // Case C: deletion overlaps token
        //
        applyDeletionOverlap(token, delStart, delEnd, newlines, cursor.startLine,
                             cursor.startColumn);

        if (token.length == 0)
            continue; // fully removed

        shifts.insert(token.offset, item);
    }

    highlights.swap(shifts);
}

/*
Equal:
- Just advance the running offset by length.
- No changes to the map.

Insert:
- Insert new entries at the current offset.
- case A: token before insertion offset: no highlight change
- case B: token after insertion offset: slide all offsets forward by the length of the inserted text.
          sub case: if the insertion is on the same line as the token, adjust the column accordingly.
- case C: insertion overlaps token: expand the token length by the length of the inserted text
         sub case 1: insertion is inside the token: expand length
        sub case 2: insertion touches left of the token: adjust offset to insertion start,
                        expand length, adjust line/column if needed.

Delete:
- Case A: token before deletion offset: no highlight change
- case B: token after deletion offset: slide all offsets backward by the length of the deleted text.
          sub case: if the deletion ends on the same line as the token, adjust the column accordingly.
- case C: deletion overlaps token:
        sub case 1: spans across deletion: reduce length by deleted length
        sub case 2: left fragment remains: adjust length to the left fragment length
        sub case 3: right fragment remains: adjust offset to deletion start, adjust length to right fragment length,
                        adjust line/column if needed.
        sub case 4: fully removed: remove the token from the map.
*/
void Utils::applyDiffs(HighlightsContainer &highlights, const QList<QQmlLSUtils::Diff> &diffs)
{
    using namespace QQmlLSUtils;
    if (highlights.isEmpty())
        return;

    QQmlJS::SourceLocation cursor;
    cursor.offset = 0;
    cursor.length = 0;
    cursor.startLine = 1;
    cursor.startColumn = 1;

    for (const Diff &diff : diffs) {
        switch (diff.command) {
        case Diff::Equal:
            // Just advance cursor
            updateCursorPositionByDiff(diff.text, cursor);
            break;
        case Diff::Insert: {
            updateHighlightsOnInsert(highlights, cursor, diff);
            break;
        }
        case Diff::Delete: {
            updateHighlightsOnDelete(highlights, cursor, diff);
            break;
        }
        }
    }
}

} // namespace QmlHighlighting

QT_END_NAMESPACE
