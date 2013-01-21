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

#include "qv4globalobject.h"
#include "qv4ir_p.h"
#include "qv4isel_p.h"
#include "qv4objectproto.h"
#include "qv4stringobject.h"
#include "qv4mm.h"

#include <private/qqmljsengine_p.h>
#include <private/qqmljslexer_p.h>
#include <private/qqmljsparser_p.h>
#include <private/qqmljsast_p.h>
#include <qv4ir_p.h>
#include <qv4codegen_p.h>
#include "private/qlocale_tools_p.h"

#include <QtCore/qmath.h>
#include <QtCore/QDebug>
#include <cassert>
#include <typeinfo>
#include <iostream>
#include <alloca.h>

using namespace QQmlJS::VM;


Value EvalFunction::call(ExecutionContext *context, Value /*thisObject*/, Value *args, int argc)
{
    if (argc < 1)
        return Value::undefinedValue();

    if (!args[0].isString())
        return args[0];

    // ### how to determine this correctly?
    bool directCall = true;

    const QString code = args[0].stringValue()->toQString();
    QQmlJS::VM::Function *f = parseSource(context, QStringLiteral("eval code"), code, QQmlJS::Codegen::EvalCode);
    if (!f)
        return Value::undefinedValue();

    bool strict = f->isStrict || context->strictMode;

    ExecutionContext k, *ctx;
    if (!directCall) {
        qDebug() << "!direct";
        // ###
    } else if (strict) {
        ctx = &k;
        ctx->initCallContext(context, context->thisObject, this, args, argc);
    } else {
        // use the surrounding context
        ctx = context;
    }

    // set the correct strict mode flag on the context
    bool cstrict = ctx->strictMode;
    ctx->strictMode = strict;

    Value result = f->code(ctx, f->codeData);

    ctx->strictMode = cstrict;

    if (strict)
        ctx->leaveCallContext();

    return result;
}

EvalFunction::EvalFunction(ExecutionContext *scope)
    : FunctionObject(scope)
{
    name = scope->engine->newString(QLatin1String("eval"));
}

QQmlJS::VM::Function *EvalFunction::parseSource(QQmlJS::VM::ExecutionContext *ctx,
                                                const QString &fileName, const QString &source,
                                                QQmlJS::Codegen::Mode mode)
{
    using namespace QQmlJS;

    MemoryManager::GCBlocker gcBlocker(ctx->engine->memoryManager);

    VM::ExecutionEngine *vm = ctx->engine;
    IR::Module module;
    VM::Function *globalCode = 0;

    {
        QQmlJS::Engine ee, *engine = &ee;
        Lexer lexer(engine);
        lexer.setCode(source, 1, false);
        Parser parser(engine);

        const bool parsed = parser.parseProgram();

        VM::DiagnosticMessage *error = 0, **errIt = &error;
        foreach (const QQmlJS::DiagnosticMessage &m, parser.diagnosticMessages()) {
            if (m.isError()) {
                *errIt = new VM::DiagnosticMessage;
                (*errIt)->fileName = fileName;
                (*errIt)->offset = m.loc.offset;
                (*errIt)->length = m.loc.length;
                (*errIt)->startLine = m.loc.startLine;
                (*errIt)->startColumn = m.loc.startColumn;
                (*errIt)->type = VM::DiagnosticMessage::Error;
                (*errIt)->message = m.message;
                errIt = &(*errIt)->next;
            } else {
                std::cerr << qPrintable(fileName) << ':' << m.loc.startLine << ':' << m.loc.startColumn
                          << ": warning: " << qPrintable(m.message) << std::endl;
            }
        }
        if (error)
            ctx->throwSyntaxError(error);

        if (parsed) {
            using namespace AST;
            Program *program = AST::cast<Program *>(parser.rootNode());
            if (!program) {
                // if parsing was successful, and we have no program, then
                // we're done...:
                return 0;
            }

            Codegen cg(ctx);
            IR::Function *globalIRCode = cg(fileName, program, &module, mode);
            QScopedPointer<EvalInstructionSelection> isel(ctx->engine->iselFactory->create(vm, &module));
            if (globalIRCode)
                globalCode = isel->vmFunction(globalIRCode);
        }

        if (! globalCode)
            // ### should be a syntax error
            __qmljs_throw_type_error(ctx);
    }

    return globalCode;
}

// parseInt [15.1.2.2]
ParseIntFunction::ParseIntFunction(ExecutionContext *scope)
    : FunctionObject(scope)
{
    name = scope->engine->newString(QLatin1String("parseInt"));
}

static inline int toInt(const QChar &qc, int R)
{
    ushort c = qc.unicode();
    int v = -1;
    if (c >= '0' && c <= '9')
        v = c - '0';
    else if (c >= 'A' && c <= 'Z')
        v = c - 'A' + 10;
    else if (c >= 'a' && c <= 'z')
        v = c - 'a' + 10;
    if (v >= 0 && v < R)
        return v;
    else
        return -1;
}

Value ParseIntFunction::call(ExecutionContext *context, Value thisObject, Value *args, int argc)
{
    Q_UNUSED(thisObject);

    Value string = (argc > 0) ? args[0] : Value::undefinedValue();
    Value radix = (argc > 1) ? args[1] : Value::undefinedValue();
    int R = radix.isUndefined() ? 0 : radix.toInt32(context);

    // [15.1.2.2] step by step:
    String *inputString = string.toString(context); // 1
    QString trimmed = inputString->toQString().trimmed(); // 2
    const QChar *pos = trimmed.constData();
    const QChar *end = pos + trimmed.length();

    int sign = 1; // 3
    if (pos != end) {
        if (*pos == QLatin1Char('-'))
            sign = -1; // 4
        if (*pos == QLatin1Char('-') || *pos == QLatin1Char('+'))
            ++pos; // 5
    }
    bool stripPrefix = true; // 7
    if (R) { // 8
        if (R < 2 || R > 36)
            return Value::fromDouble(nan("")); // 8a
        if (R != 16)
            stripPrefix = false; // 8b
    } else { // 9
        R = 10; // 9a
    }
    if (stripPrefix) { // 10
        if ((end - pos >= 2)
                && (pos[0] == QLatin1Char('0'))
                && (pos[1] == QLatin1Char('x') || pos[1] == QLatin1Char('X'))) { // 10a
            pos += 2;
            R = 16;
        }
    }
    // 11: Z is progressively built below
    // 13: this is handled by the toInt function
    if (pos == end) // 12
        return Value::fromDouble(nan(""));
    bool overflow = false;
    qint64 v_overflow;
    unsigned overflow_digit_count = 0;
    int d = toInt(*pos++, R);
    if (d == -1)
        return Value::fromDouble(nan(""));
    qint64 v = d;
    while (pos != end) {
        d = toInt(*pos++, R);
        if (d == -1)
            break;
        if (overflow) {
            if (overflow_digit_count == 0) {
                v_overflow = v;
                v = 0;
            }
            ++overflow_digit_count;
            v = v * R + d;
        } else {
            qint64 vNew = v * R + d;
            if (vNew < v) {
                overflow = true;
                --pos;
            } else {
                v = vNew;
            }
        }
    }

    if (overflow) {
        double result = (double) v_overflow * pow(R, overflow_digit_count);
        result += v;
        return Value::fromDouble(sign * result);
    } else {
        return Value::fromDouble(sign * (double) v); // 15
    }
}

// parseFloat [15.1.2.3]
ParseFloatFunction::ParseFloatFunction(ExecutionContext *scope)
    : FunctionObject(scope)
{
    name = scope->engine->newString(QLatin1String("parseFloat"));
}

Value ParseFloatFunction::call(ExecutionContext *context, Value thisObject, Value *args, int argc)
{
    Q_UNUSED(context);
    Q_UNUSED(thisObject);

    Value string = (argc > 0) ? args[0] : Value::undefinedValue();

    // [15.1.2.3] step by step:
    String *inputString = string.toString(context); // 1
    QString trimmed = inputString->toQString().trimmed(); // 2

    // 4:
    if (trimmed.startsWith(QLatin1String("Infinity"))
            || trimmed.startsWith(QLatin1String("+Infinity")))
        return Value::fromDouble(INFINITY);
    if (trimmed.startsWith("-Infinity"))
        return Value::fromDouble(-INFINITY);
    QByteArray ba = trimmed.toLatin1();
    bool ok;
    const char *begin = ba.constData();
    const char *end = 0;
    double d = qstrtod(begin, &end, &ok);
    if (end - begin == 0)
        return Value::fromDouble(nan("")); // 3
    else
        return Value::fromDouble(d);
}

/// isNaN [15.1.2.4]
IsNaNFunction::IsNaNFunction(ExecutionContext *scope)
    : FunctionObject(scope)
{
    name = scope->engine->newString(QLatin1String("isNaN"));
}

Value IsNaNFunction::call(ExecutionContext *context, Value /*thisObject*/, Value *args, int argc)
{
    const Value &v = (argc > 0) ? args[0] : Value::undefinedValue();
    if (v.integerCompatible())
        return Value::fromBoolean(false);

    double d = v.toNumber(context);
    return Value::fromBoolean(std::isnan(d));
}

/// isFinite [15.1.2.5]
IsFiniteFunction::IsFiniteFunction(ExecutionContext *scope)
    : FunctionObject(scope)
{
    name = scope->engine->newString(QLatin1String("isFinite"));
}

Value IsFiniteFunction::call(ExecutionContext *context, Value /*thisObject*/, Value *args, int argc)
{
    const Value &v = (argc > 0) ? args[0] : Value::undefinedValue();
    if (v.integerCompatible())
        return Value::fromBoolean(true);

    double d = v.toNumber(context);
    return Value::fromBoolean(std::isfinite(d));
}
