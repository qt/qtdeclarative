// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <qqmlformatting_p.h>
#include <qqmlcodemodel_p.h>
#include <qqmllsutils_p.h>

#include <QtQmlDom/private/qqmldomitem_p.h>
#include <QtQmlDom/private/qqmldomindentinglinewriter_p.h>
#include <QtQmlDom/private/qqmldomoutwriter_p.h>

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(formatLog, "qt.languageserver.formatting")

QQmlDocumentFormatting::QQmlDocumentFormatting(QmlLsp::QQmlCodeModel *codeModel)
    : QQmlBaseModule(codeModel)
{
}

QString QQmlDocumentFormatting::name() const
{
    return u"QQmlDocumentFormatting"_s;
}

void QQmlDocumentFormatting::registerHandlers(QLanguageServer *, QLanguageServerProtocol *protocol)
{
    protocol->registerDocumentFormattingRequestHandler(getRequestHandler());
}

void QQmlDocumentFormatting::setupCapabilities(
        const QLspSpecification::InitializeParams &,
        QLspSpecification::InitializeResult &serverCapabilities)
{
    // TODO: Allow customized formatting in future
    serverCapabilities.capabilities.documentFormattingProvider = true;
}

void QQmlDocumentFormatting::process(RequestPointerArgument request)
{
    using namespace QQmlJS::Dom;
    QmlLsp::OpenDocument doc = m_codeModel->openDocumentByUrl(
                QQmlLSUtils::lspUriToQmlUrl(request->m_parameters.textDocument.uri));

    DomItem file = doc.snapshot.doc.fileObject(GoTo::MostLikely);
    if (!file) {
        qWarning() << u"Could not find the file"_s << doc.snapshot.doc.toString();
        return;
    }
    if (!file.field(Fields::isValid).value().toBool(false)) {
        qWarning() << u"Invalid document will not be formatted"_s;
        return;
    }
    if (auto envPtr = file.environment().ownerAs<DomEnvironment>())
        envPtr->clearReferenceCache();

    auto qmlFile = file.ownerAs<QmlFile>();
    if (!qmlFile || !qmlFile->isValid()) {
        file.iterateErrors(
                [](DomItem, ErrorMessage msg) {
                    errorToQDebug(msg);
                    return true;
                },
                true);
        qWarning().noquote() << "Failed to parse" << file;
        return;
    }

    // TODO: implement formatting options
    // For now, qmlformat's default options.
    LineWriterOptions options;
    options.updateOptions = LineWriterOptions::Update::None;
    options.attributesSequence = LineWriterOptions::AttributesSequence::Preserve;

    QLspSpecification::TextEdit formattedText;
    LineWriter lw([&formattedText](QStringView s) {formattedText.newText += s.toUtf8(); }, QString(), options);
    OutWriter ow(lw);
    MutableDomItem result = file.writeOutForFile(ow, WriteOutCheck::Default);
    ow.flush();

    const auto &code = qmlFile->code();
    const auto [endLine, endColumn] = QQmlLSUtils::textRowAndColumnFrom(code, code.length());

    Q_UNUSED(endColumn);
    formattedText.range = QLspSpecification::Range{ QLspSpecification::Position{ 0, 0 },
                                                    QLspSpecification::Position{ endLine + 1, 0 } };

    request->m_response.sendResponse(QList<QLspSpecification::TextEdit>{ formattedText });
}

QT_END_NAMESPACE
