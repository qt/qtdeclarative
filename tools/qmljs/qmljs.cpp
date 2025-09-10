// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "private/qv4object_p.h"
#include "private/qv4runtime_p.h"
#include "private/qv4functionobject_p.h"
#include "private/qv4errorobject_p.h"
#include "private/qv4globalobject_p.h"
#include "private/qv4codegen_p.h"
#include "private/qv4objectproto_p.h"
#include "private/qv4mm_p.h"
#include "private/qv4context_p.h"
#include "private/qv4script_p.h"
#include "private/qv4string_p.h"
#include "private/qv4module_p.h"
#include "private/qqmlbuiltinfunctions_p.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QDateTime>
#include <QtCore/qcommandlineparser.h>
#include <private/qqmljsengine_p.h>
#include <private/qqmljslexer_p.h>
#include <private/qqmljsparser_p.h>
#include <private/qqmljsast_p.h>

#include <iostream>

static void showException(QV4::ExecutionContext *ctx, const QV4::Value &exception, const QV4::StackTrace &trace)
{
    QV4::Scope scope(ctx);
    QV4::ScopedValue ex(scope, exception);
    QV4::ErrorObject *e = ex->as<QV4::ErrorObject>();
    if (!e) {
        std::cerr << "Uncaught exception: " << qPrintable(ex->toQString()) << std::endl;
    } else {
        std::cerr << "Uncaught exception: " << qPrintable(e->toQStringNoThrow()) << std::endl;
    }

    for (const QV4::StackFrame &frame : trace) {
        std::cerr << "    at " << qPrintable(frame.function) << " (" << qPrintable(frame.source);
        if (frame.line >= 0)
            std::cerr << ':' << frame.line;
        std::cerr << ')' << std::endl;
    }
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationVersion(QLatin1String(QT_VERSION_STR));

    QCommandLineParser parser;
    parser.addHelpOption();
    parser.setApplicationDescription("Utility to execute scripts in QML's V4 engine");
    parser.addVersionOption();
    parser.addPositionalArgument("files", "Files to execute.", "[files...]");


    QCommandLineOption forceJit("jit", "Force JIT.");
    parser.addOption(forceJit);

    QCommandLineOption forceInterpreter("interpret", "Force interpreter.");
    parser.addOption(forceInterpreter);

    QCommandLineOption qml("qml", "Run as QML.");
    parser.addOption(qml);

    QCommandLineOption module("module", "Run as Module.");
    parser.addOption(module);

    QCommandLineOption cache("cache", "Use cache.");
    parser.addOption(cache);

    parser.process(app);

    bool jitEnabled = false;

    if (parser.isSet(forceJit)) {
        qputenv("QV4_JIT_CALL_THRESHOLD", QByteArray("0"));
        jitEnabled = true;
    }
    if (parser.isSet(forceInterpreter)) {
        qputenv("QV4_FORCE_INTERPRETER", QByteArray("1"));
        if (jitEnabled) {
            std::cerr << "You cannot use 'Force JIT' and 'Force Interpreter' at the same time.";
            return EXIT_FAILURE;
        }
    }
    const bool runAsQml = parser.isSet(qml);
    const bool runAsModule = parser.isSet(module);
    const bool useCache = parser.isSet(cache);
    const QStringList args = parser.positionalArguments();

    QV4::ExecutionEngine vm;

    QV4::Scope scope(&vm);
    QV4::ScopedContext ctx(scope, vm.rootContext());

    QV4::GlobalExtensions::init(vm.globalObject, QJSEngine::ConsoleExtension | QJSEngine::GarbageCollectionExtension);

    for (const QString &fn : args) {
        QV4::ScopedValue result(scope);
        if (runAsModule) {
            if (auto module
                    = vm.loadModule(QUrl::fromLocalFile(QFileInfo(fn).absoluteFilePath()))) {
                if (module->instantiate())
                    module->evaluate();
            } else {
                vm.throwError(QStringLiteral("Could not load module file"));
            }
        } else {
            QFile file(fn);
            if (!file.open(QFile::ReadOnly)) {
                std::cerr << "Error: cannot open file " << fn.toUtf8().constData() << std::endl;
                return EXIT_FAILURE;
            }
            QScopedPointer<QV4::Script> script;
            if (useCache && QFile::exists(fn + QLatin1Char('c'))) {
                auto unit = QQml::makeRefPointer<QV4::CompiledData::CompilationUnit>();
                QString error;
                if (unit->loadFromDisk(QUrl::fromLocalFile(fn), QFileInfo(fn).lastModified(), &error)) {
                    script.reset(new QV4::Script(
                            &vm, nullptr, vm.insertCompilationUnit(std::move(unit))));
                } else {
                    std::cout << "Error loading" << qPrintable(fn) << "from disk cache:" << qPrintable(error) << std::endl;
                }
            }
            if (!script) {
                QByteArray ba = file.readAll();
                const QString code = QString::fromUtf8(ba.constData(), ba.size());
                file.close();

                script.reset(new QV4::Script(ctx, QV4::Compiler::ContextType::Global, code, fn));
                script->parseAsBinding = runAsQml;
                script->parse();
            }
            if (!scope.hasException()) {
                const auto unit = script->compilationUnit;
                if (useCache && unit && !(unit->unitData()->flags & QV4::CompiledData::Unit::StaticData)) {
                    if (unit->unitData()->sourceTimeStamp == 0) {
                        const_cast<QV4::CompiledData::Unit*>(unit->unitData())->sourceTimeStamp = QFileInfo(fn).lastModified().toMSecsSinceEpoch();
                    }
                    QString saveError;
                    if (!unit->baseCompilationUnit()->saveToDisk(
                                QUrl::fromLocalFile(fn), &saveError)) {
                        std::cout << "Error saving JS cache file: " << qPrintable(saveError) << std::endl;
                    }
                }
//                QElapsedTimer t; t.start();
                result = script->run();
//                std::cout << t.elapsed() << " ms. elapsed" << std::endl;
            }
        }
        if (scope.hasException()) {
            QV4::StackTrace trace;
            QV4::ScopedValue ex(scope, scope.engine->catchException(&trace));
            showException(ctx, ex, trace);
            return EXIT_FAILURE;
        }
        if (!result->isUndefined()) {
            if (! qgetenv("SHOW_EXIT_VALUE").isEmpty())
                std::cout << "exit value: " << qPrintable(result->toQString()) << std::endl;
        }
    }

    return EXIT_SUCCESS;
}
