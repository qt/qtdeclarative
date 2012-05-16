
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
            Value v;
            __qmljs_to_string(ctx, &v, &ctx->arguments[i]);
            if (i)
                std::cout << ' ';
            std::cout << qPrintable(v.stringValue->toQString());
        }
        std::cout << std::endl;
    }
};

} // builtins


void evaluate(QQmlJS::VM::ExecutionEngine *vm, QQmlJS::Engine *engine, const QString &fileName, const QString &code)
{
    using namespace QQmlJS;

    Lexer lexer(engine);
    lexer.setCode(code, 1, false);
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
        IR::Module module;
        IR::Function *globalCode = cg(program, &module);

        const size_t codeSize = 10 * getpagesize();
        uchar *code = (uchar *) malloc(codeSize);

        x86_64::InstructionSelection isel(vm, &module, code);
        foreach (IR::Function *function, module.functions) {
            isel(function);
        }

        if (! protect(code, codeSize))
            Q_UNREACHABLE();

        VM::Object *globalObject = vm->globalObject.objectValue;
        VM::Context *ctx = vm->rootContext;

        globalObject->put(vm->identifier(QLatin1String("print")),
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

        delete[] ctx->locals;
        delete[] ctx->vars;
    }
}

int main(int argc, char *argv[])
{
    using namespace QQmlJS;

    QCoreApplication app(argc, argv);
    QStringList args = app.arguments();
    args.removeFirst();

    VM::ExecutionEngine vm;
    Engine engine;
    foreach (const QString &fn, args) {
        QFile file(fn);
        if (file.open(QFile::ReadOnly)) {
            const QString code = QString::fromUtf8(file.readAll());
            file.close();
            evaluate(&vm, &engine, fn, code);
        }
    }
}
