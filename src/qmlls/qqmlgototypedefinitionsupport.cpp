// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

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
    QScopeGuard onExit([&results, &request]() { request->m_response.sendResponse(results); });

    auto itemsFound = itemsForRequest(request);
    if (!itemsFound) {
        return;
    }

    QQmlJS::Dom::DomItem base = QQmlLSUtils::findTypeDefinitionOf(itemsFound->first().domItem);
    if (base.domKind() == QQmlJS::Dom::DomKind::Empty) {
        qWarning() << u"Could not obtain the type definition, was the type correctly resolved?"_s
                   << u"\n Obtained type was:\n"_s << base.toString()
                   << u"\nbut selected item was:\n"
                   << itemsFound->first().domItem.toString();
        return;
    }

    if (base.domKind() == QQmlJS::Dom::DomKind::Empty) {
        qWarning() << u"Could not obtain the base from the item"_s;
        return;
    }
    auto locationInfo = QQmlJS::Dom::FileLocations::fileLocationsOf(base);
    if (!locationInfo) {
        qWarning()
                << u"Could not obtain the text location from the base item, was it correctly resolved?\nBase was "_s
                << base.toString();
        return;
    }

    QQmlJS::Dom::DomItem fileOfBase = base.containingFile();
    auto fileOfBasePtr = fileOfBase.ownerAs<QQmlJS::Dom::QmlFile>();
    if (!fileOfBasePtr) {
        qWarning() << u"Could not obtain the file of the base."_s;
        return;
    }

    QLspSpecification::Location l;
    l.uri = QUrl::fromLocalFile(fileOfBasePtr->canonicalFilePath()).toEncoded();

    const QString qmlCode = fileOfBasePtr->code();
    l.range = QQmlLSUtils::qmlLocationToLspLocation(qmlCode, locationInfo->fullRegion);

    results.append(l);
}

QT_END_NAMESPACE
