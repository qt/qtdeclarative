/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the tools applications of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/
#include "qmllintsuggestions.h"
#include <QtLanguageServer/private/qlanguageserverspec_p.h>
#include <QtQmlLint/private/qqmllinter_p.h>
#include <QtQmlCompiler/private/qqmljslogger_p.h>
#include <QtCore/qlibraryinfo.h>
#include <QtCore/qtimer.h>
#include <QtCore/qdebug.h>

using namespace QLspSpecification;
using namespace QQmlJS::Dom;

Q_LOGGING_CATEGORY(lintLog, "qt.languageserver.lint")

QT_BEGIN_NAMESPACE
namespace QmlLsp {

static DiagnosticSeverity severityFromString(const QStringView &str)
{
    if (str.compare(u"debug", Qt::CaseInsensitive) == 0)
        return DiagnosticSeverity::Hint;
    else if (str.compare(u"warning", Qt::CaseInsensitive) == 0)
        return DiagnosticSeverity::Warning;
    else if (str.compare(u"critical", Qt::CaseInsensitive) == 0)
        return DiagnosticSeverity::Error;
    else if (str.compare(u"fatal", Qt::CaseInsensitive) == 0)
        return DiagnosticSeverity::Error;
    else if (str.compare(u"info", Qt::CaseInsensitive) == 0)
        return DiagnosticSeverity::Information;
    else
        return DiagnosticSeverity::Information;
}

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

QmlLintSuggestions::QmlLintSuggestions(QLanguageServer *server, QmlLsp::QQmlCodeModel *codeModel)
    : m_server(server), m_codeModel(codeModel)
{
    QObject::connect(m_codeModel, &QmlLsp::QQmlCodeModel::updatedSnapshot, this,
                     &QmlLintSuggestions::diagnose, Qt::DirectConnection);
}

void QmlLintSuggestions::diagnose(const QByteArray &uri)
{
    const int maxInvalidMsec = 4000;
    qCDebug(lintLog) << "diagnose start";
    QmlLsp::OpenDocumentSnapshot snapshot = m_codeModel->snapshotByUri(uri);
    QList<Diagnostic> diagnostics;
    std::optional<int> version;
    DomItem doc;
    {
        QMutexLocker l(&m_mutex);
        LastLintUpdate &lastUpdate = m_lastUpdate[uri];
        if (lastUpdate.version && *lastUpdate.version == version) {
            qCDebug(lspServerLog) << "skipped update of " << uri << "unchanged valid doc";
            return;
        }
        if (snapshot.validDocVersion
            && (!lastUpdate.version || *snapshot.validDocVersion > *lastUpdate.version)) {
            doc = snapshot.validDoc;
            version = snapshot.validDocVersion;
        } else if (snapshot.docVersion
                   && (!lastUpdate.version || *snapshot.docVersion > *lastUpdate.version)) {
            if (!lastUpdate.version || !snapshot.validDocVersion
                || (lastUpdate.invalidUpdatesSince
                    && lastUpdate.invalidUpdatesSince->msecsTo(QDateTime::currentDateTime())
                            > maxInvalidMsec)) {
                doc = snapshot.doc;
                version = snapshot.docVersion;
            } else if (!lastUpdate.invalidUpdatesSince) {
                lastUpdate.invalidUpdatesSince = QDateTime::currentDateTime();
                QTimer::singleShot(maxInvalidMsec, Qt::VeryCoarseTimer, this,
                                   [this, uri]() { diagnose(uri); });
            }
        }
        if (doc) {
            // update immediately, and do not keep track of sent version, thus in extreme cases sent
            // updates could be out of sync
            lastUpdate.version = version;
            lastUpdate.invalidUpdatesSince = {};
        }
    }
    QString fileContents;
    if (doc) {
        qCDebug(lintLog) << "has doc, do real lint";
        QStringList imports;
        imports.append(QLibraryInfo::path(QLibraryInfo::QmlImportsPath));
        // add m_server->clientInfo().rootUri & co?
        bool useAbsolutePath = false;
        bool silent = true;
        QString filename = doc.canonicalFilePath();
        fileContents = doc.field(Fields::code).value().toString();
        QJsonArray json;
        QStringList qmltypesFiles;
        QStringList resourceFiles;
        QMap<QString, QQmlJSLogger::Option> options;

        QQmlLinter linter(imports, useAbsolutePath);

        linter.lintFile(filename, &fileContents, silent, &json, imports, qmltypesFiles,
                        resourceFiles, options);
        auto addLength = [&fileContents](Position &position, int startOffset, int length) {
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

        auto jsonToDiagnostic = [&addLength](const QJsonValue &message) {
            Diagnostic diagnostic;
            diagnostic.severity = severityFromString(message[u"type"].toString());
            Range &range = diagnostic.range;
            Position &position = range.start;
            position.line = message[u"line"].toInt(1) - 1;
            position.character = message[u"column"].toInt(1) - 1;
            range.end = position;
            addLength(range.end, message[u"charOffset"].toInt(), message[u"length"].toInt());
            diagnostic.message = message[u"message"].toString().toUtf8();
            diagnostic.source = QByteArray("qmllint");
            return diagnostic;
        };
        doc.iterateErrors(
                [&diagnostics, &addLength](DomItem, ErrorMessage msg) {
                    Diagnostic diagnostic;
                    diagnostic.severity = severityFromMsgType(QtMsgType(int(msg.level)));
                    // do something with msg.errorGroups ?
                    auto &location = msg.location;
                    Range &range = diagnostic.range;
                    range.start.line = location.startLine - 1;
                    range.start.character = location.startColumn - 1;
                    range.end = range.start;
                    addLength(range.end, location.offset, location.length);
                    diagnostic.code = QByteArray(msg.errorId.data(), msg.errorId.size());
                    diagnostic.source = "domParsing";
                    diagnostic.message = msg.message.toUtf8();
                    diagnostics.append(diagnostic);
                    return true;
                },
                true);
        for (const auto &results : qAsConst(json)) {
            if (results[u"filename"].toString() == filename) {
                for (const auto &message : results[u"warnings"].toArray())
                    diagnostics.append(jsonToDiagnostic(message));
            }
        }
    }
    PublishDiagnosticsParams diagnosticParams;
    diagnosticParams.uri = uri;
    diagnosticParams.diagnostics = diagnostics;
    diagnosticParams.version = version;

    m_server->protocol()->notifyPublishDiagnostics(diagnosticParams);
    qCDebug(lintLog) << "lint" << QString::fromUtf8(uri) << "found"
                     << diagnosticParams.diagnostics.size() << "issues"
                     << QTypedJson::toJsonValue(diagnosticParams);
}

} // namespace QmlLsp
QT_END_NAMESPACE
