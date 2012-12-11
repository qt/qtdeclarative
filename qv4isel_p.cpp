#include "debugging.h"
#include "qmljs_engine.h"
#include "qv4ir_p.h"
#include "qv4isel_p.h"
#include "qv4isel_util_p.h"

#include <QString>

#include <cassert>

using namespace QQmlJS;

EvalInstructionSelection::EvalInstructionSelection(VM::ExecutionEngine *engine, IR::Module *module)
    : _engine(engine)
{
    assert(engine);
    assert(module);

    foreach (IR::Function *f, module->functions)
        _irToVM.insert(f, createFunctionMapping(engine, f));
}

EvalInstructionSelection::~EvalInstructionSelection()
{}

EvalISelFactory::~EvalISelFactory()
{}

VM::Function *EvalInstructionSelection::createFunctionMapping(VM::ExecutionEngine *engine, IR::Function *irFunction)
{
    VM::Function *vmFunction = engine->newFunction(irFunction->name ? *irFunction->name : QString());
    vmFunction->hasDirectEval = irFunction->hasDirectEval;
    vmFunction->isStrict = irFunction->isStrict;

    foreach (const QString *formal, irFunction->formals)
        if (formal)
            vmFunction->formals.append(*formal);
    foreach (const QString *local, irFunction->locals)
        if (local)
            vmFunction->locals.append(*local);

    if (engine->debugger)
        engine->debugger->mapFunction(vmFunction, irFunction);

    return vmFunction;
}
