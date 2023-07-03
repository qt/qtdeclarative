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
        auto scope = QQmlLSUtils::resolveExpressionType(
                object, QQmlLSUtilsResolveOptions::ResolveActualTypeForFieldMemberExpression);
        if (!scope)
            return {};

        return QQmlLSUtils::sourceLocationToDomItem(object.containingFile(),
                                                    scope->semanticScope->sourceLocation());
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

static void findUsagesOfNonJSIdentifiers(DomItem item, const QString &name,
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

    auto findUsages = [&targetType, &result, &name, &checkName](Path, DomItem &current,
                                                                bool) -> bool {
        bool resolveType = false;
        bool continueForChildren = true;
        DomItem toBeResolved = current;
        QString subRegion;

        if (auto scope = current.semanticScope()) {
            // is the current property shadowed by some JS identifier? ignore current + its children
            if (scope.value()->JSIdentifier(name)) {
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

static void findUsagesHelper(DomItem item, const QString &name, QList<QQmlLSUtilsLocation> &result)
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
            return QQmlLSUtilsExpressionType{ current, *methodType };

        if (const auto signalName = QQmlSignalNames::handlerNameToSignalName(name)) {
            if (auto methodType = hasMethodOrSignal(current, *signalName)) {
                return QQmlLSUtilsExpressionType{ current, *methodType };
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
                return QQmlLSUtilsExpressionType{ current,
                                                  QQmlLSUtilsIdentifierType::PropertyIdentifier };
            case ResolveActualTypeForFieldMemberExpression:
                return QQmlLSUtilsExpressionType{ property.type(),
                                                  QQmlLSUtilsIdentifierType::PropertyIdentifier };
            }
        }
    }
    return {};
}

static std::optional<QQmlLSUtilsExpressionType>
resolveIdentifierExpressionType(DomItem item, QQmlLSUtilsResolveOptions options)
{
    auto referrerScope = item.nearestSemanticScope();
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
                return QQmlLSUtilsExpressionType{ owner->semanticScope, type };
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
                return QQmlLSUtilsExpressionType{ owner->semanticScope,
                                                  QQmlLSUtilsIdentifierType::PropertyIdentifier };
            case ResolveActualTypeForFieldMemberExpression:
                return QQmlLSUtilsExpressionType{ property.type(),
                                                  QQmlLSUtilsIdentifierType::PropertyIdentifier };
            }
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
            auto scope = definitionOfItem.semanticScope()
                                 .value()
                                 ->JSIdentifier(name)
                                 ->scope.toStrongRef();
            return QQmlLSUtilsExpressionType{ scope,
                                              QQmlLSUtilsIdentifierType::JavaScriptIdentifier };
        }

        // check if its a method
        if (auto scope = methodFromReferrerScope(*referrerScope, name, options)) {
            return *scope;
        }

        // check if its an (unqualified) property
        if (auto scope = propertyFromReferrerScope(*referrerScope, name, options)) {
            return *scope;
        }
    }

    // check if its an id
    auto resolver = item.containingFile().ownerAs<QmlFile>()->typeResolver();
    if (!resolver)
        return {};
    QQmlJSScope::ConstPtr fromId = resolver.value()->scopeForId(name, referrerScope.value());
    if (fromId)
        return QQmlLSUtilsExpressionType{ fromId, QmlObjectIdIdentifier };

    return {};
}
/*!
   \internal
    Resolves the type of the given DomItem, when possible (e.g., when there are enough type
    annotations).
*/
std::optional<QQmlLSUtilsExpressionType>
QQmlLSUtils::resolveExpressionType(QQmlJS::Dom::DomItem item, QQmlLSUtilsResolveOptions options)
{
    switch (item.internalKind()) {
    case DomType::ScriptIdentifierExpression: {
        return resolveIdentifierExpressionType(item, options);
    }
    case DomType::PropertyDefinition: {
        auto propertyDefinition = item.as<PropertyDefinition>();
        if (propertyDefinition && propertyDefinition->scope) {
            auto &scope = propertyDefinition->scope.value();
            return QQmlLSUtilsExpressionType{ scope, PropertyIdentifier };
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
                return QQmlLSUtilsExpressionType{ owner.value(), QmlObjectIdIdentifier };

            auto signalOrProperty = resolveNameInQmlScope(name, owner.value());
            if (signalOrProperty)
                return QQmlLSUtilsExpressionType{ owner.value(), signalOrProperty->type };

            qDebug(QQmlLSUtilsLog) << "QQmlLSUtils::resolveExpressionType() could not resolve the"
                                      "type of a Binding.";
        }

        return {};
    }
    case DomType::QmlObject: {
        auto object = item.as<QmlObject>();
        if (object && object->semanticScope())
            return QQmlLSUtilsExpressionType{ object->semanticScope().value(),
                                              QmlObjectIdentifier };

        return {};
    }
    case DomType::MethodInfo: {
        auto object = item.as<MethodInfo>();
        if (object && object->semanticScope()) {
            auto scope = object->semanticScope();
            if (!scope)
                return {};

            // in case scope is the semantic scope for the function bodies: grab the owner's scope
            // this happens for all methods but not for signals (they do not have a body)
            if (scope.value()->scopeType() == QQmlJSScope::ScopeType::JSFunctionScope)
                scope = scope.value()->parentScope();

            if (auto type = hasMethodOrSignal(scope.value(), object->name))
                return QQmlLSUtilsExpressionType{ scope.value(), type.value() };

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

static std::optional<QQmlLSUtilsLocation>
findMethodDefinitionOf(DomItem file, QQmlJS::SourceLocation location, const QString &name)
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
findPropertyDefinitionOf(DomItem file, QQmlJS::SourceLocation propertyDefinitionLocation,
                         const QString &name)
{
    DomItem propertyOwner = QQmlLSUtils::sourceLocationToDomItem(file, propertyDefinitionLocation);
    DomItem propertyDefinition = propertyOwner.field(Fields::propertyDefs).key(name).index(0);
    if (!propertyDefinition)
        return {};

    QQmlLSUtilsLocation result;
    result.sourceLocation = FileLocations::treeOf(propertyDefinition)->info().fullRegion;
    result.filename = propertyDefinition.canonicalFilePath();
    return result;
}

std::optional<QQmlLSUtilsLocation> QQmlLSUtils::findDefinitionOf(DomItem item)
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

        // not a JS identifier, check for ids and properties and methods
        auto referrerScope = item.nearestSemanticScope();
        if (!referrerScope)
            return {};

        // check: is it a method name?
        if (auto scope = methodFromReferrerScope(*referrerScope, name)) {
            const QString canonicalPath = scope->semanticScope->filePath();
            DomItem file = item.goToFile(canonicalPath);
            return findMethodDefinitionOf(file, scope->semanticScope->sourceLocation(), name);
        }

        if (auto scope = propertyFromReferrerScope(*referrerScope, name,
                                                   QQmlLSUtilsResolveOptions::ResolveOwnerType)) {
            const QString canonicalPath = scope->semanticScope->filePath();
            DomItem file = item.goToFile(canonicalPath);
            return findPropertyDefinitionOf(file, scope->semanticScope->sourceLocation(), name);
        }

        // check if its an id
        auto resolver = item.containingFile().ownerAs<QmlFile>()->typeResolver();
        if (!resolver)
            return {};
        QQmlJSScope::ConstPtr fromId = resolver.value()->scopeForId(name, referrerScope.value());
        if (fromId) {
            QQmlLSUtilsLocation result;
            result.sourceLocation = fromId->sourceLocation();
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

std::optional<QQmlLSUtilsErrorMessage> QQmlLSUtils::checkNameForRename(DomItem item,
                                                                       const QString &newName)
{
    // TODO
    Q_UNUSED(item);
    Q_UNUSED(newName);
    return {};
}

static std::optional<QString> oldNameFrom(DomItem item)
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
QList<QQmlLSUtilsEdit> QQmlLSUtils::renameUsagesOf(DomItem item, const QString &dirtyNewName)
{
    QList<QQmlLSUtilsEdit> results;
    const QList<QQmlLSUtilsLocation> locations = findUsagesOf(item);
    if (locations.isEmpty())
        return results;

    auto oldName = oldNameFrom(item);
    if (!oldName)
        return results;

    auto targetType =
            QQmlLSUtils::resolveExpressionType(item, QQmlLSUtilsResolveOptions::ResolveOwnerType);

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

QT_END_NAMESPACE
