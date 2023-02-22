// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QMLGOTOTYPEDEFINITIONSUPPORT_P_H
#define QMLGOTOTYPEDEFINITIONSUPPORT_P_H

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

#include "qlanguageserver_p.h"
#include "qqmlcodemodel_p.h"
#include "qqmlbasemodule_p.h"

struct TypeDefinitionRequest
    : public BaseRequest<
              QLspSpecification::TypeDefinitionParams,
              QLspSpecification::LSPPartialResponse<
                      std::variant<QLspSpecification::Location, QList<QLspSpecification::Location>,
                                   QList<QLspSpecification::LocationLink>, std::nullptr_t>,
                      std::variant<QList<QLspSpecification::Location>,
                                   QList<QLspSpecification::LocationLink>>>>
{
    bool fillFrom(QmlLsp::OpenDocument doc, const Parameters &params, Response &&response);
};

class QmlGoToTypeDefinitionSupport : public QQmlBaseModule<TypeDefinitionRequest>
{
    Q_OBJECT
public:
    QmlGoToTypeDefinitionSupport(QmlLsp::QQmlCodeModel *codeModel);

    QString name() const override;
    void registerHandlers(QLanguageServer *server, QLanguageServerProtocol *protocol) override;
    void setupCapabilities(const QLspSpecification::InitializeParams &clientInfo,
                           QLspSpecification::InitializeResult &) override;

    void process(RequestPointerArgument request) override;

    void typeDefinitionRequestHandler(const QByteArray &,
                                      const QLspSpecification::TypeDefinitionParams &params,
                                      TypeDefinitionRequest::Response &&response);

public Q_SLOTS:
    void updatedSnapshot(const QByteArray &uri);
};

#endif // QMLGOTOTYPEDEFINITIONSUPPORT_P_H
