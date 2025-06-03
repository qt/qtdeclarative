// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qqmljstypereader_p.h"
#include "qqmljsimportvisitor_p.h"

#include <QtQml/private/qqmljsast_p.h>
#include <QtQml/private/qqmljsengine_p.h>
#include <QtQml/private/qqmljslexer_p.h>
#include <QtQml/private/qqmljsparser_p.h>
#include <QtQml/private/qqmlimportresolver_p.h>

#include <QtCore/qfileinfo.h>
#include <QtCore/qdebug.h>

QT_BEGIN_NAMESPACE

bool QQmlJSTypeReader::operator ()(const QSharedPointer<QQmlJSScope> &scope)
{
    using namespace QQmlJS::AST;
    const QFileInfo info { m_file };
    const QString baseName = info.baseName();
    scope->setInternalName(baseName.endsWith(QStringLiteral(".ui")) ? baseName.chopped(3)
                                                                    : baseName);

    QQmlJS::Engine engine;
    QQmlJS::Lexer lexer(&engine);

    const QString lowerSuffix = info.suffix().toLower();
    const bool isESModule = lowerSuffix == QLatin1String("mjs");
    const bool isJavaScript = isESModule || lowerSuffix == QLatin1String("js");


    QFile file(m_file);
    if (!file.open(QFile::ReadOnly))
        return false;

    QString code = QString::fromUtf8(file.readAll());
    file.close();

    lexer.setCode(code, /*line = */ 1, /*qmlMode=*/ !isJavaScript);
    QQmlJS::Parser parser(&engine);

    const bool success = isJavaScript ? (isESModule ? parser.parseModule()
                                                    : parser.parseProgram())
                                      : parser.parse();
    if (!success)
        return false;

    QQmlJS::AST::Node *rootNode = parser.rootNode();
    if (!rootNode)
        return false;

    QQmlJSLogger logger;
    logger.setFileName(m_file);
    logger.setCode(code);
    logger.setSilent(true);

    auto membersVisitor = m_importer->makeImportVisitor(
            scope, m_importer, &logger,
            QQmlJSImportVisitor::implicitImportDirectory(m_file, m_importer->resourceFileMapper()));
    rootNode->accept(membersVisitor.get());
    return true;
}

QT_END_NAMESPACE
