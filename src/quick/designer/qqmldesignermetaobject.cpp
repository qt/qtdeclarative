// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qqmldesignermetaobject_p.h"

#include <QSharedPointer>
#include <QMetaProperty>
#include <QtCore/private/qnumeric_p.h>
#include <QDebug>

#include <private/qqmlengine_p.h>

QT_BEGIN_NAMESPACE

static QHash<QDynamicMetaObjectData *, bool> nodeInstanceMetaObjectList;
static void (*notifyPropertyChangeCallBack)(QObject*, const QQuickDesignerSupport::PropertyName &propertyName) = nullptr;

struct MetaPropertyData {
    inline QPair<QVariant, bool> &getDataRef(int idx) {
        while (m_data.size() <= idx)
            m_data << QPair<QVariant, bool>(QVariant(), false);
        return m_data[idx];
    }

    inline QVariant &getData(int idx) {
        QPair<QVariant, bool> &prop = getDataRef(idx);
        if (!prop.second) {
            prop.first = QVariant();
            prop.second = true;
        }
        return prop.first;
    }

    inline bool hasData(int idx) const {
        if (idx >= m_data.size())
            return false;
        return m_data[idx].second;
    }

    inline int count() { return m_data.size(); }

    QVector<QPair<QVariant, bool> > m_data;
};

QQmlDesignerMetaObject* QQmlDesignerMetaObject::getNodeInstanceMetaObject(QObject *object, QQmlEngine *engine)
{
    //Avoid setting up multiple MetaObjects on the same QObject
    QObjectPrivate *op = QObjectPrivate::get(object);
    QDynamicMetaObjectData *parent = op->metaObject;
    if (nodeInstanceMetaObjectList.contains(parent))
        return static_cast<QQmlDesignerMetaObject *>(parent);

    // we just create one and the ownership goes automatically to the object in nodeinstance see init method

    QQmlData *ddata = QQmlData::get(object, false);

    QQmlDesignerMetaObject *mo = new QQmlDesignerMetaObject(object, engine);
    ddata->hasVMEMetaObject = false;
    ddata->hasInterceptorMetaObject = false;

    return mo;
}

void QQmlDesignerMetaObject::init(QObject *object)
{
    //Assign this to object
    QObjectPrivate *op = QObjectPrivate::get(object);
    op->metaObject = this;
    nodeInstanceMetaObjectList.insert(this, true);
}

QQmlPropertyCache::Ptr QQmlDesignerMetaObject::cache() const
{
    return type()->cache();
}

QQmlDesignerMetaObject::QQmlDesignerMetaObject(QObject *object, QQmlEngine *engine)
    : QQmlOpenMetaObject(object),
      m_context(engine->contextForObject(object)),
      m_data(new MetaPropertyData)
{
    init(object);
    setCached(true);
}

QQmlDesignerMetaObject::~QQmlDesignerMetaObject()
{
    nodeInstanceMetaObjectList.remove(this);
}

void QQmlDesignerMetaObject::createNewDynamicProperty(const QString &name)
{
    int id = type()->createProperty(name.toUtf8());
    setValue(id, QVariant());
    Q_ASSERT(id >= 0);

    //Updating cache
    cache()->invalidate(this);

    QQmlProperty property(myObject(), name, m_context);
    Q_ASSERT(property.isValid());
}

int QQmlDesignerMetaObject::createProperty(const char *name, const char *passAlong)
{
    int ret = QQmlOpenMetaObject::createProperty(name, passAlong);
    if (ret != -1) {
        // compare createNewDynamicProperty
        cache()->invalidate(this);
    }
    return ret;
}

void QQmlDesignerMetaObject::setValue(int id, const QVariant &value)
{
    QPair<QVariant, bool> &prop = m_data->getDataRef(id);
    prop.first = propertyWriteValue(id, value);
    prop.second = true;
    QMetaObject::activate(myObject(), id + type()->signalOffset(), nullptr);
}

QVariant QQmlDesignerMetaObject::propertyWriteValue(int, const QVariant &value)
{
    return value;
}

QDynamicMetaObjectData *QQmlDesignerMetaObject::dynamicMetaObjectParent() const
{
    return parent();
}

int QQmlDesignerMetaObject::propertyOffset() const
{
    return cache()->propertyOffset();
}

int QQmlDesignerMetaObject::openMetaCall(QObject *o, QMetaObject::Call call, int id, void **a)
{
    if ((call == QMetaObject::ReadProperty || call == QMetaObject::WriteProperty)
            && id >= type()->propertyOffset()) {
        int propId = id - type()->propertyOffset();
        if (call == QMetaObject::ReadProperty) {
            //propertyRead(propId);
            *reinterpret_cast<QVariant *>(a[0]) = m_data->getData(propId);
        } else if (call == QMetaObject::WriteProperty) {
            if (propId <= m_data->count() || m_data->m_data[propId].first != *reinterpret_cast<QVariant *>(a[0]))  {
                //propertyWrite(propId);
                QPair<QVariant, bool> &prop = m_data->getDataRef(propId);
                prop.first = propertyWriteValue(propId, *reinterpret_cast<QVariant *>(a[0]));
                prop.second = true;
                //propertyWritten(propId);
                activate(myObject(), type()->signalOffset() + propId, nullptr);
            }
        }
        return -1;
    } else {
        QDynamicMetaObjectData *dynamicParent = dynamicMetaObjectParent();
        if (dynamicParent)
            return dynamicParent->metaCall(o, call, id, a);
        else
            return myObject()->qt_metacall(call, id, a);
    }
}

int QQmlDesignerMetaObject::metaCall(QObject *o, QMetaObject::Call call, int id, void **a)
{
    Q_ASSERT(myObject() == o);

    int metaCallReturnValue = -1;

    const QMetaProperty propertyById = property(id);

    if (call == QMetaObject::WriteProperty
            && propertyById.userType() == QMetaType::QVariant
            && reinterpret_cast<QVariant *>(a[0])->userType() == QMetaType::Double
            && qt_is_nan(reinterpret_cast<QVariant *>(a[0])->toDouble())) {
        return -1;
    }

    if (call == QMetaObject::WriteProperty
            && propertyById.userType() == QMetaType::Double
            && qt_is_nan(*reinterpret_cast<double*>(a[0]))) {
        return -1;
    }

    if (call == QMetaObject::WriteProperty
            && propertyById.userType() == QMetaType::Float
            && qt_is_nan(*reinterpret_cast<float*>(a[0]))) {
        return -1;
    }

    QVariant oldValue;

    if (call == QMetaObject::WriteProperty && !propertyById.hasNotifySignal())
    {
        oldValue = propertyById.read(myObject());
    }

    QDynamicMetaObjectData *dynamicParent = dynamicMetaObjectParent();
    const QMetaObject *staticParent = dynamicParent
            ? dynamicParent->toDynamicMetaObject(object())
            : nullptr;
    if (staticParent && id < staticParent->propertyOffset())
        metaCallReturnValue = dynamicParent->metaCall(o, call, id, a);
    else
        openMetaCall(o, call, id, a);


    if (call == QMetaObject::WriteProperty
            && !propertyById.hasNotifySignal()
            && oldValue != propertyById.read(myObject()))
        notifyPropertyChange(id);

    return metaCallReturnValue;
}

void QQmlDesignerMetaObject::notifyPropertyChange(int id)
{
    const QMetaProperty propertyById = property(id);

    if (id < propertyOffset()) {
        if (notifyPropertyChangeCallBack)
            notifyPropertyChangeCallBack(myObject(), propertyById.name());
    } else {
        if (notifyPropertyChangeCallBack)
            notifyPropertyChangeCallBack(myObject(), name(id - propertyOffset()));
    }
}

int QQmlDesignerMetaObject::count() const
{
    return type()->propertyCount();
}

QByteArray QQmlDesignerMetaObject::name(int idx) const
{
    return type()->propertyName(idx);
}

void QQmlDesignerMetaObject::registerNotifyPropertyChangeCallBack(void (*callback)(QObject *, const QQuickDesignerSupport::PropertyName &))
{
    notifyPropertyChangeCallBack = callback;
}

QT_END_NAMESPACE
