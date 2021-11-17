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

bool QQmlLinter::lintFile(const QString &filename, const bool silent, QJsonArray *json,
                          const QStringList &qmlImportPaths, const QStringList &qmltypesFiles,
                          const QStringList &resourceFiles,
                          const QMap<QString, QQmlJSLogger::Option> &options)
{
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

    auto addJsonWarning = [&](const QQmlJS::DiagnosticMessage &message) {
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

        warnings << jsonMessage;
    };

    QFile file(filename);
    if (!file.open(QFile::ReadOnly)) {
        if (json) {
            result[u"openFailed"] = true;
            success = false;
        } else if (!silent) {
            qWarning() << "Failed to open file" << filename << file.error();
        }
        return false;
    }

    QString code = QString::fromUtf8(file.readAll());
    file.close();

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

    if (!success && !silent) {
        const auto diagnosticMessages = parser.diagnosticMessages();
        for (const QQmlJS::DiagnosticMessage &m : diagnosticMessages) {
            if (json) {
                addJsonWarning(m);
            } else {
                qWarning().noquote() << QString::fromLatin1("%1:%2 : %3")
                                                .arg(filename)
                                                .arg(m.loc.startLine)
                                                .arg(m.message);
            }
        }
    }

    if (success && !isJavaScript) {
        const auto check = [&](QQmlJSResourceFileMapper *mapper) {
            if (m_importer.importPaths() != qmlImportPaths)
                m_importer.setImportPaths(qmlImportPaths);

            m_importer.setResourceFileMapper(mapper);

            QQmlJSLogger logger(m_useAbsolutePath ? info.absoluteFilePath() : filename, code,
                                silent || json);
            FindWarningVisitor v {
                &m_importer,
                &logger,
                qmltypesFiles,
                engine.comments(),
            };

            for (auto it = options.cbegin(); it != options.cend(); ++it) {
                logger.setCategoryError(it.value().m_category, it.value().m_error);
                logger.setCategoryLevel(it.value().m_category, it.value().m_level);
            }

            parser.rootNode()->accept(&v);
            success = v.check();

            if (logger.hasErrors())
                return;

            QQmlJSTypeInfo typeInfo;
            Codegen codegen { &m_importer, filename, qmltypesFiles, &logger, &typeInfo, code };
            QQmlJSSaveFunction saveFunction = [](const QV4::CompiledData::SaveableUnitPointer &,
                                                 const QQmlJSAotFunctionMap &,
                                                 QString *) { return true; };

            QQmlJSCompileError error;

            QLoggingCategory::setFilterRules(u"qt.qml.compiler=false"_qs);

            CodegenWarningInterface interface(&logger);
            qCompileQmlFile(filename, saveFunction, &codegen, &error, true, &interface);

            success &= !logger.hasWarnings() && !logger.hasErrors();

            if (json) {
                for (const auto &error : logger.errors())
                    addJsonWarning(error);
                for (const auto &warning : logger.warnings())
                    addJsonWarning(warning);
                for (const auto &info : logger.infos())
                    addJsonWarning(info);
            }
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
