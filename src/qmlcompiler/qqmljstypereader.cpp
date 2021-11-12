/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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

QQmlJSScope::Ptr QQmlJSTypeReader::operator()()
{
    using namespace QQmlJS::AST;
    const QFileInfo info { m_file };
    QString baseName = info.baseName();
    const QString scopeName = baseName.endsWith(QStringLiteral(".ui")) ? baseName.chopped(3)
                                                                       : baseName;

    QQmlJS::Engine engine;
    QQmlJS::Lexer lexer(&engine);

    const QString lowerSuffix = info.suffix().toLower();
    const bool isESModule = lowerSuffix == QLatin1String("mjs");
    const bool isJavaScript = isESModule || lowerSuffix == QLatin1String("js");

    auto errorResult = [&](){
        QQmlJSScope::Ptr result = QQmlJSScope::create(
                    isJavaScript ? QQmlJSScope::JSLexicalScope : QQmlJSScope::QMLScope);
        result->setInternalName(scopeName);
        return result;
    };

    QFile file(m_file);
    if (!file.open(QFile::ReadOnly))
        return errorResult();

    QString code = QString::fromUtf8(file.readAll());
    file.close();

    lexer.setCode(code, /*line = */ 1, /*qmlMode=*/ !isJavaScript);
    QQmlJS::Parser parser(&engine);

    const bool success = isJavaScript ? (isESModule ? parser.parseModule()
                                                    : parser.parseProgram())
                                      : parser.parse();
    if (!success)
        return errorResult();

    QQmlJS::AST::Node *rootNode = parser.rootNode();
    if (!rootNode)
        return errorResult();

    QQmlJSImportVisitor membersVisitor(
                m_importer,
                QQmlJSImportVisitor::implicitImportDirectory(
                    m_file, m_importer->resourceFileMapper()),
                m_qmltypesFiles, m_file, code);
    rootNode->accept(&membersVisitor);
    auto result = membersVisitor.result();
    Q_ASSERT(result);
    result->setInternalName(scopeName);
    return result;
}

QT_END_NAMESPACE
