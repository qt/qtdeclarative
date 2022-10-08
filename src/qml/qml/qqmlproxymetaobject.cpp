// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qqmlproxymetaobject_p.h"
#include "qqmlproperty_p.h"

QT_BEGIN_NAMESPACE

QQmlProxyMetaObject::QQmlProxyMetaObject(QObject *obj, QList<ProxyData> *mList)
: metaObjects(mList), proxies(nullptr), parent(nullptr), object(obj)
{
    metaObject = metaObjects->constFirst().metaObject;

    QObjectPrivate *op = QObjectPrivate::get(obj);
    if (op->metaObject)
        parent = op->metaObject;

    op->metaObject = this;
}

QQmlProxyMetaObject::~QQmlProxyMetaObject()
{
    if (parent)
        delete parent;
    parent = nullptr;

    if (proxies)
        delete [] proxies;
    proxies = nullptr;
}

QObject *QQmlProxyMetaObject::getProxy(int index)
{
    if (!proxies) {
        proxies = new QObject *[metaObjects->size()];
        ::memset(proxies, 0, sizeof(QObject *) * metaObjects->size());
    }

    if (!proxies[index]) {
        const ProxyData &data = metaObjects->at(index);
        if (!data.createFunc)
            return nullptr;

        QObject *proxy = data.createFunc(object);
        const QMetaObject *metaObject = proxy->metaObject();
        proxies[index] = proxy;

        int localOffset = data.metaObject->methodOffset();
        int methodOffset = metaObject->methodOffset();
        int methods = metaObject->methodCount() - methodOffset;

        // ### - Can this be done more optimally?
        for (int jj = 0; jj < methods; ++jj) {
            QMetaMethod method =
                metaObject->method(jj + methodOffset);
            if (method.methodType() == QMetaMethod::Signal)
                QQmlPropertyPrivate::connect(proxy, methodOffset + jj, object, localOffset + jj);
        }
    }

    return proxies[index];
}

int QQmlProxyMetaObject::metaCall(QObject *o, QMetaObject::Call c, int id, void **a)
{
    Q_ASSERT(object == o);

    switch (c) {
    case QMetaObject::ReadProperty:
    case QMetaObject::WriteProperty: {
        if (id < metaObjects->constLast().propertyOffset)
            break;

        for (int ii = 0; ii < metaObjects->size(); ++ii) {
            const int globalPropertyOffset = metaObjects->at(ii).propertyOffset;
            if (id < globalPropertyOffset)
                continue;

            QObject *proxy = getProxy(ii);
            Q_ASSERT(proxy);
            const int localProxyOffset = proxy->metaObject()->propertyOffset();
            const int localProxyId = id - globalPropertyOffset + localProxyOffset;
            return proxy->qt_metacall(c, localProxyId, a);
        }
        break;
    }
    case QMetaObject::InvokeMetaMethod: {
        if (id < metaObjects->constLast().methodOffset)
            break;

        QMetaMethod m = object->metaObject()->method(id);
        if (m.methodType() == QMetaMethod::Signal) {
            QMetaObject::activate(object, id, a);
            return -1;
        }

        for (int ii = 0; ii < metaObjects->size(); ++ii) {
            const int globalMethodOffset = metaObjects->at(ii).methodOffset;
            if (id < globalMethodOffset)
                continue;

            QObject *proxy = getProxy(ii);
            Q_ASSERT(proxy);
            const int localMethodOffset = proxy->metaObject()->methodOffset();
            const int localMethodId = id - globalMethodOffset + localMethodOffset;
            return proxy->qt_metacall(c, localMethodId, a);
        }

        break;
    }
    case QMetaObject::CustomCall: {
        if ((id & ~MaxExtensionCount) != ExtensionObjectId)
            break;
        int index = id & MaxExtensionCount;
        if (qsizetype(index) >= metaObjects->size())
            break;
        a[0] = getProxy(index);
        return id;
    }
    default:
        break;
    }

    if (parent)
        return parent->metaCall(o, c, id, a);
    else
        return object->qt_metacall(c, id, a);
}

QMetaObject *QQmlProxyMetaObject::toDynamicMetaObject(QObject *)
{
    return metaObject;
}

QT_END_NAMESPACE
