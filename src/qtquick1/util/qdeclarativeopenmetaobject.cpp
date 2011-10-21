/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtDeclarative module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "QtQuick1/private/qdeclarativeopenmetaobject_p.h"
#include "QtDeclarative/private/qdeclarativepropertycache_p.h"
#include "QtDeclarative/private/qdeclarativedata_p.h"
#include <private/qmetaobjectbuilder_p.h>
#include <qdebug.h>

QT_BEGIN_NAMESPACE




class QDeclarative1OpenMetaObjectTypePrivate
{
public:
    QDeclarative1OpenMetaObjectTypePrivate() : mem(0), cache(0), engine(0) {}

    void init(const QMetaObject *metaObj);

    int propertyOffset;
    int signalOffset;
    QHash<QByteArray, int> names;
    QMetaObjectBuilder mob;
    QMetaObject *mem;
    QDeclarativePropertyCache *cache;
    QDeclarativeEngine *engine;
    QSet<QDeclarative1OpenMetaObject*> referers;
};

QDeclarative1OpenMetaObjectType::QDeclarative1OpenMetaObjectType(const QMetaObject *base, QDeclarativeEngine *engine)
    : d(new QDeclarative1OpenMetaObjectTypePrivate)
{
    d->engine = engine;
    d->init(base);
}

QDeclarative1OpenMetaObjectType::~QDeclarative1OpenMetaObjectType()
{
    if (d->mem)
        qFree(d->mem);
    if (d->cache)
        d->cache->release();
    delete d;
}


int QDeclarative1OpenMetaObjectType::propertyOffset() const
{
    return d->propertyOffset;
}

int QDeclarative1OpenMetaObjectType::signalOffset() const
{
    return d->signalOffset;
}

int QDeclarative1OpenMetaObjectType::createProperty(const QByteArray &name)
{
    int id = d->mob.propertyCount();
    d->mob.addSignal("__" + QByteArray::number(id) + "()");
    QMetaPropertyBuilder build = d->mob.addProperty(name, "QVariant", id);
    propertyCreated(id, build);
    qFree(d->mem);
    d->mem = d->mob.toMetaObject();
    d->names.insert(name, id);
    QSet<QDeclarative1OpenMetaObject*>::iterator it = d->referers.begin();
    while (it != d->referers.end()) {
        QDeclarative1OpenMetaObject *omo = *it;
        *static_cast<QMetaObject *>(omo) = *d->mem;
        if (d->cache)
            d->cache->update(d->engine, omo);
        ++it;
    }

    return d->propertyOffset + id;
}

void QDeclarative1OpenMetaObjectType::propertyCreated(int id, QMetaPropertyBuilder &builder)
{
    if (d->referers.count())
        (*d->referers.begin())->propertyCreated(id, builder);
}

void QDeclarative1OpenMetaObjectTypePrivate::init(const QMetaObject *metaObj)
{
    if (!mem) {
        mob.setSuperClass(metaObj);
        mob.setClassName(metaObj->className());
        mob.setFlags(QMetaObjectBuilder::DynamicMetaObject);

        mem = mob.toMetaObject();

        propertyOffset = mem->propertyOffset();
        signalOffset = mem->methodOffset();
    }
}

//----------------------------------------------------------------------------

class QDeclarative1OpenMetaObjectPrivate
{
public:
    QDeclarative1OpenMetaObjectPrivate(QDeclarative1OpenMetaObject *_q)
        : q(_q), parent(0), type(0), cacheProperties(false) {}

    inline QVariant &getData(int idx) {
        while (data.count() <= idx)
            data << QPair<QVariant, bool>(QVariant(), false);
        QPair<QVariant, bool> &prop = data[idx];
        if (!prop.second) {
            prop.first = q->initialValue(idx);
            prop.second = true;
        }
        return prop.first;
    }

    inline void writeData(int idx, const QVariant &value) {
        while (data.count() <= idx)
            data << QPair<QVariant, bool>(QVariant(), false);
        QPair<QVariant, bool> &prop = data[idx];
        prop.first = value;
        prop.second = true;
    }

    inline bool hasData(int idx) const {
        if (idx >= data.count())
            return false;
        return data[idx].second;
    }

    bool autoCreate;
    QDeclarative1OpenMetaObject *q;
    QAbstractDynamicMetaObject *parent;
    QList<QPair<QVariant, bool> > data;
    QObject *object;
    QDeclarative1OpenMetaObjectType *type;
    bool cacheProperties;
};

QDeclarative1OpenMetaObject::QDeclarative1OpenMetaObject(QObject *obj, bool automatic)
: d(new QDeclarative1OpenMetaObjectPrivate(this))
{
    d->autoCreate = automatic;
    d->object = obj;

    d->type = new QDeclarative1OpenMetaObjectType(obj->metaObject(), 0);
    d->type->d->referers.insert(this);

    QObjectPrivate *op = QObjectPrivate::get(obj);
    d->parent = static_cast<QAbstractDynamicMetaObject *>(op->metaObject);
    *static_cast<QMetaObject *>(this) = *d->type->d->mem;
    op->metaObject = this;
}

QDeclarative1OpenMetaObject::QDeclarative1OpenMetaObject(QObject *obj, QDeclarative1OpenMetaObjectType *type, bool automatic)
: d(new QDeclarative1OpenMetaObjectPrivate(this))
{
    d->autoCreate = automatic;
    d->object = obj;

    d->type = type;
    d->type->addref();
    d->type->d->referers.insert(this);

    QObjectPrivate *op = QObjectPrivate::get(obj);
    d->parent = static_cast<QAbstractDynamicMetaObject *>(op->metaObject);
    *static_cast<QMetaObject *>(this) = *d->type->d->mem;
    op->metaObject = this;
}

QDeclarative1OpenMetaObject::~QDeclarative1OpenMetaObject()
{
    if (d->parent)
        delete d->parent;
    d->type->d->referers.remove(this);
    d->type->release();
    delete d;
}

QDeclarative1OpenMetaObjectType *QDeclarative1OpenMetaObject::type() const
{
    return d->type;
}

int QDeclarative1OpenMetaObject::metaCall(QMetaObject::Call c, int id, void **a)
{
    if (( c == QMetaObject::ReadProperty || c == QMetaObject::WriteProperty)
            && id >= d->type->d->propertyOffset) {
        int propId = id - d->type->d->propertyOffset;
        if (c == QMetaObject::ReadProperty) {
            propertyRead(propId);
            *reinterpret_cast<QVariant *>(a[0]) = d->getData(propId);
        } else if (c == QMetaObject::WriteProperty) {
            if (propId <= d->data.count() || d->data[propId].first != *reinterpret_cast<QVariant *>(a[0]))  {
                propertyWrite(propId);
                d->writeData(propId, *reinterpret_cast<QVariant *>(a[0]));
                propertyWritten(propId);
                activate(d->object, d->type->d->signalOffset + propId, 0);
            }
        } 
        return -1;
    } else {
        if (d->parent)
            return d->parent->metaCall(c, id, a);
        else
            return d->object->qt_metacall(c, id, a);
    }
}

QAbstractDynamicMetaObject *QDeclarative1OpenMetaObject::parent() const
{
    return d->parent;
}

QVariant QDeclarative1OpenMetaObject::value(int id) const
{
    return d->getData(id);
}

void QDeclarative1OpenMetaObject::setValue(int id, const QVariant &value)
{
    d->writeData(id, value);
    activate(d->object, id + d->type->d->signalOffset, 0);
}

QVariant QDeclarative1OpenMetaObject::value(const QByteArray &name) const
{
    QHash<QByteArray, int>::ConstIterator iter = d->type->d->names.find(name);
    if (iter == d->type->d->names.end())
        return QVariant();

    return d->getData(*iter);
}

QVariant &QDeclarative1OpenMetaObject::operator[](const QByteArray &name)
{
    QHash<QByteArray, int>::ConstIterator iter = d->type->d->names.find(name);
    Q_ASSERT(iter != d->type->d->names.end());

    return d->getData(*iter);
}

QVariant &QDeclarative1OpenMetaObject::operator[](int id)
{
    return d->getData(id);
}

void QDeclarative1OpenMetaObject::setValue(const QByteArray &name, const QVariant &val)
{
    QHash<QByteArray, int>::ConstIterator iter = d->type->d->names.find(name);

    int id = -1;
    if (iter == d->type->d->names.end()) {
        id = createProperty(name.constData(), "") - d->type->d->propertyOffset;
    } else {
        id = *iter;
    }

    if (id >= 0) {
        QVariant &dataVal = d->getData(id);
        if (dataVal == val)
            return;

        dataVal = val;
        activate(d->object, id + d->type->d->signalOffset, 0);
    }
}

// returns true if this value has been initialized by a call to either value() or setValue()
bool QDeclarative1OpenMetaObject::hasValue(int id) const
{
    return d->hasData(id);
}

void QDeclarative1OpenMetaObject::setCached(bool c)
{
    if (c == d->cacheProperties || !d->type->d->engine)
        return;

    d->cacheProperties = c;

    QDeclarativeData *qmldata = QDeclarativeData::get(d->object, true);
    if (d->cacheProperties) {
        if (!d->type->d->cache)
            d->type->d->cache = new QDeclarativePropertyCache(d->type->d->engine, this);
        qmldata->propertyCache = d->type->d->cache;
        d->type->d->cache->addref();
    } else {
        if (d->type->d->cache)
            d->type->d->cache->release();
        qmldata->propertyCache = 0;
    }
}


int QDeclarative1OpenMetaObject::createProperty(const char *name, const char *)
{
    if (d->autoCreate)
        return d->type->createProperty(name);
    else
        return -1;
}

void QDeclarative1OpenMetaObject::propertyRead(int)
{
}

void QDeclarative1OpenMetaObject::propertyWrite(int)
{
}

void QDeclarative1OpenMetaObject::propertyWritten(int)
{
}

void QDeclarative1OpenMetaObject::propertyCreated(int, QMetaPropertyBuilder &)
{
}

QVariant QDeclarative1OpenMetaObject::initialValue(int)
{
    return QVariant();
}

int QDeclarative1OpenMetaObject::count() const
{
    return d->type->d->names.count();
}

QByteArray QDeclarative1OpenMetaObject::name(int idx) const
{
    Q_ASSERT(idx >= 0 && idx < d->type->d->names.count());

    return d->type->d->mob.property(idx).name();
}

QObject *QDeclarative1OpenMetaObject::object() const
{
    return d->object;
}



QT_END_NAMESPACE
