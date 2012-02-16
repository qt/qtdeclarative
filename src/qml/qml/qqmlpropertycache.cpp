/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the QtQml module of the Qt Toolkit.
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
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qqmlpropertycache_p.h"

#include "qqmlengine_p.h"
#include "qqmlbinding_p.h"
#include <private/qv8engine_p.h>

#include <private/qmetaobject_p.h>
#include <private/qqmlaccessors_p.h>

#include <QtCore/qdebug.h>

Q_DECLARE_METATYPE(QJSValue)
Q_DECLARE_METATYPE(QQmlV8Handle);

QT_BEGIN_NAMESPACE

#define Q_INT16_MAX 32767

class QQmlPropertyCacheMethodArguments 
{
public:
    QQmlPropertyCacheMethodArguments *next;
    int arguments[0];
};

// Flags that do *NOT* depend on the property's QMetaProperty::userType() and thus are quick
// to load
static QQmlPropertyData::Flags fastFlagsForProperty(const QMetaProperty &p)
{
    QQmlPropertyData::Flags flags;

    if (p.isConstant())
        flags |= QQmlPropertyData::IsConstant;
    if (p.isWritable())
        flags |= QQmlPropertyData::IsWritable;
    if (p.isResettable())
        flags |= QQmlPropertyData::IsResettable;
    if (p.isFinal())
        flags |= QQmlPropertyData::IsFinal;
    if (p.isEnumType())
        flags |= QQmlPropertyData::IsEnumType;

    return flags;
}

// Flags that do depend on the property's QMetaProperty::userType() and thus are slow to 
// load
static QQmlPropertyData::Flags flagsForPropertyType(int propType, QQmlEngine *engine)
{
    Q_ASSERT(propType != -1);

    QQmlPropertyData::Flags flags;

    if (propType == QMetaType::QObjectStar || propType == QMetaType::QWidgetStar) {
        flags |= QQmlPropertyData::IsQObjectDerived;
    } else if (propType == QMetaType::QVariant) {
        flags |= QQmlPropertyData::IsQVariant;
    } else if (propType < (int)QVariant::UserType) {
    } else if (propType == qMetaTypeId<QQmlBinding *>()) {
        flags |= QQmlPropertyData::IsQmlBinding;
    } else if (propType == qMetaTypeId<QJSValue>()) {
        flags |= QQmlPropertyData::IsQJSValue;
    } else if (propType == qMetaTypeId<QQmlV8Handle>()) {
        flags |= QQmlPropertyData::IsV8Handle;
    } else {
        QQmlMetaType::TypeCategory cat = 
            engine ? QQmlEnginePrivate::get(engine)->typeCategory(propType)
                   : QQmlMetaType::typeCategory(propType);

        if (cat == QQmlMetaType::Object)
            flags |= QQmlPropertyData::IsQObjectDerived;
        else if (cat == QQmlMetaType::List)
            flags |= QQmlPropertyData::IsQList;
    }

    return flags;
}

static int metaObjectSignalCount(const QMetaObject *metaObject)
{
    int signalCount = 0;
    for (const QMetaObject *obj = metaObject; obj; obj = obj->superClass())
        signalCount += QMetaObjectPrivate::get(obj)->signalCount;
    return signalCount;
}

QQmlPropertyData::Flags
QQmlPropertyData::flagsForProperty(const QMetaProperty &p, QQmlEngine *engine)
{
    return fastFlagsForProperty(p) | flagsForPropertyType(p.userType(), engine);
}

void QQmlPropertyData::lazyLoad(const QMetaProperty &p, QQmlEngine *engine)
{
    Q_UNUSED(engine);

    coreIndex = p.propertyIndex();
    notifyIndex = p.notifySignalIndex();
    Q_ASSERT(p.revision() <= Q_INT16_MAX);
    revision = p.revision();

    flags = fastFlagsForProperty(p);

    int type = p.type();
    if (type == QMetaType::QObjectStar || type == QMetaType::QWidgetStar) {
        propType = type;
        flags |= QQmlPropertyData::IsQObjectDerived;
    } else if (type == QMetaType::QVariant) {
        propType = type;
        flags |= QQmlPropertyData::IsQVariant;
    } else if (type == QVariant::UserType || type == -1) {
        propTypeName = p.typeName();
        flags |= QQmlPropertyData::NotFullyResolved;
    } else {
        propType = type;
    }
}

void QQmlPropertyData::load(const QMetaProperty &p, QQmlEngine *engine)
{
    propType = p.userType();
    coreIndex = p.propertyIndex();
    notifyIndex = p.notifySignalIndex();
    flags = fastFlagsForProperty(p) | flagsForPropertyType(propType, engine);
    Q_ASSERT(p.revision() <= Q_INT16_MAX);
    revision = p.revision();
}

void QQmlPropertyData::load(const QMetaMethod &m)
{
    coreIndex = m.methodIndex();
    arguments = 0;
    flags |= IsFunction;
    if (m.methodType() == QMetaMethod::Signal)
        flags |= IsSignal;
    propType = QVariant::Invalid;

    const char *returnType = m.typeName();
    if (returnType) 
        propType = QMetaType::type(returnType);

    const char *signature = m.signature();
    while (*signature != '(') { Q_ASSERT(*signature != 0); ++signature; }

    ++signature;
    if (*signature != ')') {
        flags |= HasArguments;
        if (0 == ::strcmp(signature, "QQmlV8Function*)")) {
            flags |= IsV8Function;
        }
    }

    Q_ASSERT(m.revision() <= Q_INT16_MAX);
    revision = m.revision();
}

void QQmlPropertyData::lazyLoad(const QMetaMethod &m)
{
    coreIndex = m.methodIndex();
    arguments = 0;
    flags |= IsFunction;
    if (m.methodType() == QMetaMethod::Signal)
        flags |= IsSignal;
    propType = QVariant::Invalid;

    const char *returnType = m.typeName();
    if (returnType && *returnType) {
        propTypeName = returnType;
        flags |= NotFullyResolved;
    }

    const char *signature = m.signature();
    while (*signature != '(') { Q_ASSERT(*signature != 0); ++signature; }

    ++signature;
    if (*signature != ')') {
        flags |= HasArguments;
        if (0 == ::strcmp(signature, "QQmlV8Function*)")) {
            flags |= IsV8Function;
        }
    }

    Q_ASSERT(m.revision() <= Q_INT16_MAX);
    revision = m.revision();
}

/*!
Creates a new empty QQmlPropertyCache.
*/
QQmlPropertyCache::QQmlPropertyCache(QQmlEngine *e)
: engine(e), parent(0), propertyIndexCacheStart(0), methodIndexCacheStart(0),
  signalHanderIndexCacheStart(0), metaObject(0), argumentsCache(0)
{
    Q_ASSERT(engine);
}

/*!
Creates a new QQmlPropertyCache of \a metaObject.
*/
QQmlPropertyCache::QQmlPropertyCache(QQmlEngine *e, const QMetaObject *metaObject)
: engine(e), parent(0), propertyIndexCacheStart(0), methodIndexCacheStart(0),
  signalHanderIndexCacheStart(0), metaObject(0), argumentsCache(0)
{
    Q_ASSERT(engine);
    Q_ASSERT(metaObject);

    update(engine, metaObject);
}

QQmlPropertyCache::~QQmlPropertyCache()
{
    clear();

    QQmlPropertyCacheMethodArguments *args = argumentsCache;
    while (args) {
        QQmlPropertyCacheMethodArguments *next = args->next;
        free(args);
        args = next;
    }

    // We must clear this prior to releasing the parent incase it is a
    // linked hash
    stringCache.clear();
    if (parent) parent->release();
    parent = 0;
    engine = 0;
}

void QQmlPropertyCache::destroy()
{
    Q_ASSERT(engine || constructor.IsEmpty());
    if (constructor.IsEmpty())
        delete this;
    else
        QQmlEnginePrivate::deleteInEngineThread(engine, this);
}

// This is inherited from QQmlCleanup, so it should only clear the things
// that are tied to the specific QQmlEngine.
void QQmlPropertyCache::clear()
{
    qPersistentDispose(constructor);
    engine = 0;
}

QQmlPropertyCache *QQmlPropertyCache::copy(int reserve)
{
    QQmlPropertyCache *cache = new QQmlPropertyCache(engine);
    cache->parent = this;
    cache->parent->addref();
    cache->propertyIndexCacheStart = propertyIndexCache.count() + propertyIndexCacheStart;
    cache->methodIndexCacheStart = methodIndexCache.count() + methodIndexCacheStart;
    cache->signalHanderIndexCacheStart = signalHandlerIndexCache.count() + signalHanderIndexCacheStart;
    cache->stringCache.linkAndReserve(stringCache, reserve);
    cache->allowedRevisionCache = allowedRevisionCache;
    cache->metaObject = metaObject;

    // We specifically do *NOT* copy the constructor

    return cache;
}

QQmlPropertyCache *QQmlPropertyCache::copy()
{
    return copy(0);
}

QQmlPropertyCache *
QQmlPropertyCache::copyAndAppend(QQmlEngine *engine, const QMetaObject *metaObject,
                                         QQmlPropertyData::Flag propertyFlags,
                                         QQmlPropertyData::Flag methodFlags,
                                         QQmlPropertyData::Flag signalFlags)
{
    return copyAndAppend(engine, metaObject, -1, propertyFlags, methodFlags, signalFlags);
}

QQmlPropertyCache *
QQmlPropertyCache::copyAndAppend(QQmlEngine *engine, const QMetaObject *metaObject,
                                         int revision,
                                         QQmlPropertyData::Flag propertyFlags,
                                         QQmlPropertyData::Flag methodFlags,
                                         QQmlPropertyData::Flag signalFlags)
{
    Q_ASSERT(QMetaObjectPrivate::get(metaObject)->revision >= 4);

    // Reserve enough space in the name hash for all the methods (including signals), all the
    // signal handlers and all the properties.  This assumes no name clashes, but this is the
    // common case.
    QQmlPropertyCache *rv = copy(QMetaObjectPrivate::get(metaObject)->methodCount +
                                         QMetaObjectPrivate::get(metaObject)->signalCount +
                                         QMetaObjectPrivate::get(metaObject)->propertyCount);

    rv->append(engine, metaObject, revision, propertyFlags, methodFlags, signalFlags);

    return rv;
}

void QQmlPropertyCache::append(QQmlEngine *engine, const QMetaObject *metaObject, 
                                       QQmlPropertyData::Flag propertyFlags,
                                       QQmlPropertyData::Flag methodFlags,
                                       QQmlPropertyData::Flag signalFlags)
{
    append(engine, metaObject, -1, propertyFlags, methodFlags, signalFlags);
}

void QQmlPropertyCache::append(QQmlEngine *engine, const QMetaObject *metaObject, 
                                       int revision, 
                                       QQmlPropertyData::Flag propertyFlags,
                                       QQmlPropertyData::Flag methodFlags,
                                       QQmlPropertyData::Flag signalFlags)
{
    Q_UNUSED(revision);
    Q_ASSERT(constructor.IsEmpty()); // We should not be appending to an in-use property cache

    this->metaObject = metaObject;

    bool dynamicMetaObject = isDynamicMetaObject(metaObject);

    allowedRevisionCache.append(0);

    int methodCount = metaObject->methodCount();
    Q_ASSERT(QMetaObjectPrivate::get(metaObject)->revision >= 4);
    int signalCount = metaObjectSignalCount(metaObject);
    int classInfoCount = QMetaObjectPrivate::get(metaObject)->classInfoCount;

    QQmlAccessorProperties::Properties accessorProperties;

    // Special case QObject as we don't want to add a qt_HasQmlAccessors classinfo to it
    if (metaObject == &QObject::staticMetaObject) {
        accessorProperties = QQmlAccessorProperties::properties(metaObject);
    } else if (classInfoCount) {
        int classInfoOffset = metaObject->classInfoOffset();
        bool hasFastProperty = false;
        for (int ii = 0; ii < classInfoCount; ++ii) {
            int idx = ii + classInfoOffset;

            if (0 == qstrcmp(metaObject->classInfo(idx).name(), "qt_HasQmlAccessors")) {
                hasFastProperty = true;
                break;
            }
        }

        if (hasFastProperty) {
            accessorProperties = QQmlAccessorProperties::properties(metaObject);
            if (accessorProperties.count == 0)
                qFatal("QQmlPropertyCache: %s has FastProperty class info, but has not "
                       "installed property accessors", metaObject->className());
        } else {
#ifndef QT_NO_DEBUG
            accessorProperties = QQmlAccessorProperties::properties(metaObject);
            if (accessorProperties.count != 0)
                qFatal("QQmlPropertyCache: %s has fast property accessors, but is missing "
                       "FastProperty class info", metaObject->className());
#endif
        }
    }

    // qMax(defaultMethods, methodOffset) to block the signals and slots of QObject::staticMetaObject
    // incl. destroyed signals, objectNameChanged signal, deleteLater slot, _q_reregisterTimers slot.
    int methodOffset = qMax(QObject::staticMetaObject.methodCount(), metaObject->methodOffset());
    int signalOffset = signalCount - QMetaObjectPrivate::get(metaObject)->signalCount;

    // update() should have reserved enough space in the vector that this doesn't cause a realloc
    // and invalidate the stringCache.
    methodIndexCache.resize(methodCount - methodIndexCacheStart);
    signalHandlerIndexCache.resize(signalCount - signalHanderIndexCacheStart);
    int signalHandlerIndex = signalOffset;
    for (int ii = methodOffset; ii < methodCount; ++ii) {
        QMetaMethod m = metaObject->method(ii);
        if (m.access() == QMetaMethod::Private) 
            continue;

        // Extract method name
        const char *signature = m.signature();
        const char *cptr = signature;
        char utf8 = 0;
        while (*cptr != '(') {
            Q_ASSERT(*cptr != 0);
            utf8 |= *cptr & 0x80;
            ++cptr;
        }

        QQmlPropertyData *data = &methodIndexCache[ii - methodIndexCacheStart];
        QQmlPropertyData *sigdata = 0;

        data->lazyLoad(m);

        if (data->isSignal())
            data->flags |= signalFlags;
        else
            data->flags |= methodFlags;

        if (!dynamicMetaObject)
            data->flags |= QQmlPropertyData::IsDirect;

        Q_ASSERT((allowedRevisionCache.count() - 1) < Q_INT16_MAX);
        data->metaObjectOffset = allowedRevisionCache.count() - 1;

        if (data->isSignal()) {
            sigdata = &signalHandlerIndexCache[signalHandlerIndex - signalHanderIndexCacheStart];
            *sigdata = *data;
            sigdata->flags |= QQmlPropertyData::IsSignalHandler;
        }

        QQmlPropertyData *old = 0;

        if (utf8) {
            QHashedString methodName(QString::fromUtf8(signature, cptr - signature));
            if (QQmlPropertyData **it = stringCache.value(methodName))
                old = *it;
            stringCache.insert(methodName, data);

            if (data->isSignal()) {
                QHashedString on(QStringLiteral("on") % methodName.at(0).toUpper() % methodName.midRef(1));
                stringCache.insert(on, sigdata);
                ++signalHandlerIndex;
            }
        } else {
            QHashedCStringRef methodName(signature, cptr - signature);
            if (QQmlPropertyData **it = stringCache.value(methodName))
                old = *it;
            stringCache.insert(methodName, data);

            if (data->isSignal()) {
                int length = methodName.length();

                QVarLengthArray<char, 128> str(length+3);
                str[0] = 'o';
                str[1] = 'n';
                str[2] = toupper(signature[0]);
                if (length > 1)
                    memcpy(&str[3], &signature[1], length - 1);
                str[length + 2] = '\0';

                QHashedString on(QString::fromLatin1(str.data()));
                stringCache.insert(on, sigdata);
                ++signalHandlerIndex;
            }
        }

        if (old) {
            // We only overload methods in the same class, exactly like C++
            if (old->isFunction() && old->coreIndex >= methodOffset)
                data->flags |= QQmlPropertyData::IsOverload;
            data->overrideIndexIsProperty = !old->isFunction();
            data->overrideIndex = old->coreIndex;
        }
    }

    int propCount = metaObject->propertyCount();
    int propOffset = metaObject->propertyOffset();

    // update() should have reserved enough space in the vector that this doesn't cause a realloc
    // and invalidate the stringCache.
    propertyIndexCache.resize(propCount - propertyIndexCacheStart);
    for (int ii = propOffset; ii < propCount; ++ii) {
        QMetaProperty p = metaObject->property(ii);
        if (!p.isScriptable())
            continue;

        const char *str = p.name();
        char utf8 = 0;
        const char *cptr = str;
        while (*cptr != 0) {
            utf8 |= *cptr & 0x80;
            ++cptr;
        }

        QQmlPropertyData *data = &propertyIndexCache[ii - propertyIndexCacheStart];

        data->lazyLoad(p, engine);
        data->flags |= propertyFlags;

        if (!dynamicMetaObject) 
            data->flags |= QQmlPropertyData::IsDirect;

        Q_ASSERT((allowedRevisionCache.count() - 1) < Q_INT16_MAX);
        data->metaObjectOffset = allowedRevisionCache.count() - 1;

        QQmlPropertyData *old = 0;

        if (utf8) {
            QHashedString propName(QString::fromUtf8(str, cptr - str));
            if (QQmlPropertyData **it = stringCache.value(propName))
                old = *it;
            stringCache.insert(propName, data);
        } else {
            QHashedCStringRef propName(str, cptr - str);
            if (QQmlPropertyData **it = stringCache.value(propName))
                old = *it;
            stringCache.insert(propName, data);
        }

        QQmlAccessorProperties::Property *accessorProperty = accessorProperties.property(str);

        // Fast properties may not be overrides or revisioned
        Q_ASSERT(accessorProperty == 0 || (old == 0 && data->revision == 0));

        if (accessorProperty) {
            data->flags |= QQmlPropertyData::HasAccessors;
            data->accessors = accessorProperty->accessors;
            data->accessorData = accessorProperty->data;
        } else if (old) {
            data->overrideIndexIsProperty = !old->isFunction();
            data->overrideIndex = old->coreIndex;
        }
    }
}

void QQmlPropertyCache::resolve(QQmlPropertyData *data) const
{
    Q_ASSERT(data->notFullyResolved());

    data->propType = QMetaType::type(data->propTypeName);

    if (!data->isFunction())
        data->flags |= flagsForPropertyType(data->propType, engine);

    data->flags &= ~QQmlPropertyData::NotFullyResolved;
}

void QQmlPropertyCache::updateRecur(QQmlEngine *engine, const QMetaObject *metaObject)
{
    if (!metaObject)
        return;

    updateRecur(engine, metaObject->superClass());

    append(engine, metaObject);
}

void QQmlPropertyCache::update(QQmlEngine *engine, const QMetaObject *metaObject)
{
    Q_ASSERT(engine);
    Q_ASSERT(metaObject);
    Q_ASSERT(stringCache.isEmpty());

    // Preallocate enough space in the index caches for all the properties/methods/signals that
    // are not cached in a parent cache so that the caches never need to be reallocated as this
    // would invalidate pointers stored in the stringCache.
    int pc = metaObject->propertyCount();
    int mc = metaObject->methodCount();
    int sc = metaObjectSignalCount(metaObject);
    propertyIndexCache.reserve(pc - propertyIndexCacheStart);
    methodIndexCache.reserve(mc - methodIndexCacheStart);
    signalHandlerIndexCache.reserve(sc - signalHanderIndexCacheStart);

    // Reserve enough space in the stringCache for all properties/methods/signals including those
    // cached in a parent cache.
    stringCache.reserve(pc + mc + sc);

    updateRecur(engine,metaObject);
}

QQmlPropertyData *
QQmlPropertyCache::property(int index) const
{
    if (index < 0 || index >= (propertyIndexCacheStart + propertyIndexCache.count()))
        return 0;
    
    if (index < propertyIndexCacheStart)
        return parent->property(index);

    QQmlPropertyData *rv = const_cast<QQmlPropertyData *>(&propertyIndexCache.at(index - propertyIndexCacheStart));
    if (rv->notFullyResolved()) resolve(rv);
    return rv;
}

QQmlPropertyData *
QQmlPropertyCache::method(int index) const
{
    if (index < 0 || index >= (methodIndexCacheStart + methodIndexCache.count()))
        return 0;

    if (index < methodIndexCacheStart)
        return parent->method(index);

    QQmlPropertyData *rv = const_cast<QQmlPropertyData *>(&methodIndexCache.at(index - methodIndexCacheStart));
    if (rv->notFullyResolved()) resolve(rv);
    return rv;
}

QQmlPropertyData *
QQmlPropertyCache::property(const QHashedStringRef &str) const
{
    QQmlPropertyData **rv = stringCache.value(str);
    if (rv && (*rv)->notFullyResolved()) resolve(*rv);
    return rv?*rv:0;
}

QQmlPropertyData *
QQmlPropertyCache::property(const QHashedCStringRef &str) const
{
    QQmlPropertyData **rv = stringCache.value(str);
    if (rv && (*rv)->notFullyResolved()) resolve(*rv);
    return rv?*rv:0;
}

QQmlPropertyData *
QQmlPropertyCache::property(const QString &str) const
{
    QQmlPropertyData **rv = stringCache.value(str);
    if (rv && (*rv)->notFullyResolved()) resolve(*rv);
    return rv?*rv:0;
}

QString QQmlPropertyData::name(QObject *object)
{
    if (!object)
        return QString();

    return name(object->metaObject());
}

QString QQmlPropertyData::name(const QMetaObject *metaObject)
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

QStringList QQmlPropertyCache::propertyNames() const
{
    QStringList keys;
    for (StringCache::ConstIterator iter = stringCache.begin(); iter != stringCache.end(); ++iter) 
        keys.append(iter.key());
    return keys;
}

static int EnumType(const QMetaObject *meta, const QByteArray &str)
{
    QByteArray scope;
    QByteArray name;
    int scopeIdx = str.lastIndexOf("::");
    if (scopeIdx != -1) {
        scope = str.left(scopeIdx);
        name = str.mid(scopeIdx + 2);
    } else { 
        name = str;
    }
    for (int i = meta->enumeratorCount() - 1; i >= 0; --i) {
        QMetaEnum m = meta->enumerator(i);
        if ((m.name() == name) && (scope.isEmpty() || (m.scope() == scope)))
            return QVariant::Int;
    }
    return QVariant::Invalid;
}

// Returns an array of the arguments for method \a index.  The first entry in the array
// is the number of arguments.
int *QQmlPropertyCache::methodParameterTypes(QObject *object, int index, 
                                                     QVarLengthArray<int, 9> &dummy,
                                                     QByteArray *unknownTypeError)
{
    Q_ASSERT(object && index >= 0);

    QQmlData *ddata = QQmlData::get(object, false);

    if (ddata && ddata->propertyCache) {
        typedef QQmlPropertyCacheMethodArguments A;

        QQmlPropertyCache *c = ddata->propertyCache;
        Q_ASSERT(index < c->methodIndexCacheStart + c->methodIndexCache.count());

        while (index < c->methodIndexCacheStart)
            c = c->parent;

        QQmlPropertyData *rv = const_cast<QQmlPropertyData *>(&c->methodIndexCache.at(index - c->methodIndexCacheStart));

        if (rv->arguments)  
            return static_cast<A *>(rv->arguments)->arguments;

        const QMetaObject *metaObject = object->metaObject();
        QMetaMethod m = metaObject->method(index);
        QList<QByteArray> argTypeNames = m.parameterTypes();

        A *args = static_cast<A *>(malloc(sizeof(A) + (argTypeNames.count() + 1) * sizeof(int)));
        args->arguments[0] = argTypeNames.count();

        for (int ii = 0; ii < argTypeNames.count(); ++ii) {
            int type = QMetaType::type(argTypeNames.at(ii));
            if (type == QVariant::Invalid)
                type = EnumType(object->metaObject(), argTypeNames.at(ii));
            if (type == QVariant::Invalid) {
                if (unknownTypeError) *unknownTypeError = argTypeNames.at(ii);
                free(args);
                return 0;
            }
            args->arguments[ii + 1] = type;
        }

        rv->arguments = args;
        args->next = c->argumentsCache;
        c->argumentsCache = args;
        return static_cast<A *>(rv->arguments)->arguments;

    } else {
        QMetaMethod m = object->metaObject()->method(index);
        QList<QByteArray> argTypeNames = m.parameterTypes();
        dummy.resize(argTypeNames.count() + 1);
        dummy[0] = argTypeNames.count();

        for (int ii = 0; ii < argTypeNames.count(); ++ii) {
            int type = QMetaType::type(argTypeNames.at(ii));
            if (type == QVariant::Invalid)
                type = EnumType(object->metaObject(), argTypeNames.at(ii));
            if (type == QVariant::Invalid) {
                if (unknownTypeError) *unknownTypeError = argTypeNames.at(ii);
                return 0;
            }
            dummy[ii + 1] = type;
        }

        return dummy.data();
    }
}

QQmlPropertyData qQmlPropertyCacheCreate(const QMetaObject *metaObject,
                                                         const QString &property)
{
    Q_ASSERT(metaObject);

    QQmlPropertyData rv;
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
    int defaultMethods = QObject::staticMetaObject.methodCount();
    for (int ii = methodCount - 1; ii >= defaultMethods; --ii) {
        // >=defaultMethods to block the signals and slots of QObject::staticMetaObject
        // incl. destroyed signals, objectNameChanged signal, deleteLater slot, _q_reregisterTimers slot.
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

inline const QString &qQmlPropertyCacheToString(const QString &string)
{
    return string;
}

inline QString qQmlPropertyCacheToString(const QHashedV8String &string)
{
    return QV8Engine::toStringStatic(string.string());
}

template<typename T>
QQmlPropertyData *
qQmlPropertyCacheProperty(QQmlEngine *engine, QObject *obj,
                                  const T &name, QQmlPropertyData &local)
{
    QQmlPropertyCache *cache = 0;

    if (engine) {
        QQmlData *ddata = QQmlData::get(obj);

        if (ddata && ddata->propertyCache) {
            cache = ddata->propertyCache;
        } else if (engine) {
            QQmlEnginePrivate *ep = QQmlEnginePrivate::get(engine);
            cache = ep->cache(obj);
            if (cache) {
                ddata = QQmlData::get(obj, true);
                cache->addref();
                ddata->propertyCache = cache;
            }
        }
    }

    QQmlPropertyData *rv = 0;

    if (cache) {
        rv = cache->property(name);
    } else {
        local = qQmlPropertyCacheCreate(obj->metaObject(),
                                                qQmlPropertyCacheToString(name));
        if (local.isValid())
            rv = &local;
    }

    return rv;
}

QQmlPropertyData *
QQmlPropertyCache::property(QQmlEngine *engine, QObject *obj, 
                                    const QHashedV8String &name, QQmlPropertyData &local)
{
    return qQmlPropertyCacheProperty<QHashedV8String>(engine, obj, name, local);
}

QQmlPropertyData *
QQmlPropertyCache::property(QQmlEngine *engine, QObject *obj,
                                    const QString &name, QQmlPropertyData &local)
{
    return qQmlPropertyCacheProperty<QString>(engine, obj, name, local);
}

static inline const QMetaObjectPrivate *priv(const uint* data)
{ return reinterpret_cast<const QMetaObjectPrivate*>(data); }

bool QQmlPropertyCache::isDynamicMetaObject(const QMetaObject *mo)
{
    return priv(mo->d.data)->revision >= 3 && priv(mo->d.data)->flags & DynamicMetaObject;
}

QT_END_NAMESPACE
