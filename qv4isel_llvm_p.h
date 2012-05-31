#ifndef QV4ISEL_LLVM_P_H
#define QV4ISEL_LLVM_P_H

#include "qv4isel_p.h"
#include "qv4ir_p.h"
#include <llvm/Module.h>
#include <llvm/Support/IRBuilder.h>

namespace QQmlJS {

class LLVMInstructionSelection:
        public llvm::IRBuilder<>,
        protected IR::StmtVisitor,
        protected IR::ExprVisitor
{
public:
    LLVMInstructionSelection(llvm::LLVMContext &context);

    llvm::Module *getLLVMModule(IR::Module *module);
    llvm::Function *getLLVMFunction(IR::Function *function);
    llvm::BasicBlock *getLLVMBasicBlock(IR::BasicBlock *block);
    llvm::Value *getLLVMValue(IR::Expr *expr);
    llvm::Value *getLLVMCondition(IR::Expr *expr);
    llvm::Value *getLLVMTemp(IR::Temp *temp);

    virtual void visitExp(IR::Exp *);
    virtual void visitEnter(IR::Enter *);
    virtual void visitLeave(IR::Leave *);
    virtual void visitMove(IR::Move *);
    virtual void visitJump(IR::Jump *);
    virtual void visitCJump(IR::CJump *);
    virtual void visitRet(IR::Ret *);

    virtual void visitConst(IR::Const *);
    virtual void visitString(IR::String *);
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
    llvm::Module *_llvmModule;
    llvm::Function *_llvmFunction;
    llvm::Value *_llvmValue;
    llvm::Type *_numberTy;
    llvm::Type *_valueTy;
    llvm::Type *_contextTy;
    llvm::FunctionType *_functionTy;
    IR::Function *_function;
    IR::BasicBlock *_block;
    QHash<IR::Function *, llvm::Function *> _functionMap;
    QHash<IR::BasicBlock *, llvm::BasicBlock *> _blockMap;
    QVector<llvm::Value *> _tempMap;
};

} // end of namespace QQmlJS

#endif // QV4ISEL_LLVM_P_H
