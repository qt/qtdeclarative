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

#include "qv4ir_p.h"

namespace QQmlJS {

class LLVMInstructionSelection:
        public llvm::IRBuilder<>,
        protected IR::StmtVisitor,
        protected IR::ExprVisitor
{
public:
    LLVMInstructionSelection(llvm::LLVMContext &context);

    void buildLLVMModule(IR::Module *module, llvm::Module *llvmModule, llvm::FunctionPassManager *fpm);
    llvm::Function *getLLVMFunction(IR::Function *function);
    llvm::Function *compileLLVMFunction(IR::Function *function);
    llvm::BasicBlock *getLLVMBasicBlock(IR::BasicBlock *block);
    llvm::Value *getLLVMValue(IR::Expr *expr);
    llvm::Value *getLLVMTempReference(IR::Expr *expr);
    llvm::Value *getLLVMCondition(IR::Expr *expr);
    llvm::Value *getLLVMTemp(IR::Temp *temp);
    llvm::Value *getStringPtr(const QString &s);
    llvm::Value *getIdentifier(const QString &s);
    void genUnop(llvm::Value *result, IR::Unop *e);
    void genBinop(llvm::Value *result, IR::Binop *e);
    llvm::AllocaInst *newLLVMTemp(llvm::Type *type, llvm::Value *size = 0);
    llvm::Value * genArguments(IR::ExprList *args, int &argc);
    void genCallTemp(IR::Call *e, llvm::Value *result = 0);
    void genCallName(IR::Call *e, llvm::Value *result = 0);
    void genCallMember(IR::Call *e, llvm::Value *result = 0);
    void genConstructTemp(IR::New *e, llvm::Value *result = 0);
    void genConstructName(IR::New *e, llvm::Value *result = 0);
    void genConstructMember(IR::New *e, llvm::Value *result = 0);
    void genMoveSubscript(IR::Move *s);
    void genMoveMember(IR::Move *s);

    virtual void visitExp(IR::Exp *);
    virtual void visitEnter(IR::Enter *);
    virtual void visitLeave(IR::Leave *);
    virtual void visitMove(IR::Move *);
    virtual void visitJump(IR::Jump *);
    virtual void visitCJump(IR::CJump *);
    virtual void visitRet(IR::Ret *);

    virtual void visitConst(IR::Const *);
    virtual void visitString(IR::String *);
    virtual void visitRegExp(IR::RegExp *);
    virtual void visitName(IR::Name *);
    virtual void visitTemp(IR::Temp *);
    virtual void visitClosure(IR::Closure *);
    virtual void visitUnop(IR::Unop *);
    virtual void visitBinop(IR::Binop *);
    virtual void visitCall(IR::Call *);
    virtual void visitNew(IR::New *);
    virtual void visitSubscript(IR::Subscript *);
    virtual void visitMember(IR::Member *);

private:
    llvm::Value *createValue(IR::Const *e);
    llvm::Value *toValuePtr(IR::Expr *e);

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

} // end of namespace QQmlJS

#endif // QV4ISEL_LLVM_P_H
