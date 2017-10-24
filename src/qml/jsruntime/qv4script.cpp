/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qv4script_p.h"
#include <private/qv4mm_p.h>
#include "qv4functionobject_p.h"
#include "qv4function_p.h"
#include "qv4context_p.h"
#include "qv4debugging_p.h"
#include "qv4profiling_p.h"
#include "qv4scopedvalue_p.h"
#include "qv4jscall_p.h"

#include <private/qqmljsengine_p.h>
#include <private/qqmljslexer_p.h>
#include <private/qqmljsparser_p.h>
#include <private/qqmljsast_p.h>
#include <private/qqmlengine_p.h>
#include <private/qv4profiling_p.h>
#include <qv4runtimecodegen_p.h>

#include <QtCore/QDebug>
#include <QtCore/QString>

using namespace QV4;

Script::Script(ExecutionEngine *v4, QmlContext *qml, CompiledData::CompilationUnit *compilationUnit)
    : line(1), column(0), context(v4->rootContext()), strictMode(false), inheritContext(true), parsed(false)
    , compilationUnit(compilationUnit), vmFunction(0), parseAsBinding(true)
{
    if (qml)
        qmlContext.set(v4, *qml);

    parsed = true;

    vmFunction = compilationUnit ? compilationUnit->linkToEngine(v4) : 0;
}

Script::~Script()
{
}

void Script::parse()
{
    if (parsed)
        return;

    using namespace QV4::Compiler;

    parsed = true;

    ExecutionEngine *v4 = context->engine();
    Scope valueScope(v4);

    Module module(v4->debugger() != 0);

    Engine ee, *engine = &ee;
    Lexer lexer(engine);
    lexer.setCode(sourceCode, line, parseAsBinding);
    Parser parser(engine);

    const bool parsed = parser.parseProgram();

    const auto diagnosticMessages = parser.diagnosticMessages();
    for (const DiagnosticMessage &m : diagnosticMessages) {
        if (m.isError()) {
            valueScope.engine->throwSyntaxError(m.message, sourceFile, m.loc.startLine, m.loc.startColumn);
            return;
        } else {
            qWarning() << sourceFile << ':' << m.loc.startLine << ':' << m.loc.startColumn
                      << ": warning: " << m.message;
        }
    }

    if (parsed) {
        using namespace AST;
        Program *program = AST::cast<Program *>(parser.rootNode());
        if (!program) {
            // if parsing was successful, and we have no program, then
            // we're done...:
            return;
        }

        QV4::Compiler::JSUnitGenerator jsGenerator(&module);
        RuntimeCodegen cg(v4, &jsGenerator, strictMode);
        if (inheritContext)
            cg.setUseFastLookups(false);
        cg.generateFromProgram(sourceFile, sourceCode, program, &module, compilationMode);
        if (v4->hasException)
            return;

        compilationUnit = cg.generateCompilationUnit();
        vmFunction = compilationUnit->linkToEngine(v4);
    }

    if (!vmFunction) {
        // ### FIX file/line number
        ScopedObject error(valueScope, v4->newSyntaxErrorObject(QStringLiteral("Syntax error")));
        v4->throwError(error);
    }
}

ReturnedValue Script::run()
{
    if (!parsed)
        parse();
    if (!vmFunction)
        return Encode::undefined();

    QV4::ExecutionEngine *engine = context->engine();
    QV4::Scope valueScope(engine);

    if (qmlContext.isUndefined()) {
        TemporaryAssignment<Function*> savedGlobalCode(engine->globalCode, vmFunction);

        return vmFunction->call(engine->globalObject, 0, 0, context);
    } else {
        Scoped<QmlContext> qml(valueScope, qmlContext.value());
        return vmFunction->call(0, 0, 0, qml);
    }
}

Function *Script::function()
{
    if (!parsed)
        parse();
    return vmFunction;
}

QQmlRefPointer<QV4::CompiledData::CompilationUnit> Script::precompile(QV4::Compiler::Module *module, Compiler::JSUnitGenerator *unitGenerator,
                                                                      const QUrl &url, const QString &source, QList<QQmlError> *reportedErrors,
                                                                      Directives *directivesCollector)
{
    using namespace QV4::Compiler;
    using namespace QQmlJS::AST;

    Engine ee;
    if (directivesCollector)
        ee.setDirectives(directivesCollector);
    Lexer lexer(&ee);
    lexer.setCode(source, /*line*/1, /*qml mode*/false);
    Parser parser(&ee);

    parser.parseProgram();

    QList<QQmlError> errors;

    const auto diagnosticMessages = parser.diagnosticMessages();
    for (const DiagnosticMessage &m : diagnosticMessages) {
        if (m.isWarning()) {
            qWarning("%s:%d : %s", qPrintable(url.toString()), m.loc.startLine, qPrintable(m.message));
            continue;
        }

        QQmlError error;
        error.setUrl(url);
        error.setDescription(m.message);
        error.setLine(m.loc.startLine);
        error.setColumn(m.loc.startColumn);
        errors << error;
    }

    if (!errors.isEmpty()) {
        if (reportedErrors)
            *reportedErrors << errors;
        return 0;
    }

    Program *program = AST::cast<Program *>(parser.rootNode());
    if (!program) {
        // if parsing was successful, and we have no program, then
        // we're done...:
        return 0;
    }

    Codegen cg(unitGenerator, /*strict mode*/false);
    cg.setUseFastLookups(false);
    cg.generateFromProgram(url.toString(), source, program, module, GlobalCode);
    errors = cg.qmlErrors();
    if (!errors.isEmpty()) {
        if (reportedErrors)
            *reportedErrors << errors;
        return 0;
    }

    return cg.generateCompilationUnit(/*generate unit data*/false);
}

QV4::ReturnedValue Script::evaluate(ExecutionEngine *engine, const QString &script, QmlContext *qmlContext)
{
    QV4::Scope scope(engine);
    QV4::Script qmlScript(engine, qmlContext, script, QString());

    qmlScript.parse();
    QV4::ScopedValue result(scope);
    if (!scope.engine->hasException)
        result = qmlScript.run();
    if (scope.engine->hasException) {
        scope.engine->catchException();
        return Encode::undefined();
    }
    return result->asReturnedValue();
}
