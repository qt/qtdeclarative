// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
// Qt-Security score:significant

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

namespace QQmlJS {
void defaultTypeReader(QQmlJSImporter *importer, const QString &filePath,
                       const QSharedPointer<QQmlJSScope> &scope)
{
    using namespace QQmlJS::AST;
    const QFileInfo info{ filePath };
    const QString baseName = info.baseName();
    scope->setInternalName(baseName.endsWith(QStringLiteral(".ui")) ? baseName.chopped(3)
                                                                    : baseName);

    QQmlJS::Engine engine;
    QQmlJS::Lexer lexer(&engine);

    const QString lowerSuffix = info.suffix().toLower();
    const bool isESModule = lowerSuffix == QLatin1String("mjs");
    const bool isJavaScript = isESModule || lowerSuffix == QLatin1String("js");

    QFile file(filePath);
    if (!file.open(QFile::ReadOnly))
        return;

    QString code = QString::fromUtf8(file.readAll());
    file.close();

    lexer.setCode(code, /*line = */ 1, /*qmlMode=*/ !isJavaScript);
    QQmlJS::Parser parser(&engine);

    isJavaScript ? (isESModule ? parser.parseModule() : parser.parseProgram()) : parser.parse();

    QQmlJS::AST::Node *rootNode = parser.rootNode();

    QQmlJSLogger logger;
    logger.setFilePath(filePath);
    logger.setCode(code);
    logger.setSilent(true);
    logger.setIsDisabled(true);

    importer->runImportVisitor(rootNode,
                               { scope,
                                 &logger,
                                 QQmlJSImportVisitor::implicitImportDirectory(
                                         filePath, importer->resourceFileMapper()),
                                 { } });
}
} // namespace QQmlJS

QT_END_NAMESPACE
