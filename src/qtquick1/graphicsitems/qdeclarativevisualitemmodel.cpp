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

#include "QtQuick1/private/qdeclarativevisualitemmodel_p.h"

#include "QtQuick1/qdeclarativeitem.h"

#include <QtDeclarative/qdeclarativecontext.h>
#include <QtDeclarative/private/qdeclarativecontext_p.h>
#include <QtDeclarative/qdeclarativeengine.h>
#include <QtDeclarative/qdeclarativeexpression.h>
#include <QtQuick1/private/qdeclarativepackage_p.h>
#include <QtQuick1/private/qdeclarativeopenmetaobject_p.h>
#include <QtQuick1/private/qdeclarativelistaccessor_p.h>
#include <QtDeclarative/qdeclarativeinfo.h>
#include <QtDeclarative/private/qdeclarativedata_p.h>
#include <QtDeclarative/private/qdeclarativepropertycache_p.h>
#include <QtDeclarative/private/qdeclarativeguard_p.h>
#include <QtDeclarative/private/qdeclarativeglobal_p.h>

#include <qgraphicsscene.h>
#include <QtDeclarative/private/qlistmodelinterface_p.h>
#include <qhash.h>
#include <qlist.h>
#include <private/qmetaobjectbuilder_p.h>
#include <QtCore/qdebug.h>

#include <private/qobject_p.h>

QT_BEGIN_NAMESPACE



QHash<QObject*, QDeclarative1VisualItemModelAttached*> QDeclarative1VisualItemModelAttached::attachedProperties;


class QDeclarative1VisualItemModelPrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QDeclarative1VisualItemModel)
public:
    QDeclarative1VisualItemModelPrivate() : QObjectPrivate() {}

    static void children_append(QDeclarativeListProperty<QDeclarativeItem> *prop, QDeclarativeItem *item) {
        QDeclarative_setParent_noEvent(item, prop->object);
        static_cast<QDeclarative1VisualItemModelPrivate *>(prop->data)->children.append(Item(item));
        static_cast<QDeclarative1VisualItemModelPrivate *>(prop->data)->itemAppended();
        static_cast<QDeclarative1VisualItemModelPrivate *>(prop->data)->emitChildrenChanged();
    }

    static int children_count(QDeclarativeListProperty<QDeclarativeItem> *prop) {
        return static_cast<QDeclarative1VisualItemModelPrivate *>(prop->data)->children.count();
    }

    static QDeclarativeItem *children_at(QDeclarativeListProperty<QDeclarativeItem> *prop, int index) {
        return static_cast<QDeclarative1VisualItemModelPrivate *>(prop->data)->children.at(index).item;
    }

    void itemAppended() {
        Q_Q(QDeclarative1VisualItemModel);
        QDeclarative1VisualItemModelAttached *attached = QDeclarative1VisualItemModelAttached::properties(children.last().item);
        attached->setIndex(children.count()-1);
        emit q->itemsInserted(children.count()-1, 1);
        emit q->countChanged();
    }

    void emitChildrenChanged() {
        Q_Q(QDeclarative1VisualItemModel);
        emit q->childrenChanged();
    }

    int indexOf(QDeclarativeItem *item) const {
        for (int i = 0; i < children.count(); ++i)
            if (children.at(i).item == item)
                return i;
        return -1;
    }

    class Item {
    public:
        Item(QDeclarativeItem *i) : item(i), ref(0) {}

        void addRef() { ++ref; }
        bool deref() { return --ref == 0; }

        QDeclarativeItem *item;
        int ref;
    };

    QList<Item> children;
};


/*!
    \qmlclass VisualItemModel QDeclarative1VisualItemModel
    \inqmlmodule QtQuick 1
    \ingroup qml-working-with-data
  \since QtQuick 1.0
    \brief The VisualItemModel allows items to be provided to a view.

    A VisualItemModel contains the visual items to be used in a view.
    When a VisualItemModel is used in a view, the view does not require
    a delegate since the VisualItemModel already contains the visual
    delegate (items).

    An item can determine its index within the
    model via the \l{VisualItemModel::index}{index} attached property.

    The example below places three colored rectangles in a ListView.
    \code
    import QtQuick 1.0

    Rectangle {
        VisualItemModel {
            id: itemModel
            Rectangle { height: 30; width: 80; color: "red" }
            Rectangle { height: 30; width: 80; color: "green" }
            Rectangle { height: 30; width: 80; color: "blue" }
        }

        ListView {
            anchors.fill: parent
            model: itemModel
        }
    }
    \endcode

    \image visualitemmodel.png

    \sa {declarative/modelviews/visualitemmodel}{VisualItemModel example}
*/
QDeclarative1VisualItemModel::QDeclarative1VisualItemModel(QObject *parent)
    : QDeclarative1VisualModel(*(new QDeclarative1VisualItemModelPrivate), parent)
{
}

/*!
    \qmlattachedproperty int VisualItemModel::index
    This attached property holds the index of this delegate's item within the model.

    It is attached to each instance of the delegate.
*/

QDeclarativeListProperty<QDeclarativeItem> QDeclarative1VisualItemModel::children()
{
    Q_D(QDeclarative1VisualItemModel);
    return QDeclarativeListProperty<QDeclarativeItem>(this, d, d->children_append,
                                                      d->children_count, d->children_at);
}

/*!
    \qmlproperty int QtQuick1::VisualItemModel::count

    The number of items in the model.  This property is readonly.
*/
int QDeclarative1VisualItemModel::count() const
{
    Q_D(const QDeclarative1VisualItemModel);
    return d->children.count();
}

bool QDeclarative1VisualItemModel::isValid() const
{
    return true;
}

QDeclarativeItem *QDeclarative1VisualItemModel::item(int index, bool)
{
    Q_D(QDeclarative1VisualItemModel);
    QDeclarative1VisualItemModelPrivate::Item &item = d->children[index];
    item.addRef();
    return item.item;
}

QDeclarative1VisualModel::ReleaseFlags QDeclarative1VisualItemModel::release(QDeclarativeItem *item)
{
    Q_D(QDeclarative1VisualItemModel);
    int idx = d->indexOf(item);
    if (idx >= 0) {
        if (d->children[idx].deref()) {
            if (item->scene())
                item->scene()->removeItem(item);
            QDeclarative_setParent_noEvent(item, this);
        }
    }
    return 0;
}

bool QDeclarative1VisualItemModel::completePending() const
{
    return false;
}

void QDeclarative1VisualItemModel::completeItem()
{
    // Nothing to do
}

QString QDeclarative1VisualItemModel::stringValue(int index, const QString &name)
{
    Q_D(QDeclarative1VisualItemModel);
    if (index < 0 || index >= d->children.count())
        return QString();
    return QDeclarativeEngine::contextForObject(d->children.at(index).item)->contextProperty(name).toString();
}

int QDeclarative1VisualItemModel::indexOf(QDeclarativeItem *item, QObject *) const
{
    Q_D(const QDeclarative1VisualItemModel);
    return d->indexOf(item);
}

QDeclarative1VisualItemModelAttached *QDeclarative1VisualItemModel::qmlAttachedProperties(QObject *obj)
{
    return QDeclarative1VisualItemModelAttached::properties(obj);
}

//============================================================================

class VDMDelegateDataType : public QDeclarative1OpenMetaObjectType
{
public:
    VDMDelegateDataType(const QMetaObject *base, QDeclarativeEngine *engine) : QDeclarative1OpenMetaObjectType(base, engine) {}

    void propertyCreated(int, QMetaPropertyBuilder &prop) {
        prop.setWritable(false);
    }
};

class QDeclarative1VisualDataModelParts;
class QDeclarative1VisualDataModelData;
class QDeclarative1VisualDataModelPrivate : public QObjectPrivate
{
public:
    QDeclarative1VisualDataModelPrivate(QDeclarativeContext *);

    static QDeclarative1VisualDataModelPrivate *get(QDeclarative1VisualDataModel *m) {
        return static_cast<QDeclarative1VisualDataModelPrivate *>(QObjectPrivate::get(m));
    }

    QDeclarativeGuard<QListModelInterface> m_listModelInterface;
    QDeclarativeGuard<QAbstractItemModel> m_abstractItemModel;
    QDeclarativeGuard<QDeclarative1VisualDataModel> m_visualItemModel;
    QString m_part;

    QDeclarativeComponent *m_delegate;
    QDeclarativeGuard<QDeclarativeContext> m_context;
    QList<int> m_roles;
    QHash<QByteArray,int> m_roleNames;
    void ensureRoles() {
        if (m_roleNames.isEmpty()) {
            if (m_listModelInterface) {
                m_roles = m_listModelInterface->roles();
                for (int ii = 0; ii < m_roles.count(); ++ii)
                    m_roleNames.insert(m_listModelInterface->toString(m_roles.at(ii)).toUtf8(), m_roles.at(ii));
            } else if (m_abstractItemModel) {
                for (QHash<int,QByteArray>::const_iterator it = m_abstractItemModel->roleNames().begin();
                        it != m_abstractItemModel->roleNames().end(); ++it) {
                    m_roles.append(it.key());
                    m_roleNames.insert(*it, it.key());
                }
                if (m_roles.count())
                    m_roleNames.insert("hasModelChildren", -1);
            } else if (m_listAccessor) {
                m_roleNames.insert("modelData", 0);
                if (m_listAccessor->type() == QDeclarative1ListAccessor::Instance) {
                    if (QObject *object = m_listAccessor->at(0).value<QObject*>()) {
                        int count = object->metaObject()->propertyCount();
                        for (int ii = 1; ii < count; ++ii) {
                            const QMetaProperty &prop = object->metaObject()->property(ii);
                            m_roleNames.insert(prop.name(), 0);
                        }
                    }
                }
            }
        }
    }

    QHash<int,int> m_roleToPropId;
    int m_modelDataPropId;
    void createMetaData() {
        if (!m_metaDataCreated) {
            ensureRoles();
            if (m_roleNames.count()) {
                QHash<QByteArray, int>::const_iterator it = m_roleNames.begin();
                while (it != m_roleNames.end()) {
                    int propId = m_delegateDataType->createProperty(it.key()) - m_delegateDataType->propertyOffset();
                    m_roleToPropId.insert(*it, propId);
                    ++it;
                }
                // Add modelData property
                if (m_roles.count() == 1)
                    m_modelDataPropId = m_delegateDataType->createProperty("modelData") - m_delegateDataType->propertyOffset();
                m_metaDataCreated = true;
            }
        }
    }

    struct ObjectRef {
        ObjectRef(QObject *object=0) : obj(object), ref(1) {}
        QObject *obj;
        int ref;
    };
    class Cache : public QHash<int, ObjectRef> {
    public:
        QObject *getItem(int index) {
            QObject *item = 0;
            QHash<int,ObjectRef>::iterator it = find(index);
            if (it != end()) {
                (*it).ref++;
                item = (*it).obj;
            }
            return item;
        }
        QObject *item(int index) {
            QObject *item = 0;
            QHash<int, ObjectRef>::const_iterator it = find(index);
            if (it != end())
                item = (*it).obj;
            return item;
        }
        void insertItem(int index, QObject *obj) {
            insert(index, ObjectRef(obj));
        }
        bool releaseItem(QObject *obj) {
            QHash<int, ObjectRef>::iterator it = begin();
            for (; it != end(); ++it) {
                ObjectRef &objRef = *it;
                if (objRef.obj == obj) {
                    if (--objRef.ref == 0) {
                        erase(it);
                        return true;
                    }
                    break;
                }
            }
            return false;
        }
    };

    int modelCount() const {
        if (m_visualItemModel)
            return m_visualItemModel->count();
        if (m_listModelInterface)
            return m_listModelInterface->count();
        if (m_abstractItemModel)
            return m_abstractItemModel->rowCount(m_root);
        if (m_listAccessor)
            return m_listAccessor->count();
        return 0;
    }

    Cache m_cache;
    QHash<QObject *, QDeclarative1Package*> m_packaged;

    QDeclarative1VisualDataModelParts *m_parts;
    friend class QDeclarative1VisualItemParts;

    VDMDelegateDataType *m_delegateDataType;
    friend class QDeclarative1VisualDataModelData;
    bool m_metaDataCreated : 1;
    bool m_metaDataCacheable : 1;
    bool m_delegateValidated : 1;
    bool m_completePending : 1;

    QDeclarative1VisualDataModelData *data(QObject *item);

    QVariant m_modelVariant;
    QDeclarative1ListAccessor *m_listAccessor;

    QModelIndex m_root;
    QList<QByteArray> watchedRoles;
    QList<int> watchedRoleIds;
};

class QDeclarative1VisualDataModelDataMetaObject : public QDeclarative1OpenMetaObject
{
public:
    QDeclarative1VisualDataModelDataMetaObject(QObject *parent, QDeclarative1OpenMetaObjectType *type)
    : QDeclarative1OpenMetaObject(parent, type) {}

    virtual QVariant initialValue(int);
    virtual int createProperty(const char *, const char *);

private:
    friend class QDeclarative1VisualDataModelData;
};

class QDeclarative1VisualDataModelData : public QObject
{
Q_OBJECT
public:
    QDeclarative1VisualDataModelData(int index, QDeclarative1VisualDataModel *model);
    ~QDeclarative1VisualDataModelData();

    Q_PROPERTY(int index READ index NOTIFY indexChanged)
    int index() const;
    void setIndex(int index);

    int propForRole(int) const;
    int modelDataPropertyId() const {
        QDeclarative1VisualDataModelPrivate *model = QDeclarative1VisualDataModelPrivate::get(m_model);
        return model->m_modelDataPropId;
    }

    void setValue(int, const QVariant &);
    bool hasValue(int id) const {
        return m_meta->hasValue(id);
    }

    void ensureProperties();

Q_SIGNALS:
    void indexChanged();

private:
    friend class QDeclarative1VisualDataModelDataMetaObject;
    int m_index;
    QDeclarativeGuard<QDeclarative1VisualDataModel> m_model;
    QDeclarative1VisualDataModelDataMetaObject *m_meta;
};

int QDeclarative1VisualDataModelData::propForRole(int id) const
{
    QDeclarative1VisualDataModelPrivate *model = QDeclarative1VisualDataModelPrivate::get(m_model);
    QHash<int,int>::const_iterator it = model->m_roleToPropId.find(id);
    if (it != model->m_roleToPropId.end())
        return *it;

    return -1;
}

void QDeclarative1VisualDataModelData::setValue(int id, const QVariant &val)
{
    m_meta->setValue(id, val);
}

int QDeclarative1VisualDataModelDataMetaObject::createProperty(const char *name, const char *type)
{
    QDeclarative1VisualDataModelData *data =
        static_cast<QDeclarative1VisualDataModelData *>(object());

    if (!data->m_model)
        return -1;

    QDeclarative1VisualDataModelPrivate *model = QDeclarative1VisualDataModelPrivate::get(data->m_model);
    if (data->m_index < 0 || data->m_index >= model->modelCount())
        return -1;

    if ((!model->m_listModelInterface || !model->m_abstractItemModel) && model->m_listAccessor) {
        if (model->m_listAccessor->type() == QDeclarative1ListAccessor::ListProperty) {
            model->ensureRoles();
            if (qstrcmp(name,"modelData") == 0)
                return QDeclarative1OpenMetaObject::createProperty(name, type);
        }
    }
    return -1;
}

QVariant QDeclarative1VisualDataModelDataMetaObject::initialValue(int propId)
{
    QDeclarative1VisualDataModelData *data =
        static_cast<QDeclarative1VisualDataModelData *>(object());

    Q_ASSERT(data->m_model);
    QDeclarative1VisualDataModelPrivate *model = QDeclarative1VisualDataModelPrivate::get(data->m_model);

    QByteArray propName = name(propId);
    if ((!model->m_listModelInterface || !model->m_abstractItemModel) && model->m_listAccessor) {
        if (propName == "modelData") {
            if (model->m_listAccessor->type() == QDeclarative1ListAccessor::Instance) {
                QObject *object = model->m_listAccessor->at(0).value<QObject*>();
                return object->metaObject()->property(1).read(object); // the first property after objectName
            }
            return model->m_listAccessor->at(data->m_index);
        } else {
            // return any property of a single object instance.
            QObject *object = model->m_listAccessor->at(data->m_index).value<QObject*>();
            return object->property(propName);
        }
    } else if (model->m_listModelInterface) {
        model->ensureRoles();
        QHash<QByteArray,int>::const_iterator it = model->m_roleNames.find(propName);
        if (it != model->m_roleNames.end()) {
            QVariant value = model->m_listModelInterface->data(data->m_index, *it);
            return value;
        } else if (model->m_roles.count() == 1 && propName == "modelData") {
            //for compatibility with other lists, assign modelData if there is only a single role
            QVariant value = model->m_listModelInterface->data(data->m_index, model->m_roles.first());
            return value;
        }
    } else if (model->m_abstractItemModel) {
        model->ensureRoles();
        QModelIndex index = model->m_abstractItemModel->index(data->m_index, 0, model->m_root);
        if (propName == "hasModelChildren") {
            return model->m_abstractItemModel->hasChildren(index);
        } else {
            QHash<QByteArray,int>::const_iterator it = model->m_roleNames.find(propName);
            if (it != model->m_roleNames.end()) {
                return model->m_abstractItemModel->data(index, *it);
            } else if (model->m_roles.count() == 1 && propName == "modelData") {
                //for compatibility with other lists, assign modelData if there is only a single role
                return model->m_abstractItemModel->data(index, model->m_roles.first());
            }
        }
    }
    Q_ASSERT(!"Can never be reached");
    return QVariant();
}

QDeclarative1VisualDataModelData::QDeclarative1VisualDataModelData(int index,
                                               QDeclarative1VisualDataModel *model)
: m_index(index), m_model(model),
m_meta(new QDeclarative1VisualDataModelDataMetaObject(this, QDeclarative1VisualDataModelPrivate::get(model)->m_delegateDataType))
{
    ensureProperties();
}

QDeclarative1VisualDataModelData::~QDeclarative1VisualDataModelData()
{
}

void QDeclarative1VisualDataModelData::ensureProperties()
{
    QDeclarative1VisualDataModelPrivate *modelPriv = QDeclarative1VisualDataModelPrivate::get(m_model);
    if (modelPriv->m_metaDataCacheable) {
        if (!modelPriv->m_metaDataCreated)
            modelPriv->createMetaData();
        if (modelPriv->m_metaDataCreated)
            m_meta->setCached(true);
    }
}

int QDeclarative1VisualDataModelData::index() const
{
    return m_index;
}

// This is internal only - it should not be set from qml
void QDeclarative1VisualDataModelData::setIndex(int index)
{
    m_index = index;
    emit indexChanged();
}

//---------------------------------------------------------------------------

class QDeclarative1VisualDataModelPartsMetaObject : public QDeclarative1OpenMetaObject
{
public:
    QDeclarative1VisualDataModelPartsMetaObject(QObject *parent)
    : QDeclarative1OpenMetaObject(parent) {}

    virtual void propertyCreated(int, QMetaPropertyBuilder &);
    virtual QVariant initialValue(int);
};

class QDeclarative1VisualDataModelParts : public QObject
{
Q_OBJECT
public:
    QDeclarative1VisualDataModelParts(QDeclarative1VisualDataModel *parent);

private:
    friend class QDeclarative1VisualDataModelPartsMetaObject;
    QDeclarative1VisualDataModel *model;
};

void QDeclarative1VisualDataModelPartsMetaObject::propertyCreated(int, QMetaPropertyBuilder &prop)
{
    prop.setWritable(false);
}

QVariant QDeclarative1VisualDataModelPartsMetaObject::initialValue(int id)
{
    QDeclarative1VisualDataModel *m = new QDeclarative1VisualDataModel;
    m->setParent(object());
    m->setPart(QString::fromUtf8(name(id)));
    m->setModel(QVariant::fromValue(static_cast<QDeclarative1VisualDataModelParts *>(object())->model));

    QVariant var = QVariant::fromValue((QObject *)m);
    return var;
}

QDeclarative1VisualDataModelParts::QDeclarative1VisualDataModelParts(QDeclarative1VisualDataModel *parent)
: QObject(parent), model(parent)
{
    new QDeclarative1VisualDataModelPartsMetaObject(this);
}

QDeclarative1VisualDataModelPrivate::QDeclarative1VisualDataModelPrivate(QDeclarativeContext *ctxt)
: m_listModelInterface(0), m_abstractItemModel(0), m_visualItemModel(0), m_delegate(0)
, m_context(ctxt), m_modelDataPropId(-1), m_parts(0), m_delegateDataType(0), m_metaDataCreated(false)
, m_metaDataCacheable(false), m_delegateValidated(false), m_completePending(false), m_listAccessor(0)
{
}

QDeclarative1VisualDataModelData *QDeclarative1VisualDataModelPrivate::data(QObject *item)
{
    QDeclarative1VisualDataModelData *dataItem =
        item->findChild<QDeclarative1VisualDataModelData *>();
    Q_ASSERT(dataItem);
    return dataItem;
}

//---------------------------------------------------------------------------

/*!
    \qmlclass VisualDataModel QDeclarative1VisualDataModel
    \inqmlmodule QtQuick 1
    \ingroup qml-working-with-data
    \brief The VisualDataModel encapsulates a model and delegate

    A VisualDataModel encapsulates a model and the delegate that will
    be instantiated for items in the model.

    It is usually not necessary to create VisualDataModel elements.
    However, it can be useful for manipulating and accessing the \l modelIndex 
    when a QAbstractItemModel subclass is used as the 
    model. Also, VisualDataModel is used together with \l Package to 
    provide delegates to multiple views.

    The example below illustrates using a VisualDataModel with a ListView.

    \snippet doc/src/snippets/qtquick1/visualdatamodel.qml 0
*/

QDeclarative1VisualDataModel::QDeclarative1VisualDataModel()
: QDeclarative1VisualModel(*(new QDeclarative1VisualDataModelPrivate(0)))
{
}

QDeclarative1VisualDataModel::QDeclarative1VisualDataModel(QDeclarativeContext *ctxt, QObject *parent)
: QDeclarative1VisualModel(*(new QDeclarative1VisualDataModelPrivate(ctxt)), parent)
{
}

QDeclarative1VisualDataModel::~QDeclarative1VisualDataModel()
{
    Q_D(QDeclarative1VisualDataModel);
    if (d->m_listAccessor)
        delete d->m_listAccessor;
    if (d->m_delegateDataType)
        d->m_delegateDataType->release();
}

/*!
    \qmlproperty model QtQuick1::VisualDataModel::model
    This property holds the model providing data for the VisualDataModel.

    The model provides a set of data that is used to create the items
    for a view.  For large or dynamic datasets the model is usually
    provided by a C++ model object.  The C++ model object must be a \l
    {QAbstractItemModel} subclass or a simple list.

    Models can also be created directly in QML, using a \l{ListModel} or
    \l{XmlListModel}.

    \sa {qmlmodels}{Data Models}
*/
QVariant QDeclarative1VisualDataModel::model() const
{
    Q_D(const QDeclarative1VisualDataModel);
    return d->m_modelVariant;
}

void QDeclarative1VisualDataModel::setModel(const QVariant &model)
{
    Q_D(QDeclarative1VisualDataModel);
    delete d->m_listAccessor;
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
    } else if (d->m_visualItemModel) {
        QObject::disconnect(d->m_visualItemModel, SIGNAL(itemsInserted(int,int)),
                         this, SIGNAL(itemsInserted(int,int)));
        QObject::disconnect(d->m_visualItemModel, SIGNAL(itemsRemoved(int,int)),
                         this, SIGNAL(itemsRemoved(int,int)));
        QObject::disconnect(d->m_visualItemModel, SIGNAL(itemsMoved(int,int,int)),
                         this, SIGNAL(itemsMoved(int,int,int)));
        QObject::disconnect(d->m_visualItemModel, SIGNAL(createdPackage(int,QDeclarative1Package*)),
                         this, SLOT(_q_createdPackage(int,QDeclarative1Package*)));
        QObject::disconnect(d->m_visualItemModel, SIGNAL(destroyingPackage(QDeclarative1Package*)),
                         this, SLOT(_q_destroyingPackage(QDeclarative1Package*)));
        d->m_visualItemModel = 0;
    }

    d->m_roles.clear();
    d->m_roleNames.clear();
    if (d->m_delegateDataType)
        d->m_delegateDataType->release();
    d->m_metaDataCreated = 0;
    d->m_metaDataCacheable = false;
    d->m_delegateDataType = new VDMDelegateDataType(&QDeclarative1VisualDataModelData::staticMetaObject, d->m_context?d->m_context->engine():qmlEngine(this));

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
        d->m_metaDataCacheable = true;
        if (d->m_delegate && d->m_listModelInterface->count())
            emit itemsInserted(0, d->m_listModelInterface->count());
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
        d->m_metaDataCacheable = true;
        if (d->m_abstractItemModel->canFetchMore(d->m_root))
            d->m_abstractItemModel->fetchMore(d->m_root);
        return;
    }
    if ((d->m_visualItemModel = qvariant_cast<QDeclarative1VisualDataModel *>(model))) {
        QObject::connect(d->m_visualItemModel, SIGNAL(itemsInserted(int,int)),
                         this, SIGNAL(itemsInserted(int,int)));
        QObject::connect(d->m_visualItemModel, SIGNAL(itemsRemoved(int,int)),
                         this, SIGNAL(itemsRemoved(int,int)));
        QObject::connect(d->m_visualItemModel, SIGNAL(itemsMoved(int,int,int)),
                         this, SIGNAL(itemsMoved(int,int,int)));
        QObject::connect(d->m_visualItemModel, SIGNAL(createdPackage(int,QDeclarative1Package*)),
                         this, SLOT(_q_createdPackage(int,QDeclarative1Package*)));
        QObject::connect(d->m_visualItemModel, SIGNAL(destroyingPackage(QDeclarative1Package*)),
                         this, SLOT(_q_destroyingPackage(QDeclarative1Package*)));
        return;
    }
    d->m_listAccessor = new QDeclarative1ListAccessor;
    d->m_listAccessor->setList(model, d->m_context?d->m_context->engine():qmlEngine(this));
    if (d->m_listAccessor->type() != QDeclarative1ListAccessor::ListProperty)
        d->m_metaDataCacheable = true;
    if (d->m_delegate && d->modelCount()) {
        emit itemsInserted(0, d->modelCount());
        emit countChanged();
    }
}

/*!
    \qmlproperty Component QtQuick1::VisualDataModel::delegate

    The delegate provides a template defining each item instantiated by a view.
    The index is exposed as an accessible \c index property.  Properties of the
    model are also available depending upon the type of \l {qmlmodels}{Data Model}.
*/
QDeclarativeComponent *QDeclarative1VisualDataModel::delegate() const
{
    Q_D(const QDeclarative1VisualDataModel);
    if (d->m_visualItemModel)
        return d->m_visualItemModel->delegate();
    return d->m_delegate;
}

void QDeclarative1VisualDataModel::setDelegate(QDeclarativeComponent *delegate)
{
    Q_D(QDeclarative1VisualDataModel);
    bool wasValid = d->m_delegate != 0;
    d->m_delegate = delegate;
    d->m_delegateValidated = false;
    if (!wasValid && d->modelCount() && d->m_delegate) {
        emit itemsInserted(0, d->modelCount());
        emit countChanged();
    }
    if (wasValid && !d->m_delegate && d->modelCount()) {
        emit itemsRemoved(0, d->modelCount());
        emit countChanged();
    }
}

/*!
    \qmlproperty QModelIndex QtQuick1::VisualDataModel::rootIndex

    QAbstractItemModel provides a hierarchical tree of data, whereas
    QML only operates on list data.  \c rootIndex allows the children of
    any node in a QAbstractItemModel to be provided by this model.

    This property only affects models of type QAbstractItemModel that
    are hierarchical (e.g, a tree model). 

    For example, here is a simple interactive file system browser.
    When a directory name is clicked, the view's \c rootIndex is set to the
    QModelIndex node of the clicked directory, thus updating the view to show
    the new directory's contents.

    \c main.cpp:
    \snippet doc/src/snippets/qtquick1/visualdatamodel_rootindex/main.cpp 0
   
    \c view.qml:
    \snippet doc/src/snippets/qtquick1/visualdatamodel_rootindex/view.qml 0

    If the \l model is a QAbstractItemModel subclass, the delegate can also
    reference a \c hasModelChildren property (optionally qualified by a
    \e model. prefix) that indicates whether the delegate's model item has 
    any child nodes.


    \sa modelIndex(), parentModelIndex()
*/
QVariant QDeclarative1VisualDataModel::rootIndex() const
{
    Q_D(const QDeclarative1VisualDataModel);
    return QVariant::fromValue(d->m_root);
}

void QDeclarative1VisualDataModel::setRootIndex(const QVariant &root)
{
    Q_D(QDeclarative1VisualDataModel);
    QModelIndex modelIndex = qvariant_cast<QModelIndex>(root);
    if (d->m_root != modelIndex) {
        int oldCount = d->modelCount();
        d->m_root = modelIndex;
        if (d->m_abstractItemModel && d->m_abstractItemModel->canFetchMore(modelIndex))
            d->m_abstractItemModel->fetchMore(modelIndex);
        int newCount = d->modelCount();
        if (d->m_delegate && oldCount)
            emit itemsRemoved(0, oldCount);
        if (d->m_delegate && newCount)
            emit itemsInserted(0, newCount);
        if (newCount != oldCount)
            emit countChanged();
        emit rootIndexChanged();
    }
}


/*!
    \qmlmethod QModelIndex QtQuick1::VisualDataModel::modelIndex(int index)

    QAbstractItemModel provides a hierarchical tree of data, whereas
    QML only operates on list data.  This function assists in using
    tree models in QML.

    Returns a QModelIndex for the specified index.
    This value can be assigned to rootIndex.

    \sa rootIndex
*/
QVariant QDeclarative1VisualDataModel::modelIndex(int idx) const
{
    Q_D(const QDeclarative1VisualDataModel);
    if (d->m_abstractItemModel)
        return QVariant::fromValue(d->m_abstractItemModel->index(idx, 0, d->m_root));
    return QVariant::fromValue(QModelIndex());
}

/*!
    \qmlmethod QModelIndex QtQuick1::VisualDataModel::parentModelIndex()

    QAbstractItemModel provides a hierarchical tree of data, whereas
    QML only operates on list data.  This function assists in using
    tree models in QML.

    Returns a QModelIndex for the parent of the current rootIndex.
    This value can be assigned to rootIndex.

    \sa rootIndex
*/
QVariant QDeclarative1VisualDataModel::parentModelIndex() const
{
    Q_D(const QDeclarative1VisualDataModel);
    if (d->m_abstractItemModel)
        return QVariant::fromValue(d->m_abstractItemModel->parent(d->m_root));
    return QVariant::fromValue(QModelIndex());
}

QString QDeclarative1VisualDataModel::part() const
{
    Q_D(const QDeclarative1VisualDataModel);
    return d->m_part;
}

void QDeclarative1VisualDataModel::setPart(const QString &part)
{
    Q_D(QDeclarative1VisualDataModel);
    d->m_part = part;
}

int QDeclarative1VisualDataModel::count() const
{
    Q_D(const QDeclarative1VisualDataModel);
    if (d->m_visualItemModel)
        return d->m_visualItemModel->count();
    if (!d->m_delegate)
        return 0;
    return d->modelCount();
}

QDeclarativeItem *QDeclarative1VisualDataModel::item(int index, bool complete)
{
    Q_D(QDeclarative1VisualDataModel);
    if (d->m_visualItemModel)
        return d->m_visualItemModel->item(index, d->m_part.toUtf8(), complete);
    return item(index, QByteArray(), complete);
}

/*
  Returns ReleaseStatus flags.
*/
QDeclarative1VisualDataModel::ReleaseFlags QDeclarative1VisualDataModel::release(QDeclarativeItem *item)
{
    Q_D(QDeclarative1VisualDataModel);
    if (d->m_visualItemModel)
        return d->m_visualItemModel->release(item);

    ReleaseFlags stat = 0;
    QObject *obj = item;
    bool inPackage = false;

    QHash<QObject*,QDeclarative1Package*>::iterator it = d->m_packaged.find(item);
    if (it != d->m_packaged.end()) {
        QDeclarative1Package *package = *it;
        d->m_packaged.erase(it);
        if (d->m_packaged.contains(item))
            stat |= Referenced;
        inPackage = true;
        obj = package; // fall through and delete
    }

    if (d->m_cache.releaseItem(obj)) {
        // Remove any bindings to avoid warnings due to parent change.
        QObjectPrivate *p = QObjectPrivate::get(obj);
        Q_ASSERT(p->declarativeData);
        QDeclarativeData *d = static_cast<QDeclarativeData*>(p->declarativeData);
        if (d->ownContext && d->context)
            d->context->clearContext();

        if (inPackage) {
            emit destroyingPackage(qobject_cast<QDeclarative1Package*>(obj));
        } else {
            if (item->scene())
                item->scene()->removeItem(item);
        }
        stat |= Destroyed;
        obj->deleteLater();
    } else if (!inPackage) {
        stat |= Referenced;
    }

    return stat;
}

/*!
    \qmlproperty object QtQuick1::VisualDataModel::parts

    The \a parts property selects a VisualDataModel which creates
    delegates from the part named.  This is used in conjunction with
    the \l Package element.

    For example, the code below selects a model which creates
    delegates named \e list from a \l Package:

    \code
    VisualDataModel {
        id: visualModel
        delegate: Package {
            Item { Package.name: "list" }
        }
        model: myModel
    }

    ListView {
        width: 200; height:200
        model: visualModel.parts.list
    }
    \endcode

    \sa Package
*/
QObject *QDeclarative1VisualDataModel::parts()
{
    Q_D(QDeclarative1VisualDataModel);
    if (!d->m_parts)
        d->m_parts = new QDeclarative1VisualDataModelParts(this);
    return d->m_parts;
}

QDeclarativeItem *QDeclarative1VisualDataModel::item(int index, const QByteArray &viewId, bool complete)
{
    Q_D(QDeclarative1VisualDataModel);
    if (d->m_visualItemModel)
        return d->m_visualItemModel->item(index, viewId, complete);

    if (d->modelCount() <= 0 || !d->m_delegate)
        return 0;
    QObject *nobj = d->m_cache.getItem(index);
    bool needComplete = false;
    if (!nobj) {
        QDeclarativeContext *ccontext = d->m_context;
        if (!ccontext) ccontext = qmlContext(this);
        QDeclarativeContext *ctxt = new QDeclarativeContext(ccontext);
        QDeclarative1VisualDataModelData *data = new QDeclarative1VisualDataModelData(index, this);
        if ((!d->m_listModelInterface || !d->m_abstractItemModel) && d->m_listAccessor
            && d->m_listAccessor->type() == QDeclarative1ListAccessor::ListProperty) {
            ctxt->setContextObject(d->m_listAccessor->at(index).value<QObject*>());
            ctxt = new QDeclarativeContext(ctxt, ctxt);
        }
        ctxt->setContextProperty(QLatin1String("model"), data);
        ctxt->setContextObject(data);
        d->m_completePending = false;
        nobj = d->m_delegate->beginCreate(ctxt);
        if (complete) {
            d->m_delegate->completeCreate();
        } else {
            d->m_completePending = true;
            needComplete = true;
        }
        if (nobj) {
            QDeclarative_setParent_noEvent(ctxt, nobj);
            QDeclarative_setParent_noEvent(data, nobj);
            d->m_cache.insertItem(index, nobj);
            if (QDeclarative1Package *package = qobject_cast<QDeclarative1Package *>(nobj))
                emit createdPackage(index, package);
        } else {
            delete data;
            delete ctxt;
            qmlInfo(this, d->m_delegate->errors()) << "Error creating delegate";
        }
    }
    QDeclarativeItem *item = qobject_cast<QDeclarativeItem *>(nobj);
    if (!item) {
        QDeclarative1Package *package = qobject_cast<QDeclarative1Package *>(nobj);
        if (package) {
            QObject *o = package->part(QString::fromUtf8(viewId));
            item = qobject_cast<QDeclarativeItem *>(o);
            if (item)
                d->m_packaged.insertMulti(item, package);
        }
    }
    if (!item) {
        if (needComplete)
            d->m_delegate->completeCreate();
        d->m_cache.releaseItem(nobj);
        if (!d->m_delegateValidated) {
            qmlInfo(d->m_delegate) << QDeclarative1VisualDataModel::tr("Delegate component must be Item type.");
            d->m_delegateValidated = true;
        }
    }
    if (d->modelCount()-1 == index && d->m_abstractItemModel && d->m_abstractItemModel->canFetchMore(d->m_root))
        d->m_abstractItemModel->fetchMore(d->m_root);

    return item;
}

bool QDeclarative1VisualDataModel::completePending() const
{
    Q_D(const QDeclarative1VisualDataModel);
    if (d->m_visualItemModel)
        return d->m_visualItemModel->completePending();
    return d->m_completePending;
}

void QDeclarative1VisualDataModel::completeItem()
{
    Q_D(QDeclarative1VisualDataModel);
    if (d->m_visualItemModel) {
        d->m_visualItemModel->completeItem();
        return;
    }

    d->m_delegate->completeCreate();
    d->m_completePending = false;
}

QString QDeclarative1VisualDataModel::stringValue(int index, const QString &name)
{
    Q_D(QDeclarative1VisualDataModel);
    if (d->m_visualItemModel)
        return d->m_visualItemModel->stringValue(index, name);

    if ((!d->m_listModelInterface || !d->m_abstractItemModel) && d->m_listAccessor) {
        if (QObject *object = d->m_listAccessor->at(index).value<QObject*>())
            return object->property(name.toUtf8()).toString();
    }

    if ((!d->m_listModelInterface && !d->m_abstractItemModel) || !d->m_delegate)
        return QString();

    QString val;
    QObject *data = 0;
    bool tempData = false;

    if (QObject *nobj = d->m_cache.item(index))
        data = d->data(nobj);
    if (!data) {
        data = new QDeclarative1VisualDataModelData(index, this);
        tempData = true;
    }

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

    if (tempData)
        delete data;

    return val;
}

int QDeclarative1VisualDataModel::indexOf(QDeclarativeItem *item, QObject *) const
{
    QVariant val = QDeclarativeEngine::contextForObject(item)->contextProperty(QLatin1String("index"));
        return val.toInt();
    return -1;
}

void QDeclarative1VisualDataModel::setWatchedRoles(QList<QByteArray> roles)
{
    Q_D(QDeclarative1VisualDataModel);
    d->watchedRoles = roles;
    d->watchedRoleIds.clear();
}

void QDeclarative1VisualDataModel::_q_itemsChanged(int index, int count,
                                         const QList<int> &roles)
{
    Q_D(QDeclarative1VisualDataModel);
    bool changed = false;
    if (!d->watchedRoles.isEmpty() && d->watchedRoleIds.isEmpty()) {
        foreach (QByteArray r, d->watchedRoles) {
            if (d->m_roleNames.contains(r))
                d->watchedRoleIds << d->m_roleNames.value(r);
        }
    }

    for (QHash<int,QDeclarative1VisualDataModelPrivate::ObjectRef>::ConstIterator iter = d->m_cache.begin();
        iter != d->m_cache.end(); ++iter) {
        const int idx = iter.key();

        if (idx >= index && idx < index+count) {
            QDeclarative1VisualDataModelPrivate::ObjectRef objRef = *iter;
            QDeclarative1VisualDataModelData *data = d->data(objRef.obj);
            for (int roleIdx = 0; roleIdx < roles.count(); ++roleIdx) {
                int role = roles.at(roleIdx);
                if (!changed && !d->watchedRoleIds.isEmpty() && d->watchedRoleIds.contains(role))
                    changed = true;
                int propId = data->propForRole(role);
                if (propId != -1) {
                    if (data->hasValue(propId)) {
                        if (d->m_listModelInterface) {
                            data->setValue(propId, d->m_listModelInterface->data(idx, role));
                        } else if (d->m_abstractItemModel) {
                            QModelIndex index = d->m_abstractItemModel->index(idx, 0, d->m_root);
                            data->setValue(propId, d->m_abstractItemModel->data(index, role));
                        }
                    }
                } else {
                    QString roleName;
                    if (d->m_listModelInterface)
                        roleName = d->m_listModelInterface->toString(role);
                    else if (d->m_abstractItemModel)
                        roleName = QString::fromUtf8(d->m_abstractItemModel->roleNames().value(role));
                    qmlInfo(this) << "Changing role not present in item: " << roleName;
                }
            }
            if (d->m_roles.count() == 1) {
                // Handle the modelData role we add if there is just one role.
                int propId = data->modelDataPropertyId();
                if (data->hasValue(propId)) {
                    int role = d->m_roles.at(0);
                    if (d->m_listModelInterface) {
                        data->setValue(propId, d->m_listModelInterface->data(idx, role));
                    } else if (d->m_abstractItemModel) {
                        QModelIndex index = d->m_abstractItemModel->index(idx, 0, d->m_root);
                        data->setValue(propId, d->m_abstractItemModel->data(index, role));
                    }
                }
            }
        }
    }
    if (changed)
        emit itemsChanged(index, count);
}

void QDeclarative1VisualDataModel::_q_itemsInserted(int index, int count)
{
    Q_D(QDeclarative1VisualDataModel);
    if (!count)
        return;
    // XXX - highly inefficient
    QHash<int,QDeclarative1VisualDataModelPrivate::ObjectRef> items;
    for (QHash<int,QDeclarative1VisualDataModelPrivate::ObjectRef>::Iterator iter = d->m_cache.begin();
        iter != d->m_cache.end(); ) {

        if (iter.key() >= index) {
            QDeclarative1VisualDataModelPrivate::ObjectRef objRef = *iter;
            int index = iter.key() + count;
            iter = d->m_cache.erase(iter);

            items.insert(index, objRef);

            QDeclarative1VisualDataModelData *data = d->data(objRef.obj);
            data->setIndex(index);
        } else {
            ++iter;
        }
    }
    d->m_cache.unite(items);

    emit itemsInserted(index, count);
    emit countChanged();
}

void QDeclarative1VisualDataModel::_q_itemsRemoved(int index, int count)
{
    Q_D(QDeclarative1VisualDataModel);
    if (!count)
        return;
    // XXX - highly inefficient
    QHash<int, QDeclarative1VisualDataModelPrivate::ObjectRef> items;
    for (QHash<int, QDeclarative1VisualDataModelPrivate::ObjectRef>::Iterator iter = d->m_cache.begin();
        iter != d->m_cache.end(); ) {
        if (iter.key() >= index && iter.key() < index + count) {
            QDeclarative1VisualDataModelPrivate::ObjectRef objRef = *iter;
            iter = d->m_cache.erase(iter);
            items.insertMulti(-1, objRef); //XXX perhaps better to maintain separately
            QDeclarative1VisualDataModelData *data = d->data(objRef.obj);
            data->setIndex(-1);
        } else if (iter.key() >= index + count) {
            QDeclarative1VisualDataModelPrivate::ObjectRef objRef = *iter;
            int index = iter.key() - count;
            iter = d->m_cache.erase(iter);
            items.insert(index, objRef);
            QDeclarative1VisualDataModelData *data = d->data(objRef.obj);
            data->setIndex(index);
        } else {
            ++iter;
        }
    }

    d->m_cache.unite(items);
    emit itemsRemoved(index, count);
    emit countChanged();
}

void QDeclarative1VisualDataModel::_q_itemsMoved(int from, int to, int count)
{
    Q_D(QDeclarative1VisualDataModel);
    // XXX - highly inefficient
    QHash<int,QDeclarative1VisualDataModelPrivate::ObjectRef> items;
    for (QHash<int,QDeclarative1VisualDataModelPrivate::ObjectRef>::Iterator iter = d->m_cache.begin();
        iter != d->m_cache.end(); ) {

        if (iter.key() >= from && iter.key() < from + count) {
            QDeclarative1VisualDataModelPrivate::ObjectRef objRef = *iter;
            int index = iter.key() - from + to;
            iter = d->m_cache.erase(iter);

            items.insert(index, objRef);

            QDeclarative1VisualDataModelData *data = d->data(objRef.obj);
            data->setIndex(index);
        } else {
            ++iter;
        }
    }
    for (QHash<int,QDeclarative1VisualDataModelPrivate::ObjectRef>::Iterator iter = d->m_cache.begin();
        iter != d->m_cache.end(); ) {

        int diff = from > to ? count : -count;
        if (iter.key() >= qMin(from,to) && iter.key() < qMax(from+count,to+count)) {
            QDeclarative1VisualDataModelPrivate::ObjectRef objRef = *iter;
            int index = iter.key() + diff;
            iter = d->m_cache.erase(iter);

            items.insert(index, objRef);

            QDeclarative1VisualDataModelData *data = d->data(objRef.obj);
            data->setIndex(index);
        } else {
            ++iter;
        }
    }
    d->m_cache.unite(items);

    emit itemsMoved(from, to, count);
}

void QDeclarative1VisualDataModel::_q_rowsInserted(const QModelIndex &parent, int begin, int end)
{
    Q_D(QDeclarative1VisualDataModel);
    if (parent == d->m_root)
        _q_itemsInserted(begin, end - begin + 1);
}

void QDeclarative1VisualDataModel::_q_rowsRemoved(const QModelIndex &parent, int begin, int end)
{
    Q_D(QDeclarative1VisualDataModel);
    if (parent == d->m_root)
        _q_itemsRemoved(begin, end - begin + 1);
}

void QDeclarative1VisualDataModel::_q_rowsMoved(const QModelIndex &sourceParent, int sourceStart, int sourceEnd, const QModelIndex &destinationParent, int destinationRow)
{
    Q_D(QDeclarative1VisualDataModel);
    const int count = sourceEnd - sourceStart + 1;
    if (destinationParent == d->m_root && sourceParent == d->m_root) {
        _q_itemsMoved(sourceStart, sourceStart > destinationRow ? destinationRow : destinationRow-count, count);
    } else if (sourceParent == d->m_root) {
        _q_itemsRemoved(sourceStart, count);
    } else if (destinationParent == d->m_root) {
        _q_itemsInserted(destinationRow, count);
    }
}

void QDeclarative1VisualDataModel::_q_dataChanged(const QModelIndex &begin, const QModelIndex &end)
{
    Q_D(QDeclarative1VisualDataModel);
    if (begin.parent() == d->m_root)
        _q_itemsChanged(begin.row(), end.row() - begin.row() + 1, d->m_roles);
}

void QDeclarative1VisualDataModel::_q_layoutChanged()
{
    Q_D(QDeclarative1VisualDataModel);
    _q_itemsChanged(0, count(), d->m_roles);
}

void QDeclarative1VisualDataModel::_q_modelReset()
{
    Q_D(QDeclarative1VisualDataModel);
    d->m_root = QModelIndex();
    emit modelReset();
    emit rootIndexChanged();
    if (d->m_abstractItemModel && d->m_abstractItemModel->canFetchMore(d->m_root))
        d->m_abstractItemModel->fetchMore(d->m_root);
}

void QDeclarative1VisualDataModel::_q_createdPackage(int index, QDeclarative1Package *package)
{
    Q_D(QDeclarative1VisualDataModel);
    emit createdItem(index, qobject_cast<QDeclarativeItem*>(package->part(d->m_part)));
}

void QDeclarative1VisualDataModel::_q_destroyingPackage(QDeclarative1Package *package)
{
    Q_D(QDeclarative1VisualDataModel);
    emit destroyingItem(qobject_cast<QDeclarativeItem*>(package->part(d->m_part)));
}



QT_END_NAMESPACE

QML_DECLARE_TYPE(QListModelInterface)

#include <qdeclarativevisualitemmodel.moc>
