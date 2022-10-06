// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qqmlopenmetaobject_p.h"
#include <private/qqmlpropertycache_p.h>
#include <private/qqmldata_p.h>
#include <private/qqmlmetatype_p.h>
#include <private/qmetaobjectbuilder_p.h>
#include <qdebug.h>
#include <QtCore/qset.h>

QT_BEGIN_NAMESPACE


class QQmlOpenMetaObjectTypePrivate
{
public:
    QQmlOpenMetaObjectTypePrivate() : mem(nullptr) {}

    void init(const QMetaObject *metaObj);

    int propertyOffset;
    int signalOffset;
    QHash<QByteArray, int> names;
    QMetaObjectBuilder mob;
    QMetaObject *mem;

    // TODO: We need to make sure that this does not escape into other threads.
    //       In particular, all its non-const uses are probably wrong. You should
    //       only set the open metaobject to "cached" once it's not going to be
    //       modified anymore.
    QQmlPropertyCache::Ptr cache;

    QSet<QQmlOpenMetaObject*> referers;
};

QQmlOpenMetaObjectType::QQmlOpenMetaObjectType(const QMetaObject *base)
    : d(new QQmlOpenMetaObjectTypePrivate)
{
    d->init(base);
}

QQmlOpenMetaObjectType::~QQmlOpenMetaObjectType()
{
    if (d->mem)
        free(d->mem);
    delete d;
}

int QQmlOpenMetaObjectType::propertyOffset() const
{
    return d->propertyOffset;
}

int QQmlOpenMetaObjectType::signalOffset() const
{
    return d->signalOffset;
}

int QQmlOpenMetaObjectType::propertyCount() const
{
    return d->names.size();
}

QByteArray QQmlOpenMetaObjectType::propertyName(int idx) const
{
    Q_ASSERT(idx >= 0 && idx < d->names.size());

    return d->mob.property(idx).name();
}

void QQmlOpenMetaObjectType::createProperties(const QVector<QByteArray> &names)
{
    for (int i = 0; i < names.size(); ++i) {
        const QByteArray &name = names.at(i);
        const int id = d->mob.propertyCount();
        d->mob.addSignal("__" + QByteArray::number(id) + "()");
        QMetaPropertyBuilder build = d->mob.addProperty(name, "QVariant", id);
        propertyCreated(id, build);
        d->names.insert(name, id);
    }
    free(d->mem);
    d->mem = d->mob.toMetaObject();
    QSet<QQmlOpenMetaObject*>::iterator it = d->referers.begin();
    while (it != d->referers.end()) {
        QQmlOpenMetaObject *omo = *it;
        *static_cast<QMetaObject *>(omo) = *d->mem;
        if (d->cache)
            d->cache->update(omo);
        ++it;
    }
}

int QQmlOpenMetaObjectType::createProperty(const QByteArray &name)
{
    const int signalIdx = d->mob.addSignal(
                "__" + QByteArray::number(d->mob.propertyCount()) + "()").index();
    QMetaPropertyBuilder build = d->mob.addProperty(name, "QVariant", signalIdx);
    propertyCreated(build.index(), build);
    free(d->mem);
    d->mem = d->mob.toMetaObject();
    d->names.insert(name, build.index());
    QSet<QQmlOpenMetaObject*>::iterator it = d->referers.begin();
    while (it != d->referers.end()) {
        QQmlOpenMetaObject *omo = *it;
        *static_cast<QMetaObject *>(omo) = *d->mem;
        if (d->cache)
            d->cache->update(omo);
        ++it;
    }

    return d->propertyOffset + build.index();
}

void QQmlOpenMetaObjectType::propertyCreated(int id, QMetaPropertyBuilder &builder)
{
    if (d->referers.size())
        (*d->referers.begin())->propertyCreated(id, builder);
}

void QQmlOpenMetaObjectTypePrivate::init(const QMetaObject *metaObj)
{
    if (!mem) {
        mob.setSuperClass(metaObj);
        mob.setClassName(metaObj->className());
        mob.setFlags(MetaObjectFlag::DynamicMetaObject);

        mem = mob.toMetaObject();

        propertyOffset = mem->propertyOffset();
        signalOffset = mem->methodOffset();
    }
}

//----------------------------------------------------------------------------

class QQmlOpenMetaObjectPrivate
{
public:
    QQmlOpenMetaObjectPrivate(QQmlOpenMetaObject *_q, QObject *obj)
        : q(_q), object(obj) {}

    struct Property {
    private:
        QVariant m_value;
        QPointer<QObject> qobjectTracker;
    public:
        bool valueSet = false;

        QVariant value() const {
            if (m_value.metaType().flags() & QMetaType::PointerToQObject
                && qobjectTracker.isNull())
                return QVariant::fromValue<QObject*>(nullptr);
            return m_value;
        }
        QVariant &valueRef() { return m_value; }
        void setValue(const QVariant &v) {
            m_value = v;
            valueSet = true;
            if (v.metaType().flags() & QMetaType::PointerToQObject)
                qobjectTracker = m_value.value<QObject*>();
        }
    };

    inline void setPropertyValue(int idx, const QVariant &value) {
        if (data.size() <= idx)
            data.resize(idx + 1);
        data[idx].setValue(value);
    }

    inline Property &propertyRef(int idx) {
        if (data.size() <= idx)
            data.resize(idx + 1);
        Property &prop = data[idx];
        if (!prop.valueSet)
            prop.setValue(q->initialValue(idx));
        return prop;
    }

    inline QVariant propertyValue(int idx) {
        auto &prop = propertyRef(idx);
        return prop.value();
    }

    inline QVariant &propertyValueRef(int idx) {
        auto &prop = propertyRef(idx);
        return prop.valueRef();
    }

    inline bool hasProperty(int idx) const {
        if (idx >= data.size())
            return false;
        return data[idx].valueSet;
    }

    void dropPropertyCache() {
        if (QQmlData *ddata = QQmlData::get(object, /*create*/false))
            ddata->propertyCache.reset();
    }

    QQmlOpenMetaObject *q;
    QDynamicMetaObjectData *parent = nullptr;
    QVector<Property> data;
    QObject *object;
    QQmlRefPointer<QQmlOpenMetaObjectType> type;
    QVector<QByteArray> *deferredPropertyNames = nullptr;
    bool autoCreate = true;
    bool cacheProperties = false;
};

QQmlOpenMetaObject::QQmlOpenMetaObject(QObject *obj, const QMetaObject *base)
: d(new QQmlOpenMetaObjectPrivate(this, obj))
{
    d->type.adopt(new QQmlOpenMetaObjectType(base ? base : obj->metaObject()));
    d->type->d->referers.insert(this);

    QObjectPrivate *op = QObjectPrivate::get(obj);
    d->parent = op->metaObject;
    *static_cast<QMetaObject *>(this) = *d->type->d->mem;
    op->metaObject = this;
}

QQmlOpenMetaObject::QQmlOpenMetaObject(
        QObject *obj, const QQmlRefPointer<QQmlOpenMetaObjectType> &type)
: d(new QQmlOpenMetaObjectPrivate(this, obj))
{
    d->type = type;
    d->type->d->referers.insert(this);

    QObjectPrivate *op = QObjectPrivate::get(obj);
    d->parent = op->metaObject;
    *static_cast<QMetaObject *>(this) = *d->type->d->mem;
    op->metaObject = this;
}

QQmlOpenMetaObject::~QQmlOpenMetaObject()
{
    if (d->parent)
        delete d->parent;
    d->type->d->referers.remove(this);
    delete d;
}

QQmlOpenMetaObjectType *QQmlOpenMetaObject::type() const
{
    return d->type.data();
}

void QQmlOpenMetaObject::emitPropertyNotification(const QByteArray &propertyName)
{
    QHash<QByteArray, int>::ConstIterator iter = d->type->d->names.constFind(propertyName);
    if (iter == d->type->d->names.constEnd())
        return;
    activate(d->object, *iter + d->type->d->signalOffset, nullptr);
}

void QQmlOpenMetaObject::unparent()
{
    d->parent = nullptr;
}

int QQmlOpenMetaObject::metaCall(QObject *o, QMetaObject::Call c, int id, void **a)
{
    Q_ASSERT(d->object == o);

    if (( c == QMetaObject::ReadProperty || c == QMetaObject::WriteProperty)
            && id >= d->type->d->propertyOffset) {
        int propId = id - d->type->d->propertyOffset;
        if (c == QMetaObject::ReadProperty) {
            propertyRead(propId);
            *reinterpret_cast<QVariant *>(a[0]) = d->propertyValue(propId);
        } else if (c == QMetaObject::WriteProperty) {
            if (propId >= d->data.size() || d->data.at(propId).value() != *reinterpret_cast<QVariant *>(a[0]))  {
                propertyWrite(propId);
                d->setPropertyValue(propId, propertyWriteValue(propId, *reinterpret_cast<QVariant *>(a[0])));
                propertyWritten(propId);
                activate(o, d->type->d->signalOffset + propId, nullptr);
            }
        }
        return -1;
    } else {
        if (d->parent)
            return d->parent->metaCall(o, c, id, a);
        else
            return o->qt_metacall(c, id, a);
    }
}

QDynamicMetaObjectData *QQmlOpenMetaObject::parent() const
{
    return d->parent;
}

bool QQmlOpenMetaObject::checkedSetValue(int index, const QVariant &value, bool force)
{
    if (!force && d->propertyValue(index) == value)
        return false;

    d->setPropertyValue(index, value);
    activate(d->object, index + d->type->d->signalOffset, nullptr);
    return true;
}

QVariant QQmlOpenMetaObject::value(int id) const
{
    return d->propertyValue(id);
}

void QQmlOpenMetaObject::setValue(int id, const QVariant &value)
{
    d->setPropertyValue(id, propertyWriteValue(id, value));
    activate(d->object, id + d->type->d->signalOffset, nullptr);
}

QVariant QQmlOpenMetaObject::value(const QByteArray &name) const
{
    QHash<QByteArray, int>::ConstIterator iter = d->type->d->names.constFind(name);
    if (iter == d->type->d->names.cend())
        return QVariant();

    return d->propertyValue(*iter);
}

QVariant &QQmlOpenMetaObject::valueRef(const QByteArray &name)
{
    QHash<QByteArray, int>::ConstIterator iter = d->type->d->names.constFind(name);
    Q_ASSERT(iter != d->type->d->names.cend());

    return d->propertyValueRef(*iter);
}

bool QQmlOpenMetaObject::setValue(const QByteArray &name, const QVariant &val, bool force)
{
    QHash<QByteArray, int>::ConstIterator iter = d->type->d->names.constFind(name);

    int id = -1;
    if (iter == d->type->d->names.cend()) {
        id = createProperty(name.constData(), "") - d->type->d->propertyOffset;
    } else {
        id = *iter;
    }

    if (id >= 0)
        return checkedSetValue(id, val, force);

    return false;
}

void QQmlOpenMetaObject::setValues(const QHash<QByteArray, QVariant> &values, bool force)
{
    QVector<QByteArray> missingProperties;
    d->deferredPropertyNames = &missingProperties;
    const auto &names = d->type->d->names;

    for (auto valueIt = values.begin(), end = values.end(); valueIt != end; ++valueIt) {
        const auto nameIt = names.constFind(valueIt.key());
        if (nameIt == names.constEnd()) {
            const int id = createProperty(valueIt.key(), "") - d->type->d->propertyOffset;

            // If id >= 0 some override of createProperty() created it. Then set it.
            // Else it either ends up in missingProperties and we create it later
            // or it cannot be created.

            if (id >= 0)
                checkedSetValue(id, valueIt.value(), force);
        } else {
            checkedSetValue(*nameIt, valueIt.value(), force);
        }
    }

    d->deferredPropertyNames = nullptr;
    if (missingProperties.isEmpty())
        return;

    d->type->createProperties(missingProperties);
    d->dropPropertyCache();

    for (const QByteArray &name : std::as_const(missingProperties))
        checkedSetValue(names[name], values[name], force);
}

// returns true if this value has been initialized by a call to either value() or setValue()
bool QQmlOpenMetaObject::hasValue(int id) const
{
    return d->hasProperty(id);
}

void QQmlOpenMetaObject::setCached(bool c)
{
    if (c == d->cacheProperties)
        return;

    d->cacheProperties = c;

    QQmlData *qmldata = QQmlData::get(d->object, true);
    if (d->cacheProperties) {
        // As the propertyCache is not saved in QQmlMetaType (due to it being dynamic)
        // we cannot leak it to other places before we're done with it. Yes, it's still
        // terrible.
        if (!d->type->d->cache)
            d->type->d->cache = QQmlPropertyCache::createStandalone(this);
        qmldata->propertyCache = d->type->d->cache;
    } else {
        d->type->d->cache.reset();
        qmldata->propertyCache.reset();
    }
}

bool QQmlOpenMetaObject::autoCreatesProperties() const
{
    return d->autoCreate;
}

void QQmlOpenMetaObject::setAutoCreatesProperties(bool autoCreate)
{
    d->autoCreate = autoCreate;
}


int QQmlOpenMetaObject::createProperty(const char *name, const char *)
{
    if (d->autoCreate) {
        if (d->deferredPropertyNames) {
            // Defer the creation of new properties. See setValues(QHash<QByteArray, QVariant>)
            d->deferredPropertyNames->append(name);
            return -1;
        }

        const int result = d->type->createProperty(name);
        d->dropPropertyCache();
        return result;
    } else
        return -1;
}

void QQmlOpenMetaObject::propertyRead(int)
{
}

void QQmlOpenMetaObject::propertyWrite(int)
{
}

QVariant QQmlOpenMetaObject::propertyWriteValue(int, const QVariant &value)
{
    return value;
}

void QQmlOpenMetaObject::propertyWritten(int)
{
}

void QQmlOpenMetaObject::propertyCreated(int, QMetaPropertyBuilder &)
{
}

QVariant QQmlOpenMetaObject::initialValue(int)
{
    return QVariant();
}

int QQmlOpenMetaObject::count() const
{
    return d->type->d->names.size();
}

QByteArray QQmlOpenMetaObject::name(int idx) const
{
    Q_ASSERT(idx >= 0 && idx < d->type->d->names.size());

    return d->type->d->mob.property(idx).name();
}

QObject *QQmlOpenMetaObject::object() const
{
    return d->object;
}

QT_END_NAMESPACE
