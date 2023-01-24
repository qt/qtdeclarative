// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qqmllsutils_p.h"

#include <QtLanguageServer/private/qlanguageserverspectypes_p.h>
#include <QtCore/qthreadpool.h>
#include <QtCore/private/qduplicatetracker_p.h>
#include <QtCore/QRegularExpression>
#include <QtQmlDom/private/qqmldomexternalitems_p.h>
#include <QtQmlDom/private/qqmldomtop_p.h>

QT_USE_NAMESPACE
using namespace QLspSpecification;
using namespace QQmlJS::Dom;
using namespace Qt::StringLiterals;

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
    If line and character point to whitespace, it might return an inner node of the QmlDom-Tree
    if \c IgnoreWhitespace is set to \c Off. If set to \c onSameLine, it will continue at the
    nearest child that starts or ends on the same line. The distance is measured from the
    child's start-or endcharacter to the character-value passed to this function.
    This function can return internal objects, e.g. when line and character point to whitespace.
 */
QList<QQmlLSUtilsItemLocation>
QQmlLSUtils::itemsFromTextLocation(DomItem file, int line, int character, IgnoreWhitespace option)
{
    // TODO: write tests for this method
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
        loc.depth = 0;
        loc.domItem = file;
        loc.fileLocation = t;
        toDo.append(loc);
    }
    while (!toDo.isEmpty()) {
        QQmlLSUtilsItemLocation iLoc = toDo.last();
        toDo.removeLast();
        if (itemsFound.isEmpty() || itemsFound.constFirst().depth <= iLoc.depth) {
            if (!itemsFound.isEmpty() && itemsFound.constFirst().depth < iLoc.depth)
                itemsFound.clear();
            itemsFound.append(iLoc);
        }

        bool inParentButOutsideChildren = true;
        auto subEls = iLoc.fileLocation->subItems();
        for (auto it = subEls.begin(); it != subEls.end(); ++it) {
            auto subLoc = std::static_pointer_cast<AttachedInfoT<FileLocations>>(it.value());
            Q_ASSERT(subLoc);
            DomItem tmp = iLoc.domItem.path(it.key());
            if (containsTarget(subLoc->info().fullRegion)) {
                QQmlLSUtilsItemLocation subItem;
                subItem.depth = iLoc.depth + 1;
                subItem.domItem = iLoc.domItem.path(it.key());
                subItem.fileLocation = subLoc;
                toDo.append(subItem);
                inParentButOutsideChildren = false;
            }
        }

        if (option == OnSameLine && inParentButOutsideChildren) {
            // try again, as none of the children contained the current position
            QList<QQmlLSUtilsItemLocation> nearestChildren;
            quint32 distanceToNearest = -1;
            for (auto it = subEls.begin(); it != subEls.end(); ++it) {
                auto subLoc = std::static_pointer_cast<AttachedInfoT<FileLocations>>(it.value());
                DomItem tmp = iLoc.domItem.path(it.key());
                Q_ASSERT(subLoc);
                if (subLoc->info().fullRegion.startLine != quint32(line + 1)
                    || quint32(targetPos) > subLoc->info().fullRegion.begin())
                    continue;

                const quint32 distance = subLoc->info().fullRegion.begin() - targetPos;
                if (distance < distanceToNearest) {
                    // remove the others
                    nearestChildren.clear();
                    distanceToNearest = distance;
                }
                if (distance <= distanceToNearest) {
                    // add to nearest
                    QQmlLSUtilsItemLocation subItem;
                    subItem.depth = iLoc.depth + 1;
                    subItem.domItem = iLoc.domItem.path(it.key());
                    subItem.fileLocation = subLoc;
                    nearestChildren.append(subItem);
                }
            }
            toDo.append(nearestChildren);
        }
    }
    return itemsFound;
}

QQmlJS::Dom::FileLocations::Tree QQmlLSUtils::textLocationFromItem(QQmlJS::Dom::DomItem qmlObject)
{
    Q_UNUSED(qmlObject);
    // TODO
    // TODO: write tests for this method
    DomItem file = qmlObject.containingFile();
    std::shared_ptr<QmlFile> filePtr = file.ownerAs<QmlFile>();
    if (!filePtr)
        return {};
    FileLocations::Tree t = filePtr->fileLocationsTree();

    Path fromFileToObject = qmlObject.canonicalPath();
    t = t->find(t, fromFileToObject, QQmlJS::Dom::AttachedInfo::PathType::Canonical);
    return t;
}
