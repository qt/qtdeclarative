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
#  include "qv4_llvm_p.h"
#endif

#include "debugging.h"
#include "qv4object.h"
#include "qmljs_runtime.h"
#include "qv4functionobject.h"
#include "qv4errorobject.h"
#include "qv4globalobject.h"
#include "qv4codegen_p.h"
#include "qv4isel_masm_p.h"
#include "qv4isel_moth_p.h"
#include "qv4vme_moth_p.h"
#include "qv4syntaxchecker_p.h"
#include "qv4ecmaobjects_p.h"
#include "qv4isel_p.h"
#include "qv4mm.h"
#include "qmljs_environment.h"

#include <QtCore>
#include <private/qqmljsengine_p.h>
#include <private/qqmljslexer_p.h>
#include <private/qqmljsparser_p.h>
#include <private/qqmljsast_p.h>

#include <sys/mman.h>
#include <iostream>

namespace builtins {

using namespace QQmlJS::VM;

struct Print: FunctionObject
{
    Print(ExecutionContext *scope): FunctionObject(scope) {
        name = scope->engine->newString("print");
    }

    virtual Value call(ExecutionContext *ctx)
    {
        for (unsigned int i = 0; i < ctx->argumentCount; ++i) {
            String *s = ctx->argument(i).toString(ctx);
            if (i)
                std::cout << ' ';
            std::cout << qPrintable(s->toQString());
        }
        std::cout << std::endl;
        return Value::undefinedValue();
    }
};

struct TestHarnessError: FunctionObject
{
    TestHarnessError(ExecutionContext *scope, bool &errorInTestHarness): FunctionObject(scope), errorOccurred(errorInTestHarness) {
        name = scope->engine->newString("$ERROR");
    }

    virtual Value call(ExecutionContext *ctx)
    {
        errorOccurred = true;

        for (unsigned int i = 0; i < ctx->argumentCount; ++i) {
            String *s = ctx->argument(i).toString(ctx);
            if (i)
                std::cerr << ' ';
            std::cerr << qPrintable(s->toQString());
        }
        std::cerr << std::endl;
        return Value::undefinedValue();
    }

    bool &errorOccurred;
};

struct GC: public FunctionObject
{
    GC(ExecutionContext* scope)
        : FunctionObject(scope)
    {
        name = scope->engine->newString("gc");
    }
    virtual Value call(ExecutionContext *ctx)
    {
        ctx->engine->memoryManager->runGC();
        return Value::undefinedValue();
    }
};

} // builtins

static void showException(QQmlJS::VM::ExecutionContext *ctx)
{
    QQmlJS::VM::ErrorObject *e = ctx->engine->exception.asErrorObject();
    if (!e) {
        std::cerr << "Uncaught exception: " << qPrintable(ctx->engine->exception.toString(ctx)->toQString()) << std::endl;
        return;
    }

    if (QQmlJS::VM::SyntaxErrorObject *err = e->asSyntaxError()) {
        QQmlJS::VM::DiagnosticMessage *msg = err->message();
        if (!msg) {
            std::cerr << "Uncaught exception: Syntax error" << std::endl;
            return;
        }

        for (; msg; msg = msg->next) {
            std::cerr << qPrintable(msg->buildFullMessage(ctx)->toQString()) << std::endl;
        }
    } else {
        std::cerr << "Uncaught exception: " << qPrintable(e->__get__(ctx, ctx->engine->identifier(QStringLiteral("message")), 0).toString(ctx)->toQString()) << std::endl;
    }
}

#ifndef QMLJS_NO_LLVM
int executeLLVMCode(void *codePtr)
{
    using namespace QQmlJS;

    if (!codePtr)
        return EXIT_FAILURE;
    void (*code)(VM::ExecutionContext *) = (void (*)(VM::ExecutionContext *)) codePtr;

    QScopedPointer<QQmlJS::EvalISelFactory> iSelFactory(new QQmlJS::Moth::ISelFactory);
    VM::ExecutionEngine vm(iSelFactory.data());
    VM::ExecutionContext *ctx = vm.rootContext;

    QQmlJS::VM::Object *globalObject = vm.globalObject.objectValue();
    globalObject->__put__(ctx, vm.identifier(QStringLiteral("print")),
                          QQmlJS::VM::Value::fromObject(new (ctx->engine->memoryManager) builtins::Print(ctx)));

    void * buf = __qmljs_create_exception_handler(ctx);
    if (setjmp(*(jmp_buf *)buf)) {
        showException(ctx);
        return EXIT_FAILURE;
    }

    code(ctx);
    return EXIT_SUCCESS;
}

int compile(const QString &fileName, const QString &source, QQmlJS::LLVMOutputType outputType)
{
    using namespace QQmlJS;

    IR::Module module;
    QQmlJS::Engine ee, *engine = &ee;
    Lexer lexer(engine);
    lexer.setCode(source, 1, false);
    Parser parser(engine);

    const bool parsed = parser.parseProgram();

    foreach (const DiagnosticMessage &m, parser.diagnosticMessages()) {
        std::cerr << qPrintable(fileName) << ':'
                  << m.loc.startLine << ':'
                  << m.loc.startColumn << ": error: "
                  << qPrintable(m.message) << std::endl;
    }

    if (!parsed)
        return EXIT_FAILURE;

    using namespace AST;
    Program *program = AST::cast<Program *>(parser.rootNode());

    class MyErrorHandler: public ErrorHandler {
    public:
        virtual void syntaxError(QQmlJS::VM::DiagnosticMessage *message) {
            for (; message; message = message->next) {
                std::cerr << qPrintable(message->fileName) << ':'
                          << message->startLine << ':'
                          << message->startColumn << ": "
                          << (message->type == QQmlJS::VM::DiagnosticMessage::Error ? "error" : "warning") << ": "
                          << qPrintable(message->message) << std::endl;
            }
        }
    } errorHandler;

    Codegen cg(&errorHandler, false);
    // FIXME: if the program is empty, we should we generate an empty %entry, or give an error?
    /*IR::Function *globalCode =*/ cg(fileName, program, &module);

    int (*exec)(void *) = outputType == LLVMOutputJit ? executeLLVMCode : 0;
    return compileWithLLVM(&module, fileName, outputType, exec);
}

int compileFiles(const QStringList &files, QQmlJS::LLVMOutputType outputType)
{
    foreach (const QString &fileName, files) {
        QFile file(fileName);
        if (file.open(QFile::ReadOnly)) {
            QString source = QString::fromUtf8(file.readAll());
            int result = compile(fileName, source, outputType);
            if (result != EXIT_SUCCESS)
                return result;
        } else {
            std::cerr << "Error: cannot open file " << fileName.toUtf8().constData() << std::endl;
            return EXIT_FAILURE;
        }
    }
    return EXIT_SUCCESS;
}

int evaluateCompiledCode(const QStringList &files)
{
    using namespace QQmlJS;

    foreach (const QString &libName, files) {
        QFileInfo libInfo(libName);
        QLibrary lib(libInfo.absoluteFilePath());
        lib.load();
        QFunctionPointer ptr = lib.resolve("%entry");
//        qDebug("_%%entry resolved to address %p", ptr);
        int result = executeLLVMCode((void *) ptr);
        if (result != EXIT_SUCCESS)
            return result;
    }

    return EXIT_SUCCESS;
}

#endif


int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QStringList args = app.arguments();
    args.removeFirst();

    enum {
        use_masm,
        use_moth,
        use_llvm_compiler,
        use_llvm_runtime,
        use_llvm_jit
    } mode = use_masm;

#ifndef QMLJS_NO_LLVM
    QQmlJS::LLVMOutputType fileType = QQmlJS::LLVMOutputObject;
#endif // QMLJS_NO_LLVM
    bool enableDebugging = false;

    if (!args.isEmpty()) {
        if (args.first() == QLatin1String("-d") || args.first() == QLatin1String("--debug")) {
            enableDebugging = true;
            args.removeFirst();
        }
    }

    if (!args.isEmpty()) {
        if (args.first() == QLatin1String("--jit")) {
            mode = use_masm;
            args.removeFirst();
        }

        if (args.first() == QLatin1String("--interpret")) {
            mode = use_moth;
            args.removeFirst();
        }

#ifndef QMLJS_NO_LLVM
        if (args.first() == QLatin1String("--compile")) {
            mode = use_llvm_compiler;
            args.removeFirst();

            if (!args.isEmpty() && args.first() == QLatin1String("-t")) {
                args.removeFirst();
                // Note: keep this list in sync with the enum!
                static QStringList fileTypes = QStringList() << QLatin1String("ll") << QLatin1String("bc") << QLatin1String("asm") << QLatin1String("obj");
                if (args.isEmpty() || !fileTypes.contains(args.first())) {
                    std::cerr << "file types: ll, bc, asm, obj" << std::endl;
                    return EXIT_FAILURE;
                }
                fileType = (QQmlJS::LLVMOutputType) fileTypes.indexOf(args.first());
                args.removeFirst();
            }
        }

        if (args.first() == QLatin1String("--aot")) {
            mode = use_llvm_runtime;
            args.removeFirst();
        }

        if (args.first() == QLatin1String("--llvm-jit")) {
            mode = use_llvm_jit;
            args.removeFirst();
        }
#endif // QMLJS_NO_LLVM
        if (args.first() == QLatin1String("--help")) {
            std::cerr << "Usage: v4 [|--debug|-d] [|--jit|--interpret|--compile|--aot|--llvm-jit] file..." << std::endl;
            return EXIT_SUCCESS;
        }
    }

    switch (mode) {
#ifdef QMLJS_NO_LLVM
    case use_llvm_compiler:
    case use_llvm_runtime:
    case use_llvm_jit:
        std::cerr << "LLVM backend was not built, compiler is unavailable." << std::endl;
        return EXIT_FAILURE;
#else // QMLJS_NO_LLVM
    case use_llvm_jit:
        return compileFiles(args, QQmlJS::LLVMOutputJit);
    case use_llvm_compiler:
        return compileFiles(args, fileType);
    case use_llvm_runtime:
        return evaluateCompiledCode(args);
#endif // QMLJS_NO_LLVM
    case use_masm:
    case use_moth: {
        QScopedPointer<QQmlJS::EvalISelFactory> iSelFactory;
        if (mode == use_moth) {
            iSelFactory.reset(new QQmlJS::Moth::ISelFactory);
        } else {
            iSelFactory.reset(new QQmlJS::MASM::ISelFactory);
        }

        QQmlJS::VM::ExecutionEngine vm(iSelFactory.data());

        QScopedPointer<QQmlJS::Debugging::Debugger> debugger;
        if (enableDebugging)
            debugger.reset(new QQmlJS::Debugging::Debugger(&vm));
        vm.debugger = debugger.data();

        QQmlJS::VM::ExecutionContext *ctx = vm.rootContext;

        QQmlJS::VM::Object *globalObject = vm.globalObject.objectValue();
        QQmlJS::VM::Object *print = new (ctx->engine->memoryManager) builtins::Print(ctx);
        print->prototype = ctx->engine->objectPrototype;
        globalObject->__put__(ctx, vm.identifier(QStringLiteral("print")),
                                  QQmlJS::VM::Value::fromObject(print));
        QQmlJS::VM::Object *gc = new (ctx->engine->memoryManager) builtins::GC(ctx);
        gc->prototype = ctx->engine->objectPrototype;
        globalObject->__put__(ctx, vm.identifier(QStringLiteral("gc")),
                                  QQmlJS::VM::Value::fromObject(gc));

        bool errorInTestHarness = false;
        if (!qgetenv("IN_TEST_HARNESS").isEmpty())
            globalObject->__put__(ctx, vm.identifier(QStringLiteral("$ERROR")),
                                  QQmlJS::VM::Value::fromObject(new (ctx->engine->memoryManager) builtins::TestHarnessError(ctx, errorInTestHarness)));

        foreach (const QString &fn, args) {
            QFile file(fn);
            if (file.open(QFile::ReadOnly)) {
                const QString code = QString::fromUtf8(file.readAll());
                file.close();

                void * buf = __qmljs_create_exception_handler(ctx);
                if (setjmp(*(jmp_buf *)buf)) {
                    showException(ctx);
                    return EXIT_FAILURE;
                }

                QQmlJS::VM::Function *f = QQmlJS::VM::EvalFunction::parseSource(ctx, fn, code, QQmlJS::Codegen::GlobalCode);
                if (!f)
                    continue;

                ctx->strictMode = f->isStrict;
                if (debugger)
                    debugger->aboutToCall(0, ctx);
                QQmlJS::VM::Value result = f->code(ctx, f->codeData);
                if (debugger)
                    debugger->justLeft(ctx);
                if (!result.isUndefined()) {
                    if (! qgetenv("SHOW_EXIT_VALUE").isEmpty())
                        std::cout << "exit value: " << qPrintable(result.toString(ctx)->toQString()) << std::endl;
                }

                if (errorInTestHarness)
                    return EXIT_FAILURE;
            } else {
                std::cerr << "Error: cannot open file " << fn.toUtf8().constData() << std::endl;
                return EXIT_FAILURE;
            }
        }

        vm.memoryManager->dumpStats();
    } return EXIT_SUCCESS;
    }
}
