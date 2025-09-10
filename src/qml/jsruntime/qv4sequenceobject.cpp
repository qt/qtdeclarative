// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtCore/qsequentialiterable.h>

#include "qv4sequenceobject_p.h"

#include <private/qv4functionobject_p.h>
#include <private/qv4arrayobject_p.h>
#include <private/qqmlengine_p.h>
#include <private/qv4scopedvalue_p.h>
#include <private/qv4jscall_p.h>
#include <private/qqmlmetatype_p.h>
#include <private/qqmltype_p_p.h>
#include <private/qqmlvaluetypewrapper_p.h>

#include <algorithm>

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(lcListValueConversion, "qt.qml.listvalueconversion")

namespace QV4 {

DEFINE_OBJECT_VTABLE(Sequence);

static ReturnedValue doGetIndexed(const Sequence *s, qsizetype index) {
    QV4::Scope scope(s->engine());

    Heap::ReferenceObject::Flags flags =
            Heap::ReferenceObject::EnforcesLocation;
    if (s->d()->metaSequence().canSetValueAtIndex())
        flags |= Heap::ReferenceObject::CanWriteBack;
    if (s->d()->valueMetaType() == QMetaType::fromType<QVariant>())
        flags |= Heap::ReferenceObject::IsVariant;

    QV4::ScopedValue v(scope, scope.engine->fromVariant(
                           s->at(index), s->d(), index, flags));
    if (QQmlValueTypeWrapper *ref = v->as<QQmlValueTypeWrapper>()) {
        if (CppStackFrame *frame = scope.engine->currentStackFrame)
            ref->d()->setLocation(frame->v4Function, frame->statementNumber());
        // No need to read the reference. at() has done that already.
    }
    return v->asReturnedValue();
}

template<typename Compare>
void sortSequence(Sequence *sequence, const Compare &compare)
{
    /* non-const */ Heap::Sequence *p = sequence->d();

    QSequentialIterable iterable(p->metaSequence(), p->listType(), p->storagePointer());
    if (iterable.canRandomAccessIterate()) {
        std::sort(QSequentialIterable::RandomAccessIterator(iterable.mutableBegin()),
                  QSequentialIterable::RandomAccessIterator(iterable.mutableEnd()),
                  compare);
    } else if (iterable.canReverseIterate()) {
        std::sort(QSequentialIterable::BidirectionalIterator(iterable.mutableBegin()),
                  QSequentialIterable::BidirectionalIterator(iterable.mutableEnd()),
                  compare);
    } else {
        qWarning() << "Container has no suitable iterator for sorting";
    }
}

// helper function to generate valid warnings if errors occur during sequence operations.
static void generateWarning(QV4::ExecutionEngine *v4, const QString& description)
{
    QQmlEngine *engine = v4->qmlEngine();
    if (!engine)
        return;
    QQmlError retn;
    retn.setDescription(description);

    QV4::CppStackFrame *stackFrame = v4->currentStackFrame;

    retn.setLine(stackFrame->lineNumber());
    retn.setUrl(QUrl(stackFrame->source()));
    QQmlEnginePrivate::warning(engine, retn);
}

struct SequenceOwnPropertyKeyIterator : ObjectOwnPropertyKeyIterator
{
    ~SequenceOwnPropertyKeyIterator() override = default;
    PropertyKey next(const Object *o, Property *pd = nullptr, PropertyAttributes *attrs = nullptr) override
    {
        const Sequence *s = static_cast<const Sequence *>(o);

        if (s->d()->isReference() && !s->loadReference())
            return PropertyKey::invalid();

        const qsizetype size = s->size();
        if (size > 0 && qIsAtMostSizetypeLimit(arrayIndex, size - 1)) {
            const uint index = arrayIndex;
            ++arrayIndex;
            if (attrs)
                *attrs = QV4::Attr_Data;
            if (pd)
                pd->value = doGetIndexed(s, index);
            return PropertyKey::fromArrayIndex(index);
        }

        if (memberIndex == 0) {
            ++memberIndex;
            return o->engine()->id_length()->propertyKey();
        }

        // You cannot add any own properties via the regular JavaScript interfaces.
        return PropertyKey::invalid();
    }
};

struct SequenceCompareFunctor
{
    SequenceCompareFunctor(QV4::ExecutionEngine *v4, const QV4::Value &compareFn)
        : m_v4(v4), m_compareFn(&compareFn)
    {}

    bool operator()(const QVariant &lhs, const QVariant &rhs)
    {
        QV4::Scope scope(m_v4);
        ScopedFunctionObject compare(scope, m_compareFn);
        if (!compare)
            return m_v4->throwTypeError();
        Value *argv = scope.alloc(2);
        argv[0] = m_v4->fromVariant(lhs);
        argv[1] = m_v4->fromVariant(rhs);
        QV4::ScopedValue result(scope, compare->call(m_v4->globalObject, argv, 2));
        if (scope.hasException())
            return false;
        return result->toNumber() < 0;
    }

private:
    QV4::ExecutionEngine *m_v4;
    const QV4::Value *m_compareFn;
};

struct SequenceDefaultCompareFunctor
{
    bool operator()(const QVariant &lhs, const QVariant &rhs)
    {
        return lhs.toString() < rhs.toString();
    }
};

void Heap::Sequence::initTypes(QMetaType listType, QMetaSequence metaSequence)
{
    m_listType = listType.iface();
    Q_ASSERT(m_listType);
    m_metaSequence = metaSequence.iface();
    Q_ASSERT(m_metaSequence);
    QV4::Scope scope(internalClass->engine);
    QV4::Scoped<QV4::Sequence> o(scope, this);
    o->setArrayType(Heap::ArrayData::Custom);
}

void Heap::Sequence::init(QMetaType listType, QMetaSequence metaSequence, const void *container)
{
    ReferenceObject::init(nullptr, -1, NoFlag);
    initTypes(listType,  metaSequence);
    m_container = listType.create(container);
}

void Heap::Sequence::init(
    QMetaType listType, QMetaSequence metaSequence, const void *container,
    Heap::Object *object, int propertyIndex, Heap::ReferenceObject::Flags flags)
{
    ReferenceObject::init(object, propertyIndex, flags);
    initTypes(listType, metaSequence);

    if (CppStackFrame *frame = internalClass->engine->currentStackFrame)
        setLocation(frame->v4Function, frame->statementNumber());
    if (container)
        m_container = listType.create(container);
    else if (flags & EnforcesLocation)
        QV4::ReferenceObject::readReference(this);
}

Heap::Sequence *Heap::Sequence::detached() const
{
    return internalClass->engine->memoryManager->allocate<QV4::Sequence>(
        QMetaType(m_listType), QMetaSequence(m_metaSequence), m_container);
}

void Heap::Sequence::destroy()
{
    if (m_container)
        listType().destroy(m_container);
    ReferenceObject::destroy();
}

void *Heap::Sequence::storagePointer()
{
    if (!m_container)
        m_container = listType().create();
    return m_container;
}

bool Heap::Sequence::setVariant(const QVariant &variant)
{
    const QMetaType variantReferenceType = variant.metaType();
    if (variantReferenceType != listType()) {
        // This is a stale reference. That is, the property has been
        // overwritten with a different type in the meantime.
        // We need to modify this reference to the updated type, if
        // possible, or return false if it is not a sequence.
        const QQmlType newType = QQmlMetaType::qmlListType(variantReferenceType);
        if (newType.isSequentialContainer()) {
            if (m_container)
                listType().destroy(m_container);
            m_listType = newType.qListTypeId().iface();
            m_metaSequence = newType.listMetaSequence().iface();
            m_container = listType().create(variant.constData());
            return true;
        } else {
            return false;
        }
    }
    if (m_container) {
        variantReferenceType.destruct(m_container);
        variantReferenceType.construct(m_container, variant.constData());
    } else {
        m_container = variantReferenceType.create(variant.constData());
    }
    return true;
}
QVariant Heap::Sequence::toVariant() const
{
    return QVariant(listType(), m_container);
}

qsizetype Sequence::size() const
{
    const auto *p = d();
    Q_ASSERT(p->storagePointer()); // Must readReference() before
    return p->metaSequence().size(p->storagePointer());
}

QVariant Sequence::at(qsizetype index) const
{
    const auto *p = d();
    Q_ASSERT(p->storagePointer()); // Must readReference() before
    const QMetaType v = p->valueMetaType();
    QVariant result;
    if (v == QMetaType::fromType<QVariant>()) {
        p->metaSequence().valueAtIndex(p->storagePointer(), index, &result);
    } else {
        result = QVariant(v);
        p->metaSequence().valueAtIndex(p->storagePointer(), index, result.data());
    }
    return result;
}

QVariant Sequence::shift()
{
    auto *p = d();
    void *storage = p->storagePointer();
    Q_ASSERT(storage); // Must readReference() before
    const QMetaType v = p->valueMetaType();
    const QMetaSequence m = p->metaSequence();

    const auto variantData = [&](QVariant *variant) -> void *{
        if (v == QMetaType::fromType<QVariant>())
            return variant;

        *variant = QVariant(v);
        return variant->data();
    };

    QVariant result;
    void *resultData = variantData(&result);
    m.valueAtIndex(storage, 0, resultData);

    if (m.canRemoveValueAtBegin()) {
        m.removeValueAtBegin(storage);
        return result;
    }

    QVariant t;
    void *tData = variantData(&t);
    for (qsizetype i = 1, end = m.size(storage); i < end; ++i) {
        m.valueAtIndex(storage, i, tData);
        m.setValueAtIndex(storage, i - 1, tData);
    }
    m.removeValueAtEnd(storage);

    return result;
}


template<typename Action>
void convertAndDo(const QVariant &item, const QMetaType v, Action action)
{
    if (item.metaType() == v) {
        action(item.constData());
    } else if (v == QMetaType::fromType<QVariant>()) {
        action(&item);
    } else {
        QVariant converted = item;
        if (!converted.convert(v))
            converted = QVariant(v);
        action(converted.constData());
    }
}

void Sequence::append(const QVariant &item)
{
    Heap::Sequence *p = d();
    convertAndDo(item, p->valueMetaType(), [p](const void *data) {
        p->metaSequence().addValueAtEnd(p->storagePointer(), data);
    });
}

void Sequence::append(qsizetype num, const QVariant &item)
{
    Heap::Sequence *p = d();
    convertAndDo(item, p->valueMetaType(), [p, num](const void *data) {
        const QMetaSequence m = p->metaSequence();
        void *container = p->storagePointer();
        for (qsizetype i = 0; i < num; ++i)
            m.addValueAtEnd(container, data);
    });
}

void Sequence::replace(qsizetype index, const QVariant &item)
{
    Heap::Sequence *p = d();
    convertAndDo(item, p->valueMetaType(), [p, index](const void *data) {
        p->metaSequence().setValueAtIndex(p->storagePointer(), index, data);
    });
}

void Sequence::removeLast(qsizetype num)
{
    auto *p = d();
    const QMetaSequence m = p->metaSequence();

    if (m.canEraseRangeAtIterator() && m.hasRandomAccessIterator() && num > 1) {
        void *i = m.end(p->storagePointer());
        m.advanceIterator(i, -num);
        void *j = m.end(p->storagePointer());
        m.eraseRangeAtIterator(p->storagePointer(), i, j);
        m.destroyIterator(i);
        m.destroyIterator(j);
    } else {
        for (int i = 0; i < num; ++i)
            m.removeValueAtEnd(p->storagePointer());
    }
}

ReturnedValue Sequence::containerGetIndexed(qsizetype index, bool *hasProperty) const
{
    if (d()->isReference() && !loadReference())
        return Encode::undefined();

    if (index >= 0 && index < size()) {
        if (hasProperty)
            *hasProperty = true;
        return doGetIndexed(this, index);
    }
    if (hasProperty)
        *hasProperty = false;
    return Encode::undefined();
}

bool Sequence::containerPutIndexed(qsizetype index, const Value &value)
{
    if (internalClass()->engine->hasException)
        return false;

    if (d()->isReadOnly()) {
        engine()->throwTypeError(QLatin1String("Cannot insert into a readonly container"));
        return false;
    }

    if (d()->isReference() && !loadReference())
        return false;

    const qsizetype count = size();
    const QMetaType valueType = d()->valueMetaType();
    const QVariant element = ExecutionEngine::toVariant(value, valueType, false);

    if (index < 0)
        return false;

    if (index == count) {
        append(element);
    } else if (index < count) {
        replace(index, element);
    } else {
        /* according to ECMA262r3 we need to insert */
        /* the value at the given index, increasing length to index+1. */
        append(index - count,
               valueType == QMetaType::fromType<QVariant>() ? QVariant() : QVariant(valueType));
        append(element);
    }

    if (d()->object())
        storeReference();
    return true;
}

SequenceOwnPropertyKeyIterator *containerOwnPropertyKeys(const Object *m, Value *target)
{
    *target = *m;
    return new SequenceOwnPropertyKeyIterator;
}

bool Sequence::containerDeleteIndexedProperty(qsizetype index)
{
    if (d()->isReadOnly())
        return false;
    if (d()->isReference() && !loadReference())
        return false;
    if (index < 0 || index >= size())
        return false;

    /* according to ECMA262r3 it should be Undefined, */
    /* but we cannot, so we insert a default-value instead. */
    replace(index, QVariant());

    if (d()->object())
        storeReference();

    return true;
}

bool Sequence::containerIsEqualTo(Managed *other)
{
    if (!other)
        return false;
    Sequence *otherSequence = other->as<Sequence>();
    if (!otherSequence)
        return false;
    if (d()->object() && otherSequence->d()->object()) {
        return d()->object() == otherSequence->d()->object()
                && d()->property() == otherSequence->d()->property();
    } else if (!d()->object() && !otherSequence->d()->object()) {
        return this == otherSequence;
    }
    return false;
}

bool Sequence::sort(const FunctionObject *f, const Value *, const Value *argv, int argc)
{
    if (d()->isReadOnly())
        return false;
    if (d()->isReference() && !loadReference())
        return false;

    if (argc == 1 && argv[0].as<FunctionObject>())
        sortSequence(this, SequenceCompareFunctor(f->engine(), argv[0]));
    else
        sortSequence(this, SequenceDefaultCompareFunctor());

    if (d()->object())
        storeReference();

    return true;
}

void *Sequence::getRawContainerPtr() const
{ return d()->storagePointer(); }

bool Sequence::loadReference() const
{
    Q_ASSERT(d()->object());
    // If locations are enforced we only read once
    return d()->enforcesLocation() || QV4::ReferenceObject::readReference(d());
}

bool Sequence::storeReference()
{
    Q_ASSERT(d()->object());
    return d()->isAttachedToProperty() && QV4::ReferenceObject::writeBack(d());
}

ReturnedValue Sequence::virtualGet(const Managed *that, PropertyKey id, const Value *receiver, bool *hasProperty)
{
    if (id.isArrayIndex()) {
        const uint index = id.asArrayIndex();
        if (qIsAtMostSizetypeLimit(index))
            return static_cast<const Sequence *>(that)->containerGetIndexed(qsizetype(index), hasProperty);

        generateWarning(that->engine(), QLatin1String("Index out of range during indexed get"));
        return false;
    }

    return Object::virtualGet(that, id, receiver, hasProperty);
}

qint64 Sequence::virtualGetLength(const Managed *m)
{
    const Sequence *s = static_cast<const Sequence *>(m);
    if (s->d()->isReference() && !s->loadReference())
        return 0;
    return s->size();
}

bool Sequence::virtualPut(Managed *that, PropertyKey id, const Value &value, Value *receiver)
{
    if (id.isArrayIndex()) {
        const uint index = id.asArrayIndex();
        if (qIsAtMostSizetypeLimit(index))
            return static_cast<Sequence *>(that)->containerPutIndexed(qsizetype(index), value);

        generateWarning(that->engine(), QLatin1String("Index out of range during indexed set"));
        return false;
    }
    return Object::virtualPut(that, id, value, receiver);
}

bool Sequence::virtualDeleteProperty(Managed *that, PropertyKey id)
{
    if (id.isArrayIndex()) {
        const uint index = id.asArrayIndex();
        if (qIsAtMostSizetypeLimit(index))
            return static_cast<Sequence *>(that)->containerDeleteIndexedProperty(qsizetype(index));

        generateWarning(that->engine(), QLatin1String("Index out of range during indexed delete"));
        return false;
    }
    return Object::virtualDeleteProperty(that, id);
}

bool Sequence::virtualIsEqualTo(Managed *that, Managed *other)
{
    return static_cast<Sequence *>(that)->containerIsEqualTo(other);
}

OwnPropertyKeyIterator *Sequence::virtualOwnPropertyKeys(const Object *m, Value *target)
{
    return containerOwnPropertyKeys(m, target);
}

int Sequence::virtualMetacall(Object *object, QMetaObject::Call call, int index, void **a)
{
    Sequence *sequence = static_cast<Sequence *>(object);
    Q_ASSERT(sequence);

    switch (call) {
    case QMetaObject::ReadProperty: {
        const QMetaType valueType = sequence->d()->valueMetaType();
        if (sequence->d()->isReference() && !sequence->loadReference())
            return 0;
        const QMetaSequence metaSequence = sequence->d()->metaSequence();
        if (metaSequence.valueMetaType() != valueType)
            return 0; // value metatype is not what the caller expects anymore.

        const void *storagePointer = sequence->d()->storagePointer();
        if (index < 0 || index >= metaSequence.size(storagePointer))
            return 0;
        metaSequence.valueAtIndex(storagePointer, index, a[0]);
        break;
    }
    case QMetaObject::WriteProperty: {
        void *storagePointer = sequence->d()->storagePointer();
        const QMetaSequence metaSequence = sequence->d()->metaSequence();
        if (index < 0 || index >= metaSequence.size(storagePointer))
            return 0;
        metaSequence.setValueAtIndex(storagePointer, index, a[0]);
        if (sequence->d()->isReference())
            sequence->storeReference();
        break;
    }
    default:
        return 0; // not supported
    }

    return -1;
}

static QV4::ReturnedValue method_get_length(const FunctionObject *b, const Value *thisObject, const Value *, int)
{
    QV4::Scope scope(b);
    QV4::Scoped<Sequence> This(scope, thisObject->as<Sequence>());
    if (!This)
        THROW_TYPE_ERROR();

    if (This->d()->isReference() && !This->loadReference())
        return Encode::undefined();

    const qsizetype size = This->size();
    if (qIsAtMostUintLimit(size))
        RETURN_RESULT(Encode(uint(size)));

    return scope.engine->throwRangeError(QLatin1String("Sequence length out of range"));
}

static QV4::ReturnedValue method_set_length(const FunctionObject *f, const Value *thisObject, const Value *argv, int argc)
{
    QV4::Scope scope(f);
    QV4::Scoped<Sequence> This(scope, thisObject->as<Sequence>());
    if (!This)
        THROW_TYPE_ERROR();

    bool ok = false;
    const quint32 argv0 = argc ? argv[0].asArrayLength(&ok) : 0;
    if (!ok || !qIsAtMostSizetypeLimit(argv0)) {
        generateWarning(scope.engine, QLatin1String("Index out of range during length set"));
        RETURN_UNDEFINED();
    }

    if (This->d()->isReadOnly())
        THROW_TYPE_ERROR();

    const qsizetype newCount = qsizetype(argv0);

    /* Read the sequence from the QObject property if we're a reference */
    if (This->d()->isReference() && !This->loadReference())
        RETURN_UNDEFINED();

    /* Determine whether we need to modify the sequence */
    const qsizetype count = This->size();
    if (newCount == count) {
        RETURN_UNDEFINED();
    } else if (newCount > count) {
        const QMetaType valueMetaType = This->d()->valueMetaType();
        /* according to ECMA262r3 we need to insert */
        /* undefined values increasing length to newLength. */
        /* We cannot, so we insert default-values instead. */
        This->append(newCount - count, QVariant(valueMetaType));
    } else {
        /* according to ECMA262r3 we need to remove */
        /* elements until the sequence is the required length. */
        Q_ASSERT(newCount < count);
        This->removeLast(count - newCount);
    }

    /* write back if required. */
    if (This->d()->object())
        This->storeReference();

    RETURN_UNDEFINED();
}

void SequencePrototype::init()
{
    defineDefaultProperty(QStringLiteral("sort"), method_sort, 1);
    defineDefaultProperty(engine()->id_valueOf(), method_valueOf, 0);
    defineAccessorProperty(QStringLiteral("length"), method_get_length, method_set_length);
    defineDefaultProperty(QStringLiteral("shift"), method_shift, 0);
}

ReturnedValue SequencePrototype::method_valueOf(const FunctionObject *f, const Value *thisObject, const Value *, int)
{
    return Encode(thisObject->toString(f->engine()));
}

ReturnedValue SequencePrototype::method_sort(const FunctionObject *b, const Value *thisObject, const Value *argv, int argc)
{
    Scope scope(b);
    QV4::ScopedObject o(scope, thisObject);
    if (!o || !o->isV4SequenceType())
        THROW_TYPE_ERROR();

    if (argc >= 2)
        return o.asReturnedValue();

    if (auto *s = o->as<Sequence>()) {
        if (!s->sort(b, thisObject, argv, argc))
            THROW_TYPE_ERROR();
    }

    return o.asReturnedValue();
}

ReturnedValue SequencePrototype::method_shift(
        const FunctionObject *b, const Value *thisObject, const Value *argv, int argc)
{
    Scope scope(b);
    Scoped<Sequence> s(scope, thisObject);
    if (!s)
        return ArrayPrototype::method_shift(b, thisObject, argv, argc);

    if (s->d()->isReference() && !s->loadReference())
        RETURN_UNDEFINED();

    const qsizetype len = s->size();
    if (!len)
        RETURN_UNDEFINED();

    ScopedValue result(scope, scope.engine->fromVariant(s->shift()));

    if (s->d()->object())
        s->storeReference();

    return result->asReturnedValue();
}

ReturnedValue SequencePrototype::newSequence(
    QV4::ExecutionEngine *engine, QMetaType type, QMetaSequence metaSequence, const void *data,
    Heap::Object *object, int propertyIndex, Heap::ReferenceObject::Flags flags)
{
    // This function is called when the property is a QObject Q_PROPERTY of
    // the given sequence type.  Internally we store a sequence
    // (as well as object ptr + property index for updated-read and write-back)
    // and so access/mutate avoids variant conversion.

    return engine->memoryManager->allocate<Sequence>(
                type, metaSequence, data, object, propertyIndex, flags)->asReturnedValue();
}

ReturnedValue SequencePrototype::fromVariant(QV4::ExecutionEngine *engine, const QVariant &v)
{
    const QMetaType type = v.metaType();
    const QQmlType qmlType = QQmlMetaType::qmlListType(type);
    if (qmlType.isSequentialContainer())
        return fromData(engine, type, qmlType.listMetaSequence(), v.constData());

    QSequentialIterable iterable;
    if (QMetaType::convert(
            type, v.constData(), QMetaType::fromType<QSequentialIterable>(), &iterable)) {
        return fromData(engine, type, iterable.metaContainer(), v.constData());
    }

    return Encode::undefined();
}

ReturnedValue SequencePrototype::fromData(
    ExecutionEngine *engine, QMetaType type, QMetaSequence metaSequence, const void *data)
{
    // This function is called when assigning a sequence value to a normal JS var
    // in a JS block.  Internally, we store a sequence of the specified type.
    // Access and mutation is extremely fast since it will not need to modify any
    // QObject property.

    return engine->memoryManager->allocate<Sequence>(type, metaSequence, data)->asReturnedValue();
}

/*!
 * \internal
 *
 * Produce a QVariant containing a copy of the list stored in \a object.
 */
QVariant SequencePrototype::toVariant(const Sequence *object)
{
    Q_ASSERT(object->isV4SequenceType());
    const auto *p = object->d();

    // Note: For historical reasons, we ignore the result of loadReference()
    //       here. This allows us to retain sequences whose objects have vaninshed
    //       as "var" properties. It comes at the price of potentially returning
    //       outdated data. This is the behavior sequences have always shown.
    if (p->isReference())
        object->loadReference();
    if (!p->hasData())
        return QVariant();

    return QVariant(p->listType(), p->storagePointer());
}

bool convertToIterable(QMetaType metaType, void *data, QV4::Object *sequence)
{
    QSequentialIterable iterable;
    if (!QMetaType::view(metaType, data, QMetaType::fromType<QSequentialIterable>(), &iterable))
        return false;

    const QMetaType elementMetaType = iterable.valueMetaType();
    QV4::Scope scope(sequence->engine());
    QV4::ScopedValue v(scope);
    for (qsizetype i = 0, end = sequence->getLength(); i < end; ++i) {
        QVariant element(elementMetaType);
        v = sequence->get(i);
        ExecutionEngine::metaTypeFromJS(v, elementMetaType, element.data());
        iterable.addValue(element, QSequentialIterable::AtEnd);
    }
    return true;
}

/*!
 * \internal
 *
 * Convert the Object \a array to \a targetType. If \a targetType is not a sequential container,
 * or if \a array is not an Object, return an invalid QVariant. Otherwise return a QVariant of
 * \a targetType with the converted data. It is assumed that this actual requires a conversion. If
 * the conversion is not needed, the single-argument toVariant() is faster.
 */
QVariant SequencePrototype::toVariant(const QV4::Value &array, QMetaType targetType)
{
    if (!array.as<Object>())
        return QVariant();

    QMetaSequence meta;
    QVariant result(targetType);
    if (const QQmlType type = QQmlMetaType::qmlListType(targetType);
            type.isSequentialContainer()) {
        // If the QML type declares a custom sequential container, use that.
        meta = type.priv()->extraData.sequentialContainerTypeData;
    } else if (QSequentialIterable iterable;
            QMetaType::view(targetType, result.data(), QMetaType::fromType<QSequentialIterable>(),
                            &iterable)) {
        // Otherwise try to convert to QSequentialIterable via QMetaType conversion.
        meta = iterable.metaContainer();
    }

    if (!meta.canAddValue())
        return QVariant();

    QV4::Scope scope(array.as<Object>()->engine());
    QV4::ScopedObject a(scope, array);

    const QMetaType valueMetaType = meta.valueMetaType();
    const qint64 length = a->getLength();
    Q_ASSERT(length >= 0);
    Q_ASSERT(length <= qint64(std::numeric_limits<quint32>::max()));

    QV4::ScopedValue v(scope);
    for (quint32 i = 0; i < quint32(length); ++i) {
        QVariant variant;
        QV4::ScopedValue element(scope, a->get(i));

        // Note: We can convert to any sequence type here, even those that don't have a specified
        //       order. Therefore the meta.addValue() below. meta.addValue() preferably adds to the
        //       end, but will also add in other places if that's not possible.

        // If the target type is QVariant itself, let the conversion produce any interanl type.
        if (valueMetaType == QMetaType::fromType<QVariant>()) {
            variant = ExecutionEngine::toVariant(element, QMetaType(), false);
            meta.addValue(result.data(), &variant);
            continue;
        }

        // Try to convert to the specific type requested ...
        variant = QVariant(valueMetaType);
        if (ExecutionEngine::metaTypeFromJS(element, valueMetaType, variant.data())) {
            meta.addValue(result.data(), variant.constData());
            continue;
        }

        // If that doesn't work, produce any QVariant and try to convert it afterwards.
        // toVariant() is free to ignore the type hint and can produce the "original" type.
        variant = ExecutionEngine::toVariant(element, valueMetaType, false);
        const QMetaType originalType = variant.metaType();

        // Try value type constructors.
        const QVariant converted = QQmlValueTypeProvider::createValueType(
                variant, valueMetaType, scope.engine);
        if (converted.isValid()) {
            meta.addValue(result.data(), converted.constData());
            continue;
        }

        const auto warn = [&](QLatin1String base) {
            // If the original type was void, we're converting a "hole" in a sparse
            // array. There is no point in warning about that.
            if (!originalType.isValid())
                return;

            qCWarning(lcListValueConversion).noquote()
                    << base.arg(QString::number(i), QString::fromUtf8(originalType.name()),
                                QString::fromUtf8(valueMetaType.name()));
        };

        // Note: Ideally you should use constructible value types for everything below, but we'd
        //       probably get a lot of pushback for warning about that.

        // Before attempting a conversion from the concrete types, check if there exists a
        // conversion from QJSValue to the result type. Prefer that one for compatibility reasons.
        // This is a rather surprising "feature". Therefore, warn if a concrete conversion wouldn't
        // be possible. You should at least make your type conversions consistent.
        if (QMetaType::canConvert(QMetaType::fromType<QJSValue>(), valueMetaType)) {
            QVariant wrappedJsValue = QVariant::fromValue(QJSValuePrivate::fromReturnedValue(
                    element->asReturnedValue()));
            if (wrappedJsValue.convert(valueMetaType)) {
                if (!QMetaType::canConvert(originalType, valueMetaType)) {
                    warn(QLatin1String("Converting array value at position %1 from %2 to %3 via "
                                       "QJSValue even though they are not directly convertible"));
                }
                meta.addValue(result.data(), wrappedJsValue.constData());
                continue;
            }
        }

        // Last ditch effort: Try QVariant::convert()
        if (!variant.convert(valueMetaType))
            warn(QLatin1String("Could not convert array value at position %1 from %2 to %3"));

        meta.addValue(result.data(), variant.constData());
    }
    return result;

}

void *SequencePrototype::getRawContainerPtr(const Sequence *object, QMetaType typeHint)
{
    if (object->d()->listType() == typeHint)
        return object->getRawContainerPtr();
    return nullptr;
}

QMetaType SequencePrototype::metaTypeForSequence(const Sequence *object)
{
    return object->d()->listType();
}

} // namespace QV4

QT_END_NAMESPACE

#include "moc_qv4sequenceobject_p.cpp"
