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
#include "qv4lookup.h"

QT_BEGIN_NAMESPACE

namespace QQmlJS {
namespace VM {

void Lookup::lookupPropertyGeneric(QQmlJS::VM::Lookup *l, ExecutionContext *ctx, QQmlJS::VM::Value *result, const QQmlJS::VM::Value &object)
{
    if (Object *o = object.asObject()) {
        PropertyAttributes attrs;
        Property *p = l->lookup(o, &attrs);
        if (p) {
            if (attrs.isData()) {
                if (l->level == 0)
                    l->lookupProperty = lookupProperty0;
                else if (l->level == 1)
                    l->lookupProperty = lookupProperty1;
                else if (l->level == 2)
                    l->lookupProperty = lookupProperty2;
                if (result)
                    *result = p->value;
                return;
            } else {
                Value res = o->getValue(ctx, p, attrs);
                if (result)
                    *result = res;
                return;
            }
        } else if (result) {
            *result = Value::undefinedValue();
        }
    } else {
        Value res;
        if (Managed *m = object.asManaged()) {
            res = m->get(ctx, l->name);
        } else {
            o = __qmljs_convert_to_object(ctx, object);
            res = o->get(ctx, l->name);
        }
        if (result)
            *result = res;
    }
}

void Lookup::lookupProperty0(Lookup *l, ExecutionContext *ctx, Value *result, const Value &object)
{
    if (Object *o = object.asObject()) {
        if (l->classList[0] == o->internalClass) {
            if (result)
                *result = o->memberData[l->index].value;
            return;
        }
    }
    l->lookupProperty = lookupPropertyGeneric;
    lookupPropertyGeneric(l, ctx, result, object);
}

void Lookup::lookupProperty1(Lookup *l, ExecutionContext *ctx, Value *result, const Value &object)
{
    if (Object *o = object.asObject()) {
        if (l->classList[0] == o->internalClass &&
            l->classList[1] == o->prototype->internalClass) {
            if (result)
                *result = o->prototype->memberData[l->index].value;
            return;
        }
    }
    l->lookupProperty = lookupPropertyGeneric;
    lookupPropertyGeneric(l, ctx, result, object);
}

void Lookup::lookupProperty2(Lookup *l, ExecutionContext *ctx, Value *result, const Value &object)
{
    if (Object *o = object.asObject()) {
        if (l->classList[0] == o->internalClass) {
            o = o->prototype;
            if (l->classList[1] == o->internalClass) {
                o = o->prototype;
                if (l->classList[2] == o->internalClass) {
                    if (result)
                        *result = o->prototype->memberData[l->index].value;
                    return;
                }
            }
        }
    }
    l->lookupProperty = lookupPropertyGeneric;
    lookupPropertyGeneric(l, ctx, result, object);
}

void Lookup::lookupGlobalGeneric(Lookup *l, ExecutionContext *ctx, Value *result)
{
    Object *o = ctx->engine->globalObject;
    PropertyAttributes attrs;
    Property *p = l->lookup(o, &attrs);
    if (p) {
        if (attrs.isData()) {
            if (l->level == 0)
                l->lookupGlobal = lookupGlobal0;
            else if (l->level == 1)
                l->lookupGlobal = lookupGlobal1;
            else if (l->level == 2)
                l->lookupGlobal = lookupGlobal2;
            *result = p->value;
            return;
        } else {
            Value res = o->getValue(ctx, p, attrs);
            if (result)
                *result = res;
            return;
        }
    }
    ctx->throwReferenceError(Value::fromString(l->name));
}

void Lookup::lookupGlobal0(Lookup *l, ExecutionContext *ctx, Value *result)
{
    Object *o = ctx->engine->globalObject;
    if (l->classList[0] == o->internalClass) {
        *result = o->memberData[l->index].value;
        return;
    }
    l->lookupGlobal = lookupGlobalGeneric;
    lookupGlobalGeneric(l, ctx, result);
}

void Lookup::lookupGlobal1(Lookup *l, ExecutionContext *ctx, Value *result)
{
    Object *o = ctx->engine->globalObject;
    if (l->classList[0] == o->internalClass &&
        l->classList[1] == o->prototype->internalClass) {
        *result = o->prototype->memberData[l->index].value;
        return;
    }
    l->lookupGlobal = lookupGlobalGeneric;
    lookupGlobalGeneric(l, ctx, result);
}

void Lookup::lookupGlobal2(Lookup *l, ExecutionContext *ctx, Value *result)
{
    Object *o = ctx->engine->globalObject;
    if (l->classList[0] == o->internalClass) {
        o = o->prototype;
        if (l->classList[1] == o->internalClass) {
            o = o->prototype;
            if (l->classList[2] == o->internalClass) {
                *result = o->prototype->memberData[l->index].value;
                return;
            }
        }
    }
    l->lookupGlobal = lookupGlobalGeneric;
    lookupGlobalGeneric(l, ctx, result);
}

}
}

QT_END_NAMESPACE
