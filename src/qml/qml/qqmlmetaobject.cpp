/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qqmlmetaobject_p.h"

#include <private/qqmlengine_p.h>
#include <private/qqmlpropertycachemethodarguments_p.h>

QT_BEGIN_NAMESPACE

// Returns true if \a from is assignable to a property of type \a to
bool QQmlMetaObject::canConvert(const QQmlMetaObject &from, const QQmlMetaObject &to)
{
    Q_ASSERT(!from.isNull() && !to.isNull());

    auto equal = [] (const QMetaObject *lhs, const QMetaObject *rhs) -> bool {
        return lhs == rhs || (lhs && rhs && lhs->d.stringdata == rhs->d.stringdata);
    };

    const QMetaObject *tom = to.metaObject();
    if (tom == &QObject::staticMetaObject) return true;

    const QMetaObject *fromm = from.metaObject();
    while (fromm) {
        if (equal(fromm, tom))
            return true;
        fromm = fromm->superClass();
    }

    return false;
}

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
        type = QMetaType::fromType<int>();
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
            type = QMetaType::fromType<int>();
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
