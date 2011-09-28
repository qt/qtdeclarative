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

#include "qsgvisualdatamodel_p.h"
#include "qsgitem.h"

#include <QtCore/qcoreapplication.h>
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
#include <private/qmetaobjectbuilder_p.h>
#include <private/qdeclarativeproperty_p.h>
#include <private/qsgvisualadaptormodel_p.h>
#include <private/qdeclarativechangeset_p.h>
#include <private/qdeclarativelistcompositor_p.h>
#include <private/qdeclarativeengine_p.h>
#include <private/qobject_p.h>

#include <QtCore/qhash.h>
#include <QtCore/qlist.h>

QT_BEGIN_NAMESPACE

typedef QDeclarativeListCompositor Compositor;

class QSGVisualDataGroupEmitter
{
public:
    virtual void emitModelUpdated(const QDeclarativeChangeSet &changeSet, bool reset) = 0;
    virtual void createdPackage(int, QDeclarativePackage *) {}
    virtual void destroyingPackage(QDeclarativePackage *) {}

    QIntrusiveListNode emitterNode;
};

typedef QIntrusiveList<QSGVisualDataGroupEmitter, &QSGVisualDataGroupEmitter::emitterNode> QSGVisualDataGroupEmitterList;

//---------------------------------------------------------------------------

class QSGVisualDataGroupPrivate : public QObjectPrivate
{
public:
    Q_DECLARE_PUBLIC(QSGVisualDataGroup)

    QSGVisualDataGroupPrivate() : group(Compositor::Cache), defaultInclude(false) {}

    static QSGVisualDataGroupPrivate *get(QSGVisualDataGroup *group) {
        return static_cast<QSGVisualDataGroupPrivate *>(QObjectPrivate::get(group)); }

    void setModel(QSGVisualDataModel *model, Compositor::Group group);
    void emitChanges(QV8Engine *engine);
    void emitModelUpdated(bool reset);

    void createdPackage(int index, QDeclarativePackage *package);
    void destroyingPackage(QDeclarativePackage *package);

    bool parseGroupArgs(QDeclarativeV8Function *args, int *index, int *count, int *groups) const;

    Compositor::Group group;
    QDeclarativeGuard<QSGVisualDataModel> model;
    QSGVisualDataGroupEmitterList emitters;
    QDeclarativeChangeSet changeSet;
    QString name;
    bool defaultInclude;
};

//---------------------------------------------------------------------------

class QSGVisualDataModelCacheItem;
class QSGVisualDataModelCacheMetaType;
class QSGVisualDataModelParts;

class QSGVisualDataModelPrivate : public QObjectPrivate, public QSGVisualDataGroupEmitter
{
    Q_DECLARE_PUBLIC(QSGVisualDataModel)
public:
    QSGVisualDataModelPrivate(QDeclarativeContext *);

    static QSGVisualDataModelPrivate *get(QSGVisualDataModel *m) {
        return static_cast<QSGVisualDataModelPrivate *>(QObjectPrivate::get(m));
    }

    void init();
    void connectModel(QSGVisualAdaptorModel *model);

    QObject *object(Compositor::Group group, int index, bool complete, bool reference);
    void destroy(QObject *object);
    QSGVisualDataModel::ReleaseFlags release(QObject *object);
    QString stringValue(Compositor::Group group, int index, const QString &name);
    int cacheIndexOf(QObject *object) const;
    void emitCreatedPackage(Compositor::iterator at, QDeclarativePackage *package);
    void emitCreatedItem(Compositor::iterator at, QSGItem *item) {
        emit q_func()->createdItem(at.index[m_compositorGroup], item); }
    void emitDestroyingPackage(QDeclarativePackage *package);
    void emitDestroyingItem(QSGItem *item) { emit q_func()->destroyingItem(item); }

    void updateFilterGroup();

    void addGroups(Compositor::Group group, int index, int count, int groupFlags);
    void removeGroups(Compositor::Group group, int index, int count, int groupFlags);
    void setGroups(Compositor::Group group, int index, int count, int groupFlags);

    void itemsInserted(
            const QVector<Compositor::Insert> &inserts,
            QVarLengthArray<QVector<QDeclarativeChangeSet::Insert>, Compositor::MaximumGroupCount> *translatedInserts,
            QHash<int, QList<QSGVisualDataModelCacheItem *> > *movedItems = 0);
    void itemsInserted(const QVector<Compositor::Insert> &inserts);
    void itemsRemoved(
            const QVector<Compositor::Remove> &removes,
            QVarLengthArray<QVector<QDeclarativeChangeSet::Remove>, Compositor::MaximumGroupCount> *translatedRemoves,
            QHash<int, QList<QSGVisualDataModelCacheItem *> > *movedItems = 0);
    void itemsRemoved(const QVector<Compositor::Remove> &removes);
    void itemsMoved(
            const QVector<Compositor::Remove> &removes, const QVector<Compositor::Insert> &inserts);
    void itemsChanged(const QVector<Compositor::Change> &changes);
    template <typename T> static v8::Local<v8::Array> buildChangeList(const QVector<T> &changes);
    void emitChanges();
    void emitModelUpdated(const QDeclarativeChangeSet &changeSet, bool reset);


    static void group_append(QDeclarativeListProperty<QSGVisualDataGroup> *property, QSGVisualDataGroup *group);
    static int group_count(QDeclarativeListProperty<QSGVisualDataGroup> *property);
    static QSGVisualDataGroup *group_at(QDeclarativeListProperty<QSGVisualDataGroup> *property, int index);

    QSGVisualAdaptorModel *m_adaptorModel;
    QDeclarativeComponent *m_delegate;
    QSGVisualDataModelCacheMetaType *m_cacheMetaType;
    QDeclarativeGuard<QDeclarativeContext> m_context;

    QList<QSGVisualDataModelCacheItem *> m_cache;
    QSGVisualDataModelParts *m_parts;
    QSGVisualDataGroupEmitterList m_pendingParts;

    QDeclarativeListCompositor m_compositor;
    QDeclarativeListCompositor::Group m_compositorGroup;
    bool m_complete : 1;
    bool m_delegateValidated : 1;
    bool m_completePending : 1;
    bool m_reset : 1;
    bool m_transaction : 1;

    QString m_filterGroup;
    QList<QByteArray> watchedRoles;

    union {
        struct {
            QSGVisualDataGroup *m_cacheItems;
            QSGVisualDataGroup *m_items;
            QSGVisualDataGroup *m_persistedItems;
        };
        QSGVisualDataGroup *m_groups[Compositor::MaximumGroupCount];
    };
    int m_groupCount;
};

//---------------------------------------------------------------------------

class QSGVisualPartsModel : public QSGVisualModel, public QSGVisualDataGroupEmitter
{
    Q_OBJECT
    Q_PROPERTY(QString filterOnGroup READ filterGroup WRITE setFilterGroup NOTIFY filterGroupChanged RESET resetFilterGroup)
public:
    QSGVisualPartsModel(QSGVisualDataModel *model, const QString &part, QObject *parent = 0);
    ~QSGVisualPartsModel();

    QString filterGroup() const;
    void setFilterGroup(const QString &group);
    void resetFilterGroup();
    void updateFilterGroup();
    void updateFilterGroup(Compositor::Group group, const QDeclarativeChangeSet &changeSet);

    int count() const;
    bool isValid() const;
    QSGItem *item(int index, bool complete=true);
    ReleaseFlags release(QSGItem *item);
    bool completePending() const;
    void completeItem();
    QString stringValue(int index, const QString &role);
    void setWatchedRoles(QList<QByteArray> roles);

    int indexOf(QSGItem *item, QObject *objectContext) const;

    void emitModelUpdated(const QDeclarativeChangeSet &changeSet, bool reset);

    void createdPackage(int index, QDeclarativePackage *package);
    void destroyingPackage(QDeclarativePackage *package);

Q_SIGNALS:
    void filterGroupChanged();

private:
    QSGVisualDataModel *m_model;
    QHash<QObject *, QDeclarativePackage *> m_packaged;
    QString m_part;
    QString m_filterGroup;
    QList<QByteArray> m_watchedRoles;
    Compositor::Group m_compositorGroup;
    bool m_inheritGroup;
};

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

    QSGVisualDataModel *model;
    QList<QSGVisualPartsModel *> models;
};

void QSGVisualDataModelPartsMetaObject::propertyCreated(int, QMetaPropertyBuilder &prop)
{
    prop.setWritable(false);
}

QVariant QSGVisualDataModelPartsMetaObject::initialValue(int id)
{
    QSGVisualDataModelParts *parts = static_cast<QSGVisualDataModelParts *>(object());
    QSGVisualPartsModel *m = new QSGVisualPartsModel(
            parts->model, QString::fromUtf8(name(id)), parts);
    parts->models.append(m);
    return QVariant::fromValue(static_cast<QObject *>(m));
}

QSGVisualDataModelParts::QSGVisualDataModelParts(QSGVisualDataModel *parent)
: QObject(parent), model(parent)
{
    new QSGVisualDataModelPartsMetaObject(this);
}

//---------------------------------------------------------------------------

class QSGVisualDataModelCacheMetaType : public QDeclarativeRefCount
{
public:
    QSGVisualDataModelCacheMetaType(QV8Engine *engine, QSGVisualDataModel *model, const QStringList &groupNames);
    ~QSGVisualDataModelCacheMetaType();

    int parseGroups(const QStringList &groupNames) const;
    int parseGroups(QV8Engine *engine, const v8::Local<v8::Value> &groupNames) const;

    static v8::Handle<v8::Value> get_model(v8::Local<v8::String>, const v8::AccessorInfo &info);
    static v8::Handle<v8::Value> get_groups(v8::Local<v8::String>, const v8::AccessorInfo &info);
    static void set_groups(
            v8::Local<v8::String>, v8::Local<v8::Value> value, const v8::AccessorInfo &info);
    static v8::Handle<v8::Value> get_member(v8::Local<v8::String>, const v8::AccessorInfo &info);
    static void set_member(
            v8::Local<v8::String>, v8::Local<v8::Value> value, const v8::AccessorInfo &info);
    static v8::Handle<v8::Value> get_index(v8::Local<v8::String>, const v8::AccessorInfo &info);

    QDeclarativeGuard<QSGVisualDataModel> model;
    const int groupCount;
    const int memberPropertyOffset;
    const int indexPropertyOffset;
    QV8Engine * const v8Engine;
    QMetaObject *metaObject;
    const QStringList groupNames;
    v8::Persistent<v8::Function> constructor;
};

class QSGVisualDataModelCacheItem : public QV8ObjectResource
{
    V8_RESOURCE_TYPE(VisualDataItemType)
public:
    QSGVisualDataModelCacheItem(QSGVisualDataModelCacheMetaType *metaType)
        : QV8ObjectResource(metaType->v8Engine)
        , metaType(metaType)
        , object(0)
        , attached(0)
        , objectRef(0)
        , scriptRef(0)
        , groups(0)
    {
        metaType->addref();
    }

    ~QSGVisualDataModelCacheItem()
    {
        Q_ASSERT(scriptRef == 0);
        Q_ASSERT(objectRef == 0);
        Q_ASSERT(!object);

        metaType->release();
    }

    void referenceObject() { ++objectRef; }
    bool releaseObject() { return --objectRef == 0 && !(groups & Compositor::PersistedFlag); }
    bool isObjectReferenced() const { return objectRef == 0 && !(groups & Compositor::PersistedFlag); }

    bool isReferenced() const { return objectRef || scriptRef || (groups & Compositor::PersistedFlag); }

    void Dispose();

    QSGVisualDataModelCacheMetaType * const metaType;
    QDeclarativeGuard<QObject> object;
    QSGVisualDataModelAttached *attached;
    int objectRef;
    int scriptRef;
    int groups;
    int index[Compositor::MaximumGroupCount];
};

class QSGVisualDataModelAttachedMetaObject : public QAbstractDynamicMetaObject
{
public:
    QSGVisualDataModelAttachedMetaObject(
            QSGVisualDataModelAttached *attached, QSGVisualDataModelCacheMetaType *metaType);
    ~QSGVisualDataModelAttachedMetaObject();

    int metaCall(QMetaObject::Call, int _id, void **);

private:
    QSGVisualDataModelAttached *attached;
    QSGVisualDataModelCacheMetaType *metaType;
};

//---------------------------------------------------------------------------

QHash<QObject*, QSGVisualDataModelAttached*> QSGVisualDataModelAttached::attachedProperties;

/*!
    \qmlclass VisualDataModel QSGVisualDataModel
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

    \snippet doc/src/snippets/declarative/visualdatamodel.qml 0
*/

QSGVisualDataModelPrivate::QSGVisualDataModelPrivate(QDeclarativeContext *ctxt)
    : m_adaptorModel(0)
    , m_delegate(0)
    , m_cacheMetaType(0)
    , m_context(ctxt)
    , m_parts(0)
    , m_compositorGroup(Compositor::Cache)
    , m_complete(false)
    , m_delegateValidated(false)
    , m_completePending(false)
    , m_reset(false)
    , m_transaction(false)
    , m_filterGroup(QStringLiteral("items"))
    , m_cacheItems(0)
    , m_items(0)
    , m_groupCount(3)
{
}

void QSGVisualDataModelPrivate::connectModel(QSGVisualAdaptorModel *model)
{
    Q_Q(QSGVisualDataModel);

    QObject::connect(model, SIGNAL(itemsInserted(int,int)), q, SLOT(_q_itemsInserted(int,int)));
    QObject::connect(model, SIGNAL(itemsRemoved(int,int)), q, SLOT(_q_itemsRemoved(int,int)));
    QObject::connect(model, SIGNAL(itemsMoved(int,int,int)), q, SLOT(_q_itemsMoved(int,int,int)));
    QObject::connect(model, SIGNAL(itemsChanged(int,int)), q, SLOT(_q_itemsChanged(int,int)));
    QObject::connect(model, SIGNAL(modelReset(int,int)), q, SLOT(_q_modelReset(int,int)));
}

void QSGVisualDataModelPrivate::init()
{
    Q_Q(QSGVisualDataModel);
    m_compositor.setRemoveGroups(Compositor::GroupMask & ~Compositor::PersistedFlag);

    m_adaptorModel = new QSGVisualAdaptorModel;
    QObject::connect(m_adaptorModel, SIGNAL(rootIndexChanged()), q, SIGNAL(rootIndexChanged()));

    m_items = new QSGVisualDataGroup(QStringLiteral("items"), q, Compositor::Default, q);
    m_items->setDefaultInclude(true);
    m_persistedItems = new QSGVisualDataGroup(QStringLiteral("persistedItems"), q, Compositor::Persisted, q);
    QSGVisualDataGroupPrivate::get(m_items)->emitters.insert(this);
}

QSGVisualDataModel::QSGVisualDataModel()
: QSGVisualModel(*(new QSGVisualDataModelPrivate(0)))
{
    Q_D(QSGVisualDataModel);
    d->init();
}

QSGVisualDataModel::QSGVisualDataModel(QDeclarativeContext *ctxt, QObject *parent)
: QSGVisualModel(*(new QSGVisualDataModelPrivate(ctxt)), parent)
{
    Q_D(QSGVisualDataModel);
    d->init();
}

QSGVisualDataModel::~QSGVisualDataModel()
{
    Q_D(QSGVisualDataModel);
    foreach (QSGVisualDataModelCacheItem *cacheItem, d->m_cache) {
        cacheItem->object = 0;
        cacheItem->objectRef = 0;
        if (!cacheItem->isReferenced())
            delete cacheItem;
    }

    delete d->m_adaptorModel;
    if (d->m_cacheMetaType)
        d->m_cacheMetaType->release();
}


void QSGVisualDataModel::classBegin()
{
}

void QSGVisualDataModel::componentComplete()
{
    Q_D(QSGVisualDataModel);
    d->m_complete = true;

    int defaultGroups = 0;
    QStringList groupNames;
    groupNames.append(QStringLiteral("items"));
    groupNames.append(QStringLiteral("persistedItems"));
    if (QSGVisualDataGroupPrivate::get(d->m_items)->defaultInclude)
        defaultGroups |= Compositor::DefaultFlag;
    if (QSGVisualDataGroupPrivate::get(d->m_persistedItems)->defaultInclude)
        defaultGroups |= Compositor::PersistedFlag;
    for (int i = 3; i < d->m_groupCount; ++i) {
        QString name = d->m_groups[i]->name();
        if (name.isEmpty()) {
            d->m_groups[i] = d->m_groups[d->m_groupCount - 1];
            --d->m_groupCount;
            --i;
        } else if (name.at(0).isUpper()) {
            qmlInfo(d->m_groups[i]) << QSGVisualDataGroup::tr("Group names must start with a lower case letter");
            d->m_groups[i] = d->m_groups[d->m_groupCount - 1];
            --d->m_groupCount;
            --i;
        } else {
            groupNames.append(name);

            QSGVisualDataGroupPrivate *group = QSGVisualDataGroupPrivate::get(d->m_groups[i]);
            group->setModel(this, Compositor::Group(i));
            if (group->defaultInclude)
                defaultGroups |= (1 << i);
        }
    }
    if (!d->m_context)
        d->m_context = qmlContext(this);

    d->m_cacheMetaType = new QSGVisualDataModelCacheMetaType(
            QDeclarativeEnginePrivate::getV8Engine(d->m_context->engine()), this, groupNames);

    d->m_compositor.setGroupCount(d->m_groupCount);
    d->m_compositor.setDefaultGroups(defaultGroups);
    d->updateFilterGroup();

    while (!d->m_pendingParts.isEmpty())
        static_cast<QSGVisualPartsModel *>(d->m_pendingParts.first())->updateFilterGroup();

    d->connectModel(d->m_adaptorModel);
    QVector<Compositor::Insert> inserts;
    d->m_reset = true;
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
QVariant QSGVisualDataModel::model() const
{
    Q_D(const QSGVisualDataModel);
    return d->m_adaptorModel->model();
}

void QSGVisualDataModel::setModel(const QVariant &model)
{
    Q_D(QSGVisualDataModel);
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
QDeclarativeComponent *QSGVisualDataModel::delegate() const
{
    Q_D(const QSGVisualDataModel);
    return d->m_delegate;
}

void QSGVisualDataModel::setDelegate(QDeclarativeComponent *delegate)
{
    Q_D(QSGVisualDataModel);
    if (d->m_transaction) {
        qmlInfo(this) << tr("The delegate of a VisualDataModel cannot be changed within onUpdated.");
        return;
    }
    bool wasValid = d->m_delegate != 0;
    d->m_delegate = delegate;
    d->m_delegateValidated = false;
    if (wasValid && d->m_complete) {
        for (int i = 1; i < d->m_groupCount; ++i) {
            QSGVisualDataGroupPrivate::get(d->m_groups[i])->changeSet.remove(
                    0, d->m_compositor.count(Compositor::Group(i)));
        }
    }
    if (d->m_complete && d->m_delegate) {
        for (int i = 1; i < d->m_groupCount; ++i) {
            QSGVisualDataGroupPrivate::get(d->m_groups[i])->changeSet.insert(
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
    \snippet doc/src/snippets/declarative/visualdatamodel_rootindex/main.cpp 0

    \c view.qml:
    \snippet doc/src/snippets/declarative/visualdatamodel_rootindex/view.qml 0

    If the \l model is a QAbstractItemModel subclass, the delegate can also
    reference a \c hasModelChildren property (optionally qualified by a
    \e model. prefix) that indicates whether the delegate's model item has
    any child nodes.


    \sa modelIndex(), parentModelIndex()
*/
QVariant QSGVisualDataModel::rootIndex() const
{
    Q_D(const QSGVisualDataModel);
    return d->m_adaptorModel->rootIndex();
}

void QSGVisualDataModel::setRootIndex(const QVariant &root)
{
    Q_D(QSGVisualDataModel);
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
QVariant QSGVisualDataModel::modelIndex(int idx) const
{
    Q_D(const QSGVisualDataModel);
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
QVariant QSGVisualDataModel::parentModelIndex() const
{
    Q_D(const QSGVisualDataModel);
    return d->m_adaptorModel->parentModelIndex();
}

/*!
    \qmlproperty int QtQuick2::VisualDataModel::count
*/

int QSGVisualDataModel::count() const
{
    Q_D(const QSGVisualDataModel);
    if (!d->m_delegate)
        return 0;
    return d->m_compositor.count(d->m_compositorGroup);
}

void QSGVisualDataModelPrivate::destroy(QObject *object)
{
    QObjectPrivate *p = QObjectPrivate::get(object);
    Q_ASSERT(p->declarativeData);
    QDeclarativeData *data = static_cast<QDeclarativeData*>(p->declarativeData);
    if (data->ownContext && data->context)
        data->context->clearContext();
    object->deleteLater();
}

QSGVisualDataModel::ReleaseFlags QSGVisualDataModelPrivate::release(QObject *object)
{
    QSGVisualDataModel::ReleaseFlags stat = 0;
    if (!object)
        return stat;

    int cacheIndex = cacheIndexOf(object);
    if (cacheIndex != -1) {
        QSGVisualDataModelCacheItem *cacheItem = m_cache.at(cacheIndex);
        if (cacheItem->releaseObject()) {
            destroy(object);
            cacheItem->object = 0;
            stat |= QSGVisualModel::Destroyed;
            if (!cacheItem->isReferenced()) {
                m_compositor.clearFlags(Compositor::Cache, cacheIndex, 1, Compositor::CacheFlag);
                m_cache.removeAt(cacheIndex);
                delete cacheItem;
                Q_ASSERT(m_cache.count() == m_compositor.count(Compositor::Cache));
            }
        } else {
            stat |= QSGVisualDataModel::Referenced;
        }
    }
    return stat;
}

/*
  Returns ReleaseStatus flags.
*/

QSGVisualDataModel::ReleaseFlags QSGVisualDataModel::release(QSGItem *item)
{
    Q_D(QSGVisualDataModel);
    QSGVisualModel::ReleaseFlags stat = d->release(item);
    if (stat & Destroyed)
        item->setParentItem(0);
    return stat;
}

void QSGVisualDataModelPrivate::group_append(
        QDeclarativeListProperty<QSGVisualDataGroup> *property, QSGVisualDataGroup *group)
{
    QSGVisualDataModelPrivate *d = static_cast<QSGVisualDataModelPrivate *>(property->data);
    if (d->m_complete)
        return;
    if (d->m_groupCount == 11) {
        qmlInfo(d->q_func()) << QSGVisualDataModel::tr("The maximum number of supported VisualDataGroups is 8");
        return;
    }
    d->m_groups[d->m_groupCount] = group;
    d->m_groupCount += 1;
}

int QSGVisualDataModelPrivate::group_count(
        QDeclarativeListProperty<QSGVisualDataGroup> *property)
{
    QSGVisualDataModelPrivate *d = static_cast<QSGVisualDataModelPrivate *>(property->data);
    return d->m_groupCount - 1;
}

QSGVisualDataGroup *QSGVisualDataModelPrivate::group_at(
        QDeclarativeListProperty<QSGVisualDataGroup> *property, int index)
{
    QSGVisualDataModelPrivate *d = static_cast<QSGVisualDataModelPrivate *>(property->data);
    return index >= 0 && index < d->m_groupCount - 1
            ? d->m_groups[index - 1]
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

    \snippet doc/src/snippets/declarative/visualdatagroup.qml 0
*/

QDeclarativeListProperty<QSGVisualDataGroup> QSGVisualDataModel::groups()
{
    Q_D(QSGVisualDataModel);
    return QDeclarativeListProperty<QSGVisualDataGroup>(
            this,
            d,
            QSGVisualDataModelPrivate::group_append,
            QSGVisualDataModelPrivate::group_count,
            QSGVisualDataModelPrivate::group_at);
}

/*!
    \qmlproperty VisualDataGroup QtQuick2::VisualDataModel::items

    This property holds visual data model's default group to which all new items are added.
*/

QSGVisualDataGroup *QSGVisualDataModel::items()
{
    Q_D(QSGVisualDataModel);
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

QSGVisualDataGroup *QSGVisualDataModel::persistedItems()
{
    Q_D(QSGVisualDataModel);
    return d->m_persistedItems;
}

/*!
    \qmlproperty string QtQuick2::VisualDataModel::filterOnGroup

    This property holds the name of the group used to filter the visual data model.

    Only items which belong to this group are visible to a view.

    By default this is the \l items group.
*/

QString QSGVisualDataModel::filterGroup() const
{
    Q_D(const QSGVisualDataModel);
    return d->m_filterGroup;
}

void QSGVisualDataModel::setFilterGroup(const QString &group)
{
    Q_D(QSGVisualDataModel);

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

void QSGVisualDataModel::resetFilterGroup()
{
    setFilterGroup(QStringLiteral("items"));
}

void QSGVisualDataModelPrivate::updateFilterGroup()
{
    Q_Q(QSGVisualDataModel);
    if (!m_cacheMetaType)
        return;

    QDeclarativeListCompositor::Group previousGroup = m_compositorGroup;
    m_compositorGroup = Compositor::Default;
    for (int i = 1; i < m_groupCount; ++i) {
        if (m_filterGroup == m_cacheMetaType->groupNames.at(i - 1)) {
            m_compositorGroup = Compositor::Group(i);
            break;
        }
    }

    QSGVisualDataGroupPrivate::get(m_groups[m_compositorGroup])->emitters.insert(this);
    if (m_compositorGroup != previousGroup) {
        QVector<QDeclarativeChangeSet::Remove> removes;
        QVector<QDeclarativeChangeSet::Insert> inserts;
        m_compositor.transition(previousGroup, m_compositorGroup, &removes, &inserts);

        QDeclarativeChangeSet changeSet;
        changeSet.apply(removes, inserts);
        emit q->modelUpdated(changeSet, false);

        if (changeSet.difference() != 0)
            emit q->countChanged();

        if (m_parts) {
            foreach (QSGVisualPartsModel *model, m_parts->models)
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

QObject *QSGVisualDataModel::parts()
{
    Q_D(QSGVisualDataModel);
    if (!d->m_parts)
        d->m_parts = new QSGVisualDataModelParts(this);
    return d->m_parts;
}

void QSGVisualDataModelPrivate::emitCreatedPackage(Compositor::iterator at, QDeclarativePackage *package)
{
    for (int i = 1; i < m_groupCount; ++i)
        QSGVisualDataGroupPrivate::get(m_groups[i])->createdPackage(at.index[i], package);
}

void QSGVisualDataModelPrivate::emitDestroyingPackage(QDeclarativePackage *package)
{
    for (int i = 1; i < m_groupCount; ++i)
        QSGVisualDataGroupPrivate::get(m_groups[i])->destroyingPackage(package);
}

QObject *QSGVisualDataModelPrivate::object(Compositor::Group group, int index, bool complete, bool reference)
{
    Q_Q(QSGVisualDataModel);

    Compositor::iterator it = m_compositor.find(group, index);
    QSGVisualDataModelCacheItem *cacheItem = it->inCache() ? m_cache.at(it.cacheIndex) : 0;

    if (!cacheItem) {
        cacheItem = new QSGVisualDataModelCacheItem(m_cacheMetaType);
        for (int i = 0; i < m_groupCount; ++i)
            cacheItem->index[i] = it.index[i];
        cacheItem->groups = it->flags & Compositor::GroupMask;
    }

    if (!cacheItem->object) {
        QObject *data = m_adaptorModel->data(it.modelIndex());

        QDeclarativeContext *creationContext = m_delegate->creationContext();
        QDeclarativeContext *rootContext = new QDeclarativeContext(
                creationContext ? creationContext : m_context.data());
        QDeclarativeContext *ctxt = rootContext;
        if (m_adaptorModel->flags() & QSGVisualAdaptorModel::ProxiedObject) {
            if (QSGVisualAdaptorModelProxyInterface *proxy = qobject_cast<QSGVisualAdaptorModelProxyInterface *>(data)) {
                ctxt->setContextObject(proxy->proxiedObject());
                ctxt = new QDeclarativeContext(ctxt, ctxt);
            }
        }

        QDeclarative_setParent_noEvent(data, ctxt);
        ctxt->setContextProperty(QLatin1String("model"), data);
        ctxt->setContextObject(data);

        m_completePending = false;
        cacheItem->object = m_delegate->beginCreate(ctxt);

        if (cacheItem->object) {
            QDeclarative_setParent_noEvent(rootContext, cacheItem->object);
            if (!it->inCache()) {
                m_cache.insert(it.cacheIndex, cacheItem);
                m_compositor.setFlags(it, 1, Compositor::CacheFlag);
                Q_ASSERT(m_cache.count() == m_compositor.count(Compositor::Cache));
            }

            cacheItem->attached = QSGVisualDataModelAttached::properties(cacheItem->object);
            cacheItem->attached->m_cacheItem = cacheItem;
            new QSGVisualDataModelAttachedMetaObject(cacheItem->attached, m_cacheMetaType);
            cacheItem->attached->emitChanges();

            if (QDeclarativePackage *package = qobject_cast<QDeclarativePackage *>(cacheItem->object)) {
                emitCreatedPackage(it, package);
            } else if (!reference) {
                if (QSGItem *item = qobject_cast<QSGItem *>(cacheItem->object))
                    emitCreatedItem(it, item);
            }

            m_completePending = !complete;
            if (complete)
                m_delegate->completeCreate();
        } else {
            delete rootContext;
            if (!it->inCache())
                delete cacheItem;
            qmlInfo(q, m_delegate->errors()) << "Error creating delegate";
            return 0;
        }
    }

    if (index == m_compositor.count(group) - 1 && m_adaptorModel->canFetchMore())
        QCoreApplication::postEvent(q, new QEvent(QEvent::UpdateRequest));
    if (reference)
        cacheItem->referenceObject();
    return cacheItem->object;
}

QSGItem *QSGVisualDataModel::item(int index, bool complete)
{
    Q_D(QSGVisualDataModel);
    if (!d->m_delegate || index < 0 || index >= d->m_compositor.count(d->m_compositorGroup)) {
        qWarning() << "VisualDataModel::item: index out range" << index << d->m_compositor.count(d->m_compositorGroup);
        return 0;
    }

    QObject *object = d->object(d->m_compositorGroup, index, complete, true);
    if (QSGItem *item = qobject_cast<QSGItem *>(object))
        return item;
    if (d->m_completePending)
        completeItem();
    d->release(object);
    if (!d->m_delegateValidated) {
        if (object)
            qmlInfo(d->m_delegate) << QSGVisualDataModel::tr("Delegate component must be Item type.");
        d->m_delegateValidated = true;
    }
    return 0;
}

bool QSGVisualDataModel::completePending() const
{
    Q_D(const QSGVisualDataModel);
    return d->m_completePending;
}

void QSGVisualDataModel::completeItem()
{
    Q_D(QSGVisualDataModel);
    d->m_delegate->completeCreate();
    d->m_completePending = false;
}

QString QSGVisualDataModelPrivate::stringValue(Compositor::Group group, int index, const QString &name)
{
    Compositor::iterator it = m_compositor.find(group, index);
    if (QSGVisualAdaptorModel *model = it.list<QSGVisualAdaptorModel>()) {
        return model->stringValue(it.modelIndex(), name);
    }
    return QString();
}

QString QSGVisualDataModel::stringValue(int index, const QString &name)
{
    Q_D(QSGVisualDataModel);
    return d->stringValue(d->m_compositorGroup, index, name);
}

int QSGVisualDataModelPrivate::cacheIndexOf(QObject *object) const
{
    for (int cacheIndex = 0; cacheIndex < m_cache.count(); ++cacheIndex) {
        if (m_cache.at(cacheIndex)->object == object)
            return cacheIndex;
    }
    return -1;
}

int QSGVisualDataModel::indexOf(QSGItem *item, QObject *) const
{
    Q_D(const QSGVisualDataModel);
    const int cacheIndex = d->cacheIndexOf(item);
    return cacheIndex != -1
            ? d->m_cache.at(cacheIndex)->index[d->m_compositorGroup]
            : -1;
}

void QSGVisualDataModel::setWatchedRoles(QList<QByteArray> roles)
{
    Q_D(QSGVisualDataModel);
    d->m_adaptorModel->replaceWatchedRoles(d->watchedRoles, roles);
    d->watchedRoles = roles;
}

void QSGVisualDataModelPrivate::addGroups(Compositor::Group group, int index, int count, int groupFlags)
{
    QVector<Compositor::Insert> inserts;
    m_compositor.setFlags(group, index, count, groupFlags, &inserts);
    itemsInserted(inserts);
    emitChanges();
}

void QSGVisualDataModelPrivate::removeGroups(Compositor::Group group, int index, int count, int groupFlags)
{
    QVector<Compositor::Remove> removes;
    m_compositor.clearFlags(group, index, count, groupFlags, &removes);
    itemsRemoved(removes);
    emitChanges();
}

void QSGVisualDataModelPrivate::setGroups(Compositor::Group group, int index, int count, int groupFlags)
{
    QVector<Compositor::Insert> inserts;
    m_compositor.setFlags(group, index, count, groupFlags, &inserts);
    itemsInserted(inserts);

    const int removeFlags = ~groupFlags & Compositor::GroupMask;
    QVector<Compositor::Remove> removes;
    m_compositor.clearFlags(group, index, count, removeFlags, &removes);
    itemsRemoved(removes);

    emitChanges();
}

bool QSGVisualDataModel::event(QEvent *e)
{
    Q_D(QSGVisualDataModel);
    if (e->type() == QEvent::UpdateRequest)
        d->m_adaptorModel->fetchMore();
    return QSGVisualModel::event(e);
}

void QSGVisualDataModelPrivate::itemsChanged(const QVector<Compositor::Change> &changes)
{
    if (!m_delegate)
        return;

    QVarLengthArray<QVector<QDeclarativeChangeSet::Change>, Compositor::MaximumGroupCount> translatedChanges(m_groupCount);

    foreach (const Compositor::Change &change, changes) {
        for (int i = 1; i < m_groupCount; ++i) {
            if (change.inGroup(i)) {
                translatedChanges[i].append(
                        QDeclarativeChangeSet::Change(change.index[i], change.count));
            }
        }
    }

    for (int i = 1; i < m_groupCount; ++i)
        QSGVisualDataGroupPrivate::get(m_groups[i])->changeSet.apply(translatedChanges.at(i));
}

void QSGVisualDataModel::_q_itemsChanged(int index, int count)
{
    Q_D(QSGVisualDataModel);
    if (count <= 0)
        return;
    QVector<Compositor::Change> changes;
    d->m_compositor.listItemsChanged(d->m_adaptorModel, index, count, &changes);
    d->itemsChanged(changes);
    d->emitChanges();
}

void QSGVisualDataModelPrivate::itemsInserted(
        const QVector<Compositor::Insert> &inserts,
        QVarLengthArray<QVector<QDeclarativeChangeSet::Insert>, Compositor::MaximumGroupCount> *translatedInserts,
        QHash<int, QList<QSGVisualDataModelCacheItem *> > *movedItems)
{
    int cacheIndex = 0;

    int inserted[Compositor::MaximumGroupCount];
    for (int i = 1; i < m_groupCount; ++i)
        inserted[i] = 0;

    foreach (const Compositor::Insert &insert, inserts) {
        for (; cacheIndex < insert.cacheIndex; ++cacheIndex) {
            QSGVisualDataModelCacheItem *cacheItem = m_cache.at(cacheIndex);
            if (!cacheItem->groups)
                continue;
            for (int i = 1; i < m_groupCount; ++i)
                cacheItem->index[i] += inserted[i];
        }
        for (int i = 1; i < m_groupCount; ++i) {
            if (insert.inGroup(i)) {
                (*translatedInserts)[i].append(
                        QDeclarativeChangeSet::Insert(insert.index[i], insert.count, insert.moveId));
                inserted[i] += insert.count;
            }
        }

        if (!insert.inCache())
            continue;

        if (movedItems && insert.isMove()) {
            QList<QSGVisualDataModelCacheItem *> items = movedItems->take(insert.moveId);
            Q_ASSERT(items.count() == insert.count);
            m_cache = m_cache.mid(0, insert.cacheIndex) + items + m_cache.mid(insert.cacheIndex);
        }
        if (insert.inGroup()) {
            for (int offset = 0; cacheIndex < insert.cacheIndex + insert.count; ++cacheIndex, ++offset) {
                QSGVisualDataModelCacheItem *cacheItem = m_cache.at(cacheIndex);
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
        QSGVisualDataModelCacheItem *cacheItem = m_cache.at(cacheIndex);
        if (!cacheItem->groups)
            continue;
        for (int i = 1; i < m_groupCount; ++i)
            cacheItem->index[i] += inserted[i];
    }
}

void QSGVisualDataModelPrivate::itemsInserted(const QVector<Compositor::Insert> &inserts)
{
    QVarLengthArray<QVector<QDeclarativeChangeSet::Insert>, Compositor::MaximumGroupCount> translatedInserts(m_groupCount);
    itemsInserted(inserts, &translatedInserts);
    Q_ASSERT(m_cache.count() == m_compositor.count(Compositor::Cache));
    if (!m_delegate)
        return;

    for (int i = 1; i < m_groupCount; ++i)
        QSGVisualDataGroupPrivate::get(m_groups[i])->changeSet.apply(translatedInserts.at(i));
}

void QSGVisualDataModel::_q_itemsInserted(int index, int count)
{

    Q_D(QSGVisualDataModel);
    if (count <= 0)
        return;
    QVector<Compositor::Insert> inserts;
    d->m_compositor.listItemsInserted(d->m_adaptorModel, index, count, &inserts);
    d->itemsInserted(inserts);
    d->emitChanges();
}

void QSGVisualDataModelPrivate::itemsRemoved(
        const QVector<Compositor::Remove> &removes,
        QVarLengthArray<QVector<QDeclarativeChangeSet::Remove>, Compositor::MaximumGroupCount> *translatedRemoves,
        QHash<int, QList<QSGVisualDataModelCacheItem *> > *movedItems)
{
    int cacheIndex = 0;
    int removedCache = 0;

    int removed[Compositor::MaximumGroupCount];
    for (int i = 1; i < m_groupCount; ++i)
        removed[i] = 0;

    foreach (const Compositor::Remove &remove, removes) {
        for (; cacheIndex < remove.cacheIndex; ++cacheIndex) {
            QSGVisualDataModelCacheItem *cacheItem = m_cache.at(cacheIndex);
            if (!cacheItem->groups)
                continue;
            for (int i = 1; i < m_groupCount; ++i)
                cacheItem->index[i] -= removed[i];
        }
        for (int i = 1; i < m_groupCount; ++i) {
            if (remove.inGroup(i)) {
                (*translatedRemoves)[i].append(
                        QDeclarativeChangeSet::Remove(remove.index[i], remove.count, remove.moveId));
                removed[i] += remove.count;
            }
        }

        if (!remove.inCache())
            continue;

        if (movedItems && remove.isMove()) {
            movedItems->insert(remove.moveId, m_cache.mid(remove.cacheIndex, remove.count));
            QList<QSGVisualDataModelCacheItem *>::iterator begin = m_cache.begin() + remove.cacheIndex;
            QList<QSGVisualDataModelCacheItem *>::iterator end = begin + remove.count;
            m_cache.erase(begin, end);
        } else {
            for (; cacheIndex < remove.cacheIndex + remove.count - removedCache; ++cacheIndex) {
                QSGVisualDataModelCacheItem *cacheItem = m_cache.at(cacheIndex);
                if (remove.inGroup(Compositor::Persisted) && cacheItem->objectRef == 0) {
                    destroy(cacheItem->object);
                    if (QDeclarativePackage *package = qobject_cast<QDeclarativePackage *>(cacheItem->object))
                        emitDestroyingPackage(package);
                    else if (QSGItem *item = qobject_cast<QSGItem *>(cacheItem->object))
                        emitDestroyingItem(item);
                    cacheItem->object = 0;
                }
                if (remove.groups() == cacheItem->groups && !cacheItem->isReferenced()) {
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
                    cacheItem->groups &= ~remove.flags & Compositor::GroupMask;
                }
            }
        }
    }

    for (; cacheIndex < m_cache.count(); ++cacheIndex) {
        QSGVisualDataModelCacheItem *cacheItem = m_cache.at(cacheIndex);
        if (!cacheItem->groups)
            continue;
        for (int i = 1; i < m_groupCount; ++i)
            cacheItem->index[i] -= removed[i];
    }
}

void QSGVisualDataModelPrivate::itemsRemoved(const QVector<Compositor::Remove> &removes)
{
    QVarLengthArray<QVector<QDeclarativeChangeSet::Remove>, Compositor::MaximumGroupCount> translatedRemoves(m_groupCount);
    itemsRemoved(removes, &translatedRemoves);
    Q_ASSERT(m_cache.count() == m_compositor.count(Compositor::Cache));
    if (!m_delegate)
        return;

    for (int i = 1; i < m_groupCount; ++i)
       QSGVisualDataGroupPrivate::get(m_groups[i])->changeSet.apply(translatedRemoves.at(i));
}

void QSGVisualDataModel::_q_itemsRemoved(int index, int count)
{
    Q_D(QSGVisualDataModel);
    if (count <= 0)
        return;

    QVector<Compositor::Remove> removes;
    d->m_compositor.listItemsRemoved(d->m_adaptorModel, index, count, &removes);
    d->itemsRemoved(removes);
    d->emitChanges();
}

void QSGVisualDataModelPrivate::itemsMoved(
        const QVector<Compositor::Remove> &removes, const QVector<Compositor::Insert> &inserts)
{
    QHash<int, QList<QSGVisualDataModelCacheItem *> > movedItems;

    QVarLengthArray<QVector<QDeclarativeChangeSet::Remove>, Compositor::MaximumGroupCount> translatedRemoves(m_groupCount);
    itemsRemoved(removes, &translatedRemoves, &movedItems);

    QVarLengthArray<QVector<QDeclarativeChangeSet::Insert>, Compositor::MaximumGroupCount> translatedInserts(m_groupCount);
    itemsInserted(inserts, &translatedInserts, &movedItems);
    Q_ASSERT(m_cache.count() == m_compositor.count(Compositor::Cache));
    Q_ASSERT(movedItems.isEmpty());
    if (!m_delegate)
        return;

    for (int i = 1; i < m_groupCount; ++i) {
        QSGVisualDataGroupPrivate::get(m_groups[i])->changeSet.apply(
                    translatedRemoves.at(i),
                    translatedInserts.at(i));
    }
}

void QSGVisualDataModel::_q_itemsMoved(int from, int to, int count)
{
    Q_D(QSGVisualDataModel);
    if (count <= 0)
        return;

    QVector<Compositor::Remove> removes;
    QVector<Compositor::Insert> inserts;
    d->m_compositor.listItemsMoved(d->m_adaptorModel, from, to, count, &removes, &inserts);
    d->itemsMoved(removes, inserts);
    d->emitChanges();
}

template <typename T> v8::Local<v8::Array>
QSGVisualDataModelPrivate::buildChangeList(const QVector<T> &changes)
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

void QSGVisualDataModelPrivate::emitModelUpdated(const QDeclarativeChangeSet &changeSet, bool reset)
{
    Q_Q(QSGVisualDataModel);
    emit q->modelUpdated(changeSet, reset);
    if (changeSet.difference() != 0)
        emit q->countChanged();
}

void QSGVisualDataModelPrivate::emitChanges()
{
    if (m_transaction || !m_complete)
        return;

    m_transaction = true;
    QV8Engine *engine = QDeclarativeEnginePrivate::getV8Engine(m_context->engine());
    for (int i = 1; i < m_groupCount; ++i)
        QSGVisualDataGroupPrivate::get(m_groups[i])->emitChanges(engine);
    m_transaction = false;

    const bool reset = m_reset;
    m_reset = false;
    for (int i = 1; i < m_groupCount; ++i)
        QSGVisualDataGroupPrivate::get(m_groups[i])->emitModelUpdated(reset);

    foreach (QSGVisualDataModelCacheItem *cacheItem, m_cache) {
        if (cacheItem->object)
            cacheItem->attached->emitChanges();
    }
}

void QSGVisualDataModel::_q_modelReset(int oldCount, int newCount)
{
    Q_D(QSGVisualDataModel);
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

QSGVisualDataModelAttached *QSGVisualDataModel::qmlAttachedProperties(QObject *obj)
{
    return QSGVisualDataModelAttached::properties(obj);
}

//============================================================================

QSGVisualDataModelCacheMetaType::QSGVisualDataModelCacheMetaType(
        QV8Engine *engine, QSGVisualDataModel *model, const QStringList &groupNames)
    : model(model)
    , groupCount(groupNames.count() + 1)
    , memberPropertyOffset(QSGVisualDataModelAttached::staticMetaObject.propertyCount())
    , indexPropertyOffset(QSGVisualDataModelAttached::staticMetaObject.propertyCount() + groupNames.count())
    , v8Engine(engine)
    , metaObject(0)
    , groupNames(groupNames)
{
    QMetaObjectBuilder builder;
    builder.setFlags(QMetaObjectBuilder::DynamicMetaObject);
    builder.setClassName(QSGVisualDataModelAttached::staticMetaObject.className());
    builder.setSuperClass(&QSGVisualDataModelAttached::staticMetaObject);

    v8::HandleScope handleScope;
    v8::Context::Scope contextScope(engine->context());
    v8::Local<v8::FunctionTemplate> ft = v8::FunctionTemplate::New();
    ft->InstanceTemplate()->SetHasExternalResource(true);
    ft->PrototypeTemplate()->SetAccessor(v8::String::New("model"), get_model);
    ft->PrototypeTemplate()->SetAccessor(v8::String::New("groups"), get_groups, set_groups);

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

QSGVisualDataModelCacheMetaType::~QSGVisualDataModelCacheMetaType()
{
    qFree(metaObject);
    qPersistentDispose(constructor);
}

int QSGVisualDataModelCacheMetaType::parseGroups(const QStringList &groups) const
{
    int groupFlags = 0;
    foreach (const QString &groupName, groups) {
        int index = groupNames.indexOf(groupName);
        if (index != -1)
            groupFlags |= 2 << index;
    }
    return groupFlags;
}

int QSGVisualDataModelCacheMetaType::parseGroups(QV8Engine *engine, const v8::Local<v8::Value> &groups) const
{
    int groupFlags = 0;
    if (groups->IsString()) {
        const QString groupName = engine->toString(groups);
        int index = groupNames.indexOf(groupName);
        if (index != -1)
            groupFlags |= 2 << index;
    } else if (groups->IsArray()) {
        v8::Local<v8::Array> array = v8::Local<v8::Array>::Cast(groups);
        for (uint i = 0; i < array->Length(); ++i) {
            const QString groupName = engine->toString(array->Get(i));
            int index = groupNames.indexOf(groupName);
            if (index != -1)
                groupFlags |= 2 << index;
        }
    }
    return groupFlags;
}

v8::Handle<v8::Value> QSGVisualDataModelCacheMetaType::get_model(
        v8::Local<v8::String>, const v8::AccessorInfo &info)
{
    QSGVisualDataModelCacheItem *cacheItem = v8_resource_cast<QSGVisualDataModelCacheItem>(info.This());
    if (!cacheItem)
        V8THROW_ERROR("Not a valid VisualData object");
    if (!cacheItem->metaType->model)
        return v8::Undefined();
    QObject *data = 0;

    QSGVisualDataModelPrivate *model = QSGVisualDataModelPrivate::get(cacheItem->metaType->model);
    for (int i = 1; i < cacheItem->metaType->groupCount; ++i) {
        if (cacheItem->groups & (1 << i)) {
            Compositor::iterator it = model->m_compositor.find(
                    Compositor::Group(i), cacheItem->index[i]);
            if (QSGVisualAdaptorModel *list = it.list<QSGVisualAdaptorModel>())
                data = list->data(it.modelIndex());
            break;
        }
    }
    if (!data)
        return v8::Undefined();
    return cacheItem->engine->newQObject(data);
}

v8::Handle<v8::Value> QSGVisualDataModelCacheMetaType::get_groups(
        v8::Local<v8::String>, const v8::AccessorInfo &info)
{
    QSGVisualDataModelCacheItem *cacheItem = v8_resource_cast<QSGVisualDataModelCacheItem>(info.This());
    if (!cacheItem)
        V8THROW_ERROR("Not a valid VisualData object");

    QStringList groups;
    for (int i = 1; i < cacheItem->metaType->groupCount; ++i) {
        if (cacheItem->groups & (1 << i))
            groups.append(cacheItem->metaType->groupNames.at(i - 1));
    }

    return cacheItem->engine->fromVariant(groups);
}

void QSGVisualDataModelCacheMetaType::set_groups(
        v8::Local<v8::String>, v8::Local<v8::Value> value, const v8::AccessorInfo &info)
{
    QSGVisualDataModelCacheItem *cacheItem = v8_resource_cast<QSGVisualDataModelCacheItem>(info.This());
    if (!cacheItem)
        V8THROW_ERROR_SETTER("Not a valid VisualData object");

    if (!cacheItem->metaType->model)
        return;
    QSGVisualDataModelPrivate *model = QSGVisualDataModelPrivate::get(cacheItem->metaType->model);

    const int groupFlags = model->m_cacheMetaType->parseGroups(cacheItem->engine, value);
    for (int i = 1; i < cacheItem->metaType->groupCount; ++i) {
        if (cacheItem->groups & (1 << i)) {
            model->setGroups(Compositor::Group(i), cacheItem->index[i], 1, groupFlags);
            break;
        }
    }
}

v8::Handle<v8::Value> QSGVisualDataModelCacheMetaType::get_member(
        v8::Local<v8::String>, const v8::AccessorInfo &info)
{
    QSGVisualDataModelCacheItem *cacheItem = v8_resource_cast<QSGVisualDataModelCacheItem>(info.This());
    if (!cacheItem)
        V8THROW_ERROR("Not a valid VisualData object");

    return v8::Boolean::New(cacheItem->groups & (1 << info.Data()->Int32Value()));
}

void QSGVisualDataModelCacheMetaType::set_member(
        v8::Local<v8::String>, v8::Local<v8::Value> value, const v8::AccessorInfo &info)
{
    QSGVisualDataModelCacheItem *cacheItem = v8_resource_cast<QSGVisualDataModelCacheItem>(info.This());
    if (!cacheItem)
        V8THROW_ERROR_SETTER("Not a valid VisualData object");

    if (!cacheItem->metaType->model)
        return;
    QSGVisualDataModelPrivate *model = QSGVisualDataModelPrivate::get(cacheItem->metaType->model);

    Compositor::Group group = Compositor::Group(info.Data()->Int32Value());
    const bool member = value->BooleanValue();
    const int groupFlag = (1 << group);
    if (member == ((cacheItem->groups & groupFlag) != 0))
        return;

    for (int i = 1; i < cacheItem->metaType->groupCount; ++i) {
        if (cacheItem->groups & (1 << i)) {
            if (member)
                model->addGroups(Compositor::Group(i), cacheItem->index[i], 1, groupFlag);
            else
                model->removeGroups(Compositor::Group(i), cacheItem->index[i], 1, groupFlag);
            break;
        }
    }
}

v8::Handle<v8::Value> QSGVisualDataModelCacheMetaType::get_index(
        v8::Local<v8::String>, const v8::AccessorInfo &info)
{
    QSGVisualDataModelCacheItem *cacheItem = v8_resource_cast<QSGVisualDataModelCacheItem>(info.This());
    if (!cacheItem)
        V8THROW_ERROR("Not a valid VisualData object");

    return v8::Integer::New(cacheItem->index[info.Data()->Int32Value()]);
}


//---------------------------------------------------------------------------

void QSGVisualDataModelCacheItem::Dispose()
{
    --scriptRef;
    if (isReferenced())
        return;

    if (metaType->model) {
        QSGVisualDataModelPrivate *model = QSGVisualDataModelPrivate::get(metaType->model);
        const int cacheIndex = model->m_cache.indexOf(this);
        if (cacheIndex != -1) {
            model->m_compositor.clearFlags(Compositor::Cache, cacheIndex, 1, Compositor::CacheFlag);
            model->m_cache.removeAt(cacheIndex);
        }
    }
    delete this;
}

//---------------------------------------------------------------------------

QSGVisualDataModelAttachedMetaObject::QSGVisualDataModelAttachedMetaObject(
        QSGVisualDataModelAttached *attached, QSGVisualDataModelCacheMetaType *metaType)
    : attached(attached)
    , metaType(metaType)
{
    metaType->addref();
    *static_cast<QMetaObject *>(this) = *metaType->metaObject;
    QObjectPrivate::get(attached)->metaObject = this;
}

QSGVisualDataModelAttachedMetaObject::~QSGVisualDataModelAttachedMetaObject()
{
    metaType->release();
}

int QSGVisualDataModelAttachedMetaObject::metaCall(QMetaObject::Call call, int _id, void **arguments)
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
            Compositor::Group group = Compositor::Group(_id - metaType->memberPropertyOffset + 1);
            const bool member = attached->m_cacheItem->groups & (1 << group);
            if (member != *static_cast<bool *>(arguments[0])) {
                QSGVisualDataModelPrivate *model = QSGVisualDataModelPrivate::get(metaType->model);
                const int cacheIndex = model->m_cache.indexOf(attached->m_cacheItem);
                if (member)
                    model->removeGroups(Compositor::Cache, cacheIndex, 1, (1 << group));
                else
                    model->addGroups(Compositor::Cache, cacheIndex, 1, (1 << group));
            }
            return -1;
        }
    }
    return attached->qt_metacall(call, _id, arguments);
}

/*!
    \qmlattachedproperty int QtQuick2::VisualDataModel::model

    This attached property holds the visual data model this delegate instance belongs to.

    It is attached to each instance of the delegate.
*/

QSGVisualDataModel *QSGVisualDataModelAttached::model() const
{
    return m_cacheItem ? m_cacheItem->metaType->model : 0;
}

/*!
    \qmlattachedproperty stringlist QtQuick2::VisualDataModel::groups

    This attached property holds the name of VisualDataGroups the item belongs to.

    It is attached to each instance of the delegate.
*/

QStringList QSGVisualDataModelAttached::groups() const
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

void QSGVisualDataModelAttached::setGroups(const QStringList &groups)
{
    if (!m_cacheItem)
        return;

    QSGVisualDataModelPrivate *model = QSGVisualDataModelPrivate::get(m_cacheItem->metaType->model);

    const int cacheIndex = model->m_cache.indexOf(m_cacheItem);
    const int groupFlags = model->m_cacheMetaType->parseGroups(groups);
    model->setGroups(Compositor::Cache, cacheIndex, 1, groupFlags);
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

void QSGVisualDataModelAttached::emitChanges()
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

void QSGVisualDataGroupPrivate::setModel(QSGVisualDataModel *m, Compositor::Group g)
{
    Q_ASSERT(!model);
    model = m;
    group = g;
}

void QSGVisualDataGroupPrivate::emitChanges(QV8Engine *engine)
{
    Q_Q(QSGVisualDataGroup);
    static int idx = signalIndex("changed(QDeclarativeV8Handle,QDeclarativeV8Handle)");
    if (isSignalConnected(idx)) {
        v8::HandleScope handleScope;
        v8::Context::Scope contextScope(engine->context());
        v8::Local<v8::Array> removed  = QSGVisualDataModelPrivate::buildChangeList(changeSet.removes());
        v8::Local<v8::Array> inserted = QSGVisualDataModelPrivate::buildChangeList(changeSet.inserts());
        emit q->changed(
                QDeclarativeV8Handle::fromHandle(removed), QDeclarativeV8Handle::fromHandle(inserted));
    }
    if (changeSet.difference() != 0)
        emit q->countChanged();
}

void QSGVisualDataGroupPrivate::emitModelUpdated(bool reset)
{
    for (QSGVisualDataGroupEmitterList::iterator it = emitters.begin(); it != emitters.end(); ++it)
        it->emitModelUpdated(changeSet, reset);
    changeSet.clear();
}

void QSGVisualDataGroupPrivate::createdPackage(int index, QDeclarativePackage *package)
{
    for (QSGVisualDataGroupEmitterList::iterator it = emitters.begin(); it != emitters.end(); ++it)
        it->createdPackage(index, package);
}

void QSGVisualDataGroupPrivate::destroyingPackage(QDeclarativePackage *package)
{
    for (QSGVisualDataGroupEmitterList::iterator it = emitters.begin(); it != emitters.end(); ++it)
        it->destroyingPackage(package);
}

/*!
    \qmlclass VisualDataGroup QSGVisualDataGroup
    \inqmlmodule QtQuick 2
    \ingroup qml-working-with-data
    \brief The VisualDataGroup encapsulates a filtered set of visual data items.

*/

QSGVisualDataGroup::QSGVisualDataGroup(QObject *parent)
    : QObject(*new QSGVisualDataGroupPrivate, parent)
{
}

QSGVisualDataGroup::QSGVisualDataGroup(
        const QString &name, QSGVisualDataModel *model, int index, QObject *parent)
    : QObject(*new QSGVisualDataGroupPrivate, parent)
{
    Q_D(QSGVisualDataGroup);
    d->name = name;
    d->setModel(model, Compositor::Group(index));
}

QSGVisualDataGroup::~QSGVisualDataGroup()
{
}

/*!
    \qmlproperty string QtQuick2::VisualDataGroup::name

    This property holds the name of the group.

    Each group in a model must have a unique name starting with a lower case letter.
*/

QString QSGVisualDataGroup::name() const
{
    Q_D(const QSGVisualDataGroup);
    return d->name;
}

void QSGVisualDataGroup::setName(const QString &name)
{
    Q_D(QSGVisualDataGroup);
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

int QSGVisualDataGroup::count() const
{
    Q_D(const QSGVisualDataGroup);
    if (!d->model)
        return 0;
    return QSGVisualDataModelPrivate::get(d->model)->m_compositor.count(d->group);
}

/*!
    \qmlproperty bool QtQuick2::VisualDataGroup::includeByDefault

    This property holds whether new items are assigned to this group by default.
*/

bool QSGVisualDataGroup::defaultInclude() const
{
    Q_D(const QSGVisualDataGroup);
    return d->defaultInclude;
}

void QSGVisualDataGroup::setDefaultInclude(bool include)
{
    Q_D(QSGVisualDataGroup);
    if (d->defaultInclude != include) {
        d->defaultInclude = include;

        if (d->model) {
            if (include)
                QSGVisualDataModelPrivate::get(d->model)->m_compositor.setDefaultGroup(d->group);
            else
                QSGVisualDataModelPrivate::get(d->model)->m_compositor.clearDefaultGroup(d->group);
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

QDeclarativeV8Handle QSGVisualDataGroup::get(int index)
{
    Q_D(QSGVisualDataGroup);
    if (!d->model)
        return QDeclarativeV8Handle::fromHandle(v8::Undefined());;

    QSGVisualDataModelPrivate *model = QSGVisualDataModelPrivate::get(d->model);
    if (index < 0 || index >= model->m_compositor.count(d->group)) {
        qmlInfo(this) << tr("get: index out of range");
        return QDeclarativeV8Handle::fromHandle(v8::Undefined());
    }

    Compositor::iterator it = model->m_compositor.find(d->group, index);
    QSGVisualDataModelCacheItem *cacheItem = it->inCache()
            ? model->m_cache.at(it.cacheIndex)
            : 0;

    if (!cacheItem) {
        cacheItem = new QSGVisualDataModelCacheItem(model->m_cacheMetaType);
        for (int i = 0; i < model->m_groupCount; ++i)
            cacheItem->index[i] = it.index[i];
        cacheItem->groups = it->flags & Compositor::GroupMask;

        model->m_cache.insert(it.cacheIndex, cacheItem);
        model->m_compositor.setFlags(it, 1, Compositor::CacheFlag);
    }

    ++cacheItem->scriptRef;

    v8::Local<v8::Object> rv = model->m_cacheMetaType->constructor->NewInstance();
    rv->SetExternalResource(cacheItem);
    return QDeclarativeV8Handle::fromHandle(rv);
}

/*!
    \qmlmethod QtQuick2::VisualDataGroup::create(int index)

    Returns a reference to the instantiated item at \a index in the group.

    All items returned by create are added to the persistedItems group.  Items in this
    group remain instantiated when not referenced by any view.
*/

QObject *QSGVisualDataGroup::create(int index)
{
    Q_D(QSGVisualDataGroup);
    if (!d->model)
        return 0;

    QSGVisualDataModelPrivate *model = QSGVisualDataModelPrivate::get(d->model);
    if (index < 0 || index >= model->m_compositor.count(d->group)) {
        qmlInfo(this) << tr("create: index out of range");
        return 0;
    }

    QObject *object = model->object(d->group, index, true, false);
    if (object)
        model->addGroups(d->group, index, 1, Compositor::PersistedFlag);
    return object;
}

/*!
    \qmlmethod QtQuick2::VisualDataGroup::remove(int index, int count)

    Removes \a count items starting at \a index from the group.
*/

void QSGVisualDataGroup::remove(QDeclarativeV8Function *args)
{
    Q_D(QSGVisualDataGroup);
    if (!d->model)
        return;
    int index = -1;
    int count = 1;

    if (args->Length() == 0)
        return;

    int i = 0;
    v8::Local<v8::Value> v = (*args)[i];
    if (!v->IsInt32())
        return;
    index = v->Int32Value();

    if (++i < args->Length()) {
        v = (*args)[i];
        if (v->IsInt32())
            count = v->Int32Value();
    }

    QSGVisualDataModelPrivate *model = QSGVisualDataModelPrivate::get(d->model);
    if (count < 0) {
        qmlInfo(this) << tr("remove: invalid count");
    } else if (index < 0 || index + count > model->m_compositor.count(d->group)) {
        qmlInfo(this) << tr("remove: index out of range");
    } else if (count > 0) {
        model->removeGroups(d->group, index, count, 1 << d->group);
    }
}

bool QSGVisualDataGroupPrivate::parseGroupArgs(
        QDeclarativeV8Function *args, int *index, int *count, int *groups) const
{
    if (!model)
        return false;

    if (args->Length() < 2)
        return false;

    int i = 0;
    v8::Local<v8::Value> v = (*args)[i];
    if (!v->IsInt32())
        return false;
    *index = v->Int32Value();

    v = (*args)[++i];
    if (v->IsInt32()) {
        *count = v->Int32Value();

        if (++i == args->Length())
            return false;
        v = (*args)[i];
    }

    *groups = QSGVisualDataModelPrivate::get(model)->m_cacheMetaType->parseGroups(args->engine(), v);

    return true;
}

/*!
    \qmlmethod QtQuick2::VisualDataGroup::addGroups(int index, int count, stringlist groups)

    Adds \a count items starting at \a index to \a groups.
*/

void QSGVisualDataGroup::addGroups(QDeclarativeV8Function *args)
{
    Q_D(QSGVisualDataGroup);
    int index = -1;
    int count = 1;
    int groups = 0;

    if (!d->parseGroupArgs(args, &index, &count, &groups))
        return;

    QSGVisualDataModelPrivate *model = QSGVisualDataModelPrivate::get(d->model);
    if (count < 0) {
        qmlInfo(this) << tr("addGroups: invalid count");
    } else if (index < 0 || index + count > model->m_compositor.count(d->group)) {
        qmlInfo(this) << tr("addGroups: index out of range");
    } else if (count > 0 && groups) {
        model->addGroups(d->group, index, count, groups);
    }
}

/*!
    \qmlmethod QtQuick2::VisualDataGroup::removeGroups(int index, int count, stringlist groups)

    Removes \a count items starting at \a index from \a groups.
*/

void QSGVisualDataGroup::removeGroups(QDeclarativeV8Function *args)
{
    Q_D(QSGVisualDataGroup);
    int index = -1;
    int count = 1;
    int groups = 0;

    if (!d->parseGroupArgs(args, &index, &count, &groups))
        return;

    QSGVisualDataModelPrivate *model = QSGVisualDataModelPrivate::get(d->model);
    if (count < 0) {
        qmlInfo(this) << tr("removeGroups: invalid count");
    } else if (index < 0 || index + count > model->m_compositor.count(d->group)) {
        qmlInfo(this) << tr("removeGroups: index out of range");
    } else if (count > 0 && groups) {
        model->removeGroups(d->group, index, count, groups);
    }
}

/*!
    \qmlmethod QtQuick2::VisualDataGroup::setGroups(int index, int count, stringlist groups)

    Sets the \a groups \a count items starting at \a index belong to.
*/

void QSGVisualDataGroup::setGroups(QDeclarativeV8Function *args)
{
    Q_D(QSGVisualDataGroup);
    int index = -1;
    int count = 1;
    int groups = 0;

    if (!d->parseGroupArgs(args, &index, &count, &groups))
        return;

    QSGVisualDataModelPrivate *model = QSGVisualDataModelPrivate::get(d->model);
    if (count < 0) {
        qmlInfo(this) << tr("setGroups: invalid count");
    } else if (index < 0 || index + count > model->m_compositor.count(d->group)) {
        qmlInfo(this) << tr("setGroups: index out of range");
    } else if (count > 0) {
        model->setGroups(d->group, index, count, groups);
    }
}

/*!
    \qmlmethod QtQuick2::VisualDataGroup::setGroups(int index, int count, stringlist groups)

    Sets the \a groups \a count items starting at \a index belong to.
*/

/*!
    \qmlmethod QtQuick2::VisualDataGroup::move(int from, int to, int count)

    Moves \a count at \a from in a group \a to a new position.
*/

void QSGVisualDataGroup::move(QDeclarativeV8Function *args)
{
    Q_D(QSGVisualDataGroup);

    if (args->Length() < 2)
        return;

    Compositor::Group fromGroup = d->group;
    Compositor::Group toGroup = d->group;
    int from = -1;
    int to = -1;
    int count = 1;

    int i = 0;
    v8::Local<v8::Value> v = (*args)[i];
    if (QSGVisualDataGroup *group = qobject_cast<QSGVisualDataGroup *>(args->engine()->toQObject(v))) {
        QSGVisualDataGroupPrivate *g_d = QSGVisualDataGroupPrivate::get(group);
        if (g_d->model != d->model)
            return;
        fromGroup = g_d->group;
        v = (*args)[++i];
    }

    if (!v->IsInt32())
        return;
    from = v->Int32Value();

    if (++i == args->Length())
        return;
    v = (*args)[i];

    if (QSGVisualDataGroup *group = qobject_cast<QSGVisualDataGroup *>(args->engine()->toQObject(v))) {
        QSGVisualDataGroupPrivate *g_d = QSGVisualDataGroupPrivate::get(group);
        if (g_d->model != d->model)
            return;
        toGroup = g_d->group;

        if (++i == args->Length())
            return;
        v = (*args)[i];
    }

    if (!v->IsInt32())
        return;
    to = v->Int32Value();

    if (++i < args->Length()) {
        v = (*args)[i];
        if (v->IsInt32())
            count = v->Int32Value();
    }

    QSGVisualDataModelPrivate *model = QSGVisualDataModelPrivate::get(d->model);

    if (count < 0) {
        qmlInfo(this) << tr("move: invalid count");
    } else if (from < 0 || from + count > model->m_compositor.count(fromGroup)) {
        qmlInfo(this) << tr("move: from index out of range");
    } else if (!model->m_compositor.verifyMoveTo(fromGroup, from, toGroup, to, count)) {
        qmlInfo(this) << tr("move: to index out of range");
    } else if (count > 0) {
        QVector<Compositor::Remove> removes;
        QVector<Compositor::Insert> inserts;

        model->m_compositor.move(fromGroup, from, toGroup, to, count, &removes, &inserts);
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

QSGVisualPartsModel::QSGVisualPartsModel(QSGVisualDataModel *model, const QString &part, QObject *parent)
    : QSGVisualModel(*new QObjectPrivate, parent)
    , m_model(model)
    , m_part(part)
    , m_compositorGroup(Compositor::Cache)
    , m_inheritGroup(true)
{
    QSGVisualDataModelPrivate *d = QSGVisualDataModelPrivate::get(m_model);
    if (d->m_cacheMetaType) {
        QSGVisualDataGroupPrivate::get(d->m_groups[1])->emitters.insert(this);
        m_compositorGroup = Compositor::Default;
    } else {
        d->m_pendingParts.insert(this);
    }
}

QSGVisualPartsModel::~QSGVisualPartsModel()
{
}

QString QSGVisualPartsModel::filterGroup() const
{
    if (m_inheritGroup)
        return m_model->filterGroup();
    return m_filterGroup;
}

void QSGVisualPartsModel::setFilterGroup(const QString &group)
{
    if (QSGVisualDataModelPrivate::get(m_model)->m_transaction) {
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

void QSGVisualPartsModel::resetFilterGroup()
{
    if (!m_inheritGroup) {
        m_inheritGroup = true;
        updateFilterGroup();
        emit filterGroupChanged();
    }
}

void QSGVisualPartsModel::updateFilterGroup()
{
    QSGVisualDataModelPrivate *model = QSGVisualDataModelPrivate::get(m_model);
    if (!model->m_cacheMetaType)
        return;

    if (m_inheritGroup)
        return;

    QDeclarativeListCompositor::Group previousGroup = model->m_compositorGroup;
    m_compositorGroup = Compositor::Default;
    QSGVisualDataGroupPrivate::get(model->m_groups[Compositor::Default])->emitters.insert(this);
    for (int i = 1; i < model->m_groupCount; ++i) {
        if (m_filterGroup == model->m_cacheMetaType->groupNames.at(i - 1)) {
            m_compositorGroup = Compositor::Group(i);
            break;
        }
    }

    QSGVisualDataGroupPrivate::get(model->m_groups[m_compositorGroup])->emitters.insert(this);
    if (m_compositorGroup != previousGroup) {
        QVector<QDeclarativeChangeSet::Remove> removes;
        QVector<QDeclarativeChangeSet::Insert> inserts;
        model->m_compositor.transition(previousGroup, m_compositorGroup, &removes, &inserts);

        QDeclarativeChangeSet changeSet;
        changeSet.apply(removes, inserts);
        if (!changeSet.isEmpty())
            emit modelUpdated(changeSet, false);

        if (changeSet.difference() != 0)
            emit countChanged();
    }
}

void QSGVisualPartsModel::updateFilterGroup(
        Compositor::Group group, const QDeclarativeChangeSet &changeSet)
{
    if (!m_inheritGroup)
        return;

    m_compositorGroup = group;
    QSGVisualDataGroupPrivate::get(QSGVisualDataModelPrivate::get(m_model)->m_groups[m_compositorGroup])->emitters.insert(this);

    if (!changeSet.isEmpty())
        emit modelUpdated(changeSet, false);

    if (changeSet.difference() != 0)
        emit countChanged();

    emit filterGroupChanged();
}

int QSGVisualPartsModel::count() const
{
    QSGVisualDataModelPrivate *model = QSGVisualDataModelPrivate::get(m_model);
    return model->m_delegate
            ? model->m_compositor.count(m_compositorGroup)
            : 0;
}

bool QSGVisualPartsModel::isValid() const
{
    return m_model->isValid();
}

QSGItem *QSGVisualPartsModel::item(int index, bool complete)
{
    QSGVisualDataModelPrivate *model = QSGVisualDataModelPrivate::get(m_model);

    if (!model->m_delegate || index < 0 || index >= model->m_compositor.count(m_compositorGroup)) {
        qWarning() << "VisualDataModel::item: index out range" << index << model->m_compositor.count(m_compositorGroup);
        return 0;
    }

    QObject *object = model->object(m_compositorGroup, index, complete, true);

    if (QDeclarativePackage *package = qobject_cast<QDeclarativePackage *>(object)) {
        QObject *part = package->part(m_part);
        if (!part)
            return 0;
        if (QSGItem *item = qobject_cast<QSGItem *>(part)) {
            m_packaged.insertMulti(item, package);
            return item;
        }
    }

    if (m_model->completePending())
        m_model->completeItem();
    model->release(object);
    if (!model->m_delegateValidated) {
        if (object)
            qmlInfo(model->m_delegate) << tr("Delegate component must be Package type.");
        model->m_delegateValidated = true;
    }

    return 0;
}

QSGVisualModel::ReleaseFlags QSGVisualPartsModel::release(QSGItem *item)
{
    QSGVisualModel::ReleaseFlags flags = 0;

    QHash<QObject *, QDeclarativePackage *>::iterator it = m_packaged.find(item);
    if (it != m_packaged.end()) {
        QDeclarativePackage *package = *it;
        QSGVisualDataModelPrivate *model = QSGVisualDataModelPrivate::get(m_model);
        flags = model->release(package);
        m_packaged.erase(it);
        if (!m_packaged.contains(item))
            flags &= ~Referenced;
        if (flags & Destroyed)
            QSGVisualDataModelPrivate::get(m_model)->emitDestroyingPackage(package);
    }
    return flags;
}

bool QSGVisualPartsModel::completePending() const
{
    return m_model->completePending();
}

void QSGVisualPartsModel::completeItem()
{
    m_model->completeItem();
}

QString QSGVisualPartsModel::stringValue(int index, const QString &role)
{
    return QSGVisualDataModelPrivate::get(m_model)->stringValue(m_compositorGroup, index, role);
}

void QSGVisualPartsModel::setWatchedRoles(QList<QByteArray> roles)
{
    QSGVisualDataModelPrivate *model = QSGVisualDataModelPrivate::get(m_model);
    model->m_adaptorModel->replaceWatchedRoles(m_watchedRoles, roles);
    m_watchedRoles = roles;
}

int QSGVisualPartsModel::indexOf(QSGItem *item, QObject *) const
{
    QHash<QObject *, QDeclarativePackage *>::const_iterator it = m_packaged.find(item);
    if (it != m_packaged.end()) {
        const QSGVisualDataModelPrivate *model = QSGVisualDataModelPrivate::get(m_model);
        const int cacheIndex = model->cacheIndexOf(*it);
        return cacheIndex != -1
                ? model->m_cache.at(cacheIndex)->index[m_compositorGroup]
                : -1;
    }
    return -1;
}

void QSGVisualPartsModel::createdPackage(int index, QDeclarativePackage *package)
{
    if (QSGItem *item = qobject_cast<QSGItem *>(package->part(m_part)))
        emit createdItem(index, item);
}

void QSGVisualPartsModel::destroyingPackage(QDeclarativePackage *package)
{
    if (QSGItem *item = qobject_cast<QSGItem *>(package->part(m_part))) {
        Q_ASSERT(!m_packaged.contains(item));
        emit destroyingItem(item);
    }
}

void QSGVisualPartsModel::emitModelUpdated(const QDeclarativeChangeSet &changeSet, bool reset)
{
    emit modelUpdated(changeSet, reset);
    if (changeSet.difference() != 0)
        emit countChanged();
}


QT_END_NAMESPACE

#include <qsgvisualdatamodel.moc>
