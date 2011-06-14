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

#include "private/qdeclarativepropertycache_p.h"

#include "private/qdeclarativeengine_p.h"
#include "private/qdeclarativebinding_p.h"
#include "private/qv8engine_p.h"

#include <private/qmetaobject_p.h>

#include <QtCore/qdebug.h>

Q_DECLARE_METATYPE(QScriptValue)
Q_DECLARE_METATYPE(QDeclarativeV8Handle);

QT_BEGIN_NAMESPACE

QDeclarativePropertyCache::Data::Flags QDeclarativePropertyCache::Data::flagsForProperty(const QMetaProperty &p, QDeclarativeEngine *engine) 
{
    int propType = p.userType();

    Flags flags;

    if (p.isConstant())
        flags |= Data::IsConstant;
    if (p.isWritable())
        flags |= Data::IsWritable;
    if (p.isResettable())
        flags |= Data::IsResettable;
    if (p.isFinal())
        flags |= Data::IsFinal;

    if (propType == qMetaTypeId<QDeclarativeBinding *>()) {
        flags |= Data::IsQmlBinding;
    } else if (propType == qMetaTypeId<QScriptValue>()) {
        flags |= Data::IsQScriptValue;
    } else if (propType == qMetaTypeId<QDeclarativeV8Handle>()) {
        flags |= Data::IsV8Handle;
    } else if (p.isEnumType()) {
        flags |= Data::IsEnumType;
    } else {
        QDeclarativeMetaType::TypeCategory cat = engine ? QDeclarativeEnginePrivate::get(engine)->typeCategory(propType)
                                               : QDeclarativeMetaType::typeCategory(propType);
        if (cat == QDeclarativeMetaType::Object)
            flags |= Data::IsQObjectDerived;
        else if (cat == QDeclarativeMetaType::List)
            flags |= Data::IsQList;
    }

    return flags;
}

void QDeclarativePropertyCache::Data::load(const QMetaProperty &p, QDeclarativeEngine *engine)
{
    propType = p.userType();
    if (QVariant::Type(propType) == QVariant::LastType)
        propType = qMetaTypeId<QVariant>();
    coreIndex = p.propertyIndex();
    notifyIndex = p.notifySignalIndex();
    flags = flagsForProperty(p, engine);
    revision = p.revision();
}

void QDeclarativePropertyCache::Data::load(const QMetaMethod &m)
{
    coreIndex = m.methodIndex();
    relatedIndex = -1;
    flags |= Data::IsFunction;
    if (m.methodType() == QMetaMethod::Signal)
        flags |= Data::IsSignal;
    propType = QVariant::Invalid;

    const char *returnType = m.typeName();
    if (returnType) 
        propType = QMetaType::type(returnType);

    QList<QByteArray> params = m.parameterTypes();
    if (!params.isEmpty()) {
        flags |= Data::HasArguments;
        if (params.at(0).length() == 23 && 
            0 == qstrcmp(params.at(0).constData(), "QDeclarativeV8Function*")) {
            flags |= Data::IsV8Function;
        }
    }
    revision = m.revision();
}


/*!
Creates a new empty QDeclarativePropertyCache.
*/
QDeclarativePropertyCache::QDeclarativePropertyCache(QDeclarativeEngine *e)
: QDeclarativeCleanup(e), engine(e)
{
    Q_ASSERT(engine);
}

/*!
Creates a new QDeclarativePropertyCache of \a metaObject.
*/
QDeclarativePropertyCache::QDeclarativePropertyCache(QDeclarativeEngine *e, const QMetaObject *metaObject)
: QDeclarativeCleanup(e), engine(e)
{
    Q_ASSERT(engine);
    Q_ASSERT(metaObject);

    update(engine, metaObject);
}

QDeclarativePropertyCache::~QDeclarativePropertyCache()
{
    clear();
}

void QDeclarativePropertyCache::clear()
{
    for (int ii = 0; ii < indexCache.count(); ++ii) {
        if (indexCache.at(ii)) indexCache.at(ii)->release();
    }

    for (int ii = 0; ii < methodIndexCache.count(); ++ii) {
        RData *data = methodIndexCache.at(ii);
        if (data) data->release(); 
    }

    for (StringCache::ConstIterator iter = stringCache.begin(); 
            iter != stringCache.end(); ++iter) {
        RData *data = (*iter);
        data->release(); 
    }

    indexCache.clear();
    methodIndexCache.clear();
    stringCache.clear();
    qPersistentDispose(constructor);
}

QDeclarativePropertyCache::Data QDeclarativePropertyCache::create(const QMetaObject *metaObject, 
                                                                  const QString &property)
{
    Q_ASSERT(metaObject);

    QDeclarativePropertyCache::Data rv;
    {
        const QMetaObject *cmo = metaObject;
        while (cmo) {
            int idx = metaObject->indexOfProperty(property.toUtf8());
            if (idx != -1) {
                QMetaProperty p = metaObject->property(idx);
                if (p.isScriptable()) {
                    rv.load(metaObject->property(idx));
                    return rv;
                } else {
                    while (cmo && cmo->propertyOffset() >= idx)
                        cmo = cmo->superClass();
                }
            } else {
                cmo = 0;
            }
        }
    }

    int methodCount = metaObject->methodCount();
    for (int ii = methodCount - 1; ii >= 3; --ii) { // >=3 to block the destroyed signal and deleteLater() slot
        QMetaMethod m = metaObject->method(ii);
        if (m.access() == QMetaMethod::Private)
            continue;
        QString methodName = QString::fromUtf8(m.signature());

        int parenIdx = methodName.indexOf(QLatin1Char('('));
        Q_ASSERT(parenIdx != -1);
        QStringRef methodNameRef = methodName.leftRef(parenIdx);

        if (methodNameRef == property) {
            rv.load(m);
            return rv;
        }
    }

    return rv;
}

QDeclarativePropertyCache *QDeclarativePropertyCache::copy() const
{
    QDeclarativePropertyCache *cache = new QDeclarativePropertyCache(engine);
    cache->indexCache = indexCache;
    cache->methodIndexCache = methodIndexCache;
    cache->stringCache = stringCache;
    cache->allowedRevisionCache = allowedRevisionCache;

    for (int ii = 0; ii < indexCache.count(); ++ii) {
        if (indexCache.at(ii)) indexCache.at(ii)->addref();
    }
    for (int ii = 0; ii < methodIndexCache.count(); ++ii) {
        if (methodIndexCache.at(ii)) methodIndexCache.at(ii)->addref();
    }
    for (StringCache::ConstIterator iter = stringCache.begin(); iter != stringCache.end(); ++iter)
        (*iter)->addref();

    // We specifically do *NOT* copy the constructor

    return cache;
}

void QDeclarativePropertyCache::append(QDeclarativeEngine *engine, const QMetaObject *metaObject, 
                                       Data::Flag propertyFlags, Data::Flag methodFlags, Data::Flag signalFlags)
{
    append(engine, metaObject, -1, propertyFlags, methodFlags, signalFlags);
}

void QDeclarativePropertyCache::append(QDeclarativeEngine *engine, const QMetaObject *metaObject, 
                                       int revision, 
                                       Data::Flag propertyFlags, Data::Flag methodFlags, Data::Flag signalFlags)
{
    Q_UNUSED(revision);

    qPersistentDispose(constructor); // Now invalid

    bool dynamicMetaObject = isDynamicMetaObject(metaObject);

    allowedRevisionCache.append(0);

    QDeclarativeEnginePrivate *enginePriv = QDeclarativeEnginePrivate::get(engine);
    int methodCount = metaObject->methodCount();
    // 3 to block the destroyed signal and the deleteLater() slot
    int methodOffset = qMax(3, metaObject->methodOffset()); 

    methodIndexCache.resize(methodCount);
    for (int ii = methodOffset; ii < methodCount; ++ii) {
        QMetaMethod m = metaObject->method(ii);
        if (m.access() == QMetaMethod::Private) 
            continue;
        QString methodName = QString::fromUtf8(m.signature());

        int parenIdx = methodName.indexOf(QLatin1Char('('));
        Q_ASSERT(parenIdx != -1);
        methodName = methodName.left(parenIdx);

        RData *data = new RData;
        methodIndexCache[ii] = data;  

        data->load(m);
        if (m.methodType() == QMetaMethod::Slot || m.methodType() == QMetaMethod::Method) 
            data->flags |= methodFlags;
        else if (m.methodType() == QMetaMethod::Signal)
            data->flags |= signalFlags;

        if (!dynamicMetaObject)
            data->flags |= Data::IsDirect;

        data->metaObjectOffset = allowedRevisionCache.count() - 1;

        if (stringCache.contains(methodName)) {
            RData *old = stringCache[methodName];
            // We only overload methods in the same class, exactly like C++
            if (old->flags & Data::IsFunction && old->coreIndex >= methodOffset)
                data->relatedIndex = old->coreIndex;
            data->overrideIndexIsProperty = !bool(old->flags & Data::IsFunction);
            data->overrideIndex = old->coreIndex;
            stringCache[methodName]->release();
        }

        stringCache.insert(methodName, data);
        data->addref();
    }

    int propCount = metaObject->propertyCount();
    int propOffset = metaObject->propertyOffset();

    indexCache.resize(propCount);
    for (int ii = propOffset; ii < propCount; ++ii) {
        QMetaProperty p = metaObject->property(ii);
        if (!p.isScriptable())
            continue;

        QString propName = QString::fromUtf8(p.name());

        RData *data = new RData;
        indexCache[ii] = data;

        data->load(p, engine);
        data->flags |= propertyFlags;

        if (!dynamicMetaObject) 
            data->flags |= Data::IsDirect;

        data->metaObjectOffset = allowedRevisionCache.count() - 1;

        if (stringCache.contains(propName)) {
            RData *old = stringCache[propName];
            data->overrideIndexIsProperty = !bool(old->flags & Data::IsFunction);
            data->overrideIndex = old->coreIndex;
            stringCache[propName]->release();
        }

        stringCache.insert(propName, data);
        data->addref();
    }
}

void QDeclarativePropertyCache::updateRecur(QDeclarativeEngine *engine, const QMetaObject *metaObject)
{
    if (!metaObject)
        return;

    updateRecur(engine, metaObject->superClass());

    append(engine, metaObject);
}

void QDeclarativePropertyCache::update(QDeclarativeEngine *engine, const QMetaObject *metaObject)
{
    Q_ASSERT(engine);
    Q_ASSERT(metaObject);

    clear();

    // Optimization to prevent unnecessary reallocation of lists
    indexCache.reserve(metaObject->propertyCount());
    methodIndexCache.reserve(metaObject->methodCount());

    updateRecur(engine,metaObject);
}

QDeclarativePropertyCache::Data *
QDeclarativePropertyCache::property(int index) const
{
    if (index < 0 || index >= indexCache.count())
        return 0;

    return indexCache.at(index);
}

QDeclarativePropertyCache::Data *
QDeclarativePropertyCache::method(int index) const
{
    if (index < 0 || index >= methodIndexCache.count())
        return 0;

    return methodIndexCache.at(index);
}

QDeclarativePropertyCache::Data *
QDeclarativePropertyCache::property(const QString &str) const
{
    QDeclarativePropertyCache::RData **rv = stringCache.value(str);
    return rv?*rv:0;
}

QString QDeclarativePropertyCache::Data::name(QObject *object)
{
    if (!object)
        return QString();

    return name(object->metaObject());
}

QString QDeclarativePropertyCache::Data::name(const QMetaObject *metaObject)
{
    if (!metaObject || coreIndex == -1)
        return QString();

    if (flags & IsFunction) {
        QMetaMethod m = metaObject->method(coreIndex);

        QString name = QString::fromUtf8(m.signature());
        int parenIdx = name.indexOf(QLatin1Char('('));
        if (parenIdx != -1)
            name = name.left(parenIdx);
        return name;
    } else {
        QMetaProperty p = metaObject->property(coreIndex);
        return QString::fromUtf8(p.name());
    }
}

QStringList QDeclarativePropertyCache::propertyNames() const
{
    QStringList keys;
    for (StringCache::ConstIterator iter = stringCache.begin(); iter != stringCache.end(); ++iter) 
        keys.append(iter.key());
    return keys;
}

QDeclarativePropertyCache::Data *
QDeclarativePropertyCache::property(QDeclarativeEngine *engine, QObject *obj, 
                                    const QHashedV8String &name, Data &local)
{
    // XXX Optimize for worker script case where engine isn't available
    QDeclarativePropertyCache *cache = 0;
    if (engine) {
        QDeclarativeEnginePrivate *ep = QDeclarativeEnginePrivate::get(engine);

        QDeclarativeData *ddata = QDeclarativeData::get(obj);
        if (ddata && ddata->propertyCache && ddata->propertyCache->qmlEngine() == engine) // XXX aakenend
            cache = ddata->propertyCache;
        if (!cache) {
            cache = ep->cache(obj);
            if (cache && ddata && !ddata->propertyCache) { cache->addref(); ddata->propertyCache = cache; }
        }
    }

    QDeclarativePropertyCache::Data *rv = 0;

    if (cache) {
        rv = cache->property(name);
    } else {
        QString strname = QV8Engine::toStringStatic(name.string());
        // QString strname = ep->v8engine.toString(name);
        local = QDeclarativePropertyCache::create(obj->metaObject(), strname);
        if (local.isValid())
            rv = &local;
    }

    return rv;
}

QDeclarativePropertyCache::Data *
QDeclarativePropertyCache::property(QDeclarativeEngine *engine, QObject *obj, 
                                    const QString &name, Data &local)
{
    QDeclarativePropertyCache::Data *rv = 0;

    if (!engine) {
        local = QDeclarativePropertyCache::create(obj->metaObject(), name);
        if (local.isValid())
            rv = &local;
    } else {
        QDeclarativeEnginePrivate *enginePrivate = QDeclarativeEnginePrivate::get(engine);

        QDeclarativePropertyCache *cache = 0;
        QDeclarativeData *ddata = QDeclarativeData::get(obj);
        if (ddata && ddata->propertyCache && ddata->propertyCache->qmlEngine() == engine)
            cache = ddata->propertyCache;
        if (!cache) {
            cache = enginePrivate->cache(obj);
            if (cache && ddata && !ddata->propertyCache) { cache->addref(); ddata->propertyCache = cache; }
        }

        if (cache) {
            rv = cache->property(name);
        } else {
            local = QDeclarativePropertyCache::create(obj->metaObject(), name);
            if (local.isValid())
                rv = &local;
        }
    }

    return rv;
}

static inline const QMetaObjectPrivate *priv(const uint* data)
{ return reinterpret_cast<const QMetaObjectPrivate*>(data); }

bool QDeclarativePropertyCache::isDynamicMetaObject(const QMetaObject *mo)
{
    return priv(mo->d.data)->revision >= 3 && priv(mo->d.data)->flags & DynamicMetaObject;
}

QT_END_NAMESPACE
