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
#include <optional>
#include <tuple>
#include <variant>

QT_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(QQmlLSUtilsLog);

namespace QQmlLSUtils {

struct ItemLocation
{
    QQmlJS::Dom::DomItem domItem;
    QQmlJS::Dom::FileLocations::Tree fileLocation;
};

struct TextPosition
{
    int line;
    int character;
};

enum IdentifierType : char {
    JavaScriptIdentifier,
    PropertyIdentifier,
    PropertyChangedSignalIdentifier,
    PropertyChangedHandlerIdentifier,
    SignalIdentifier,
    SignalHandlerIdentifier,
    MethodIdentifier,
    QmlObjectIdIdentifier,
    SingletonIdentifier,
    EnumeratorIdentifier,
    EnumeratorValueIdentifier,
    AttachedTypeIdentifier,
    GroupedPropertyIdentifier,
    QmlComponentIdentifier,
};

struct ErrorMessage
{
    int code;
    QString message;
};

struct ExpressionType
{
    std::optional<QString> name;
    QQmlJSScope::ConstPtr semanticScope;
    IdentifierType type;
};

struct Location
{
    QString filename;
    QQmlJS::SourceLocation sourceLocation;

    static Location from(const QString &fileName, const QString &code, quint32 startLine,
                         quint32 startCharacter, quint32 length);

    friend bool operator<(const Location &a, const Location &b)
    {
        return std::make_tuple(a.filename, a.sourceLocation.begin(), a.sourceLocation.end())
                < std::make_tuple(b.filename, b.sourceLocation.begin(), b.sourceLocation.end());
    }
    friend bool operator==(const Location &a, const Location &b)
    {
        return std::make_tuple(a.filename, a.sourceLocation.begin(), a.sourceLocation.end())
                == std::make_tuple(b.filename, b.sourceLocation.begin(), b.sourceLocation.end());
    }
};

struct Edit
{
    Location location;
    QString replacement;

    static Edit from(const QString &fileName, const QString &code, quint32 startLine,
                     quint32 startCharacter, quint32 length, const QString &newName);

    friend bool operator<(const Edit &a, const Edit &b)
    {
        return std::make_tuple(a.location, a.replacement)
                < std::make_tuple(b.location, b.replacement);
    }
    friend bool operator==(const Edit &a, const Edit &b)
    {
        return std::make_tuple(a.location, a.replacement)
                == std::make_tuple(b.location, b.replacement);
    }
};

/*!
   \internal
    Choose whether to resolve the owner type or the entire type (the latter is only required to
    resolve the types of qualified names and property accesses).

    For properties, methods, enums and co:
    * ResolveOwnerType returns the base type of the owner that owns the property, method, enum
      and co. For example, resolving "x" in "myRectangle.x" will return the Item as the owner, as
      Item is the base type of Rectangle that defines the "x" property.
    * ResolveActualTypeForFieldMemberExpression is used to resolve field member expressions, and
      might lose some information about the owner. For example, resolving "x" in "myRectangle.x"
      will return the JS type for float that was used to define the "x" property.
 */
enum ResolveOptions {
    ResolveOwnerType,
    ResolveActualTypeForFieldMemberExpression,
};

using DomItem = QQmlJS::Dom::DomItem;

qsizetype textOffsetFrom(const QString &code, int row, int character);
TextPosition textRowAndColumnFrom(const QString &code, qsizetype offset);
QList<ItemLocation> itemsFromTextLocation(const DomItem &file, int line, int character);
DomItem sourceLocationToDomItem(const DomItem &file, const QQmlJS::SourceLocation &location);
QByteArray lspUriToQmlUrl(const QByteArray &uri);
QByteArray qmlUrlToLspUri(const QByteArray &url);
QLspSpecification::Range qmlLocationToLspLocation(const QString &code,
                                                  QQmlJS::SourceLocation qmlLocation);
DomItem baseObject(const DomItem &qmlObject);
std::optional<Location> findTypeDefinitionOf(const DomItem &item);
std::optional<Location> findDefinitionOf(const DomItem &item);
QList<Location> findUsagesOf(const DomItem &item);

std::optional<ErrorMessage>
checkNameForRename(const DomItem &item, const QString &newName,
                   const std::optional<ExpressionType> &targetType = std::nullopt);
QList<Edit> renameUsagesOf(const DomItem &item, const QString &newName,
                           const std::optional<ExpressionType> &targetType = std::nullopt);

std::optional<ExpressionType> resolveExpressionType(const DomItem &item, ResolveOptions);
bool isValidEcmaScriptIdentifier(QStringView view);

QPair<QString, QStringList> cmakeBuildCommand(const QString &path);

// Documentation Hints
QByteArray getDocumentationFromLocation(const DomItem &file, const TextPosition &position);

bool isFieldMemberExpression(const DomItem &item);
bool isFieldMemberAccess(const DomItem &item);
QStringList fieldMemberExpressionBits(const DomItem &item, const DomItem &stopAtChild = {});

QString qualifiersFrom(const DomItem &el);
} // namespace QQmlLSUtils
QT_END_NAMESPACE

#endif // QLANGUAGESERVERUTILS_P_H
