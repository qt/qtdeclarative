
#include "qmljs_objects.h"
#include "qv4codegen_p.h"
#include "qv4isel_x86_64_p.h"
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

#ifndef QMLJS_NO_LLVM
#  include "qv4isel_llvm_p.h"

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
#endif

static inline bool protect(const void *addr, size_t size)
{
    size_t pageSize = sysconf(_SC_PAGESIZE);
    size_t iaddr = reinterpret_cast<size_t>(addr);
    size_t roundAddr = iaddr & ~(pageSize - static_cast<size_t>(1));
    int mode = PROT_READ | PROT_WRITE | PROT_EXEC;
    return mprotect(reinterpret_cast<void*>(roundAddr), size + (iaddr - roundAddr), mode) == 0;
}

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

        QQmlJS::VM::Object *globalObject = vm.globalObject.objectValue;
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

void evaluate(QQmlJS::VM::ExecutionEngine *vm, const QString &fileName, const QString &source)
{
    using namespace QQmlJS;

    IR::Module module;
    IR::Function *globalCode = 0;

    const size_t codeSize = 400 * getpagesize();
    uchar *code = (uchar *) malloc(codeSize);
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
            globalCode = cg(program, &module);

            if (useMoth) {
                Moth::InstructionSelection isel(vm, &module, code);
                foreach (IR::Function *function, module.functions)
                    isel(function);
            } else {
                x86_64::InstructionSelection isel(vm, &module, code);
                foreach (IR::Function *function, module.functions)
                    isel(function);
            }
        }

        if (! globalCode)
            return;
    }

    if (!useMoth) {
        if (! protect(code, codeSize))
            Q_UNREACHABLE();
    }

    VM::Context *ctx = vm->rootContext;

    ctx->varCount = globalCode->locals.size();
    if (ctx->varCount) {
        ctx->locals = new VM::Value[ctx->varCount];
        ctx->vars = new VM::String*[ctx->varCount];
        std::fill(ctx->locals, ctx->locals + ctx->varCount, VM::Value::undefinedValue());
        for (unsigned int i = 0; i < ctx->varCount; ++i)
            ctx->vars[i] = ctx->engine->identifier(*globalCode->locals.at(i));
    }

    if (useMoth) {
        Moth::VME vme;
        vme(ctx, code);
    } else {
        globalCode->code(ctx);
    }

    if (ctx->hasUncaughtException) {
        if (VM::ErrorObject *e = ctx->result.asErrorObject())
            std::cerr << "Uncaught exception: " << qPrintable(e->value.toString(ctx)->toQString()) << std::endl;
        else
            std::cerr << "Uncaught exception: " << qPrintable(ctx->result.toString(ctx)->toQString()) << std::endl;
    }

    delete[] ctx->locals;
    delete[] ctx->vars;
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

    foreach (const QString &fn, args) {
        QFile file(fn);
        if (file.open(QFile::ReadOnly)) {
            const QString code = QString::fromUtf8(file.readAll());
            file.close();
            QQmlJS::VM::ExecutionEngine vm;
            QQmlJS::VM::Context *ctx = vm.rootContext;

            QQmlJS::VM::Object *globalObject = vm.globalObject.objectValue;
            globalObject->setProperty(ctx, vm.identifier(QStringLiteral("print")),
                                      QQmlJS::VM::Value::fromObject(new builtins::Print(ctx)));

            evaluate(&vm, fn, code);
        }
    }
}
