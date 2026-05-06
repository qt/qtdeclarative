// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "qqmlgotodefinitionsupport_p.h"
#include "qqmllsutils_p.h"
#include <QtLanguageServer/private/qlanguageserverspectypes_p.h>
#include <QtQmlDom/private/qqmldomexternalitems_p.h>
#include <QtQmlDom/private/qqmldomtop_p.h>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

QmlGoToDefinitionSupport::QmlGoToDefinitionSupport(QmlLsp::QQmlCodeModelManager *codeModel)
    : BaseT(codeModel)
{
}

QString QmlGoToDefinitionSupport::name() const
{
    return u"QmlDefinitionSupport"_s;
}

void QmlGoToDefinitionSupport::setupCapabilities(
        const QLspSpecification::InitializeParams &,
        QLspSpecification::InitializeResult &serverCapabilities)
{
    // just assume serverCapabilities.capabilities.typeDefinitionProvider is a bool for now
    // handle the TypeDefinitionOptions and TypeDefinitionRegistrationOptions cases later on, if
    // needed (as they just allow more fancy go-to-type-definition action).
    serverCapabilities.capabilities.definitionProvider = true;
}

void QmlGoToDefinitionSupport::registerHandlers(QLanguageServer *,
                                                QLanguageServerProtocol *protocol)
{
    protocol->registerDefinitionRequestHandler(getRequestHandler());
}

void QmlGoToDefinitionSupport::process(RequestPointerArgument request)
{
    QList<QLspSpecification::Location> results;
    ResponseScopeGuard guard(results, request->m_response);

    auto itemsFound = itemsForRequest(request);

    if (guard.setErrorFrom(itemsFound))
        return;

    auto &front = std::get<QList<QQmlLSUtils::ItemLocation>>(itemsFound).front();

    const QByteArray shortestRootUrl =
            m_codeModelManager->shortestRootUrlForFile(request->m_parameters.textDocument.uri);

    const QStringList headerDirectories = shortestRootUrl.isEmpty()
            ? QStringList{}
            : QStringList{ QUrl::fromEncoded(shortestRootUrl).toLocalFile() };

    const auto locations = QQmlLSUtils::findDefinitionOf(front.domItem, headerDirectories);

    for (const QQmlLSUtils::Location &location : locations) {
        results.append({ QUrl::fromLocalFile(location.filename()).toEncoded(),
                         QQmlLSUtils::qmlLocationToLspLocation(location) });
    }
}
QT_END_NAMESPACE
