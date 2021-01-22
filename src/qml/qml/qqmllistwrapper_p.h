/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

#ifndef QQMLLISTWRAPPER_P_H
#define QQMLLISTWRAPPER_P_H

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

#include <QtCore/qglobal.h>
#include <QtCore/qpointer.h>

#include <QtQml/qqmllist.h>

#include <private/qv4value_p.h>
#include <private/qv4object_p.h>

QT_BEGIN_NAMESPACE

namespace QV4 {

namespace Heap {

struct QmlListWrapper : Object {
    void init();
    void destroy();
    QQmlQPointer<QObject> object;

    QQmlListProperty<QObject> &property() {
        return *reinterpret_cast<QQmlListProperty<QObject>*>(propertyData);
    }

    int propertyType;

private:
    void *propertyData[sizeof(QQmlListProperty<QObject>)/sizeof(void*)];
};

}

struct Q_QML_EXPORT QmlListWrapper : Object
{
    V4_OBJECT2(QmlListWrapper, Object)
    V4_NEEDS_DESTROY
    V4_PROTOTYPE(propertyListPrototype)

    static ReturnedValue create(ExecutionEngine *engine, QObject *object, int propId, int propType);
    static ReturnedValue create(ExecutionEngine *engine, const QQmlListProperty<QObject> &prop, int propType);

    QVariant toVariant() const;

    static ReturnedValue virtualGet(const Managed *m, PropertyKey id, const Value *receiver, bool *hasProperty);
    static bool virtualPut(Managed *m, PropertyKey id, const Value &value, Value *receiver);
    static OwnPropertyKeyIterator *virtualOwnPropertyKeys(const Object *m, Value *target);
};

struct PropertyListPrototype : Object
{
    void init(ExecutionEngine *engine);

    static ReturnedValue method_push(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
};

}

QT_END_NAMESPACE

#endif

