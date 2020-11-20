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

static bool isNamedEnumeratorInScope(const QMetaObject *resolvedMetaObject, const QByteArray &scope,
                                     const QByteArray &name)
{
    for (int i = resolvedMetaObject->enumeratorCount() - 1; i >= 0; --i) {
        QMetaEnum m = resolvedMetaObject->enumerator(i);
        if ((m.name() == name) && (scope.isEmpty() || (m.scope() == scope)))
            return true;
    }
    return false;
}

static bool isNamedEnumerator(const QMetaObject *metaObj, const QByteArray &scopedName)
{
    QByteArray scope;
    QByteArray name;
    int scopeIdx = scopedName.lastIndexOf("::");
    if (scopeIdx != -1) {
        scope = scopedName.left(scopeIdx);
        name = scopedName.mid(scopeIdx + 2);
    } else {
        name = scopedName;
    }

    if (scope == "Qt")
        return isNamedEnumeratorInScope(&Qt::staticMetaObject, scope, name);

    if (isNamedEnumeratorInScope(metaObj, scope, name))
        return true;

    if (metaObj->d.relatedMetaObjects && !scope.isEmpty()) {
        for (auto related = metaObj->d.relatedMetaObjects; *related; ++related) {
            if (isNamedEnumeratorInScope(*related, scope, name))
                return true;
        }
    }

    return false;
}

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

int QQmlMetaObject::methodReturnType(const QQmlPropertyData &data, QByteArray *unknownTypeError) const
{
    Q_ASSERT(_m && data.coreIndex() >= 0);

    QMetaType type = data.propType();

    const char *propTypeName = nullptr;

    if (!type.isValid()) {
        // Find the return type name from the method info
        QMetaMethod m = _m->method(data.coreIndex());

        type = m.returnMetaType();
        propTypeName = m.typeName();
    }

    if (type.sizeOf() <= qsizetype(sizeof(int))) {
        if (type.flags() & QMetaType::IsEnumeration)
            return QMetaType::Int;

        if (isNamedEnumerator(_m, propTypeName))
            return QMetaType::Int;

        if (!type.isValid()) {
            if (unknownTypeError)
                *unknownTypeError = propTypeName;
        }
    } // else we know that it's a known type, as sizeOf(UnknownType) == 0

    return type.id();
}

int *QQmlMetaObject::methodParameterTypes(int index, ArgTypeStorage *argStorage,
                                          QByteArray *unknownTypeError) const
{
    Q_ASSERT(_m && index >= 0);

    QMetaMethod m = _m->method(index);
    return methodParameterTypes(m, argStorage, unknownTypeError);
}

int *QQmlMetaObject::constructorParameterTypes(int index, ArgTypeStorage *dummy,
                                                     QByteArray *unknownTypeError) const
{
    QMetaMethod m = _m->constructor(index);
    return methodParameterTypes(m, dummy, unknownTypeError);
}

int *QQmlMetaObject::methodParameterTypes(const QMetaMethod &m, ArgTypeStorage *argStorage,
                                          QByteArray *unknownTypeError) const
{
    Q_ASSERT(argStorage);

    int argc = m.parameterCount();
    argStorage->resize(argc + 1);
    argStorage->operator[](0) = argc;
    QList<QByteArray> argTypeNames; // Only loaded if needed

    for (int ii = 0; ii < argc; ++ii) {
        int type = m.parameterType(ii);
        if (QMetaType(type).sizeOf() > qsizetype(sizeof(int))) {
            // Cannot be passed as int
            // We know that it's a known type, as sizeOf(UnknownType) == 0
        } else if (QMetaType(type).flags() & QMetaType::IsEnumeration) {
            type = QMetaType::Int;
        } else {
            if (argTypeNames.isEmpty())
                argTypeNames = m.parameterTypes();
            if (isNamedEnumerator(_m, argTypeNames.at(ii))) {
                type = QMetaType::Int;
            } else if (type == QMetaType::UnknownType) {
                if (unknownTypeError)
                    *unknownTypeError = argTypeNames.at(ii);
                return nullptr;
            }
        }
        argStorage->operator[](ii + 1) = type;
    }

    return argStorage->data();
}

QT_END_NAMESPACE
