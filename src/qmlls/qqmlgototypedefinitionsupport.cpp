// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qqmlgototypedefinitionsupport_p.h"
#include "qqmllsutils_p.h"
#include <QtLanguageServer/private/qlanguageserverspectypes_p.h>
#include <QtQmlDom/private/qqmldomexternalitems_p.h>
#include <QtQmlDom/private/qqmldomtop_p.h>

QT_USE_NAMESPACE
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

    QmlLsp::OpenDocument doc;
    {
        doc = m_codeModel->openDocumentByUrl(
                QQmlLSUtils::lspUriToQmlUrl(request->m_parameters.textDocument.uri));
    }

    QQmlJS::Dom::DomItem file = doc.snapshot.validDoc.fileObject(QQmlJS::Dom::GoTo::MostLikely);
    // clear reference cache to resolve latest versions (use a local env instead?)
    if (auto envPtr = file.environment().ownerAs<QQmlJS::Dom::DomEnvironment>())
        envPtr->clearReferenceCache();
    if (!file) {
        qWarning() << u"Could not find file in Dom Environment from Codemodel :"_s
                   << doc.snapshot.doc.toString();
        return;
    }

    auto itemsFound = QQmlLSUtils::itemsFromTextLocation(file, request->m_parameters.position.line,
                                                         request->m_parameters.position.character);
    if (itemsFound.size() == 0) {
        qWarning() << u"Could not find any items at given text location."_s;
        return;
    }

    QQmlJS::Dom::DomItem base = QQmlLSUtils::findTypeDefinitionOf(itemsFound.first().domItem);
    if (base.domKind() == QQmlJS::Dom::DomKind::Empty) {
        qWarning() << u"Could not obtain the type definition, was the type correctly resolved?"_s
                   << u"\n Obtained type was:\n"_s << base.toString()
                   << u"\nbut selected item was:\n"
                   << itemsFound.first().domItem.toString();
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

    auto begin = QQmlLSUtils::textRowAndColumnFrom(qmlCode, locationInfo->fullRegion.begin());
    l.range.start.line = begin.line;
    l.range.start.character = begin.character;

    auto end = QQmlLSUtils::textRowAndColumnFrom(qmlCode, locationInfo->fullRegion.end());
    l.range.end.line = end.line;
    l.range.end.character = end.character;

    results.append(l);
}
