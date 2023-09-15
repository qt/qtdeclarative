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
                                                              const QString &regionName = QString())
{
    QQmlLSUtilsLocation location;
    location.filename = item.canonicalFilePath();

    auto tree = FileLocations::treeOf(item);
    // tree is null for C++ defined types, for example
    if (!tree)
        return {};

    auto info = tree->info();
    if (!regionName.isEmpty() && info.regions.contains(regionName)) {
        location.sourceLocation = info.regions[regionName];
    } else {
        location.sourceLocation = info.fullRegion;
    }
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

        return locationFromDomItem(typeDefinition, u"identifier"_s);
    }
    default:
        qDebug() << "QQmlLSUtils::findTypeDefinitionOf: Found unimplemented Type"
                 << object.internalKindStr();
        return {};
    }

    return locationFromDomItem(typeDefinition);
}

static bool findDefinitionFromItem(const DomItem &item, const QString &name)
{
    if (const QQmlJSScope::ConstPtr &scope = item.semanticScope()) {
        qCDebug(QQmlLSUtilsLog) << "Searching for definition in" << item.internalKindStr();
        if (auto jsIdentifier = scope->JSIdentifier(name)) {
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

static void findUsagesOfNonJSIdentifiers(const DomItem &item, const QString &name,
                                         QList<QQmlLSUtilsLocation> &result)
{
    auto expressionType =
            QQmlLSUtils::resolveExpressionType(item, QQmlLSUtilsResolveOptions::ResolveOwnerType);
    if (!expressionType)
        return;
    QQmlJSScope::ConstPtr targetType = expressionType->semanticScope;

    const QStringList namesToCheck = namesOfPossibleUsages(name, targetType);

    auto checkName = [&namesToCheck](const QString &nameToCheck) -> bool {
        return namesToCheck.contains(nameToCheck);
    };

    auto findUsages = [&targetType, &result, &name, &checkName](Path, const DomItem &current,
                                                                bool) -> bool {
        bool resolveType = false;
        bool continueForChildren = true;
        DomItem toBeResolved = current;
        QString subRegion;

        if (auto scope = current.semanticScope()) {
            // is the current property shadowed by some JS identifier? ignore current + its children
            if (scope->JSIdentifier(name)) {
                return false;
            }
        }

        switch (current.internalKind()) {
        case DomType::Binding:
        case DomType::PropertyDefinition: {
            const QString propertyName = current.field(Fields::name).value().toString();
            if (!checkName(propertyName))
                return true;

            subRegion = u"identifier"_s;
            resolveType = true;
            break;
        }
        case DomType::ScriptIdentifierExpression: {
            const QString identifierName = current.field(Fields::identifier).value().toString();
            if (!checkName(identifierName))
                return true;

            resolveType = true;
            break;
        }
        case DomType::MethodInfo: {
            const QString methodName = current.field(Fields::name).value().toString();
            if (!checkName(methodName))
                return true;

            subRegion = u"identifier"_s;
            resolveType = true;
            break;
        }
        default:
            break;
        };

        if (resolveType) {
            auto currentType = QQmlLSUtils::resolveExpressionType(
                    toBeResolved, QQmlLSUtilsResolveOptions::ResolveOwnerType);
            if (!currentType)
                return continueForChildren;

            qCDebug(QQmlLSUtilsLog) << "Will resolve type of" << toBeResolved.internalKindStr();
            if (currentType->semanticScope == targetType) {
                auto tree = FileLocations::treeOf(current);
                QQmlJS::SourceLocation sourceLocation;

                if (subRegion.isEmpty()) {
                    sourceLocation = tree->info().fullRegion;
                } else {
                    auto regions = tree->info().regions;
                    auto it = regions.constFind(subRegion);
                    if (it == regions.constEnd())
                        return continueForChildren;
                    sourceLocation = *it;
                }

                QQmlLSUtilsLocation location{ current.canonicalFilePath(), sourceLocation };
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

static QQmlLSUtilsLocation locationFromJSIdentifierDefinition(const DomItem &definitionOfItem,
                                                              const QString &name)
{
    Q_ASSERT_X(!definitionOfItem.semanticScope().isNull()
                       && definitionOfItem.semanticScope()->JSIdentifier(name).has_value(),
               "QQmlLSUtils::locationFromJSIdentifierDefinition",
               "JS definition does not actually define the JS identifier. "
               "Did you obtain definitionOfItem from findJSIdentifierDefinition() ?");
    QQmlJS::SourceLocation location =
            definitionOfItem.semanticScope()->JSIdentifier(name).value().location;

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
                           scope && scope->JSIdentifier(name)) {
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
        if (auto methodType = hasMethodOrSignal(current, name))
            return QQmlLSUtilsExpressionType{ name, current, *methodType };

        if (const auto signalName = QQmlSignalNames::handlerNameToSignalName(name)) {
            if (auto methodType = hasMethodOrSignal(current, *signalName)) {
                return QQmlLSUtilsExpressionType{ name, current, *methodType };
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
        if (auto property = current->property(propertyName); property.isValid()) {
            switch (options) {
            case ResolveOwnerType:
                return QQmlLSUtilsExpressionType{ propertyName, current,
                                                  QQmlLSUtilsIdentifierType::PropertyIdentifier };
            case ResolveActualTypeForFieldMemberExpression:
                return QQmlLSUtilsExpressionType{ propertyName, property.type(),
                                                  QQmlLSUtilsIdentifierType::PropertyIdentifier };
            }
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
        return resolver.value()->scopeForId(targetName, scope);
    } else {
        // No binding to target property, return the container object's scope
        return scope->parentScope();
    }

    return {};
}

static std::optional<QQmlLSUtilsExpressionType>
resolveIdentifierExpressionType(const DomItem &item, QQmlLSUtilsResolveOptions options)
{
    const auto referrerScope = item.nearestSemanticScope();
    if (!referrerScope)
        return {};

    const QString name = item.field(Fields::identifier).value().toString();

    if (isFieldMemberAccess(item)) {
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
                QQmlLSUtilsIdentifierType type = isSignal
                        ? QQmlLSUtilsIdentifierType::SignalIdentifier
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
    } else {
        DomItem definitionOfItem = findJSIdentifierDefinition(item, name);
        if (definitionOfItem) {
            Q_ASSERT_X(!definitionOfItem.semanticScope().isNull()
                               && definitionOfItem.semanticScope()->JSIdentifier(name),
                       "QQmlLSUtils::findDefinitionOf",
                       "JS definition does not actually define the JS identifer. "
                       "It should be empty.");
            auto scope = definitionOfItem.semanticScope();
            auto jsIdentifier = scope->JSIdentifier(name);
            if (jsIdentifier->scope) {
                return QQmlLSUtilsExpressionType{ name, jsIdentifier->scope.toStrongRef(),
                                                  QQmlLSUtilsIdentifierType::JavaScriptIdentifier };
            } else {
                return QQmlLSUtilsExpressionType{ name, scope,
                                                  QQmlLSUtilsIdentifierType::JavaScriptIdentifier };
            }
        }

        // check if its a method
        if (auto scope = methodFromReferrerScope(referrerScope, name, options))
            return scope;

        // check if its an (unqualified) property
        if (auto scope = propertyFromReferrerScope(referrerScope, name, options))
            return *scope;
    }

    // check if its an id
    auto resolver = item.containingFile().ownerAs<QmlFile>()->typeResolver();
    if (!resolver)
        return {};
    QQmlJSScope::ConstPtr fromId = resolver.value()->scopeForId(name, referrerScope);
    if (fromId)
        return QQmlLSUtilsExpressionType{ name, fromId, QmlObjectIdIdentifier };

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
        if (object && object->semanticScope())
            return QQmlLSUtilsExpressionType{ std::nullopt, object->semanticScope(),
                                              QmlObjectIdentifier };

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

    if (auto it = regions.constFind(u"identifier"_s); it != regions.constEnd()) {
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

    if (auto it = regions.constFind(u"identifier"_s); it != regions.constEnd()) {
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
        QQmlJSScope::ConstPtr fromId = resolver.value()->scopeForId(name, referrerScope);
        if (fromId) {
            DomItem qmlObject = QQmlLSUtils::sourceLocationToDomItem(item.containingFile(),
                                                                     fromId->sourceLocation());
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

static std::optional<QQmlJSScope::ConstPtr> propertyOwnerFrom(const QQmlJSScope::ConstPtr &type,
                                                              const std::optional<QString> &name)
{
    if (!name) {
        qCDebug(QQmlLSUtilsLog) << "QQmlLSUtils::checkNameForRename cannot find property name,"
                                   " ignoring.";
        return {};
    }

    QQmlJSScope::ConstPtr typeWithDefinition = type;
    while (!typeWithDefinition->hasOwnProperty(*name)) {
        typeWithDefinition = type->baseType();
        if (!typeWithDefinition) {
            qCDebug(QQmlLSUtilsLog)
                    << "QQmlLSUtils::checkNameForRename cannot find property definition,"
                       " ignoring.";
            return {};
        }
    }
    return typeWithDefinition;
}

static std::optional<QQmlJSScope::ConstPtr> methodOwnerFrom(const QQmlJSScope::ConstPtr &type,
                                                            const std::optional<QString> &name)
{
    if (!name) {
        qCDebug(QQmlLSUtilsLog) << "QQmlLSUtils::checkNameForRename cannot find method name,"
                                   " ignoring.";
        return {};
    }

    QQmlJSScope::ConstPtr typeWithDefinition = type;
    while (!typeWithDefinition->hasOwnMethod(*name)) {
        typeWithDefinition = type->baseType();
        if (!typeWithDefinition) {
            qCDebug(QQmlLSUtilsLog)
                    << "QQmlLSUtils::checkNameForRename cannot find method definition,"
                       " ignoring.";
            return {};
        }
    }
    return typeWithDefinition;
}

static std::optional<QQmlJSScope::ConstPtr>
expressionTypeWithDefinition(const QQmlLSUtilsExpressionType &ownerType)
{
    switch (ownerType.type) {
    case PropertyIdentifier:
        return propertyOwnerFrom(ownerType.semanticScope, ownerType.name);
    case PropertyChangedHandlerIdentifier: {
        const auto propertyName =
                QQmlSignalNames::changedHandlerNameToPropertyName(*ownerType.name);
        return propertyOwnerFrom(ownerType.semanticScope, propertyName);
        break;
    }
    case PropertyChangedSignalIdentifier: {
        const auto propertyName = QQmlSignalNames::changedSignalNameToPropertyName(*ownerType.name);
        return propertyOwnerFrom(ownerType.semanticScope, propertyName);
    }
    case MethodIdentifier:
    case SignalIdentifier:
        return methodOwnerFrom(ownerType.semanticScope, ownerType.name);
    case SignalHandlerIdentifier: {
        const auto signalName = QQmlSignalNames::handlerNameToSignalName(*ownerType.name);
        return methodOwnerFrom(ownerType.semanticScope, signalName);
    }
    case JavaScriptIdentifier:
    case QmlObjectIdIdentifier:
    case QmlObjectIdentifier:
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
    if (!typeWithDefinition.value()->isComposite()) {
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

QList<CompletionItem> QQmlLSUtils::bindingsCompletions(const DomItem &containingObject)
{
    // returns valid bindings completions (i.e. reachable properties and signal handlers)
    QList<CompletionItem> res;
    qCDebug(QQmlLSCompletionLog) << "binding completions";
    containingObject.visitPrototypeChain(
            [&res](const DomItem &it) {
                qCDebug(QQmlLSCompletionLog)
                        << "prototypeChain" << it.internalKindStr() << it.canonicalPath();
                if (const QmlObject *itPtr = it.as<QmlObject>()) {
                    // signal handlers
                    auto methods = itPtr->methods();
                    auto it = methods.cbegin();
                    while (it != methods.cend()) {
                        if (it.value().methodType == MethodInfo::MethodType::Signal) {
                            CompletionItem comp;
                            QString signal = it.key();
                            comp.label = QQmlSignalNames::signalNameToHandlerName(signal).toUtf8();
                            res.append(comp);
                        }
                        ++it;
                    }
                    // properties that can be bound
                    auto pDefs = itPtr->propertyDefs();
                    for (auto it2 = pDefs.keyBegin(); it2 != pDefs.keyEnd(); ++it2) {
                        qCDebug(QQmlLSCompletionLog) << "adding property" << *it2;
                        CompletionItem comp;
                        comp.label = it2->toUtf8();
                        comp.insertText = (*it2 + u": "_s).toUtf8();
                        comp.kind = int(CompletionItemKind::Property);
                        res.append(comp);
                    }
                }
                return true;
            },
            VisitPrototypesOption::Normal);
    return res;
}

QList<CompletionItem> QQmlLSUtils::importCompletions(const DomItem &file,
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

static void reachableTypes(QSet<QString> &symbols, const DomItem &el, LocalSymbolsTypes options)
{
    switch (el.internalKind()) {
    case DomType::ImportScope: {

        const QSet<QString> localSymbols = el.localSymbolNames(options);
        qCDebug(QQmlLSCompletionLog) << "adding local symbols of:" << el.internalKindStr()
                                     << el.canonicalPath() << localSymbols;
        symbols += localSymbols;
        break;
    }
    default: {
        qCDebug(QQmlLSCompletionLog) << "skipping local symbols for non type"
                                     << el.internalKindStr() << el.canonicalPath();
        break;
    }
    }
}

QList<CompletionItem> QQmlLSUtils::reachableSymbols(const DomItem &context,
                                                    const CompletionContextStrings &ctx,
                                                    TypeCompletionOptions typeCompletionType)
{
    // returns completions for the reachable types or attributes from context
    QList<CompletionItem> res;
    QMap<CompletionItemKind, QSet<QString>> symbols;
    QSet<quintptr> visited;
    QList<Path> visitedRefs;
    auto addLocalSymbols = [&typeCompletionType, &symbols](const DomItem &el) {
        LocalSymbolsTypes options;
        if (typeCompletionType.testFlag(TypeCompletionOption::Types)) {
            options.setFlag(LocalSymbolsType::Namespaces);
            options.setFlag(LocalSymbolsType::Types);
        }
        if (typeCompletionType.testFlag(TypeCompletionOption::QmlTypes)) {
            options.setFlag(LocalSymbolsType::QmlTypes);
            options.setFlag(LocalSymbolsType::Namespaces);
        }
        if (options != LocalSymbolsType::None) {
            reachableTypes(symbols[CompletionItemKind::Class], el, options);
        }
        return true;
    };
    if (ctx.base().isEmpty()) {
        if (typeCompletionType != TypeCompletionOption::None) {
            qCDebug(QQmlLSCompletionLog)
                    << "adding symbols reachable from:" << context.internalKindStr()
                    << context.canonicalPath();
            DomItem it = context.proceedToScope();
            it.visitScopeChain(addLocalSymbols, LookupOption::Normal, &defaultErrorHandler,
                               &visited, &visitedRefs);
        }
    } else {
        QList<QStringView> baseItems = ctx.base().split(u'.', Qt::SkipEmptyParts);
        Q_ASSERT(!baseItems.isEmpty());
        auto addReachableSymbols = [&visited, &visitedRefs, &addLocalSymbols](Path,
                                                                              const DomItem &it) -> bool {
            qCDebug(QQmlLSCompletionLog) << "adding directly accessible symbols of"
                                         << it.internalKindStr() << it.canonicalPath();
            it.visitDirectAccessibleScopes(addLocalSymbols, VisitPrototypesOption::Normal,
                                           &defaultErrorHandler, &visited, &visitedRefs);
            return true;
        };
        Path toSearch = Paths::lookupSymbolPath(ctx.base().toString().chopped(1));
        context.resolve(toSearch, addReachableSymbols, &defaultErrorHandler);
        // add attached types? technically we should...
    }
    for (auto symbolKinds = symbols.constBegin(); symbolKinds != symbols.constEnd();
         ++symbolKinds) {
        for (auto symbol = symbolKinds.value().constBegin();
             symbol != symbolKinds.value().constEnd(); ++symbol) {
            CompletionItem comp;
            comp.label = symbol->toUtf8();
            comp.kind = int(symbolKinds.key());
            res.append(comp);
        }
    }
    return res;
}

static QList<CompletionItem> jsIdentifierCompletion(const QQmlJSScope *scope,
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

static QList<CompletionItem> methodCompletion(const QQmlJSScope *scope,
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

static QList<CompletionItem> propertyCompletion(const QQmlJSScope *scope,
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
decltype(auto) collectFromAllJavaScriptParents(const QQmlJSScope *scope, T... args)
{
    decltype(F(scope, args...)) result;
    for (const QQmlJSScope *current = scope; current; current = current->parentScope().get()) {
        result << F(current, args...);
        if (current->scopeType() == QQmlSA::ScopeType::QMLScope)
            break;
    }
    return result;
}

QList<CompletionItem> QQmlLSUtils::scriptIdentifierCompletion(const DomItem &context,
                                                              const CompletionContextStrings &ctx)
{
    QList<CompletionItem> result;
    QDuplicateTracker<QString> usedNames;
    const QQmlJSScope *nearestScope;
    const bool hasQualifier = !ctx.base().isEmpty();

    if (!hasQualifier) {
        result << idsCompletions(context.component());

        auto scope = context.nearestSemanticScope();
        if (!scope)
            return result;
        nearestScope = scope.get();
    } else {
        auto expressionType = QQmlLSUtils::resolveExpressionType(context, ResolveOwnerType);
        if (!expressionType)
            return result;
        nearestScope = expressionType->semanticScope.get();
    }

    if (!nearestScope)
        return result;

    result << methodCompletion(nearestScope, &usedNames)
           << propertyCompletion(nearestScope, &usedNames);

    if (!hasQualifier) {
        // collect all of the stuff from parents
        result << collectFromAllJavaScriptParents<jsIdentifierCompletion>(nearestScope, &usedNames)
               << collectFromAllJavaScriptParents<methodCompletion>(nearestScope, &usedNames)
               << collectFromAllJavaScriptParents<propertyCompletion>(nearestScope, &usedNames);
    }

    return result;
}

static const QQmlJSScope *resolve(const QQmlJSScope *current, QStringList names)
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
    auto region = location.regions.constFind(u"colon"_s);

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

static QList<CompletionItem> pragmaCompletion(QQmlJS::Dom::DomItem currentItem,
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

QList<CompletionItem> QQmlLSUtils::completions(const DomItem &currentItem,
                                               const CompletionContextStrings &ctx)
{
    if (currentItem.internalKind() == DomType::Id) {
        // suppress completions for ids
        return {};
    }

    if (currentItem.internalKind() == DomType::Pragma) {
        return pragmaCompletion(currentItem, ctx);
    }

    const DomItem containingType = currentItem.filterUp(
            [](DomType type, const QQmlJS::Dom::DomItem &) { return type == DomType::ScriptType; },
            FilterUpOptions::ReturnInner);
    if (containingType) {
        TypeCompletionOptions typeCompletionType;
        typeCompletionType.setFlag(TypeCompletionOption::Types);
        typeCompletionType.setFlag(TypeCompletionOption::QmlTypes);
        return reachableSymbols(currentItem, ctx, typeCompletionType);
    }

    const DomItem containingParameter = currentItem.filterUp(
            [](DomType type, const QQmlJS::Dom::DomItem &) {
                return type == DomType::ScriptFormalParameter;
            },
            FilterUpOptions::ReturnInner);
    if (containingParameter) {
        // no autocompletion inside of function parameter definition
        return {};
    }

    const DomItem containingScriptExpression = currentItem.containingScriptExpression();
    if (containingScriptExpression) {
        return scriptIdentifierCompletion(currentItem, ctx);
    }
    const DomItem containingObject = currentItem.qmlObject();
    const DomItem containingFile = currentItem.containingFile();
    if (currentItem.internalKind() == DomType::Binding) {
        QList<CompletionItem> res;
        // do scriptidentifiercompletion after the ':' of a binding

        if (cursorAfterColon(currentItem, ctx)) {
            QList<CompletionItem> res;
            res << scriptIdentifierCompletion(currentItem, ctx);
            if (auto type = resolveExpressionType(currentItem, ResolveOwnerType)) {
                const QStringList names = currentItem.field(Fields::name).toString().split(u'.');
                const QQmlJSScope *current = resolve(type->semanticScope.get(), names);
                // add type names when binding to an object type or a property with var type
                if (!current || current->accessSemantics() == QQmlSA::AccessSemantics::Reference) {
                    res << reachableSymbols(currentItem, ctx, TypeCompletionOption::QmlTypes);
                }
            }
            return res;
        }
        res << bindingsCompletions(containingObject);
        // add Qml Types for default binding
        res += reachableSymbols(containingFile, ctx, TypeCompletionOption::QmlTypes);
        return res;
    }

    if (currentItem.internalKind() == DomType::Import) {
        QList<CompletionItem> res;
        res += importCompletions(containingFile, ctx);

        // when in front of the import statement: propose types for root Qml Object completion
        if (cursorInFrontOfItem(currentItem, ctx))
            res += reachableSymbols(containingFile, ctx, TypeCompletionOption::QmlTypes);

        return res;
    }

    if (!containingObject) {
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
        res += reachableSymbols(containingFile, ctx, TypeCompletionOption::QmlTypes);
        return res;
    }

    if (ctx.atLineStart() && currentItem.internalKind() != DomType::List) {
        // inside some Qml Object
        QList<CompletionItem> res;
        if (ctx.base().isEmpty()) {
            // TODO: complete also the brackets after function?
            for (const QStringView &s : std::array<QStringView, 5>(
                         { u"property", u"readonly", u"default", u"signal", u"function" })) {
                CompletionItem comp;
                comp.label = s.toUtf8();
                comp.kind = int(CompletionItemKind::Keyword);
                res.append(comp);
            }
            // add bindings
            res += bindingsCompletions(containingObject);
            // add Qml Types for default binding
            res += reachableSymbols(containingFile, ctx, TypeCompletionOption::QmlTypes);
        }
        return res;
    }

    // no completion could be found
    qCDebug(QQmlLSUtilsLog) << "No completion was found for current request.";
    return {};
}

QT_END_NAMESPACE
