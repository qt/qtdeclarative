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

#include "qv8sequencewrapper_p.h"
#include "qv8sequencewrapper_p_p.h"
#include "qv8engine_p.h"

QT_BEGIN_NAMESPACE

QV8SequenceWrapper::QV8SequenceWrapper()
    : m_engine(0)
{
}

QV8SequenceWrapper::~QV8SequenceWrapper()
{
}

#define REGISTER_QML_SEQUENCE_METATYPE(unused, unused2, SequenceType, unused3) qRegisterMetaType<SequenceType>();
void QV8SequenceWrapper::init(QV8Engine *engine)
{
    FOREACH_QML_SEQUENCE_TYPE(REGISTER_QML_SEQUENCE_METATYPE)

    m_engine = engine;
    m_toString = qPersistentNew<v8::Function>(v8::FunctionTemplate::New(ToString)->GetFunction());
    m_valueOf = qPersistentNew<v8::Function>(v8::FunctionTemplate::New(ValueOf)->GetFunction());

    QString defaultSortString = QLatin1String(
        "(function compare(x,y) {"
        "       if (x === y) return 0;"
        "       x = x.toString();"
        "       y = y.toString();"
        "       if (x == y) return 0;"
        "       else return x < y ? -1 : 1;"
        "})");

    m_sort = qPersistentNew<v8::Function>(v8::FunctionTemplate::New(Sort)->GetFunction());
    m_arrayPrototype = qPersistentNew<v8::Value>(v8::Array::New(1)->GetPrototype());
    v8::Local<v8::Script> defaultSortCompareScript = v8::Script::Compile(engine->toString(defaultSortString));
    m_defaultSortComparer = qPersistentNew<v8::Function>(v8::Handle<v8::Function>(v8::Function::Cast(*defaultSortCompareScript->Run())));

    v8::Local<v8::FunctionTemplate> ft = v8::FunctionTemplate::New();
    ft->InstanceTemplate()->SetFallbackPropertyHandler(Getter, Setter);
    ft->InstanceTemplate()->SetIndexedPropertyHandler(IndexedGetter, IndexedSetter, 0, IndexedDeleter, IndexedEnumerator);
    ft->InstanceTemplate()->SetAccessor(v8::String::New("length"), LengthGetter, LengthSetter,
                                        v8::Handle<v8::Value>(), v8::DEFAULT,
                                        v8::PropertyAttribute(v8::DontDelete | v8::DontEnum));
    ft->InstanceTemplate()->SetAccessor(v8::String::New("toString"), ToStringGetter, 0,
                                        m_toString, v8::DEFAULT,
                                        v8::PropertyAttribute(v8::ReadOnly | v8::DontDelete | v8::DontEnum));
    ft->InstanceTemplate()->SetAccessor(v8::String::New("valueOf"), ValueOfGetter, 0,
                                        m_valueOf, v8::DEFAULT,
                                        v8::PropertyAttribute(v8::ReadOnly | v8::DontDelete | v8::DontEnum));
    ft->InstanceTemplate()->SetAccessor(v8::String::New("sort"), SortGetter, 0,
                                        m_sort, v8::DEFAULT,
                                        v8::PropertyAttribute(v8::ReadOnly | v8::DontDelete | v8::DontEnum));
    ft->InstanceTemplate()->SetHasExternalResource(true);
    ft->InstanceTemplate()->MarkAsUseUserObjectComparison();
    m_constructor = qPersistentNew<v8::Function>(ft->GetFunction());
}
#undef REGISTER_QML_SEQUENCE_METATYPE

void QV8SequenceWrapper::destroy()
{
    qPersistentDispose(m_defaultSortComparer);
    qPersistentDispose(m_sort);
    qPersistentDispose(m_arrayPrototype);
    qPersistentDispose(m_toString);
    qPersistentDispose(m_valueOf);
    qPersistentDispose(m_constructor);
}

#define IS_SEQUENCE(unused1, unused2, SequenceType, unused3) \
    if (sequenceTypeId == qMetaTypeId<SequenceType>()) { \
        return true; \
    } else

bool QV8SequenceWrapper::isSequenceType(int sequenceTypeId) const
{
    FOREACH_QML_SEQUENCE_TYPE(IS_SEQUENCE) { /* else */ return false; }
}
#undef IS_SEQUENCE

bool QV8SequenceWrapper::isEqual(QV8ObjectResource *lhs, QV8ObjectResource *rhs)
{
    Q_ASSERT(lhs && rhs && lhs->resourceType() == QV8ObjectResource::SequenceType && rhs->resourceType() == QV8ObjectResource::SequenceType);
    QV8SequenceResource *lr = static_cast<QV8SequenceResource *>(lhs);
    QV8SequenceResource *rr = static_cast<QV8SequenceResource *>(rhs);
    return lr->isEqual(rr);
}

quint32 QV8SequenceWrapper::sequenceLength(QV8ObjectResource *r)
{
    Q_ASSERT(r->resourceType() == QV8ObjectResource::SequenceType);
    QV8SequenceResource *sr = static_cast<QV8SequenceResource *>(r);
    Q_ASSERT(sr);
    return sr->lengthGetter();
}

#define NEW_REFERENCE_SEQUENCE(ElementType, ElementTypeName, SequenceType, unused) \
    if (sequenceType == qMetaTypeId<SequenceType>()) { \
        r = new QV8##ElementTypeName##SequenceResource(m_engine, object, propertyIndex); \
    } else

v8::Local<v8::Object> QV8SequenceWrapper::newSequence(int sequenceType, QObject *object, int propertyIndex, bool *succeeded)
{
    // This function is called when the property is a QObject Q_PROPERTY of
    // the given sequence type.  Internally we store a typed-sequence
    // (as well as object ptr + property index for updated-read and write-back)
    // and so access/mutate avoids variant conversion.
    *succeeded = true;
    QV8SequenceResource *r = 0;
    FOREACH_QML_SEQUENCE_TYPE(NEW_REFERENCE_SEQUENCE) { /* else */ *succeeded = false; return v8::Local<v8::Object>(); }

    v8::Local<v8::Object> rv = m_constructor->NewInstance();
    rv->SetExternalResource(r);
    rv->SetPrototype(m_arrayPrototype);
    return rv;
}
#undef NEW_REFERENCE_SEQUENCE

#define NEW_COPY_SEQUENCE(ElementType, ElementTypeName, SequenceType, unused) \
    if (sequenceType == qMetaTypeId<SequenceType>()) { \
        r = new QV8##ElementTypeName##SequenceResource(m_engine, v.value<SequenceType>()); \
    } else

v8::Local<v8::Object> QV8SequenceWrapper::fromVariant(const QVariant& v, bool *succeeded)
{
    // This function is called when assigning a sequence value to a normal JS var
    // in a JS block.  Internally, we store a sequence of the specified type.
    // Access and mutation is extremely fast since it will not need to modify any
    // QObject property.
    int sequenceType = v.userType();
    *succeeded = true;
    QV8SequenceResource *r = 0;
    FOREACH_QML_SEQUENCE_TYPE(NEW_COPY_SEQUENCE) { /* else */ *succeeded = false; return v8::Local<v8::Object>(); }

    v8::Local<v8::Object> rv = m_constructor->NewInstance();
    rv->SetExternalResource(r);
    rv->SetPrototype(m_arrayPrototype);
    return rv;
}
#undef NEW_COPY_SEQUENCE

QVariant QV8SequenceWrapper::toVariant(QV8ObjectResource *r)
{
    Q_ASSERT(r->resourceType() == QV8ObjectResource::SequenceType);
    QV8SequenceResource *resource = static_cast<QV8SequenceResource *>(r);
    return resource->toVariant();
}

#define SEQUENCE_TO_VARIANT(ElementType, ElementTypeName, SequenceType, unused) \
    if (typeHint == qMetaTypeId<SequenceType>()) { \
        return QV8##ElementTypeName##SequenceResource::toVariant(m_engine, array, length, succeeded); \
    } else

QVariant QV8SequenceWrapper::toVariant(v8::Handle<v8::Array> array, int typeHint, bool *succeeded)
{
    *succeeded = true;
    uint32_t length = array->Length();
    FOREACH_QML_SEQUENCE_TYPE(SEQUENCE_TO_VARIANT) { /* else */ *succeeded = false; return QVariant(); }
}
#undef SEQUENCE_TO_VARIANT

v8::Handle<v8::Value> QV8SequenceWrapper::IndexedSetter(quint32 index, v8::Local<v8::Value> value, const v8::AccessorInfo &info)
{
    QV8SequenceResource *sr = v8_resource_cast<QV8SequenceResource>(info.This());
    Q_ASSERT(sr);
    return sr->indexedSetter(index, value);
}

v8::Handle<v8::Value> QV8SequenceWrapper::IndexedGetter(quint32 index, const v8::AccessorInfo &info)
{
    QV8SequenceResource *sr = v8_resource_cast<QV8SequenceResource>(info.This());
    Q_ASSERT(sr);
    return sr->indexedGetter(index);
}

v8::Handle<v8::Boolean> QV8SequenceWrapper::IndexedDeleter(quint32 index, const v8::AccessorInfo &info)
{
    QV8SequenceResource *sr = v8_resource_cast<QV8SequenceResource>(info.This());
    Q_ASSERT(sr);
    return sr->indexedDeleter(index);
}

v8::Handle<v8::Array> QV8SequenceWrapper::IndexedEnumerator(const v8::AccessorInfo &info)
{
    QV8SequenceResource *sr = v8_resource_cast<QV8SequenceResource>(info.This());
    Q_ASSERT(sr);
    return sr->indexedEnumerator();
}

v8::Handle<v8::Value> QV8SequenceWrapper::LengthGetter(v8::Local<v8::String> property, const v8::AccessorInfo &info)
{
    Q_UNUSED(property);
    QV8SequenceResource *sr = v8_resource_cast<QV8SequenceResource>(info.This());
    Q_ASSERT(sr);
    return v8::Integer::NewFromUnsigned(sr->lengthGetter());
}

void QV8SequenceWrapper::LengthSetter(v8::Local<v8::String> property, v8::Local<v8::Value> value, const v8::AccessorInfo &info)
{
    Q_UNUSED(property);
    QV8SequenceResource *sr = v8_resource_cast<QV8SequenceResource>(info.This());
    Q_ASSERT(sr);
    sr->lengthSetter(value);
}

v8::Handle<v8::Value> QV8SequenceWrapper::ToStringGetter(v8::Local<v8::String> property, const v8::AccessorInfo &info)
{
    Q_UNUSED(property);
    return info.Data();
}

v8::Handle<v8::Value> QV8SequenceWrapper::ValueOfGetter(v8::Local<v8::String> property,
                                                               const v8::AccessorInfo &info)
{
    Q_UNUSED(property);
    return info.Data();
}

v8::Handle<v8::Value> QV8SequenceWrapper::SortGetter(v8::Local<v8::String> property, const v8::AccessorInfo &info)
{
    Q_UNUSED(property);
    return info.Data();
}

v8::Handle<v8::Value> QV8SequenceWrapper::Sort(const v8::Arguments &args)
{
    int argCount = args.Length();

    if (argCount < 2) {
        QV8SequenceResource *sr = v8_resource_cast<QV8SequenceResource>(args.This());
        Q_ASSERT(sr);

        qint32 length = sr->lengthGetter();
        if (length > 1) {
            v8::Handle<v8::Function> jsCompareFn = sr->engine->sequenceWrapper()->m_defaultSortComparer;
            if (argCount == 1 && args[0]->IsFunction())
                jsCompareFn = v8::Handle<v8::Function>(v8::Function::Cast(*args[0]));

            sr->sort(jsCompareFn);
        }
    }

    return args.This();
}

v8::Handle<v8::Value> QV8SequenceWrapper::ToString(const v8::Arguments &args)
{
    QV8SequenceResource *sr = v8_resource_cast<QV8SequenceResource>(args.This());
    Q_ASSERT(sr);
    return sr->toString();
}

v8::Handle<v8::Value> QV8SequenceWrapper::ValueOf(const v8::Arguments &args)
{
    QV8SequenceResource *sr = v8_resource_cast<QV8SequenceResource>(args.This());
    Q_ASSERT(sr);
    v8::Handle<v8::Value> tostringValue = sr->toString();
    if (!tostringValue.IsEmpty())
        return tostringValue;
    return v8::Integer::NewFromUnsigned(sr->lengthGetter());
}

v8::Handle<v8::Value> QV8SequenceWrapper::Getter(v8::Local<v8::String> property,
                                                    const v8::AccessorInfo &info)
{
    Q_UNUSED(property);
    Q_UNUSED(info);
    return v8::Handle<v8::Value>();
}

v8::Handle<v8::Value> QV8SequenceWrapper::Setter(v8::Local<v8::String> property,
                                                    v8::Local<v8::Value> value,
                                                    const v8::AccessorInfo &info)
{
    Q_UNUSED(property);
    Q_UNUSED(info);
    return value;
}

QT_END_NAMESPACE
