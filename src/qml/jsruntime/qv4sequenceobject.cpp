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

QT_BEGIN_NAMESPACE

Q_STATIC_LOGGING_CATEGORY(lcListValueConversion, "qt.qml.listvalueconversion")

namespace QV4 {

DEFINE_OBJECT_VTABLE(Sequence);

static ReturnedValue doGetIndexed(Heap::Sequence *p, qsizetype index)
{
    QV4::Scope scope(p->internalClass->engine);

    const QMetaType valueMetaType = p->valueMetaType();
    const QMetaSequence metaSequence = p->metaSequence();

    Heap::ReferenceObject::Flags flags = Heap::ReferenceObject::EnforcesLocation;
    if (metaSequence.canSetValueAtIndex())
        flags |= Heap::ReferenceObject::CanWriteBack;

    const void *container = p->storagePointer();
    Q_ASSERT(container); // Must readReference() before

    QVariant result;
    if (valueMetaType == QMetaType::fromType<QVariant>()) {
        flags |= Heap::ReferenceObject::IsVariant;
        metaSequence.valueAtIndex(container, index, &result);
    } else {
        result = QVariant(valueMetaType);
        metaSequence.valueAtIndex(container, index, result.data());
    }

    QV4::ScopedValue v(scope, scope.engine->fromVariant(result, p, index, flags));
    if (QQmlValueTypeWrapper *ref = v->as<QQmlValueTypeWrapper>()) {
        if (CppStackFrame *frame = scope.engine->currentStackFrame)
            ref->d()->setLocation(frame->v4Function, frame->statementNumber());
        // No need to read the reference. We've done that above already.
    }
    return v->asReturnedValue();
}

static void *createVariantData(QMetaType type, QVariant *variant)
{
    if (type == QMetaType::fromType<QVariant>())
        return variant;
    *variant = QVariant(type);
    return variant->data();
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

static qsizetype sizeInline(const Heap::Sequence *p)
{
    if (const void *container = p->storagePointer())
        return p->metaSequence().size(container);

    // It can be stored inline, and the container can still be nullptr.
    // This happens if we construct it from a nullptr in the first place and never update it.
    // It means it's empty.
    return 0;
}

struct SequenceOwnPropertyKeyIterator : ObjectOwnPropertyKeyIterator
{
    ~SequenceOwnPropertyKeyIterator() override = default;
    PropertyKey next(const Object *o, Property *pd = nullptr, PropertyAttributes *attrs = nullptr) override
    {
        Heap::Sequence *p = static_cast<const Sequence *>(o)->d();

        if (p->isReference() && !p->loadReference())
            return PropertyKey::invalid();

        const qsizetype size = sizeInline(p);
        if (size > 0 && qIsAtMostSizetypeLimit(arrayIndex, size - 1)) {
            const uint index = arrayIndex;
            ++arrayIndex;
            if (attrs)
                *attrs = QV4::Attr_Data;
            if (pd)
                pd->value = doGetIndexed(p, index);
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

void Heap::Sequence::initTypes(QMetaType listType, QMetaSequence metaSequence)
{
    m_listType = listType.iface();
    Q_ASSERT(m_listType);
    m_metaSequence = metaSequence.iface();
    Q_ASSERT(m_metaSequence);
}

void Heap::Sequence::init(QMetaType listType, QMetaSequence metaSequence, const void *container)
{
    ReferenceObject::init(nullptr, -1, NoFlag);
    initTypes(listType,  metaSequence);
    createInlineStorage(container);
}

void Heap::Sequence::init(
    QMetaType listType, QMetaSequence metaSequence, const void *container,
    Heap::Object *object, int propertyIndex, Heap::ReferenceObject::Flags flags)
{
    ReferenceObject::init(object, propertyIndex, flags);
    initTypes(listType, metaSequence);

    if (CppStackFrame *frame = internalClass->engine->currentStackFrame)
        setLocation(frame->v4Function, frame->statementNumber());
    createInlineStorage(container);
    if (!container && (flags & EnforcesLocation))
        QV4::ReferenceObject::readReference(this);
}

void Heap::Sequence::createInlineStorage(const void *container)
{
    QV4::Scope scope(internalClass->engine);
    QV4::Scoped<QV4::Sequence> o(scope, this);
    o->setArrayType(Heap::ArrayData::Custom);
    if (container)
        m_container = listType().create(container);
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

static void appendInline(Heap::Sequence *p, const QVariant &item)
{
    convertAndDo(item, p->valueMetaType(), [p](const void *data) {
        p->metaSequence().addValueAtEnd(p->storagePointer(), data);
    });
}

static void appendDefaultConstructedInline(Heap::Sequence *p, qsizetype num)
{
    QVariant item;
    const void *data = createVariantData(p->valueMetaType(), &item);
    const QMetaSequence m = p->metaSequence();
    void *container = p->storagePointer();
    for (qsizetype i = 0; i < num; ++i)
        m.addValueAtEnd(container, data);
}

static void replaceInline(Heap::Sequence *p, qsizetype index, const QVariant &item)
{
    convertAndDo(item, p->valueMetaType(), [p, index](const void *data) {
        p->metaSequence().setValueAtIndex(p->storagePointer(), index, data);
    });
}

static void removeLastInline(Heap::Sequence *p, qsizetype num)
{
    const QMetaSequence m = p->metaSequence();
    void *container = p->storagePointer();

    if (m.canEraseRangeAtIterator() && m.hasRandomAccessIterator() && num > 1) {
        void *i = m.end(container);
        m.advanceIterator(i, -num);
        void *j = m.end(container);
        m.eraseRangeAtIterator(container, i, j);
        m.destroyIterator(i);
        m.destroyIterator(j);
    } else {
        for (int i = 0; i < num; ++i)
            m.removeValueAtEnd(container);
    }
}

bool Heap::Sequence::loadReference()
{
    Q_ASSERT(isReference());
    // If locations are enforced we only read once
    return enforcesLocation() || QV4::ReferenceObject::readReference(this);
}

bool Heap::Sequence::storeReference()
{
    Q_ASSERT(isReference());
    return isAttachedToProperty() && QV4::ReferenceObject::writeBack(this);
}

ReturnedValue Sequence::virtualGet(const Managed *that, PropertyKey id, const Value *receiver, bool *hasProperty)
{
    if (!id.isArrayIndex())
        return ReferenceObject::virtualGet(that, id, receiver, hasProperty);

    const uint arrayIndex = id.asArrayIndex();
    if (!qIsAtMostSizetypeLimit(arrayIndex)) {
        generateWarning(that->engine(), QLatin1String("Index out of range during indexed get"));
        return false;
    }

    Heap::Sequence *p = static_cast<const Sequence *>(that)->d();

    if (p->isReference() && !p->loadReference())
        return Encode::undefined();

    const qsizetype index = arrayIndex;
    if (index < sizeInline(p)) {
        if (hasProperty)
            *hasProperty = true;
        return doGetIndexed(p, index);
    }

    if (hasProperty)
        *hasProperty = false;
    return Encode::undefined();
}

qint64 Sequence::virtualGetLength(const Managed *m)
{
    Heap::Sequence *p = static_cast<const Sequence *>(m)->d();
    if (p->isReference() && !p->loadReference())
        return 0;
    return sizeInline(p);
}

bool Sequence::virtualPut(Managed *that, PropertyKey id, const Value &value, Value *receiver)
{
    if (!id.isArrayIndex())
        return ReferenceObject::virtualPut(that, id, value, receiver);

    const uint arrayIndex = id.asArrayIndex();
    if (!qIsAtMostSizetypeLimit(arrayIndex)) {
        generateWarning(that->engine(), QLatin1String("Index out of range during indexed set"));
        return false;
    }

    Heap::Sequence *p = static_cast<Sequence *>(that)->d();
    if (p->internalClass->engine->hasException)
        return false;

    if (p->isReadOnly()) {
        p->internalClass->engine->throwTypeError(
                QLatin1String("Cannot insert into a readonly container"));
        return false;
    }

    if (p->isReference() && !p->loadReference())
        return false;

    const qsizetype index = arrayIndex;
    const qsizetype count = sizeInline(p);
    const QMetaType valueType = p->valueMetaType();
    const QVariant element = ExecutionEngine::toVariant(value, valueType, false);

    if (index == count) {
        appendInline(p, element);
    } else if (index < count) {
        replaceInline(p, index, element);
    } else {
        /* according to ECMA262r3 we need to insert */
        /* the value at the given index, increasing length to index+1. */
        appendDefaultConstructedInline(p, index - count);
        appendInline(p, element);
    }

    if (p->isReference())
        p->storeReference();
    return true;
}

bool Sequence::virtualDeleteProperty(Managed *that, PropertyKey id)
{
    if (!id.isArrayIndex())
        return ReferenceObject::virtualDeleteProperty(that, id);

    const uint arrayIndex = id.asArrayIndex();
    if (!qIsAtMostSizetypeLimit(arrayIndex)) {
        generateWarning(that->engine(), QLatin1String("Index out of range during indexed delete"));
        return false;
    }

    Heap::Sequence *p = static_cast<Sequence *>(that)->d();

    if (p->isReadOnly())
        return false;
    if (p->isReference() && !p->loadReference())
        return false;

    const qsizetype index = arrayIndex;
    if (index >= sizeInline(p))
        return false;

    /* according to ECMA262r3 it should be Undefined, */
    /* but we cannot, so we insert a default-value instead. */
    replaceInline(p, index, QVariant());

    if (p->isReference())
        p->storeReference();

    return true;
}

bool Sequence::virtualIsEqualTo(Managed *that, Managed *other)
{
    if (!other)
        return false;

    const Sequence *otherS = other->as<Sequence>();
    if (!otherS)
        return false;

    const Sequence *s = static_cast<Sequence *>(that);
    const Heap::Sequence *p = s->d();
    const Heap::Sequence *otherP = otherS->d();

    const Heap::Object *object = p->object();
    const Heap::Object *otherObject = otherP->object();

    if (object && otherObject)
        return object == otherObject && p->property() == otherP->property();

    if (!object && !otherObject)
        return s == otherS;

    return false;
}

OwnPropertyKeyIterator *Sequence::virtualOwnPropertyKeys(const Object *m, Value *target)
{
    *target = *m;
    return new SequenceOwnPropertyKeyIterator;
}

int Sequence::virtualMetacall(Object *object, QMetaObject::Call call, int index, void **a)
{
    Heap::Sequence *p = static_cast<Sequence *>(object)->d();
    Q_ASSERT(p);

    switch (call) {
    case QMetaObject::ReadProperty: {
        const QMetaType valueType = p->valueMetaType();
        if (p->isReference() && !p->loadReference())
            return 0;
        const QMetaSequence metaSequence = p->metaSequence();
        if (metaSequence.valueMetaType() != valueType)
            return 0; // value metatype is not what the caller expects anymore.

        const void *storagePointer = p->storagePointer();
        if (index < 0 || index >= metaSequence.size(storagePointer))
            return 0;
        metaSequence.valueAtIndex(storagePointer, index, a[0]);
        break;
    }
    case QMetaObject::WriteProperty: {
        void *storagePointer = p->storagePointer();
        const QMetaSequence metaSequence = p->metaSequence();
        if (index < 0 || index >= metaSequence.size(storagePointer))
            return 0;
        metaSequence.setValueAtIndex(storagePointer, index, a[0]);
        if (p->isReference())
            p->storeReference();
        break;
    }
    default:
        return 0; // not supported
    }

    return -1;
}

QV4::ReturnedValue SequencePrototype::method_getLength(
        const FunctionObject *b, const Value *thisObject, const Value *, int)
{
    QV4::Scope scope(b);
    QV4::Scoped<Sequence> This(scope, thisObject->as<Sequence>());
    if (!This)
        THROW_TYPE_ERROR();

    Heap::Sequence *p = This->d();

    if (p->isReference() && !p->loadReference())
        return Encode::undefined();

    const qsizetype size = sizeInline(p);
    if (qIsAtMostUintLimit(size))
        RETURN_RESULT(Encode(uint(size)));

    return scope.engine->throwRangeError(QLatin1String("Sequence length out of range"));
}

QV4::ReturnedValue SequencePrototype::method_setLength(
        const FunctionObject *f, const Value *thisObject, const Value *argv, int argc)
{
    QV4::Scope scope(f);
    QV4::Scoped<Sequence> This(scope, thisObject->as<Sequence>());
    if (!This)
        THROW_TYPE_ERROR();

    Heap::Sequence *p = This->d();

    bool ok = false;
    const quint32 argv0 = argc ? argv[0].asArrayLength(&ok) : 0;
    if (!ok || !qIsAtMostSizetypeLimit(argv0)) {
        generateWarning(scope.engine, QLatin1String("Index out of range during length set"));
        RETURN_UNDEFINED();
    }

    if (p->isReadOnly())
        THROW_TYPE_ERROR();

    const qsizetype newCount = qsizetype(argv0);

    /* Read the sequence from the QObject property if we're a reference */
    if (p->isReference() && !p->loadReference())
        RETURN_UNDEFINED();

    /* Determine whether we need to modify the sequence */
    const qsizetype count = sizeInline(p);
    if (newCount == count) {
        RETURN_UNDEFINED();
    } else if (newCount > count) {
        /* according to ECMA262r3 we need to insert */
        /* undefined values increasing length to newLength. */
        /* We cannot, so we insert default-values instead. */
        appendDefaultConstructedInline(p, newCount - count);
    } else {
        /* according to ECMA262r3 we need to remove */
        /* elements until the sequence is the required length. */
        Q_ASSERT(newCount < count);
        removeLastInline(p, count - newCount);
    }

    /* write back if required. */
    if (p->isReference())
        p->storeReference();

    RETURN_UNDEFINED();
}

void SequencePrototype::init()
{
    defineDefaultProperty(engine()->id_valueOf(), method_valueOf, 0);
    defineAccessorProperty(QStringLiteral("length"), method_getLength, method_setLength);
    defineDefaultProperty(QStringLiteral("shift"), method_shift, 0);
}

ReturnedValue SequencePrototype::method_valueOf(const FunctionObject *f, const Value *thisObject, const Value *, int)
{
    return Encode(thisObject->toString(f->engine()));
}

ReturnedValue SequencePrototype::method_shift(
        const FunctionObject *b, const Value *thisObject, const Value *argv, int argc)
{
    Scope scope(b);
    Scoped<Sequence> s(scope, thisObject);
    if (!s)
        return ArrayPrototype::method_shift(b, thisObject, argv, argc);

    Heap::Sequence *p = s->d();

    if (p->isReference() && !p->loadReference())
        RETURN_UNDEFINED();

    const qsizetype len = sizeInline(p);
    if (!len)
        RETURN_UNDEFINED();

    void *storage = p->storagePointer();
    Q_ASSERT(storage); // Must readReference() before
    const QMetaType v = p->valueMetaType();
    const QMetaSequence m = p->metaSequence();

    QVariant shifted;
    void *resultData = createVariantData(v, &shifted);
    m.valueAtIndex(storage, 0, resultData);

    if (m.canRemoveValueAtBegin()) {
        m.removeValueAtBegin(storage);
    } else {
        QVariant t;
        void *tData = createVariantData(v, &t);
        for (qsizetype i = 1, end = m.size(storage); i < end; ++i) {
            m.valueAtIndex(storage, i, tData);
            m.setValueAtIndex(storage, i - 1, tData);
        }
        m.removeValueAtEnd(storage);
    }

    if (p->isReference())
        p->storeReference();

    return scope.engine->fromVariant(shifted);
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
    Heap::Sequence *p = object->d();

    // Note: For historical reasons, we ignore the result of loadReference()
    //       here. This allows us to retain sequences whose objects have vaninshed
    //       as "var" properties. It comes at the price of potentially returning
    //       outdated data. This is the behavior sequences have always shown.
    if (p->isReference())
        p->loadReference();

    if (const void *storage = p->m_container)
        return QVariant(p->listType(), storage);

    return QVariant();
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

static Heap::Sequence *resolveHeapSequence(const Sequence *sequence, QMetaType typeHint)
{
    if (!sequence)
        return nullptr;
    Heap::Sequence *p = sequence->d();
    if (p->listType() != typeHint)
        return nullptr;
    return p;
}

void *SequencePrototype::rawContainerPtr(const Sequence *sequence, QMetaType typeHint)
{
    Heap::Sequence *p = resolveHeapSequence(sequence, typeHint);
    if (!p)
        return nullptr;

    return p->storagePointer();
}

SequencePrototype::RawCopyResult SequencePrototype::setRawContainer(
        Sequence *sequence, const void *container, QMetaType typeHint)
{
    Heap::Sequence *p = resolveHeapSequence(sequence, typeHint);
    if (!p)
        return TypeMismatch;

    if (typeHint.equals(p->m_container, container))
        return WasEqual;
    typeHint.destruct(p->m_container);
    typeHint.construct(p->storagePointer(), container);
    return Copied;
}

SequencePrototype::RawCopyResult SequencePrototype::getRawContainer(
        const Sequence *sequence, void *container, QMetaType typeHint)
{
    Heap::Sequence *p = resolveHeapSequence(sequence, typeHint);
    if (!p)
        return TypeMismatch;

    if (typeHint.equals(p->m_container, container))
        return WasEqual;
    typeHint.destruct(container);
    typeHint.construct(container, p->m_container);
    return Copied;
}

QMetaType SequencePrototype::metaTypeForSequence(const Sequence *object)
{
    return object->d()->listType();
}

} // namespace QV4

QT_END_NAMESPACE

#include "moc_qv4sequenceobject_p.cpp"
