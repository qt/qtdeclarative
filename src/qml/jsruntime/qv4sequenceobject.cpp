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

namespace QV4 {

DEFINE_OBJECT_VTABLE(Sequence);

static ReturnedValue doGetIndexed(const Sequence *s, qsizetype index) {
    QV4::Scope scope(s->engine());

    Heap::ReferenceObject::Flags flags =
            Heap::ReferenceObject::EnforcesLocation;
    if (s->d()->typePrivate()->extraData.ld->canSetValueAtIndex())
        flags |= Heap::ReferenceObject::CanWriteBack;
    if (Sequence::valueMetaType(s->d()) == QMetaType::fromType<QVariant>())
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

static const QMetaSequence *metaSequence(const Heap::Sequence *p)
{
    return p->typePrivate()->extraData.ld;
}

template<typename Compare>
void sortSequence(Sequence *sequence, const Compare &compare)
{
    auto *p = sequence->d();
    const auto *m = metaSequence(p);

    QSequentialIterable iterable(*m, p->typePrivate()->listId, p->storagePointer());
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

void Heap::Sequence::init(const QQmlType &qmlType, const void *container)
{
    ReferenceObject::init(nullptr, -1, NoFlag);

    Q_ASSERT(qmlType.isSequentialContainer());
    m_typePrivate = qmlType.priv();
    QQmlType::refHandle(m_typePrivate);

    m_container = m_typePrivate->listId.create(container);

    QV4::Scope scope(internalClass->engine);
    QV4::Scoped<QV4::Sequence> o(scope, this);
    o->setArrayType(Heap::ArrayData::Custom);
}

void Heap::Sequence::init(
        const QQmlType &qmlType, const void *container,
        Heap::Object *object, int propertyIndex, Heap::ReferenceObject::Flags flags)
{
    ReferenceObject::init(object, propertyIndex, flags);

    Q_ASSERT(qmlType.isSequentialContainer());
    m_typePrivate = qmlType.priv();
    QQmlType::refHandle(m_typePrivate);
    QV4::Scope scope(internalClass->engine);
    QV4::Scoped<QV4::Sequence> o(scope, this);
    o->setArrayType(Heap::ArrayData::Custom);
    if (CppStackFrame *frame = scope.engine->currentStackFrame)
        setLocation(frame->v4Function, frame->statementNumber());
    if (container)
        m_container = QMetaType(m_typePrivate->listId).create(container);
    else if (flags & EnforcesLocation)
        QV4::ReferenceObject::readReference(this);
}

Heap::Sequence *Heap::Sequence::detached() const
{
    return internalClass->engine->memoryManager->allocate<QV4::Sequence>(
                QQmlType(m_typePrivate), m_container);
}

void Heap::Sequence::destroy()
{
    if (m_container)
        m_typePrivate->listId.destroy(m_container);
    QQmlType::derefHandle(m_typePrivate);
    ReferenceObject::destroy();
}

void *Heap::Sequence::storagePointer()
{
    if (!m_container)
        m_container = m_typePrivate->listId.create();
    return m_container;
}

bool Heap::Sequence::setVariant(const QVariant &variant)
{
    const QMetaType variantReferenceType = variant.metaType();
    if (variantReferenceType != m_typePrivate->listId) {
        // This is a stale reference. That is, the property has been
        // overwritten with a different type in the meantime.
        // We need to modify this reference to the updated type, if
        // possible, or return false if it is not a sequence.
        const QQmlType newType = QQmlMetaType::qmlListType(variantReferenceType);
        if (newType.isSequentialContainer()) {
            if (m_container)
                m_typePrivate->listId.destroy(m_container);
            QQmlType::derefHandle(m_typePrivate);
            m_typePrivate = newType.priv();
            QQmlType::refHandle(m_typePrivate);
            m_container = m_typePrivate->listId.create(variant.constData());
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
    return QVariant(m_typePrivate->listId, m_container);
}

const QMetaType Sequence::valueMetaType(const Heap::Sequence *p)
{
    return p->typePrivate()->typeId;
}

qsizetype Sequence::size() const
{
    const auto *p = d();
    Q_ASSERT(p->storagePointer()); // Must readReference() before
    return metaSequence(p)->size(p->storagePointer());
}

QVariant Sequence::at(qsizetype index) const
{
    const auto *p = d();
    Q_ASSERT(p->storagePointer()); // Must readReference() before
    const QMetaType v = valueMetaType(p);
    QVariant result;
    if (v == QMetaType::fromType<QVariant>()) {
        metaSequence(p)->valueAtIndex(p->storagePointer(), index, &result);
    } else {
        result = QVariant(v);
        metaSequence(p)->valueAtIndex(p->storagePointer(), index, result.data());
    }
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
    convertAndDo(item, valueMetaType(p), [p](const void *data) {
        metaSequence(p)->addValueAtEnd(p->storagePointer(), data);
    });
}

void Sequence::append(qsizetype num, const QVariant &item)
{
    Heap::Sequence *p = d();
    convertAndDo(item, valueMetaType(p), [p, num](const void *data) {
        const QMetaSequence *m = metaSequence(p);
        void *container = p->storagePointer();
        for (qsizetype i = 0; i < num; ++i)
            m->addValueAtEnd(container, data);
    });
}

void Sequence::replace(qsizetype index, const QVariant &item)
{
    Heap::Sequence *p = d();
    convertAndDo(item, valueMetaType(p), [p, index](const void *data) {
        metaSequence(p)->setValueAtIndex(p->storagePointer(), index, data);
    });
}

void Sequence::removeLast(qsizetype num)
{
    auto *p = d();
    const auto *m = metaSequence(p);

    if (m->canEraseRangeAtIterator() && m->hasRandomAccessIterator() && num > 1) {
        void *i = m->end(p->storagePointer());
        m->advanceIterator(i, -num);
        void *j = m->end(p->storagePointer());
        m->eraseRangeAtIterator(p->storagePointer(), i, j);
        m->destroyIterator(i);
        m->destroyIterator(j);
    } else {
        for (int i = 0; i < num; ++i)
            m->removeValueAtEnd(p->storagePointer());
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
    const QMetaType valueType = valueMetaType(d());
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
        const QMetaType valueType = valueMetaType(sequence->d());
        if (sequence->d()->isReference() && !sequence->loadReference())
            return 0;
        const QMetaSequence *metaSequence = sequence->d()->typePrivate()->extraData.ld;
        if (metaSequence->valueMetaType() != valueType)
            return 0; // value metatype is not what the caller expects anymore.

        const void *storagePointer = sequence->d()->storagePointer();
        if (index < 0 || index >= metaSequence->size(storagePointer))
            return 0;
        metaSequence->valueAtIndex(storagePointer, index, a[0]);
        break;
    }
    case QMetaObject::WriteProperty: {
        void *storagePointer = sequence->d()->storagePointer();
        const QMetaSequence *metaSequence = sequence->d()->typePrivate()->extraData.ld;
        if (index < 0 || index >= metaSequence->size(storagePointer))
            return 0;
        metaSequence->setValueAtIndex(storagePointer, index, a[0]);
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
        const QMetaType valueMetaType = metaSequence(This->d())->valueMetaType();
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

ReturnedValue SequencePrototype::newSequence(
        QV4::ExecutionEngine *engine, QMetaType sequenceType, const void *data,
        Heap::Object *object, int propertyIndex, Heap::ReferenceObject::Flags flags)
{
    // This function is called when the property is a QObject Q_PROPERTY of
    // the given sequence type.  Internally we store a sequence
    // (as well as object ptr + property index for updated-read and write-back)
    // and so access/mutate avoids variant conversion.

    const QQmlType qmlType = QQmlMetaType::qmlListType(sequenceType);
    if (qmlType.isSequentialContainer()) {
        return engine->memoryManager->allocate<Sequence>(
                    qmlType, data,  object, propertyIndex, flags)->asReturnedValue();
    }

    return Encode::undefined();
}

ReturnedValue SequencePrototype::fromVariant(QV4::ExecutionEngine *engine, const QVariant &v)
{
    return fromData(engine, v.metaType(), v.constData());
}

ReturnedValue SequencePrototype::fromData(ExecutionEngine *engine, QMetaType type, const void *data)
{
    // This function is called when assigning a sequence value to a normal JS var
    // in a JS block.  Internally, we store a sequence of the specified type.
    // Access and mutation is extremely fast since it will not need to modify any
    // QObject property.

    const QQmlType qmlType = QQmlMetaType::qmlListType(type);
    if (qmlType.isSequentialContainer())
        return engine->memoryManager->allocate<Sequence>(qmlType, data)->asReturnedValue();

    return Encode::undefined();
}

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

    return QVariant(p->typePrivate()->listId, p->storagePointer());
}

QVariant SequencePrototype::toVariant(const QV4::Value &array, QMetaType typeHint)
{
    if (!array.as<ArrayObject>())
        return QVariant();

    QV4::Scope scope(array.as<Object>()->engine());
    QV4::ScopedArrayObject a(scope, array);

    const QQmlType type = QQmlMetaType::qmlListType(typeHint);
    if (type.isSequentialContainer()) {
        const QQmlTypePrivate *priv = type.priv();
        const QMetaSequence *meta = priv->extraData.ld;
        const QMetaType containerMetaType(priv->listId);
        QVariant result(containerMetaType);
        qint64 length = a->getLength();
        Q_ASSERT(length >= 0);
        Q_ASSERT(length <= qint64(std::numeric_limits<quint32>::max()));

        QV4::ScopedValue v(scope);
        for (quint32 i = 0; i < quint32(length); ++i) {
            const QMetaType valueMetaType = priv->typeId;
            QVariant variant = ExecutionEngine::toVariant(a->get(i), valueMetaType, false);
            if (valueMetaType == QMetaType::fromType<QVariant>()) {
                meta->addValueAtEnd(result.data(), &variant);
            } else {
                const QMetaType originalType = variant.metaType();
                if (originalType != valueMetaType) {
                    const QVariant converted = QQmlValueTypeProvider::createValueType(
                                variant, valueMetaType);
                    if (converted.isValid()) {
                        variant = converted;
                    } else if (!variant.convert(valueMetaType)) {
                        qWarning().noquote()
                                << QLatin1String("Could not convert array value "
                                                 "at position %1 from %2 to %3")
                                   .arg(QString::number(i),
                                        QString::fromUtf8(originalType.name()),
                                        QString::fromUtf8(valueMetaType.name()));
                        variant = QVariant(valueMetaType);
                    }
                }
                meta->addValueAtEnd(result.data(), variant.constData());
            }
        }
        return result;
    }

    return QVariant();
}

void *SequencePrototype::getRawContainerPtr(const Sequence *object, QMetaType typeHint)
{
    if (object->d()->typePrivate()->listId == typeHint)
        return object->getRawContainerPtr();
    return nullptr;
}

QMetaType SequencePrototype::metaTypeForSequence(const Sequence *object)
{
    return object->d()->typePrivate()->listId;
}

} // namespace QV4

QT_END_NAMESPACE

#include "moc_qv4sequenceobject_p.cpp"
