// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <qqmlformatting_p.h>
#include <qqmlcodemodel_p.h>
#include <qqmllsutils_p.h>

#include <QtQmlDom/private/qqmldomitem_p.h>
#include <QtQmlDom/private/qqmldomindentinglinewriter_p.h>
#include <QtQmlDom/private/qqmldomoutwriter_p.h>
#include <QtQmlFormat/private/qqmlformatsettings_p.h>

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
    QList<QLspSpecification::TextEdit> result;
    ResponseScopeGuard guard(result, request->m_response);

    using namespace QQmlJS::Dom;
    QmlLsp::OpenDocument doc = m_codeModel->openDocumentByUrl(
                QQmlLSUtils::lspUriToQmlUrl(request->m_parameters.textDocument.uri));

    DomItem file = doc.snapshot.doc.fileObject(GoTo::MostLikely);
    if (!file) {
        guard.setError(QQmlLSUtils::ErrorMessage{
                0, u"Could not find the file %1"_s.arg(doc.snapshot.doc.canonicalFilePath()) });
        return;
    }
    if (!file.field(Fields::isValid).value().toBool(false)) {
        guard.setError(QQmlLSUtils::ErrorMessage{ 0, u"Cannot format invalid documents!"_s });
        return;
    }
    if (auto envPtr = file.environment().ownerAs<DomEnvironment>())
        envPtr->clearReferenceCache();

    auto qmlFile = file.ownerAs<QmlFile>();
    if (!qmlFile || !qmlFile->isValid()) {
        file.iterateErrors(
                [](const DomItem &, const ErrorMessage &msg) {
                    errorToQDebug(msg);
                    return true;
                },
                true);
        guard.setError(QQmlLSUtils::ErrorMessage{
                0, u"Failed to parse %1"_s.arg(file.canonicalFilePath()) });
        return;
    }

    // TODO: implement formatting options via LSP
    // For now, qmlformat's default options via m_formatOptions and read .qmlformat.ini
    QQmlFormatSettings settings(QLatin1String("qmlformat"));
    settings.search(qmlFile->canonicalFilePath());

    QQmlFormatOptions currentFormatOptions = m_formatOptions;
    currentFormatOptions.applySettings(settings);

    QLspSpecification::TextEdit formattedText;
    LineWriter lw([&formattedText](QStringView s) { formattedText.newText += s.toUtf8(); },
                  QString(), currentFormatOptions.optionsForCode(qmlFile->code()));
    OutWriter ow(lw);
    file.writeOutForFile(ow, WriteOutCheck::None);
    ow.flush();
    const auto &code = qmlFile->code();
    const auto [endLine, endColumn] = QQmlLSUtils::textRowAndColumnFrom(code, code.length());

    Q_UNUSED(endColumn);
    formattedText.range = QLspSpecification::Range{ QLspSpecification::Position{ 0, 0 },
                                                    QLspSpecification::Position{ endLine + 1, 0 } };

    result.append(formattedText);
}

QT_END_NAMESPACE
