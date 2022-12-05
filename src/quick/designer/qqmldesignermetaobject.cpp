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

static QQmlPropertyCache::ConstPtr cacheForObject(QObject *object)
{
    QQmlVMEMetaObject *metaObject = QQmlVMEMetaObject::get(object);
    if (metaObject)
        return metaObject->cache;

    return QQmlMetaType::propertyCache(object);
}

QQmlDesignerMetaObject* QQmlDesignerMetaObject::getNodeInstanceMetaObject(QObject *object, QQmlEngine *engine)
{
    //Avoid setting up multiple MetaObjects on the same QObject
    QObjectPrivate *op = QObjectPrivate::get(object);
    QDynamicMetaObjectData *parent = op->metaObject;
    if (nodeInstanceMetaObjectList.contains(parent))
        return static_cast<QQmlDesignerMetaObject *>(parent);

    // we just create one and the ownership goes automatically to the object in nodeinstance see init method

    QQmlData *ddata = QQmlData::get(object, false);

    const bool hadVMEMetaObject = ddata ? ddata->hasVMEMetaObject : false;
    QQmlDesignerMetaObject *mo = new QQmlDesignerMetaObject(object, engine);
    //If our parent is not a VMEMetaObject we just set the flag to false again
    if (ddata)
        ddata->hasVMEMetaObject = hadVMEMetaObject;
    return mo;
}

void QQmlDesignerMetaObject::init(QObject *object)
{
    //Creating QQmlOpenMetaObjectType
    m_openMetaObject = std::make_unique<QQmlOpenMetaObject>(object, metaObjectParent());
    //Assigning type to this
    copyTypeMetaObject();

    //Assign this to object
    QObjectPrivate *op = QObjectPrivate::get(object);
    op->metaObject = this;

    m_cache = QQmlPropertyCache::createStandalone(metaObject.data());
    cache = m_cache;

    nodeInstanceMetaObjectList.insert(this, true);
}

QQmlDesignerMetaObject::QQmlDesignerMetaObject(QObject *object, QQmlEngine *engine)
    : QQmlVMEMetaObject(engine->handle(), object, cacheForObject(object), /*qml compilation unit*/nullptr, /*qmlObjectId*/-1),
      m_context(engine->contextForObject(object)),
      m_data(new MetaPropertyData)
{
    init(object);

    QQmlData *ddata = QQmlData::get(object, false);
    //Assign cache to object
    if (ddata && ddata->propertyCache) {
        m_cache->setParent(ddata->propertyCache);
        m_cache->invalidate(metaObject.data());
        ddata->propertyCache = m_cache;
    }

}

QQmlDesignerMetaObject::~QQmlDesignerMetaObject()
{
    // m_openMetaObject has this metaobject as its parent.
    // We need to remove it in order to avoid a dtor recursion
    m_openMetaObject->unparent();
    nodeInstanceMetaObjectList.remove(this);
}

void QQmlDesignerMetaObject::createNewDynamicProperty(const QString &name)
{
    int id = type()->createProperty(name.toUtf8());
    copyTypeMetaObject();
    setValue(id, QVariant());
    Q_ASSERT(id >= 0);

    //Updating cache
    m_cache->invalidate(metaObject.data());

    QQmlProperty property(myObject(), name, m_context);
    Q_ASSERT(property.isValid());
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
    if (QQmlVMEMetaObject::parent.isT1())
        return QQmlVMEMetaObject::parent.asT1();
    else
        return nullptr;
}

const QMetaObject *QQmlDesignerMetaObject::metaObjectParent() const
{
    if (QQmlVMEMetaObject::parent.isT1())
        return QQmlVMEMetaObject::parent.asT1()->toDynamicMetaObject(QQmlVMEMetaObject::object);

    return QQmlVMEMetaObject::parent.asT2();
}

int QQmlDesignerMetaObject::propertyOffset() const
{
    return cache->propertyOffset();
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

    const QMetaProperty propertyById = metaObject->property(id);

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
            ? dynamicParent->toDynamicMetaObject(QQmlVMEMetaObject::object)
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
    const QMetaProperty propertyById = metaObject->property(id);

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

void QQmlDesignerMetaObject::copyTypeMetaObject()
{
    metaObject = m_openMetaObject.get();
}

void QQmlDesignerMetaObject::registerNotifyPropertyChangeCallBack(void (*callback)(QObject *, const QQuickDesignerSupport::PropertyName &))
{
    notifyPropertyChangeCallBack = callback;
}

QT_END_NAMESPACE
