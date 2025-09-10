// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qv4managed_p.h"
#include <private/qv4mm_p.h>

using namespace QV4;

DEFINE_MANAGED_VTABLE(Managed);

DEFINE_MANAGED_VTABLE(InternalClass);


QString Managed::className() const
{
    const char *s = nullptr;
    switch (Type(vtable()->type)) {
    case Type_Invalid:
        return QString();
    case Type_String:
        s = "String";
        break;
    case Type_Symbol:
        s = "Symbol";
        break;
    case Type_Object:
        s = "Object";
        break;
    case Type_ArrayObject:
        s = "Array";
        break;
    case Type_FunctionObject:
        s = "Function";
        break;
    case Type_GeneratorObject:
        s = "Generator";
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
    case Type_SymbolObject:
        s = "Symbol";
        break;
    case Type_DateObject:
        s = "Date";
        break;
    case Type_RegExpObject:
        s = "RegExp";
        break;
    case Type_ErrorObject:
        s = "Error";
        break;
    case Type_ArgumentsObject:
        s = "Arguments";
        break;
    case Type_JsonObject:
        s = "JSON";
        break;
    case Type_ProxyObject:
        s = "ProxyObject";
        break;
    case Type_MathObject:
        s = "Math";
        break;
    case Type_UrlObject:
        s = "URL";
        break;
    case Type_UrlSearchParamsObject:
        s = "URLSearchParams";
        break;

    case Type_ExecutionContext:
        s = "__ExecutionContext";
        break;
    case Type_MapIteratorObject:
        s = "Map Iterator";
        break;
    case Type_SetIteratorObject:
        s = "Set Iterator";
        break;
    case Type_ArrayIteratorObject:
        s = "Array Iterator";
        break;
    case Type_StringIteratorObject:
        s = "String Iterator";
        break;
    case Type_ForInIterator:
        s = "__ForIn Iterator";
        break;
    case Type_InternalClass:
        s = "__InternalClass";
        break;
    case Type_RegExp:
        s = "__RegExp";
        break;

    case Type_V4Sequence:
        s = "V4Sequence";
        break;
    case Type_QmlListProperty:
        s = "QML List";
        break;
    }
    return QString::fromLatin1(s);
}

bool Managed::virtualIsEqualTo(Managed *, Managed *)
{
    return false;
}


OwnPropertyKeyIterator::~OwnPropertyKeyIterator()
{
}
