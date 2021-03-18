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

struct StaticQtMetaObject : public QObject
{
    static const QMetaObject *get()
    { return &staticQtMetaObject; }
};

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
        return isNamedEnumeratorInScope(StaticQtMetaObject::get(), scope, name);

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

    struct I { static bool equal(const QMetaObject *lhs, const QMetaObject *rhs) {
            return lhs == rhs || (lhs && rhs && lhs->d.stringdata == rhs->d.stringdata);
        } };

    const QMetaObject *tom = to._m.isT1()?to._m.asT1()->metaObject():to._m.asT2();
    if (tom == &QObject::staticMetaObject) return true;

    if (from._m.isT1() && to._m.isT1()) { // QQmlPropertyCache -> QQmlPropertyCache
        QQmlPropertyCache *fromp = from._m.asT1();
        QQmlPropertyCache *top = to._m.asT1();

        while (fromp) {
            if (fromp == top) return true;
            fromp = fromp->parent();
        }
    } else if (from._m.isT1() && to._m.isT2()) { // QQmlPropertyCache -> QMetaObject
        QQmlPropertyCache *fromp = from._m.asT1();

        while (fromp) {
            const QMetaObject *fromm = fromp->metaObject();
            if (fromm && I::equal(fromm, tom)) return true;
            fromp = fromp->parent();
        }
    } else if (from._m.isT2() && to._m.isT1()) { // QMetaObject -> QQmlPropertyCache
        const QMetaObject *fromm = from._m.asT2();

        if (!tom) return false;

        while (fromm) {
            if (I::equal(fromm, tom)) return true;
            fromm = fromm->superClass();
        }
    } else { // QMetaObject -> QMetaObject
        const QMetaObject *fromm = from._m.asT2();

        while (fromm) {
            if (I::equal(fromm, tom)) return true;
            fromm = fromm->superClass();
        }
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
    case QMetaObject::QueryPropertyDesignable:
    case QMetaObject::QueryPropertyEditable:
    case QMetaObject::QueryPropertyScriptable:
    case QMetaObject::QueryPropertyStored:
    case QMetaObject::QueryPropertyUser:
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

QQmlPropertyCache *QQmlMetaObject::propertyCache(QQmlEnginePrivate *e) const
{
    if (_m.isNull()) return nullptr;
    if (_m.isT1()) return _m.asT1();
    else return e->cache(_m.asT2());
}

int QQmlMetaObject::methodReturnType(const QQmlPropertyData &data, QByteArray *unknownTypeError) const
{
    Q_ASSERT(!_m.isNull() && data.coreIndex() >= 0);

    int type = data.propType();

    const char *propTypeName = nullptr;

    if (type == QMetaType::UnknownType) {
        // Find the return type name from the method info
        QMetaMethod m;

        if (_m.isT1()) {
            QQmlPropertyCache *c = _m.asT1();
            Q_ASSERT(data.coreIndex() < c->methodIndexCacheStart + c->methodIndexCache.count());

            while (data.coreIndex() < c->methodIndexCacheStart)
                c = c->_parent;

            const QMetaObject *metaObject = c->createMetaObject();
            Q_ASSERT(metaObject);
            m = metaObject->method(data.coreIndex());
        } else {
            m = _m.asT2()->method(data.coreIndex());
        }

        type = m.returnType();
        propTypeName = m.typeName();
    }

    if (QMetaType::sizeOf(type) <= int(sizeof(int))) {
        if (QMetaType::typeFlags(type) & QMetaType::IsEnumeration)
            return QMetaType::Int;

        if (isNamedEnumerator(metaObject(), propTypeName))
            return QMetaType::Int;

        if (type == QMetaType::UnknownType) {
            if (unknownTypeError)
                *unknownTypeError = propTypeName;
        }
    } // else we know that it's a known type, as sizeOf(UnknownType) == 0

    return type;
}

int *QQmlMetaObject::methodParameterTypes(int index, ArgTypeStorage *argStorage,
                                          QByteArray *unknownTypeError) const
{
    Q_ASSERT(!_m.isNull() && index >= 0);

    if (_m.isT1()) {
        typedef QQmlPropertyCacheMethodArguments A;

        QQmlPropertyCache *c = _m.asT1();
        Q_ASSERT(index < c->methodIndexCacheStart + c->methodIndexCache.count());

        while (index < c->methodIndexCacheStart)
            c = c->_parent;

        QQmlPropertyData *rv = const_cast<QQmlPropertyData *>(&c->methodIndexCache.at(index - c->methodIndexCacheStart));

        if (rv->arguments() && static_cast<A *>(rv->arguments())->argumentsValid)
            return static_cast<A *>(rv->arguments())->arguments;

        const QMetaObject *metaObject = c->createMetaObject();
        Q_ASSERT(metaObject);
        QMetaMethod m = metaObject->method(index);

        int argc = m.parameterCount();
        if (!rv->arguments()) {
            A *args = c->createArgumentsObject(argc, m.parameterNames());
            rv->setArguments(args);
        }
        A *args = static_cast<A *>(rv->arguments());

        QList<QByteArray> argTypeNames; // Only loaded if needed

        for (int ii = 0; ii < argc; ++ii) {
            int type = m.parameterType(ii);

            if (QMetaType::sizeOf(type) > int(sizeof(int))) {
                // Cannot be passed as int
                // We know that it's a known type, as sizeOf(UnknownType) == 0
            } else if (QMetaType::typeFlags(type) & QMetaType::IsEnumeration) {
                type = QMetaType::Int;
            } else {
                if (argTypeNames.isEmpty())
                    argTypeNames = m.parameterTypes();
                if (isNamedEnumerator(metaObject, argTypeNames.at(ii))) {
                    type = QMetaType::Int;
                } else if (type == QMetaType::UnknownType){
                    if (unknownTypeError)
                        *unknownTypeError = argTypeNames.at(ii);
                    return nullptr;
                }

            }
            args->arguments[ii + 1] = type;
        }
        args->argumentsValid = true;
        return static_cast<A *>(rv->arguments())->arguments;

    } else {
        QMetaMethod m = _m.asT2()->method(index);
        return methodParameterTypes(m, argStorage, unknownTypeError);

    }
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
        if (QMetaType::sizeOf(type) > int(sizeof(int))) {
            // Cannot be passed as int
            // We know that it's a known type, as sizeOf(UnknownType) == 0
        } else if (QMetaType::typeFlags(type) & QMetaType::IsEnumeration) {
            type = QMetaType::Int;
        } else {
            if (argTypeNames.isEmpty())
                argTypeNames = m.parameterTypes();
            if (isNamedEnumerator(_m.asT2(), argTypeNames.at(ii))) {
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
