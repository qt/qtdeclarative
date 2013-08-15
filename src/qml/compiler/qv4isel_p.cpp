/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtQml module of the Qt Toolkit.
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

#include "qv4debugging_p.h"
#include "qv4engine_p.h"
#include "qv4jsir_p.h"
#include "qv4isel_p.h"
#include "qv4isel_util_p.h"
#include "qv4functionobject_p.h"
#include "qv4function_p.h"

#include <QString>

#include <cassert>

namespace {
QTextStream qout(stderr, QIODevice::WriteOnly);
} // anonymous namespace

using namespace QQmlJS;
using namespace QQmlJS::V4IR;

EvalInstructionSelection::EvalInstructionSelection(QV4::ExecutionEngine *engine, Module *module)
    : _engine(engine)
    , useFastLookups(true)
    , jsUnitGenerator(engine, module)
{
    assert(engine);
    assert(module);

    createFunctionMapping(0, module->rootFunction);
    foreach (V4IR::Function *f, module->functions) {
        assert(_irToVM.contains(f));
    }
}

EvalInstructionSelection::~EvalInstructionSelection()
{}

EvalISelFactory::~EvalISelFactory()
{}

QV4::Function *EvalInstructionSelection::createFunctionMapping(QV4::Function *outer, Function *irFunction)
{
    QV4::Function *vmFunction = _engine->newFunction(irFunction->name ? *irFunction->name : QString());
    _irToVM.insert(irFunction, vmFunction);

    if (outer)
        outer->addNestedFunction(vmFunction);

    foreach (V4IR::Function *function, irFunction->nestedFunctions)
        createFunctionMapping(vmFunction, function);

    return vmFunction;
}

QV4::CompiledData::CompilationUnit *EvalInstructionSelection::compile()
{
    Function *rootFunction = jsUnitGenerator.irModule->rootFunction;
    if (!rootFunction)
        return 0;
    for (QHash<V4IR::Function*, QV4::Function*>::Iterator it = _irToVM.begin(), end = _irToVM.end();
         it != end; ++it) {
        if (!(*it)->code)
            run(it.value(), it.key());
    }

    return backendCompileStep();
}

void IRDecoder::visitMove(V4IR::Move *s)
{
    if (s->op == V4IR::OpInvalid) {
        if (V4IR::Name *n = s->target->asName()) {
            if (s->source->asTemp()) {
                setActivationProperty(s->source->asTemp(), *n->id);
                return;
            }
        } else if (V4IR::Temp *t = s->target->asTemp()) {
            if (V4IR::Name *n = s->source->asName()) {
                if (*n->id == QStringLiteral("this")) // TODO: `this' should be a builtin.
                    loadThisObject(t);
                else
                    getActivationProperty(n, t);
                return;
            } else if (V4IR::Const *c = s->source->asConst()) {
                loadConst(c, t);
                return;
            } else if (V4IR::Temp *t2 = s->source->asTemp()) {
                copyValue(t2, t);
                return;
            } else if (V4IR::String *str = s->source->asString()) {
                loadString(*str->value, t);
                return;
            } else if (V4IR::RegExp *re = s->source->asRegExp()) {
                loadRegexp(re, t);
                return;
            } else if (V4IR::Closure *clos = s->source->asClosure()) {
                initClosure(clos, t);
                return;
            } else if (V4IR::New *ctor = s->source->asNew()) {
                if (Name *func = ctor->base->asName()) {
                    constructActivationProperty(func, ctor->args, t);
                    return;
                } else if (V4IR::Member *member = ctor->base->asMember()) {
                    constructProperty(member->base->asTemp(), *member->name, ctor->args, t);
                    return;
                } else if (V4IR::Temp *value = ctor->base->asTemp()) {
                    constructValue(value, ctor->args, t);
                    return;
                }
            } else if (V4IR::Member *m = s->source->asMember()) {
                if (V4IR::Temp *base = m->base->asTemp()) {
                    getProperty(base, *m->name, t);
                    return;
                }
            } else if (V4IR::Subscript *ss = s->source->asSubscript()) {
                getElement(ss->base->asTemp(), ss->index->asTemp(), t);
                return;
            } else if (V4IR::Unop *u = s->source->asUnop()) {
                if (V4IR::Temp *e = u->expr->asTemp()) {
                    unop(u->op, e, t);
                    return;
                }
            } else if (V4IR::Binop *b = s->source->asBinop()) {
                binop(b->op, b->left, b->right, t);
                return;
            } else if (V4IR::Call *c = s->source->asCall()) {
                if (c->base->asName()) {
                    callBuiltin(c, t);
                    return;
                } else if (Member *member = c->base->asMember()) {
                    Q_ASSERT(member->base->asTemp());
                    callProperty(member->base->asTemp(), *member->name, c->args, t);
                    return;
                } else if (Subscript *s = c->base->asSubscript()) {
                    Q_ASSERT(s->base->asTemp());
                    Q_ASSERT(s->index->asTemp());
                    callSubscript(s->base->asTemp(), s->index->asTemp(), c->args, t);
                    return;
                } else if (V4IR::Temp *value = c->base->asTemp()) {
                    callValue(value, c->args, t);
                    return;
                }
            } else if (V4IR::Convert *c = s->source->asConvert()) {
                Q_ASSERT(c->expr->asTemp());
                convertType(c->expr->asTemp(), t);
                return;
            }
        } else if (V4IR::Member *m = s->target->asMember()) {
            if (V4IR::Temp *base = m->base->asTemp()) {
                if (s->source->asTemp()) {
                    setProperty(s->source->asTemp(), base, *m->name);
                    return;
                }
            }
        } else if (V4IR::Subscript *ss = s->target->asSubscript()) {
            if (s->source->asTemp()) {
                setElement(s->source->asTemp(), ss->base->asTemp(), ss->index->asTemp());
                return;
            }
        }
    } else {
        // inplace assignment, e.g. x += 1, ++x, ...
        if (V4IR::Temp *t = s->target->asTemp()) {
            if (s->source->asTemp()) {
                binop(s->op, t, s->source->asTemp(), t);
                return;
            }
        } else if (V4IR::Name *n = s->target->asName()) {
            if (s->source->asTemp()) {
                inplaceNameOp(s->op, s->source->asTemp(), *n->id);
                return;
            }
        } else if (V4IR::Subscript *ss = s->target->asSubscript()) {
            if (s->source->asTemp()) {
                inplaceElementOp(s->op, s->source->asTemp(), ss->base->asTemp(),
                                 ss->index->asTemp());
                return;
            }
        } else if (V4IR::Member *m = s->target->asMember()) {
            if (s->source->asTemp()) {
                inplaceMemberOp(s->op, s->source->asTemp(), m->base->asTemp(), *m->name);
                return;
            }
        }
    }

    // For anything else...:
    Q_UNIMPLEMENTED();
    s->dump(qout, V4IR::Stmt::MIR);
    qout << endl;
    assert(!"TODO");
}

IRDecoder::~IRDecoder()
{
}

void IRDecoder::visitExp(V4IR::Exp *s)
{
    if (V4IR::Call *c = s->expr->asCall()) {
        // These are calls where the result is ignored.
        if (c->base->asName()) {
            callBuiltin(c, 0);
        } else if (Temp *value = c->base->asTemp()) {
            callValue(value, c->args, 0);
        } else if (Member *member = c->base->asMember()) {
            Q_ASSERT(member->base->asTemp());
            callProperty(member->base->asTemp(), *member->name, c->args, 0);
        } else if (Subscript *s = c->base->asSubscript()) {
            Q_ASSERT(s->base->asTemp());
            Q_ASSERT(s->index->asTemp());
            callSubscript(s->base->asTemp(), s->index->asTemp(), c->args, 0);
        } else {
            Q_UNIMPLEMENTED();
        }
    } else {
        Q_UNIMPLEMENTED();
    }
}

void IRDecoder::callBuiltin(V4IR::Call *call, V4IR::Temp *result)
{
    V4IR::Name *baseName = call->base->asName();
    assert(baseName != 0);

    switch (baseName->builtin) {
    case V4IR::Name::builtin_invalid:
        callBuiltinInvalid(baseName, call->args, result);
        return;

    case V4IR::Name::builtin_typeof: {
        if (V4IR::Member *m = call->args->expr->asMember()) {
            callBuiltinTypeofMember(m->base->asTemp(), *m->name, result);
            return;
        } else if (V4IR::Subscript *ss = call->args->expr->asSubscript()) {
            callBuiltinTypeofSubscript(ss->base->asTemp(), ss->index->asTemp(), result);
            return;
        } else if (V4IR::Name *n = call->args->expr->asName()) {
            callBuiltinTypeofName(*n->id, result);
            return;
        } else if (V4IR::Temp *arg = call->args->expr->asTemp()){
            assert(arg != 0);
            callBuiltinTypeofValue(arg, result);
            return;
        }
    } break;

    case V4IR::Name::builtin_delete: {
        if (V4IR::Member *m = call->args->expr->asMember()) {
            callBuiltinDeleteMember(m->base->asTemp(), *m->name, result);
            return;
        } else if (V4IR::Subscript *ss = call->args->expr->asSubscript()) {
            callBuiltinDeleteSubscript(ss->base->asTemp(), ss->index->asTemp(), result);
            return;
        } else if (V4IR::Name *n = call->args->expr->asName()) {
            callBuiltinDeleteName(*n->id, result);
            return;
        } else if (call->args->expr->asTemp()){
            // TODO: should throw in strict mode
            callBuiltinDeleteValue(result);
            return;
        }
    } break;

    case V4IR::Name::builtin_postincrement: {
        if (V4IR::Member *m = call->args->expr->asMember()) {
            callBuiltinPostIncrementMember(m->base->asTemp(), *m->name, result);
            return;
        } else if (V4IR::Subscript *ss = call->args->expr->asSubscript()) {
            callBuiltinPostIncrementSubscript(ss->base->asTemp(), ss->index->asTemp(), result);
            return;
        } else if (V4IR::Name *n = call->args->expr->asName()) {
            callBuiltinPostIncrementName(*n->id, result);
            return;
        } else if (V4IR::Temp *arg = call->args->expr->asTemp()){
            assert(arg != 0);
            callBuiltinPostIncrementValue(arg, result);
            return;
        }
    } break;

    case V4IR::Name::builtin_postdecrement: {
        if (V4IR::Member *m = call->args->expr->asMember()) {
            callBuiltinPostDecrementMember(m->base->asTemp(), *m->name, result);
            return;
        } else if (V4IR::Subscript *ss = call->args->expr->asSubscript()) {
            callBuiltinPostDecrementSubscript(ss->base->asTemp(), ss->index->asTemp(), result);
            return;
        } else if (V4IR::Name *n = call->args->expr->asName()) {
            callBuiltinPostDecrementName(*n->id, result);
            return;
        } else if (V4IR::Temp *arg = call->args->expr->asTemp()){
            assert(arg != 0);
            callBuiltinPostDecrementValue(arg, result);
            return;
        }
    } break;

    case V4IR::Name::builtin_throw: {
        V4IR::Temp *arg = call->args->expr->asTemp();
        assert(arg != 0);
        callBuiltinThrow(arg);
    } return;

    case V4IR::Name::builtin_finish_try:
        callBuiltinFinishTry();
        return;

    case V4IR::Name::builtin_foreach_iterator_object: {
        V4IR::Temp *arg = call->args->expr->asTemp();
        assert(arg != 0);
        callBuiltinForeachIteratorObject(arg, result);
    } return;

    case V4IR::Name::builtin_foreach_next_property_name: {
        V4IR::Temp *arg = call->args->expr->asTemp();
        assert(arg != 0);
        callBuiltinForeachNextPropertyname(arg, result);
    } return;
    case V4IR::Name::builtin_push_with_scope: {
        V4IR::Temp *arg = call->args->expr->asTemp();
        assert(arg != 0);
        callBuiltinPushWithScope(arg);
    } return;

    case V4IR::Name::builtin_pop_scope:
        callBuiltinPopScope();
        return;

    case V4IR::Name::builtin_declare_vars: {
        if (!call->args)
            return;
        V4IR::Const *deletable = call->args->expr->asConst();
        assert(deletable->type == V4IR::BoolType);
        for (V4IR::ExprList *it = call->args->next; it; it = it->next) {
            V4IR::Name *arg = it->expr->asName();
            assert(arg != 0);
            callBuiltinDeclareVar(deletable->value != 0, *arg->id);
        }
    } return;

    case V4IR::Name::builtin_define_getter_setter: {
        if (!call->args)
            return;
        V4IR::ExprList *args = call->args;
        V4IR::Temp *object = args->expr->asTemp();
        assert(object);
        args = args->next;
        assert(args);
        V4IR::Name *name = args->expr->asName();
        args = args->next;
        assert(args);
        V4IR::Temp *getter = args->expr->asTemp();
        args = args->next;
        assert(args);
        V4IR::Temp *setter = args->expr->asTemp();

        callBuiltinDefineGetterSetter(object, *name->id, getter, setter);
    } return;

    case V4IR::Name::builtin_define_property: {
        if (!call->args)
            return;
        V4IR::ExprList *args = call->args;
        V4IR::Temp *object = args->expr->asTemp();
        assert(object);
        args = args->next;
        assert(args);
        V4IR::Name *name = args->expr->asName();
        args = args->next;
        assert(args);
        V4IR::Temp *value = args->expr->asTemp();

        callBuiltinDefineProperty(object, *name->id, value);
    } return;

    case V4IR::Name::builtin_define_array:
        callBuiltinDefineArray(result, call->args);
        return;

    case V4IR::Name::builtin_define_object_literal:
        callBuiltinDefineObjectLiteral(result, call->args);
        return;

    default:
        break;
    }

    Q_UNIMPLEMENTED();
    call->dump(qout); qout << endl;
    assert(!"TODO!");
    Q_UNREACHABLE();
}
