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
#include <QtCore/qloggingcategory.h>
#include <QtCore/QLine>
#include <QtCore/QLineF>
#include <QtCore/QSize>
#include <QtCore/QSizeF>

QT_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(lcBindingRemoval)

DEFINE_OBJECT_VTABLE(QV4::QQmlValueTypeWrapper);
DEFINE_OBJECT_VTABLE(QV4::QQmlValueTypeReference);

using namespace QV4;

void Heap::QQmlValueTypeWrapper::destroy()
{
    if (m_gadgetPtr) {
        m_valueType->metaType.destruct(m_gadgetPtr);
        ::operator delete(m_gadgetPtr);
    }
    Object::destroy();
}

void Heap::QQmlValueTypeWrapper::setData(const void *data) const
{
    if (auto *gadget = gadgetPtr())
        valueType()->metaType.destruct(gadget);
    if (!gadgetPtr())
        setGadgetPtr(::operator new(valueType()->metaType.sizeOf()));
    valueType()->metaType.construct(gadgetPtr(), data);
}

void Heap::QQmlValueTypeWrapper::setValue(const QVariant &value) const
{
    Q_ASSERT(valueType()->metaType.id() == value.userType());
    setData(value.constData());
}

QVariant Heap::QQmlValueTypeWrapper::toVariant() const
{
    Q_ASSERT(gadgetPtr());
    return QVariant(valueType()->metaType, gadgetPtr());
}


bool QQmlValueTypeReference::readReferenceValue() const
{
    if (!d()->object)
        return false;
    // A reference resource may be either a "true" reference (eg, to a QVector3D property)
    // or a "variant" reference (eg, to a QVariant property which happens to contain a value-type).
    QMetaProperty writebackProperty = d()->object->metaObject()->property(d()->property);
    if (writebackProperty.userType() == QMetaType::QVariant) {
        // variant-containing-value-type reference
        QVariant variantReferenceValue;

        void *a[] = { &variantReferenceValue, nullptr };
        QMetaObject::metacall(d()->object, QMetaObject::ReadProperty, d()->property, a);

        const QMetaType variantReferenceType = variantReferenceValue.metaType();
        if (variantReferenceType != type()) {
            // This is a stale VariantReference.  That is, the variant has been
            // overwritten with a different type in the meantime.
            // We need to modify this reference to the updated value type, if
            // possible, or return false if it is not a value type.
            if (QQmlMetaType::isValueType(variantReferenceType)) {
                const QMetaObject *mo = QQmlMetaType::metaObjectForValueType(variantReferenceType);
                if (d()->gadgetPtr()) {
                    d()->valueType()->metaType.destruct(d()->gadgetPtr());
                    ::operator delete(d()->gadgetPtr());
                }
                d()->setGadgetPtr(nullptr);
                d()->setMetaObject(mo);
                d()->setValueType(QQmlMetaType::valueType(variantReferenceType));
                if (!mo)
                    return false;
            } else {
                return false;
            }
        }
        d()->setValue(variantReferenceValue);
    } else {
        if (!d()->gadgetPtr()) {
            d()->setGadgetPtr(::operator new(d()->valueType()->metaType.sizeOf()));
            d()->valueType()->metaType.construct(d()->gadgetPtr(), nullptr);
        }
        // value-type reference
        void *args[] = { d()->gadgetPtr(), nullptr };
        QMetaObject::metacall(d()->object, QMetaObject::ReadProperty, d()->property, args);
    }
    return true;
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

ReturnedValue QQmlValueTypeWrapper::create(ExecutionEngine *engine, QObject *object, int property, const QMetaObject *metaObject, QMetaType type)
{
    Scope scope(engine);
    initProto(engine);

    Scoped<QQmlValueTypeReference> r(scope, engine->memoryManager->allocate<QQmlValueTypeReference>());
    r->d()->object = object;
    r->d()->property = property;
    r->d()->setMetaObject(metaObject);
    auto valueType = QQmlMetaType::valueType(type);
    if (!valueType) {
        return engine->throwTypeError(QLatin1String("Type %1 is not a value type")
                                      .arg(QString::fromUtf8(type.name())));
    }
    r->d()->setValueType(valueType);
    r->d()->setGadgetPtr(nullptr);
    return r->asReturnedValue();
}

ReturnedValue QQmlValueTypeWrapper::create(
        ExecutionEngine *engine, const QVariant &value, const QMetaObject *metaObject,
        QMetaType type)
{
    Q_ASSERT(value.metaType() == QQmlMetaType::valueType(type)->metaType);
    return create(engine, value.constData(), metaObject, type);
}

ReturnedValue QQmlValueTypeWrapper::create(
        ExecutionEngine *engine, const void *data, const QMetaObject *metaObject, QMetaType type)
{
    Scope scope(engine);
    initProto(engine);

    Scoped<QQmlValueTypeWrapper> r(scope, engine->memoryManager->allocate<QQmlValueTypeWrapper>());
    r->d()->setMetaObject(metaObject);
    auto valueType = QQmlMetaType::valueType(type);
    if (!valueType) {
        return engine->throwTypeError(QLatin1String("Type %1 is not a value type")
                                      .arg(QString::fromUtf8(type.name())));
    }
    r->d()->setValueType(valueType);
    r->d()->setGadgetPtr(nullptr);
    r->d()->setData(data);
    return r->asReturnedValue();
}

QVariant QQmlValueTypeWrapper::toVariant() const
{
    if (const QQmlValueTypeReference *ref = as<const QQmlValueTypeReference>())
        if (!ref->readReferenceValue())
            return QVariant();
    return d()->toVariant();
}

bool QQmlValueTypeWrapper::toGadget(void *data) const
{
    if (const QQmlValueTypeReference *ref = as<const QQmlValueTypeReference>())
        if (!ref->readReferenceValue())
            return false;
    const QMetaType type = d()->valueType()->metaType;
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

static ReturnedValue getGadgetProperty(ExecutionEngine *engine,
                                       Heap::QQmlValueTypeWrapper *valueTypeWrapper,
                                       QMetaType metaType, quint16 coreIndex, bool isFunction, bool isEnum)
{
    if (isFunction) {
        // calling a Q_INVOKABLE function of a value type
        return QV4::QObjectMethod::create(engine->rootContext(), valueTypeWrapper, coreIndex);
    }
#define VALUE_TYPE_LOAD(metatype, cpptype, constructor) \
    if (metaTypeId == metatype) { \
        cpptype v; \
        void *args[] = { &v, nullptr }; \
        metaObject->d.static_metacall(reinterpret_cast<QObject*>(valueTypeWrapper->gadgetPtr()), \
                                      QMetaObject::ReadProperty, index, args); \
        return QV4::Encode(constructor(v)); \
    }
    const QMetaObject *metaObject = valueTypeWrapper->metaObject();
    int index = coreIndex;
    QQmlMetaObject::resolveGadgetMethodOrPropertyIndex(QMetaObject::ReadProperty, &metaObject, &index);
    // These four types are the most common used by the value type wrappers
    int metaTypeId = metaType.id();
    VALUE_TYPE_LOAD(QMetaType::Double, double, double);
    VALUE_TYPE_LOAD(QMetaType::Float, float, float);
    VALUE_TYPE_LOAD(QMetaType::Int || isEnum, int, int);
    VALUE_TYPE_LOAD(QMetaType::QString, QString, engine->newString);
    VALUE_TYPE_LOAD(QMetaType::Bool, bool, bool);
    QVariant v;
    void *args[] = { nullptr, nullptr };
    if (metaType == QMetaType::fromType<QVariant>()) {
        args[0] = &v;
    } else {
        v = QVariant(metaType, static_cast<void *>(nullptr));
        args[0] = v.data();
    }
    metaObject->d.static_metacall(reinterpret_cast<QObject*>(valueTypeWrapper->gadgetPtr()), QMetaObject::ReadProperty,
                                  index, args);
    return engine->fromVariant(v);
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

        const QQmlValueTypeReference *ref = r->as<const QQmlValueTypeReference>();
        if (!ref || ref->readReferenceValue()) {
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

    if (const QQmlValueTypeReference *ref = that->as<QQmlValueTypeReference>()) {
        if (!ref->readReferenceValue())
            return PropertyKey::invalid();
    }

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
    if (const QQmlValueTypeReference *ref = as<const QQmlValueTypeReference>())
        if (!ref->readReferenceValue())
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
    return d()->valueType()->metaType.id();
}

QMetaType QQmlValueTypeWrapper::type() const
{
    return d()->valueType()->metaType;
}

bool QQmlValueTypeWrapper::write(QObject *target, int propertyIndex) const
{
    bool destructGadgetOnExit = false;
    Q_ALLOCA_DECLARE(void, gadget);
    if (const QQmlValueTypeReference *ref = as<const QQmlValueTypeReference>()) {
        if (!d()->gadgetPtr()) {
            Q_ALLOCA_ASSIGN(void, gadget, d()->valueType()->metaType.sizeOf());
            d()->setGadgetPtr(gadget);
            d()->valueType()->metaType.construct(d()->gadgetPtr(), nullptr);
            destructGadgetOnExit = true;
        }
        if (!ref->readReferenceValue())
            return false;
    }

    int flags = 0;
    int status = -1;
    void *a[] = { d()->gadgetPtr(), nullptr, &status, &flags };
    QMetaObject::metacall(target, QMetaObject::WriteProperty, propertyIndex, a);

    if (destructGadgetOnExit) {
        d()->valueType()->metaType.destruct(d()->gadgetPtr());
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

    if (const QQmlValueTypeReference *ref = w->as<QQmlValueTypeReference>())
        if (!ref->readReferenceValue())
            RETURN_UNDEFINED();

    QString result;
    if (!QMetaType::convert(w->d()->valueType()->metaType, w->d()->gadgetPtr(),
                            QMetaType(QMetaType::QString), &result)) {
        result = QString::fromUtf8(w->d()->valueType()->metaType.name()) + QLatin1Char('(');
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
    if (const QQmlValueTypeReference *reference = r->as<QQmlValueTypeReference>()) {
        if (!reference->readReferenceValue())
            return Value::undefinedValue().asReturnedValue();
    }

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

    if (lookup->qgadgetLookup.ic->vtable == QQmlValueTypeReference::staticVTable()) {
        Scope scope(engine);
        Scoped<QQmlValueTypeReference> referenceWrapper(scope, valueTypeWrapper);
        referenceWrapper->readReferenceValue();
    }

    return getGadgetProperty(engine, valueTypeWrapper, QMetaType(lookup->qgadgetLookup.metaType), lookup->qgadgetLookup.coreIndex, lookup->qgadgetLookup.isFunction, lookup->qgadgetLookup.isEnum);
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
    if (const QQmlValueTypeReference *reference = r->as<QQmlValueTypeReference>()) {
        if (!reference->readReferenceValue())
            return Value::undefinedValue().asReturnedValue();
    }

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
    Scoped<QQmlValueTypeReference> reference(scope, m->d());

    QMetaType writeBackPropertyType;

    if (reference) {
        QMetaProperty writebackProperty = reference->d()->object->metaObject()->property(reference->d()->property);

        if (!writebackProperty.isWritable() || !reference->readReferenceValue())
            return false;

        writeBackPropertyType = writebackProperty.metaType();
    }

    const QMetaObject *metaObject = r->d()->metaObject();
    const QQmlPropertyData pd = r->dataForPropertyKey(id);
    if (!pd.isValid())
        return false;

    if (reference) {
        QV4::ScopedFunctionObject f(scope, value);
        const QV4QPointer<QObject> &referenceObject = reference->d()->object;
        const int referencePropertyIndex = reference->d()->property;

        if (f) {
            if (!f->isBinding()) {
                // assigning a JS function to a non-var-property is not allowed.
                QString error = QStringLiteral("Cannot assign JavaScript function to value-type property");
                ScopedString e(scope, v4->newString(error));
                v4->throwError(e);
                return false;
            }

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
        } else {
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

    QVariant v = v4->toVariant(value, property.metaType());

    if (property.isEnumType() && (QMetaType::Type)v.userType() == QMetaType::Double)
        v = v.toInt();

    void *gadget = r->d()->gadgetPtr();
    property.writeOnGadget(gadget, v);

    if (reference)
        reference->d()->writeBack();

    return true;
}

QT_END_NAMESPACE
