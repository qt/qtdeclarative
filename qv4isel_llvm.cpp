
#include "qv4isel_llvm_p.h"
#include "qv4ir_p.h"

using namespace QQmlJS;

LLVMInstructionSelection::LLVMInstructionSelection(llvm::LLVMContext &context)
    : llvm::IRBuilder<>(context)
    , _llvmModule(0)
    , _llvmFunction(0)
    , _llvmValue(0)
    , _numberTy(0)
    , _valueTy(0)
    , _contextTy(0)
    , _functionTy(0)
    , _function(0)
    , _block(0)
{
    _numberTy = getDoubleTy();

    {
        llvm::StructType *ty = llvm::StructType::create(getContext(), llvm::StringRef("Value"));
        ty->setBody(getInt32Ty(), llvm::StructType::get(_numberTy, NULL), NULL);
        _valueTy = ty;
    }

    {
        // ### we use a pointer for now
        _contextTy = getInt1Ty()->getPointerTo();
    }

    {
        llvm::Type *args[] = { _contextTy };
        _functionTy = llvm::FunctionType::get(getVoidTy(), llvm::makeArrayRef(args), false);
    }
}

llvm::Module *LLVMInstructionSelection::getLLVMModule(IR::Module *module)
{
    llvm::Module *llvmModule = new llvm::Module("a.out", getContext());
    qSwap(_llvmModule, llvmModule);
    foreach (IR::Function *function, module->functions)
        (void) getLLVMFunction(function);
    qSwap(_llvmModule, llvmModule);
    return llvmModule;
}

llvm::Function *LLVMInstructionSelection::getLLVMFunction(IR::Function *function)
{
    llvm::Function *llvmFunction =
            llvm::Function::Create(_functionTy, llvm::Function::InternalLinkage,
                                   llvm::Twine(function->name ? qPrintable(*function->name) : 0), _llvmModule);

    QHash<IR::BasicBlock *, llvm::BasicBlock *> blockMap;
    QVector<llvm::Value *> tempMap;

    qSwap(_llvmFunction, llvmFunction);
    qSwap(_function, function);
    qSwap(_tempMap, tempMap);
    qSwap(_blockMap, blockMap);

    // entry block
    SetInsertPoint(getLLVMBasicBlock(_function->basicBlocks.first()));
    for (int i = 0; i < _function->tempCount; ++i) {
        llvm::AllocaInst *t = CreateAlloca(_valueTy, 0, llvm::StringRef("t"));
        _tempMap.append(t);
    }

    foreach (llvm::Value *t, _tempMap) {
        CreateStore(llvm::Constant::getNullValue(_valueTy), t);
    }

    foreach (IR::BasicBlock *block, _function->basicBlocks) {
        qSwap(_block, block);
        SetInsertPoint(getLLVMBasicBlock(_block));
        foreach (IR::Stmt *s, _block->statements)
            s->accept(this);
        qSwap(_block, block);
    }

    qSwap(_blockMap, blockMap);
    qSwap(_tempMap, tempMap);
    qSwap(_function, function);
    qSwap(_llvmFunction, llvmFunction);
    return llvmFunction;
}

llvm::BasicBlock *LLVMInstructionSelection::getLLVMBasicBlock(IR::BasicBlock *block)
{
    llvm::BasicBlock *&llvmBlock = _blockMap[block];
    if (! llvmBlock)
        llvmBlock = llvm::BasicBlock::Create(getContext(), llvm::Twine(),
                                             _llvmFunction);
    return llvmBlock;
}

llvm::Value *LLVMInstructionSelection::getLLVMValue(IR::Expr *expr)
{
    llvm::Value *llvmValue = 0;
    if (expr) {
        qSwap(_llvmValue, llvmValue);
        expr->accept(this);
        qSwap(_llvmValue, llvmValue);
    }
    if (! llvmValue) {
        Q_UNIMPLEMENTED();
        llvmValue = llvm::Constant::getNullValue(_valueTy);
    }
    return llvmValue;
}

llvm::Value *LLVMInstructionSelection::getLLVMCondition(IR::Expr *expr)
{
    if (llvm::Value *value = getLLVMValue(expr)) {
        if (value->getType() == getInt1Ty()) {
            return value;
        }
    }

    Q_UNIMPLEMENTED();
    return getInt1(false);
}

llvm::Value *LLVMInstructionSelection::getLLVMTemp(IR::Temp *temp)
{
    if (temp->index < 0) {
        // it's an actual argument
        Q_UNIMPLEMENTED();
        return 0;
    }

    return _tempMap[temp->index];
}

void LLVMInstructionSelection::visitExp(IR::Exp *s)
{
    getLLVMValue(s->expr);
}

void LLVMInstructionSelection::visitEnter(IR::Enter *)
{
}

void LLVMInstructionSelection::visitLeave(IR::Leave *)
{
}

void LLVMInstructionSelection::visitMove(IR::Move *s)
{
    if (IR::Temp *t = s->target->asTemp()) {
        if (llvm::Value *target = getLLVMTemp(t)) {
            return;
        }
    }
    Q_UNIMPLEMENTED();
}

void LLVMInstructionSelection::visitJump(IR::Jump *s)
{
    CreateBr(getLLVMBasicBlock(s->target));
}

void LLVMInstructionSelection::visitCJump(IR::CJump *s)
{
    CreateCondBr(getLLVMCondition(s->cond),
                 getLLVMBasicBlock(s->iftrue),
                 getLLVMBasicBlock(_function->basicBlocks.at(_block->index + 1)));
}

void LLVMInstructionSelection::visitRet(IR::Ret *s)
{
    CreateRet(getLLVMValue(s->expr));
}


void LLVMInstructionSelection::visitConst(IR::Const *e)
{
    _llvmValue = llvm::ConstantFP::get(_numberTy, e->value);
}

void LLVMInstructionSelection::visitString(IR::String *)
{
    Q_UNIMPLEMENTED();
}

void LLVMInstructionSelection::visitName(IR::Name *)
{
    Q_UNIMPLEMENTED();
}

void LLVMInstructionSelection::visitTemp(IR::Temp *e)
{
    if (llvm::Value *t = getLLVMTemp(e)) {
        _llvmValue = CreateLoad(t);
    }
}

void LLVMInstructionSelection::visitClosure(IR::Closure *)
{
    Q_UNIMPLEMENTED();
}

void LLVMInstructionSelection::visitUnop(IR::Unop *)
{
    Q_UNIMPLEMENTED();
}

void LLVMInstructionSelection::visitBinop(IR::Binop *e)
{
    Q_UNIMPLEMENTED();
}

void LLVMInstructionSelection::visitCall(IR::Call *)
{
    Q_UNIMPLEMENTED();
}

void LLVMInstructionSelection::visitNew(IR::New *)
{
    Q_UNIMPLEMENTED();
}

void LLVMInstructionSelection::visitSubscript(IR::Subscript *)
{
    Q_UNIMPLEMENTED();
}

void LLVMInstructionSelection::visitMember(IR::Member *)
{
    Q_UNIMPLEMENTED();
}

