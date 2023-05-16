// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qqmlvaluetypewrapper_p.h"

#include <private/qqmlvaluetype_p.h>
#include <private/qqmlbinding_p.h>
#include <private/qqmlglobal_p.h>
#include <private/qqmlbuiltinfunctions_p.h>

#include <private/qv4engine_p.h>
#include <private/qv4functionobject_p.h>
#include <private/qv4variantobject_p.h>
#include <private/qv4alloca_p.h>
#include <private/qv4stackframe_p.h>
#include <private/qv4objectiterator_p.h>
#include <private/qv4qobjectwrapper_p.h>
#include <private/qv4identifiertable_p.h>
#include <private/qv4lookup_p.h>
#include <private/qv4sequenceobject_p.h>
#include <private/qv4arraybuffer_p.h>
#include <private/qv4dateobject_p.h>
#include <private/qv4jsonobject_p.h>
#if QT_CONFIG(regularexpression)
#include <private/qv4regexpobject_p.h>
#endif
#if QT_CONFIG(qml_locale)
#include <private/qqmllocale_p.h>
#endif
#include <QtCore/qloggingcategory.h>
#include <QtCore/qdatetime.h>
#include <QtCore/QLine>
#include <QtCore/QLineF>
#include <QtCore/QSize>
#include <QtCore/QSizeF>
#include <QtCore/QTimeZone>

QT_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(lcBindingRemoval)

DEFINE_OBJECT_VTABLE(QV4::QQmlValueTypeWrapper);

namespace QV4 {

Heap::QQmlValueTypeWrapper *Heap::QQmlValueTypeWrapper::detached() const
{
    return internalClass->engine->memoryManager->allocate<QV4::QQmlValueTypeWrapper>(
        m_gadgetPtr, QMetaType(m_metaType), m_metaObject, nullptr, -1, NoFlag);
}

void Heap::QQmlValueTypeWrapper::destroy()
{
    if (m_gadgetPtr) {
        metaType().destruct(m_gadgetPtr);
        ::operator delete(m_gadgetPtr);
    }
    ReferenceObject::destroy();
}

void Heap::QQmlValueTypeWrapper::setData(const void *data)
{
    if (auto *gadget = gadgetPtr())
        metaType().destruct(gadget);
    if (!gadgetPtr())
        setGadgetPtr(::operator new(metaType().sizeOf()));
    metaType().construct(gadgetPtr(), data);
}

QVariant Heap::QQmlValueTypeWrapper::toVariant() const
{
    Q_ASSERT(gadgetPtr());
    return QVariant(metaType(), gadgetPtr());
}

bool Heap::QQmlValueTypeWrapper::setVariant(const QVariant &variant)
{
    Q_ASSERT(isVariant());

    const QMetaType variantReferenceType = variant.metaType();
    if (variantReferenceType != metaType()) {
        // This is a stale VariantReference.  That is, the variant has been
        // overwritten with a different type in the meantime.
        // We need to modify this reference to the updated value type, if
        // possible, or return false if it is not a value type.
        if (QQmlMetaType::isValueType(variantReferenceType)) {
            const QMetaObject *mo = QQmlMetaType::metaObjectForValueType(variantReferenceType);
            if (gadgetPtr()) {
                metaType().destruct(gadgetPtr());
                ::operator delete(gadgetPtr());
            }
            setGadgetPtr(nullptr);
            setMetaObject(mo);
            setMetaType(variantReferenceType);
            if (!mo)
                return false;
        } else {
            return false;
        }
    }

    setData(variant.constData());
    return true;
}

void *Heap::QQmlValueTypeWrapper::storagePointer()
{
    if (!gadgetPtr()) {
        setGadgetPtr(::operator new(metaType().sizeOf()));
        metaType().construct(gadgetPtr(), nullptr);
    }
    return gadgetPtr();
}

bool Heap::QQmlValueTypeWrapper::readReference()
{
    // If locations are enforced we only read once
    return enforcesLocation() || QV4::ReferenceObject::readReference(this);
}

bool Heap::QQmlValueTypeWrapper::writeBack(int propertyIndex)
{
    return isAttachedToProperty() && QV4::ReferenceObject::writeBack(this, propertyIndex);
}

ReturnedValue QQmlValueTypeWrapper::create(
        ExecutionEngine *engine, Heap::QQmlValueTypeWrapper *cloneFrom, Heap::Object *object)
{
    QV4::Scope scope(engine);
    initProto(engine);

    // Either we're enforcing the location, then we have to read right away.
    // Or we don't then we lazy-load. In neither case we pass any data.
    Scoped<QQmlValueTypeWrapper> r(scope, engine->memoryManager->allocate<QQmlValueTypeWrapper>(
                                       nullptr, cloneFrom->metaType(), cloneFrom->metaObject(),
                                       object, cloneFrom->property(), cloneFrom->flags()));
    r->d()->setLocation(cloneFrom->function(), cloneFrom->statementIndex());
    if (cloneFrom->enforcesLocation())
        QV4::ReferenceObject::readReference(r->d());
    return r->asReturnedValue();
}

void QQmlValueTypeWrapper::initProto(ExecutionEngine *v4)
{
    if (v4->valueTypeWrapperPrototype()->d_unchecked())
        return;

    Scope scope(v4);
    ScopedObject o(scope, v4->newObject());
    o->defineDefaultProperty(v4->id_toString(), method_toString, 1);
    v4->jsObjects[QV4::ExecutionEngine::ValueTypeProto] = o->d();
}

int QQmlValueTypeWrapper::virtualMetacall(
        Object *object, QMetaObject::Call call, int index, void **a)
{
    QQmlValueTypeWrapper *wrapper = object->as<QQmlValueTypeWrapper>();
    Q_ASSERT(wrapper);

    switch (call) {
    case QMetaObject::InvokeMetaMethod:
    case QMetaObject::ReadProperty:
    case QMetaObject::BindableProperty:
    case QMetaObject::CustomCall:
        if (wrapper->d()->object())
            wrapper->d()->readReference();
        break;
    default:
        break;
    }

    const QMetaObject *mo = wrapper->d()->metaObject();
    if (!mo->d.static_metacall)
        return 0;

    mo->d.static_metacall(static_cast<QObject *>(wrapper->d()->gadgetPtr()), call, index, a);

    switch (call) {
    case QMetaObject::ReadProperty:
        break;
    case QMetaObject::WriteProperty:
    case QMetaObject::ResetProperty:
        if (wrapper->d()->object())
            wrapper->d()->writeBack(index);
        break;
    case QMetaObject::InvokeMetaMethod:
    case QMetaObject::CustomCall:
        if (wrapper->d()->object())
            wrapper->d()->writeBack();
        break;
    default:
        break;
    }

    return -1;
}

ReturnedValue QQmlValueTypeWrapper::create(
        ExecutionEngine *engine, const void *data, const QMetaObject *metaObject, QMetaType type,
        Heap::Object *object, int property, Heap::ReferenceObject::Flags flags)
{
    Scope scope(engine);
    initProto(engine);

    if (!type.isValid()) {
        return engine->throwTypeError(QLatin1String("Type %1 is not a value type")
                                      .arg(QString::fromUtf8(type.name())));
    }

    // If data is given explicitly, we assume it has just been read from the property
    Scoped<QQmlValueTypeWrapper> r(scope, engine->memoryManager->allocate<QQmlValueTypeWrapper>(
                                       data, type, metaObject, object, property, flags));
    if (CppStackFrame *frame = engine->currentStackFrame)
        r->d()->setLocation(frame->v4Function, frame->statementNumber());
    if (!data && r->d()->enforcesLocation())
        QV4::ReferenceObject::readReference(r->d());
    return r->asReturnedValue();
}

ReturnedValue QQmlValueTypeWrapper::create(
        ExecutionEngine *engine, const void *data, const QMetaObject *metaObject, QMetaType type)
{
    Scope scope(engine);
    initProto(engine);

    if (!type.isValid()) {
        return engine->throwTypeError(QLatin1String("Type %1 is not a value type")
                                      .arg(QString::fromUtf8(type.name())));
    }

    Scoped<QQmlValueTypeWrapper> r(
                scope, engine->memoryManager->allocate<QQmlValueTypeWrapper>(
                    data, type, metaObject, nullptr,  -1, Heap::ReferenceObject::NoFlag));
    return r->asReturnedValue();
}

QVariant QQmlValueTypeWrapper::toVariant() const
{
    if (d()->isReference() && !readReferenceValue())
        return QVariant();
    return d()->toVariant();
}

bool QQmlValueTypeWrapper::toGadget(void *data) const
{
    if (d()->isReference() && !readReferenceValue())
        return false;
    const QMetaType type = d()->metaType();
    type.destruct(data);
    type.construct(data, d()->gadgetPtr());
    return true;
}

bool QQmlValueTypeWrapper::virtualIsEqualTo(Managed *m, Managed *other)
{
    Q_ASSERT(m && m->as<QQmlValueTypeWrapper>() && other);
    QV4::QQmlValueTypeWrapper *lv = static_cast<QQmlValueTypeWrapper *>(m);

    if (QV4::VariantObject *rv = other->as<VariantObject>())
        return lv->isEqual(rv->d()->data());

    if (QV4::QQmlValueTypeWrapper *v = other->as<QQmlValueTypeWrapper>())
        return lv->isEqual(v->toVariant());

    return false;
}

bool QQmlValueTypeWrapper::virtualHasProperty(const Managed *m, PropertyKey id)
{
    if (!id.isString())
        return Object::virtualHasProperty(m, id);
    Q_ASSERT(m && m->as<QQmlValueTypeWrapper>());
    auto wrapper = static_cast<const QQmlValueTypeWrapper *>(m);
    if (auto mo = wrapper->d()->metaObject())
        if (mo->indexOfProperty(id.toQString().toUtf8()) != -1)
            return true;

    /* we don't want to fallback to QObject::virtualHasProperty
       as that would end up calling getOwnProperty which is wasteful,
       as it calls our own virtualGetOwnProperty.
       As we know that our own properties are only those found on the meta-object,
       we can instead skip the call, and simply check whether the property exists
       on the prototype.
    */
    Scope scope(m->engine());
    ScopedObject o(scope, m);
    o = o->getPrototypeOf();
    if (o)
        return o->hasProperty(id);

    return false;
}

static Heap::ReferenceObject::Flags referenceFlags(const QMetaObject *metaObject, int index)
{
    return metaObject->property(index).isWritable()
            ? (Heap::ReferenceObject::CanWriteBack | Heap::ReferenceObject::EnforcesLocation)
            : Heap::ReferenceObject::EnforcesLocation;
}

static void doStaticReadCall(
        const QMetaObject *metaObject, Heap::QQmlValueTypeWrapper *valueTypeWrapper,
        int index, void **args)
{
    metaObject->d.static_metacall(
                reinterpret_cast<QObject*>(
                    valueTypeWrapper->gadgetPtr()), QMetaObject::ReadProperty, index, args);
}

static ReturnedValue getGadgetProperty(ExecutionEngine *engine,
                                       Heap::QQmlValueTypeWrapper *valueTypeWrapper,
                                       QMetaType metaType, quint16 coreIndex, bool isFunction, bool isEnum)
{
    if (isFunction) {
        // calling a Q_INVOKABLE function of a value type
        return QV4::QObjectMethod::create(engine->rootContext(), valueTypeWrapper, coreIndex);
    }

    const QMetaObject *metaObject = valueTypeWrapper->metaObject();
    int index = coreIndex;

    const auto wrapChar16 = [engine](char16_t c) {
        return engine->newString(QChar(c));
    };
    const auto wrapQObject = [engine](QObject *object) {
        return QObjectWrapper::wrap(engine, object);
    };
    const auto wrapJsonValue = [engine](const QJsonValue &value) {
        return JsonObject::fromJsonValue(engine, value);
    };
    const auto wrapJsonObject = [engine](const QJsonObject &object) {
        return JsonObject::fromJsonObject(engine, object);
    };
    const auto wrapJsonArray = [engine](const QJsonArray &array) {
        return JsonObject::fromJsonArray(engine, array);
    };

    const auto wrapQDateTime = [&](const QDateTime &dateTime) {
        return engine->newDateObject(
                    dateTime, valueTypeWrapper, index, referenceFlags(metaObject, index));
    };
    const auto wrapQDate = [&](QDate date) {
        return engine->newDateObject(
                    date, valueTypeWrapper, index, referenceFlags(metaObject, index));
    };
    const auto wrapQTime = [&](QTime time) {
        return engine->newDateObject(
                    time, valueTypeWrapper, index, referenceFlags(metaObject, index));
    };

#if QT_CONFIG(qml_locale)
    const auto wrapLocale = [engine](const QLocale &locale) {
        return QQmlLocale::wrap(engine, locale);
    };
#endif

#define VALUE_TYPE_LOAD(metatype, cpptype, constructor) \
    case metatype: { \
        cpptype v; \
        void *args[] = { &v, nullptr }; \
        doStaticReadCall(metaObject, valueTypeWrapper, index, args); \
        return QV4::Encode(constructor(v)); \
    }

    QQmlMetaObject::resolveGadgetMethodOrPropertyIndex(
                QMetaObject::ReadProperty, &metaObject, &index);

    const int metaTypeId = isEnum
            ? metaType.underlyingType().id()
            : (metaType.flags() & QMetaType::PointerToQObject)
              ? QMetaType::QObjectStar
              : metaType.id();

    switch (metaTypeId) {
    case QMetaType::UnknownType:
    case QMetaType::Void:
        return Encode::undefined();
    case QMetaType::Nullptr:
    case QMetaType::VoidStar:
        return Encode::null();
    VALUE_TYPE_LOAD(QMetaType::Bool, bool, bool);
    VALUE_TYPE_LOAD(QMetaType::Int, int, int);
    VALUE_TYPE_LOAD(QMetaType::UInt, uint, uint);
    VALUE_TYPE_LOAD(QMetaType::Long, long, double);
    VALUE_TYPE_LOAD(QMetaType::ULong, ulong, double);
    VALUE_TYPE_LOAD(QMetaType::LongLong, qlonglong, double);
    VALUE_TYPE_LOAD(QMetaType::ULongLong, qulonglong, double);
    VALUE_TYPE_LOAD(QMetaType::Double, double, double);
    VALUE_TYPE_LOAD(QMetaType::QString, QString, engine->newString);
    VALUE_TYPE_LOAD(QMetaType::QByteArray, QByteArray, engine->newArrayBuffer);
    VALUE_TYPE_LOAD(QMetaType::Float, float, float);
    VALUE_TYPE_LOAD(QMetaType::Short, short, int);
    VALUE_TYPE_LOAD(QMetaType::UShort, unsigned short, int);
    VALUE_TYPE_LOAD(QMetaType::Char, char, int);
    VALUE_TYPE_LOAD(QMetaType::UChar, unsigned char, int);
    VALUE_TYPE_LOAD(QMetaType::SChar, signed char, int);
    VALUE_TYPE_LOAD(QMetaType::QChar, QChar, engine->newString);
    VALUE_TYPE_LOAD(QMetaType::Char16, char16_t, wrapChar16);
    VALUE_TYPE_LOAD(QMetaType::QDateTime, QDateTime, wrapQDateTime);
    VALUE_TYPE_LOAD(QMetaType::QDate, QDate, wrapQDate);
    VALUE_TYPE_LOAD(QMetaType::QTime, QTime, wrapQTime);
#if QT_CONFIG(regularexpression)
    VALUE_TYPE_LOAD(QMetaType::QRegularExpression, QRegularExpression, engine->newRegExpObject);
#endif
    VALUE_TYPE_LOAD(QMetaType::QObjectStar, QObject*, wrapQObject);
    VALUE_TYPE_LOAD(QMetaType::QJsonValue, QJsonValue, wrapJsonValue);
    VALUE_TYPE_LOAD(QMetaType::QJsonObject, QJsonObject, wrapJsonObject);
    VALUE_TYPE_LOAD(QMetaType::QJsonArray, QJsonArray, wrapJsonArray);
#if QT_CONFIG(qml_locale)
    VALUE_TYPE_LOAD(QMetaType::QLocale, QLocale, wrapLocale);
#endif
    case QMetaType::QPixmap:
    case QMetaType::QImage: {
        QVariant v(metaType);
        void *args[] = { v.data(), nullptr };
        doStaticReadCall(metaObject, valueTypeWrapper, index, args);
        return Encode(engine->newVariantObject(metaType, v.data()));
    }
    case QMetaType::QVariant: {
        QVariant v;
        void *args[] = { &v, nullptr };
        doStaticReadCall(metaObject, valueTypeWrapper, index, args);
        return engine->fromVariant(
                    v, valueTypeWrapper, index,
                    referenceFlags(metaObject, index) | Heap::ReferenceObject::IsVariant);
    }
    default:
        break;
    }

    QVariant v(metaType);
    void *args[] = { v.data(), nullptr };
    doStaticReadCall(metaObject, valueTypeWrapper, index, args);
    return engine->fromVariant(v, valueTypeWrapper, index, referenceFlags(metaObject, index));
#undef VALUE_TYPE_LOAD
}

PropertyAttributes QQmlValueTypeWrapper::virtualGetOwnProperty(const Managed *m, PropertyKey id, Property *p)
{
    if (id.isString()) {
        const QQmlValueTypeWrapper *r = static_cast<const QQmlValueTypeWrapper *>(m);
        Q_ASSERT(r);

        const QQmlPropertyData result = r->dataForPropertyKey(id);
        if (!result.isValid())
            return Attr_Invalid; // Property doesn't exist. Object shouldn't meddle with it.

        if (!p)
            return Attr_Data; // Property exists, but we're not interested in the value

        if (!r->d()->isReference() || r->readReferenceValue()) {
            // Property exists, and we can retrieve it
            p->value = getGadgetProperty(
                        r->engine(), r->d(), result.propType(), result.coreIndex(),
                        result.isFunction(), result.isEnum());
        } else {
            // Property exists, but we can't retrieve it. Make it undefined.
            p->value = Encode::undefined();
        }

        return Attr_Data;
    }

    return QV4::Object::virtualGetOwnProperty(m, id, p);
}

struct QQmlValueTypeWrapperOwnPropertyKeyIterator : ObjectOwnPropertyKeyIterator
{
    int propertyIndex = 0;
    ~QQmlValueTypeWrapperOwnPropertyKeyIterator() override = default;
    PropertyKey next(const Object *o, Property *pd = nullptr, PropertyAttributes *attrs = nullptr) override;

};

PropertyKey QQmlValueTypeWrapperOwnPropertyKeyIterator::next(const Object *o, Property *pd, PropertyAttributes *attrs) {
    const QQmlValueTypeWrapper *that = static_cast<const QQmlValueTypeWrapper *>(o);

    if (that->d()->isReference() && !that->readReferenceValue())
        return PropertyKey::invalid();

    const QMetaObject *mo = that->d()->metaObject();
    // We don't return methods, ie. they are not visible when iterating
    const int propertyCount = mo->propertyCount();
    if (propertyIndex < propertyCount) {
        Scope scope(that->engine());
        QMetaProperty p = mo->property(propertyIndex); // TODO: Implement and use QBasicMetaProperty
        ScopedString propName(scope, that->engine()->newString(QString::fromUtf8(p.name())));
        ++propertyIndex;
        if (attrs)
            *attrs = QV4::Attr_Data;
        if (pd) {
            QQmlPropertyData data;
            data.load(p);
            pd->value = getGadgetProperty(that->engine(), that->d(), data.propType(), data.coreIndex(), data.isFunction(), data.isEnum());
        }
        return propName->toPropertyKey();
    }

    return ObjectOwnPropertyKeyIterator::next(o, pd, attrs);
}


OwnPropertyKeyIterator *QQmlValueTypeWrapper::virtualOwnPropertyKeys(const Object *m, Value *target)
{
    *target = *m;
    return new QQmlValueTypeWrapperOwnPropertyKeyIterator;
}

bool QQmlValueTypeWrapper::isEqual(const QVariant& value) const
{
    if (d()->isReference() && !readReferenceValue())
        return false;
    int id1 = value.metaType().id();
    QVariant v = d()->toVariant();
    int id2 = v.metaType().id();
    if (id1 != id2) {
        // conversions for weak comparison
        switch (id1) {
        case QMetaType::QPoint:
            if (id2 == QMetaType::QPointF)
                return value.value<QPointF>() == v.value<QPointF>();
            break;
        case QMetaType::QPointF:
            if (id2 == QMetaType::QPoint)
                return value.value<QPointF>() == v.value<QPointF>();
            break;
        case QMetaType::QRect:
            if (id2 == QMetaType::QRectF)
                return value.value<QRectF>() == v.value<QRectF>();
            break;
        case QMetaType::QRectF:
            if (id2 == QMetaType::QRect)
                return value.value<QRectF>() == v.value<QRectF>();
            break;
        case QMetaType::QLine:
            if (id2 == QMetaType::QLineF)
                return value.value<QLineF>() == v.value<QLineF>();
            break;
        case QMetaType::QLineF:
            if (id2 == QMetaType::QLine)
                return value.value<QLineF>() == v.value<QLineF>();
            break;
        case QMetaType::QSize:
            if (id2 == QMetaType::QSizeF)
                return value.value<QSizeF>() == v.value<QSizeF>();
            break;
        case QMetaType::QSizeF:
            if (id2 == QMetaType::QSize)
                return value.value<QSizeF>() == v.value<QSizeF>();
            break;
        default:
            break;
        }
    }
    return (value == v);
}

int QQmlValueTypeWrapper::typeId() const
{
    return d()->metaType().id();
}

QMetaType QQmlValueTypeWrapper::type() const
{
    return d()->metaType();
}

bool QQmlValueTypeWrapper::write(QObject *target, int propertyIndex) const
{
    bool destructGadgetOnExit = false;
    Q_ALLOCA_DECLARE(void, gadget);
    if (d()->isReference()) {
        if (!d()->gadgetPtr()) {
            Q_ALLOCA_ASSIGN(void, gadget, d()->metaType().sizeOf());
            d()->setGadgetPtr(gadget);
            d()->metaType().construct(d()->gadgetPtr(), nullptr);
            destructGadgetOnExit = true;
        }
        if (!readReferenceValue())
            return false;
    }

    int flags = 0;
    int status = -1;
    void *a[] = { d()->gadgetPtr(), nullptr, &status, &flags };
    QMetaObject::metacall(target, QMetaObject::WriteProperty, propertyIndex, a);

    if (destructGadgetOnExit) {
        d()->metaType().destruct(d()->gadgetPtr());
        d()->setGadgetPtr(nullptr);
    }
    return true;
}

QQmlPropertyData QQmlValueTypeWrapper::dataForPropertyKey(PropertyKey id) const
{
    if (!id.isStringOrSymbol())
        return QQmlPropertyData {};
    QByteArray name = id.asStringOrSymbol()->toQString().toUtf8();
    const QMetaObject *mo = d()->metaObject();
    QQmlPropertyData result;
    QMetaMethod metaMethod = QMetaObjectPrivate::firstMethod(mo, name);
    if (metaMethod.isValid()) {
        result.load(metaMethod);
    } else {
        int propertyIndex = d()->metaObject()->indexOfProperty(name.constData());
        if (propertyIndex >= 0)
            result.load(mo->property(propertyIndex));
    }
    return result;
}

ReturnedValue QQmlValueTypeWrapper::method_toString(const FunctionObject *b, const Value *thisObject, const Value *, int)
{
    const Object *o = thisObject->as<Object>();
    if (!o)
        return b->engine()->throwTypeError();
    const QQmlValueTypeWrapper *w = o->as<QQmlValueTypeWrapper>();
    if (!w)
        return b->engine()->throwTypeError();

    if (w->d()->isReference() && !w->readReferenceValue())
        RETURN_UNDEFINED();

    QString result;
    if (!QMetaType::convert(w->d()->metaType(), w->d()->gadgetPtr(),
                            QMetaType(QMetaType::QString), &result)) {
        result = QString::fromUtf8(w->d()->metaType().name()) + QLatin1Char('(');
        const QMetaObject *mo = w->d()->metaObject();
        const int propCount = mo->propertyCount();
        for (int i = 0; i < propCount; ++i) {
            if (mo->property(i).isDesignable()) {
                QVariant value = mo->property(i).readOnGadget(w->d()->gadgetPtr());
                if (i > 0)
                    result += QLatin1String(", ");
                result += value.toString();
            }
        }
        result += QLatin1Char(')');
    }
    return Encode(b->engine()->newString(result));
}

ReturnedValue QQmlValueTypeWrapper::virtualResolveLookupGetter(const Object *object, ExecutionEngine *engine,
                                                               Lookup *lookup)
{
    PropertyKey id = engine->identifierTable->asPropertyKey(engine->currentStackFrame->v4Function->compilationUnit->
                                                            runtimeStrings[lookup->nameIndex]);
    if (!id.isString())
        return Object::virtualResolveLookupGetter(object, engine, lookup);

    const QQmlValueTypeWrapper *r = static_cast<const QQmlValueTypeWrapper *>(object);
    QV4::ExecutionEngine *v4 = r->engine();
    Scope scope(v4);
    ScopedString name(scope, id.asStringOrSymbol());

    // Note: readReferenceValue() can change the reference->type.
    if (r->d()->isReference() && !r->readReferenceValue())
        return Value::undefinedValue().asReturnedValue();

    QQmlPropertyData result = r->dataForPropertyKey(id);
    if (!result.isValid())
        return QV4::Object::virtualResolveLookupGetter(object, engine, lookup);

    lookup->qgadgetLookup.ic = r->internalClass();
    // & 1 to tell the gc that this is not heap allocated; see markObjects in qv4lookup_p.h
    lookup->qgadgetLookup.metaObject = quintptr(r->d()->metaObject()) + 1;
    lookup->qgadgetLookup.metaType = result.propType().iface();
    lookup->qgadgetLookup.coreIndex = result.coreIndex();
    lookup->qgadgetLookup.isFunction = result.isFunction();
    lookup->qgadgetLookup.isEnum = result.isEnum();
    lookup->getter = QQmlValueTypeWrapper::lookupGetter;
    return lookup->getter(lookup, engine, *object);
}

ReturnedValue QQmlValueTypeWrapper::lookupGetter(Lookup *lookup, ExecutionEngine *engine, const Value &object)
{
    const auto revertLookup = [lookup, engine, &object]() {
        lookup->qgadgetLookup.metaObject = quintptr(0);
        lookup->getter = Lookup::getterGeneric;
        return Lookup::getterGeneric(lookup, engine, object);
    };

    // we can safely cast to a QV4::Object here. If object is something else,
    // the internal class won't match
    Heap::Object *o = static_cast<Heap::Object *>(object.heapObject());
    if (!o || o->internalClass != lookup->qgadgetLookup.ic)
        return revertLookup();

    Heap::QQmlValueTypeWrapper *valueTypeWrapper =
            const_cast<Heap::QQmlValueTypeWrapper*>(static_cast<const Heap::QQmlValueTypeWrapper *>(o));
    if (valueTypeWrapper->metaObject() != reinterpret_cast<const QMetaObject *>(lookup->qgadgetLookup.metaObject - 1))
        return revertLookup();

    if (valueTypeWrapper->isReference() && !valueTypeWrapper->readReference())
        return Encode::undefined();

    return getGadgetProperty(
                engine, valueTypeWrapper, QMetaType(lookup->qgadgetLookup.metaType),
                lookup->qgadgetLookup.coreIndex, lookup->qgadgetLookup.isFunction,
                lookup->qgadgetLookup.isEnum);
}

bool QQmlValueTypeWrapper::lookupSetter(
        Lookup *l, ExecutionEngine *engine, Value &object, const Value &value)
{
    return QV4::Lookup::setterFallback(l, engine, object, value);
}

bool QQmlValueTypeWrapper::virtualResolveLookupSetter(Object *object, ExecutionEngine *engine, Lookup *lookup,
                                                      const Value &value)
{
    return Object::virtualResolveLookupSetter(object, engine, lookup, value);
}

ReturnedValue QQmlValueTypeWrapper::virtualGet(const Managed *m, PropertyKey id, const Value *receiver, bool *hasProperty)
{
    Q_ASSERT(m->as<QQmlValueTypeWrapper>());

    if (!id.isString())
        return Object::virtualGet(m, id, receiver, hasProperty);

    const QQmlValueTypeWrapper *r = static_cast<const QQmlValueTypeWrapper *>(m);
    QV4::ExecutionEngine *v4 = r->engine();

    // Note: readReferenceValue() can change the reference->type.
    if (r->d()->isReference() && !r->readReferenceValue())
        return Value::undefinedValue().asReturnedValue();

    QQmlPropertyData result = r->dataForPropertyKey(id);
    if (!result.isValid())
        return Object::virtualGet(m, id, receiver, hasProperty);

    if (hasProperty)
        *hasProperty = true;

    return getGadgetProperty(v4, r->d(), result.propType(), result.coreIndex(), result.isFunction(), result.isEnum());
}

bool QQmlValueTypeWrapper::virtualPut(Managed *m, PropertyKey id, const Value &value, Value *receiver)
{
    if (!id.isString())
        return Object::virtualPut(m, id, value, receiver);

    Q_ASSERT(m->as<QQmlValueTypeWrapper>());
    ExecutionEngine *v4 = static_cast<QQmlValueTypeWrapper *>(m)->engine();
    Scope scope(v4);
    if (scope.hasException())
        return false;

    Scoped<QQmlValueTypeWrapper> r(scope, static_cast<QQmlValueTypeWrapper *>(m));
    Heap::Object *heapObject = nullptr;
    if (r->d()->isReference()) {
        heapObject = r->d()->object();
        if (!r->readReferenceValue() || !r->d()->canWriteBack())
            return false;
    }

    const QMetaObject *metaObject = r->d()->metaObject();
    const QQmlPropertyData pd = r->dataForPropertyKey(id);
    if (!pd.isValid())
        return false;

    if (heapObject) {
        QObject *referenceObject = nullptr;
        QV4::ScopedFunctionObject f(scope, value);
        const int referencePropertyIndex = r->d()->property();
        QV4::Scoped<QV4::QObjectWrapper> o(scope, heapObject);
        if (o) {
            referenceObject = o->object();
        } else {
            QV4::Scoped<QV4::QQmlTypeWrapper> t(scope, heapObject);
            if (t)
                referenceObject = t->object();
        }

        if (f) {
            if (!f->isBinding()) {
                // assigning a JS function to a non-var-property is not allowed.
                QString error = QStringLiteral("Cannot assign JavaScript function to value-type property");
                ScopedString e(scope, v4->newString(error));
                v4->throwError(e);
                return false;
            }

            if (!referenceObject) {
                QString error = QStringLiteral("Cannot create binding on nested value type property");
                ScopedString e(scope, v4->newString(error));
                v4->throwError(e);
                return false;
            }

            const QMetaProperty writebackProperty
                    = referenceObject->metaObject()->property(referencePropertyIndex);
            const QMetaType writeBackPropertyType = writebackProperty.metaType();

            QQmlRefPointer<QQmlContextData> context = v4->callingQmlContext();

            QQmlPropertyData cacheData;
            cacheData.setWritable(true);
            cacheData.setPropType(writeBackPropertyType);
            cacheData.setCoreIndex(referencePropertyIndex);

            QV4::Scoped<QQmlBindingFunction> bindingFunction(scope, (const Value &)f);

            QV4::ScopedFunctionObject f(scope, bindingFunction->bindingFunction());
            QV4::ScopedContext ctx(scope, f->scope());
            QQmlBinding *newBinding = QQmlBinding::create(&cacheData, f->function(), referenceObject, context, ctx);
            newBinding->setSourceLocation(bindingFunction->currentLocation());
            if (f->isBoundFunction())
                newBinding->setBoundFunction(static_cast<QV4::BoundFunction *>(f.getPointer()));
            newBinding->setSourceLocation(bindingFunction->currentLocation());
            newBinding->setTarget(referenceObject, cacheData, &pd);
            QQmlPropertyPrivate::setBinding(newBinding);
            return true;
        } else if (referenceObject) {
            if (Q_UNLIKELY(lcBindingRemoval().isInfoEnabled())) {
                if (auto binding = QQmlPropertyPrivate::binding(referenceObject, QQmlPropertyIndex(referencePropertyIndex, pd.coreIndex()))) {
                    Q_ASSERT(binding->kind() == QQmlAbstractBinding::QmlBinding);
                    const auto qmlBinding = static_cast<const QQmlBinding*>(binding);
                    const auto stackFrame = v4->currentStackFrame;
                    qCInfo(lcBindingRemoval,
                           "Overwriting binding on %s::%s which was initially bound at %s by setting \"%s\" at %s:%d",
                           referenceObject->metaObject()->className(), referenceObject->metaObject()->property(referencePropertyIndex).name(),
                           qPrintable(qmlBinding->expressionIdentifier()),
                           metaObject->property(pd.coreIndex()).name(),
                           qPrintable(stackFrame->source()), stackFrame->lineNumber());
                }
            }
            QQmlPropertyPrivate::removeBinding(referenceObject, QQmlPropertyIndex(referencePropertyIndex, pd.coreIndex()));
        }
    }

    QMetaProperty property = metaObject->property(pd.coreIndex());
    Q_ASSERT(property.isValid());
    if (value.isUndefined() && pd.isResettable()) {
        property.resetOnGadget(reinterpret_cast<QObject *>(r->d()->gadgetPtr()));
        if (heapObject)
            r->d()->writeBack(pd.coreIndex());
        return true;
    }

    QVariant v = QV4::ExecutionEngine::toVariant(value, property.metaType());

    if (property.isEnumType() && (QMetaType::Type)v.userType() == QMetaType::Double)
        v = v.toInt();

    void *gadget = r->d()->gadgetPtr();
    property.writeOnGadget(gadget, std::move(v));

    if (heapObject)
        r->d()->writeBack(pd.coreIndex());

    return true;
}

} // namespace QV4

QT_END_NAMESPACE
