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

#include "qsgvisualadaptormodel_p.h"
#include "qsgitem.h"

#include <QtDeclarative/qdeclarativecontext.h>
#include <QtDeclarative/qdeclarativeengine.h>
#include <QtDeclarative/qdeclarativeexpression.h>
#include <QtDeclarative/qdeclarativeinfo.h>

#include <private/qdeclarativecontext_p.h>
#include <private/qdeclarativepackage_p.h>
#include <private/qdeclarativeopenmetaobject_p.h>
#include <private/qdeclarativelistaccessor_p.h>
#include <private/qdeclarativedata_p.h>
#include <private/qdeclarativepropertycache_p.h>
#include <private/qdeclarativeguard_p.h>
#include <private/qdeclarativeglobal_p.h>
#include <private/qlistmodelinterface_p.h>
#include <private/qmetaobjectbuilder_p.h>
#include <private/qdeclarativeproperty_p.h>
#include <private/qintrusivelist_p.h>
#include <private/qobject_p.h>

#include <QtCore/qhash.h>
#include <QtCore/qlist.h>

Q_DECLARE_METATYPE(QModelIndex)

QT_BEGIN_NAMESPACE

class VDMDelegateDataType : public QDeclarativeRefCount
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
        qFree(metaObject);
    }

    QMetaObject *metaObject;
    QDeclarativePropertyCache *propertyCache;
    int propertyOffset;
    int signalOffset;
    bool shared : 1;
    QMetaObjectBuilder builder;
};

class QSGVisualAdaptorModelData : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int index READ index NOTIFY indexChanged)
public:
    QSGVisualAdaptorModelData(int index, QSGVisualAdaptorModel *model);
    ~QSGVisualAdaptorModelData();

    int index() const;
    void setIndex(int index);

Q_SIGNALS:
    void indexChanged();

public:
    int m_index;
    QDeclarativeGuard<QSGVisualAdaptorModel> m_model;
    QIntrusiveListNode m_cacheNode;
};

typedef QIntrusiveList<QSGVisualAdaptorModelData, &QSGVisualAdaptorModelData::m_cacheNode> QSGVisualAdaptorModelDataCache;

class QSGVisualAdaptorModelDataMetaObject;
class QSGVisualAdaptorModelPrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QSGVisualAdaptorModel)
public:
    QSGVisualAdaptorModelPrivate()
        : m_engine(0)
        , m_listAccessor(0)
        , m_delegateDataType(0)
        , createModelData(&initializeModelData)
        , m_ref(0)
        , m_count(0)
        , m_objectList(false)
    {
    }


    static QSGVisualAdaptorModelPrivate *get(QSGVisualAdaptorModel *m) {
        return static_cast<QSGVisualAdaptorModelPrivate *>(QObjectPrivate::get(m));
    }

    void addProperty(int role, int propertyId, const char *propertyName, const char *propertyType, bool isModelData = false);
    template <typename T> void setModelDataType()
    {
        createModelData = &T::create;
        m_delegateDataType->builder.setFlags(QMetaObjectBuilder::DynamicMetaObject);
        m_delegateDataType->builder.setClassName(T::staticMetaObject.className());
        m_delegateDataType->builder.setSuperClass(&T::staticMetaObject);
        m_delegateDataType->propertyOffset = T::staticMetaObject.propertyCount();
        m_delegateDataType->signalOffset = T::staticMetaObject.methodCount();
    }
    QSGVisualAdaptorModelData *createMetaObject(int index, QSGVisualAdaptorModel *model);

    static QSGVisualAdaptorModelData *initializeModelData(int index, QSGVisualAdaptorModel *model) {
        return get(model)->createMetaObject(index, model);
    }

    typedef QSGVisualAdaptorModelData *(*CreateModelData)(int index, QSGVisualAdaptorModel *model);

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

    QDeclarativeGuard<QDeclarativeEngine> m_engine;
    QDeclarativeGuard<QListModelInterface> m_listModelInterface;
    QDeclarativeGuard<QAbstractItemModel> m_abstractItemModel;
    QDeclarativeListAccessor *m_listAccessor;
    VDMDelegateDataType *m_delegateDataType;
    CreateModelData createModelData;

    int m_ref;
    int m_count;
    QSGVisualAdaptorModel::Flags m_flags;
    bool m_objectList : 1;

    QVariant m_modelVariant;
    QModelIndex m_root;

    QList<int> m_roles;
    QList<int> watchedRoleIds;
    QList<QByteArray> watchedRoles;
    QHash<QByteArray,int> m_roleNames;
    QVector<PropertyData> m_propertyData;
    QSGVisualAdaptorModelDataCache m_cache;
};

class QSGVisualAdaptorModelDataMetaObject : public QAbstractDynamicMetaObject
{
public:
    QSGVisualAdaptorModelDataMetaObject(QSGVisualAdaptorModelData *data, VDMDelegateDataType *type)
        : m_data(data)
        , m_type(type)
    {
        QObjectPrivate *op = QObjectPrivate::get(m_data);
        *static_cast<QMetaObject *>(this) = *type->metaObject;
        op->metaObject = this;
        m_type->addref();
    }

    ~QSGVisualAdaptorModelDataMetaObject() { m_type->release(); }

    QSGVisualAdaptorModelData *m_data;
    VDMDelegateDataType *m_type;
};

class QSGVDMAbstractItemModelDataMetaObject : public QSGVisualAdaptorModelDataMetaObject
{
public:
    QSGVDMAbstractItemModelDataMetaObject(QSGVisualAdaptorModelData *object, VDMDelegateDataType *type)
        : QSGVisualAdaptorModelDataMetaObject(object, type) {}

    int metaCall(QMetaObject::Call call, int id, void **arguments)
    {
        if (call == QMetaObject::ReadProperty && id >= m_type->propertyOffset) {
            QSGVisualAdaptorModelPrivate *model = QSGVisualAdaptorModelPrivate::get(m_data->m_model);
            if (m_data->m_index == -1 || !model->m_abstractItemModel)
                return -1;
            *static_cast<QVariant *>(arguments[0]) = model->m_abstractItemModel->index(
                    m_data->m_index, 0, model->m_root).data(model->m_propertyData.at(id - m_type->propertyOffset).role);
            return -1;
        } else {
            return m_data->qt_metacall(call, id, arguments);
        }
    }
};

class QSGVDMAbstractItemModelData : public QSGVisualAdaptorModelData
{
    Q_OBJECT
    Q_PROPERTY(bool hasModelChildren READ hasModelChildren CONSTANT)
public:
    bool hasModelChildren() const
    {
        QSGVisualAdaptorModelPrivate *model = QSGVisualAdaptorModelPrivate::get(m_model);
        return model->m_abstractItemModel->hasChildren(model->m_abstractItemModel->index(m_index, 0, model->m_root));
    }

    static QSGVisualAdaptorModelData *create(int index, QSGVisualAdaptorModel *model) {
        return new QSGVDMAbstractItemModelData(index, model); }
private:
    QSGVDMAbstractItemModelData(int index, QSGVisualAdaptorModel *model)
        : QSGVisualAdaptorModelData(index, model)
    {
        new QSGVDMAbstractItemModelDataMetaObject(
                this, QSGVisualAdaptorModelPrivate::get(m_model)->m_delegateDataType);
    }
};

class QSGVDMListModelInterfaceDataMetaObject : public QSGVisualAdaptorModelDataMetaObject
{
public:
    QSGVDMListModelInterfaceDataMetaObject(QSGVisualAdaptorModelData *object, VDMDelegateDataType *type)
        : QSGVisualAdaptorModelDataMetaObject(object, type) {}

    int metaCall(QMetaObject::Call call, int id, void **arguments)
    {
        if (call == QMetaObject::ReadProperty && id >= m_type->propertyOffset) {
            QSGVisualAdaptorModelPrivate *model = QSGVisualAdaptorModelPrivate::get(m_data->m_model);
            if (m_data->m_index == -1 || !model->m_listModelInterface)
                return -1;
            *static_cast<QVariant *>(arguments[0]) = model->m_listModelInterface->data(
                    m_data->m_index, model->m_propertyData.at(id - m_type->propertyOffset).role);
            return -1;
        } else {
            return m_data->qt_metacall(call, id, arguments);
        }
    }
};

class QSGVDMListModelInterfaceData : public QSGVisualAdaptorModelData
{
public:
    static QSGVisualAdaptorModelData *create(int index, QSGVisualAdaptorModel *model) {
        return new QSGVDMListModelInterfaceData(index, model); }
private:
    QSGVDMListModelInterfaceData(int index, QSGVisualAdaptorModel *model)
        : QSGVisualAdaptorModelData(index, model)
    {
        new QSGVDMListModelInterfaceDataMetaObject(
                this, QSGVisualAdaptorModelPrivate::get(m_model)->m_delegateDataType);
    }
};

class QSGVDMListAccessorData : public QSGVisualAdaptorModelData
{
    Q_OBJECT
    Q_PROPERTY(QVariant modelData READ modelData CONSTANT)
public:
    QVariant modelData() const {
        return QSGVisualAdaptorModelPrivate::get(m_model)->m_listAccessor->at(m_index); }

    static QSGVisualAdaptorModelData *create(int index, QSGVisualAdaptorModel *model) {
        return new QSGVDMListAccessorData(index, model); }
private:
    QSGVDMListAccessorData(int index, QSGVisualAdaptorModel *model)
        : QSGVisualAdaptorModelData(index, model)
    {
    }
};

class QSGVDMObjectDataMetaObject : public QSGVisualAdaptorModelDataMetaObject
{
public:
    QSGVDMObjectDataMetaObject(QSGVisualAdaptorModelData *data, VDMDelegateDataType *type)
        : QSGVisualAdaptorModelDataMetaObject(data, type)
        , m_object(QSGVisualAdaptorModelPrivate::get(data->m_model)->m_listAccessor->at(data->m_index).value<QObject *>())
    {}

    int metaCall(QMetaObject::Call call, int id, void **arguments)
    {
        if (id >= m_type->propertyOffset
                && (call == QMetaObject::ReadProperty
                || call == QMetaObject::WriteProperty
                || call == QMetaObject::ResetProperty)) {
            if (m_object)
                QMetaObject::metacall(m_object, call, id - m_type->propertyOffset + 1, arguments);
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

        const int previousPropertyCount = propertyCount() - propertyOffset();
        int propertyIndex = metaObject->indexOfProperty(name);
        if (propertyIndex == -1)
            return -1;
        if (previousPropertyCount + 1 == metaObject->propertyCount())
            return propertyIndex + m_type->propertyOffset - 1;

        if (m_type->shared) {
            VDMDelegateDataType *type = m_type;
            m_type = new VDMDelegateDataType(*m_type);
            type->release();
        }

        const int previousMethodCount = methodCount();
        int notifierId = previousMethodCount;
        for (int propertyId = previousPropertyCount; propertyId < metaObject->propertyCount() - 1; ++propertyId) {
            QMetaProperty property = metaObject->property(propertyId + 1);
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
            qFree(m_type->metaObject);
        m_type->metaObject = m_type->builder.toMetaObject();
        *static_cast<QMetaObject *>(this) = *m_type->metaObject;

        notifierId = previousMethodCount;
        for (int i = previousPropertyCount; i < metaObject->propertyCount(); ++i) {
            QMetaProperty property = metaObject->property(i);
            if (property.hasNotifySignal()) {
                QDeclarativePropertyPrivate::connect(
                        m_object, property.notifySignalIndex(), m_data, notifierId);
                ++notifierId;
            }
        }
        return propertyIndex + m_type->propertyOffset - 1;
    }

    QDeclarativeGuard<QObject> m_object;
};

class QSGVDMObjectData : public QSGVisualAdaptorModelData, public QSGVisualAdaptorModelProxyInterface
{
    Q_OBJECT
    Q_PROPERTY(QObject *modelData READ modelData CONSTANT)
    Q_INTERFACES(QSGVisualAdaptorModelProxyInterface)
public:
    QObject *modelData() const { return m_metaObject->m_object; }
    QObject *proxiedObject() { return m_metaObject->m_object; }

    static QSGVisualAdaptorModelData *create(int index, QSGVisualAdaptorModel *model) {
        return new QSGVDMObjectData(index, model); }

private:
    QSGVDMObjectData(int index, QSGVisualAdaptorModel *model)
        : QSGVisualAdaptorModelData(index, model)
        , m_metaObject(new QSGVDMObjectDataMetaObject(this, QSGVisualAdaptorModelPrivate::get(m_model)->m_delegateDataType))
    {
    }

    QSGVDMObjectDataMetaObject *m_metaObject;
};

void QSGVisualAdaptorModelPrivate::addProperty(
        int role, int propertyId, const char *propertyName, const char *propertyType, bool isModelData)
{
    PropertyData propertyData;
    propertyData.role = role;
    propertyData.isModelData = isModelData;
    m_delegateDataType->builder.addSignal("__" + QByteArray::number(propertyId) + "()");
    QMetaPropertyBuilder property = m_delegateDataType->builder.addProperty(
            propertyName, propertyType, propertyId);
    property.setWritable(false);

    m_propertyData.append(propertyData);
}

QSGVisualAdaptorModelData *QSGVisualAdaptorModelPrivate::createMetaObject(int index, QSGVisualAdaptorModel *model)
{
    Q_ASSERT(!m_delegateDataType);

    m_objectList = false;
    m_propertyData.clear();
    if (m_listAccessor
            && m_listAccessor->type() != QDeclarativeListAccessor::ListProperty
            && m_listAccessor->type() != QDeclarativeListAccessor::Instance) {
        createModelData = &QSGVDMListAccessorData::create;
        m_flags = QSGVisualAdaptorModel::MetaObjectCacheable;
        return QSGVDMListAccessorData::create(index, model);
    }

    m_delegateDataType = new VDMDelegateDataType;
    if (m_listModelInterface) {
        setModelDataType<QSGVDMListModelInterfaceData>();
        QList<int> roles = m_listModelInterface->roles();
        for (int propertyId = 0; propertyId < roles.count(); ++propertyId) {
            const int role = roles.at(propertyId);
            const QByteArray propertyName = m_listModelInterface->toString(role).toUtf8();
            addProperty(role, propertyId, propertyName, "QVariant");
            m_roleNames.insert(propertyName, role);
        }
        if (m_propertyData.count() == 1)
            addProperty(roles.first(), 1, "modelData", "QVariant", true);
        m_flags = QSGVisualAdaptorModel::MetaObjectCacheable;
    } else if (m_abstractItemModel) {
        setModelDataType<QSGVDMAbstractItemModelData>();
        QHash<int, QByteArray> roleNames = m_abstractItemModel->roleNames();
        for (QHash<int, QByteArray>::const_iterator it = roleNames.begin(); it != roleNames.end(); ++it) {
            addProperty(it.key(), m_propertyData.count(), it.value(), "QVariant");
            m_roleNames.insert(it.value(), it.key());
        }
        if (m_propertyData.count() == 1)
            addProperty(roleNames.begin().key(), 1, "modelData", "QVariant", true);
        m_flags = QSGVisualAdaptorModel::MetaObjectCacheable;
    } else if (m_listAccessor) {
        setModelDataType<QSGVDMObjectData>();
        m_objectList = true;
        m_flags = QSGVisualAdaptorModel::ProxiedObject;
    } else {
        Q_ASSERT(!"No model set on VisualDataModel");
        return 0;
    }
    m_delegateDataType->metaObject = m_delegateDataType->builder.toMetaObject();
    if (!m_objectList) {
        m_delegateDataType->propertyCache = new QDeclarativePropertyCache(
                m_engine, m_delegateDataType->metaObject);
    }
    return createModelData(index, model);
}

QSGVisualAdaptorModelData::QSGVisualAdaptorModelData(int index, QSGVisualAdaptorModel *model)
    : m_index(index)
    , m_model(model)
{
}

QSGVisualAdaptorModelData::~QSGVisualAdaptorModelData()
{
}

int QSGVisualAdaptorModelData::index() const
{
    return m_index;
}

// This is internal only - it should not be set from qml
void QSGVisualAdaptorModelData::setIndex(int index)
{
    m_index = index;
    emit indexChanged();
}

//---------------------------------------------------------------------------

QSGVisualAdaptorModel::QSGVisualAdaptorModel(QObject *parent)
    : QObject(*(new QSGVisualAdaptorModelPrivate), parent)
{
}

QSGVisualAdaptorModel::~QSGVisualAdaptorModel()
{
    Q_D(QSGVisualAdaptorModel);
    if (d->m_listAccessor)
        delete d->m_listAccessor;
    if (d->m_delegateDataType)
        d->m_delegateDataType->release();
}

QSGVisualAdaptorModel::Flags QSGVisualAdaptorModel::flags() const
{
    Q_D(const QSGVisualAdaptorModel);
    return d->m_flags;
}

QVariant QSGVisualAdaptorModel::model() const
{
    Q_D(const QSGVisualAdaptorModel);
    return d->m_modelVariant;
}

void QSGVisualAdaptorModel::setModel(const QVariant &model, QDeclarativeEngine *engine)
{
    Q_D(QSGVisualAdaptorModel);
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
    d->m_flags = QSGVisualAdaptorModel::Flags();
    if (d->m_delegateDataType)
        d->m_delegateDataType->release();
    d->m_delegateDataType = 0;
    d->createModelData = &QSGVisualAdaptorModelPrivate::initializeModelData;

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

    d->m_listAccessor = new QDeclarativeListAccessor;
    d->m_listAccessor->setList(model, d->m_engine);
    if ((d->m_count = d->m_listAccessor->count()))
        emit itemsInserted(0, d->m_count);
}

QVariant QSGVisualAdaptorModel::rootIndex() const
{
    Q_D(const QSGVisualAdaptorModel);
    return QVariant::fromValue(d->m_root);
}

void QSGVisualAdaptorModel::setRootIndex(const QVariant &root)
{
    Q_D(QSGVisualAdaptorModel);
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

QVariant QSGVisualAdaptorModel::modelIndex(int idx) const
{
    Q_D(const QSGVisualAdaptorModel);
    if (d->m_abstractItemModel)
        return QVariant::fromValue(d->m_abstractItemModel->index(idx, 0, d->m_root));
    return QVariant::fromValue(QModelIndex());
}

QVariant QSGVisualAdaptorModel::parentModelIndex() const
{
    Q_D(const QSGVisualAdaptorModel);
    if (d->m_abstractItemModel)
        return QVariant::fromValue(d->m_abstractItemModel->parent(d->m_root));
    return QVariant::fromValue(QModelIndex());
}

int QSGVisualAdaptorModel::count() const
{
    Q_D(const QSGVisualAdaptorModel);
    return d->modelCount();
}

QObject *QSGVisualAdaptorModel::data(int index)
{
    Q_D(QSGVisualAdaptorModel);
    QSGVisualAdaptorModelData *data = d->createModelData(index, this);
    d->m_cache.insert(data);
    return data;
}

QString QSGVisualAdaptorModel::stringValue(int index, const QString &name)
{
    Q_D(QSGVisualAdaptorModel);
    if ((!d->m_listModelInterface || !d->m_abstractItemModel) && d->m_listAccessor) {
        if (QObject *object = d->m_listAccessor->at(index).value<QObject*>())
            return object->property(name.toUtf8()).toString();
    }

    QString val;
    QSGVisualAdaptorModelData *data = d->createModelData(index, this);

    QDeclarativeData *ddata = QDeclarativeData::get(data);
    if (ddata && ddata->propertyCache) {
        QDeclarativePropertyCache::Data *prop = ddata->propertyCache->property(name);
        if (prop) {
            if (prop->propType == QVariant::String) {
                void *args[] = { &val, 0 };
                QMetaObject::metacall(data, QMetaObject::ReadProperty, prop->coreIndex, args);
            } else if (prop->propType == qMetaTypeId<QVariant>()) {
                QVariant v;
                void *args[] = { &v, 0 };
                QMetaObject::metacall(data, QMetaObject::ReadProperty, prop->coreIndex, args);
                val = v.toString();
            }
        } else {
            val = data->property(name.toUtf8()).toString();
        }
    } else {
        val = data->property(name.toUtf8()).toString();
    }

    delete data;

    return val;
}

int QSGVisualAdaptorModel::indexOf(QObject *object) const
{
    if (QSGVisualAdaptorModelData *data = qobject_cast<QSGVisualAdaptorModelData *>(object))
        return data->index();
    return -1;
}

bool QSGVisualAdaptorModel::canFetchMore() const
{
    Q_D(const QSGVisualAdaptorModel);
    return d->m_abstractItemModel && d->m_abstractItemModel->canFetchMore(d->m_root);
}

void QSGVisualAdaptorModel::fetchMore()
{
    Q_D(QSGVisualAdaptorModel);
    if (d->m_abstractItemModel)
        d->m_abstractItemModel->fetchMore(d->m_root);
}

void QSGVisualAdaptorModel::replaceWatchedRoles(const QList<QByteArray> &oldRoles, const QList<QByteArray> &newRoles)
{
    Q_D(QSGVisualAdaptorModel);
    d->watchedRoleIds.clear();
    foreach (const QByteArray &oldRole, oldRoles)
        d->watchedRoles.removeOne(oldRole);
    d->watchedRoles += newRoles;
}

void QSGVisualAdaptorModel::_q_itemsChanged(int index, int count, const QList<int> &roles)
{
    Q_D(QSGVisualAdaptorModel);
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

    typedef QSGVisualAdaptorModelDataCache::iterator iterator;
    for (iterator it = d->m_cache.begin(); it != d->m_cache.end(); ++it) {
        const int idx = it->index();
        if (idx >= index && idx < index + count) {
            QSGVisualAdaptorModelData *data = *it;
            for (int i = 0; i < signalIndexes.count(); ++i)
                QMetaObject::activate(data, signalIndexes.at(i), 0);
        }
    }
    if (changed)
        emit itemsChanged(index, count);
}

void QSGVisualAdaptorModel::_q_itemsInserted(int index, int count)
{
    Q_D(QSGVisualAdaptorModel);
    if (count <= 0)
        return;
    d->m_count += count;

    typedef QSGVisualAdaptorModelDataCache::iterator iterator;
    for (iterator it = d->m_cache.begin(); it != d->m_cache.end(); ++it) {
        if (it->index() >= index)
            it->setIndex(it->index() + count);
    }

    emit itemsInserted(index, count);
}

void QSGVisualAdaptorModel::_q_itemsRemoved(int index, int count)
{
    Q_D(QSGVisualAdaptorModel);
    if (count <= 0)
        return;
    d->m_count -= count;

    typedef QSGVisualAdaptorModelDataCache::iterator iterator;
    for (iterator it = d->m_cache.begin(); it != d->m_cache.end(); ++it) {
        if (it->index() >= index + count)
            it->setIndex(it->index() - count);
        else  if (it->index() >= index)
            it->setIndex(-1);
    }

    emit itemsRemoved(index, count);
}

void QSGVisualAdaptorModel::_q_itemsMoved(int from, int to, int count)
{
    Q_D(QSGVisualAdaptorModel);
    const int minimum = qMin(from, to);
    const int maximum = qMax(from, to) + count;
    const int difference = from > to ? count : -count;

    typedef QSGVisualAdaptorModelDataCache::iterator iterator;
    for (iterator it = d->m_cache.begin(); it != d->m_cache.end(); ++it) {
        if (it->index() >= from && it->index() < from + count)
            it->setIndex(it->index() - from + to);
        else if (it->index() >= minimum && it->index() < maximum)
            it->setIndex(it->index() + difference);
    }
    emit itemsMoved(from, to, count);
}

void QSGVisualAdaptorModel::_q_rowsInserted(const QModelIndex &parent, int begin, int end)
{
    Q_D(QSGVisualAdaptorModel);
    if (parent == d->m_root)
        _q_itemsInserted(begin, end - begin + 1);
}

void QSGVisualAdaptorModel::_q_rowsRemoved(const QModelIndex &parent, int begin, int end)
{
    Q_D(QSGVisualAdaptorModel);
    if (parent == d->m_root)
        _q_itemsRemoved(begin, end - begin + 1);
}

void QSGVisualAdaptorModel::_q_rowsMoved(const QModelIndex &sourceParent, int sourceStart, int sourceEnd, const QModelIndex &destinationParent, int destinationRow)
{
   Q_D(QSGVisualAdaptorModel);
    const int count = sourceEnd - sourceStart + 1;
    if (destinationParent == d->m_root && sourceParent == d->m_root) {
        _q_itemsMoved(sourceStart, sourceStart > destinationRow ? destinationRow : destinationRow-count, count);
    } else if (sourceParent == d->m_root) {
        _q_itemsRemoved(sourceStart, count);
    } else if (destinationParent == d->m_root) {
        _q_itemsInserted(destinationRow, count);
    }
}

void QSGVisualAdaptorModel::_q_dataChanged(const QModelIndex &begin, const QModelIndex &end)
{
    Q_D(QSGVisualAdaptorModel);
    if (begin.parent() == d->m_root)
        _q_itemsChanged(begin.row(), end.row() - begin.row() + 1, d->m_roles);
}

void QSGVisualAdaptorModel::_q_layoutChanged()
{
    Q_D(QSGVisualAdaptorModel);
    _q_itemsChanged(0, count(), d->m_roles);
}

void QSGVisualAdaptorModel::_q_modelReset()
{
    Q_D(QSGVisualAdaptorModel);
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

#include <qsgvisualadaptormodel.moc>
