// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qqmlmetaobject_p.h"

#include <private/qqmlengine_p.h>
#include <private/qqmlpropertycachemethodarguments_p.h>

QT_BEGIN_NAMESPACE

void QQmlMetaObject::resolveGadgetMethodOrPropertyIndex(QMetaObject::Call type, const QMetaObject **metaObject, int *index)
{
    int offset;

    switch (type) {
    case QMetaObject::ReadProperty:
    case QMetaObject::WriteProperty:
    case QMetaObject::ResetProperty:
        offset = (*metaObject)->propertyOffset();
        while (*index < offset) {
            *metaObject = (*metaObject)->superClass();
            offset = (*metaObject)->propertyOffset();
        }
        break;
    case QMetaObject::InvokeMetaMethod:
        offset = (*metaObject)->methodOffset();
        while (*index < offset) {
            *metaObject = (*metaObject)->superClass();
            offset = (*metaObject)->methodOffset();
        }
        break;
    default:
        offset = 0;
        Q_UNIMPLEMENTED();
        offset = INT_MAX;
    }

    *index -= offset;
}

QMetaType QQmlMetaObject::methodReturnType(const QQmlPropertyData &data, QByteArray *unknownTypeError) const
{
    Q_ASSERT(_m && data.coreIndex() >= 0);

    QMetaType type = data.propType();
    if (!type.isValid()) {
        // Find the return type name from the method info
        type = _m->method(data.coreIndex()).returnMetaType();
    }
    if (type.flags().testFlag(QMetaType::IsEnumeration))
        type = type.underlyingType();
    if (type.isValid())
        return type;
    else if (unknownTypeError)
        *unknownTypeError = _m->method(data.coreIndex()).typeName();
    return QMetaType();
}

bool QQmlMetaObject::methodParameterTypes(int index, ArgTypeStorage *argStorage,
                                          QByteArray *unknownTypeError) const
{
    Q_ASSERT(_m && index >= 0);

    QMetaMethod m = _m->method(index);
    return methodParameterTypes(m, argStorage, unknownTypeError);
}

bool QQmlMetaObject::constructorParameterTypes(int index, ArgTypeStorage *dummy,
                                                     QByteArray *unknownTypeError) const
{
    QMetaMethod m = _m->constructor(index);
    return methodParameterTypes(m, dummy, unknownTypeError);
}

bool QQmlMetaObject::methodParameterTypes(const QMetaMethod &m, ArgTypeStorage *argStorage,
                                          QByteArray *unknownTypeError)
{
    Q_ASSERT(argStorage);

    int argc = m.parameterCount();
    argStorage->resize(argc);
    for (int ii = 0; ii < argc; ++ii) {
        QMetaType type = m.parameterMetaType(ii);
        // we treat enumerations as int
        if (type.flags().testFlag(QMetaType::IsEnumeration))
            type = type.underlyingType();
        if (!type.isValid()) {
            if (unknownTypeError)
                *unknownTypeError =  m.parameterTypeName(ii);
            return false;
        }
        argStorage->operator[](ii) = type;
    }
    return true;
}

QT_END_NAMESPACE
