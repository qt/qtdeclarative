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
#include <QtQml/private/qqmlsignalnames_p.h>
#include <QtQml/private/qqmljslexer_p.h>
#include <QtQmlCompiler/private/qqmljsutils_p.h>

#include <algorithm>
#include <iterator>
#include <memory>
#include <optional>
#include <set>
#include <stack>
#include <type_traits>
#include <utility>
#include <variant>

using namespace QLspSpecification;
using namespace QQmlJS::Dom;
using namespace Qt::StringLiterals;

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(QQmlLSUtilsLog, "qt.languageserver.utils")
Q_LOGGING_CATEGORY(QQmlLSCompletionLog, "qt.languageserver.completions")

static QList<CompletionItem> methodCompletion(const QQmlJSScope::ConstPtr &scope,
                                              QDuplicateTracker<QString> *usedNames);
static QList<CompletionItem> propertyCompletion(const QQmlJSScope::ConstPtr &scope,
                                                QDuplicateTracker<QString> *usedNames);
/*!
   \internal
    Helper to check if item is a Field Member Expression \c {<someExpression>.propertyName}.
*/
static bool isFieldMemberExpression(const DomItem &item)
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
static bool isFieldMemberAccess(const DomItem &item)
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

static QList<QQmlLSUtilsItemLocation>::const_iterator
handlePropertyDefinitionAndBindingOverlap(const QList<QQmlLSUtilsItemLocation> &items,
                                          qsizetype offsetInFile)
{
    auto smallest = std::min_element(
            items.begin(), items.end(),
            [](const QQmlLSUtilsItemLocation &a, const QQmlLSUtilsItemLocation &b) {
                return a.fileLocation->info().fullRegion.length
                        < b.fileLocation->info().fullRegion.length;
            });

    if (smallest->domItem.internalKind() == DomType::Binding) {
        // weird edge case: the filelocations of property definitions and property bindings are
        // actually overlapping, which means that qmlls cannot distinguish between bindings and
        // bindings in property definitions. Those need to be treated differently for
        // autocompletion, for example.
        // Therefore: when inside a binding and a propertydefinition, choose the property definition
        // if offsetInFile is before the colon, like for example:
        // property var helloProperty: Rectangle { /*...*/ }
        // |----return propertydef---|-- return Binding ---|

        // get the smallest property definition to avoid getting the property definition that the
        // current QmlObject is getting bound to!
        auto smallestPropertyDefinition = std::min_element(
                items.begin(), items.end(),
                [](const QQmlLSUtilsItemLocation &a, const QQmlLSUtilsItemLocation &b) {
                    // make property definition smaller to avoid getting smaller items that are not
                    // property definitions
                    const bool aIsPropertyDefinition =
                            a.domItem.internalKind() == DomType::PropertyDefinition;
                    const bool bIsPropertyDefinition =
                            b.domItem.internalKind() == DomType::PropertyDefinition;
                    return aIsPropertyDefinition > bIsPropertyDefinition
                            && a.fileLocation->info().fullRegion.length
                            < b.fileLocation->info().fullRegion.length;
                });

        if (smallestPropertyDefinition->domItem.internalKind() != DomType::PropertyDefinition)
            return smallest;

        const auto propertyDefinitionColon =
                smallestPropertyDefinition->fileLocation->info().regions[ColonTokenRegion];
        const auto smallestColon = smallest->fileLocation->info().regions[ColonTokenRegion];
        // sanity check: is it the definition of the current binding? check if they both have their
        // ':' at the same location
        if (propertyDefinitionColon.isValid() && propertyDefinitionColon == smallestColon
            && offsetInFile < smallestColon.offset) {
            return smallestPropertyDefinition;
        }
    }
    return smallest;
}

static QList<QQmlLSUtilsItemLocation>
filterItemsFromTextLocation(const QList<QQmlLSUtilsItemLocation> &items, qsizetype offsetInFile)
{
    if (items.size() < 2)
        return items;

    // if there are multiple items, take the smallest one + its neighbors
    // this allows to prefer inline components over main components, when both contain the
    // current textposition, and to disregard internal structures like property maps, which
    // "contain" everything from their first-appearing to last-appearing property (e.g. also
    // other stuff in between those two properties).

    QList<QQmlLSUtilsItemLocation> filteredItems;

    auto smallest = handlePropertyDefinitionAndBindingOverlap(items, offsetInFile);

    filteredItems.append(*smallest);

    const QQmlJS::SourceLocation smallestLoc = smallest->fileLocation->info().fullRegion;
    const quint32 smallestBegin = smallestLoc.begin();
    const quint32 smallestEnd = smallestLoc.end();

    for (auto it = items.begin(); it != items.end(); it++) {
        if (it == smallest)
            continue;

        const QQmlJS::SourceLocation itLoc = it->fileLocation->info().fullRegion;
        const quint32 itBegin = itLoc.begin();
        const quint32 itEnd = itLoc.end();
        if (itBegin == smallestEnd || smallestBegin == itEnd) {
            filteredItems.append(*it);
        }
    }
    return filteredItems;
}

/*!
    \internal
    \brief Find the DomItem representing the object situated in file at given line and
   character/column.

    If line and character point between two objects, two objects might be returned.
    If line and character point to whitespace, it might return an inner node of the QmlDom-Tree.
 */
QList<QQmlLSUtilsItemLocation> QQmlLSUtils::itemsFromTextLocation(const DomItem &file, int line,
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
    auto filtered = filterItemsFromTextLocation(itemsFound, targetPos);
    return filtered;
}

DomItem QQmlLSUtils::baseObject(const DomItem &object)
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

static std::optional<QQmlLSUtilsLocation> locationFromDomItem(const DomItem &item,
                                                              FileLocationRegion region)
{
    QQmlLSUtilsLocation location;
    location.filename = item.canonicalFilePath();

    auto tree = FileLocations::treeOf(item);
    // tree is null for C++ defined types, for example
    if (!tree)
        return {};

    location.sourceLocation = FileLocations::region(tree, region);
    if (!location.sourceLocation.isValid() && region != QQmlJS::Dom::MainRegion)
        location.sourceLocation = FileLocations::region(tree, MainRegion);
    return location;
}

/*!
   \internal
   \brief Returns the location of the type definition pointed by object.

   For a \c PropertyDefinition, return the location of the type of the property.
   For a \c Binding, return the bound item's type location if an QmlObject is bound, and otherwise
   the type of the property.
   For a \c QmlObject, return the location of the QmlObject's base.
   For an \c Id, return the location of the object to which the id resolves.
   For a \c Methodparameter, return the location of the type of the parameter.
   Otherwise, return std::nullopt.
 */
std::optional<QQmlLSUtilsLocation> QQmlLSUtils::findTypeDefinitionOf(const DomItem &object)
{
    DomItem typeDefinition;

    switch (object.internalKind()) {
    case QQmlJS::Dom::DomType::QmlComponent:
        typeDefinition = object.field(Fields::objects).index(0);
        break;
    case QQmlJS::Dom::DomType::QmlObject:
        typeDefinition = baseObject(object);
        break;
    case QQmlJS::Dom::DomType::Binding: {
        auto binding = object.as<Binding>();
        Q_ASSERT(binding);

        // try to grab the type from the bound object
        if (binding->valueKind() == BindingValueKind::Object) {
            typeDefinition = baseObject(object.field(Fields::value));
            break;
        } else {
            // use the type of the property it is bound on for scriptexpression etc.
            DomItem propertyDefinition;
            const QString bindingName = binding->name();
            object.containingObject().visitLookup(
                    bindingName,
                    [&propertyDefinition](const DomItem &item) {
                        if (item.internalKind() == QQmlJS::Dom::DomType::PropertyDefinition) {
                            propertyDefinition = item;
                            return false;
                        }
                        return true;
                    },
                    LookupType::PropertyDef);
            typeDefinition = propertyDefinition.field(Fields::type).proceedToScope();
            break;
        }
        Q_UNREACHABLE();
    }
    case QQmlJS::Dom::DomType::Id:
        typeDefinition = object.field(Fields::referredObject).proceedToScope();
        break;
    case QQmlJS::Dom::DomType::PropertyDefinition:
    case QQmlJS::Dom::DomType::MethodParameter:
    case QQmlJS::Dom::DomType::MethodInfo:
        typeDefinition = object.field(Fields::type).proceedToScope();
        break;
    case QQmlJS::Dom::DomType::ScriptIdentifierExpression: {
        if (object.directParent().internalKind() == DomType::ScriptType) {
            DomItem type =
                    object.filterUp([](DomType k, const DomItem &) { return k == DomType::ScriptType; },
                                    FilterUpOptions::ReturnOuter);

            const QString name = type.field(Fields::typeName).value().toString();
            typeDefinition = object.path(Paths::lookupTypePath(name));
            break;
        }

        auto scope = QQmlLSUtils::resolveExpressionType(
                object, QQmlLSUtilsResolveOptions::ResolveActualTypeForFieldMemberExpression);
        if (!scope)
            return {};

        typeDefinition = QQmlLSUtils::sourceLocationToDomItem(
                object.containingFile(), scope->semanticScope->sourceLocation());

        switch (scope->type) {
        case QmlObjectIdIdentifier:
            break;
        default:
            typeDefinition = typeDefinition.component();
        }

        return locationFromDomItem(typeDefinition, FileLocationRegion::IdentifierRegion);
    }
    default:
        qDebug() << "QQmlLSUtils::findTypeDefinitionOf: Found unimplemented Type"
                 << object.internalKindStr();
        return {};
    }

    return locationFromDomItem(typeDefinition, FileLocationRegion::MainRegion);
}

static bool findDefinitionFromItem(const DomItem &item, const QString &name)
{
    if (const QQmlJSScope::ConstPtr &scope = item.semanticScope()) {
        qCDebug(QQmlLSUtilsLog) << "Searching for definition in" << item.internalKindStr();
        if (auto jsIdentifier = scope->ownJSIdentifier(name)) {
            qCDebug(QQmlLSUtilsLog) << "Found scope" << scope->baseTypeName();
            return true;
        }
    }
    return false;
}

static DomItem findJSIdentifierDefinition(const DomItem &item, const QString &name)
{
    DomItem definitionOfItem;
    item.visitUp([&name, &definitionOfItem](const DomItem &i) {
        if (findDefinitionFromItem(i, name)) {
            definitionOfItem = i;
            return false;
        }
        // early exit: no JS definitions/usages outside the ScriptExpression DOM element.
        if (i.internalKind() == DomType::ScriptExpression)
            return false;
        return true;
    });

    if (definitionOfItem)
        return definitionOfItem;

    // special case: somebody asks for usages of a function parameter from its definition
    // function parameters are defined in the method's scope
    if (DomItem res = item.filterUp([](DomType k, const DomItem &) { return k == DomType::MethodInfo; },
                                    FilterUpOptions::ReturnOuter)) {
        DomItem candidate = res.field(Fields::body).field(Fields::scriptElement);
        if (findDefinitionFromItem(candidate, name)) {
            return candidate;
        }
    }

    return definitionOfItem;
}

/*!
\internal
Represents a signal, signal handler, property, property changed signal or a property changed
handler.
 */
struct SignalOrProperty
{
    /*!
    \internal The name of the signal or property, independent of whether this is a changed signal
    or handler.
     */
    QString name;
    QQmlLSUtilsIdentifierType type;
};

/*!
\internal
\brief Find out if \c{name} is a signal, signal handler, property, property changed signal, or a
property changed handler in the given QQmlJSScope.

Heuristic to find if name is a property, property changed signal, .... because those can appear
under different names, for example \c{mySignal} and \c{onMySignal} for a signal.
This will give incorrect results as soon as properties/signals/methods are called \c{onMySignal},
\c{on<some already existing property>Changed}, ..., but the good news is that the engine also
will act weird in these cases (e.g. one cannot bind to a property called like an already existing
signal or a property changed handler).
For future reference: you can always add additional checks to check the existence of those buggy
properties/signals/methods by looking if they exist in the QQmlJSScope.
*/
static std::optional<SignalOrProperty> resolveNameInQmlScope(const QString &name,
                                                             const QQmlJSScope::ConstPtr &owner)
{
    if (owner->hasProperty(name)) {
        return SignalOrProperty{ name, PropertyIdentifier };
    }

    if (const auto propertyName = QQmlSignalNames::changedHandlerNameToPropertyName(name)) {
        if (owner->hasProperty(*propertyName)) {
            return SignalOrProperty{ *propertyName, PropertyChangedHandlerIdentifier };
        }
    }

    if (const auto signalName = QQmlSignalNames::handlerNameToSignalName(name)) {
        if (auto methods = owner->methods(*signalName); !methods.isEmpty()) {
            if (methods.front().methodType() == QQmlJSMetaMethodType::Signal) {
                return SignalOrProperty{ *signalName, SignalHandlerIdentifier };
            }
        }
    }

    if (const auto propertyName = QQmlSignalNames::changedSignalNameToPropertyName(name)) {
        if (owner->hasProperty(*propertyName)) {
            return SignalOrProperty{ *propertyName, PropertyChangedSignalIdentifier };
        }
    }

    if (auto methods = owner->methods(name); !methods.isEmpty()) {
        if (methods.front().methodType() == QQmlJSMetaMethodType::Signal) {
            return SignalOrProperty{ name, SignalIdentifier };
        }
    }
    return std::nullopt;
}

/*!
\internal
Returns a list of names, that when belonging to the same targetType, should be considered equal.
This is used to find signal handlers as usages of their corresponding signals, for example.
*/
static QStringList namesOfPossibleUsages(const QString &name,
                                         const QQmlJSScope::ConstPtr &targetType)
{
    QStringList namesToCheck = { name };

    auto namings = resolveNameInQmlScope(name, targetType);
    if (!namings)
        return namesToCheck;
    switch (namings->type) {
    case PropertyIdentifier: {
        // for a property, also find bindings to its onPropertyChanged handler + propertyChanged
        // signal
        const QString propertyChangedHandler =
                QQmlSignalNames::propertyNameToChangedHandlerName(namings->name);
        namesToCheck.append(propertyChangedHandler);

        const QString propertyChangedSignal =
                QQmlSignalNames::propertyNameToChangedSignalName(namings->name);
        namesToCheck.append(propertyChangedSignal);
        break;
    }
    case PropertyChangedHandlerIdentifier: {
        // for a property changed handler, also find the usages of its property + propertyChanged
        // signal
        namesToCheck.append(namings->name);
        namesToCheck.append(QQmlSignalNames::propertyNameToChangedSignalName(namings->name));
        break;
    }
    case PropertyChangedSignalIdentifier: {
        // for a property changed signal, also find the usages of its property + onPropertyChanged
        // handlers
        namesToCheck.append(namings->name);
        namesToCheck.append(QQmlSignalNames::propertyNameToChangedHandlerName(namings->name));
        break;
    }
    case SignalIdentifier: {
        // for a signal, also find bindings to its onSignalHandler.
        namesToCheck.append(QQmlSignalNames::signalNameToHandlerName(namings->name));
        break;
    }
    case SignalHandlerIdentifier: {
        // for a signal handler, also find the usages of the signal it handles
        namesToCheck.append(namings->name);
        break;
    }
    default: {
        break;
    }
    }
    return namesToCheck;
}

/*! \internal
    \brief finds the scope that a property, method or enum is first defined.
    Starts looking for the name starting from the given scope and traverse
    through base and extension types.
*/
static QQmlJSScope::ConstPtr findDefiningScopeForNames(QQmlJSScope::ConstPtr referrerScope, const QStringList &namesToCheck)
{
    QQmlJSScope::ConstPtr result;
    QQmlJSUtils::searchBaseAndExtensionTypes(referrerScope, [&](QQmlJSScope::ConstPtr scope) {
        for (const auto &name : namesToCheck) {
            if (scope->hasOwnProperty(name) || scope->hasOwnMethod(name) || scope->hasOwnEnumeration(name)) {
                result = scope;
                return true;
            }
        }
        return false;
    });

    return result;
}

static void findUsagesOfNonJSIdentifiers(const DomItem &item, const QString &name,
                                         QList<QQmlLSUtilsLocation> &result)
{
    const auto expressionType = QQmlLSUtils::resolveExpressionType(item, ResolveOwnerType);
    if (!expressionType)
        return;

    const QStringList namesToCheck = namesOfPossibleUsages(name, expressionType->semanticScope);
    QQmlJSScope::ConstPtr targetType = findDefiningScopeForNames(expressionType->semanticScope, namesToCheck);

    const auto addLocationIfTypeMatchesTarget = [&result, &targetType, &namesToCheck](const DomItem &toBeResolved,
                                                                FileLocationRegion subRegion) {
        const auto currentType = QQmlLSUtils::resolveExpressionType(
                toBeResolved, QQmlLSUtilsResolveOptions::ResolveOwnerType);
        if (!currentType)
            return;

        const auto foundBaseType =
                findDefiningScopeForNames(currentType->semanticScope, namesToCheck);

        if (foundBaseType == targetType) {
            auto tree = FileLocations::treeOf(toBeResolved);
            QQmlJS::SourceLocation sourceLocation;

            sourceLocation = FileLocations::region(tree, subRegion);
            if (!sourceLocation.isValid())
                return;

            QQmlLSUtilsLocation location{ toBeResolved.canonicalFilePath(), sourceLocation };
            if (!result.contains(location))
                result.append(location);
        }
    };

    auto findUsages = [&addLocationIfTypeMatchesTarget, &name, &namesToCheck](Path, const DomItem &current,
                                                                bool) -> bool {
        bool continueForChildren = true;
        if (auto scope = current.semanticScope()) {
            // is the current property shadowed by some JS identifier? ignore current + its children
            if (scope->ownJSIdentifier(name)) {
                return false;
            }
        }
        switch (current.internalKind()) {
        case DomType::QmlObject:
        case DomType::Binding:
        case DomType::MethodInfo:
        case DomType::PropertyDefinition: {
            const QString propertyName = current.field(Fields::name).value().toString();
            if (namesToCheck.contains(propertyName))
                addLocationIfTypeMatchesTarget(current, IdentifierRegion);
            return continueForChildren;
        }
        case DomType::ScriptIdentifierExpression: {
            const QString identifierName = current.field(Fields::identifier).value().toString();
            if (namesToCheck.contains(identifierName))
                addLocationIfTypeMatchesTarget(current, MainRegion);
            return continueForChildren;
        }
        default:
            return continueForChildren;
        };

        Q_UNREACHABLE_RETURN(continueForChildren);;
    };

    item.containingFile()
            .field(Fields::components)
            .visitTree(Path(), emptyChildrenVisitor, VisitOption::Recurse | VisitOption::VisitSelf,
                       findUsages);
}

static QQmlLSUtilsLocation locationFromJSIdentifierDefinition(const DomItem &definitionOfItem,
                                                              const QString &name)
{
    Q_ASSERT_X(!definitionOfItem.semanticScope().isNull()
                       && definitionOfItem.semanticScope()->ownJSIdentifier(name).has_value(),
               "QQmlLSUtils::locationFromJSIdentifierDefinition",
               "JS definition does not actually define the JS identifier. "
               "Did you obtain definitionOfItem from findJSIdentifierDefinition() ?");
    QQmlJS::SourceLocation location =
            definitionOfItem.semanticScope()->ownJSIdentifier(name).value().location;

    QQmlLSUtilsLocation result = { definitionOfItem.canonicalFilePath(), location };
    return result;
}

static void findUsagesHelper(
        const DomItem &item, const QString &name, QList<QQmlLSUtilsLocation> &result)
{
    qCDebug(QQmlLSUtilsLog) << "Looking for JS identifier with name" << name;
    DomItem definitionOfItem = findJSIdentifierDefinition(item, name);

    // if there is no definition found: check if name was a property or an id instead
    if (!definitionOfItem) {
        qCDebug(QQmlLSUtilsLog) << "No defining JS-Scope found!";
        findUsagesOfNonJSIdentifiers(item, name, result);
        return;
    }

    definitionOfItem.visitTree(
            Path(), emptyChildrenVisitor, VisitOption::VisitAdopted | VisitOption::Recurse,
            [&name, &result](Path, const DomItem &item, bool) -> bool {
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
                } else if (QQmlJSScope::ConstPtr scope = item.semanticScope();
                           scope && scope->ownJSIdentifier(name)) {
                    // current JS identifier has been redefined, do not visit children
                    return false;
                }
                return true;
            });

    const QQmlLSUtilsLocation definition =
            locationFromJSIdentifierDefinition(definitionOfItem, name);
    if (!result.contains(definition))
        result.append(definition);
}

QList<QQmlLSUtilsLocation> QQmlLSUtils::findUsagesOf(const DomItem &item)
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
    case DomType::QmlObject:
    case DomType::PropertyDefinition:
    case DomType::Binding:
    case DomType::MethodInfo: {
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
                    << r.filename << " @ " << r.sourceLocation.startLine << ":"
                    << r.sourceLocation.startColumn << " with length " << r.sourceLocation.length;
        }
    }

    return result;
}

static std::optional<QQmlLSUtilsIdentifierType>
hasMethodOrSignal(const QQmlJSScope::ConstPtr &scope, const QString &name)
{
    auto methods = scope->methods(name);
    if (methods.isEmpty())
        return {};

    const bool isSignal = methods.front().methodType() == QQmlJSMetaMethodType::Signal;
    QQmlLSUtilsIdentifierType type = isSignal ? QQmlLSUtilsIdentifierType::SignalIdentifier
                                             : QQmlLSUtilsIdentifierType::MethodIdentifier;
    return type;
}

// note: ignores the QQmlLSUtilsResolveOptions because function properties/prototypes are not
// implemented
static std::optional<QQmlLSUtilsExpressionType>
methodFromReferrerScope(const QQmlJSScope::ConstPtr &referrerScope, const QString &name,
                        QQmlLSUtilsResolveOptions = ResolveOwnerType)
{
    for (QQmlJSScope::ConstPtr current = referrerScope; current; current = current->parentScope()) {
        if (auto type = hasMethodOrSignal(current, name))
            return QQmlLSUtilsExpressionType{ name, current, *type };

        if (const auto signalName = QQmlSignalNames::handlerNameToSignalName(name)) {
            if (auto type = hasMethodOrSignal(current, *signalName)) {
                return QQmlLSUtilsExpressionType{ name, current, SignalHandlerIdentifier };
            }
        }
    }
    return {};
}

static std::optional<QQmlLSUtilsExpressionType>
propertyFromReferrerScope(const QQmlJSScope::ConstPtr &referrerScope, const QString &propertyName,
                          QQmlLSUtilsResolveOptions options)
{
    for (QQmlJSScope::ConstPtr current = referrerScope; current; current = current->parentScope()) {
        const auto resolved = resolveNameInQmlScope(propertyName, current);
        if (!resolved)
            continue;

        if (auto property = current->property(resolved->name); property.isValid()) {
            switch (options) {
            case ResolveOwnerType:
                return QQmlLSUtilsExpressionType{ propertyName, current,
                                                  resolved->type };
            case ResolveActualTypeForFieldMemberExpression:
                return QQmlLSUtilsExpressionType{ propertyName, property.type(),
                                                  resolved->type };
            }
        }
    }
    return {};
}

static std::optional<QQmlLSUtilsExpressionType>
propertyBindingFromReferrerScope(const QQmlJSScope::ConstPtr &referrerScope, const QString &name,
                          QQmlLSUtilsResolveOptions options)
{
    if (auto bindings = referrerScope->propertyBindings(name); !bindings.isEmpty()) {
        const auto binding = bindings.front();

        if ((binding.bindingType() != QQmlSA::BindingType::AttachedProperty) &&
            (binding.bindingType() != QQmlSA::BindingType::GroupProperty))
            return {};

        const auto getTypeIdentifier = [&binding]{
            switch (binding.bindingType()) {
            case QQmlSA::BindingType::AttachedProperty: return AttachedTypeIdentifier;
            case QQmlSA::BindingType::GroupProperty: return GroupedPropertyIdentifier;
            default:
                Q_UNREACHABLE();
            }
        };

        const auto getScope = [&binding]{
            switch (binding.bindingType()) {
            case QQmlSA::BindingType::AttachedProperty: return binding.attachingType();
            case QQmlSA::BindingType::GroupProperty: return binding.groupType();
            default:
                Q_UNREACHABLE();
            }
        };

        switch (options) {
        case ResolveOwnerType: {
            return QQmlLSUtilsExpressionType{ name, referrerScope, getTypeIdentifier()};
        }
        case ResolveActualTypeForFieldMemberExpression:
            return QQmlLSUtilsExpressionType{name, getScope(), getTypeIdentifier()};
        }
    }

    return {};
}

static QQmlJSScope::ConstPtr findScopeInConnections(QQmlJSScope::ConstPtr scope, const DomItem &item)
{
    if (!scope || (scope->baseType() && scope->baseType()->internalName() != u"QQmlConnections"_s))
        return {};

    // Perform target name search if there is binding to property "target"
    QString targetName;
    if (scope->hasOwnPropertyBindings(u"target"_s)) {
        // TODO: propagate the whole binding.
        //       We can figure out the meaning of target in more cases.

        DomItem current = item.qmlObject();
        auto target = current.bindings().key(u"target"_s).index(0);
        if (target) {
            targetName = target.field(Fields::value)
                                 .field(Fields::scriptElement)
                                 .field(Fields::identifier)
                                 .value()
                                 .toString();
        }
    }

    if (!targetName.isEmpty()) {
        // look for the scope of target
        auto resolver = item.containingFile().ownerAs<QmlFile>()->typeResolver();
        if (!resolver)
            return {};

        // Note: It does not have to be an ID. It can be a property.
        return resolver->containedType(resolver->scopedType(scope, targetName));
    } else {
        // No binding to target property, return the container object's scope
        return scope->parentScope();
    }

    return {};
}

static std::optional<QQmlLSUtilsExpressionType>
resolveFieldMemberExpressionType(const DomItem &item, QQmlLSUtilsResolveOptions options)
{
    const QString name = item.field(Fields::identifier).value().toString();
    DomItem parent = item.directParent();
    auto owner = QQmlLSUtils::resolveExpressionType(
            parent.field(Fields::left),
            QQmlLSUtilsResolveOptions::ResolveActualTypeForFieldMemberExpression);
    if (!owner)
        return {};

    if (auto methods = owner.value().semanticScope->methods(name); !methods.isEmpty()) {
        switch (options) {
        case ResolveOwnerType: {
            const bool isSignal = methods.front().methodType() == QQmlJSMetaMethodType::Signal;
            QQmlLSUtilsIdentifierType type = isSignal ? QQmlLSUtilsIdentifierType::SignalIdentifier
                                                      : QQmlLSUtilsIdentifierType::MethodIdentifier;
            return QQmlLSUtilsExpressionType{ name, owner->semanticScope, type };
        }
        case ResolveActualTypeForFieldMemberExpression:
            // not implemented, but JS functions have methods and properties
            // see
            // https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Function
            // for the list of properties/methods of functions
            // see also code below for non-qualified method access
            break;
        }
    }

    if (auto scope = propertyBindingFromReferrerScope(owner->semanticScope,name, options))
        return *scope;

    if (auto property = owner->semanticScope->property(name); property.isValid()) {
        switch (options) {
        case ResolveOwnerType:
            return QQmlLSUtilsExpressionType{ name, owner->semanticScope,
                                              QQmlLSUtilsIdentifierType::PropertyIdentifier };
        case ResolveActualTypeForFieldMemberExpression:
            return QQmlLSUtilsExpressionType{ name, property.type(),
                                              QQmlLSUtilsIdentifierType::PropertyIdentifier };
        }
    }

    // distinguish between the `someItem.MyEnumerator.MyEnumeratorValue` case and the
    // `someItem.MyEnumeratorValue` case by using the QQmlLSUtilsIdentifierType::Enumerator,
    // as they have the same owner type (enumerators do not have their own qqmljsscope).
    switch (owner->type) {
    case QQmlLSUtilsIdentifierType::EnumeratorValueIdentifier:
        // for example, 'someItem.MyEnumerator.MyEnumeratorValue.<current name to be resolved>'
        // or 'someItem.MyEnumeratorValue.<current name to be resolved>'
        break;
    case QQmlLSUtilsIdentifierType::EnumeratorIdentifier:
        // for example, 'someItem.MyEnumerator.<current name to be resolved>'
        // do not check name in this case, just assumes its a value of the enumerator so the
        // autocompletion still works when the user is still typing the enumerator value name.
        if (auto enumerator = owner->semanticScope->enumeration(owner->name.value_or(QString()));
            enumerator.isValid()) {
            return QQmlLSUtilsExpressionType{
                owner->name, owner->semanticScope,
                QQmlLSUtilsIdentifierType::EnumeratorValueIdentifier
            };
        }
        break;
    default:
        // for example, 'someItem.<current name to be resolved>'
        if (auto enumerator = owner->semanticScope->enumeration(name); enumerator.isValid()) {
            return QQmlLSUtilsExpressionType{ name, owner->semanticScope,
                                              QQmlLSUtilsIdentifierType::EnumeratorIdentifier };
        }
        for (const QQmlJSMetaEnum &enumerator : owner->semanticScope->enumerations()) {
            if (enumerator.hasKey(name)) {
                return QQmlLSUtilsExpressionType{
                    name, owner->semanticScope, QQmlLSUtilsIdentifierType::EnumeratorValueIdentifier
                };
            }
        }
    }
    qCDebug(QQmlLSUtilsLog) << "Could not find identifier expression for" << item.internalKindStr();
    return {};
}

static std::optional<QQmlLSUtilsExpressionType>
resolveIdentifierExpressionType(const DomItem &item, QQmlLSUtilsResolveOptions options)
{
    if (isFieldMemberAccess(item)) {
        return resolveFieldMemberExpressionType(item, options);
    }

    const QString name = item.field(Fields::identifier).value().toString();

    if (DomItem definitionOfItem = findJSIdentifierDefinition(item, name)) {
        Q_ASSERT_X(!definitionOfItem.semanticScope().isNull()
                            && definitionOfItem.semanticScope()->ownJSIdentifier(name),
                    "QQmlLSUtils::findDefinitionOf",
                    "JS definition does not actually define the JS identifer. "
                    "It should be empty.");
        auto scope = definitionOfItem.semanticScope();
        auto jsIdentifier = scope->ownJSIdentifier(name);
        if (jsIdentifier->scope) {
            return QQmlLSUtilsExpressionType{ name, jsIdentifier->scope.toStrongRef(),
                                                QQmlLSUtilsIdentifierType::JavaScriptIdentifier };
        } else {
            return QQmlLSUtilsExpressionType{ name, scope,
                                                QQmlLSUtilsIdentifierType::JavaScriptIdentifier };
        }
    }

    const auto referrerScope = item.nearestSemanticScope();
    if (!referrerScope)
        return {};

    // check if its a method
    if (auto scope = methodFromReferrerScope(referrerScope, name, options))
        return scope;

    // check if its found as a property binding
    if (auto scope = propertyBindingFromReferrerScope(referrerScope, name, options))
        return *scope;

    // check if its an (unqualified) property
    if (auto scope = propertyFromReferrerScope(referrerScope, name, options))
        return *scope;

    const auto resolver = item.containingFile().ownerAs<QmlFile>()->typeResolver();
    if (!resolver)
        return {};

    // Returns the baseType, can't use it with options.
    if (auto scope = resolver->typeForName(name)) {
        if (scope->isSingleton())
            return QQmlLSUtilsExpressionType{ name, scope,
                                              QQmlLSUtilsIdentifierType::SingletonIdentifier };

        if (auto attachedScope = scope->attachedType()) {
            return QQmlLSUtilsExpressionType{
                name, attachedScope, QQmlLSUtilsIdentifierType::AttachedTypeIdentifier
            };
        }
    }

    // check if its an id
    QQmlJSRegisterContent fromId = resolver->scopedType(referrerScope, name);
    if (fromId.variant() == QQmlJSRegisterContent::ObjectById)
        return QQmlLSUtilsExpressionType{ name, fromId.type(), QmlObjectIdIdentifier };

    const QQmlJSScope::ConstPtr jsGlobal = resolver->jsGlobalObject();
    // check if its a JS global method
    if (auto scope = methodFromReferrerScope(jsGlobal, name, options))
        return scope;

    // check if its an JS global property
    if (auto scope = propertyFromReferrerScope(jsGlobal, name, options))
        return *scope;

    return {};
}
/*!
   \internal
    Resolves the type of the given DomItem, when possible (e.g., when there are enough type
    annotations).
*/
std::optional<QQmlLSUtilsExpressionType>
QQmlLSUtils::resolveExpressionType(const QQmlJS::Dom::DomItem &item,
                                   QQmlLSUtilsResolveOptions options)
{
    switch (item.internalKind()) {
    case DomType::ScriptIdentifierExpression: {
        return resolveIdentifierExpressionType(item, options);
    }
    case DomType::PropertyDefinition: {
        auto propertyDefinition = item.as<PropertyDefinition>();
        if (propertyDefinition && propertyDefinition->scope) {
            const auto &scope = propertyDefinition->scope;
            return QQmlLSUtilsExpressionType{ propertyDefinition->name, scope, PropertyIdentifier };
        }
        return {};
    }
    case DomType::Binding: {
        auto binding = item.as<Binding>();
        if (binding) {
            std::optional<QQmlJSScope::ConstPtr> owner = item.qmlObject().semanticScope();
            if (!owner)
                return {};
            const QString name = binding->name();

            if (name == u"id")
                return QQmlLSUtilsExpressionType{ name, owner.value(), QmlObjectIdIdentifier };

            if (QQmlJSScope::ConstPtr targetScope = findScopeInConnections(owner.value(), item)) {
                return QQmlLSUtilsExpressionType{ name, targetScope,
                                                  resolveNameInQmlScope(name, targetScope)->type };
            }
            auto signalOrProperty = resolveNameInQmlScope(name, owner.value());
            if (signalOrProperty)
                return QQmlLSUtilsExpressionType{ name, owner.value(), signalOrProperty->type };

            qDebug(QQmlLSUtilsLog) << "QQmlLSUtils::resolveExpressionType() could not resolve the"
                                      "type of a Binding.";
        }

        return {};
    }
    case DomType::QmlObject: {
        auto object = item.as<QmlObject>();
        if (!object)
            return {};
        if (const auto scope = object->semanticScope()) {
            const auto name = item.name();
            const auto resolved = resolveNameInQmlScope(name, scope);
            switch (options) {
            case ResolveOwnerType:
                return QQmlLSUtilsExpressionType{name, scope->parentScope(), resolved->type};
            case ResolveActualTypeForFieldMemberExpression:
                return QQmlLSUtilsExpressionType{name, scope, resolved->type};
            }
        }
        return {};
    }
    case DomType::MethodInfo: {
        auto object = item.as<MethodInfo>();
        if (object && object->semanticScope()) {
            std::optional<QQmlJSScope::ConstPtr> scope = object->semanticScope();
            if (!scope)
                return {};

            if (QQmlJSScope::ConstPtr targetScope =
                        findScopeInConnections(scope.value()->parentScope(), item)) {
                return QQmlLSUtilsExpressionType{
                    object->name, targetScope,
                    resolveNameInQmlScope(object->name, targetScope)->type
                };
            }

            // in case scope is the semantic scope for the function bodies: grab the owner's scope
            // this happens for all methods but not for signals (they do not have a body)
            if (scope.value()->scopeType() == QQmlJSScope::ScopeType::JSFunctionScope)
                scope = scope.value()->parentScope();

            if (auto type = hasMethodOrSignal(scope.value(), object->name))
                return QQmlLSUtilsExpressionType{ object->name, scope.value(), type.value() };

            qDebug(QQmlLSUtilsLog) << "QQmlLSUtils::resolveExpressionType() could not resolve the"
                                      "type of a MethodInfo.";
        }

        return {};
    }
    case DomType::ScriptBinaryExpression: {
        if (isFieldMemberExpression(item)) {
            return resolveExpressionType(item.field(Fields::right), options);
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

DomItem QQmlLSUtils::sourceLocationToDomItem(const DomItem &file,
                                             const QQmlJS::SourceLocation &location)
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

static std::optional<QQmlLSUtilsLocation>
findMethodDefinitionOf(const DomItem &file, QQmlJS::SourceLocation location, const QString &name)
{
    DomItem owner = QQmlLSUtils::sourceLocationToDomItem(file, location);
    DomItem method = owner.field(Fields::methods).key(name).index(0);
    auto fileLocation = FileLocations::treeOf(method);
    if (!fileLocation)
        return {};

    auto regions = fileLocation->info().regions;

    if (auto it = regions.constFind(IdentifierRegion); it != regions.constEnd()) {
        QQmlLSUtilsLocation result;
        result.sourceLocation = *it;
        result.filename = method.canonicalFilePath();
        return result;
    }

    return {};
}

static std::optional<QQmlLSUtilsLocation>
findPropertyDefinitionOf(const DomItem &file, QQmlJS::SourceLocation propertyDefinitionLocation,
                         const QString &name)
{
    DomItem propertyOwner = QQmlLSUtils::sourceLocationToDomItem(file, propertyDefinitionLocation);
    DomItem propertyDefinition = propertyOwner.field(Fields::propertyDefs).key(name).index(0);
    auto fileLocation = FileLocations::treeOf(propertyDefinition);
    if (!fileLocation)
        return {};

    auto regions = fileLocation->info().regions;

    if (auto it = regions.constFind(IdentifierRegion); it != regions.constEnd()) {
        QQmlLSUtilsLocation result;
        result.sourceLocation = *it;
        result.filename = propertyDefinition.canonicalFilePath();
        return result;
    }

    return {};
}

std::optional<QQmlLSUtilsLocation> QQmlLSUtils::findDefinitionOf(const DomItem &item)
{

    switch (item.internalKind()) {
    case QQmlJS::Dom::DomType::ScriptIdentifierExpression: {
        const QString name = item.value().toString();
        if (isFieldMemberAccess(item)) {
            if (auto ownerScope = QQmlLSUtils::resolveExpressionType(
                        item, QQmlLSUtilsResolveOptions::ResolveOwnerType)) {
                const DomItem ownerFile = item.goToFile(ownerScope->semanticScope->filePath());
                const QQmlJS::SourceLocation ownerLocation =
                        ownerScope->semanticScope->sourceLocation();
                if (auto methodDefinition =
                            findMethodDefinitionOf(ownerFile, ownerLocation, name)) {
                    return methodDefinition;
                }
                if (auto propertyDefinition =
                            findPropertyDefinitionOf(ownerFile, ownerLocation, name)) {
                    return propertyDefinition;
                }
            }
            return {};
        }

        // check: is it a JS identifier?
        if (DomItem definitionOfItem = findJSIdentifierDefinition(item, name)) {
            return locationFromJSIdentifierDefinition(definitionOfItem, name);
        }

        // not a JS identifier, check for ids and properties and methods
        const auto referrerScope = item.nearestSemanticScope();
        if (!referrerScope)
            return {};

        // check: is it a method name?
        if (auto scope = methodFromReferrerScope(referrerScope, name)) {
            const QString canonicalPath = scope->semanticScope->filePath();
            DomItem file = item.goToFile(canonicalPath);
            return findMethodDefinitionOf(file, scope->semanticScope->sourceLocation(), name);
        }

        if (auto scope = propertyFromReferrerScope(referrerScope, name,
                                                   QQmlLSUtilsResolveOptions::ResolveOwnerType)) {
            const QString canonicalPath = scope->semanticScope->filePath();
            DomItem file = item.goToFile(canonicalPath);
            return findPropertyDefinitionOf(file, scope->semanticScope->sourceLocation(), name);
        }

        // check if its an id
        auto resolver = item.containingFile().ownerAs<QmlFile>()->typeResolver();
        if (!resolver)
            return {};
        QQmlJSRegisterContent fromId = resolver->scopedType(referrerScope, name);
        if (fromId.variant() == QQmlJSRegisterContent::ObjectById) {
            DomItem qmlObject = QQmlLSUtils::sourceLocationToDomItem(
                    item.containingFile(), fromId.type()->sourceLocation());
            // in the Dom, the id is saved in a QMultiHash inside the Component of an QmlObject.
            DomItem domId = qmlObject.component()
                                    .field(Fields::ids)
                                    .key(name)
                                    .index(0)
                                    .field(Fields::value);
            if (!domId) {
                qCDebug(QQmlLSUtilsLog)
                        << "QmlComponent in Dom structure has no id, was it misconstructed?";
                return {};
            }

            QQmlLSUtilsLocation result;
            result.sourceLocation = FileLocations::treeOf(domId)->info().fullRegion;
            result.filename = domId.canonicalFilePath();
            return result;
        }
        return {};
    }
    default:
        qCDebug(QQmlLSUtilsLog) << "QQmlLSUtils::findDefinitionOf: Found unimplemented Type "
                                << item.internalKindStr();
        return {};
    }

    Q_UNREACHABLE_RETURN(std::nullopt);
}

static QQmlJSScope::ConstPtr propertyOwnerFrom(const QQmlJSScope::ConstPtr &type,
                                               const QString &name)
{
    Q_ASSERT(!name.isEmpty());
    Q_ASSERT(type);

    QQmlJSScope::ConstPtr typeWithDefinition = type;
    while (typeWithDefinition && !typeWithDefinition->hasOwnProperty(name))
        typeWithDefinition = typeWithDefinition->baseType();

    if (!typeWithDefinition) {
        qCDebug(QQmlLSUtilsLog)
                << "QQmlLSUtils::checkNameForRename cannot find property definition,"
                    " ignoring.";
    }

    return typeWithDefinition;
}

static QQmlJSScope::ConstPtr methodOwnerFrom(const QQmlJSScope::ConstPtr &type,
                                             const QString &name)
{
    Q_ASSERT(!name.isEmpty());
    Q_ASSERT(type);

    QQmlJSScope::ConstPtr typeWithDefinition = type;
    while (typeWithDefinition && !typeWithDefinition->hasOwnMethod(name))
        typeWithDefinition = typeWithDefinition->baseType();

    if (!typeWithDefinition) {
        qCDebug(QQmlLSUtilsLog)
                << "QQmlLSUtils::checkNameForRename cannot find method definition,"
                    " ignoring.";
    }

    return typeWithDefinition;
}

static QQmlJSScope::ConstPtr
expressionTypeWithDefinition(const QQmlLSUtilsExpressionType &ownerType)
{
    switch (ownerType.type) {
    case PropertyIdentifier:
        return propertyOwnerFrom(ownerType.semanticScope, *ownerType.name);
    case PropertyChangedHandlerIdentifier: {
        const auto propertyName =
                QQmlSignalNames::changedHandlerNameToPropertyName(*ownerType.name);
        return propertyOwnerFrom(ownerType.semanticScope, *propertyName);
        break;
    }
    case PropertyChangedSignalIdentifier: {
        const auto propertyName = QQmlSignalNames::changedSignalNameToPropertyName(*ownerType.name);
        return propertyOwnerFrom(ownerType.semanticScope, *propertyName);
    }
    case MethodIdentifier:
    case SignalIdentifier:
        return methodOwnerFrom(ownerType.semanticScope, *ownerType.name);
    case SignalHandlerIdentifier: {
        const auto signalName = QQmlSignalNames::handlerNameToSignalName(*ownerType.name);
        return methodOwnerFrom(ownerType.semanticScope, *signalName);
    }
    case JavaScriptIdentifier:
    case QmlObjectIdIdentifier:
    case QmlObjectIdentifier:
    case SingletonIdentifier:
    case EnumeratorIdentifier:
    case EnumeratorValueIdentifier:
    case AttachedTypeIdentifier:
    case GroupedPropertyIdentifier:
        return ownerType.semanticScope;
    }
    return {};
}

std::optional<QQmlLSUtilsErrorMessage>
QQmlLSUtils::checkNameForRename(const DomItem &item, const QString &dirtyNewName,
                                std::optional<QQmlLSUtilsExpressionType> ownerType)
{
    // general checks for ECMAscript identifiers
    if (!isValidEcmaScriptIdentifier(dirtyNewName))
        return QQmlLSUtilsErrorMessage{ 0, u"Invalid EcmaScript identifier!"_s };

    if (!ownerType)
        ownerType = QQmlLSUtils::resolveExpressionType(item, ResolveOwnerType);

    const auto userSemanticScope = item.nearestSemanticScope();

    if (!ownerType || !userSemanticScope) {
        return QQmlLSUtilsErrorMessage{ 0, u"Requested item cannot be renamed"_s };
    }

    // type specific checks
    switch (ownerType->type) {
    case PropertyChangedSignalIdentifier: {
        if (!QQmlSignalNames::isChangedSignalName(dirtyNewName)) {
            return QQmlLSUtilsErrorMessage{ 0, u"Invalid name for a property changed signal."_s };
        }
        break;
    }
    case PropertyChangedHandlerIdentifier: {
        if (!QQmlSignalNames::isChangedHandlerName(dirtyNewName)) {
            return QQmlLSUtilsErrorMessage{
                0, u"Invalid name for a property changed handler identifier."_s
            };
        }
        break;
    }
    case SignalHandlerIdentifier: {
        if (!QQmlSignalNames::isHandlerName(dirtyNewName)) {
            return QQmlLSUtilsErrorMessage{ 0, u"Invalid name for a signal handler identifier."_s };
        }
        break;
    }
    // TODO: any other specificities?
    case QmlObjectIdIdentifier:
        if (dirtyNewName.front().isLetter() && !dirtyNewName.front().isLower()) {
            return QQmlLSUtilsErrorMessage{
                0, u"Object id names cannot start with an upper case letter."_s
            };
        }
        break;
    case JavaScriptIdentifier:
    case PropertyIdentifier:
    case SignalIdentifier:
    case MethodIdentifier:
    case QmlObjectIdentifier:
    default:
        break;
    };

    auto typeWithDefinition = expressionTypeWithDefinition(*ownerType);

    if (!typeWithDefinition) {
        return QQmlLSUtilsErrorMessage{
            0,
            u"Renaming has not been implemented for the requested item."_s,
        };
    }

    // is it not defined in QML?
    if (!typeWithDefinition->isComposite()) {
        return QQmlLSUtilsErrorMessage{ 0, u"Cannot rename items defined in non-QML files."_s };
    }

    // is it defined in the current module?
    const QString moduleOfDefinition = ownerType->semanticScope->moduleName();
    const QString moduleOfCurrentItem = userSemanticScope->moduleName();
    if (moduleOfDefinition != moduleOfCurrentItem) {
        return QQmlLSUtilsErrorMessage{
            0,
            u"Cannot rename items defined in the %1 module fromits usage in the %2 module."_s
                    .arg(moduleOfDefinition, moduleOfCurrentItem),
        };
    }

    return {};
}

static std::optional<QString> oldNameFrom(const DomItem &item)
{
    switch (item.internalKind()) {
    case DomType::ScriptIdentifierExpression: {
        return item.field(Fields::identifier).value().toString();
    }
    case DomType::ScriptVariableDeclarationEntry: {
        return item.field(Fields::identifier).value().toString();
    }
    case DomType::PropertyDefinition:
    case DomType::Binding:
    case DomType::MethodInfo: {
        return item.field(Fields::name).value().toString();
    }
    default:
        qCDebug(QQmlLSUtilsLog) << item.internalKindStr()
                                << "was not implemented for QQmlLSUtils::renameUsagesOf";
        return std::nullopt;
    }
    Q_UNREACHABLE_RETURN(std::nullopt);
}

static std::optional<QString> newNameFrom(const QString &dirtyNewName,
                                          QQmlLSUtilsIdentifierType alternative)
{
    // When renaming signal/property changed handlers and property changed signals:
    // Get the actual corresponding signal name (for signal handlers) or property name (for
    // property changed signal + handlers) that will be used for the renaming.
    switch (alternative) {
    case SignalHandlerIdentifier: {
        return QQmlSignalNames::handlerNameToSignalName(dirtyNewName);
    }
    case PropertyChangedHandlerIdentifier: {
        return QQmlSignalNames::changedHandlerNameToPropertyName(dirtyNewName);
    }
    case PropertyChangedSignalIdentifier: {
        return QQmlSignalNames::changedSignalNameToPropertyName(dirtyNewName);
    }
    case SignalIdentifier:
    case PropertyIdentifier:
    default:
        return std::nullopt;
    }
    Q_UNREACHABLE_RETURN(std::nullopt);
}

/*!
\internal
\brief Rename the appearance of item to newName.

Special cases:
\list
    \li Renaming a property changed signal or property changed handler does the same as renaming
    the underlying property, except that newName gets
    \list
        \li its "on"-prefix and "Changed"-suffix chopped of if item is a property changed handlers
        \li its "Changed"-suffix chopped of if item is a property changed signals
    \endlist
    \li Renaming a signal handler does the same as renaming a signal, but the "on"-prefix in newName
    is chopped of.

    All of the chopping operations are done using the static helpers from QQmlSignalNames.
\endlist
*/
QList<QQmlLSUtilsEdit>
QQmlLSUtils::renameUsagesOf(const DomItem &item, const QString &dirtyNewName,
                            std::optional<QQmlLSUtilsExpressionType> targetType)
{
    QList<QQmlLSUtilsEdit> results;
    const QList<QQmlLSUtilsLocation> locations = findUsagesOf(item);
    if (locations.isEmpty())
        return results;

    auto oldName = oldNameFrom(item);
    if (!oldName)
        return results;

    if (!targetType)
        targetType = QQmlLSUtils::resolveExpressionType(
                item, QQmlLSUtilsResolveOptions::ResolveOwnerType);

    if (!targetType)
        return results;

    QString newName;
    if (const auto resolved = resolveNameInQmlScope(*oldName, targetType->semanticScope)) {
        newName = newNameFrom(dirtyNewName, resolved->type).value_or(dirtyNewName);
        oldName = resolved->name;
    } else {
        newName = dirtyNewName;
    }

    const qsizetype oldNameLength = oldName->length();
    const qsizetype oldHandlerNameLength =
            QQmlSignalNames::signalNameToHandlerName(*oldName).length();
    const qsizetype oldChangedSignalNameLength =
            QQmlSignalNames::propertyNameToChangedSignalName(*oldName).length();
    const qsizetype oldChangedHandlerNameLength =
            QQmlSignalNames::propertyNameToChangedHandlerName(*oldName).length();

    const QString newHandlerName = QQmlSignalNames::signalNameToHandlerName(newName);
    const QString newChangedSignalName = QQmlSignalNames::propertyNameToChangedSignalName(newName);
    const QString newChangedHandlerName =
            QQmlSignalNames::propertyNameToChangedHandlerName(newName);

    // set the new name at the found usages, but add "on"-prefix and "Changed"-suffix if needed
    for (const auto &location : locations) {
        const qsizetype currentLength = location.sourceLocation.length;
        QQmlLSUtilsEdit edit;
        edit.location = location;
        if (oldNameLength == currentLength) {
            // normal case, nothing to do
            edit.replacement = newName;

        } else if (oldHandlerNameLength == currentLength) {
            // signal handler location
            edit.replacement = newHandlerName;

        } else if (oldChangedSignalNameLength == currentLength) {
            // property changed signal location
            edit.replacement = newChangedSignalName;

        } else if (oldChangedHandlerNameLength == currentLength) {
            // property changed handler location
            edit.replacement = newChangedHandlerName;

        } else {
            qCDebug(QQmlLSUtilsLog) << "Found usage with wrong identifier length, ignoring...";
            continue;
        }
        results.append(edit);
    }

    return results;
}

QQmlLSUtilsLocation QQmlLSUtilsLocation::from(const QString &fileName, const QString &code,
                                              quint32 startLine, quint32 startCharacter,
                                              quint32 length)
{
    quint32 offset = QQmlLSUtils::textOffsetFrom(code, startLine - 1, startCharacter - 1);

    QQmlLSUtilsLocation location{
        fileName, QQmlJS::SourceLocation{ offset, length, startLine, startCharacter }
    };
    return location;
}

QQmlLSUtilsEdit QQmlLSUtilsEdit::from(const QString &fileName, const QString &code,
                                      quint32 startLine, quint32 startCharacter, quint32 length,
                                      const QString &newName)
{
    QQmlLSUtilsEdit rename;
    rename.location = QQmlLSUtilsLocation::from(fileName, code, startLine, startCharacter, length);
    rename.replacement = newName;
    return rename;
}

bool QQmlLSUtils::isValidEcmaScriptIdentifier(QStringView identifier)
{
    QQmlJS::Lexer lexer(nullptr);
    lexer.setCode(identifier.toString(), 0);
    const int token = lexer.lex();
    if (token != static_cast<int>(QQmlJS::Lexer::T_IDENTIFIER))
        return false;
    // make sure there is nothing following the lexed identifier
    const int eofToken = lexer.lex();
    return eofToken == static_cast<int>(QQmlJS::Lexer::EOF_SYMBOL);
}

static QList<CompletionItem> signalHandlerCompletion(const QQmlJSScope::ConstPtr &scope,
                                                     QDuplicateTracker<QString> *usedNames)
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

static QList<CompletionItem> &&insertColonsForCompletions(QList<CompletionItem> &&completions)
{
    for (auto &completion : completions) {
        // ignore the completions that already have insertText set
        if (completion.insertText)
            continue;

        completion.insertText = QByteArray(completion.label).append(": ");
    }
    return std::move(completions);
}

static QList<CompletionItem> suggestBindingCompletion(const DomItem &containingObject)
{
    QList<CompletionItem> res;

    res << QQmlLSUtils::reachableTypes(containingObject, LocalSymbolsType::AttachedType,
                          CompletionItemKind::Class);

    const QQmlJSScope::ConstPtr scope = containingObject.semanticScope();
    if (!scope)
        return res;

    res << insertColonsForCompletions(propertyCompletion(scope, nullptr));
    res << insertColonsForCompletions(signalHandlerCompletion(scope, nullptr));

    return res;
}

static QList<CompletionItem> insideImportCompletionHelper(const DomItem &file,
                                                          const CompletionContextStrings &ctx)
{
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

QList<CompletionItem> QQmlLSUtils::idsCompletions(const DomItem& component)
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

QList<CompletionItem> QQmlLSUtils::reachableTypes(const DomItem &el, LocalSymbolsTypes options,
                                                  CompletionItemKind kind)
{
    auto file = el.containingFile().as<QmlFile>();
    if (!file)
        return {};
    auto resolver = file->typeResolver();
    if (!resolver)
        return {};

    QList<CompletionItem> res;
    const auto keyValueRange = resolver->importedTypes().asKeyValueRange();
    for (const auto &type : keyValueRange) {
        // ignore special QQmlJSImporterMarkers
        const bool isMarkerType = type.first.contains(u"$internal$.")
                || type.first.contains(u"$anonymous$.") || type.first.contains(u"$module$.");
        if (isMarkerType)
            continue;

        auto &scope = type.second.scope;
        if (!scope)
            continue;

        if (!testScopeSymbol(scope, options, kind))
            continue;

        CompletionItem completion;
        completion.label = type.first.toUtf8();
        completion.kind = int(kind);
        res << completion;
    }
    return res;
}

static QList<CompletionItem> jsIdentifierCompletion(const QQmlJSScope::ConstPtr &scope,
                                                    QDuplicateTracker<QString> *usedNames)
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

static QList<CompletionItem> methodCompletion(const QQmlJSScope::ConstPtr &scope,
                                              QDuplicateTracker<QString> *usedNames)
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

static QList<CompletionItem> propertyCompletion(const QQmlJSScope::ConstPtr &scope,
                                                QDuplicateTracker<QString> *usedNames)
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

static QList<CompletionItem> enumerationCompletion(const QQmlJSScope::ConstPtr &scope,
                                                   QDuplicateTracker<QString> *usedNames)
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

static QList<CompletionItem> enumerationValueCompletionHelper(const QStringList &enumeratorKeys)
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

static QList<CompletionItem> enumerationValueCompletion(const QQmlJSScope::ConstPtr &scope,
                                                        const QString &enumeratorName)
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
template<auto F, typename... T>
decltype(auto) collectFromAllJavaScriptParents(const QQmlJSScope::ConstPtr &scope, T... args)
{
    decltype(F(scope, args...)) result;
    for (QQmlJSScope::ConstPtr current = scope; current; current = current->parentScope()) {
        result << F(current, args...);
        if (current->scopeType() == QQmlSA::ScopeType::QMLScope)
            break;
    }
    return result;
}

// TODO: rename this to 'expressionCompletion', expression as in expression vs statements
/*!
\internal
Generate autocompletions for JS expressions (expressions as in 'expressions and statements').
*/
QList<CompletionItem> QQmlLSUtils::scriptIdentifierCompletion(const DomItem &context,
                                                              const CompletionContextStrings &ctx)
{
    Q_UNUSED(ctx); // could be needed later
    QList<CompletionItem> result;
    QDuplicateTracker<QString> usedNames;
    QQmlJSScope::ConstPtr nearestScope;
    const bool hasQualifier = isFieldMemberAccess(context);

    if (!hasQualifier) {
        result << idsCompletions(context.component())
               << reachableTypes(context,
                                 LocalSymbolsType::Singleton | LocalSymbolsType::AttachedType,
                                 CompletionItemKind::Class);

        auto scope = context.nearestSemanticScope();
        if (!scope)
            return result;
        nearestScope = scope;

        result << enumerationCompletion(nearestScope, &usedNames);
    } else {
        const DomItem owner = context.directParent().field(Fields::left);
        auto expressionType = QQmlLSUtils::resolveExpressionType(
                owner, ResolveActualTypeForFieldMemberExpression);
        if (!expressionType || !expressionType->semanticScope)
            return result;
        nearestScope = expressionType->semanticScope;

        if (expressionType->name) {
            // note: you only get enumeration values in qualified expressions, never alone
            result << enumerationValueCompletion(nearestScope, *expressionType->name);

            // skip enumeration types if already inside an enumeration type
            if (auto enumerator = nearestScope->enumeration(*expressionType->name);
                !enumerator.isValid()) {
                result << enumerationCompletion(nearestScope, &usedNames);
            }
        }
    }

    Q_ASSERT(nearestScope);

    result << methodCompletion(nearestScope, &usedNames)
           << propertyCompletion(nearestScope, &usedNames);

    if (!hasQualifier) {
        // collect all of the stuff from parents
        result << collectFromAllJavaScriptParents<jsIdentifierCompletion>(nearestScope, &usedNames)
               << collectFromAllJavaScriptParents<methodCompletion>(nearestScope, &usedNames)
               << collectFromAllJavaScriptParents<propertyCompletion>(nearestScope, &usedNames);

        auto file = context.containingFile().as<QmlFile>();
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

static bool cursorInFrontOfItem(const DomItem &currentItem,
                                const CompletionContextStrings &ctx)
{
    auto fileLocations = FileLocations::treeOf(currentItem)->info().fullRegion;
    return ctx.offset() <= fileLocations.offset;
}

static bool cursorAfterColon(const DomItem &currentItem, const CompletionContextStrings &ctx)
{
    auto location = FileLocations::treeOf(currentItem)->info();
    auto region = location.regions.constFind(ColonTokenRegion);

    if (region == location.regions.constEnd())
        return false;

    if (region.value().isValid() && region.value().offset < ctx.offset()) {
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

static QList<CompletionItem> insidePragmaCompletion(QQmlJS::Dom::DomItem currentItem,
                                                    const CompletionContextStrings &ctx)
{
    if (cursorAfterColon(currentItem, ctx)) {
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

static CompletionItem makeSnippet(QUtf8StringView label, QUtf8StringView insertText)
{
    CompletionItem res;
    res.label = label.data();
    res.insertTextFormat = InsertTextFormat::Snippet;
    res.insertText = insertText.data();
    res.kind = int(CompletionItemKind::Snippet);
    res.insertTextMode = InsertTextMode::AdjustIndentation;
    return res;
}

static QList<CompletionItem> insideQmlObjectCompletion(const DomItem &currentItem,
                                                       const CompletionContextStrings &ctx)
{
    QList<CompletionItem> res;
    if (ctx.base().isEmpty()) {
        // default/required property completion
        for (QUtf8StringView view :
             std::array<QUtf8StringView, 6>{ "", "readonly ", "default ", "default required ",
                                             "required default ", "required " }) {
            // readonly properties require an initializer
            if (view != "readonly ") {
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
        const DomItem containingObject = currentItem.qmlObject();
        res += suggestBindingCompletion(containingObject);

        // add Qml Types for default binding
        const DomItem containingFile = currentItem.containingFile();
        res += QQmlLSUtils::reachableTypes(containingFile, LocalSymbolsType::ObjectType,
                                           CompletionItemKind::Constructor);
    }
    return res;
}

static QList<CompletionItem> insidePropertyDefinitionCompletion(const DomItem &currentItem,
                                                const CompletionContextStrings &ctx)
{
    auto info = FileLocations::treeOf(currentItem)->info();
    const QQmlJS::SourceLocation propertyKeyword = info.regions[PropertyKeywordRegion];

    // do completions for the keywords
    if (ctx.offset() < propertyKeyword.offset + propertyKeyword.length) {
        const QQmlJS::SourceLocation readonlyKeyword = info.regions[ReadonlyKeywordRegion];
        const QQmlJS::SourceLocation defaultKeyword = info.regions[DefaultKeywordRegion];
        const QQmlJS::SourceLocation requiredKeyword = info.regions[RequiredKeywordRegion];

        bool completeReadonly = true;
        bool completeRequired = true;
        bool completeDefault = true;

        // if there is already a readonly keyword before the cursor: do not auto complete it again
        if (readonlyKeyword.isValid() && readonlyKeyword.offset < ctx.offset()) {
            completeReadonly = false;
            // also, required keywords do not like readonly keywords
            completeRequired = false;
        }

        // same for required
        if (requiredKeyword.isValid() && requiredKeyword.offset < ctx.offset()) {
            completeRequired = false;
            // also, required keywords do not like readonly keywords
            completeReadonly = false;
        }

        // same for default
        if (defaultKeyword.isValid() && defaultKeyword.offset < ctx.offset()) {
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
    if (propertyKeyword.end() <= ctx.offset() && ctx.offset() < propertyIdentifier.offset) {
        return QQmlLSUtils::reachableTypes(
                currentItem, LocalSymbolsType::ObjectType | LocalSymbolsType::ValueType,
                CompletionItemKind::Class);
    }
    // do not autocomplete the rest
    return {};
}

static QList<CompletionItem> insideBindingCompletion(const DomItem &currentItem,
                                               const CompletionContextStrings &ctx)
{
    const DomItem containingBinding = currentItem.filterUp(
            [](DomType type, const QQmlJS::Dom::DomItem &) { return type == DomType::Binding; },
            FilterUpOptions::ReturnOuter);

    // do scriptidentifiercompletion after the ':' of a binding
    if (cursorAfterColon(containingBinding, ctx)) {
        QList<CompletionItem> res;
        res << QQmlLSUtils::scriptIdentifierCompletion(currentItem, ctx);
        if (auto type = QQmlLSUtils::resolveExpressionType(currentItem, ResolveOwnerType)) {
            const QStringList names = currentItem.field(Fields::name).toString().split(u'.');
            const QQmlJSScope *current = resolve(type->semanticScope.get(), names);
            // add type names when binding to an object type or a property with var type
            if (!current || current->accessSemantics() == QQmlSA::AccessSemantics::Reference) {
                LocalSymbolsTypes options;
                options.setFlag(LocalSymbolsType::ObjectType);
                res << QQmlLSUtils::reachableTypes(currentItem, options,
                                                   CompletionItemKind::Constructor);
            }
        }
        return res;
    }

    // ignore the binding if asking for completion in front of the binding
    if (cursorInFrontOfItem(containingBinding, ctx)) {
        return insideQmlObjectCompletion(currentItem, ctx);
    }

    QList<CompletionItem> res;
    const DomItem containingObject = currentItem.qmlObject();
    const QStringList toResolve =
            containingBinding.field(Fields::name).value().toString().split(u'.');
    if (toResolve.size() == 1) {
        res << suggestBindingCompletion(containingObject);
    } else {
        // TODO: without QTBUG-117380, containingBinding.field(Fields::name) cannot be
        // resolved to its actual type
        res << suggestBindingCompletion(containingObject);
    }
    // add Qml Types for default binding
    res += QQmlLSUtils::reachableTypes(currentItem, LocalSymbolsType::ObjectType,
                                       CompletionItemKind::Constructor);
    return res;
}

static QList<CompletionItem> insideImportCompletion(const DomItem &currentItem,
                                               const CompletionContextStrings &ctx)
{
    const DomItem containingFile = currentItem.containingFile();
    QList<CompletionItem> res;
    res += insideImportCompletionHelper(containingFile, ctx);

    // when in front of the import statement: propose types for root Qml Object completion
    if (cursorInFrontOfItem(currentItem, ctx))
        res += QQmlLSUtils::reachableTypes(containingFile, LocalSymbolsType::ObjectType,
                                           CompletionItemKind::Constructor);

    return res;
}

static QList<CompletionItem> insideQmlFileCompletion(const DomItem &currentItem,
                                               const CompletionContextStrings &ctx)
{
    const DomItem containingFile = currentItem.containingFile();
    QList<CompletionItem> res;
    // completions for code outside the root Qml Object
    // global completions
    if (ctx.atLineStart()) {
        if (ctx.base().isEmpty()) {
            for (const QStringView &s : std::array<QStringView, 2>({ u"pragma", u"import" })) {
                CompletionItem comp;
                comp.label = s.toUtf8();
                comp.kind = int(CompletionItemKind::Keyword);
                res.append(comp);
            }
        }
    }
    // Types for root Qml Object completion
    res += QQmlLSUtils::reachableTypes(containingFile, LocalSymbolsType::ObjectType,
                                       CompletionItemKind::Constructor);
    return res;
}

/*!
\internal
Generate the snippets for let, var and const variable declarations.
*/
QList<CompletionItem>
QQmlLSUtils::suggestVariableDeclarationStatementCompletion(QQmlLSUtilsAppendOption option)
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
Generates snippets or keywords for all possible JS statements where it makes sense. To use whenever
any JS statement can be expected.

Here is a list of statements that do \e{not} get any completions:
\list
    \li BlockStatement does not need a code snippet, editors automatically include the closing bracket
    anyway.
    \li EmptyStatement completion would only generate a single \c{;}
    \li ExpressionStatement completion cannot generate any snippet, only identifiers
    \li WithStatement completion is not recommended: qmllint will warn about usage of with statements
    \li LabelledStatement completion might need to propose labels (TODO?)
    \li DebuggerStatement completion does not strike as being very useful
\endlist
*/
QList<CompletionItem> QQmlLSUtils::suggestJSStatementCompletion(const DomItem &currentItem)
{
    QList<CompletionItem> result = suggestVariableDeclarationStatementCompletion();

    // expression statements
    CompletionContextStrings empty{ QString(), 0 };
    result << scriptIdentifierCompletion(currentItem, empty);

    // block statement
    result.append(makeSnippet("{ statements... }", "{\n\t$0\n}"));

    // if statement
    result.append(makeSnippet("if (condition) statement", "if ($1)\n\t$0"));

    // if + brackets statement
    result.append(makeSnippet("if (condition) { statements }", "if ($1) {\n\t$0\n}"));

    // do statement
    result.append(makeSnippet( "do { statements } while (condition);", "do {\n\t$1\n} while ($0);"));

    // while statement
    result.append(makeSnippet("while (condition) statement", "while ($1)\n\t$0"));

    // while + brackets statement
    result.append(makeSnippet("while (condition) { statements...}", "while ($1) {\n\t$0\n}"));

    // for loop statement
    result.append(
            makeSnippet("for (initializer; condition; increment) statement", "for ($1;$2;$3)\n\t$0"));

    // for + brackets loop statement
    result.append(makeSnippet("for (initializer; condition; increment) { statements... }",
                              "for ($1;$2;$3) {\n\t$0\n}"));

    // for ... in + brackets loop statement
    result.append(makeSnippet("for (property in object) { statements... }",
                              "for ($1 in $2) {\n\t$0\n}"));

    // for ... of + brackets loop statement
    result.append(makeSnippet("for (element of array) { statements... }",
                              "for ($1 of $2) {\n\t$0\n}"));

    // try + catch statement
    result.append(makeSnippet("try { statements... } catch(error) { statements... }",
                              "try {\n\t$1\n} catch($2) {\n\t$0\n}"));

    // try + finally statement
    result.append(makeSnippet("try { statements... } finally { statements... }",
                              "try {\n\t$1\n} finally {\n\t$0\n}"));

    // try + catch + finally statement
    result.append(makeSnippet("try { statements... } catch(error) { statements... } finally { statements... }",
                              "try {\n\t$1\n} catch($2) {\n\t$3\n} finally {\n\t$0\n}"));

    // one can always assume that JS code in QML is inside a function, so always propose `return`
    for (auto&& view : { "return"_ba, "throw"_ba }) {
        result.emplaceBack();
        result.back().label = std::move(view);
        result.back().kind = int(CompletionItemKind::Keyword);
    }

    DomItem loopOrSwitchParent = currentItem;
    bool alreadyInLoop = false;
    bool alreadyInSwitch = false;
    for (DomItem current = currentItem; current; current = current.directParent()) {
        switch (current.internalKind()) {
        case DomType::ScriptExpression:
            // reached end of script expression
            return result;

        case DomType::ScriptForStatement:
        case DomType::ScriptForEachStatement:
        case DomType::ScriptWhileStatement:
        case DomType::ScriptDoWhileStatement:
            if (alreadyInLoop)
                continue;
            alreadyInLoop = true;
            for (auto view : std::array<QUtf8StringView, 2>{ "continue", "break" }) {
                result.emplaceBack();
                result.back().label = view.data();
                result.back().kind = int(CompletionItemKind::Keyword);
            }
            break;

        case DomType::ScriptSwitchStatement:
            if (alreadyInSwitch)
                continue;
            alreadyInSwitch = true;

            // case snippet
            result.append(makeSnippet("case value: statements...", "case ${1:value}:\n\t$0"));
            // case + brackets snippet
            result.append(
                    makeSnippet("case value: { statements... }", "case ${1:value}: {\n\t$0\n}"));

            // default snippet
            result.append(makeSnippet("default: statements...", "default:\n\t$0"));
            // case + brackets snippet
            result.append(makeSnippet("default: { statements... }", "default: {\n\t$0\n}"));

            // break
            result.emplaceBack();
            result.back().label = "break";
            result.back().kind = int(CompletionItemKind::Keyword);
            break;
        default:
            continue;
        }
        if (alreadyInLoop && alreadyInSwitch)
            return result;
    }
    return result;
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
static bool betweenLocations(QQmlJS::SourceLocation left, const CompletionContextStrings &ctx,
                             QQmlJS::SourceLocation right)
{
    if (!left.isValid())
        return false;
    // note: left.end() == ctx.offset() means that the cursor lies exactly after left
    if (!(left.end() <= ctx.offset()))
        return false;
    if (!right.isValid())
        return true;

    // note: ctx.offset() == right.begin() means that the cursor lies exactly before right
    return ctx.offset() <= right.begin();
}

/*!
\internal
Returns true if ctx denotes an offset lying behind left.end(), and false otherwise.
*/
static bool afterLocation(QQmlJS::SourceLocation left, const CompletionContextStrings &ctx)
{
    return betweenLocations(left, ctx, QQmlJS::SourceLocation{});
}

/*!
\internal
Returns true if ctx denotes an offset lying before right.begin(), and false otherwise.
*/
static bool beforeLocation(const CompletionContextStrings &ctx, QQmlJS::SourceLocation right)
{
    if (!right.isValid())
        return true;

    // note: ctx.offset() == right.begin() means that the cursor lies exactly before right
    if (ctx.offset() <= right.begin())
        return true;

    return false;
}

static QList<CompletionItem> insideForStatementCompletion(const DomItem &currentItem,
                                                    const CompletionContextStrings &ctx)
{
    const auto regions = FileLocations::treeOf(currentItem)->info().regions;

    const QQmlJS::SourceLocation leftParenthesis = regions[LeftParenthesisRegion];
    const QQmlJS::SourceLocation firstSemicolon = regions[FirstSemicolonTokenRegion];
    const QQmlJS::SourceLocation secondSemicolon = regions[SecondSemicolonRegion];
    const QQmlJS::SourceLocation rightParenthesis = regions[RightParenthesisRegion];

    if (betweenLocations(leftParenthesis, ctx, firstSemicolon)) {
        QList<CompletionItem> res;
        res << QQmlLSUtils::scriptIdentifierCompletion(currentItem, ctx)
            << QQmlLSUtils::suggestVariableDeclarationStatementCompletion();
        return res;
    }
    if (betweenLocations(firstSemicolon, ctx, secondSemicolon)
        || betweenLocations(secondSemicolon, ctx, rightParenthesis)) {
        QList<CompletionItem> res;
        res << QQmlLSUtils::scriptIdentifierCompletion(currentItem, ctx);
        return res;
    }

    if (afterLocation(rightParenthesis, ctx)) {
        return QQmlLSUtils::suggestJSStatementCompletion(currentItem);
    }

    return {};
}

static QList<CompletionItem> insideScriptLiteralCompletion(const DomItem &currentItem,
                                                           const CompletionContextStrings &ctx)
{
    if (ctx.base().isEmpty())
        return QQmlLSUtils::scriptIdentifierCompletion(currentItem, ctx);
    return {};
}

static QList<CompletionItem> insideCallExpression(const DomItem &currentItem,
                                                  const CompletionContextStrings &ctx)
{
    const auto regions = FileLocations::treeOf(currentItem)->info().regions;
    const QQmlJS::SourceLocation leftParenthesis = regions[LeftParenthesisRegion];
    const QQmlJS::SourceLocation rightParenthesis = regions[RightParenthesisRegion];
    if (beforeLocation(ctx, leftParenthesis)) {
        return QQmlLSUtils::scriptIdentifierCompletion(currentItem, ctx);
    }
    if (betweenLocations(leftParenthesis, ctx, rightParenthesis)) {
        return QQmlLSUtils::scriptIdentifierCompletion(currentItem, ctx);
    }
    return {};
}

static QList<CompletionItem> insideIfStatement(const DomItem &currentItem,
                                               const CompletionContextStrings &ctx)
{
    const auto regions = FileLocations::treeOf(currentItem)->info().regions;
    const QQmlJS::SourceLocation leftParenthesis = regions[LeftParenthesisRegion];
    const QQmlJS::SourceLocation rightParenthesis = regions[RightParenthesisRegion];
    const QQmlJS::SourceLocation elseKeyword = regions[ElseKeywordRegion];

    if (betweenLocations(leftParenthesis, ctx, rightParenthesis)) {
        return QQmlLSUtils::scriptIdentifierCompletion(currentItem, ctx);
    }
    if (betweenLocations(rightParenthesis, ctx, elseKeyword)) {
        return QQmlLSUtils::suggestJSStatementCompletion(currentItem);
    }
    if (afterLocation(elseKeyword, ctx)) {
        return QQmlLSUtils::suggestJSStatementCompletion(currentItem);
    }
    return {};
}

static QList<CompletionItem> insideReturnStatement(const DomItem &currentItem,
                                                   const CompletionContextStrings &ctx)
{
    const auto regions = FileLocations::treeOf(currentItem)->info().regions;
    const QQmlJS::SourceLocation returnKeyword = regions[ReturnKeywordRegion];

    if (afterLocation(returnKeyword, ctx)) {
        return QQmlLSUtils::scriptIdentifierCompletion(currentItem, ctx);
    }
    return {};
}

static QList<CompletionItem> insideWhileStatement(const DomItem &currentItem,
                                                  const CompletionContextStrings &ctx)
{
    const auto regions = FileLocations::treeOf(currentItem)->info().regions;
    const QQmlJS::SourceLocation leftParenthesis = regions[LeftParenthesisRegion];
    const QQmlJS::SourceLocation rightParenthesis = regions[RightParenthesisRegion];

    if (betweenLocations(leftParenthesis, ctx, rightParenthesis)) {
        return QQmlLSUtils::scriptIdentifierCompletion(currentItem, ctx);
    }
    if (afterLocation(rightParenthesis, ctx)) {
        return QQmlLSUtils::suggestJSStatementCompletion(currentItem);
    }
    return {};
}

static QList<CompletionItem> insideDoWhileStatement(const DomItem &currentItem,
                                                  const CompletionContextStrings &ctx)
{
    const auto regions = FileLocations::treeOf(currentItem)->info().regions;
    const QQmlJS::SourceLocation doKeyword = regions[DoKeywordRegion];
    const QQmlJS::SourceLocation whileKeyword = regions[WhileKeywordRegion];
    const QQmlJS::SourceLocation leftParenthesis = regions[LeftParenthesisRegion];
    const QQmlJS::SourceLocation rightParenthesis = regions[RightParenthesisRegion];

    if (betweenLocations(doKeyword, ctx, whileKeyword)) {
        return QQmlLSUtils::suggestJSStatementCompletion(currentItem);
    }
    if (betweenLocations(leftParenthesis, ctx, rightParenthesis)) {
        return QQmlLSUtils::scriptIdentifierCompletion(currentItem, ctx);
    }
    return {};
}

static QList<CompletionItem> insideForEachStatement(const DomItem &currentItem,
                                                              const CompletionContextStrings &ctx)
{
    const auto regions = FileLocations::treeOf(currentItem)->info().regions;

    const QQmlJS::SourceLocation inOf = regions[InOfTokenRegion];
    const QQmlJS::SourceLocation leftParenthesis = regions[LeftParenthesisRegion];
    const QQmlJS::SourceLocation rightParenthesis = regions[RightParenthesisRegion];

    if (betweenLocations(leftParenthesis, ctx, inOf)) {
        QList<CompletionItem> res;
        res << QQmlLSUtils::scriptIdentifierCompletion(currentItem, ctx)
            << QQmlLSUtils::suggestVariableDeclarationStatementCompletion();
        return res;
    }
    if (betweenLocations(inOf, ctx, rightParenthesis)) {
        const QList<CompletionItem> res = QQmlLSUtils::scriptIdentifierCompletion(currentItem, ctx);
        return res;
    }

    if (afterLocation(rightParenthesis, ctx)) {
        return QQmlLSUtils::suggestJSStatementCompletion(currentItem);
    }

    return {};
}

static QList<CompletionItem> insideBinaryExpressionCompletion(const DomItem &currentItem,
                                                              const CompletionContextStrings &ctx)
{
    const auto regions = FileLocations::treeOf(currentItem)->info().regions;

    const QQmlJS::SourceLocation operatorLocation = regions[OperatorTokenRegion];

    if (beforeLocation(ctx, operatorLocation)) {
        const DomItem lhs = currentItem.field(Fields::left);
        return QQmlLSUtils::scriptIdentifierCompletion(lhs, ctx);
    }
    if (afterLocation(operatorLocation, ctx)) {
        const DomItem rhs = currentItem.field(Fields::right);
        return QQmlLSUtils::scriptIdentifierCompletion(rhs, ctx);
    }

    return {};
}

QList<CompletionItem> QQmlLSUtils::completions(const DomItem &currentItem,
                                               const CompletionContextStrings &ctx)
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
    */
    for (DomItem currentParent = currentItem; currentParent;
         currentParent = currentParent.directParent()) {
        const DomType currentType = currentParent.internalKind();
        switch (currentType) {
        case DomType::Id:
            // suppress completions for ids
            return {};
        case DomType::Pragma:
            return insidePragmaCompletion(currentItem, ctx);
        case DomType::ScriptType: {
            LocalSymbolsTypes options;
            options.setFlag(LocalSymbolsType::ObjectType);
            options.setFlag(LocalSymbolsType::ValueType);
            return reachableTypes(currentItem, options, CompletionItemKind::Class);
        }
        case DomType::ScriptFormalParameter:
            // no autocompletion inside of function parameter definition
            return {};
        case DomType::Binding:
            return insideBindingCompletion(currentParent, ctx);
        case DomType::ScriptExpression:
            return scriptIdentifierCompletion(currentParent, ctx);
        case DomType::Import:
            return insideImportCompletion(currentParent, ctx);
        case DomType::ScriptForStatement:
            return insideForStatementCompletion(currentParent, ctx);
        case DomType::ScriptBlockStatement:
            return QQmlLSUtils::suggestJSStatementCompletion(currentParent);
        case DomType::QmlFile:
            return insideQmlFileCompletion(currentParent, ctx);
        case DomType::QmlObject:
            return insideQmlObjectCompletion(currentParent, ctx);
        case DomType::MethodInfo:
            // suppress completions
            return {};
        case DomType::PropertyDefinition:
            return insidePropertyDefinitionCompletion(currentParent, ctx);
        case DomType::ScriptBinaryExpression:
            return insideBinaryExpressionCompletion(currentParent, ctx);
        case DomType::ScriptLiteral:
            return insideScriptLiteralCompletion(currentParent, ctx);
        case DomType::ScriptCallExpression:
            return insideCallExpression(currentParent, ctx);
        case DomType::ScriptIfStatement:
            return insideIfStatement(currentParent, ctx);
        case DomType::ScriptReturnStatement:
            return insideReturnStatement(currentParent, ctx);
        case DomType::ScriptWhileStatement:
            return insideWhileStatement(currentParent, ctx);
        case DomType::ScriptDoWhileStatement:
            return insideDoWhileStatement(currentParent, ctx);
        case DomType::ScriptForEachStatement:
            return insideForEachStatement(currentParent, ctx);
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

        // TODO: Implement those statements.
        // In the meanwhile, suppress completions to avoid weird behaviors.
        case DomType::ScriptVariableDeclaration:
        case DomType::ScriptVariableDeclarationEntry:
        case DomType::ScriptArray:
        case DomType::ScriptObject:
        case DomType::ScriptProperty:
        case DomType::ScriptElision:
        case DomType::ScriptArrayEntry:
        case DomType::ScriptPattern:
        case DomType::ScriptSwitchStatement:
        case DomType::ScriptCaseBlock:
        case DomType::ScriptCaseClauses:
        case DomType::ScriptCaseClause:
        case DomType::ScriptDefaultClause:
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
