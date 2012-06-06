
#include "qv4isel_llvm_p.h"
#include "qv4ir_p.h"

#include <llvm/Support/system_error.h>
#include <llvm/Support/MemoryBuffer.h>
#include <llvm/Bitcode/ReaderWriter.h>
#include <llvm/Linker.h>
#include <cstdio>

using namespace QQmlJS;

LLVMInstructionSelection::LLVMInstructionSelection(llvm::LLVMContext &context)
    : llvm::IRBuilder<>(context)
    , _llvmModule(0)
    , _llvmFunction(0)
    , _llvmValue(0)
    , _numberTy(0)
    , _valueTy(0)
    , _contextPtrTy(0)
    , _stringPtrTy(0)
    , _functionTy(0)
    , _allocaInsertPoint(0)
    , _function(0)
    , _block(0)
{
}

llvm::Module *LLVMInstructionSelection::getLLVMModule(IR::Module *module)
{
    llvm::Module *llvmModule = new llvm::Module("a.out", getContext());
    qSwap(_llvmModule, llvmModule);

    _numberTy = getDoubleTy();

    std::string err;

    llvm::OwningPtr<llvm::MemoryBuffer> buffer;
    llvm::error_code ec = llvm::MemoryBuffer::getFile(llvm::StringRef("llvm_runtime.bc"), buffer);
    if (ec) {
        qWarning() << ec.message().c_str();
        assert(!"cannot load QML/JS LLVM runtime, you can generate the runtime with the command `make llvm_runtime'");
    }

    llvm::Module *llvmRuntime = llvm::getLazyBitcodeModule(buffer.get(), getContext(), &err);
    if (! err.empty()) {
        qWarning() << err.c_str();
        assert(!"cannot load QML/JS LLVM runtime");
    }

    err.clear();
    llvm::Linker::LinkModules(_llvmModule, llvmRuntime, llvm::Linker::DestroySource, &err);
    if (! err.empty()) {
        qWarning() << err.c_str();
        assert(!"cannot link the QML/JS LLVM runtime");
    }

    _valueTy = _llvmModule->getTypeByName("struct.QQmlJS::VM::Value");
    _contextPtrTy = _llvmModule->getTypeByName("struct.QQmlJS::VM::Context")->getPointerTo();
    _stringPtrTy = _llvmModule->getTypeByName("struct.QQmlJS::VM::String")->getPointerTo();

    {
        llvm::Type *args[] = { _contextPtrTy };
        _functionTy = llvm::FunctionType::get(getVoidTy(), llvm::makeArrayRef(args), false);
    }


    foreach (IR::Function *function, module->functions)
        (void) getLLVMFunction(function);
    qSwap(_llvmModule, llvmModule);
    return llvmModule;
}

llvm::Function *LLVMInstructionSelection::getLLVMFunction(IR::Function *function)
{
    llvm::Function *llvmFunction =
            llvm::Function::Create(_functionTy, llvm::Function::ExternalLinkage, // ### make it internal
                                   llvm::Twine(function->name ? qPrintable(*function->name) : 0), _llvmModule);

    QHash<IR::BasicBlock *, llvm::BasicBlock *> blockMap;
    QVector<llvm::Value *> tempMap;

    qSwap(_llvmFunction, llvmFunction);
    qSwap(_function, function);
    qSwap(_tempMap, tempMap);
    qSwap(_blockMap, blockMap);

    // entry block
    SetInsertPoint(getLLVMBasicBlock(_function->basicBlocks.first()));

    llvm::Instruction *allocaInsertPoint = new llvm::BitCastInst(llvm::UndefValue::get(getInt32Ty()),
                                                                 getInt32Ty(), "", GetInsertBlock());
    qSwap(_allocaInsertPoint, allocaInsertPoint);

    for (int i = 0; i < _function->tempCount; ++i) {
        llvm::AllocaInst *t = newLLVMTemp(_valueTy);
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

    qSwap(_allocaInsertPoint, allocaInsertPoint);

    allocaInsertPoint->eraseFromParent();

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
    llvm::Value *value = 0;
    if (IR::Temp *t = expr->asTemp()) {
        value = getLLVMTemp(t);
    } else {
        value = getLLVMValue(expr);
        if (! value) {
            Q_UNIMPLEMENTED();
            return getInt1(false);
        }

        llvm::Value *tmp = newLLVMTemp(_valueTy);
        CreateStore(value, tmp);
        value = tmp;
    }

    return CreateCall2(_llvmModule->getFunction("__qmljs_llvm_to_boolean"),
                       _llvmFunction->arg_begin(),
                       value);
}

llvm::Value *LLVMInstructionSelection::getLLVMTemp(IR::Temp *temp)
{
    if (temp->index < 0) {
        const int index = -temp->index -1;
        return CreateCall2(_llvmModule->getFunction("__qmljs_llvm_get_argument"),
                           _llvmFunction->arg_begin(), getInt32(index));
    }

    return _tempMap[temp->index];
}

llvm::Value *LLVMInstructionSelection::getStringPtr(const QString &s)
{
    llvm::Value *&value = _stringMap[s];
    if (! value) {
        const QByteArray bytes = s.toUtf8();
        value = CreateGlobalStringPtr(llvm::StringRef(bytes.constData(), bytes.size()));
        _stringMap[s] = value;
    }
    return value;
}

void LLVMInstructionSelection::visitExp(IR::Exp *s)
{
    getLLVMValue(s->expr);
}

void LLVMInstructionSelection::visitEnter(IR::Enter *)
{
    Q_UNREACHABLE();
}

void LLVMInstructionSelection::visitLeave(IR::Leave *)
{
    Q_UNREACHABLE();
}

void LLVMInstructionSelection::visitMove(IR::Move *s)
{
    if (IR::Temp *t = s->target->asTemp()) {
        llvm::Value *target = getLLVMTemp(t);
        llvm::Value *source = getLLVMValue(s->source);
        assert(source);
        if (source->getType()->getPointerTo() != target->getType()) {
            source->dump();
            assert(!"not cool");
        }
        CreateStore(source, target);
        return;
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
                 getLLVMBasicBlock(s->iffalse));
}

void LLVMInstructionSelection::visitRet(IR::Ret *s)
{
    IR::Temp *t = s->expr->asTemp();
    assert(t != 0);
    llvm::Value *result = getLLVMTemp(t);
    llvm::Value *ctx = _llvmFunction->arg_begin();
    CreateCall2(_llvmModule->getFunction("__qmljs_llvm_return"), ctx, result);
    CreateRetVoid();
}


void LLVMInstructionSelection::visitConst(IR::Const *e)
{
    llvm::Value *k = llvm::ConstantFP::get(_numberTy, e->value);
    llvm::Value *tmp = newLLVMTemp(_valueTy);
    CreateCall2(_llvmModule->getFunction("__qmljs_llvm_init_number"), tmp, k);
    _llvmValue = CreateLoad(tmp);
}

void LLVMInstructionSelection::visitString(IR::String *e)
{
    llvm::Value *tmp = newLLVMTemp(_valueTy);
    CreateCall3(_llvmModule->getFunction("__qmljs_llvm_init_string"),
                _llvmFunction->arg_begin(), tmp,
                getStringPtr(*e->value));
    _llvmValue = CreateLoad(tmp);
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

void LLVMInstructionSelection::visitUnop(IR::Unop *e)
{
    llvm::Value *result = newLLVMTemp(_valueTy);
    genUnop(result, e);
    _llvmValue = CreateLoad(result);
}

void LLVMInstructionSelection::visitBinop(IR::Binop *e)
{
    llvm::Value *result = newLLVMTemp(_valueTy);
    genBinop(result, e);
    _llvmValue = CreateLoad(result);
}

void LLVMInstructionSelection::genUnop(llvm::Value *result, IR::Unop *e)
{
    IR::Temp *t = e->expr->asTemp();
    assert(t != 0);

    llvm::Value *expr = getLLVMTemp(t);
    llvm::Value *op = 0;

    switch (e->op) {
    default:
        Q_UNREACHABLE();
        break;

    case IR::OpNot: _llvmModule->getFunction("__qmljs_llvm_not"); break;
    case IR::OpUMinus: _llvmModule->getFunction("__qmljs_llvm_uminus"); break;
    case IR::OpUPlus: _llvmModule->getFunction("__qmljs_llvm_uplus"); break;
    case IR::OpCompl: _llvmModule->getFunction("__qmljs_llvm_compl"); break;
    }

    CreateCall3(op, _llvmFunction->arg_begin(), result, expr);
}

void LLVMInstructionSelection::genBinop(llvm::Value *result, IR::Binop *e)
{
    IR::Temp *t1 = e->left->asTemp();
    IR::Temp *t2 = e->right->asTemp();
    assert(t1 != 0);
    assert(t2 != 0);

    llvm::Value *left = getLLVMTemp(t1);
    llvm::Value *right = getLLVMTemp(t2);
    llvm::Value *op = 0;
    switch (e->op) {
    case IR::OpInvalid:
    case IR::OpIfTrue:
    case IR::OpNot:
    case IR::OpUMinus:
    case IR::OpUPlus:
    case IR::OpCompl:
        Q_UNREACHABLE();
        break;

    case IR::OpBitAnd: op = _llvmModule->getFunction("__qmljs_llvm_bit_and"); break;
    case IR::OpBitOr: op = _llvmModule->getFunction("__qmljs_llvm_bit_or"); break;
    case IR::OpBitXor: op = _llvmModule->getFunction("__qmljs_llvm_bit_xor"); break;
    case IR::OpAdd: op = _llvmModule->getFunction("__qmljs_llvm_add"); break;
    case IR::OpSub: op = _llvmModule->getFunction("__qmljs_llvm_sub"); break;
    case IR::OpMul: op = _llvmModule->getFunction("__qmljs_llvm_mul"); break;
    case IR::OpDiv: op = _llvmModule->getFunction("__qmljs_llvm_div"); break;
    case IR::OpMod: op = _llvmModule->getFunction("__qmljs_llvm_mod"); break;
    case IR::OpLShift: op = _llvmModule->getFunction("__qmljs_llvm_shl"); break;
    case IR::OpRShift: op = _llvmModule->getFunction("__qmljs_llvm_shr"); break;
    case IR::OpURShift: op = _llvmModule->getFunction("__qmljs_llvm_ushr"); break;
    case IR::OpGt: op = _llvmModule->getFunction("__qmljs_llvm_gt"); break;
    case IR::OpLt: op = _llvmModule->getFunction("__qmljs_llvm_lt"); break;
    case IR::OpGe: op = _llvmModule->getFunction("__qmljs_llvm_ge"); break;
    case IR::OpLe: op = _llvmModule->getFunction("__qmljs_llvm_le"); break;
    case IR::OpEqual: op = _llvmModule->getFunction("__qmljs_llvm_eq"); break;
    case IR::OpNotEqual: op = _llvmModule->getFunction("__qmljs_llvm_ne"); break;
    case IR::OpStrictEqual: op = _llvmModule->getFunction("__qmljs_llvm_se"); break;
    case IR::OpStrictNotEqual: op = _llvmModule->getFunction("__qmljs_llvm_sne"); break;
    case IR::OpInstanceof: op = _llvmModule->getFunction("__qmljs_llvm_instanceof"); break;
    case IR::OpIn: op = _llvmModule->getFunction("__qmljs_llvm_in"); break;

    case IR::OpAnd:
    case IR::OpOr:
        Q_UNREACHABLE();
        break;
    }

    CreateCall4(op, _llvmFunction->arg_begin(), result, left, right);
}

llvm::AllocaInst *LLVMInstructionSelection::newLLVMTemp(llvm::Type *type, llvm::Value *size)
{
    llvm::AllocaInst *addr = new llvm::AllocaInst(type, size, llvm::Twine(), _allocaInsertPoint);
    return addr;
}

void LLVMInstructionSelection::visitCall(IR::Call *e)
{
    llvm::Value *func = 0;
    llvm::Value *base = 0;
    if (IR::Temp *t = e->base->asTemp()) {
        base = getLLVMTemp(t);
        func = _llvmModule->getFunction("__qmljs_llvm_call_value");
    } else if (IR::Name *n = e->base->asName()) {
        if (n->id) {
            llvm::Value *str = getStringPtr(*n->id);
            base = CreateCall2(_llvmModule->getFunction("__qmljs_llvm_get_identifier"),
                               _llvmFunction->arg_begin(), str);
            func = _llvmModule->getFunction("__qmljs_llvm_call_activation_property");
        }
    }

    int argc = 0;
    for (IR::ExprList *it = e->args; it; it = it->next) {
        ++argc;
    }

    llvm::Value *args = 0;
    if (argc)
        args = newLLVMTemp(_valueTy, getInt32(argc));
    else
        args = llvm::Constant::getNullValue(_valueTy->getPointerTo());

    int i = 0;
    for (IR::ExprList *it = e->args; it; it = it->next) {
        llvm::Value *arg = getLLVMValue(it->expr);
        CreateStore(arg, CreateConstGEP1_32(args, i++));
    }

    if (func) {
        llvm::Value *result = newLLVMTemp(_valueTy);
        CreateStore(llvm::Constant::getNullValue(_valueTy), result);
        CreateCall5(func, _llvmFunction->arg_begin(), result, base, args, getInt32(argc));
        _llvmValue = CreateLoad(result);
    } else {
        Q_UNIMPLEMENTED();
    }
}

void LLVMInstructionSelection::visitNew(IR::New *e)
{
    llvm::Value *base = getLLVMValue(e->base);
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

