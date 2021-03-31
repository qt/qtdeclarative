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

#ifndef QQMLMETAOBJECT_P_H
#define QQMLMETAOBJECT_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtQml/qtqmlglobal.h>

#include <private/qflagpointer_p.h>
#include <private/qqmlpropertycache_p.h>

#include <QtCore/qvarlengtharray.h>
#include <QtCore/qmetaobject.h>

QT_BEGIN_NAMESPACE

// QQmlMetaObject serves as a wrapper around either QMetaObject or QQmlPropertyCache.
// This is necessary as we delay creation of QMetaObject for synthesized QObjects, but
// we don't want to needlessly generate QQmlPropertyCaches every time we encounter a
// QObject type used in assignment or when we don't have a QQmlEngine etc.
//
// This class does NOT reference the propertycache.
class QQmlEnginePrivate;
class QQmlPropertyData;
class Q_QML_EXPORT QQmlMetaObject
{
public:
    typedef QVarLengthArray<int, 9> ArgTypeStorage;

    inline QQmlMetaObject() = default;
    inline QQmlMetaObject(const QObject *);
    inline QQmlMetaObject(const QMetaObject *);
    inline QQmlMetaObject(QQmlPropertyCache *);
    inline QQmlMetaObject(const QQmlMetaObject &);

    inline QQmlMetaObject &operator=(const QQmlMetaObject &);

    inline bool isNull() const;

    inline const char *className() const;
    inline int propertyCount() const;

    inline const QMetaObject *metaObject() const;

    int methodReturnType(const QQmlPropertyData &data, QByteArray *unknownTypeError) const;
    int *methodParameterTypes(int index, ArgTypeStorage *argStorage,
                              QByteArray *unknownTypeError) const;
    int *constructorParameterTypes(int index, ArgTypeStorage *dummy, QByteArray *unknownTypeError) const;


    static bool canConvert(const QQmlMetaObject &from, const QQmlMetaObject &to);

    // static_metacall (on Gadgets) doesn't call the base implementation and therefore
    // we need a helper to find the correct meta object and property/method index.
    static void resolveGadgetMethodOrPropertyIndex(QMetaObject::Call type, const QMetaObject **metaObject, int *index);

    static int *methodParameterTypes(const QMetaMethod &method, ArgTypeStorage *argStorage,
                              QByteArray *unknownTypeError);
protected:
    const QMetaObject *_m = nullptr;
};

QQmlMetaObject::QQmlMetaObject(const QObject *o)
{
    if (o)
        _m = o->metaObject();
}

QQmlMetaObject::QQmlMetaObject(const QMetaObject *m)
    : _m(m)
{
}

QQmlMetaObject::QQmlMetaObject(QQmlPropertyCache *m)
{
    if (m)
        _m = m->createMetaObject();
}

QQmlMetaObject::QQmlMetaObject(const QQmlMetaObject &o)
    : _m(o._m)
{
}

QQmlMetaObject &QQmlMetaObject::operator=(const QQmlMetaObject &o)
{
    _m = o._m;
    return *this;
}

bool QQmlMetaObject::isNull() const
{
    return !_m;
}

const char *QQmlMetaObject::className() const
{
    if (!_m)
        return nullptr;
    return metaObject()->className();
}

int QQmlMetaObject::propertyCount() const
{
    if (!_m)
        return 0;
    return metaObject()->propertyCount();
}

const QMetaObject *QQmlMetaObject::metaObject() const
{
    return _m;
}

QT_END_NAMESPACE

#endif // QQMLMETAOBJECT_P_H
