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
#ifndef QV4PROPERTYDESCRIPTOR_H
#define QV4PROPERTYDESCRIPTOR_H

#include "qv4global.h"
#include "qv4value.h"

QT_BEGIN_NAMESPACE

namespace QQmlJS {
namespace VM {

struct FunctionObject;

struct PropertyDescriptor {
    enum Type {
        Generic,
        Data,
        Accessor
    };
    enum State {
        Undefined,
        Disabled,
        Enabled
    };
    union {
        Value value;
        struct {
            FunctionObject *get;
            FunctionObject *set;
        };
    };
    uint type : 8;
    uint writable : 8;
    uint enumerable : 8;
    uint configurable : 8;

    PropertyFlags propertyFlags() {
        int f = 0;
        if (writable == Enabled)
            f |= Writable;
        if (configurable == Enabled)
            f |= Configurable;
        if (enumerable == Enabled)
            f |= Enumerable;
        return PropertyFlags(f);
    }

    static inline PropertyDescriptor fromValue(Value v) {
        PropertyDescriptor pd;
        pd.value = v;
        pd.type = Data;
        pd.writable = Undefined;
        pd.enumerable = Undefined;
        pd.configurable = Undefined;
        return pd;
    }
    static inline PropertyDescriptor fromAccessor(FunctionObject *getter, FunctionObject *setter) {
        PropertyDescriptor pd;
        pd.get = getter;
        pd.set = setter;
        pd.type = Accessor;
        pd.writable = Undefined;
        pd.enumerable = Undefined;
        pd.configurable = Undefined;
        return pd;
    }

    // Section 8.10
    inline void fullyPopulated() {
        if (type == Generic) {
            type = Data;
            value = Value::undefinedValue();
        }
        if (type == Data) {
            if (writable == Undefined)
                writable = Disabled;
        } else {
            writable = Undefined;
            if ((quintptr)get == 0x1)
                get = 0;
            if ((quintptr)set == 0x1)
                set = 0;
        }
        if (enumerable == Undefined)
            enumerable = Disabled;
        if (configurable == Undefined)
            configurable = Disabled;
    }

    inline bool isData() const { return type == Data || writable != Undefined; }
    inline bool isAccessor() const { return type == Accessor; }
    inline bool isGeneric() const { return type == Generic && writable == Undefined; }

    inline bool isWritable() const { return writable == Enabled; }
    inline bool isEnumerable() const { return enumerable == Enabled; }
    inline bool isConfigurable() const { return configurable == Enabled; }

    inline bool isEmpty() const {
        return type == Generic && writable == Undefined && enumerable == Undefined && configurable == Undefined;
    }
    inline bool isSubset(PropertyDescriptor *other) const {
        if (type != Generic && type != other->type)
            return false;
        if (enumerable != Undefined && enumerable != other->enumerable)
            return false;
        if (configurable != Undefined && configurable != other->configurable)
            return false;
        if (writable != Undefined && writable != other->writable)
            return false;
        if (type == Data && !value.sameValue(other->value))
            return false;
        if (type == Accessor) {
            if (get != other->get)
                return false;
            if (set != other->set)
                return false;
        }
        return true;
    }
    inline void operator+=(const PropertyDescriptor &other) {
        if (other.enumerable != Undefined)
            enumerable = other.enumerable;
        if (other.configurable != Undefined)
            configurable = other.configurable;
        if (other.writable != Undefined)
            writable = other.writable;
        if (other.type == Accessor) {
            type = Accessor;
            if (other.get)
                get = ((quintptr)other.get == 0x1) ? 0 : other.get;
            if (other.set)
                set = ((quintptr)other.set == 0x1) ? 0 : other.set;
        } else if (other.type == Data){
            type = Data;
            value = other.value;
        }
    }
};

} // namespace VM
} // namespace QQmlJS

QT_END_NAMESPACE

#endif
