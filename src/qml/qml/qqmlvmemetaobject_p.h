/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Copyright (C) 2016 BasysKom GmbH.
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

#include <private/qv4value_p.h>

QT_BEGIN_NAMESPACE

#define QML_ALIAS_FLAG_PTR 0x00000001

struct QQmlVMEMetaData
{
    // Make sure this structure is always aligned to int
    int dummy;

    struct AliasData {
        int propertyIdx;
        int propType;
        int flags;
        int notifySignal;

        bool isObjectAlias() const {
            return propertyIdx == -1;
        }
        bool isPropertyAlias() const {
            return !isObjectAlias() && valueTypeIndex() == -1;
        }
        bool isValueTypeAlias() const {
            return !isObjectAlias() && valueTypeIndex() != -1;
        }
        int propertyIndex() const {
            int index;
            QQmlPropertyData::decodeValueTypePropertyIndex(propertyIdx, &index);
            return index;
        }
        int valueTypeIndex() const {
            return QQmlPropertyData::decodeValueTypePropertyIndex(propertyIdx);
        }
        int valueType() const {
            return (valueTypeIndex() != -1) ? propType : 0;
        }
    };

    AliasData *aliasData() const {
        return (AliasData *)(((char *)const_cast<QQmlVMEMetaData *>(this)) + sizeof(QQmlVMEMetaData));
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


class Q_QML_PRIVATE_EXPORT QQmlInterceptorMetaObject : public QAbstractDynamicMetaObject
{
public:
    QQmlInterceptorMetaObject(QObject *obj, QQmlPropertyCache *cache);
    ~QQmlInterceptorMetaObject();

    void registerInterceptor(int index, int valueIndex, QQmlPropertyValueInterceptor *interceptor);

    static QQmlInterceptorMetaObject *get(QObject *obj);

    QAbstractDynamicMetaObject *toDynamicMetaObject(QObject *o) Q_DECL_OVERRIDE;

    // Used by auto-tests for inspection
    QQmlPropertyCache *propertyCache() const { return cache; }

protected:
    int metaCall(QObject *o, QMetaObject::Call c, int id, void **a) Q_DECL_OVERRIDE;
    bool intercept(QMetaObject::Call c, int id, void **a);

public:
    QObject *object;
    QQmlPropertyCache *cache;
    QBiPointer<QDynamicMetaObjectData, const QMetaObject> parent;

    QQmlPropertyValueInterceptor *interceptors;
    bool hasAssignedMetaObjectData;
};

inline QQmlInterceptorMetaObject *QQmlInterceptorMetaObject::get(QObject *obj)
{
    if (obj) {
        if (QQmlData *data = QQmlData::get(obj)) {
            if (data->hasInterceptorMetaObject)
                return static_cast<QQmlInterceptorMetaObject *>(QObjectPrivate::get(obj)->metaObject);
        }
    }

    return 0;
}

class QQmlVMEMetaObjectEndpoint;
class Q_QML_PRIVATE_EXPORT QQmlVMEMetaObject : public QQmlInterceptorMetaObject
{
public:
    QQmlVMEMetaObject(QObject *obj, QQmlPropertyCache *cache, const QQmlVMEMetaData *data, QV4::CompiledData::CompilationUnit *qmlCompilationUnit, int qmlObjectId);
    ~QQmlVMEMetaObject();

    bool aliasTarget(int index, QObject **target, int *coreIndex, int *valueTypeIndex) const;
    QV4::ReturnedValue vmeMethod(int index);
    void setVmeMethod(int index, const QV4::Value &function);
    QV4::ReturnedValue vmeProperty(int index);
    void setVMEProperty(int index, const QV4::Value &v);

    void connectAliasSignal(int index, bool indexInSignalRange);

    static inline QQmlVMEMetaObject *get(QObject *o);
    static QQmlVMEMetaObject *getForProperty(QObject *o, int coreIndex);
    static QQmlVMEMetaObject *getForMethod(QObject *o, int coreIndex);
    static QQmlVMEMetaObject *getForSignal(QObject *o, int coreIndex);

protected:
    int metaCall(QObject *o, QMetaObject::Call _c, int _id, void **_a) Q_DECL_OVERRIDE;

public:
    QQmlGuardedContextData ctxt;

    const QQmlVMEMetaData *metaData;
    inline int propOffset() const;
    inline int methodOffset() const;
    inline int signalOffset() const;
    inline int signalCount() const;

    QQmlVMEMetaObjectEndpoint *aliasEndpoints;

    QV4::WeakValue propertyAndMethodStorage;
    QV4::MemberData *propertyAndMethodStorageAsMemberData();

    int readPropertyAsInt(int id);
    bool readPropertyAsBool(int id);
    double readPropertyAsDouble(int id);
    QString readPropertyAsString(int id);
    QSizeF readPropertyAsSizeF(int id);
    QPointF readPropertyAsPointF(int id);
    QUrl readPropertyAsUrl(int id);
    QDate readPropertyAsDate(int id);
    QDateTime readPropertyAsDateTime(int id);
    QRectF readPropertyAsRectF(int id);
    QObject *readPropertyAsQObject(int id);
    QList<QObject *> *readPropertyAsList(int id);

    void writeProperty(int id, int v);
    void writeProperty(int id, bool v);
    void writeProperty(int id, double v);
    void writeProperty(int id, const QString& v);
    void writeProperty(int id, const QPointF& v);
    void writeProperty(int id, const QSizeF& v);
    void writeProperty(int id, const QUrl& v);
    void writeProperty(int id, const QDate& v);
    void writeProperty(int id, const QDateTime& v);
    void writeProperty(int id, const QRectF& v);
    void writeProperty(int id, QObject *v);

    void ensureQObjectWrapper();

    void mark(QV4::ExecutionEngine *e);

    void connectAlias(int aliasId);

    QV4::ReturnedValue method(int);

    QV4::ReturnedValue readVarProperty(int);
    void writeVarProperty(int, const QV4::Value &);
    QVariant readPropertyAsVariant(int);
    void writeProperty(int, const QVariant &);

    inline QQmlVMEMetaObject *parentVMEMetaObject() const;

    void activate(QObject *, int, void **);

    QList<QQmlVMEVariantQObjectPtr *> varObjectGuards;

    QQmlVMEVariantQObjectPtr *getQObjectGuardForProperty(int) const;


    // keep a reference to the compilation unit in order to still
    // do property access when the context has been invalidated.
    QQmlRefPointer<QV4::CompiledData::CompilationUnit> compilationUnit;
    const QV4::CompiledData::Object *compiledObject;
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

int QQmlVMEMetaObject::signalCount() const
{
    return cache->signalCount();
}

QQmlVMEMetaObject *QQmlVMEMetaObject::parentVMEMetaObject() const
{
    if (parent.isT1() && parent.flag())
        return static_cast<QQmlVMEMetaObject *>(parent.asT1());

    return 0;
}

QT_END_NAMESPACE

#endif // QQMLVMEMETAOBJECT_P_H
