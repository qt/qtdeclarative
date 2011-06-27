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

QHash<QObject*, QSGVisualModelAttached*> QSGVisualModelAttached::attachedProperties;


class QSGVisualModelPartsMetaObject : public QDeclarativeOpenMetaObject
{
public:
    QSGVisualModelPartsMetaObject(QObject *parent)
    : QDeclarativeOpenMetaObject(parent) {}

    virtual void propertyCreated(int, QMetaPropertyBuilder &);
    virtual QVariant initialValue(int);
};

class QSGVisualModelParts : public QObject
{
Q_OBJECT
public:
    QSGVisualModelParts(QSGVisualModel *parent);

private:
    friend class QSGVisualModelPartsMetaObject;
    QSGVisualModel *model;
};

void QSGVisualModelPartsMetaObject::propertyCreated(int, QMetaPropertyBuilder &prop)
{
    prop.setWritable(false);
}

QVariant QSGVisualModelPartsMetaObject::initialValue(int id)
{
    QSGVisualPartModel *m = new QSGVisualPartModel(
            static_cast<QSGVisualModelParts *>(object())->model, name(id), object());

    QVariant var = QVariant::fromValue(static_cast<QObject *>(m));
    return var;
}

QSGVisualModelParts::QSGVisualModelParts(QSGVisualModel *parent)
: QObject(parent), model(parent)
{
    new QSGVisualModelPartsMetaObject(this);
}


class QSGVisualModelPrivate : public QObjectPrivate, public QDeclarativeListCompositor
{
    Q_DECLARE_PUBLIC(QSGVisualModel)
public:
    QSGVisualModelPrivate(QDeclarativeContext *context = 0)
        : QObjectPrivate(), QDeclarativeListCompositor(0),context(context), pendingModel(0), parts(0)
        , childrenChanged(false), transaction(false) {}

    static void data_append(QDeclarativeListProperty<QObject> *prop, QObject *data) {
        QSGVisualModelPrivate *d = static_cast<QSGVisualModelPrivate *>(prop->data);
        QDeclarative_setParent_noEvent(data, prop->object);
        if (QSGVisualData *model = qobject_cast<QSGVisualData *>(data)) {
            d->appendModel(model);
        } else if (QSGItem *item = qobject_cast<QSGItem *>(data)) {
            d->appendData(item);
            d->itemAppended();
            d->emitChildrenChanged();
        }
    }

    static void children_append(QDeclarativeListProperty<QSGItem> *prop, QSGItem *child) {
        QSGVisualModelPrivate *d = static_cast<QSGVisualModelPrivate *>(prop->data);
        d->appendData(child);
        d->itemAppended();
        d->emitChildrenChanged();
    }

    static int children_count(QDeclarativeListProperty<QSGItem> *prop) {
        return static_cast<QSGVisualModelPrivate *>(prop->data)->children.count();
    }

    static QSGItem *children_at(QDeclarativeListProperty<QSGItem> *prop, int index) {
        return static_cast<QSGVisualModelPrivate *>(prop->data)->children.at(index).item;
    }

    void appendModel(QSGVisualData *model) {
        Q_Q(QSGVisualModel);
        const int insertIndex = count();
        const int insertCount = model->count();
        appendList(model, 0, insertCount, true);
        connectModel(model);
        if (insertCount > 0)
            emit q->itemsInserted(insertIndex, insertCount);
    }

    void connectModel(QSGVisualData *model) {
        Q_Q(QSGVisualModel);

        QObject::connect(model, SIGNAL(itemsInserted(QSGVisualData*,int,int)),
                q, SLOT(_q_itemsInserted(QSGVisualData*,int,int)), Qt::UniqueConnection);
        QObject::connect(model, SIGNAL(itemsRemoved(QSGVisualData*,int,int)),
                q, SLOT(_q_itemsRemoved(QSGVisualData*,int,int)), Qt::UniqueConnection);
        QObject::connect(model, SIGNAL(itemsMoved(QSGVisualData*,int,int,int)),
                q, SLOT(_q_itemsMoved(QSGVisualData*,int,int,int)), Qt::UniqueConnection);
        QObject::connect(model, SIGNAL(createdPackage(QSGVisualData*,int,QDeclarativePackage*)),
                q, SLOT(_q_createdPackage(QSGVisualData*,int,QDeclarativePackage*)), Qt::UniqueConnection);
        QObject::connect(model, SIGNAL(destroyingPackage(QDeclarativePackage*)),
                q, SLOT(_q_destroyingPackage(QDeclarativePackage*)), Qt::UniqueConnection);
    }

    void itemAppended() {
        Q_Q(QSGVisualModel);
        QSGVisualModelAttached *attached = QSGVisualModelAttached::properties(children.last().item);
        attached->setModel(q);
        attached->setIndex(count()-1);
        emit q->itemsInserted(count()-1, 1);
        emit q->countChanged();
    }

    void emitChildrenChanged() {
        Q_Q(QSGVisualModel);
        emit q->childrenChanged();
    }

    int indexOf(QSGItem *item) const {
        for (int i = 0; i < children.count(); ++i)
            if (children.at(i).item == item)
                return i;
        return -1;
    }

    void emitTransactionChanges() {
        Q_Q(QSGVisualModel);
        foreach (const QDeclarativeChangeSet::Remove &remove, transactionChanges.removes())
            emit q->itemsRemoved(remove.start, remove.count());
        foreach (const QDeclarativeChangeSet::Insert &insert, transactionChanges.inserts())
            emit q->itemsInserted(insert.start, insert.count());
        foreach (const QDeclarativeChangeSet::Move &move, transactionChanges.moves())
            emit q->itemsMoved(move.start, move.to, move.count());
        transactionChanges.clear();
    }

    void invalidateIndexes(int start, int end = INT_MAX) {
        for (QSGVisualModelAttached::List::iterator it = attachedItems.begin(); it != attachedItems.end(); ++it) {
            if (it->m_index >= start && it->m_index < end)
                it->setIndex(-1);
        }
    }

    void removeIndexes(int start, int count) {
        for (QSGVisualModelAttached::List::iterator it = attachedItems.begin(); it != attachedItems.end();) {
            if (it->m_index >= start) {
                it->setIndex(-1);
                if (it->m_index < start + count) {
                    it->setModel(0);
                    it = it.erase();
                } else {
                    ++it;
                }
            } else {
                ++it;
            }
        }
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
    QHash<QSGItem *, int> removedItems;
    QSGVisualModelAttached::List attachedItems;
    QDeclarativeChangeSet transactionChanges;

    QDeclarativeGuard<QDeclarativeContext> context;
    QSGVisualData *pendingModel;
    QSGVisualModelParts *parts;
    bool childrenChanged;
    bool transaction;

protected:
    void rangeCreated(void *) {}
    void rangeDestroyed(void *) {}

    bool insertInternalData(int index, const void *data)
    {
        childrenChanged = true;
        QSGItem *item = static_cast<QSGItem *>(const_cast<void *>(data));
        children.insert(index, Item(item));
        return true;
    }

    void replaceInternalData(int index, const void *data)
    {
        childrenChanged = true;
        QSGVisualModelPrivate::Item &item = children[index];
        if (item.ref > 0)
            removedItems.insert(item.item, item.ref);
        item.item = static_cast<QSGItem *>(const_cast<void *>(data));
        item.ref = 0;
    }

    void removeInternalData(int index, int count)
    {
        childrenChanged = true;
        for (int i = 0; i > count; ++i) {
            QSGVisualModelPrivate::Item &item = children[index + i];
            if (item.ref > 0)
                removedItems.insert(item.item, item.ref);
        }
        QList<Item>::iterator first = children.begin() + index;
        QList<Item>::iterator last = first + count - 1;
        children.erase(first, last);
    }

    void moveInternalData(int from, int to, int n)
    {
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

QSGVisualModel::QSGVisualModel(QObject *parent)
    : QObject(*(new QSGVisualModelPrivate), parent)
{
}

QSGVisualModel::QSGVisualModel(QSGVisualModelPrivate &dd, QObject *parent)
    : QObject(dd, parent)
{
}

QDeclarativeListProperty<QObject> QSGVisualModel::data()
{
    Q_D(QSGVisualModel);
    return QDeclarativeListProperty<QObject>(this, d, d->data_append);
}

QDeclarativeListProperty<QSGItem> QSGVisualModel::children()
{
    Q_D(QSGVisualModel);
    return QDeclarativeListProperty<QSGItem>(this, d, d->children_append,
                                                      d->children_count, d->children_at);
}

QObject *QSGVisualModel::parts()
{
    Q_D(QSGVisualModel);
    if (!d->parts)
        d->parts = new QSGVisualModelParts(this);
    return d->parts;
}

int QSGVisualModel::count() const
{
    Q_D(const QSGVisualModel);
    return d->count();
}

QSGItem *QSGVisualModel::item(int index, const QByteArray &viewId, bool complete)
{
    Q_D(QSGVisualModel);
    int offset = 0;
    int internalIndex = 0;
    QDeclarativeCompositeRange range = d->at(index, &offset, &internalIndex);
    if (!range.internal()) {
        QSGVisualData *model = static_cast<QSGVisualData *>(range.list);
        QSGItem *item = model->item(range.index + offset, viewId, complete);
        QSGVisualModelAttached *attached = QSGVisualModelAttached::properties(item);
        attached->setModel(this);
        attached->setData(model);
        attached->setIndex(index);
        d->attachedItems.insert(attached);
        if (model->completePending())
            d->pendingModel = model;
        return item;
    } else {
        QSGVisualModelPrivate::Item &item = d->children[internalIndex];
        item.addRef();
        return item.item;
    }
}

QSGItem *QSGVisualModel::item(int index)
{
    return item(index, QByteArray());
}

QSGItem *QSGVisualModel::take(int index, QSGItem *parent)
{
    Q_D(QSGVisualModel);
    QSGItem *i = item(index);
    remove(index, 1);
    i->setParentItem(parent);

    return i;
}

QSGVisualModel::ReleaseFlags QSGVisualModel::release(QSGItem *item)
{
    Q_D(QSGVisualModel);
    QSGVisualModelAttached *attached = QSGVisualModelAttached::properties(item);
    if (QSGVisualData *model = attached->m_data) {
        return model->release(item);
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

bool QSGVisualModel::completePending() const
{
    Q_D(const QSGVisualModel);
    return d->pendingModel;
}

void QSGVisualModel::completeItem()
{
    Q_D(QSGVisualModel);
    if (d->pendingModel) {
        d->pendingModel->completeItem();
        d->pendingModel = 0;
    }
}

QString QSGVisualModel::stringValue(int index, const QString &name)
{
    Q_D(QSGVisualModel);
    if (index < 0 || index >= d->children.count())
        return QString();
    return QDeclarativeEngine::contextForObject(d->children.at(index).item)->contextProperty(name).toString();
}

int QSGVisualModel::indexOf(QSGItem *item, QObject *context) const
{
    Q_D(const QSGVisualModel);
    QSGVisualModelAttached *attached = QSGVisualModelAttached::properties(item);
    if (QSGVisualData *model = attached->m_data) {
        int modelIndex = model->indexOf(item, context);
        return modelIndex != -1 ? d->absoluteIndexOf(model, modelIndex) : modelIndex;
    } else {
        int childIndex = d->indexOf(item);
        return childIndex != -1 ? d->absoluteIndexOf(childIndex) : childIndex;
    }
}

QScriptValue QSGVisualModel::getItemInfo(int index) const
{
    Q_D(const QSGVisualModel);
    QDeclarativeEngine *dengine = d->context ? d->context->engine() : qmlEngine(this);
    QScriptEngine *engine = QDeclarativeEnginePrivate::getScriptEngine(dengine);
    QScriptValue info = engine->newObject();

    int offset = 0;
    int internalIndex = 0;
    QDeclarativeCompositeRange range = d->at(index, &offset, &internalIndex);

    if (!range.internal()) {
        info.setProperty(QLatin1String("model"), engine->newVariant(static_cast<QSGVisualData *>(range.list)->model()));
        info.setProperty(QLatin1String("index"), range.index + offset);
        info.setProperty(QLatin1String("start"), range.index);
        info.setProperty(QLatin1String("end"), range.index + range.count);
    } else {
        info.setProperty(QLatin1String("model"), engine->nullValue());
    }
    return info;
}

void QSGVisualModel::append(QSGItem *item)
{
    Q_D(QSGVisualModel);
    int index = d->count();
    QSGVisualModelAttached *attached = QSGVisualModelAttached::properties(item);
    bool inserted = false;
    if (attached->m_data) {
        QSGVisualData *model = attached->m_data;
        int modelIndex = model->indexOf(item, 0);
        if (modelIndex != -1) {
            d->appendList(model, modelIndex, 1, false);
            d->connectModel(model);
            inserted = true;
        }
    } else if (d->appendData(item)) {
        d->attachedItems.insert(attached);
        inserted = true;
    }
    if (inserted) {
        QSGVisualModelAttached *attached = QSGVisualModelAttached::properties(item);
        attached->setModel(this);
        attached->setIndex(count() - 1);
        if (d->transaction) {
            d->transactionChanges.insertInsert(index, index + 1);
        } else {
            emit itemsInserted(index, 1);
            emit childrenChanged();
            emit countChanged();
        }
    }
}

void QSGVisualModel::append(QSGVisualModel *sourceModel, int sourceIndex, int count)
{
    Q_D(QSGVisualModel);
    int destinationIndex = d->count();

    if (sourceModel == this) {
        return;
    }

    for (int i = 0, difference = 0; i < count; i += difference) {
        int offset = 0;
        int internalIndex = 0;
        QDeclarativeCompositeRange range = sourceModel->d_func()->at(sourceIndex, &offset, &internalIndex);
        difference = range.count - offset;

        if (range.internal()) {
            for (int j = 0; j < qMin(count - i, range.count - offset); ++j)
                d->appendData(sourceModel->d_func()->children.at(internalIndex + j).item);
        } else {
            d->appendList(range.list, range.index + offset, qMin(count - i, range.count - offset), false);
            d->connectModel(static_cast<QSGVisualData *>(range.list));
        }
    }

    emit itemsInserted(destinationIndex, count);

    sourceModel->d_func()->removeAt(sourceIndex, count);
    emit sourceModel->itemsRemoved(sourceIndex, count);
    emit sourceModel->countChanged();
}

void QSGVisualModel::insert(int index, QSGItem *item)
{
    Q_D(QSGVisualModel);
    QSGVisualModelAttached *attached = QSGVisualModelAttached::properties(item);
    bool inserted = false;
    if (attached->m_data) {
        QSGVisualData *model = attached->m_data;
        int modelIndex = model->indexOf(item, 0);
        if (modelIndex != -1) {
            d->insertList(index, model, modelIndex, 1, false);
            d->connectModel(model);
            inserted = true;
        }
    } else if (d->insertData(index, item)) {
        d->attachedItems.insert(attached);
        inserted = true;
    }
    if (inserted) {
        attached->setModel(this);
        attached->setIndex(index);
        d->invalidateIndexes(index);
        if (d->transaction) {
            d->transactionChanges.insertInsert(index, index + 1);
        } else {
            emit itemsInserted(index, 1);
            emit childrenChanged();
            emit countChanged();
        }
    }
}

void QSGVisualModel::insert(int destinationIndex, QSGVisualModel *sourceModel, int sourceIndex, int count)
{
    Q_D(QSGVisualModel);

    if (sourceModel == this) {
        move(sourceIndex, destinationIndex, count);
        return;
    }

    for (int i = 0, difference = 0; i < count; i += difference) {
        int offset = 0;
        int internalIndex = 0;
        QDeclarativeCompositeRange range = sourceModel->d_func()->at(sourceIndex, &offset, &internalIndex);
        difference = range.count - offset;

        if (range.internal()) {
            for (int j = 0; j < qMin(count - i, range.count - offset); ++j)
                d->insertData(destinationIndex + i + j, sourceModel->d_func()->children.at(internalIndex + j).item);
        } else {
            d->insertList(destinationIndex + i, range.list, range.index + offset, qMin(count - i, range.count - offset), false);
            d->connectModel(static_cast<QSGVisualData *>(range.list));
        }
    }

    d->invalidateIndexes(destinationIndex);
    emit itemsInserted(destinationIndex, count);

    sourceModel->d_func()->removeAt(sourceIndex, count);
    sourceModel->d_func()->invalidateIndexes(sourceIndex);
    emit sourceModel->itemsRemoved(sourceIndex, count);
    emit sourceModel->countChanged();
}

void QSGVisualModel::remove(int index, int count)
{
    Q_D(QSGVisualModel);
    if (!d->transaction)
        d->childrenChanged = false;
    d->removeAt(index, count);
    d->removeIndexes(index, count);
    if (d->transaction) {
        d->transactionChanges.insertRemove(index, index + count);
    } else {      
        emit itemsRemoved(index, count);
        emit countChanged();
        if (d->childrenChanged)
            emit childrenChanged();
    }
}

void QSGVisualModel::move(int from, int to, int count)
{
    Q_D(QSGVisualModel);
    if (from == to || count == 0)
        return;
    if (!d->transaction)
        d->childrenChanged = false;
    d->move(from, to, count);
    d->invalidateIndexes(qMin(from, to), qMax(from, to) + count);
    if (d->transaction) {
        d->transactionChanges.insertMove(from, from + count, to);
    } else {
        emit itemsMoved(from, to, count);
        if (d->childrenChanged)
            emit childrenChanged();
    }
}

void QSGVisualModel::_q_itemsInserted(QSGVisualData *model, int index, int count)
{
    Q_D(QSGVisualModel);

    Q_ASSERT(count >= 0);

    QVector<QDeclarativeChangeSet::Insert> inserts;
    d->childrenChanged = false;
    d->listItemsInserted(model, index, index + count, &inserts);

    if (inserts.count() > 0) {
        d->invalidateIndexes(index);
        QDeclarativeEngine *dengine = d->context ? d->context->engine() : qmlEngine(this);
        QScriptEngine *engine = QDeclarativeEnginePrivate::getScriptEngine(dengine);
        QScriptValue insertIndexes = engine->newArray(inserts.count());
        for (int i = 0; i < inserts.count(); ++i) {
            QScriptValue range = engine->newObject();
            range.setProperty(QLatin1String("start"), inserts.at(i).start);
            range.setProperty(QLatin1String("end"), inserts.at(i).end);
            insertIndexes.setProperty(i, range);
        }
        d->transaction = true;
        d->transactionChanges.append(inserts);
        emit updated(insertIndexes);
        d->transaction = false;

        d->emitTransactionChanges();
        emit countChanged();
        if (d->childrenChanged)
            emit childrenChanged();
    }
}

void QSGVisualModel::_q_itemsRemoved(QSGVisualData *model, int index, int count)
{
    Q_D(QSGVisualModel);

    QVector<QDeclarativeChangeSet::Remove> removes;
    d->childrenChanged = false;
    d->listItemsRemoved(model, index, index + count, &removes);
    d->transactionChanges.append(removes);
    d->invalidateIndexes(index);
    d->emitTransactionChanges();
    emit countChanged();
    if (d->childrenChanged)
        emit childrenChanged();
}

void QSGVisualModel::_q_itemsMoved(QSGVisualData *model, int from, int to, int count)
{
    Q_D(QSGVisualModel);
    QVector<QDeclarativeChangeSet::Move> moves;
    d->childrenChanged = false;
    d->listItemsMoved(model, from, from + count, to, &moves);
    d->transactionChanges.append(moves);
    d->invalidateIndexes(qMin(from, to), qMax(from, to) + count);
    d->emitTransactionChanges();
    if (d->childrenChanged)
        emit childrenChanged();
}

void QSGVisualModel::_q_createdPackage(QSGVisualData *model, int index, QDeclarativePackage *package)
{
    Q_D(QSGVisualModel);
    int absoluteIndex = d->absoluteIndexOf(model, index);
    if (absoluteIndex != -1)
        emit createdPackage(absoluteIndex, package);
}

void QSGVisualModel::_q_destroyingPackage(QDeclarativePackage *package)
{
    emit destroyingPackage(package);
}

QSGVisualModelAttached *QSGVisualModel::qmlAttachedProperties(QObject *obj)
{
    return QSGVisualModelAttached::properties(obj);
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

class QSGVisualDataParts;
class QSGVisualModelData;
class QSGVisualDataPrivate : public QObjectPrivate
{
public:
    QSGVisualDataPrivate(QDeclarativeContext *);

    static QSGVisualDataPrivate *get(QSGVisualData *m) {
        return static_cast<QSGVisualDataPrivate *>(QObjectPrivate::get(m));
    }

    QDeclarativeGuard<QListModelInterface> m_listModelInterface;
    QDeclarativeGuard<QAbstractItemModel> m_abstractItemModel;
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
        if (m_listModelInterface)
            return qMax(0, m_listModelInterface->count());
        if (m_abstractItemModel)
            return qMax(0, m_abstractItemModel->rowCount(m_root));
        if (m_listAccessor)
            return m_listAccessor->count();
        return 0;
    }

    Cache m_cache;
    QHash<QObject *, QDeclarativePackage*> m_packaged;

    VDMDelegateDataType *m_delegateDataType;
    friend class QSGVisualModelData;
    bool m_metaDataCreated : 1;
    bool m_metaDataCacheable : 1;
    bool m_delegateValidated : 1;
    bool m_completePending : 1;

    QSGVisualModelData *data(QObject *item);

    QVariant m_modelVariant;
    QDeclarativeListAccessor *m_listAccessor;

    QModelIndex m_root;
    QList<QByteArray> watchedRoles;
    QList<int> watchedRoleIds;
};

class QSGVisualModelDataMetaObject : public QDeclarativeOpenMetaObject
{
public:
    QSGVisualModelDataMetaObject(QObject *parent, QDeclarativeOpenMetaObjectType *type)
    : QDeclarativeOpenMetaObject(parent, type) {}

    virtual QVariant initialValue(int);
    virtual int createProperty(const char *, const char *);

private:
    friend class QSGVisualModelData;
};

class QSGVisualModelData : public QObject
{
Q_OBJECT
public:
    QSGVisualModelData(int index, QSGVisualData *model);
    ~QSGVisualModelData();

    Q_PROPERTY(int index READ index NOTIFY indexChanged)
    int index() const;
    void setIndex(int index);

    int propForRole(int) const;
    int modelDataPropertyId() const {
        QSGVisualDataPrivate *model = QSGVisualDataPrivate::get(m_model);
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
    friend class QSGVisualModelDataMetaObject;
    int m_index;
    QDeclarativeGuard<QSGVisualData> m_model;
    QSGVisualModelDataMetaObject *m_meta;
};

int QSGVisualModelData::propForRole(int id) const
{
    QSGVisualDataPrivate *model = QSGVisualDataPrivate::get(m_model);
    QHash<int,int>::const_iterator it = model->m_roleToPropId.find(id);
    if (it != model->m_roleToPropId.end())
        return *it;

    return -1;
}

void QSGVisualModelData::setValue(int id, const QVariant &val)
{
    m_meta->setValue(id, val);
}

int QSGVisualModelDataMetaObject::createProperty(const char *name, const char *type)
{
    QSGVisualModelData *data =
        static_cast<QSGVisualModelData *>(object());

    if (!data->m_model)
        return -1;

    QSGVisualDataPrivate *model = QSGVisualDataPrivate::get(data->m_model);
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

QVariant QSGVisualModelDataMetaObject::initialValue(int propId)
{
    QSGVisualModelData *data =
        static_cast<QSGVisualModelData *>(object());

    Q_ASSERT(data->m_model);
    QSGVisualDataPrivate *model = QSGVisualDataPrivate::get(data->m_model);

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

QSGVisualModelData::QSGVisualModelData(int index,
                                               QSGVisualData *model)
: m_index(index), m_model(model),
m_meta(new QSGVisualModelDataMetaObject(this, QSGVisualDataPrivate::get(model)->m_delegateDataType))
{
    ensureProperties();
}

QSGVisualModelData::~QSGVisualModelData()
{
}

void QSGVisualModelData::ensureProperties()
{
    QSGVisualDataPrivate *modelPriv = QSGVisualDataPrivate::get(m_model);
    if (modelPriv->m_metaDataCacheable) {
        if (!modelPriv->m_metaDataCreated)
            modelPriv->createMetaData();
        if (modelPriv->m_metaDataCreated)
            m_meta->setCached(true);
    }
}

int QSGVisualModelData::index() const
{
    return m_index;
}

// This is internal only - it should not be set from qml
void QSGVisualModelData::setIndex(int index)
{
    m_index = index;
    emit indexChanged();
}


//---------------------------------------------------------------------------

QSGVisualDataPrivate::QSGVisualDataPrivate(QDeclarativeContext *ctxt)
: m_listModelInterface(0), m_abstractItemModel(0), m_delegate(0)
, m_context(ctxt), m_modelDataPropId(-1), m_delegateDataType(0), m_metaDataCreated(false)
, m_metaDataCacheable(false), m_delegateValidated(false), m_completePending(false), m_listAccessor(0)
{
}

QSGVisualModelData *QSGVisualDataPrivate::data(QObject *item)
{
    QSGVisualModelData *dataItem =
        item->findChild<QSGVisualModelData *>();
    Q_ASSERT(dataItem);
    return dataItem;
}

//---------------------------------------------------------------------------

QSGVisualData::QSGVisualData(QObject *parent)
: QObject(*(new QSGVisualDataPrivate(0)), parent)
{
}

QSGVisualData::QSGVisualData(QDeclarativeContext *ctxt, QObject *parent)
: QObject(*(new QSGVisualDataPrivate(ctxt)), parent)
{
}

QSGVisualData::~QSGVisualData()
{
    Q_D(QSGVisualData);
    if (d->m_listAccessor)
        delete d->m_listAccessor;
    if (d->m_delegateDataType)
        d->m_delegateDataType->release();
}

QVariant QSGVisualData::model() const
{
    Q_D(const QSGVisualData);
    return d->m_modelVariant;
}

void QSGVisualData::setModel(const QVariant &model)
{
    Q_D(QSGVisualData);
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
    }

    d->m_roles.clear();
    d->m_roleNames.clear();
    if (d->m_delegateDataType)
        d->m_delegateDataType->release();
    d->m_metaDataCreated = 0;
    d->m_metaDataCacheable = false;
    d->m_delegateDataType = new VDMDelegateDataType(&QSGVisualModelData::staticMetaObject, d->m_context?d->m_context->engine():qmlEngine(this));

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
            emit itemsInserted(this, 0, d->m_listModelInterface->count());
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
    d->m_listAccessor = new QDeclarativeListAccessor;
    d->m_listAccessor->setList(model, d->m_context?d->m_context->engine():qmlEngine(this));
    if (d->m_listAccessor->type() != QDeclarativeListAccessor::ListProperty)
        d->m_metaDataCacheable = true;
    if (d->m_delegate && d->modelCount())
        emit itemsInserted(this, 0, d->modelCount());
}

QDeclarativeComponent *QSGVisualData::delegate() const
{
    Q_D(const QSGVisualData);
    return d->m_delegate;
}

void QSGVisualData::setDelegate(QDeclarativeComponent *delegate)
{
    Q_D(QSGVisualData);
    bool wasValid = d->m_delegate != 0;
    d->m_delegate = delegate;
    d->m_delegateValidated = false;
    if (!wasValid && d->modelCount() && d->m_delegate)
        emit itemsInserted(this, 0, d->modelCount());
    if (wasValid && !d->m_delegate && d->modelCount())
        emit itemsRemoved(this, 0, d->modelCount());
}

QVariant QSGVisualData::rootIndex() const
{
    Q_D(const QSGVisualData);
    return QVariant::fromValue(d->m_root);
}

void QSGVisualData::setRootIndex(const QVariant &root)
{
    Q_D(QSGVisualData);
    QModelIndex modelIndex = qvariant_cast<QModelIndex>(root);
    if (d->m_root != modelIndex) {
        int oldCount = d->modelCount();
        d->m_root = modelIndex;
        if (d->m_abstractItemModel && d->m_abstractItemModel->canFetchMore(modelIndex))
            d->m_abstractItemModel->fetchMore(modelIndex);
        int newCount = d->modelCount();
        if (d->m_delegate && oldCount)
            emit itemsRemoved(this, 0, oldCount);
        if (d->m_delegate && newCount)
            emit itemsInserted(this, 0, newCount);
        emit rootIndexChanged();
    }
}

QVariant QSGVisualData::modelIndex(int idx) const
{
    Q_D(const QSGVisualData);
    if (d->m_abstractItemModel)
        return QVariant::fromValue(d->m_abstractItemModel->index(idx, 0, d->m_root));
    return QVariant::fromValue(QModelIndex());
}

QVariant QSGVisualData::parentModelIndex() const
{
    Q_D(const QSGVisualData);
    if (d->m_abstractItemModel)
        return QVariant::fromValue(d->m_abstractItemModel->parent(d->m_root));
    return QVariant::fromValue(QModelIndex());
}

int QSGVisualData::count() const
{
    Q_D(const QSGVisualData);
    if (!d->m_delegate)
        return 0;
    return d->modelCount();
}

/*
  Returns ReleaseStatus flags.
*/
QSGVisualModel::ReleaseFlags QSGVisualData::release(QSGItem *item)
{
    Q_D(QSGVisualData);
    QSGVisualModel::ReleaseFlags stat = 0;
    QObject *obj = item;
    bool inPackage = false;

    QHash<QObject*,QDeclarativePackage*>::iterator it = d->m_packaged.find(item);
    if (it != d->m_packaged.end()) {
        QDeclarativePackage *package = *it;
        d->m_packaged.erase(it);
        if (d->m_packaged.contains(item))
            stat |= QSGVisualModel::Referenced;
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
        stat |= QSGVisualModel::Destroyed;
        obj->deleteLater();
    } else if (!inPackage) {
        stat |= QSGVisualModel::Referenced;
    }

    return stat;
}

QSGItem *QSGVisualData::item(int index, const QByteArray &viewId, bool complete)
{
    Q_D(QSGVisualData);
    if (d->modelCount() <= 0 || !d->m_delegate)
        return 0;
    QObject *nobj = d->m_cache.getItem(index);
    bool needComplete = false;
    if (!nobj) {
        QDeclarativeContext *ccontext = d->m_context;
        if (!ccontext) ccontext = qmlContext(this);
        if (!ccontext) ccontext = qmlContext(parent());
        QDeclarativeContext *ctxt = new QDeclarativeContext(ccontext);
        QSGVisualModelData *data = new QSGVisualModelData(index, this);
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
                emit createdPackage(this, index, package);
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
            qmlInfo(d->m_delegate) << QSGVisualData::tr("Delegate component must be Item type.");
            d->m_delegateValidated = true;
        }
    }
    if (d->modelCount()-1 == index && d->m_abstractItemModel && d->m_abstractItemModel->canFetchMore(d->m_root))
        d->m_abstractItemModel->fetchMore(d->m_root);

    return item;
}

bool QSGVisualData::completePending() const
{
    Q_D(const QSGVisualData);
    return d->m_completePending;
}

void QSGVisualData::completeItem()
{
    Q_D(QSGVisualData);
    d->m_delegate->completeCreate();
    d->m_completePending = false;
}

QString QSGVisualData::stringValue(int index, const QString &name)
{
    Q_D(QSGVisualData);
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
        data = new QSGVisualModelData(index, this);
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

int QSGVisualData::indexOf(QSGItem *item, QObject *) const
{
    QVariant val = QDeclarativeEngine::contextForObject(item)->contextProperty(QLatin1String("index"));
        return val.toInt();
    return -1;
}

void QSGVisualData::setWatchedRoles(QList<QByteArray> roles)
{
    Q_D(QSGVisualData);
    d->watchedRoles = roles;
    d->watchedRoleIds.clear();
}

void QSGVisualData::_q_itemsChanged(int index, int count,
                                         const QList<int> &roles)
{
    Q_D(QSGVisualData);
    bool changed = false;
    if (!d->watchedRoles.isEmpty() && d->watchedRoleIds.isEmpty()) {
        foreach (QByteArray r, d->watchedRoles) {
            if (d->m_roleNames.contains(r))
                d->watchedRoleIds << d->m_roleNames.value(r);
        }
    }

    for (QHash<int,QSGVisualDataPrivate::ObjectRef>::ConstIterator iter = d->m_cache.begin();
        iter != d->m_cache.end(); ++iter) {
        const int idx = iter.key();

        if (idx >= index && idx < index+count) {
            QSGVisualDataPrivate::ObjectRef objRef = *iter;
            QSGVisualModelData *data = d->data(objRef.obj);
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
        emit itemsChanged(this, index, count);
}

void QSGVisualData::_q_itemsInserted(int index, int count)
{
    Q_D(QSGVisualData);
    if (!count)
        return;
    // XXX - highly inefficient
    QHash<int,QSGVisualDataPrivate::ObjectRef> items;
    for (QHash<int,QSGVisualDataPrivate::ObjectRef>::Iterator iter = d->m_cache.begin();
        iter != d->m_cache.end(); ) {

        if (iter.key() >= index) {
            QSGVisualDataPrivate::ObjectRef objRef = *iter;
            int index = iter.key() + count;
            iter = d->m_cache.erase(iter);

            items.insert(index, objRef);

            QSGVisualModelData *data = d->data(objRef.obj);
            data->setIndex(index);
        } else {
            ++iter;
        }
    }
    d->m_cache.unite(items);

    emit itemsInserted(this, index, count);
}

void QSGVisualData::_q_itemsRemoved(int index, int count)
{
    Q_D(QSGVisualData);
    if (!count)
        return;
    // XXX - highly inefficient
    QHash<int, QSGVisualDataPrivate::ObjectRef> items;
    for (QHash<int, QSGVisualDataPrivate::ObjectRef>::Iterator iter = d->m_cache.begin();
        iter != d->m_cache.end(); ) {
        if (iter.key() >= index && iter.key() < index + count) {
            QSGVisualDataPrivate::ObjectRef objRef = *iter;
            iter = d->m_cache.erase(iter);
            items.insertMulti(-1, objRef); //XXX perhaps better to maintain separately
            QSGVisualModelData *data = d->data(objRef.obj);
            data->setIndex(-1);
        } else if (iter.key() >= index + count) {
            QSGVisualDataPrivate::ObjectRef objRef = *iter;
            int index = iter.key() - count;
            iter = d->m_cache.erase(iter);
            items.insert(index, objRef);
            QSGVisualModelData *data = d->data(objRef.obj);
            data->setIndex(index);
        } else {
            ++iter;
        }
    }

    d->m_cache.unite(items);
    emit itemsRemoved(this, index, count);
}

void QSGVisualData::_q_itemsMoved(int from, int to, int count)
{
    Q_D(QSGVisualData);
    // XXX - highly inefficient
    QHash<int,QSGVisualDataPrivate::ObjectRef> items;
    for (QHash<int,QSGVisualDataPrivate::ObjectRef>::Iterator iter = d->m_cache.begin();
        iter != d->m_cache.end(); ) {

        if (iter.key() >= from && iter.key() < from + count) {
            QSGVisualDataPrivate::ObjectRef objRef = *iter;
            int index = iter.key() - from + to;
            iter = d->m_cache.erase(iter);

            items.insert(index, objRef);

            QSGVisualModelData *data = d->data(objRef.obj);
            data->setIndex(index);
        } else {
            ++iter;
        }
    }
    for (QHash<int,QSGVisualDataPrivate::ObjectRef>::Iterator iter = d->m_cache.begin();
        iter != d->m_cache.end(); ) {

        int diff = from > to ? count : -count;
        if (iter.key() >= qMin(from,to) && iter.key() < qMax(from+count,to+count)) {
            QSGVisualDataPrivate::ObjectRef objRef = *iter;
            int index = iter.key() + diff;
            iter = d->m_cache.erase(iter);

            items.insert(index, objRef);

            QSGVisualModelData *data = d->data(objRef.obj);
            data->setIndex(index);
        } else {
            ++iter;
        }
    }
    d->m_cache.unite(items);

    emit itemsMoved(this, from, to, count);
}

void QSGVisualData::_q_rowsInserted(const QModelIndex &parent, int begin, int end)
{
    Q_D(QSGVisualData);
    if (parent == d->m_root)
        _q_itemsInserted(begin, end - begin + 1);
}

void QSGVisualData::_q_rowsRemoved(const QModelIndex &parent, int begin, int end)
{
    Q_D(QSGVisualData);
    if (parent == d->m_root)
        _q_itemsRemoved(begin, end - begin + 1);
}

void QSGVisualData::_q_rowsMoved(const QModelIndex &sourceParent, int sourceStart, int sourceEnd, const QModelIndex &destinationParent, int destinationRow)
{
    Q_D(QSGVisualData);
    const int count = sourceEnd - sourceStart + 1;
    if (destinationParent == d->m_root && sourceParent == d->m_root) {
        _q_itemsMoved(sourceStart, sourceStart > destinationRow ? destinationRow : destinationRow-1, count);
    } else if (sourceParent == d->m_root) {
        _q_itemsRemoved(sourceStart, count);
    } else if (destinationParent == d->m_root) {
        _q_itemsInserted(destinationRow, count);
    }
}

void QSGVisualData::_q_dataChanged(const QModelIndex &begin, const QModelIndex &end)
{
    Q_D(QSGVisualData);
    if (begin.parent() == d->m_root)
        _q_itemsChanged(begin.row(), end.row() - begin.row() + 1, d->m_roles);
}

void QSGVisualData::_q_layoutChanged()
{
    Q_D(QSGVisualData);
    _q_itemsChanged(0, count(), d->m_roles);
}

void QSGVisualData::_q_modelReset()
{
    Q_D(QSGVisualData);
    d->m_root = QModelIndex();
    emit rootIndexChanged();
    if (d->m_abstractItemModel && d->m_abstractItemModel->canFetchMore(d->m_root))
        d->m_abstractItemModel->fetchMore(d->m_root);
}

//============================================================================

QSGVisualItemModel::QSGVisualItemModel(QObject *parent)
    : QSGVisualModel(parent)
{
}

QSGVisualItemModel::~QSGVisualItemModel()
{
}


class QSGVisualDataModelPrivate : public QSGVisualModelPrivate
{
    Q_DECLARE_PUBLIC(QSGVisualDataModel)
public:
    QSGVisualDataModelPrivate(QDeclarativeContext *context = 0)
        : QSGVisualModelPrivate(context) {}

    QSGVisualData *data;

    void init()
    {
        Q_Q(QSGVisualDataModel);
        data = new QSGVisualData(context, q);
        appendModel(data);
    }
};

QSGVisualDataModel::QSGVisualDataModel()
    : QSGVisualModel(*new QSGVisualDataModelPrivate, 0)
{
    Q_D(QSGVisualDataModel);
    d->init();
}

QSGVisualDataModel::QSGVisualDataModel(QDeclarativeContext *context, QObject *parent)
    : QSGVisualModel(*new QSGVisualDataModelPrivate(context), parent)
{
    Q_D(QSGVisualDataModel);
    d->init();
}

QSGVisualDataModel::~QSGVisualDataModel()
{
}

QVariant QSGVisualDataModel::model() const
{
    Q_D(const QSGVisualDataModel);
    return d->data->model();
}

void QSGVisualDataModel::setModel(const QVariant &model)
{
    Q_D(QSGVisualDataModel);
    d->data->setModel(model);
}

QDeclarativeComponent *QSGVisualDataModel::delegate() const
{
    Q_D(const QSGVisualDataModel);
    return d->data->delegate();
}

void QSGVisualDataModel::setDelegate(QDeclarativeComponent *delegate)
{
    Q_D(QSGVisualDataModel);
    d->data->setDelegate(delegate);
}

QVariant QSGVisualDataModel::rootIndex() const
{
    Q_D(const QSGVisualDataModel);
    return d->data->rootIndex();
}

void QSGVisualDataModel::setRootIndex(const QVariant &root)
{
    Q_D(QSGVisualDataModel);
    d->data->setRootIndex(root);
}

QVariant QSGVisualDataModel::modelIndex(int idx) const
{
    Q_D(const QSGVisualDataModel);
    return d->data->modelIndex(idx);
}

QVariant QSGVisualDataModel::parentModelIndex() const
{
    Q_D(const QSGVisualDataModel);
    return d->data->parentModelIndex();
}

QSGVisualPartModel::QSGVisualPartModel(QSGVisualModel *model, const QByteArray &part, QObject *parent)
  : QObject(parent), m_model(model), m_part(part)
{
}

QSGVisualModel *QSGVisualPartModel::model() const
{
    return m_model;
}

QByteArray QSGVisualPartModel::part() const
{
    return m_part;
}

QSGItem *QSGVisualPartModel::item(int index)
{
    return m_model->item(index, m_part);
}

QSGItem *QSGVisualPartModel::take(int index, QSGItem *parent)
{
    QSGItem *item = m_model->item(index, m_part);
    m_model->remove(index, 1);
    item->setParentItem(parent);
    return item;
}

QT_END_NAMESPACE

#include <qsgvisualitemmodel.moc>
