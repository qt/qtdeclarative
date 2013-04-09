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

#ifndef QV4GLOBAL_H
#define QV4GLOBAL_H

#include <QtCore/qglobal.h>

QT_BEGIN_NAMESPACE

#ifndef QT_STATIC
#  if defined(QT_BUILD_V4_LIB)
#    define Q_V4_EXPORT Q_DECL_EXPORT
#  else
#    define Q_V4_EXPORT Q_DECL_IMPORT
#  endif
#else
#  define Q_V4_EXPORT
#endif

namespace QQmlJS {
namespace VM {


enum PropertyFlag {
    Attr_Default = 0,
    Attr_Accessor = 0x1,
    Attr_NotWritable = 0x2,
    Attr_NotEnumerable = 0x4,
    Attr_NotConfigurable = 0x8,
    Attr_ReadOnly = Attr_NotWritable|Attr_NotEnumerable|Attr_NotConfigurable,
    Attr_Invalid = 0xff
};

Q_DECLARE_FLAGS(PropertyFlags, PropertyFlag);
Q_DECLARE_OPERATORS_FOR_FLAGS(PropertyFlags);

struct PropertyAttributes
{
    union {
        uchar m_all;
        struct {
            uchar m_flags : 4;
            uchar m_mask : 4;
        };
        struct {
            uchar m_type : 1;
            uchar m_writable : 1;
            uchar m_enumerable : 1;
            uchar m_configurable : 1;
            uchar type_set : 1;
            uchar writable_set : 1;
            uchar enumerable_set : 1;
            uchar configurable_set : 1;
        };
    };

    enum Type {
        Data = 0,
        Accessor = 1,
        Generic = 2
    };

    PropertyAttributes() : m_all(0) {}
    PropertyAttributes(PropertyFlag f) : m_all(0) {
        setType(f & Attr_Accessor ? Accessor : Data);
        setWritable(!(f & Attr_NotWritable));
        setEnumberable(!(f & Attr_NotEnumerable));
        setConfigurable(!(f & Attr_NotConfigurable));
    }
    PropertyAttributes(PropertyFlags f) : m_all(0) {
        setType(f & Attr_Accessor ? Accessor : Data);
        setWritable(!(f & Attr_NotWritable));
        setEnumberable(!(f & Attr_NotEnumerable));
        setConfigurable(!(f & Attr_NotConfigurable));
    }
    PropertyAttributes(const PropertyAttributes &other) : m_all(other.m_all) {}
    PropertyAttributes & operator=(const PropertyAttributes &other) { m_all = other.m_all; return *this; }

    void setType(Type t) { m_type = t; type_set = true; }
    Type type() const { return type_set ? (Type)m_type : Generic; }

    void setWritable(bool b) { m_writable = b; writable_set = true; }
    void setConfigurable(bool b) { m_configurable = b; configurable_set = true; }
    void setEnumberable(bool b) { m_enumerable = b; enumerable_set = true; }

    bool writable() const { return m_writable; }
    bool enumerable() const { return m_enumerable; }
    bool configurable() const { return m_configurable; }
};

}
}

QT_END_NAMESPACE

#endif // QV4GLOBAL_H
