// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <qqmlrangeformatting_p.h>
#include <qqmlcodemodel_p.h>
#include <qqmllsutils_p.h>

#include <QtQmlDom/private/qqmldomitem_p.h>
#include <QtQmlDom/private/qqmldomindentinglinewriter_p.h>
#include <QtQmlDom/private/qqmldomcodeformatter_p.h>
#include <QtQmlDom/private/qqmldomoutwriter_p.h>
#include <QtQmlDom/private/qqmldommock_p.h>
#include <QtQmlDom/private/qqmldomcompare_p.h>

QT_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(formatLog)

QQmlRangeFormatting::QQmlRangeFormatting(QmlLsp::QQmlCodeModel *codeModel)
    : QQmlBaseModule(codeModel)
{
}

QString QQmlRangeFormatting::name() const
{
    return u"QQmlRangeFormatting"_s;
}

void QQmlRangeFormatting::registerHandlers(QLanguageServer *, QLanguageServerProtocol *protocol)
{
    protocol->registerDocumentRangeFormattingRequestHandler(getRequestHandler());
}

void QQmlRangeFormatting::setupCapabilities(const QLspSpecification::InitializeParams &,
                                            QLspSpecification::InitializeResult &serverCapabilities)
{
    serverCapabilities.capabilities.documentRangeFormattingProvider = true;
}

void QQmlRangeFormatting::process(RequestPointerArgument request)
{
    using namespace QQmlJS::Dom;
    QList<QLspSpecification::TextEdit> result{};

    QmlLsp::OpenDocument doc = m_codeModel->openDocumentByUrl(
            QQmlLSUtils::lspUriToQmlUrl(request->m_parameters.textDocument.uri));

    DomItem file = doc.snapshot.doc.fileObject(GoTo::MostLikely);
    if (!file) {
        qWarning() << u"Could not find the file"_s << doc.snapshot.doc.toString();
        return;
    }

    if (auto envPtr = file.environment().ownerAs<DomEnvironment>())
        envPtr->clearReferenceCache();

    auto qmlFile = file.ownerAs<QmlFile>();
    auto code = qmlFile->code();

    // Range requested to be formatted
    const auto selectedRange = request->m_parameters.range;
    const auto selectedRangeStartLine = selectedRange.start.line;
    const auto selectedRangeEndLine = selectedRange.end.line;
    Q_ASSERT(selectedRangeStartLine >= 0);
    Q_ASSERT(selectedRangeEndLine >= 0);

    LineWriterOptions options;
    options.updateOptions = LineWriterOptions::Update::None;
    options.attributesSequence = LineWriterOptions::AttributesSequence::Preserve;

    QTextStream in(&code);
    FormatTextStatus status = FormatTextStatus::initialStatus();
    FormatPartialStatus partialStatus({}, options.formatOptions, status);

    // Get the token status of the previous line without performing write operation
    int lineNumber = 0;
    while (!in.atEnd()) {
        const auto line = in.readLine();
        partialStatus = formatCodeLine(line, options.formatOptions, partialStatus.currentStatus);
        if (++lineNumber >= selectedRangeStartLine)
            break;
    }

    QString resultText;
    QTextStream out(&resultText);
    IndentingLineWriter lw([&out](QStringView writtenText) { out << writtenText.toUtf8(); },
                           QString(), options, partialStatus.currentStatus);
    OutWriter ow(lw);
    ow.indentNextlines = true;

    // TODO: This is a workaround and will/should be handled by the actual formatter
    // once we improve the range-formatter design in QTBUG-116139
    const auto removeSpaces = [](const QString &line) {
        QString result;
        QTextStream out(&result);
        bool previousIsSpace = false;

        int newLineCount = 0;
        for (int i = 0; i < line.length(); ++i) {
            QChar c = line.at(i);
            if (c.isSpace()) {
                if (c == '\n'_L1 && newLineCount < 2) {
                    out << '\n'_L1;
                    ++newLineCount;
                } else if (c == '\r'_L1 && (i + 1) < line.length() && line.at(i + 1) == '\n'_L1
                           && newLineCount < 2) {
                    out << "\r\n";
                    ++newLineCount;
                    ++i;
                } else {
                    if (!previousIsSpace)
                        out << ' '_L1;
                }
                previousIsSpace = true;
            } else {
                out << c;
                previousIsSpace = false;
                newLineCount = 0;
            }
        }

        out.flush();
        return result;
    };

    const auto startOffset = QQmlLSUtils::textOffsetFrom(code, selectedRangeStartLine, 0);
    auto endOffset = QQmlLSUtils::textOffsetFrom(code, selectedRangeEndLine + 1, 0);

    // note: the character at endOffset (usually a \n) is ignored. Therefore avoid
    // formatting \r\n that will ignore \n and format \r as a space (' ').
    if (endOffset < code.size() && code[endOffset - 1] == u'\r' && code[endOffset] == u'\n')
        --endOffset;

    const auto &toFormat = code.mid(startOffset, endOffset - startOffset);
    ow.write(removeSpaces(toFormat));
    ow.flush();
    ow.eof();

    const auto documentLineCount = QQmlLSUtils::textRowAndColumnFrom(code, code.length()).line;
    code.replace(startOffset, toFormat.length(), resultText);

    QLspSpecification::TextEdit add;
    add.newText = code.toUtf8();
    add.range = { { 0, 0 }, { documentLineCount + 1 } };
    result.append(add);

    request->m_response.sendResponse(result);
}

QT_END_NAMESPACE
