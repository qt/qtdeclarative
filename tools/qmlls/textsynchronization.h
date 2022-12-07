// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#ifndef TEXTSYNCH_H
#define TEXTSYNCH_H

#include "qqmlcodemodel.h"

#include "qlanguageserver_p.h"

QT_BEGIN_NAMESPACE

class TextSynchronization : public QLanguageServerModule
{
    Q_OBJECT
public:
    TextSynchronization(QmlLsp::QQmlCodeModel *codeModel, QObject *parent = nullptr);
    QString name() const override;
    void registerHandlers(QLanguageServer *server, QLanguageServerProtocol *protocol) override;
    void setupCapabilities(const QLspSpecification::InitializeParams &clientInfo,
                           QLspSpecification::InitializeResult &) override;

public Q_SLOTS:
    void didOpenTextDocument(const QLspSpecification::DidOpenTextDocumentParams &params);
    void didDidChangeTextDocument(const QLspSpecification::DidChangeTextDocumentParams &params);
    void didCloseTextDocument(const QLspSpecification::DidCloseTextDocumentParams &params);

private:
    QmlLsp::QQmlCodeModel *m_codeModel;
};

QT_END_NAMESPACE
#endif // TEXTSYNCH_H
