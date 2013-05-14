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

#ifndef QV8SEQUENCEWRAPPER_P_P_H
#define QV8SEQUENCEWRAPPER_P_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <private/qqmlengine_p.h>
#include <private/qqmlmetatype_p.h>

#include <private/qv4arrayobject_p.h>

QT_BEGIN_NAMESPACE

// helper function to generate valid warnings if errors occur during sequence operations.
static void generateWarning(QV8Engine *engine, const QString& description)
{
    if (!engine)
        return;
    v8::Handle<v8::StackTrace> currStack = v8::StackTrace::CurrentStackTrace(1);
    if (currStack.IsEmpty())
        return;
    v8::Handle<v8::StackFrame> currFrame = currStack->GetFrame(0);
    if (currFrame.IsEmpty())
        return;

    QQmlError retn;
    retn.setDescription(description);
    retn.setLine(currFrame->GetLineNumber());
    retn.setUrl(QUrl(currFrame->GetScriptName()->v4Value().toQString()));
    QQmlEnginePrivate::warning(engine->engine(), retn);
}

//  F(elementType, elementTypeName, sequenceType, defaultValue)
#define FOREACH_QML_SEQUENCE_TYPE(F) \
    F(int, Int, QList<int>, 0) \
    F(qreal, Real, QList<qreal>, 0.0) \
    F(bool, Bool, QList<bool>, false) \
    F(QString, String, QList<QString>, QString()) \
    F(QString, QString, QStringList, QString()) \
    F(QUrl, Url, QList<QUrl>, QUrl())

class QV4_JS_CLASS(QQmlSequenceBase) : public QV4::Object
{
public:
    QQmlSequenceBase(QV4::ExecutionEngine *engine)
        : QV4::Object(engine)
    {}

    void initClass(QV4::ExecutionEngine *engine);

    QV4::Value method_get_length(QV4::SimpleCallContext* ctx) QV4_ANNOTATE(attributes QV4::Attr_ReadOnly);

    QV4::Value method_set_length(QV4::SimpleCallContext* ctx);
};

class QV4_JS_CLASS(QQmlSequencePrototype) : public QV4::Object
{
    QV4_ANNOTATE(staticInitClass true)
public:
    static void initClass(QV4::ExecutionEngine *engine, const QV4::Value &value);

    static QV4::Value method_valueOf(QV4::SimpleCallContext *ctx)
    {
        return QV4::Value::fromString(ctx->thisObject.toString(ctx));
    }

    static QV4::Value method_sort(QV4::SimpleCallContext *ctx) QV4_ARGC(1);
};

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

template <typename Container, int ManagedType>
class QQmlSequence : public QQmlSequenceBase
{
public:
    QQmlSequence(QV4::ExecutionEngine *engine, const Container &container)
        : QQmlSequenceBase(engine)
        , m_container(container)
        , m_object(0)
        , m_propertyIndex(-1)
        , m_isReference(false)
    {
        type = ManagedType;
        vtbl = &static_vtbl;
        prototype = engine->arrayPrototype;
        initClass(engine);
    }

    QQmlSequence(QV4::ExecutionEngine *engine, QObject *object, int propertyIndex)
        : QQmlSequenceBase(engine)
        , m_object(object)
        , m_propertyIndex(propertyIndex)
        , m_isReference(true)
    {
        type = ManagedType;
        vtbl = &static_vtbl;
        prototype = engine->arrayPrototype;
        loadReference();
        initClass(engine);
    }

    QV4::Value containerGetIndexed(QV4::ExecutionContext *ctx, uint index, bool *hasProperty)
    {
        /* Qt containers have int (rather than uint) allowable indexes. */
        if (index > INT_MAX) {
            generateWarning(QV8Engine::get(ctx->engine->publicEngine), QLatin1String("Index out of range during indexed get"));
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
            generateWarning(QV8Engine::get(ctx->engine->publicEngine), QLatin1String("Index out of range during indexed put"));
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
            generateWarning(QV8Engine::get(ctx->engine->publicEngine), QLatin1String("Index out of range during indexed query"));
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
                QV4::Managed *fun = m_compareFn.asManaged();
                QV4::Value argv[2] = {
                    convertElementToValue(m_ctx, lhs),
                    convertElementToValue(m_ctx, rhs)
                };
                QV4::Value result = fun->call(m_ctx, QV4::Value::fromObject(m_ctx->engine->globalObject), argv, 2);
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
            generateWarning(QV8Engine::get(ctx->engine->publicEngine), QLatin1String("Index out of range during length set"));
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
    { return static_cast<QQmlSequence<Container, ManagedType> *>(that)->containerGetIndexed(ctx, index, hasProperty); }
    static void putIndexed(Managed *that, QV4::ExecutionContext *ctx, uint index, const QV4::Value &value)
    { static_cast<QQmlSequence<Container, ManagedType> *>(that)->containerPutIndexed(ctx, index, value); }
    static QV4::PropertyAttributes queryIndexed(QV4::Managed *that, QV4::ExecutionContext *ctx, uint index)
    { return static_cast<QQmlSequence<Container, ManagedType> *>(that)->containerQueryIndexed(ctx, index); }
    static bool deleteIndexedProperty(QV4::Managed *that, QV4::ExecutionContext *ctx, uint index)
    { return static_cast<QQmlSequence<Container, ManagedType> *>(that)->containerDeleteIndexedProperty(ctx, index); }

    static void destroy(Managed *that)
    {
        static_cast<QQmlSequence<Container, ManagedType> *>(that)->~QQmlSequence<Container, ManagedType>();
    }

    static const QV4::ManagedVTable static_vtbl;
};

typedef QQmlSequence<QStringList, QV4::Managed::Type_QmlQStringList> QQmlQStringList;
template<>
DEFINE_MANAGED_VTABLE(QQmlQStringList);
typedef QQmlSequence<QList<QString>, QV4::Managed::Type_QmlStringList> QQmlStringList;
template<>
DEFINE_MANAGED_VTABLE(QQmlStringList);
typedef QQmlSequence<QList<int>, QV4::Managed::Type_QmlIntList> QQmlIntList;
template<>
DEFINE_MANAGED_VTABLE(QQmlIntList);
typedef QQmlSequence<QList<QUrl>, QV4::Managed::Type_QmlUrlList> QQmlUrlList;
template<>
DEFINE_MANAGED_VTABLE(QQmlUrlList);
typedef QQmlSequence<QList<bool>, QV4::Managed::Type_QmlBoolList> QQmlBoolList;
template<>
DEFINE_MANAGED_VTABLE(QQmlBoolList);
typedef QQmlSequence<QList<qreal>, QV4::Managed::Type_QmlRealList> QQmlRealList;
template<>
DEFINE_MANAGED_VTABLE(QQmlRealList);

QV4::Value QQmlSequencePrototype::method_sort(QV4::SimpleCallContext *ctx)
{
    QV4::Object *o = ctx->thisObject.asObject();
    if (!o || !o->isListType())
        ctx->throwTypeError();

    if (ctx->argumentCount < 2) {
#define CALL_SORT(SequenceElementType, SequenceElementTypeName, SequenceType, DefaultValue) \
        case QV4::Managed::Type_Qml##SequenceElementTypeName##List: o->asQml##SequenceElementTypeName##List()->sort(ctx); break;

        switch (o->internalType()) {
            FOREACH_QML_SEQUENCE_TYPE(CALL_SORT)
            default: break;
        }

#undef CALL_SORT
    }
    return ctx->thisObject;
}

QV4::Value QQmlSequenceBase::method_get_length(QV4::SimpleCallContext* ctx) QV4_ANNOTATE(attributes QV4::Attr_ReadOnly)
{
#define CALL_LENGTH_GETTER(SequenceElementType, SequenceElementTypeName, SequenceType, DefaultValue) \
    case QV4::Managed::Type_Qml##SequenceElementTypeName##List: return asQml##SequenceElementTypeName##List()->lengthGetter(ctx);

    switch (internalType()) {
        FOREACH_QML_SEQUENCE_TYPE(CALL_LENGTH_GETTER)
        default: QV4::Value::undefinedValue();
    }

#undef CALL_LENGTH_GETTER
}

QV4::Value QQmlSequenceBase::method_set_length(QV4::SimpleCallContext* ctx)
{
#define CALL_LENGTH_SETTER(SequenceElementType, SequenceElementTypeName, SequenceType, DefaultValue) \
    case QV4::Managed::Type_Qml##SequenceElementTypeName##List: asQml##SequenceElementTypeName##List()->lengthSetter(ctx); break;

    switch (internalType()) {
        FOREACH_QML_SEQUENCE_TYPE(CALL_LENGTH_SETTER)
        default: break;
    }
#undef CALL_LENGTH_SETTER

    return QV4::Value::undefinedValue();
}

QT_END_NAMESPACE

#endif // QV8SEQUENCEWRAPPER_P_P_H
