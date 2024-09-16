// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <qqmlsemantictokens_p.h>

#include <QtQmlLS/private/qqmllsutils_p.h>
#include <QtQmlDom/private/qqmldomscriptelements_p.h>
#include <QtQmlDom/private/qqmldomfieldfilter_p.h>

#include <QtLanguageServer/private/qlanguageserverprotocol_p.h>

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(semanticTokens, "qt.languageserver.semanticTokens")

using namespace QQmlJS::AST;
using namespace QQmlJS::Dom;
using namespace QLspSpecification;
using namespace HighlightingUtils;

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
    case QmlHighlightKind::QmlExternalId:
        return int(SemanticTokenProtocolTypes::QmlLocalId);
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
    case QmlHighlightKind::Unknown:
    default:
        return int(SemanticTokenProtocolTypes::JsScopeVar);
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
    case QmlHighlightKind::Unknown:
    default:
        return int(SemanticTokenProtocolTypes::Variable);
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
    using namespace HighlightingUtils;
    int modifier = 0;

    if (highlightModifier.testFlag(QmlHighlightModifier::QmlPropertyDefinition))
        addModifier(SemanticTokenModifiers::Definition, &modifier);

    if (highlightModifier.testFlag(QmlHighlightModifier::QmlDefaultProperty))
        addModifier(SemanticTokenModifiers::DefaultLibrary, &modifier);

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
        { QString(), QString::fromUtf16(Fields::propertyInfos) },
        { QString(), QString::fromUtf16(Fields::fileLocationsTree) },
        { QString(), QString::fromUtf16(Fields::importScope) },
        { QString(), QString::fromUtf16(Fields::defaultPropertyName) },
        { QString(), QString::fromUtf16(Fields::get) },
    };
    return FieldFilter{ fieldFilterAdd, fieldFilterRemove };
}

HighlightingVisitor::HighlightingVisitor(Highlights &highlights,
                                         const std::optional<HighlightsRange> &range)
    : m_highlights(highlights), m_range(range)
{
}

bool HighlightingVisitor::operator()(Path, const DomItem &item, bool)
{
    if (m_range.has_value()) {
        const auto fLocs = FileLocations::treeOf(item);
        if (!fLocs)
            return true;
        const auto regions = fLocs->info().regions;
        if (!HighlightingUtils::rangeOverlapsWithSourceLocation(regions[MainRegion],
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
    const auto locs = HighlightingUtils::sourceLocationsFromMultiLineToken(
            comment->info().comment(), comment->info().sourceLocation());
    for (const auto &loc : locs)
        m_highlights.addHighlight(loc, QmlHighlightKind::Comment);
}

void HighlightingVisitor::highlightImport(const DomItem &item)
{
    const auto fLocs = FileLocations::treeOf(item);
    if (!fLocs)
        return;
    const auto regions = fLocs->info().regions;
    const auto import = item.as<Import>();
    Q_ASSERT(import);
    m_highlights.addHighlight(regions[ImportTokenRegion], QmlHighlightKind::QmlKeyword);
    if (import->uri.isModule())
        m_highlights.addHighlight(regions[ImportUriRegion], QmlHighlightKind::QmlImportId);
    else
        m_highlights.addHighlight(regions[ImportUriRegion], QmlHighlightKind::String);
    if (regions.contains(VersionRegion))
        m_highlights.addHighlight(regions[VersionRegion], QmlHighlightKind::Number);
    if (regions.contains(AsTokenRegion)) {
        m_highlights.addHighlight(regions[AsTokenRegion], QmlHighlightKind::QmlKeyword);
        m_highlights.addHighlight(regions[IdNameRegion], QmlHighlightKind::QmlNamespace);
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
        m_highlights.addHighlight(regions[OnTokenRegion], QmlHighlightKind::QmlKeyword);
        m_highlights.addHighlight(regions[IdentifierRegion], QmlHighlightKind::QmlProperty);
        return;
    }

    return highlightBySemanticAnalysis(item, regions[IdentifierRegion]);
}

void HighlightingVisitor::highlightPragma(const DomItem &item)
{
    const auto fLocs = FileLocations::treeOf(item);
    if (!fLocs)
        return;
    const auto regions = fLocs->info().regions;
    m_highlights.addHighlight(regions[PragmaKeywordRegion], QmlHighlightKind::QmlKeyword);
    m_highlights.addHighlight(regions[IdentifierRegion], QmlHighlightKind::QmlPragmaName );
    const auto pragma = item.as<Pragma>();
    for (auto i = 0; i < pragma->values.size(); ++i) {
        DomItem value = item.field(Fields::values).index(i);
        const auto valueRegions = FileLocations::treeOf(value)->info().regions;
        m_highlights.addHighlight(valueRegions[PragmaValuesRegion], QmlHighlightKind::QmlPragmaValue);
    }
    return;
}

void HighlightingVisitor::highlightEnumDecl(const DomItem &item)
{
    const auto fLocs = FileLocations::treeOf(item);
    if (!fLocs)
        return;
    const auto regions = fLocs->info().regions;
    m_highlights.addHighlight(regions[EnumKeywordRegion], QmlHighlightKind::QmlKeyword);
    m_highlights.addHighlight(regions[IdentifierRegion], QmlHighlightKind::QmlEnumName);
}

void HighlightingVisitor::highlightEnumItem(const DomItem &item)
{
    const auto fLocs = FileLocations::treeOf(item);
    if (!fLocs)
        return;
    const auto regions = fLocs->info().regions;
    m_highlights.addHighlight(regions[IdentifierRegion], QmlHighlightKind::QmlEnumMember);
    if (regions.contains(EnumValueRegion))
        m_highlights.addHighlight(regions[EnumValueRegion], QmlHighlightKind::Number);
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
        m_highlights.addHighlight(regions[IdTokenRegion], QmlHighlightKind::QmlProperty);
        m_highlights.addHighlight(regions[IdNameRegion], QmlHighlightKind::QmlLocalId);
    }
    // If dotted name, then defer it to be handled in ScriptIdentifierExpression
    if (qmlObject->name().contains("."_L1))
        return;

    m_highlights.addHighlight(regions[IdentifierRegion], QmlHighlightKind::QmlType);
}

void HighlightingVisitor::highlightComponent(const DomItem &item)
{
    const auto fLocs = FileLocations::treeOf(item);
    if (!fLocs)
        return;
    const auto regions = fLocs->info().regions;
    m_highlights.addHighlight(regions[ComponentKeywordRegion], QmlHighlightKind::QmlKeyword);
    m_highlights.addHighlight(regions[IdentifierRegion], QmlHighlightKind::QmlType);
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
        m_highlights.addHighlight(regions[DefaultKeywordRegion], QmlHighlightKind::QmlKeyword);
    }
    if (propertyDef->isRequired) {
        modifier |= QmlHighlightModifier::QmlRequiredProperty;
        m_highlights.addHighlight(regions[RequiredKeywordRegion], QmlHighlightKind::QmlKeyword);
    }
    if (propertyDef->isReadonly) {
        modifier |= QmlHighlightModifier::QmlReadonlyProperty;
        m_highlights.addHighlight(regions[ReadonlyKeywordRegion], QmlHighlightKind::QmlKeyword);
    }
    m_highlights.addHighlight(regions[PropertyKeywordRegion], QmlHighlightKind::QmlKeyword);
    if (propertyDef->isAlias())
        m_highlights.addHighlight(regions[TypeIdentifierRegion], QmlHighlightKind::QmlKeyword);
    else
        m_highlights.addHighlight(regions[TypeIdentifierRegion], QmlHighlightKind::QmlType);

    m_highlights.addHighlight(regions[TypeModifierRegion], QmlHighlightKind::QmlTypeModifier);
    m_highlights.addHighlight(regions[IdentifierRegion], QmlHighlightKind::QmlProperty,
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
        m_highlights.addHighlight(regions[SignalKeywordRegion], QmlHighlightKind::QmlKeyword);
        m_highlights.addHighlight(regions[IdentifierRegion], QmlHighlightKind::QmlMethod);
        break;
    }
    case MethodInfo::Method: {
        m_highlights.addHighlight(regions[FunctionKeywordRegion], QmlHighlightKind::QmlKeyword);
        m_highlights.addHighlight(regions[IdentifierRegion], QmlHighlightKind::QmlMethod);
        m_highlights.addHighlight(regions[TypeIdentifierRegion], QmlHighlightKind::QmlType);
        break;
    }
    default:
        Q_UNREACHABLE();
    }

    for (auto i = 0; i < method->parameters.size(); ++i) {
        DomItem parameter = item.field(Fields::parameters).index(i);
        const auto paramRegions = FileLocations::treeOf(parameter)->info().regions;
        m_highlights.addHighlight(paramRegions[IdentifierRegion],
                                    QmlHighlightKind::QmlMethodParameter);
        m_highlights.addHighlight(paramRegions[TypeIdentifierRegion], QmlHighlightKind::QmlType);
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
        const auto &locs = HighlightingUtils::sourceLocationsFromMultiLineToken(
                literalCode, regions[MainRegion]);
        for (const auto &loc : locs)
            m_highlights.addHighlight(loc, QmlHighlightKind::String);
    } else if (std::holds_alternative<double>(literal->literalValue()))
        m_highlights.addHighlight(regions[MainRegion], QmlHighlightKind::Number);
    else if (std::holds_alternative<bool>(literal->literalValue()))
        m_highlights.addHighlight(regions[MainRegion], QmlHighlightKind::QmlKeyword);
    else if (std::holds_alternative<std::nullptr_t>(literal->literalValue()))
        m_highlights.addHighlight(regions[MainRegion], QmlHighlightKind::QmlKeyword);
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
    if (m_highlights.highlights().contains(loc.offset))
        return;

    highlightBySemanticAnalysis(item, loc);
}

void HighlightingVisitor::highlightBySemanticAnalysis(const DomItem &item, QQmlJS::SourceLocation loc)
{
    const auto expression = QQmlLSUtils::resolveExpressionType(
            item, QQmlLSUtils::ResolveOptions::ResolveOwnerType);

    if (!expression) {
        m_highlights.addHighlight(loc, QmlHighlightKind::Unknown);
        return;
    }
    switch (expression->type) {
    case QQmlLSUtils::QmlComponentIdentifier:
        m_highlights.addHighlight(loc, QmlHighlightKind::QmlType);
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
                m_highlights.addHighlight(loc, tokenType, modifier);
                return;
            }
        }
        if (const auto name = expression->name) {
            if (const auto highlightKind = resolveJsGlobalObjectKind(item, *name))
                return m_highlights.addHighlight(loc, *highlightKind);
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
            m_highlights.addHighlight(loc, tokenType, modifier);
        }
        return;
    }
    case QQmlLSUtils::PropertyChangedSignalIdentifier:
        m_highlights.addHighlight(loc, QmlHighlightKind::QmlSignal);
        return;
    case QQmlLSUtils::PropertyChangedHandlerIdentifier:
        m_highlights.addHighlight(loc, QmlHighlightKind::QmlSignalHandler);
        return;
    case QQmlLSUtils::SignalIdentifier:
        m_highlights.addHighlight(loc, QmlHighlightKind::QmlSignal);
        return;
    case QQmlLSUtils::SignalHandlerIdentifier:
        m_highlights.addHighlight(loc, QmlHighlightKind::QmlSignalHandler);
        return;
    case QQmlLSUtils::MethodIdentifier:
        m_highlights.addHighlight(loc, QmlHighlightKind::QmlMethod);
        return;
    case QQmlLSUtils::QmlObjectIdIdentifier:
        m_highlights.addHighlight(loc, QmlHighlightKind::QmlLocalId);
        return;
    case QQmlLSUtils::SingletonIdentifier:
        m_highlights.addHighlight(loc, QmlHighlightKind::QmlType);
        return;
    case QQmlLSUtils::EnumeratorIdentifier:
        m_highlights.addHighlight(loc, QmlHighlightKind::QmlEnumName);
        return;
    case QQmlLSUtils::EnumeratorValueIdentifier:
        m_highlights.addHighlight(loc, QmlHighlightKind::QmlEnumMember);
        return;
    case QQmlLSUtils::AttachedTypeIdentifier:
        m_highlights.addHighlight(loc, QmlHighlightKind::QmlType);
        return;
    case QQmlLSUtils::GroupedPropertyIdentifier:
        m_highlights.addHighlight(loc, QmlHighlightKind::QmlProperty);
        return;
    case QQmlLSUtils::QualifiedModuleIdentifier:
        m_highlights.addHighlight(loc, QmlHighlightKind::QmlNamespace);
        return;
    default:
        qCWarning(semanticTokens)
                << QString::fromLatin1("Semantic token for %1 has not been implemented yet")
                            .arg(int(expression->type));
    }
    Q_UNREACHABLE_RETURN();
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
        m_highlights.addHighlight(regions[ForKeywordRegion], QmlHighlightKind::QmlKeyword);
        m_highlights.addHighlight(regions[TypeIdentifierRegion],
                                    QmlHighlightKind::QmlKeyword);
        return;

    case DomType::ScriptVariableDeclaration: {
        m_highlights.addHighlight(regions[TypeIdentifierRegion],
                                   QmlHighlightKind::QmlKeyword);
        return;
    }
    case DomType::ScriptReturnStatement:
        m_highlights.addHighlight(regions[ReturnKeywordRegion], QmlHighlightKind::QmlKeyword);
        return;
    case DomType::ScriptCaseClause:
        m_highlights.addHighlight(regions[CaseKeywordRegion], QmlHighlightKind::QmlKeyword);
        return;
    case DomType::ScriptDefaultClause:
        m_highlights.addHighlight(regions[DefaultKeywordRegion], QmlHighlightKind::QmlKeyword);
        return;
    case DomType::ScriptSwitchStatement:
        m_highlights.addHighlight(regions[SwitchKeywordRegion], QmlHighlightKind::QmlKeyword);
        return;
    case DomType::ScriptWhileStatement:
        m_highlights.addHighlight(regions[WhileKeywordRegion], QmlHighlightKind::QmlKeyword);
        return;
    case DomType::ScriptDoWhileStatement:
        m_highlights.addHighlight(regions[DoKeywordRegion], QmlHighlightKind::QmlKeyword);
        m_highlights.addHighlight(regions[WhileKeywordRegion], QmlHighlightKind::QmlKeyword);
        return;
    case DomType::ScriptTryCatchStatement:
        m_highlights.addHighlight(regions[TryKeywordRegion], QmlHighlightKind::QmlKeyword);
        m_highlights.addHighlight(regions[CatchKeywordRegion], QmlHighlightKind::QmlKeyword);
        m_highlights.addHighlight(regions[FinallyKeywordRegion], QmlHighlightKind::QmlKeyword);
        return;
    case DomType::ScriptForEachStatement:
        m_highlights.addHighlight(regions[TypeIdentifierRegion], QmlHighlightKind::QmlKeyword);
        m_highlights.addHighlight(regions[ForKeywordRegion], QmlHighlightKind::QmlKeyword);
        m_highlights.addHighlight(regions[InOfTokenRegion], QmlHighlightKind::QmlKeyword);
        return;
    case DomType::ScriptThrowStatement:
        m_highlights.addHighlight(regions[ThrowKeywordRegion], QmlHighlightKind::QmlKeyword);
        return;
    case DomType::ScriptBreakStatement:
        m_highlights.addHighlight(regions[BreakKeywordRegion], QmlHighlightKind::QmlKeyword);
        return;
    case DomType::ScriptContinueStatement:
        m_highlights.addHighlight(regions[ContinueKeywordRegion], QmlHighlightKind::QmlKeyword);
        return;
    case DomType::ScriptIfStatement:
        m_highlights.addHighlight(regions[IfKeywordRegion], QmlHighlightKind::QmlKeyword);
        m_highlights.addHighlight(regions[ElseKeywordRegion], QmlHighlightKind::QmlKeyword);
        return;
    case DomType::ScriptLabelledStatement:
        m_highlights.addHighlight(regions[IdentifierRegion], QmlHighlightKind::JsLabel);
        return;
    case DomType::ScriptConditionalExpression:
        m_highlights.addHighlight(regions[QuestionMarkTokenRegion], QmlHighlightKind::Operator);
        m_highlights.addHighlight(regions[ColonTokenRegion], QmlHighlightKind::Operator);
        return;
    case DomType::ScriptUnaryExpression:
    case DomType::ScriptPostExpression:
        m_highlights.addHighlight(regions[OperatorTokenRegion], QmlHighlightKind::Operator);
        return;
    case DomType::ScriptType:
        m_highlights.addHighlight(regions[IdentifierRegion], QmlHighlightKind::QmlType);
        m_highlights.addHighlight(regions[TypeIdentifierRegion], QmlHighlightKind::QmlType);
        return;
    case DomType::ScriptFunctionExpression: {
        m_highlights.addHighlight(regions[FunctionKeywordRegion], QmlHighlightKind::QmlKeyword);
        m_highlights.addHighlight(regions[IdentifierRegion], QmlHighlightKind::QmlMethod);
        return;
    }
    case DomType::ScriptYieldExpression:
        m_highlights.addHighlight(regions[YieldKeywordRegion], QmlHighlightKind::QmlKeyword);
        return;
    case DomType::ScriptThisExpression:
        m_highlights.addHighlight(regions[ThisKeywordRegion], QmlHighlightKind::QmlKeyword);
        return;
    case DomType::ScriptSuperLiteral:
        m_highlights.addHighlight(regions[SuperKeywordRegion], QmlHighlightKind::QmlKeyword);
        return;
    case DomType::ScriptNewMemberExpression:
    case DomType::ScriptNewExpression:
        m_highlights.addHighlight(regions[NewKeywordRegion], QmlHighlightKind::QmlKeyword);
        return;
    default:
        qCDebug(semanticTokens)
                << "Script Expressions with kind" << item.internalKind() << "not implemented";
        return;
    }
    Q_UNREACHABLE_RETURN();
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
HighlightingUtils::sourceLocationsFromMultiLineToken(QStringView stringLiteral,
                                                     const QQmlJS::SourceLocation &locationInDocument)
{
    auto lineBreakLength = qsizetype(std::char_traits<char>::length("\n"));
    const auto lineLengths = [&lineBreakLength](QStringView literal) {
        std::vector<qsizetype> lineLengths;
        qsizetype startIndex = 0;
        qsizetype pos = literal.indexOf(u'\n');
        while (pos != -1) {
            // TODO: QTBUG-106813
            // Since a document could be opened in normalized form
            // we can't use platform dependent newline handling here.
            // Thus, we check manually if the literal contains \r so that we split
            // the literal at the correct offset.
            if (pos - 1 > 0 && literal[pos - 1] == u'\r') {
                // Handle Windows line endings
                lineBreakLength = qsizetype(std::char_traits<char>::length("\r\n"));
                // Move pos to the index of '\r'
                pos = pos - 1;
            }
            lineLengths.push_back(pos - startIndex);
            // Advance the lookup index, so it won't find the same index.
            startIndex = pos + lineBreakLength;
            pos = literal.indexOf('\n'_L1, startIndex);
        }
        // Push the last line
        if (startIndex < literal.length()) {
            lineLengths.push_back(literal.length() - startIndex);
        }
        return lineLengths;
    };

    QList<QQmlJS::SourceLocation> result;
    // First token location should start from the "stringLiteral"'s
    // location in the qml document.
    QQmlJS::SourceLocation lineLoc = locationInDocument;
    for (const auto lineLength : lineLengths(stringLiteral)) {
        lineLoc.length = lineLength;
        result.push_back(lineLoc);

        // update for the next line
        lineLoc.offset += lineLoc.length + lineBreakLength;
        ++lineLoc.startLine;
        lineLoc.startColumn = 1;
    }
    return result;
}

QList<int> HighlightingUtils::encodeSemanticTokens(Highlights &highlights)
{
    QList<int> result;
    const auto highlightingTokens = highlights.highlights();
    constexpr auto tokenEncodingLength = 5;
    result.reserve(tokenEncodingLength * highlightingTokens.size());

    int prevLine = 0;
    int prevColumn = 0;

    std::for_each(highlightingTokens.constBegin(), highlightingTokens.constEnd(), [&](const auto &token) {
        Q_ASSERT(token.startLine >= prevLine);
        if (token.startLine != prevLine)
            prevColumn = 0;
        result.emplace_back(token.startLine - prevLine);
        result.emplace_back(token.startColumn - prevColumn);
        result.emplace_back(token.length);
        result.emplace_back(token.tokenType);
        result.emplace_back(token.tokenModifier);
        prevLine = token.startLine;
        prevColumn = token.startColumn;
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
void HighlightingUtils::addModifier(SemanticTokenModifiers modifier, int *baseModifier)
{
    if (!baseModifier)
        return;
    *baseModifier |= (1 << int(modifier));
}

/*!
\internal
Check if the ranges overlap by ensuring that one range starts before the other ends
*/
bool HighlightingUtils::rangeOverlapsWithSourceLocation(const QQmlJS::SourceLocation &loc,
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
void HighlightingUtils::updateResultID(QByteArray &resultID)
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
QList<SemanticTokensEdit> HighlightingUtils::computeDiff(const QList<int> &oldData, const QList<int> &newData)
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

Highlights::Highlights(HighlightingMode mode)
    : m_mapToProtocol(mode == HighlightingMode::QtCHighlighting ? mapToProtocolForQtCreator
                                                                : mapToProtocolDefault)
{
}

void Highlights::addHighlight(const QQmlJS::SourceLocation &loc, QmlHighlightKind highlightKind,
                              QmlHighlightModifiers modifierKind)
{
    int tokenType = m_mapToProtocol(highlightKind);
    int modifierType = fromQmlModifierKindToLspTokenType(modifierKind);
    return addHighlightImpl(loc, tokenType, modifierType);
}

void Highlights::addHighlightImpl(const QQmlJS::SourceLocation &loc, int tokenType, int tokenModifier)
{
    if (!loc.isValid()) {
        qCDebug(semanticTokens) << "Invalid locations: Cannot add highlight to token";
        return;
    }

    if (loc.length == 0)
        return;

    if (!m_highlights.contains(loc.offset))
        m_highlights.insert(loc.offset, QT_PREPEND_NAMESPACE(Token)(loc, tokenType, tokenModifier));
}

QList<int> HighlightingUtils::collectTokens(const QQmlJS::Dom::DomItem &item,
                                     const std::optional<HighlightsRange> &range,
                                     HighlightingMode mode)
{
    using namespace QQmlJS::Dom;
    Highlights highlights(mode);
    HighlightingVisitor highlightDomElements(highlights, range);
    // In QmlFile level, visitTree visits even FileLocations tree which takes quite a time to
    // finish. HighlightingFilter is added to prevent unnecessary visits.
    item.visitTree(Path(), highlightDomElements, VisitOption::Default, emptyChildrenVisitor,
                   emptyChildrenVisitor, highlightingFilter());

    return HighlightingUtils::encodeSemanticTokens(highlights);
}

QT_END_NAMESPACE
