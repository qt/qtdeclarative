// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "qqmllscodeaction_p.h"

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;
using namespace QLspSpecification;

static QList<std::variant<Command, CodeAction>> provideCodeActions(const CodeActionParams &params)
{
    QList<std::variant<Command, CodeAction>> responseData;

    for (const Diagnostic &diagnostic : params.context.diagnostics) {
        if (!diagnostic.data.has_value())
            continue;

        const auto &data = diagnostic.data.value();

        int version = data[u"version"].toInt();
        QJsonArray suggestions = data[u"suggestions"].toArray();

        QList<WorkspaceEdit::DocumentChange> edits;
        QString message;
        for (const QJsonValue &suggestion : std::as_const(suggestions)) {
            message += suggestion[u"message"_s].toString() + u'\n';
            const auto &documentEdits = suggestion[u"documentEdits"_s].toArray();
            TextDocumentEdit textDocEdit;
            for (const auto &documentEdit : documentEdits) {
                TextEdit textEdit;
                textEdit.range = {
                    Position{
                            static_cast<unsigned>(documentEdit[u"lspBeginLine"].toDouble()),
                            static_cast<unsigned>(documentEdit[u"lspBeginCharacter"].toDouble()) },
                    Position{ static_cast<unsigned>(documentEdit[u"lspEndLine"].toDouble()),
                              static_cast<unsigned>(documentEdit[u"lspEndCharacter"].toDouble()) }
                };

                textEdit.newText = documentEdit[u"replacement"_s].toString().toUtf8();
                QString filename = documentEdit[u"filename"_s].toString();
                textDocEdit.textDocument = { { filename.toUtf8() }, version };
                textDocEdit.edits.append(textEdit);
            }
            edits.append(textDocEdit);
        }
        message.chop(1);
        WorkspaceEdit edit;
        edit.documentChanges = edits;

        CodeAction action;
        // VS Code and QtC ignore everything that is not a 'quickfix'.
        action.kind = CodeActionKind::QuickFix;
        action.edit = edit;
        action.title = message.toUtf8();

        responseData.append(action);
    }
    return responseData;
}

QQmlCodeActionSupport::QQmlCodeActionSupport(QmlLsp::QQmlCodeModelManager *model) : BaseT(model) { }

void QQmlCodeActionSupport::setupCapabilities(QLspSpecification::ServerCapabilities &caps)
{
    caps.codeActionProvider = true;
}

void QQmlCodeActionSupport::registerHandlers(QLanguageServer *, QLanguageServerProtocol *protocol)
{
    protocol->registerCodeActionRequestHandler(getRequestHandler());
}

void QQmlCodeActionSupport::process(QQmlCodeActionSupport::RequestPointerArgument request)
{
    QList<std::variant<Command, CodeAction>> results = provideCodeActions(request->m_parameters);
    request->m_response.sendResponse(results);
}

QT_END_NAMESPACE
