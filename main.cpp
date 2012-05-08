
#include "qmljs_objects.h"
#include "qv4codegen_p.h"
#include "qv4isel_p.h"
#include "qv4syntaxchecker_p.h"

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
    virtual void call(Context *ctx)
    {
        for (size_t i = 0; i < ctx->argumentCount; ++i) {
            Value v;
            __qmljs_to_string(ctx, &v, &ctx->arguments[i]);
            if (i)
                std::cout << ' ';
            std::cout << qPrintable(v.stringValue->text());
        }
        std::cout << std::endl;
    }
};
} // builtins


void evaluate(QQmlJS::Engine *engine, const QString &fileName, const QString &code)
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
        cg(program, &module);

        const size_t codeSize = 10 * getpagesize();
        uchar *code = (uchar *) malloc(codeSize);

        x86_64::InstructionSelection isel(&module, code);
        QHash<QString, IR::Function *> codeByName;
        foreach (IR::Function *function, module.functions) {
            isel(function);
            if (function->name && ! function->name->isEmpty()) {
                codeByName.insert(*function->name, function);
            }
        }

        if (! protect(code, codeSize))
            Q_UNREACHABLE();

        VM::Context *ctx = new VM::Context;
        ctx->init();
        ctx->activation = VM::Value::object(ctx, new VM::ArgumentsObject(ctx));
        ctx->activation.objectValue->put(VM::String::get(ctx, QLatin1String("print")),
                                         VM::Value::object(ctx, new builtins::Print()));
        foreach (IR::Function *function, module.functions) {
            if (function->name && ! function->name->isEmpty()) {
                ctx->activation.objectValue->put(VM::String::get(ctx, *function->name),
                                                 VM::Value::object(ctx, new VM::ScriptFunction(function)));
            }
        }
        codeByName.value(QLatin1String("%entry"))->code(ctx);
    }
}

int main(int argc, char *argv[])
{
    using namespace QQmlJS;

    QCoreApplication app(argc, argv);
    QStringList args = app.arguments();
    args.removeFirst();

    Engine engine;
    foreach (const QString &fn, args) {
        QFile file(fn);
        if (file.open(QFile::ReadOnly)) {
            const QString code = QString::fromUtf8(file.readAll());
            file.close();
            evaluate(&engine, fn, code);
        }
    }
}
