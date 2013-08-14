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

#ifndef QV4ISEL_P_H
#define QV4ISEL_P_H

#include "private/qv4global_p.h"
#include "qv4jsir_p.h"
#include <private/qv4compileddata_p.h>
#include <private/qv4compiler_p.h>

#include <qglobal.h>
#include <QHash>

QT_BEGIN_NAMESPACE

namespace QV4 {
struct ExecutionEngine;
struct Function;
}

namespace QQmlJS {

class Q_QML_EXPORT EvalInstructionSelection
{
public:
    EvalInstructionSelection(QV4::ExecutionEngine *engine, V4IR::Module *module);
    virtual ~EvalInstructionSelection() = 0;

    QV4::CompiledData::CompilationUnit *compile();

    void setUseFastLookups(bool b) { useFastLookups = b; }

protected:
    QV4::Function *createFunctionMapping(QV4::Function *outer, V4IR::Function *irFunction);
    QV4::ExecutionEngine *engine() const { return _engine; }
    virtual void run(QV4::Function *vmFunction, V4IR::Function *function) = 0;

    int stringId(const QString &str) { return jsUnitGenerator.registerString(str); }

private:
    QV4::ExecutionEngine *_engine;
protected:
    QHash<V4IR::Function *, QV4::Function *> _irToVM;
    bool useFastLookups;
    QV4::Compiler::JSUnitGenerator jsUnitGenerator;
    QV4::CompiledData::CompilationUnit *compilationUnit; // subclass ctor needs to initialize.
};

class Q_QML_EXPORT EvalISelFactory
{
public:
    virtual ~EvalISelFactory() = 0;
    virtual EvalInstructionSelection *create(QV4::ExecutionEngine *engine, V4IR::Module *module) = 0;
    virtual bool jitCompileRegexps() const = 0;
};

namespace V4IR {
class Q_QML_EXPORT IRDecoder: protected V4IR::StmtVisitor
{
public:
    virtual ~IRDecoder() = 0;

    virtual void visitPhi(V4IR::Phi *) {}

public: // visitor methods for StmtVisitor:
    virtual void visitMove(V4IR::Move *s);
    virtual void visitExp(V4IR::Exp *s);

public: // to implement by subclasses:
    virtual void callBuiltinInvalid(V4IR::Name *func, V4IR::ExprList *args, V4IR::Temp *result) = 0;
    virtual void callBuiltinTypeofMember(V4IR::Temp *base, const QString &name, V4IR::Temp *result) = 0;
    virtual void callBuiltinTypeofSubscript(V4IR::Temp *base, V4IR::Temp *index, V4IR::Temp *result) = 0;
    virtual void callBuiltinTypeofName(const QString &name, V4IR::Temp *result) = 0;
    virtual void callBuiltinTypeofValue(V4IR::Temp *value, V4IR::Temp *result) = 0;
    virtual void callBuiltinDeleteMember(V4IR::Temp *base, const QString &name, V4IR::Temp *result) = 0;
    virtual void callBuiltinDeleteSubscript(V4IR::Temp *base, V4IR::Temp *index, V4IR::Temp *result) = 0;
    virtual void callBuiltinDeleteName(const QString &name, V4IR::Temp *result) = 0;
    virtual void callBuiltinDeleteValue(V4IR::Temp *result) = 0;
    virtual void callBuiltinPostDecrementMember(V4IR::Temp *base, const QString &name, V4IR::Temp *result) = 0;
    virtual void callBuiltinPostDecrementSubscript(V4IR::Temp *base, V4IR::Temp *index, V4IR::Temp *result) = 0;
    virtual void callBuiltinPostDecrementName(const QString &name, V4IR::Temp *result) = 0;
    virtual void callBuiltinPostDecrementValue(V4IR::Temp *value, V4IR::Temp *result) = 0;
    virtual void callBuiltinPostIncrementMember(V4IR::Temp *base, const QString &name, V4IR::Temp *result) = 0;
    virtual void callBuiltinPostIncrementSubscript(V4IR::Temp *base, V4IR::Temp *index, V4IR::Temp *result) = 0;
    virtual void callBuiltinPostIncrementName(const QString &name, V4IR::Temp *result) = 0;
    virtual void callBuiltinPostIncrementValue(V4IR::Temp *value, V4IR::Temp *result) = 0;
    virtual void callBuiltinThrow(V4IR::Temp *arg) = 0;
    virtual void callBuiltinFinishTry() = 0;
    virtual void callBuiltinForeachIteratorObject(V4IR::Temp *arg, V4IR::Temp *result) = 0;
    virtual void callBuiltinForeachNextPropertyname(V4IR::Temp *arg, V4IR::Temp *result) = 0;
    virtual void callBuiltinPushWithScope(V4IR::Temp *arg) = 0;
    virtual void callBuiltinPopScope() = 0;
    virtual void callBuiltinDeclareVar(bool deletable, const QString &name) = 0;
    virtual void callBuiltinDefineGetterSetter(V4IR::Temp *object, const QString &name, V4IR::Temp *getter, V4IR::Temp *setter) = 0;
    virtual void callBuiltinDefineProperty(V4IR::Temp *object, const QString &name, V4IR::Temp *value) = 0;
    virtual void callBuiltinDefineArray(V4IR::Temp *result, V4IR::ExprList *args) = 0;
    virtual void callBuiltinDefineObjectLiteral(V4IR::Temp *result, V4IR::ExprList *args) = 0;
    virtual void callValue(V4IR::Temp *value, V4IR::ExprList *args, V4IR::Temp *result) = 0;
    virtual void callProperty(V4IR::Temp *base, const QString &name, V4IR::ExprList *args, V4IR::Temp *result) = 0;
    virtual void callSubscript(V4IR::Temp *base, V4IR::Temp *index, V4IR::ExprList *args, V4IR::Temp *result) = 0;
    virtual void convertType(V4IR::Temp *source, V4IR::Temp *target) = 0;
    virtual void constructActivationProperty(V4IR::Name *func, V4IR::ExprList *args, V4IR::Temp *result) = 0;
    virtual void constructProperty(V4IR::Temp *base, const QString &name, V4IR::ExprList *args, V4IR::Temp *result) = 0;
    virtual void constructValue(V4IR::Temp *value, V4IR::ExprList *args, V4IR::Temp *result) = 0;
    virtual void loadThisObject(V4IR::Temp *temp) = 0;
    virtual void loadConst(V4IR::Const *sourceConst, V4IR::Temp *targetTemp) = 0;
    virtual void loadString(const QString &str, V4IR::Temp *targetTemp) = 0;
    virtual void loadRegexp(V4IR::RegExp *sourceRegexp, V4IR::Temp *targetTemp) = 0;
    virtual void getActivationProperty(const V4IR::Name *name, V4IR::Temp *temp) = 0;
    virtual void setActivationProperty(V4IR::Temp *source, const QString &targetName) = 0;
    virtual void initClosure(V4IR::Closure *closure, V4IR::Temp *target) = 0;
    virtual void getProperty(V4IR::Temp *base, const QString &name, V4IR::Temp *target) = 0;
    virtual void setProperty(V4IR::Temp *source, V4IR::Temp *targetBase, const QString &targetName) = 0;
    virtual void getElement(V4IR::Temp *base, V4IR::Temp *index, V4IR::Temp *target) = 0;
    virtual void setElement(V4IR::Temp *source, V4IR::Temp *targetBase, V4IR::Temp *targetIndex) = 0;
    virtual void copyValue(V4IR::Temp *sourceTemp, V4IR::Temp *targetTemp) = 0;
    virtual void unop(V4IR::AluOp oper, V4IR::Temp *sourceTemp, V4IR::Temp *targetTemp) = 0;
    virtual void binop(V4IR::AluOp oper, V4IR::Expr *leftSource, V4IR::Expr *rightSource, V4IR::Temp *target) = 0;
    virtual void inplaceNameOp(V4IR::AluOp oper, V4IR::Temp *rightSource, const QString &targetName) = 0;
    virtual void inplaceElementOp(V4IR::AluOp oper, V4IR::Temp *source, V4IR::Temp *targetBaseTemp, V4IR::Temp *targetIndexTemp) = 0;
    virtual void inplaceMemberOp(V4IR::AluOp oper, V4IR::Temp *source, V4IR::Temp *targetBase, const QString &targetName) = 0;

private:
    void callBuiltin(V4IR::Call *c, V4IR::Temp *temp);
};
} // namespace IR

} // namespace QQmlJS

QT_END_NAMESPACE

#endif // QV4ISEL_P_H
