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

    QQmlLSUtils::ItemLocation &front =
            std::get<QList<QQmlLSUtils::ItemLocation>>(itemsFound).front();

    const QString newName = QString::fromUtf8(request->m_parameters.newName);
    auto expressionType =
            QQmlLSUtils::resolveExpressionType(front.domItem, QQmlLSUtils::ResolveOwnerType);

    if (!expressionType) {
        guard.setError(QQmlLSUtils::ErrorMessage{ 0, u"Cannot rename the requested object"_s });
        return;
    }

    if (guard.setErrorFrom(QQmlLSUtils::checkNameForRename(front.domItem, newName, expressionType)))
        return;

    auto &editsByFileForResult = result.documentChanges.emplace();

    // The QLspSpecification::WorkspaceEdit requires the changes to be grouped by files, so
    // collect them into editsByFileUris.
    QMap<QUrl, QList<QLspSpecification::TextEdit>> editsByFileUris;

    const auto renames = QQmlLSUtils::renameUsagesOf(front.domItem, newName, expressionType);
    for (const auto &rename : renames.renameInFile()) {
        QLspSpecification::TextEdit edit;

        const QUrl uri = QUrl::fromLocalFile(rename.location.filename());
        edit.range = QQmlLSUtils::qmlLocationToLspLocation(rename.location);
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

    // if files need to be renamed, then do it after the text edits
    for (const auto &rename : renames.renameInFilename()) {
        QLspSpecification::RenameFile currentRenameFile;
        currentRenameFile.kind = "rename";
        currentRenameFile.oldUri = QUrl::fromLocalFile(rename.oldFilename).toEncoded();
        currentRenameFile.newUri = QUrl::fromLocalFile(rename.newFilename).toEncoded();
        editsByFileForResult.append(currentRenameFile);
    }
}

QT_END_NAMESPACE
