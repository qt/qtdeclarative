// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QLANGUAGESERVERUTILS_P_H
#define QLANGUAGESERVERUTILS_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtLanguageServer/private/qlanguageserverspectypes_p.h>
#include <QtQmlDom/private/qqmldomexternalitems_p.h>
#include <QtQmlDom/private/qqmldomtop_p.h>
#include <algorithm>
#include <tuple>
#include <variant>

QT_BEGIN_NAMESPACE

struct QQmlLSUtilsItemLocation
{
    QQmlJS::Dom::DomItem domItem;
    QQmlJS::Dom::FileLocations::Tree fileLocation;
};

struct QQmlLSUtilsTextPosition
{
    int line;
    int character;
};

enum QQmlLSUtilsIdentifierType : char {
    JavaScriptIdentifier,
    PropertyIdentifier,
    PropertyChangedSignalIdentifier,
    PropertyChangedHandlerIdentifier,
    SignalIdentifier,
    SignalHandlerIdentifier,
    MethodIdentifier,
    QmlObjectIdIdentifier,
    QmlObjectIdentifier
};

struct QQmlLSUtilsErrorMessage
{
    int code;
    QString message;
};

struct QQmlLSUtilsExpressionType
{
    QQmlJSScope::ConstPtr semanticScope;
    QQmlLSUtilsIdentifierType type;
};

struct QQmlLSUtilsLocation
{
    QString filename;
    QQmlJS::SourceLocation sourceLocation;

    static QQmlLSUtilsLocation from(const QString &fileName, const QString &code, quint32 startLine,
                                    quint32 startCharacter, quint32 length);

    friend bool operator<(const QQmlLSUtilsLocation &a, const QQmlLSUtilsLocation &b)
    {
        return std::make_tuple(a.filename, a.sourceLocation.begin(), a.sourceLocation.end())
                < std::make_tuple(b.filename, b.sourceLocation.begin(), b.sourceLocation.end());
    }
    friend bool operator==(const QQmlLSUtilsLocation &a, const QQmlLSUtilsLocation &b)
    {
        return std::make_tuple(a.filename, a.sourceLocation.begin(), a.sourceLocation.end())
                == std::make_tuple(b.filename, b.sourceLocation.begin(), b.sourceLocation.end());
    }
};

struct QQmlLSUtilsEdit
{
    QQmlLSUtilsLocation location;
    QString replacement;

    static QQmlLSUtilsEdit from(const QString &fileName, const QString &code, quint32 startLine,
                                quint32 startCharacter, quint32 length, const QString &newName);

    friend bool operator<(const QQmlLSUtilsEdit &a, const QQmlLSUtilsEdit &b)
    {
        return std::make_tuple(a.location, a.replacement)
                < std::make_tuple(b.location, b.replacement);
    }
    friend bool operator==(const QQmlLSUtilsEdit &a, const QQmlLSUtilsEdit &b)
    {
        return std::make_tuple(a.location, a.replacement)
                == std::make_tuple(b.location, b.replacement);
    }
};

/*!
   \internal
    Choose whether to resolve the owner type or the entire type (the latter is only required to
    resolve the types of qualified names and property accesses).
 */
enum QQmlLSUtilsResolveOptions {
    ResolveOwnerType,
    ResolveActualTypeForFieldMemberExpression,
};

class QQmlLSUtils
{
public:
    static qsizetype textOffsetFrom(const QString &code, int row, int character);
    static QQmlLSUtilsTextPosition textRowAndColumnFrom(const QString &code, qsizetype offset);
    static QList<QQmlLSUtilsItemLocation> itemsFromTextLocation(QQmlJS::Dom::DomItem file, int line,
                                                                int character);
    static QQmlJS::Dom::DomItem sourceLocationToDomItem(QQmlJS::Dom::DomItem file,
                                                        const QQmlJS::SourceLocation &location);
    static QByteArray lspUriToQmlUrl(const QByteArray &uri);
    static QByteArray qmlUrlToLspUri(const QByteArray &url);
    static QLspSpecification::Range qmlLocationToLspLocation(const QString &code,
                                                             QQmlJS::SourceLocation qmlLocation);
    static QQmlJS::Dom::DomItem baseObject(QQmlJS::Dom::DomItem qmlObject);
    static QQmlJS::Dom::DomItem findTypeDefinitionOf(QQmlJS::Dom::DomItem item);
    static std::optional<QQmlLSUtilsLocation> findDefinitionOf(QQmlJS::Dom::DomItem item);
    static QList<QQmlLSUtilsLocation> findUsagesOf(QQmlJS::Dom::DomItem item);

    static std::optional<QQmlLSUtilsErrorMessage> checkNameForRename(QQmlJS::Dom::DomItem item,
                                                                     const QString &newName);
    static QList<QQmlLSUtilsEdit> renameUsagesOf(QQmlJS::Dom::DomItem item, const QString &newName);

    static std::optional<QQmlLSUtilsExpressionType>
    resolveExpressionType(QQmlJS::Dom::DomItem item, QQmlLSUtilsResolveOptions);
};
QT_END_NAMESPACE

#endif // QLANGUAGESERVERUTILS_P_H
