/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qv4managed_p.h"
#include "qv4mm_p.h"
#include "qv4errorobject_p.h"

using namespace QV4;


const ManagedVTable Managed::static_vtbl =
{
    0,
    Managed::IsExecutionContext,
    Managed::IsString,
    Managed::IsObject,
    Managed::IsFunctionObject,
    Managed::IsErrorObject,
    Managed::IsArrayData,
    0,
    Managed::MyType,
    "Managed",
    0,
    0 /*markObjects*/,
    isEqualTo
};


QString Managed::className() const
{
    const char *s = 0;
    switch (Type(d()->vtable->type)) {
    case Type_Invalid:
    case Type_String:
        return QString();
    case Type_Object:
        s = "Object";
        break;
    case Type_ArrayObject:
        s = "Array";
        break;
    case Type_FunctionObject:
        s = "Function";
        break;
    case Type_BooleanObject:
        s = "Boolean";
        break;
    case Type_NumberObject:
        s = "Number";
        break;
    case Type_StringObject:
        s = "String";
        break;
    case Type_DateObject:
        s = "Date";
        break;
    case Type_RegExpObject:
        s = "RegExp";
        break;
    case Type_ErrorObject:
        switch (static_cast<Heap::ErrorObject *>(d())->errorType) {
        case Heap::ErrorObject::Error:
            s = "Error";
            break;
        case Heap::ErrorObject::EvalError:
            s = "EvalError";
            break;
        case Heap::ErrorObject::RangeError:
            s = "RangeError";
            break;
        case Heap::ErrorObject::ReferenceError:
            s = "ReferenceError";
            break;
        case Heap::ErrorObject::SyntaxError:
            s = "SyntaxError";
            break;
        case Heap::ErrorObject::TypeError:
            s = "TypeError";
            break;
        case Heap::ErrorObject::URIError:
            s = "URIError";
            break;
        }
        break;
    case Type_ArgumentsObject:
        s = "Arguments";
        break;
    case Type_JsonObject:
        s = "JSON";
        break;
    case Type_MathObject:
        s = "Math";
        break;

    case Type_ExecutionContext:
        s = "__ExecutionContext";
        break;
    case Type_ForeachIteratorObject:
        s = "__ForeachIterator";
        break;
    case Type_RegExp:
        s = "__RegExp";
        break;

    case Type_QmlSequence:
        s = "QmlSequence";
        break;
    }
    return QString::fromLatin1(s);
}

bool Managed::isEqualTo(Managed *, Managed *)
{
    return false;
}
