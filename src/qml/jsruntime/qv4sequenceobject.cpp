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

#include <QtCore/qsequentialiterable.h>

#include "qv4sequenceobject_p.h"

#include <private/qv4functionobject_p.h>
#include <private/qv4arrayobject_p.h>
#include <private/qqmlengine_p.h>
#include <private/qv4scopedvalue_p.h>
#include <private/qv4jscall_p.h>
#include "qv4runtime_p.h"
#include "qv4objectiterator_p.h"
#include <private/qqmlmetatype_p.h>
#include <private/qqmltype_p_p.h>

#include <algorithm>

QT_BEGIN_NAMESPACE

using namespace QV4;

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

namespace QV4 {

namespace Heap {

struct QV4Sequence : Object {
    void init(const QQmlType &qmlType, const void *container);
    void init(QObject *object, int propertyIndex, const QQmlType &qmlType, bool readOnly);
    void destroy() {
        typePrivate->typeId.destroy(container);
        QQmlType::derefHandle(typePrivate);
        object.destroy();
        Object::destroy();
    }

    mutable void *container;
    const QQmlTypePrivate *typePrivate;
    QV4QPointer<QObject> object;
    int propertyIndex;
    bool isReference : 1;
    bool isReadOnly : 1;
};

}

struct QV4Sequence : public QV4::Object
{
    V4_OBJECT2(QV4Sequence, QV4::Object)
    Q_MANAGED_TYPE(V4Sequence)
    V4_PROTOTYPE(sequencePrototype)
    V4_NEEDS_DESTROY
public:

    static const QMetaSequence *meta(const Heap::QV4Sequence *p)
    {
        return p->typePrivate->extraData.ld;
    }

    qsizetype size() const
    {
        const auto *p = d();
        return meta(p)->size(p->container);
    }

    QVariant at(int index) const
    {
        const auto *p = d();
        const auto *m = meta(p);
        QVariant result(m->valueMetaType());
        m->valueAtIndex(p->container, index, result.data());
        return result;
    }

    void append(const QVariant &item)
    {
        const auto *p = d();
        const auto *m = meta(p);
        if (item.metaType() == m->valueMetaType()) {
            m->addValueAtEnd(p->container, item.constData());
        } else {
            QVariant converted = item;
            if (!converted.convert(m->valueMetaType()))
                converted = QVariant(m->valueMetaType());
            m->addValueAtEnd(p->container, converted.constData());
        }
    }

    void replace(int index, const QVariant &item)
    {
        const auto *p = d();
        const auto *m = meta(p);
        if (item.metaType() == m->valueMetaType()) {
            m->setValueAtIndex(p->container, index, item.constData());
        } else {
            QVariant converted = item;
            if (!converted.convert(m->valueMetaType()))
                converted = QVariant(m->valueMetaType());
            m->setValueAtIndex(p->container, index, converted.constData());
        }
    }

    template<typename Compare>
    void sort(const Compare &compare)
    {
        const auto *p = d();
        const auto *m = meta(p);

        QSequentialIterable iterable(*m, p->typePrivate->typeId, p->container);
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

    void removeLast(int num)
    {
        const auto *p = d();
        const auto *m = meta(p);

        if (m->canEraseRangeAtIterator() && m->hasRandomAccessIterator() && num > 1) {
            void *i = m->end(p->container);
            m->advanceIterator(i, -num);
            void *j = m->end(p->container);
            m->eraseRangeAtIterator(p->container, i, j);
            m->destroyIterator(i);
            m->destroyIterator(j);
        } else {
            for (int i = 0; i < num; ++i)
                m->removeValueAtEnd(p->container);
        }
    }

    QVariant toVariant()
    {
        const auto *p = d();
        return QVariant(p->typePrivate->typeId, p->container);
    }

    void init()
    {
        defineAccessorProperty(QStringLiteral("length"), method_get_length, method_set_length);
    }

    QV4::ReturnedValue containerGetIndexed(uint index, bool *hasProperty) const
    {
        /* Qt containers have int (rather than uint) allowable indexes. */
        if (index > INT_MAX) {
            generateWarning(engine(), QLatin1String("Index out of range during indexed get"));
            if (hasProperty)
                *hasProperty = false;
            return Encode::undefined();
        }
        if (d()->isReference) {
            if (!d()->object) {
                if (hasProperty)
                    *hasProperty = false;
                return Encode::undefined();
            }
            loadReference();
        }
        if (index < quint32(size())) {
            if (hasProperty)
                *hasProperty = true;
            return engine()->fromVariant(at(index));
        }
        if (hasProperty)
            *hasProperty = false;
        return Encode::undefined();
    }

    bool containerPutIndexed(uint index, const QV4::Value &value)
    {
        if (internalClass()->engine->hasException)
            return false;

        /* Qt containers have int (rather than uint) allowable indexes. */
        if (index > INT_MAX) {
            generateWarning(engine(), QLatin1String("Index out of range during indexed set"));
            return false;
        }

        if (d()->isReadOnly) {
            engine()->throwTypeError(QLatin1String("Cannot insert into a readonly container"));
            return false;
        }

        if (d()->isReference) {
            if (!d()->object)
                return false;
            loadReference();
        }

        quint32 count = quint32(size());
        const QMetaType valueMetaType = meta(d())->valueMetaType();
        const QVariant element = engine()->toVariant(value, valueMetaType, false);

        if (index == count) {
            append(element);
        } else if (index < count) {
            replace(index, element);
        } else {
            /* according to ECMA262r3 we need to insert */
            /* the value at the given index, increasing length to index+1. */
            while (index > count++)
                append(QVariant(valueMetaType));
            append(element);
        }

        if (d()->isReference)
            storeReference();
        return true;
    }

    QV4::PropertyAttributes containerQueryIndexed(uint index) const
    {
        /* Qt containers have int (rather than uint) allowable indexes. */
        if (index > INT_MAX) {
            generateWarning(engine(), QLatin1String("Index out of range during indexed query"));
            return QV4::Attr_Invalid;
        }
        if (d()->isReference) {
            if (!d()->object)
                return QV4::Attr_Invalid;
            loadReference();
        }
        return (index < quint32(size())) ? QV4::Attr_Data : QV4::Attr_Invalid;
    }

    struct OwnPropertyKeyIterator : ObjectOwnPropertyKeyIterator
    {
        ~OwnPropertyKeyIterator() override = default;
        PropertyKey next(const Object *o, Property *pd = nullptr, PropertyAttributes *attrs = nullptr) override
        {
            const QV4Sequence *s = static_cast<const QV4Sequence *>(o);

            if (s->d()->isReference) {
                if (!s->d()->object)
                    return ObjectOwnPropertyKeyIterator::next(o, pd, attrs);
                s->loadReference();
            }

            if (arrayIndex < quint32(s->size())) {
                uint index = arrayIndex;
                ++arrayIndex;
                if (attrs)
                    *attrs = QV4::Attr_Data;
                if (pd)
                    pd->value = s->engine()->fromVariant(s->at(index));
                return PropertyKey::fromArrayIndex(index);
            }

            return ObjectOwnPropertyKeyIterator::next(o, pd, attrs);
        }
    };

    static OwnPropertyKeyIterator *containerOwnPropertyKeys(const Object *m, Value *target)
    {
        *target = *m;
        return new OwnPropertyKeyIterator;
    }

    bool containerDeleteIndexedProperty(uint index)
    {
        /* Qt containers have int (rather than uint) allowable indexes. */
        if (index > INT_MAX)
            return false;
        if (d()->isReadOnly)
            return false;
        if (d()->isReference) {
            if (!d()->object)
                return false;
            loadReference();
        }

        if (index >= quint32(size()))
            return false;

        /* according to ECMA262r3 it should be Undefined, */
        /* but we cannot, so we insert a default-value instead. */
        replace(index, QVariant());

        if (d()->isReference)
            storeReference();

        return true;
    }

    bool containerIsEqualTo(Managed *other)
    {
        if (!other)
            return false;
        QV4Sequence *otherSequence = other->as<QV4Sequence>();
        if (!otherSequence)
            return false;
        if (d()->isReference && otherSequence->d()->isReference) {
            return d()->object == otherSequence->d()->object && d()->propertyIndex == otherSequence->d()->propertyIndex;
        } else if (!d()->isReference && !otherSequence->d()->isReference) {
            return this == otherSequence;
        }
        return false;
    }

    struct DefaultCompareFunctor
    {
        bool operator()(const QVariant &lhs, const QVariant &rhs)
        {
            return lhs.toString() < rhs.toString();
        }
    };

    struct CompareFunctor
    {
        CompareFunctor(QV4::ExecutionEngine *v4, const QV4::Value &compareFn)
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
            if (scope.engine->hasException)
                return false;
            return result->toNumber() < 0;
        }

    private:
        QV4::ExecutionEngine *m_v4;
        const QV4::Value *m_compareFn;
    };

    bool sort(const FunctionObject *f, const Value *, const Value *argv, int argc)
    {
        if (d()->isReadOnly)
            return false;
        if (d()->isReference) {
            if (!d()->object)
                return false;
            loadReference();
        }

        if (argc == 1 && argv[0].as<FunctionObject>())
            sort(CompareFunctor(f->engine(), argv[0]));
        else
            sort(DefaultCompareFunctor());

        if (d()->isReference)
            storeReference();

        return true;
    }

    static QV4::ReturnedValue method_get_length(const FunctionObject *b, const Value *thisObject, const Value *, int)
    {
        QV4::Scope scope(b);
        QV4::Scoped<QV4Sequence> This(scope, thisObject->as<QV4Sequence>());
        if (!This)
            THROW_TYPE_ERROR();

        if (This->d()->isReference) {
            if (!This->d()->object)
                RETURN_RESULT(Encode(0));
            This->loadReference();
        }
        RETURN_RESULT(Encode(qint32(This->size())));
    }

    static QV4::ReturnedValue method_set_length(const FunctionObject *f, const Value *thisObject, const Value *argv, int argc)
    {
        QV4::Scope scope(f);
        QV4::Scoped<QV4Sequence> This(scope, thisObject->as<QV4Sequence>());
        if (!This)
            THROW_TYPE_ERROR();

        quint32 newLength = argc ? argv[0].toUInt32() : 0;
        /* Qt containers have int (rather than uint) allowable indexes. */
        if (newLength > INT_MAX) {
            generateWarning(scope.engine, QLatin1String("Index out of range during length set"));
            RETURN_UNDEFINED();
        }

        if (This->d()->isReadOnly)
            THROW_TYPE_ERROR();

        /* Read the sequence from the QObject property if we're a reference */
        if (This->d()->isReference) {
            if (!This->d()->object)
                RETURN_UNDEFINED();
            This->loadReference();
        }
        /* Determine whether we need to modify the sequence */
        quint32 newCount = static_cast<quint32>(newLength);
        quint32 count = static_cast<quint32>(This->size());
        if (newCount == count) {
            RETURN_UNDEFINED();
        } else if (newCount > count) {
            const QMetaType valueMetaType = meta(This->d())->valueMetaType();
            /* according to ECMA262r3 we need to insert */
            /* undefined values increasing length to newLength. */
            /* We cannot, so we insert default-values instead. */
            while (newCount > count++)
                This->append(QVariant(valueMetaType));
        } else {
            /* according to ECMA262r3 we need to remove */
            /* elements until the sequence is the required length. */
            if (newCount < count)
                This->removeLast(count - newCount);
        }
        /* write back if required. */
        if (This->d()->isReference) {
            /* write back.  already checked that object is non-null, so skip that check here. */
            This->storeReference();
        }
        RETURN_UNDEFINED();
    }

    void* getRawContainerPtr() const
    { return d()->container; }

    void loadReference() const
    {
        Q_ASSERT(d()->object);
        Q_ASSERT(d()->isReference);
        void *a[] = { d()->container, nullptr };
        QMetaObject::metacall(d()->object, QMetaObject::ReadProperty, d()->propertyIndex, a);
    }

    void storeReference()
    {
        Q_ASSERT(d()->object);
        Q_ASSERT(d()->isReference);
        int status = -1;
        QQmlPropertyData::WriteFlags flags = QQmlPropertyData::DontRemoveBinding;
        void *a[] = { d()->container, nullptr, &status, &flags };
        QMetaObject::metacall(d()->object, QMetaObject::WriteProperty, d()->propertyIndex, a);
    }

    static QV4::ReturnedValue virtualGet(const QV4::Managed *that, PropertyKey id, const Value *receiver, bool *hasProperty)
    {
        if (!id.isArrayIndex())
            return Object::virtualGet(that, id, receiver, hasProperty);
        return static_cast<const QV4Sequence *>(that)->containerGetIndexed(id.asArrayIndex(), hasProperty);
    }
    static bool virtualPut(Managed *that, PropertyKey id, const QV4::Value &value, Value *receiver)
    {
        if (id.isArrayIndex())
            return static_cast<QV4Sequence *>(that)->containerPutIndexed(id.asArrayIndex(), value);
        return Object::virtualPut(that, id, value, receiver);
    }
    static QV4::PropertyAttributes queryIndexed(const QV4::Managed *that, uint index)
    { return static_cast<const QV4Sequence *>(that)->containerQueryIndexed(index); }
    static bool virtualDeleteProperty(QV4::Managed *that, PropertyKey id)
    {
        if (id.isArrayIndex()) {
            uint index = id.asArrayIndex();
            return static_cast<QV4Sequence *>(that)->containerDeleteIndexedProperty(index);
        }
        return Object::virtualDeleteProperty(that, id);
    }
    static bool virtualIsEqualTo(Managed *that, Managed *other)
    { return static_cast<QV4Sequence *>(that)->containerIsEqualTo(other); }
    static QV4::OwnPropertyKeyIterator *virtualOwnPropertyKeys(const Object *m, Value *target)
    { return static_cast<const QV4Sequence *>(m)->containerOwnPropertyKeys(m, target);}
};


void Heap::QV4Sequence::init(const QQmlType &qmlType, const void *container)
{
    Object::init();

    Q_ASSERT(qmlType.isSequentialContainer());
    typePrivate = qmlType.priv();
    QQmlType::refHandle(typePrivate);

    this->container = QMetaType(typePrivate->typeId).create(container);
    propertyIndex = -1;
    isReference = false;
    isReadOnly = false;
    object.init();

    QV4::Scope scope(internalClass->engine);
    QV4::Scoped<QV4::QV4Sequence> o(scope, this);
    o->setArrayType(Heap::ArrayData::Custom);
    o->init();
}

void Heap::QV4Sequence::init(QObject *object, int propertyIndex, const QQmlType &qmlType,
                             bool readOnly)
{
    Object::init();

    Q_ASSERT(qmlType.isSequentialContainer());
    typePrivate = qmlType.priv();
    QQmlType::refHandle(typePrivate);
    container = QMetaType(typePrivate->typeId).create();
    this->propertyIndex = propertyIndex;
    isReference = true;
    this->isReadOnly = readOnly;
    this->object.init(object);
    QV4::Scope scope(internalClass->engine);
    QV4::Scoped<QV4::QV4Sequence> o(scope, this);
    o->setArrayType(Heap::ArrayData::Custom);
    o->loadReference();
    o->init();
}

}

namespace QV4 {
DEFINE_OBJECT_VTABLE(QV4Sequence);
}

void SequencePrototype::init()
{
    defineDefaultProperty(QStringLiteral("sort"), method_sort, 1);
    defineDefaultProperty(engine()->id_valueOf(), method_valueOf, 0);
}

ReturnedValue SequencePrototype::method_valueOf(const FunctionObject *f, const Value *thisObject, const Value *, int)
{
    return Encode(thisObject->toString(f->engine()));
}

ReturnedValue SequencePrototype::method_sort(const FunctionObject *b, const Value *thisObject, const Value *argv, int argc)
{
    Scope scope(b);
    QV4::ScopedObject o(scope, thisObject);
    if (!o || !o->isListType())
        THROW_TYPE_ERROR();

    if (argc >= 2)
        return o.asReturnedValue();

    if (auto *s = o->as<QV4Sequence>()) {
        if (!s->sort(b, thisObject, argv, argc))
            THROW_TYPE_ERROR();
    }

    return o.asReturnedValue();
}

ReturnedValue SequencePrototype::newSequence(QV4::ExecutionEngine *engine, int sequenceType, QObject *object, int propertyIndex, bool readOnly, bool *succeeded)
{
    QV4::Scope scope(engine);
    // This function is called when the property is a QObject Q_PROPERTY of
    // the given sequence type.  Internally we store a sequence
    // (as well as object ptr + property index for updated-read and write-back)
    // and so access/mutate avoids variant conversion.


    const QQmlType qmlType = QQmlMetaType::qmlType(
                sequenceType, QQmlMetaType::TypeIdCategory::MetaType);
    if (qmlType.isSequentialContainer()) {
        *succeeded = true;
        QV4::ScopedObject obj(scope, engine->memoryManager->allocate<QV4Sequence>(
                                  object, propertyIndex, qmlType, readOnly));
        return obj.asReturnedValue();
    }

    *succeeded = false;
    return Encode::undefined();
}

ReturnedValue SequencePrototype::fromVariant(
        QV4::ExecutionEngine *engine, const QVariant &v, bool *succeeded)
{
    return fromData(engine, v.metaType(), v.constData(), succeeded);
}

ReturnedValue SequencePrototype::fromData(
        ExecutionEngine *engine, const QMetaType &type, const void *data, bool *succeeded)
{
    QV4::Scope scope(engine);
    // This function is called when assigning a sequence value to a normal JS var
    // in a JS block.  Internally, we store a sequence of the specified type.
    // Access and mutation is extremely fast since it will not need to modify any
    // QObject property.

    const QQmlType qmlType = QQmlMetaType::qmlType(
                type.id(), QQmlMetaType::TypeIdCategory::MetaType);
    if (qmlType.isSequentialContainer()) {
        *succeeded = true;
        QV4::ScopedObject obj(scope, engine->memoryManager->allocate<QV4Sequence>(qmlType, data));
        return obj.asReturnedValue();
    }

    *succeeded = false;
    return Encode::undefined();
}

QVariant SequencePrototype::toVariant(Object *object)
{
    Q_ASSERT(object->isListType());
    return object->as<QV4Sequence>()->toVariant();
}

QVariant SequencePrototype::toVariant(const QV4::Value &array, int typeHint, bool *succeeded)
{
    *succeeded = true;

    if (!array.as<ArrayObject>()) {
        *succeeded = false;
        return QVariant();
    }
    QV4::Scope scope(array.as<Object>()->engine());
    QV4::ScopedArrayObject a(scope, array);

    const QQmlType type = QQmlMetaType::qmlType(typeHint, QQmlMetaType::TypeIdCategory::MetaType);
    if (type.isSequentialContainer()) {
        const QMetaSequence *meta = type.priv()->extraData.ld;
        const QMetaType containerMetaType(type.priv()->typeId);
        QVariant result(containerMetaType);
        quint32 length = a->getLength();
        QV4::ScopedValue v(scope);
        for (quint32 i = 0; i < length; ++i) {
            const QMetaType valueMetaType = meta->valueMetaType();
            QVariant variant = scope.engine->toVariant(a->get(i), valueMetaType, false);
            if (variant.metaType() != valueMetaType && !variant.convert(valueMetaType))
                variant = QVariant(valueMetaType);
            meta->addValueAtEnd(result.data(), variant.constData());
        }
        return result;
    }

    *succeeded = false;
    return QVariant();
}

void* SequencePrototype::getRawContainerPtr(const Object *object, int typeHint)
{
    if (auto *s = object->as<QV4Sequence>()) {
        if (s->d()->typePrivate->typeId.id() == typeHint)
            return s->getRawContainerPtr();
    }
    return nullptr;
}

int SequencePrototype::metaTypeForSequence(const QV4::Object *object)
{
    if (auto *s = object->as<QV4Sequence>())
        return s->d()->typePrivate->typeId.id();
    return -1;
}

QT_END_NAMESPACE
