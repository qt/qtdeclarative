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
    retn.setLine(ctx->currentLineNumber());
    retn.setUrl(QUrl(ctx->currentFileName()));
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

static QV4::Value convertElementToValue(QV4::ExecutionContext *ctx, const QString &element)
{
    return QV4::Value::fromString(ctx, element);
}

static QV4::Value convertElementToValue(QV4::ExecutionContext *ctx, int element)
{
    return QV4::Value::fromInt32(element);
}

static QV4::Value convertElementToValue(QV4::ExecutionContext *ctx, const QUrl &element)
{
    return QV4::Value::fromString(ctx, element.toString());
}

static QV4::Value convertElementToValue(QV4::ExecutionContext *ctx, qreal element)
{
    return QV4::Value::fromDouble(element);
}

static QV4::Value convertElementToValue(QV4::ExecutionContext *ctx, bool element)
{
    return QV4::Value::fromBoolean(element);
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
    __qmljs_numberToString(&qstr, element, 10);
    return qstr;
}

static QString convertElementToString(bool element)
{
    if (element)
        return QStringLiteral("true");
    else
        return QStringLiteral("false");
}

template <typename ElementType> ElementType convertValueToElement(const QV4::Value &value);

template <> QString convertValueToElement(const QV4::Value &value)
{
    return value.toQString();
}

template <> int convertValueToElement(const QV4::Value &value)
{
    return value.toInt32();
}

template <> QUrl convertValueToElement(const QV4::Value &value)
{
    return QUrl(value.toQString());
}

template <> qreal convertValueToElement(const QV4::Value &value)
{
    return value.toNumber();
}

template <> bool convertValueToElement(const QV4::Value &value)
{
    return value.toBoolean();
}

template <typename Container>
class QQmlSequence : public QQmlSequenceBase
{
    Q_MANAGED
public:
    QQmlSequence(QV4::ExecutionEngine *engine, const Container &container)
        : QQmlSequenceBase(engine)
        , m_container(container)
        , m_object(0)
        , m_propertyIndex(-1)
        , m_isReference(false)
    {
        type = Type_QmlSequence;
        vtbl = &static_vtbl;
        prototype = engine->sequencePrototype;
        initClass(engine);
    }

    QQmlSequence(QV4::ExecutionEngine *engine, QObject *object, int propertyIndex)
        : QQmlSequenceBase(engine)
        , m_object(object)
        , m_propertyIndex(propertyIndex)
        , m_isReference(true)
    {
        type = Type_QmlSequence;
        vtbl = &static_vtbl;
        prototype = engine->sequencePrototype;
        loadReference();
        initClass(engine);
    }

    QV4::Value containerGetIndexed(QV4::ExecutionContext *ctx, uint index, bool *hasProperty)
    {
        /* Qt containers have int (rather than uint) allowable indexes. */
        if (index > INT_MAX) {
            generateWarning(ctx, QLatin1String("Index out of range during indexed get"));
            if (hasProperty)
                *hasProperty = false;
            return QV4::Value::undefinedValue();
        }
        if (m_isReference) {
            if (!m_object) {
                if (hasProperty)
                    *hasProperty = false;
                return QV4::Value::undefinedValue();
            }
            loadReference();
        }
        qint32 signedIdx = static_cast<qint32>(index);
        if (signedIdx < m_container.count()) {
            if (hasProperty)
                *hasProperty = true;
            return convertElementToValue(ctx, m_container.at(signedIdx));
        }
        if (hasProperty)
            *hasProperty = false;
        return QV4::Value::undefinedValue();
    }

    void containerPutIndexed(QV4::ExecutionContext *ctx, uint index, const QV4::Value &value)
    {
        /* Qt containers have int (rather than uint) allowable indexes. */
        if (index > INT_MAX) {
            generateWarning(ctx, QLatin1String("Index out of range during indexed put"));
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

    QV4::PropertyAttributes containerQueryIndexed(QV4::ExecutionContext *ctx, uint index)
    {
        /* Qt containers have int (rather than uint) allowable indexes. */
        if (index > INT_MAX) {
            generateWarning(ctx, QLatin1String("Index out of range during indexed query"));
            return QV4::Attr_Invalid;
        }
        if (m_isReference) {
            if (!m_object)
                return QV4::Attr_Invalid;
            loadReference();
        }
        qint32 signedIdx = static_cast<qint32>(index);
        return (index < m_container.count()) ? QV4::Attr_Data : QV4::Attr_Invalid;
    }

    bool containerDeleteIndexedProperty(QV4::ExecutionContext *ctx, uint index)
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

    void sort(QV4::SimpleCallContext *ctx)
    {
        if (m_isReference) {
            if (!m_object)
                return;
            loadReference();
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
            CompareFunctor(QV4::ExecutionContext *ctx, const QV4::Value &compareFn)
                : m_ctx(ctx), m_compareFn(compareFn)
            {}

            bool operator()(typename Container::value_type lhs, typename Container::value_type rhs)
            {
                QV4::Managed *fun = this->m_compareFn.asManaged();
                QV4::Value argv[2] = {
                    convertElementToValue(this->m_ctx, lhs),
                    convertElementToValue(this->m_ctx, rhs)
                };
                QV4::Value result = fun->call(this->m_ctx, QV4::Value::fromObject(this->m_ctx->engine->globalObject), argv, 2);
                return result.toNumber() < 0;
            }

        private:
            QV4::ExecutionContext *m_ctx;
            QV4::Value m_compareFn;
        };

        if (ctx->argumentCount == 1 && ctx->arguments[0].asFunctionObject()) {
            QV4::Value compareFn = ctx->arguments[0];
            CompareFunctor cf(ctx, compareFn);
            qSort(m_container.begin(), m_container.end(), cf);
        } else {
            DefaultCompareFunctor cf;
            qSort(m_container.begin(), m_container.end(), cf);
        }

        if (m_isReference)
            storeReference();
    }

    QV4::Value lengthGetter(QV4::SimpleCallContext*)
    {
        if (m_isReference) {
            if (!m_object)
                return QV4::Value::fromInt32(0);
            loadReference();
        }
        return QV4::Value::fromInt32(m_container.count());
    }

    void lengthSetter(QV4::SimpleCallContext* ctx)
    {
        quint32 newLength = ctx->arguments[0].toUInt32();
        /* Qt containers have int (rather than uint) allowable indexes. */
        if (newLength > INT_MAX) {
            generateWarning(ctx, QLatin1String("Index out of range during length set"));
            return;
        }
        /* Read the sequence from the QObject property if we're a reference */
        if (m_isReference) {
            if (!m_object)
                return;
            loadReference();
        }
        /* Determine whether we need to modify the sequence */
        qint32 newCount = static_cast<qint32>(newLength);
        qint32 count = m_container.count();
        if (newCount == count) {
            return;
        } else if (newCount > count) {
            /* according to ECMA262r3 we need to insert */
            /* undefined values increasing length to newLength. */
            /* We cannot, so we insert default-values instead. */
            m_container.reserve(newCount);
            while (newCount > count++) {
                m_container.append(typename Container::value_type());
            }
        } else {
            /* according to ECMA262r3 we need to remove */
            /* elements until the sequence is the required length. */
            while (newCount < count) {
                count--;
                m_container.removeAt(count);
            }
        }
        /* write back if required. */
        if (m_isReference) {
            /* write back.  already checked that object is non-null, so skip that check here. */
            storeReference();
        }
    }

    QVariant toVariant() const
    { return QVariant::fromValue<Container>(m_container); }

    static QVariant toVariant(QV4::ArrayObject *array)
    {
        Container result;
        uint32_t length = array->arrayLength();
        for (uint32_t i = 0; i < length; ++i)
            result << convertValueToElement<typename Container::value_type>(array->getIndexed(i));
        return QVariant::fromValue(result);
    }

private:
    void loadReference()
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

    Container m_container;
    QQmlGuard<QObject> m_object;
    int m_propertyIndex;
    bool m_isReference;

    static QV4::Value getIndexed(QV4::Managed *that, QV4::ExecutionContext *ctx, uint index, bool *hasProperty)
    { return static_cast<QQmlSequence<Container> *>(that)->containerGetIndexed(ctx, index, hasProperty); }
    static void putIndexed(Managed *that, QV4::ExecutionContext *ctx, uint index, const QV4::Value &value)
    { static_cast<QQmlSequence<Container> *>(that)->containerPutIndexed(ctx, index, value); }
    static QV4::PropertyAttributes queryIndexed(QV4::Managed *that, QV4::ExecutionContext *ctx, uint index)
    { return static_cast<QQmlSequence<Container> *>(that)->containerQueryIndexed(ctx, index); }
    static bool deleteIndexedProperty(QV4::Managed *that, QV4::ExecutionContext *ctx, uint index)
    { return static_cast<QQmlSequence<Container> *>(that)->containerDeleteIndexedProperty(ctx, index); }

    static void destroy(Managed *that)
    {
        static_cast<QQmlSequence<Container> *>(that)->~QQmlSequence<Container>();
    }
};

typedef QQmlSequence<QStringList> QQmlQStringList;
template<>
DEFINE_MANAGED_VTABLE(QQmlQStringList);
typedef QQmlSequence<QList<QString> > QQmlStringList;
template<>
DEFINE_MANAGED_VTABLE(QQmlStringList);
typedef QQmlSequence<QList<int> > QQmlIntList;
template<>
DEFINE_MANAGED_VTABLE(QQmlIntList);
typedef QQmlSequence<QList<QUrl> > QQmlUrlList;
template<>
DEFINE_MANAGED_VTABLE(QQmlUrlList);
typedef QQmlSequence<QList<bool> > QQmlBoolList;
template<>
DEFINE_MANAGED_VTABLE(QQmlBoolList);
typedef QQmlSequence<QList<qreal> > QQmlRealList;
template<>
DEFINE_MANAGED_VTABLE(QQmlRealList);

#define REGISTER_QML_SEQUENCE_METATYPE(unused, unused2, SequenceType, unused3) qRegisterMetaType<SequenceType>();
SequencePrototype::SequencePrototype(ExecutionEngine *engine)
    : QV4::Object(engine)
{
    prototype = engine->arrayPrototype;
    FOREACH_QML_SEQUENCE_TYPE(REGISTER_QML_SEQUENCE_METATYPE)
}
#undef REGISTER_QML_SEQUENCE_METATYPE

QV4::Value SequencePrototype::method_sort(QV4::SimpleCallContext *ctx)
{
    QV4::Object *o = ctx->thisObject.asObject();
    if (!o || !o->isListType())
        ctx->throwTypeError();

    if (ctx->argumentCount >= 2)
        return ctx->thisObject;

#define CALL_SORT(SequenceElementType, SequenceElementTypeName, SequenceType, DefaultValue) \
        if (QQml##SequenceElementTypeName##List *s = o->as<QQml##SequenceElementTypeName##List>()) { \
            s->sort(ctx); \
        } else

        FOREACH_QML_SEQUENCE_TYPE(CALL_SORT)

#undef CALL_SORT
    return ctx->thisObject;
}

QV4::Value QQmlSequenceBase::method_get_length(QV4::SimpleCallContext* ctx) QV4_ANNOTATE(attributes QV4::Attr_ReadOnly)
{
    QV4::Object *o = ctx->thisObject.asObject();
    if (!o)
        ctx->throwTypeError();
#define CALL_LENGTH_GETTER(SequenceElementType, SequenceElementTypeName, SequenceType, DefaultValue) \
    if (QQml##SequenceElementTypeName##List *s = o->as<QQml##SequenceElementTypeName##List>()) { \
        return s->lengthGetter(ctx); \
    } else

    FOREACH_QML_SEQUENCE_TYPE(CALL_LENGTH_GETTER)
#undef CALL_LENGTH_GETTER

    return QV4::Value::undefinedValue();
}

QV4::Value QQmlSequenceBase::method_set_length(QV4::SimpleCallContext* ctx)
{
    QV4::Object *o = ctx->thisObject.asObject();
    if (!o)
        ctx->throwTypeError();
#define CALL_LENGTH_SETTER(SequenceElementType, SequenceElementTypeName, SequenceType, DefaultValue) \
    if (QQml##SequenceElementTypeName##List *s = o->as<QQml##SequenceElementTypeName##List>()) { \
        s->lengthSetter(ctx); \
    } else

    FOREACH_QML_SEQUENCE_TYPE(CALL_LENGTH_SETTER)
#undef CALL_LENGTH_SETTER

    return QV4::Value::undefinedValue();
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
        QV4::Object *obj = new (engine->memoryManager) QQml##ElementTypeName##List(engine, object, propertyIndex); \
        return QV4::Value::fromObject(obj); \
    } else

QV4::Value SequencePrototype::newSequence(QV4::ExecutionEngine *engine, int sequenceType, QObject *object, int propertyIndex, bool *succeeded)
{
    // This function is called when the property is a QObject Q_PROPERTY of
    // the given sequence type.  Internally we store a typed-sequence
    // (as well as object ptr + property index for updated-read and write-back)
    // and so access/mutate avoids variant conversion.
    *succeeded = true;
    FOREACH_QML_SEQUENCE_TYPE(NEW_REFERENCE_SEQUENCE) { /* else */ *succeeded = false; return QV4::Value::undefinedValue(); }
}
#undef NEW_REFERENCE_SEQUENCE

#define NEW_COPY_SEQUENCE(ElementType, ElementTypeName, SequenceType, unused) \
    if (sequenceType == qMetaTypeId<SequenceType>()) { \
        QV4::Object *obj = new (engine->memoryManager) QQml##ElementTypeName##List(engine, v.value<SequenceType >()); \
        return QV4::Value::fromObject(obj); \
    } else

QV4::Value SequencePrototype::fromVariant(QV4::ExecutionEngine *engine, const QVariant& v, bool *succeeded)
{
    // This function is called when assigning a sequence value to a normal JS var
    // in a JS block.  Internally, we store a sequence of the specified type.
    // Access and mutation is extremely fast since it will not need to modify any
    // QObject property.
    int sequenceType = v.userType();
    *succeeded = true;
    FOREACH_QML_SEQUENCE_TYPE(NEW_COPY_SEQUENCE) { /* else */ *succeeded = false; return QV4::Value::undefinedValue(); }
}
#undef NEW_COPY_SEQUENCE

#define SEQUENCE_TO_VARIANT(ElementType, ElementTypeName, SequenceType, unused) \
    if (QQml##ElementTypeName##List *list = object->as<QQml##ElementTypeName##List>()) \
        return list->toVariant(); \
    else

QVariant SequencePrototype::toVariant(QV4::Object *object)
{
    Q_ASSERT(object->isListType());
    FOREACH_QML_SEQUENCE_TYPE(SEQUENCE_TO_VARIANT) { /* else */ return QVariant(); }
}

#define SEQUENCE_TO_VARIANT(ElementType, ElementTypeName, SequenceType, unused) \
    if (typeHint == qMetaTypeId<SequenceType>()) { \
        return QQml##ElementTypeName##List::toVariant(a); \
    } else

QVariant SequencePrototype::toVariant(const QV4::Value &array, int typeHint, bool *succeeded)
{
    *succeeded = true;

    QV4::ArrayObject *a = array.asArrayObject();
    if (!a) {
        *succeeded = false;
        return QVariant();
    }
    FOREACH_QML_SEQUENCE_TYPE(SEQUENCE_TO_VARIANT) { /* else */ *succeeded = false; return QVariant(); }
}

#undef SEQUENCE_TO_VARIANT

#define MAP_META_TYPE(ElementType, ElementTypeName, SequenceType, unused) \
    if (object->as<QQml##ElementTypeName##List>()) { \
        return qMetaTypeId<SequenceType>(); \
    } else

int SequencePrototype::metaTypeForSequence(QV4::Object *object)
{
    FOREACH_QML_SEQUENCE_TYPE(MAP_META_TYPE)
    /*else*/ {
        return -1;
    }
}

#undef MAP_META_TYPE

#include "qv4sequenceobject_p_jsclass.cpp"

QT_END_NAMESPACE
