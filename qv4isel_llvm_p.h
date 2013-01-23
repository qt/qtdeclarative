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
#ifndef QV4ISEL_LLVM_P_H
#define QV4ISEL_LLVM_P_H

#ifdef __clang__
#  pragma clang diagnostic push
#  pragma clang diagnostic ignored "-Wunused-parameter"
#endif // __clang__

#include <llvm/Module.h>
#include <llvm/PassManager.h>
#include <llvm/IRBuilder.h>

#ifdef __clang__
#  pragma clang diagnostic pop
#endif // __clang__

#include "qv4isel_p.h"
#include "qv4ir_p.h"

namespace QQmlJS {
namespace LLVM {

class InstructionSelection:
        public llvm::IRBuilder<>,
        public IR::InstructionSelection
{
public:
    InstructionSelection(llvm::LLVMContext &context);

    void buildLLVMModule(IR::Module *module, llvm::Module *llvmModule, llvm::FunctionPassManager *fpm);

public: // methods from InstructionSelection:
    virtual void callBuiltinInvalid(IR::Name *func, IR::ExprList *args, IR::Temp *result);
    virtual void callBuiltinTypeofMember(IR::Temp *base, const QString &name, IR::Temp *result);
    virtual void callBuiltinTypeofSubscript(IR::Temp *base, IR::Temp *index, IR::Temp *result);
    virtual void callBuiltinTypeofName(const QString &name, IR::Temp *result);
    virtual void callBuiltinTypeofValue(IR::Temp *value, IR::Temp *result);
    virtual void callBuiltinDeleteMember(IR::Temp *base, const QString &name, IR::Temp *result);
    virtual void callBuiltinDeleteSubscript(IR::Temp *base, IR::Temp *index, IR::Temp *result);
    virtual void callBuiltinDeleteName(const QString &name, IR::Temp *result);
    virtual void callBuiltinDeleteValue(IR::Temp *result);
    virtual void callBuiltinThrow(IR::Temp *arg);
    virtual void callBuiltinRethrow();
    virtual void callBuiltinCreateExceptionHandler(IR::Temp *result);
    virtual void callBuiltinDeleteExceptionHandler();
    virtual void callBuiltinGetException(IR::Temp *result);
    virtual void callBuiltinForeachIteratorObject(IR::Temp *arg, IR::Temp *result);
    virtual void callBuiltinForeachNextPropertyname(IR::Temp *arg, IR::Temp *result);
    virtual void callBuiltinPushWith(IR::Temp *arg);
    virtual void callBuiltinPopWith();
    virtual void callBuiltinDeclareVar(bool deletable, const QString &name);
    virtual void callBuiltinDefineGetterSetter(IR::Temp *object, const QString &name, IR::Temp *getter, IR::Temp *setter);
    virtual void callBuiltinDefineProperty(IR::Temp *object, const QString &name, IR::Temp *value);
    virtual void callValue(IR::Temp *value, IR::ExprList *args, IR::Temp *result);
    virtual void callProperty(IR::Temp *base, const QString &name, IR::ExprList *args, IR::Temp *result);
    virtual void constructActivationProperty(IR::Name *func, IR::ExprList *args, IR::Temp *result);
    virtual void constructProperty(IR::Temp *base, const QString &name, IR::ExprList *args, IR::Temp *result);
    virtual void constructValue(IR::Temp *value, IR::ExprList *args, IR::Temp *result);
    virtual void loadThisObject(IR::Temp *temp);
    virtual void loadConst(IR::Const *con, IR::Temp *temp);
    virtual void loadString(const QString &str, IR::Temp *targetTemp);
    virtual void loadRegexp(IR::RegExp *sourceRegexp, IR::Temp *targetTemp);
    virtual void getActivationProperty(const QString &name, IR::Temp *temp);
    virtual void setActivationProperty(IR::Expr *source, const QString &targetName);
    virtual void initClosure(IR::Closure *closure, IR::Temp *target);
    virtual void getProperty(IR::Temp *sourceBase, const QString &sourceName, IR::Temp *target);
    virtual void setProperty(IR::Expr *source, IR::Temp *targetBase, const QString &targetName);
    virtual void getElement(IR::Temp *sourceBase, IR::Temp *sourceIndex, IR::Temp *target);
    virtual void setElement(IR::Expr *source, IR::Temp *targetBase, IR::Temp *targetIndex);
    virtual void copyValue(IR::Temp *sourceTemp, IR::Temp *targetTemp);
    virtual void unop(IR::AluOp oper, IR::Temp *sourceTemp, IR::Temp *targetTemp);
    virtual void binop(IR::AluOp oper, IR::Expr *leftSource, IR::Expr *rightSource, IR::Temp *target);
    virtual void inplaceNameOp(IR::AluOp oper, IR::Expr *sourceExpr, const QString &targetName);
    virtual void inplaceElementOp(IR::AluOp oper, IR::Expr *sourceExpr, IR::Temp *targetBaseTemp, IR::Temp *targetIndexTemp);
    virtual void inplaceMemberOp(IR::AluOp oper, IR::Expr *source, IR::Temp *targetBase, const QString &targetName);

public: // visitor methods for StmtVisitor:
    virtual void visitJump(IR::Jump *);
    virtual void visitCJump(IR::CJump *);
    virtual void visitRet(IR::Ret *);

private:
    llvm::Function *getRuntimeFunction(llvm::StringRef str);
    llvm::Function *getLLVMFunction(IR::Function *function);
    llvm::Function *compileLLVMFunction(IR::Function *function);
    llvm::BasicBlock *getLLVMBasicBlock(IR::BasicBlock *block);
    llvm::Value *getLLVMTempReference(IR::Expr *expr);
    llvm::Value *getLLVMCondition(IR::Expr *expr);
    llvm::Value *getLLVMTemp(IR::Temp *temp);
    llvm::Value *getStringPtr(const QString &s);
    llvm::Value *getIdentifier(const QString &s);
    llvm::AllocaInst *newLLVMTemp(llvm::Type *type, llvm::Value *size = 0);
    llvm::Value * genArguments(IR::ExprList *args, int &argc);
    void genCallTemp(IR::Call *e, llvm::Value *result = 0);
    void genCallName(IR::Call *e, llvm::Value *result = 0);
    void genCallMember(IR::Call *e, llvm::Value *result = 0);
    void genConstructTemp(IR::New *e, llvm::Value *result = 0);
    void genConstructName(IR::New *e, llvm::Value *result = 0);
    void genConstructMember(IR::New *e, llvm::Value *result = 0);
    llvm::Value *createValue(IR::Const *e);
    llvm::Value *toValuePtr(IR::Expr *e);
    llvm::Value *genStringList(const QList<const QString *> &strings,
                               const char *arrayName, const char *elementName);


private:
    llvm::Module *_llvmModule;
    llvm::Function *_llvmFunction;
    llvm::Value *_llvmValue;
    llvm::Type *_numberTy;
    llvm::Type *_valueTy;
    llvm::Type *_contextPtrTy;
    llvm::Type *_stringPtrTy;
    llvm::FunctionType *_functionTy;
    llvm::Instruction *_allocaInsertPoint;
    IR::Function *_function;
    IR::BasicBlock *_block;
    QHash<IR::Function *, llvm::Function *> _functionMap;
    QHash<IR::BasicBlock *, llvm::BasicBlock *> _blockMap;
    QVector<llvm::Value *> _tempMap;
    QHash<QString, llvm::Value *> _stringMap;
    llvm::FunctionPassManager *_fpm;
};

} // LLVM namespace
} // QQmlJS namespace

#endif // QV4ISEL_LLVM_P_H
