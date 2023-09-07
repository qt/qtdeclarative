// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qqmlcompletionsupport_p.h"
#include "qqmllsutils_p.h"

#include <QtLanguageServer/private/qlanguageserverspectypes_p.h>
#include <QtCore/qthreadpool.h>
#include <QtCore/private/qduplicatetracker_p.h>
#include <QtCore/QRegularExpression>
#include <QtQmlDom/private/qqmldomexternalitems_p.h>
#include <QtQmlDom/private/qqmldomtop_p.h>
#include <QtQml/private/qqmlsignalnames_p.h>

QT_BEGIN_NAMESPACE
using namespace QLspSpecification;
using namespace QQmlJS::Dom;
using namespace Qt::StringLiterals;

bool CompletionRequest::fillFrom(QmlLsp::OpenDocument doc, const Parameters &params,
                                 Response &&response)
{
    // do not call BaseRequest::fillFrom() to avoid taking the Mutex twice and getting an
    // inconsistent state.
    m_parameters = params;
    m_response = std::move(response);

    if (!doc.textDocument)
        return false;

    std::optional<int> targetVersion;
    {
        QMutexLocker l(doc.textDocument->mutex());
        targetVersion = doc.textDocument->version();
        code = doc.textDocument->toPlainText();
    }
    m_minVersion = (targetVersion ? *targetVersion : 0);

    return true;
}

void QmlCompletionSupport::registerHandlers(QLanguageServer *, QLanguageServerProtocol *protocol)
{
    protocol->registerCompletionRequestHandler(getRequestHandler());
    protocol->registerCompletionItemResolveRequestHandler(
            [](const QByteArray &, const CompletionItem &cParams,
               LSPResponse<CompletionItem> &&response) { response.sendResponse(cParams); });
}

QString QmlCompletionSupport::name() const
{
    return u"QmlCompletionSupport"_s;
}

void QmlCompletionSupport::setupCapabilities(
        const QLspSpecification::InitializeParams &,
        QLspSpecification::InitializeResult &serverCapabilities)
{
    QLspSpecification::CompletionOptions cOptions;
    if (serverCapabilities.capabilities.completionProvider)
        cOptions = *serverCapabilities.capabilities.completionProvider;
    cOptions.resolveProvider = false;
    cOptions.triggerCharacters = QList<QByteArray>({ QByteArray(".") });
    serverCapabilities.capabilities.completionProvider = cOptions;
}

void QmlCompletionSupport::process(RequestPointerArgument req)
{
    QmlLsp::OpenDocumentSnapshot doc =
            m_codeModel->snapshotByUrl(req->m_parameters.textDocument.uri);
    req->sendCompletions(doc);
}

QString CompletionRequest::urlAndPos() const
{
    return QString::fromUtf8(m_parameters.textDocument.uri) + u":"
            + QString::number(m_parameters.position.line) + u":"
            + QString::number(m_parameters.position.character);
}

void CompletionRequest::sendCompletions(QmlLsp::OpenDocumentSnapshot &doc)
{
    QList<CompletionItem> res = completions(doc);
    m_response.sendResponse(res);
}

QList<CompletionItem> CompletionRequest::completions(QmlLsp::OpenDocumentSnapshot &doc) const
{
    QList<CompletionItem> res;
    if (!doc.validDoc) {
        qCWarning(QQmlLSCompletionLog) << "No valid document for completions  for "
                                       << QString::fromUtf8(m_parameters.textDocument.uri);
        // try to add some import and global completions?
        return res;
    }
    if (!doc.docVersion || *doc.docVersion < m_minVersion) {
        qCWarning(QQmlLSCompletionLog) << "sendCompletions on older doc version";
    } else if (!doc.validDocVersion || *doc.validDocVersion < m_minVersion) {
        qCWarning(QQmlLSCompletionLog) << "using outdated valid doc, position might be incorrect";
    }
    DomItem file = doc.validDoc.fileObject(QQmlJS::Dom::GoTo::MostLikely);
    // clear reference cache to resolve latest versions (use a local env instead?)
    if (std::shared_ptr<DomEnvironment> envPtr = file.environment().ownerAs<DomEnvironment>())
        envPtr->clearReferenceCache();
    qsizetype pos = QQmlLSUtils::textOffsetFrom(code, m_parameters.position.line,
                                                m_parameters.position.character);
    CompletionContextStrings ctx(code, pos);
    auto itemsFound = QQmlLSUtils::itemsFromTextLocation(file, m_parameters.position.line,
                                                         m_parameters.position.character
                                                                 - ctx.filterChars().size());
    if (itemsFound.size() > 1) {
        QStringList paths;
        for (auto &it : itemsFound)
            paths.append(it.domItem.canonicalPath().toString());
        qCWarning(QQmlLSCompletionLog) << "Multiple elements of " << urlAndPos()
                                       << " at the same depth:" << paths << "(using first)";
    }
    DomItem currentItem;
    if (!itemsFound.isEmpty())
        currentItem = itemsFound.first().domItem;
    qCDebug(QQmlLSCompletionLog) << "Completion at " << urlAndPos() << " "
                                 << m_parameters.position.line << ":"
                                 << m_parameters.position.character << "offset:" << pos
                                 << "base:" << ctx.base() << "filter:" << ctx.filterChars()
                                 << "lastVersion:" << (doc.docVersion ? (*doc.docVersion) : -1)
                                 << "validVersion:"
                                 << (doc.validDocVersion ? (*doc.validDocVersion) : -1) << "in"
                                 << currentItem.internalKindStr() << currentItem.canonicalPath();
    auto result = QQmlLSUtils::completions(currentItem, ctx);
    return result;
}
QT_END_NAMESPACE
