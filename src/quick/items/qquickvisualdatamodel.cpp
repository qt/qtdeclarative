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

#include "qquickvisualdatamodel_p_p.h"
#include "qquickitem.h"

#include <QtQml/qqmlinfo.h>

#include <private/qquickpackage_p.h>
#include <private/qmetaobjectbuilder_p.h>
#include <private/qquickvisualadaptormodel_p.h>
#include <private/qquickchangeset_p.h>
#include <private/qqmlengine_p.h>

QT_BEGIN_NAMESPACE

void QQuickVisualDataModelPartsMetaObject::propertyCreated(int, QMetaPropertyBuilder &prop)
{
    prop.setWritable(false);
}

QVariant QQuickVisualDataModelPartsMetaObject::initialValue(int id)
{
    QQuickVisualDataModelParts *parts = static_cast<QQuickVisualDataModelParts *>(object());
    QQuickVisualPartsModel *m = new QQuickVisualPartsModel(
            parts->model, QString::fromUtf8(name(id)), parts);
    parts->models.append(m);
    return QVariant::fromValue(static_cast<QObject *>(m));
}

QQuickVisualDataModelParts::QQuickVisualDataModelParts(QQuickVisualDataModel *parent)
: QObject(parent), model(parent)
{
    new QQuickVisualDataModelPartsMetaObject(this);
}

//---------------------------------------------------------------------------

QHash<QObject*, QQuickVisualDataModelAttached*> QQuickVisualDataModelAttached::attachedProperties;

/*!
    \qmlclass VisualDataModel QQuickVisualDataModel
    \inqmlmodule QtQuick 2
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

    \snippet doc/src/snippets/qml/visualdatamodel.qml 0
*/

QQuickVisualDataModelPrivate::QQuickVisualDataModelPrivate(QQmlContext *ctxt)
    : m_adaptorModel(0)
    , m_delegate(0)
    , m_cacheMetaType(0)
    , m_context(ctxt)
    , m_parts(0)
    , m_compositorGroup(Compositor::Cache)
    , m_complete(false)
    , m_delegateValidated(false)
    , m_reset(false)
    , m_transaction(false)
    , m_incubatorCleanupScheduled(false)
    , m_filterGroup(QStringLiteral("items"))
    , m_cacheItems(0)
    , m_items(0)
    , m_persistedItems(0)
    , m_groupCount(Compositor::MinimumGroupCount)
{
}

QQuickVisualDataModelPrivate::~QQuickVisualDataModelPrivate()
{
    qDeleteAll(m_finishedIncubating);
}

void QQuickVisualDataModelPrivate::connectModel(QQuickVisualAdaptorModel *model)
{
    Q_Q(QQuickVisualDataModel);

    QObject::connect(model, SIGNAL(itemsInserted(int,int)), q, SLOT(_q_itemsInserted(int,int)));
    QObject::connect(model, SIGNAL(itemsRemoved(int,int)), q, SLOT(_q_itemsRemoved(int,int)));
    QObject::connect(model, SIGNAL(itemsMoved(int,int,int)), q, SLOT(_q_itemsMoved(int,int,int)));
    QObject::connect(model, SIGNAL(itemsChanged(int,int)), q, SLOT(_q_itemsChanged(int,int)));
    QObject::connect(model, SIGNAL(modelReset(int,int)), q, SLOT(_q_modelReset(int,int)));
}

void QQuickVisualDataModelPrivate::init()
{
    Q_Q(QQuickVisualDataModel);
    m_compositor.setRemoveGroups(Compositor::GroupMask & ~Compositor::PersistedFlag);

    m_adaptorModel = new QQuickVisualAdaptorModel;
    QObject::connect(m_adaptorModel, SIGNAL(rootIndexChanged()), q, SIGNAL(rootIndexChanged()));

    m_items = new QQuickVisualDataGroup(QStringLiteral("items"), q, Compositor::Default, q);
    m_items->setDefaultInclude(true);
    m_persistedItems = new QQuickVisualDataGroup(QStringLiteral("persistedItems"), q, Compositor::Persisted, q);
    QQuickVisualDataGroupPrivate::get(m_items)->emitters.insert(this);
}

QQuickVisualDataModel::QQuickVisualDataModel()
: QQuickVisualModel(*(new QQuickVisualDataModelPrivate(0)))
{
    Q_D(QQuickVisualDataModel);
    d->init();
}

QQuickVisualDataModel::QQuickVisualDataModel(QQmlContext *ctxt, QObject *parent)
: QQuickVisualModel(*(new QQuickVisualDataModelPrivate(ctxt)), parent)
{
    Q_D(QQuickVisualDataModel);
    d->init();
}

QQuickVisualDataModel::~QQuickVisualDataModel()
{
    Q_D(QQuickVisualDataModel);

    foreach (QQuickVisualDataModelItem *cacheItem, d->m_cache) {
        // If the object holds the last reference to the cache item deleting it will also
        // delete the cache item, temporarily increase the reference count to avoid this.
        cacheItem->scriptRef += 1;
        delete cacheItem->object;
        cacheItem->scriptRef -= 1;
        cacheItem->objectRef = 0;
        cacheItem->object = 0;
        if (!cacheItem->isReferenced())
            delete cacheItem;
    }

    delete d->m_adaptorModel;
    if (d->m_cacheMetaType)
        d->m_cacheMetaType->release();
}


void QQuickVisualDataModel::classBegin()
{
}

void QQuickVisualDataModel::componentComplete()
{
    Q_D(QQuickVisualDataModel);
    d->m_complete = true;

    int defaultGroups = 0;
    QStringList groupNames;
    groupNames.append(QStringLiteral("items"));
    groupNames.append(QStringLiteral("persistedItems"));
    if (QQuickVisualDataGroupPrivate::get(d->m_items)->defaultInclude)
        defaultGroups |= Compositor::DefaultFlag;
    if (QQuickVisualDataGroupPrivate::get(d->m_persistedItems)->defaultInclude)
        defaultGroups |= Compositor::PersistedFlag;
    for (int i = Compositor::MinimumGroupCount; i < d->m_groupCount; ++i) {
        QString name = d->m_groups[i]->name();
        if (name.isEmpty()) {
            d->m_groups[i] = d->m_groups[d->m_groupCount - 1];
            --d->m_groupCount;
            --i;
        } else if (name.at(0).isUpper()) {
            qmlInfo(d->m_groups[i]) << QQuickVisualDataGroup::tr("Group names must start with a lower case letter");
            d->m_groups[i] = d->m_groups[d->m_groupCount - 1];
            --d->m_groupCount;
            --i;
        } else {
            groupNames.append(name);

            QQuickVisualDataGroupPrivate *group = QQuickVisualDataGroupPrivate::get(d->m_groups[i]);
            group->setModel(this, Compositor::Group(i));
            if (group->defaultInclude)
                defaultGroups |= (1 << i);
        }
    }
    if (!d->m_context)
        d->m_context = qmlContext(this);

    d->m_cacheMetaType = new QQuickVisualDataModelItemMetaType(
            QQmlEnginePrivate::getV8Engine(d->m_context->engine()), this, groupNames);

    d->m_compositor.setGroupCount(d->m_groupCount);
    d->m_compositor.setDefaultGroups(defaultGroups);
    d->updateFilterGroup();

    while (!d->m_pendingParts.isEmpty())
        static_cast<QQuickVisualPartsModel *>(d->m_pendingParts.first())->updateFilterGroup();

    d->connectModel(d->m_adaptorModel);
    QVector<Compositor::Insert> inserts;
    d->m_compositor.append(
            d->m_adaptorModel,
            0,
            qMax(0, d->m_adaptorModel->count()),
            defaultGroups | Compositor::AppendFlag | Compositor::PrependFlag,
            &inserts);
    d->itemsInserted(inserts);
    d->emitChanges();

    if (d->m_adaptorModel->canFetchMore())
        QCoreApplication::postEvent(this, new QEvent(QEvent::UpdateRequest));
}

/*!
    \qmlproperty model QtQuick2::VisualDataModel::model
    This property holds the model providing data for the VisualDataModel.

    The model provides a set of data that is used to create the items
    for a view.  For large or dynamic datasets the model is usually
    provided by a C++ model object.  The C++ model object must be a \l
    {QAbstractItemModel} subclass or a simple list.

    Models can also be created directly in QML, using a \l{ListModel} or
    \l{XmlListModel}.

    \sa {qmlmodels}{Data Models}
*/
QVariant QQuickVisualDataModel::model() const
{
    Q_D(const QQuickVisualDataModel);
    return d->m_adaptorModel->model();
}

void QQuickVisualDataModel::setModel(const QVariant &model)
{
    Q_D(QQuickVisualDataModel);
    d->m_adaptorModel->setModel(model, d->m_context ? d->m_context->engine() : qmlEngine(this));
    if (d->m_complete && d->m_adaptorModel->canFetchMore())
        QCoreApplication::postEvent(this, new QEvent(QEvent::UpdateRequest));
}

/*!
    \qmlproperty Component QtQuick2::VisualDataModel::delegate

    The delegate provides a template defining each item instantiated by a view.
    The index is exposed as an accessible \c index property.  Properties of the
    model are also available depending upon the type of \l {qmlmodels}{Data Model}.
*/
QQmlComponent *QQuickVisualDataModel::delegate() const
{
    Q_D(const QQuickVisualDataModel);
    return d->m_delegate;
}

void QQuickVisualDataModel::setDelegate(QQmlComponent *delegate)
{
    Q_D(QQuickVisualDataModel);
    if (d->m_transaction) {
        qmlInfo(this) << tr("The delegate of a VisualDataModel cannot be changed within onUpdated.");
        return;
    }
    bool wasValid = d->m_delegate != 0;
    d->m_delegate = delegate;
    d->m_delegateValidated = false;
    if (wasValid && d->m_complete) {
        for (int i = 1; i < d->m_groupCount; ++i) {
            QQuickVisualDataGroupPrivate::get(d->m_groups[i])->changeSet.remove(
                    0, d->m_compositor.count(Compositor::Group(i)));
        }
    }
    if (d->m_complete && d->m_delegate) {
        for (int i = 1; i < d->m_groupCount; ++i) {
            QQuickVisualDataGroupPrivate::get(d->m_groups[i])->changeSet.insert(
                    0, d->m_compositor.count(Compositor::Group(i)));
        }
    }
    d->emitChanges();
}

/*!
    \qmlproperty QModelIndex QtQuick2::VisualDataModel::rootIndex

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
    \snippet doc/src/snippets/qml/visualdatamodel_rootindex/main.cpp 0

    \c view.qml:
    \snippet doc/src/snippets/qml/visualdatamodel_rootindex/view.qml 0

    If the \l model is a QAbstractItemModel subclass, the delegate can also
    reference a \c hasModelChildren property (optionally qualified by a
    \e model. prefix) that indicates whether the delegate's model item has
    any child nodes.


    \sa modelIndex(), parentModelIndex()
*/
QVariant QQuickVisualDataModel::rootIndex() const
{
    Q_D(const QQuickVisualDataModel);
    return d->m_adaptorModel->rootIndex();
}

void QQuickVisualDataModel::setRootIndex(const QVariant &root)
{
    Q_D(QQuickVisualDataModel);
    d->m_adaptorModel->setRootIndex(root);
}

/*!
    \qmlmethod QModelIndex QtQuick2::VisualDataModel::modelIndex(int index)

    QAbstractItemModel provides a hierarchical tree of data, whereas
    QML only operates on list data.  This function assists in using
    tree models in QML.

    Returns a QModelIndex for the specified index.
    This value can be assigned to rootIndex.

    \sa rootIndex
*/
QVariant QQuickVisualDataModel::modelIndex(int idx) const
{
    Q_D(const QQuickVisualDataModel);
    return d->m_adaptorModel->modelIndex(idx);
}

/*!
    \qmlmethod QModelIndex QtQuick2::VisualDataModel::parentModelIndex()

    QAbstractItemModel provides a hierarchical tree of data, whereas
    QML only operates on list data.  This function assists in using
    tree models in QML.

    Returns a QModelIndex for the parent of the current rootIndex.
    This value can be assigned to rootIndex.

    \sa rootIndex
*/
QVariant QQuickVisualDataModel::parentModelIndex() const
{
    Q_D(const QQuickVisualDataModel);
    return d->m_adaptorModel->parentModelIndex();
}

/*!
    \qmlproperty int QtQuick2::VisualDataModel::count
*/

int QQuickVisualDataModel::count() const
{
    Q_D(const QQuickVisualDataModel);
    if (!d->m_delegate)
        return 0;
    return d->m_compositor.count(d->m_compositorGroup);
}

void QQuickVisualDataModelPrivate::destroy(QObject *object)
{
    QObjectPrivate *p = QObjectPrivate::get(object);
    Q_ASSERT(p->declarativeData);
    QQmlData *data = static_cast<QQmlData*>(p->declarativeData);
    if (data->ownContext && data->context)
        data->context->clearContext();
    object->deleteLater();
}

QQuickVisualDataModel::ReleaseFlags QQuickVisualDataModelPrivate::release(QObject *object)
{
    QQuickVisualDataModel::ReleaseFlags stat = 0;
    if (!object)
        return stat;

    if (QQuickVisualDataModelAttached *attached = QQuickVisualDataModelAttached::properties(object)) {
        QQuickVisualDataModelItem *cacheItem = attached->m_cacheItem;
        if (cacheItem->releaseObject()) {
            destroy(object);
            if (QQuickItem *item = qobject_cast<QQuickItem *>(object))
                emitDestroyingItem(item);
            cacheItem->object = 0;
            if (cacheItem->incubationTask) {
                releaseIncubator(cacheItem->incubationTask);
                cacheItem->incubationTask = 0;
            }
            stat |= QQuickVisualModel::Destroyed;
        } else {
            stat |= QQuickVisualDataModel::Referenced;
        }
    }
    return stat;
}

/*
  Returns ReleaseStatus flags.
*/

QQuickVisualDataModel::ReleaseFlags QQuickVisualDataModel::release(QQuickItem *item)
{
    Q_D(QQuickVisualDataModel);
    QQuickVisualModel::ReleaseFlags stat = d->release(item);
    if (stat & Destroyed)
        item->setParentItem(0);
    return stat;
}

void QQuickVisualDataModelPrivate::group_append(
        QQmlListProperty<QQuickVisualDataGroup> *property, QQuickVisualDataGroup *group)
{
    QQuickVisualDataModelPrivate *d = static_cast<QQuickVisualDataModelPrivate *>(property->data);
    if (d->m_complete)
        return;
    if (d->m_groupCount == Compositor::MaximumGroupCount) {
        qmlInfo(d->q_func()) << QQuickVisualDataModel::tr("The maximum number of supported VisualDataGroups is 8");
        return;
    }
    d->m_groups[d->m_groupCount] = group;
    d->m_groupCount += 1;
}

int QQuickVisualDataModelPrivate::group_count(
        QQmlListProperty<QQuickVisualDataGroup> *property)
{
    QQuickVisualDataModelPrivate *d = static_cast<QQuickVisualDataModelPrivate *>(property->data);
    return d->m_groupCount - 1;
}

QQuickVisualDataGroup *QQuickVisualDataModelPrivate::group_at(
        QQmlListProperty<QQuickVisualDataGroup> *property, int index)
{
    QQuickVisualDataModelPrivate *d = static_cast<QQuickVisualDataModelPrivate *>(property->data);
    return index >= 0 && index < d->m_groupCount - 1
            ? d->m_groups[index + 1]
            : 0;
}

/*!
    \qmlproperty list<VisualDataGroup> QtQuick2::VisualDataModel::groups

    This property holds a visual data model's group definitions.

    Groups define a sub-set of the items in a visual data model and can be used to filter
    a model.

    For every group defined in a VisualDataModel two attached properties are added to each
    delegate item.  The first of the form VisualDataModel.in\e{GroupName} holds whether the
    item belongs to the group and the second VisualDataModel.\e{groupName}Index holds the
    index of the item in that group.

    The following example illustrates using groups to select items in a model.

    \snippet doc/src/snippets/qml/visualdatagroup.qml 0
*/

QQmlListProperty<QQuickVisualDataGroup> QQuickVisualDataModel::groups()
{
    Q_D(QQuickVisualDataModel);
    return QQmlListProperty<QQuickVisualDataGroup>(
            this,
            d,
            QQuickVisualDataModelPrivate::group_append,
            QQuickVisualDataModelPrivate::group_count,
            QQuickVisualDataModelPrivate::group_at);
}

/*!
    \qmlproperty VisualDataGroup QtQuick2::VisualDataModel::items

    This property holds visual data model's default group to which all new items are added.
*/

QQuickVisualDataGroup *QQuickVisualDataModel::items()
{
    Q_D(QQuickVisualDataModel);
    return d->m_items;
}

/*!
    \qmlproperty VisualDataGroup QtQuick2::VisualDataModel::persistedItems

    This property holds visual data model's persisted items group.

    Items in this group are not destroyed when released by a view, instead they are persisted
    until removed from the group.

    An item can be removed from the persistedItems group by setting the
    VisualDataModel.inPersistedItems property to false.  If the item is not referenced by a view
    at that time it will be destroyed.  Adding an item to this group will not create a new
    instance.

    Items returned by the \l QtQuick2::VisualDataGroup::create() function are automatically added
    to this group.
*/

QQuickVisualDataGroup *QQuickVisualDataModel::persistedItems()
{
    Q_D(QQuickVisualDataModel);
    return d->m_persistedItems;
}

/*!
    \qmlproperty string QtQuick2::VisualDataModel::filterOnGroup

    This property holds the name of the group used to filter the visual data model.

    Only items which belong to this group are visible to a view.

    By default this is the \l items group.
*/

QString QQuickVisualDataModel::filterGroup() const
{
    Q_D(const QQuickVisualDataModel);
    return d->m_filterGroup;
}

void QQuickVisualDataModel::setFilterGroup(const QString &group)
{
    Q_D(QQuickVisualDataModel);

    if (d->m_transaction) {
        qmlInfo(this) << tr("The group of a VisualDataModel cannot be changed within onChanged");
        return;
    }

    if (d->m_filterGroup != group) {
        d->m_filterGroup = group;
        d->updateFilterGroup();
        emit filterGroupChanged();
    }
}

void QQuickVisualDataModel::resetFilterGroup()
{
    setFilterGroup(QStringLiteral("items"));
}

void QQuickVisualDataModelPrivate::updateFilterGroup()
{
    Q_Q(QQuickVisualDataModel);
    if (!m_cacheMetaType)
        return;

    QQuickListCompositor::Group previousGroup = m_compositorGroup;
    m_compositorGroup = Compositor::Default;
    for (int i = 1; i < m_groupCount; ++i) {
        if (m_filterGroup == m_cacheMetaType->groupNames.at(i - 1)) {
            m_compositorGroup = Compositor::Group(i);
            break;
        }
    }

    QQuickVisualDataGroupPrivate::get(m_groups[m_compositorGroup])->emitters.insert(this);
    if (m_compositorGroup != previousGroup) {
        QVector<QQuickChangeSet::Remove> removes;
        QVector<QQuickChangeSet::Insert> inserts;
        m_compositor.transition(previousGroup, m_compositorGroup, &removes, &inserts);

        QQuickChangeSet changeSet;
        changeSet.apply(removes, inserts);
        emit q->modelUpdated(changeSet, false);

        if (changeSet.difference() != 0)
            emit q->countChanged();

        if (m_parts) {
            foreach (QQuickVisualPartsModel *model, m_parts->models)
                model->updateFilterGroup(m_compositorGroup, changeSet);
        }
    }
}

/*!
    \qmlproperty object QtQuick2::VisualDataModel::parts

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

QObject *QQuickVisualDataModel::parts()
{
    Q_D(QQuickVisualDataModel);
    if (!d->m_parts)
        d->m_parts = new QQuickVisualDataModelParts(this);
    return d->m_parts;
}

void QQuickVisualDataModelPrivate::emitCreatedPackage(QQuickVisualDataModelItem *cacheItem, QQuickPackage *package)
{
    for (int i = 1; i < m_groupCount; ++i)
        QQuickVisualDataGroupPrivate::get(m_groups[i])->createdPackage(cacheItem->index[i], package);
}

void QQuickVisualDataModelPrivate::emitInitPackage(QQuickVisualDataModelItem *cacheItem, QQuickPackage *package)
{
    for (int i = 1; i < m_groupCount; ++i)
        QQuickVisualDataGroupPrivate::get(m_groups[i])->initPackage(cacheItem->index[i], package);
}

void QQuickVisualDataModelPrivate::emitDestroyingPackage(QQuickPackage *package)
{
    for (int i = 1; i < m_groupCount; ++i)
        QQuickVisualDataGroupPrivate::get(m_groups[i])->destroyingPackage(package);
}

void QVDMIncubationTask::statusChanged(Status status)
{
    vdm->incubatorStatusChanged(this, status);
}

void QQuickVisualDataModelPrivate::releaseIncubator(QVDMIncubationTask *incubationTask)
{
    Q_Q(QQuickVisualDataModel);
    if (!incubationTask->isError())
        incubationTask->clear();
    m_finishedIncubating.append(incubationTask);
    if (!m_incubatorCleanupScheduled) {
        m_incubatorCleanupScheduled = true;
        QCoreApplication::postEvent(q, new QEvent(QEvent::User));
    }
}

void QQuickVisualDataModelPrivate::incubatorStatusChanged(QVDMIncubationTask *incubationTask, QQmlIncubator::Status status)
{
    Q_Q(QQuickVisualDataModel);
    if (status != QQmlIncubator::Ready && status != QQmlIncubator::Error)
        return;

    QQuickVisualDataModelItem *cacheItem = incubationTask->incubating;
    cacheItem->incubationTask = 0;

    if (status == QQmlIncubator::Ready) {
        incubationTask->incubating = 0;
        releaseIncubator(incubationTask);
        if (QQuickPackage *package = qobject_cast<QQuickPackage *>(cacheItem->object))
            emitCreatedPackage(cacheItem, package);
        else if (QQuickItem *item = qobject_cast<QQuickItem *>(cacheItem->object))
            emitCreatedItem(cacheItem, item);
    } else if (status == QQmlIncubator::Error) {
        delete incubationTask->incubatingContext;
        incubationTask->incubatingContext = 0;
        if (!cacheItem->isReferenced()) {
            int cidx = m_cache.indexOf(cacheItem);
            m_compositor.clearFlags(Compositor::Cache, cidx, 1, Compositor::CacheFlag);
            m_cache.removeAt(cidx);
            delete cacheItem;
            Q_ASSERT(m_cache.count() == m_compositor.count(Compositor::Cache));
        }
        releaseIncubator(incubationTask);
        qmlInfo(q, m_delegate->errors()) << "Error creating delegate";
    }
}

void QVDMIncubationTask::setInitialState(QObject *o)
{
    vdm->setInitialState(this, o);
}

void QQuickVisualDataModelPrivate::setInitialState(QVDMIncubationTask *incubationTask, QObject *o)
{
    QQuickVisualDataModelItem *cacheItem = incubationTask->incubating;
    cacheItem->object = o;
    QQml_setParent_noEvent(incubationTask->incubatingContext, cacheItem->object);
    incubationTask->incubatingContext = 0;

    cacheItem->attached = QQuickVisualDataModelAttached::properties(cacheItem->object);
    cacheItem->attached->setCacheItem(cacheItem);
    new QQuickVisualDataModelAttachedMetaObject(cacheItem->attached, m_cacheMetaType);
    cacheItem->attached->emitChanges();

    if (QQuickPackage *package = qobject_cast<QQuickPackage *>(cacheItem->object))
        emitInitPackage(cacheItem, package);
    else if (QQuickItem *item = qobject_cast<QQuickItem *>(cacheItem->object))
        emitInitItem(cacheItem, item);
}

QObject *QQuickVisualDataModelPrivate::object(Compositor::Group group, int index, bool asynchronous, bool reference)
{
    Q_Q(QQuickVisualDataModel);
    if (!m_delegate || index < 0 || index >= m_compositor.count(group)) {
        qWarning() << "VisualDataModel::item: index out range" << index << m_compositor.count(group);
        return 0;
    }

    Compositor::iterator it = m_compositor.find(group, index);

    QQuickVisualDataModelItem *cacheItem = it->inCache() ? m_cache.at(it.cacheIndex) : 0;

    if (!cacheItem) {
        cacheItem = m_adaptorModel->createItem(m_cacheMetaType, it.modelIndex());
        for (int i = 1; i < m_groupCount; ++i)
            cacheItem->index[i] = it.index[i];

        cacheItem->groups = it->flags;

        m_cache.insert(it.cacheIndex, cacheItem);
        m_compositor.setFlags(it, 1, Compositor::CacheFlag);
        Q_ASSERT(m_cache.count() == m_compositor.count(Compositor::Cache));
    }

    if (cacheItem->incubationTask) {
        if (!asynchronous) {
            // previously requested async - now needed immediately
            cacheItem->incubationTask->forceCompletion();
        }
    } else if (!cacheItem->object) {
        QVDMIncubationTask *incubator = new QVDMIncubationTask(this, asynchronous ? QQmlIncubator::Asynchronous : QQmlIncubator::AsynchronousIfNested);
        cacheItem->incubationTask = incubator;

        QQmlContext *creationContext = m_delegate->creationContext();
        QQmlContext *rootContext = new QQuickVisualDataModelContext(
                cacheItem, creationContext ? creationContext : m_context.data());
        QQmlContext *ctxt = rootContext;
        if (m_adaptorModel->flags() & QQuickVisualAdaptorModel::ProxiedObject) {
            if (QQuickVisualAdaptorModelProxyInterface *proxy = qobject_cast<QQuickVisualAdaptorModelProxyInterface *>(cacheItem)) {
                ctxt->setContextObject(proxy->proxiedObject());
                ctxt = new QQuickVisualDataModelContext(cacheItem, ctxt, ctxt);
            }
        }

        ctxt->setContextObject(cacheItem);

        incubator->incubating = cacheItem;
        incubator->incubatingContext = rootContext;
        m_delegate->create(*incubator, ctxt, m_context);
    }

    if (index == m_compositor.count(group) - 1 && m_adaptorModel->canFetchMore())
        QCoreApplication::postEvent(q, new QEvent(QEvent::UpdateRequest));
    if (cacheItem->object && reference)
        cacheItem->referenceObject();
    return cacheItem->object;
}

/*
  If asynchronous is true or the component is being loaded asynchronously due
  to an ancestor being loaded asynchronously, item() may return 0.  In this
  case itemCreated() will be emitted when the item is available.  The item
  at this stage does not have any references, so item() must be called again
  to ensure a reference is held.  Any call to item() which returns a valid item
  must be matched by a call to release() in order to destroy the item.
*/
QQuickItem *QQuickVisualDataModel::item(int index, bool asynchronous)
{
    Q_D(QQuickVisualDataModel);
    if (!d->m_delegate || index < 0 || index >= d->m_compositor.count(d->m_compositorGroup)) {
        qWarning() << "VisualDataModel::item: index out range" << index << d->m_compositor.count(d->m_compositorGroup);
        return 0;
    }

    QObject *object = d->object(d->m_compositorGroup, index, asynchronous, true);
    if (!object)
        return 0;

    if (QQuickItem *item = qobject_cast<QQuickItem *>(object))
        return item;

    d->release(object);
    if (!d->m_delegateValidated) {
        if (object)
            qmlInfo(d->m_delegate) << QQuickVisualDataModel::tr("Delegate component must be Item type.");
        d->m_delegateValidated = true;
    }
    return 0;
}

QString QQuickVisualDataModelPrivate::stringValue(Compositor::Group group, int index, const QString &name)
{
    Compositor::iterator it = m_compositor.find(group, index);
    if (QQuickVisualAdaptorModel *model = it.list<QQuickVisualAdaptorModel>()) {
        return model->stringValue(it.modelIndex(), name);
    }
    return QString();
}

QString QQuickVisualDataModel::stringValue(int index, const QString &name)
{
    Q_D(QQuickVisualDataModel);
    return d->stringValue(d->m_compositorGroup, index, name);
}

int QQuickVisualDataModel::indexOf(QQuickItem *item, QObject *) const
{
    Q_D(const QQuickVisualDataModel);
    if (QQuickVisualDataModelAttached *attached = QQuickVisualDataModelAttached::properties(item))
        return attached->m_cacheItem->index[d->m_compositorGroup];
    return -1;
}

void QQuickVisualDataModel::setWatchedRoles(QList<QByteArray> roles)
{
    Q_D(QQuickVisualDataModel);
    d->m_adaptorModel->replaceWatchedRoles(d->watchedRoles, roles);
    d->watchedRoles = roles;
}

void QQuickVisualDataModelPrivate::addGroups(
        Compositor::iterator from, int count, Compositor::Group group, int groupFlags)
{
    QVector<Compositor::Insert> inserts;
    m_compositor.setFlags(from, count, group, groupFlags, &inserts);
    itemsInserted(inserts);
    emitChanges();
}

void QQuickVisualDataModelPrivate::removeGroups(
        Compositor::iterator from, int count, Compositor::Group group, int groupFlags)
{
    QVector<Compositor::Remove> removes;
    m_compositor.clearFlags(from, count, group, groupFlags, &removes);
    itemsRemoved(removes);
    emitChanges();
}

void QQuickVisualDataModelPrivate::setGroups(
        Compositor::iterator from, int count, Compositor::Group group, int groupFlags)
{
    QVector<Compositor::Remove> removes;
    QVector<Compositor::Insert> inserts;

    m_compositor.setFlags(from, count, group, groupFlags, &inserts);
    itemsInserted(inserts);
    const int removeFlags = ~groupFlags & Compositor::GroupMask;

    from = m_compositor.find(from.group, from.index[from.group]);
    m_compositor.clearFlags(from, count, group, removeFlags, &removes);
    itemsRemoved(removes);
    emitChanges();
}

bool QQuickVisualDataModel::event(QEvent *e)
{
    Q_D(QQuickVisualDataModel);
    if (e->type() == QEvent::UpdateRequest) {
        d->m_adaptorModel->fetchMore();
    } else if (e->type() == QEvent::User) {
        d->m_incubatorCleanupScheduled = false;
        qDeleteAll(d->m_finishedIncubating);
        d->m_finishedIncubating.clear();
    }
    return QQuickVisualModel::event(e);
}

void QQuickVisualDataModelPrivate::itemsChanged(const QVector<Compositor::Change> &changes)
{
    if (!m_delegate)
        return;

    QVarLengthArray<QVector<QQuickChangeSet::Change>, Compositor::MaximumGroupCount> translatedChanges(m_groupCount);

    foreach (const Compositor::Change &change, changes) {
        for (int i = 1; i < m_groupCount; ++i) {
            if (change.inGroup(i)) {
                translatedChanges[i].append(
                        QQuickChangeSet::Change(change.index[i], change.count));
            }
        }
    }

    for (int i = 1; i < m_groupCount; ++i)
        QQuickVisualDataGroupPrivate::get(m_groups[i])->changeSet.apply(translatedChanges.at(i));
}

void QQuickVisualDataModel::_q_itemsChanged(int index, int count)
{
    Q_D(QQuickVisualDataModel);
    if (count <= 0)
        return;
    QVector<Compositor::Change> changes;
    d->m_compositor.listItemsChanged(d->m_adaptorModel, index, count, &changes);
    d->itemsChanged(changes);
    d->emitChanges();
}

void QQuickVisualDataModelPrivate::itemsInserted(
        const QVector<Compositor::Insert> &inserts,
        QVarLengthArray<QVector<QQuickChangeSet::Insert>, Compositor::MaximumGroupCount> *translatedInserts,
        QHash<int, QList<QQuickVisualDataModelItem *> > *movedItems)
{
    int cacheIndex = 0;

    int inserted[Compositor::MaximumGroupCount];
    for (int i = 1; i < m_groupCount; ++i)
        inserted[i] = 0;

    foreach (const Compositor::Insert &insert, inserts) {
        for (; cacheIndex < insert.cacheIndex; ++cacheIndex) {
            QQuickVisualDataModelItem *cacheItem = m_cache.at(cacheIndex);
            for (int i = 1; i < m_groupCount; ++i)
                cacheItem->index[i] += inserted[i];
        }
        for (int i = 1; i < m_groupCount; ++i) {
            if (insert.inGroup(i)) {
                (*translatedInserts)[i].append(
                        QQuickChangeSet::Insert(insert.index[i], insert.count, insert.moveId));
                inserted[i] += insert.count;
            }
        }

        if (!insert.inCache())
            continue;

        if (movedItems && insert.isMove()) {
            QList<QQuickVisualDataModelItem *> items = movedItems->take(insert.moveId);
            Q_ASSERT(items.count() == insert.count);
            m_cache = m_cache.mid(0, insert.cacheIndex) + items + m_cache.mid(insert.cacheIndex);
        }
        if (insert.inGroup()) {
            for (int offset = 0; cacheIndex < insert.cacheIndex + insert.count; ++cacheIndex, ++offset) {
                QQuickVisualDataModelItem *cacheItem = m_cache.at(cacheIndex);
                cacheItem->groups |= insert.flags & Compositor::GroupMask;
                for (int i = 1; i < m_groupCount; ++i) {
                    cacheItem->index[i] = cacheItem->groups & (1 << i)
                            ? insert.index[i] + offset
                            : insert.index[i];
                }
            }
        } else {
            cacheIndex = insert.cacheIndex + insert.count;
        }
    }
    for (; cacheIndex < m_cache.count(); ++cacheIndex) {
        QQuickVisualDataModelItem *cacheItem = m_cache.at(cacheIndex);
        for (int i = 1; i < m_groupCount; ++i)
            cacheItem->index[i] += inserted[i];
    }
}

void QQuickVisualDataModelPrivate::itemsInserted(const QVector<Compositor::Insert> &inserts)
{
    QVarLengthArray<QVector<QQuickChangeSet::Insert>, Compositor::MaximumGroupCount> translatedInserts(m_groupCount);
    itemsInserted(inserts, &translatedInserts);
    Q_ASSERT(m_cache.count() == m_compositor.count(Compositor::Cache));
    if (!m_delegate)
        return;

    for (int i = 1; i < m_groupCount; ++i)
        QQuickVisualDataGroupPrivate::get(m_groups[i])->changeSet.apply(translatedInserts.at(i));
}

void QQuickVisualDataModel::_q_itemsInserted(int index, int count)
{

    Q_D(QQuickVisualDataModel);
    if (count <= 0)
        return;
    QVector<Compositor::Insert> inserts;
    d->m_compositor.listItemsInserted(d->m_adaptorModel, index, count, &inserts);
    d->itemsInserted(inserts);
    d->emitChanges();
}

void QQuickVisualDataModelPrivate::itemsRemoved(
        const QVector<Compositor::Remove> &removes,
        QVarLengthArray<QVector<QQuickChangeSet::Remove>, Compositor::MaximumGroupCount> *translatedRemoves,
        QHash<int, QList<QQuickVisualDataModelItem *> > *movedItems)
{
    int cacheIndex = 0;
    int removedCache = 0;

    int removed[Compositor::MaximumGroupCount];
    for (int i = 1; i < m_groupCount; ++i)
        removed[i] = 0;

    foreach (const Compositor::Remove &remove, removes) {
        for (; cacheIndex < remove.cacheIndex; ++cacheIndex) {
            QQuickVisualDataModelItem *cacheItem = m_cache.at(cacheIndex);
            for (int i = 1; i < m_groupCount; ++i)
                cacheItem->index[i] -= removed[i];
        }
        for (int i = 1; i < m_groupCount; ++i) {
            if (remove.inGroup(i)) {
                (*translatedRemoves)[i].append(
                        QQuickChangeSet::Remove(remove.index[i], remove.count, remove.moveId));
                removed[i] += remove.count;
            }
        }

        if (!remove.inCache())
            continue;

        if (movedItems && remove.isMove()) {
            movedItems->insert(remove.moveId, m_cache.mid(remove.cacheIndex, remove.count));
            QList<QQuickVisualDataModelItem *>::iterator begin = m_cache.begin() + remove.cacheIndex;
            QList<QQuickVisualDataModelItem *>::iterator end = begin + remove.count;
            m_cache.erase(begin, end);
        } else {
            for (; cacheIndex < remove.cacheIndex + remove.count - removedCache; ++cacheIndex) {
                QQuickVisualDataModelItem *cacheItem = m_cache.at(cacheIndex);
                if (remove.inGroup(Compositor::Persisted) && cacheItem->objectRef == 0 && cacheItem->object) {
                    destroy(cacheItem->object);
                    if (QQuickPackage *package = qobject_cast<QQuickPackage *>(cacheItem->object))
                        emitDestroyingPackage(package);
                    else if (QQuickItem *item = qobject_cast<QQuickItem *>(cacheItem->object))
                        emitDestroyingItem(item);
                    cacheItem->object = 0;
                }
                if (!cacheItem->isReferenced()) {
                    m_compositor.clearFlags(Compositor::Cache, cacheIndex, 1, Compositor::CacheFlag);
                    m_cache.removeAt(cacheIndex);
                    delete cacheItem;
                    --cacheIndex;
                    ++removedCache;
                    Q_ASSERT(m_cache.count() == m_compositor.count(Compositor::Cache));
                } else if (remove.groups() == cacheItem->groups) {
                    cacheItem->groups = 0;
                    for (int i = 1; i < m_groupCount; ++i)
                        cacheItem->index[i] = -1;
                } else {
                    for (int i = 1; i < m_groupCount; ++i) {
                        if (remove.inGroup(i))
                            cacheItem->index[i] = remove.index[i];
                    }
                    cacheItem->groups &= ~remove.flags;
                }
            }
        }
    }

    for (; cacheIndex < m_cache.count(); ++cacheIndex) {
        QQuickVisualDataModelItem *cacheItem = m_cache.at(cacheIndex);
        for (int i = 1; i < m_groupCount; ++i)
            cacheItem->index[i] -= removed[i];
    }
}

void QQuickVisualDataModelPrivate::itemsRemoved(const QVector<Compositor::Remove> &removes)
{
    QVarLengthArray<QVector<QQuickChangeSet::Remove>, Compositor::MaximumGroupCount> translatedRemoves(m_groupCount);
    itemsRemoved(removes, &translatedRemoves);
    Q_ASSERT(m_cache.count() == m_compositor.count(Compositor::Cache));
    if (!m_delegate)
        return;

    for (int i = 1; i < m_groupCount; ++i)
       QQuickVisualDataGroupPrivate::get(m_groups[i])->changeSet.apply(translatedRemoves.at(i));
}

void QQuickVisualDataModel::_q_itemsRemoved(int index, int count)
{
    Q_D(QQuickVisualDataModel);
    if (count <= 0)
        return;

    QVector<Compositor::Remove> removes;
    d->m_compositor.listItemsRemoved(d->m_adaptorModel, index, count, &removes);
    d->itemsRemoved(removes);

    d->emitChanges();
}

void QQuickVisualDataModelPrivate::itemsMoved(
        const QVector<Compositor::Remove> &removes, const QVector<Compositor::Insert> &inserts)
{
    QHash<int, QList<QQuickVisualDataModelItem *> > movedItems;

    QVarLengthArray<QVector<QQuickChangeSet::Remove>, Compositor::MaximumGroupCount> translatedRemoves(m_groupCount);
    itemsRemoved(removes, &translatedRemoves, &movedItems);

    QVarLengthArray<QVector<QQuickChangeSet::Insert>, Compositor::MaximumGroupCount> translatedInserts(m_groupCount);
    itemsInserted(inserts, &translatedInserts, &movedItems);
    Q_ASSERT(m_cache.count() == m_compositor.count(Compositor::Cache));
    Q_ASSERT(movedItems.isEmpty());
    if (!m_delegate)
        return;

    for (int i = 1; i < m_groupCount; ++i) {
        QQuickVisualDataGroupPrivate::get(m_groups[i])->changeSet.apply(
                    translatedRemoves.at(i),
                    translatedInserts.at(i));
    }
}

void QQuickVisualDataModel::_q_itemsMoved(int from, int to, int count)
{
    Q_D(QQuickVisualDataModel);
    if (count <= 0)
        return;

    QVector<Compositor::Remove> removes;
    QVector<Compositor::Insert> inserts;
    d->m_compositor.listItemsMoved(d->m_adaptorModel, from, to, count, &removes, &inserts);
    d->itemsMoved(removes, inserts);
    d->emitChanges();
}

template <typename T> v8::Local<v8::Array>
QQuickVisualDataModelPrivate::buildChangeList(const QVector<T> &changes)
{
    v8::Local<v8::Array> indexes = v8::Array::New(changes.count());
    v8::Local<v8::String> indexKey = v8::String::New("index");
    v8::Local<v8::String> countKey = v8::String::New("count");
    v8::Local<v8::String> moveIdKey = v8::String::New("moveId");

    for (int i = 0; i < changes.count(); ++i) {
        v8::Local<v8::Object> object = v8::Object::New();
        object->Set(indexKey, v8::Integer::New(changes.at(i).index));
        object->Set(countKey, v8::Integer::New(changes.at(i).count));
        object->Set(moveIdKey, changes.at(i).moveId != -1 ? v8::Integer::New(changes.at(i).count) : v8::Undefined());
        indexes->Set(i, object);
    }
    return indexes;
}

void QQuickVisualDataModelPrivate::emitModelUpdated(const QQuickChangeSet &changeSet, bool reset)
{
    Q_Q(QQuickVisualDataModel);
    emit q->modelUpdated(changeSet, reset);
    if (changeSet.difference() != 0)
        emit q->countChanged();
}

void QQuickVisualDataModelPrivate::emitChanges()
{
    if (m_transaction || !m_complete)
        return;

    m_transaction = true;
    QV8Engine *engine = QQmlEnginePrivate::getV8Engine(m_context->engine());
    for (int i = 1; i < m_groupCount; ++i)
        QQuickVisualDataGroupPrivate::get(m_groups[i])->emitChanges(engine);
    m_transaction = false;

    const bool reset = m_reset;
    m_reset = false;
    for (int i = 1; i < m_groupCount; ++i)
        QQuickVisualDataGroupPrivate::get(m_groups[i])->emitModelUpdated(reset);

    foreach (QQuickVisualDataModelItem *cacheItem, m_cache) {
        if (cacheItem->object && cacheItem->attached)
            cacheItem->attached->emitChanges();
    }
}

void QQuickVisualDataModel::_q_modelReset(int oldCount, int newCount)
{
    Q_D(QQuickVisualDataModel);
    if (!d->m_delegate)
        return;

    QVector<Compositor::Remove> removes;
    QVector<Compositor::Insert> inserts;
    if (oldCount)
        d->m_compositor.listItemsRemoved(d->m_adaptorModel, 0, oldCount, &removes);
    if (newCount)
        d->m_compositor.listItemsInserted(d->m_adaptorModel, 0, newCount, &inserts);
    d->itemsMoved(removes, inserts);
    d->m_reset = true;
    d->emitChanges();
}

QQuickVisualDataModelAttached *QQuickVisualDataModel::qmlAttachedProperties(QObject *obj)
{
    return QQuickVisualDataModelAttached::properties(obj);
}

bool QQuickVisualDataModelPrivate::insert(
        Compositor::insert_iterator &before, const v8::Local<v8::Object> &object, int groups)
{
    QQuickVisualDataModelItem *cacheItem = m_adaptorModel->createItem(m_cacheMetaType, -1);
    if (!cacheItem)
        return false;

    for (int i = 1; i < m_groupCount; ++i)
        cacheItem->index[i] = before.index[i];

    v8::Local<v8::Array> propertyNames = object->GetPropertyNames();
    for (uint i = 0; i < propertyNames->Length(); ++i) {
        v8::Local<v8::String> propertyName = propertyNames->Get(i)->ToString();
        cacheItem->setValue(
                m_cacheMetaType->v8Engine->toString(propertyName),
                m_cacheMetaType->v8Engine->toVariant(object->Get(propertyName), QVariant::Invalid));
    }

    cacheItem->groups = groups | Compositor::UnresolvedFlag | Compositor::CacheFlag;

    // Must be before the new object is inserted into the cache or its indexes will be adjusted too.
    itemsInserted(QVector<Compositor::Insert>() << Compositor::Insert(before, 1, cacheItem->groups & ~Compositor::CacheFlag));

    before = m_compositor.insert(before, 0, 0, 1, cacheItem->groups);
    m_cache.insert(before.cacheIndex, cacheItem);

    return true;
}

//============================================================================

QQuickVisualDataModelItemMetaType::QQuickVisualDataModelItemMetaType(
        QV8Engine *engine, QQuickVisualDataModel *model, const QStringList &groupNames)
    : model(model)
    , groupCount(groupNames.count() + 1)
    , memberPropertyOffset(QQuickVisualDataModelAttached::staticMetaObject.propertyCount())
    , indexPropertyOffset(QQuickVisualDataModelAttached::staticMetaObject.propertyCount() + groupNames.count())
    , v8Engine(engine)
    , metaObject(0)
    , groupNames(groupNames)
{
    QMetaObjectBuilder builder;
    builder.setFlags(QMetaObjectBuilder::DynamicMetaObject);
    builder.setClassName(QQuickVisualDataModelAttached::staticMetaObject.className());
    builder.setSuperClass(&QQuickVisualDataModelAttached::staticMetaObject);

    v8::HandleScope handleScope;
    v8::Context::Scope contextScope(engine->context());
    v8::Local<v8::FunctionTemplate> ft = v8::FunctionTemplate::New();
    ft->InstanceTemplate()->SetHasExternalResource(true);
    ft->PrototypeTemplate()->SetAccessor(v8::String::New("model"), get_model);
    ft->PrototypeTemplate()->SetAccessor(v8::String::New("groups"), get_groups, set_groups);
    ft->PrototypeTemplate()->SetAccessor(v8::String::New("isUnresolved"), get_member, 0, v8::Int32::New(30));

    int notifierId = 0;
    for (int i = 0; i < groupNames.count(); ++i, ++notifierId) {
        QString propertyName = QStringLiteral("in") + groupNames.at(i);
        propertyName.replace(2, 1, propertyName.at(2).toUpper());
        builder.addSignal("__" + propertyName.toUtf8() + "Changed()");
        QMetaPropertyBuilder propertyBuilder = builder.addProperty(
                propertyName.toUtf8(), "bool", notifierId);
        propertyBuilder.setWritable(true);

        ft->PrototypeTemplate()->SetAccessor(
                engine->toString(propertyName), get_member, set_member, v8::Int32::New(i + 1));
    }
    for (int i = 0; i < groupNames.count(); ++i, ++notifierId) {
        const QString propertyName = groupNames.at(i) + QStringLiteral("Index");
        builder.addSignal("__" + propertyName.toUtf8() + "Changed()");
        QMetaPropertyBuilder propertyBuilder = builder.addProperty(
                propertyName.toUtf8(), "int", notifierId);
        propertyBuilder.setWritable(true);

        ft->PrototypeTemplate()->SetAccessor(
                engine->toString(propertyName), get_index, 0, v8::Int32::New(i + 1));
    }

    metaObject = builder.toMetaObject();

    constructor = qPersistentNew<v8::Function>(ft->GetFunction());
}

QQuickVisualDataModelItemMetaType::~QQuickVisualDataModelItemMetaType()
{
    free(metaObject);
    qPersistentDispose(constructor);
}

int QQuickVisualDataModelItemMetaType::parseGroups(const QStringList &groups) const
{
    int groupFlags = 0;
    foreach (const QString &groupName, groups) {
        int index = groupNames.indexOf(groupName);
        if (index != -1)
            groupFlags |= 2 << index;
    }
    return groupFlags;
}

int QQuickVisualDataModelItemMetaType::parseGroups(const v8::Local<v8::Value> &groups) const
{
    int groupFlags = 0;
    if (groups->IsString()) {
        const QString groupName = v8Engine->toString(groups);
        int index = groupNames.indexOf(groupName);
        if (index != -1)
            groupFlags |= 2 << index;
    } else if (groups->IsArray()) {
        v8::Local<v8::Array> array = v8::Local<v8::Array>::Cast(groups);
        for (uint i = 0; i < array->Length(); ++i) {
            const QString groupName = v8Engine->toString(array->Get(i));
            int index = groupNames.indexOf(groupName);
            if (index != -1)
                groupFlags |= 2 << index;
        }
    }
    return groupFlags;
}

void QQuickVisualDataModelItemMetaType::release_index(v8::Persistent<v8::Value> object, void *data)
{
    static_cast<QQuickVisualDataModelItem *>(data)->indexHandle.Clear();
    qPersistentDispose(object);
}

void QQuickVisualDataModelItemMetaType::release_model(v8::Persistent<v8::Value> object, void *data)
{
    static_cast<QQuickVisualDataModelItem *>(data)->modelHandle.Clear();
    qPersistentDispose(object);
}

v8::Handle<v8::Value> QQuickVisualDataModelItemMetaType::get_model(
        v8::Local<v8::String>, const v8::AccessorInfo &info)
{
    QQuickVisualDataModelItem *cacheItem = v8_resource_cast<QQuickVisualDataModelItem>(info.This());
    if (!cacheItem)
        V8THROW_ERROR("Not a valid VisualData object");
    if (!cacheItem->metaType->model)
        return v8::Undefined();

    if (cacheItem->modelHandle.IsEmpty()) {
        cacheItem->modelHandle = qPersistentNew(cacheItem->get());
        cacheItem->modelHandle.MakeWeak(cacheItem, &release_model);

        ++cacheItem->scriptRef;
    }

    return cacheItem->modelHandle;
}

v8::Handle<v8::Value> QQuickVisualDataModelItemMetaType::get_groups(
        v8::Local<v8::String>, const v8::AccessorInfo &info)
{
    QQuickVisualDataModelItem *cacheItem = v8_resource_cast<QQuickVisualDataModelItem>(info.This());
    if (!cacheItem)
        V8THROW_ERROR("Not a valid VisualData object");

    QStringList groups;
    for (int i = 1; i < cacheItem->metaType->groupCount; ++i) {
        if (cacheItem->groups & (1 << i))
            groups.append(cacheItem->metaType->groupNames.at(i - 1));
    }

    return cacheItem->engine->fromVariant(groups);
}

void QQuickVisualDataModelItemMetaType::set_groups(
        v8::Local<v8::String>, v8::Local<v8::Value> value, const v8::AccessorInfo &info)
{
    QQuickVisualDataModelItem *cacheItem = v8_resource_cast<QQuickVisualDataModelItem>(info.This());
    if (!cacheItem)
        V8THROW_ERROR_SETTER("Not a valid VisualData object");

    if (!cacheItem->metaType->model)
        return;
    QQuickVisualDataModelPrivate *model = QQuickVisualDataModelPrivate::get(cacheItem->metaType->model);

    const int groupFlags = model->m_cacheMetaType->parseGroups(value);
    for (int i = 1; i < cacheItem->metaType->groupCount; ++i) {
        if (cacheItem->groups & (1 << i)) {
            Compositor::iterator it = model->m_compositor.find(Compositor::Group(i), cacheItem->index[i]);
            model->setGroups(it, 1, Compositor::Group(i), groupFlags);
            break;
        }
    }
}

v8::Handle<v8::Value> QQuickVisualDataModelItemMetaType::get_member(
        v8::Local<v8::String>, const v8::AccessorInfo &info)
{
    QQuickVisualDataModelItem *cacheItem = v8_resource_cast<QQuickVisualDataModelItem>(info.This());
    if (!cacheItem)
        V8THROW_ERROR("Not a valid VisualData object");

    return v8::Boolean::New(cacheItem->groups & (1 << info.Data()->Int32Value()));
}

void QQuickVisualDataModelItemMetaType::set_member(
        v8::Local<v8::String>, v8::Local<v8::Value> value, const v8::AccessorInfo &info)
{
    QQuickVisualDataModelItem *cacheItem = v8_resource_cast<QQuickVisualDataModelItem>(info.This());
    if (!cacheItem)
        V8THROW_ERROR_SETTER("Not a valid VisualData object");

    if (!cacheItem->metaType->model)
        return;
    QQuickVisualDataModelPrivate *model = QQuickVisualDataModelPrivate::get(cacheItem->metaType->model);

    Compositor::Group group = Compositor::Group(info.Data()->Int32Value());
    const bool member = value->BooleanValue();
    const int groupFlag = (1 << group);
    if (member == ((cacheItem->groups & groupFlag) != 0))
        return;

    for (int i = 1; i < cacheItem->metaType->groupCount; ++i) {
        if (cacheItem->groups & (1 << i)) {
            Compositor::iterator it = model->m_compositor.find(Compositor::Group(i), cacheItem->index[i]);
            if (member)
                model->addGroups(it, 1, Compositor::Group(i), groupFlag);
            else
                model->removeGroups(it, 1, Compositor::Group(i), groupFlag);
            break;
        }
    }
}

v8::Handle<v8::Value> QQuickVisualDataModelItemMetaType::get_index(
        v8::Local<v8::String>, const v8::AccessorInfo &info)
{
    QQuickVisualDataModelItem *cacheItem = v8_resource_cast<QQuickVisualDataModelItem>(info.This());
    if (!cacheItem)
        V8THROW_ERROR("Not a valid VisualData object");

    return v8::Integer::New(cacheItem->index[info.Data()->Int32Value()]);
}


//---------------------------------------------------------------------------

QQuickVisualDataModelItem::QQuickVisualDataModelItem(QQuickVisualDataModelItemMetaType *metaType, QQuickVisualAdaptorModel *model, int modelIndex)
    : QV8ObjectResource(metaType->v8Engine)
    , metaType(metaType)
    , model(model)
    , object(0)
    , attached(0)
    , objectRef(0)
    , scriptRef(0)
    , groups(0)
    , incubationTask(0)
{
    index[0] = modelIndex;
    metaType->addref();
}

QQuickVisualDataModelItem::~QQuickVisualDataModelItem()
{
    Q_ASSERT(scriptRef == 0);
    Q_ASSERT(objectRef == 0);
    Q_ASSERT(!object);
    Q_ASSERT(indexHandle.IsEmpty());
    Q_ASSERT(modelHandle.IsEmpty());

    if (incubationTask && metaType->model)
        QQuickVisualDataModelPrivate::get(metaType->model)->releaseIncubator(incubationTask);

    metaType->release();

}

void QQuickVisualDataModelItem::Dispose()
{
    --scriptRef;
    if (isReferenced())
        return;

    if (metaType->model) {
        QQuickVisualDataModelPrivate *model = QQuickVisualDataModelPrivate::get(metaType->model);
        const int cacheIndex = model->m_cache.indexOf(this);
        if (cacheIndex != -1) {
            model->m_compositor.clearFlags(Compositor::Cache, cacheIndex, 1, Compositor::CacheFlag);
            model->m_cache.removeAt(cacheIndex);
            Q_ASSERT(model->m_cache.count() == model->m_compositor.count(Compositor::Cache));
        }
    }
    delete this;
}

//---------------------------------------------------------------------------

QQuickVisualDataModelAttachedMetaObject::QQuickVisualDataModelAttachedMetaObject(
        QQuickVisualDataModelAttached *attached, QQuickVisualDataModelItemMetaType *metaType)
    : attached(attached)
    , metaType(metaType)
{
    metaType->addref();
    *static_cast<QMetaObject *>(this) = *metaType->metaObject;
    QObjectPrivate::get(attached)->metaObject = this;
}

QQuickVisualDataModelAttachedMetaObject::~QQuickVisualDataModelAttachedMetaObject()
{
    metaType->release();
}

int QQuickVisualDataModelAttachedMetaObject::metaCall(QMetaObject::Call call, int _id, void **arguments)
{
    if (call == QMetaObject::ReadProperty) {
        if (_id >= metaType->indexPropertyOffset) {
            Compositor::Group group = Compositor::Group(_id - metaType->indexPropertyOffset + 1);
            *static_cast<int *>(arguments[0]) = attached->m_cacheItem->index[group];
            return -1;
        } else if (_id >= metaType->memberPropertyOffset) {
            Compositor::Group group = Compositor::Group(_id - metaType->memberPropertyOffset + 1);
            *static_cast<bool *>(arguments[0]) = attached->m_cacheItem->groups & (1 << group);
            return -1;
        }
    } else if (call == QMetaObject::WriteProperty) {
        if (_id >= metaType->memberPropertyOffset) {
            if (!metaType->model)
                return -1;
            QQuickVisualDataModelPrivate *model = QQuickVisualDataModelPrivate::get(metaType->model);
            Compositor::Group group = Compositor::Group(_id - metaType->memberPropertyOffset + 1);
            const int groupFlag = 1 << group;
            const bool member = attached->m_cacheItem->groups & groupFlag;
            if (member && !*static_cast<bool *>(arguments[0])) {
                Compositor::iterator it = model->m_compositor.find(
                        group, attached->m_cacheItem->index[group]);
                model->removeGroups(it, 1, group, groupFlag);
            } else if (!member && *static_cast<bool *>(arguments[0])) {
                for (int i = 1; i < metaType->groupCount; ++i) {
                    if (attached->m_cacheItem->groups & (1 << i)) {
                        Compositor::iterator it = model->m_compositor.find(
                                Compositor::Group(i), attached->m_cacheItem->index[i]);
                        model->addGroups(it, 1, Compositor::Group(i), groupFlag);
                        break;
                    }
                }
            }
            return -1;
        }
    }
    return attached->qt_metacall(call, _id, arguments);
}

void QQuickVisualDataModelAttached::setCacheItem(QQuickVisualDataModelItem *item)
{
    m_cacheItem = item;
    for (int i = 1; i < m_cacheItem->metaType->groupCount; ++i)
        m_previousIndex[i] = m_cacheItem->index[i];
}

/*!
    \qmlattachedproperty int QtQuick2::VisualDataModel::model

    This attached property holds the visual data model this delegate instance belongs to.

    It is attached to each instance of the delegate.
*/

QQuickVisualDataModel *QQuickVisualDataModelAttached::model() const
{
    return m_cacheItem ? m_cacheItem->metaType->model : 0;
}

/*!
    \qmlattachedproperty stringlist QtQuick2::VisualDataModel::groups

    This attached property holds the name of VisualDataGroups the item belongs to.

    It is attached to each instance of the delegate.
*/

QStringList QQuickVisualDataModelAttached::groups() const
{
    QStringList groups;

    if (!m_cacheItem)
        return groups;
    for (int i = 1; i < m_cacheItem->metaType->groupCount; ++i) {
        if (m_cacheItem->groups & (1 << i))
            groups.append(m_cacheItem->metaType->groupNames.at(i - 1));
    }
    return groups;
}

void QQuickVisualDataModelAttached::setGroups(const QStringList &groups)
{
    if (!m_cacheItem)
        return;

    QQuickVisualDataModelPrivate *model = QQuickVisualDataModelPrivate::get(m_cacheItem->metaType->model);

    const int groupFlags = model->m_cacheMetaType->parseGroups(groups);
    for (int i = 1; i < m_cacheItem->metaType->groupCount; ++i) {
        if (m_cacheItem->groups & (1 << i)) {
            Compositor::iterator it = model->m_compositor.find(Compositor::Group(i), m_cacheItem->index[i]);
            model->setGroups(it, 1, Compositor::Group(i), groupFlags);
            return;
        }
    }
}

bool QQuickVisualDataModelAttached::isUnresolved() const
{
    if (!m_cacheItem)
        return false;

    return m_cacheItem->groups & Compositor::UnresolvedFlag;
}

/*!
    \qmlattachedproperty int QtQuick2::VisualDataModel::inItems

    This attached property holds whether the item belongs to the default \l items VisualDataGroup.

    Changing this property will add or remove the item from the items group.

    It is attached to each instance of the delegate.
*/

/*!
    \qmlattachedproperty int QtQuick2::VisualDataModel::itemsIndex

    This attached property holds the index of the item in the default \l items VisualDataGroup.

    It is attached to each instance of the delegate.
*/

/*!
    \qmlattachedproperty int QtQuick2::VisualDataModel::inPersistedItems

    This attached property holds whether the item belongs to the \l persistedItems VisualDataGroup.

    Changing this property will add or remove the item from the items group.  Change with caution
    as removing an item from the persistedItems group will destroy the current instance if it is
    not referenced by a model.

    It is attached to each instance of the delegate.
*/

/*!
    \qmlattachedproperty int QtQuick2::VisualDataModel::persistedItemsIndex

    This attached property holds the index of the item in the \l persistedItems VisualDataGroup.

    It is attached to each instance of the delegate.
*/

void QQuickVisualDataModelAttached::emitChanges()
{
    if (m_modelChanged) {
        m_modelChanged = false;
        emit modelChanged();
    }

    const int groupChanges = m_previousGroups ^ m_cacheItem->groups;
    m_previousGroups = m_cacheItem->groups;

    int indexChanges = 0;
    for (int i = 1; i < m_cacheItem->metaType->groupCount; ++i) {
        if (m_previousIndex[i] != m_cacheItem->index[i]) {
            m_previousIndex[i] = m_cacheItem->index[i];
            indexChanges |= (1 << i);
        }
    }

    int notifierId = 0;
    const QMetaObject *meta = metaObject();
    for (int i = 1; i < m_cacheItem->metaType->groupCount; ++i, ++notifierId) {
        if (groupChanges & (1 << i))
            QMetaObject::activate(this, meta, notifierId, 0);
    }
    for (int i = 1; i < m_cacheItem->metaType->groupCount; ++i, ++notifierId) {
        if (indexChanges & (1 << i))
            QMetaObject::activate(this, meta, notifierId, 0);
    }

    if (groupChanges)
        emit groupsChanged();
}

//============================================================================

void QQuickVisualDataGroupPrivate::setModel(QQuickVisualDataModel *m, Compositor::Group g)
{
    Q_ASSERT(!model);
    model = m;
    group = g;
}

void QQuickVisualDataGroupPrivate::emitChanges(QV8Engine *engine)
{
    Q_Q(QQuickVisualDataGroup);
    static int idx = signalIndex("changed(QQmlV8Handle,QQmlV8Handle)");
    if (isSignalConnected(idx) && !changeSet.isEmpty()) {
        v8::HandleScope handleScope;
        v8::Context::Scope contextScope(engine->context());
        v8::Local<v8::Array> removed  = QQuickVisualDataModelPrivate::buildChangeList(changeSet.removes());
        v8::Local<v8::Array> inserted = QQuickVisualDataModelPrivate::buildChangeList(changeSet.inserts());
        emit q->changed(
                QQmlV8Handle::fromHandle(removed), QQmlV8Handle::fromHandle(inserted));
    }
    if (changeSet.difference() != 0)
        emit q->countChanged();
}

void QQuickVisualDataGroupPrivate::emitModelUpdated(bool reset)
{
    for (QQuickVisualDataGroupEmitterList::iterator it = emitters.begin(); it != emitters.end(); ++it)
        it->emitModelUpdated(changeSet, reset);
    changeSet.clear();
}

void QQuickVisualDataGroupPrivate::createdPackage(int index, QQuickPackage *package)
{
    for (QQuickVisualDataGroupEmitterList::iterator it = emitters.begin(); it != emitters.end(); ++it)
        it->createdPackage(index, package);
}

void QQuickVisualDataGroupPrivate::initPackage(int index, QQuickPackage *package)
{
    for (QQuickVisualDataGroupEmitterList::iterator it = emitters.begin(); it != emitters.end(); ++it)
        it->initPackage(index, package);
}

void QQuickVisualDataGroupPrivate::destroyingPackage(QQuickPackage *package)
{
    for (QQuickVisualDataGroupEmitterList::iterator it = emitters.begin(); it != emitters.end(); ++it)
        it->destroyingPackage(package);
}

/*!
    \qmlclass VisualDataGroup QQuickVisualDataGroup
    \inqmlmodule QtQuick 2
    \ingroup qml-working-with-data
    \brief The VisualDataGroup encapsulates a filtered set of visual data items.

*/

QQuickVisualDataGroup::QQuickVisualDataGroup(QObject *parent)
    : QObject(*new QQuickVisualDataGroupPrivate, parent)
{
}

QQuickVisualDataGroup::QQuickVisualDataGroup(
        const QString &name, QQuickVisualDataModel *model, int index, QObject *parent)
    : QObject(*new QQuickVisualDataGroupPrivate, parent)
{
    Q_D(QQuickVisualDataGroup);
    d->name = name;
    d->setModel(model, Compositor::Group(index));
}

QQuickVisualDataGroup::~QQuickVisualDataGroup()
{
}

/*!
    \qmlproperty string QtQuick2::VisualDataGroup::name

    This property holds the name of the group.

    Each group in a model must have a unique name starting with a lower case letter.
*/

QString QQuickVisualDataGroup::name() const
{
    Q_D(const QQuickVisualDataGroup);
    return d->name;
}

void QQuickVisualDataGroup::setName(const QString &name)
{
    Q_D(QQuickVisualDataGroup);
    if (d->model)
        return;
    if (d->name != name) {
        d->name = name;
        emit nameChanged();
    }
}

/*!
    \qmlproperty int QtQuick2::VisualDataGroup::count

    This property holds the number of items in the group.
*/

int QQuickVisualDataGroup::count() const
{
    Q_D(const QQuickVisualDataGroup);
    if (!d->model)
        return 0;
    return QQuickVisualDataModelPrivate::get(d->model)->m_compositor.count(d->group);
}

/*!
    \qmlproperty bool QtQuick2::VisualDataGroup::includeByDefault

    This property holds whether new items are assigned to this group by default.
*/

bool QQuickVisualDataGroup::defaultInclude() const
{
    Q_D(const QQuickVisualDataGroup);
    return d->defaultInclude;
}

void QQuickVisualDataGroup::setDefaultInclude(bool include)
{
    Q_D(QQuickVisualDataGroup);
    if (d->defaultInclude != include) {
        d->defaultInclude = include;

        if (d->model) {
            if (include)
                QQuickVisualDataModelPrivate::get(d->model)->m_compositor.setDefaultGroup(d->group);
            else
                QQuickVisualDataModelPrivate::get(d->model)->m_compositor.clearDefaultGroup(d->group);
        }
        emit defaultIncludeChanged();
    }
}

/*!
    \qmlmethod var QtQuick2::VisualDataGroup::get(int index)

    Returns a javascript object describing the item at \a index in the group.

    The returned object contains the same information that is available to a delegate from the
    VisualDataModel attached as well as the model for that item.  It has the properties:

    \list
    \o \b model The model data of the item.  This is the same as the model context property in
    a delegate
    \o \b groups A list the of names of groups the item is a member of.  This property can be
    written to change the item's membership.
    \o \b inItems Whether the item belongs to the \l {QtQuick2::VisualDataModel::items}{items} group.
    Writing to this property will add or remove the item from the group.
    \o \b itemsIndex The index of the item within the \l {QtQuick2::VisualDataModel::items}{items} group.
    \o \b {in\i{GroupName}} Whether the item belongs to the dynamic group \i groupName.  Writing to
    this property will add or remove the item from the group.
    \o \b {\i{groupName}Index} The index of the item within the dynamic group \i groupName.
    \endlist
*/

QQmlV8Handle QQuickVisualDataGroup::get(int index)
{
    Q_D(QQuickVisualDataGroup);
    if (!d->model)
        return QQmlV8Handle::fromHandle(v8::Undefined());;

    QQuickVisualDataModelPrivate *model = QQuickVisualDataModelPrivate::get(d->model);
    if (index < 0 || index >= model->m_compositor.count(d->group)) {
        qmlInfo(this) << tr("get: index out of range");
        return QQmlV8Handle::fromHandle(v8::Undefined());
    }

    Compositor::iterator it = model->m_compositor.find(d->group, index);
    QQuickVisualDataModelItem *cacheItem = it->inCache()
            ? model->m_cache.at(it.cacheIndex)
            : 0;

    if (!cacheItem) {
        cacheItem = model->m_adaptorModel->createItem(model->m_cacheMetaType, it.modelIndex());
        for (int i = 1; i < model->m_groupCount; ++i)
            cacheItem->index[i] = it.index[i];
        cacheItem->groups = it->flags;

        model->m_cache.insert(it.cacheIndex, cacheItem);
        model->m_compositor.setFlags(it, 1, Compositor::CacheFlag);
    }

    if (cacheItem->indexHandle.IsEmpty()) {
        cacheItem->indexHandle = qPersistentNew(model->m_cacheMetaType->constructor->NewInstance());
        cacheItem->indexHandle->SetExternalResource(cacheItem);
        cacheItem->indexHandle.MakeWeak(cacheItem, QQuickVisualDataModelItemMetaType::release_index);

        ++cacheItem->scriptRef;
    }

    return QQmlV8Handle::fromHandle(cacheItem->indexHandle);
}

bool QQuickVisualDataGroupPrivate::parseIndex(
        const v8::Local<v8::Value> &value, int *index, Compositor::Group *group) const
{
    if (value->IsInt32()) {
        *index = value->Int32Value();
        return true;
    } else if (value->IsObject()) {
        v8::Local<v8::Object> object = value->ToObject();
        QQuickVisualDataModelItem * const cacheItem = v8_resource_cast<QQuickVisualDataModelItem>(object);
        for (int i = 1; cacheItem && i < cacheItem->metaType->groupCount; ++i) {
            if (cacheItem->groups & (1 << i)) {
                *group = Compositor::Group(i);
                *index = cacheItem->index[i];
                return true;
            }
        }
    }
    return false;
}

void QQuickVisualDataGroup::insert(QQmlV8Function *args)
{
    Q_D(QQuickVisualDataGroup);
    QQuickVisualDataModelPrivate *model = QQuickVisualDataModelPrivate::get(d->model);

    int index = model->m_compositor.count(d->group);
    Compositor::Group group = d->group;

    if (args->Length() == 0)
        return;

    int  i = 0;
    v8::Local<v8::Value> v = (*args)[i];
    if (d->parseIndex(v, &index, &group)) {
        if (index < 0 || index > model->m_compositor.count(group)) {
            qmlInfo(this) << tr("insert: index out of range");
            return;
        }
        if (++i == args->Length())
            return;
        v = (*args)[i];
    }

    Compositor::insert_iterator before = index < model->m_compositor.count(group)
            ? model->m_compositor.findInsertPosition(group, index)
            : model->m_compositor.end();

    int groups = 1 << d->group;
    if (++i < args->Length())
        groups |= model->m_cacheMetaType->parseGroups((*args)[i]);

    if (v->IsArray()) {
        return;
    } else if (v->IsObject()) {
        model->insert(before, v->ToObject(), groups);
        model->emitChanges();
    }
}

/*!
    \qmlmethod QtQuick2::VisualDataGroup::create(var index)
    \qmlmethod QtQuick2::VisualDataGroup::create(var index, jsdict data)
    \qmlmethod QtQuick2::VisualDataGroup::create(jsdict data)

    Returns a reference to the instantiated item at \a index in the group.

    All items returned by create are added to the persistedItems group.  Items in this
    group remain instantiated when not referenced by any view.
*/

void QQuickVisualDataGroup::create(QQmlV8Function *args)
{
    Q_D(QQuickVisualDataGroup);
    if (!d->model)
        return;

    if (args->Length() == 0)
        return;

    QQuickVisualDataModelPrivate *model = QQuickVisualDataModelPrivate::get(d->model);

    int index = model->m_compositor.count(d->group);
    Compositor::Group group = d->group;

    int  i = 0;
    v8::Local<v8::Value> v = (*args)[i];
    if (d->parseIndex(v, &index, &group))
        ++i;

    if (i < args->Length() && index >= 0 && index <= model->m_compositor.count(group)) {
        v = (*args)[i];
        if (v->IsObject()) {
            int groups = 1 << d->group;
            if (++i < args->Length())
                groups |= model->m_cacheMetaType->parseGroups((*args)[i]);

            Compositor::insert_iterator before = index < model->m_compositor.count(group)
                    ? model->m_compositor.findInsertPosition(group, index)
                    : model->m_compositor.end();

            index = before.index[d->group];
            group = d->group;

            if (!model->insert(before, v->ToObject(), groups)) {
                return;
            }
        }
    }
    if (index < 0 || index >= model->m_compositor.count(group)) {
        qmlInfo(this) << tr("create: index out of range");
        return;
    }

    QObject *object = model->object(group, index, false, false);
    if (object) {
        QVector<Compositor::Insert> inserts;
        Compositor::iterator it = model->m_compositor.find(group, index);
        model->m_compositor.setFlags(it, 1, d->group, Compositor::PersistedFlag, &inserts);
        model->itemsInserted(inserts);
    }

    args->returnValue(args->engine()->newQObject(object));
    model->emitChanges();
}

void QQuickVisualDataGroup::resolve(QQmlV8Function *args)
{
    Q_D(QQuickVisualDataGroup);
    if (!d->model)
        return;

    QQuickVisualDataModelPrivate *model = QQuickVisualDataModelPrivate::get(d->model);

    if (args->Length() < 2)
        return;

    int from = -1;
    int to = -1;
    Compositor::Group fromGroup = d->group;
    Compositor::Group toGroup = d->group;

    v8::Local<v8::Value> v = (*args)[0];
    if (d->parseIndex(v, &from, &fromGroup)) {
        if (from < 0 || from >= model->m_compositor.count(fromGroup)) {
            qmlInfo(this) << tr("resolve: from index out of range");
            return;
        }
    } else {
        qmlInfo(this) << tr("resolve: from index invalid");
        return;
    }

    v = (*args)[1];
    if (d->parseIndex(v, &to, &toGroup)) {
        if (to < 0 || to >= model->m_compositor.count(toGroup)) {
            qmlInfo(this) << tr("resolve: to index out of range");
            return;
        }
    } else {
        qmlInfo(this) << tr("resolve: to index invalid");
        return;
    }

    Compositor::iterator fromIt = model->m_compositor.find(fromGroup, from);
    Compositor::iterator toIt = model->m_compositor.find(toGroup, to);

    if (!fromIt->isUnresolved()) {
        qmlInfo(this) << tr("resolve: from is not an unresolved item");
        return;
    }
    if (!toIt->list) {
        qmlInfo(this) << tr("resolve: to is not a model item");
        return;
    }

    const int unresolvedFlags = fromIt->flags;
    const int resolvedFlags = toIt->flags;
    const int resolvedIndex = toIt.modelIndex();
    void * const resolvedList = toIt->list;

    QQuickVisualDataModelItem *cacheItem = model->m_cache.at(fromIt.cacheIndex);
    cacheItem->groups &= ~Compositor::UnresolvedFlag;

    if (toIt.cacheIndex > fromIt.cacheIndex)
        toIt.decrementIndexes(1, unresolvedFlags);
    if (!toIt->inGroup(fromGroup) || toIt.index[fromGroup] > from)
        from += 1;

    model->itemsMoved(
            QVector<Compositor::Remove>() << Compositor::Remove(fromIt, 1, unresolvedFlags, 0),
            QVector<Compositor::Insert>() << Compositor::Insert(toIt, 1, unresolvedFlags, 0));
    model->itemsInserted(
            QVector<Compositor::Insert>() << Compositor::Insert(toIt, 1, (resolvedFlags & ~unresolvedFlags) | Compositor::CacheFlag));
    toIt.incrementIndexes(1, resolvedFlags | unresolvedFlags);
    model->itemsRemoved(QVector<Compositor::Remove>() << Compositor::Remove(toIt, 1, resolvedFlags));

    model->m_compositor.setFlags(toGroup, to, 1, unresolvedFlags & ~Compositor::UnresolvedFlag);
    model->m_compositor.clearFlags(fromGroup, from, 1, unresolvedFlags);

    if (resolvedFlags & Compositor::CacheFlag)
        model->m_compositor.insert(Compositor::Cache, toIt.cacheIndex, resolvedList, resolvedIndex, 1, Compositor::CacheFlag);

    Q_ASSERT(model->m_cache.count() == model->m_compositor.count(Compositor::Cache));

    if (!cacheItem->isReferenced()) {
        Q_ASSERT(toIt.cacheIndex == model->m_cache.indexOf(cacheItem));
        model->m_cache.removeAt(toIt.cacheIndex);
        model->m_compositor.clearFlags(Compositor::Cache, toIt.cacheIndex, 1, Compositor::CacheFlag);
        delete cacheItem;
        Q_ASSERT(model->m_cache.count() == model->m_compositor.count(Compositor::Cache));
    } else {
        cacheItem->resolveIndex(resolvedIndex);
        if (cacheItem->object)
            cacheItem->attached->emitUnresolvedChanged();
    }

    model->emitChanges();
}

/*!
    \qmlmethod QtQuick2::VisualDataGroup::remove(int index, int count)

    Removes \a count items starting at \a index from the group.
*/

void QQuickVisualDataGroup::remove(QQmlV8Function *args)
{
    Q_D(QQuickVisualDataGroup);
    if (!d->model)
        return;
    Compositor::Group group = d->group;
    int index = -1;
    int count = 1;

    if (args->Length() == 0)
        return;

    int i = 0;
    v8::Local<v8::Value> v = (*args)[i];
    if (!d->parseIndex(v, &index, &group)) {
        qmlInfo(this) << tr("remove: invalid index");
        return;
    }

    if (++i < args->Length()) {
        v = (*args)[i];
        if (v->IsInt32())
            count = v->Int32Value();
    }

    QQuickVisualDataModelPrivate *model = QQuickVisualDataModelPrivate::get(d->model);
    if (index < 0 || index >= model->m_compositor.count(group)) {
        qmlInfo(this) << tr("remove: index out of range");
    } else if (count != 0) {
        Compositor::iterator it = model->m_compositor.find(group, index);
        if (count < 0 || count > model->m_compositor.count(d->group) - it.index[d->group]) {
            qmlInfo(this) << tr("remove: invalid count");
        } else {
            model->removeGroups(it, count, d->group, 1 << d->group);
        }
    }
}

bool QQuickVisualDataGroupPrivate::parseGroupArgs(
        QQmlV8Function *args, Compositor::Group *group, int *index, int *count, int *groups) const
{
    if (!model || !QQuickVisualDataModelPrivate::get(model)->m_cacheMetaType)
        return false;

    if (args->Length() < 2)
        return false;

    int i = 0;
    v8::Local<v8::Value> v = (*args)[i];
    if (!parseIndex(v, index, group))
        return false;

    v = (*args)[++i];
    if (v->IsInt32()) {
        *count = v->Int32Value();

        if (++i == args->Length())
            return false;
        v = (*args)[i];
    }

    *groups = QQuickVisualDataModelPrivate::get(model)->m_cacheMetaType->parseGroups(v);

    return true;
}

/*!
    \qmlmethod QtQuick2::VisualDataGroup::addGroups(int index, int count, stringlist groups)

    Adds \a count items starting at \a index to \a groups.
*/

void QQuickVisualDataGroup::addGroups(QQmlV8Function *args)
{
    Q_D(QQuickVisualDataGroup);
    Compositor::Group group = d->group;
    int index = -1;
    int count = 1;
    int groups = 0;

    if (!d->parseGroupArgs(args, &group, &index, &count, &groups))
        return;

    QQuickVisualDataModelPrivate *model = QQuickVisualDataModelPrivate::get(d->model);
    if (index < 0 || index >= model->m_compositor.count(group)) {
        qmlInfo(this) << tr("addGroups: index out of range");
    } else if (count != 0) {
        Compositor::iterator it = model->m_compositor.find(group, index);
        if (count < 0 || count > model->m_compositor.count(d->group) - it.index[d->group]) {
            qmlInfo(this) << tr("addGroups: invalid count");
        } else {
            model->addGroups(it, count, d->group, groups);
        }
    }
}

/*!
    \qmlmethod QtQuick2::VisualDataGroup::removeGroups(int index, int count, stringlist groups)

    Removes \a count items starting at \a index from \a groups.
*/

void QQuickVisualDataGroup::removeGroups(QQmlV8Function *args)
{
    Q_D(QQuickVisualDataGroup);
    Compositor::Group group = d->group;
    int index = -1;
    int count = 1;
    int groups = 0;

    if (!d->parseGroupArgs(args, &group, &index, &count, &groups))
        return;

    QQuickVisualDataModelPrivate *model = QQuickVisualDataModelPrivate::get(d->model);
    if (index < 0 || index >= model->m_compositor.count(group)) {
        qmlInfo(this) << tr("removeGroups: index out of range");
    } else if (count != 0) {
        Compositor::iterator it = model->m_compositor.find(group, index);
        if (count < 0 || count > model->m_compositor.count(d->group) - it.index[d->group]) {
            qmlInfo(this) << tr("removeGroups: invalid count");
        } else {
            model->removeGroups(it, count, d->group, groups);
        }
    }
}

/*!
    \qmlmethod QtQuick2::VisualDataGroup::setGroups(int index, int count, stringlist groups)

    Sets the \a groups \a count items starting at \a index belong to.
*/

void QQuickVisualDataGroup::setGroups(QQmlV8Function *args)
{
    Q_D(QQuickVisualDataGroup);
    Compositor::Group group = d->group;
    int index = -1;
    int count = 1;
    int groups = 0;

    if (!d->parseGroupArgs(args, &group, &index, &count, &groups))
        return;

    QQuickVisualDataModelPrivate *model = QQuickVisualDataModelPrivate::get(d->model);
    if (index < 0 || index >= model->m_compositor.count(group)) {
        qmlInfo(this) << tr("setGroups: index out of range");
    } else if (count != 0) {
        Compositor::iterator it = model->m_compositor.find(group, index);
        if (count < 0 || count > model->m_compositor.count(d->group) - it.index[d->group]) {
            qmlInfo(this) << tr("setGroups: invalid count");
        } else {
            model->setGroups(it, count, d->group, groups);
        }
    }
}

/*!
    \qmlmethod QtQuick2::VisualDataGroup::setGroups(int index, int count, stringlist groups)

    Sets the \a groups \a count items starting at \a index belong to.
*/

/*!
    \qmlmethod QtQuick2::VisualDataGroup::move(var from, var to, int count)

    Moves \a count at \a from in a group \a to a new position.
*/

void QQuickVisualDataGroup::move(QQmlV8Function *args)
{
    Q_D(QQuickVisualDataGroup);

    if (args->Length() < 2)
        return;

    Compositor::Group fromGroup = d->group;
    Compositor::Group toGroup = d->group;
    int from = -1;
    int to = -1;
    int count = 1;

    if (!d->parseIndex((*args)[0], &from, &fromGroup)) {
        qmlInfo(this) << tr("move: invalid from index");
        return;
    }

    if (!d->parseIndex((*args)[1], &to, &toGroup)) {
        qmlInfo(this) << tr("move: invalid to index");
        return;
    }

    if (args->Length() > 2) {
        v8::Local<v8::Value> v = (*args)[2];
        if (v->IsInt32())
            count = v->Int32Value();
    }

    QQuickVisualDataModelPrivate *model = QQuickVisualDataModelPrivate::get(d->model);

    if (count < 0) {
        qmlInfo(this) << tr("move: invalid count");
    } else if (from < 0 || from + count > model->m_compositor.count(fromGroup)) {
        qmlInfo(this) << tr("move: from index out of range");
    } else if (!model->m_compositor.verifyMoveTo(fromGroup, from, toGroup, to, count, d->group)) {
        qmlInfo(this) << tr("move: to index out of range");
    } else if (count > 0) {
        QVector<Compositor::Remove> removes;
        QVector<Compositor::Insert> inserts;

        model->m_compositor.move(fromGroup, from, toGroup, to, count, d->group, &removes, &inserts);
        model->itemsMoved(removes, inserts);
        model->emitChanges();
    }

}

/*!
    \qmlsignal QtQuick2::VisualDataGroup::onChanged(array removed, array inserted)

    This handler is called when items have been removed from or inserted into the group.

    Each object in the \a removed and \a inserted arrays has two values; the \e index of the first
    item inserted or removed and a \e count of the number of consecutive items inserted or removed.

    Each index is adjusted for previous changes with all removed items preceding any inserted
    items.
*/

//============================================================================

QQuickVisualPartsModel::QQuickVisualPartsModel(QQuickVisualDataModel *model, const QString &part, QObject *parent)
    : QQuickVisualModel(*new QObjectPrivate, parent)
    , m_model(model)
    , m_part(part)
    , m_compositorGroup(Compositor::Cache)
    , m_inheritGroup(true)
{
    QQuickVisualDataModelPrivate *d = QQuickVisualDataModelPrivate::get(m_model);
    if (d->m_cacheMetaType) {
        QQuickVisualDataGroupPrivate::get(d->m_groups[1])->emitters.insert(this);
        m_compositorGroup = Compositor::Default;
    } else {
        d->m_pendingParts.insert(this);
    }
}

QQuickVisualPartsModel::~QQuickVisualPartsModel()
{
}

QString QQuickVisualPartsModel::filterGroup() const
{
    if (m_inheritGroup)
        return m_model->filterGroup();
    return m_filterGroup;
}

void QQuickVisualPartsModel::setFilterGroup(const QString &group)
{
    if (QQuickVisualDataModelPrivate::get(m_model)->m_transaction) {
        qmlInfo(this) << tr("The group of a VisualDataModel cannot be changed within onChanged");
        return;
    }

    if (m_filterGroup != group || m_inheritGroup) {
        m_filterGroup = group;
        m_inheritGroup = false;
        updateFilterGroup();

        emit filterGroupChanged();
    }
}

void QQuickVisualPartsModel::resetFilterGroup()
{
    if (!m_inheritGroup) {
        m_inheritGroup = true;
        updateFilterGroup();
        emit filterGroupChanged();
    }
}

void QQuickVisualPartsModel::updateFilterGroup()
{
    QQuickVisualDataModelPrivate *model = QQuickVisualDataModelPrivate::get(m_model);
    if (!model->m_cacheMetaType)
        return;

    if (m_inheritGroup) {
        if (m_filterGroup == model->m_filterGroup)
            return;
        m_filterGroup = model->m_filterGroup;
    }

    QQuickListCompositor::Group previousGroup = m_compositorGroup;
    m_compositorGroup = Compositor::Default;
    QQuickVisualDataGroupPrivate::get(model->m_groups[Compositor::Default])->emitters.insert(this);
    for (int i = 1; i < model->m_groupCount; ++i) {
        if (m_filterGroup == model->m_cacheMetaType->groupNames.at(i - 1)) {
            m_compositorGroup = Compositor::Group(i);
            break;
        }
    }

    QQuickVisualDataGroupPrivate::get(model->m_groups[m_compositorGroup])->emitters.insert(this);
    if (m_compositorGroup != previousGroup) {
        QVector<QQuickChangeSet::Remove> removes;
        QVector<QQuickChangeSet::Insert> inserts;
        model->m_compositor.transition(previousGroup, m_compositorGroup, &removes, &inserts);

        QQuickChangeSet changeSet;
        changeSet.apply(removes, inserts);
        if (!changeSet.isEmpty())
            emit modelUpdated(changeSet, false);

        if (changeSet.difference() != 0)
            emit countChanged();
    }
}

void QQuickVisualPartsModel::updateFilterGroup(
        Compositor::Group group, const QQuickChangeSet &changeSet)
{
    if (!m_inheritGroup)
        return;

    m_compositorGroup = group;
    QQuickVisualDataGroupPrivate::get(QQuickVisualDataModelPrivate::get(m_model)->m_groups[m_compositorGroup])->emitters.insert(this);

    if (!changeSet.isEmpty())
        emit modelUpdated(changeSet, false);

    if (changeSet.difference() != 0)
        emit countChanged();

    emit filterGroupChanged();
}

int QQuickVisualPartsModel::count() const
{
    QQuickVisualDataModelPrivate *model = QQuickVisualDataModelPrivate::get(m_model);
    return model->m_delegate
            ? model->m_compositor.count(m_compositorGroup)
            : 0;
}

bool QQuickVisualPartsModel::isValid() const
{
    return m_model->isValid();
}

QQuickItem *QQuickVisualPartsModel::item(int index, bool asynchronous)
{
    QQuickVisualDataModelPrivate *model = QQuickVisualDataModelPrivate::get(m_model);

    if (!model->m_delegate || index < 0 || index >= model->m_compositor.count(m_compositorGroup)) {
        qWarning() << "VisualDataModel::item: index out range" << index << model->m_compositor.count(m_compositorGroup);
        return 0;
    }

    QObject *object = model->object(m_compositorGroup, index, asynchronous, true);

    if (QQuickPackage *package = qobject_cast<QQuickPackage *>(object)) {
        QObject *part = package->part(m_part);
        if (!part)
            return 0;
        if (QQuickItem *item = qobject_cast<QQuickItem *>(part)) {
            m_packaged.insertMulti(item, package);
            return item;
        }
    }

    model->release(object);
    if (!model->m_delegateValidated) {
        if (object)
            qmlInfo(model->m_delegate) << tr("Delegate component must be Package type.");
        model->m_delegateValidated = true;
    }

    return 0;
}

QQuickVisualModel::ReleaseFlags QQuickVisualPartsModel::release(QQuickItem *item)
{
    QQuickVisualModel::ReleaseFlags flags = 0;

    QHash<QObject *, QQuickPackage *>::iterator it = m_packaged.find(item);
    if (it != m_packaged.end()) {
        QQuickPackage *package = *it;
        QQml_setParent_noEvent(item, package);
        QQuickVisualDataModelPrivate *model = QQuickVisualDataModelPrivate::get(m_model);
        flags = model->release(package);
        m_packaged.erase(it);
        if (!m_packaged.contains(item))
            flags &= ~Referenced;
        if (flags & Destroyed)
            QQuickVisualDataModelPrivate::get(m_model)->emitDestroyingPackage(package);
    }
    return flags;
}

QString QQuickVisualPartsModel::stringValue(int index, const QString &role)
{
    return QQuickVisualDataModelPrivate::get(m_model)->stringValue(m_compositorGroup, index, role);
}

void QQuickVisualPartsModel::setWatchedRoles(QList<QByteArray> roles)
{
    QQuickVisualDataModelPrivate *model = QQuickVisualDataModelPrivate::get(m_model);
    model->m_adaptorModel->replaceWatchedRoles(m_watchedRoles, roles);
    m_watchedRoles = roles;
}

int QQuickVisualPartsModel::indexOf(QQuickItem *item, QObject *) const
{
    QHash<QObject *, QQuickPackage *>::const_iterator it = m_packaged.find(item);
    if (it != m_packaged.end()) {
        if (QQuickVisualDataModelAttached *attached = QQuickVisualDataModelAttached::properties(*it))
            return attached->m_cacheItem->index[m_compositorGroup];
    }
    return -1;
}

void QQuickVisualPartsModel::createdPackage(int index, QQuickPackage *package)
{
    if (QQuickItem *item = qobject_cast<QQuickItem *>(package->part(m_part)))
        emit createdItem(index, item);
}

void QQuickVisualPartsModel::initPackage(int index, QQuickPackage *package)
{
    if (QQuickItem *item = qobject_cast<QQuickItem *>(package->part(m_part)))
        emit initItem(index, item);
}

void QQuickVisualPartsModel::destroyingPackage(QQuickPackage *package)
{
    if (QQuickItem *item = qobject_cast<QQuickItem *>(package->part(m_part))) {
        Q_ASSERT(!m_packaged.contains(item));
        emit destroyingItem(item);
        item->setParentItem(0);
        QQml_setParent_noEvent(item, package);
    }
}

void QQuickVisualPartsModel::emitModelUpdated(const QQuickChangeSet &changeSet, bool reset)
{
    emit modelUpdated(changeSet, reset);
    if (changeSet.difference() != 0)
        emit countChanged();
}


QT_END_NAMESPACE

