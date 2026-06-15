// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQMLHIGHLIGHTSUPPORT_P_H
#define QQMLHIGHLIGHTSUPPORT_P_H

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

#include <private/qlanguageserver_p.h>
#include <private/qqmlbasemodule_p.h>
#include <private/qqmlsemantictokens_p.h>

QT_BEGIN_NAMESPACE

// We don't need these overrides as we register the request handlers in a single
// module QQmlHighlightSupport. This is an unusual pattern because QQmlBaseModule
// and QLanguageServerModule abstractions are designed to handle a single module
// which has a single request handlers. That is not the case for the semanticTokens
// module which has a one server module but also has three different handlers.
#define HIDE_UNUSED_OVERRIDES \
    void setupCapabilities(QLspSpecification::ServerCapabilities &) override { }

using SemanticTokensRequest = BaseRequest<QLspSpecification::SemanticTokensParams,
                                          QLspSpecification::Responses::SemanticTokensResponseType>;

using SemanticTokensDeltaRequest =
        BaseRequest<QLspSpecification::SemanticTokensDeltaParams,
                    QLspSpecification::Responses::SemanticTokensDeltaResponseType>;

using SemanticTokensRangeRequest =
        BaseRequest<QLspSpecification::SemanticTokensRangeParams,
                    QLspSpecification::Responses::SemanticTokensRangeResponseType>;

class SemanticTokenFullHandler : public QQmlBaseModule<SemanticTokensRequest>
{
public:
    SemanticTokenFullHandler(QmlLsp::QQmlCodeModelManager *codeModel);
    void process(QQmlBaseModule<SemanticTokensRequest>::RequestPointerArgument req) override;
    void registerHandlers(QLanguageServer *, QLanguageServerProtocol *) override;
    void setHighlightingMode(QmlHighlighting::HighlightingMode mode) { m_mode = mode; }
    HIDE_UNUSED_OVERRIDES
    QmlLsp::QQmlCodeModelManager *manager() const { return m_codeModelManager; }

private:
    QmlHighlighting::HighlightingMode m_mode;
};

class SemanticTokenDeltaHandler : public QQmlBaseModule<SemanticTokensDeltaRequest>
{
public:
    SemanticTokenDeltaHandler(QmlLsp::QQmlCodeModelManager *codeModel);
    void process(QQmlBaseModule<SemanticTokensDeltaRequest>::RequestPointerArgument req) override;
    void registerHandlers(QLanguageServer *, QLanguageServerProtocol *) override;
    void setHighlightingMode(QmlHighlighting::HighlightingMode mode) { m_mode = mode; }
    HIDE_UNUSED_OVERRIDES
private:
    QmlHighlighting::HighlightingMode m_mode;
};

class SemanticTokenRangeHandler : public QQmlBaseModule<SemanticTokensRangeRequest>
{
public:
    SemanticTokenRangeHandler(QmlLsp::QQmlCodeModelManager *codeModel);
    void process(QQmlBaseModule<SemanticTokensRangeRequest>::RequestPointerArgument req) override;
    void registerHandlers(QLanguageServer *, QLanguageServerProtocol *) override;
    void setHighlightingMode(QmlHighlighting::HighlightingMode mode) { m_mode = mode; }
    HIDE_UNUSED_OVERRIDES
private:
    QmlHighlighting::HighlightingMode m_mode;;
};

class QQmlHighlightSupport : public QLanguageServerModule
{
public:
    QQmlHighlightSupport(QmlLsp::QQmlCodeModelManager *codeModel);
    void registerHandlers(QLanguageServer *server, QLanguageServerProtocol *protocol) override;
    void setupCapabilities(QLspSpecification::ServerCapabilities &caps) override;
public Q_SLOTS:
    void clientInitialized(QLanguageServer *);

private:
    SemanticTokenFullHandler m_full;
    SemanticTokenDeltaHandler m_delta;
    SemanticTokenRangeHandler m_range;
};

#undef HIDE_UNUSED_OVERRIDES

QT_END_NAMESPACE

#endif // QQMLHIGHLIGHTSUPPORT_P_H
