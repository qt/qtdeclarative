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

#include "qquickvisualadaptormodel_p.h"
#include "qquickvisualdatamodel_p_p.h"

#include <private/qqmlengine_p.h>
#include <private/qquicklistaccessor_p.h>
#include <private/qqmlpropertycache_p.h>
#include <private/qlistmodelinterface_p.h>
#include <private/qmetaobjectbuilder_p.h>
#include <private/qintrusivelist_p.h>
#include <private/qobject_p.h>

QT_BEGIN_NAMESPACE

class VDMDelegateDataType : public QQmlRefCount
{
public:
    VDMDelegateDataType()
        : metaObject(0)
        , propertyCache(0)
        , propertyOffset(0)
        , signalOffset(0)
        , shared(true)
    {
    }

    VDMDelegateDataType(const VDMDelegateDataType &type)
        : metaObject(0)
        , propertyCache(0)
        , propertyOffset(type.propertyOffset)
        , signalOffset(type.signalOffset)
        , shared(false)
        , builder(type.metaObject, QMetaObjectBuilder::Properties
                | QMetaObjectBuilder::Signals
                | QMetaObjectBuilder::SuperClass
                | QMetaObjectBuilder::ClassName)
    {
        builder.setFlags(QMetaObjectBuilder::DynamicMetaObject);
    }

    ~VDMDelegateDataType()
    {
        if (propertyCache)
            propertyCache->release();
        free(metaObject);
    }

    QMetaObject *metaObject;
    QQmlPropertyCache *propertyCache;
    int propertyOffset;
    int signalOffset;
    bool shared : 1;
    QMetaObjectBuilder builder;
};

typedef QIntrusiveList<QQuickVisualDataModelItem, &QQuickVisualDataModelItem::cacheNode> QQuickVisualDataModelItemCache;

class QQuickVisualDataModelItemMetaObject;
class QQuickVisualAdaptorModelPrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QQuickVisualAdaptorModel)
public:
    QQuickVisualAdaptorModelPrivate()
        : m_engine(0)
        , m_listAccessor(0)
        , m_delegateDataType(0)
        , createItem(&initializeModelData)
        , stringValue(&initializeStringValue)
        , m_ref(0)
        , m_count(0)
        , m_roleCount(0)
        , m_objectList(false)
    {
    }

    ~QQuickVisualAdaptorModelPrivate()
    {
        qPersistentDispose(m_constructor);
    }

    static QQuickVisualAdaptorModelPrivate *get(QQuickVisualAdaptorModel *m) {
        return static_cast<QQuickVisualAdaptorModelPrivate *>(QObjectPrivate::get(m));
    }

    void addProperty(int role, int propertyId, const char *propertyName, const char *propertyType, bool isModelData = false);
    template <typename T> void setModelDataType()
    {
        createItem = &T::create;
        stringValue = &T::stringValue;
        m_delegateDataType->builder.setFlags(QMetaObjectBuilder::DynamicMetaObject);
        m_delegateDataType->builder.setClassName(T::staticMetaObject.className());
        m_delegateDataType->builder.setSuperClass(&T::staticMetaObject);
        m_delegateDataType->propertyOffset = T::staticMetaObject.propertyCount();
        m_delegateDataType->signalOffset = T::staticMetaObject.methodCount();
    }

    void createMetaObject();

    static QQuickVisualDataModelItem *initializeModelData(
            QQuickVisualDataModelItemMetaType *metaType, QQuickVisualAdaptorModel *model, int index) {
        QQuickVisualAdaptorModelPrivate *d = get(model);
        d->createMetaObject();
        return d->createItem(metaType, model, index);
    }

    static QString initializeStringValue(QQuickVisualAdaptorModelPrivate *model, int index, const QString &name) {
        model->createMetaObject();
        return model->stringValue(model, index, name);
    }

    typedef QQuickVisualDataModelItem *(*CreateModelData)(QQuickVisualDataModelItemMetaType *metaType, QQuickVisualAdaptorModel *model, int index);
    typedef QString (*StringValue)(QQuickVisualAdaptorModelPrivate *model, int index, const QString &name);

    struct PropertyData {
        int role;
        bool isModelData : 1;
    };

    int modelCount() const {
        if (m_listModelInterface)
            return m_listModelInterface->count();
        if (m_abstractItemModel)
            return m_abstractItemModel->rowCount(m_root);
        if (m_listAccessor)
            return m_listAccessor->count();
        return 0;
    }

    QQmlGuard<QQmlEngine> m_engine;
    QQmlGuard<QListModelInterface> m_listModelInterface;
    QQmlGuard<QAbstractItemModel> m_abstractItemModel;
    QQuickListAccessor *m_listAccessor;
    VDMDelegateDataType *m_delegateDataType;
    CreateModelData createItem;
    StringValue stringValue;
    v8::Persistent<v8::ObjectTemplate> m_constructor;

    int m_ref;
    int m_count;
    int m_roleCount;
    QQuickVisualAdaptorModel::Flags m_flags;
    bool m_objectList : 1;

    QVariant m_modelVariant;
    QModelIndex m_root;

    QList<int> m_roles;
    QList<int> watchedRoleIds;
    QList<QByteArray> watchedRoles;
    QHash<QByteArray,int> m_roleNames;
    QVector<PropertyData> m_propertyData;
    QQuickVisualDataModelItemCache m_cache;
};

class QQuickVDMCachedModelData : public QQuickVisualDataModelItem
{
public:
    virtual QVariant value(QQuickVisualAdaptorModelPrivate *model, int role) const = 0;
    virtual void setValue(QQuickVisualAdaptorModelPrivate *model, int role, const QVariant &value) = 0;

    void setValue(const QString &role, const QVariant &value)
    {
        QQuickVisualAdaptorModelPrivate *d = QQuickVisualAdaptorModelPrivate::get(model);
        QHash<QByteArray, int>::iterator it = d->m_roleNames.find(role.toUtf8());
        if (it != d->m_roleNames.end()) {
            for (int i = 0; i < d->m_propertyData.count(); ++i) {
                if (d->m_propertyData.at(i).role == *it) {
                    cachedData[i] = value;
                    return;
                }
            }
        }
    }

    bool resolveIndex(int idx)
    {
        if (index[0] == -1) {
            Q_ASSERT(idx >= 0);
            QQuickVisualAdaptorModelPrivate *d = QQuickVisualAdaptorModelPrivate::get(model);
            index[0] = idx;
            cachedData.clear();
            emit modelIndexChanged();
            const QMetaObject *meta = metaObject();
            const int propertyCount = d->m_propertyData.count();
            for (int i = 0; i < propertyCount; ++i)
                QMetaObject::activate(this, meta, i, 0);
            return true;
        } else {
            return false;
        }
    }

    static v8::Handle<v8::Value> get_property(v8::Local<v8::String>, const v8::AccessorInfo &info)
    {
        QQuickVisualDataModelItem *data = v8_resource_cast<QQuickVisualDataModelItem>(info.This());
        if (!data)
            V8THROW_ERROR("Not a valid VisualData object");

        QQuickVisualAdaptorModelPrivate *model = QQuickVisualAdaptorModelPrivate::get(data->model);
        QQuickVDMCachedModelData *modelData = static_cast<QQuickVDMCachedModelData *>(data);
        const int propertyId = info.Data()->Int32Value();
        if (data->index[0] == -1) {
            if (!modelData->cachedData.isEmpty()) {
                return data->engine->fromVariant(
                        modelData->cachedData.at(modelData->cachedData.count() > 1 ? propertyId : 0));
            }
        } else {
            return data->engine->fromVariant(
                    modelData->value(model, model->m_propertyData.at(propertyId).role));
        }
        return v8::Undefined();
    }

    static void set_property(
            v8::Local<v8::String>, v8::Local<v8::Value> value, const v8::AccessorInfo &info)
    {
        QQuickVisualDataModelItem *data = v8_resource_cast<QQuickVisualDataModelItem>(info.This());
        if (!data)
            V8THROW_ERROR_SETTER("Not a valid VisualData object");

        const int propertyId = info.Data()->Int32Value();
        if (data->index[0] == -1) {
            QQuickVDMCachedModelData *modelData = static_cast<QQuickVDMCachedModelData *>(data);
            if (!modelData->cachedData.isEmpty()) {
                if (modelData->cachedData.count() > 1) {
                    modelData->cachedData[propertyId] = data->engine->toVariant(value, QVariant::Invalid);
                    QMetaObject::activate(data, data->metaObject(), propertyId, 0);
                } else if (modelData->cachedData.count() == 1) {
                    modelData->cachedData[0] = data->engine->toVariant(value, QVariant::Invalid);
                    QMetaObject::activate(data, data->metaObject(), 0, 0);
                    QMetaObject::activate(data, data->metaObject(), 1, 0);
                }
            }
        }
    }

    v8::Handle<v8::Value> get()
    {
        QQuickVisualAdaptorModelPrivate *d = QQuickVisualAdaptorModelPrivate::get(model);

        v8::Local<v8::Object> data = d->m_constructor->NewInstance();
        data->SetExternalResource(this);
        return data;
    }


    QQuickVDMCachedModelData(
            QQuickVisualDataModelItemMetaType *metaType, QQuickVisualAdaptorModel *model, int index)
        : QQuickVisualDataModelItem(metaType, model, index)
    {
        if (index == -1)
            cachedData.resize(QQuickVisualAdaptorModelPrivate::get(model)->m_roleCount);
    }

    QVector<QVariant> cachedData;
};

class QQuickVisualDataModelItemMetaObject : public QAbstractDynamicMetaObject
{
public:
    QQuickVisualDataModelItemMetaObject(QQuickVisualDataModelItem *data, VDMDelegateDataType *type)
        : m_data(data)
        , m_type(type)
    {
        QObjectPrivate *op = QObjectPrivate::get(m_data);
        *static_cast<QMetaObject *>(this) = *type->metaObject;
        op->metaObject = this;
        m_type->addref();
    }

    ~QQuickVisualDataModelItemMetaObject() { m_type->release(); }

    static v8::Handle<v8::Value> get_index(v8::Local<v8::String>, const v8::AccessorInfo &info)
    {
        QQuickVisualDataModelItem *data = v8_resource_cast<QQuickVisualDataModelItem>(info.This());
        if (!data)
            V8THROW_ERROR("Not a valid VisualData object");

        return v8::Int32::New(data->index[0]);
    }

    QQuickVisualDataModelItem *m_data;
    VDMDelegateDataType *m_type;
};

class QQuickVDMCachedModelDataMetaObject : public QQuickVisualDataModelItemMetaObject
{
public:
    QQuickVDMCachedModelDataMetaObject(QQuickVisualDataModelItem *object, VDMDelegateDataType *type)
        : QQuickVisualDataModelItemMetaObject(object, type) {}

    int metaCall(QMetaObject::Call call, int id, void **arguments)
    {
        if (call == QMetaObject::ReadProperty && id >= m_type->propertyOffset) {
            QQuickVisualAdaptorModelPrivate *model = QQuickVisualAdaptorModelPrivate::get(m_data->model);
            const int propertyIndex = id - m_type->propertyOffset;
            if (m_data->index[0] == -1) {
                QQuickVDMCachedModelData *data = static_cast<QQuickVDMCachedModelData *>(m_data);
                if (!data->cachedData.isEmpty()) {
                    *static_cast<QVariant *>(arguments[0]) = data->cachedData.count() > 1
                            ? data->cachedData.at(propertyIndex)
                            : data->cachedData.at(0);
                }
            } else {
                *static_cast<QVariant *>(arguments[0]) = static_cast<QQuickVDMCachedModelData *>(
                        m_data)->value(model, model->m_propertyData.at(propertyIndex).role);
            }
            return -1;
        } else if (call == QMetaObject::WriteProperty && id >= m_type->propertyOffset) {
            QQuickVisualAdaptorModelPrivate *model = QQuickVisualAdaptorModelPrivate::get(m_data->model);
            const int propertyIndex = id - m_type->propertyOffset;
            if (m_data->index[0] == -1) {
                QQuickVDMCachedModelData *data = static_cast<QQuickVDMCachedModelData *>(m_data);
                if (data->cachedData.count() > 1) {
                    data->cachedData[propertyIndex] = *static_cast<QVariant *>(arguments[0]);
                    activate(data, this, propertyIndex, 0);
                } else if (data->cachedData.count() == 1) {
                    data->cachedData[0] = *static_cast<QVariant *>(arguments[0]);
                    activate(data, this, 0, 0);
                    activate(data, this, 1, 0);
                }
            } else {
                static_cast<QQuickVDMCachedModelData *>(m_data)->setValue(
                        model,
                        model->m_propertyData.at(propertyIndex).role,
                        *static_cast<QVariant *>(arguments[0]));
            }
            return -1;
        } else {
            return m_data->qt_metacall(call, id, arguments);
        }
    }
};

class QQuickVDMAbstractItemModelData : public QQuickVDMCachedModelData
{
    Q_OBJECT
    Q_PROPERTY(bool hasModelChildren READ hasModelChildren CONSTANT)
public:
    bool hasModelChildren() const
    {
        QQuickVisualAdaptorModelPrivate *d = QQuickVisualAdaptorModelPrivate::get(model);
        return d->m_abstractItemModel->hasChildren(d->m_abstractItemModel->index(index[0], 0, d->m_root));
    }

    static QQuickVisualDataModelItem *create(
            QQuickVisualDataModelItemMetaType *metaType, QQuickVisualAdaptorModel *model, int index) {
        return new QQuickVDMAbstractItemModelData(metaType, model, index); }

    static QString stringValue(QQuickVisualAdaptorModelPrivate *model, int index, const QString &role)
    {
        QHash<QByteArray, int>::const_iterator it = model->m_roleNames.find(role.toUtf8());
        if (it != model->m_roleNames.end()) {
            return model->m_abstractItemModel->index(index, 0, model->m_root).data(*it).toString();
        } else if (role == QLatin1String("hasModelChildren")) {
            return QVariant(model->m_abstractItemModel->hasChildren(
                    model->m_abstractItemModel->index(index, 0, model->m_root))).toString();
        } else {
            return QString();
        }
    }

    QVariant value(QQuickVisualAdaptorModelPrivate *model, int role) const
    {
        return model->m_abstractItemModel
                ? model->m_abstractItemModel->index(index[0], 0, model->m_root).data(role)
                : 0;
    }

    void setValue(QQuickVisualAdaptorModelPrivate *model, int role, const QVariant &value)
    {
        model->m_abstractItemModel->setData(
                model->m_abstractItemModel->index(index[0], 0, model->m_root), value, role);
    }

    static v8::Handle<v8::Value> get_hasModelChildren(v8::Local<v8::String>, const v8::AccessorInfo &info)
    {
        QQuickVisualDataModelItem *data = v8_resource_cast<QQuickVisualDataModelItem>(info.This());
        if (!data)
            V8THROW_ERROR("Not a valid VisualData object");

        QQuickVisualAdaptorModelPrivate *model = QQuickVisualAdaptorModelPrivate::get(data->model);

        return v8::Boolean::New(model->m_abstractItemModel->hasChildren(
                model->m_abstractItemModel->index(data->index[0], 0, model->m_root)));
    }

private:
    QQuickVDMAbstractItemModelData(
            QQuickVisualDataModelItemMetaType *metaType, QQuickVisualAdaptorModel *model, int index)
        : QQuickVDMCachedModelData(metaType, model, index)
    {
        new QQuickVDMCachedModelDataMetaObject(
                this, QQuickVisualAdaptorModelPrivate::get(model)->m_delegateDataType);
    }
};

class QQuickVDMListModelInterfaceDataMetaObject : public QQuickVisualDataModelItemMetaObject
{
public:
    QQuickVDMListModelInterfaceDataMetaObject(QQuickVisualDataModelItem *object, VDMDelegateDataType *type)
        : QQuickVisualDataModelItemMetaObject(object, type) {}

    int metaCall(QMetaObject::Call call, int id, void **arguments)
    {
        if (call == QMetaObject::ReadProperty && id >= m_type->propertyOffset) {
            QQuickVisualAdaptorModelPrivate *model = QQuickVisualAdaptorModelPrivate::get(m_data->model);
            if (m_data->index[0] == -1 || !model->m_listModelInterface)
                return -1;
            *static_cast<QVariant *>(arguments[0]) = model->m_listModelInterface->data(
                    m_data->index[0], model->m_propertyData.at(id - m_type->propertyOffset).role);
            return -1;
        } else {
            return m_data->qt_metacall(call, id, arguments);
        }
    }

};

class QQuickVDMListModelInterfaceData : public QQuickVDMCachedModelData
{
public:
    static QQuickVisualDataModelItem *create(
            QQuickVisualDataModelItemMetaType *metaType, QQuickVisualAdaptorModel *model, int index) {
        return new QQuickVDMListModelInterfaceData(metaType, model, index); }

    static QString stringValue(QQuickVisualAdaptorModelPrivate *model, int index, const QString &role)
    {
        QHash<QByteArray, int>::const_iterator it = model->m_roleNames.find(role.toUtf8());
        return it != model->m_roleNames.end()
                ? model->m_listModelInterface->data(index, *it).toString()
                : QString();
    }

    QVariant value(QQuickVisualAdaptorModelPrivate *model, int role) const
    {
        return model->m_listModelInterface
                ? model->m_listModelInterface->data(index[0], role)
                : 0;
    }

    void setValue(QQuickVisualAdaptorModelPrivate *, int, const QVariant &) {}

private:
    QQuickVDMListModelInterfaceData(QQuickVisualDataModelItemMetaType *metaType, QQuickVisualAdaptorModel *model, int index)
        : QQuickVDMCachedModelData(metaType, model, index)
    {
        new QQuickVDMCachedModelDataMetaObject(
                this, QQuickVisualAdaptorModelPrivate::get(model)->m_delegateDataType);
    }
};

class QQuickVDMListAccessorData : public QQuickVisualDataModelItem
{
    Q_OBJECT
    Q_PROPERTY(QVariant modelData READ modelData WRITE setModelData NOTIFY modelDataChanged)
public:
    QVariant modelData() const
    {
        QQuickVisualAdaptorModelPrivate *d = QQuickVisualAdaptorModelPrivate::get(model);
        return index[0] != -1 && d->m_listAccessor
                ? d->m_listAccessor->at(index[0])
                : cachedData;
    }

    void setModelData(const QVariant &data)
    {
        if (index[0] == -1 && data != cachedData) {
            cachedData = data;
            emit modelDataChanged();
        }
    }

    static QQuickVisualDataModelItem *create(
            QQuickVisualDataModelItemMetaType *metaType, QQuickVisualAdaptorModel *model, int index) {
        return new QQuickVDMListAccessorData(metaType, model, index); }

    static QString stringValue(QQuickVisualAdaptorModelPrivate *model, int index, const QString &role)
    {
        return role == QLatin1String("modelData")
                ? model->m_listAccessor->at(index).toString()
                : QString();
    }

    static v8::Handle<v8::Value> get_modelData(v8::Local<v8::String>, const v8::AccessorInfo &info)
    {
        QQuickVisualDataModelItem *data = v8_resource_cast<QQuickVisualDataModelItem>(info.This());
        if (!data)
            V8THROW_ERROR("Not a valid VisualData object");

        QQuickVisualAdaptorModelPrivate *d = QQuickVisualAdaptorModelPrivate::get(data->model);
        if (data->index[0] == -1 || !d->m_listAccessor)
            return data->engine->fromVariant(static_cast<QQuickVDMListAccessorData *>(data)->cachedData);

        return data->engine->fromVariant(d->m_listAccessor->at(data->index[0]));
    }

    static void set_modelData(v8::Local<v8::String>, const v8::Handle<v8::Value> &value, const v8::AccessorInfo &info)
    {
        QQuickVisualDataModelItem *data = v8_resource_cast<QQuickVisualDataModelItem>(info.This());
        if (!data)
            V8THROW_ERROR_SETTER("Not a valid VisualData object");

        if (data->index[0] == -1) {
            static_cast<QQuickVDMListAccessorData *>(data)->setModelData(
                    data->engine->toVariant(value, QVariant::Invalid));
        }
    }

    void setValue(const QString &role, const QVariant &value)
    {
        if (role == QLatin1String("modelData"))
            cachedData = value;
    }

    bool resolveIndex(int idx)
    {
        if (index[0] == -1) {
            index[0] = idx;
            cachedData.clear();
            emit modelIndexChanged();
            emit modelDataChanged();
            return true;
        } else {
            return false;
        }
    }

Q_SIGNALS:
    void modelDataChanged();

private:
    QQuickVDMListAccessorData(QQuickVisualDataModelItemMetaType *metaType, QQuickVisualAdaptorModel *model, int index)
        : QQuickVisualDataModelItem(metaType, model, index)
    {
    }

    QVariant cachedData;
};

class QQuickVDMObjectDataMetaObject : public QQuickVisualDataModelItemMetaObject
{
public:
    QQuickVDMObjectDataMetaObject(QQuickVisualDataModelItem *data, VDMDelegateDataType *type)
        : QQuickVisualDataModelItemMetaObject(data, type)
        , m_object(QQuickVisualAdaptorModelPrivate::get(data->model)->m_listAccessor->at(data->index[0]).value<QObject *>())
    {}

    int metaCall(QMetaObject::Call call, int id, void **arguments)
    {
        static const int objectPropertyOffset = QObject::staticMetaObject.propertyCount();
        if (id >= m_type->propertyOffset
                && (call == QMetaObject::ReadProperty
                || call == QMetaObject::WriteProperty
                || call == QMetaObject::ResetProperty)) {
            if (m_object)
                QMetaObject::metacall(m_object, call, id - m_type->propertyOffset + objectPropertyOffset, arguments);
            return -1;
        } else if (id >= m_type->signalOffset && call == QMetaObject::InvokeMetaMethod) {
            QMetaObject::activate(m_data, this, id, 0);
            return -1;
        } else {
            return m_data->qt_metacall(call, id, arguments);
        }
    }

    int createProperty(const char *name, const char *)
    {
        if (!m_object)
            return -1;
        const QMetaObject *metaObject = m_object->metaObject();
        static const int objectPropertyOffset = QObject::staticMetaObject.propertyCount();

        const int previousPropertyCount = propertyCount() - propertyOffset();
        int propertyIndex = metaObject->indexOfProperty(name);
        if (propertyIndex == -1)
            return -1;
        if (previousPropertyCount + objectPropertyOffset == metaObject->propertyCount())
            return propertyIndex + m_type->propertyOffset - objectPropertyOffset;

        if (m_type->shared) {
            VDMDelegateDataType *type = m_type;
            m_type = new VDMDelegateDataType(*m_type);
            type->release();
        }

        const int previousMethodCount = methodCount();
        int notifierId = previousMethodCount;
        for (int propertyId = previousPropertyCount; propertyId < metaObject->propertyCount() - objectPropertyOffset; ++propertyId) {
            QMetaProperty property = metaObject->property(propertyId + objectPropertyOffset);
            QMetaPropertyBuilder propertyBuilder;
            if (property.hasNotifySignal()) {
                m_type->builder.addSignal("__" + QByteArray::number(propertyId) + "()");
                propertyBuilder = m_type->builder.addProperty(property.name(), property.typeName(), notifierId);
                ++notifierId;
            } else {
                propertyBuilder = m_type->builder.addProperty(property.name(), property.typeName());
            }
            propertyBuilder.setWritable(property.isWritable());
            propertyBuilder.setResettable(property.isResettable());
            propertyBuilder.setConstant(property.isConstant());
        }

        if (m_type->metaObject)
            free(m_type->metaObject);
        m_type->metaObject = m_type->builder.toMetaObject();
        *static_cast<QMetaObject *>(this) = *m_type->metaObject;

        notifierId = previousMethodCount;
        for (int i = previousPropertyCount; i < metaObject->propertyCount() - objectPropertyOffset; ++i) {
            QMetaProperty property = metaObject->property(i + objectPropertyOffset);
            if (property.hasNotifySignal()) {
                QQmlPropertyPrivate::connect(
                        m_object, property.notifySignalIndex(), m_data, notifierId);
                ++notifierId;
            }
        }
        return propertyIndex + m_type->propertyOffset - objectPropertyOffset;
    }

    QQmlGuard<QObject> m_object;
};

class QQuickVDMObjectData : public QQuickVisualDataModelItem, public QQuickVisualAdaptorModelProxyInterface
{
    Q_OBJECT
    Q_PROPERTY(QObject *modelData READ modelData CONSTANT)
    Q_INTERFACES(QQuickVisualAdaptorModelProxyInterface)
public:
    QObject *modelData() const { return m_metaObject->m_object; }
    QObject *proxiedObject() { return m_metaObject->m_object; }

    static QQuickVisualDataModelItem *create(
            QQuickVisualDataModelItemMetaType *metaType, QQuickVisualAdaptorModel *model, int index) {
        return index >= 0 ? new QQuickVDMObjectData(metaType, model, index) : 0; }

    static QString stringValue(QQuickVisualAdaptorModelPrivate *model, int index, const QString &name)
    {
        if (QObject *object = model->m_listAccessor->at(index).value<QObject *>())
            return object->property(name.toUtf8()).toString();
        return QString();
    }

private:
    QQuickVDMObjectData(QQuickVisualDataModelItemMetaType *metaType, QQuickVisualAdaptorModel *model, int index)
        : QQuickVisualDataModelItem(metaType, model, index)
        , m_metaObject(new QQuickVDMObjectDataMetaObject(this, QQuickVisualAdaptorModelPrivate::get(model)->m_delegateDataType))
    {
    }

    QQuickVDMObjectDataMetaObject *m_metaObject;
};

void QQuickVisualAdaptorModelPrivate::addProperty(
        int role, int propertyId, const char *propertyName, const char *propertyType, bool isModelData)
{
    PropertyData propertyData;
    propertyData.role = role;
    propertyData.isModelData = isModelData;
    m_delegateDataType->builder.addSignal("__" + QByteArray::number(propertyId) + "()");
    QMetaPropertyBuilder property = m_delegateDataType->builder.addProperty(
            propertyName, propertyType, propertyId);
    property.setWritable(true); // No, yes, yes no?

    m_propertyData.append(propertyData);
}

void QQuickVisualAdaptorModelPrivate::createMetaObject()
{
    Q_ASSERT(!m_delegateDataType);

    m_objectList = false;
    m_propertyData.clear();

    QV8Engine *v8Engine  = QQmlEnginePrivate::getV8Engine(m_engine);

    v8::HandleScope handleScope;
    v8::Context::Scope contextScope(v8Engine->context());
    v8::Local<v8::ObjectTemplate> constructor = v8::ObjectTemplate::New();
    constructor->SetHasExternalResource(true);
    constructor->SetAccessor(
            v8::String::New("index"), QQuickVisualDataModelItemMetaObject::get_index);

    if (m_listAccessor
            && m_listAccessor->type() != QQuickListAccessor::ListProperty
            && m_listAccessor->type() != QQuickListAccessor::Instance) {
        createItem = &QQuickVDMListAccessorData::create;
        stringValue = &QQuickVDMListAccessorData::stringValue;
        constructor->SetAccessor(
                v8::String::New("modelData"), QQuickVDMListAccessorData::get_modelData);
        m_constructor = qPersistentNew(constructor);
        return;
    }

    m_delegateDataType = new VDMDelegateDataType;
    if (m_listModelInterface) {
        setModelDataType<QQuickVDMListModelInterfaceData>();
        QList<int> roles = m_listModelInterface->roles();
        for (int propertyId = 0; propertyId < roles.count(); ++propertyId) {
            const int role = roles.at(propertyId);
            const QString roleName = m_listModelInterface->toString(role);
            const QByteArray propertyName = roleName.toUtf8();
            addProperty(role, propertyId, propertyName, "QVariant");
            constructor->SetAccessor(
                    v8Engine->toString(roleName),
                    QQuickVDMListModelInterfaceData::get_property,
                    QQuickVDMListModelInterfaceData::set_property,
                    v8::Int32::New(propertyId));
            m_roleNames.insert(propertyName, role);
        }
        m_roleCount = m_propertyData.count();
        if (m_propertyData.count() == 1) {
            addProperty(roles.first(), 1, "modelData", "QVariant", true);
            constructor->SetAccessor(
                    v8::String::New("modelData"),
                    QQuickVDMListModelInterfaceData::get_property,
                    QQuickVDMListModelInterfaceData::set_property,
                    v8::Int32::New(0));
            m_roleNames.insert("modelData", roles.first());
        }
    } else if (m_abstractItemModel) {
        setModelDataType<QQuickVDMAbstractItemModelData>();
        QHash<int, QByteArray> roleNames = m_abstractItemModel->roleNames();
        for (QHash<int, QByteArray>::const_iterator it = roleNames.begin(); it != roleNames.end(); ++it) {
            const int propertyId = m_propertyData.count();
            addProperty(it.key(), propertyId, it.value(), "QVariant");
            constructor->SetAccessor(
                    v8::String::New(it.value().constData(), it.value().length()),
                    QQuickVDMAbstractItemModelData::get_property,
                    QQuickVDMAbstractItemModelData::set_property,
                    v8::Int32::New(propertyId));
            m_roleNames.insert(it.value(), it.key());
        }
        m_roleCount = m_propertyData.count();
        if (m_propertyData.count() == 1) {
            addProperty(roleNames.begin().key(), 1, "modelData", "QVariant", true);
            constructor->SetAccessor(
                    v8::String::New("modelData"),
                    QQuickVDMAbstractItemModelData::get_property,
                    QQuickVDMAbstractItemModelData::set_property,
                    v8::Int32::New(0));
            m_roleNames.insert("modelData", roleNames.begin().key());
        }
        constructor->SetAccessor(
                v8::String::New("hasModelChildren"),
                QQuickVDMAbstractItemModelData::get_hasModelChildren);
    } else if (m_listAccessor) {
        setModelDataType<QQuickVDMObjectData>();
        m_objectList = true;
        m_flags = QQuickVisualAdaptorModel::ProxiedObject;
    } else {
        Q_ASSERT(!"No model set on VisualDataModel");
        return;
    }
    m_delegateDataType->metaObject = m_delegateDataType->builder.toMetaObject();
    if (!m_objectList) {
        m_delegateDataType->propertyCache = new QQmlPropertyCache(
                m_engine, m_delegateDataType->metaObject);
        m_constructor = qPersistentNew(constructor);
    }
}

//---------------------------------------------------------------------------

QQuickVisualAdaptorModel::QQuickVisualAdaptorModel(QObject *parent)
    : QObject(*(new QQuickVisualAdaptorModelPrivate), parent)
{
}

QQuickVisualAdaptorModel::~QQuickVisualAdaptorModel()
{
    Q_D(QQuickVisualAdaptorModel);
    if (d->m_listAccessor)
        delete d->m_listAccessor;
    if (d->m_delegateDataType)
        d->m_delegateDataType->release();
}

QQuickVisualAdaptorModel::Flags QQuickVisualAdaptorModel::flags() const
{
    Q_D(const QQuickVisualAdaptorModel);
    return d->m_flags;
}

QVariant QQuickVisualAdaptorModel::model() const
{
    Q_D(const QQuickVisualAdaptorModel);
    return d->m_modelVariant;
}

void QQuickVisualAdaptorModel::setModel(const QVariant &model, QQmlEngine *engine)
{
    Q_D(QQuickVisualAdaptorModel);
    delete d->m_listAccessor;
    d->m_engine = engine;
    d->m_listAccessor = 0;
    d->m_modelVariant = model;
    if (d->m_listModelInterface) {
        // Assume caller has released all items.
        QObject::disconnect(d->m_listModelInterface, SIGNAL(itemsChanged(int,int,QList<int>)),
                this, SLOT(_q_itemsChanged(int,int,QList<int>)));
        QObject::disconnect(d->m_listModelInterface, SIGNAL(itemsInserted(int,int)),
                this, SLOT(_q_itemsInserted(int,int)));
        QObject::disconnect(d->m_listModelInterface, SIGNAL(itemsRemoved(int,int)),
                this, SLOT(_q_itemsRemoved(int,int)));
        QObject::disconnect(d->m_listModelInterface, SIGNAL(itemsMoved(int,int,int)),
                this, SLOT(_q_itemsMoved(int,int,int)));
        d->m_listModelInterface = 0;
    } else if (d->m_abstractItemModel) {
        QObject::disconnect(d->m_abstractItemModel, SIGNAL(rowsInserted(QModelIndex,int,int)),
                            this, SLOT(_q_rowsInserted(QModelIndex,int,int)));
        QObject::disconnect(d->m_abstractItemModel, SIGNAL(rowsRemoved(QModelIndex,int,int)),
                            this, SLOT(_q_rowsRemoved(QModelIndex,int,int)));
        QObject::disconnect(d->m_abstractItemModel, SIGNAL(dataChanged(QModelIndex,QModelIndex)),
                            this, SLOT(_q_dataChanged(QModelIndex,QModelIndex)));
        QObject::disconnect(d->m_abstractItemModel, SIGNAL(rowsMoved(QModelIndex,int,int,QModelIndex,int)),
                            this, SLOT(_q_rowsMoved(QModelIndex,int,int,QModelIndex,int)));
        QObject::disconnect(d->m_abstractItemModel, SIGNAL(modelReset()), this, SLOT(_q_modelReset()));
        QObject::disconnect(d->m_abstractItemModel, SIGNAL(layoutChanged()), this, SLOT(_q_layoutChanged()));
        d->m_abstractItemModel = 0;
    }

    d->m_roles.clear();
    d->m_roleNames.clear();
    d->m_roleCount = 0;
    d->m_flags = QQuickVisualAdaptorModel::Flags();
    if (d->m_delegateDataType)
        d->m_delegateDataType->release();
    d->m_delegateDataType = 0;
    d->createItem = &QQuickVisualAdaptorModelPrivate::initializeModelData;
    d->stringValue = &QQuickVisualAdaptorModelPrivate::initializeStringValue;
    qPersistentDispose(d->m_constructor);

    if (d->m_count)
        emit itemsRemoved(0, d->m_count);

    QObject *object = qvariant_cast<QObject *>(model);
    if (object && (d->m_listModelInterface = qobject_cast<QListModelInterface *>(object))) {
        QObject::connect(d->m_listModelInterface, SIGNAL(itemsChanged(int,int,QList<int>)),
                         this, SLOT(_q_itemsChanged(int,int,QList<int>)));
        QObject::connect(d->m_listModelInterface, SIGNAL(itemsInserted(int,int)),
                         this, SLOT(_q_itemsInserted(int,int)));
        QObject::connect(d->m_listModelInterface, SIGNAL(itemsRemoved(int,int)),
                         this, SLOT(_q_itemsRemoved(int,int)));
        QObject::connect(d->m_listModelInterface, SIGNAL(itemsMoved(int,int,int)),
                         this, SLOT(_q_itemsMoved(int,int,int)));
        if ((d->m_count = d->m_listModelInterface->count()))
            emit itemsInserted(0, d->m_count);
        return;
    } else if (object && (d->m_abstractItemModel = qobject_cast<QAbstractItemModel *>(object))) {
        QObject::connect(d->m_abstractItemModel, SIGNAL(rowsInserted(QModelIndex,int,int)),
                            this, SLOT(_q_rowsInserted(QModelIndex,int,int)));
        QObject::connect(d->m_abstractItemModel, SIGNAL(rowsRemoved(QModelIndex,int,int)),
                            this, SLOT(_q_rowsRemoved(QModelIndex,int,int)));
        QObject::connect(d->m_abstractItemModel, SIGNAL(dataChanged(QModelIndex,QModelIndex)),
                            this, SLOT(_q_dataChanged(QModelIndex,QModelIndex)));
        QObject::connect(d->m_abstractItemModel, SIGNAL(rowsMoved(QModelIndex,int,int,QModelIndex,int)),
                            this, SLOT(_q_rowsMoved(QModelIndex,int,int,QModelIndex,int)));
        QObject::connect(d->m_abstractItemModel, SIGNAL(modelReset()), this, SLOT(_q_modelReset()));
        QObject::connect(d->m_abstractItemModel, SIGNAL(layoutChanged()), this, SLOT(_q_layoutChanged()));

        if ((d->m_count = d->m_abstractItemModel->rowCount(d->m_root)))
            emit itemsInserted(0, d->m_count);
        return;
    }

    d->m_listAccessor = new QQuickListAccessor;
    d->m_listAccessor->setList(model, d->m_engine);
    if ((d->m_count = d->m_listAccessor->count()))
        emit itemsInserted(0, d->m_count);
}

QVariant QQuickVisualAdaptorModel::rootIndex() const
{
    Q_D(const QQuickVisualAdaptorModel);
    return QVariant::fromValue(d->m_root);
}

void QQuickVisualAdaptorModel::setRootIndex(const QVariant &root)
{
    Q_D(QQuickVisualAdaptorModel);
    QModelIndex modelIndex = qvariant_cast<QModelIndex>(root);
    if (d->m_root != modelIndex) {
        int oldCount = d->modelCount();
        d->m_root = modelIndex;
        if (d->m_abstractItemModel && d->m_abstractItemModel->canFetchMore(modelIndex))
            d->m_abstractItemModel->fetchMore(modelIndex);
        int newCount = d->modelCount();
        if (oldCount)
            emit itemsRemoved(0, oldCount);
        if (newCount)
            emit itemsInserted(0, newCount);
        emit rootIndexChanged();
    }
}

QVariant QQuickVisualAdaptorModel::modelIndex(int idx) const
{
    Q_D(const QQuickVisualAdaptorModel);
    if (d->m_abstractItemModel)
        return QVariant::fromValue(d->m_abstractItemModel->index(idx, 0, d->m_root));
    return QVariant::fromValue(QModelIndex());
}

QVariant QQuickVisualAdaptorModel::parentModelIndex() const
{
    Q_D(const QQuickVisualAdaptorModel);
    if (d->m_abstractItemModel)
        return QVariant::fromValue(d->m_abstractItemModel->parent(d->m_root));
    return QVariant::fromValue(QModelIndex());
}

int QQuickVisualAdaptorModel::count() const
{
    Q_D(const QQuickVisualAdaptorModel);
    return d->modelCount();
}

QQuickVisualDataModelItem *QQuickVisualAdaptorModel::createItem(QQuickVisualDataModelItemMetaType *metaType, int index)
{
    Q_D(QQuickVisualAdaptorModel);

    if (QQuickVisualDataModelItem *item = d->createItem(metaType, this, index)) {
        d->m_cache.insert(item);

        if (d->m_delegateDataType && d->m_delegateDataType->propertyCache) {
            QQmlData *qmldata = QQmlData::get(item, true);
            qmldata->propertyCache = d->m_delegateDataType->propertyCache;
            qmldata->propertyCache->addref();
        }
        return item;
    }
    return 0;
}

QString QQuickVisualAdaptorModel::stringValue(int index, const QString &name)
{
    Q_D(QQuickVisualAdaptorModel);
    return d->stringValue(d, index, name);
}

bool QQuickVisualAdaptorModel::canFetchMore() const
{
    Q_D(const QQuickVisualAdaptorModel);
    return d->m_abstractItemModel && d->m_abstractItemModel->canFetchMore(d->m_root);
}

void QQuickVisualAdaptorModel::fetchMore()
{
    Q_D(QQuickVisualAdaptorModel);
    if (d->m_abstractItemModel)
        d->m_abstractItemModel->fetchMore(d->m_root);
}

void QQuickVisualAdaptorModel::replaceWatchedRoles(const QList<QByteArray> &oldRoles, const QList<QByteArray> &newRoles)
{
    Q_D(QQuickVisualAdaptorModel);
    d->watchedRoleIds.clear();
    foreach (const QByteArray &oldRole, oldRoles)
        d->watchedRoles.removeOne(oldRole);
    d->watchedRoles += newRoles;
}

void QQuickVisualAdaptorModel::_q_itemsChanged(int index, int count, const QList<int> &roles)
{
    Q_D(QQuickVisualAdaptorModel);
    bool changed = roles.isEmpty();
    if (!d->watchedRoles.isEmpty() && d->watchedRoleIds.isEmpty()) {
        foreach (QByteArray r, d->watchedRoles) {
            if (d->m_roleNames.contains(r))
                d->watchedRoleIds << d->m_roleNames.value(r);
        }
    }

    QVector<int> signalIndexes;
    for (int i = 0; i < roles.count(); ++i) {
        const int role = roles.at(i);
        if (!changed && d->watchedRoleIds.contains(role))
            changed = true;
        for (int propertyId = 0; propertyId < d->m_propertyData.count(); ++propertyId) {
            if (d->m_propertyData.at(propertyId).role == role)
                signalIndexes.append(propertyId + d->m_delegateDataType->signalOffset);
        }
    }
    if (roles.isEmpty()) {
        for (int propertyId = 0; propertyId < d->m_propertyData.count(); ++propertyId)
            signalIndexes.append(propertyId + d->m_delegateDataType->signalOffset);
    }

    typedef QQuickVisualDataModelItemCache::iterator iterator;
    for (iterator it = d->m_cache.begin(); it != d->m_cache.end(); ++it) {
        const int idx = it->modelIndex();
        if (idx >= index && idx < index + count) {
            QQuickVisualDataModelItem *data = *it;
            for (int i = 0; i < signalIndexes.count(); ++i)
                QMetaObject::activate(data, signalIndexes.at(i), 0);
        }
    }
    if (changed)
        emit itemsChanged(index, count);
}

void QQuickVisualAdaptorModel::_q_itemsInserted(int index, int count)
{
    Q_D(QQuickVisualAdaptorModel);
    if (count <= 0)
        return;
    d->m_count += count;

    typedef QQuickVisualDataModelItemCache::iterator iterator;
    for (iterator it = d->m_cache.begin(); it != d->m_cache.end(); ++it) {
        if (it->modelIndex() >= index)
            it->setModelIndex(it->modelIndex() + count);
    }

    emit itemsInserted(index, count);
}

void QQuickVisualAdaptorModel::_q_itemsRemoved(int index, int count)
{
    Q_D(QQuickVisualAdaptorModel);
    if (count <= 0)
        return;
    d->m_count -= count;

    typedef QQuickVisualDataModelItemCache::iterator iterator;
    for (iterator it = d->m_cache.begin(); it != d->m_cache.end(); ++it) {
        if (it->modelIndex() >= index + count)
            it->setModelIndex(it->modelIndex() - count);
        else  if (it->modelIndex() >= index)
            it->setModelIndex(-1);
    }

    emit itemsRemoved(index, count);
}

void QQuickVisualAdaptorModel::_q_itemsMoved(int from, int to, int count)
{
    Q_D(QQuickVisualAdaptorModel);
    const int minimum = qMin(from, to);
    const int maximum = qMax(from, to) + count;
    const int difference = from > to ? count : -count;

    typedef QQuickVisualDataModelItemCache::iterator iterator;
    for (iterator it = d->m_cache.begin(); it != d->m_cache.end(); ++it) {
        if (it->modelIndex() >= from && it->modelIndex() < from + count)
            it->setModelIndex(it->modelIndex() - from + to);
        else if (it->modelIndex() >= minimum && it->modelIndex() < maximum)
            it->setModelIndex(it->modelIndex() + difference);
    }
    emit itemsMoved(from, to, count);
}

void QQuickVisualAdaptorModel::_q_rowsInserted(const QModelIndex &parent, int begin, int end)
{
    Q_D(QQuickVisualAdaptorModel);
    if (parent == d->m_root)
        _q_itemsInserted(begin, end - begin + 1);
}

void QQuickVisualAdaptorModel::_q_rowsRemoved(const QModelIndex &parent, int begin, int end)
{
    Q_D(QQuickVisualAdaptorModel);
    if (parent == d->m_root)
        _q_itemsRemoved(begin, end - begin + 1);
}

void QQuickVisualAdaptorModel::_q_rowsMoved(const QModelIndex &sourceParent, int sourceStart, int sourceEnd, const QModelIndex &destinationParent, int destinationRow)
{
   Q_D(QQuickVisualAdaptorModel);
    const int count = sourceEnd - sourceStart + 1;
    if (destinationParent == d->m_root && sourceParent == d->m_root) {
        _q_itemsMoved(sourceStart, sourceStart > destinationRow ? destinationRow : destinationRow-count, count);
    } else if (sourceParent == d->m_root) {
        _q_itemsRemoved(sourceStart, count);
    } else if (destinationParent == d->m_root) {
        _q_itemsInserted(destinationRow, count);
    }
}

void QQuickVisualAdaptorModel::_q_dataChanged(const QModelIndex &begin, const QModelIndex &end)
{
    Q_D(QQuickVisualAdaptorModel);
    if (begin.parent() == d->m_root)
        _q_itemsChanged(begin.row(), end.row() - begin.row() + 1, d->m_roles);
}

void QQuickVisualAdaptorModel::_q_layoutChanged()
{
    Q_D(QQuickVisualAdaptorModel);
    _q_itemsChanged(0, count(), d->m_roles);
}

void QQuickVisualAdaptorModel::_q_modelReset()
{
    Q_D(QQuickVisualAdaptorModel);
    int oldCount = d->m_count;
    d->m_root = QModelIndex();
    d->m_count = d->modelCount();
    emit modelReset(oldCount, d->m_count);
    emit rootIndexChanged();
    if (d->m_abstractItemModel && d->m_abstractItemModel->canFetchMore(d->m_root))
        d->m_abstractItemModel->fetchMore(d->m_root);
}

QT_END_NAMESPACE

QML_DECLARE_TYPE(QListModelInterface)

#include <qquickvisualadaptormodel.moc>
