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

Q_DECLARE_METATYPE(QJSValue)
Q_DECLARE_METATYPE(QDeclarativeV8Handle);

QT_BEGIN_NAMESPACE

// Flags that do *NOT* depend on the property's QMetaProperty::userType() and thus are quick
// to load
static QDeclarativePropertyCache::Data::Flags fastFlagsForProperty(const QMetaProperty &p)
{
    QDeclarativePropertyCache::Data::Flags flags;

    if (p.isConstant())
        flags |= QDeclarativePropertyCache::Data::IsConstant;
    if (p.isWritable())
        flags |= QDeclarativePropertyCache::Data::IsWritable;
    if (p.isResettable())
        flags |= QDeclarativePropertyCache::Data::IsResettable;
    if (p.isFinal())
        flags |= QDeclarativePropertyCache::Data::IsFinal;
    if (p.isEnumType())
        flags |= QDeclarativePropertyCache::Data::IsEnumType;

    return flags;
}

// Flags that do depend on the property's QMetaProperty::userType() and thus are slow to 
// load
static QDeclarativePropertyCache::Data::Flags flagsForPropertyType(int propType, QDeclarativeEngine *engine)
{
    QDeclarativePropertyCache::Data::Flags flags;

    if (propType < QMetaType::User && propType != QMetaType::QObjectStar && propType != QMetaType::QWidgetStar) {
    } else if (propType == qMetaTypeId<QDeclarativeBinding *>()) {
        flags |= QDeclarativePropertyCache::Data::IsQmlBinding;
    } else if (propType == qMetaTypeId<QJSValue>()) {
        flags |= QDeclarativePropertyCache::Data::IsQJSValue;
    } else if (propType == qMetaTypeId<QDeclarativeV8Handle>()) {
        flags |= QDeclarativePropertyCache::Data::IsV8Handle;
    } else {
        QDeclarativeMetaType::TypeCategory cat = 
            engine ? QDeclarativeEnginePrivate::get(engine)->typeCategory(propType)
                   : QDeclarativeMetaType::typeCategory(propType);

        if (cat == QDeclarativeMetaType::Object)
            flags |= QDeclarativePropertyCache::Data::IsQObjectDerived;
        else if (cat == QDeclarativeMetaType::List)
            flags |= QDeclarativePropertyCache::Data::IsQList;
    }

    return flags;
}

QDeclarativePropertyCache::Data::Flags 
QDeclarativePropertyCache::Data::flagsForProperty(const QMetaProperty &p, QDeclarativeEngine *engine) 
{
    return fastFlagsForProperty(p) | flagsForPropertyType(p.userType(), engine);
}

void QDeclarativePropertyCache::Data::lazyLoad(const QMetaProperty &p, QDeclarativeEngine *engine)
{
    Q_UNUSED(engine);

    coreIndex = p.propertyIndex();
    notifyIndex = p.notifySignalIndex();
    revision = p.revision();

    flags = fastFlagsForProperty(p);

    int type = p.type();
    if (type == QMetaType::QObjectStar || type == QMetaType::QWidgetStar) {
        propType = type;
        flags |= QDeclarativePropertyCache::Data::IsQObjectDerived;
    } else if (type == QVariant::UserType || type == -1) {
        propTypeName = p.typeName();
        flags |= QDeclarativePropertyCache::Data::NotFullyResolved;
    } else {
        propType = type;
    }
}

void QDeclarativePropertyCache::Data::load(const QMetaProperty &p, QDeclarativeEngine *engine)
{
    propType = p.userType();
    if (QVariant::Type(propType) == QVariant::LastType)
        propType = qMetaTypeId<QVariant>();
    coreIndex = p.propertyIndex();
    notifyIndex = p.notifySignalIndex();
    flags = fastFlagsForProperty(p) | flagsForPropertyType(propType, engine);
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

    const char *signature = m.signature();
    while (*signature != '(') { Q_ASSERT(*signature != 0); ++signature; }

    ++signature;
    if (*signature != ')') {
        flags |= Data::HasArguments;
        if (0 == ::strcmp(signature, "QDeclarativeV8Function*)")) {
            flags |= Data::IsV8Function;
        }
    }

    revision = m.revision();
}

void QDeclarativePropertyCache::Data::lazyLoad(const QMetaMethod &m)
{
    coreIndex = m.methodIndex();
    relatedIndex = -1;
    flags |= Data::IsFunction;
    if (m.methodType() == QMetaMethod::Signal)
        flags |= Data::IsSignal;
    propType = QVariant::Invalid;

    const char *returnType = m.typeName();
    if (returnType && *returnType) {
        propTypeName = returnType;
        flags |= Data::NotFullyResolved;
    }

    const char *signature = m.signature();
    while (*signature != '(') { Q_ASSERT(*signature != 0); ++signature; }

    ++signature;
    if (*signature != ')') {
        flags |= Data::HasArguments;
        if (0 == ::strcmp(signature, "QDeclarativeV8Function*)")) {
            flags |= Data::IsV8Function;
        }
    }

    revision = m.revision();
}

/*!
Creates a new empty QDeclarativePropertyCache.
*/
QDeclarativePropertyCache::QDeclarativePropertyCache(QDeclarativeEngine *e)
: QDeclarativeCleanup(e), engine(e), parent(0), propertyIndexCacheStart(0), methodIndexCacheStart(0)
{
    Q_ASSERT(engine);
}

/*!
Creates a new QDeclarativePropertyCache of \a metaObject.
*/
QDeclarativePropertyCache::QDeclarativePropertyCache(QDeclarativeEngine *e, const QMetaObject *metaObject)
: QDeclarativeCleanup(e), engine(e), parent(0), propertyIndexCacheStart(0), methodIndexCacheStart(0)
{
    Q_ASSERT(engine);
    Q_ASSERT(metaObject);

    update(engine, metaObject);
}

QDeclarativePropertyCache::~QDeclarativePropertyCache()
{
    clear();

    if (parent) parent->release();
    parent = 0;
}

// This is inherited from QDeclarativeCleanup, so it should only clear the things
// that are tied to the specific QDeclarativeEngine.
void QDeclarativePropertyCache::clear()
{
    qPersistentDispose(constructor);
    engine = 0;
}

QDeclarativePropertyCache::Data QDeclarativePropertyCache::create(const QMetaObject *metaObject, 
                                                                  const QString &property)
{
    Q_ASSERT(metaObject);

    QDeclarativePropertyCache::Data rv;
    {
        const QMetaObject *cmo = metaObject;
        const QByteArray propertyName = property.toUtf8();
        while (cmo) {
            int idx = cmo->indexOfProperty(propertyName);
            if (idx != -1) {
                QMetaProperty p = cmo->property(idx);
                if (p.isScriptable()) {
                    rv.load(p);
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

QDeclarativePropertyCache *QDeclarativePropertyCache::copy(int reserve) 
{
    QDeclarativePropertyCache *cache = new QDeclarativePropertyCache(engine);
    cache->parent = this;
    cache->parent->addref();
    cache->propertyIndexCacheStart = propertyIndexCache.count() + propertyIndexCacheStart;
    cache->methodIndexCacheStart = methodIndexCache.count() + methodIndexCacheStart;
    cache->stringCache.copyAndReserve(stringCache, reserve);
    cache->allowedRevisionCache = allowedRevisionCache;

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

    int methodCount = metaObject->methodCount();
    Q_ASSERT(QMetaObjectPrivate::get(metaObject)->revision >= 4);
    int signalCount = QMetaObjectPrivate::get(metaObject)->signalCount;
    // 3 to block the destroyed signal and the deleteLater() slot
    int methodOffset = qMax(3, metaObject->methodOffset()); 

    methodIndexCache.resize(methodCount - methodIndexCacheStart);
    signalHandlerIndexCache.resize(signalCount);
    int signalHandlerIndex = 0;
    for (int ii = methodOffset; ii < methodCount; ++ii) {
        QMetaMethod m = metaObject->method(ii);
        if (m.access() == QMetaMethod::Private) 
            continue;

        // Extract method name
        const char *signature = m.signature();
        const char *cptr = signature;
        bool utf8 = false;
        while (*cptr != '(') { Q_ASSERT(*cptr != 0); utf8 |= *cptr & 0x80; ++cptr; }

        Data *data = &methodIndexCache[ii - methodIndexCacheStart];
        Data *sigdata = 0;

        data->lazyLoad(m);

        if (data->isSignal())
            data->flags |= signalFlags;
        else
            data->flags |= methodFlags;

        if (!dynamicMetaObject)
            data->flags |= Data::IsDirect;

        data->metaObjectOffset = allowedRevisionCache.count() - 1;

        if (data->isSignal()) {
            sigdata = &signalHandlerIndexCache[signalHandlerIndex];
            *sigdata = *data;
            sigdata->flags |= Data::IsSignalHandler;
        }

        Data *old = 0;

        if (utf8) {
            QHashedString methodName(QString::fromUtf8(signature, cptr - signature));
            if (Data **it = stringCache.value(methodName))
                old = *it;
            stringCache.insert(methodName, data);

            if (data->isSignal()) {
                QHashedString on(QStringLiteral("on") % methodName.at(0).toUpper() % methodName.midRef(1));
                stringCache.insert(on, sigdata);
                ++signalHandlerIndex;
            }
        } else {
            QHashedCStringRef methodName(signature, cptr - signature);
            if (Data **it = stringCache.value(methodName))
                old = *it;
            stringCache.insert(methodName, data);

            if (data->isSignal()) {
                int length = methodName.length();

                char str[length + 3];
                str[0] = 'o';
                str[1] = 'n';
                str[2] = toupper(signature[0]);
                if (length > 1)
                    memcpy(&str[3], &signature[1], length - 1);
                str[length + 2] = '\0';

                QHashedString on(QString::fromLatin1(str));
                stringCache.insert(on, sigdata);
                ++signalHandlerIndex;
            }
        }

        if (old) {
            // We only overload methods in the same class, exactly like C++
            if (old->flags & Data::IsFunction && old->coreIndex >= methodOffset)
                data->relatedIndex = old->coreIndex;
            data->overrideIndexIsProperty = !bool(old->flags & Data::IsFunction);
            data->overrideIndex = old->coreIndex;
        }
    }

    int propCount = metaObject->propertyCount();
    int propOffset = metaObject->propertyOffset();

    propertyIndexCache.resize(propCount - propertyIndexCacheStart);
    for (int ii = propOffset; ii < propCount; ++ii) {
        QMetaProperty p = metaObject->property(ii);
        if (!p.isScriptable())
            continue;

        const char *str = p.name();
        bool utf8 = false;
        const char *cptr = str;
        while (*cptr != 0) { utf8 |= *cptr & 0x80; ++cptr; }

        Data *data = &propertyIndexCache[ii - propertyIndexCacheStart];

        data->lazyLoad(p, engine);
        data->flags |= propertyFlags;

        if (!dynamicMetaObject) 
            data->flags |= Data::IsDirect;

        data->metaObjectOffset = allowedRevisionCache.count() - 1;

        Data *old = 0;

        if (utf8) {
            QHashedString propName(QString::fromUtf8(str, cptr - str));
            if (Data **it = stringCache.value(propName))
                old = *it;
            stringCache.insert(propName, data);
        } else {
            QHashedCStringRef propName(str, cptr - str);
            if (Data **it = stringCache.value(propName))
                old = *it;
            stringCache.insert(propName, data);
        }

        if (old) {
            data->overrideIndexIsProperty = !bool(old->flags & Data::IsFunction);
            data->overrideIndex = old->coreIndex;
        }
    }
}

void QDeclarativePropertyCache::resolve(Data *data) const
{
    Q_ASSERT(data->notFullyResolved());

    data->propType = QMetaType::type(data->propTypeName);
    if (QVariant::Type(data->propType) == QVariant::LastType)
        data->propType = qMetaTypeId<QVariant>();


    if (!(data->flags & Data::IsFunction))
        data->flags |= flagsForPropertyType(data->propType, engine);

    data->flags &= ~Data::NotFullyResolved;
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
    Q_ASSERT(stringCache.isEmpty());

    // Optimization to prevent unnecessary reallocation of lists
    int pc = metaObject->propertyCount();
    int mc = metaObject->methodCount();
    propertyIndexCache.reserve(pc);
    methodIndexCache.reserve(mc);

    stringCache.reserve(pc + mc);

    updateRecur(engine,metaObject);
}

QDeclarativePropertyCache::Data *
QDeclarativePropertyCache::property(int index) const
{
    if (index < 0 || index >= (propertyIndexCacheStart + propertyIndexCache.count()))
        return 0;
    
    if (index < propertyIndexCacheStart)
        return parent->property(index);

    Data *rv = const_cast<Data *>(&propertyIndexCache.at(index - propertyIndexCacheStart));
    if (rv->notFullyResolved()) resolve(rv);
    return rv;
}

QDeclarativePropertyCache::Data *
QDeclarativePropertyCache::method(int index) const
{
    if (index < 0 || index >= (methodIndexCacheStart + methodIndexCache.count()))
        return 0;

    if (index < methodIndexCacheStart)
        return parent->method(index);

    Data *rv = const_cast<Data *>(&methodIndexCache.at(index - methodIndexCacheStart));
    if (rv->notFullyResolved()) resolve(rv);
    return rv;
}

QDeclarativePropertyCache::Data *
QDeclarativePropertyCache::property(const QHashedStringRef &str) const
{
    QDeclarativePropertyCache::Data **rv = stringCache.value(str);
    if (rv && (*rv)->notFullyResolved()) resolve(*rv);
    return rv?*rv:0;
}

QDeclarativePropertyCache::Data *
QDeclarativePropertyCache::property(const QHashedCStringRef &str) const
{
    QDeclarativePropertyCache::Data **rv = stringCache.value(str);
    if (rv && (*rv)->notFullyResolved()) resolve(*rv);
    return rv?*rv:0;
}

QDeclarativePropertyCache::Data *
QDeclarativePropertyCache::property(const QString &str) const
{
    QDeclarativePropertyCache::Data **rv = stringCache.value(str);
    if (rv && (*rv)->notFullyResolved()) resolve(*rv);
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
        if (ddata && ddata->propertyCache)
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
        // QString strname = ep->v8engine()->toString(name);
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
        if (ddata && ddata->propertyCache)
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
