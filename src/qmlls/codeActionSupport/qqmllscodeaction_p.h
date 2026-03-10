// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQMLLSCODEACTION_P_H
#define QQMLLSCODEACTION_P_H

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

#include <QtQmlLS/private/qlanguageserver_p.h>
#include <QtQmlLS/private/qqmlbasemodule_p.h>
#include <QtQmlLS/private/qqmlcodemodelmanager_p.h>

QT_BEGIN_NAMESPACE

struct CodeActionRequest : public BaseRequest<QLspSpecification::CodeActionParams,
                                              QLspSpecification::Responses::CodeActionResponseType>
{
};

class QQmlCodeActionSupport : public QQmlBaseModule<CodeActionRequest>
{
public:
    QQmlCodeActionSupport(QmlLsp::QQmlCodeModelManager *codeModel);
    void registerHandlers(QLanguageServer *server, QLanguageServerProtocol *protocol) override;
    void setupCapabilities(QLspSpecification::ServerCapabilities &caps) override;
    void process(RequestPointerArgument req) override;
};

QT_END_NAMESPACE

#endif // QQMLLSCODEACTION_P_H
