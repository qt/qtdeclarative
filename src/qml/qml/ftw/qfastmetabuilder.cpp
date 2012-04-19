/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the QtQml module of the Qt Toolkit.
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
    : m_stringData(0), m_stringCount(0), m_stringDataLength(0),
      m_stringCountAllocated(0), m_stringCountLoaded(0)
{
}

QFastMetaBuilder::~QFastMetaBuilder()
{
}

QFastMetaBuilder::StringRef QFastMetaBuilder::init(int classNameLength,
                                                   int propertyCount, int methodCount, 
                                                   int signalCount, int classInfoCount,
                                                   int paramDataSize, int *paramIndex)
{
    Q_ASSERT(m_data.isEmpty());
    Q_ASSERT(classNameLength > 0);
    Q_ASSERT(propertyCount >= 0);
    Q_ASSERT(methodCount >= 0);
    Q_ASSERT(signalCount >= 0);
    Q_ASSERT(classInfoCount >= 0);
    Q_ASSERT(paramDataSize >= 0);
    Q_ASSERT((paramIndex != 0) || (methodCount + signalCount == 0));

    int fieldCount = FMBHEADER_FIELD_COUNT +
                     HEADER_FIELD_COUNT + 
                     propertyCount * (PROPERTY_FIELD_COUNT + PROPERTY_NOTIFY_FIELD_COUNT) +
                     methodCount * (METHOD_FIELD_COUNT) +
                     signalCount * (METHOD_FIELD_COUNT) +
                     paramDataSize +
                     classInfoCount * CLASSINFO_FIELD_COUNT;
    // Ensure stringdata alignment (void*)
    fieldCount += fieldCount % (sizeof(void*) / sizeof(uint));

    m_stringCount = 2; // class name and zero string
    m_stringDataLength = classNameLength + 1;
    m_data.resize(fieldCount * sizeof(uint) + m_stringCount * sizeof(QByteArrayData) + m_stringDataLength);
    m_stringCountAllocated = m_stringCount;
    m_stringData = reinterpret_cast<QByteArrayData *>(m_data.data() + fieldCount * sizeof(uint));

    m_zeroString._b = this;
    m_zeroString._i = 1;
    m_zeroString._o = classNameLength;
    m_zeroString._l = 0;

    header(m_data)->fieldCount = fieldCount;

    QMetaObjectPrivate *p = priv(m_data);

    int dataIndex = HEADER_FIELD_COUNT;

    p->revision = 7;
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
        *paramIndex = dataIndex;
        dataIndex += paramDataSize;
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
    className._i = 0;
    className._o = 0;
    className._l = classNameLength;
    return className;
}

// Allocate a string of \a length.  \a length should *not* include the null terminator.
QFastMetaBuilder::StringRef QFastMetaBuilder::newString(int length)
{
    Q_ASSERT(length > 0);
    Q_ASSERT_X(m_stringCountLoaded == 0, Q_FUNC_INFO,
               "All strings must be created before string loading begins");

    StringRef sr;
    sr._b = this;
    sr._i = m_stringCount;
    sr._o = m_stringDataLength;
    sr._l = length;

    ++m_stringCount;
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
    ptr[0] = key.index(); ptr[1] = value.index();
}

void QFastMetaBuilder::setProperty(int index, const StringRef &name, int type,
                                   PropertyFlag flags, int notifySignal)
{
    Q_ASSERT(!m_data.isEmpty());
    Q_ASSERT(!name.isEmpty());
    Q_ASSERT(type != 0);
    Q_ASSERT(QMetaType::isRegistered(type));

    QMetaObjectPrivate *p = priv(m_data);
    Q_ASSERT(index < p->propertyCount);

    uint *ptr = fieldPointer(m_data) + p->propertyData + index * PROPERTY_FIELD_COUNT;
    // properties: name, type, flags
    ptr[0] = name.index();
    ptr[1] = type;
    if (notifySignal == -1) {
        ptr[2] = flags | Scriptable | Readable;
        *(fieldPointer(m_data) + p->propertyData + p->propertyCount * PROPERTY_FIELD_COUNT + index) = 0;
    } else {
        ptr[2] = flags | Scriptable | Readable | Notify;
        *(fieldPointer(m_data) + p->propertyData + p->propertyCount * PROPERTY_FIELD_COUNT + index) = notifySignal;
    }
}

void QFastMetaBuilder::setSignal(int index, const StringRef &name,
                                 int paramIndex, int argc, const int *types,
                                 const StringRef *parameterNames,
                                 QMetaType::Type type)
{
    Q_ASSERT(!m_data.isEmpty());
    Q_ASSERT(!name.isEmpty());
    Q_ASSERT(QMetaType::isRegistered(type));

    QMetaObjectPrivate *p = priv(m_data);
    int mindex = metaObjectIndexForSignal(index);

    uint *ptr = fieldPointer(m_data) + p->methodData + mindex * METHOD_FIELD_COUNT;
    // methods: name, arc, parameters, tag, flags
    ptr[0] = name.index();
    ptr[1] = argc;
    ptr[2] = paramIndex;
    ptr[3] = m_zeroString.index();
    ptr[4] = AccessProtected | MethodSignal;

    uint *paramPtr = fieldPointer(m_data) + paramIndex;
    paramPtr[0] = type;
    if (argc) {
        Q_ASSERT(types != 0);
        Q_ASSERT(parameterNames != 0);
        for (int i = 0; i < argc; ++i) {
            Q_ASSERT(types[i] != 0);
            Q_ASSERT(QMetaType::isRegistered(types[i]));
            paramPtr[1+i] = types[i];
            paramPtr[1+argc+i] = parameterNames[i].index();
        }
    }
}

void QFastMetaBuilder::setMethod(int index, const StringRef &name,
                                 int paramIndex, int argc, const int *types,
                                 const StringRef *parameterNames,
                                 QMetaType::Type type)
{
    Q_ASSERT(!m_data.isEmpty());
    Q_ASSERT(!name.isEmpty());
    Q_ASSERT(QMetaType::isRegistered(type));

    QMetaObjectPrivate *p = priv(m_data);
    int mindex = metaObjectIndexForMethod(index);

    uint *ptr = fieldPointer(m_data) + p->methodData + mindex * METHOD_FIELD_COUNT;
    // methods: name, arc, parameters, tag, flags
    ptr[0] = name.index();
    ptr[1] = argc;
    ptr[2] = paramIndex;
    ptr[3] = m_zeroString.index();
    ptr[4] = AccessProtected | MethodSlot;

    uint *paramPtr = fieldPointer(m_data) + paramIndex;
    paramPtr[0] = type;
    if (argc) {
        Q_ASSERT(types != 0);
        Q_ASSERT(parameterNames != 0);
        for (int i = 0; i < argc; ++i) {
            Q_ASSERT(types[i] != 0);
            Q_ASSERT(QMetaType::isRegistered(types[i]));
            paramPtr[1+i] = types[i];
            paramPtr[1+argc+i] = parameterNames[i].index();
        }
    }
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
    if (m_stringCountAllocated < m_stringCount) {
        m_data.resize(header(m_data)->fieldCount * sizeof(uint)
                      + m_stringCount * sizeof(QByteArrayData) + m_stringDataLength);
        m_stringCountAllocated = m_stringCount;
        char *rawStringData = m_data.data() + header(m_data)->fieldCount * sizeof(uint);
        m_stringData = reinterpret_cast<QByteArrayData *>(rawStringData);
    }
}

void QFastMetaBuilder::fromData(QMetaObject *output, const QMetaObject *parent, const QByteArray &data)
{
    output->d.superdata = parent;
    output->d.stringdata = reinterpret_cast<const QByteArrayData *>(data.constData() + header(data)->fieldCount * sizeof(uint));
    output->d.data = fieldPointer(data);
    output->d.extradata = 0;
    output->d.static_metacall = 0;
    output->d.relatedMetaObjects = 0;
}

QT_END_NAMESPACE
