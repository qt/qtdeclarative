// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qqmlfindusagessupport_p.h"
#include "qqmllsutils_p.h"
#include <QtLanguageServer/private/qlanguageserverspectypes_p.h>
#include <QtQmlDom/private/qqmldomexternalitems_p.h>
#include <QtQmlDom/private/qqmldomtop_p.h>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

QQmlFindUsagesSupport::QQmlFindUsagesSupport(QmlLsp::QQmlCodeModel *codeModel)
    : BaseT(codeModel) { }

QString QQmlFindUsagesSupport::name() const
{
    return u"QmlFindUsagesSupport"_s;
}

void QQmlFindUsagesSupport::setupCapabilities(
        const QLspSpecification::InitializeParams &,
        QLspSpecification::InitializeResult &serverCapabilities)
{
    // just assume serverCapabilities.capabilities.typeDefinitionProvider is a bool for now
    // handle the ReferenceOptions later if needed (it adds the possibility to communicate the
    // current progress).
    serverCapabilities.capabilities.referencesProvider = true;
}

void QQmlFindUsagesSupport::registerHandlers(QLanguageServer *, QLanguageServerProtocol *protocol)
{
    protocol->registerReferenceRequestHandler(getRequestHandler());
}

void QQmlFindUsagesSupport::process(QQmlFindUsagesSupport::RequestPointerArgument request)
{
    QList<QLspSpecification::Location> results;
    QScopeGuard onExit([&results, &request]() { request->m_response.sendResponse(results); });

    auto itemsFound = itemsForRequest(request);
    if (!itemsFound) {
        return;
    }

    auto usages = QQmlLSUtils::findUsagesOf(itemsFound->front().domItem);

    QQmlJS::Dom::DomItem files =
            itemsFound->front().domItem.top().field(QQmlJS::Dom::Fields::qmlFileWithPath);

    QHash<QString, QString> codeCache;

    for (const auto &usage : usages) {
        QLspSpecification::Location location;
        location.uri = QUrl::fromLocalFile(usage.filename).toEncoded();

        auto cacheEntry = codeCache.find(usage.filename);
        if (cacheEntry == codeCache.end()) {
            auto file = files.key(usage.filename)
                                .field(QQmlJS::Dom::Fields::currentItem)
                                .ownerAs<QQmlJS::Dom::QmlFile>();
            if (!file) {
                qDebug() << "File" << usage.filename << "not found in DOM! Available files are"
                         << files.keys();
                continue;
            }
            cacheEntry = codeCache.insert(usage.filename, file->code());
        }

        location.range = QQmlLSUtils::qmlLocationToLspLocation(cacheEntry.value(), usage.location);

        results.append(location);
    }
}
QT_END_NAMESPACE

