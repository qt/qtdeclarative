// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "qqmllintsuggestions_p.h"

#include <QtLanguageServer/private/qlanguageserverspec_p.h>
#include <QtQmlCompiler/private/qqmljslogger_p.h>
#include <QtQmlCompiler/private/qqmljsutils_p.h>
#include <QtQmlDom/private/qqmldom_utils_p.h>
#include <QtQmlDom/private/qqmldomtop_p.h>
#include <QtQmlLint/private/qqmljslinter_p.h>
#include <QtCore/qdebug.h>
#include <QtCore/qdir.h>
#include <QtCore/qfileinfo.h>
#include <QtCore/qlibraryinfo.h>
#include <QtCore/qtimer.h>
#include <QtCore/qxpfunctional.h>
#include <chrono>

QT_BEGIN_NAMESPACE

Q_STATIC_LOGGING_CATEGORY(lintLog, "qt.languageserver.lint")

using namespace QLspSpecification;
using namespace QQmlJS::Dom;
using namespace Qt::StringLiterals;

namespace QmlLsp {

static DiagnosticSeverity severityFromMsgType(QtMsgType t)
{
    switch (t) {
    case QtDebugMsg:
        return DiagnosticSeverity::Hint;
    case QtInfoMsg:
        return DiagnosticSeverity::Information;
    case QtWarningMsg:
        return DiagnosticSeverity::Warning;
    case QtCriticalMsg:
    case QtFatalMsg:
        break;
    }
    return DiagnosticSeverity::Error;
}

QmlLintSuggestions::QmlLintSuggestions(QLanguageServer *server,
                                       QmlLsp::QQmlCodeModelManager *codeModelManager)
    : m_server(server), m_codeModelManager(codeModelManager)
{
    QObject::connect(m_codeModelManager, &QmlLsp::QQmlCodeModelManager::updatedSnapshot, this,
                     &QmlLintSuggestions::diagnose, Qt::DirectConnection);
}

static void advancePositionPastLocation_helper(const QString &fileContents, const QQmlJS::SourceLocation &location, Position &position) {
    const int startOffset = location.offset;
    const int length = location.length;
    int i = startOffset;
    int iEnd = i + length;
    if (iEnd > int(fileContents.size()))
        iEnd = fileContents.size();
    while (i < iEnd) {
        if (fileContents.at(i) == u'\n') {
            ++position.line;
            position.character = 0;
            if (i + 1 < iEnd && fileContents.at(i) == u'\r')
                ++i;
        } else {
            ++position.character;
        }
        ++i;
    }
};

static Diagnostic createMissingBuildDirDiagnostic()
{
    Diagnostic diagnostic;
    diagnostic.severity = DiagnosticSeverity::Warning;
    Range &range = diagnostic.range;
    Position &position = range.start;
    position.line = 0;
    position.character = 0;
    Position &positionEnd = range.end;
    positionEnd.line = 1;
    diagnostic.message =
            "qmlls couldn't find a build directory. Pass the \"--build-dir <buildDir>\" option to "
            "qmlls, set the environment variable \"QMLLS_BUILD_DIRS\", or create a .qmlls.ini "
            "configuration file with a \"buildDir\" value in your project's source folder to avoid "
            "spurious warnings";
    diagnostic.source = QByteArray("qmllint");
    return diagnostic;
}

using AdvanceFunc = qxp::function_ref<void(const QQmlJS::SourceLocation &, Position &)>;
QJsonArray suggestionToJson(AdvanceFunc advancePositionPastLocation, const Message &message)
{
    const auto addLocationToJsonObject = [&](const auto &location, QJsonObject &object) {
        const unsigned line = location.isValid() ? location.startLine - 1 : 0;
        const unsigned column = location.isValid() ? location.startColumn - 1 : 0;
        Position end = { line, column };
        if (location.isValid())
            advancePositionPastLocation(location, end);
        object["lspBeginLine"_L1] = double(line);
        object["lspBeginCharacter"_L1] = double(column);
        object["lspEndLine"_L1] = double(end.line);
        object["lspEndCharacter"_L1] = double(end.character);
    };

    if (!message.fixSuggestion.has_value())
        return QJsonArray();

    const QQmlJSFixSuggestion &suggestion = message.fixSuggestion.value();
    QJsonArray documentEditsJson;
    const auto &documentEdits = suggestion.documentEdits();
    for (const auto &documentEdit : documentEdits) {
        // We need to interject the information about where the fix suggestions end
        // here since we don't have access to the textDocument to calculate it later.
        const QQmlJS::SourceLocation cut = suggestion.location();
        const unsigned line = cut.isValid() ? cut.startLine - 1 : 0;
        const unsigned column = cut.isValid() ? cut.startColumn - 1 : 0;

        Position end = { line, column };
        if (cut.isValid())
            advancePositionPastLocation(cut, end);

        QJsonObject documentEditJson{
            { "filename"_L1, QUrl::fromLocalFile(documentEdit.m_filename).toString() },
            { "replacement"_L1, documentEdit.m_replacement }
        };
        addLocationToJsonObject(documentEdit.m_location, documentEditJson);
        documentEditsJson.append(documentEditJson);
    }

    QJsonObject fixSuggestionJson;
    fixSuggestionJson["message"_L1] = message.fixSuggestion->description();
    addLocationToJsonObject(message.fixSuggestion->location(), fixSuggestionJson);
    fixSuggestionJson["documentEdits"_L1] = documentEditsJson;
    fixSuggestionJson["autoApplicable"_L1] = message.fixSuggestion->isAutoApplicable();

    QJsonArray fixSuggestionsJson;
    fixSuggestionsJson.append(fixSuggestionJson);
    return fixSuggestionsJson;
}

static Diagnostic messageToDiagnostic_helper(AdvanceFunc advancePositionPastLocation,
                                             std::optional<int> version, const Message &message)
{
    Diagnostic diagnostic;
    diagnostic.severity = severityFromMsgType(message.type);
    Range &range = diagnostic.range;
    Position &position = range.start;

    QQmlJS::SourceLocation srcLoc = message.loc;

    if (srcLoc.isValid()) {
        position.line = srcLoc.startLine - 1;
        position.character = srcLoc.startColumn - 1;
        range.end = position;
        advancePositionPastLocation(message.loc, range.end);
    }

    if (message.fixSuggestion && !message.fixSuggestion->description().isEmpty()) {
        diagnostic.message = u"%1: %2 [%3]"_s.arg(message.message, message.fixSuggestion->description(), message.id.toString())
                                     .simplified()
                                     .toUtf8();
    } else {
        diagnostic.message = u"%1 [%2]"_s.arg(message.message, message.id.toString()).toUtf8();
    }

    diagnostic.source = QByteArray("qmllint");

    auto suggestion = message.fixSuggestion;
    if (!suggestion.has_value())
        return diagnostic;

    QJsonObject data;
    data[u"suggestions"] = suggestionToJson(advancePositionPastLocation, message);
    Q_ASSERT(version.has_value());
    data[u"version"] = version.value();

    diagnostic.data = data;
    return diagnostic;
};

static bool isSnapshotNew(std::optional<int> snapshotVersion, std::optional<int> processedVersion,
                          QmlLsp::UpdatePolicy policy)
{
    if (!snapshotVersion)
        return false;
    if (policy == ForceUpdate)
        return true;
    if (!processedVersion || *snapshotVersion > *processedVersion)
        return true;
    return false;
}

using namespace std::chrono_literals;

QmlLintSuggestions::VersionToDiagnose
QmlLintSuggestions::chooseVersionToDiagnoseHelper(const QByteArray &url,
                                                  QmlLsp::UpdatePolicy policy)
{
    const std::chrono::milliseconds maxInvalidTime = 400ms;
    QmlLsp::OpenDocumentSnapshot snapshot = m_codeModelManager->snapshotByUrl(url);

    LastLintUpdate &lastUpdate = m_lastUpdate[url];

    // ignore updates when already processed
    if (policy != ForceUpdate && lastUpdate.version && *lastUpdate.version == snapshot.docVersion) {
        qCDebug(lspServerLog) << "skipped update of " << url << "unchanged valid doc";
        return NoDocumentAvailable{};
    }

    // try out a valid version, if there is one
    if (isSnapshotNew(snapshot.validDocVersion, lastUpdate.version, policy))
        return VersionedDocument{ snapshot.validDocVersion, snapshot.validDoc };

    // try out an invalid version, if there is one
    if (isSnapshotNew(snapshot.docVersion, lastUpdate.version, policy)) {
        if (auto since = lastUpdate.invalidUpdatesSince) {
            // did we wait enough to get a valid document?
            if (std::chrono::steady_clock::now() - *since > maxInvalidTime) {
                return VersionedDocument{ snapshot.docVersion, snapshot.doc };
            }
        } else {
            // first time hitting the invalid document:
            lastUpdate.invalidUpdatesSince = std::chrono::steady_clock::now();
        }

        // wait some time for extra keystrokes before diagnose
        return TryAgainLater{ maxInvalidTime };
    }
    return NoDocumentAvailable{};
}

QmlLintSuggestions::VersionToDiagnose
QmlLintSuggestions::chooseVersionToDiagnose(const QByteArray &url, QmlLsp::UpdatePolicy policy)
{
    QMutexLocker l(&m_mutex);
    auto versionToDiagnose = chooseVersionToDiagnoseHelper(url, policy);
    if (auto versionedDocument = std::get_if<VersionedDocument>(&versionToDiagnose)) {
        // update immediately, and do not keep track of sent version, thus in extreme cases sent
        // updates could be out of sync
        LastLintUpdate &lastUpdate = m_lastUpdate[url];
        lastUpdate.version = versionedDocument->version;
        lastUpdate.invalidUpdatesSince.reset();
    }
    return versionToDiagnose;
}

void QmlLintSuggestions::diagnose(const QByteArray &url, QmlLsp::UpdatePolicy policy)
{
    auto versionedDocument = chooseVersionToDiagnose(url, policy);

    std::visit(qOverloadedVisitor{
                       [](NoDocumentAvailable) {},
                       [this, &url, &policy](const TryAgainLater &tryAgainLater) {
                           QTimer::singleShot(tryAgainLater.time, Qt::VeryCoarseTimer, this,
                                              [this, url, policy]() { diagnose(url, policy); });
                       },
                       [this, &url](const VersionedDocument &versionedDocument) {
                           diagnoseHelper(url, versionedDocument);
                       },

               },
               versionedDocument);
}

void QmlLintSuggestions::diagnoseHelper(const QByteArray &url,
                                        const VersionedDocument &versionedDocument)
{
    auto [version, doc] = versionedDocument;

    PublishDiagnosticsParams diagnosticParams;
    diagnosticParams.uri = url;
    diagnosticParams.version = version;

    qCDebug(lintLog) << "has doc, do real lint";
    QStringList imports = m_codeModelManager->importPathsForUrl(url);
    const QString filename = doc.canonicalFilePath();
    // add source directory as last import as fallback in case there is no qmldir in the build
    // folder this mimics qmllint behaviors
    imports.append(QFileInfo(filename).dir().absolutePath());
    // add m_server->clientInfo().rootUri & co?
    bool silent = true;
    const QString fileContents = doc.field(Fields::code).value().toString();
    const QStringList qmltypesFiles;
    const QStringList resourceFiles = m_codeModelManager->resourceFilesForFileUrl(url);

    QList<QQmlJS::LoggerCategory> categories = QQmlJSLogger::builtinCategories();

    QQmlJSLinter linter(imports);

    for (const QQmlJSLinter::Plugin &plugin : linter.plugins()) {
        for (const QQmlJS::LoggerCategory &category : plugin.categories())
            categories.append(category);
    }

    QQmlToolingSettings settings(QLatin1String("qmllint"));
    if (settings.search(filename).isValid()) {
        QQmlJS::LoggingUtils::updateLogSeverities(categories, settings, nullptr);
    }

    // TODO: pass the workspace folders to QQmlJSLinter
    linter.lintFile(filename, &fileContents, silent, nullptr, imports, qmltypesFiles, resourceFiles,
                    categories);

    // ###  TODO: C++20 replace with bind_front
    auto advancePositionPastLocation = [&fileContents](const QQmlJS::SourceLocation &location, Position &position)
    {
        advancePositionPastLocation_helper(fileContents, location, position);
    };
    auto messageToDiagnostic = [&advancePositionPastLocation,
                                versionedDocument](const Message &message) {
        return messageToDiagnostic_helper(advancePositionPastLocation, versionedDocument.version,
                                          message);
    };

    QList<Diagnostic> diagnostics;
    doc.iterateErrors(
            [&diagnostics, &advancePositionPastLocation](const DomItem &, const ErrorMessage &msg) {
                Diagnostic diagnostic;
                diagnostic.severity = severityFromMsgType(QtMsgType(int(msg.level)));
                // do something with msg.errorGroups ?
                auto &location = msg.location;
                Range &range = diagnostic.range;
                range.start.line = location.startLine - 1;
                range.start.character = location.startColumn - 1;
                range.end = range.start;
                advancePositionPastLocation(location, range.end);
                diagnostic.code = QByteArray(msg.errorId.data(), msg.errorId.size());
                diagnostic.source = "domParsing";
                diagnostic.message = msg.message.toUtf8();
                diagnostics.append(diagnostic);
                return true;
            },
            true);

    if (const QQmlJSLogger *logger = linter.logger()) {
        qsizetype nDiagnostics = diagnostics.size();
        logger->iterateAllMessages([&](const Message &message) {
            if (!message.message.contains(u"Failed to import")) {
                diagnostics.append(messageToDiagnostic(message));
                return;
            }

            Message modified {message};
            modified.message.append(u" Did you build your project?");

            diagnostics.append(messageToDiagnostic(modified));
        });
        if (diagnostics.size() != nDiagnostics && imports.size() == 1)
            diagnostics.append(createMissingBuildDirDiagnostic());
    }

    diagnosticParams.diagnostics = diagnostics;

    m_server->protocol()->notifyPublishDiagnostics(diagnosticParams);
    qCDebug(lintLog) << "lint" << QString::fromUtf8(url) << "found"
                     << diagnosticParams.diagnostics.size() << "issues"
                     << QTypedJson::toJsonValue(diagnosticParams);
}

} // namespace QmlLsp
QT_END_NAMESPACE
