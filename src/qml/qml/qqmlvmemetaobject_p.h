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

#ifndef QQMLVMEMETAOBJECT_P_H
#define QQMLVMEMETAOBJECT_P_H

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

#include "qqml.h"

#include <QtCore/QMetaObject>
#include <QtCore/QBitArray>
#include <QtCore/QPair>
#include <QtCore/QDate>
#include <QtCore/qlist.h>
#include <QtCore/qdebug.h>

#include <private/qobject_p.h>

#include "qqmlguard_p.h"
#include "qqmlcompiler_p.h"
#include "qqmlcontext_p.h"

#include <private/qv8engine_p.h>
#include <private/qflagpointer_p.h>

#include <private/qv8_p.h>

QT_BEGIN_NAMESPACE

#define QML_ALIAS_FLAG_PTR 0x00000001

struct QQmlVMEMetaData
{
    short varPropertyCount;
    short propertyCount;
    short aliasCount;
    short signalCount;
    short methodCount;
    short dummyForAlignment; // Add padding to ensure that the following
                             // AliasData/PropertyData/MethodData is int aligned.

    struct AliasData {
        int contextIdx;
        int propertyIdx;
        int flags;
        int notifySignal;

        bool isObjectAlias() const {
            return propertyIdx == -1;
        }
        bool isPropertyAlias() const {
            return !isObjectAlias() && !(propertyIdx & 0xFF000000);
        }
        bool isValueTypeAlias() const {
            return !isObjectAlias() && (propertyIdx & 0xFF000000);
        }
        int propertyIndex() const {
            return propertyIdx & 0x0000FFFF;
        }
        int valueTypeIndex() const {
            return (propertyIdx & 0x00FF0000) >> 16;
        }
        int valueType() const {
            return ((unsigned int)propertyIdx) >> 24;
        }
    };
    
    struct PropertyData {
        int propertyType;
    };

    struct MethodData {
        int parameterCount;
        int bodyOffset;
        int bodyLength;
        int lineNumber;
    };

    PropertyData *propertyData() const {
        return (PropertyData *)(((const char *)this) + sizeof(QQmlVMEMetaData));
    }

    AliasData *aliasData() const {
        return (AliasData *)(propertyData() + propertyCount);
    }

    MethodData *methodData() const {
        return (MethodData *)(aliasData() + aliasCount);
    }
};

class QQmlVMEMetaObject;
class QQmlVMEVariantQObjectPtr : public QQmlGuard<QObject>
{
public:
    inline QQmlVMEVariantQObjectPtr();
    inline ~QQmlVMEVariantQObjectPtr();
    inline void objectDestroyed(QObject *);
    inline void setGuardedValue(QObject *obj, QQmlVMEMetaObject *target, int index);

    QQmlVMEMetaObject *m_target;
    int m_index;
};

class QV8QObjectWrapper;
class QQmlVMEVariant;
class QQmlRefCount;
class QQmlVMEMetaObjectEndpoint;
class Q_AUTOTEST_EXPORT QQmlVMEMetaObject : public QAbstractDynamicMetaObject,
                                                    public QV8GCCallback::Node
{
public:
    QQmlVMEMetaObject(QObject *obj, QQmlPropertyCache *cache, const QQmlVMEMetaData *data);
    ~QQmlVMEMetaObject();

    bool aliasTarget(int index, QObject **target, int *coreIndex, int *valueTypeIndex) const;
    void registerInterceptor(int index, int valueIndex, QQmlPropertyValueInterceptor *interceptor);
    v8::Handle<v8::Function> vmeMethod(int index);
    int vmeMethodLineNumber(int index);
    void setVmeMethod(int index, v8::Persistent<v8::Function>);
    v8::Handle<v8::Value> vmeProperty(int index);
    void setVMEProperty(int index, v8::Handle<v8::Value> v);

    void connectAliasSignal(int index, bool indexInSignalRange);

    virtual QAbstractDynamicMetaObject *toDynamicMetaObject(QObject *o);

    static inline QQmlVMEMetaObject *get(QObject *o);
    static QQmlVMEMetaObject *getForProperty(QObject *o, int coreIndex);
    static QQmlVMEMetaObject *getForMethod(QObject *o, int coreIndex);
    static QQmlVMEMetaObject *getForSignal(QObject *o, int coreIndex);

protected:
    virtual int metaCall(QMetaObject::Call _c, int _id, void **_a);

public:
    friend class QQmlVMEMetaObjectEndpoint;
    friend class QQmlVMEVariantQObjectPtr;

    QObject *object;
    QQmlGuardedContextData ctxt;
    QQmlPropertyCache *cache;

    const QQmlVMEMetaData *metaData;
    inline int propOffset() const;
    inline int methodOffset() const;
    inline int signalOffset() const;

    bool hasAssignedMetaObjectData;
    QQmlVMEVariant *data;
    QQmlVMEMetaObjectEndpoint *aliasEndpoints;

    v8::Persistent<v8::Array> varProperties;
    int firstVarPropertyIndex;
    bool varPropertiesInitialized;
    static void VarPropertiesWeakReferenceCallback(v8::Persistent<v8::Value> object, void* parameter);
    static void GcPrologueCallback(QV8GCCallback::Node *node);
    inline void allocateVarPropertiesArray();
    inline bool ensureVarPropertiesAllocated();

    void connectAlias(int aliasId);
    QBitArray aConnected;

    QQmlPropertyValueInterceptor *interceptors;

    v8::Persistent<v8::Function> *v8methods;
    v8::Handle<v8::Function> method(int);

    v8::Handle<v8::Value> readVarProperty(int);
    void writeVarProperty(int, v8::Handle<v8::Value>);
    QVariant readPropertyAsVariant(int);
    void writeProperty(int, const QVariant &);

    QBiPointer<QDynamicMetaObjectData, const QMetaObject> parent;

    void listChanged(int);
    class List : public QList<QObject*>
    {
    public:
        List(int lpi, QQmlVMEMetaObject *mo) : notifyIndex(lpi), mo(mo) {}
        int notifyIndex;
        QQmlVMEMetaObject *mo;
    };
    QList<List> listProperties;

    static void list_append(QQmlListProperty<QObject> *, QObject *);
    static int list_count(QQmlListProperty<QObject> *);
    static QObject *list_at(QQmlListProperty<QObject> *, int);
    static void list_clear(QQmlListProperty<QObject> *);

    void activate(QObject *, int, void **);

    friend class QV8GCCallback;
    friend class QV8QObjectWrapper;
};

QQmlVMEMetaObject *QQmlVMEMetaObject::get(QObject *obj)
{
    if (obj) {
        if (QQmlData *data = QQmlData::get(obj)) {
            if (data->hasVMEMetaObject)
                return static_cast<QQmlVMEMetaObject *>(QObjectPrivate::get(obj)->metaObject);
        }
    }

    return 0;
}

int QQmlVMEMetaObject::propOffset() const
{
    return cache->propertyOffset();
}

int QQmlVMEMetaObject::methodOffset() const
{
    return cache->methodOffset();
}

int QQmlVMEMetaObject::signalOffset() const
{
    return cache->signalOffset();
}

QT_END_NAMESPACE

#endif // QQMLVMEMETAOBJECT_P_H
