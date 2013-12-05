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
#include <private/qqmlpropertycache_p.h>

#include <QString>

#include <cassert>

namespace {
QTextStream qout(stderr, QIODevice::WriteOnly);
} // anonymous namespace

using namespace QQmlJS;
using namespace QQmlJS::V4IR;

EvalInstructionSelection::EvalInstructionSelection(QV4::ExecutableAllocator *execAllocator, Module *module, QV4::Compiler::JSUnitGenerator *jsGenerator)
    : useFastLookups(true)
    , executableAllocator(execAllocator)
    , irModule(module)
{
    if (!jsGenerator) {
        jsGenerator = new QV4::Compiler::JSUnitGenerator(module);
        ownJSGenerator.reset(jsGenerator);
    }
    this->jsGenerator = jsGenerator;
    assert(execAllocator);
    assert(module);
}

EvalInstructionSelection::~EvalInstructionSelection()
{}

EvalISelFactory::~EvalISelFactory()
{}

QV4::CompiledData::CompilationUnit *EvalInstructionSelection::compile(bool generateUnitData)
{
    for (int i = 0; i < irModule->functions.size(); ++i)
        run(i);

    QV4::CompiledData::CompilationUnit *unit = backendCompileStep();
    if (generateUnitData) {
        unit->data = jsGenerator->generateUnit();
        unit->ownsData = true;
    }
    return unit;
}

void IRDecoder::visitMove(V4IR::Move *s)
{
    if (V4IR::Name *n = s->target->asName()) {
        if (s->source->asTemp() || s->source->asConst()) {
            setActivationProperty(s->source, *n->id);
            return;
        }
    } else if (V4IR::Temp *t = s->target->asTemp()) {
        if (V4IR::Name *n = s->source->asName()) {
            if (n->id && *n->id == QStringLiteral("this")) // TODO: `this' should be a builtin.
                loadThisObject(t);
            else if (n->builtin == V4IR::Name::builtin_qml_id_array)
                loadQmlIdArray(t);
            else if (n->builtin == V4IR::Name::builtin_qml_context_object)
                loadQmlContextObject(t);
            else if (n->builtin == V4IR::Name::builtin_qml_scope_object)
                loadQmlScopeObject(t);
            else if (n->builtin == V4IR::Name::builtin_qml_imported_scripts_object)
                loadQmlImportedScripts(t);
            else if (n->qmlSingleton)
                loadQmlSingleton(*n->id, t);
            else
                getActivationProperty(n, t);
            return;
        } else if (V4IR::Const *c = s->source->asConst()) {
            loadConst(c, t);
            return;
        } else if (V4IR::Temp *t2 = s->source->asTemp()) {
            if (s->swap)
                swapValues(t2, t);
            else
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
            if (m->property) {
                bool captureRequired = true;

                Q_ASSERT(m->kind != V4IR::Member::MemberOfEnum);
                const int attachedPropertiesId = m->attachedPropertiesIdOrEnumValue;

                if (_function && attachedPropertiesId == 0 && !m->property->isConstant()) {
                    if (m->kind == V4IR::Member::MemberOfQmlContextObject) {
                        _function->contextObjectPropertyDependencies.insert(m->property->coreIndex, m->property->notifyIndex);
                        captureRequired = false;
                    } else if (m->kind == V4IR::Member::MemberOfQmlScopeObject) {
                        _function->scopeObjectPropertyDependencies.insert(m->property->coreIndex, m->property->notifyIndex);
                        captureRequired = false;
                    }
                }
                getQObjectProperty(m->base, m->property->coreIndex, captureRequired, attachedPropertiesId, t);
                return;
            } else if (m->base->asTemp() || m->base->asConst()) {
                getProperty(m->base, *m->name, t);
                return;
            }
        } else if (V4IR::Subscript *ss = s->source->asSubscript()) {
            getElement(ss->base->asTemp(), ss->index, t);
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
                callProperty(member->base, *member->name, c->args, t);
                return;
            } else if (Subscript *ss = c->base->asSubscript()) {
                callSubscript(ss->base, ss->index, c->args, t);
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
        if (m->base->asTemp() || m->base->asConst()) {
            if (s->source->asTemp() || s->source->asConst()) {
                Q_ASSERT(m->kind != V4IR::Member::MemberOfEnum);
                const int attachedPropertiesId = m->attachedPropertiesIdOrEnumValue;
                if (m->property && attachedPropertiesId == 0) {
                    setQObjectProperty(s->source, m->base, m->property->coreIndex);
                    return;
                } else {
                    setProperty(s->source, m->base, *m->name);
                    return;
                }
            }
        }
    } else if (V4IR::Subscript *ss = s->target->asSubscript()) {
        if (s->source->asTemp() || s->source->asConst()) {
            setElement(s->source, ss->base, ss->index);
            return;
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
            callSubscript(s->base, s->index, c->args, 0);
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
            callBuiltinTypeofMember(m->base, *m->name, result);
            return;
        } else if (V4IR::Subscript *ss = call->args->expr->asSubscript()) {
            callBuiltinTypeofSubscript(ss->base, ss->index, result);
            return;
        } else if (V4IR::Name *n = call->args->expr->asName()) {
            callBuiltinTypeofName(*n->id, result);
            return;
        } else if (call->args->expr->asTemp() || call->args->expr->asConst()){
            callBuiltinTypeofValue(call->args->expr, result);
            return;
        }
    } break;

    case V4IR::Name::builtin_delete: {
        if (V4IR::Member *m = call->args->expr->asMember()) {
            callBuiltinDeleteMember(m->base->asTemp(), *m->name, result);
            return;
        } else if (V4IR::Subscript *ss = call->args->expr->asSubscript()) {
            callBuiltinDeleteSubscript(ss->base->asTemp(), ss->index, result);
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

    case V4IR::Name::builtin_throw: {
        V4IR::Expr *arg = call->args->expr;
        assert(arg->asTemp() || arg->asConst());
        callBuiltinThrow(arg);
    } return;

    case V4IR::Name::builtin_rethrow: {
        callBuiltinReThrow();
    } return;

    case V4IR::Name::builtin_unwind_exception: {
        callBuiltinUnwindException(result);
    } return;

    case V4IR::Name::builtin_push_catch_scope: {
        V4IR::String *s = call->args->expr->asString();
        Q_ASSERT(s);
        callBuiltinPushCatchScope(*s->value);
    } return;

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
        V4IR::Expr *value = args->expr;

        callBuiltinDefineProperty(object, *name->id, value);
    } return;

    case V4IR::Name::builtin_define_array:
        callBuiltinDefineArray(result, call->args);
        return;

    case V4IR::Name::builtin_define_object_literal:
        callBuiltinDefineObjectLiteral(result, call->args);
        return;

    case V4IR::Name::builtin_setup_argument_object:
        callBuiltinSetupArgumentObject(result);
        return;

    case V4IR::Name::builtin_convert_this_to_object:
        callBuiltinConvertThisToObject();
        return;

    default:
        break;
    }

    Q_UNIMPLEMENTED();
    call->dump(qout); qout << endl;
    assert(!"TODO!");
    Q_UNREACHABLE();
}
