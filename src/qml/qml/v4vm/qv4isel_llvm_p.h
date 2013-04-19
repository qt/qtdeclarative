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
#include "qv4jsir_p.h"

namespace QQmlJS {
namespace LLVM {

class InstructionSelection:
        public llvm::IRBuilder<>,
        public V4IR::InstructionSelection
{
public:
    InstructionSelection(llvm::LLVMContext &context);

    void buildLLVMModule(V4IR::Module *module, llvm::Module *llvmModule, llvm::FunctionPassManager *fpm);

public: // methods from InstructionSelection:
    virtual void callBuiltinInvalid(V4IR::Name *func, V4IR::ExprList *args, V4IR::Temp *result);
    virtual void callBuiltinTypeofMember(V4IR::Temp *base, const QString &name, V4IR::Temp *result);
    virtual void callBuiltinTypeofSubscript(V4IR::Temp *base, V4IR::Temp *index, V4IR::Temp *result);
    virtual void callBuiltinTypeofName(const QString &name, V4IR::Temp *result);
    virtual void callBuiltinTypeofValue(V4IR::Temp *value, V4IR::Temp *result);
    virtual void callBuiltinDeleteMember(V4IR::Temp *base, const QString &name, V4IR::Temp *result);
    virtual void callBuiltinDeleteSubscript(V4IR::Temp *base, V4IR::Temp *index, V4IR::Temp *result);
    virtual void callBuiltinDeleteName(const QString &name, V4IR::Temp *result);
    virtual void callBuiltinDeleteValue(V4IR::Temp *result);
    virtual void callBuiltinPostDecrementMember(V4IR::Temp *base, const QString &name, V4IR::Temp *result);
    virtual void callBuiltinPostDecrementSubscript(V4IR::Temp *base, V4IR::Temp *index, V4IR::Temp *result);
    virtual void callBuiltinPostDecrementName(const QString &name, V4IR::Temp *result);
    virtual void callBuiltinPostDecrementValue(V4IR::Temp *value, V4IR::Temp *result);
    virtual void callBuiltinPostIncrementMember(V4IR::Temp *base, const QString &name, V4IR::Temp *result);
    virtual void callBuiltinPostIncrementSubscript(V4IR::Temp *base, V4IR::Temp *index, V4IR::Temp *result);
    virtual void callBuiltinPostIncrementName(const QString &name, V4IR::Temp *result);
    virtual void callBuiltinPostIncrementValue(V4IR::Temp *value, V4IR::Temp *result);
    virtual void callBuiltinThrow(V4IR::Temp *arg);
    virtual void callBuiltinCreateExceptionHandler(V4IR::Temp *result);
    virtual void callBuiltinFinishTry();
    virtual void callBuiltinForeachIteratorObject(V4IR::Temp *arg, V4IR::Temp *result);
    virtual void callBuiltinForeachNextPropertyname(V4IR::Temp *arg, V4IR::Temp *result);
    virtual void callBuiltinPushWithScope(V4IR::Temp *arg);
    virtual void callBuiltinPopScope();
    virtual void callBuiltinDeclareVar(bool deletable, const QString &name);
    virtual void callBuiltinDefineGetterSetter(V4IR::Temp *object, const QString &name, V4IR::Temp *getter, V4IR::Temp *setter);
    virtual void callBuiltinDefineProperty(V4IR::Temp *object, const QString &name, V4IR::Temp *value);
    virtual void callBuiltinDefineArray(V4IR::Temp *result, V4IR::ExprList *args);
    virtual void callValue(V4IR::Temp *value, V4IR::ExprList *args, V4IR::Temp *result);
    virtual void callProperty(V4IR::Temp *base, const QString &name, V4IR::ExprList *args, V4IR::Temp *result);
    virtual void callSubscript(V4IR::Temp *base, V4IR::Temp *index, V4IR::ExprList *args, V4IR::Temp *result);
    virtual void constructActivationProperty(V4IR::Name *func, V4IR::ExprList *args, V4IR::Temp *result);
    virtual void constructProperty(V4IR::Temp *base, const QString &name, V4IR::ExprList *args, V4IR::Temp *result);
    virtual void constructValue(V4IR::Temp *value, V4IR::ExprList *args, V4IR::Temp *result);
    virtual void loadThisObject(V4IR::Temp *temp);
    virtual void loadConst(V4IR::Const *con, V4IR::Temp *temp);
    virtual void loadString(const QString &str, V4IR::Temp *targetTemp);
    virtual void loadRegexp(V4IR::RegExp *sourceRegexp, V4IR::Temp *targetTemp);
    virtual void getActivationProperty(const V4IR::Name *name, V4IR::Temp *temp);
    virtual void setActivationProperty(V4IR::Temp *source, const QString &targetName);
    virtual void initClosure(V4IR::Closure *closure, V4IR::Temp *target);
    virtual void getProperty(V4IR::Temp *sourceBase, const QString &sourceName, V4IR::Temp *target);
    virtual void setProperty(V4IR::Temp *source, V4IR::Temp *targetBase, const QString &targetName);
    virtual void getElement(V4IR::Temp *sourceBase, V4IR::Temp *sourceIndex, V4IR::Temp *target);
    virtual void setElement(V4IR::Temp *source, V4IR::Temp *targetBase, V4IR::Temp *targetIndex);
    virtual void copyValue(V4IR::Temp *sourceTemp, V4IR::Temp *targetTemp);
    virtual void unop(V4IR::AluOp oper, V4IR::Temp *sourceTemp, V4IR::Temp *targetTemp);
    virtual void binop(V4IR::AluOp oper, V4IR::Temp *leftSource, V4IR::Temp *rightSource, V4IR::Temp *target);
    virtual void inplaceNameOp(V4IR::AluOp oper, V4IR::Temp *rightSource, const QString &targetName);
    virtual void inplaceElementOp(V4IR::AluOp oper, V4IR::Temp *source, V4IR::Temp *targetBaseTemp, V4IR::Temp *targetIndexTemp);
    virtual void inplaceMemberOp(V4IR::AluOp oper, V4IR::Temp *source, V4IR::Temp *targetBase, const QString &targetName);

public: // visitor methods for StmtVisitor:
    virtual void visitJump(V4IR::Jump *);
    virtual void visitCJump(V4IR::CJump *);
    virtual void visitRet(V4IR::Ret *);
    virtual void visitTry(V4IR::Try *);

private:
    llvm::Function *getRuntimeFunction(llvm::StringRef str);
    llvm::Function *getLLVMFunction(V4IR::Function *function);
    llvm::Function *compileLLVMFunction(V4IR::Function *function);
    llvm::BasicBlock *getLLVMBasicBlock(V4IR::BasicBlock *block);
    llvm::Value *getLLVMTempReference(V4IR::Expr *expr);
    llvm::Value *getLLVMCondition(V4IR::Expr *expr);
    llvm::Value *getLLVMTemp(V4IR::Temp *temp);
    llvm::Value *getStringPtr(const QString &s);
    llvm::Value *getIdentifier(const QString &s);
    llvm::AllocaInst *newLLVMTemp(llvm::Type *type, llvm::Value *size = 0);
    llvm::Value * genArguments(V4IR::ExprList *args, int &argc);
    void genCallTemp(V4IR::Call *e, llvm::Value *result = 0);
    void genCallName(V4IR::Call *e, llvm::Value *result = 0);
    void genCallMember(V4IR::Call *e, llvm::Value *result = 0);
    void genConstructTemp(V4IR::New *e, llvm::Value *result = 0);
    void genConstructName(V4IR::New *e, llvm::Value *result = 0);
    void genConstructMember(V4IR::New *e, llvm::Value *result = 0);
    llvm::Value *createValue(V4IR::Const *e);
    llvm::Value *toValuePtr(V4IR::Expr *e);
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
    V4IR::Function *_function;
    V4IR::BasicBlock *_block;
    QHash<V4IR::Function *, llvm::Function *> _functionMap;
    QHash<V4IR::BasicBlock *, llvm::BasicBlock *> _blockMap;
    QVector<llvm::Value *> _tempMap;
    QHash<QString, llvm::Value *> _stringMap;
    llvm::FunctionPassManager *_fpm;
};

} // LLVM namespace
} // QQmlJS namespace

#endif // QV4ISEL_LLVM_P_H
