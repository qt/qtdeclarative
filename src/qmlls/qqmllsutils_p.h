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
    SingletonIdentifier,
    EnumeratorIdentifier,
    EnumeratorValueIdentifier,
    AttachedTypeIdentifier,
    GroupedPropertyIdentifier,
    QmlComponentIdentifier,
};

struct QQmlLSUtilsErrorMessage
{
    int code;
    QString message;
};

struct QQmlLSUtilsExpressionType
{
    std::optional<QString> name;
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

    For properties, methods, enums and co:
    * ResolveOwnerType returns the base type of the owner that owns the property, method, enum
      and co. For example, resolving "x" in "myRectangle.x" will return the Item as the owner, as
      Item is the base type of Rectangle that defines the "x" property.
    * ResolveActualTypeForFieldMemberExpression is used to resolve field member expressions, and
      might lose some information about the owner. For example, resolving "x" in "myRectangle.x"
      will return the JS type for float that was used to define the "x" property.
 */
enum QQmlLSUtilsResolveOptions {
    ResolveOwnerType,
    ResolveActualTypeForFieldMemberExpression,
};

enum class ImportCompletionType { None, Module, Version };

using DomItem = QQmlJS::Dom::DomItem;

class QQmlLSUtils
{
public:
    static qsizetype textOffsetFrom(const QString &code, int row, int character);
    static QQmlLSUtilsTextPosition textRowAndColumnFrom(const QString &code, qsizetype offset);
    static QList<QQmlLSUtilsItemLocation> itemsFromTextLocation(const DomItem &file,
                                                                int line, int character);
    static DomItem sourceLocationToDomItem(const DomItem &file,
                                                        const QQmlJS::SourceLocation &location);
    static QByteArray lspUriToQmlUrl(const QByteArray &uri);
    static QByteArray qmlUrlToLspUri(const QByteArray &url);
    static QLspSpecification::Range qmlLocationToLspLocation(const QString &code,
                                                             QQmlJS::SourceLocation qmlLocation);
    static DomItem baseObject(const DomItem &qmlObject);
    static std::optional<QQmlLSUtilsLocation>
    findTypeDefinitionOf(const DomItem &item);
    static std::optional<QQmlLSUtilsLocation> findDefinitionOf(const DomItem &item);
    static QList<QQmlLSUtilsLocation> findUsagesOf(const DomItem &item);

    static std::optional<QQmlLSUtilsErrorMessage> checkNameForRename(
            const DomItem &item, const QString &newName,
            const std::optional<QQmlLSUtilsExpressionType> &targetType = std::nullopt);
    static QList<QQmlLSUtilsEdit> renameUsagesOf(
            const DomItem &item, const QString &newName,
            const std::optional<QQmlLSUtilsExpressionType> &targetType = std::nullopt);

    static std::optional<QQmlLSUtilsExpressionType> resolveExpressionType(
            const DomItem &item, QQmlLSUtilsResolveOptions);
    static bool isValidEcmaScriptIdentifier(QStringView view);

    static QPair<QString, QStringList> cmakeBuildCommand(const QString &path);

    // Documentation Hints
    static QByteArray getDocumentationFromLocation(const DomItem &file, const QQmlLSUtilsTextPosition &position);

    static bool isFieldMemberExpression(const DomItem &item);
    static bool isFieldMemberAccess(const DomItem &item);
    static QStringList fieldMemberExpressionBits(const DomItem &item,
                                                 const DomItem &stopAtChild = {});

    static QString qualifiersFrom(const DomItem &el);
};
QT_END_NAMESPACE

#endif // QLANGUAGESERVERUTILS_P_H
