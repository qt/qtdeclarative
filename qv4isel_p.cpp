#include "debugging.h"
#include "qmljs_engine.h"
#include "qv4ir_p.h"
#include "qv4isel_p.h"
#include "qv4isel_util_p.h"

#include <QString>

#include <cassert>

namespace {
QTextStream qout(stderr, QIODevice::WriteOnly);
} // anonymous namespace

using namespace QQmlJS;
using namespace QQmlJS::IR;

EvalInstructionSelection::EvalInstructionSelection(VM::ExecutionEngine *engine, Module *module)
    : _engine(engine)
{
    assert(engine);
    assert(module);

    createFunctionMapping(engine, module->rootFunction);
    foreach (IR::Function *f, module->functions) {
        assert(_irToVM.contains(f));
    }
}

EvalInstructionSelection::~EvalInstructionSelection()
{}

EvalISelFactory::~EvalISelFactory()
{}

VM::Function *EvalInstructionSelection::createFunctionMapping(VM::ExecutionEngine *engine, Function *irFunction)
{
    VM::Function *vmFunction = engine->newFunction(irFunction->name ? *irFunction->name : QString());
    _irToVM.insert(irFunction, vmFunction);

    vmFunction->hasDirectEval = irFunction->hasDirectEval;
    vmFunction->usesArgumentsObject = irFunction->usesArgumentsObject;
    vmFunction->isStrict = irFunction->isStrict;

    foreach (const QString *formal, irFunction->formals)
        if (formal)
            vmFunction->formals.append(*formal);
    foreach (const QString *local, irFunction->locals)
        if (local)
            vmFunction->locals.append(*local);

    foreach (IR::Function *function, irFunction->nestedFunctions)
        vmFunction->nestedFunctions.append(createFunctionMapping(engine, function));

    if (engine->debugger)
        engine->debugger->mapFunction(vmFunction, irFunction);

    return vmFunction;
}

VM::Function *EvalInstructionSelection::vmFunction(Function *f) {
    VM::Function *function = _irToVM[f];
    if (!function->code)
        run(function, f);
    return function;
}

void InstructionSelection::visitMove(IR::Move *s)
{
    if (s->op == IR::OpInvalid) {
        if (IR::Name *n = s->target->asName()) {
            if (s->source->asTemp() || s->source->asConst()) {
                setActivationProperty(s->source, *n->id);
                return;
            }
        } else if (IR::Temp *t = s->target->asTemp()) {
            if (IR::Name *n = s->source->asName()) {
                if (*n->id == QStringLiteral("this")) // TODO: `this' should be a builtin.
                    loadThisObject(t);
                else
                    getActivationProperty(*n->id, t);
                return;
            } else if (IR::Const *c = s->source->asConst()) {
                loadConst(c, t);
                return;
            } else if (IR::Temp *t2 = s->source->asTemp()) {
                copyValue(t2, t);
                return;
            } else if (IR::String *str = s->source->asString()) {
                loadString(*str->value, t);
                return;
            } else if (IR::RegExp *re = s->source->asRegExp()) {
                loadRegexp(re, t);
                return;
            } else if (IR::Closure *clos = s->source->asClosure()) {
                initClosure(clos, t);
                return;
            } else if (IR::New *ctor = s->source->asNew()) {
                if (ctor->base->asName()) {
                    constructActivationProperty(ctor, t);
                    return;
                } else if (ctor->base->asMember()) {
                    constructProperty(ctor, t);
                    return;
                } else if (ctor->base->asTemp()) {
                    constructValue(ctor, t);
                    return;
                }
            } else if (IR::Member *m = s->source->asMember()) {
                if (IR::Temp *base = m->base->asTemp()) {
                    getProperty(base, *m->name, t);
                    return;
                }
            } else if (IR::Subscript *ss = s->source->asSubscript()) {
                getElement(ss->base->asTemp(), ss->index->asTemp(), t);
                return;
            } else if (IR::Unop *u = s->source->asUnop()) {
                if (IR::Temp *e = u->expr->asTemp()) {
                    unop(u->op, e, t);
                    return;
                }
            } else if (IR::Binop *b = s->source->asBinop()) {
                if ((b->left->asTemp() || b->left->asConst())
                        && (b->right->asTemp() || b->right->asConst())) {
                    binop(b->op, b->left, b->right, t);
                    return;
                }
            } else if (IR::Call *c = s->source->asCall()) {
                if (c->base->asName()) {
                    callActivationProperty(c, t);
                    return;
                } else if (c->base->asMember()) {
                    callProperty(c, t);
                    return;
                } else if (c->base->asTemp()) {
                    callValue(c, t);
                    return;
                }
            }
        } else if (IR::Member *m = s->target->asMember()) {
            if (IR::Temp *base = m->base->asTemp()) {
                if (s->source->asTemp() || s->source->asConst()) {
                    setProperty(s->source, base, *m->name);
                    return;
                }
            }
        } else if (IR::Subscript *ss = s->target->asSubscript()) {
            if (s->source->asTemp() || s->source->asConst()) {
                setElement(s->source, ss->base->asTemp(), ss->index->asTemp());
                return;
            }
        }
    } else {
        // inplace assignment, e.g. x += 1, ++x, ...
        if (IR::Temp *t = s->target->asTemp()) {
            if (s->source->asTemp() || s->source->asConst()) {
                binop(s->op, t, s->source, t);
                return;
            }
        } else if (IR::Name *n = s->target->asName()) {
            if (s->source->asTemp() || s->source->asConst()) {
                inplaceNameOp(s->op, s->source, *n->id);
                return;
            }
        } else if (IR::Subscript *ss = s->target->asSubscript()) {
            if (s->source->asTemp() || s->source->asConst()) {
                inplaceElementOp(s->op, s->source, ss->base->asTemp(),
                                 ss->index->asTemp());
                return;
            }
        } else if (IR::Member *m = s->target->asMember()) {
            if (s->source->asTemp() || s->source->asConst()) {
                inplaceMemberOp(s->op, s->source, m->base->asTemp(), *m->name);
                return;
            }
        }
    }

    // For anything else...:
    Q_UNIMPLEMENTED();
    s->dump(qout, IR::Stmt::MIR);
    qout << endl;
    assert(!"TODO");
}

InstructionSelection::~InstructionSelection()
{
}

void InstructionSelection::visitEnter(Enter *)
{
    Q_UNREACHABLE();
}

void InstructionSelection::visitLeave(Leave *)
{
    Q_UNREACHABLE();
}

void InstructionSelection::visitExp(IR::Exp *s)
{
    if (IR::Call *c = s->expr->asCall()) {
        // These are calls where the result is ignored.
        if (c->base->asName()) {
            callActivationProperty(c, 0);
        } else if (c->base->asTemp()) {
            callValue(c, 0);
        } else if (c->base->asMember()) {
            callProperty(c, 0);
        } else {
            Q_UNREACHABLE();
        }
    } else {
        Q_UNREACHABLE();
    }
}
