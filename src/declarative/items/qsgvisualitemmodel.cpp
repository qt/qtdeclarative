// Commit: dcb9148091cbf6872b60407c301d7c92427583a6
/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtDeclarative module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qsgvisualitemmodel_p.h"
#include "qsgitem.h"

#include <QtDeclarative/qdeclarativecontext.h>
#include <QtDeclarative/qdeclarativeengine.h>
#include <QtDeclarative/qdeclarativeexpression.h>
#include <QtDeclarative/qdeclarativeinfo.h>

#include <private/qdeclarativecontext_p.h>
#include <private/qdeclarativeengine_p.h>
#include <private/qdeclarativepackage_p.h>
#include <private/qdeclarativeopenmetaobject_p.h>
#include <private/qdeclarativelistaccessor_p.h>
#include <private/qdeclarativedata_p.h>
#include <private/qdeclarativepropertycache_p.h>
#include <private/qdeclarativeguard_p.h>
#include <private/qdeclarativeglobal_p.h>
#include <private/qlistmodelinterface_p.h>
#include <private/qmetaobjectbuilder_p.h>
#include <private/qobject_p.h>
#include <private/qdeclarativechangeset_p.h>
#include <private/qdeclarativelistcompositor_p.h>

#include <QtCore/qhash.h>
#include <QtCore/qlist.h>

QT_BEGIN_NAMESPACE

QHash<QObject*, QSGVisualItemModelAttached*> QSGVisualItemModelAttached::attachedProperties;

class QSGVisualItemModelPrivate : public QObjectPrivate, public QDeclarativeListCompositor
{
    Q_DECLARE_PUBLIC(QSGVisualItemModel)
public:
    QSGVisualItemModelPrivate() : QObjectPrivate(), QDeclarativeListCompositor(0), pendingModel(0), childrenChanged(false), transaction(false) {}

    static void children_append(QDeclarativeListProperty<QObject> *prop, QObject *child) {
        QSGVisualItemModelPrivate *d = static_cast<QSGVisualItemModelPrivate *>(prop->data);
        QDeclarative_setParent_noEvent(child, prop->object);
        if (QSGVisualDataModel *model = qobject_cast<QSGVisualDataModel *>(child)) {
            QSGVisualItemModel *q = d->q_func();
            const int insertIndex = d->count();
            const int insertCount = model->count();
            d->appendList(model, 0, insertCount, true);
            if (insertCount > 0)
                emit q->itemsInserted(insertIndex, insertCount);
            QObject::connect(model, SIGNAL(itemsInserted(int,int)),
                    q, SLOT(_q_itemsInserted(int,int)));
            QObject::connect(model, SIGNAL(itemsRemoved(int,int)),
                    q, SLOT(_q_itemsRemoved(int,int)));
            QObject::connect(model, SIGNAL(itemsMoved(int,int,int)),
                    q, SLOT(_q_itemsMoved(int,int,int)));
        } else if (QSGItem *item = qobject_cast<QSGItem *>(child)) {
            d->appendData(item);
            d->itemAppended();
            d->emitChildrenChanged();
        }
    }

    static int children_count(QDeclarativeListProperty<QObject> *prop) {
        return static_cast<QSGVisualItemModelPrivate *>(prop->data)->children.count();
    }

    static QObject *children_at(QDeclarativeListProperty<QObject> *prop, int index) {
        return static_cast<QSGVisualItemModelPrivate *>(prop->data)->children.at(index).item;
    }

    void itemAppended() {
        Q_Q(QSGVisualItemModel);
        QSGVisualItemModelAttached *attached = QSGVisualItemModelAttached::properties(children.last().item);
        attached->setIndex(count()-1);
        emit q->itemsInserted(count()-1, 1);
        emit q->countChanged();
    }

    void emitChildrenChanged() {
        Q_Q(QSGVisualItemModel);
        emit q->childrenChanged();
    }

    int indexOf(QSGItem *item) const {
        for (int i = 0; i < children.count(); ++i)
            if (children.at(i).item == item)
                return i;
        return -1;
    }

    void emitTransactionChanges() {
        Q_Q(QSGVisualItemModel);
        foreach (const QDeclarativeChangeSet::Remove &remove, transactionChanges.removes())
            emit q->itemsRemoved(remove.start, remove.count());
        foreach (const QDeclarativeChangeSet::Insert &insert, transactionChanges.inserts()) {
            qDebug() << "Insert" << insert.start << insert.end;
            emit q->itemsInserted(insert.start, insert.count());
        }
        foreach (const QDeclarativeChangeSet::Move &move, transactionChanges.moves())
            emit q->itemsMoved(move.start, move.to, move.count());
        transactionChanges.clear();
    }

    class Item {
    public:
        Item(QSGItem *i) : item(i), ref(0) {}

        void addRef() { ++ref; }
        bool deref() { return --ref == 0; }

        QSGItem *item;
        int ref;
    };

    QList<Item> children;
    QHash<QSGItem *, QSGVisualDataModel *> itemModels;
    QHash<QSGItem *, int> removedItems;
    QDeclarativeChangeSet transactionChanges;
    QSGVisualDataModel *pendingModel;
    bool childrenChanged;
    bool transaction;

protected:
    void rangeCreated(void *) {}
    void rangeDestroyed(void *) {}

    bool insertInternalData(int index, const void *data)
    {
        qDebug() << Q_FUNC_INFO << index;
        childrenChanged = true;
        QSGItem *item = static_cast<QSGItem *>(const_cast<void *>(data));
        children.insert(index, Item(item));
        return true;
    }

    void replaceInternalData(int index, const void *data)
    {
        childrenChanged = true;
        QSGVisualItemModelPrivate::Item &item = children[index];
        if (item.ref > 0)
            removedItems.insert(item.item, item.ref);
        item.item = static_cast<QSGItem *>(const_cast<void *>(data));
        item.ref = 0;
    }

    void removeInternalData(int index, int count)
    {
        childrenChanged = true;
        for (int i = 0; i > count; ++i) {
            QSGVisualItemModelPrivate::Item &item = children[index + i];
            if (item.ref > 0)
                removedItems.insert(item.item, item.ref);
        }
        QList<Item>::iterator first = children.begin() + index;
        QList<Item>::iterator last = first + count - 1;
        children.erase(first, last);
    }

    void moveInternalData(int from, int to, int n)
    {
        qDebug() << Q_FUNC_INFO << from << to << n;
        childrenChanged = true;
        if (from > to) {
            // Only move forwards - flip if backwards moving
            int tfrom = from;
            int tto = to;
            from = tto;
            to = tto+n;
            n = tfrom-tto;
        }

        if (n == 1) {
            children.move(from, to);
        } else {
            QList<Item> replaced;
            int i=0;
            QList<Item>::ConstIterator it=children.begin(); it += from+n;
            for (; i<to-from; ++i,++it)
                replaced.append(*it);
            i=0;
            it=children.begin(); it += from;
            for (; i<n; ++i,++it)
                replaced.append(*it);
            QList<Item>::ConstIterator f=replaced.begin();
            QList<Item>::Iterator t=children.begin(); t += from;
            for (; f != replaced.end(); ++f, ++t)
                *t = *f;
        }
    }
};

QSGVisualItemModel::QSGVisualItemModel(QObject *parent)
    : QSGVisualModel(*(new QSGVisualItemModelPrivate), parent)
{
}

QDeclarativeListProperty<QObject> QSGVisualItemModel::children()
{
    Q_D(QSGVisualItemModel);
    return QDeclarativeListProperty<QObject>(this, d, d->children_append,
                                                      d->children_count, d->children_at);
}

int QSGVisualItemModel::count() const
{
    Q_D(const QSGVisualItemModel);
    return d->count();
}

bool QSGVisualItemModel::isValid() const
{
    return true;
}

QSGItem *QSGVisualItemModel::item(int index, bool complete)
{
    Q_D(QSGVisualItemModel);
    int modelIndex = 0;
    int internalIndex = 0;
    QDeclarativeCompositeRange range = d->at(index, &modelIndex, &internalIndex);
    if (!range.internal()) {
        QSGVisualDataModel *model = static_cast<QSGVisualDataModel *>(range.list);
        QSGItem *item = model->item(modelIndex, complete);
        d->itemModels.insert(item, model);
        if (model->completePending())
            d->pendingModel = model;
        return item;
    } else {
        QSGVisualItemModelPrivate::Item &item = d->children[internalIndex];
        item.addRef();
        return item.item;
    }
}

QSGVisualModel::ReleaseFlags QSGVisualItemModel::release(QSGItem *item)
{
    Q_D(QSGVisualItemModel);
    if (QSGVisualDataModel *model = d->itemModels.value(item)) {
        ReleaseFlags flags = model->release(item);
        if (flags)
            d->itemModels.remove(item);
        return flags;
    } else {
        int idx = d->indexOf(item);
        if (idx >= 0) {
            if (d->children[idx].deref()) {
                // XXX todo - the original did item->scene()->removeItem().  Why?
                item->setParentItem(0);
                QDeclarative_setParent_noEvent(item, this);
            }
        } else {
            QHash<QSGItem *, int>::iterator it = d->removedItems.find(item);
            if (it != d->removedItems.end()) {
                if (--(*it) == 0) {
                    delete it.key();
                    d->removedItems.erase(it);
                }
            }
        }
    }
    return 0;
}

bool QSGVisualItemModel::completePending() const
{
    Q_D(const QSGVisualItemModel);
    return d->pendingModel;
}

void QSGVisualItemModel::completeItem()
{
    Q_D(QSGVisualItemModel);
    if (d->pendingModel) {
        d->pendingModel->completeItem();
        d->pendingModel = 0;
    }
}

QString QSGVisualItemModel::stringValue(int index, const QString &name)
{
    Q_D(QSGVisualItemModel);
    if (index < 0 || index >= d->children.count())
        return QString();
    return QDeclarativeEngine::contextForObject(d->children.at(index).item)->contextProperty(name).toString();
}

int QSGVisualItemModel::indexOf(QSGItem *item, QObject *context) const
{
    Q_D(const QSGVisualItemModel);
    if (QSGVisualDataModel *model = d->itemModels.value(item)) {
        int modelIndex = model->indexOf(item, context);
        return modelIndex != -1 ? d->absoluteIndexOf(model, modelIndex) : modelIndex;
    } else {
        int childIndex = d->indexOf(item);
        return childIndex != -1 ? d->absoluteIndexOf(childIndex) : childIndex;
    }
}

QScriptValue QSGVisualItemModel::getItemInfo(int index) const
{
    Q_D(const QSGVisualItemModel);
    QScriptEngine *engine = QDeclarativeEnginePrivate::getScriptEngine(qmlEngine(this));
    QScriptValue info = engine->newObject();

    int modelIndex = 0;
    int internalIndex = 0;
    QDeclarativeCompositeRange range = d->at(index, &modelIndex, &internalIndex);

    if (!range.internal()) {
        info.setProperty(QLatin1String("modelItem"), true);
        info.setProperty(QLatin1String("model"), engine->newVariant(static_cast<QSGVisualDataModel *>(range.list)->model()));
        info.setProperty(QLatin1String("index"), modelIndex);
        info.setProperty(QLatin1String("start"), range.index);
        info.setProperty(QLatin1String("end"), range.index + range.count);
    } else {
        info.setProperty(QLatin1String("modelItem"), false);
    }
    return info;
}

void QSGVisualItemModel::append(QSGItem *item)
{
    Q_D(QSGVisualItemModel);
    int index = d->count();
    if (d->appendData(item)) {
        if (d->transaction) {
            d->transactionChanges.insertInsert(index, index + 1);
        } else {
            emit itemsInserted(index, 1);
            emit childrenChanged();
            emit countChanged();
        }
    }
}

void QSGVisualItemModel::insert(int index, QSGItem *item)
{
    qDebug() << Q_FUNC_INFO << index;
    Q_D(QSGVisualItemModel);
    if (d->insertData(index, item)) {
        if (d->transaction) {
            d->transactionChanges.insertInsert(index, index + 1);
        } else {
            emit itemsInserted(index, 1);
            emit childrenChanged();
            emit countChanged();
        }
    }
}

void QSGVisualItemModel::remove(int index, int count)
{
    Q_D(QSGVisualItemModel);
    if (!d->transaction)
        d->childrenChanged = false;
    d->removeAt(index, count);
    if (d->transaction) {
        d->transactionChanges.insertRemove(index, index + count);
    } else {      
        emit itemsRemoved(index, count);
        emit countChanged();
        if (d->childrenChanged)
            emit childrenChanged();
    }
}

void QSGVisualItemModel::move(int from, int to, int count)
{
    Q_D(QSGVisualItemModel);
    if (from == to || count == 0)
        return;
    if (!d->transaction)
        d->childrenChanged = false;
    d->move(from, to, count);
    if (d->transaction) {
        d->transactionChanges.insertMove(from, from + count, to);
    } else {
        emit itemsMoved(from, to, count);
        if (d->childrenChanged)
            emit childrenChanged();
    }
}

void QSGVisualItemModel::replace(int index, QSGItem *item, QSGVisualItemModel::CachePolicy policy)
{
    Q_D(QSGVisualItemModel);
    const int internalIndex = d->indexOf(item);
    if (internalIndex != -1) {
        int from = d->absoluteIndexOf(internalIndex);
        if (d->merge(from, index)) {
            d->transactionChanges.insertRemove(index, index + 1);
            if (from > index)
                from -= 1;
            else
                index -= 1;
            if (from != index)
                d->transactionChanges.insertMove(from, from + 1, index);

            if (!d->transaction)
                d->emitTransactionChanges();
        }

    } else {
        d->replaceAt(index, item);
    }
}

void QSGVisualItemModel::_q_itemsInserted(int index, int count)
{
    Q_D(QSGVisualItemModel);

    QVector<QDeclarativeChangeSet::Insert> inserts;
    d->childrenChanged = false;
    d->listItemsInserted(static_cast<QSGVisualDataModel *>(sender()), index, index + count, &inserts);

    if (inserts.count() > 0) {
        QScriptEngine *engine = QDeclarativeEnginePrivate::getScriptEngine(qmlEngine(this));
        QScriptValue indexes = engine->newArray(inserts.count());
        for (int i = 0; i < inserts.count(); ++i) {
            QScriptValue range = engine->newObject();
            range.setProperty(QLatin1String("start"), inserts.at(i).start);
            range.setProperty(QLatin1String("end"), inserts.at(i).end);
            indexes.setProperty(i, range);
        }
        d->transaction = true;
        d->transactionChanges.append(inserts);
        emit itemDataInserted(indexes);
        d->transaction = false;

        d->emitTransactionChanges();
        emit countChanged();
        if (d->childrenChanged)
            emit childrenChanged();
    }
}

void QSGVisualItemModel::_q_itemsRemoved(int index, int count)
{
    Q_D(QSGVisualItemModel);

    QVector<QDeclarativeChangeSet::Remove> removes;
    d->childrenChanged = false;
    d->listItemsRemoved(static_cast<QSGVisualDataModel *>(sender()), index, index + count, &removes);
    d->transactionChanges.append(removes);
    d->emitTransactionChanges();
    emit countChanged();
    if (d->childrenChanged)
        emit childrenChanged();
}

void QSGVisualItemModel::_q_itemsMoved(int from, int to, int count)
{
    Q_D(QSGVisualItemModel);
    QVector<QDeclarativeChangeSet::Move> moves;
    d->childrenChanged = false;
    d->listItemsMoved(static_cast<QSGVisualDataModel *>(sender()), from, from + count, to, &moves);
    d->transactionChanges.append(moves);
    d->emitTransactionChanges();
    if (d->childrenChanged)
        emit childrenChanged();
}

QSGVisualItemModelAttached *QSGVisualItemModel::qmlAttachedProperties(QObject *obj)
{
    return QSGVisualItemModelAttached::properties(obj);
}

//============================================================================

class VDMDelegateDataType : public QDeclarativeOpenMetaObjectType
{
public:
    VDMDelegateDataType(const QMetaObject *base, QDeclarativeEngine *engine) : QDeclarativeOpenMetaObjectType(base, engine) {}

    void propertyCreated(int, QMetaPropertyBuilder &prop) {
        prop.setWritable(false);
    }
};

class QSGVisualDataModelParts;
class QSGVisualDataModelData;
class QSGVisualDataModelPrivate : public QObjectPrivate
{
public:
    QSGVisualDataModelPrivate(QDeclarativeContext *);

    static QSGVisualDataModelPrivate *get(QSGVisualDataModel *m) {
        return static_cast<QSGVisualDataModelPrivate *>(QObjectPrivate::get(m));
    }

    QDeclarativeGuard<QListModelInterface> m_listModelInterface;
    QDeclarativeGuard<QAbstractItemModel> m_abstractItemModel;
    QDeclarativeGuard<QSGVisualDataModel> m_visualItemModel;
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
                if (m_listAccessor->type() == QDeclarativeListAccessor::Instance) {
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
    QHash<QObject *, QDeclarativePackage*> m_packaged;

    QSGVisualDataModelParts *m_parts;
    friend class QSGVisualItemParts;

    VDMDelegateDataType *m_delegateDataType;
    friend class QSGVisualDataModelData;
    bool m_metaDataCreated : 1;
    bool m_metaDataCacheable : 1;
    bool m_delegateValidated : 1;
    bool m_completePending : 1;

    QSGVisualDataModelData *data(QObject *item);

    QVariant m_modelVariant;
    QDeclarativeListAccessor *m_listAccessor;

    QModelIndex m_root;
    QList<QByteArray> watchedRoles;
    QList<int> watchedRoleIds;
};

class QSGVisualDataModelDataMetaObject : public QDeclarativeOpenMetaObject
{
public:
    QSGVisualDataModelDataMetaObject(QObject *parent, QDeclarativeOpenMetaObjectType *type)
    : QDeclarativeOpenMetaObject(parent, type) {}

    virtual QVariant initialValue(int);
    virtual int createProperty(const char *, const char *);

private:
    friend class QSGVisualDataModelData;
};

class QSGVisualDataModelData : public QObject
{
Q_OBJECT
public:
    QSGVisualDataModelData(int index, QSGVisualDataModel *model);
    ~QSGVisualDataModelData();

    Q_PROPERTY(int index READ index NOTIFY indexChanged)
    int index() const;
    void setIndex(int index);

    int propForRole(int) const;
    int modelDataPropertyId() const {
        QSGVisualDataModelPrivate *model = QSGVisualDataModelPrivate::get(m_model);
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
    friend class QSGVisualDataModelDataMetaObject;
    int m_index;
    QDeclarativeGuard<QSGVisualDataModel> m_model;
    QSGVisualDataModelDataMetaObject *m_meta;
};

int QSGVisualDataModelData::propForRole(int id) const
{
    QSGVisualDataModelPrivate *model = QSGVisualDataModelPrivate::get(m_model);
    QHash<int,int>::const_iterator it = model->m_roleToPropId.find(id);
    if (it != model->m_roleToPropId.end())
        return *it;

    return -1;
}

void QSGVisualDataModelData::setValue(int id, const QVariant &val)
{
    m_meta->setValue(id, val);
}

int QSGVisualDataModelDataMetaObject::createProperty(const char *name, const char *type)
{
    QSGVisualDataModelData *data =
        static_cast<QSGVisualDataModelData *>(object());

    if (!data->m_model)
        return -1;

    QSGVisualDataModelPrivate *model = QSGVisualDataModelPrivate::get(data->m_model);
    if (data->m_index < 0 || data->m_index >= model->modelCount())
        return -1;

    if ((!model->m_listModelInterface || !model->m_abstractItemModel) && model->m_listAccessor) {
        if (model->m_listAccessor->type() == QDeclarativeListAccessor::ListProperty) {
            model->ensureRoles();
            if (qstrcmp(name,"modelData") == 0)
                return QDeclarativeOpenMetaObject::createProperty(name, type);
        }
    }
    return -1;
}

QVariant QSGVisualDataModelDataMetaObject::initialValue(int propId)
{
    QSGVisualDataModelData *data =
        static_cast<QSGVisualDataModelData *>(object());

    Q_ASSERT(data->m_model);
    QSGVisualDataModelPrivate *model = QSGVisualDataModelPrivate::get(data->m_model);

    QByteArray propName = name(propId);
    if ((!model->m_listModelInterface || !model->m_abstractItemModel) && model->m_listAccessor) {
        if (propName == "modelData") {
            if (model->m_listAccessor->type() == QDeclarativeListAccessor::Instance) {
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

QSGVisualDataModelData::QSGVisualDataModelData(int index,
                                               QSGVisualDataModel *model)
: m_index(index), m_model(model),
m_meta(new QSGVisualDataModelDataMetaObject(this, QSGVisualDataModelPrivate::get(model)->m_delegateDataType))
{
    ensureProperties();
}

QSGVisualDataModelData::~QSGVisualDataModelData()
{
}

void QSGVisualDataModelData::ensureProperties()
{
    QSGVisualDataModelPrivate *modelPriv = QSGVisualDataModelPrivate::get(m_model);
    if (modelPriv->m_metaDataCacheable) {
        if (!modelPriv->m_metaDataCreated)
            modelPriv->createMetaData();
        if (modelPriv->m_metaDataCreated)
            m_meta->setCached(true);
    }
}

int QSGVisualDataModelData::index() const
{
    return m_index;
}

// This is internal only - it should not be set from qml
void QSGVisualDataModelData::setIndex(int index)
{
    m_index = index;
    emit indexChanged();
}


//---------------------------------------------------------------------------

class QSGVisualDataModelPartsMetaObject : public QDeclarativeOpenMetaObject
{
public:
    QSGVisualDataModelPartsMetaObject(QObject *parent)
    : QDeclarativeOpenMetaObject(parent) {}

    virtual void propertyCreated(int, QMetaPropertyBuilder &);
    virtual QVariant initialValue(int);
};

class QSGVisualDataModelParts : public QObject
{
Q_OBJECT
public:
    QSGVisualDataModelParts(QSGVisualDataModel *parent);

private:
    friend class QSGVisualDataModelPartsMetaObject;
    QSGVisualDataModel *model;
};

void QSGVisualDataModelPartsMetaObject::propertyCreated(int, QMetaPropertyBuilder &prop)
{
    prop.setWritable(false);
}

QVariant QSGVisualDataModelPartsMetaObject::initialValue(int id)
{
    QSGVisualDataModel *m = new QSGVisualDataModel;
    m->setParent(object());
    m->setPart(QString::fromUtf8(name(id)));
    m->setModel(QVariant::fromValue(static_cast<QSGVisualDataModelParts *>(object())->model));

    QVariant var = QVariant::fromValue((QObject *)m);
    return var;
}

QSGVisualDataModelParts::QSGVisualDataModelParts(QSGVisualDataModel *parent)
: QObject(parent), model(parent)
{
    new QSGVisualDataModelPartsMetaObject(this);
}

QSGVisualDataModelPrivate::QSGVisualDataModelPrivate(QDeclarativeContext *ctxt)
: m_listModelInterface(0), m_abstractItemModel(0), m_visualItemModel(0), m_delegate(0)
, m_context(ctxt), m_modelDataPropId(-1), m_parts(0), m_delegateDataType(0), m_metaDataCreated(false)
, m_metaDataCacheable(false), m_delegateValidated(false), m_completePending(false), m_listAccessor(0)
{
}

QSGVisualDataModelData *QSGVisualDataModelPrivate::data(QObject *item)
{
    QSGVisualDataModelData *dataItem =
        item->findChild<QSGVisualDataModelData *>();
    Q_ASSERT(dataItem);
    return dataItem;
}

//---------------------------------------------------------------------------

QSGVisualDataModel::QSGVisualDataModel()
: QSGVisualModel(*(new QSGVisualDataModelPrivate(0)))
{
}

QSGVisualDataModel::QSGVisualDataModel(QDeclarativeContext *ctxt, QObject *parent)
: QSGVisualModel(*(new QSGVisualDataModelPrivate(ctxt)), parent)
{
}

QSGVisualDataModel::QSGVisualDataModel(QSGVisualDataModelPrivate &dd, QObject *parent)
: QSGVisualModel(dd, parent)
{
}

QSGVisualDataModel::~QSGVisualDataModel()
{
    Q_D(QSGVisualDataModel);
    if (d->m_listAccessor)
        delete d->m_listAccessor;
    if (d->m_delegateDataType)
        d->m_delegateDataType->release();
}

QVariant QSGVisualDataModel::model() const
{
    Q_D(const QSGVisualDataModel);
    return d->m_modelVariant;
}

void QSGVisualDataModel::setModel(const QVariant &model)
{
    Q_D(QSGVisualDataModel);
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
        QObject::disconnect(d->m_visualItemModel, SIGNAL(createdPackage(int,QDeclarativePackage*)),
                         this, SLOT(_q_createdPackage(int,QDeclarativePackage*)));
        QObject::disconnect(d->m_visualItemModel, SIGNAL(destroyingPackage(QDeclarativePackage*)),
                         this, SLOT(_q_destroyingPackage(QDeclarativePackage*)));
        d->m_visualItemModel = 0;
    }

    d->m_roles.clear();
    d->m_roleNames.clear();
    if (d->m_delegateDataType)
        d->m_delegateDataType->release();
    d->m_metaDataCreated = 0;
    d->m_metaDataCacheable = false;
    d->m_delegateDataType = new VDMDelegateDataType(&QSGVisualDataModelData::staticMetaObject, d->m_context?d->m_context->engine():qmlEngine(this));

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
    if ((d->m_visualItemModel = qvariant_cast<QSGVisualDataModel *>(model))) {
        QObject::connect(d->m_visualItemModel, SIGNAL(countChanged()),
                         this, SIGNAL(countChanged()));
        QObject::connect(d->m_visualItemModel, SIGNAL(itemsInserted(int,int)),
                         this, SIGNAL(itemsInserted(int,int)));
        QObject::connect(d->m_visualItemModel, SIGNAL(itemsRemoved(int,int)),
                         this, SIGNAL(itemsRemoved(int,int)));
        QObject::connect(d->m_visualItemModel, SIGNAL(itemsMoved(int,int,int)),
                         this, SIGNAL(itemsMoved(int,int,int)));
        QObject::connect(d->m_visualItemModel, SIGNAL(createdPackage(int,QDeclarativePackage*)),
                         this, SLOT(_q_createdPackage(int,QDeclarativePackage*)));
        QObject::connect(d->m_visualItemModel, SIGNAL(destroyingPackage(QDeclarativePackage*)),
                         this, SLOT(_q_destroyingPackage(QDeclarativePackage*)));
        return;
    }
    d->m_listAccessor = new QDeclarativeListAccessor;
    d->m_listAccessor->setList(model, d->m_context?d->m_context->engine():qmlEngine(this));
    if (d->m_listAccessor->type() != QDeclarativeListAccessor::ListProperty)
        d->m_metaDataCacheable = true;
    if (d->m_delegate && d->modelCount()) {
        emit itemsInserted(0, d->modelCount());
        emit countChanged();
    }
}

QDeclarativeComponent *QSGVisualDataModel::delegate() const
{
    Q_D(const QSGVisualDataModel);
    if (d->m_visualItemModel)
        return d->m_visualItemModel->delegate();
    return d->m_delegate;
}

void QSGVisualDataModel::setDelegate(QDeclarativeComponent *delegate)
{
    Q_D(QSGVisualDataModel);
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

QVariant QSGVisualDataModel::rootIndex() const
{
    Q_D(const QSGVisualDataModel);
    return QVariant::fromValue(d->m_root);
}

void QSGVisualDataModel::setRootIndex(const QVariant &root)
{
    Q_D(QSGVisualDataModel);
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

QVariant QSGVisualDataModel::modelIndex(int idx) const
{
    Q_D(const QSGVisualDataModel);
    if (d->m_abstractItemModel)
        return QVariant::fromValue(d->m_abstractItemModel->index(idx, 0, d->m_root));
    return QVariant::fromValue(QModelIndex());
}

QVariant QSGVisualDataModel::parentModelIndex() const
{
    Q_D(const QSGVisualDataModel);
    if (d->m_abstractItemModel)
        return QVariant::fromValue(d->m_abstractItemModel->parent(d->m_root));
    return QVariant::fromValue(QModelIndex());
}

QString QSGVisualDataModel::part() const
{
    Q_D(const QSGVisualDataModel);
    return d->m_part;
}

void QSGVisualDataModel::setPart(const QString &part)
{
    Q_D(QSGVisualDataModel);
    d->m_part = part;
}

int QSGVisualDataModel::count() const
{
    Q_D(const QSGVisualDataModel);
    if (d->m_visualItemModel)
        return d->m_visualItemModel->count();
    if (!d->m_delegate)
        return 0;
    return d->modelCount();
}

QSGItem *QSGVisualDataModel::item(int index, bool complete)
{
    Q_D(QSGVisualDataModel);
    if (d->m_visualItemModel)
        return d->m_visualItemModel->item(index, d->m_part.toUtf8(), complete);
    return item(index, QByteArray(), complete);
}

/*
  Returns ReleaseStatus flags.
*/
QSGVisualDataModel::ReleaseFlags QSGVisualDataModel::release(QSGItem *item)
{
    Q_D(QSGVisualDataModel);
    if (d->m_visualItemModel)
        return d->m_visualItemModel->release(item);

    ReleaseFlags stat = 0;
    QObject *obj = item;
    bool inPackage = false;

    QHash<QObject*,QDeclarativePackage*>::iterator it = d->m_packaged.find(item);
    if (it != d->m_packaged.end()) {
        QDeclarativePackage *package = *it;
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
            emit destroyingPackage(qobject_cast<QDeclarativePackage*>(obj));
        } else {
            // XXX todo - the original did item->scene()->removeItem().  Why?
            item->setParentItem(0);
        }
        stat |= Destroyed;
        obj->deleteLater();
    } else if (!inPackage) {
        stat |= Referenced;
    }

    return stat;
}

QObject *QSGVisualDataModel::parts()
{
    Q_D(QSGVisualDataModel);
    if (!d->m_parts)
        d->m_parts = new QSGVisualDataModelParts(this);
    return d->m_parts;
}

QSGItem *QSGVisualDataModel::item(int index, const QByteArray &viewId, bool complete)
{
    Q_D(QSGVisualDataModel);
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
        QSGVisualDataModelData *data = new QSGVisualDataModelData(index, this);
        if ((!d->m_listModelInterface || !d->m_abstractItemModel) && d->m_listAccessor
            && d->m_listAccessor->type() == QDeclarativeListAccessor::ListProperty) {
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
            if (QDeclarativePackage *package = qobject_cast<QDeclarativePackage *>(nobj))
                emit createdPackage(index, package);
        } else {
            delete data;
            delete ctxt;
            qmlInfo(this, d->m_delegate->errors()) << "Error creating delegate";
        }
    }
    QSGItem *item = qobject_cast<QSGItem *>(nobj);
    if (!item) {
        QDeclarativePackage *package = qobject_cast<QDeclarativePackage *>(nobj);
        if (package) {
            QObject *o = package->part(QString::fromUtf8(viewId));
            item = qobject_cast<QSGItem *>(o);
            if (item)
                d->m_packaged.insertMulti(item, package);
        }
    }
    if (!item) {
        if (needComplete)
            d->m_delegate->completeCreate();
        d->m_cache.releaseItem(nobj);
        if (!d->m_delegateValidated) {
            qmlInfo(d->m_delegate) << QSGVisualDataModel::tr("Delegate component must be Item type.");
            d->m_delegateValidated = true;
        }
    }
    if (d->modelCount()-1 == index && d->m_abstractItemModel && d->m_abstractItemModel->canFetchMore(d->m_root))
        d->m_abstractItemModel->fetchMore(d->m_root);

    return item;
}

bool QSGVisualDataModel::completePending() const
{
    Q_D(const QSGVisualDataModel);
    if (d->m_visualItemModel)
        return d->m_visualItemModel->completePending();
    return d->m_completePending;
}

void QSGVisualDataModel::completeItem()
{
    Q_D(QSGVisualDataModel);
    if (d->m_visualItemModel) {
        d->m_visualItemModel->completeItem();
        return;
    }

    d->m_delegate->completeCreate();
    d->m_completePending = false;
}

QString QSGVisualDataModel::stringValue(int index, const QString &name)
{
    Q_D(QSGVisualDataModel);
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
        data = new QSGVisualDataModelData(index, this);
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

int QSGVisualDataModel::indexOf(QSGItem *item, QObject *) const
{
    QVariant val = QDeclarativeEngine::contextForObject(item)->contextProperty(QLatin1String("index"));
        return val.toInt();
    return -1;
}

void QSGVisualDataModel::setWatchedRoles(QList<QByteArray> roles)
{
    Q_D(QSGVisualDataModel);
    d->watchedRoles = roles;
    d->watchedRoleIds.clear();
}

void QSGVisualDataModel::_q_itemsChanged(int index, int count,
                                         const QList<int> &roles)
{
    Q_D(QSGVisualDataModel);
    bool changed = false;
    if (!d->watchedRoles.isEmpty() && d->watchedRoleIds.isEmpty()) {
        foreach (QByteArray r, d->watchedRoles) {
            if (d->m_roleNames.contains(r))
                d->watchedRoleIds << d->m_roleNames.value(r);
        }
    }

    for (QHash<int,QSGVisualDataModelPrivate::ObjectRef>::ConstIterator iter = d->m_cache.begin();
        iter != d->m_cache.end(); ++iter) {
        const int idx = iter.key();

        if (idx >= index && idx < index+count) {
            QSGVisualDataModelPrivate::ObjectRef objRef = *iter;
            QSGVisualDataModelData *data = d->data(objRef.obj);
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

void QSGVisualDataModel::insertItems(int index, const QList<QSGItem *> &items)
{
    Q_D(QSGVisualDataModel);
    for (int i=0; i<items.count(); i++) {
        d->m_cache.insertItem(index + i, items[i]);
        QSGVisualDataModelData *data = d->data(items[i]);
        data->setIndex(index + i);
    }
}

QSGItem *QSGVisualDataModel::takeItem(int index)
{
    Q_D(QSGVisualDataModel);
    QSGItem *item = 0;
    if (d->m_cache.contains(index)) {
        item = qobject_cast<QSGItem *>(d->m_cache[index].obj);
        d->m_cache.remove(index);
    }
    return item;
}

void QSGVisualDataModel::_q_itemsInserted(int index, int count)
{
    Q_D(QSGVisualDataModel);
    if (!count)
        return;
    // XXX - highly inefficient
    QHash<int,QSGVisualDataModelPrivate::ObjectRef> items;
    for (QHash<int,QSGVisualDataModelPrivate::ObjectRef>::Iterator iter = d->m_cache.begin();
        iter != d->m_cache.end(); ) {

        if (iter.key() >= index) {
            QSGVisualDataModelPrivate::ObjectRef objRef = *iter;
            int index = iter.key() + count;
            iter = d->m_cache.erase(iter);

            items.insert(index, objRef);

            QSGVisualDataModelData *data = d->data(objRef.obj);
            data->setIndex(index);
        } else {
            ++iter;
        }
    }
    d->m_cache.unite(items);

    insertChangeComplete(index);

    emit itemsInserted(index, count);
    emit countChanged();
}

void QSGVisualDataModel::_q_itemsRemoved(int index, int count)
{
    Q_D(QSGVisualDataModel);
    if (!count)
        return;
    // XXX - highly inefficient
    QHash<int, QSGVisualDataModelPrivate::ObjectRef> items;
    for (QHash<int, QSGVisualDataModelPrivate::ObjectRef>::Iterator iter = d->m_cache.begin();
        iter != d->m_cache.end(); ) {
        if (iter.key() >= index && iter.key() < index + count) {
            QSGVisualDataModelPrivate::ObjectRef objRef = *iter;
            iter = d->m_cache.erase(iter);
            items.insertMulti(-1, objRef); //XXX perhaps better to maintain separately
            QSGVisualDataModelData *data = d->data(objRef.obj);
            data->setIndex(-1);
        } else if (iter.key() >= index + count) {
            QSGVisualDataModelPrivate::ObjectRef objRef = *iter;
            int index = iter.key() - count;
            iter = d->m_cache.erase(iter);
            items.insert(index, objRef);
            QSGVisualDataModelData *data = d->data(objRef.obj);
            data->setIndex(index);
        } else {
            ++iter;
        }
    }

    d->m_cache.unite(items);
    emit itemsRemoved(index, count);
    emit countChanged();
}

void QSGVisualDataModel::_q_itemsMoved(int from, int to, int count)
{
    Q_D(QSGVisualDataModel);
    // XXX - highly inefficient
    QHash<int,QSGVisualDataModelPrivate::ObjectRef> items;
    for (QHash<int,QSGVisualDataModelPrivate::ObjectRef>::Iterator iter = d->m_cache.begin();
        iter != d->m_cache.end(); ) {

        if (iter.key() >= from && iter.key() < from + count) {
            QSGVisualDataModelPrivate::ObjectRef objRef = *iter;
            int index = iter.key() - from + to;
            iter = d->m_cache.erase(iter);

            items.insert(index, objRef);

            QSGVisualDataModelData *data = d->data(objRef.obj);
            data->setIndex(index);
        } else {
            ++iter;
        }
    }
    for (QHash<int,QSGVisualDataModelPrivate::ObjectRef>::Iterator iter = d->m_cache.begin();
        iter != d->m_cache.end(); ) {

        int diff = from > to ? count : -count;
        if (iter.key() >= qMin(from,to) && iter.key() < qMax(from+count,to+count)) {
            QSGVisualDataModelPrivate::ObjectRef objRef = *iter;
            int index = iter.key() + diff;
            iter = d->m_cache.erase(iter);

            items.insert(index, objRef);

            QSGVisualDataModelData *data = d->data(objRef.obj);
            data->setIndex(index);
        } else {
            ++iter;
        }
    }
    d->m_cache.unite(items);

    emit itemsMoved(from, to, count);
}

void QSGVisualDataModel::_q_rowsInserted(const QModelIndex &parent, int begin, int end)
{
    Q_D(QSGVisualDataModel);
    if (parent == d->m_root)
        _q_itemsInserted(begin, end - begin + 1);
}

void QSGVisualDataModel::_q_rowsRemoved(const QModelIndex &parent, int begin, int end)
{
    Q_D(QSGVisualDataModel);
    if (parent == d->m_root)
        _q_itemsRemoved(begin, end - begin + 1);
}

void QSGVisualDataModel::_q_rowsMoved(const QModelIndex &sourceParent, int sourceStart, int sourceEnd, const QModelIndex &destinationParent, int destinationRow)
{
    Q_D(QSGVisualDataModel);
    const int count = sourceEnd - sourceStart + 1;
    if (destinationParent == d->m_root && sourceParent == d->m_root) {
        _q_itemsMoved(sourceStart, sourceStart > destinationRow ? destinationRow : destinationRow-1, count);
    } else if (sourceParent == d->m_root) {
        _q_itemsRemoved(sourceStart, count);
    } else if (destinationParent == d->m_root) {
        _q_itemsInserted(destinationRow, count);
    }
}

void QSGVisualDataModel::_q_dataChanged(const QModelIndex &begin, const QModelIndex &end)
{
    Q_D(QSGVisualDataModel);
    if (begin.parent() == d->m_root)
        _q_itemsChanged(begin.row(), end.row() - begin.row() + 1, d->m_roles);
}

void QSGVisualDataModel::_q_layoutChanged()
{
    Q_D(QSGVisualDataModel);
    _q_itemsChanged(0, count(), d->m_roles);
}

void QSGVisualDataModel::_q_modelReset()
{
    Q_D(QSGVisualDataModel);
    d->m_root = QModelIndex();
    emit modelReset();
    emit rootIndexChanged();
    if (d->m_abstractItemModel && d->m_abstractItemModel->canFetchMore(d->m_root))
        d->m_abstractItemModel->fetchMore(d->m_root);
}

void QSGVisualDataModel::_q_createdPackage(int index, QDeclarativePackage *package)
{
    Q_D(QSGVisualDataModel);
    emit createdItem(index, qobject_cast<QSGItem*>(package->part(d->m_part)));
}

void QSGVisualDataModel::_q_destroyingPackage(QDeclarativePackage *package)
{
    Q_D(QSGVisualDataModel);
    emit destroyingItem(qobject_cast<QSGItem*>(package->part(d->m_part)));
}


//============================================================================

class VisualListModelList : public QListModelInterface
{
    Q_OBJECT
public:
    VisualListModelList(QObject *parent = 0);

    virtual int count() const;
    virtual QVariant data(int index, int role) const;
    virtual QList<int> roles() const;
    virtual QString toString(int role) const;

    void setModel(const QVariant &model);

    void inserted(int index, const QList<QHash<int, QVariant> > &values);
    void moved(int from, int to, int count);
    void removed(int index, int count);

    bool contains(int index) const;
    void dump(QSGVisualListModel *);

    friend class QSGVisualListModel;
    QListModelInterface *m_model;
    QVariant m_modelValue;

private slots:
    void _q_itemsChanged(int, int, const QList<int> &);
    void _q_itemsInserted(int index, int count);
    void _q_itemsRemoved(int index, int count);
    void _q_itemsMoved(int from, int to, int count);

private:
    QHash<int, QVariant> data(int index) const;

    struct IndexData {
        int index;
        QHash<int, QVariant> values;
    };

    QHash<int, IndexData> m_indexes;
};


VisualListModelList::VisualListModelList(QObject *parent)
    : QListModelInterface(parent),
      m_model(0)
{
}

int VisualListModelList::count() const
{
    return m_indexes.count();
}

QVariant VisualListModelList::data(int index, int role) const
{
    if (!m_indexes.contains(index)) {
        qWarning() << "VisualListModelList::data(): bad index!" << index;
        return QVariant();
    }
    if (m_indexes[index].index >= 0 && m_model)
        return m_model->data(m_indexes[index].index, role);
    return m_indexes[index].values.value(role);
}

QHash<int, QVariant> VisualListModelList::data(int index) const
{
    if (!m_indexes.contains(index)) {
        qWarning() << "VisualListModelList::data(): bad index!" << index;
        return QHash<int, QVariant>();
    }
    if (m_indexes[index].index >= 0 && m_model) {
        QHash<int, QVariant> values;
        QList<int> roles = m_model->roles();
        for (int i=0; i<roles.count(); i++)
            values.insert(roles[i], m_model->data(m_indexes[index].index, roles[i]));
        return values;
    }
    return m_indexes[index].values;
}

QList<int> VisualListModelList::roles() const
{
    return m_model->roles();
}

QString VisualListModelList::toString(int role) const
{
    return m_model->toString(role);
}

void VisualListModelList::setModel(const QVariant &model)
{
    if (model == m_modelValue)
        return;
    QListModelInterface *newModel = qobject_cast<QListModelInterface *>(qvariant_cast<QObject *>(model));
    if (!newModel) {
        // XXX should use the models adaptor object from models2 research to accept all model types
        qmlInfo(this) << "setModel: model object is not valid";
        return;
    }

    if (m_model) {
        QObject::disconnect(m_model, SIGNAL(itemsChanged(int,int,QList<int>)),
                this, SLOT(_q_itemsChanged(int,int,QList<int>)));
        QObject::disconnect(m_model, SIGNAL(itemsInserted(int,int)),
                this, SLOT(_q_itemsInserted(int,int)));
        QObject::disconnect(m_model, SIGNAL(itemsRemoved(int,int)),
                this, SLOT(_q_itemsRemoved(int,int)));
        QObject::disconnect(m_model, SIGNAL(itemsMoved(int,int,int)),
                this, SLOT(_q_itemsMoved(int,int,int)));
    }

    m_indexes.clear();
    m_modelValue = model;
    m_model = newModel;
    for (int i=0; i<m_model->count(); i++) {
        IndexData data = { i, QHash<int, QVariant>() };
        m_indexes.insert(i, data);
    }

    QObject::connect(m_model, SIGNAL(itemsChanged(int,int,QList<int>)),
                     this, SLOT(_q_itemsChanged(int,int,QList<int>)));
    QObject::connect(m_model, SIGNAL(itemsInserted(int,int)),
                     this, SLOT(_q_itemsInserted(int,int)));
    QObject::connect(m_model, SIGNAL(itemsRemoved(int,int)),
                     this, SLOT(_q_itemsRemoved(int,int)));
    QObject::connect(m_model, SIGNAL(itemsMoved(int,int,int)),
                     this, SLOT(_q_itemsMoved(int,int,int)));
}

void VisualListModelList::inserted(int index, const QList<QHash<int, QVariant> > &values)
{
    if (values.isEmpty())
        return;
    QHash<int, IndexData> items;
    for (QHash<int, IndexData>::Iterator iter = m_indexes.begin(); iter != m_indexes.end();) {
        if (iter.key() >= index) {
            IndexData data = *iter;
            int newIndex = iter.key() + values.count();
            items.insert(newIndex, data);
            iter = m_indexes.erase(iter);
        } else {
            ++iter;
        }
    }
    for (int i=0; i<values.count(); i++) {
        IndexData data = { -1, values[i] };
        items.insert(index + i, data);
    }
    m_indexes.unite(items);
    emit itemsInserted(index, values.count());
}

void VisualListModelList::moved(int from, int to, int count)
{
    QHash<int, IndexData> items;
    for (QHash<int, IndexData>::Iterator iter = m_indexes.begin(); iter != m_indexes.end(); ) {
        if (iter.key() >= from && iter.key() < from + count) {
            IndexData data = *iter;
            int newIndex = iter.key() - from + to;
            items.insert(newIndex, data);
            iter = m_indexes.erase(iter);
        } else {
            ++iter;
        }
    }
    for (QHash<int, IndexData>::Iterator iter = m_indexes.begin(); iter != m_indexes.end(); ) {
        int diff = from > to ? count : -count;
        if (iter.key() >= qMin(from,to) && iter.key() < qMax(from+count,to+count)) {
            IndexData data = *iter;
            int newIndex = iter.key() + diff;
            items.insert(newIndex, data);
            iter = m_indexes.erase(iter);
        } else {
            ++iter;
        }
    }
    m_indexes.unite(items);
    emit itemsMoved(from, to, count);
}

void VisualListModelList::removed(int index, int count)
{
    if (count < 1)
        return;
    QHash<int, IndexData> items;
    for (QHash<int, IndexData>::Iterator iter = m_indexes.begin(); iter != m_indexes.end(); ) {
        if (iter.key() >= index && iter.key() < index + count) {
            iter = m_indexes.erase(iter);
        } else if (iter.key() >= index + count) {
            IndexData data = *iter;
            int newIndex = iter.key() - count;
            items.insert(newIndex, data);
            iter = m_indexes.erase(iter);
        } else {
            ++iter;
        }
    }
    m_indexes.unite(items);
    emit itemsRemoved(index, count);
}

bool VisualListModelList::contains(int index) const
{
    return m_indexes.contains(index);
}

void VisualListModelList::dump(QSGVisualListModel *visualListModel)
{
    qDebug() << "\tKeys:" << m_indexes.keys();
    qDebug() << "\tCount:" << m_indexes.count();
    for (QHash<int, IndexData>::Iterator it = m_indexes.begin(); it != m_indexes.end(); ++it) {
        IndexData data = *it;
        qDebug() << "\t" << it.key() << ":" << data.index << data.values << visualListModel->item(it.key());
    }
}

void VisualListModelList::_q_itemsChanged(int index, int count, const QList<int> &roles)
{
    // XXX this just emits itemsChanged() for all visual items in between the real items, regardless
    // of whether they have changed
    // Once we have the new model classes this method can be removed

    int visualModelFrom = -1;
    int visualModelTo = -1;

    for (QHash<int, IndexData>::Iterator iter = m_indexes.begin(); iter != m_indexes.end(); ) {
        if ((*iter).index == index)
            visualModelFrom = iter.key();
        else if ((*iter).index == index + count - 1)
            visualModelTo = iter.key();
        if (visualModelFrom >= 0 && visualModelTo >= 0)
            break;
    }

    emit itemsChanged(visualModelFrom, visualModelTo - visualModelFrom + 1, roles);
}

void VisualListModelList::_q_itemsInserted(int listModelIndex, int count)
{
    if (listModelIndex < 0 || count < 1)
        return;
    int visualModelIndex = -1;

    // update listModelIndex for items that point to actual model indexes
    for (QHash<int, IndexData>::Iterator iter = m_indexes.begin(); iter != m_indexes.end(); ++iter) {
        IndexData &data = *iter;
        if (data.index == listModelIndex) {
            visualModelIndex = iter.key();
            data.index += count;
        } else if (data.index >= listModelIndex + count) {
            data.index += count;
        }
    }

    QHash<int, IndexData> items;
    if (visualModelIndex >= 0) {
        // update index key for all items that moved
        for (QHash<int, IndexData>::Iterator iter = m_indexes.begin(); iter != m_indexes.end(); ) {
            if (iter.key() >= visualModelIndex) {
                IndexData data = *iter;
                int index = iter.key() + count;
                items.insert(index, data);
                iter = m_indexes.erase(iter);
            } else {
                ++iter;
            }
        }
    } else {
        // there are no items that refer to actual model items, so add the new items to the end
        visualModelIndex = m_indexes.count();
    }
    for (int i=0; i<count; i++) {
        IndexData data = { listModelIndex + i, QHash<int, QVariant>() };
        items.insert(visualModelIndex + i, data);
    }

    m_indexes.unite(items);
    emit itemsInserted(visualModelIndex, count);
}

void VisualListModelList::_q_itemsRemoved(int listModelIndex, int count)
{
    if (count < 1)
        return;
    int visualModelIndex = -1;

    // update listModelIndex for items that point to actual model indexes
    for (QHash<int, IndexData>::Iterator iter = m_indexes.begin(); iter != m_indexes.end(); ) {
        IndexData &data = *iter;
        if (data.index == listModelIndex) {
            visualModelIndex = iter.key();
            iter = m_indexes.erase(iter);
        } else if (data.index >= listModelIndex + count) {
            data.index -= count;
            ++iter;
        } else {
            ++iter;
        }
    }

    if (visualModelIndex >= 0) {
        // update index key for all items that moved
        QHash<int, IndexData> items;
        for (QHash<int, IndexData>::Iterator iter = m_indexes.begin(); iter != m_indexes.end(); ) {
            if (iter.key() >= visualModelIndex) {
                IndexData data = *iter;
                int index = iter.key() - count;
                items.insert(index, data);
                iter = m_indexes.erase(iter);
            } else {
                ++iter;
            }
        }
        m_indexes.unite(items);

        // as for _q_itemsInserted(), don't need to insert visual list model itemsRemoved()
        emit itemsRemoved(visualModelIndex, count);
    }
}

void VisualListModelList::_q_itemsMoved(int from, int to, int count)
{
    if (count < 1 || from == to || from < 0 || to < 0)
        return;
    int visualModelFrom = -1;
    int visualModelTo = -1;
    QSet<int> visualItemsInBetween;
    QSet<int> otherShiftedVisualItems;

    // find the matching visual model from & to indexes
    // and update the actual list model index for moved items
    QHash<int, IndexData> items;
    for (QHash<int, IndexData>::Iterator iter = m_indexes.begin(); iter != m_indexes.end(); ) {
        IndexData data = *iter;
        int listModelIndex = data.index;

        if (listModelIndex == from)
            visualModelFrom = iter.key();
        else if (listModelIndex == to)
            visualModelTo = iter.key();

        if (listModelIndex >= 0 && listModelIndex >= from && listModelIndex < from + count) {
            data.index = listModelIndex - from + to;
            items.insert(iter.key(), data);
            iter = m_indexes.erase(iter);
        } else {
            ++iter;
        }
    }

    // record the visual model indexes that are affected by the move
    // and update the actual list model index for other indexes affected by move
    for (QHash<int, IndexData>::Iterator iter = m_indexes.begin(); iter != m_indexes.end(); ) {
        int index = iter.key();
        IndexData data = *iter;
        int listModelIndex = data.index;

        if (listModelIndex < 0 && index > visualModelFrom && iter.key() < visualModelTo)
            visualItemsInBetween.insert(index);
        else if (listModelIndex < 0 && index >= qMin(visualModelFrom,visualModelTo)
                && index < qMax(visualModelFrom+count,visualModelTo+count))
            otherShiftedVisualItems.insert(index);

        int diff = from > to ? count : -count;
        if (listModelIndex >= 0 && listModelIndex >= qMin(from,to) && listModelIndex < qMax(from+count,to+count)) {
            data.index = listModelIndex + diff;
            items.insert(index, data);
            iter = m_indexes.erase(iter);
        } else {
            ++iter;
        }
    }
    m_indexes.unite(items);

    count += visualItemsInBetween.count();
    if (from < to)
        visualModelTo = visualModelTo + otherShiftedVisualItems.count() - visualItemsInBetween.count();

    // update the visual model index for all items affected by the move
    items.clear();
    for (QHash<int, IndexData>::Iterator iter = m_indexes.begin(); iter != m_indexes.end(); ) {
        if (iter.key() >= visualModelFrom && iter.key() < visualModelFrom + count) {
            int newIndex = iter.key() - visualModelFrom + visualModelTo;
            items.insert(newIndex, *iter);
            iter = m_indexes.erase(iter);
        } else {
            ++iter;
        }
    }
    for (QHash<int, IndexData>::Iterator iter = m_indexes.begin(); iter != m_indexes.end(); ) {
        int diff = from > to ? count : -count;
        if (iter.key() >= qMin(visualModelFrom,visualModelTo)
                && iter.key() < qMax(visualModelFrom+count,visualModelTo+count)) {
            int newIndex = iter.key() + diff;
            items.insert(newIndex, *iter);
            iter = m_indexes.erase(iter);
        } else {
            ++iter;
        }
    }
    m_indexes.unite(items);

    if (visualModelFrom >= 0 && visualModelTo >= 0)
        emit itemsMoved(visualModelFrom, visualModelTo, count);
}



class QSGVisualListModelPrivate : public QSGVisualDataModelPrivate
{
    Q_DECLARE_PUBLIC(QSGVisualListModel)
public:
    QSGVisualListModelPrivate()
        : QSGVisualDataModelPrivate(0)
    {
        wrapperModel = new VisualListModelList;
    }

    ~QSGVisualListModelPrivate() {
        delete wrapperModel;
    }

    VisualListModelList *wrapperModel;
    QList<QSGItem *> itemsToTransfer;
};


QSGVisualListModel::QSGVisualListModel(QObject *parent)
    : QSGVisualDataModel(*(new QSGVisualListModelPrivate), parent)
{
}

QSGVisualListModel::~QSGVisualListModel()
{
}

void QSGVisualListModel::setModel(const QVariant &model)
{
    Q_D(QSGVisualListModel);
    d->wrapperModel->setModel(model);

    QSGVisualDataModel::setModel(QVariant::fromValue(static_cast<QObject*>(d->wrapperModel)));
}

QVariant QSGVisualListModel::model() const
{
    Q_D(const QSGVisualListModel);
    return d->wrapperModel->m_modelValue;
}

int QSGVisualListModel::count() const
{
    Q_D(const QSGVisualListModel);
    return d->wrapperModel->count();
}

QSGItem *QSGVisualListModel::get(int index)
{
    return item(index);
}

void QSGVisualListModel::remove(int index)
{
    Q_D(QSGVisualListModel);

    QSGItem *item = takeItem(index);

    if (item) {
        d->wrapperModel->removed(index, 1); // causes parent VisualDataModel to emit itemsRemoved()
        item->setParentItem(0);
        QDeclarative_setParent_noEvent(item, 0);
        item->deleteLater();
    }
}

void QSGVisualListModel::move(int from, int to, int count)
{
    Q_D(QSGVisualListModel);
    if (from < 0 || count < 0 || from == to)
        return;

    d->wrapperModel->moved(from, to, count);  // causes parent VisualDataModel to emit itemsMoved()
}

// XXX remove newParent arg, transfer ownership via signals
void QSGVisualListModel::transfer(int fromIndex, int count, QSGVisualListModel *dest, int toIndex, QSGItem *newParent)
{
    QList<int> srcIndexes;
    QList<int> destIndexes;
    for (int i=0; i<count; i++) {
        srcIndexes << fromIndex + i;
        destIndexes << toIndex + i;
    }

    transfer(srcIndexes, dest, destIndexes, newParent);
}

// XXX remove newParent arg, transfer ownership via signals
void QSGVisualListModel::transfer(const QVariantList &sourceIndexes, QSGVisualListModel *dest, const QVariantList &destIndexes, QSGItem *newParent)
{
    QList<int> sourceInts;
    QList<int> destInts;
    bool ok = false;
    for (int i=0; i<sourceIndexes.count(); i++) {
        int value = sourceIndexes[i].toInt(&ok);
        if (!ok) {
            qmlInfo(this) << "transfer: sourceIndexes contains non-integer values";
            return;
        }
        sourceInts << value;
    }
    for (int i=0; i<destIndexes.count(); i++) {
        int value = destIndexes[i].toInt(&ok);
        if (!ok) {
            qmlInfo(this) << "transfer: destIndexes contains non-integer values";
            return;
        }
        destInts << value;
    }

    transfer(sourceInts, dest, destInts, newParent);
}

void QSGVisualListModel::transfer(const QList<int> &sourceIndexes, QSGVisualListModel *dest, const QList<int> &destIndexes, QSGItem *newParent)
{
    if (!newParent) {
        qmlInfo(this) << "transfer: invalid newParent argument";
        return;
    }

    if (this == dest) {
        qmlInfo(this) << "transfer: source and destination models are the same";
        return;
    }

    if (sourceIndexes.isEmpty() || destIndexes.isEmpty() || sourceIndexes.count() != destIndexes.count()) {
        qmlInfo(this) << "transfer: source or destination index lists are empty or differ in size";
        return;
    }

    Q_D(QSGVisualListModel);
    QList<int> updatedSrcIndexes = sourceIndexes;
    QList<QSGVisualItemData> itemData;
    for (int i=0; i<updatedSrcIndexes.count();) {
        int removeCount = 0;
        int fromIndex = updatedSrcIndexes[i];
        int currIndex = 0;
        do {
            currIndex = updatedSrcIndexes[i];
            QSGItem *item = QSGVisualDataModel::takeItem(currIndex);
            if (!item)
                qmlInfo(this) << "transfer: no item found at source model index" << currIndex;
            QHash<int, QVariant> values = d->wrapperModel->data(currIndex);
            itemData.append(qMakePair(item, values));
            removeCount++;
            i++;
        } while (i < updatedSrcIndexes.count() && updatedSrcIndexes[i] == currIndex + 1);

        // update wrapper model for all removed indexes
        // This causes parent VisualDataModel to emit itemsRemoved(). Ideally when model classes
        // get the bulk changes feature, all these remove changes would be done together instead
        // of once each loop
        d->wrapperModel->removed(fromIndex, removeCount);

        // update indexes shifted by the removal
        for (int j=i; j<updatedSrcIndexes.count(); j++) {
            if (updatedSrcIndexes[j] > currIndex)
                updatedSrcIndexes[j] -= removeCount;
        }
    }

    dest->insertItems(itemData, destIndexes, newParent);
}

void QSGVisualListModel::insertItems(const QList<QSGVisualItemData> &itemData, const QList<int> &destIndexes, QSGItem *newParent)
{
    Q_D(QSGVisualListModel);
    Q_ASSERT(!destIndexes.isEmpty() && destIndexes.count() == itemData.count());

    if (itemData.isEmpty())
        return;

    QList<QSGItem *> items;
    QList<QHash<int, QVariant> > values;

    for (int i=0; i<destIndexes.count();) {
        int toIndex = destIndexes[i];
        int currIndex = 0;
        do {
            currIndex = destIndexes[i];

            QSGItem *item = itemData[i].first;
            if (item) {
                // XXX also need to set a mapped transform?
                QPointF mappedPos = newParent->mapFromItem(item->parentItem(), item->pos());
                item->setPos(mappedPos);
                item->setParentItem(newParent);
                QDeclarative_setParent_noEvent(item, this);

                items << item;
                values << itemData[i].second;
            }
            i++;
        } while (i < destIndexes.count() && destIndexes[i] == currIndex + 1);

        d->itemsToTransfer = items;
        d->wrapperModel->inserted(toIndex, values);
        d->itemsToTransfer.clear();

        items.clear();
        values.clear();
    }
}

QSGVisualModel::ReleaseFlags QSGVisualListModel::release(QSGItem *item)
{
    // this does not mean the index can be removed from d->m_indexes because
    // the index may be requested again later
    return QSGVisualDataModel::release(item);
}

void QSGVisualListModel::insertChangeComplete(int index)
{
    Q_D(QSGVisualListModel);
    if (!d->itemsToTransfer.isEmpty())
        QSGVisualDataModel::insertItems(index, d->itemsToTransfer);
}

QT_END_NAMESPACE

QML_DECLARE_TYPE(QListModelInterface)

#include <qsgvisualitemmodel.moc>
