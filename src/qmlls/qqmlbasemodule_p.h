// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

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
#include "qqmlcodemodel_p.h"

#include <QObject>
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
    int minVersion;
    Parameters parameters;
    Response response;

    bool fillFrom(QmlLsp::OpenDocument doc, const Parameters &params, Response &&response);
    void sendResponse();
};

template<typename RequestType>
struct QQmlBaseModule : public QLanguageServerModule
{
    using RequestParameters = typename RequestType::Parameters;
    using RequestResponse = typename RequestType::Response;
    using RequestPointer = std::unique_ptr<RequestType>;
    using RequestPointerArgument = RequestPointer &&;

    QQmlBaseModule(QmlLsp::QQmlCodeModel *codeModel);
    ~QQmlBaseModule();

    bool addRequestAndCheckForUpdate(const QmlLsp::OpenDocument,
                                     const RequestParameters &parameters,
                                     RequestResponse &&response);
    void processPending(const QByteArray &url);
    // processes a request in a different thread.
    virtual void process(RequestPointerArgument toBeProcessed) = 0;

protected:
    QMutex m_pending_mutex;
    std::unordered_multimap<QString, RequestPointer> m_pending;
    QmlLsp::QQmlCodeModel *m_codeModel;
};

template<typename Parameters, typename Response>
bool BaseRequest<Parameters, Response>::fillFrom(QmlLsp::OpenDocument doc, const Parameters &params,
                                                 Response &&resp)
{
    Q_UNUSED(doc);
    parameters = params;
    response = std::move(resp);
    return true;
}

template<typename RequestType>
QQmlBaseModule<RequestType>::QQmlBaseModule(QmlLsp::QQmlCodeModel *codeModel)
    : m_codeModel(codeModel)
{
}

template<typename RequestType>
QQmlBaseModule<RequestType>::~QQmlBaseModule()
{
    QMutexLocker l(&m_pending_mutex);
    m_pending.clear(); // empty the m_pending while the mutex is hold
}

// make the registerHandlers method easier to write
template<typename RequestType>
bool QQmlBaseModule<RequestType>::addRequestAndCheckForUpdate(QmlLsp::OpenDocument doc,
                                                              const RequestParameters &parameters,
                                                              RequestResponse &&response)
{
    auto req = std::make_unique<RequestType>();
    const bool requestIsValid = req->fillFrom(doc, parameters, std::move(response));
    if (!requestIsValid) {
        req->response.sendErrorResponse(0, "Received invalid request", parameters);
        return false;
    }
    const int minVersion = req->minVersion;
    {
        QMutexLocker l(&m_pending_mutex);
        m_pending.insert({ QString::fromUtf8(req->parameters.textDocument.uri), std::move(req) });
    }
    const bool requireSnapshotUpdate =
            doc.snapshot.docVersion && *doc.snapshot.docVersion >= minVersion;
    return requireSnapshotUpdate;
}

// make the updatedSnapshot method easier to write
template<typename RequestType>
void QQmlBaseModule<RequestType>::processPending(const QByteArray &url)
{
    QmlLsp::OpenDocumentSnapshot doc = m_codeModel->snapshotByUrl(url);
    std::vector<RequestPointer> toCompl;
    {
        QMutexLocker l(&m_pending_mutex);
        for (auto [it, end] = m_pending.equal_range(QString::fromUtf8(url)); it != end;) {
            if (auto &[key, value] = *it; doc.docVersion && value->minVersion <= *doc.docVersion) {
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

#endif // QQMLBASEMODULE_P_H
