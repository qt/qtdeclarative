/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the V4VM module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qv4script_p.h"
#include "qv4mm_p.h"
#include "qv4functionobject_p.h"
#include "qv4function_p.h"
#include "qv4context_p.h"
#include "qv4debugging_p.h"

#include <private/qqmljsengine_p.h>
#include <private/qqmljslexer_p.h>
#include <private/qqmljsparser_p.h>
#include <private/qqmljsast_p.h>
#include <qv4jsir_p.h>
#include <qv4codegen_p.h>

#include <QtCore/QDebug>
#include <QtCore/QString>

using namespace QV4;

struct FunctionWrapper : FunctionObject
{
    FunctionWrapper(ExecutionContext *scope, Function *f)
        : FunctionObject(scope, scope->engine->id_eval)
    {
        function = f;
        usesArgumentsObject = function->usesArgumentsObject;
        needsActivation = function->needsActivation();
        defineReadonlyProperty(scope->engine->id_length, Value::fromInt32(1));
    }
};


void Script::parse()
{
    using namespace QQmlJS;

    ExecutionEngine *v4 = scope->engine;

    MemoryManager::GCBlocker gcBlocker(v4->memoryManager);

    V4IR::Module module;
    Function *globalCode = 0;

    {
        QQmlJS::Engine ee, *engine = &ee;
        Lexer lexer(engine);
        lexer.setCode(sourceCode, 1, false);
        Parser parser(engine);

        const bool parsed = parser.parseProgram();

        DiagnosticMessage *error = 0, **errIt = &error;
        foreach (const QQmlJS::DiagnosticMessage &m, parser.diagnosticMessages()) {
            if (m.isError()) {
                *errIt = new DiagnosticMessage;
                (*errIt)->fileName = sourceFile;
                (*errIt)->offset = m.loc.offset;
                (*errIt)->length = m.loc.length;
                (*errIt)->startLine = m.loc.startLine;
                (*errIt)->startColumn = m.loc.startColumn;
                (*errIt)->type = DiagnosticMessage::Error;
                (*errIt)->message = m.message;
                errIt = &(*errIt)->next;
            } else {
                qWarning() << sourceFile << ':' << m.loc.startLine << ':' << m.loc.startColumn
                          << ": warning: " << m.message;
            }
        }
        if (error)
            scope->throwSyntaxError(error);

        if (parsed) {
            using namespace AST;
            Program *program = AST::cast<Program *>(parser.rootNode());
            if (!program) {
                // if parsing was successful, and we have no program, then
                // we're done...:
                return;
            }

            QStringList inheritedLocals;
            if (inheritContext)
                for (String * const *i = scope->variables(), * const *ei = i + scope->variableCount(); i < ei; ++i)
                    inheritedLocals.append(*i ? (*i)->toQString() : QString());

            Codegen cg(scope, strictMode);
            V4IR::Function *globalIRCode = cg(sourceFile, sourceCode, program, &module, QQmlJS::Codegen::EvalCode, inheritedLocals);
            QScopedPointer<EvalInstructionSelection> isel(v4->iselFactory->create(v4, &module));
            if (inheritContext)
                isel->setUseFastLookups(false);
            if (globalIRCode)
                globalCode = isel->vmFunction(globalIRCode);
        }

        if (! globalCode)
            // ### should be a syntax error
            scope->throwTypeError();
    }
    if (!globalCode)
        // ### FIX file/line number
        __qmljs_throw(v4->current, QV4::Value::fromObject(v4->newSyntaxErrorObject(v4->current, 0)), -1);

    functionWrapper = Value::fromObject(new (v4->memoryManager) FunctionWrapper(scope, globalCode));
}

Value Script::run()
{
    if (functionWrapper.isEmpty())
        parse();

    QV4::ExecutionEngine *engine = scope->engine;
    QV4::FunctionObject *f = functionWrapper.value().asFunctionObject();

    if (engine->debugger)
        engine->debugger->aboutToCall(0, scope);

    if (!qml) {
        QV4::Function *function = this->function();
        TemporaryAssignment<Function*> savedGlobalCode(engine->globalCode, function);

        bool strict = scope->strictMode;
        Lookup *lookups = scope->lookups;

        scope->strictMode = function->isStrict;
        scope->lookups = function->lookups;

        if (engine->debugger)
            engine->debugger->aboutToCall(0, scope);

        QV4::Value result;
        try {
            result = function->code(scope, function->codeData);
        } catch (Exception &e) {
            scope->strictMode = strict;
            scope->lookups = lookups;
            throw;
        }

        if (engine->debugger)
            engine->debugger->justLeft(scope);
        return result;

    } else {
        ExecutionContext *ctx = engine->newQmlContext(f, qml);

        Value result = f->function->code(ctx, f->function->codeData);

        engine->popContext();

        return result;
    }
}

Function *Script::function()
{
    return functionWrapper.value().asFunctionObject()->function;
}

QV4::Value Script::evaluate(ExecutionEngine *engine,  const QString &script, Object *scopeObject)
{
    QV4::Script qmlScript(engine, scopeObject, script, QString());

    QV4::ExecutionContext *ctx = engine->current;
    QV4::Value result = QV4::Value::undefinedValue();
    try {
        qmlScript.parse();
        result = qmlScript.run();
    } catch (QV4::Exception &e) {
        e.accept(ctx);
    }
    return result;
}
