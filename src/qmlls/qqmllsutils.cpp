// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qqmllsutils_p.h"

#include <QtLanguageServer/private/qlanguageserverspectypes_p.h>
#include <QtCore/qthreadpool.h>
#include <QtCore/private/qduplicatetracker_p.h>
#include <QtCore/QRegularExpression>
#include <QtQmlDom/private/qqmldomexternalitems_p.h>
#include <QtQmlDom/private/qqmldomtop_p.h>
#include <QtQmlDom/private/qqmldomscriptelements_p.h>
#include <QtQmlDom/private/qqmldom_utils_p.h>
#include <memory>
#include <optional>
#include <stack>
#include <type_traits>
#include <utility>
#include <variant>

using namespace QLspSpecification;
using namespace QQmlJS::Dom;
using namespace Qt::StringLiterals;

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(QQmlLSUtilsLog, "qt.languageserver.utils")

/*!
   \internal
    Helper to check if item is a Field Member Expression \c {<someExpression>.propertyName}.
*/
static bool isFieldMemberExpression(DomItem &item)
{
    return item.internalKind() == DomType::ScriptBinaryExpression
            && item.field(Fields::operation).value().toInteger()
            == ScriptElements::BinaryExpression::FieldMemberAccess;
}

/*!
   \internal
    Helper to check if item is a Field Member Access \c memberAccess in
    \c {<someExpression>.memberAccess}.
*/
static bool isFieldMemberAccess(DomItem &item)
{
    auto parent = item.directParent();
    if (!isFieldMemberExpression(parent))
        return false;

    DomItem rightHandSide = parent.field(Fields::right);
    return item == rightHandSide;
}

/*!
   \internal
   The language server protocol calls "URI" what QML calls "URL".
   According to RFC 3986, a URL is a special case of URI that not only
   identifies a resource but also shows how to access it.
   In QML, however, URIs are distinct from URLs. URIs are the
   identifiers of modules, for example "QtQuick.Controls".
   In order to not confuse the terms we interpret language server URIs
   as URLs in the QML code model.
   This method marks a point of translation between the terms, but does
   not have to change the actual URI/URL.

   \sa QQmlLSUtils::qmlUriToLspUrl
 */
QByteArray QQmlLSUtils::lspUriToQmlUrl(const QByteArray &uri)
{
    return uri;
}

QByteArray QQmlLSUtils::qmlUrlToLspUri(const QByteArray &url)
{
    return url;
}

/*!
   \internal
   \brief Converts a QQmlJS::SourceLocation to a LSP Range.

   QQmlJS::SourceLocation starts counting lines and rows at 1, but the LSP Range starts at 0.
   Also, the QQmlJS::SourceLocation contains startLine, startColumn and length while the LSP Range
   contains startLine, startColumn, endLine and endColumn, which must be computed from the actual
   qml code.
 */
QLspSpecification::Range QQmlLSUtils::qmlLocationToLspLocation(const QString &code,
                                                               QQmlJS::SourceLocation qmlLocation)
{
    Range range;

    range.start.line = qmlLocation.startLine - 1;
    range.start.character = qmlLocation.startColumn - 1;

    auto end = QQmlLSUtils::textRowAndColumnFrom(code, qmlLocation.end());
    range.end.line = end.line;
    range.end.character = end.character;
    return range;
}

/*!
   \internal
   \brief Convert a text position from (line, column) into an offset.

   Row, Column and the offset are all 0-based.
   For example, \c{s[textOffsetFrom(s, 5, 55)]} returns the character of s at line 5 and column 55.

   \sa QQmlLSUtils::textRowAndColumnFrom
*/
qsizetype QQmlLSUtils::textOffsetFrom(const QString &text, int row, int column)
{
    int targetLine = row;
    qsizetype i = 0;
    while (i != text.size() && targetLine != 0) {
        QChar c = text.at(i++);
        if (c == u'\n') {
            --targetLine;
        }
        if (c == u'\r') {
            if (i != text.size() && text.at(i) == u'\n')
                ++i;
            --targetLine;
        }
    }
    qsizetype leftChars = column;
    while (i != text.size() && leftChars) {
        QChar c = text.at(i);
        if (c == u'\n' || c == u'\r')
            break;
        ++i;
        if (!c.isLowSurrogate())
            --leftChars;
    }
    return i;
}

/*!
   \internal
   \brief Convert a text position from an offset into (line, column).

   Row, Column and the offset are all 0-based.
   For example, \c{textRowAndColumnFrom(s, 55)} returns the line and columns of the
   character at \c {s[55]}.

   \sa QQmlLSUtils::textOffsetFrom
*/
QQmlLSUtilsTextPosition QQmlLSUtils::textRowAndColumnFrom(const QString &text, qsizetype offset)
{
    int row = 0;
    int column = 0;
    qsizetype currentLineOffset = 0;
    for (qsizetype i = 0; i < offset; i++) {
        QChar c = text[i];
        if (c == u'\n') {
            row++;
            currentLineOffset = i + 1;
        } else if (c == u'\r') {
            if (i > 0 && text[i - 1] == u'\n')
                currentLineOffset++;
        }
    }
    column = offset - currentLineOffset;

    return { row, column };
}

/*!
    \internal
    \brief Find the DomItem representing the object situated in file at given line and
   character/column.

    If line and character point between two objects, two objects might be returned.
    If line and character point to whitespace, it might return an inner node of the QmlDom-Tree.
 */
QList<QQmlLSUtilsItemLocation> QQmlLSUtils::itemsFromTextLocation(DomItem file, int line,
                                                                  int character)
{
    QList<QQmlLSUtilsItemLocation> itemsFound;
    std::shared_ptr<QmlFile> filePtr = file.ownerAs<QmlFile>();
    if (!filePtr)
        return itemsFound;
    FileLocations::Tree t = filePtr->fileLocationsTree();
    Q_ASSERT(t);
    QString code = filePtr->code(); // do something more advanced wrt to changes wrt to this->code?
    QList<QQmlLSUtilsItemLocation> toDo;
    qsizetype targetPos = textOffsetFrom(code, line, character);
    Q_ASSERT(targetPos >= 0);
    auto containsTarget = [targetPos](QQmlJS::SourceLocation l) {
        if constexpr (sizeof(qsizetype) <= sizeof(quint32)) {
            return l.begin() <= quint32(targetPos) && quint32(targetPos) <= l.end();
        } else {
            return l.begin() <= targetPos && targetPos <= l.end();
        }
    };
    if (containsTarget(t->info().fullRegion)) {
        QQmlLSUtilsItemLocation loc;
        loc.domItem = file;
        loc.fileLocation = t;
        toDo.append(loc);
    }
    while (!toDo.isEmpty()) {
        QQmlLSUtilsItemLocation iLoc = toDo.last();
        toDo.removeLast();

        bool inParentButOutsideChildren = true;

        auto subEls = iLoc.fileLocation->subItems();
        for (auto it = subEls.begin(); it != subEls.end(); ++it) {
            auto subLoc = std::static_pointer_cast<AttachedInfoT<FileLocations>>(it.value());
            Q_ASSERT(subLoc);

            if (containsTarget(subLoc->info().fullRegion)) {
                QQmlLSUtilsItemLocation subItem;
                subItem.domItem = iLoc.domItem.path(it.key());
                if (!subItem.domItem) {
                    qCDebug(QQmlLSUtilsLog)
                            << "A DomItem child is missing or the FileLocationsTree structure does "
                               "not follow the DomItem Structure.";
                    continue;
                }
                subItem.fileLocation = subLoc;
                toDo.append(subItem);
                inParentButOutsideChildren = false;
            }
        }
        if (inParentButOutsideChildren) {
            itemsFound.append(iLoc);
        }
    }

    // filtering step:
    if (itemsFound.size() > 1) {
        // if there are multiple items, take the smallest one + its neighbors
        // this allows to prefer inline components over main components, when both contain the
        // current textposition, and to disregard internal structures like property maps, which
        // "contain" everything from their first-appearing to last-appearing property (e.g. also
        // other stuff in between those two properties).
        auto smallest = std::min_element(
                itemsFound.begin(), itemsFound.end(),
                [](const QQmlLSUtilsItemLocation &a, const QQmlLSUtilsItemLocation &b) {
                    return a.fileLocation->info().fullRegion.length
                            < b.fileLocation->info().fullRegion.length;
                });
        QList<QQmlLSUtilsItemLocation> filteredItems;
        filteredItems.append(*smallest);

        const QQmlJS::SourceLocation smallestLoc = smallest->fileLocation->info().fullRegion;
        const quint32 smallestBegin = smallestLoc.begin();
        const quint32 smallestEnd = smallestLoc.end();

        for (auto it = itemsFound.begin(); it != itemsFound.end(); it++) {
            if (it == smallest)
                continue;

            const QQmlJS::SourceLocation itLoc = it->fileLocation->info().fullRegion;
            const quint32 itBegin = itLoc.begin();
            const quint32 itEnd = itLoc.end();
            if (itBegin == smallestEnd || smallestBegin == itEnd) {
                filteredItems.append(*it);
            }
        }
        itemsFound = filteredItems;
    }
    return itemsFound;
}

DomItem QQmlLSUtils::baseObject(DomItem object)
{
    if (!object.as<QmlObject>())
        return {};

    auto prototypes = object.field(QQmlJS::Dom::Fields::prototypes);
    switch (prototypes.indexes()) {
    case 0:
        return {};
    case 1:
        break;
    default:
        qDebug() << "Multiple prototypes found for " << object.name() << ", taking the first one.";
        break;
    }
    QQmlJS::Dom::DomItem base = prototypes.index(0).proceedToScope();
    return base;
}

/*!
   \internal
   \brief Extracts the QML object type of an \l DomItem.

   For a \c PropertyDefinition, return the type of the property.
   For a \c Binding, return the bound item's type if an QmlObject is bound, and otherwise the type
   of the property.
   For a \c QmlObject, do nothing and return it.
   For an \c Id, return the object to
   which the id resolves.
   For a \c Methodparameter, return the type of the parameter. =
   Otherwise, return an empty item.
 */
DomItem QQmlLSUtils::findTypeDefinitionOf(DomItem object)
{
    DomItem result;

    switch (object.internalKind()) {
    case QQmlJS::Dom::DomType::QmlComponent:
        result = object.field(Fields::objects).index(0);
        break;
    case QQmlJS::Dom::DomType::QmlObject:
        result = baseObject(object);
        break;
    case QQmlJS::Dom::DomType::Binding: {
        auto binding = object.as<Binding>();
        Q_ASSERT(binding);

        // try to grab the type from the bound object
        if (binding->valueKind() == BindingValueKind::Object) {
            result = baseObject(object.field(Fields::value));
        } else {
            // use the type of the property it is bound on for scriptexpression etc.
            DomItem propertyDefinition;
            const QString bindingName = binding->name();
            object.containingObject().visitLookup(
                    bindingName,
                    [&propertyDefinition](DomItem &item) {
                        if (item.internalKind() == QQmlJS::Dom::DomType::PropertyDefinition) {
                            propertyDefinition = item;
                            return false;
                        }
                        return true;
                    },
                    LookupType::PropertyDef);
            result = propertyDefinition.field(Fields::type).proceedToScope();
        }
        break;
    }
    case QQmlJS::Dom::DomType::Id:
        result = object.field(Fields::referredObject).proceedToScope();
        break;
    case QQmlJS::Dom::DomType::PropertyDefinition:
    case QQmlJS::Dom::DomType::MethodParameter:
    case QQmlJS::Dom::DomType::MethodInfo:
        result = object.field(Fields::type).proceedToScope();
        break;
    case QQmlJS::Dom::DomType::ScriptIdentifierExpression: {
        if (object.directParent().internalKind() == DomType::ScriptType) {
            DomItem type =
                    object.filterUp([](DomType k, DomItem &) { return k == DomType::ScriptType; },
                                    FilterUpOptions::ReturnOuter);

            const QString name = type.field(Fields::typeName).value().toString();
            result = object.path(Paths::lookupTypePath(name));
            break;
        }
        auto scope =
                QQmlLSUtils::resolveExpressionType(object, QQmlLSUtilsResolveOptions::Everything);
        if (!scope)
            return {};

        return QQmlLSUtils::sourceLocationToDomItem(object.containingFile(),
                                                    scope->sourceLocation());
    }
    case QQmlJS::Dom::DomType::Empty:
        break;
    default:
        qDebug() << "QQmlLSUtils::findTypeDefinitionOf: Found unimplemented Type"
                 << object.internalKindStr();
        result = {};
    }

    return result;
}

static DomItem findJSIdentifierDefinition(DomItem item, const QString &name)
{
    DomItem definitionOfItem;
    item.visitUp([&name, &definitionOfItem](DomItem &i) {
        if (std::optional<QQmlJSScope::Ptr> scope = i.semanticScope(); scope) {
            qCDebug(QQmlLSUtilsLog) << "Searching for definition in" << i.internalKindStr();
            if (auto jsIdentifier = scope.value()->JSIdentifier(name)) {
                qCDebug(QQmlLSUtilsLog) << "Found scope" << scope.value()->baseTypeName();
                definitionOfItem = i;
                return false;
            }
        }
        // early exit: no JS definitions/usages outside the ScriptExpression DOM element.
        if (i.internalKind() == DomType::ScriptExpression)
            return false;
        return true;
    });

    return definitionOfItem;
}

static void findUsagesOfPropertiesAndIds(DomItem item, const QString &name,
                                         QList<QQmlLSUtilsLocation> &result)
{
    QQmlJSScope::ConstPtr targetType;
    targetType = QQmlLSUtils::resolveExpressionType(item, QQmlLSUtilsResolveOptions::JustOwner);

    auto findUsages = [&targetType, &result, &name](Path, DomItem &current, bool) -> bool {
        bool resolveType = false;
        bool continueForChildren = true;
        DomItem toBeResolved = current;

        if (auto scope = current.semanticScope()) {
            // is the current property shadowed by some JS identifier? ignore current + its children
            if (scope.value()->JSIdentifier(name)) {
                return false;
            }
        }

        switch (current.internalKind()) {
        case DomType::PropertyDefinition: {
            const QString propertyName = current.field(Fields::name).value().toString();
            if (name == propertyName) {
                resolveType = true;
            } else {
                return true;
            }
            break;
        }
        case DomType::ScriptIdentifierExpression: {
            const QString propertyName = current.field(Fields::identifier).value().toString();
            if (name != propertyName)
                return true;

            resolveType = true;
            break;
        }
        default:
            break;
        };

        if (resolveType) {
            auto currentType = QQmlLSUtils::resolveExpressionType(
                    toBeResolved, QQmlLSUtilsResolveOptions::JustOwner);
            qCDebug(QQmlLSUtilsLog) << "Will resolve type of" << toBeResolved.internalKindStr();
            if (currentType == targetType) {
                auto tree = FileLocations::treeOf(current);
                QQmlLSUtilsLocation location{ current.canonicalFilePath(),
                                              tree->info().fullRegion };
                result.append(location);
            }
        }
        return continueForChildren;
    };

    item.containingFile()
            .field(Fields::components)
            .visitTree(Path(), emptyChildrenVisitor, VisitOption::Recurse | VisitOption::VisitSelf,
                       findUsages);
}

static void findUsagesHelper(DomItem item, const QString &name, QList<QQmlLSUtilsLocation> &result)
{
    qCDebug(QQmlLSUtilsLog) << "Looking for JS identifier with name" << name;
    DomItem definitionOfItem = findJSIdentifierDefinition(item, name);

    // if there is no definition found: check if name was a property or an id instead
    if (!definitionOfItem) {
        qCDebug(QQmlLSUtilsLog) << "No defining JS-Scope found!";
        findUsagesOfPropertiesAndIds(item, name, result);
        return;
    }

    definitionOfItem.visitTree(
            Path(), emptyChildrenVisitor, VisitOption::VisitAdopted | VisitOption::Recurse,
            [&name, &result](Path, DomItem &item, bool) -> bool {
                qCDebug(QQmlLSUtilsLog) << "Visiting a " << item.internalKindStr();
                if (item.internalKind() == DomType::ScriptIdentifierExpression
                    && item.field(Fields::identifier).value().toString() == name) {
                    // add this usage
                    auto fileLocation = FileLocations::treeOf(item);
                    if (!fileLocation) {
                        qCWarning(QQmlLSUtilsLog) << "Failed finding filelocation of found usage";
                        return true;
                    }
                    const QQmlJS::SourceLocation location = fileLocation->info().fullRegion;
                    const QString fileName = item.canonicalFilePath();
                    result.append({ fileName, location });
                    return true;
                } else if (std::optional<QQmlJSScope::Ptr> scope = item.semanticScope();
                           scope && scope.value()->JSIdentifier(name)) {
                    // current JS identifier has been redefined, do not visit children
                    return false;
                }
                return true;
            });
}

QList<QQmlLSUtilsLocation> QQmlLSUtils::findUsagesOf(DomItem item)
{
    QList<QQmlLSUtilsLocation> result;

    switch (item.internalKind()) {
    case DomType::ScriptIdentifierExpression: {
        const QString name = item.field(Fields::identifier).value().toString();
        findUsagesHelper(item, name, result);
        break;
    }
    case DomType::ScriptVariableDeclarationEntry: {
        const QString name = item.field(Fields::identifier).value().toString();
        findUsagesHelper(item, name, result);
        break;
    }
    case DomType::PropertyDefinition: {
        const QString name = item.field(Fields::name).value().toString();
        findUsagesHelper(item, name, result);
        break;
    }
    default:
        qCDebug(QQmlLSUtilsLog) << item.internalKindStr()
                                << "was not implemented for QQmlLSUtils::findUsagesOf";
        return result;
    }

    std::sort(result.begin(), result.end());

    if (QQmlLSUtilsLog().isDebugEnabled()) {
        qCDebug(QQmlLSUtilsLog) << "Found following usages:";
        for (auto r : result) {
            qCDebug(QQmlLSUtilsLog)
                    << r.filename << " @ " << r.location.startLine << ":" << r.location.startColumn
                    << " with length " << r.location.length;
        }
    }

    return result;
}

static QQmlJSScope::ConstPtr findPropertyIn(const QQmlJSScope::Ptr &referrerScope,
                                            const QString &propertyName,
                                            QQmlLSUtilsResolveOptions options)
{
    for (QQmlJSScope::Ptr current = referrerScope; current; current = current->parentScope()) {
        if (auto property = current->property(propertyName); property.isValid()) {
            switch (options) {
            case JustOwner:
                return current;
            case Everything:
                return property.type();
            }
        }
    }
    return {};
}

/*!
   \internal
    Resolves the type of the given DomItem, when possible (e.g., when there are enough type
    annotations).
*/
QQmlJSScope::ConstPtr QQmlLSUtils::resolveExpressionType(QQmlJS::Dom::DomItem item,
                                                         QQmlLSUtilsResolveOptions options)
{
    switch (item.internalKind()) {
    case DomType::ScriptIdentifierExpression: {
        auto referrerScope = item.nearestSemanticScope();
        if (!referrerScope)
            return {};

        const QString name = item.field(Fields::identifier).value().toString();

        if (isFieldMemberAccess(item)) {
            DomItem parent = item.directParent();
            auto owner = QQmlLSUtils::resolveExpressionType(parent.field(Fields::left),
                                                            QQmlLSUtilsResolveOptions::Everything);
            if (owner) {
                if (auto property = owner->property(name); property.isValid()) {
                    switch (options) {
                    case JustOwner:
                        return owner;
                    case Everything:
                        return property.type();
                    }
                }
            } else {
                return {};
            }
        } else {
            DomItem definitionOfItem = findJSIdentifierDefinition(item, name);
            if (definitionOfItem) {
                Q_ASSERT_X(definitionOfItem.semanticScope().has_value()
                                   && definitionOfItem.semanticScope()
                                              .value()
                                              ->JSIdentifier(name)
                                              .has_value(),
                           "QQmlLSUtils::findDefinitionOf",
                           "JS definition does not actually define the JS identifer. "
                           "It should be empty.");
                auto scope = definitionOfItem.semanticScope().value()->JSIdentifier(name)->scope.toStrongRef();;
                return scope;
            }

            // check if its an (unqualified) property
            if (QQmlJSScope::ConstPtr scope = findPropertyIn(*referrerScope, name, options)) {
                return scope;
            }
        }

        // check if its an id
        auto resolver = item.containingFile().ownerAs<QmlFile>()->typeResolver();
        if (!resolver)
            return {};
        QQmlJSScope::ConstPtr fromId = resolver.value()->scopeForId(name, referrerScope.value());
        if (fromId)
            return fromId;

        return {};
    }
    case DomType::PropertyDefinition: {
        auto propertyDefinition = item.as<PropertyDefinition>();
        if (propertyDefinition && propertyDefinition->scope) {
            switch (options) {
            case JustOwner:
                return propertyDefinition->scope.value();
            case Everything:
                return propertyDefinition->scope.value()->property(propertyDefinition->name).type();
            }
        }

        return {};
    }
    case DomType::QmlObject: {
        auto object = item.as<QmlObject>();
        if (object && object->semanticScope())
            return object->semanticScope().value();

        return {};
    }
    case DomType::ScriptBinaryExpression: {
        if (isFieldMemberExpression(item)) {
            DomItem owner = item.field(Fields::left);
            QString propertyName =
                    item.field(Fields::right).field(Fields::identifier).value().toString();
            auto ownerType = QQmlLSUtils::resolveExpressionType(
                    owner, QQmlLSUtilsResolveOptions::Everything);
            if (!ownerType)
                return ownerType;
            switch (options) {
            case JustOwner:
                return ownerType;
            case Everything:
                return ownerType->property(propertyName).type();
            }
        }
        return {};
    }
    default: {
        qCDebug(QQmlLSUtilsLog) << "Type" << item.internalKindStr()
                                << "is unimplemented in QQmlLSUtils::resolveExpressionType";
        return {};
    }
    }
    Q_UNREACHABLE();
}

DomItem QQmlLSUtils::sourceLocationToDomItem(DomItem file, const QQmlJS::SourceLocation &location)
{
    // QQmlJS::SourceLocation starts counting at 1 but the utils and the LSP start at 0.
    auto items = QQmlLSUtils::itemsFromTextLocation(file, location.startLine - 1,
                                                    location.startColumn - 1);
    switch (items.size()) {
    case 0:
        return {};
    case 1:
        return items.front().domItem;
    case 2: {
        // special case: because location points to the beginning of the type definition,
        // itemsFromTextLocation might also return the type on its left, in case it is directly
        // adjacent to it. In this case always take the right (=with the higher column-number)
        // item.
        auto &first = items.front();
        auto &second = items.back();
        Q_ASSERT_X(first.fileLocation->info().fullRegion.startLine
                           == second.fileLocation->info().fullRegion.startLine,
                   "QQmlLSUtils::findTypeDefinitionOf(DomItem)",
                   "QQmlLSUtils::itemsFromTextLocation returned non-adjacent items.");
        if (first.fileLocation->info().fullRegion.startColumn
            > second.fileLocation->info().fullRegion.startColumn)
            return first.domItem;
        else
            return second.domItem;
        break;
    }
    default:
        qDebug() << "Found multiple candidates for type of scriptidentifierexpression";
        break;
    }
    return {};
}

std::optional<QQmlLSUtilsLocation>
findPropertyDefinitionOf(DomItem file, QQmlJS::SourceLocation propertyDefinitionLocation,
                         const QString &name)
{
    DomItem propertyOwner = QQmlLSUtils::sourceLocationToDomItem(file, propertyDefinitionLocation);
    DomItem propertyDefinition = propertyOwner.field(Fields::propertyDefs).key(name).index(0);
    if (!propertyDefinition)
        return {};

    QQmlLSUtilsLocation result;
    result.location = FileLocations::treeOf(propertyDefinition)->info().fullRegion;
    result.filename = propertyDefinition.canonicalFilePath();
    return result;
}

std::optional<QQmlLSUtilsLocation> QQmlLSUtils::findDefinitionOf(DomItem item)
{

    switch (item.internalKind()) {
    case QQmlJS::Dom::DomType::ScriptIdentifierExpression: {
        // first check if its a JS Identifier

        const QString name = item.value().toString();
        if (isFieldMemberAccess(item)) {
            if (auto ownerScope = QQmlLSUtils::resolveExpressionType(
                        item, QQmlLSUtilsResolveOptions::JustOwner)) {
                const DomItem ownerFile = item.goToFile(ownerScope->filePath());
                const QQmlJS::SourceLocation ownerLocation = ownerScope->sourceLocation();
                if (auto propertyDefinition =
                            findPropertyDefinitionOf(ownerFile, ownerLocation, name)) {
                    return propertyDefinition;
                }
            }
            return {};
        }

        // check: is it a JS identifier?
        if (DomItem definitionOfItem = findJSIdentifierDefinition(item, name)) {
            Q_ASSERT_X(definitionOfItem.semanticScope().has_value()
                               && definitionOfItem.semanticScope()
                                          .value()
                                          ->JSIdentifier(name)
                                          .has_value(),
                       "QQmlLSUtils::findDefinitionOf",
                       "JS definition does not actually define the JS identifer. "
                       "It should be empty.");
            QQmlJS::SourceLocation location =
                    definitionOfItem.semanticScope().value()->JSIdentifier(name).value().location;

            QQmlLSUtilsLocation result = { definitionOfItem.canonicalFilePath(), location };
            return result;
        }

        // not a JS identifier, check for ids and properties
        auto referrerScope = item.nearestSemanticScope();
        if (!referrerScope)
            return {};

        if (QQmlJSScope::ConstPtr scope =
                    findPropertyIn(*referrerScope, name, QQmlLSUtilsResolveOptions::JustOwner)) {
            const QString canonicalPath = scope->filePath();
            qDebug() << canonicalPath;
            DomItem file = item.goToFile(canonicalPath);
            return findPropertyDefinitionOf(file, scope->sourceLocation(), name);
        }

        // check if its an id
        auto resolver = item.containingFile().ownerAs<QmlFile>()->typeResolver();
        if (!resolver)
            return {};
        QQmlJSScope::ConstPtr fromId = resolver.value()->scopeForId(name, referrerScope.value());
        if (fromId) {
            QQmlLSUtilsLocation result;
            result.location = fromId->sourceLocation();
            result.filename = item.canonicalFilePath();
            return result;
        }
        return {};
    }
    default:
        qDebug() << "QQmlLSUtils::findDefinitionOf: Found unimplemented Type "
                 << item.internalKindStr();
        return {};
    }

    Q_UNREACHABLE_RETURN(std::nullopt);
}

QT_END_NAMESPACE
