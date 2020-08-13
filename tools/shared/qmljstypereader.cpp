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

#include "qmljstypereader.h"
#include "importedmembersvisitor.h"

#include <QtQml/private/qqmljsast_p.h>
#include <QtQml/private/qqmljsengine_p.h>
#include <QtQml/private/qqmljslexer_p.h>
#include <QtQml/private/qqmljsparser_p.h>
#include <QtQml/private/qqmlimportresolver_p.h>

#include <QtCore/qfileinfo.h>
#include <QtCore/qdebug.h>

static QList<QmlJSTypeReader::Import> parseHeaders(QQmlJS::AST::UiHeaderItemList *header)
{
    using namespace QQmlJS::AST;
    QList<QmlJSTypeReader::Import> imports;

    for (; header; header = header->next) {
        auto import = cast<UiImport *>(header->headerItem);
        if (!import)
            continue;

        QString path;
        auto uri = import->importUri;
        while (uri) {
            path.append(uri->name);
            path.append(u'.');
            uri = uri->next;
        }
        path.chop(1);
        imports.append({
                path,
                import->version ? import->version->version : QTypeRevision(),
                import->asToken.isValid() ? import->importId.toString() : QString()
        });
    }

    return imports;
}

static ScopeTree::Ptr parseProgram(QQmlJS::AST::Program *program, const QString &name)
{
    using namespace QQmlJS::AST;
    ScopeTree::Ptr result = ScopeTree::create(ScopeType::JSLexicalScope, name);
    for (auto *statement = program->statements; statement; statement = statement->next) {
        if (auto *function = cast<FunctionDeclaration *>(statement->statement)) {
            MetaMethod method(function->name.toString());
            method.setMethodType(MetaMethod::Method);
            for (auto *parameters = function->formals; parameters; parameters = parameters->next)
                method.addParameter(parameters->element->bindingIdentifier.toString(), QString());
            result->addMethod(method);
        }
    }
    return result;
}

ScopeTree::Ptr QmlJSTypeReader::operator()()
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

    QFile file(m_file);
    if (!file.open(QFile::ReadOnly)) {
        return ScopeTree::create(isJavaScript ? ScopeType::JSLexicalScope : ScopeType::QMLScope,
                                 scopeName);
    }

    QString code = QString::fromUtf8(file.readAll());
    file.close();

    lexer.setCode(code, /*line = */ 1, /*qmlMode=*/ !isJavaScript);
    QQmlJS::Parser parser(&engine);

    const bool success = isJavaScript ? (isESModule ? parser.parseModule()
                                                    : parser.parseProgram())
                                      : parser.parse();
    if (!success) {
        return ScopeTree::create(isJavaScript ? ScopeType::JSLexicalScope : ScopeType::QMLScope,
                                 scopeName);
    }

    if (!isJavaScript) {
        QQmlJS::AST::UiProgram *program = parser.ast();
        m_imports = parseHeaders(program->headers);
        ImportedMembersVisitor membersVisitor;
        program->members->accept(&membersVisitor);
        m_errors = membersVisitor.errors();
        return membersVisitor.result(scopeName);
    }

    // TODO: Anything special to do with ES modules here?
    return parseProgram(QQmlJS::AST::cast<QQmlJS::AST::Program *>(parser.rootNode()), scopeName);
}
