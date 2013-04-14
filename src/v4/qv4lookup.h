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
#ifndef QV4LOOKUP_H
#define QV4LOOKUP_H

#include "qv4global.h"
#include "qv4runtime.h"
#include "qv4engine.h"
#include "qv4context.h"
#include "qv4object.h"
#include "qv4internalclass.h"

QT_BEGIN_NAMESPACE

namespace QQmlJS {
namespace VM {

struct Lookup {
    enum { Size = 3 };
    union {
        void (*lookupProperty)(Lookup *l, ExecutionContext *ctx, Value *result, const Value &object);
        void (*lookupGlobal)(Lookup *l, ExecutionContext *ctx, Value *result);
    };
    InternalClass *classList[Size];
    int level;
    uint index;
    String *name;

    static void lookupPropertyGeneric(Lookup *l, ExecutionContext *ctx, Value *result, const Value &object);
    static void lookupProperty0(Lookup *l, ExecutionContext *ctx, Value *result, const Value &object);
    static void lookupProperty1(Lookup *l, ExecutionContext *ctx, Value *result, const Value &object);
    static void lookupProperty2(Lookup *l, ExecutionContext *ctx, Value *result, const Value &object);
    static void lookupPropertyAccessor0(Lookup *l, ExecutionContext *ctx, Value *result, const Value &object);
    static void lookupPropertyAccessor1(Lookup *l, ExecutionContext *ctx, Value *result, const Value &object);
    static void lookupPropertyAccessor2(Lookup *l, ExecutionContext *ctx, Value *result, const Value &object);

    static void lookupGlobalGeneric(Lookup *l, ExecutionContext *ctx, Value *result);
    static void lookupGlobal0(Lookup *l, ExecutionContext *ctx, Value *result);
    static void lookupGlobal1(Lookup *l, ExecutionContext *ctx, Value *result);
    static void lookupGlobal2(Lookup *l, ExecutionContext *ctx, Value *result);
    static void lookupGlobalAccessor0(Lookup *l, ExecutionContext *ctx, Value *result);
    static void lookupGlobalAccessor1(Lookup *l, ExecutionContext *ctx, Value *result);
    static void lookupGlobalAccessor2(Lookup *l, ExecutionContext *ctx, Value *result);

    Property *lookup(Object *obj, PropertyAttributes *attrs) {
        int i = 0;
        while (i < level && obj && obj->internalClass == classList[i]) {
            obj = obj->prototype;
            ++i;
        }

        if (index != UINT_MAX && obj->internalClass == classList[i]) {
            *attrs = obj->internalClass->propertyData.at(index);
            return obj->memberData + index;
        }

        while (i < Size && obj) {
            classList[i] = obj->internalClass;

            index = obj->internalClass->find(name);
            if (index != UINT_MAX) {
                level = i;
                *attrs = obj->internalClass->propertyData.at(index);
                return obj->memberData + index;
            }

            obj = obj->prototype;
            ++i;
        }
        level = i;

        while (obj) {
            index = obj->internalClass->find(name);
            if (index != UINT_MAX) {
                *attrs = obj->internalClass->propertyData.at(index);
                return obj->memberData + index;
            }

            obj = obj->prototype;
        }
        return 0;
    }

    Property *setterLookup(Object *o, PropertyAttributes *attrs) {
        if (o->internalClass == classList[0]) {
            *attrs = o->internalClass->propertyData[index];
            return o->memberData + index;
        }

        uint idx = o->internalClass->find(name);
        if (idx != UINT_MAX) {
            classList[0] = o->internalClass;
            index = idx;
            *attrs = o->internalClass->propertyData[index];
            return o->memberData + index;
        }
        return 0;
    }
};

}
}

QT_END_NAMESPACE

#endif
