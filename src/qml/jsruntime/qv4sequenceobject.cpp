/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtQml/qqml.h>

#include "qv4sequenceobject_p.h"

#include <private/qv4functionobject_p.h>
#include <private/qv4arrayobject_p.h>
#include <private/qqmlengine_p.h>
#include <private/qv4scopedvalue_p.h>
#include <private/qv4internalclass_p.h>

#include <algorithm>

QT_BEGIN_NAMESPACE

using namespace QV4;

// helper function to generate valid warnings if errors occur during sequence operations.
static void generateWarning(QV4::ExecutionContext *ctx, const QString& description)
{
    QQmlEngine *engine = ctx->engine->v8Engine->engine();
    if (!engine)
        return;
    QQmlError retn;
    retn.setDescription(description);

    QV4::StackFrame frame = ctx->engine->currentStackFrame();

    retn.setLine(frame.line);
    retn.setUrl(QUrl(frame.source));
    QQmlEnginePrivate::warning(engine, retn);
}

//  F(elementType, elementTypeName, sequenceType, defaultValue)
#define FOREACH_QML_SEQUENCE_TYPE(F) \
    F(int, Int, QList<int>, 0) \
    F(qreal, Real, QList<qreal>, 0.0) \
    F(bool, Bool, QList<bool>, false) \
    F(QString, String, QList<QString>, QString()) \
    F(QString, QString, QStringList, QString()) \
    F(QUrl, Url, QList<QUrl>, QUrl())

static QV4::ReturnedValue convertElementToValue(QV4::ExecutionEngine *engine, const QString &element)
{
    return engine->newString(element)->asReturnedValue();
}

static QV4::ReturnedValue convertElementToValue(QV4::ExecutionEngine *, int element)
{
    return QV4::Encode(element);
}

static QV4::ReturnedValue convertElementToValue(QV4::ExecutionEngine *engine, const QUrl &element)
{
    return engine->newString(element.toString())->asReturnedValue();
}

static QV4::ReturnedValue convertElementToValue(QV4::ExecutionEngine *, qreal element)
{
    return QV4::Encode(element);
}

static QV4::ReturnedValue convertElementToValue(QV4::ExecutionEngine *, bool element)
{
    return QV4::Encode(element);
}

static QString convertElementToString(const QString &element)
{
    return element;
}

static QString convertElementToString(int element)
{
    return QString::number(element);
}

static QString convertElementToString(const QUrl &element)
{
    return element.toString();
}

static QString convertElementToString(qreal element)
{
    QString qstr;
    RuntimeHelpers::numberToString(&qstr, element, 10);
    return qstr;
}

static QString convertElementToString(bool element)
{
    if (element)
        return QStringLiteral("true");
    else
        return QStringLiteral("false");
}

template <typename ElementType> ElementType convertValueToElement(const QV4::ValueRef value);

template <> QString convertValueToElement(const QV4::ValueRef value)
{
    return value->toQString();
}

template <> int convertValueToElement(const QV4::ValueRef value)
{
    return value->toInt32();
}

template <> QUrl convertValueToElement(const QV4::ValueRef value)
{
    return QUrl(value->toQString());
}

template <> qreal convertValueToElement(const QV4::ValueRef value)
{
    return value->toNumber();
}

template <> bool convertValueToElement(const ValueRef value)
{
    return value->toBoolean();
}

template <typename Container>
class QQmlSequence : public QV4::Object
{
    V4_OBJECT
    Q_MANAGED_TYPE(QmlSequence)
public:
    QQmlSequence(QV4::ExecutionEngine *engine, const Container &container)
        : QV4::Object(InternalClass::create(engine, staticVTable(), engine->sequencePrototype.asObject()))
        , m_container(container)
        , m_object(0)
        , m_propertyIndex(-1)
        , m_isReference(false)
    {
        QV4::Scope scope(engine);
        QV4::ScopedObject protectThis(scope, this);
        Q_UNUSED(protectThis);
        setArrayType(ArrayData::Custom);
        init();
    }

    QQmlSequence(QV4::ExecutionEngine *engine, QObject *object, int propertyIndex)
        : QV4::Object(InternalClass::create(engine, staticVTable(), engine->sequencePrototype.asObject()))
        , m_object(object)
        , m_propertyIndex(propertyIndex)
        , m_isReference(true)
    {
        QV4::Scope scope(engine);
        QV4::ScopedObject protectThis(scope, this);
        Q_UNUSED(protectThis);
        setArrayType(ArrayData::Custom);
        loadReference();
        init();
    }

    void init()
    {
        defineAccessorProperty(QStringLiteral("length"), method_get_length, method_set_length);
    }

    QV4::ReturnedValue containerGetIndexed(uint index, bool *hasProperty)
    {
        /* Qt containers have int (rather than uint) allowable indexes. */
        if (index > INT_MAX) {
            generateWarning(engine()->currentContext(), QLatin1String("Index out of range during indexed get"));
            if (hasProperty)
                *hasProperty = false;
            return Encode::undefined();
        }
        if (m_isReference) {
            if (!m_object) {
                if (hasProperty)
                    *hasProperty = false;
                return Encode::undefined();
            }
            loadReference();
        }
        qint32 signedIdx = static_cast<qint32>(index);
        if (signedIdx < m_container.count()) {
            if (hasProperty)
                *hasProperty = true;
            return convertElementToValue(engine(), m_container.at(signedIdx));
        }
        if (hasProperty)
            *hasProperty = false;
        return Encode::undefined();
    }

    void containerPutIndexed(uint index, const QV4::ValueRef value)
    {
        if (internalClass->engine->hasException)
            return;

        /* Qt containers have int (rather than uint) allowable indexes. */
        if (index > INT_MAX) {
            generateWarning(engine()->currentContext(), QLatin1String("Index out of range during indexed set"));
            return;
        }

        if (m_isReference) {
            if (!m_object)
                return;
            loadReference();
        }

        qint32 signedIdx = static_cast<qint32>(index);

        int count = m_container.count();

        typename Container::value_type element = convertValueToElement<typename Container::value_type>(value);

        if (signedIdx == count) {
            m_container.append(element);
        } else if (signedIdx < count) {
            m_container[signedIdx] = element;
        } else {
            /* according to ECMA262r3 we need to insert */
            /* the value at the given index, increasing length to index+1. */
            m_container.reserve(signedIdx + 1);
            while (signedIdx > count++) {
                m_container.append(typename Container::value_type());
            }
            m_container.append(element);
        }

        if (m_isReference)
            storeReference();
    }

    QV4::PropertyAttributes containerQueryIndexed(uint index) const
    {
        /* Qt containers have int (rather than uint) allowable indexes. */
        if (index > INT_MAX) {
            generateWarning(engine()->currentContext(), QLatin1String("Index out of range during indexed query"));
            return QV4::Attr_Invalid;
        }
        if (m_isReference) {
            if (!m_object)
                return QV4::Attr_Invalid;
            loadReference();
        }
        qint32 signedIdx = static_cast<qint32>(index);
        return (signedIdx < m_container.count()) ? QV4::Attr_Data : QV4::Attr_Invalid;
    }

    void containerAdvanceIterator(ObjectIterator *it, StringRef name, uint *index, Property *p, PropertyAttributes *attrs)
    {
        name = (String *)0;
        *index = UINT_MAX;

        if (m_isReference) {
            if (!m_object) {
                QV4::Object::advanceIterator(this, it, name, index, p, attrs);
                return;
            }
            loadReference();
        }

        if (it->arrayIndex < static_cast<uint>(m_container.count())) {
            *index = it->arrayIndex;
            ++it->arrayIndex;
            *attrs = QV4::Attr_Data;
            p->value = convertElementToValue(engine(), m_container.at(*index));
            return;
        }
        QV4::Object::advanceIterator(this, it, name, index, p, attrs);
    }

    bool containerDeleteIndexedProperty(uint index)
    {
        /* Qt containers have int (rather than uint) allowable indexes. */
        if (index > INT_MAX)
            return false;
        if (m_isReference) {
            if (!m_object)
                return false;
            loadReference();
        }
        qint32 signedIdx = static_cast<qint32>(index);

        if (signedIdx >= m_container.count())
            return false;

        /* according to ECMA262r3 it should be Undefined, */
        /* but we cannot, so we insert a default-value instead. */
        m_container.replace(signedIdx, typename Container::value_type());

        if (m_isReference)
            storeReference();

        return true;
    }

    bool containerIsEqualTo(Managed *other)
    {
        QQmlSequence<Container> *otherSequence = other->as<QQmlSequence<Container> >();
        if (!otherSequence)
            return false;
        if (m_isReference && otherSequence->m_isReference) {
            return m_object == otherSequence->m_object && m_propertyIndex == otherSequence->m_propertyIndex;
        } else if (!m_isReference && !otherSequence->m_isReference) {
            return this == otherSequence;
        }
        return false;
    }

    struct DefaultCompareFunctor
    {
        bool operator()(typename Container::value_type lhs, typename Container::value_type rhs)
        {
            return convertElementToString(lhs) < convertElementToString(rhs);
        }
    };

    struct CompareFunctor
    {
        CompareFunctor(QV4::ExecutionContext *ctx, const QV4::ValueRef compareFn)
            : m_ctx(ctx), m_compareFn(compareFn)
        {}

        bool operator()(typename Container::value_type lhs, typename Container::value_type rhs)
        {
            QV4::Scope scope(m_ctx);
            ScopedObject compare(scope, m_compareFn);
            ScopedCallData callData(scope, 2);
            callData->args[0] = convertElementToValue(this->m_ctx->engine, lhs);
            callData->args[1] = convertElementToValue(this->m_ctx->engine, rhs);
            callData->thisObject = this->m_ctx->engine->globalObject;
            QV4::ScopedValue result(scope, compare->call(callData));
            return result->toNumber() < 0;
        }

    private:
        QV4::ExecutionContext *m_ctx;
        QV4::ValueRef m_compareFn;
    };

    void sort(QV4::CallContext *ctx)
    {
        if (m_isReference) {
            if (!m_object)
                return;
            loadReference();
        }

        QV4::Scope scope(ctx);
        if (ctx->callData->argc == 1 && ctx->callData->args[0].asFunctionObject()) {
            CompareFunctor cf(ctx, ctx->callData->args[0]);
            std::sort(m_container.begin(), m_container.end(), cf);
        } else {
            DefaultCompareFunctor cf;
            std::sort(m_container.begin(), m_container.end(), cf);
        }

        if (m_isReference)
            storeReference();
    }

    static QV4::ReturnedValue method_get_length(QV4::CallContext *ctx)
    {
        QV4::Scope scope(ctx);
        QV4::Scoped<QQmlSequence<Container> > This(scope, ctx->callData->thisObject.as<QQmlSequence<Container> >());
        if (!This)
            return ctx->throwTypeError();

        if (This->m_isReference) {
            if (!This->m_object)
                return QV4::Encode(0);
            This->loadReference();
        }
        return QV4::Encode(This->m_container.count());
    }

    static QV4::ReturnedValue method_set_length(QV4::CallContext* ctx)
    {
        QV4::Scope scope(ctx);
        QV4::Scoped<QQmlSequence<Container> > This(scope, ctx->callData->thisObject.as<QQmlSequence<Container> >());
        if (!This)
            return ctx->throwTypeError();

        quint32 newLength = ctx->callData->args[0].toUInt32();
        /* Qt containers have int (rather than uint) allowable indexes. */
        if (newLength > INT_MAX) {
            generateWarning(ctx, QLatin1String("Index out of range during length set"));
            return QV4::Encode::undefined();
        }
        /* Read the sequence from the QObject property if we're a reference */
        if (This->m_isReference) {
            if (!This->m_object)
                return QV4::Encode::undefined();
            This->loadReference();
        }
        /* Determine whether we need to modify the sequence */
        qint32 newCount = static_cast<qint32>(newLength);
        qint32 count = This->m_container.count();
        if (newCount == count) {
            return QV4::Encode::undefined();
        } else if (newCount > count) {
            /* according to ECMA262r3 we need to insert */
            /* undefined values increasing length to newLength. */
            /* We cannot, so we insert default-values instead. */
            This->m_container.reserve(newCount);
            while (newCount > count++) {
                This->m_container.append(typename Container::value_type());
            }
        } else {
            /* according to ECMA262r3 we need to remove */
            /* elements until the sequence is the required length. */
            while (newCount < count) {
                count--;
                This->m_container.removeAt(count);
            }
        }
        /* write back if required. */
        if (This->m_isReference) {
            /* write back.  already checked that object is non-null, so skip that check here. */
            This->storeReference();
        }
        return QV4::Encode::undefined();
    }

    QVariant toVariant() const
    { return QVariant::fromValue<Container>(m_container); }

    static QVariant toVariant(QV4::ArrayObjectRef array)
    {
        QV4::Scope scope(array->engine());
        Container result;
        quint32 length = array->getLength();
        QV4::ScopedValue v(scope);
        for (quint32 i = 0; i < length; ++i)
            result << convertValueToElement<typename Container::value_type>((v = array->getIndexed(i)));
        return QVariant::fromValue(result);
    }

private:
    void loadReference() const
    {
        Q_ASSERT(m_object);
        Q_ASSERT(m_isReference);
        void *a[] = { &m_container, 0 };
        QMetaObject::metacall(m_object, QMetaObject::ReadProperty, m_propertyIndex, a);
    }

    void storeReference()
    {
        Q_ASSERT(m_object);
        Q_ASSERT(m_isReference);
        int status = -1;
        QQmlPropertyPrivate::WriteFlags flags = QQmlPropertyPrivate::DontRemoveBinding;
        void *a[] = { &m_container, 0, &status, &flags };
        QMetaObject::metacall(m_object, QMetaObject::WriteProperty, m_propertyIndex, a);
    }

    mutable Container m_container;
    QPointer<QObject> m_object;
    int m_propertyIndex;
    bool m_isReference;

    static QV4::ReturnedValue getIndexed(QV4::Managed *that, uint index, bool *hasProperty)
    { return static_cast<QQmlSequence<Container> *>(that)->containerGetIndexed(index, hasProperty); }
    static void putIndexed(Managed *that, uint index, const QV4::ValueRef value)
    { static_cast<QQmlSequence<Container> *>(that)->containerPutIndexed(index, value); }
    static QV4::PropertyAttributes queryIndexed(const QV4::Managed *that, uint index)
    { return static_cast<const QQmlSequence<Container> *>(that)->containerQueryIndexed(index); }
    static bool deleteIndexedProperty(QV4::Managed *that, uint index)
    { return static_cast<QQmlSequence<Container> *>(that)->containerDeleteIndexedProperty(index); }
    static bool isEqualTo(Managed *that, Managed *other)
    { return static_cast<QQmlSequence<Container> *>(that)->containerIsEqualTo(other); }
    static void advanceIterator(Managed *that, ObjectIterator *it, StringRef name, uint *index, Property *p, PropertyAttributes *attrs)
    { return static_cast<QQmlSequence<Container> *>(that)->containerAdvanceIterator(it, name, index, p, attrs); }

    static void destroy(Managed *that)
    {
        static_cast<QQmlSequence<Container> *>(that)->~QQmlSequence<Container>();
    }
};

typedef QQmlSequence<QStringList> QQmlQStringList;
template<>
DEFINE_OBJECT_VTABLE(QQmlQStringList);
typedef QQmlSequence<QList<QString> > QQmlStringList;
template<>
DEFINE_OBJECT_VTABLE(QQmlStringList);
typedef QQmlSequence<QList<int> > QQmlIntList;
template<>
DEFINE_OBJECT_VTABLE(QQmlIntList);
typedef QQmlSequence<QList<QUrl> > QQmlUrlList;
template<>
DEFINE_OBJECT_VTABLE(QQmlUrlList);
typedef QQmlSequence<QList<bool> > QQmlBoolList;
template<>
DEFINE_OBJECT_VTABLE(QQmlBoolList);
typedef QQmlSequence<QList<qreal> > QQmlRealList;
template<>
DEFINE_OBJECT_VTABLE(QQmlRealList);

#define REGISTER_QML_SEQUENCE_METATYPE(unused, unused2, SequenceType, unused3) qRegisterMetaType<SequenceType>(#SequenceType);
SequencePrototype::SequencePrototype(InternalClass *ic)
    : QV4::Object(ic)
{
    FOREACH_QML_SEQUENCE_TYPE(REGISTER_QML_SEQUENCE_METATYPE)
}
#undef REGISTER_QML_SEQUENCE_METATYPE

void SequencePrototype::init()
{
    defineDefaultProperty(QStringLiteral("sort"), method_sort, 1);
    defineDefaultProperty(engine()->id_valueOf, method_valueOf, 0);
}

QV4::ReturnedValue SequencePrototype::method_sort(QV4::CallContext *ctx)
{
    QV4::Scope scope(ctx);
    QV4::ScopedObject o(scope, ctx->callData->thisObject);
    if (!o || !o->isListType())
        return ctx->throwTypeError();

    if (ctx->callData->argc >= 2)
        return o.asReturnedValue();

#define CALL_SORT(SequenceElementType, SequenceElementTypeName, SequenceType, DefaultValue) \
        if (QQml##SequenceElementTypeName##List *s = o->as<QQml##SequenceElementTypeName##List>()) { \
            s->sort(ctx); \
        } else

        FOREACH_QML_SEQUENCE_TYPE(CALL_SORT)

#undef CALL_SORT
        {}
    return o.asReturnedValue();
}

#define IS_SEQUENCE(unused1, unused2, SequenceType, unused3) \
    if (sequenceTypeId == qMetaTypeId<SequenceType>()) { \
        return true; \
    } else

bool SequencePrototype::isSequenceType(int sequenceTypeId)
{
    FOREACH_QML_SEQUENCE_TYPE(IS_SEQUENCE) { /* else */ return false; }
}
#undef IS_SEQUENCE

#define NEW_REFERENCE_SEQUENCE(ElementType, ElementTypeName, SequenceType, unused) \
    if (sequenceType == qMetaTypeId<SequenceType>()) { \
        QV4::Scoped<QV4::Object> obj(scope, new (engine->memoryManager) QQml##ElementTypeName##List(engine, object, propertyIndex)); \
        return obj.asReturnedValue(); \
    } else

ReturnedValue SequencePrototype::newSequence(QV4::ExecutionEngine *engine, int sequenceType, QObject *object, int propertyIndex, bool *succeeded)
{
    QV4::Scope scope(engine);
    // This function is called when the property is a QObject Q_PROPERTY of
    // the given sequence type.  Internally we store a typed-sequence
    // (as well as object ptr + property index for updated-read and write-back)
    // and so access/mutate avoids variant conversion.
    *succeeded = true;
    FOREACH_QML_SEQUENCE_TYPE(NEW_REFERENCE_SEQUENCE) { /* else */ *succeeded = false; return QV4::Encode::undefined(); }
}
#undef NEW_REFERENCE_SEQUENCE

#define NEW_COPY_SEQUENCE(ElementType, ElementTypeName, SequenceType, unused) \
    if (sequenceType == qMetaTypeId<SequenceType>()) { \
        QV4::Scoped<QV4::Object> obj(scope, new (engine->memoryManager) QQml##ElementTypeName##List(engine, v.value<SequenceType >())); \
        return obj.asReturnedValue(); \
    } else

ReturnedValue SequencePrototype::fromVariant(QV4::ExecutionEngine *engine, const QVariant& v, bool *succeeded)
{
    QV4::Scope scope(engine);
    // This function is called when assigning a sequence value to a normal JS var
    // in a JS block.  Internally, we store a sequence of the specified type.
    // Access and mutation is extremely fast since it will not need to modify any
    // QObject property.
    int sequenceType = v.userType();
    *succeeded = true;
    FOREACH_QML_SEQUENCE_TYPE(NEW_COPY_SEQUENCE) { /* else */ *succeeded = false; return QV4::Encode::undefined(); }
}
#undef NEW_COPY_SEQUENCE

#define SEQUENCE_TO_VARIANT(ElementType, ElementTypeName, SequenceType, unused) \
    if (QQml##ElementTypeName##List *list = object->as<QQml##ElementTypeName##List>()) \
        return list->toVariant(); \
    else

QVariant SequencePrototype::toVariant(ObjectRef object)
{
    Q_ASSERT(object->isListType());
    FOREACH_QML_SEQUENCE_TYPE(SEQUENCE_TO_VARIANT) { /* else */ return QVariant(); }
}

#undef SEQUENCE_TO_VARIANT
#define SEQUENCE_TO_VARIANT(ElementType, ElementTypeName, SequenceType, unused) \
    if (typeHint == qMetaTypeId<SequenceType>()) { \
        return QQml##ElementTypeName##List::toVariant(a); \
    } else

QVariant SequencePrototype::toVariant(const QV4::ValueRef array, int typeHint, bool *succeeded)
{
    *succeeded = true;

    if (!array->asArrayObject()) {
        *succeeded = false;
        return QVariant();
    }
    QV4::Scope scope(array->engine());
    QV4::ScopedArrayObject a(scope, array);

    FOREACH_QML_SEQUENCE_TYPE(SEQUENCE_TO_VARIANT) { /* else */ *succeeded = false; return QVariant(); }
}

#undef SEQUENCE_TO_VARIANT

#define MAP_META_TYPE(ElementType, ElementTypeName, SequenceType, unused) \
    if (object->as<QQml##ElementTypeName##List>()) { \
        return qMetaTypeId<SequenceType>(); \
    } else

int SequencePrototype::metaTypeForSequence(QV4::ObjectRef object)
{
    FOREACH_QML_SEQUENCE_TYPE(MAP_META_TYPE)
    /*else*/ {
        return -1;
    }
}

#undef MAP_META_TYPE

QT_END_NAMESPACE
