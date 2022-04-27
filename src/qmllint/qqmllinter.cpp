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

#include "qqmllinter_p.h"

#include "codegen_p.h"
#include "codegenwarninginterface_p.h"
#include "findwarnings_p.h"

#include <QtQmlCompiler/private/qqmljsimporter_p.h>
#include <QtQmlCompiler/private/qqmljsliteralbindingcheck_p.h>

#include <QtCore/qjsonobject.h>
#include <QtCore/qfileinfo.h>
#include <QtCore/qloggingcategory.h>

#include <QtQml/private/qqmljslexer_p.h>
#include <QtQml/private/qqmljsparser_p.h>
#include <QtQml/private/qqmljsengine_p.h>
#include <QtQml/private/qqmljsastvisitor_p.h>
#include <QtQml/private/qqmljsast_p.h>
#include <QtQml/private/qqmljsdiagnosticmessage_p.h>

QT_BEGIN_NAMESPACE

QQmlLinter::QQmlLinter(const QStringList &importPaths, bool useAbsolutePath)
    : m_useAbsolutePath(useAbsolutePath), m_importer(importPaths, nullptr)
{
}

bool QQmlLinter::lintFile(const QString &filename, const QString *fileContents, const bool silent,
                          QJsonArray *json, const QStringList &qmlImportPaths,
                          const QStringList &qmldirFiles, const QStringList &resourceFiles,
                          const QMap<QString, QQmlJSLogger::Option> &options)
{
    // Make sure that we don't expose an old logger if we return before a new one is created.
    m_logger.reset();

    QJsonArray warnings;
    QJsonObject result;

    bool success = true;

    QScopeGuard jsonOutput([&] {
        if (!json)
            return;

        result[u"filename"_qs] = QFileInfo(filename).absoluteFilePath();
        result[u"warnings"] = warnings;
        result[u"success"] = success;

        json->append(result);
    });

    auto addJsonWarning = [&](const QQmlJS::DiagnosticMessage &message,
                              const std::optional<FixSuggestion> &suggestion = {}) {
        QJsonObject jsonMessage;

        QString type;
        switch (message.type) {
        case QtDebugMsg:
            type = u"debug"_qs;
            break;
        case QtWarningMsg:
            type = u"warning"_qs;
            break;
        case QtCriticalMsg:
            type = u"critical"_qs;
            break;
        case QtFatalMsg:
            type = u"fatal"_qs;
            break;
        case QtInfoMsg:
            type = u"info"_qs;
            break;
        default:
            type = u"unknown"_qs;
            break;
        }

        jsonMessage[u"type"_qs] = type;

        if (message.loc.isValid()) {
            jsonMessage[u"line"_qs] = static_cast<int>(message.loc.startLine);
            jsonMessage[u"column"_qs] = static_cast<int>(message.loc.startColumn);
            jsonMessage[u"charOffset"_qs] = static_cast<int>(message.loc.offset);
            jsonMessage[u"length"_qs] = static_cast<int>(message.loc.length);
        }

        jsonMessage[u"message"_qs] = message.message;

        QJsonArray suggestions;
        if (suggestion.has_value()) {
            for (const auto &fix : suggestion->fixes) {
                QJsonObject jsonFix;
                jsonFix[u"message"] = fix.message;
                jsonFix[u"line"_qs] = static_cast<int>(fix.cutLocation.startLine);
                jsonFix[u"column"_qs] = static_cast<int>(fix.cutLocation.startColumn);
                jsonFix[u"charOffset"_qs] = static_cast<int>(fix.cutLocation.offset);
                jsonFix[u"length"_qs] = static_cast<int>(fix.cutLocation.length);
                jsonFix[u"replacement"_qs] = fix.replacementString;
                suggestions << jsonFix;
            }
        }
        jsonMessage[u"suggestions"] = suggestions;

        warnings << jsonMessage;
    };

    QString code;

    if (fileContents == nullptr) {
        QFile file(filename);
        if (!file.open(QFile::ReadOnly)) {
            if (json) {
                addJsonWarning(QQmlJS::DiagnosticMessage {
                    QStringLiteral("Failed to open file %1: %2").arg(filename, file.errorString()),
                    QtCriticalMsg,
                    QQmlJS::SourceLocation()
                });
                success = false;
            } else if (!silent) {
                qWarning() << "Failed to open file" << filename << file.error();
            }
            return false;
        }

        code = QString::fromUtf8(file.readAll());
        file.close();
    } else {
        code = *fileContents;
    }

    QQmlJS::Engine engine;
    QQmlJS::Lexer lexer(&engine);

    QFileInfo info(filename);
    const QString lowerSuffix = info.suffix().toLower();
    const bool isESModule = lowerSuffix == QLatin1String("mjs");
    const bool isJavaScript = isESModule || lowerSuffix == QLatin1String("js");

    lexer.setCode(code, /*lineno = */ 1, /*qmlMode=*/!isJavaScript);
    QQmlJS::Parser parser(&engine);

    success = isJavaScript ? (isESModule ? parser.parseModule() : parser.parseProgram())
                           : parser.parse();

    if (!success) {
        const auto diagnosticMessages = parser.diagnosticMessages();
        for (const QQmlJS::DiagnosticMessage &m : diagnosticMessages) {
            if (json) {
                addJsonWarning(m);
            } else if (!silent) {
                qWarning().noquote() << QString::fromLatin1("%1:%2:%3: %4")
                                                .arg(filename)
                                                .arg(m.loc.startLine)
                                                .arg(m.loc.startColumn)
                                                .arg(m.message);
            }
        }
    }

    if (success && !isJavaScript) {
        const auto processMessages = [&]() {
            if (json) {
                for (const auto &error : m_logger->errors())
                    addJsonWarning(error, error.fixSuggestion);
                for (const auto &warning : m_logger->warnings())
                    addJsonWarning(warning, warning.fixSuggestion);
                for (const auto &info : m_logger->infos())
                    addJsonWarning(info, info.fixSuggestion);
            }
        };

        const auto check = [&](QQmlJSResourceFileMapper *mapper) {
            if (m_importer.importPaths() != qmlImportPaths)
                m_importer.setImportPaths(qmlImportPaths);

            m_importer.setResourceFileMapper(mapper);

            m_logger.reset(new QQmlJSLogger);
            m_logger->setFileName(m_useAbsolutePath ? info.absoluteFilePath() : filename);
            m_logger->setCode(code);
            m_logger->setSilent(silent || json);
            FindWarningVisitor v {
                QQmlJSScope::create(),
                &m_importer,
                m_logger.get(),
                qmldirFiles,
                engine.comments(),
            };

            for (auto it = options.cbegin(); it != options.cend(); ++it) {
                m_logger->setCategoryError(it.value().m_category, it.value().m_error);
                m_logger->setCategoryLevel(it.value().m_category, it.value().m_level);
            }

            QQmlJSTypeResolver typeResolver(&m_importer);

            // Type resolving is using document parent mode here so that it produces fewer false positives
            // on the "parent" property of QQuickItem. It does produce a few false negatives this way
            // because items can be reparented. Furthermore, even if items are not reparented, the document
            // parent may indeed not be their visual parent. See QTBUG-95530. Eventually, we'll need
            // cleverer logic to deal with this.
            typeResolver.setParentMode(QQmlJSTypeResolver::UseDocumentParent);

            typeResolver.init(&v, parser.rootNode());

            QQmlJSLiteralBindingCheck literalCheck;
            literalCheck.run(&v, &typeResolver);

            success = v.check();

            if (m_logger->hasErrors()) {
                processMessages();
                return;
            }

            QQmlJSTypeInfo typeInfo;

            const QStringList resourcePaths = mapper
                    ? mapper->resourcePaths(QQmlJSResourceFileMapper::localFileFilter(filename))
                    : QStringList();
            const QString resolvedPath = (resourcePaths.size() == 1)
                    ? u':' + resourcePaths.first()
                    : filename;

            Codegen codegen { &m_importer, resolvedPath, qmldirFiles, m_logger.get(), &typeInfo };
            codegen.setTypeResolver(std::move(typeResolver));
            QQmlJSSaveFunction saveFunction = [](const QV4::CompiledData::SaveableUnitPointer &,
                                                 const QQmlJSAotFunctionMap &,
                                                 QString *) { return true; };

            QQmlJSCompileError error;

            QLoggingCategory::setFilterRules(u"qt.qml.compiler=false"_qs);

            CodegenWarningInterface interface(m_logger.get());
            qCompileQmlFile(filename, saveFunction, &codegen, &error, true, &interface,
                            fileContents);

            success &= !m_logger->hasWarnings() && !m_logger->hasErrors();

            processMessages();
        };

        if (resourceFiles.isEmpty()) {
            check(nullptr);
        } else {
            QQmlJSResourceFileMapper mapper(resourceFiles);
            check(&mapper);
        }
    }

    return success;
}

QT_END_NAMESPACE
