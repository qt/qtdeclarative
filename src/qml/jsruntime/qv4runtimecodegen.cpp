// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qv4engine_p.h"
#include "qv4runtimecodegen_p.h"
#include <private/qv4compilerscanfunctions_p.h>

using namespace QV4;
using namespace QQmlJS;

void RuntimeCodegen::generateFromFunctionExpression(const QString &fileName,
                                                    const QString &sourceCode,
                                                    AST::FunctionExpression *ast,
                                                    Compiler::Module *module)
{
    _module = module;
    _module->fileName = fileName;
    _module->finalUrl = fileName;
    _context = nullptr;

    Compiler::ScanFunctions scan(this, sourceCode, Compiler::ContextType::Global);
    // fake a global environment
    scan.enterEnvironment(nullptr, Compiler::ContextType::Function, QString());
    scan(ast);
    scan.leaveEnvironment();

    if (hasError())
        return;

    int index = defineFunction(ast->name.toString(), ast, ast->formals, ast->body);
    _module->rootContext = _module->functions.at(index);
}

void RuntimeCodegen::throwSyntaxError(const SourceLocation &loc, const QString &detail)
{
    if (hasError())
        return;

    Codegen::throwSyntaxError(loc, detail);
    engine->throwSyntaxError(detail, _module->fileName, loc.startLine, loc.startColumn);
}

void RuntimeCodegen::throwReferenceError(const SourceLocation &loc, const QString &detail)
{
    if (hasError())
        return;

    Codegen::throwReferenceError(loc, detail);
    engine->throwReferenceError(detail, _module->fileName, loc.startLine, loc.startColumn);
}

