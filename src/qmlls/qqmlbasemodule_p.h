// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQMLBASEMODULE_P_H
#define QQMLBASEMODULE_P_H

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
#include "qqmlcodemodelmanager_p.h"
#include "qqmllsutils_p.h"
#include <QtQmlDom/private/qqmldom_utils_p.h>

#include <QObject>
#include <type_traits>
#include <unordered_map>

template<typename ParametersT, typename ResponseT>
struct BaseRequest
{
    // allow using Parameters and Response type aliases in the
    // implementations of the different requests.
    using Parameters = ParametersT;
    using Response = ResponseT;

    // The version of the code on which the typedefinition request was made.
    // Request is received: mark it with the current version of the textDocument.
    // Then, wait for the codemodel to finish creating a snapshot version that is newer or equal to
    // the textDocument version at request-received-time.
    int m_minVersion;
    Parameters m_parameters;
    Response m_response;

    bool fillFrom(QmlLsp::OpenDocument doc, const Parameters &params, Response &&response);
};

/*!
\internal
\brief This class sends a result or an error when going out of scope.

It has a helper method \c setErrorFrom that sets an error from variant and optionals.
*/

template<typename Result, typename ResponseCallback>
struct ResponseScopeGuard
{
    Q_DISABLE_COPY_MOVE(ResponseScopeGuard)

    std::variant<Result *, QQmlLSUtils::ErrorMessage> m_response;
    ResponseCallback &m_callback;

    ResponseScopeGuard(Result &results, ResponseCallback &callback)
        : m_response(&results), m_callback(callback)
    {
    }

    // note: discards the current result or error message, if there is any
    void setError(const QQmlLSUtils::ErrorMessage &error) { m_response = error; }

    ~ResponseScopeGuard()
    {
        std::visit(qOverloadedVisitor{ [this](Result *result) { m_callback.sendResponse(*result); },
                                       [this](const QQmlLSUtils::ErrorMessage &error) {
                                           m_callback.sendErrorResponse(error.code,
                                                                        error.message.toUtf8());
                                       } },
                   m_response);
    }
};

template<typename RequestType>
struct QQmlBaseModule : public QLanguageServerModule
{
    using RequestParameters = typename RequestType::Parameters;
    using RequestResponse = typename RequestType::Response;
    using RequestPointer = std::unique_ptr<RequestType>;
    using RequestPointerArgument = RequestPointer &&;
    using BaseT = QQmlBaseModule<RequestType>;

    QQmlBaseModule(QmlLsp::QQmlCodeModelManager *codeModel);
    ~QQmlBaseModule();

    void requestHandler(const RequestParameters &parameters, RequestResponse &&response);
    decltype(auto) getRequestHandler();
    // processes a request in a different thread.
    virtual void process(RequestPointerArgument toBeProcessed) = 0;
    q23::expected<QList<QQmlLSUtils::ItemLocation>, QQmlLSUtils::ErrorMessage>
    itemsForRequest(const RequestPointer &request);

    q23::expected<QList<QQmlLSUtils::ItemLocation>, QQmlLSUtils::ErrorMessage>
    tryLocateItems(const QmlLsp::OpenDocument &doc, const QLspSpecification::Position &position);

    q23::expected<QmlLsp::OpenDocument, QQmlLSUtils::ErrorMessage>
    tryOpenDocument(const QByteArray &uri);

public Q_SLOTS:
    void updatedSnapshot(const QByteArray &uri);

protected:
    QMutex m_pending_mutex;
    std::unordered_multimap<QString, RequestPointer> m_pending;
    QmlLsp::QQmlCodeModelManager *m_codeModelManager;
};

template<typename Parameters, typename Response>
bool BaseRequest<Parameters, Response>::fillFrom(QmlLsp::OpenDocument doc, const Parameters &params,
                                                 Response &&response)
{
    Q_UNUSED(doc);
    m_parameters = params;
    m_response = std::move(response);

    if (!doc.textDocument) {
        qDebug() << "Cannot find document in qmlls's codemodel, did you open it before accessing "
                    "it?";
        return false;
    }

    {
        QMutexLocker l(doc.textDocument->mutex());
        m_minVersion = doc.textDocument->version().value_or(0);
    }
    return true;
}

template <typename RequestType>
QQmlBaseModule<RequestType>::QQmlBaseModule(QmlLsp::QQmlCodeModelManager *codeModelManager)
    : m_codeModelManager(codeModelManager)
{
    QObject::connect(m_codeModelManager, &QmlLsp::QQmlCodeModelManager::updatedSnapshot, this,
                     &QQmlBaseModule<RequestType>::updatedSnapshot);
}

template<typename RequestType>
QQmlBaseModule<RequestType>::~QQmlBaseModule()
{
    QMutexLocker l(&m_pending_mutex);
    m_pending.clear(); // empty the m_pending while the mutex is hold
}

template<typename RequestType>
decltype(auto) QQmlBaseModule<RequestType>::getRequestHandler()
{
    auto handler = [this](const QByteArray &, const RequestParameters &parameters,
                          RequestResponse &&response) {
        requestHandler(parameters, std::move(response));
    };
    return handler;
}

template<typename RequestType>
void QQmlBaseModule<RequestType>::requestHandler(const RequestParameters &parameters,
                                                 RequestResponse &&response)
{
    auto req = std::make_unique<RequestType>();
    QmlLsp::OpenDocument doc = m_codeModelManager->openDocumentByUrl(
            QQmlLSUtils::lspUriToQmlUrl(parameters.textDocument.uri));

    if (!req->fillFrom(doc, parameters, std::move(response))) {
        req->m_response.sendErrorResponse(0, "Received invalid request.", parameters);
        return;
    }
    const int minVersion = req->m_minVersion;
    {
        QMutexLocker l(&m_pending_mutex);
        m_pending.insert({ QString::fromUtf8(req->m_parameters.textDocument.uri), std::move(req) });
    }

    if (doc.snapshot.docVersion && *doc.snapshot.docVersion >= minVersion)
        updatedSnapshot(QQmlLSUtils::lspUriToQmlUrl(parameters.textDocument.uri));
}

template<typename RequestType>
void QQmlBaseModule<RequestType>::updatedSnapshot(const QByteArray &url)
{
    QmlLsp::OpenDocumentSnapshot doc = m_codeModelManager->snapshotByUrl(url);
    std::vector<RequestPointer> toCompl;
    {
        QMutexLocker l(&m_pending_mutex);
        for (auto [it, end] = m_pending.equal_range(QString::fromUtf8(url)); it != end;) {
            if (auto &[key, value] = *it;
                doc.docVersion && value->m_minVersion <= *doc.docVersion) {
                toCompl.push_back(std::move(value));
                it = m_pending.erase(it);
            } else {
                ++it;
            }
        }
    }
    for (auto it = toCompl.rbegin(), end = toCompl.rend(); it != end; ++it) {
        process(std::move(*it));
    }
}

template <typename RequestType>
q23::expected<QList<QQmlLSUtils::ItemLocation>, QQmlLSUtils::ErrorMessage>
QQmlBaseModule<RequestType>::itemsForRequest(const RequestPointer &request)
{
    return tryOpenDocument(request->m_parameters.textDocument.uri).and_then([&](const auto &doc) {
        return tryLocateItems(doc, request->m_parameters.position);
    });
}

template <typename RequestType>
q23::expected<QmlLsp::OpenDocument, QQmlLSUtils::ErrorMessage>
QQmlBaseModule<RequestType>::tryOpenDocument(const QByteArray &uri)
{
    QmlLsp::OpenDocument doc =
            m_codeModelManager->openDocumentByUrl(QQmlLSUtils::lspUriToQmlUrl(uri));

    if (!doc.snapshot.validDocVersion || doc.snapshot.validDocVersion != doc.snapshot.docVersion) {
        return q23::unexpected(
                QQmlLSUtils::ErrorMessage{ 0,
                                           u"Cannot proceed: current QML document is invalid! Fix"
                                           u" all the errors in your QML code and try again."_s });
    }
    return doc;
}

template <typename RequestType>
q23::expected<QList<QQmlLSUtils::ItemLocation>, QQmlLSUtils::ErrorMessage>
QQmlBaseModule<RequestType>::tryLocateItems(const QmlLsp::OpenDocument &doc,
                                            const QLspSpecification::Position &position)
{
    QQmlJS::Dom::DomItem file = doc.snapshot.validDoc.fileObject(QQmlJS::Dom::GoTo::MostLikely);
    // clear reference cache to resolve latest versions (use a local env instead?)
    if (auto envPtr = file.environment().ownerAs<QQmlJS::Dom::DomEnvironment>())
        envPtr->clearReferenceCache();
    if (!file) {
        return q23::unexpected(QQmlLSUtils::ErrorMessage{
                0,
                u"Could not find file %1 in project."_s.arg(doc.snapshot.doc.toString()),
        });
    }

    auto itemsFound = QQmlLSUtils::itemsFromTextLocation(file, position.line, position.character);

    if (itemsFound.isEmpty()) {
        return q23::unexpected(QQmlLSUtils::ErrorMessage{
                0,
                u"Could not find any items at given text location."_s,
        });
    }
    return itemsFound;
}

#endif // QQMLBASEMODULE_P_H
