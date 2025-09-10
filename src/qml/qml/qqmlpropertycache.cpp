// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qqmlpropertycache_p.h"

#include <private/qqmlengine_p.h>
#include <private/qqmlbinding_p.h>
#include <private/qqmlvmemetaobject_p.h>

#include <private/qmetaobject_p.h>
#include <private/qmetaobjectbuilder_p.h>
#include <private/qqmlpropertycachemethodarguments_p.h>
#include <private/qqmlsignalnames_p.h>

#include <private/qv4value_p.h>

#include <QtCore/qdebug.h>
#include <QtCore/QCryptographicHash>
#include <QtCore/private/qtools_p.h>

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
        flags.setType(QQmlPropertyData::Flags::EnumType);
    } else if (metaType.flags() & QMetaType::PointerToQObject) {
        flags.setType(QQmlPropertyData::Flags::QObjectDerivedType);
    } else if (propType == QMetaType::QVariant) {
        flags.setType(QQmlPropertyData::Flags::QVariantType);
    } else if (metaType.flags() & QMetaType::IsQmlList) {
        flags.setType(QQmlPropertyData::Flags::QListType);
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
    m_flags.setType(Flags::FunctionType);

    // We need to set the constructor, signal, constant, arguments, V4Function, cloned flags.
    // These are specific to methods and change with each method.
    // The same QQmlPropertyData may be loaded with multiple methods in sequence.

    switch (m.methodType()) {
    case QMetaMethod::Signal:
        m_flags.setIsSignal(true);
        m_flags.setIsConstructor(false);
        setPropType(m.returnMetaType());
        break;
    case QMetaMethod::Constructor:
        m_flags.setIsSignal(false);
        m_flags.setIsConstructor(true);
        break;
    default:
        m_flags.setIsSignal(false);
        m_flags.setIsConstructor(false);
        setPropType(m.returnMetaType());
        break;
    }

    m_flags.setIsConstant(m.isConst());

    const int paramCount = m.parameterCount();
    if (paramCount) {
        m_flags.setHasArguments(true);
        m_flags.setIsV4Function(
                paramCount == 1 &&
                m.parameterMetaType(0) == QMetaType::fromType<QQmlV4FunctionPtr>());
    } else {
        m_flags.setHasArguments(false);
        m_flags.setIsV4Function(false);
    }

    m_flags.setIsCloned(m.attributes() & QMetaMethod::Cloned);

    Q_ASSERT(m.revision() <= std::numeric_limits<quint16>::max());
    setRevision(QTypeRevision::fromEncodedVersion(m.revision()));
}

/*!
    Creates a standalone QQmlPropertyCache of \a metaObject. It is separate from the usual
    QQmlPropertyCache hierarchy. It's parent is not equal to any other QQmlPropertyCache
    created from QObject::staticMetaObject, for example.
*/
QQmlPropertyCache::Ptr QQmlPropertyCache::createStandalone(
        const QMetaObject *metaObject, QTypeRevision metaObjectRevision)
{
    Q_ASSERT(metaObject);

    Ptr result;
    if (const QMetaObject *super = metaObject->superClass()) {
        result = createStandalone(
                    super, metaObjectRevision)->copyAndAppend(metaObject, metaObjectRevision);
    } else {
        result.adopt(new QQmlPropertyCache(metaObject));
        result->update(metaObject);
    }

    if (metaObjectRevision.isValid() && metaObjectRevision != QTypeRevision::zero()) {
        // Set the revision of the meta object that this cache describes to be
        // 'metaObjectRevision'. This is useful when constructing a property cache
        // from a type that was created directly in C++, and not through QML. For such
        // types, the revision for each recorded QMetaObject would normally be zero, which
        // would exclude any revisioned properties.
        for (int metaObjectOffset = 0; metaObjectOffset < result->allowedRevisionCache.size();
             ++metaObjectOffset) {
            result->allowedRevisionCache[metaObjectOffset] = metaObjectRevision;
        }
    }

    return result;
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
}

QQmlPropertyCache::Ptr QQmlPropertyCache::copy(const QQmlMetaObjectPointer &mo, int reserve) const
{
    QQmlPropertyCache::Ptr cache = QQmlPropertyCache::Ptr(
            new QQmlPropertyCache(mo), QQmlPropertyCache::Ptr::Adopt);
    cache->_parent.reset(this);
    cache->propertyIndexCacheStart = propertyIndexCache.size() + propertyIndexCacheStart;
    cache->methodIndexCacheStart = methodIndexCache.size() + methodIndexCacheStart;
    cache->signalHandlerIndexCacheStart = signalHandlerIndexCache.size() + signalHandlerIndexCacheStart;
    cache->stringCache.linkAndReserve(stringCache, reserve);
    cache->allowedRevisionCache = allowedRevisionCache;
    cache->_defaultPropertyName = _defaultPropertyName;
    cache->_listPropertyAssignBehavior = _listPropertyAssignBehavior;

    return cache;
}

QQmlPropertyCache::Ptr QQmlPropertyCache::copy() const
{
    return copy(_metaObject, 0);
}

QQmlPropertyCache::Ptr QQmlPropertyCache::copyAndReserve(
        int propertyCount, int methodCount, int signalCount, int enumCount) const
{
    QQmlPropertyCache::Ptr rv = copy(
                QQmlMetaObjectPointer(), propertyCount + methodCount + signalCount);
    rv->propertyIndexCache.reserve(propertyCount);
    rv->methodIndexCache.reserve(methodCount);
    rv->signalHandlerIndexCache.reserve(signalCount);
    rv->enumCache.reserve(enumCount);
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

    int index = propertyIndexCache.size();
    propertyIndexCache.append(data);

    setNamedProperty(name, index + propertyOffset(), propertyIndexCache.data() + index);
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
        const auto argumentCount = names.size();
        QQmlPropertyCacheMethodArguments *args = createArgumentsObject(argumentCount, names);
        new (args->types) QMetaType; // Invalid return type
        ::memcpy(args->types + 1, types, argumentCount * sizeof(QMetaType));
        data.setArguments(args);
    }

    const OverrideResult overrideResult = handleOverride(name, &data);
    if (overrideResult == InvalidOverride)
        return;

    int methodIndex = methodIndexCache.size();
    methodIndexCache.append(data);

    int signalHandlerIndex = signalHandlerIndexCache.size();
    signalHandlerIndexCache.append(handler);

    const QString handlerName = QQmlSignalNames::signalNameToHandlerName(name);

    setNamedProperty(name, methodIndex + methodOffset(), methodIndexCache.data() + methodIndex);
    setNamedProperty(handlerName, signalHandlerIndex + signalOffset(),
                     signalHandlerIndexCache.data() + signalHandlerIndex);
}

void QQmlPropertyCache::appendMethod(const QString &name, QQmlPropertyData::Flags flags,
                                     int coreIndex, QMetaType returnType,
                                     const QList<QByteArray> &names,
                                     const QVector<QMetaType> &parameterTypes)
{
    int argumentCount = names.size();

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

    int methodIndex = methodIndexCache.size();
    methodIndexCache.append(data);

    setNamedProperty(name, methodIndex + methodOffset(), methodIndexCache.data() + methodIndex);
}

void QQmlPropertyCache::appendEnum(const QString &name, const QVector<QQmlEnumValue> &values)
{
    QQmlEnumData data;
    data.name = name;
    data.values = values;
    enumCache.append(data);
}

// Returns this property cache's metaObject, creating it if necessary.
const QMetaObject *QQmlPropertyCache::createMetaObject() const
{
    if (_metaObject.isNull()) {
        QMetaObjectBuilder builder;
        toMetaObjectBuilder(builder);
        builder.setSuperClass(_parent->createMetaObject());
        _metaObject.setSharedOnce(builder.toMetaObject());
    }

    return _metaObject.metaObject();
}

const QQmlPropertyData *QQmlPropertyCache::maybeUnresolvedProperty(int index) const
{
    if (index < 0 || index >= propertyCount())
        return nullptr;

    const QQmlPropertyData *rv = nullptr;
    if (index < propertyIndexCacheStart)
        return _parent->maybeUnresolvedProperty(index);
    else
        rv = const_cast<const QQmlPropertyData *>(&propertyIndexCache.at(index - propertyIndexCacheStart));
    return rv;
}

const QQmlPropertyData *QQmlPropertyCache::defaultProperty() const
{
    return property(defaultPropertyName(), nullptr, nullptr);
}

void QQmlPropertyCache::setParent(QQmlPropertyCache::ConstPtr newParent)
{
    if (_parent != newParent)
        _parent = std::move(newParent);
}

QQmlPropertyCache::Ptr
QQmlPropertyCache::copyAndAppend(const QMetaObject *metaObject,
                                 QTypeRevision typeVersion,
                                 QQmlPropertyData::Flags propertyFlags,
                                 QQmlPropertyData::Flags methodFlags,
                                 QQmlPropertyData::Flags signalFlags) const
{
    Q_ASSERT(QMetaObjectPrivate::get(metaObject)->revision >= 4);

    // Reserve enough space in the name hash for all the methods (including signals), all the
    // signal handlers and all the properties.  This assumes no name clashes, but this is the
    // common case.
    QQmlPropertyCache::Ptr rv = copy(
                metaObject,
                QMetaObjectPrivate::get(metaObject)->methodCount
                + QMetaObjectPrivate::get(metaObject)->signalCount
                + QMetaObjectPrivate::get(metaObject)->propertyCount);

    rv->append(metaObject, typeVersion, propertyFlags, methodFlags, signalFlags);

    return rv;
}

static QHashedString signalNameToHandlerName(const QHashedString &methodName)
{
    return QQmlSignalNames::signalNameToHandlerName(methodName);
}

static QHashedString signalNameToHandlerName(const QHashedCStringRef &methodName)
{
    return QQmlSignalNames::signalNameToHandlerName(
            QLatin1StringView{ methodName.constData(), methodName.length() });
}


void QQmlPropertyCache::append(const QMetaObject *metaObject,
                               QTypeRevision typeVersion,
                               QQmlPropertyData::Flags propertyFlags,
                               QQmlPropertyData::Flags methodFlags,
                               QQmlPropertyData::Flags signalFlags)
{
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
            } else if (0 == qstrcmp(name, "QML.ListPropertyAssignBehavior")) {
                _listPropertyAssignBehavior = mci.value();
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

        Q_ASSERT((allowedRevisionCache.size() - 1) < Q_INT16_MAX);
        data->setMetaObjectOffset(allowedRevisionCache.size() - 1);

        if (data->isSignal()) {
            sigdata = &signalHandlerIndexCache[signalHandlerIndex - signalHandlerIndexCacheStart];
            *sigdata = *data;
            sigdata->m_flags.setIsSignalHandler(true);
        }

        QQmlPropertyData *old = nullptr;

        const auto doSetNamedProperty = [&](const auto &methodName) {
            if (StringCache::mapped_type *it = stringCache.value(methodName)) {
                if (handleOverride(methodName, data, (old = it->second)) == InvalidOverride)
                    return;
            }

            setNamedProperty(methodName, ii, data);

            if (data->isSignal()) {

                // TODO: Remove this once we can. Signals should not be overridable.
                if (!utf8)
                    data->m_flags.setIsOverridableSignal(true);

                setNamedProperty(signalNameToHandlerName(methodName), ii, sigdata);
                ++signalHandlerIndex;
            }
        };

        if (utf8)
            doSetNamedProperty(QHashedString(QString::fromUtf8(rawName, cptr - rawName)));
        else
            doSetNamedProperty(QHashedCStringRef(rawName, cptr - rawName));
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

        Q_ASSERT((allowedRevisionCache.size() - 1) < Q_INT16_MAX);
        data->setMetaObjectOffset(allowedRevisionCache.size() - 1);

        QQmlPropertyData *old = nullptr;

        if (utf8) {
            QHashedString propName(QString::fromUtf8(str, cptr - str));
            if (StringCache::mapped_type *it = stringCache.value(propName)) {
                if (handleOverride(propName, data, (old = it->second)) == InvalidOverride)
                    continue;
            }
            setNamedProperty(propName, ii, data);
        } else {
            QHashedCStringRef propName(str, cptr - str);
            if (StringCache::mapped_type *it = stringCache.value(propName)) {
                if (handleOverride(propName, data, (old = it->second)) == InvalidOverride)
                    continue;
            }
            setNamedProperty(propName, ii, data);
        }

        bool isGadget = true;
        for (const QMetaObject *it = metaObject; it != nullptr; it = it->superClass()) {
            if (it == &QObject::staticMetaObject)
                isGadget = false;
        }

        // otherwise always dispatch over a 'normal' meta-call so the QQmlValueType can intercept
        if (!isGadget)
            data->trySetStaticMetaCallFunction(metaObject->d.static_metacall, ii - propOffset);
    }
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

    if (metaObject)
        append(metaObject, QTypeRevision());
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

    argumentsCache = nullptr;

    int pc = metaObject->propertyCount();
    int mc = metaObject->methodCount();
    int sc = metaObjectSignalCount(metaObject);
    int reserve = pc + mc + sc;

    if (parent()) {
        propertyIndexCacheStart = parent()->propertyIndexCache.size() + parent()->propertyIndexCacheStart;
        methodIndexCacheStart = parent()->methodIndexCache.size() + parent()->methodIndexCacheStart;
        signalHandlerIndexCacheStart = parent()->signalHandlerIndexCache.size() + parent()->signalHandlerIndexCacheStart;
        stringCache.linkAndReserve(parent()->stringCache, reserve);
        append(metaObject, QTypeRevision());
    } else {
        propertyIndexCacheStart = 0;
        methodIndexCacheStart = 0;
        signalHandlerIndexCacheStart = 0;
        update(metaObject);
    }
}

const QQmlPropertyData *QQmlPropertyCache::findProperty(
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
    const QQmlRefPointer<QQmlContextData> parent = context->parent();
    return (!parent || !parent->imports());
}

inline int maximumIndexForProperty(const QQmlPropertyData *prop, const int methodCount, const int signalCount, const int propertyCount)
{
    return prop->isFunction() ? methodCount
                              : prop->isSignalHandler() ? signalCount
                                                        : propertyCount;
}

}

const QQmlPropertyData *QQmlPropertyCache::findProperty(
        StringCache::ConstIterator it, const QQmlVMEMetaObject *vmemo,
        const QQmlRefPointer<QQmlContextData> &context) const
{
    StringCache::ConstIterator end = stringCache.end();

    if (it != end) {
        const QQmlPropertyData *result = it.value().second;

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

    const qsizetype count = parameterNameList.size();
    if (count > std::numeric_limits<quint16>::max())
        *errorString = QCoreApplication::translate("QQmlRewrite", "Signal has an excessive number of parameters: %1").arg(count);

    for (qsizetype i = 0; i < count; ++i) {
        if (i > 0)
            parameters += QLatin1Char(',');
        const QByteArray &param = parameterNameList.at(i);
        if (param.isEmpty()) {
            unnamedParameter = true;
        } else if (unnamedParameter) {
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

int QQmlPropertyCache::originalClone(int index) const
{
    while (signal(index)->isCloned())
        --index;
    return index;
}

int QQmlPropertyCache::originalClone(const QObject *object, int index)
{
    QQmlData *data = QQmlData::get(object);
    if (data && data->propertyCache) {
        const QQmlPropertyCache *cache = data->propertyCache.data();
        const QQmlPropertyData *sig = cache->signal(index);
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
const QQmlPropertyData *
qQmlPropertyCacheProperty(QObject *obj, T name, const QQmlRefPointer<QQmlContextData> &context,
                          QQmlPropertyData *local)
{
    const QQmlPropertyCache *cache = nullptr;

    QQmlData *ddata = QQmlData::get(obj, false);

    if (ddata && ddata->propertyCache) {
        cache = ddata->propertyCache.data();
    } else if (auto newCache = QQmlMetaType::propertyCache(obj)) {
        cache = newCache.data();
        ddata = QQmlData::get(obj, true);
        ddata->propertyCache = std::move(newCache);
    }

    const QQmlPropertyData *rv = nullptr;

    if (cache) {
        rv = cache->property(name, obj, context);
    } else if (local) {
        *local = qQmlPropertyCacheCreate(obj->metaObject(), qQmlPropertyCacheToString(name));
        if (local->isValid())
            rv = local;
    }

    return rv;
}

const QQmlPropertyData *QQmlPropertyCache::property(
        QObject *obj, const QV4::String *name, const QQmlRefPointer<QQmlContextData> &context,
        QQmlPropertyData *local)
{
    return qQmlPropertyCacheProperty<const QV4::String *>(obj, name, context, local);
}

const QQmlPropertyData *QQmlPropertyCache::property(
        QObject *obj, QStringView name, const QQmlRefPointer<QQmlContextData> &context,
        QQmlPropertyData *local)
{
    return qQmlPropertyCacheProperty<const QStringView &>(obj, name, context, local);
}

const QQmlPropertyData *QQmlPropertyCache::property(
        QObject *obj, const QLatin1String &name, const QQmlRefPointer<QQmlContextData> &context,
        QQmlPropertyData *local)
{
    return qQmlPropertyCacheProperty<const QLatin1String &>(obj, name, context, local);
}

// this function is copied from qmetaobject.cpp
static inline const QByteArray stringData(const QMetaObject *mo, int index)
{
    uint offset = mo->d.stringdata[2*index];
    uint length = mo->d.stringdata[2*index + 1];
    const char *string = reinterpret_cast<const char *>(mo->d.stringdata) + offset;
    return QByteArray::fromRawData(string, length);
}

const char *QQmlPropertyCache::className() const
{
    if (const QMetaObject *mo = _metaObject.metaObject())
        return mo->className();
    else
        return _dynamicClassName.constData();
}

void QQmlPropertyCache::toMetaObjectBuilder(QMetaObjectBuilder &builder) const
{
    struct Sort { static bool lt(const QPair<QString, const QQmlPropertyData *> &lhs,
                                 const QPair<QString, const QQmlPropertyData *> &rhs) {
        return lhs.second->coreIndex() < rhs.second->coreIndex();
    } };

    struct Insert { static void in(const QQmlPropertyCache *This,
                                   QList<QPair<QString, const QQmlPropertyData *> > &properties,
                                   QList<QPair<QString, const QQmlPropertyData *> > &methods,
                                   StringCache::ConstIterator iter, const QQmlPropertyData *data) {
        if (data->isSignalHandler())
            return;

        if (data->isFunction()) {
            if (data->coreIndex() < This->methodIndexCacheStart)
                return;

            QPair<QString, const QQmlPropertyData *> entry = qMakePair((QString)iter.key(), data);
            // Overrides can cause the entry to already exist
            if (!methods.contains(entry)) methods.append(entry);

            data = This->overrideData(data);
            if (data && !data->isFunction()) Insert::in(This, properties, methods, iter, data);
        } else {
            if (data->coreIndex() < This->propertyIndexCacheStart)
                return;

            QPair<QString, const QQmlPropertyData *> entry = qMakePair((QString)iter.key(), data);
            // Overrides can cause the entry to already exist
            if (!properties.contains(entry)) properties.append(entry);

            data = This->overrideData(data);
            if (data) Insert::in(This, properties, methods, iter, data);
        }

    } };

    builder.setClassName(_dynamicClassName);

    QList<QPair<QString, const QQmlPropertyData *> > properties;
    QList<QPair<QString, const QQmlPropertyData *> > methods;

    for (StringCache::ConstIterator iter = stringCache.begin(), cend = stringCache.end(); iter != cend; ++iter)
        Insert::in(this, properties, methods, iter, iter.value().second);

    Q_ASSERT(properties.size() == propertyIndexCache.size());
    Q_ASSERT(methods.size() == methodIndexCache.size());

    std::sort(properties.begin(), properties.end(), Sort::lt);
    std::sort(methods.begin(), methods.end(), Sort::lt);

    for (int ii = 0; ii < properties.size(); ++ii) {
        const QQmlPropertyData *data = properties.at(ii).second;

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
        property.setBindable(data->notifiesViaBindable());
        property.setAlias(data->isAlias());
    }

    for (int ii = 0; ii < methods.size(); ++ii) {
        const QQmlPropertyData *data = methods.at(ii).second;

        QByteArray returnType;
        if (data->propType().isValid())
            returnType = data->propType().name();

        QByteArray signature;
        // '+=' reserves extra capacity. Follow-up appending will be probably free.
        signature += methods.at(ii).first.toUtf8() + '(';

        QQmlPropertyCacheMethodArguments *arguments = nullptr;
        if (data->hasArguments()) {
            arguments = data->arguments();
            for (int ii = 0, end = arguments->names ? arguments->names->size() : 0;
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

    for (int ii = 0; ii < enumCache.size(); ++ii) {
        const QQmlEnumData &enumData = enumCache.at(ii);
        QMetaEnumBuilder enumeration = builder.addEnumerator(enumData.name.toUtf8());
        enumeration.setIsScoped(true);
        for (int jj = 0; jj < enumData.values.size(); ++jj) {
            const QQmlEnumValue &value = enumData.values.at(jj);
            enumeration.addKey(value.namedValue.toUtf8(), value.value);
        }
    }

    if (!_defaultPropertyName.isEmpty()) {
        const QQmlPropertyData *dp = property(_defaultPropertyName, nullptr, nullptr);
        if (dp && dp->coreIndex() >= propertyIndexCacheStart) {
            Q_ASSERT(!dp->isFunction());
            builder.addClassInfo("DefaultProperty", _defaultPropertyName.toUtf8());
        }
    }

    if (!_listPropertyAssignBehavior.isEmpty())
        builder.addClassInfo("QML.ListPropertyAssignBehavior", _listPropertyAssignBehavior);
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

static_assert(QMetaObjectPrivate::OutputRevision == 12, "Check and adjust determineMetaObjectSizes");

bool QQmlPropertyCache::determineMetaObjectSizes(const QMetaObject &mo, int *fieldCount,
                                                 int *stringCount)
{
    const QMetaObjectPrivate *priv = reinterpret_cast<const QMetaObjectPrivate*>(mo.d.data);
    if (priv->revision != QMetaObjectPrivate::OutputRevision)
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

    hash.addData({reinterpret_cast<const char *>(mo.d.data), qsizetype(fieldCount * sizeof(uint))});
    for (int i = 0; i < stringCount; ++i) {
        hash.addData(stringData(&mo, i));
    }

    return true;
}

QByteArray QQmlPropertyCache::checksum(QHash<quintptr, QByteArray> *checksums, bool *ok) const
{
    auto it = checksums->constFind(quintptr(this));
    if (it != checksums->constEnd()) {
        *ok = true;
        return *it;
    }

    // Generate a checksum on the meta-object data only on C++ types.
    if (_metaObject.isShared()) {
        *ok = false;
        return QByteArray();
    }

    QCryptographicHash hash(QCryptographicHash::Md5);

    if (_parent) {
        hash.addData(_parent->checksum(checksums, ok));
        if (!*ok)
            return QByteArray();
    }

    if (!addToHash(hash, *_metaObject.metaObject())) {
        *ok = false;
        return QByteArray();
    }

    const QByteArray result = hash.result();
    if (result.isEmpty()) {
        *ok = false;
    } else {
        *ok = true;
        checksums->insert(quintptr(this), result);
    }
    return result;
}

/*! \internal
    \a index MUST be in the signal index range (see QObjectPrivate::signalIndex()).
    This is different from QMetaMethod::methodIndex().
*/
QList<QByteArray> QQmlPropertyCache::signalParameterNames(int index) const
{
    const QQmlPropertyData *signalData = signal(index);
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
