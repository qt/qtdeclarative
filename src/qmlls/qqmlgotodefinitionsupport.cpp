// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qqmlgotodefinitionsupport_p.h"
#include "qqmllsutils_p.h"
#include <QtLanguageServer/private/qlanguageserverspectypes_p.h>
#include <QtQmlDom/private/qqmldomexternalitems_p.h>
#include <QtQmlDom/private/qqmldomtop_p.h>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

QmlGoToDefinitionSupport::QmlGoToDefinitionSupport(QmlLsp::QQmlCodeModel *codeModel)
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
    QScopeGuard onExit([&results, &request]() { request->m_response.sendResponse(results); });

    auto itemsFound = itemsForRequest(request);
    if (!itemsFound) {
        return;
    }

    auto location = QQmlLSUtils::findDefinitionOf(itemsFound->front().domItem);
    if (!location)
        return;

    QLspSpecification::Location l;
    l.uri = QUrl::fromLocalFile(location->filename).toEncoded();

    QQmlJS::Dom::DomItem file = itemsFound->front().domItem.goToFile(location->filename);
    auto fileOfBasePtr = file.ownerAs<QQmlJS::Dom::QmlFile>();
    if (!fileOfBasePtr) {
        qWarning() << u"Could not obtain the file of the base."_s;
        return;
    }
    const QString qmlCode = fileOfBasePtr->code();
    l.range = QQmlLSUtils::qmlLocationToLspLocation(qmlCode, location->location);

    results.append(l);
}
QT_END_NAMESPACE
