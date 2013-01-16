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

#ifdef __clang__
#  pragma clang diagnostic push
#  pragma clang diagnostic ignored "-Wunused-parameter"
#endif // __clang__

#include <llvm/Analysis/Passes.h>
#include <llvm/Analysis/Verifier.h>
#include <llvm/Assembly/PrintModulePass.h>
#include <llvm/Bitcode/ReaderWriter.h>
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/ExecutionEngine/JIT.h>
#include <llvm/ExecutionEngine/JITMemoryManager.h>
#include <llvm/Support/FormattedStream.h>
#include <llvm/Support/Host.h>
#include <llvm/Support/MemoryBuffer.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/system_error.h>
#include <llvm/Support/TargetRegistry.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Transforms/Scalar.h>
#include <llvm/Transforms/IPO.h>
#include <llvm/Linker.h>

#ifdef __clang__
#  pragma clang diagnostic pop
#endif // __clang__

#include <QtCore/QFileInfo>
#include <QtCore/QLibrary>
#include <QtCore/QStringList>
#include <QtCore/QTextStream>
#include <cstdio>
#include <iostream>

// These includes have to come last, because WTF/Platform.h defines some macros
// with very unfriendly names that collide with class fields in LLVM.
#include "qv4isel_llvm_p.h"
#include "qv4_llvm_p.h"
#include "qv4ir_p.h"

namespace QQmlJS {

int compileWithLLVM(IR::Module *module, const QString &fileName, LLVMOutputType outputType, int (*exec)(void *))
{
    Q_ASSERT(module);
    Q_ASSERT(exec || outputType != LLVMOutputJit);

    // TODO: should this be done here?
    LLVMInitializeX86TargetInfo();
    LLVMInitializeX86Target();
    LLVMInitializeX86AsmPrinter();
    LLVMInitializeX86AsmParser();
    LLVMInitializeX86Disassembler();
    LLVMInitializeX86TargetMC();

    //----

    llvm::InitializeNativeTarget();
    LLVM::InstructionSelection llvmIsel(llvm::getGlobalContext());

    const QString moduleName = QFileInfo(fileName).fileName();
    llvm::StringRef moduleId(moduleName.toUtf8().constData());
    llvm::Module *llvmModule = new llvm::Module(moduleId, llvmIsel.getContext());

    if (outputType == LLVMOutputJit) {
        // The execution engine takes ownership of the model. No need to delete it anymore.
        std::string errStr;
        llvm::ExecutionEngine *execEngine = llvm::EngineBuilder(llvmModule)
//                .setUseMCJIT(true)
                .setErrorStr(&errStr).create();
        if (!execEngine) {
            std::cerr << "Could not create LLVM JIT: " << errStr << std::endl;
            return EXIT_FAILURE;
        }

        llvm::FunctionPassManager functionPassManager(llvmModule);
        // Set up the optimizer pipeline.  Start with registering info about how the
        // target lays out data structures.
        functionPassManager.add(new llvm::DataLayout(*execEngine->getDataLayout()));
        // Promote allocas to registers.
        functionPassManager.add(llvm::createPromoteMemoryToRegisterPass());
        // Provide basic AliasAnalysis support for GVN.
        functionPassManager.add(llvm::createBasicAliasAnalysisPass());
        // Do simple "peephole" optimizations and bit-twiddling optzns.
        functionPassManager.add(llvm::createInstructionCombiningPass());
        // Reassociate expressions.
        functionPassManager.add(llvm::createReassociatePass());
        // Eliminate Common SubExpressions.
        functionPassManager.add(llvm::createGVNPass());
        // Simplify the control flow graph (deleting unreachable blocks, etc).
        functionPassManager.add(llvm::createCFGSimplificationPass());

        functionPassManager.doInitialization();

        llvmIsel.buildLLVMModule(module, llvmModule, &functionPassManager);

        llvm::Function *entryPoint = llvmModule->getFunction("%entry");
        Q_ASSERT(entryPoint);
        void *funcPtr = execEngine->getPointerToFunction(entryPoint);
        return exec(funcPtr);
    } else {
        llvm::FunctionPassManager functionPassManager(llvmModule);
        // Set up the optimizer pipeline.
        // Promote allocas to registers.
        functionPassManager.add(llvm::createPromoteMemoryToRegisterPass());
        // Provide basic AliasAnalysis support for GVN.
        functionPassManager.add(llvm::createBasicAliasAnalysisPass());
        // Do simple "peephole" optimizations and bit-twiddling optzns.
        functionPassManager.add(llvm::createInstructionCombiningPass());
        // Reassociate expressions.
        functionPassManager.add(llvm::createReassociatePass());
        // Eliminate Common SubExpressions.
        functionPassManager.add(llvm::createGVNPass());
        // Simplify the control flow graph (deleting unreachable blocks, etc).
        functionPassManager.add(llvm::createCFGSimplificationPass());

        functionPassManager.doInitialization();

        llvmIsel.buildLLVMModule(module, llvmModule, &functionPassManager);

        // TODO: if output type is .ll, print the module to file

        const std::string triple = llvm::sys::getDefaultTargetTriple();

        std::string err;
        const llvm::Target *target = llvm::TargetRegistry::lookupTarget(triple, err);
        if (! err.empty()) {
            std::cerr << err << ", triple: " << triple << std::endl;
            assert(!"cannot create target for the host triple");
        }

        std::string cpu;
        std::string features;
        llvm::TargetOptions options;
        llvm::TargetMachine *targetMachine = target->createTargetMachine(triple, cpu, features, options, llvm::Reloc::PIC_);
        assert(targetMachine);

        llvm::TargetMachine::CodeGenFileType ft;
        QString ofName;

        if (outputType == LLVMOutputObject) {
            ft = llvm::TargetMachine::CGFT_ObjectFile;
            ofName = fileName + QLatin1String(".o");
        } else if (outputType == LLVMOutputAssembler) {
            ft = llvm::TargetMachine::CGFT_AssemblyFile;
            ofName = fileName + QLatin1String(".s");
        } else {
            // ft is not used.
            ofName = fileName + QLatin1String(".ll");
        }

        llvm::raw_fd_ostream dest(ofName.toUtf8().constData(), err, llvm::raw_fd_ostream::F_Binary);
        llvm::formatted_raw_ostream destf(dest);
        if (!err.empty()) {
            std::cerr << err << std::endl;
            delete llvmModule;
        }

        llvm::PassManager globalPassManager;
        globalPassManager.add(llvm::createScalarReplAggregatesPass());
        globalPassManager.add(llvm::createInstructionCombiningPass());
        globalPassManager.add(llvm::createGlobalOptimizerPass());
        globalPassManager.add(llvm::createFunctionInliningPass(25));
//        globalPassManager.add(llvm::createFunctionInliningPass(125));

        if (outputType == LLVMOutputObject || outputType == LLVMOutputAssembler) {
            if (targetMachine->addPassesToEmitFile(globalPassManager, destf, ft)) {
                std::cerr << err << " (probably no DataLayout in TargetMachine)" << std::endl;
            } else {
                globalPassManager.run(*llvmModule);

                destf.flush();
                dest.flush();
            }
        } else { // .ll
            globalPassManager.run(*llvmModule);
            llvmModule->print(destf, 0);

            destf.flush();
            dest.flush();
        }

        delete llvmModule;
        return EXIT_SUCCESS;
    }
}

} // QQmlJS

using namespace QQmlJS;
using namespace QQmlJS::LLVM;

namespace {
QTextStream qerr(stderr, QIODevice::WriteOnly);
}

InstructionSelection::InstructionSelection(llvm::LLVMContext &context)
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
    , _fpm(0)
{
}

void InstructionSelection::buildLLVMModule(IR::Module *module, llvm::Module *llvmModule, llvm::FunctionPassManager *fpm)
{
    qSwap(_llvmModule, llvmModule);
    qSwap(_fpm, fpm);

    _numberTy = getDoubleTy();

    std::string err;

    llvm::OwningPtr<llvm::MemoryBuffer> buffer;
    qDebug()<<"llvm runtime:"<<LLVM_RUNTIME;
    llvm::error_code ec = llvm::MemoryBuffer::getFile(llvm::StringRef(LLVM_RUNTIME), buffer);
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
        (void) compileLLVMFunction(function);
    qSwap(_fpm, fpm);
    qSwap(_llvmModule, llvmModule);
}

void InstructionSelection::callActivationProperty(IR::Call *c, IR::Temp *temp)
{
    // TODO: implement instead of visitExp
    Q_UNREACHABLE();
}

void InstructionSelection::callValue(IR::Call *c, IR::Temp *temp)
{
    // TODO: implement instead of visitExp
    Q_UNREACHABLE();
}

void InstructionSelection::callProperty(IR::Call *c, IR::Temp *temp)
{
    // TODO: implement instead of visitExp
    Q_UNREACHABLE();
}

void InstructionSelection::constructActivationProperty(IR::New *call, IR::Temp *result)
{
    assert(!"TODO!");
    Q_UNREACHABLE();
}

void InstructionSelection::constructProperty(IR::New *call, IR::Temp *result)
{
    assert(!"TODO!");
    Q_UNREACHABLE();
}

void InstructionSelection::constructValue(IR::New *call, IR::Temp *result)
{
    assert(!"TODO!");
    Q_UNREACHABLE();
}

void InstructionSelection::loadThisObject(IR::Temp *temp)
{
    assert(!"TODO!");
    Q_UNREACHABLE();
}

void InstructionSelection::loadConst(IR::Const *con, IR::Temp *temp)
{
    assert(!"TODO!");
    Q_UNREACHABLE();
}

void InstructionSelection::loadString(const QString &str, IR::Temp *targetTemp)
{
    assert(!"TODO!");
    Q_UNREACHABLE();
}

void InstructionSelection::loadRegexp(IR::RegExp *sourceRegexp, IR::Temp *targetTemp)
{
    assert(!"TODO!");
    Q_UNREACHABLE();
}

void InstructionSelection::getActivationProperty(const QString &name, IR::Temp *temp)
{
    assert(!"TODO!");
    Q_UNREACHABLE();
}

void InstructionSelection::setActivationProperty(IR::Expr *source, const QString &targetName)
{
    assert(!"TODO!");
    Q_UNREACHABLE();
}

void InstructionSelection::initClosure(IR::Closure *closure, IR::Temp *target)
{
    assert(!"TODO!");
    Q_UNREACHABLE();
}

void InstructionSelection::getProperty(IR::Temp *base, const QString &name, IR::Temp *target)
{
    assert(!"TODO!");
    Q_UNREACHABLE();
}

void InstructionSelection::setProperty(IR::Expr *source, IR::Temp *targetBase, const QString &targetName)
{
    assert(!"TODO!");
    Q_UNREACHABLE();
}

void InstructionSelection::getElement(IR::Temp *base, IR::Temp *index, IR::Temp *target)
{
    assert(!"TODO!");
    Q_UNREACHABLE();
}

void InstructionSelection::setElement(IR::Expr *source, IR::Temp *targetBase, IR::Temp *targetIndex)
{
    assert(!"TODO!");
    Q_UNREACHABLE();
}

void InstructionSelection::copyValue(IR::Temp *sourceTemp, IR::Temp *targetTemp)
{
    assert(!"TODO!");
    Q_UNREACHABLE();
}

void InstructionSelection::unop(IR::AluOp oper, IR::Temp *sourceTemp, IR::Temp *targetTemp)
{
    assert(!"TODO!");
    Q_UNREACHABLE();
}

void InstructionSelection::binop(IR::AluOp oper, IR::Expr *leftSource, IR::Expr *rightSource, IR::Temp *target)
{
    assert(!"TODO!");
    Q_UNREACHABLE();
}

void InstructionSelection::inplaceNameOp(IR::AluOp oper, IR::Expr *sourceExpr, const QString &targetName)
{
    assert(!"TODO!");
    Q_UNREACHABLE();
}

void InstructionSelection::inplaceElementOp(IR::AluOp oper, IR::Expr *sourceExpr, IR::Temp *targetBaseTemp, IR::Temp *targetIndexTemp)
{
    assert(!"TODO!");
    Q_UNREACHABLE();
}

void InstructionSelection::inplaceMemberOp(IR::AluOp oper, IR::Expr *source, IR::Temp *targetBase, const QString &targetName)
{
    assert(!"TODO!");
    Q_UNREACHABLE();
}

llvm::Function *InstructionSelection::getLLVMFunction(IR::Function *function)
{
    llvm::Function *&f = _functionMap[function];
    if (! f) {
        QString name = QStringLiteral("__qmljs_native_");
        if (function->name) {
            if (*function->name == QStringLiteral("%entry"))
                name = *function->name;
            else
                name += *function->name;
        }
        f = llvm::Function::Create(_functionTy, llvm::Function::ExternalLinkage, // ### make it internal
                                   qPrintable(name), _llvmModule);
    }
    return f;
}

llvm::Function *InstructionSelection::compileLLVMFunction(IR::Function *function)
{
    llvm::Function *llvmFunction = getLLVMFunction(function);

    QHash<IR::BasicBlock *, llvm::BasicBlock *> blockMap;
    QVector<llvm::Value *> tempMap;

    qSwap(_llvmFunction, llvmFunction);
    qSwap(_function, function);
    qSwap(_tempMap, tempMap);
    qSwap(_blockMap, blockMap);

    // create the LLVM blocks
    foreach (IR::BasicBlock *block, _function->basicBlocks)
        (void) getLLVMBasicBlock(block);

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

    CreateCall(_llvmModule->getFunction("__qmljs_llvm_init_this_object"),
               _llvmFunction->arg_begin());

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

    // Validate the generated code, checking for consistency.
    llvm::verifyFunction(*llvmFunction);
    // Optimize the function.
    if (_fpm)
        _fpm->run(*llvmFunction);

    return llvmFunction;
}

llvm::BasicBlock *InstructionSelection::getLLVMBasicBlock(IR::BasicBlock *block)
{
    llvm::BasicBlock *&llvmBlock = _blockMap[block];
    if (! llvmBlock)
        llvmBlock = llvm::BasicBlock::Create(getContext(), llvm::Twine(),
                                             _llvmFunction);
    return llvmBlock;
}

llvm::Value *InstructionSelection::getLLVMValue(IR::Expr *expr)
{
    llvm::Value *llvmValue = 0;
    if (expr) {
        qSwap(_llvmValue, llvmValue);
        expr->accept(this);
        qSwap(_llvmValue, llvmValue);
    }
    if (! llvmValue) {
        expr->dump(qerr);qerr<<endl;
        Q_UNIMPLEMENTED();
        llvmValue = llvm::Constant::getNullValue(_valueTy);
    }
    return llvmValue;
}

llvm::Value *InstructionSelection::getLLVMTempReference(IR::Expr *expr)
{
    if (IR::Temp *t = expr->asTemp())
        return getLLVMTemp(t);

    llvm::Value *addr = newLLVMTemp(_valueTy);
    CreateStore(getLLVMValue(expr), addr);
    return addr;
}

llvm::Value *InstructionSelection::getLLVMCondition(IR::Expr *expr)
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

llvm::Value *InstructionSelection::getLLVMTemp(IR::Temp *temp)
{
    if (temp->index < 0) {
        const int index = -temp->index -1;
        return CreateCall2(_llvmModule->getFunction("__qmljs_llvm_get_argument"),
                           _llvmFunction->arg_begin(), getInt32(index));
    }

    return _tempMap[temp->index];
}

llvm::Value *InstructionSelection::getStringPtr(const QString &s)
{
    llvm::Value *&value = _stringMap[s];
    if (! value) {
        const QByteArray bytes = s.toUtf8();
        value = CreateGlobalStringPtr(llvm::StringRef(bytes.constData(), bytes.size()));
        _stringMap[s] = value;
    }
    return value;
}

llvm::Value *InstructionSelection::getIdentifier(const QString &s)
{
    llvm::Value *str = getStringPtr(s);
    llvm::Value *id = CreateCall2(_llvmModule->getFunction("__qmljs_identifier_from_utf8"),
                                  _llvmFunction->arg_begin(), str);
    return id;
}

void InstructionSelection::visitExp(IR::Exp *s)
{
    getLLVMValue(s->expr);
}

void InstructionSelection::genMoveSubscript(IR::Move *s)
{
    IR::Subscript *subscript = s->target->asSubscript();
    llvm::Value *base = getLLVMTempReference(subscript->base);
    llvm::Value *index = getLLVMTempReference(subscript->index);
    llvm::Value *source = toValuePtr(s->source);
    CreateCall4(_llvmModule->getFunction("__qmljs_llvm_set_element"),
                _llvmFunction->arg_begin(), base, index, source);
}

void InstructionSelection::genMoveMember(IR::Move *s)
{
    IR::Member *m = s->target->asMember();
    llvm::Value *base = getLLVMTempReference(m->base);
    llvm::Value *name = getIdentifier(*m->name);
    llvm::Value *source = toValuePtr(s->source);
    CreateCall4(_llvmModule->getFunction("__qmljs_llvm_set_property"),
                _llvmFunction->arg_begin(), base, name, source);
}

void InstructionSelection::visitMove(IR::Move *s)
{
    if (s->op == IR::OpInvalid) {
        if (s->target->asSubscript()) {
            genMoveSubscript(s);
            return;
        } else if (s->target->asMember()) {
            genMoveMember(s);
            return;
        } else if (IR::Name *n = s->target->asName()) {
            llvm::Value *name = getIdentifier(*n->id);
            llvm::Value *source = toValuePtr(s->source);
            CreateCall3(_llvmModule->getFunction("__qmljs_llvm_set_activation_property"),
                        _llvmFunction->arg_begin(), name, source);
            return;
        } else if (IR::Temp *t = s->target->asTemp()) {
            llvm::Value *target = getLLVMTemp(t);
            llvm::Value *source = getLLVMValue(s->source);
            CreateStore(source, target);
            return;
        }
    } else {
        if (IR::Temp *t = s->target->asTemp()) {
            if (s->source->asTemp() || s->source->asConst()) {
                const char *opName = 0;
                switch (s->op) {
                case IR::OpBitAnd: opName = "__qmljs_llvm_bit_and"; break;
                case IR::OpBitOr: opName = "__qmljs_llvm_bit_or"; break;
                case IR::OpBitXor: opName = "__qmljs_llvm_bit_xor"; break;
                case IR::OpAdd: opName = "__qmljs_llvm_add"; break;
                case IR::OpSub: opName = "__qmljs_llvm_sub"; break;
                case IR::OpMul: opName = "__qmljs_llvm_mul"; break;
                case IR::OpDiv: opName = "__qmljs_llvm_div"; break;
                case IR::OpMod: opName = "__qmljs_llvm_mod"; break;
                case IR::OpLShift: opName = "__qmljs_llvm_shl"; break;
                case IR::OpRShift: opName = "__qmljs_llvm_shr"; break;
                case IR::OpURShift: opName = "__qmljs_llvm_ushr"; break;
                default:
                    Q_UNREACHABLE();
                    break;
                }

                if (opName) {
                    llvm::Value *target = getLLVMTemp(t);
                    llvm::Value *s1 = toValuePtr(s->target);
                    llvm::Value *s2 = toValuePtr(s->source);
                    CreateCall4(_llvmModule->getFunction(opName),
                                _llvmFunction->arg_begin(), target, s1, s2);
                    return;
                }
            }
        } else if (IR::Name *n = s->target->asName()) {
            // inplace assignment, e.g. x += 1, ++x, ...
            if (s->source->asTemp() || s->source->asConst()) {
                const char *opName = 0;
                switch (s->op) {
                case IR::OpBitAnd: opName = "__qmljs_llvm_inplace_bit_and_name"; break;
                case IR::OpBitOr: opName = "__qmljs_llvm_inplace_bit_or_name"; break;
                case IR::OpBitXor: opName = "__qmljs_llvm_inplace_bit_xor_name"; break;
                case IR::OpAdd: opName = "__qmljs_llvm_inplace_add_name"; break;
                case IR::OpSub: opName = "__qmljs_llvm_inplace_sub_name"; break;
                case IR::OpMul: opName = "__qmljs_llvm_inplace_mul_name"; break;
                case IR::OpDiv: opName = "__qmljs_llvm_inplace_div_name"; break;
                case IR::OpMod: opName = "__qmljs_llvm_inplace_mod_name"; break;
                case IR::OpLShift: opName = "__qmljs_llvm_inplace_shl_name"; break;
                case IR::OpRShift: opName = "__qmljs_llvm_inplace_shr_name"; break;
                case IR::OpURShift: opName = "__qmljs_llvm_inplace_ushr_name"; break;
                default:
                    Q_UNREACHABLE();
                    break;
                }

                if (opName) {
                    llvm::Value *dst = getIdentifier(*n->id);
                    llvm::Value *src = toValuePtr(s->source);
                    CreateCall3(_llvmModule->getFunction(opName),
                                _llvmFunction->arg_begin(), dst, src);
                    return;
                }
            }
        } else if (IR::Subscript *ss = s->target->asSubscript()) {
            if (s->source->asTemp() || s->source->asConst()) {
                const char *opName = 0;
                switch (s->op) {
                case IR::OpBitAnd: opName = "__qmljs_llvm_inplace_bit_and_element"; break;
                case IR::OpBitOr: opName = "__qmljs_llvm_inplace_bit_or_element"; break;
                case IR::OpBitXor: opName = "__qmljs_llvm_inplace_bit_xor_element"; break;
                case IR::OpAdd: opName = "__qmljs_llvm_inplace_add_element"; break;
                case IR::OpSub: opName = "__qmljs_llvm_inplace_sub_element"; break;
                case IR::OpMul: opName = "__qmljs_llvm_inplace_mul_element"; break;
                case IR::OpDiv: opName = "__qmljs_llvm_inplace_div_element"; break;
                case IR::OpMod: opName = "__qmljs_llvm_inplace_mod_element"; break;
                case IR::OpLShift: opName = "__qmljs_llvm_inplace_shl_element"; break;
                case IR::OpRShift: opName = "__qmljs_llvm_inplace_shr_element"; break;
                case IR::OpURShift: opName = "__qmljs_llvm_inplace_ushr_element"; break;
                default:
                    Q_UNREACHABLE();
                    break;
                }

                if (opName) {
                    llvm::Value *base = getLLVMTemp(ss->base->asTemp());
                    llvm::Value *index = getLLVMTemp(ss->index->asTemp());
                    llvm::Value *value = toValuePtr(s->source);
                    CreateCall4(_llvmModule->getFunction(opName),
                                _llvmFunction->arg_begin(), base, index, value);
                    // TODO: checkExceptions();
                }
                return;
            }
        } else if (IR::Member *m = s->target->asMember()) {
            if (s->source->asTemp() || s->source->asConst()) {
                const char *opName = 0;
                switch (s->op) {
                case IR::OpBitAnd: opName = "__qmljs_llvm_inplace_bit_and_member"; break;
                case IR::OpBitOr: opName = "__qmljs_llvm_inplace_bit_or_member"; break;
                case IR::OpBitXor: opName = "__qmljs_llvm_inplace_bit_xor_member"; break;
                case IR::OpAdd: opName = "__qmljs_llvm_inplace_add_member"; break;
                case IR::OpSub: opName = "__qmljs_llvm_inplace_sub_member"; break;
                case IR::OpMul: opName = "__qmljs_llvm_inplace_mul_member"; break;
                case IR::OpDiv: opName = "__qmljs_llvm_inplace_div_member"; break;
                case IR::OpMod: opName = "__qmljs_llvm_inplace_mod_member"; break;
                case IR::OpLShift: opName = "__qmljs_llvm_inplace_shl_member"; break;
                case IR::OpRShift: opName = "__qmljs_llvm_inplace_shr_member"; break;
                case IR::OpURShift: opName = "__qmljs_llvm_inplace_ushr_member"; break;
                default:
                    Q_UNREACHABLE();
                    break;
                }

                if (opName) {
                    llvm::Value *base = getLLVMTemp(m->base->asTemp());
                    llvm::Value *member = getIdentifier(*m->name);
                    llvm::Value *value = toValuePtr(s->source);
                    CreateCall4(_llvmModule->getFunction(opName),
                                _llvmFunction->arg_begin(), value, base, member);
                    // TODO: checkExceptions();
                }
                return;
            }
        }
    }

    // For anything else:
    s->dump(qerr, IR::Stmt::HIR);
    qerr << endl;
    Q_UNIMPLEMENTED();
    return;
}

void InstructionSelection::visitJump(IR::Jump *s)
{
    CreateBr(getLLVMBasicBlock(s->target));
}

void InstructionSelection::visitCJump(IR::CJump *s)
{
    CreateCondBr(getLLVMCondition(s->cond),
                 getLLVMBasicBlock(s->iftrue),
                 getLLVMBasicBlock(s->iffalse));
}

void InstructionSelection::visitRet(IR::Ret *s)
{
    IR::Temp *t = s->expr->asTemp();
    assert(t != 0);
    llvm::Value *result = getLLVMTemp(t);
    llvm::Value *ctx = _llvmFunction->arg_begin();
    CreateCall2(_llvmModule->getFunction("__qmljs_llvm_return"), ctx, result);
    CreateRetVoid();
}

void InstructionSelection::visitConst(IR::Const *e)
{
    llvm::Value *tmp = createValue(e);

    _llvmValue = CreateLoad(tmp);
}

void InstructionSelection::visitString(IR::String *e)
{
    llvm::Value *tmp = newLLVMTemp(_valueTy);
    CreateCall3(_llvmModule->getFunction("__qmljs_llvm_init_string"),
                _llvmFunction->arg_begin(), tmp,
                getStringPtr(*e->value));
    _llvmValue = CreateLoad(tmp);
}

void InstructionSelection::visitRegExp(IR::RegExp *e)
{
    e->dump(qerr);
    qerr << endl;
    Q_UNIMPLEMENTED();
    _llvmValue = llvm::Constant::getNullValue(_valueTy);
}

void InstructionSelection::visitName(IR::Name *e)
{
    llvm::Value *result = newLLVMTemp(_valueTy);

    if (e->id == QStringLiteral("this")) {
        CreateCall2(_llvmModule->getFunction("__qmljs_llvm_get_this_object"),
                    _llvmFunction->arg_begin(), result);
    } else {
        llvm::Value *name = getIdentifier(*e->id);
        CreateCall3(_llvmModule->getFunction("__qmljs_llvm_get_activation_property"),
                    _llvmFunction->arg_begin(), result, name);
    }
    _llvmValue = CreateLoad(result);

}

void InstructionSelection::visitTemp(IR::Temp *e)
{
    if (llvm::Value *t = getLLVMTemp(e)) {
        _llvmValue = CreateLoad(t);
    }
}

void InstructionSelection::visitClosure(IR::Closure *e)
{
    llvm::Value *tmp = newLLVMTemp(_valueTy);
    llvm::Value *clos = getLLVMFunction(e->value);
    assert("!broken: pass function name!");
    CreateCall3(_llvmModule->getFunction("__qmljs_llvm_init_native_function"),
                _llvmFunction->arg_begin(), tmp, clos);
    _llvmValue = CreateLoad(tmp);
}

void InstructionSelection::visitUnop(IR::Unop *e)
{
    llvm::Value *result = newLLVMTemp(_valueTy);
    genUnop(result, e);
    _llvmValue = CreateLoad(result);
}

void InstructionSelection::visitBinop(IR::Binop *e)
{
    llvm::Value *result = newLLVMTemp(_valueTy);
    genBinop(result, e);
    _llvmValue = CreateLoad(result);
}

void InstructionSelection::genUnop(llvm::Value *result, IR::Unop *e)
{
    IR::Temp *t = e->expr->asTemp();
    assert(t != 0);

    llvm::Value *expr = getLLVMTemp(t);
    llvm::Value *op = 0;

    switch (e->op) {
    default:
        Q_UNREACHABLE();
        break;

    case IR::OpNot: op = _llvmModule->getFunction("__qmljs_llvm_not"); break;
    case IR::OpUMinus: op = _llvmModule->getFunction("__qmljs_llvm_uminus"); break;
    case IR::OpUPlus: op = _llvmModule->getFunction("__qmljs_llvm_uplus"); break;
    case IR::OpCompl: op = _llvmModule->getFunction("__qmljs_llvm_compl"); break;
    }

    CreateCall3(op, _llvmFunction->arg_begin(), result, expr);
}

void InstructionSelection::genBinop(llvm::Value *result, IR::Binop *e)
{
    assert(e->left->asTemp() || e->left->asConst());
    assert(e->right->asTemp() || e->right->asConst());

    llvm::Value *left = toValuePtr(e->left);
    llvm::Value *right = toValuePtr(e->right);
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

    case IR::OpIncrement:
    case IR::OpDecrement:
        assert(!"TODO!");
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

llvm::AllocaInst *InstructionSelection::newLLVMTemp(llvm::Type *type, llvm::Value *size)
{
    llvm::AllocaInst *addr = new llvm::AllocaInst(type, size, llvm::Twine(), _allocaInsertPoint);
    return addr;
}

llvm::Value * InstructionSelection::genArguments(IR::ExprList *exprs, int &argc)
{
    llvm::Value *args = 0;

    argc = 0;
    for (IR::ExprList *it = exprs; it; it = it->next)
        ++argc;

    if (argc)
        args = newLLVMTemp(_valueTy, getInt32(argc));
    else
        args = llvm::Constant::getNullValue(_valueTy->getPointerTo());

    int i = 0;
    for (IR::ExprList *it = exprs; it; it = it->next) {
        llvm::Value *arg = getLLVMValue(it->expr);
        CreateStore(arg, CreateConstGEP1_32(args, i++));
    }

    return args;
}

void InstructionSelection::genCallMember(IR::Call *e, llvm::Value *result)
{
    if (! result)
        result = newLLVMTemp(_valueTy);

    IR::Member *m = e->base->asMember();
    llvm::Value *thisObject = getLLVMTemp(m->base->asTemp());
    llvm::Value *name = getIdentifier(*m->name);

    int argc = 0;
    llvm::Value *args = genArguments(e->args, argc);

    llvm::Value *actuals[] = {
        _llvmFunction->arg_begin(),
        result,
        thisObject,
        name,
        args,
        getInt32(argc)
    };

    CreateCall(_llvmModule->getFunction("__qmljs_llvm_call_property"), llvm::ArrayRef<llvm::Value *>(actuals));
    _llvmValue = CreateLoad(result);
}

void InstructionSelection::genConstructMember(IR::New *e, llvm::Value *result)
{
    if (! result)
        result = newLLVMTemp(_valueTy);

    IR::Member *m = e->base->asMember();
    llvm::Value *thisObject = getLLVMTemp(m->base->asTemp());
    llvm::Value *name = getIdentifier(*m->name);

    int argc = 0;
    llvm::Value *args = genArguments(e->args, argc);

    llvm::Value *actuals[] = {
        _llvmFunction->arg_begin(),
        result,
        thisObject,
        name,
        args,
        getInt32(argc)
    };

    CreateCall(_llvmModule->getFunction("__qmljs_llvm_construct_property"), llvm::ArrayRef<llvm::Value *>(actuals));
    _llvmValue = CreateLoad(result);
}

void InstructionSelection::genCallTemp(IR::Call *e, llvm::Value *result)
{
    if (! result)
        result = newLLVMTemp(_valueTy);

    llvm::Value *func = getLLVMTempReference(e->base);

    int argc = 0;
    llvm::Value *args = genArguments(e->args, argc);

    llvm::Value *thisObject = llvm::Constant::getNullValue(_valueTy->getPointerTo());

    llvm::Value *actuals[] = {
        _llvmFunction->arg_begin(),
        result,
        thisObject,
        func,
        args,
        getInt32(argc)
    };

    CreateCall(_llvmModule->getFunction("__qmljs_llvm_call_value"), actuals);

    _llvmValue = CreateLoad(result);
}

void InstructionSelection::genConstructTemp(IR::New *e, llvm::Value *result)
{
    if (! result)
        result = newLLVMTemp(_valueTy);

    llvm::Value *func = getLLVMTempReference(e->base);

    int argc = 0;
    llvm::Value *args = genArguments(e->args, argc);

    llvm::Value *actuals[] = {
        _llvmFunction->arg_begin(),
        result,
        func,
        args,
        getInt32(argc)
    };

    CreateCall(_llvmModule->getFunction("__qmljs_llvm_construct_value"), actuals);

    _llvmValue = CreateLoad(result);
}

void InstructionSelection::genCallName(IR::Call *e, llvm::Value *result)
{
    IR::Name *base = e->base->asName();

    if (! result)
        result = newLLVMTemp(_valueTy);

    if (! base->id) {
        switch (base->builtin) {
        case IR::Name::builtin_invalid:
            break;

        case IR::Name::builtin_typeof:
            CreateCall3(_llvmModule->getFunction("__qmljs_llvm_typeof"),
                        _llvmFunction->arg_begin(), result, getLLVMTempReference(e->args->expr));
            _llvmValue = CreateLoad(result);
            return;

        case IR::Name::builtin_throw:
            CreateCall2(_llvmModule->getFunction("__qmljs_llvm_throw"),
                        _llvmFunction->arg_begin(), getLLVMTempReference(e->args->expr));
            _llvmValue = llvm::UndefValue::get(_valueTy);
            return;

        case IR::Name::builtin_create_exception_handler:
            CreateCall2(_llvmModule->getFunction("__qmljs_llvm_create_exception_handler"),
                        _llvmFunction->arg_begin(), result);
            _llvmValue = CreateLoad(result);
            return;

        case IR::Name::builtin_delete_exception_handler:
            CreateCall(_llvmModule->getFunction("__qmljs_llvm_delete_exception_handler"),
                       _llvmFunction->arg_begin());
            return;

        case IR::Name::builtin_get_exception:
            CreateCall2(_llvmModule->getFunction("__qmljs_llvm_get_exception"),
                        _llvmFunction->arg_begin(), result);
            _llvmValue = CreateLoad(result);
            return;

        case IR::Name::builtin_foreach_iterator_object:
            CreateCall3(_llvmModule->getFunction("__qmljs_llvm_foreach_iterator_object"),
                        _llvmFunction->arg_begin(), result, getLLVMTempReference(e->args->expr));
            _llvmValue = CreateLoad(result);
            return;

        case IR::Name::builtin_foreach_next_property_name:
            CreateCall2(_llvmModule->getFunction("__qmljs_llvm_foreach_next_property_name"),
                        result, getLLVMTempReference(e->args->expr));
            _llvmValue = CreateLoad(result);
            return;

        case IR::Name::builtin_delete: {
            if (IR::Subscript *subscript = e->args->expr->asSubscript()) {
                CreateCall4(_llvmModule->getFunction("__qmljs_llvm_delete_subscript"),
                           _llvmFunction->arg_begin(),
                           result,
                           getLLVMTempReference(subscript->base),
                           getLLVMTempReference(subscript->index));
                _llvmValue = CreateLoad(result);
                return;
            } else if (IR::Member *member = e->args->expr->asMember()) {
                CreateCall4(_llvmModule->getFunction("__qmljs_llvm_delete_member"),
                           _llvmFunction->arg_begin(),
                           result,
                           getLLVMTempReference(member->base),
                           getIdentifier(*member->name));
                _llvmValue = CreateLoad(result);
                return;
            } else if (IR::Name *name = e->args->expr->asName()) {
                CreateCall3(_llvmModule->getFunction("__qmljs_llvm_delete_property"),
                           _llvmFunction->arg_begin(),
                           result,
                           getIdentifier(*name->id));
                _llvmValue = CreateLoad(result);
                return;
            } else {
                CreateCall3(_llvmModule->getFunction("__qmljs_llvm_delete_value"),
                           _llvmFunction->arg_begin(),
                           result,
                           getLLVMTempReference(e->args->expr));
                _llvmValue = CreateLoad(result);
                return;
            }
        } break;

        default:
            Q_UNREACHABLE();
        }
    } else {
        llvm::Value *name = getIdentifier(*base->id);

        int argc = 0;
        llvm::Value *args = genArguments(e->args, argc);

        CreateCall5(_llvmModule->getFunction("__qmljs_llvm_call_activation_property"),
                    _llvmFunction->arg_begin(), result, name, args, getInt32(argc));

        _llvmValue = CreateLoad(result);
    }
}

void InstructionSelection::genConstructName(IR::New *e, llvm::Value *result)
{
    IR::Name *base = e->base->asName();

    if (! result)
        result = newLLVMTemp(_valueTy);

    if (! base->id) {
        Q_UNREACHABLE();
    } else {
        llvm::Value *name = getIdentifier(*base->id);

        int argc = 0;
        llvm::Value *args = genArguments(e->args, argc);

        CreateCall5(_llvmModule->getFunction("__qmljs_llvm_construct_activation_property"),
                    _llvmFunction->arg_begin(), result, name, args, getInt32(argc));

        _llvmValue = CreateLoad(result);
    }
}

void InstructionSelection::visitCall(IR::Call *e)
{
    if (e->base->asMember()) {
        genCallMember(e);
    } else if (e->base->asTemp()) {
        genCallTemp(e);
    } else if (e->base->asName()) {
        genCallName(e);
    } else if (IR::Temp *t = e->base->asTemp()) {
        llvm::Value *base = getLLVMTemp(t);

        int argc = 0;
        llvm::Value *args = genArguments(e->args, argc);

        llvm::Value *result = newLLVMTemp(_valueTy);
        CreateStore(llvm::Constant::getNullValue(_valueTy), result);
        CreateCall5(_llvmModule->getFunction("__qmljs_llvm_call_value"),
                    _llvmFunction->arg_begin(), result, base, args, getInt32(argc));
        _llvmValue = CreateLoad(result);
    } else {
        Q_UNIMPLEMENTED();
    }
}

void InstructionSelection::visitNew(IR::New *e)
{
    if (e->base->asMember()) {
        genConstructMember(e);
    } else if (e->base->asTemp()) {
        genConstructTemp(e);
    } else if (e->base->asName()) {
        genConstructName(e);
    } else if (IR::Temp *t = e->base->asTemp()) {
        llvm::Value *base = getLLVMTemp(t);

        int argc = 0;
        llvm::Value *args = genArguments(e->args, argc);

        llvm::Value *result = newLLVMTemp(_valueTy);
        CreateStore(llvm::Constant::getNullValue(_valueTy), result);
        CreateCall5(_llvmModule->getFunction("__qmljs_llvm_construct_value"),
                    _llvmFunction->arg_begin(), result, base, args, getInt32(argc));
        _llvmValue = CreateLoad(result);
    } else {
        Q_UNIMPLEMENTED();
    }
}

void InstructionSelection::visitSubscript(IR::Subscript *e)
{
    llvm::Value *result = newLLVMTemp(_valueTy);
    llvm::Value *base = getLLVMTempReference(e->base);
    llvm::Value *index = getLLVMTempReference(e->index);
    CreateCall4(_llvmModule->getFunction("__qmljs_llvm_get_element"),
                _llvmFunction->arg_begin(), result, base, index);
    _llvmValue = CreateLoad(result);
}

void InstructionSelection::visitMember(IR::Member *e)
{
    llvm::Value *result = newLLVMTemp(_valueTy);
    llvm::Value *base = getLLVMTempReference(e->base);
    llvm::Value *name = getIdentifier(*e->name);

    CreateCall4(_llvmModule->getFunction("__qmljs_llvm_get_property"),
                _llvmFunction->arg_begin(), result, base, name);
    _llvmValue = CreateLoad(result);
}

llvm::Value *InstructionSelection::createValue(IR::Const *e)
{
    llvm::Value *tmp = newLLVMTemp(_valueTy);

    switch (e->type) {
    case IR::UndefinedType:
        CreateCall(_llvmModule->getFunction("__qmljs_llvm_init_undefined"), tmp);
        break;

    case IR::NullType:
        CreateCall(_llvmModule->getFunction("__qmljs_llvm_init_null"), tmp);
        break;

    case IR::BoolType:
        CreateCall2(_llvmModule->getFunction("__qmljs_llvm_init_boolean"), tmp,
                    getInt1(e->value ? 1 : 0));
        break;

    case IR::NumberType:
        CreateCall2(_llvmModule->getFunction("__qmljs_llvm_init_number"), tmp,
                    llvm::ConstantFP::get(_numberTy, e->value));
        break;

    default:
        Q_UNREACHABLE();
    }

    return tmp;
}

llvm::Value *InstructionSelection::toValuePtr(IR::Expr *e)
{
    if (IR::Temp *t = e->asTemp()) {
        return getLLVMTemp(t);
    } else if (IR::Const *c = e->asConst()) {
        return createValue(c);
    } else {
        Q_UNREACHABLE();
    }
}
