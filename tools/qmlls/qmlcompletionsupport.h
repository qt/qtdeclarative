// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef QMLCOMPLETIONSUPPORT_H
#define QMLCOMPLETIONSUPPORT_H

#include "qlanguageserver.h"
#include "qqmlcodemodel.h"
#include <QtCore/qmutex.h>
#include <QtCore/qhash.h>

struct CompletionRequest
{
    int minVersion;
    QString code;
    QLspSpecification::CompletionParams completionParams;
    QLspSpecification::LSPPartialResponse<
            std::variant<QList<QLspSpecification::CompletionItem>,
                         QLspSpecification::CompletionList, std::nullptr_t>,
            std::variant<QLspSpecification::CompletionList,
                         QList<QLspSpecification::CompletionItem>>>
            response;
    void sendCompletions(QmlLsp::OpenDocumentSnapshot &);
    QString urlAndPos() const;
    QList<QLspSpecification::CompletionItem> completions(QmlLsp::OpenDocumentSnapshot &doc) const;
};

class QmlCompletionSupport : public QLanguageServerModule
{
    Q_OBJECT
public:
    QmlCompletionSupport(QmlLsp::QQmlCodeModel *codeModel);
    ~QmlCompletionSupport();
    QString name() const override;
    void registerHandlers(QLanguageServer *server, QLanguageServerProtocol *protocol) override;
    void setupCapabilities(const QLspSpecification::InitializeParams &clientInfo,
                           QLspSpecification::InitializeResult &) override;
public Q_SLOTS:
    void updatedSnapshot(const QByteArray &uri);

private:
    QmlLsp::QQmlCodeModel *m_codeModel;
    QMutex m_mutex;
    QMultiHash<QString, CompletionRequest *> m_completions;
};

#endif // QMLCOMPLETIONSUPPORT_H
