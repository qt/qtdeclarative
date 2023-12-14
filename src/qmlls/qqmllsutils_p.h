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

#include "qqmlcompletioncontextstrings_p.h"

#include <QtLanguageServer/private/qlanguageserverspectypes_p.h>
#include <QtQmlDom/private/qqmldomexternalitems_p.h>
#include <QtQmlDom/private/qqmldomtop_p.h>
#include <algorithm>
#include <optional>
#include <tuple>
#include <variant>

QT_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(QQmlLSCompletionLog);

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
    QmlObjectIdentifier,
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
 */
enum QQmlLSUtilsResolveOptions {
    ResolveOwnerType,
    ResolveActualTypeForFieldMemberExpression,
};

enum class ImportCompletionType { None, Module, Version };

using DomItem = QQmlJS::Dom::DomItem;

enum QQmlLSUtilsAppendOption { AppendSemicolon, AppendNothing };

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

    static std::optional<QQmlLSUtilsErrorMessage>
    checkNameForRename(const DomItem &item, const QString &newName,
                       std::optional<QQmlLSUtilsExpressionType> targetType = std::nullopt);
    static QList<QQmlLSUtilsEdit>
    renameUsagesOf(const DomItem &item, const QString &newName,
                   std::optional<QQmlLSUtilsExpressionType> targetType = std::nullopt);

    static std::optional<QQmlLSUtilsExpressionType>
    resolveExpressionType(const DomItem &item, QQmlLSUtilsResolveOptions);
    static bool isValidEcmaScriptIdentifier(QStringView view);

    // completion stuff
    using CompletionItem = QLspSpecification::CompletionItem;
    static QList<CompletionItem> idsCompletions(const DomItem& component);

    static QList<CompletionItem> reachableTypes(const DomItem &context,
                                                QQmlJS::Dom::LocalSymbolsTypes typeCompletionType,
                                                QLspSpecification::CompletionItemKind kind);

    static QList<CompletionItem> suggestJSExpressionCompletion(const DomItem &context);
    static QList<CompletionItem> completions(const DomItem& currentItem,
                                             const CompletionContextStrings &ctx);


    // JS statement completion
    static QList<CompletionItem> suggestJSStatementCompletion(const DomItem &currentItem);
    static QList<CompletionItem>
    suggestCaseAndDefaultStatementCompletion();
    static QList<CompletionItem>
    suggestVariableDeclarationStatementCompletion(QQmlLSUtilsAppendOption option = AppendSemicolon);
    static QPair<QString, QStringList> cmakeBuildCommand(const QString &path);
};
QT_END_NAMESPACE

#endif // QLANGUAGESERVERUTILS_P_H
