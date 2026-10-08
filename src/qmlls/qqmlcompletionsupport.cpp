// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

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

QmlCompletionSupport::QmlCompletionSupport(QmlLsp::QQmlCodeModel *codeModel)
    : BaseT(codeModel), m_completionEngine(codeModel->pluginLoader())
{
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
    req->sendCompletions(req->completions(doc, m_completionEngine));
}

QString CompletionRequest::urlAndPos() const
{
    return QString::fromUtf8(m_parameters.textDocument.uri) + u":"
            + QString::number(m_parameters.position.line) + u":"
            + QString::number(m_parameters.position.character);
}

void CompletionRequest::sendCompletions(const QList<CompletionItem> &completions)
{
    m_response.sendResponse(completions);
}

static bool positionIsFollowedBySpaces(qsizetype position, const QString &code)
{
    if (position >= code.size())
        return false;

    auto newline =
            std::find_if(std::next(code.cbegin(), position), code.cend(),
                         [](const QChar &c) { return c == u'\n' || c == u'\r' || !c.isSpace(); });

    return newline == code.cend() || newline->isSpace();
}

/*!
\internal

\note Remove this method and all its usages once the new fault-tolerant parser from QTBUG-118053 is
introduced!!!

Tries to make the document valid for the parser, to be able to provide completions after dots.
The created DomItem is not in the qqmlcodemodel which mean it cannot be seen and cannot bother
other modules: it would be bad to have the linting module complain about code that was modified
here, but cannot be seen by the user.
*/
DomItem CompletionRequest::patchInvalidFileForParser(const DomItem &file, qsizetype position) const
{
    // automatic semicolon insertion after dots, if there is nothing behind the dot!
    if (position > 0 && code[position - 1] == u'.' && positionIsFollowedBySpaces(position, code)) {
        qCWarning(QQmlLSCompletionLog)
                << "Patching invalid document: adding a semicolon after '.' for "
                << QString::fromUtf8(m_parameters.textDocument.uri);

        const QString patchedCode =
                code.first(position).append(u"_dummyIdentifier;").append(code.sliced(position));

        // create a new (local) Dom only for the completions.
        // This avoids weird behaviors, like the linting module complaining about the inserted
        // semicolon that the user cannot see, for example.
        DomItem newCurrent = file.environment().makeCopy(DomItem::CopyOption::EnvConnected).item();

        DomItem result;
        auto newCurrentPtr = newCurrent.ownerAs<DomEnvironment>();
        newCurrentPtr->loadFile(
                FileToLoad::fromMemory(newCurrentPtr, file.canonicalFilePath(), patchedCode),
                [&result](Path, const DomItem &, const DomItem &newValue) {
                    result = newValue.fileObject();
                });
        newCurrentPtr->loadPendingDependencies();
        return result;
    }

    qCWarning(QQmlLSCompletionLog) << "No valid document for completions for "
                                   << QString::fromUtf8(m_parameters.textDocument.uri);

    return file;
}

QList<CompletionItem> CompletionRequest::completions(QmlLsp::OpenDocumentSnapshot &doc,
                                                     const QQmlLSCompletion &completionEngine) const
{
    QList<CompletionItem> res;


    const qsizetype pos = QQmlLSUtils::textOffsetFrom(code, m_parameters.position.line,
                                                      m_parameters.position.character);

    const bool useValidDoc =
            doc.validDoc && doc.validDocVersion && *doc.validDocVersion >= m_minVersion;

    const DomItem file = useValidDoc
            ? doc.validDoc.fileObject(QQmlJS::Dom::GoTo::MostLikely)
            : patchInvalidFileForParser(doc.doc.fileObject(QQmlJS::Dom::GoTo::MostLikely), pos);

    // clear reference cache to resolve latest versions (use a local env instead?)
    if (std::shared_ptr<DomEnvironment> envPtr = file.environment().ownerAs<DomEnvironment>())
        envPtr->clearReferenceCache();


    CompletionContextStrings ctx(code, pos);
    auto itemsFound = QQmlLSUtils::itemsFromTextLocation(file, m_parameters.position.line,
                                                         m_parameters.position.character
                                                                 - ctx.filterChars().size());
    if (itemsFound.isEmpty()) {
        qCDebug(QQmlLSCompletionLog) << "No items found for completions at" << urlAndPos();
        return {};
    }

    if (itemsFound.size() > 1) {
        QStringList paths;
        for (auto &it : itemsFound)
            paths.append(it.domItem.canonicalPath().toString());
        qCWarning(QQmlLSCompletionLog) << "Multiple elements of " << urlAndPos()
                                       << " at the same depth:" << paths << "(using first)";
    }
    const DomItem currentItem = itemsFound.first().domItem;
    qCDebug(QQmlLSCompletionLog) << "Completion at " << urlAndPos() << " "
                                 << m_parameters.position.line << ":"
                                 << m_parameters.position.character << "offset:" << pos
                                 << "base:" << ctx.base() << "filter:" << ctx.filterChars()
                                 << "lastVersion:" << (doc.docVersion ? (*doc.docVersion) : -1)
                                 << "validVersion:"
                                 << (doc.validDocVersion ? (*doc.validDocVersion) : -1) << "in"
                                 << currentItem.internalKindStr() << currentItem.canonicalPath();
    auto result = completionEngine.completions(currentItem, ctx);
    return result;
}
QT_END_NAMESPACE
