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
#include "qv4internalclass.h"

QT_BEGIN_NAMESPACE

namespace QQmlJS {
namespace VM {

struct FunctionObject;

struct Property {
    union {
        Value value;
        struct {
            FunctionObject *get;
            FunctionObject *set;
        };
    };
};

struct PropertyDescriptor : public Property
{
    PropertyAttributes attrs;

    static inline PropertyDescriptor fromValue(Value v) {
        PropertyDescriptor pd;
        pd.value = v;
        pd.attrs.setType(PropertyAttributes::Data);
        return pd;
    }
    static inline PropertyDescriptor fromAccessor(FunctionObject *getter, FunctionObject *setter) {
        PropertyDescriptor pd;
        pd.get = getter;
        pd.set = setter;
        pd.attrs.setType(PropertyAttributes::Accessor);
        return pd;
    }

    PropertyAttributes toPropertyAttributes() const {
        return attrs;
    }

    // Section 8.10
    inline void fullyPopulated() {
        if (!attrs.hasType()) {
            attrs.setType(PropertyAttributes::Data);
            value = Value::undefinedValue();
        }
        if (attrs.type() == PropertyAttributes::Data) {
            attrs.resolveWritable();
        } else {
            attrs.clearWritable();
            if ((quintptr)get == 0x1)
                get = 0;
            if ((quintptr)set == 0x1)
                set = 0;
        }
        attrs.resolveEnumerable();
        attrs.resolveConfigurable();
    }

    inline bool isData() const { return attrs.isData(); }
    inline bool isAccessor() const { return attrs.isAccessor(); }
    inline bool isGeneric() const { return attrs.isGeneric(); }

    inline bool isWritable() const { return attrs.writable(); }
    inline bool isEnumerable() const { return attrs.enumerable(); }
    inline bool isConfigurable() const { return attrs.configurable(); }

    inline bool isEmpty() const {
        return attrs.isEmpty();
    }
    inline bool isSubset(PropertyDescriptor *other) const {
        if (attrs.type() != PropertyAttributes::Generic && attrs.type() != other->attrs.type())
            return false;
        if (attrs.hasEnumerable() && attrs.enumerable() != other->attrs.enumerable())
            return false;
        if (attrs.hasConfigurable() && attrs.configurable() != other->attrs.configurable())
            return false;
        if (attrs.hasWritable() && attrs.writable() != other->attrs.writable())
            return false;
        if (attrs.type() == PropertyAttributes::Data && !value.sameValue(other->value))
            return false;
        if (attrs.type() == PropertyAttributes::Accessor) {
            if (get != other->get)
                return false;
            if (set != other->set)
                return false;
        }
        return true;
    }
    inline void operator+=(const PropertyDescriptor &other) {
        if (other.attrs.hasEnumerable())
            attrs.setEnumerable(other.attrs.enumerable());
        if (other.attrs.hasConfigurable())
            attrs.setConfigurable(other.attrs.configurable());
        if (other.attrs.hasWritable())
            attrs.setWritable(other.attrs.writable());
        if (other.attrs.type() == PropertyAttributes::Accessor) {
            attrs.setType(PropertyAttributes::Accessor);
            if (other.get)
                get = ((quintptr)other.get == 0x1) ? 0 : other.get;
            if (other.set)
                set = ((quintptr)other.set == 0x1) ? 0 : other.set;
        } else if (other.attrs.type() == PropertyAttributes::Data){
            attrs.setType(PropertyAttributes::Data);
            value = other.value;
        }
    }
};

} // namespace VM
} // namespace QQmlJS

QT_END_NAMESPACE

#endif
