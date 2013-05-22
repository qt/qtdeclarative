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


void Script::parse()
{
    using namespace QQmlJS;

    MemoryManager::GCBlocker gcBlocker(scope->engine->memoryManager);

    ExecutionEngine *vm = scope->engine;
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
            QScopedPointer<EvalInstructionSelection> isel(scope->engine->iselFactory->create(vm, &module));
            if (inheritContext)
                isel->setUseFastLookups(false);
            if (globalIRCode)
                globalCode = isel->vmFunction(globalIRCode);
        }

        if (! globalCode)
            // ### should be a syntax error
            scope->throwTypeError();
    }

    function = globalCode;
}

struct QmlFunction : FunctionObject
{
    QmlFunction(ExecutionContext *scope)
        : FunctionObject(scope, scope->engine->id_eval)
    {
        defineReadonlyProperty(scope->engine->id_length, Value::fromInt32(1));
    }
};

Value Script::run()
{
    QV4::ExecutionEngine *engine = scope->engine;

    if (!function)
        // ### FIX file/line number
        __qmljs_throw(engine->current, QV4::Value::fromObject(engine->newSyntaxErrorObject(engine->current, 0)), -1);

    if (!qml) {
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
        QmlFunction *f = new (engine->memoryManager) QmlFunction(scope);
        f->function = function;
        f->usesArgumentsObject = function->usesArgumentsObject;
        f->needsActivation = function->needsActivation();

        CallContext *ctx = engine->newQmlContext(f, qml);

        Value result = function->code(ctx, function->codeData);

        engine->popContext();

        return result;
    }
}

