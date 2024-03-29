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

QString QQmlLSUtils::qualifiersFrom(const DomItem &el)
{
    const bool isAccess = QQmlLSUtils::isFieldMemberAccess(el);
    if (!isAccess && !QQmlLSUtils::isFieldMemberExpression(el))
        return {};

    const DomItem fieldMemberExpressionBeginning = el.filterUp(
            [](DomType, const DomItem &item) { return !QQmlLSUtils::isFieldMemberAccess(item); },
            FilterUpOptions::ReturnOuter);
    QStringList qualifiers =
            QQmlLSUtils::fieldMemberExpressionBits(fieldMemberExpressionBeginning, el);

    QString result;
    for (const QString &qualifier : qualifiers)
        result.append(qualifier).append(QChar(u'.'));
    return result;
}

/*!
   \internal
    Helper to check if item is a Field Member Expression \c {<someExpression>.propertyName}.
*/
bool QQmlLSUtils::isFieldMemberExpression(const DomItem &item)
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
bool QQmlLSUtils::isFieldMemberAccess(const DomItem &item)
{
    auto parent = item.directParent();
    if (!isFieldMemberExpression(parent))
        return false;

    DomItem rightHandSide = parent.field(Fields::right);
    return item == rightHandSide;
}

/*!
   \internal
    Get the bits of a field member expression, like \c{a}, \c{b} and \c{c} for \c{a.b.c}.

   stopAtChild can either be an FieldMemberExpression, a ScriptIdentifierExpression or a default
   constructed DomItem: This exits early before processing Field::right of an
   FieldMemberExpression stopAtChild, or before processing a ScriptIdentifierExpression stopAtChild.
   No early exits if stopAtChild is default constructed.
*/
QStringList QQmlLSUtils::fieldMemberExpressionBits(const DomItem &item, const DomItem &stopAtChild)
{
    const bool isAccess = isFieldMemberAccess(item);
    const bool isExpression = isFieldMemberExpression(item);

    // assume it is a non-qualified name
    if (!isAccess && !isExpression)
        return { item.value().toString() };

    const DomItem stopMarker =
            isFieldMemberExpression(stopAtChild) ? stopAtChild : stopAtChild.directParent();

    QStringList result;
    DomItem current =
            isAccess ? item.directParent() : (isFieldMemberExpression(item) ? item : DomItem{});

    for (; isFieldMemberExpression(current); current = current.field(Fields::right)) {
        result << current.field(Fields::left).value().toString();

        if (current == stopMarker)
            return result;
    }
    result << current.value().toString();

    return result;
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
                // the parser inserts empty Script Expressions for bindings that are not completely
                // written out yet. Ignore them here.
                if (subItem.domItem.internalKind() == DomType::ScriptExpression
                    && subLoc->info().fullRegion.length == 0) {
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
    DomItem prototypes;
    DomItem qmlObject = object.qmlObject();
    // object is (or is inside) an inline component definition
    if (object.internalKind() == DomType::QmlComponent || !qmlObject) {
        prototypes = object.component()
                             .field(Fields::objects)
                             .index(0)
                             .field(QQmlJS::Dom::Fields::prototypes);
    } else {
        // object is (or is inside) a QmlObject
        prototypes = qmlObject.field(QQmlJS::Dom::Fields::prototypes);
    }
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
        if (DomItem type = object.filterUp(
                    [](DomType k, const DomItem &) { return k == DomType::ScriptType; },
                    FilterUpOptions::ReturnOuter)) {

            const QString name = fieldMemberExpressionBits(type.field(Fields::typeName)).join(u'.');
            switch (type.directParent().internalKind()) {
            case DomType::QmlObject:
                // is the type name of a QmlObject, like Item in `Item {...}`
                typeDefinition = baseObject(type.directParent());
                break;
            case DomType::QmlComponent:
                typeDefinition = type.directParent();
                return locationFromDomItem(typeDefinition, FileLocationRegion::IdentifierRegion);
                break;
            default:
                // is a type annotation, like Item in `function f(x: Item) { ... }`
                typeDefinition = object.path(Paths::lookupTypePath(name));
                if (typeDefinition.internalKind() == DomType::Export) {
                    typeDefinition = typeDefinition.field(Fields::type).get();
                }
            }
            break;
        }
        if (DomItem id = object.filterUp(
                    [](DomType k, const DomItem &) { return k == DomType::Id; },
                    FilterUpOptions::ReturnOuter)) {

            typeDefinition = id.field(Fields::referredObject).proceedToScope();
            break;
        }

        auto scope = QQmlLSUtils::resolveExpressionType(
                object, QQmlLSUtilsResolveOptions::ResolveActualTypeForFieldMemberExpression);
        if (!scope)
            return {};

        if (scope->type == QmlObjectIdIdentifier) {
            return QQmlLSUtilsLocation{ scope->semanticScope->filePath(),
                                        scope->semanticScope->sourceLocation() };
        }

        typeDefinition = QQmlLSUtils::sourceLocationToDomItem(
                object.containingFile(), scope->semanticScope->sourceLocation());
        return locationFromDomItem(typeDefinition.component(),
                                   FileLocationRegion::IdentifierRegion);
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
        return SignalOrProperty{ name, MethodIdentifier };
    }
    return std::nullopt;
}

/*!
\internal
Returns a list of names, that when belonging to the same targetType, should be considered equal.
This is used to find signal handlers as usages of their corresponding signals, for example.
*/
static QStringList namesOfPossibleUsages(const QString &name,
                                        const DomItem &item,
                                         const QQmlJSScope::ConstPtr &targetType)
{
    QStringList namesToCheck = { name };
    if (item.internalKind() == DomType::EnumItem || item.internalKind() == DomType::EnumDecl)
        return namesToCheck;

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

template<typename Predicate>
QQmlJSScope::ConstPtr findDefiningScopeIf(QQmlJSScope::ConstPtr referrerScope, Predicate &&check)
{
    QQmlJSScope::ConstPtr result;
    QQmlJSUtils::searchBaseAndExtensionTypes(referrerScope, [&](QQmlJSScope::ConstPtr scope) {
        if (check(scope)) {
            result = scope;
            return true;
        }
        return false;
    });

    return result;
}

/*!
\internal
\brief Finds the scope where a property is first defined.

Starts looking for the name starting from the given scope and traverse through base and
extension types.
*/
static QQmlJSScope::ConstPtr findDefiningScopeForProperty(QQmlJSScope::ConstPtr referrerScope,
                                                          const QString &nameToCheck)
{
    return findDefiningScopeIf(referrerScope, [&nameToCheck](const QQmlJSScope::ConstPtr &scope) {
        return scope->hasOwnProperty(nameToCheck);
    });
}

/*!
\internal
See also findDefiningScopeForProperty().

Special case: you can also bind to a signal handler.
*/
static QQmlJSScope::ConstPtr findDefiningScopeForBinding(QQmlJSScope::ConstPtr referrerScope,
                                                         const QString &nameToCheck)
{
    return findDefiningScopeIf(referrerScope, [&nameToCheck](const QQmlJSScope::ConstPtr &scope) {
        return scope->hasOwnProperty(nameToCheck) || scope->hasOwnMethod(nameToCheck);
    });
}

/*!
\internal
See also findDefiningScopeForProperty().
*/
static QQmlJSScope::ConstPtr findDefiningScopeForMethod(QQmlJSScope::ConstPtr referrerScope,
                                                        const QString &nameToCheck)
{
    return findDefiningScopeIf(referrerScope, [&nameToCheck](const QQmlJSScope::ConstPtr &scope) {
        return scope->hasOwnMethod(nameToCheck);
    });
}

/*!
\internal
See also findDefiningScopeForProperty().
*/
static QQmlJSScope::ConstPtr findDefiningScopeForEnumeration(QQmlJSScope::ConstPtr referrerScope,
                                                             const QString &nameToCheck)
{
    return findDefiningScopeIf(referrerScope, [&nameToCheck](const QQmlJSScope::ConstPtr &scope) {
        return scope->hasOwnEnumeration(nameToCheck);
    });
}

/*!
\internal
See also findDefiningScopeForProperty().
*/
static QQmlJSScope::ConstPtr findDefiningScopeForEnumerationKey(QQmlJSScope::ConstPtr referrerScope,
                                                          const QString &nameToCheck)
{
    return findDefiningScopeIf(referrerScope, [&nameToCheck](const QQmlJSScope::ConstPtr &scope) {
        return scope->hasOwnEnumerationKey(nameToCheck);
    });
}

/*!
    Filter away the parts of the Dom not needed for find usages, by following the profiler's
   information.
    1. "propertyInfos" tries to require all inherited properties of some QmlObject. That is super
   slow (profiler says it eats 90% of the time needed by `tst_qmlls_utils findUsages`!) and is not
   needed for usages.
    2. "get" tries to resolve references, like base types saved in prototypes for example, and is not
   needed to find usages. Profiler says it eats 70% of the time needed by `tst_qmlls_utils
   findUsages`.
    3. "defaultPropertyName" also recurses through base types and is not needed to find usages.
*/
static FieldFilter filterForFindUsages()
{
    FieldFilter filter{ {},
                        {
                                { QString(), QString::fromUtf16(Fields::propertyInfos) },
                                { QString(), QString::fromUtf16(Fields::defaultPropertyName) },
                                { QString(), QString::fromUtf16(Fields::get) },
                        } };
    return filter;
};

static void findUsagesOfNonJSIdentifiers(const DomItem &item, const QString &name,
                                         QList<QQmlLSUtilsLocation> &result)
{
    const auto expressionType = QQmlLSUtils::resolveExpressionType(item, ResolveOwnerType);
    if (!expressionType)
        return;

    const QStringList namesToCheck = namesOfPossibleUsages(name, item, expressionType->semanticScope);

    const auto addLocationIfTypeMatchesTarget = [&result,
                                                 &expressionType](const DomItem &toBeResolved,
                                                                  FileLocationRegion subRegion) {
        const auto currentType = QQmlLSUtils::resolveExpressionType(
                toBeResolved, QQmlLSUtilsResolveOptions::ResolveOwnerType);
        if (!currentType)
            return;

        const QQmlJSScope::ConstPtr target = expressionType->semanticScope;
        const QQmlJSScope::ConstPtr current = currentType->semanticScope;
        if (target == current) {
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

    auto findUsages = [&addLocationIfTypeMatchesTarget, &name,
                       &namesToCheck](Path, const DomItem &current, bool) -> bool {
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
        case DomType::ScriptLiteral: {
            const QString literal = current.field(Fields::value).value().toString();
            if (namesToCheck.contains(literal))
                addLocationIfTypeMatchesTarget(current, MainRegion);
            return continueForChildren;
        }
        case DomType::EnumItem: {
            // Only look for the first enum defined. The inner enums
            // have no way to be accessed.
            const auto parentPath = current.containingObject().pathFromOwner();
            const auto index = parentPath.last().headIndex();
            if (index != 0)
                return continueForChildren;
            const QString enumValue = current.field(Fields::name).value().toString();
            if (namesToCheck.contains(enumValue))
                addLocationIfTypeMatchesTarget(current, IdentifierRegion);
            return continueForChildren;
        }
        case DomType::EnumDecl: {
            // Only look for the first enum defined. The inner enums
            // have no way to be accessed.
            const auto parentPath = current.pathFromOwner();
            const auto index = parentPath.last().headIndex();
            if (index != 0)
                return continueForChildren;
            const QString enumValue = current.field(Fields::name).value().toString();
            if (namesToCheck.contains(enumValue))
                addLocationIfTypeMatchesTarget(current, IdentifierRegion);
            return continueForChildren;
        }
        default:
            return continueForChildren;
        };

        Q_UNREACHABLE_RETURN(continueForChildren);
    };

    const DomItem qmlFiles = item.top().field(Fields::qmlFileWithPath);
    const auto filter = filterForFindUsages();
    for (const QString &file : qmlFiles.keys()) {
        const DomItem currentFileComponents =
                qmlFiles.key(file).field(Fields::currentItem).field(Fields::components);
        currentFileComponents.visitTree(Path(), emptyChildrenVisitor,
                                        VisitOption::Recurse | VisitOption::VisitSelf, findUsages,
                                        emptyChildrenVisitor, filter);
    }
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
            },
            emptyChildrenVisitor, filterForFindUsages());

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
    case DomType::EnumDecl:
    case DomType::EnumItem:
    case DomType::QmlObject:
    case DomType::PropertyDefinition:
    case DomType::Binding:
    case DomType::MethodInfo: {
        const QString name = item.field(Fields::name).value().toString();
        findUsagesHelper(item, name, result);
        break;
    }
    case DomType::QmlComponent: {
        QString name = item.field(Fields::name).value().toString();

        // get rid of extra qualifiers
        if (const auto dotIndex = name.indexOf(u'.'); dotIndex != -1)
            name = name.sliced(dotIndex + 1);
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

/*!
\internal
Searches for a method by traversing the parent scopes.

We assume here that it is possible to call methods from parent scope to simplify things, as the
linting module already warns about calling methods from parent scopes.

Note: in QML, one can only call methods from the current scope, and from the QML file root scope.
Everything else needs a qualifier.
*/
static std::optional<QQmlLSUtilsExpressionType>
methodFromReferrerScope(const QQmlJSScope::ConstPtr &referrerScope, const QString &name,
                        QQmlLSUtilsResolveOptions options)
{
    for (QQmlJSScope::ConstPtr current = referrerScope; current; current = current->parentScope()) {
        if (auto type = hasMethodOrSignal(current, name)) {
            switch (options) {
            case ResolveOwnerType:
                return QQmlLSUtilsExpressionType{ name,
                                                  findDefiningScopeForMethod(current, name),
                                                  *type };
            case ResolveActualTypeForFieldMemberExpression:
                // not implemented, but JS functions have methods and properties
                // see
                // https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Function
                // for the list of properties/methods of functions
                // see also code below for non-qualified method access
                return {};
            }
        }

        if (const auto signalName = QQmlSignalNames::handlerNameToSignalName(name)) {
            if (auto type = hasMethodOrSignal(current, *signalName)) {
                switch (options) {
                case ResolveOwnerType:
                    return QQmlLSUtilsExpressionType{
                        name, findDefiningScopeForMethod(current, *signalName),
                        SignalHandlerIdentifier
                    };
                case ResolveActualTypeForFieldMemberExpression:
                    // Properties and methods of JS methods are not supported yet
                    return {};
                }
            }
        }
    }
    return {};
}


/*!
\internal
See comment on methodFromReferrerScope: the same applies to properties.
*/
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
                return QQmlLSUtilsExpressionType{
                    propertyName, findDefiningScopeForProperty(current, propertyName),
                    resolved->type
                };
            case ResolveActualTypeForFieldMemberExpression:
                return QQmlLSUtilsExpressionType{ propertyName, property.type(),
                                                  resolved->type };
            }
        }
    }
    return {};
}

/*!
\internal
See comment on methodFromReferrerScope: the same applies to property bindings.
*/
static std::optional<QQmlLSUtilsExpressionType>
propertyBindingFromReferrerScope(const QQmlJSScope::ConstPtr &referrerScope, const QString &name,
                          QQmlLSUtilsResolveOptions options)
{
    if (auto bindings = referrerScope->propertyBindings(name); !bindings.isEmpty()) {
        const auto binding = bindings.front();

        if ((binding.bindingType() != QQmlSA::BindingType::AttachedProperty) &&
            (binding.bindingType() != QQmlSA::BindingType::GroupProperty))
            return {};

        const bool bindingIsAttached =
                binding.bindingType() == QQmlSA::BindingType::AttachedProperty;

        const auto getTypeIdentifier = [&bindingIsAttached, &referrerScope, &name]{
            if (bindingIsAttached)
                return AttachedTypeIdentifier;

            // TODO: QTBUG-123618: you can actually have grouped properties on deferred properties
            // that are not id's!
            // If generalized group property, then it is actually an id.
            if (referrerScope->isNameDeferred(name)) {
                return QmlObjectIdIdentifier;
            }
            return GroupedPropertyIdentifier;
        };

        const auto getScope = [&bindingIsAttached, &binding]() -> QQmlJSScope::ConstPtr {
            if (bindingIsAttached)
                return binding.attachingType();

            return binding.groupType();
        };

        switch (options) {
        case ResolveOwnerType: {
            return QQmlLSUtilsExpressionType{
                name,
                // note: always return the type of the attached type as the owner.
                // Find usages on "Keys.", for example, should yield all usages of the "Keys"
                // attached property.
                bindingIsAttached ? getScope() : findDefiningScopeForProperty(referrerScope, name),
                getTypeIdentifier()
            };
        }
        case ResolveActualTypeForFieldMemberExpression:
            return QQmlLSUtilsExpressionType{name, getScope(), getTypeIdentifier()};
        }
    }

    return {};
}

/*! \internal
    Finds the scope within the special elements like Connections,
    PropertyChanges, Bindings or AnchorChanges.
*/
static QQmlJSScope::ConstPtr findScopeOfSpecialItems(QQmlJSScope::ConstPtr scope, const DomItem &item)
{
    if (!scope)
        return {};

    const QSet<QString> specialItems = {u"QQmlConnections"_s,
                                        u"QQuickPropertyChanges"_s,
                                        u"QQmlBind"_s,
                                        u"QQuickAnchorChanges"_s};

    const auto special = QQmlJSUtils::searchBaseAndExtensionTypes(
            scope, [&specialItems](QQmlJSScope::ConstPtr visitedScope) {
                const auto typeName = visitedScope->internalName();
                if (specialItems.contains(typeName))
                    return true;
                return false;
            });

    if (!special)
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
        // target: someId
        auto resolver = item.containingFile().ownerAs<QmlFile>()->typeResolver();
        if (!resolver)
            return {};

        // Note: It does not have to be an ID. It can be a property.
        return resolver->containedType(resolver->scopedType(scope, targetName));
    } else {
        if (item.internalKind() == DomType::Binding &&
            item.field(Fields::bindingType).value().toInteger() == int(BindingType::OnBinding)) {
                // Binding on sth : {} syntax
                // Target scope is the current scope
                return scope;
        }
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

    if (auto scope = methodFromReferrerScope(owner->semanticScope, name, options))
        return *scope;

    if (auto scope = propertyBindingFromReferrerScope(owner->semanticScope,name, options))
        return *scope;

    if (auto scope = propertyFromReferrerScope(owner->semanticScope, name, options))
        return *scope;

    // Ignore enum usages from other files for now.
    if (owner->type == QmlComponentIdentifier) {
        // Check if name is a enum value <TypeName>.<EnumValue>
        // Enumerations should live under the root element scope of the file that defines the enum,
        // therefore use the DomItem to find the root element of the qml file instead of directly
        // using owner->semanticScope.
        const auto scope = item.goToFile(owner->semanticScope->filePath())
                                   .rootQmlObject(GoTo::MostLikely)
                                   .semanticScope();
        if (scope->hasEnumerationKey(name)) {
            return QQmlLSUtilsExpressionType{name, scope, EnumeratorValueIdentifier};
        }
        // Or it is a enum name <TypeName>.<EnumName>.<EnumValue>
        else if (scope->hasEnumeration(name)) {
            return QQmlLSUtilsExpressionType{name, scope, EnumeratorIdentifier};
        }

        // check inline components <TypeName>.<InlineComponentName>
        for (auto it = owner->semanticScope->childScopesBegin(),
                  end = owner->semanticScope->childScopesEnd();
             it != end; ++it) {
            if ((*it)->inlineComponentName() == name) {
                return QQmlLSUtilsExpressionType{ name, *it, QmlComponentIdentifier };
            }
        }
        return {};
    }

    qCDebug(QQmlLSUtilsLog) << "Could not find identifier expression for" << item.internalKindStr();
    return owner;
}

static std::optional<QQmlLSUtilsExpressionType>
resolveIdentifierExpressionType(const DomItem &item, QQmlLSUtilsResolveOptions options)
{
    if (QQmlLSUtils::isFieldMemberAccess(item)) {
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

        // its a (inline) component!
        return QQmlLSUtilsExpressionType{ name, scope, QmlComponentIdentifier };
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

static std::optional<QQmlLSUtilsExpressionType>
resolveSignalOrPropertyExpressionType(const QString &name, const QQmlJSScope::ConstPtr &scope,
                                      QQmlLSUtilsResolveOptions options)
{
    auto signalOrProperty = resolveNameInQmlScope(name, scope);
    if (!signalOrProperty)
        return {};

    switch (signalOrProperty->type) {
    case PropertyIdentifier:
        switch (options) {
        case ResolveOwnerType:
            return QQmlLSUtilsExpressionType{ name, findDefiningScopeForProperty(scope, name),
                                              signalOrProperty->type };
        case ResolveActualTypeForFieldMemberExpression:
            return QQmlLSUtilsExpressionType{ name, scope->property(name).type(),
                                              signalOrProperty->type };
        }
        Q_UNREACHABLE_RETURN({});
    case PropertyChangedHandlerIdentifier:
        switch (options) {
        case ResolveOwnerType:
            return QQmlLSUtilsExpressionType{
                name, findDefiningScopeForProperty(scope, signalOrProperty->name),
                signalOrProperty->type
            };
        case ResolveActualTypeForFieldMemberExpression:
            // Properties and methods are not implemented on methods.
            Q_UNREACHABLE_RETURN({});
        }
        Q_UNREACHABLE_RETURN({});
    case SignalHandlerIdentifier:
    case PropertyChangedSignalIdentifier:
    case SignalIdentifier:
    case MethodIdentifier:
        switch (options) {
        case ResolveOwnerType: {
            return QQmlLSUtilsExpressionType{ name, findDefiningScopeForMethod(scope, name),
                                              signalOrProperty->type };
        }
        case ResolveActualTypeForFieldMemberExpression:
            // Properties and methods are not implemented on methods.
            Q_UNREACHABLE_RETURN({});
        }
        Q_UNREACHABLE_RETURN({});
    default:
        Q_UNREACHABLE_RETURN({});
    }
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
        if (propertyDefinition && propertyDefinition->semanticScope()) {
            const auto &scope = propertyDefinition->semanticScope();
            switch (options) {
            case ResolveOwnerType:
                return QQmlLSUtilsExpressionType{ propertyDefinition->name, scope,
                                                  PropertyIdentifier };
            case ResolveActualTypeForFieldMemberExpression:
                // There should not be any PropertyDefinition inside a FieldMemberExpression.
                Q_UNREACHABLE_RETURN({});
            }
            Q_UNREACHABLE_RETURN({});
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

            if (QQmlJSScope::ConstPtr targetScope = findScopeOfSpecialItems(owner.value(), item)) {
                const auto signalOrProperty = resolveNameInQmlScope(name, targetScope);
                if (!signalOrProperty)
                    return {};
                switch (options) {
                case ResolveOwnerType:
                    return QQmlLSUtilsExpressionType{
                        name, findDefiningScopeForBinding(targetScope, signalOrProperty->name),
                        signalOrProperty->type
                    };
                case ResolveActualTypeForFieldMemberExpression:
                    // Bindings can't be inside of FieldMemberExpressions.
                    Q_UNREACHABLE_RETURN({});
                }
            }
            if (auto result = resolveSignalOrPropertyExpressionType(name, owner.value(), options)) {
                return result;
            }
            qDebug(QQmlLSUtilsLog) << "QQmlLSUtils::resolveExpressionType() could not resolve the"
                                      "type of a Binding.";
        }

        return {};
    }
    case DomType::QmlObject: {
        auto object = item.as<QmlObject>();
        if (!object)
            return {};
        if (auto scope = object->semanticScope()) {
            const auto name = item.name();
            const bool isComponent = name.front().isUpper();
            if (isComponent)
                scope = scope->baseType();
            const QQmlLSUtilsIdentifierType type =
                    isComponent ? QmlComponentIdentifier : GroupedPropertyIdentifier;
            switch (options) {
            case ResolveOwnerType:
                return QQmlLSUtilsExpressionType{ name, scope, type };
            case ResolveActualTypeForFieldMemberExpression:
                return QQmlLSUtilsExpressionType{ name, scope, type};
            }
        }
        return {};
    }
    case DomType::QmlComponent: {
        auto component = item.as<QmlComponent>();
        if (!component)
            return {};
        const auto scope = component->semanticScope();
        if (!scope)
            return {};

        QString name = item.name();
        if (auto dotIndex = name.indexOf(u'.'); dotIndex != -1)
            name = name.sliced(dotIndex + 1);
        switch (options) {
        case ResolveOwnerType:
            return QQmlLSUtilsExpressionType{ name, scope, QmlComponentIdentifier };
        case ResolveActualTypeForFieldMemberExpression:
            return QQmlLSUtilsExpressionType{ name, scope, QmlComponentIdentifier };
        }
        Q_UNREACHABLE_RETURN({});
    }
    case DomType::MethodInfo: {
        auto object = item.as<MethodInfo>();
        if (object && object->semanticScope()) {
            std::optional<QQmlJSScope::ConstPtr> scope = object->semanticScope();
            if (!scope)
                return {};

            if (QQmlJSScope::ConstPtr targetScope =
                        findScopeOfSpecialItems(scope.value()->parentScope(), item)) {
                const auto signalOrProperty = resolveNameInQmlScope(object->name, targetScope);
                if (!signalOrProperty)
                    return {};

                switch (options) {
                case ResolveOwnerType:
                    return QQmlLSUtilsExpressionType{ object->name,
                                                      findDefiningScopeForMethod(
                                                              targetScope, signalOrProperty->name),
                                                      signalOrProperty->type };
                case ResolveActualTypeForFieldMemberExpression:
                    // not supported for methods
                    return {};
                }
            }

            // in case scope is the semantic scope for the function bodies: grab the owner's scope
            // this happens for all methods but not for signals (they do not have a body)
            if (scope.value()->scopeType() == QQmlJSScope::ScopeType::JSFunctionScope)
                scope = scope.value()->parentScope();

            if (auto result = resolveSignalOrPropertyExpressionType(object->name, scope.value(),
                                                                    options)) {
                return result;
            }
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
    case DomType::ScriptLiteral: {
        /* special case
        Binding { target: someId; property: "someProperty"}
        */
        const auto scope = item.qmlObject().semanticScope();
        const auto name = item.field(Fields::value).value().toString();
        if (QQmlJSScope::ConstPtr targetScope = findScopeOfSpecialItems(scope, item)) {
            const auto signalOrProperty = resolveNameInQmlScope(name, targetScope);
            if (!signalOrProperty)
                return {};
            switch (options) {
            case ResolveOwnerType:
                return QQmlLSUtilsExpressionType{
                    name, findDefiningScopeForProperty(targetScope, signalOrProperty->name),
                    signalOrProperty->type
                };
            case ResolveActualTypeForFieldMemberExpression:
                // ScriptLiteral's can't be inside of FieldMemberExpression's, especially when they
                // are inside a special item.
                Q_UNREACHABLE_RETURN({});
            }
        }
        return {};
    }
    case DomType::EnumItem: {
        const QString enumValue = item.field(Fields::name).value().toString();
         QQmlJSScope::ConstPtr referrerScope = item.rootQmlObject(GoTo::MostLikely).semanticScope();
         if (!referrerScope->hasEnumerationKey(enumValue))
             return {};
         switch (options) {
             // special case: use the owner's scope here, as enums do not have their own
             // QQmlJSScope.
         case ResolveActualTypeForFieldMemberExpression:
         case ResolveOwnerType:
             return QQmlLSUtilsExpressionType{
                 enumValue, findDefiningScopeForEnumerationKey(referrerScope, enumValue),
                 EnumeratorValueIdentifier
             };
             return {};
         }
         Q_UNREACHABLE_RETURN({});
    }
    case DomType::EnumDecl: {
        const QString enumName = item.field(Fields::name).value().toString();
        QQmlJSScope::ConstPtr referrerScope = item.rootQmlObject(GoTo::MostLikely).semanticScope();
        if (!referrerScope->hasEnumeration(enumName))
            return {};
        switch (options) {
        // special case: use the owner's scope here, as enums do not have their own QQmlJSScope.
        case ResolveActualTypeForFieldMemberExpression:
        case ResolveOwnerType:
            return QQmlLSUtilsExpressionType{
                enumName, findDefiningScopeForEnumeration(referrerScope, enumName),
                EnumeratorIdentifier
            };
        }

        Q_UNREACHABLE_RETURN({});
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
    DomItem owner = QQmlLSUtils::sourceLocationToDomItem(file, location).qmlObject();
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
    DomItem propertyOwner =
            QQmlLSUtils::sourceLocationToDomItem(file, propertyDefinitionLocation).qmlObject();
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
    auto resolvedExpression =
            resolveExpressionType(item, QQmlLSUtilsResolveOptions::ResolveOwnerType);

    if (!resolvedExpression || !resolvedExpression->name || !resolvedExpression->semanticScope) {
        qCDebug(QQmlLSUtilsLog) << "QQmlLSUtils::findDefinitionOf: Type could not be resolved.";
        return {};
    }

    switch (resolvedExpression->type) {
    case JavaScriptIdentifier: {
        const QQmlJS::SourceLocation location =
                resolvedExpression->semanticScope->ownJSIdentifier(*resolvedExpression->name)
                        .value()
                        .location;

        return QQmlLSUtilsLocation{ resolvedExpression->semanticScope->filePath(), location };
    }

    case PropertyIdentifier: {
        const DomItem ownerFile = item.goToFile(resolvedExpression->semanticScope->filePath());
        const QQmlJS::SourceLocation ownerLocation =
                resolvedExpression->semanticScope->sourceLocation();
        return findPropertyDefinitionOf(ownerFile, ownerLocation, *resolvedExpression->name);
    }
    case PropertyChangedSignalIdentifier:
    case PropertyChangedHandlerIdentifier:
    case SignalIdentifier:
    case SignalHandlerIdentifier:
    case MethodIdentifier: {
        const DomItem ownerFile = item.goToFile(resolvedExpression->semanticScope->filePath());
        const QQmlJS::SourceLocation ownerLocation =
                resolvedExpression->semanticScope->sourceLocation();
        return findMethodDefinitionOf(ownerFile, ownerLocation, *resolvedExpression->name);
    }
    case QmlObjectIdIdentifier: {
        DomItem qmlObject = QQmlLSUtils::sourceLocationToDomItem(
                item.containingFile(), resolvedExpression->semanticScope->sourceLocation());
        // in the Dom, the id is saved in a QMultiHash inside the Component of an QmlObject.
        const DomItem domId = qmlObject.component()
                                      .field(Fields::ids)
                                      .key(*resolvedExpression->name)
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
    case SingletonIdentifier:
    case EnumeratorIdentifier:
    case EnumeratorValueIdentifier:
    case AttachedTypeIdentifier:
    case GroupedPropertyIdentifier:
    case QmlComponentIdentifier:
        qCDebug(QQmlLSUtilsLog) << "QQmlLSUtils::findDefinitionOf was not implemented for type"
                                << resolvedExpression->type;
        return {};
    }
    Q_UNREACHABLE_RETURN({});
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
    case SingletonIdentifier:
    case EnumeratorIdentifier:
    case EnumeratorValueIdentifier:
    case AttachedTypeIdentifier:
    case GroupedPropertyIdentifier:
    case QmlComponentIdentifier:
        return ownerType.semanticScope;
    }
    return {};
}

std::optional<QQmlLSUtilsErrorMessage> QQmlLSUtils::checkNameForRename(
        const DomItem &item, const QString &dirtyNewName,
        const std::optional<QQmlLSUtilsExpressionType> &ownerType)
{
    if (!ownerType) {
        if (const auto resolved = QQmlLSUtils::resolveExpressionType(item, ResolveOwnerType))
            return checkNameForRename(item, dirtyNewName, resolved);
    }

    // general checks for ECMAscript identifiers
    if (!isValidEcmaScriptIdentifier(dirtyNewName))
        return QQmlLSUtilsErrorMessage{ 0, u"Invalid EcmaScript identifier!"_s };

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
QList<QQmlLSUtilsEdit> QQmlLSUtils::renameUsagesOf(
        const DomItem &item, const QString &dirtyNewName,
        const std::optional<QQmlLSUtilsExpressionType> &targetType)
{
    QList<QQmlLSUtilsEdit> results;
    const QList<QQmlLSUtilsLocation> locations = findUsagesOf(item);
    if (locations.isEmpty())
        return results;

    auto oldName = oldNameFrom(item);
    if (!oldName)
        return results;

    QQmlJSScope::ConstPtr semanticScope;
    if (targetType) {
        semanticScope = targetType->semanticScope;
    } else if (const auto resolved = QQmlLSUtils::resolveExpressionType(
                       item, QQmlLSUtilsResolveOptions::ResolveOwnerType)) {
        semanticScope = resolved->semanticScope;
    } else {
        return results;
    }

    QString newName;
    if (const auto resolved = resolveNameInQmlScope(*oldName, semanticScope)) {
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


/*!
\internal
Returns the name of the cmake program along with the arguments needed to build the
qmltyperegistration. This command generates the .qmltypes, qmldir and .qrc files required for qmlls
to provide correct information on C++ defined QML elements.

We assume here that CMake is available in the path. This should be the case for linux and macOS by
default.
For windows, having CMake in the path is not too unrealistic, for example,
https://doc.qt.io/qt-6/windows-building.html#step-2-install-build-requirements claims that you need
to have CMake in your path to build Qt. So a developer machine running qmlls has a high chance of
having CMake in their path, if CMake is installed and used.
*/
QPair<QString, QStringList> QQmlLSUtils::cmakeBuildCommand(const QString &path)
{
    const QPair<QString, QStringList> result{
        u"cmake"_s, { u"--build"_s, path, u"-t"_s, u"all_qmltyperegistrations"_s }
    };
    return result;
}


QByteArray QQmlLSUtils::getDocumentationFromLocation(const DomItem &file, const QQmlLSUtilsTextPosition &position)
{
    QByteArray result;
    const auto [line, character] = position;
    const auto itemLocation = itemsFromTextLocation(file,  line, character);

    // TODO:
    // Process found item's internalKind and fetch its documentation.
    Q_UNUSED(itemLocation);

    return result;
}

QT_END_NAMESPACE
