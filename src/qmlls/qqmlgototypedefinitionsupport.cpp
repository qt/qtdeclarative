// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "qqmlgototypedefinitionsupport_p.h"
#include "qqmllsutils_p.h"
#include <QtLanguageServer/private/qlanguageserverspectypes_p.h>
#include <QtQmlDom/private/qqmldomexternalitems_p.h>
#include <QtQmlDom/private/qqmldomtop_p.h>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

QmlGoToTypeDefinitionSupport::QmlGoToTypeDefinitionSupport(QmlLsp::QQmlCodeModel *codeModel)
    : BaseT(codeModel)
{
}

QString QmlGoToTypeDefinitionSupport::name() const
{
    return u"QmlNavigationSupport"_s;
}

void QmlGoToTypeDefinitionSupport::setupCapabilities(
        const QLspSpecification::InitializeParams &,
        QLspSpecification::InitializeResult &serverCapabilities)
{
    // just assume serverCapabilities.capabilities.typeDefinitionProvider is a bool for now
    // handle the TypeDefinitionOptions and TypeDefinitionRegistrationOptions cases later on, if
    // needed (as they just allow more fancy go-to-type-definition action).
    serverCapabilities.capabilities.typeDefinitionProvider = true;
}

void QmlGoToTypeDefinitionSupport::registerHandlers(QLanguageServer *,
                                                    QLanguageServerProtocol *protocol)
{
    protocol->registerTypeDefinitionRequestHandler(getRequestHandler());
}

void QmlGoToTypeDefinitionSupport::process(RequestPointerArgument request)
{
    QList<QLspSpecification::Location> results;
    ResponseScopeGuard guard(results, request->m_response);

    auto itemsFound = itemsForRequest(request);
    if (guard.setErrorFrom(itemsFound))
        return;

    auto &front = std::get<QList<QQmlLSUtils::ItemLocation>>(itemsFound).front();

    auto base = QQmlLSUtils::findTypeDefinitionOf(front.domItem);

    if (!base) {
        qDebug() << u"Could not obtain the base from the item"_s;
        return;
    }

    QLspSpecification::Location l;
    l.uri = QUrl::fromLocalFile(base->filename()).toEncoded();
    l.range = QQmlLSUtils::qmlLocationToLspLocation(*base);

    results.append(l);
}

QT_END_NAMESPACE
