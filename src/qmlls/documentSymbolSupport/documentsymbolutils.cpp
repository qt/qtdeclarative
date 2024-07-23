// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qqmllsutils_p.h"
#include "documentsymbolutils_p.h"
#include <QtLanguageServer/private/qlanguageserverspectypes_p.h>
#include <QtQmlDom/private/qqmldomitem_p.h>
#include <QtQmlDom/private/qqmldomoutwriter_p.h>
#include <stack>

QT_BEGIN_NAMESPACE

namespace DocumentSymbolUtils {
using QLspSpecification::SymbolKind;
using namespace QQmlJS::Dom;

struct TypeSymbolRelation
{
    DomType domType;
    SymbolKind symbolKind;
};

constexpr static std::array<TypeSymbolRelation, 9> s_TypeSymbolRelations = { {
        { DomType::Binding, SymbolKind::Variable },
        { DomType::PropertyDefinition, SymbolKind::Property },
        // Although MethodInfo simply relates to Method, SymbolKind requires special handling:
        // When MethodInfo represents a Signal, its SymbolKind is set to Event.
        // This distinction is explicitly managed in the symbolKindOf() helper function.
        // see also QTBUG-128423
        { DomType::MethodInfo, SymbolKind::Method },
        { DomType::Id, SymbolKind::Key },
        { DomType::QmlObject, SymbolKind::Object },
        { DomType::EnumDecl, SymbolKind::Enum },
        { DomType::EnumItem, SymbolKind::EnumMember },
        { DomType::QmlComponent, SymbolKind::Module },
        { DomType::QmlFile, SymbolKind::File },
} };

[[nodiscard]] constexpr static inline SymbolKind symbolKindFor(const DomType &type)
{
    // constexpr std::find_if is only from c++20
    for (const auto &mapping : s_TypeSymbolRelations) {
        if (mapping.domType == type) {
            return mapping.symbolKind;
        }
    }
    return SymbolKind::Null;
}

constexpr static inline bool documentSymbolNotSupportedFor(const DomType &type)
{
    return symbolKindFor(type) == SymbolKind::Null;
}

static bool propertyBoundAtDefinitionLine(const DomItem &propertyDefinition)
{
    Q_ASSERT(propertyDefinition.internalKind() == DomType::PropertyDefinition);
    return FileLocations::treeOf(propertyDefinition)->info().regions[ColonTokenRegion].isValid();
}

static inline bool shouldFilterOut(const DomItem &item)
{
    const auto itemType = item.internalKind();
    if (documentSymbolNotSupportedFor(itemType)) {
        return true;
    }
    if (itemType == DomType::PropertyDefinition && propertyBoundAtDefinitionLine(item)) {
        // without this check there is a "duplication" of symbols.
        // one representing PropertyDefinition another one - Binding
        return true;
    }
    return false;
}

static std::optional<QByteArray> tryGetQmlObjectDetail(const DomItem &qmlObj)
{
    using namespace QQmlJS::Dom;
    Q_ASSERT(qmlObj.internalKind() == DomType::QmlObject);
    bool hasId = !qmlObj.idStr().isEmpty();
    if (hasId) {
        return qmlObj.idStr().toUtf8();
    }
    const bool isRootObject = qmlObj.component().field(Fields::objects).index(0) == qmlObj;
    if (isRootObject) {
        return "root";
    }
    return std::nullopt;
}

static std::optional<QByteArray> tryGetBindingDetail(const DomItem &bItem)
{
    const auto *bindingPtr = bItem.as<Binding>();
    Q_ASSERT(bindingPtr);
    switch (bindingPtr->valueKind()) {
    case BindingValueKind::ScriptExpression: {
        auto exprCode = bindingPtr->scriptExpressionValue()->code();
        if (exprCode.length() > 25) {
            return QStringView(exprCode).first(22).toUtf8().append("...");
        }
        if (exprCode.endsWith(QStringLiteral(";"))) {
            exprCode.chop(1);
        }
        return exprCode.toUtf8();
    }
    default:
        // Value is QmlObject or QList<QmlObject> => no detail
        return std::nullopt;
    }
}

static inline QByteArray getMethodDetail(const DomItem &mItem)
{
    const auto *methodInfoPtr = mItem.as<MethodInfo>();
    Q_ASSERT(methodInfoPtr);
    return methodInfoPtr->signature(mItem).toUtf8();
}

std::optional<QByteArray> tryGetDetailOf(const DomItem &item)
{
    switch (item.internalKind()) {
    case DomType::Id: {
        const auto name = item.name();
        return name.isEmpty() ? std::nullopt : std::make_optional(name.toUtf8());
    }
    case DomType::EnumItem:
        return QByteArray::number(item.as<EnumItem>()->value());
    case DomType::QmlObject:
        return tryGetQmlObjectDetail(item);
    case DomType::MethodInfo:
        return getMethodDetail(item);
    case DomType::Binding:
        return tryGetBindingDetail(item);
    default:
        return std::nullopt;
    }
}

// TODO move to qmllsUtils?
static inline bool isSubRange(const QLspSpecification::Range &potentialSubRange,
                              const QLspSpecification::Range &range)
{
    // Check if the start of a is greater than or equal to the start of b
    bool startContained = (potentialSubRange.start.line > range.start.line
                           || (potentialSubRange.start.line == range.start.line
                               && potentialSubRange.start.character >= range.start.character));

    // Check if the end of a is less than or equal to the end of b
    bool endContained = (potentialSubRange.end.line < range.end.line
                         || (potentialSubRange.end.line == range.end.line
                             && potentialSubRange.end.character <= range.end.character));

    return startContained && endContained;
}

using MutableRefToDocumentSymbol = QLspSpecification::DocumentSymbol &;
[[nodiscard]] static MutableRefToDocumentSymbol
findDirectParentFor(const QLspSpecification::DocumentSymbol &child,
                    MutableRefToDocumentSymbol currentParent)
{
    const auto containsChildRange =
            [&range = child.range](const QLspSpecification::DocumentSymbol &symbol) {
                return isSubRange(range, symbol.range);
            };
    // Parent's Range covers children's Ranges
    // all children, grand-children, grand-grand-children and so forth
    // are not supposed to have overlapping Ranges, hence it's just "gready" approach
    // 1. find a Symbol among children, containing a Range
    // 2. set it as currentCandidate
    // 3. repeat
    std::reference_wrapper<QLspSpecification::DocumentSymbol> currentCandidate(currentParent);
    while (containsChildRange(currentCandidate) && currentCandidate.get().children.has_value()) {
        auto newCandidate =
                std::find_if(currentCandidate.get().children->begin(),
                             currentCandidate.get().children->end(), containsChildRange);
        if (newCandidate == currentCandidate.get().children->end()) {
            break;
        }
        currentCandidate = std::ref(*newCandidate);
    }
    return currentCandidate;
}

using DocumentSymbolPredicate =
        qxp::function_ref<bool(const QLspSpecification::DocumentSymbol &) const>;
[[nodiscard]] static SymbolsList extractChildrenIf(const DocumentSymbolPredicate shouldBeReadopted,
                                                   MutableRefToDocumentSymbol currentParent)
{
    if (!currentParent.children.has_value()) {
        return {};
    }
    auto &parentsChildren = currentParent.children.value();
    SymbolsList extractedChildren;
    extractedChildren.reserve(parentsChildren.size());
    auto [_, toBeRemoved] = std::partition_copy(parentsChildren.cbegin(), parentsChildren.cend(),
                                                std::back_inserter(extractedChildren),
                                                parentsChildren.begin(), shouldBeReadopted);
    parentsChildren.erase(toBeRemoved, parentsChildren.end());
    return extractedChildren;
}

static inline void adopt(QLspSpecification::DocumentSymbol &&child,
                         MutableRefToDocumentSymbol parent)
{
    if (!parent.children.has_value()) {
        parent.children.emplace({ std::move(child) });
        return;
    }
    parent.children->emplace_back(std::move(child));
}

static void readoptChildrenIf(const DocumentSymbolPredicate unaryPred,
                              MutableRefToDocumentSymbol currentParent)
{
    auto childrenToBeReadopted = extractChildrenIf(unaryPred, currentParent);
    for (auto &&child : childrenToBeReadopted) {
        auto &newParentRef = findDirectParentFor(child, currentParent);
        adopt(std::move(child), newParentRef);
    }
}

// Readopts all Enum-s and Id-s
static void reorganizeQmlComponentSymbol(MutableRefToDocumentSymbol qmlCompSymbol)
{
    Q_ASSERT(qmlCompSymbol.kind == symbolKindFor(DomType::QmlComponent));
    if (!qmlCompSymbol.children.has_value()) {
        // nothing to reorganize
        return;
    }

    constexpr auto enumDeclSymbolKind = symbolKindFor(DomType::EnumDecl);
    const auto symbolIsEnumDecl = [](const QLspSpecification::DocumentSymbol &symbol) -> bool {
        return symbol.kind == enumDeclSymbolKind;
    };
    readoptChildrenIf(symbolIsEnumDecl, qmlCompSymbol);
}

/*! \internal
 *  This function reorganizes \c qmlFileSymbols (result of assembleSymbolsForQmlFile)
 *  in the following way:
 *  1. Moves Symbol-s representing Enum-s and inline QmlComponent-s
 * to their respective range-containing parents , a.k.a. direct structural parents.
 *  2. Reassignes head to the DocumentSymbol representing root QmlObject of the main
 * QmlComponent
 */
void reorganizeForOutlineView(SymbolsList &qmlFileSymbols)
{
    Q_ASSERT(qmlFileSymbols.at(0).kind == symbolKindFor(DomType::QmlFile)
             && qmlFileSymbols.at(0).children.has_value());

    auto &qmlFileSymbol = qmlFileSymbols[0];
    constexpr auto qmlCompSymbolKind = symbolKindFor(DomType::QmlComponent);
    for (auto &childSymbol : qmlFileSymbol.children.value()) {
        if (childSymbol.kind == qmlCompSymbolKind) {
            reorganizeQmlComponentSymbol(childSymbol);
        }
    }

    const auto symbolIsInlineComp = [](const QLspSpecification::DocumentSymbol &symbol) -> bool {
        return symbol.kind == qmlCompSymbolKind && symbol.name.contains(".");
    };
    readoptChildrenIf(symbolIsInlineComp, qmlFileSymbol);

    // move pointer from the documentSymbol representing QmlFile
    // to the documentSymbols representing children of main QmlComponent
    // a.k.a. ignore / not to show QmlFile and mainComponent symbols
    qmlFileSymbols = qmlFileSymbol.children->at(0).children.value();
}

/*! \internal
 * Constructs a \c DocumentSymbol for an \c Item with the provided \c children.
 * Returns \c children if the current \c Item should not be represented via a \c DocumentSymbol.
 */
SymbolsList buildSymbolOrReturnChildren(const DomItem &item, SymbolsList &&children)
{
    if (shouldFilterOut(item)) {
        // nothing to build, just returning children
        return std::move(children);
    }

    const auto buildPartialSymbol = [](const DomItem &item) {
        QLspSpecification::DocumentSymbol symbol;
        symbol.kind = symbolKindOf(item);
        symbol.name = symbolNameOf(item);
        symbol.detail = tryGetDetailOf(item);
        std::tie(symbol.range, symbol.selectionRange) = symbolRangesOf(item);
        return symbol;
    };

    auto symbol = buildPartialSymbol(item);
    if (!children.empty()) {
        symbol.children.emplace(std::move(children));
    }
    /*
     To avoid pushing down Id items through the DocumentSymbol tree,
     as part of rearrangement step, it was decided to handle them here explicitly.
     That Id issue atm only affects objects
     If / when Id is moving from component level to Object level this should be reflected
     also in the visiting logic.
     TODO(QTBUG-128274)
     */
    if (const auto objPtr = item.as<QmlObject>()) {
        if (const auto idItem = item.component().field(Fields::ids).key(objPtr->idStr()).index(0)) {
            auto idSymbol = buildPartialSymbol(idItem);
            adopt(std::move(idSymbol), symbol);
        }
    }
    return SymbolsList{ std::move(symbol) };
}

std::pair<QLspSpecification::Range, QLspSpecification::Range> symbolRangesOf(const DomItem &item)
{
    const auto &fLoc = FileLocations::treeOf(item)->info();
    const auto fullRangeSourceloc = fLoc.fullRegion;
    const auto selectionRangeSourceLoc = fLoc.regions[IdentifierRegion].isValid()
            ? fLoc.regions[IdentifierRegion]
            : fullRangeSourceloc;

    auto fItem = item.containingFile();
    Q_ASSERT(fItem);
    const QString &code = fItem.ownerAs<QmlFile>()->code();
    return { QQmlLSUtils::qmlLocationToLspLocation(
                     QQmlLSUtils::Location::from({}, fullRangeSourceloc, code)),
             QQmlLSUtils::qmlLocationToLspLocation(
                     QQmlLSUtils::Location::from({}, selectionRangeSourceLoc, code)) };
}

QByteArray symbolNameOf(const DomItem &item)
{
    if (item.internalKind() == DomType::Id) {
        return "id";
    }
    return (item.name().isEmpty() ? item.internalKindStr() : item.name()).toUtf8();
}

QLspSpecification::SymbolKind symbolKindOf(const DomItem &item)
{
    if (item.internalKind() == DomType::MethodInfo) {
        const auto *methodInfoPtr = item.as<MethodInfo>();
        Q_ASSERT(methodInfoPtr);
        return methodInfoPtr->methodType == MethodInfo::MethodType::Signal
                ? SymbolKind::Event
                : symbolKindFor(DomType::MethodInfo);
    }
    return symbolKindFor(item.internalKind());
}

/*! \internal
 * Design decisions behind this class are the following:
 * 1. It is an implementation detail of the free \c assembleSymbolsForQmlFile function
 * 2. It can only be initialized and used once per \c Item.
 * This is enforced by its \c refToRootItem reference member.
 * 3. It is tested via the public \c assembleSymbolsForQmlFile function.
 */
class DocumentSymbolVisitor
{
public:
    DocumentSymbolVisitor(const DomItem &item, const AssemblingFunction af)
        : m_assemble(af), m_refToRootItem(item) {};

    static const FieldFilter &fieldsFilter();

    [[nodiscard]] SymbolsList assembleSymbols();

private:
    [[nodiscard]] SymbolsList popAndAssembleSymbolsFor(const DomItem &item);

    void appendToTop(const SymbolsList &symbols);

private:
    const AssemblingFunction m_assemble;
    const DomItem &m_refToRootItem;
    std::stack<SymbolsList> m_stackOfChildrenSymbols;
};

const FieldFilter &DocumentSymbolVisitor::fieldsFilter()
{
    // TODO(QTBUG-128118) add only fields to be visited and not the ones
    // to be removed.
    static const FieldFilter ff{
        {}, // to add
        {
                // to remove
                { QString(), QString::fromUtf16(Fields::code) },
                { QString(), QString::fromUtf16(Fields::postCode) },
                { QString(), QString::fromUtf16(Fields::preCode) },
                { QString(), QString::fromUtf16(Fields::importScope) },
                { QString(), QString::fromUtf16(Fields::fileLocationsTree) },
                { QString(), QString::fromUtf16(Fields::astComments) },
                { QString(), QString::fromUtf16(Fields::comments) },
                { QString(), QString::fromUtf16(Fields::exports) },
                { QString(), QString::fromUtf16(Fields::propertyInfos) },
                { QLatin1String("AttachedInfo"), QString::fromUtf16(Fields::parent) },
                //^^^ FieldFilter::default
                { QString(), QString::fromUtf16(Fields::errors) },
                { QString(), QString::fromUtf16(Fields::imports) },
                { QString(), QString::fromUtf16(Fields::prototypes) },
                { QString(), QString::fromUtf16(Fields::annotations) },
                { QString(), QString::fromUtf16(Fields::attachedType) },
                { QString(), QString::fromUtf16(Fields::canonicalFilePath) },
                { QString(), QString::fromUtf16(Fields::isValid) },
                { QString(), QString::fromUtf16(Fields::isSingleton) },
                { QString(), QString::fromUtf16(Fields::isCreatable) },
                { QString(), QString::fromUtf16(Fields::isComposite) },
                { QString(), QString::fromUtf16(Fields::attachedTypeName) },
                { QString(), QString::fromUtf16(Fields::pragmas) },
                { QString(), QString::fromUtf16(Fields::defaultPropertyName) },
                { QString(), QString::fromUtf16(Fields::name) },
                { QString(), QString::fromUtf16(Fields::nameIdentifiers) },
                { QString(), QString::fromUtf16(Fields::prototypes) },
                { QString(), QString::fromUtf16(Fields::nextScope) },
                { QString(), QString::fromUtf16(Fields::parameters) },
                { QString(), QString::fromUtf16(Fields::methodType) },
                { QString(), QString::fromUtf16(Fields::type) },
                { QString(), QString::fromUtf16(Fields::isConstructor) },
                { QString(), QString::fromUtf16(Fields::returnType) },
                { QString(), QString::fromUtf16(Fields::body) },
                { QString(), QString::fromUtf16(Fields::access) },
                { QString(), QString::fromUtf16(Fields::typeName) },
                { QString(), QString::fromUtf16(Fields::isReadonly) },
                { QString(), QString::fromUtf16(Fields::isList) },
                { QString(), QString::fromUtf16(Fields::bindingIdentifiers) },
                { QString(), QString::fromUtf16(Fields::bindingType) },
                { QString(), QString::fromUtf16(Fields::isSignalHandler) },
                // prop def?
                { QString(), QString::fromUtf16(Fields::isPointer) },
                { QString(), QString::fromUtf16(Fields::isFinal) },
                { QString(), QString::fromUtf16(Fields::isAlias) },
                { QString(), QString::fromUtf16(Fields::isDefaultMember) },
                { QString(), QString::fromUtf16(Fields::isRequired) },
                { QString(), QString::fromUtf16(Fields::read) },
                { QString(), QString::fromUtf16(Fields::write) },
                { QString(), QString::fromUtf16(Fields::bindable) },
                { QString(), QString::fromUtf16(Fields::notify) },
                { QString(), QString::fromUtf16(Fields::type) },
                // scriptExpr
                { QString(), QString::fromUtf16(Fields::scriptElement) },
                { QString(), QString::fromUtf16(Fields::localOffset) },
                { QString(), QString::fromUtf16(Fields::astRelocatableDump) },
                { QString(), QString::fromUtf16(Fields::expressionType) },
                // components
                { QString(), QString::fromUtf16(Fields::subComponents) },
                // BEWARE
                // Ids and IdStr are filtered out during the visit, because
                // documentSymbol-s for them will be explicitly handled as part of the
                // creation of symbol for QmlObject
                { QString(), QString::fromUtf16(Fields::ids) },
                { QString(), QString::fromUtf16(Fields::idStr) },

                // id
                { QString(), QString::fromUtf16(Fields::referredObject) },
                // enum item
                { QLatin1String("EnumItem"), QString::fromUtf16(Fields::value) },
        }
    };
    return ff;
}

SymbolsList DocumentSymbolVisitor::assembleSymbols()
{
    using namespace QQmlJS::Dom;
    auto openingVisitor = [this](const Path &, const DomItem &, bool) -> bool {
        m_stackOfChildrenSymbols.emplace();
        return true;
    };
    auto closingVisitor = [this](const Path &, const DomItem &item, bool) -> bool {
        // it's closing Visitor, openingVisitor must have pushed something
        Q_ASSERT(!m_stackOfChildrenSymbols.empty());
        if (m_stackOfChildrenSymbols.size() == 1) {
            // reached children of root, nothing to do
            return false;
        }
        auto symbols = popAndAssembleSymbolsFor(item);
        appendToTop(symbols);
        return true;
    };
    m_refToRootItem.visitTree(Path(), emptyChildrenVisitor, VisitOption::Default, openingVisitor,
                              closingVisitor, fieldsFilter());
    return popAndAssembleSymbolsFor(m_refToRootItem);
}

SymbolsList DocumentSymbolVisitor::popAndAssembleSymbolsFor(const DomItem &item)
{
    Q_ASSERT(!m_stackOfChildrenSymbols.empty());
    auto atEnd = qScopeGuard([this]() { m_stackOfChildrenSymbols.pop(); });
    return m_assemble(item, std::move(m_stackOfChildrenSymbols.top()));
}

void DocumentSymbolVisitor::appendToTop(const SymbolsList &symbols)
{
    Q_ASSERT(!m_stackOfChildrenSymbols.empty());
    m_stackOfChildrenSymbols.top().append(symbols);
}

SymbolsList assembleSymbolsForQmlFile(const DomItem &item, const AssemblingFunction af)
{
    Q_ASSERT(item.internalKind() == DomType::QmlFile);
    DocumentSymbolVisitor visitor(item, af);
    return visitor.assembleSymbols();
}
} // namespace DocumentSymbolUtils

QT_END_NAMESPACE
