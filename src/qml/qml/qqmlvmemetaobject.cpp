// Copyright (C) 2016 The Qt Company Ltd.
// Copyright (C) 2016 BasysKom GmbH.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qqmlvmemetaobject_p.h"

#include <private/qqmlrefcount_p.h>
#include "qqmlpropertyvalueinterceptor_p.h"
#include <qqmlinfo.h>

#include <private/qqmlglobal_p.h>

#include <private/qv4object_p.h>
#include <private/qv4variantobject_p.h>
#include <private/qv4functionobject_p.h>
#include <private/qv4scopedvalue_p.h>
#include <private/qv4jscall_p.h>
#include <private/qv4qobjectwrapper_p.h>
#include <private/qv4sequenceobject_p.h>
#include <private/qqmlpropertycachecreator_p.h>
#include <private/qqmlpropertycachemethodarguments_p.h>
#include <private/qqmlvaluetypewrapper_p.h>

#include <climits> // for CHAR_BIT

QT_BEGIN_NAMESPACE

QQmlVMEResolvedList::QQmlVMEResolvedList(QQmlListProperty<QObject> *prop)
{
    // see QQmlVMEMetaObject::metaCall for how this was constructed
    auto encodedIndex = quintptr(prop->data);
    constexpr quintptr usableBits = sizeof(quintptr) * CHAR_BIT;
    quintptr inheritanceDepth = encodedIndex >> (usableBits / 2);
    m_id = encodedIndex & ((quintptr(1) << (usableBits / 2)) - 1);

    // walk up to the correct meta object if necessary
    auto mo = static_cast<QQmlVMEMetaObject *>(QObjectPrivate::get(prop->object)->metaObject);
    while (inheritanceDepth--)
        mo = mo->parentVMEMetaObject();
    m_metaObject = mo;
    Q_ASSERT(m_metaObject);
    Q_ASSERT(::strstr(m_metaObject->toDynamicMetaObject(prop->object)
                              ->property(m_metaObject->propOffset() + m_id)
                              .typeName(),
                      "QQmlListProperty"));
    Q_ASSERT(m_metaObject->object == prop->object);

    // readPropertyAsList() with checks transformed into Q_ASSERT
    // and without allocation.
    if (m_metaObject->propertyAndMethodStorage.isUndefined()
        && m_metaObject->propertyAndMethodStorage.valueRef()) {
        return;
    }

    if (auto *md = static_cast<QV4::MemberData *>(
                m_metaObject->propertyAndMethodStorage.asManaged())) {
        const QV4::Value *v = md->data() + m_id;
        Q_ASSERT(v->as<QV4::Object>());
        m_list = static_cast<QV4::Heap::Object *>(v->heapObject());
        Q_ASSERT(m_list);
    }
}

void QQmlVMEResolvedList::append(QObject *o) const
{
    QV4::Scope scope(m_list->internalClass->engine);
    QV4::Heap::ArrayData *arrayData = m_list->arrayData;

    const uint length = arrayData->length();
    if (Q_UNLIKELY(length == std::numeric_limits<uint>::max())) {
        scope.engine->throwRangeError(QLatin1String("Too many elements."));
        return;
    }

    QV4::ScopedObject object(scope, m_list);
    QV4::ArrayData::realloc(object, QV4::Heap::ArrayData::Simple, length + 1, false);
    arrayData->vtable()->put(
            object, length, QV4::QObjectWrapper::wrap(scope.engine, o));
}

QObject *QQmlVMEResolvedList::at(qsizetype i) const
{
    QV4::Scope scope(m_list->internalClass->engine);
    QV4::Scoped<QV4::QObjectWrapper> result(scope, m_list->arrayData->get(i));
    return result ? result->object() : nullptr;
}

void QQmlVMEResolvedList::replace(qsizetype i, QObject *o) const
{
    QV4::Scope scope(m_list->internalClass->engine);
    QV4::ScopedObject object(scope, m_list);
    m_list->arrayData->vtable()->put(object, i, QV4::QObjectWrapper::wrap(scope.engine, o));
}

QQmlVMEResolvedList::~QQmlVMEResolvedList() = default;

void QQmlVMEResolvedList::activateSignal() const
{
    m_metaObject->activate(m_metaObject->object, int(m_id + m_metaObject->methodOffset()), nullptr);
}

void QQmlVMEMetaObject::list_append(QQmlListProperty<QObject> *prop, QObject *o)
{
    const QQmlVMEResolvedList resolved(prop);
    resolved.append(o);
    resolved.activateSignal();
}

void QQmlVMEMetaObject::list_append_nosignal(QQmlListProperty<QObject> *prop, QObject *o)
{
    QQmlVMEResolvedList(prop).append(o);
}

static qsizetype list_count(QQmlListProperty<QObject> *prop)
{
    return QQmlVMEResolvedList(prop).size();
}

static QObject *list_at(QQmlListProperty<QObject> *prop, qsizetype index)
{
    return QQmlVMEResolvedList(prop).at(index);
}

void QQmlVMEMetaObject::list_clear(QQmlListProperty<QObject> *prop)
{
    const QQmlVMEResolvedList resolved(prop);
    resolved.clear();
    resolved.activateSignal();
}

void QQmlVMEMetaObject::list_clear_nosignal(QQmlListProperty<QObject> *prop)
{
    QQmlVMEResolvedList(prop).clear();
}

static void list_replace(QQmlListProperty<QObject> *prop, qsizetype index, QObject *o)
{
    const QQmlVMEResolvedList resolved(prop);
    resolved.replace(index, o);
    resolved.activateSignal();
}

static void list_removeLast(QQmlListProperty<QObject> *prop)
{
    const QQmlVMEResolvedList resolved(prop);
    resolved.removeLast();
    resolved.activateSignal();
}

QQmlVMEVariantQObjectPtr::QQmlVMEVariantQObjectPtr()
    : QQmlGuard<QObject>(QQmlVMEVariantQObjectPtr::objectDestroyedImpl, nullptr), m_target(nullptr), m_index(-1)
{
}

void QQmlVMEVariantQObjectPtr::objectDestroyedImpl(QQmlGuardImpl *guard)
{
    auto This = static_cast<QQmlVMEVariantQObjectPtr *>(guard);
    if (!This->m_target || QQmlData::wasDeleted(This->m_target->object))
        return;

    if (This->m_index >= 0) {
        QV4::ExecutionEngine *v4 = This->m_target->propertyAndMethodStorage.engine();
        if (v4) {
            QV4::Scope scope(v4);
            QV4::Scoped<QV4::MemberData> sp(scope, This->m_target->propertyAndMethodStorage.value());
            if (sp) {
                QV4::PropertyIndex index{ sp->d(), sp->d()->values.values + This->m_index };
                index.set(v4, QV4::Value::nullValue());
            }
        }

        This->m_target->activate(This->m_target->object, This->m_target->methodOffset() + This->m_index, nullptr);
    }
}

void QQmlVMEVariantQObjectPtr::setGuardedValue(QObject *obj, QQmlVMEMetaObject *target, int index)
{
    m_target = target;
    m_index = index;
    setObject(obj);
}

class QQmlVMEMetaObjectEndpoint : public QQmlNotifierEndpoint
{
public:
    QQmlVMEMetaObjectEndpoint();
    void tryConnect();

    enum Tag {
        NoTag,
        EndPointIsConnected
    };

    QTaggedPointer<QQmlVMEMetaObject, Tag> metaObject;
};

QQmlVMEMetaObjectEndpoint::QQmlVMEMetaObjectEndpoint()
    : QQmlNotifierEndpoint(QQmlNotifierEndpoint::QQmlVMEMetaObjectEndpoint)
{
}

void QQmlVMEMetaObjectEndpoint_callback(QQmlNotifierEndpoint *e, void **)
{
    QQmlVMEMetaObjectEndpoint *vmee = static_cast<QQmlVMEMetaObjectEndpoint*>(e);
    vmee->tryConnect();
}

void QQmlVMEMetaObjectEndpoint::tryConnect()
{
    Q_ASSERT(metaObject->compiledObject);
    int aliasId = this - metaObject->aliasEndpoints;

    if (metaObject.tag() == EndPointIsConnected) {
        // This is actually notify
        int sigIdx = metaObject->methodOffset() + aliasId + metaObject->compiledObject->nProperties;
        metaObject->activate(metaObject->object, sigIdx, nullptr);
    } else {
        const QV4::CompiledData::Alias *aliasData = &metaObject->compiledObject->aliasTable()[aliasId];
        if (!aliasData->isObjectAlias()) {
            QQmlRefPointer<QQmlContextData> ctxt = metaObject->ctxt;
            QObject *target = ctxt->idValue(aliasData->targetObjectId());
            if (!target)
                return;

            QQmlPropertyIndex encodedIndex = QQmlPropertyIndex::fromEncoded(aliasData->encodedMetaPropertyIndex);
            int coreIndex = encodedIndex.coreIndex();
            int valueTypeIndex = encodedIndex.valueTypeIndex();
            const QQmlPropertyData *pd = QQmlData::ensurePropertyCache(target)->property(coreIndex);
            if (pd && valueTypeIndex != -1 && !QQmlMetaType::valueType(pd->propType())) {
                // deep alias
                const QQmlPropertyCache::ConstPtr newPropertyCache
                        = QQmlMetaType::propertyCacheForType(pd->propType());
                void *argv[1] = { &target };
                QMetaObject::metacall(target, QMetaObject::ReadProperty, coreIndex, argv);
                Q_ASSERT(newPropertyCache);
                pd = newPropertyCache->property(valueTypeIndex);
            }
            if (!pd)
                return;

            if (pd->notifyIndex() != -1 && ctxt->engine())
                connect(target, pd->notifyIndex(), ctxt->engine());
        }

        metaObject.setTag(EndPointIsConnected);
    }
}


QQmlInterceptorMetaObject::QQmlInterceptorMetaObject(QObject *obj, const QQmlPropertyCache::ConstPtr &cache)
    : object(obj),
      cache(cache)
{
    QObjectPrivate *op = QObjectPrivate::get(obj);

    if (op->metaObject) {
        parent = op->metaObject;
        // Use the extra flag in QBiPointer to know if we can safely cast parent.asT1() to QQmlVMEMetaObject*
        parent.setFlagValue(QQmlData::get(obj)->hasVMEMetaObject);
    } else {
        parent = obj->metaObject();
    }

    op->metaObject = this;
    QQmlData::get(obj)->hasInterceptorMetaObject = true;
}

QQmlInterceptorMetaObject::~QQmlInterceptorMetaObject()
{

}

void QQmlInterceptorMetaObject::registerInterceptor(QQmlPropertyIndex index, QQmlPropertyValueInterceptor *interceptor)
{
    for (QQmlPropertyValueInterceptor *vi = interceptors; vi; vi = vi->m_next) {
        if (Q_UNLIKELY(vi->m_propertyIndex.coreIndex() == index.coreIndex())) {
            qWarning() << "Attempting to set another interceptor on "
                       << object->metaObject()->className() << "property"
                       << object->metaObject()->property(index.coreIndex()).name()
                       << "- unsupported";
        }
    }

    interceptor->m_propertyIndex = index;
    interceptor->m_next = interceptors;
    interceptors = interceptor;
}

int QQmlInterceptorMetaObject::metaCall(QObject *o, QMetaObject::Call c, int id, void **a)
{
    Q_ASSERT(o == object);
    Q_UNUSED(o);

    if (intercept(c, id, a))
        return -1;
    return object->qt_metacall(c, id, a);
}

bool QQmlInterceptorMetaObject::doIntercept(QMetaObject::Call c, int id, void **a)
{
    for (QQmlPropertyValueInterceptor *vi = interceptors; vi; vi = vi->m_next) {
        if (vi->m_propertyIndex.coreIndex() != id)
            continue;

        const int valueIndex = vi->m_propertyIndex.valueTypeIndex();
        const QQmlData *data = QQmlData::get(object);
        const QMetaType metaType = data->propertyCache->property(id)->propType();

        if (metaType.isValid()) {
            if (valueIndex != -1 && c == QMetaObject::WriteProperty) {

                // If we didn't intend to change the property this interceptor cares about,
                // then don't bother intercepting it. There may be an animation running on
                // the property. We shouldn't disturb it.
                const int changedProperty
                        = (*static_cast<int *>(a[3]) & QQmlPropertyData::HasInternalIndex)
                            ? *static_cast<int *>(a[4])
                            : QV4::ReferenceObject::AllProperties;
                if (changedProperty == QV4::ReferenceObject::AllProperties
                        || changedProperty == valueIndex) {
                    // TODO: handle intercepting bindable properties for value types?
                    QQmlGadgetPtrWrapper *valueType = QQmlGadgetPtrWrapper::instance(
                                data->context->engine(), metaType);
                    Q_ASSERT(valueType);

                    //
                    // Consider the following case:
                    //  color c = { 0.1, 0.2, 0.3 }
                    //  interceptor exists on c.r
                    //  write { 0.2, 0.4, 0.6 }
                    //
                    // The interceptor may choose not to update the r component at this
                    // point (for example, a behavior that creates an animation). But we
                    // need to ensure that the g and b components are updated correctly.
                    //
                    // So we need to perform a full write where the value type is:
                    //    r = old value, g = new value, b = new value
                    //
                    // And then call the interceptor which may or may not write the
                    // new value to the r component.
                    //
                    // This will ensure that the other components don't contain stale data
                    // and any relevant signals are emitted.
                    //
                    // To achieve this:
                    //   (1) Store the new value type as a whole (needed due to
                    //       aliasing between a[0] and static storage in value type).
                    //   (2) Read the entire existing value type from object -> valueType temp.
                    //   (3) Read the previous value of the component being changed
                    //       from the valueType temp.
                    //   (4) Write the entire new value type into the temp.
                    //   (5) Overwrite the component being changed with the old value.
                    //   (6) Perform a full write to the value type (which may emit signals etc).
                    //   (7) Issue the interceptor call with the new component value.
                    //

                    QMetaProperty valueProp = valueType->property(valueIndex);
                    QVariant newValue(metaType, a[0]);

                    valueType->read(object, id);
                    QVariant prevComponentValue = valueType->readOnGadget(valueProp);

                    valueType->setValue(newValue);
                    QVariant newComponentValue = valueType->readOnGadget(valueProp);

                    // If the intercepted value seemingly has not changed, we still need to
                    // invoke the interceptor. There may be a pending animation that will
                    // change the value soon. Such an animation needs to be canceled if the
                    // current value is explicitly set.
                    // So, we cannot return here if prevComponentValue == newComponentValue.
                    valueType->writeOnGadget(valueProp, std::move(prevComponentValue));
                    valueType->write(object, id, QQmlPropertyData::DontRemoveBinding | QQmlPropertyData::BypassInterceptor);

                    vi->write(newComponentValue);
                    return true;
                }
            } else if (c == QMetaObject::WriteProperty) {
                vi->write(QVariant(metaType, a[0]));
                return true;
            } else {
                object->qt_metacall(c, id, a);
                QUntypedBindable target = *reinterpret_cast<QUntypedBindable *>(a[0]);
                return vi->bindable(reinterpret_cast<QUntypedBindable *>(a[0]), target);
            }
        }
    }

    return false;
}

static QMetaObject *stringCastMetaObject(QObject *o, const QMetaObject *top)
{
    for (const QMetaObject *mo = top; mo; mo = mo->superClass()) {
        if (o->qt_metacast(mo->className()) != nullptr)
            return const_cast<QMetaObject *>(mo);
    }
    return nullptr;
}

QMetaObject *QQmlInterceptorMetaObject::toDynamicMetaObject(QObject *o)
{
    if (!metaObject)
        metaObject = cache->createMetaObject();

    if (Q_UNLIKELY(metaObject.tag() == MetaObjectInvalid))
        return stringCastMetaObject(o, metaObject->superClass());

    // ### Qt7: The const_cast is only due to toDynamicMetaObject having the wrong return type.
    //          It should be const QMetaObject *. Fix this.
    return const_cast<QMetaObject *>(metaObject.data());
}

QQmlVMEMetaObject::QQmlVMEMetaObject(QV4::ExecutionEngine *engine,
                                     QObject *obj,
                                     const QQmlPropertyCache::ConstPtr &cache, const QQmlRefPointer<QV4::ExecutableCompilationUnit> &qmlCompilationUnit, int qmlObjectId)
    : QQmlInterceptorMetaObject(obj, cache),
      engine(engine),
      ctxt(QQmlData::get(obj, true)->outerContext),
      aliasEndpoints(nullptr), compilationUnit(qmlCompilationUnit), compiledObject(nullptr)
{
    Q_ASSERT(engine);
    QQmlData::get(obj)->hasVMEMetaObject = true;

    if (compilationUnit && qmlObjectId >= 0) {
        compiledObject = compilationUnit->objectAt(qmlObjectId);

        if (compiledObject->nProperties || compiledObject->nFunctions) {
            uint size = compiledObject->nProperties + compiledObject->nFunctions;
            if (size) {
                QV4::Heap::MemberData *data = QV4::MemberData::allocate(engine, size);
                propertyAndMethodStorage.set(engine, data);
                std::fill(data->values.values, data->values.values + data->values.size, QV4::Encode::undefined());
            }

            // Need JS wrapper to ensure properties/methods are marked.
            ensureQObjectWrapper();
        }
    }
}

QQmlVMEMetaObject::~QQmlVMEMetaObject()
{
    if (parent.isT1()) parent.asT1()->objectDestroyed(object);
    delete [] aliasEndpoints;

    qDeleteAll(varObjectGuards);
}

QV4::MemberData *QQmlVMEMetaObject::propertyAndMethodStorageAsMemberData() const
{
    if (propertyAndMethodStorage.isUndefined()) {
        if (propertyAndMethodStorage.valueRef())
            // in some situations, the QObject wrapper (and associated data,
            // such as the varProperties array) will have been cleaned up, but the
            // QObject ptr will not yet have been deleted (eg, waiting on deleteLater).
            // In this situation, return 0.
            return nullptr;
    }

    return static_cast<QV4::MemberData*>(propertyAndMethodStorage.asManaged());
}

void QQmlVMEMetaObject::writeProperty(int id, int v)
{
    QV4::MemberData *md = propertyAndMethodStorageAsMemberData();
    if (md)
        md->set(engine, id, QV4::Value::fromInt32(v));
}

void QQmlVMEMetaObject::writeProperty(int id, bool v)
{
    QV4::MemberData *md = propertyAndMethodStorageAsMemberData();
    if (md)
        md->set(engine, id, QV4::Value::fromBoolean(v));
}

void QQmlVMEMetaObject::writeProperty(int id, double v)
{
    QV4::MemberData *md = propertyAndMethodStorageAsMemberData();
    if (md)
        md->set(engine, id, QV4::Value::fromDouble(v));
}

void QQmlVMEMetaObject::writeProperty(int id, const QString& v)
{
    QV4::MemberData *md = propertyAndMethodStorageAsMemberData();
    if (md) {
        QV4::Scope scope(engine);
        QV4::Scoped<QV4::MemberData>(scope, md)->set(engine, id, engine->newString(v));
    }
}

void QQmlVMEMetaObject::writeProperty(int id, QObject* v)
{
    QV4::MemberData *md = propertyAndMethodStorageAsMemberData();
    if (md) {
        QV4::Scope scope(engine);
        QV4::Scoped<QV4::MemberData>(scope, md)->set(engine, id, QV4::Value::fromReturnedValue(
                                                         QV4::QObjectWrapper::wrap(engine, v)));
    }

    QQmlVMEVariantQObjectPtr *guard = getQObjectGuardForProperty(id);
    if (v && !guard) {
        guard = new QQmlVMEVariantQObjectPtr();
        varObjectGuards.append(guard);
    }
    if (guard)
        guard->setGuardedValue(v, this, id);
}

int QQmlVMEMetaObject::readPropertyAsInt(int id) const
{
    QV4::MemberData *md = propertyAndMethodStorageAsMemberData();
    if (!md)
        return 0;

    QV4::Scope scope(engine);
    QV4::ScopedValue sv(scope, *(md->data() + id));
    if (!sv->isInt32())
        return 0;
    return sv->integerValue();
}

bool QQmlVMEMetaObject::readPropertyAsBool(int id) const
{
    QV4::MemberData *md = propertyAndMethodStorageAsMemberData();
    if (!md)
        return false;

    QV4::Scope scope(engine);
    QV4::ScopedValue sv(scope, *(md->data() + id));
    if (!sv->isBoolean())
        return false;
    return sv->booleanValue();
}

double QQmlVMEMetaObject::readPropertyAsDouble(int id) const
{
    QV4::MemberData *md = propertyAndMethodStorageAsMemberData();
    if (!md)
        return 0.0;

    QV4::Scope scope(engine);
    QV4::ScopedValue sv(scope, *(md->data() + id));
    if (!sv->isDouble())
        return 0.0;
    return sv->doubleValue();
}

QString QQmlVMEMetaObject::readPropertyAsString(int id) const
{
    QV4::MemberData *md = propertyAndMethodStorageAsMemberData();
    if (!md)
        return QString();

    QV4::Scope scope(engine);
    QV4::ScopedValue sv(scope, *(md->data() + id));
    if (QV4::String *s = sv->stringValue())
        return s->toQString();
    return QString();
}

QUrl QQmlVMEMetaObject::readPropertyAsUrl(int id) const
{
    QV4::MemberData *md = propertyAndMethodStorageAsMemberData();
    if (!md)
        return QUrl();

    QV4::Scope scope(engine);
    QV4::ScopedValue sv(scope, *(md->data() + id));
    const QV4::VariantObject *v = sv->as<QV4::VariantObject>();
    if (!v || v->d()->data().userType() != QMetaType::QUrl)
        return QUrl();
    return v->d()->data().value<QUrl>();
}

QDate QQmlVMEMetaObject::readPropertyAsDate(int id) const
{
    QV4::MemberData *md = propertyAndMethodStorageAsMemberData();
    if (!md)
        return QDate();

    QV4::Scope scope(engine);
    QV4::ScopedValue sv(scope, *(md->data() + id));
    const QV4::VariantObject *v = sv->as<QV4::VariantObject>();
    if (!v || v->d()->data().userType() != QMetaType::QDate)
        return QDate();
    return v->d()->data().value<QDate>();
}

QTime QQmlVMEMetaObject::readPropertyAsTime(int id) const
{
    QV4::MemberData *md = propertyAndMethodStorageAsMemberData();
    if (!md)
        return QTime();

    QV4::Scope scope(engine);
    QV4::ScopedValue sv(scope, *(md->data() + id));
    const QV4::VariantObject *v = sv->as<QV4::VariantObject>();
    if (!v || v->d()->data().userType() != QMetaType::QTime)
        return QTime();
    return v->d()->data().value<QTime>();
}

QDateTime QQmlVMEMetaObject::readPropertyAsDateTime(int id) const
{
    QV4::MemberData *md = propertyAndMethodStorageAsMemberData();
    if (!md)
        return QDateTime();

    QV4::Scope scope(engine);
    QV4::ScopedValue sv(scope, *(md->data() + id));
    const QV4::VariantObject *v = sv->as<QV4::VariantObject>();
    if (!v || v->d()->data().userType() != QMetaType::QDateTime)
        return QDateTime();
    return v->d()->data().value<QDateTime>();
}

#if QT_CONFIG(regularexpression)
QRegularExpression QQmlVMEMetaObject::readPropertyAsRegularExpression(int id) const
{
    QV4::MemberData *md = propertyAndMethodStorageAsMemberData();
    if (!md)
        return QRegularExpression();

    QV4::Scope scope(engine);
    QV4::ScopedValue sv(scope, *(md->data() + id));
    const QV4::VariantObject *v = sv->as<QV4::VariantObject>();
    if (!v || v->d()->data().userType() != QMetaType::QRegularExpression)
        return QRegularExpression();
    return v->d()->data().value<QRegularExpression>();
}
#endif

QSizeF QQmlVMEMetaObject::readPropertyAsSizeF(int id) const
{
    QV4::MemberData *md = propertyAndMethodStorageAsMemberData();
    if (!md)
        return QSizeF();

    QV4::Scope scope(engine);
    QV4::ScopedValue sv(scope, *(md->data() + id));
    const QV4::VariantObject *v = sv->as<QV4::VariantObject>();
    if (!v || v->d()->data().userType() != QMetaType::QSizeF)
        return QSizeF();
    return v->d()->data().value<QSizeF>();
}

QPointF QQmlVMEMetaObject::readPropertyAsPointF(int id) const
{
    QV4::MemberData *md = propertyAndMethodStorageAsMemberData();
    if (!md)
        return QPointF();

    QV4::Scope scope(engine);
    QV4::ScopedValue sv(scope, *(md->data() + id));
    const QV4::VariantObject *v = sv->as<QV4::VariantObject>();
    if (!v || v->d()->data().userType() != QMetaType::QPointF)
        return QPointF();
    return v->d()->data().value<QPointF>();
}

QObject* QQmlVMEMetaObject::readPropertyAsQObject(int id) const
{
    QV4::MemberData *md = propertyAndMethodStorageAsMemberData();
    if (!md)
        return nullptr;

    QV4::Scope scope(engine);
    QV4::ScopedValue sv(scope, *(md->data() + id));
    const QV4::QObjectWrapper *wrapper = sv->as<QV4::QObjectWrapper>();
    if (!wrapper)
        return nullptr;
    return wrapper->object();
}

void QQmlVMEMetaObject::initPropertyAsList(int id) const
{
    QV4::MemberData *md = propertyAndMethodStorageAsMemberData();
    if (!md)
        return;

    QV4::Scope scope(engine);
    QV4::ScopedObject v(scope, *(md->data() + id));
    if (!v) {
        v = engine->newObject();
        v->arrayCreate();
        md->set(engine, id, v);
    }
}

QRectF QQmlVMEMetaObject::readPropertyAsRectF(int id) const
{
    QV4::MemberData *md = propertyAndMethodStorageAsMemberData();
    if (!md)
        return QRectF();

    QV4::Scope scope(engine);
    QV4::ScopedValue sv(scope, *(md->data() + id));
    const QV4::VariantObject *v = sv->as<QV4::VariantObject>();
    if (!v || v->d()->data().userType() != QMetaType::QRectF)
        return QRectF();
    return v->d()->data().value<QRectF>();
}

int QQmlVMEMetaObject::metaCall(QObject *o, QMetaObject::Call c, int _id, void **a)
{
    Q_ASSERT(o == object);
    Q_UNUSED(o);

    int id = _id;

    if (intercept(c, _id, a))
        return -1;

    const int propertyCount = compiledObject ? int(compiledObject->nProperties) : 0;
    const int aliasCount = compiledObject ? int(compiledObject->nAliases) : 0;
    const int signalCount = compiledObject ? int(compiledObject->nSignals) : 0;
    const int methodCount = compiledObject ? int(compiledObject->nFunctions) : 0;

    if (c == QMetaObject::ReadProperty || c == QMetaObject::WriteProperty || c == QMetaObject::ResetProperty || c == QMetaObject::BindableProperty) {
        if (id >= propOffset()) {
            id -= propOffset();

            if (id < propertyCount) {
                // if we reach this point, propertyCount must have been > 0, and thus compiledObject != nullptr
                Q_ASSERT(compiledObject);
                const QV4::CompiledData::Property &property = compiledObject->propertyTable()[id];
                const QV4::CompiledData::CommonType t = property.commonType();

                // the context can be null if accessing var properties from cpp after re-parenting an item.
                QQmlEnginePrivate *ep = (ctxt.isNull() || ctxt->engine() == nullptr)
                        ? nullptr
                        : QQmlEnginePrivate::get(ctxt->engine());

                if (c == QMetaObject::ReadProperty) {
                    if (property.isList()) {
                        // _id because this is an absolute property ID.
                        const QQmlPropertyData *propertyData = cache->property(_id);
                        const QMetaType propType = propertyData->propType();

                        if (propType.flags().testFlag(QMetaType::IsQmlList)) {
                            // when reading from the list, we need to find the correct MetaObject,
                            // namely this. However, obejct->metaObject might point to any
                            // MetaObject down the inheritance hierarchy, so we need to store how
                            // far we have to go down
                            // To do this, we encode the hierarchy depth together with the id of the
                            // property in a single quintptr, with the first half storing the depth
                            // and the second half storing the property id
                            auto mo = static_cast<QQmlVMEMetaObject *>(
                                        QObjectPrivate::get(object)->metaObject);
                            quintptr inheritanceDepth = 0u;
                            while (mo && mo != this) {
                                mo = mo->parentVMEMetaObject();
                                ++inheritanceDepth;
                            }
                            constexpr quintptr idBits = sizeof(quintptr) * CHAR_BIT / 2u;
                            if (Q_UNLIKELY(inheritanceDepth >= (quintptr(1) << idBits))) {
                                qmlWarning(object) << "Too many objects in inheritance hierarchy "
                                                      "for list property";
                                return -1;
                            }
                            if (Q_UNLIKELY(quintptr(id) >= (quintptr(1) << idBits))) {
                                qmlWarning(object) << "Too many properties in object "
                                                      "for list property";
                                return -1;
                            }
                            quintptr encodedIndex = (inheritanceDepth << idBits) + id;

                            initPropertyAsList(id);
                            *static_cast<QQmlListProperty<QObject> *>(a[0])
                                    = QQmlListProperty<QObject>(
                                        object, reinterpret_cast<void *>(quintptr(encodedIndex)),
                                        list_append, list_count, list_at,
                                        list_clear, list_replace, list_removeLast);
                        } else if (QV4::MemberData *md = propertyAndMethodStorageAsMemberData()) {
                            // Value type list
                            QV4::Scope scope(engine);
                            QV4::Scoped<QV4::Sequence> sequence(scope, *(md->data() + id));
                            const void *data = sequence
                                    ? QV4::SequencePrototype::getRawContainerPtr(sequence, propType)
                                    : nullptr;
                            propType.destruct(a[0]);
                            propType.construct(a[0], data);
                        } else {
                            qmlWarning(object) << "Cannot find member data";
                        }
                    } else {
                        switch (t) {
                        case QV4::CompiledData::CommonType::Void:
                            break;
                        case QV4::CompiledData::CommonType::Int:
                            *reinterpret_cast<int *>(a[0]) = readPropertyAsInt(id);
                            break;
                        case QV4::CompiledData::CommonType::Bool:
                            *reinterpret_cast<bool *>(a[0]) = readPropertyAsBool(id);
                            break;
                        case QV4::CompiledData::CommonType::Real:
                            *reinterpret_cast<double *>(a[0]) = readPropertyAsDouble(id);
                            break;
                        case QV4::CompiledData::CommonType::String:
                            *reinterpret_cast<QString *>(a[0]) = readPropertyAsString(id);
                            break;
                        case QV4::CompiledData::CommonType::Url:
                            *reinterpret_cast<QUrl *>(a[0]) = readPropertyAsUrl(id);
                            break;
                        case QV4::CompiledData::CommonType::Date:
                            *reinterpret_cast<QDate *>(a[0]) = readPropertyAsDate(id);
                            break;
                        case QV4::CompiledData::CommonType::DateTime:
                            *reinterpret_cast<QDateTime *>(a[0]) = readPropertyAsDateTime(id);
                            break;
                        case QV4::CompiledData::CommonType::RegExp:
#if QT_CONFIG(regularexpression)
                            *reinterpret_cast<QRegularExpression *>(a[0])
                                = readPropertyAsRegularExpression(id);
#endif
                            break;
                        case QV4::CompiledData::CommonType::Rect:
                            *reinterpret_cast<QRectF *>(a[0]) = readPropertyAsRectF(id);
                            break;
                        case QV4::CompiledData::CommonType::Size:
                            *reinterpret_cast<QSizeF *>(a[0]) = readPropertyAsSizeF(id);
                            break;
                        case QV4::CompiledData::CommonType::Point:
                            *reinterpret_cast<QPointF *>(a[0]) = readPropertyAsPointF(id);
                            break;
                        case QV4::CompiledData::CommonType::Time:
                            *reinterpret_cast<QTime *>(a[0]) = readPropertyAsTime(id);
                            break;
                        case QV4::CompiledData::CommonType::Var:
                            if (ep) {
                                *reinterpret_cast<QVariant *>(a[0]) = readPropertyAsVariant(id);
                            } else {
                                // if the context was disposed,
                                // we just return an invalid variant from read.
                                *reinterpret_cast<QVariant *>(a[0]) = QVariant();
                            }
                            break;
                        case QV4::CompiledData::CommonType::Invalid:
                            if (QV4::MemberData *md = propertyAndMethodStorageAsMemberData()) {
                                QV4::Scope scope(engine);
                                QV4::ScopedValue sv(scope, *(md->data() + id));

                                // _id because this is an absolute property ID.
                                const QQmlPropertyData *propertyData = cache->property(_id);

                                if (propertyData->isQObject()) {
                                    if (const auto *wrap = sv->as<QV4::QObjectWrapper>())
                                        *reinterpret_cast<QObject **>(a[0]) = wrap->object();
                                    else
                                        *reinterpret_cast<QObject **>(a[0]) = nullptr;
                                } else {
                                    const QMetaType propType = propertyData->propType();
                                    const void *data = nullptr;
                                    if (const auto *v = sv->as<QV4::VariantObject>()) {
                                        const QVariant &variant = v->d()->data();
                                        if (variant.metaType() == propType)
                                            data = variant.constData();
                                    }
                                    propType.destruct(a[0]);
                                    propType.construct(a[0], data);
                                }
                            } else {
                                qmlWarning(object) << "Cannot find member data";
                            }
                        }
                    }
                } else if (c == QMetaObject::WriteProperty) {
                    bool needActivate = false;

                    if (property.isList()) {
                        // _id because this is an absolute property ID.
                        const QQmlPropertyData *propertyData = cache->property(_id);
                        const QMetaType propType = propertyData->propType();

                        if (propType.flags().testFlag(QMetaType::IsQmlList)) {
                            // Writing such a property is not supported. Content is added through
                            // the list property methods.
                        } else if (QV4::MemberData *md = propertyAndMethodStorageAsMemberData()) {
                            // Value type list
                            QV4::Scope scope(engine);
                            QV4::Scoped<QV4::Sequence> sequence(scope, *(md->data() + id));
                            void *data = sequence
                                    ? QV4::SequencePrototype::getRawContainerPtr(sequence, propType)
                                    : nullptr;
                            if (data) {
                                if (!propType.equals(data, a[0])) {
                                    propType.destruct(data);
                                    propType.construct(data, a[0]);
                                    needActivate = true;
                                }
                            } else {
                                QV4::ScopedValue sequence(scope, QV4::SequencePrototype::fromData(
                                                              engine, propType, a[0]));
                                md->set(engine, id, sequence);
                                if (sequence->isUndefined()) {
                                    qmlWarning(object)
                                            << "Could not create a QML sequence object for "
                                            << propType.name();
                                }
                                needActivate = true;
                            }
                        } else {
                            qmlWarning(object) << "Cannot find member data";
                        }
                    } else {
                        switch (t) {
                        case QV4::CompiledData::CommonType::Void:
                            break;
                        case QV4::CompiledData::CommonType::Int:
                            needActivate = *reinterpret_cast<int *>(a[0]) != readPropertyAsInt(id);
                            writeProperty(id, *reinterpret_cast<int *>(a[0]));
                            break;
                        case QV4::CompiledData::CommonType::Bool:
                            needActivate = *reinterpret_cast<bool *>(a[0]) != readPropertyAsBool(id);
                            writeProperty(id, *reinterpret_cast<bool *>(a[0]));
                            break;
                        case QV4::CompiledData::CommonType::Real:
                            needActivate = *reinterpret_cast<double *>(a[0]) != readPropertyAsDouble(id);
                            writeProperty(id, *reinterpret_cast<double *>(a[0]));
                            break;
                        case QV4::CompiledData::CommonType::String:
                            needActivate = *reinterpret_cast<QString *>(a[0]) != readPropertyAsString(id);
                            writeProperty(id, *reinterpret_cast<QString *>(a[0]));
                            break;
                        case QV4::CompiledData::CommonType::Url:
                            needActivate = *reinterpret_cast<QUrl *>(a[0]) != readPropertyAsUrl(id);
                            writeProperty(id, *reinterpret_cast<QUrl *>(a[0]));
                            break;
                        case QV4::CompiledData::CommonType::Date:
                            needActivate = *reinterpret_cast<QDate *>(a[0]) != readPropertyAsDate(id);
                            writeProperty(id, *reinterpret_cast<QDate *>(a[0]));
                            break;
                        case QV4::CompiledData::CommonType::DateTime:
                            needActivate = *reinterpret_cast<QDateTime *>(a[0]) != readPropertyAsDateTime(id);
                            writeProperty(id, *reinterpret_cast<QDateTime *>(a[0]));
                            break;
                        case QV4::CompiledData::CommonType::RegExp:
#if QT_CONFIG(regularexpression)
                            needActivate = *reinterpret_cast<QRegularExpression *>(a[0])
                                           != readPropertyAsRegularExpression(id);
                            writeProperty(id, *reinterpret_cast<QRegularExpression *>(a[0]));
#endif
                            break;
                        case QV4::CompiledData::CommonType::Rect:
                            needActivate = *reinterpret_cast<QRectF *>(a[0]) != readPropertyAsRectF(id);
                            writeProperty(id, *reinterpret_cast<QRectF *>(a[0]));
                            break;
                        case QV4::CompiledData::CommonType::Size:
                            needActivate = *reinterpret_cast<QSizeF *>(a[0]) != readPropertyAsSizeF(id);
                            writeProperty(id, *reinterpret_cast<QSizeF *>(a[0]));
                            break;
                        case QV4::CompiledData::CommonType::Point:
                            needActivate = *reinterpret_cast<QPointF *>(a[0]) != readPropertyAsPointF(id);
                            writeProperty(id, *reinterpret_cast<QPointF *>(a[0]));
                            break;
                        case QV4::CompiledData::CommonType::Time:
                            needActivate = *reinterpret_cast<QTime *>(a[0]) != readPropertyAsTime(id);
                            writeProperty(id, *reinterpret_cast<QTime *>(a[0]));
                            break;
                        case QV4::CompiledData::CommonType::Var:
                            if (ep)
                                writeProperty(id, *reinterpret_cast<QVariant *>(a[0]));
                            break;
                        case QV4::CompiledData::CommonType::Invalid:
                            if (QV4::MemberData *md = propertyAndMethodStorageAsMemberData()) {
                                QV4::Scope scope(engine);
                                QV4::ScopedValue sv(scope, *(md->data() + id));

                                // _id because this is an absolute property ID.
                                const QQmlPropertyData *propertyData = cache->property(_id);

                                if (propertyData->isQObject()) {
                                    QObject *arg = *reinterpret_cast<QObject **>(a[0]);
                                    if (const auto *wrap = sv->as<QV4::QObjectWrapper>())
                                        needActivate = wrap->object() != arg;
                                    else if (arg != nullptr || !sv->isNull())
                                        needActivate = true;
                                    if (needActivate)
                                        writeProperty(id, arg);
                                } else {
                                    const QMetaType propType = propertyData->propType();
                                    if (const auto *v = sv->as<QV4::VariantObject>()) {
                                        QVariant &variant = v->d()->data();
                                        if (variant.metaType() != propType) {
                                            needActivate = true;
                                            variant = QVariant(propType, a[0]);
                                        } else if (!propType.equals(variant.constData(), a[0])) {
                                            needActivate = true;
                                            propType.destruct(variant.data());
                                            propType.construct(variant.data(), a[0]);
                                        }
                                    } else {
                                        needActivate = true;
                                        md->set(engine, id, engine->newVariantObject(
                                                    propType, a[0]));
                                    }
                                }
                            } else {
                                qmlWarning(object) << "Cannot find member data";
                            }
                        }
                    }

                    if (needActivate)
                        activate(object, methodOffset() + id, nullptr);
                }

                return -1;
            }

            id -= propertyCount;

            if (id < aliasCount) {
                const QV4::CompiledData::Alias *aliasData = &compiledObject->aliasTable()[id];

                if (aliasData->hasFlag(QV4::CompiledData::Alias::AliasPointsToPointerObject)
                        && c == QMetaObject::ReadProperty){
                    *reinterpret_cast<void **>(a[0]) = nullptr;
                }

                if (ctxt.isNull())
                    return -1;

                while (aliasData->isAliasToLocalAlias())
                    aliasData = &compiledObject->aliasTable()[aliasData->localAliasIndex];

                QObject *target = ctxt->idValue(aliasData->targetObjectId());
                if (!target)
                    return -1;

                connectAlias(id);

                if (aliasData->isObjectAlias()) {
                    *reinterpret_cast<QObject **>(a[0]) = target;
                    return -1;
                }

                QQmlData *targetDData = QQmlData::get(target, /*create*/false);
                if (!targetDData)
                    return -1;

                QQmlPropertyIndex encodedIndex = QQmlPropertyIndex::fromEncoded(aliasData->encodedMetaPropertyIndex);
                int coreIndex = encodedIndex.coreIndex();
                const int valueTypePropertyIndex = encodedIndex.valueTypeIndex();

                const auto removePendingBinding
                        = [c, a](QObject *target, int coreIndex, QQmlPropertyIndex encodedIndex) {
                    // Remove binding (if any) on write
                    if (c == QMetaObject::WriteProperty) {
                        int flags = *reinterpret_cast<int*>(a[3]);
                        if (flags & QQmlPropertyData::RemoveBindingOnAliasWrite) {
                            QQmlData *targetData = QQmlData::get(target);
                            if (targetData && targetData->hasBindingBit(coreIndex)) {
                                QQmlPropertyPrivate::removeBinding(target, encodedIndex);
                                targetData->clearBindingBit(coreIndex);
                            }
                        }
                    }
                };

                if (valueTypePropertyIndex != -1) {
                    if (!targetDData->propertyCache)
                        return -1;
                    const QQmlPropertyData *pd = targetDData->propertyCache->property(coreIndex);
                    // Value type property or deep alias
                    QQmlGadgetPtrWrapper *valueType = QQmlGadgetPtrWrapper::instance(
                                ctxt->engine(), pd->propType());
                    if (valueType) {
                        removePendingBinding(target, coreIndex, encodedIndex);
                        valueType->read(target, coreIndex);
                        int rv = QMetaObject::metacall(valueType, c, valueTypePropertyIndex, a);

                        if (c == QMetaObject::WriteProperty)
                            valueType->write(target, coreIndex, QQmlPropertyData::HasInternalIndex,
                                             valueTypePropertyIndex);

                        return rv;
                    } else {
                        // deep alias
                        void *argv[1] = { &target };
                        QMetaObject::metacall(target, QMetaObject::ReadProperty, coreIndex, argv);
                        removePendingBinding(
                                target, valueTypePropertyIndex,
                                QQmlPropertyIndex(valueTypePropertyIndex));
                        return QMetaObject::metacall(target, c, valueTypePropertyIndex, a);
                    }

                } else {
                    removePendingBinding(target, coreIndex, encodedIndex);
                    return QMetaObject::metacall(target, c, coreIndex, a);
                }

            }
            return -1;

        }

    } else if(c == QMetaObject::InvokeMetaMethod) {

        if (id >= methodOffset()) {

            id -= methodOffset();
            int plainSignals = signalCount + propertyCount + aliasCount;
            if (id < plainSignals) {
                activate(object, _id, a);
                return -1;
            }

            id -= plainSignals;

            if (id < methodCount) {
                QQmlEngine *engine = ctxt->engine();
                if (!engine)
                    return -1; // We can't run the method

                QQmlEnginePrivate *ep = QQmlEnginePrivate::get(engine);
                QV4::ExecutionEngine *v4 = engine->handle();
                ep->referenceScarceResources(); // "hold" scarce resources in memory during evaluation.
                QV4::Scope scope(v4);


                QV4::ScopedFunctionObject function(scope, method(id));
                if (!function) {
                    // The function was not compiled.  There are some exceptional cases which the
                    // expression rewriter does not rewrite properly (e.g., \r-terminated lines
                    // are not rewritten correctly but this bug is deemed out-of-scope to fix for
                    // performance reasons; see QTBUG-24064) and thus compilation will have failed.
                    QQmlError e;
                    e.setDescription(
                                QStringLiteral(
                                    "Exception occurred during compilation of function: ")
                                + QString::fromUtf8(metaObject->method(_id).methodSignature()));
                    ep->warning(e);
                    return -1; // The dynamic method with that id is not available.
                }

                auto methodData = cache->method(_id);
                auto arguments = methodData->hasArguments() ? methodData->arguments() : nullptr;

                if (arguments && arguments->names) {
                    const quint32 parameterCount = arguments->names->size();
                    Q_ASSERT(parameterCount == function->formalParameterCount());
                    if (void *result = a[0])
                        arguments->types[0].destruct(result);
                    function->call(object, a, arguments->types, parameterCount);
                } else {
                    Q_ASSERT(function->formalParameterCount() == 0);
                    const QMetaType returnType = methodData->propType();
                    if (void *result = a[0])
                        returnType.destruct(result);
                    function->call(object, a, &returnType, 0);
                }

                if (scope.hasException()) {
                    QQmlError error = scope.engine->catchExceptionAsQmlError();
                    if (error.isValid())
                        ep->warning(error);
                }

                ep->dereferenceScarceResources(); // "release" scarce resources if top-level expression evaluation is complete.
                return -1;
            }
            return -1;
        }
    }

    if (parent.isT1())
        return parent.asT1()->metaCall(object, c, _id, a);
    else
        return object->qt_metacall(c, _id, a);
}

QV4::ReturnedValue QQmlVMEMetaObject::method(int index) const
{
    if (ctxt.isNull() || !ctxt->isValid() || !compiledObject) {
        qWarning("QQmlVMEMetaObject: Internal error - attempted to evaluate a function in an invalid context");
        return QV4::Encode::undefined();
    }

    QV4::MemberData *md = propertyAndMethodStorageAsMemberData();
    if (!md)
        return QV4::Encode::undefined();

    return (md->data() + index + compiledObject->nProperties)->asReturnedValue();
}

QV4::ReturnedValue QQmlVMEMetaObject::readVarProperty(int id) const
{
    Q_ASSERT(compiledObject && compiledObject->propertyTable()[id].commonType() == QV4::CompiledData::CommonType::Var);

    QV4::MemberData *md = propertyAndMethodStorageAsMemberData();
    if (md)
        return (md->data() + id)->asReturnedValue();
    return QV4::Value::undefinedValue().asReturnedValue();
}

QVariant QQmlVMEMetaObject::readPropertyAsVariant(int id) const
{
    QV4::MemberData *md = propertyAndMethodStorageAsMemberData();
    if (md) {
        const QV4::QObjectWrapper *wrapper = (md->data() + id)->as<QV4::QObjectWrapper>();
        if (wrapper)
            return QVariant::fromValue(wrapper->object());
        const QV4::VariantObject *v = (md->data() + id)->as<QV4::VariantObject>();
        if (v)
            return v->d()->data();
        return QV4::ExecutionEngine::toVariant(*(md->data() + id), QMetaType {});
    }
    return QVariant();
}

void QQmlVMEMetaObject::writeVarProperty(int id, const QV4::Value &value)
{
    Q_ASSERT(compiledObject && compiledObject->propertyTable()[id].commonType() == QV4::CompiledData::CommonType::Var);

    QV4::MemberData *md = propertyAndMethodStorageAsMemberData();
    if (!md)
        return;

    // Importantly, if the current value is a scarce resource, we need to ensure that it
    // gets automatically released by the engine if no other references to it exist.
    const QV4::VariantObject *oldVariant = (md->data() + id)->as<QV4::VariantObject>();
    if (oldVariant)
        oldVariant->removeVmePropertyReference();

    QObject *valueObject = nullptr;
    QQmlVMEVariantQObjectPtr *guard = getQObjectGuardForProperty(id);

    // And, if the new value is a scarce resource, we need to ensure that it does not get
    // automatically released by the engine until no other references to it exist.
    if (QV4::VariantObject *v = const_cast<QV4::VariantObject*>(value.as<QV4::VariantObject>())) {
        v->addVmePropertyReference();
        md->set(engine, id, value);
    } else if (QV4::QObjectWrapper *wrapper = const_cast<QV4::QObjectWrapper*>(value.as<QV4::QObjectWrapper>())) {
        // We need to track this QObject to signal its deletion
        valueObject = wrapper->object();

        // Do we already have a QObject guard for this property?
        if (valueObject && !guard) {
            guard = new QQmlVMEVariantQObjectPtr();
            varObjectGuards.append(guard);
        }
        md->set(engine, id, value);
    } else if (const QV4::Sequence *sequence = value.as<QV4::Sequence>()) {
        QV4::Heap::Sequence *p = sequence->d();
        if (p->enforcesLocation()) {
            // If the sequence enforces its location, we don't want it to be updated anymore after
            // being written to a property.
            md->set(engine, id, QV4::ReferenceObject::detached(p));
        } else {
            // Otherwise, make sure the reference carries some value so that we can still call
            // toVariant() on it (see note in QV4::SequencePrototype::toVariant).
            if (!p->hasData())
                QV4::ReferenceObject::readReference(p);
            md->set(engine, id, p);
        }
    } else if (const QV4::QQmlValueTypeWrapper *wrapper = value.as<QV4::QQmlValueTypeWrapper>()) {
        // If the value type enforces its location, we don't want it to be updated anymore after
        // being written to a property.
        QV4::Heap::QQmlValueTypeWrapper *p = wrapper->d();
        md->set(engine, id, p->enforcesLocation() ? QV4::ReferenceObject::detached(p) : p);
    } else {
        md->set(engine, id, value);
    }

    if (guard)
        guard->setGuardedValue(valueObject, this, id);

    // Emit change signal as appropriate.
    activate(object, methodOffset() + id, nullptr);
}

void QQmlVMEMetaObject::writeProperty(int id, const QVariant &value)
{
    if (compiledObject && compiledObject->propertyTable()[id].commonType() == QV4::CompiledData::CommonType::Var) {
        QV4::MemberData *md = propertyAndMethodStorageAsMemberData();
        if (!md)
            return;

        // Importantly, if the current value is a scarce resource, we need to ensure that it
        // gets automatically released by the engine if no other references to it exist.
        const QV4::VariantObject *oldv = (md->data() + id)->as<QV4::VariantObject>();
        if (oldv)
            oldv->removeVmePropertyReference();

        // And, if the new value is a scarce resource, we need to ensure that it does not get
        // automatically released by the engine until no other references to it exist.
        QV4::Scope scope(engine);
        QV4::ScopedValue newv(scope, engine->fromVariant(value));
        QV4::Scoped<QV4::VariantObject> v(scope, newv);
        if (!!v)
            v->addVmePropertyReference();

        // Write the value and emit change signal as appropriate.
        QVariant currentValue = readPropertyAsVariant(id);
        md->set(engine, id, newv);
        if ((currentValue.userType() != value.userType() || currentValue != value))
            activate(object, methodOffset() + id, nullptr);
    } else {
        bool needActivate = false;
        if (value.userType() == QMetaType::QObjectStar) {
            QObject *o = *(QObject *const *)value.data();
            needActivate = readPropertyAsQObject(id) != o;  // TODO: still correct?
            writeProperty(id, o);
        } else {
            QV4::MemberData *md = propertyAndMethodStorageAsMemberData();
            if (md) {
                const QV4::VariantObject *v = (md->data() + id)->as<QV4::VariantObject>();
                needActivate = (!v ||
                                 v->d()->data().userType() != value.userType() ||
                                 v->d()->data() != value);
                if (v)
                    v->removeVmePropertyReference();
                md->set(engine, id, engine->newVariantObject(value.metaType(), value.constData()));
                v = static_cast<const QV4::VariantObject *>(md->data() + id);
                v->addVmePropertyReference();
            }
        }

        if (needActivate)
            activate(object, methodOffset() + id, nullptr);
    }
}

QV4::ReturnedValue QQmlVMEMetaObject::vmeMethod(int index) const
{
    if (index < methodOffset()) {
        Q_ASSERT(parentVMEMetaObject());
        return parentVMEMetaObject()->vmeMethod(index);
    }
    if (!compiledObject)
        return QV4::Value::undefinedValue().asReturnedValue();
    const int plainSignals = compiledObject->nSignals + compiledObject->nProperties + compiledObject->nAliases;
    Q_ASSERT(index >= (methodOffset() + plainSignals) && index < (methodOffset() + plainSignals + int(compiledObject->nFunctions)));
    return method(index - methodOffset() - plainSignals);
}

// Used by debugger
void QQmlVMEMetaObject::setVmeMethod(int index, const QV4::Value &function)
{
    if (index < methodOffset()) {
        Q_ASSERT(parentVMEMetaObject());
        return parentVMEMetaObject()->setVmeMethod(index, function);
    }
    if (!compiledObject)
        return;
    const int plainSignals = compiledObject->nSignals + compiledObject->nProperties + compiledObject->nAliases;
    Q_ASSERT(index >= (methodOffset() + plainSignals) && index < (methodOffset() + plainSignals + int(compiledObject->nFunctions)));

    int methodIndex = index - methodOffset() - plainSignals;
    QV4::MemberData *md = propertyAndMethodStorageAsMemberData();
    if (!md)
        return;
    md->set(engine, methodIndex + compiledObject->nProperties, function);
}

QV4::ReturnedValue QQmlVMEMetaObject::vmeProperty(int index) const
{
    if (index < propOffset()) {
        Q_ASSERT(parentVMEMetaObject());
        return parentVMEMetaObject()->vmeProperty(index);
    }
    return readVarProperty(index - propOffset());
}

void QQmlVMEMetaObject::setVMEProperty(int index, const QV4::Value &v)
{
    if (index < propOffset()) {
        Q_ASSERT(parentVMEMetaObject());
        parentVMEMetaObject()->setVMEProperty(index, v);
        return;
    }
    return writeVarProperty(index - propOffset(), v);
}

void QQmlVMEMetaObject::ensureQObjectWrapper()
{
    Q_ASSERT(cache);
    QV4::QObjectWrapper::wrap(engine, object);
}

void QQmlVMEMetaObject::mark(QV4::MarkStack *markStack)
{
    if (engine != markStack->engine())
        return;

    propertyAndMethodStorage.markOnce(markStack);

    if (QQmlVMEMetaObject *parent = parentVMEMetaObject())
        parent->mark(markStack);
}

bool QQmlVMEMetaObject::aliasTarget(int index, QObject **target, int *coreIndex, int *valueTypeIndex) const
{
    Q_ASSERT(compiledObject && (index >= propOffset() + int(compiledObject->nProperties)));

    *target = nullptr;
    *coreIndex = -1;
    *valueTypeIndex = -1;

    if (ctxt.isNull())
        return false;

    const int aliasId = index - propOffset() - compiledObject->nProperties;
    const QV4::CompiledData::Alias *aliasData = &compiledObject->aliasTable()[aliasId];
    while (aliasData->isAliasToLocalAlias())
        aliasData = &compiledObject->aliasTable()[aliasData->localAliasIndex];
    *target = ctxt->idValue(aliasData->targetObjectId());
    if (!*target)
        return false;

    if (!aliasData->isObjectAlias()) {
        QQmlPropertyIndex encodedIndex = QQmlPropertyIndex::fromEncoded(aliasData->encodedMetaPropertyIndex);
        *coreIndex = encodedIndex.coreIndex();
        *valueTypeIndex = encodedIndex.valueTypeIndex();
    }
    return true;
}

void QQmlVMEMetaObject::connectAlias(int aliasId)
{
    Q_ASSERT(compiledObject);
    if (!aliasEndpoints)
        aliasEndpoints = new QQmlVMEMetaObjectEndpoint[compiledObject->nAliases];

    const QV4::CompiledData::Alias *aliasData = &compiledObject->aliasTable()[aliasId];

    QQmlVMEMetaObjectEndpoint *endpoint = aliasEndpoints + aliasId;
    if (endpoint->metaObject.data()) {
        // already connected
        Q_ASSERT(endpoint->metaObject.data() == this);
        return;
    }

    endpoint->metaObject = this;
    endpoint->connect(ctxt->idValueBindings(aliasData->targetObjectId()));
    endpoint->tryConnect();
}

void QQmlVMEMetaObject::connectAliasSignal(int index, bool indexInSignalRange)
{
    Q_ASSERT(compiledObject);
    int aliasId = (index - (indexInSignalRange ? signalOffset() : methodOffset())) - compiledObject->nProperties;
    if (aliasId < 0 || aliasId >= int(compiledObject->nAliases))
        return;

    connectAlias(aliasId);
}

/*! \internal
    \a index is in the method index range (QMetaMethod::methodIndex()).
*/
void QQmlVMEMetaObject::activate(QObject *object, int index, void **args)
{
    QMetaObject::activate(object, signalOffset(), index - methodOffset(), args);
}

QQmlVMEMetaObject *QQmlVMEMetaObject::getForProperty(QObject *o, int coreIndex)
{
    QQmlVMEMetaObject *vme = QQmlVMEMetaObject::get(o);
    while (vme && vme->propOffset() > coreIndex)
        vme = vme->parentVMEMetaObject();

    Q_ASSERT(vme);
    return vme;
}

QQmlVMEMetaObject *QQmlVMEMetaObject::getForMethod(QObject *o, int coreIndex)
{
    QQmlVMEMetaObject *vme = QQmlVMEMetaObject::get(o);
    while (vme && vme->methodOffset() > coreIndex)
        vme = vme->parentVMEMetaObject();

    Q_ASSERT(vme);
    return vme;
}

/*! \internal
    \a coreIndex is in the signal index range (see QObjectPrivate::signalIndex()).
    This is different from QMetaMethod::methodIndex().
*/
QQmlVMEMetaObject *QQmlVMEMetaObject::getForSignal(QObject *o, int coreIndex)
{
    QQmlVMEMetaObject *vme = QQmlVMEMetaObject::get(o);
    while (vme && vme->signalOffset() > coreIndex)
        vme = vme->parentVMEMetaObject();

    Q_ASSERT(vme);
    return vme;
}

QQmlVMEVariantQObjectPtr *QQmlVMEMetaObject::getQObjectGuardForProperty(int index) const
{
    QList<QQmlVMEVariantQObjectPtr *>::ConstIterator it = varObjectGuards.constBegin(), end = varObjectGuards.constEnd();
    for ( ; it != end; ++it) {
        if ((*it)->m_index == index) {
            return *it;
        }
    }

    return nullptr;
}

QT_END_NAMESPACE
