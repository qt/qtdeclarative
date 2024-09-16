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

QT_END_NAMESPACE
