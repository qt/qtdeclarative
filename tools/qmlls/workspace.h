// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef WORKSPACE_H
#define WORKSPACE_H

#include "qqmlcodemodel.h"
#include "qlanguageserver.h"

QT_BEGIN_NAMESPACE

class WorkspaceHandlers : public QLanguageServerModule
{
    Q_OBJECT
public:
    enum class Status { NoIndex, Indexing };
    WorkspaceHandlers(QmlLsp::QQmlCodeModel *codeModel) : m_codeModel(codeModel) { }
    QString name() const override;
    void registerHandlers(QLanguageServer *server, QLanguageServerProtocol *protocol) override;
    void setupCapabilities(const QLspSpecification::InitializeParams &clientInfo,
                           QLspSpecification::InitializeResult &) override;
public Q_SLOTS:
    void clientInitialized(QLanguageServer *);

private:
    QmlLsp::QQmlCodeModel *m_codeModel = nullptr;
    Status m_status = Status::NoIndex;
};

QT_END_NAMESPACE

#endif // WORKSPACE_H
