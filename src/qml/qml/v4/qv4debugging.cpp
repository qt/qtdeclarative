/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtQml module of the Qt Toolkit.
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

#include "qv4debugging_p.h"
#include "qv4object_p.h"
#include "qv4functionobject_p.h"
#include <iostream>

#define LOW_LEVEL_DEBUGGING_HELPERS

using namespace QQmlJS;
using namespace QQmlJS::Debugging;

FunctionState::FunctionState(QV4::ExecutionContext *context)
    : _context(context)
{
    if (debugger())
        debugger()->enterFunction(this);
}

FunctionState::~FunctionState()
{
    if (debugger())
        debugger()->leaveFunction(this);
}

QV4::Value *FunctionState::argument(unsigned idx)
{
    QV4::CallContext *c = _context->asCallContext();
    if (!c || idx >= c->argumentCount)
        return 0;
    return c->arguments + idx;
}

QV4::Value *FunctionState::local(unsigned idx)
{
    QV4::CallContext *c = _context->asCallContext();
    if (c && idx < c->variableCount())
        return c->locals + idx;
    return 0;
}

#ifdef LOW_LEVEL_DEBUGGING_HELPERS
Debugger *globalInstance = 0;

void printStackTrace()
{
    if (globalInstance)
        globalInstance->printStackTrace();
    else
        std::cerr << "No debugger." << std::endl;
}
#endif // DO_TRACE_INSTR

Debugger::Debugger(QV4::ExecutionEngine *engine)
    : _engine(engine)
{
#ifdef LOW_LEVEL_DEBUGGING_HELPERS
    globalInstance = this;
#endif // DO_TRACE_INSTR
}

Debugger::~Debugger()
{
#ifdef LOW_LEVEL_DEBUGGING_HELPERS
    globalInstance = 0;
#endif // DO_TRACE_INSTR

    qDeleteAll(_functionInfo.values());
}

void Debugger::addFunction(V4IR::Function *function)
{
    _functionInfo.insert(function, new FunctionDebugInfo(function));
}

void Debugger::setSourceLocation(V4IR::Function *function, unsigned line, unsigned column)
{
    _functionInfo[function]->setSourceLocation(line, column);
}

void Debugger::mapFunction(QV4::Function *vmf, V4IR::Function *irf)
{
    _vmToIr.insert(vmf, irf);
}

FunctionDebugInfo *Debugger::debugInfo(QV4::FunctionObject *function) const
{
    if (!function)
        return 0;

    if (function->function)
        return _functionInfo[irFunction(function->function)];
    else
        return 0;
}

QString Debugger::name(QV4::FunctionObject *function) const
{
    if (FunctionDebugInfo *i = debugInfo(function))
        return i->name;

    return QString();
}

void Debugger::aboutToCall(QV4::FunctionObject *function, QV4::ExecutionContext *context)
{
    _callStack.append(CallInfo(context, function));
}

void Debugger::justLeft(QV4::ExecutionContext *context)
{
    int idx = callIndex(context);
    if (idx < 0)
        qDebug() << "Oops, leaving a function that was not registered...?";
    else
        _callStack.resize(idx);
}

void Debugger::enterFunction(FunctionState *state)
{
    _callStack[callIndex(state->context())].state = state;

#ifdef DO_TRACE_INSTR
    QString n = name(_callStack[callIndex(state->context())].function);
    std::cerr << "*** Entering \"" << qPrintable(n) << "\" with " << state->context()->argumentCount << " args" << std::endl;
//    for (unsigned i = 0; i < state->context()->variableEnvironment->argumentCount; ++i)
//        std::cerr << "        " << i << ": " << currentArg(i) << std::endl;
#endif // DO_TRACE_INSTR
}

void Debugger::leaveFunction(FunctionState *state)
{
    _callStack[callIndex(state->context())].state = 0;
}

void Debugger::aboutToThrow(const QV4::Value &value)
{
    qDebug() << "*** We are about to throw...:" << value.toString(currentState()->context())->toQString();
}

FunctionState *Debugger::currentState() const
{
    if (_callStack.isEmpty())
        return 0;
    else
        return _callStack.last().state;
}

const char *Debugger::currentArg(unsigned idx) const
{
    FunctionState *state = currentState();
    return qPrintable(state->argument(idx)->toString(state->context())->toQString());
}

const char *Debugger::currentLocal(unsigned idx) const
{
    FunctionState *state = currentState();
    return qPrintable(state->local(idx)->toString(state->context())->toQString());
}

const char *Debugger::currentTemp(unsigned idx) const
{
    FunctionState *state = currentState();
    return qPrintable(state->temp(idx)->toString(state->context())->toQString());
}

void Debugger::printStackTrace() const
{
    for (int i = _callStack.size() - 1; i >=0; --i) {
        QString n = name(_callStack[i].function);
        std::cerr << "\tframe #" << i << ": " << qPrintable(n) << std::endl;
    }
}

int Debugger::callIndex(QV4::ExecutionContext *context)
{
    for (int idx = _callStack.size() - 1; idx >= 0; --idx) {
        if (_callStack[idx].context == context)
            return idx;
    }

    return -1;
}

V4IR::Function *Debugger::irFunction(QV4::Function *vmf) const
{
    return _vmToIr[vmf];
}

static void realDumpValue(QV4::Value v, QV4::ExecutionContext *ctx, std::string prefix)
{
    using namespace QV4;
    using namespace std;
    cout << prefix << "tag: " << hex << v.tag << dec << endl << prefix << "\t-> ";
    switch (v.type()) {
    case Value::Undefined_Type: cout << "Undefined" << endl; return;
    case Value::Null_Type: cout << "Null" << endl; return;
    case Value::Boolean_Type: cout << "Boolean"; break;
    case Value::Integer_Type: cout << "Integer"; break;
    case Value::Object_Type: cout << "Object"; break;
    case Value::String_Type: cout << "String"; break;
    default: cout << "UNKNOWN" << endl; return;
    }
    cout << endl;

    if (v.isBoolean()) {
        cout << prefix << "\t-> " << (v.booleanValue() ? "TRUE" : "FALSE") << endl;
        return;
    }

    if (v.isInteger()) {
        cout << prefix << "\t-> " << v.integerValue() << endl;
        return;
    }

    if (v.isDouble()) {
        cout << prefix << "\t-> " << v.doubleValue() << endl;
        return;
    }

    if (v.isString()) {
        // maybe check something on the Managed object?
        cout << prefix << "\t-> @" << hex << v.stringValue() << endl;
        cout << prefix << "\t-> \"" << qPrintable(v.stringValue()->toQString()) << "\"" << endl;
        return;
    }

    Object *o = v.objectValue();
    if (!o)
        return;

    cout << prefix << "\t-> @" << hex << o << endl;
    cout << prefix << "object type: " << o->internalType() << endl << prefix << "\t-> ";
    switch (o->internalType()) {
    case QV4::Managed::Type_Invalid: cout << "Invalid"; break;
    case QV4::Managed::Type_String: cout << "String"; break;
    case QV4::Managed::Type_Object: cout << "Object"; break;
    case QV4::Managed::Type_ArrayObject: cout << "ArrayObject"; break;
    case QV4::Managed::Type_FunctionObject: cout << "FunctionObject"; break;
    case QV4::Managed::Type_BooleanObject: cout << "BooleanObject"; break;
    case QV4::Managed::Type_NumberObject: cout << "NumberObject"; break;
    case QV4::Managed::Type_StringObject: cout << "StringObject"; break;
    case QV4::Managed::Type_DateObject: cout << "DateObject"; break;
    case QV4::Managed::Type_RegExpObject: cout << "RegExpObject"; break;
    case QV4::Managed::Type_ErrorObject: cout << "ErrorObject"; break;
    case QV4::Managed::Type_ArgumentsObject: cout << "ArgumentsObject"; break;
    case QV4::Managed::Type_JSONObject: cout << "JSONObject"; break;
    case QV4::Managed::Type_MathObject: cout << "MathObject"; break;
    case QV4::Managed::Type_ForeachIteratorObject: cout << "ForeachIteratorObject"; break;
    default: cout << "UNKNOWN" << endl; return;
    }
    cout << endl;

    cout << prefix << "properties:" << endl;
    ForEachIteratorObject it(ctx, o);
    for (Value name = it.nextPropertyName(); !name.isNull(); name = it.nextPropertyName()) {
        cout << prefix << "\t\"" << qPrintable(name.stringValue()->toQString()) << "\"" << endl;
        PropertyAttributes attrs;
        Property *d = o->__getOwnProperty__(name.stringValue(), &attrs);
        Value pval = o->getValue(d, attrs);
        cout << prefix << "\tvalue:" << endl;
        realDumpValue(pval, ctx, prefix + "\t");
    }
}

void dumpValue(QV4::Value v, QV4::ExecutionContext *ctx)
{
    realDumpValue(v, ctx, std::string(""));
}
