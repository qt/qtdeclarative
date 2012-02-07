/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
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
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qfastmetabuilder_p.h"

#include <QtCore/qmetaobject.h>
#include <private/qobject_p.h>
#include <private/qmetaobject_p.h>

QT_BEGIN_NAMESPACE

struct QFastMetaBuilderHeader
{
    int fieldCount;
};

#define FMBHEADER_FIELD_COUNT 1

#define HEADER_FIELD_COUNT 14
#define CLASSINFO_FIELD_COUNT 2
#define METHOD_FIELD_COUNT 5
#define PROPERTY_FIELD_COUNT 3
#define PROPERTY_NOTIFY_FIELD_COUNT 1

static inline uint *fieldPointer(QByteArray &data)
{ return reinterpret_cast<uint *>(data.data()) + FMBHEADER_FIELD_COUNT; }

static inline const uint *fieldPointer(const QByteArray &data)
{ return reinterpret_cast<const uint *>(data.constData()) + FMBHEADER_FIELD_COUNT; }

static inline QMetaObjectPrivate *priv(QByteArray &data)
{ return reinterpret_cast<QMetaObjectPrivate*>(fieldPointer(data)); }

static inline const QMetaObjectPrivate *priv(const QByteArray &data)
{ return reinterpret_cast<const QMetaObjectPrivate*>(fieldPointer(data)); }

static inline QFastMetaBuilderHeader *header(QByteArray &data)
{ return reinterpret_cast<QFastMetaBuilderHeader*>(data.data()); }

static inline const QFastMetaBuilderHeader *header(const QByteArray &data)
{ return reinterpret_cast<const QFastMetaBuilderHeader*>(data.constData()); }

QFastMetaBuilder::QFastMetaBuilder()
: m_zeroPtr(0), m_stringData(0), m_stringDataLength(0), m_stringDataAllocated(0)
{
}

QFastMetaBuilder::~QFastMetaBuilder()
{
}

QFastMetaBuilder::StringRef QFastMetaBuilder::init(int classNameLength,
                                                   int propertyCount, int methodCount, 
                                                   int signalCount, int classInfoCount)
{
    Q_ASSERT(m_data.isEmpty());
    Q_ASSERT(classNameLength > 0);
    Q_ASSERT(propertyCount >= 0);
    Q_ASSERT(methodCount >= 0);
    Q_ASSERT(signalCount >= 0);
    Q_ASSERT(classInfoCount >= 0);

    int fieldCount = FMBHEADER_FIELD_COUNT +
                     HEADER_FIELD_COUNT + 
                     propertyCount * (PROPERTY_FIELD_COUNT + PROPERTY_NOTIFY_FIELD_COUNT) +
                     methodCount * (METHOD_FIELD_COUNT) +
                     signalCount * (METHOD_FIELD_COUNT) +
                     classInfoCount * CLASSINFO_FIELD_COUNT;

    m_data.resize(fieldCount * sizeof(uint) + classNameLength + 1);
    m_stringData = m_data.data() + m_data.size() - classNameLength - 1;
    m_stringDataLength = classNameLength + 1;
    m_stringDataAllocated = classNameLength + 1;
    m_stringData[classNameLength] = 0;
    m_zeroPtr = classNameLength;

    header(m_data)->fieldCount = fieldCount;

    QMetaObjectPrivate *p = priv(m_data);

    int dataIndex = HEADER_FIELD_COUNT;

    p->revision = 6;
    p->className = 0;

    // Class infos
    p->classInfoCount = classInfoCount;
    if (p->classInfoCount) { 
        p->classInfoData = dataIndex;
        dataIndex += p->classInfoCount * CLASSINFO_FIELD_COUNT;
    } else {
        p->classInfoData = 0;
    }

    // Methods
    p->methodCount = methodCount + signalCount;
    if (p->methodCount) {
        p->methodData = dataIndex;
        dataIndex += p->methodCount * METHOD_FIELD_COUNT;
    } else {
        p->methodData = 0;
    }
    p->signalCount = signalCount;

    // Properties
    p->propertyCount = propertyCount;
    if (p->propertyCount) {
        p->propertyData = dataIndex;
        dataIndex += p->propertyCount * (PROPERTY_FIELD_COUNT + PROPERTY_NOTIFY_FIELD_COUNT);
    } else {
        p->propertyData = 0;
    }

    // Flags
    p->flags = DynamicMetaObject; // Always dynamic

    // Enums and constructors not supported
    p->enumeratorCount = 0;
    p->enumeratorData = 0;
    p->constructorCount = 0;
    p->constructorData = 0;

    StringRef className;
    className._b = this;
    className._o = 0;
    className._l = classNameLength;
    return className;
}

// Allocate a string of \a length.  \a length should *not* include the null terminator.
QFastMetaBuilder::StringRef QFastMetaBuilder::newString(int length)
{
    Q_ASSERT(length > 0);

    StringRef sr;
    sr._b = this;
    sr._o = m_stringDataLength;
    sr._l = length;

    m_stringDataLength += length + 1 /* for null terminator */;

    return sr;
}

void QFastMetaBuilder::setClassInfo(int index, const StringRef &key, const StringRef &value)
{
    Q_ASSERT(!m_data.isEmpty());
    Q_ASSERT(!key.isEmpty() && !value.isEmpty());

    QMetaObjectPrivate *p = priv(m_data);
    Q_ASSERT(index < p->classInfoCount);

    uint *ptr = fieldPointer(m_data) + p->classInfoData + index * CLASSINFO_FIELD_COUNT;
    // classinfo: key, value
    ptr[0] = key.offset(); ptr[1] = value.offset();
}

void QFastMetaBuilder::setProperty(int index, const StringRef &name, const StringRef &type, 
                                   QMetaType::Type mtype, PropertyFlag flags, int notifySignal)
{
    Q_ASSERT(!m_data.isEmpty());
    Q_ASSERT(!name.isEmpty() && !type.isEmpty());

    QMetaObjectPrivate *p = priv(m_data);
    Q_ASSERT(index < p->propertyCount);

    uint *ptr = fieldPointer(m_data) + p->propertyData + index * PROPERTY_FIELD_COUNT;
    // properties: name, type, flags
    ptr[0] = name.offset();
    ptr[1] = type.offset();
    if (notifySignal == -1) {
        ptr[2] = mtype << 24;
        ptr[2] |= flags | Scriptable | Readable;
        *(fieldPointer(m_data) + p->propertyData + p->propertyCount * PROPERTY_FIELD_COUNT + index) = 0;
    } else {
        ptr[2] = mtype << 24;
        ptr[2] |= flags | Scriptable | Readable | Notify;
        *(fieldPointer(m_data) + p->propertyData + p->propertyCount * PROPERTY_FIELD_COUNT + index) = notifySignal;
    }
}

void QFastMetaBuilder::setProperty(int index, const StringRef &name, const StringRef &type, 
                                   QFastMetaBuilder::PropertyFlag flags, int notifySignal)
{
    Q_ASSERT(!m_data.isEmpty());
    Q_ASSERT(!name.isEmpty() && !type.isEmpty());

    QMetaObjectPrivate *p = priv(m_data);
    Q_ASSERT(index < p->propertyCount);

    uint *ptr = fieldPointer(m_data) + p->propertyData + index * PROPERTY_FIELD_COUNT;
    // properties: name, type, flags
    ptr[0] = name.offset();
    ptr[1] = type.offset();
    if (notifySignal == -1) {
        ptr[2] = flags | Scriptable | Readable;
        *(fieldPointer(m_data) + p->propertyData + p->propertyCount * PROPERTY_FIELD_COUNT + index) = 0;
    } else {
        ptr[2] = flags | Scriptable | Readable | Notify;
        *(fieldPointer(m_data) + p->propertyData + p->propertyCount * PROPERTY_FIELD_COUNT + index) = notifySignal;
    }
}

void QFastMetaBuilder::setSignal(int index, const StringRef &signature,
                                 const StringRef &parameterNames,
                                 const StringRef &type)
{
    Q_ASSERT(!m_data.isEmpty());
    Q_ASSERT(!signature.isEmpty());

    QMetaObjectPrivate *p = priv(m_data);
    int mindex = metaObjectIndexForSignal(index);

    uint *ptr = fieldPointer(m_data) + p->methodData + mindex * METHOD_FIELD_COUNT;
    // methods: signature, parameters, type, tag, flags
    ptr[0] = signature.offset();
    ptr[1] = parameterNames.isEmpty()?m_zeroPtr:parameterNames.offset();
    ptr[2] = type.isEmpty()?m_zeroPtr:type.offset();
    ptr[3] = m_zeroPtr;
    ptr[4] = AccessProtected | MethodSignal;
}

void QFastMetaBuilder::setMethod(int index, const StringRef &signature,
                                 const StringRef &parameterNames,
                                 const StringRef &type)
{
    Q_ASSERT(!m_data.isEmpty());
    Q_ASSERT(!signature.isEmpty());

    QMetaObjectPrivate *p = priv(m_data);
    int mindex = metaObjectIndexForMethod(index);

    uint *ptr = fieldPointer(m_data) + p->methodData + mindex * METHOD_FIELD_COUNT;
    // methods: signature, parameters, type, tag, flags
    ptr[0] = signature.offset();
    ptr[1] = parameterNames.isEmpty()?m_zeroPtr:parameterNames.offset();
    ptr[2] = type.isEmpty()?m_zeroPtr:type.offset();
    ptr[3] = m_zeroPtr;
    ptr[4] = AccessProtected | MethodSlot;
}

int QFastMetaBuilder::metaObjectIndexForSignal(int index) const
{
    Q_ASSERT(!m_data.isEmpty());
    Q_ASSERT(index < priv(m_data)->signalCount);
    return index;
}

int QFastMetaBuilder::metaObjectIndexForMethod(int index) const
{
    Q_ASSERT(!m_data.isEmpty());

    const QMetaObjectPrivate *p = priv(m_data);
    Q_ASSERT(index < (p->methodCount - p->signalCount));
    return index + p->signalCount;
}

void QFastMetaBuilder::allocateStringData()
{
    if (m_stringDataAllocated < m_stringDataLength) {
        m_data.resize(m_data.size() + m_stringDataLength - m_stringDataAllocated);
        m_stringDataAllocated = m_stringDataLength;
        m_stringData = m_data.data() + header(m_data)->fieldCount * sizeof(uint);
    }
}

void QFastMetaBuilder::fromData(QMetaObject *output, const QMetaObject *parent, const QByteArray &data)
{
    output->d.superdata = parent;
    output->d.stringdata = data.constData() + header(data)->fieldCount * sizeof(uint);
    output->d.data = fieldPointer(data);
    output->d.extradata = 0;
}

QT_END_NAMESPACE
