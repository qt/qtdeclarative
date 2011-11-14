/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtDeclarative module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
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

#include <private/qdeclarativeengine_p.h>
#include <private/qdeclarativemetatype_p.h>

QT_BEGIN_NAMESPACE

/*!
  \internal
  \class QV8SequenceResource
  \brief The abstract base class of the external resource used in sequence type objects

  Every sequence type object returned by QV8SequenceWrapper::fromVariant() or
  QV8SequenceWrapper::newSequence() has a type-specific QV8SequenceResource which
  contains the type name, the meta type ids of the sequence and sequence element
  types, as well as either the sequence data (copy) or object pointer and property
  index (reference) data associated with the sequence.
 */
class QV8SequenceResource : public QV8ObjectResource
{
    V8_RESOURCE_TYPE(SequenceType);

public:
    virtual ~QV8SequenceResource() {}

    enum ObjectType { Reference, Copy };

    virtual QVariant toVariant() = 0;
    virtual bool isEqual(const QV8SequenceResource *v) = 0;

    virtual quint32 lengthGetter() = 0;
    virtual v8::Handle<v8::Value> indexedSetter(quint32 index, v8::Handle<v8::Value> value) = 0;
    virtual v8::Handle<v8::Value> indexedGetter(quint32 index) = 0;
    virtual v8::Handle<v8::Array> indexedEnumerator() = 0;
    virtual v8::Handle<v8::Value> toString() = 0;

    ObjectType objectType;
    QByteArray typeName;
    int sequenceMetaTypeId;
    int elementMetaTypeId;

protected:
    QV8SequenceResource(QV8Engine *engine, ObjectType type, const QByteArray &name, int sequenceId, int elementId)
        : QV8ObjectResource(engine), objectType(type), typeName(name), sequenceMetaTypeId(sequenceId), elementMetaTypeId(elementId)
    {
    }
};

static int convertV8ValueToInt(QV8Engine *, v8::Handle<v8::Value> v)
{
    return v->Int32Value();
}

static v8::Handle<v8::Value> convertIntToV8Value(QV8Engine *, int v)
{
    return v8::Integer::New(v);
}

static QString convertIntToString(QV8Engine *, int v)
{
    return QString::number(v);
}

static qreal convertV8ValueToReal(QV8Engine *, v8::Handle<v8::Value> v)
{
    return v->NumberValue();
}

static v8::Handle<v8::Value> convertRealToV8Value(QV8Engine *, qreal v)
{
    return v8::Number::New(v);
}

static QString convertRealToString(QV8Engine *, qreal v)
{
    return QString::number(v);
}

static bool convertV8ValueToBool(QV8Engine *, v8::Handle<v8::Value> v)
{
    return v->BooleanValue();
}

static v8::Handle<v8::Value> convertBoolToV8Value(QV8Engine *, bool v)
{
    return v8::Boolean::New(v);
}

static QString convertBoolToString(QV8Engine *, bool v)
{
    if (v)
        return QLatin1String("true");
    return QLatin1String("false");
}

static QString convertV8ValueToString(QV8Engine *e, v8::Handle<v8::Value> v)
{
    return e->toString(v->ToString());
}

static v8::Handle<v8::Value> convertStringToV8Value(QV8Engine *e, const QString &v)
{
    return e->toString(v);
}

static QString convertStringToString(QV8Engine *, const QString &v)
{
    return v;
}

static QString convertV8ValueToQString(QV8Engine *e, v8::Handle<v8::Value> v)
{
    return e->toString(v->ToString());
}

static v8::Handle<v8::Value> convertQStringToV8Value(QV8Engine *e, const QString &v)
{
    return e->toString(v);
}

static QString convertQStringToString(QV8Engine *, const QString &v)
{
    return v;
}

static QUrl convertV8ValueToUrl(QV8Engine *e, v8::Handle<v8::Value> v)
{
    return QUrl(e->toString(v->ToString()));
}

static v8::Handle<v8::Value> convertUrlToV8Value(QV8Engine *e, const QUrl &v)
{
    return e->toString(v.toString());
}

static QString convertUrlToString(QV8Engine *, const QUrl &v)
{
    return v.toString();
}


/*
  \internal
  \class QV8<Type>SequenceResource
  \brief The external resource used in sequence type objects

  Every sequence type object returned by QV8SequenceWrapper::newSequence() has
  a QV8<Type>SequenceResource which contains a property index and a pointer
  to the object which contains the property.

  Every sequence type object returned by QV8SequenceWrapper::fromVariant() has
  a QV8<Type>SequenceResource which contains a copy of the sequence value.
  Operations on the sequence are implemented directly in terms of that sequence data.

  There exists one QV8<Type>SequenceResource instance for every JavaScript Object
  (sequence) instance returned from QV8SequenceWrapper::newSequence() or
  QV8SequenceWrapper::fromVariant().
 */

//  F(elementType, elementTypeName, sequenceType, defaultValue)
#define FOREACH_QML_SEQUENCE_TYPE(F) \
    F(int, Int, QList<int>, 0) \
    F(qreal, Real, QList<qreal>, 0.0) \
    F(bool, Bool, QList<bool>, false) \
    F(QString, String, QList<QString>, QString()) \
    F(QString, QString, QStringList, QString()) \
    F(QUrl, Url, QList<QUrl>, QUrl())

#define QML_SEQUENCE_TYPE_RESOURCE(SequenceElementType, SequenceElementTypeName, SequenceType, DefaultValue, ConversionToV8fn, ConversionFromV8fn, ToStringfn) \
    Q_DECLARE_METATYPE(SequenceType) \
    class QV8##SequenceElementTypeName##SequenceResource : public QV8SequenceResource { \
        public:\
            QV8##SequenceElementTypeName##SequenceResource(QV8Engine *engine, QObject *obj, int propIdx) \
                : QV8SequenceResource(engine, QV8SequenceResource::Reference, #SequenceType, qMetaTypeId<SequenceType>(), qMetaTypeId<SequenceElementType>()) \
                , object(obj), propertyIndex(propIdx) \
            { \
            } \
            QV8##SequenceElementTypeName##SequenceResource(QV8Engine *engine, const SequenceType &value) \
                : QV8SequenceResource(engine, QV8SequenceResource::Copy, #SequenceType, qMetaTypeId<SequenceType>(), qMetaTypeId<SequenceElementType>()) \
                , object(0), propertyIndex(-1), c(value) \
            { \
            } \
            ~QV8##SequenceElementTypeName##SequenceResource() \
            { \
            } \
            static QVariant toVariant(QV8Engine *e, v8::Handle<v8::Array> array, uint32_t length, bool *succeeded) \
            { \
                SequenceType list; \
                for (uint32_t ii = 0; ii < length; ++ii) { \
                    list.append(ConversionFromV8fn(e, array->Get(ii))); \
                } \
                *succeeded = true; \
                return QVariant::fromValue<SequenceType>(list); \
            } \
            QVariant toVariant() \
            { \
                if (objectType == QV8SequenceResource::Reference) { \
                    if (!object) \
                        return QVariant(); \
                    void *a[] = { &c, 0 }; \
                    QMetaObject::metacall(object, QMetaObject::ReadProperty, propertyIndex, a); \
                } \
                return QVariant::fromValue<SequenceType>(c); \
            } \
            bool isEqual(const QV8SequenceResource *v) \
            { \
                /* Note: two different sequences can never be equal (even if they  */ \
                /* contain the same elements in the same order) in order to        */ \
                /* maintain JavaScript semantics.  However, if they both reference */ \
                /* the same QObject+propertyIndex, they are equal.                 */ \
                if (objectType == QV8SequenceResource::Reference && v->objectType == QV8SequenceResource::Reference) { \
                    if (sequenceMetaTypeId == v->sequenceMetaTypeId) { \
                        const QV8##SequenceElementTypeName##SequenceResource *rhs = static_cast<const QV8##SequenceElementTypeName##SequenceResource *>(v); \
                        return (object != 0 && object == rhs->object && propertyIndex == rhs->propertyIndex); \
                    } \
                } else if (objectType == QV8SequenceResource::Copy && v->objectType == QV8SequenceResource::Copy) { \
                    if (sequenceMetaTypeId == v->sequenceMetaTypeId) { \
                        const QV8##SequenceElementTypeName##SequenceResource *rhs = static_cast<const QV8##SequenceElementTypeName##SequenceResource *>(v); \
                        return (this == rhs); \
                    } \
                } \
                return false; \
            } \
            quint32 lengthGetter() \
            { \
                if (objectType == QV8SequenceResource::Reference) { \
                    if (!object) \
                        return 0; \
                    void *a[] = { &c, 0 }; \
                    QMetaObject::metacall(object, QMetaObject::ReadProperty, propertyIndex, a); \
                } \
                return c.count(); \
            } \
            v8::Handle<v8::Value> indexedSetter(quint32 index, v8::Handle<v8::Value> value) \
            { \
                if (objectType == QV8SequenceResource::Reference) { \
                    if (!object) \
                        return v8::Undefined(); \
                    void *a[] = { &c, 0 }; \
                    QMetaObject::metacall(object, QMetaObject::ReadProperty, propertyIndex, a); \
                } \
                /* modify the sequence */ \
                SequenceElementType elementValue = ConversionFromV8fn(engine, value); \
                quint32 count = c.count(); \
                if (index == count) { \
                    c.append(elementValue); \
                } else if (index < count) { \
                    c[index] = elementValue; \
                } else { \
                    /* according to ECMA262r3 we need to insert */ \
                    /* the value at the given index, increasing length to index+1. */ \
                    while (index > count++) { \
                        c.append(DefaultValue); \
                    } \
                    c.append(elementValue); \
                } \
                if (objectType == QV8SequenceResource::Reference) { \
                    /* write back.  already checked that object is non-null, so skip that check here. */ \
                    int status = -1; \
                    QDeclarativePropertyPrivate::WriteFlags flags = QDeclarativePropertyPrivate::DontRemoveBinding; \
                    void *a[] = { &c, 0, &status, &flags }; \
                    QMetaObject::metacall(object, QMetaObject::WriteProperty, propertyIndex, a); \
                } \
                return value; \
            } \
            v8::Handle<v8::Value> indexedGetter(quint32 index) \
            { \
                if (objectType == QV8SequenceResource::Reference) { \
                    if (!object) \
                        return v8::Undefined(); \
                    void *a[] = { &c, 0 }; \
                    QMetaObject::metacall(object, QMetaObject::ReadProperty, propertyIndex, a); \
                } \
                quint32 count = c.count(); \
                if (index < count) \
                    return ConversionToV8fn(engine, c.at(index)); \
                return v8::Undefined(); \
            } \
            v8::Handle<v8::Array> indexedEnumerator() \
            { \
                if (objectType == QV8SequenceResource::Reference) { \
                    if (!object) \
                        return v8::Handle<v8::Array>(); \
                    void *a[] = { &c, 0 }; \
                    QMetaObject::metacall(object, QMetaObject::ReadProperty, propertyIndex, a); \
                } \
                quint32 count = c.count(); \
                v8::Local<v8::Array> retn = v8::Array::New(count); \
                for (quint32 i = 0; i < count; ++i) { \
                    retn->Set(i, v8::Integer::NewFromUnsigned(i)); \
                } \
                return retn; \
            } \
            v8::Handle<v8::Value> toString() \
            { \
                if (objectType == QV8SequenceResource::Reference) { \
                    if (!object) \
                        return v8::Undefined(); \
                    void *a[] = { &c, 0 }; \
                    QMetaObject::metacall(object, QMetaObject::ReadProperty, propertyIndex, a); \
                } \
                QString str; \
                quint32 count = c.count(); \
                for (quint32 i = 0; i < count; ++i) { \
                    str += QString(QLatin1String("%1,")).arg(ToStringfn(engine, c[i])); \
                } \
                str.chop(1); \
                return engine->toString(str); \
            } \
        private: \
            QDeclarativeGuard<QObject> object; \
            int propertyIndex; \
            SequenceType c; \
    };

#define GENERATE_QML_SEQUENCE_TYPE_RESOURCE(ElementType, ElementTypeName, SequenceType, DefaultValue) \
    QML_SEQUENCE_TYPE_RESOURCE(ElementType, ElementTypeName, SequenceType, DefaultValue, convert##ElementTypeName##ToV8Value, convertV8ValueTo##ElementTypeName, convert##ElementTypeName##ToString)

FOREACH_QML_SEQUENCE_TYPE(GENERATE_QML_SEQUENCE_TYPE_RESOURCE)
#undef GENERATE_QML_SEQUENCE_TYPE_RESOURCE
#undef QML_SEQUENCE_TYPE_RESOURCE

#endif // QV8SEQUENCEWRAPPER_P_P_H
