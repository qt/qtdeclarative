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

#ifndef DEBUGGING_H
#define DEBUGGING_H

#include "qv4global_p.h"
#include "qv4engine_p.h"
#include "qv4context_p.h"

#include <QHash>

QT_BEGIN_NAMESPACE

namespace QQmlJS {

namespace V4IR {
struct BasicBlock;
struct Function;
} // namespace IR

namespace Debugging {

class Debugger;

struct FunctionDebugInfo { // TODO: use opaque d-pointers here
    QString name;
    unsigned startLine, startColumn;

    FunctionDebugInfo(V4IR::Function *function):
        startLine(0), startColumn(0)
    {
        if (function->name)
            name = *function->name;
    }

    void setSourceLocation(unsigned line, unsigned column)
    { startLine = line; startColumn = column; }
};

class FunctionState
{
public:
    FunctionState(QV4::ExecutionContext *context);
    virtual ~FunctionState();

    virtual QV4::Value *argument(unsigned idx);
    virtual QV4::Value *local(unsigned idx);
    virtual QV4::Value *temp(unsigned idx) = 0;

    QV4::ExecutionContext *context() const
    { return _context; }

    Debugger *debugger() const
    { return _context->engine->debugger; }

private:
    QV4::ExecutionContext *_context;
};

struct CallInfo
{
    QV4::ExecutionContext *context;
    QV4::FunctionObject *function;
    FunctionState *state;

    CallInfo(QV4::ExecutionContext *context = 0, QV4::FunctionObject *function = 0, FunctionState *state = 0)
        : context(context)
        , function(function)
        , state(state)
    {}
};

class Q_QML_EXPORT Debugger
{
public:
    Debugger(QV4::ExecutionEngine *_engine);
    ~Debugger();

public: // compile-time interface
    void addFunction(V4IR::Function *function);
    void setSourceLocation(V4IR::Function *function, unsigned line, unsigned column);
    void mapFunction(QV4::Function *vmf, V4IR::Function *irf);

public: // run-time querying interface
    FunctionDebugInfo *debugInfo(QV4::FunctionObject *function) const;
    QString name(QV4::FunctionObject *function) const;

public: // execution hooks
    void aboutToCall(QV4::FunctionObject *function, QV4::ExecutionContext *context);
    void justLeft(QV4::ExecutionContext *context);
    void enterFunction(FunctionState *state);
    void leaveFunction(FunctionState *state);
    void aboutToThrow(const QV4::Value &value);

public: // debugging hooks
    FunctionState *currentState() const;
    const char *currentArg(unsigned idx) const;
    const char *currentLocal(unsigned idx) const;
    const char *currentTemp(unsigned idx) const;
    void printStackTrace() const;

private:
    int callIndex(QV4::ExecutionContext *context);
    V4IR::Function *irFunction(QV4::Function *vmf) const;

private: // TODO: use opaque d-pointers here
    QV4::ExecutionEngine *_engine;
    QHash<V4IR::Function *, FunctionDebugInfo *> _functionInfo;
    QHash<QV4::Function *, V4IR::Function *> _vmToIr;
    QVector<CallInfo> _callStack;
};

} // namespace Debugging
} // namespace QQmlJS

QT_END_NAMESPACE

#endif // DEBUGGING_H
