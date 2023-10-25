// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qqmllsutils_p.h"
#include "qqmlrenamesymbolsupport_p.h"
#include <utility>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;
QQmlRenameSymbolSupport::QQmlRenameSymbolSupport(QmlLsp::QQmlCodeModel *model) : BaseT(model) { }

QString QQmlRenameSymbolSupport::name() const
{
    return u"QmlRenameSymbolSupport"_s;
}

void QQmlRenameSymbolSupport::setupCapabilities(
        const QLspSpecification::InitializeParams &,
        QLspSpecification::InitializeResult &serverCapabilities)
{
    // use a bool for now. Alternatively, if the client supports "prepareSupport", one could
    // use a RenameOptions here. See following page for more information about prepareSupport:
    // https://microsoft.github.io/language-server-protocol/specifications/lsp/3.17/specification/#textDocument_prepareRename
    serverCapabilities.capabilities.renameProvider = true;
}

void QQmlRenameSymbolSupport::registerHandlers(QLanguageServer *, QLanguageServerProtocol *protocol)
{
    protocol->registerRenameRequestHandler(getRequestHandler());
}

void QQmlRenameSymbolSupport::process(QQmlRenameSymbolSupport::RequestPointerArgument request)
{
    QLspSpecification::WorkspaceEdit result;
    ResponseScopeGuard guard(result, request->m_response);

    auto itemsFound = itemsForRequest(request);
    if (guard.setErrorFrom(itemsFound))
        return;

    QQmlLSUtilsItemLocation &front = std::get<QList<QQmlLSUtilsItemLocation>>(itemsFound).front();

    const QString newName = QString::fromUtf8(request->m_parameters.newName);
    auto expressionType = QQmlLSUtils::resolveExpressionType(front.domItem, ResolveOwnerType);

    if (!expressionType) {
        guard.setError(QQmlLSUtilsErrorMessage{ 0, u"Cannot rename the requested object"_s });
        return;
    }

    if (guard.setErrorFrom(QQmlLSUtils::checkNameForRename(front.domItem, newName, expressionType)))
        return;

    QList<QLspSpecification::TextDocumentEdit> editsByFileForResult;
    // The QLspSpecification::WorkspaceEdit requires the changes to be grouped by files, so
    // collect them into editsByFileUris.
    QMap<QUrl, QList<QLspSpecification::TextEdit>> editsByFileUris;

    auto renames = QQmlLSUtils::renameUsagesOf(front.domItem, newName, expressionType);

    QQmlJS::Dom::DomItem files = front.domItem.top().field(QQmlJS::Dom::Fields::qmlFileWithPath);

    QHash<QString, QString> codeCache;

    for (const auto &rename : renames) {
        QLspSpecification::TextEdit edit;

        const QUrl uri = QUrl::fromLocalFile(rename.location.filename);

        auto cacheEntry = codeCache.find(rename.location.filename);
        if (cacheEntry == codeCache.end()) {
            auto file = files.key(rename.location.filename)
                                .field(QQmlJS::Dom::Fields::currentItem)
                                .ownerAs<QQmlJS::Dom::QmlFile>();
            if (!file) {
                qDebug() << "File" << rename.location.filename
                         << "not found in DOM! Available files are" << files.keys();
                continue;
            }
            cacheEntry = codeCache.insert(rename.location.filename, file->code());
        }

        edit.range = QQmlLSUtils::qmlLocationToLspLocation(cacheEntry.value(),
                                                           rename.location.sourceLocation);
        edit.newText = rename.replacement.toUtf8();

        editsByFileUris[uri].append(edit);
    }

    for (auto it = editsByFileUris.keyValueBegin(); it != editsByFileUris.keyValueEnd(); ++it) {
        QLspSpecification::TextDocumentEdit editsForCurrentFile;
        editsForCurrentFile.textDocument.uri = it->first.toEncoded();

        // TODO: do we need to take care of the optional versioning in
        // editsForCurrentFile.textDocument.version? see
        // https://microsoft.github.io/language-server-protocol/specifications/lsp/3.17/specification/#optionalVersionedTextDocumentIdentifier
        // for more details

        for (const auto &x : std::as_const(it->second)) {
            editsForCurrentFile.edits.append(x);
        }
        editsByFileForResult.append(editsForCurrentFile);
    }

    result.documentChanges = editsByFileForResult;
}

QT_END_NAMESPACE
