#include "debugging.h"
#include "qmljs_engine.h"
#include "qv4ir_p.h"
#include "qv4isel_p.h"
#include "qv4isel_util_p.h"
#include "qv4functionobject.h"

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
    vmFunction->hasNestedFunctions = !irFunction->nestedFunctions.isEmpty();
    vmFunction->isStrict = irFunction->isStrict;

    foreach (const QString *formal, irFunction->formals)
        if (formal)
            vmFunction->formals.append(*formal);
    foreach (const QString *local, irFunction->locals)
        if (local)
            vmFunction->locals.append(*local);

    foreach (IR::Function *function, irFunction->nestedFunctions)
        createFunctionMapping(engine, function);


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
                if (Name *func = ctor->base->asName()) {
                    constructActivationProperty(func, ctor->args, t);
                    return;
                } else if (IR::Member *member = ctor->base->asMember()) {
                    constructProperty(member->base->asTemp(), *member->name, ctor->args, t);
                    return;
                } else if (IR::Temp *value = ctor->base->asTemp()) {
                    constructValue(value, ctor->args, t);
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
                    callBuiltin(c, t);
                    return;
                } else if (Member *member = c->base->asMember()) {
                    callProperty(member->base, *member->name, c->args, t);
                    return;
                } else if (Subscript *s = c->base->asSubscript()) {
                    callSubscript(s->base, s->index, c->args, t);
                    return;
                } else if (IR::Temp *value = c->base->asTemp()) {
                    callValue(value, c->args, t);
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
            callBuiltin(c, 0);
        } else if (Temp *value = c->base->asTemp()) {
            callValue(value, c->args, 0);
        } else if (Member *member = c->base->asMember()) {
            callProperty(member->base, *member->name, c->args, 0);
        } else if (Subscript *s = c->base->asSubscript()) {
            callSubscript(s->base, s->index, c->args, 0);
        } else {
            Q_UNIMPLEMENTED();
        }
    } else {
        Q_UNIMPLEMENTED();
    }
}

void InstructionSelection::callBuiltin(IR::Call *call, IR::Temp *result)
{
    IR::Name *baseName = call->base->asName();
    assert(baseName != 0);

    switch (baseName->builtin) {
    case IR::Name::builtin_invalid:
        callBuiltinInvalid(baseName, call->args, result);
        return;

    case IR::Name::builtin_typeof: {
        if (IR::Member *m = call->args->expr->asMember()) {
            callBuiltinTypeofMember(m->base->asTemp(), *m->name, result);
            return;
        } else if (IR::Subscript *ss = call->args->expr->asSubscript()) {
            callBuiltinTypeofSubscript(ss->base->asTemp(), ss->index->asTemp(), result);
            return;
        } else if (IR::Name *n = call->args->expr->asName()) {
            callBuiltinTypeofName(*n->id, result);
            return;
        } else if (IR::Temp *arg = call->args->expr->asTemp()){
            assert(arg != 0);
            callBuiltinTypeofValue(arg, result);
            return;
        }
    } break;

    case IR::Name::builtin_delete: {
        if (IR::Member *m = call->args->expr->asMember()) {
            callBuiltinDeleteMember(m->base->asTemp(), *m->name, result);
            return;
        } else if (IR::Subscript *ss = call->args->expr->asSubscript()) {
            callBuiltinDeleteSubscript(ss->base->asTemp(), ss->index->asTemp(), result);
            return;
        } else if (IR::Name *n = call->args->expr->asName()) {
            callBuiltinDeleteName(*n->id, result);
            return;
        } else if (call->args->expr->asTemp()){
            // TODO: should throw in strict mode
            callBuiltinDeleteValue(result);
            return;
        }
    } break;

    case IR::Name::builtin_throw: {
        IR::Temp *arg = call->args->expr->asTemp();
        assert(arg != 0);
        callBuiltinThrow(arg);
    } return;

    case IR::Name::builtin_create_exception_handler:
        callBuiltinCreateExceptionHandler(result);
        return;

    case IR::Name::builtin_delete_exception_handler:
        callBuiltinDeleteExceptionHandler();
        return;

    case IR::Name::builtin_get_exception:
        callBuiltinGetException(result);
        return;

    case IR::Name::builtin_foreach_iterator_object: {
        IR::Temp *arg = call->args->expr->asTemp();
        assert(arg != 0);
        callBuiltinForeachIteratorObject(arg, result);
    } return;

    case IR::Name::builtin_foreach_next_property_name: {
        IR::Temp *arg = call->args->expr->asTemp();
        assert(arg != 0);
        callBuiltinForeachNextPropertyname(arg, result);
    } return;
    case IR::Name::builtin_push_with_scope: {
        IR::Temp *arg = call->args->expr->asTemp();
        assert(arg != 0);
        callBuiltinPushWithScope(arg);
    } return;

    case IR::Name::builtin_pop_scope:
        callBuiltinPopScope();
        return;

    case IR::Name::builtin_declare_vars: {
        if (!call->args)
            return;
        IR::Const *deletable = call->args->expr->asConst();
        assert(deletable->type == IR::BoolType);
        for (IR::ExprList *it = call->args->next; it; it = it->next) {
            IR::Name *arg = it->expr->asName();
            assert(arg != 0);
            callBuiltinDeclareVar(deletable->value != 0, *arg->id);
        }
    } return;

    case IR::Name::builtin_define_getter_setter: {
        if (!call->args)
            return;
        IR::ExprList *args = call->args;
        IR::Temp *object = args->expr->asTemp();
        assert(object);
        args = args->next;
        assert(args);
        IR::Name *name = args->expr->asName();
        args = args->next;
        assert(args);
        IR::Temp *getter = args->expr->asTemp();
        args = args->next;
        assert(args);
        IR::Temp *setter = args->expr->asTemp();

        callBuiltinDefineGetterSetter(object, *name->id, getter, setter);
    } return;

    case IR::Name::builtin_define_property: {
        if (!call->args)
            return;
        IR::ExprList *args = call->args;
        IR::Temp *object = args->expr->asTemp();
        assert(object);
        args = args->next;
        assert(args);
        IR::Name *name = args->expr->asName();
        args = args->next;
        assert(args);
        IR::Temp *value = args->expr->asTemp();

        callBuiltinDefineProperty(object, *name->id, value);
    } return;

    case IR::Name::builtin_define_array_property: {
        if (!call->args)
            return;
        IR::ExprList *args = call->args;
        IR::Temp *object = args->expr->asTemp();
        assert(object);
        args = args->next;
        assert(args);
        IR::Const *index = args->expr->asConst();
        args = args->next;
        assert(args);
        IR::Temp *value = args->expr->asTemp();

        callBuiltinDefineArrayProperty(object, int(index->value), value);
    } return;

    default:
        break;
    }

    Q_UNIMPLEMENTED();
    call->dump(qout); qout << endl;
    assert(!"TODO!");
    Q_UNREACHABLE();
}
