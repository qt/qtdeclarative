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
#include <type_traits>
#include <variant>

QT_USE_NAMESPACE
using namespace QLspSpecification;
using namespace QQmlJS::Dom;
using namespace Qt::StringLiterals;

Q_LOGGING_CATEGORY(QQmlLSUtilsLog, "qt.languageserver.utils")

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
                Q_ASSERT_X(subItem.domItem, "QQmlLSUtils::itemsFromTextLocation",
                           "A DomItem child is missing or the FileLocationsTree structure does not "
                           "follow the DomItem Structure.");
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
    case QQmlJS::Dom::DomType::Empty:
        break;
    default:
        qDebug() << "Found unimplemented Type" << object.internalKindStr() << "in"
                 << object.toString();
        result = {};
    }

    return result;
}

QList<QQmlLSUtilsLocation> QQmlLSUtils::findUsagesOf(DomItem item)
{
    QList<QQmlLSUtilsLocation> result;

    QString name;

    switch (item.internalKind()) {
    case DomType::ScriptIdentifierExpression:
    case DomType::ScriptVariableDeclarationEntry:
        name = item.field(Fields::identifier).value().toString();
        break;
    default:
        qDebug() << item.internalKindStr() << "was not implemented for QQmlLSUtils::findUsagesOf";
        return result;
    }

    qCDebug(QQmlLSUtilsLog) << "Looking for JS identifier with name" << name;

    DomItem definitionOfItem;

    item.visitUp([&name, &definitionOfItem](DomItem &i) {
        if (std::optional<QQmlJSScope::Ptr> scope = i.semanticScope(); scope) {
            qCDebug(QQmlLSUtilsLog) << "Searching for definition in" << i.internalKindStr();
            if (auto jsIdentifier = scope.value()->JSIdentifier(name)) {
                qCDebug(QQmlLSUtilsLog) << "Found scope" << scope.value()->baseTypeName();
                definitionOfItem = i;
                return false;
            }
        } else {
            qCDebug(QQmlLSUtilsLog)
                    << "Searching for definition in" << i.internalKindStr() << "that has no scope";
        }
        return true;
    });

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

    std::sort(result.begin(), result.end());

    if (QQmlLSUtilsLog().isDebugEnabled()) {
        qCDebug(QQmlLSUtilsLog) << "Found following usages:";
        for (auto r : result) {
            qCDebug(QQmlLSUtilsLog)
                    << r.filename << " @ " << r.location.startLine << ":" << r.location.startColumn;
        }
    }

    return result;
}
