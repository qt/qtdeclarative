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

#include "private/qv4object_p.h"
#include "private/qv4runtime_p.h"
#include "private/qv4functionobject_p.h"
#include "private/qv4errorobject_p.h"
#include "private/qv4globalobject_p.h"
#include "private/qv4codegen_p.h"
#include "private/qv4isel_moth_p.h"
#include "private/qv4vme_moth_p.h"
#include "private/qv4objectproto_p.h"
#include "private/qv4isel_p.h"
#include "private/qv4mm_p.h"
#include "private/qv4context_p.h"
#include "private/qv4script_p.h"
#include "private/qv4exception_p.h"

#ifdef V4_ENABLE_JIT
#  include "private/qv4isel_masm_p.h"
#endif // V4_ENABLE_JIT

#include <QtCore>
#include <private/qqmljsengine_p.h>
#include <private/qqmljslexer_p.h>
#include <private/qqmljsparser_p.h>
#include <private/qqmljsast_p.h>

#include <iostream>

namespace builtins {

using namespace QV4;

struct Print: FunctionObject
{
    Print(ExecutionContext *scope): FunctionObject(scope) {
        vtbl = &static_vtbl;
        name = scope->engine->newString("print");
    }

    static Value call(Managed *, const Value &, Value *args, int argc)
    {
        for (int i = 0; i < argc; ++i) {
            QString s = args[i].toQString();
            if (i)
                std::cout << ' ';
            std::cout << qPrintable(s);
        }
        std::cout << std::endl;
        return Value::undefinedValue();
    }

    static const ManagedVTable static_vtbl;
};

DEFINE_MANAGED_VTABLE(Print);

struct GC: public FunctionObject
{
    GC(ExecutionContext* scope)
        : FunctionObject(scope)
    {
        vtbl = &static_vtbl;
        name = scope->engine->newString("gc");
    }
    static Value call(Managed *m, const Value &, Value *, int)
    {
        m->engine()->memoryManager->runGC();
        return Value::undefinedValue();
    }

    static const ManagedVTable static_vtbl;
};

DEFINE_MANAGED_VTABLE(GC);

} // builtins

static void showException(QV4::ExecutionContext *ctx, const QV4::Exception &exception)
{
    QV4::ErrorObject *e = exception.value().asErrorObject();
    if (!e) {
        std::cerr << "Uncaught exception: " << qPrintable(exception.value().toString(ctx)->toQString()) << std::endl;
    } else {
        std::cerr << "Uncaught exception: " << qPrintable(e->get(ctx->engine->newString(QStringLiteral("message")), 0).toString(ctx)->toQString()) << std::endl;
    }

    foreach (const QV4::ExecutionEngine::StackFrame &frame, exception.stackTrace()) {
        std::cerr << "    at " << qPrintable(frame.function) << " (" << qPrintable(frame.source);
        if (frame.line >= 0)
            std::cerr << ":" << frame.line;
        std::cerr << ")" << std::endl;
    }
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QStringList args = app.arguments();
    args.removeFirst();

    enum {
        use_masm,
        use_moth
    } mode;
#ifdef V4_ENABLE_JIT
    mode = use_masm;
#else
    mode = use_moth;
#endif

    bool runAsQml = false;

    if (!args.isEmpty()) {
        if (args.first() == QLatin1String("--jit")) {
            mode = use_masm;
            args.removeFirst();
        }

        if (args.first() == QLatin1String("--interpret")) {
            mode = use_moth;
            args.removeFirst();
        }

        if (args.first() == QLatin1String("--qml")) {
            runAsQml = true;
            args.removeFirst();
        }

        if (args.first() == QLatin1String("--help")) {
            std::cerr << "Usage: v4 [|--debug|-d] [|--jit|--interpret|--compile|--aot|--llvm-jit] file..." << std::endl;
            return EXIT_SUCCESS;
        }
    }

    switch (mode) {
    case use_masm:
    case use_moth: {
        QQmlJS::EvalISelFactory* iSelFactory = 0;
        if (mode == use_moth) {
            iSelFactory = new QQmlJS::Moth::ISelFactory;
#ifdef V4_ENABLE_JIT
        } else {
            iSelFactory = new QQmlJS::MASM::ISelFactory;
#endif // V4_ENABLE_JIT
        }

        QV4::ExecutionEngine vm(iSelFactory);

        QV4::ExecutionContext *ctx = vm.rootContext;

        QV4::Object *globalObject = vm.globalObject;
        QV4::Object *print = new (ctx->engine->memoryManager) builtins::Print(ctx);
        print->prototype = ctx->engine->objectPrototype;
        globalObject->put(vm.newIdentifier(QStringLiteral("print")), QV4::Value::fromObject(print));
        QV4::Object *gc = new (ctx->engine->memoryManager) builtins::GC(ctx);
        gc->prototype = ctx->engine->objectPrototype;
        globalObject->put(vm.newIdentifier(QStringLiteral("gc")), QV4::Value::fromObject(gc));

        foreach (const QString &fn, args) {
            QFile file(fn);
            if (file.open(QFile::ReadOnly)) {
                const QString code = QString::fromUtf8(file.readAll());
                file.close();

                try {
                    QV4::Script script(ctx, code, fn);
                    script.parseAsBinding = runAsQml;
                    script.parse();
                    QV4::Value result = script.run();
                    if (!result.isUndefined()) {
                        if (! qgetenv("SHOW_EXIT_VALUE").isEmpty())
                            std::cout << "exit value: " << qPrintable(result.toString(ctx)->toQString()) << std::endl;
                    }
                } catch (QV4::Exception& ex) {
                    ex.accept(ctx);
                    showException(ctx, ex);
                    return EXIT_FAILURE;
                }

            } else {
                std::cerr << "Error: cannot open file " << fn.toUtf8().constData() << std::endl;
                return EXIT_FAILURE;
            }
        }

        vm.memoryManager->dumpStats();
    } return EXIT_SUCCESS;
    } // switch (mode)
}
