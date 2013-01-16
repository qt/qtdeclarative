/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
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
****************************************************************************/

#ifndef QV4ISEL_P_H
#define QV4ISEL_P_H

#include "qv4ir_p.h"

#include <qglobal.h>
#include <QHash>

namespace QQmlJS {

namespace VM {
struct ExecutionEngine;
struct Function;
} // namespace VM

class EvalInstructionSelection
{
public:
    EvalInstructionSelection(VM::ExecutionEngine *engine, IR::Module *module);
    virtual ~EvalInstructionSelection() = 0;

    VM::Function *vmFunction(IR::Function *f);

protected:
    VM::Function *createFunctionMapping(VM::ExecutionEngine *engine, IR::Function *irFunction);
    VM::ExecutionEngine *engine() const { return _engine; }
    virtual void run(VM::Function *vmFunction, IR::Function *function) = 0;

private:
    VM::ExecutionEngine *_engine;
    QHash<IR::Function *, VM::Function *> _irToVM;
};

class EvalISelFactory
{
public:
    virtual ~EvalISelFactory() = 0;
    virtual EvalInstructionSelection *create(VM::ExecutionEngine *engine, IR::Module *module) = 0;
};

namespace IR {
class InstructionSelection: protected IR::StmtVisitor
{
public:
    virtual ~InstructionSelection() = 0;

public: // visitor methods for StmtVisitor:
    virtual void visitMove(IR::Move *s);
    virtual void visitEnter(IR::Enter *);
    virtual void visitLeave(IR::Leave *);
    virtual void visitExp(IR::Exp *s);

public: // to implement by subclasses:
    virtual void callBuiltinInvalid(IR::Name *func, IR::ExprList *args, IR::Temp *result) = 0;
    virtual void callBuiltinTypeofMember(IR::Temp *base, const QString &name, IR::Temp *result) = 0;
    virtual void callBuiltinTypeofSubscript(IR::Temp *base, IR::Temp *index, IR::Temp *result) = 0;
    virtual void callBuiltinTypeofName(const QString &name, IR::Temp *result) = 0;
    virtual void callBuiltinTypeofValue(IR::Temp *value, IR::Temp *result) = 0;
    virtual void callBuiltinDeleteMember(IR::Temp *base, const QString &name, IR::Temp *result) = 0;
    virtual void callBuiltinDeleteSubscript(IR::Temp *base, IR::Temp *index, IR::Temp *result) = 0;
    virtual void callBuiltinDeleteName(const QString &name, IR::Temp *result) = 0;
    virtual void callBuiltinDeleteValue(IR::Temp *result) = 0;
    virtual void callBuiltinThrow(IR::Temp *arg) = 0;
    virtual void callBuiltinRethrow() = 0;
    virtual void callBuiltinCreateExceptionHandler(IR::Temp *result) = 0;
    virtual void callBuiltinDeleteExceptionHandler() = 0;
    virtual void callBuiltinGetException(IR::Temp *result) = 0;
    virtual void callBuiltinForeachIteratorObject(IR::Temp *arg, IR::Temp *result) = 0;
    virtual void callBuiltinForeachNextPropertyname(IR::Temp *arg, IR::Temp *result) = 0;
    virtual void callBuiltinPushWith(IR::Temp *arg) = 0;
    virtual void callBuiltinPopWith() = 0;
    virtual void callBuiltinDeclareVar(bool deletable, const QString &name) = 0;
    virtual void callBuiltinDefineGetterSetter(IR::Temp *object, const QString &name, IR::Temp *getter, IR::Temp *setter) = 0;
    virtual void callBuiltinDefineProperty(IR::Temp *object, const QString &name, IR::Temp *value) = 0;
    virtual void callValue(IR::Call *c, IR::Temp *temp) = 0;
    virtual void callProperty(IR::Call *c, IR::Temp *temp) = 0;
    virtual void constructActivationProperty(IR::New *call, IR::Temp *result) = 0;
    virtual void constructProperty(IR::New *ctor, IR::Temp *result) = 0;
    virtual void constructValue(IR::New *call, IR::Temp *result) = 0;
    virtual void loadThisObject(IR::Temp *temp) = 0;
    virtual void loadConst(IR::Const *sourceConst, IR::Temp *targetTemp) = 0;
    virtual void loadString(const QString &str, IR::Temp *targetTemp) = 0;
    virtual void loadRegexp(IR::RegExp *sourceRegexp, IR::Temp *targetTemp) = 0;
    virtual void getActivationProperty(const QString &name, IR::Temp *temp) = 0;
    virtual void setActivationProperty(IR::Expr *source, const QString &targetName) = 0;
    virtual void initClosure(IR::Closure *closure, IR::Temp *target) = 0;
    virtual void getProperty(IR::Temp *base, const QString &name, IR::Temp *target) = 0;
    virtual void setProperty(IR::Expr *source, IR::Temp *targetBase, const QString &targetName) = 0;
    virtual void getElement(IR::Temp *base, IR::Temp *index, IR::Temp *target) = 0;
    virtual void setElement(IR::Expr *source, IR::Temp *targetBase, IR::Temp *targetIndex) = 0;
    virtual void copyValue(IR::Temp *sourceTemp, IR::Temp *targetTemp) = 0;
    virtual void unop(IR::AluOp oper, IR::Temp *sourceTemp, IR::Temp *targetTemp) = 0;
    virtual void binop(IR::AluOp oper, IR::Expr *leftSource, IR::Expr *rightSource, IR::Temp *target) = 0;
    virtual void inplaceNameOp(IR::AluOp oper, IR::Expr *sourceExpr, const QString &targetName) = 0;
    virtual void inplaceElementOp(IR::AluOp oper, IR::Expr *sourceExpr, IR::Temp *targetBaseTemp, IR::Temp *targetIndexTemp) = 0;
    virtual void inplaceMemberOp(IR::AluOp oper, IR::Expr *source, IR::Temp *targetBase, const QString &targetName) = 0;

private:
    void callBuiltin(IR::Call *c, IR::Temp *temp);
};
} // namespace IR

} // namespace QQmlJS

#endif // QV4ISEL_P_H
