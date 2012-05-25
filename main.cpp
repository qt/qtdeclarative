
#include "qmljs_objects.h"
#include "qv4codegen_p.h"
#include "qv4isel_p.h"
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

namespace builtins {

using namespace QQmlJS::VM;

struct Print: FunctionObject
{
    Print(Context *scope): FunctionObject(scope) {}

    virtual void call(Context *ctx)
    {
        for (size_t i = 0; i < ctx->argumentCount; ++i) {
            String *s = ctx->argument(i).toString(ctx);
            if (i)
                std::cout << ' ';
            std::cout << qPrintable(s->toQString());
        }
        std::cout << std::endl;
    }
};

} // builtins


void evaluate(QQmlJS::VM::ExecutionEngine *vm, const QString &fileName, const QString &source)
{
    using namespace QQmlJS;

    IR::Module module;
    IR::Function *globalCode = 0;

    const size_t codeSize = 10 * getpagesize();
    uchar *code = (uchar *) malloc(codeSize);
    assert(! (size_t(code) & 15));

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

            x86_64::InstructionSelection isel(vm, &module, code);
            foreach (IR::Function *function, module.functions) {
                isel(function);
            }
        }
    }

    if (! protect(code, codeSize))
        Q_UNREACHABLE();

    VM::Object *globalObject = vm->globalObject.objectValue;
    VM::Context *ctx = vm->rootContext;

    globalObject->setProperty(ctx, vm->identifier(QStringLiteral("print")),
                              VM::Value::fromObject(new builtins::Print(ctx)));

    ctx->varCount = globalCode->locals.size();
    if (ctx->varCount) {
        ctx->locals = new VM::Value[ctx->varCount];
        ctx->vars = new VM::String*[ctx->varCount];
        std::fill(ctx->locals, ctx->locals + ctx->varCount, VM::Value::undefinedValue());
        for (size_t i = 0; i < ctx->varCount; ++i)
            ctx->vars[i] = ctx->engine->identifier(*globalCode->locals.at(i));
    }

    globalCode->code(ctx);

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

    foreach (const QString &fn, args) {
        QFile file(fn);
        if (file.open(QFile::ReadOnly)) {
            const QString code = QString::fromUtf8(file.readAll());
            file.close();
            QQmlJS::VM::ExecutionEngine vm;
            evaluate(&vm, fn, code);
        }
    }
}
