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

#ifndef QMLJS_NO_LLVM
// These includes have to come first, because WTF/Platform.h defines some macros
// with very unfriendly names that collide with class fields in LLVM.
#  include <llvm/PassManager.h>
#  include <llvm/Analysis/Passes.h>
#  include <llvm/Transforms/Scalar.h>
#  include <llvm/Transforms/IPO.h>
#  include <llvm/Assembly/PrintModulePass.h>
#  include <llvm/Support/raw_ostream.h>
#  include <llvm/Support/FormattedStream.h>
#  include <llvm/Support/Host.h>
#  include <llvm/Support/TargetRegistry.h>
#  include <llvm/Support/TargetSelect.h>
#  include <llvm/Target/TargetMachine.h>
#  include <llvm/Target/TargetData.h>

#  include "qv4isel_llvm_p.h"
#endif

#include "qmljs_objects.h"
#include "qv4codegen_p.h"
#include "qv4isel_masm_p.h"
#include "qv4isel_moth_p.h"
#include "qv4vme_moth_p.h"
#include "qv4syntaxchecker_p.h"
#include "qv4ecmaobjects_p.h"

#include <QtCore>
#include <private/qqmljsengine_p.h>
#include <private/qqmljslexer_p.h>
#include <private/qqmljsparser_p.h>
#include <private/qqmljsast_p.h>

#include <sys/mman.h>
#include <iostream>

static inline bool protect(const void *addr, size_t size)
{
    size_t pageSize = sysconf(_SC_PAGESIZE);
    size_t iaddr = reinterpret_cast<size_t>(addr);
    size_t roundAddr = iaddr & ~(pageSize - static_cast<size_t>(1));
    int mode = PROT_READ | PROT_WRITE | PROT_EXEC;
    return mprotect(reinterpret_cast<void*>(roundAddr), size + (iaddr - roundAddr), mode) == 0;
}

static void evaluate(QQmlJS::VM::Context *ctx, const QString &fileName, const QString &source,
                     QQmlJS::Codegen::Mode mode = QQmlJS::Codegen::GlobalCode);

namespace builtins {

using namespace QQmlJS::VM;

struct Print: FunctionObject
{
    Print(Context *scope): FunctionObject(scope) {}

    virtual void call(Context *ctx)
    {
        for (unsigned int i = 0; i < ctx->argumentCount; ++i) {
            String *s = ctx->argument(i).toString(ctx);
            if (i)
                std::cout << ' ';
            std::cout << qPrintable(s->toQString());
        }
        std::cout << std::endl;
    }
};

struct Eval: FunctionObject
{
    Eval(Context *scope): FunctionObject(scope) {}

    virtual void call(Context *ctx)
    {
        const QString code = ctx->argument(0).toString(ctx)->toQString();
        evaluate(ctx, QStringLiteral("eval code"), code, QQmlJS::Codegen::EvalCode);
    }
};

} // builtins

#ifndef QMLJS_NO_LLVM
void compile(const QString &fileName, const QString &source)
{
    using namespace QQmlJS;

    IR::Module module;
    QQmlJS::Engine ee, *engine = &ee;
    Lexer lexer(engine);
    lexer.setCode(source, 1, false);
    Parser parser(engine);

    const bool parsed = parser.parseProgram();

    foreach (const DiagnosticMessage &m, parser.diagnosticMessages()) {
        std::cerr << qPrintable(fileName) << ':' << m.loc.startLine << ':' << m.loc.startColumn
                  << ": error: " << qPrintable(m.message) << std::endl;
    }

    if (parsed) {
        using namespace AST;
        Program *program = AST::cast<Program *>(parser.rootNode());

        Codegen cg;
        /*IR::Function *globalCode =*/ cg(program, &module);

        LLVMInstructionSelection llvmIsel(llvm::getGlobalContext());
        if (llvm::Module *llvmModule = llvmIsel.getLLVMModule(&module)) {
            llvm::PassManager PM;

            const std::string triple = llvm::sys::getDefaultTargetTriple();

            LLVMInitializeX86TargetInfo();
            LLVMInitializeX86Target();
            LLVMInitializeX86AsmPrinter();
            LLVMInitializeX86AsmParser();
            LLVMInitializeX86Disassembler();
            LLVMInitializeX86TargetMC();

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

            llvm::formatted_raw_ostream out(llvm::outs());
            PM.add(llvm::createScalarReplAggregatesPass());
            PM.add(llvm::createInstructionCombiningPass());
            PM.add(llvm::createGlobalOptimizerPass());
            PM.add(llvm::createFunctionInliningPass(25));
            targetMachine->addPassesToEmitFile(PM, out, llvm::TargetMachine::CGFT_AssemblyFile);
            PM.run(*llvmModule);
            delete llvmModule;
        }
    }
}

int compileFiles(const QStringList &files)
{
    foreach (const QString &fileName, files) {
        QFile file(fileName);
        if (file.open(QFile::ReadOnly)) {
            QString source = QString::fromUtf8(file.readAll());
            compile(fileName, source);
        }
    }
    return 0;
}

int evaluateCompiledCode(const QStringList &files)
{
    using namespace QQmlJS;

    foreach (const QString &libName, files) {
        QFileInfo libInfo(libName);
        QLibrary lib(libInfo.absoluteFilePath());
        lib.load();
        QFunctionPointer ptr = lib.resolve("_25_entry");
        void (*code)(VM::Context *) = (void (*)(VM::Context *)) ptr;

        VM::ExecutionEngine vm;
        VM::Context *ctx = vm.rootContext;

        QQmlJS::VM::Object *globalObject = vm.globalObject.objectValue();
        globalObject->setProperty(ctx, vm.identifier(QStringLiteral("print")),
                                  QQmlJS::VM::Value::fromObject(new builtins::Print(ctx)));

        code(ctx);

        if (ctx->hasUncaughtException) {
            if (VM::ErrorObject *e = ctx->result.asErrorObject())
                std::cerr << "Uncaught exception: " << qPrintable(e->value.toString(ctx)->toQString()) << std::endl;
            else
                std::cerr << "Uncaught exception: " << qPrintable(ctx->result.toString(ctx)->toQString()) << std::endl;
        }
    }
    return 0;
}

#endif

static void evaluate(QQmlJS::VM::Context *ctx, const QString &fileName, const QString &source,
                     QQmlJS::Codegen::Mode mode)
{
    using namespace QQmlJS;

    VM::ExecutionEngine *vm = ctx->engine;
    IR::Module module;
    IR::Function *globalCode = 0;

    const size_t codeSize = 400 * getpagesize();
    uchar *code = 0;
    if (posix_memalign((void**)&code, 16, codeSize))
        assert(!"memalign failed");
    assert(code);
    assert(! (size_t(code) & 15));

    static bool useMoth = !qgetenv("USE_MOTH").isNull();

    {
        QQmlJS::Engine ee, *engine = &ee;
        Lexer lexer(engine);
        lexer.setCode(source, 1, false);
        Parser parser(engine);

        const bool parsed = parser.parseProgram();

        foreach (const DiagnosticMessage &m, parser.diagnosticMessages()) {
            std::cerr << qPrintable(fileName) << ':' << m.loc.startLine << ':' << m.loc.startColumn
                      << ": error: " << qPrintable(m.message) << std::endl;
        }

        if (parsed) {
            using namespace AST;
            Program *program = AST::cast<Program *>(parser.rootNode());

            Codegen cg;
            globalCode = cg(program, &module, mode);

            if (useMoth) {
                Moth::InstructionSelection isel(vm, &module, code);
                foreach (IR::Function *function, module.functions)
                    isel(function);
            } else {
                foreach (IR::Function *function, module.functions) {
                    MASM::InstructionSelection isel(vm, &module, code);
                    isel(function);
                }

                if (! protect(code, codeSize))
                    Q_UNREACHABLE();
            }
        }

        if (! globalCode)
            return;
    }

    ctx->hasUncaughtException = false;
    if (! ctx->activation.isObject())
        __qmljs_init_object(&ctx->activation, new QQmlJS::VM::Object());

    foreach (const QString *local, globalCode->locals) {
        ctx->activation.objectValue()->setProperty(ctx, *local, QQmlJS::VM::Value::undefinedValue());
    }

    if (useMoth) {
        Moth::VME vme;
        vme(ctx, code);
    } else {
        globalCode->code(ctx, globalCode->codeData);
    }

    if (ctx->hasUncaughtException) {
        if (VM::ErrorObject *e = ctx->result.asErrorObject())
            std::cerr << "Uncaught exception: " << qPrintable(e->value.toString(ctx)->toQString()) << std::endl;
        else
            std::cerr << "Uncaught exception: " << qPrintable(ctx->result.toString(ctx)->toQString()) << std::endl;
    } else if (! ctx->result.isUndefined()) {
        if (! qgetenv("SHOW_EXIT_VALUE").isNull())
            std::cout << "exit value: " << qPrintable(ctx->result.toString(ctx)->toQString()) << std::endl;
    }
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QStringList args = app.arguments();
    args.removeFirst();

#ifndef QMLJS_NO_LLVM
    if (args.isEmpty()) {
        std::cerr << "Usage: v4 [--compile|--aot] file..." << std::endl;
        return 0;
    }

    if (args.first() == QLatin1String("--compile")) {
        args.removeFirst();
        return compileFiles(args);
    } else if (args.first() == QLatin1String("--aot")) {
        args.removeFirst();
        return evaluateCompiledCode(args);
    }
#endif

    QQmlJS::VM::ExecutionEngine vm;
    QQmlJS::VM::Context *ctx = vm.rootContext;

    QQmlJS::VM::Object *globalObject = vm.globalObject.objectValue();
    globalObject->setProperty(ctx, vm.identifier(QStringLiteral("print")),
                              QQmlJS::VM::Value::fromObject(new builtins::Print(ctx)));

    globalObject->setProperty(ctx, vm.identifier(QStringLiteral("eval")),
                              QQmlJS::VM::Value::fromObject(new builtins::Eval(ctx)));

    foreach (const QString &fn, args) {
        QFile file(fn);
        if (file.open(QFile::ReadOnly)) {
            const QString code = QString::fromUtf8(file.readAll());
            file.close();

            evaluate(vm.rootContext, fn, code);
        }
    }
}
