/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "qqmlpropertycache_p.h"

#include <private/qqmlengine_p.h>
#include <private/qqmlbinding_p.h>
#include <private/qqmlvmemetaobject_p.h>

#include <private/qmetaobject_p.h>
#include <private/qmetaobjectbuilder_p.h>
#include <private/qqmlpropertycachemethodarguments_p.h>

#include <private/qv4value_p.h>

#include <QtCore/qdebug.h>
#include <QtCore/QCryptographicHash>

#include <ctype.h> // for toupper
#include <limits.h>
#include <algorithm>

#ifdef Q_CC_MSVC
// nonstandard extension used : zero-sized array in struct/union.
#  pragma warning( disable : 4200 )
#endif

QT_BEGIN_NAMESPACE

#define Q_INT16_MAX 32767

static int metaObjectSignalCount(const QMetaObject *metaObject)
{
    int signalCount = 0;
    for (const QMetaObject *obj = metaObject; obj; obj = obj->superClass())
        signalCount += QMetaObjectPrivate::get(obj)->signalCount;
    return signalCount;
}

QQmlPropertyData::Flags
QQmlPropertyData::flagsForProperty(const QMetaProperty &p)
{
    QQmlPropertyData::Flags flags;

    flags.setIsConstant(p.isConstant());
    flags.setIsWritable(p.isWritable());
    flags.setIsResettable(p.isResettable());
    flags.setIsFinal(p.isFinal());
    flags.setIsRequired(p.isRequired());
    flags.setIsBindable(p.isBindable());


    const QMetaType metaType = p.metaType();
    int propType = metaType.id();
    if (p.isEnumType()) {
        flags.type = QQmlPropertyData::Flags::EnumType;
    } else if (metaType.flags() & QMetaType::PointerToQObject) {
        flags.type = QQmlPropertyData::Flags::QObjectDerivedType;
    } else if (propType == QMetaType::QVariant) {
        flags.type = QQmlPropertyData::Flags::QVariantType;
    } else if (propType < static_cast<int>(QMetaType::User)) {
        // nothing to do
    } else if (propType == qMetaTypeId<QQmlBinding *>()) {
        flags.type = QQmlPropertyData::Flags::QmlBindingType;
    } else if (propType == qMetaTypeId<QJSValue>()) {
        flags.type = QQmlPropertyData::Flags::QJSValueType;
    } else if (metaType.flags() & QMetaType::IsQmlList) {
        flags.type = QQmlPropertyData::Flags::QListType;
    }

    return flags;
}

void QQmlPropertyData::load(const QMetaProperty &p)
{
    Q_ASSERT(p.revision() <= std::numeric_limits<quint16>::max());
    setCoreIndex(p.propertyIndex());
    setNotifyIndex(QMetaObjectPrivate::signalIndex(p.notifySignal()));
    setFlags(flagsForProperty(p));
    setRevision(QTypeRevision::fromEncodedVersion(p.revision()));
    QMetaType type = p.metaType();
    setPropType(type);
}

void QQmlPropertyData::load(const QMetaMethod &m)
{
    setCoreIndex(m.methodIndex());
    setArguments(nullptr);

    setPropType(m.returnMetaType());

    m_flags.type = Flags::FunctionType;
    if (m.methodType() == QMetaMethod::Signal) {
        m_flags.setIsSignal(true);
    } else if (m.methodType() == QMetaMethod::Constructor) {
        m_flags.setIsConstructor(true);
        setPropType(QMetaType::fromType<QObject *>());
    }
    m_flags.setIsConstant(m.isConst());

    const int paramCount = m.parameterCount();
    if (paramCount) {
        m_flags.setHasArguments(true);
        if ((paramCount == 1) && (m.parameterMetaType(0) == QMetaType::fromType<QQmlV4Function *>()))
            m_flags.setIsV4Function(true);
    }

    if (m.attributes() & QMetaMethod::Cloned)
        m_flags.setIsCloned(true);

    Q_ASSERT(m.revision() <= std::numeric_limits<quint16>::max());
    setRevision(QTypeRevision::fromEncodedVersion(m.revision()));
}

/*!
Creates a new empty QQmlPropertyCache.
*/
QQmlPropertyCache::QQmlPropertyCache()
    : propertyIndexCacheStart(0), _parent(nullptr),
      argumentsCache(nullptr), methodIndexCacheStart(0), signalHandlerIndexCacheStart(0),
      _jsFactoryMethodIndex(-1), _hasPropertyOverrides(false)
{
}

/*!
Creates a new QQmlPropertyCache of \a metaObject.
*/
QQmlPropertyCache::QQmlPropertyCache(const QMetaObject *metaObject, QTypeRevision metaObjectRevision)
    : QQmlPropertyCache()
{
    Q_ASSERT(metaObject);

    update(metaObject);

    if (metaObjectRevision.isValid() && metaObjectRevision != QTypeRevision::zero()) {
        // Set the revision of the meta object that this cache describes to be
        // 'metaObjectRevision'. This is useful when constructing a property cache
        // from a type that was created directly in C++, and not through QML. For such
        // types, the revision for each recorded QMetaObject would normally be zero, which
        // would exclude any revisioned properties.
        for (int metaObjectOffset = 0; metaObjectOffset < allowedRevisionCache.size(); ++metaObjectOffset)
            allowedRevisionCache[metaObjectOffset] = metaObjectRevision;
    }
}

QQmlPropertyCache::~QQmlPropertyCache()
{
    QQmlPropertyCacheMethodArguments *args = argumentsCache;
    while (args) {
        QQmlPropertyCacheMethodArguments *next = args->next;
        delete args->names;
        free(args);
        args = next;
    }

    // We must clear this prior to releasing the parent incase it is a
    // linked hash
    stringCache.clear();
    if (_parent) _parent->release();

    _parent = nullptr;
}

QQmlPropertyCache *QQmlPropertyCache::copy(int reserve)
{
    QQmlPropertyCache *cache = new QQmlPropertyCache();
    cache->_parent = this;
    cache->_parent->addref();
    cache->propertyIndexCacheStart = propertyIndexCache.count() + propertyIndexCacheStart;
    cache->methodIndexCacheStart = methodIndexCache.count() + methodIndexCacheStart;
    cache->signalHandlerIndexCacheStart = signalHandlerIndexCache.count() + signalHandlerIndexCacheStart;
    cache->stringCache.linkAndReserve(stringCache, reserve);
    cache->allowedRevisionCache = allowedRevisionCache;
    cache->_metaObject = _metaObject;
    cache->_defaultPropertyName = _defaultPropertyName;

    return cache;
}

QQmlPropertyCache *QQmlPropertyCache::copy()
{
    return copy(0);
}

QQmlPropertyCache *QQmlPropertyCache::copyAndReserve(int propertyCount, int methodCount,
                                                     int signalCount, int enumCount)
{
    QQmlPropertyCache *rv = copy(propertyCount + methodCount + signalCount);
    rv->propertyIndexCache.reserve(propertyCount);
    rv->methodIndexCache.reserve(methodCount);
    rv->signalHandlerIndexCache.reserve(signalCount);
    rv->enumCache.reserve(enumCount);
    rv->_metaObject = RefCountedMetaObject();

    return rv;
}

/*! \internal

    \a notifyIndex MUST be in the signal index range (see QObjectPrivate::signalIndex()).
    This is different from QMetaMethod::methodIndex().
*/
void QQmlPropertyCache::appendProperty(const QString &name, QQmlPropertyData::Flags flags,
                                       int coreIndex, QMetaType propType, QTypeRevision version,
                                       int notifyIndex)
{
    QQmlPropertyData data;
    data.setPropType(propType);
    data.setCoreIndex(coreIndex);
    data.setNotifyIndex(notifyIndex);
    data.setFlags(flags);
    data.setTypeVersion(version);

    const OverrideResult overrideResult = handleOverride(name, &data);
    if (overrideResult == InvalidOverride)
        return;

    int index = propertyIndexCache.count();
    propertyIndexCache.append(data);

    setNamedProperty(name, index + propertyOffset(), propertyIndexCache.data() + index,
                     overrideResult == ValidOverride);
}

void QQmlPropertyCache::appendSignal(const QString &name, QQmlPropertyData::Flags flags,
                                     int coreIndex, const QMetaType *types,
                                     const QList<QByteArray> &names)
{
    QQmlPropertyData data;
    data.setPropType(QMetaType());
    data.setCoreIndex(coreIndex);
    data.setFlags(flags);
    data.setArguments(nullptr);

    QQmlPropertyData handler = data;
    handler.m_flags.setIsSignalHandler(true);

    if (types) {
        const auto argumentCount = names.length();
        QQmlPropertyCacheMethodArguments *args = createArgumentsObject(argumentCount, names);
        new (args->types) QMetaType; // Invalid return type
        ::memcpy(args->types + 1, types, argumentCount * sizeof(QMetaType));
        data.setArguments(args);
    }

    const OverrideResult overrideResult = handleOverride(name, &data);
    if (overrideResult == InvalidOverride)
        return;

    int methodIndex = methodIndexCache.count();
    methodIndexCache.append(data);

    int signalHandlerIndex = signalHandlerIndexCache.count();
    signalHandlerIndexCache.append(handler);

    QString handlerName = QLatin1String("on") + name;
    handlerName[2] = handlerName.at(2).toUpper();

    setNamedProperty(name, methodIndex + methodOffset(), methodIndexCache.data() + methodIndex,
                     overrideResult == ValidOverride);
    setNamedProperty(handlerName, signalHandlerIndex + signalOffset(),
                     signalHandlerIndexCache.data() + signalHandlerIndex,
                     overrideResult == ValidOverride);
}

void QQmlPropertyCache::appendMethod(const QString &name, QQmlPropertyData::Flags flags,
                                     int coreIndex, QMetaType returnType,
                                     const QList<QByteArray> &names,
                                     const QVector<QMetaType> &parameterTypes)
{
    int argumentCount = names.count();

    QQmlPropertyData data;
    data.setPropType(returnType);
    data.setCoreIndex(coreIndex);
    data.setFlags(flags);
    const OverrideResult overrideResult = handleOverride(name, &data);
    if (overrideResult == InvalidOverride)
        return;

    QQmlPropertyCacheMethodArguments *args = createArgumentsObject(argumentCount, names);
    new (args->types) QMetaType(returnType);
    for (int ii = 0; ii < argumentCount; ++ii)
        new (args->types + ii + 1) QMetaType(parameterTypes.at(ii));
    data.setArguments(args);

    int methodIndex = methodIndexCache.count();
    methodIndexCache.append(data);

    setNamedProperty(name, methodIndex + methodOffset(), methodIndexCache.data() + methodIndex,
                     overrideResult == ValidOverride);
}

void QQmlPropertyCache::appendEnum(const QString &name, const QVector<QQmlEnumValue> &values)
{
    QQmlEnumData data;
    data.name = name;
    data.values = values;
    enumCache.append(data);
}

// Returns this property cache's metaObject, creating it if necessary.
const QMetaObject *QQmlPropertyCache::createMetaObject()
{
    if (!_metaObject) {
        QMetaObjectBuilder builder;
        toMetaObjectBuilder(builder);
        builder.setSuperClass(_parent->createMetaObject());
        _metaObject = RefCountedMetaObject(builder.toMetaObject(), RefCountedMetaObject::SharedMetaObject);
    }

    return _metaObject;
}

QQmlPropertyData *QQmlPropertyCache::maybeUnresolvedProperty(int index) const
{
    if (index < 0 || index >= propertyCount())
        return nullptr;

    QQmlPropertyData *rv = nullptr;
    if (index < propertyIndexCacheStart)
        return _parent->maybeUnresolvedProperty(index);
    else
        rv = const_cast<QQmlPropertyData *>(&propertyIndexCache.at(index - propertyIndexCacheStart));
    return rv;
}

QQmlPropertyData *QQmlPropertyCache::defaultProperty() const
{
    return property(defaultPropertyName(), nullptr, nullptr);
}

void QQmlPropertyCache::setParent(QQmlPropertyCache *newParent)
{
    if (_parent == newParent)
        return;
    if (_parent)
        _parent->release();
    _parent = newParent;
    _parent->addref();
}

QQmlPropertyCache *
QQmlPropertyCache::copyAndAppend(const QMetaObject *metaObject,
                                 QTypeRevision typeVersion,
                                 QQmlPropertyData::Flags propertyFlags,
                                 QQmlPropertyData::Flags methodFlags,
                                 QQmlPropertyData::Flags signalFlags)
{
    Q_ASSERT(QMetaObjectPrivate::get(metaObject)->revision >= 4);

    // Reserve enough space in the name hash for all the methods (including signals), all the
    // signal handlers and all the properties.  This assumes no name clashes, but this is the
    // common case.
    QQmlPropertyCache *rv = copy(QMetaObjectPrivate::get(metaObject)->methodCount +
                                         QMetaObjectPrivate::get(metaObject)->signalCount +
                                         QMetaObjectPrivate::get(metaObject)->propertyCount);

    rv->append(metaObject, typeVersion, propertyFlags, methodFlags, signalFlags);

    return rv;
}

void QQmlPropertyCache::append(const QMetaObject *metaObject,
                               QTypeRevision typeVersion,
                               QQmlPropertyData::Flags propertyFlags,
                               QQmlPropertyData::Flags methodFlags,
                               QQmlPropertyData::Flags signalFlags)
{
    _metaObject = RefCountedMetaObject(metaObject, RefCountedMetaObject::StaticMetaObject);

    bool dynamicMetaObject = isDynamicMetaObject(metaObject);

    allowedRevisionCache.append(QTypeRevision::zero());

    int methodCount = metaObject->methodCount();
    Q_ASSERT(QMetaObjectPrivate::get(metaObject)->revision >= 4);
    int signalCount = metaObjectSignalCount(metaObject);
    int classInfoCount = QMetaObjectPrivate::get(metaObject)->classInfoCount;

    if (classInfoCount) {
        int classInfoOffset = metaObject->classInfoOffset();
        for (int ii = 0; ii < classInfoCount; ++ii) {
            int idx = ii + classInfoOffset;
            QMetaClassInfo mci = metaObject->classInfo(idx);
            const char *name = mci.name();
            if (0 == qstrcmp(name, "DefaultProperty")) {
                _defaultPropertyName = QString::fromUtf8(mci.value());
            } else if (0 == qstrcmp(name, "qt_QmlJSWrapperFactoryMethod")) {
                const char * const factoryMethod = mci.value();
                _jsFactoryMethodIndex = metaObject->indexOfSlot(factoryMethod);
                if (_jsFactoryMethodIndex != -1)
                    _jsFactoryMethodIndex -= metaObject->methodOffset();
            }
        }
    }

    //Used to block access to QObject::destroyed() and QObject::deleteLater() from QML
    static const int destroyedIdx1 = QObject::staticMetaObject.indexOfSignal("destroyed(QObject*)");
    static const int destroyedIdx2 = QObject::staticMetaObject.indexOfSignal("destroyed()");
    static const int deleteLaterIdx = QObject::staticMetaObject.indexOfSlot("deleteLater()");
    // These indices don't apply to gadgets, so don't block them.
    // It is enough to check for QObject::staticMetaObject here because the loop below excludes
    // methods of parent classes: It starts at metaObject->methodOffset()
    const bool preventDestruction = (metaObject == &QObject::staticMetaObject);

    int methodOffset = metaObject->methodOffset();
    int signalOffset = signalCount - QMetaObjectPrivate::get(metaObject)->signalCount;

    // update() should have reserved enough space in the vector that this doesn't cause a realloc
    // and invalidate the stringCache.
    methodIndexCache.resize(methodCount - methodIndexCacheStart);
    signalHandlerIndexCache.resize(signalCount - signalHandlerIndexCacheStart);
    int signalHandlerIndex = signalOffset;
    for (int ii = methodOffset; ii < methodCount; ++ii) {
        if (preventDestruction && (ii == destroyedIdx1 || ii == destroyedIdx2 || ii == deleteLaterIdx))
            continue;
        QMetaMethod m = metaObject->method(ii);
        if (m.access() == QMetaMethod::Private)
            continue;

        // Extract method name
        // It's safe to keep the raw name pointer
        Q_ASSERT(QMetaObjectPrivate::get(metaObject)->revision >= 7);
        const char *rawName = m.name().constData();
        const char *cptr = rawName;
        char utf8 = 0;
        while (*cptr) {
            utf8 |= *cptr & 0x80;
            ++cptr;
        }

        QQmlPropertyData *data = &methodIndexCache[ii - methodIndexCacheStart];
        QQmlPropertyData *sigdata = nullptr;

        if (m.methodType() == QMetaMethod::Signal)
            data->setFlags(signalFlags);
        else
            data->setFlags(methodFlags);

        data->load(m);

        Q_ASSERT((allowedRevisionCache.count() - 1) < Q_INT16_MAX);
        data->setMetaObjectOffset(allowedRevisionCache.count() - 1);

        if (data->isSignal()) {
            sigdata = &signalHandlerIndexCache[signalHandlerIndex - signalHandlerIndexCacheStart];
            *sigdata = *data;
            sigdata->m_flags.setIsSignalHandler(true);
        }

        QQmlPropertyData *old = nullptr;

        if (utf8) {
            QHashedString methodName(QString::fromUtf8(rawName, cptr - rawName));
            if (StringCache::mapped_type *it = stringCache.value(methodName)) {
                if (handleOverride(methodName, data, (old = it->second)) == InvalidOverride)
                    continue;
            }
            setNamedProperty(methodName, ii, data, (old != nullptr));

            if (data->isSignal()) {
                QHashedString on(QLatin1String("on") % methodName.at(0).toUpper() % QStringView{methodName}.mid(1));
                setNamedProperty(on, ii, sigdata, (old != nullptr));
                ++signalHandlerIndex;
            }
        } else {
            QHashedCStringRef methodName(rawName, cptr - rawName);
            if (StringCache::mapped_type *it = stringCache.value(methodName)) {
                if (handleOverride(methodName, data, (old = it->second)) == InvalidOverride)
                    continue;
            }
            setNamedProperty(methodName, ii, data, (old != nullptr));

            if (data->isSignal()) {
                int length = methodName.length();

                QVarLengthArray<char, 128> str(length+3);
                str[0] = 'o';
                str[1] = 'n';
                str[2] = toupper(rawName[0]);
                if (length > 1)
                    memcpy(&str[3], &rawName[1], length - 1);
                str[length + 2] = '\0';

                QHashedString on(QString::fromLatin1(str.data()));
                setNamedProperty(on, ii, data, (old != nullptr));
                ++signalHandlerIndex;
            }
        }

        if (old) {
            // We only overload methods in the same class, exactly like C++
            if (old->isFunction() && old->coreIndex() >= methodOffset)
                data->m_flags.setIsOverload(true);
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

        data->setFlags(propertyFlags);
        data->load(p);
        data->setTypeVersion(typeVersion);

        data->m_flags.setIsDirect(!dynamicMetaObject);

        Q_ASSERT((allowedRevisionCache.count() - 1) < Q_INT16_MAX);
        data->setMetaObjectOffset(allowedRevisionCache.count() - 1);

        QQmlPropertyData *old = nullptr;

        if (utf8) {
            QHashedString propName(QString::fromUtf8(str, cptr - str));
            if (StringCache::mapped_type *it = stringCache.value(propName)) {
                if (handleOverride(propName, data, (old = it->second)) == InvalidOverride)
                    continue;
            }
            setNamedProperty(propName, ii, data, (old != nullptr));
        } else {
            QHashedCStringRef propName(str, cptr - str);
            if (StringCache::mapped_type *it = stringCache.value(propName)) {
                if (handleOverride(propName, data, (old = it->second)) == InvalidOverride)
                    continue;
            }
            setNamedProperty(propName, ii, data, (old != nullptr));
        }

        bool isGadget = true;
        for (const QMetaObject *it = metaObject; it != nullptr; it = it->superClass()) {
            if (it == &QObject::staticMetaObject)
                isGadget = false;
        }

        if (isGadget) // always dispatch over a 'normal' meta-call so the QQmlValueType can intercept
            data->m_flags.setIsDirect(false);
        else
            data->trySetStaticMetaCallFunction(metaObject->d.static_metacall, ii - propOffset);
    }
}

void QQmlPropertyCache::updateRecur(const QMetaObject *metaObject)
{
    if (!metaObject)
        return;

    updateRecur(metaObject->superClass());

    append(metaObject, QTypeRevision());
}

void QQmlPropertyCache::update(const QMetaObject *metaObject)
{
    Q_ASSERT(metaObject);
    stringCache.clear();

    // Preallocate enough space in the index caches for all the properties/methods/signals that
    // are not cached in a parent cache so that the caches never need to be reallocated as this
    // would invalidate pointers stored in the stringCache.
    int pc = metaObject->propertyCount();
    int mc = metaObject->methodCount();
    int sc = metaObjectSignalCount(metaObject);
    propertyIndexCache.reserve(pc - propertyIndexCacheStart);
    methodIndexCache.reserve(mc - methodIndexCacheStart);
    signalHandlerIndexCache.reserve(sc - signalHandlerIndexCacheStart);

    // Reserve enough space in the stringCache for all properties/methods/signals including those
    // cached in a parent cache.
    stringCache.reserve(pc + mc + sc);

    updateRecur(metaObject);
}

/*! \internal
    invalidates and updates the PropertyCache if the QMetaObject has changed.
    This function is used in the tooling to update dynamic properties.
*/
void QQmlPropertyCache::invalidate(const QMetaObject *metaObject)
{
    propertyIndexCache.clear();
    methodIndexCache.clear();
    signalHandlerIndexCache.clear();

    _hasPropertyOverrides = false;
    argumentsCache = nullptr;

    int pc = metaObject->propertyCount();
    int mc = metaObject->methodCount();
    int sc = metaObjectSignalCount(metaObject);
    int reserve = pc + mc + sc;

    if (parent()) {
        propertyIndexCacheStart = parent()->propertyIndexCache.count() + parent()->propertyIndexCacheStart;
        methodIndexCacheStart = parent()->methodIndexCache.count() + parent()->methodIndexCacheStart;
        signalHandlerIndexCacheStart = parent()->signalHandlerIndexCache.count() + parent()->signalHandlerIndexCacheStart;
        stringCache.linkAndReserve(parent()->stringCache, reserve);
        append(metaObject, QTypeRevision());
    } else {
        propertyIndexCacheStart = 0;
        methodIndexCacheStart = 0;
        signalHandlerIndexCacheStart = 0;
        update(metaObject);
    }
}

QQmlPropertyData *QQmlPropertyCache::findProperty(
        StringCache::ConstIterator it, QObject *object,
        const QQmlRefPointer<QQmlContextData> &context) const
{
    QQmlData *data = (object ? QQmlData::get(object) : nullptr);
    const QQmlVMEMetaObject *vmemo = nullptr;
    if (data && data->hasVMEMetaObject) {
        QObjectPrivate *op = QObjectPrivate::get(object);
        vmemo = static_cast<const QQmlVMEMetaObject *>(op->metaObject);
    }
    return findProperty(it, vmemo, context);
}

namespace {

inline bool contextHasNoExtensions(const QQmlRefPointer<QQmlContextData> &context)
{
    // This context has no extension if its parent is the engine's rootContext,
    // which has children but no imports
    return (!context->parent() || !context->parent()->imports());
}

inline int maximumIndexForProperty(QQmlPropertyData *prop, const int methodCount, const int signalCount, const int propertyCount)
{
    return prop->isFunction() ? methodCount
                              : prop->isSignalHandler() ? signalCount
                                                        : propertyCount;
}

}

QQmlPropertyData *QQmlPropertyCache::findProperty(
        StringCache::ConstIterator it, const QQmlVMEMetaObject *vmemo,
        const QQmlRefPointer<QQmlContextData> &context) const
{
    StringCache::ConstIterator end = stringCache.end();

    if (it != end) {
        QQmlPropertyData *result = it.value().second;

        // If there exists a typed property (not a function or signal handler), of the
        // right name available to the specified context, we need to return that
        // property rather than any subsequent override

        if (vmemo && context && !contextHasNoExtensions(context)) {
            // Find the meta-object that corresponds to the supplied context
            do {
                if (vmemo->ctxt.contextData().data() == context.data())
                    break;

                vmemo = vmemo->parentVMEMetaObject();
            } while (vmemo);
        }

        if (vmemo) {
            const int methodCount = vmemo->cache->methodCount();
            const int signalCount = vmemo->cache->signalCount();
            const int propertyCount = vmemo->cache->propertyCount();

            // Ensure that the property we resolve to is accessible from this meta-object
            do {
                const StringCache::mapped_type &property(it.value());

                if (property.first < maximumIndexForProperty(property.second, methodCount, signalCount, propertyCount)) {
                    // This property is available in the specified context
                    if (property.second->isFunction() || property.second->isSignalHandler()) {
                        // Prefer the earlier resolution
                    } else {
                        // Prefer the typed property to any previous property found
                        result = property.second;
                    }
                    break;
                }

                // See if there is a better candidate
                it = stringCache.findNext(it);
            } while (it != end);
        }

        return result;
    }

    return nullptr;
}

QString QQmlPropertyData::name(QObject *object) const
{
    if (!object)
        return QString();

    return name(object->metaObject());
}

QString QQmlPropertyData::name(const QMetaObject *metaObject) const
{
    if (!metaObject || coreIndex() == -1)
        return QString();

    if (isFunction()) {
        QMetaMethod m = metaObject->method(coreIndex());

        return QString::fromUtf8(m.name().constData());
    } else {
        QMetaProperty p = metaObject->property(coreIndex());
        return QString::fromUtf8(p.name());
    }
}

bool QQmlPropertyData::markAsOverrideOf(QQmlPropertyData *predecessor)
{
    Q_ASSERT(predecessor != this);
    if (predecessor->isFinal())
        return false;

    setOverrideIndexIsProperty(!predecessor->isFunction());
    setOverrideIndex(predecessor->coreIndex());
    predecessor->m_flags.setIsOverridden(true);
    Q_ASSERT(predecessor->isOverridden());
    return true;
}

QQmlPropertyCacheMethodArguments *QQmlPropertyCache::createArgumentsObject(
        int argc, const QList<QByteArray> &names)
{
    typedef QQmlPropertyCacheMethodArguments A;
    A *args = static_cast<A *>(malloc(sizeof(A) + argc * sizeof(QMetaType)));
    args->names = argc ? new QList<QByteArray>(names) : nullptr;
    args->next = argumentsCache;
    argumentsCache = args;
    return args;
}

QString QQmlPropertyCache::signalParameterStringForJS(QV4::ExecutionEngine *engine, const QList<QByteArray> &parameterNameList, QString *errorString)
{
    bool unnamedParameter = false;
    const QSet<QString> &illegalNames = engine->illegalNames();
    QString parameters;

    for (int i = 0; i < parameterNameList.count(); ++i) {
        if (i > 0)
            parameters += QLatin1Char(',');
        const QByteArray &param = parameterNameList.at(i);
        if (param.isEmpty())
            unnamedParameter = true;
        else if (unnamedParameter) {
            if (errorString)
                *errorString = QCoreApplication::translate("QQmlRewrite", "Signal uses unnamed parameter followed by named parameter.");
            return QString();
        } else if (illegalNames.contains(QString::fromUtf8(param))) {
            if (errorString)
                *errorString = QCoreApplication::translate("QQmlRewrite", "Signal parameter \"%1\" hides global variable.").arg(QString::fromUtf8(param));
            return QString();
        }
        parameters += QString::fromUtf8(param);
    }

    return parameters;
}

int QQmlPropertyCache::originalClone(int index)
{
    while (signal(index)->isCloned())
        --index;
    return index;
}

int QQmlPropertyCache::originalClone(const QObject *object, int index)
{
    QQmlData *data = QQmlData::get(object, false);
    if (data && data->propertyCache) {
        QQmlPropertyCache *cache = data->propertyCache;
        QQmlPropertyData *sig = cache->signal(index);
        while (sig && sig->isCloned()) {
            --index;
            sig = cache->signal(index);
        }
    } else {
        while (QMetaObjectPrivate::signal(object->metaObject(), index).attributes() & QMetaMethod::Cloned)
            --index;
    }
    return index;
}

template<typename T>
static QQmlPropertyData qQmlPropertyCacheCreate(const QMetaObject *metaObject, const T& propertyName)
{
    Q_ASSERT(metaObject);

    QQmlPropertyData rv;

    /* It's important to check the method list before checking for properties;
     * otherwise, if the meta object is dynamic, a property will be created even
     * if not found and it might obscure a method having the same name. */

    //Used to block access to QObject::destroyed() and QObject::deleteLater() from QML
    static const int destroyedIdx1 = QObject::staticMetaObject.indexOfSignal("destroyed(QObject*)");
    static const int destroyedIdx2 = QObject::staticMetaObject.indexOfSignal("destroyed()");
    static const int deleteLaterIdx = QObject::staticMetaObject.indexOfSlot("deleteLater()");
    // These indices don't apply to gadgets, so don't block them.
    const bool preventDestruction = metaObject->superClass() || metaObject == &QObject::staticMetaObject;

    int methodCount = metaObject->methodCount();
    for (int ii = methodCount - 1; ii >= 0; --ii) {
        if (preventDestruction && (ii == destroyedIdx1 || ii == destroyedIdx2 || ii == deleteLaterIdx))
            continue;
        QMetaMethod m = metaObject->method(ii);
        if (m.access() == QMetaMethod::Private)
            continue;

        if (m.name() == propertyName) {
            rv.load(m);
            return rv;
        }
    }

    {
        const QMetaObject *cmo = metaObject;
        while (cmo) {
            int idx = cmo->indexOfProperty(propertyName);
            if (idx != -1) {
                QMetaProperty p = cmo->property(idx);
                if (p.isScriptable()) {
                    rv.load(p);
                    return rv;
                } else {
                    bool changed = false;
                    while (cmo && cmo->propertyOffset() >= idx) {
                        cmo = cmo->superClass();
                        changed = true;
                    }
                    /* If the "cmo" variable didn't change, set it to 0 to
                     * avoid running into an infinite loop */
                    if (!changed) cmo = nullptr;
                }
            } else {
                cmo = nullptr;
            }
        }
    }
    return rv;
}

static inline const char *qQmlPropertyCacheToString(QLatin1String string)
{
    return string.data();
}

static inline QByteArray qQmlPropertyCacheToString(QStringView string)
{
    return string.toUtf8();
}

static inline QByteArray qQmlPropertyCacheToString(const QV4::String *string)
{
    return string->toQString().toUtf8();
}

template<typename T>
QQmlPropertyData *
qQmlPropertyCacheProperty(QJSEngine *engine, QObject *obj, T name,
                          const QQmlRefPointer<QQmlContextData> &context, QQmlPropertyData *local)
{
    QQmlPropertyCache *cache = nullptr;

    QQmlData *ddata = QQmlData::get(obj, false);

    if (ddata && ddata->propertyCache) {
        cache = ddata->propertyCache;
    } else if (engine) {
        QJSEnginePrivate *ep = QJSEnginePrivate::get(engine);
        cache = ep->cache(obj);
        if (cache) {
            ddata = QQmlData::get(obj, true);
            cache->addref();
            ddata->propertyCache = cache;
        }
    }

    QQmlPropertyData *rv = nullptr;

    if (cache) {
        rv = cache->property(name, obj, context);
    } else if (local) {
        *local = qQmlPropertyCacheCreate(obj->metaObject(), qQmlPropertyCacheToString(name));
        if (local->isValid())
            rv = local;
    }

    return rv;
}

QQmlPropertyData *
QQmlPropertyCache::property(QJSEngine *engine, QObject *obj, const QV4::String *name,
                            const QQmlRefPointer<QQmlContextData> &context, QQmlPropertyData *local)
{
    return qQmlPropertyCacheProperty<const QV4::String *>(engine, obj, name, context, local);
}

QQmlPropertyData *
QQmlPropertyCache::property(QJSEngine *engine, QObject *obj, QStringView name,
                            const QQmlRefPointer<QQmlContextData> &context, QQmlPropertyData *local)
{
    return qQmlPropertyCacheProperty<const QStringView &>(engine, obj, name, context, local);
}

QQmlPropertyData *
QQmlPropertyCache::property(QJSEngine *engine, QObject *obj, const QLatin1String &name,
                            const QQmlRefPointer<QQmlContextData> &context, QQmlPropertyData *local)
{
    return qQmlPropertyCacheProperty<const QLatin1String &>(engine, obj, name, context, local);
}

// these two functions are copied from qmetaobject.cpp
static inline const QMetaObjectPrivate *priv(const uint* data)
{ return reinterpret_cast<const QMetaObjectPrivate*>(data); }

static inline const QByteArray stringData(const QMetaObject *mo, int index)
{
    uint offset = mo->d.stringdata[2*index];
    uint length = mo->d.stringdata[2*index + 1];
    const char *string = reinterpret_cast<const char *>(mo->d.stringdata) + offset;
    return QByteArray::fromRawData(string, length);
}

bool QQmlPropertyCache::isDynamicMetaObject(const QMetaObject *mo)
{
    return priv(mo->d.data)->flags & DynamicMetaObject;
}

const char *QQmlPropertyCache::className() const
{
    if (_metaObject)
        return _metaObject->className();
    else
        return _dynamicClassName.constData();
}

void QQmlPropertyCache::toMetaObjectBuilder(QMetaObjectBuilder &builder)
{
    struct Sort { static bool lt(const QPair<QString, QQmlPropertyData *> &lhs,
                                 const QPair<QString, QQmlPropertyData *> &rhs) {
        return lhs.second->coreIndex() < rhs.second->coreIndex();
    } };

    struct Insert { static void in(QQmlPropertyCache *This,
                                   QList<QPair<QString, QQmlPropertyData *> > &properties,
                                   QList<QPair<QString, QQmlPropertyData *> > &methods,
                                   StringCache::ConstIterator iter, QQmlPropertyData *data) {
        if (data->isSignalHandler())
            return;

        if (data->isFunction()) {
            if (data->coreIndex() < This->methodIndexCacheStart)
                return;

            QPair<QString, QQmlPropertyData *> entry = qMakePair((QString)iter.key(), data);
            // Overrides can cause the entry to already exist
            if (!methods.contains(entry)) methods.append(entry);

            data = This->overrideData(data);
            if (data && !data->isFunction()) Insert::in(This, properties, methods, iter, data);
        } else {
            if (data->coreIndex() < This->propertyIndexCacheStart)
                return;

            QPair<QString, QQmlPropertyData *> entry = qMakePair((QString)iter.key(), data);
            // Overrides can cause the entry to already exist
            if (!properties.contains(entry)) properties.append(entry);

            data = This->overrideData(data);
            if (data) Insert::in(This, properties, methods, iter, data);
        }

    } };

    builder.setClassName(_dynamicClassName);

    QList<QPair<QString, QQmlPropertyData *> > properties;
    QList<QPair<QString, QQmlPropertyData *> > methods;

    for (StringCache::ConstIterator iter = stringCache.begin(), cend = stringCache.end(); iter != cend; ++iter)
        Insert::in(this, properties, methods, iter, iter.value().second);

    Q_ASSERT(properties.count() == propertyIndexCache.count());
    Q_ASSERT(methods.count() == methodIndexCache.count());

    std::sort(properties.begin(), properties.end(), Sort::lt);
    std::sort(methods.begin(), methods.end(), Sort::lt);

    for (int ii = 0; ii < properties.count(); ++ii) {
        QQmlPropertyData *data = properties.at(ii).second;

        int notifierId = -1;
        if (data->notifyIndex() != -1)
            notifierId = data->notifyIndex() - signalHandlerIndexCacheStart;

        QMetaPropertyBuilder property = builder.addProperty(properties.at(ii).first.toUtf8(),
                                                            data->propType().name(),
                                                            data->propType(),
                                                            notifierId);

        property.setReadable(true);
        property.setWritable(data->isWritable());
        property.setResettable(data->isResettable());
        property.setBindable(data->isBindable());
    }

    for (int ii = 0; ii < methods.count(); ++ii) {
        QQmlPropertyData *data = methods.at(ii).second;

        QByteArray returnType;
        if (data->propType().isValid())
            returnType = data->propType().name();

        QByteArray signature;
        // '+=' reserves extra capacity. Follow-up appending will be probably free.
        signature += methods.at(ii).first.toUtf8() + '(';

        QQmlPropertyCacheMethodArguments *arguments = nullptr;
        if (data->hasArguments()) {
            arguments = data->arguments();
            for (int ii = 0, end = arguments->names ? arguments->names->length() : 0;
                 ii < end; ++ii) {
                if (ii != 0)
                    signature.append(',');
                signature.append(arguments->types[1 + ii].name());
            }
        }

        signature.append(')');

        QMetaMethodBuilder method;
        if (data->isSignal()) {
            method = builder.addSignal(signature);
        } else {
            method = builder.addSlot(signature);
        }
        method.setAccess(QMetaMethod::Public);

        if (arguments && arguments->names)
            method.setParameterNames(*arguments->names);

        if (!returnType.isEmpty())
            method.setReturnType(returnType);
    }

    for (int ii = 0; ii < enumCache.count(); ++ii) {
        const QQmlEnumData &enumData = enumCache.at(ii);
        QMetaEnumBuilder enumeration = builder.addEnumerator(enumData.name.toUtf8());
        enumeration.setIsScoped(true);
        for (int jj = 0; jj < enumData.values.count(); ++jj) {
            const QQmlEnumValue &value = enumData.values.at(jj);
            enumeration.addKey(value.namedValue.toUtf8(), value.value);
        }
    }

    if (!_defaultPropertyName.isEmpty()) {
        QQmlPropertyData *dp = property(_defaultPropertyName, nullptr, nullptr);
        if (dp && dp->coreIndex() >= propertyIndexCacheStart) {
            Q_ASSERT(!dp->isFunction());
            builder.addClassInfo("DefaultProperty", _defaultPropertyName.toUtf8());
        }
    }
}

namespace {
template <typename StringVisitor, typename TypeInfoVisitor>
int visitMethods(const QMetaObject &mo, int methodOffset, int methodCount,
                 StringVisitor visitString, TypeInfoVisitor visitTypeInfo)
{
    int fieldsForParameterData = 0;

    bool hasRevisionedMethods = false;

    for (int i = 0; i < methodCount; ++i) {
        const int handle = methodOffset + i * QMetaObjectPrivate::IntsPerMethod;

        const uint flags = mo.d.data[handle + 4];
        if (flags & MethodRevisioned)
            hasRevisionedMethods = true;

        visitString(mo.d.data[handle + 0]); // name
        visitString(mo.d.data[handle + 3]); // tag

        const int argc = mo.d.data[handle + 1];
        const int paramIndex = mo.d.data[handle + 2];

        fieldsForParameterData += argc * 2; // type and name
        fieldsForParameterData += 1; // + return type

        // return type + args
        for (int i = 0; i < 1 + argc; ++i) {
            // type name (maybe)
            visitTypeInfo(mo.d.data[paramIndex + i]);

            // parameter name
            if (i > 0)
                visitString(mo.d.data[paramIndex + argc + i]);
        }
    }

    int fieldsForRevisions = 0;
    if (hasRevisionedMethods)
        fieldsForRevisions = methodCount;

    return methodCount * QMetaObjectPrivate::IntsPerMethod
            + fieldsForRevisions + fieldsForParameterData;
}

template <typename StringVisitor, typename TypeInfoVisitor>
int visitProperties(const QMetaObject &mo, StringVisitor visitString, TypeInfoVisitor visitTypeInfo)
{
    const QMetaObjectPrivate *const priv = reinterpret_cast<const QMetaObjectPrivate*>(mo.d.data);

    for (int i = 0; i < priv->propertyCount; ++i) {
        const int handle = priv->propertyData + i * QMetaObjectPrivate::IntsPerProperty;

        visitString(mo.d.data[handle]); // name
        visitTypeInfo(mo.d.data[handle + 1]);
    }

    return priv->propertyCount * QMetaObjectPrivate::IntsPerProperty;
}

template <typename StringVisitor>
int visitClassInfo(const QMetaObject &mo, StringVisitor visitString)
{
    const QMetaObjectPrivate *const priv = reinterpret_cast<const QMetaObjectPrivate*>(mo.d.data);
    const int intsPerClassInfo = 2;

    for (int i = 0; i < priv->classInfoCount; ++i) {
        const int handle = priv->classInfoData + i * intsPerClassInfo;

        visitString(mo.d.data[handle]); // key
        visitString(mo.d.data[handle + 1]); // value
    }

    return priv->classInfoCount * intsPerClassInfo;
}

template <typename StringVisitor>
int visitEnumerations(const QMetaObject &mo, StringVisitor visitString)
{
    const QMetaObjectPrivate *const priv = reinterpret_cast<const QMetaObjectPrivate*>(mo.d.data);

    int fieldCount = priv->enumeratorCount * QMetaObjectPrivate::IntsPerEnum;

    for (int i = 0; i < priv->enumeratorCount; ++i) {
        const uint *enumeratorData = mo.d.data + priv->enumeratorData + i * QMetaObjectPrivate::IntsPerEnum;

        const uint keyCount = enumeratorData[3];
        fieldCount += keyCount * 2;

        visitString(enumeratorData[0]); // name
        visitString(enumeratorData[1]); // enum name

        const uint keyOffset = enumeratorData[4];

        for (uint j = 0; j < keyCount; ++j) {
            visitString(mo.d.data[keyOffset + 2 * j]);
        }
    }

    return fieldCount;
}

template <typename StringVisitor>
int countMetaObjectFields(const QMetaObject &mo, StringVisitor stringVisitor)
{
    const QMetaObjectPrivate *const priv = reinterpret_cast<const QMetaObjectPrivate*>(mo.d.data);

    const auto typeInfoVisitor = [&stringVisitor](uint typeInfo) {
        if (typeInfo & IsUnresolvedType)
            stringVisitor(typeInfo & TypeNameIndexMask);
    };

    int fieldCount = MetaObjectPrivateFieldCount;

    fieldCount += visitMethods(mo, priv->methodData, priv->methodCount, stringVisitor,
                               typeInfoVisitor);
    fieldCount += visitMethods(mo, priv->constructorData, priv->constructorCount, stringVisitor,
                               typeInfoVisitor);

    fieldCount += visitProperties(mo, stringVisitor, typeInfoVisitor);
    fieldCount += visitClassInfo(mo, stringVisitor);
    fieldCount += visitEnumerations(mo, stringVisitor);

    return fieldCount;
}

} // anonymous namespace

static_assert(QMetaObjectPrivate::OutputRevision == 10, "Check and adjust determineMetaObjectSizes");

bool QQmlPropertyCache::determineMetaObjectSizes(const QMetaObject &mo, int *fieldCount,
                                                 int *stringCount)
{
    const QMetaObjectPrivate *priv = reinterpret_cast<const QMetaObjectPrivate*>(mo.d.data);
    if (priv->revision != 10)
        return false;

    uint highestStringIndex = 0;
    const auto stringIndexVisitor = [&highestStringIndex](uint index) {
        highestStringIndex = qMax(highestStringIndex, index);
    };

    *fieldCount = countMetaObjectFields(mo, stringIndexVisitor);
    *stringCount = highestStringIndex + 1;

    return true;
}

bool QQmlPropertyCache::addToHash(QCryptographicHash &hash, const QMetaObject &mo)
{
    int fieldCount = 0;
    int stringCount = 0;
    if (!determineMetaObjectSizes(mo, &fieldCount, &stringCount)) {
        return false;
    }

    hash.addData(reinterpret_cast<const char *>(mo.d.data), fieldCount * sizeof(uint));
    for (int i = 0; i < stringCount; ++i) {
        hash.addData(stringData(&mo, i));
    }

    return true;
}

QByteArray QQmlPropertyCache::checksum(bool *ok)
{
    if (!_checksum.isEmpty()) {
        *ok = true;
        return _checksum;
    }

    // Generate a checksum on the meta-object data only on C++ types.
    if (!_metaObject || _metaObject.isShared()) {
        *ok = false;
        return _checksum;
    }

    QCryptographicHash hash(QCryptographicHash::Md5);

    if (_parent) {
        hash.addData(_parent->checksum(ok));
        if (!*ok)
            return QByteArray();
    }

    if (!addToHash(hash, *createMetaObject())) {
        *ok = false;
        return QByteArray();
    }

    _checksum = hash.result();
    *ok = !_checksum.isEmpty();
    return _checksum;
}

/*! \internal
    \a index MUST be in the signal index range (see QObjectPrivate::signalIndex()).
    This is different from QMetaMethod::methodIndex().
*/
QList<QByteArray> QQmlPropertyCache::signalParameterNames(int index) const
{
    QQmlPropertyData *signalData = signal(index);
    if (signalData && signalData->hasArguments()) {
        QQmlPropertyCacheMethodArguments *args = (QQmlPropertyCacheMethodArguments *)signalData->arguments();
        if (args && args->names)
            return *args->names;
        const QMetaMethod &method = QMetaObjectPrivate::signal(firstCppMetaObject(), index);
        return method.parameterNames();
    }
    return QList<QByteArray>();
}

QT_END_NAMESPACE
