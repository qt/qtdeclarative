// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QQMLDATAMODEL_P_P_H
#define QQMLDATAMODEL_P_P_H

#include "qqmldelegatemodel_p.h"
#include <private/qv4qobjectwrapper_p.h>

#include <QtQml/qqmlcontext.h>
#include <QtQml/qqmlincubator.h>

#include <private/qqmladaptormodel_p.h>
#include <private/qqmlopenmetaobject_p.h>

#include <QtCore/qloggingcategory.h>
#include <QtCore/qpointer.h>

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

QT_REQUIRE_CONFIG(qml_delegate_model);

QT_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(lcItemViewDelegateRecycling)

typedef QQmlListCompositor Compositor;

class QQmlDelegateModelAttachedMetaObject;
class QQmlAbstractDelegateComponent;
class QQmlTableInstanceModel;

class Q_QMLMODELS_EXPORT QQmlDelegateModelItemMetaType final
    : public QQmlRefCounted<QQmlDelegateModelItemMetaType>
{
public:
    enum class ModelKind : quint8 {
        InstanceModel,
        DelegateModel,
        TableInstanceModel,
    };

    QQmlDelegateModelItemMetaType(
            QV4::ExecutionEngine *engine, QQmlDelegateModel *model, const QStringList &groupNames);
    QQmlDelegateModelItemMetaType(
            QV4::ExecutionEngine *engine, QQmlTableInstanceModel *model);
    ~QQmlDelegateModelItemMetaType();

    void initializeAttachedMetaObject();
    void initializePrototype();

    int parseGroups(const QStringList &groupNames) const;
    int parseGroups(const QV4::Value &groupNames) const;

    QQmlDelegateModel *delegateModel() const
    {
        return modelKind == ModelKind::DelegateModel
                ? static_cast<QQmlDelegateModel *>(model.get())
                : nullptr;
    }

    qsizetype groupCount() const { return groupNames.size(); }

    void emitModelChanged() const;

    QPointer<QQmlInstanceModel> model;
    QV4::ExecutionEngine * const v4Engine;
    QQmlRefPointer<QQmlDelegateModelAttachedMetaObject> attachedMetaObject;
    const QStringList groupNames;
    QV4::PersistentValue modelItemProto;
    ModelKind modelKind = ModelKind::InstanceModel;
};

class QQmlAdaptorModel;
class QQDMIncubationTask;

class QQmlDelegateModelItem : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int index READ modelIndex NOTIFY modelIndexChanged)
    Q_PROPERTY(int row READ modelRow NOTIFY rowChanged REVISION(2, 12))
    Q_PROPERTY(int column READ modelColumn NOTIFY columnChanged REVISION(2, 12))
    Q_PROPERTY(QObject *model READ modelObject CONSTANT)
public:
    struct ObjectReference
    {
        Q_DISABLE_COPY_MOVE(ObjectReference)

        ObjectReference(QQmlDelegateModelItem *item) : item(item)
        {
            ++item->m_objectStrongRef;
        }

        ~ObjectReference()
        {
            Q_ASSERT(item->m_objectStrongRef > 0);
            --item->m_objectStrongRef;
        }
    private:
        QQmlDelegateModelItem *item = nullptr;
    };

    struct ObjectSpanReference
    {
        Q_DISABLE_COPY_MOVE(ObjectSpanReference)

        ObjectSpanReference(QSpan<QQmlDelegateModelItem *const> span) : items(std::move(span))
        {
            for (QQmlDelegateModelItem *item : items)
                ++item->m_objectStrongRef;
        }

        ~ObjectSpanReference()
        {
            for (QQmlDelegateModelItem *item : items) {
                Q_ASSERT(item->m_objectStrongRef > 0);
                --item->m_objectStrongRef;
            }
        }

    private:
        const QSpan<QQmlDelegateModelItem *const> items;
    };

    struct ScriptReference
    {
        Q_DISABLE_COPY_MOVE(ScriptReference)

        ScriptReference(QQmlDelegateModelItem *item) : item(item) { item->referenceSript(); }
        ~ScriptReference() { item->releaseScript(); }

        QQmlDelegateModelItem *data() const { return item; }
    private:
        QQmlDelegateModelItem *item = nullptr;
    };

    QQmlDelegateModelItem(const QQmlRefPointer<QQmlDelegateModelItemMetaType> &metaType,
                          QQmlAdaptorModel::Accessors *accessor, int modelIndex,
                          int row, int column);
    ~QQmlDelegateModelItem();

    [[nodiscard]] QObject *referenceObjectWeak()
    {
        Q_ASSERT(m_object);
        ++m_objectWeakRef;
        return m_object;
    }

    bool releaseObjectWeak()
    {
        if (m_objectWeakRef > 0)
            --m_objectWeakRef;
        return m_objectWeakRef == 0 && !(m_groups & Compositor::PersistedFlag);
    }
    void clearObjectWeakReferences()
    {
        m_objectWeakRef = 0;
    }

    bool isObjectReferenced() const
    {
        return m_objectWeakRef != 0
                || m_objectStrongRef != 0
                || (m_groups & Compositor::PersistedFlag);
    }
    void childContextObjectDestroyed(QObject *childContextObject);

    bool isScriptReferenced() const {
        return m_scriptRef
                || m_incubationTask
                || ((m_groups & Compositor::UnresolvedFlag) && (m_groups & Compositor::GroupMask));
    }

    void dispose();

    QObject *modelObject() { return this; }

    void destroyObject();
    void destroyObjectLater();

    static QQmlDelegateModelItem *dataForObject(QObject *object);

    int groupIndex(Compositor::Group group);

    int modelRow() const { return m_row; }
    int modelColumn() const { return m_column; }
    int modelIndex() const { return m_index; }
    bool hasValidModelIndex() const { return m_index >= 0; }
    virtual void setModelIndex(int idx, int newRow, int newColumn, bool alwaysEmit = false);

    bool usesStructuredModelData() const { return m_useStructuredModelData; }

    virtual QV4::ReturnedValue get() { return QV4::QObjectWrapper::wrap(m_metaType->v4Engine, this); }

    virtual void setValue(const QString &role, const QVariant &value) { Q_UNUSED(role); Q_UNUSED(value); }
    virtual bool resolveIndex(const QQmlAdaptorModel &, int) { return false; }
    virtual QQmlRefPointer<QQmlContextData> initProxy() { return m_contextData; }

    static QV4::ReturnedValue get_model(const QV4::FunctionObject *, const QV4::Value *thisObject, const QV4::Value *argv, int argc);
    static QV4::ReturnedValue get_groups(const QV4::FunctionObject *, const QV4::Value *thisObject, const QV4::Value *argv, int argc);
    static QV4::ReturnedValue set_groups(const QV4::FunctionObject *, const QV4::Value *thisObject, const QV4::Value *argv, int argc);
    static QV4::ReturnedValue get_member(QQmlDelegateModelItem *thisItem, uint flag, const QV4::Value &);
    static QV4::ReturnedValue set_member(QQmlDelegateModelItem *thisItem, uint flag, const QV4::Value &arg);
    static QV4::ReturnedValue get_index(QQmlDelegateModelItem *thisItem, uint flag, const QV4::Value &arg);


    QQmlDelegateModelAttached *attached() const
    {
        if (!m_object)
            return nullptr;

        QQmlData *ddata = QQmlData::get(m_object);
        if (!ddata || !ddata->hasExtendedData())
            return nullptr;

        return static_cast<QQmlDelegateModelAttached *>(
                ddata->attachedProperties()->value(
                        QQmlPrivate::attachedPropertiesFunc<QQmlDelegateModel>()));
    }

    void disableStructuredModelData() { m_useStructuredModelData = false; }

    QDynamicMetaObjectData *exchangeMetaObject(QDynamicMetaObjectData *metaObject)
    {
        return std::exchange(d_ptr->metaObject, metaObject);
    }

    QQDMIncubationTask *incubationTask() const { return m_incubationTask; }
    void clearIncubationTask() { m_incubationTask = nullptr; }
    void setIncubationTask(QQDMIncubationTask *incubationTask)
    {
        Q_ASSERT(!m_incubationTask);
        Q_ASSERT(incubationTask);
        m_incubationTask = incubationTask;
    }

    quint16 objectStrongRef() const { return m_objectStrongRef; }
    quint16 objectWeakRef() const { return m_objectWeakRef; }
    quint16 scriptRef() const { return m_scriptRef; }

    QObject *object() const { return m_object; }
    void setObject(QObject *object) {
        Q_ASSERT(!m_object);
        Q_ASSERT(object);
        m_object = object;
    }

    QQmlComponent *delegate() const { return m_delegate; }
    void setDelegate(QQmlComponent *delegate) { m_delegate = delegate; }

    const QQmlRefPointer<QQmlContextData> &contextData() const { return m_contextData; }
    void setContextData(const QQmlRefPointer<QQmlContextData> &contextData)
    {
        m_contextData = contextData;
    }

    int groups() const { return m_groups; }
    void setGroups(int groups) { m_groups = groups; }
    void addGroups(int groups) { m_groups |= groups; }
    void removeGroups(int groups) { m_groups &= ~groups; }
    void clearGroups() { m_groups = 0; }

    const QQmlRefPointer<QQmlDelegateModelItemMetaType> &metaType() const { return m_metaType; }

Q_SIGNALS:
    void modelIndexChanged();
    Q_REVISION(2, 12) void rowChanged();
    Q_REVISION(2, 12) void columnChanged();

private:
    void objectDestroyed(QObject *);

    void referenceSript() { ++m_scriptRef; }
    bool releaseScript()
    {
        Q_ASSERT(m_scriptRef > 0);
        return --m_scriptRef == 0;
    }

    QQmlRefPointer<QQmlDelegateModelItemMetaType> m_metaType;
    QQmlRefPointer<QQmlContextData> m_contextData;
    QPointer<QObject> m_object;
    QQDMIncubationTask *m_incubationTask = nullptr;
    QQmlComponent *m_delegate = nullptr;
    int m_groups = 0;
    int m_index = -1;
    int m_row = -1;
    int m_column = -1;

    // A strong reference to the object prevents the deletion of the object. We use them as
    // temporary guards with ObjectReference or ObjectSpanReference to make sure the objects we're
    // currently manipulating don't disappear.
    quint16 m_objectStrongRef = 0;

    // A weak reference to the object is added when handing the object out to the view via object().
    // It is dropped when the view calls release(). Weak references are advisory. We try not to
    // delete an object while weak references are still in place. However, during destruction, the
    // weak refernces may be ignored. Views are expected to use QPointer or similar guards in
    // addition.
    quint16 m_objectWeakRef = 0;

    // A script reference is a strong reference to the QQmlDelegateModelItem itself. We don't delete
    // the QQmlDelegateModelItem while script references are still in place. We can use them as
    // temporary guards to hold on to an item while working on it, or we can attach them to
    // Heap::QQmlDelegateModelItemObject in order to extend the life time of a QQmlDelegateModelItem
    // to the life time of its heap object.
    // The latter case is particular in that it contradicts our usual mechanism of only holding weak
    // references to QObjects in e.g. QV4::QObjectWrapper. The trick here is that the actual
    // QQmlDelegateModelItem is never exposed to the user, so that the user has no chance of
    // manually deleting it.
    // TODO: This is brittle.
    quint16 m_scriptRef = 0;

    bool m_useStructuredModelData = true;
};

namespace QV4 {
namespace Heap {
struct QQmlDelegateModelItemObject : Object {
    inline void init(QQmlDelegateModelItem *item);
    void destroy();

    alignas(alignof(QQmlDelegateModelItem::ScriptReference))
    std::byte ref[sizeof(QQmlDelegateModelItem::ScriptReference)];

    QQmlDelegateModelItem *item() const {
        return reinterpret_cast<const QQmlDelegateModelItem::ScriptReference *>(&ref)->data();
    }
};

}
}

struct QQmlDelegateModelItemObject : QV4::Object
{
    V4_OBJECT2(QQmlDelegateModelItemObject, QV4::Object)
    V4_NEEDS_DESTROY
};

void QV4::Heap::QQmlDelegateModelItemObject::init(QQmlDelegateModelItem *modelItem)
{
    Object::init();
    new (ref) QQmlDelegateModelItem::ScriptReference(modelItem);
}

class QQmlReusableDelegateModelItemsPool
{
public:
    bool insertItem(QQmlDelegateModelItem *modelItem);
    QQmlDelegateModelItem *takeItem(const QQmlComponent *delegate, int newIndexHint);
    void reuseItem(QQmlDelegateModelItem *item, int newModelIndex);
    void drain(int maxPoolTime, std::function<void(QQmlDelegateModelItem *cacheItem)> releaseItem);
    int size()
    {
        Q_ASSERT(m_reusableItemsPool.size() <= MaxSize);
        return int(m_reusableItemsPool.size());
    }

private:
    struct PoolItem
    {
        QQmlDelegateModelItem *item = nullptr;
        int poolTime = 0;
    };

    static constexpr size_t MaxSize = size_t(std::numeric_limits<int>::max());

    std::vector<PoolItem> m_reusableItemsPool;
};

template<typename Target>
class QQmlDelegateModelReadOnlyMetaObject : QDynamicMetaObjectData
{
    Q_DISABLE_COPY_MOVE(QQmlDelegateModelReadOnlyMetaObject)

public:
    QQmlDelegateModelReadOnlyMetaObject(const Target &target, int readOnlyProperty)
        : target(target)
        , readOnlyProperty(readOnlyProperty)
    {
        if (QQmlDelegateModelItem *object = target)
            original = object->exchangeMetaObject(this);
    }

    ~QQmlDelegateModelReadOnlyMetaObject()
    {
        if (QQmlDelegateModelItem *object = target)
            object->exchangeMetaObject(original);
    }

    void objectDestroyed(QObject *o) final
    {
        if (target)
            original->objectDestroyed(o);
    }

    QMetaObject *toDynamicMetaObject(QObject *o) final
    {
        return target ? original->toDynamicMetaObject(o) : nullptr;
    }

    int metaCall(QObject *o, QMetaObject::Call call, int id, void **argv) final
    {
        if (!target)
            return 0;

        if (id == readOnlyProperty && call == QMetaObject::WriteProperty)
            return 0;

        return original->metaCall(o, call, id, argv);
    }

private:
    Target target = {};
    QDynamicMetaObjectData *original = nullptr;
    int readOnlyProperty = -1;
};

class QQmlDelegateModelPrivate;
class QQDMIncubationTask : public QQmlIncubator
{
public:
    QQDMIncubationTask(QQmlDelegateModelPrivate *l, IncubationMode mode)
        : QQmlIncubator(mode)
        , incubating(nullptr)
        , vdm(l) {}

    void initializeRequiredProperties(
            QQmlDelegateModelItem *modelItemToIncubate, QObject* object,
            QQmlDelegateModel::DelegateModelAccess access);
    void statusChanged(Status) override;
    void setInitialState(QObject *) override;

    QQmlDelegateModelItem *incubating = nullptr;
    QQmlDelegateModelPrivate *vdm = nullptr;
    QQmlRefPointer<QQmlContextData> proxyContext;
    QPointer<QObject> proxiedObject  = nullptr; // the proxied object might disapear, so we use a QPointer instead of a raw one
    int index[QQmlListCompositor::MaximumGroupCount];
};


class QQmlDelegateModelGroupEmitter
{
public:
    virtual ~QQmlDelegateModelGroupEmitter();
    virtual void emitModelUpdated(const QQmlChangeSet &changeSet, bool reset) = 0;
    virtual void createdPackage(int, QQuickPackage *);
    virtual void initPackage(int, QQuickPackage *);
    virtual void destroyingPackage(QQuickPackage *);

    QIntrusiveListNode emitterNode;
};

typedef QIntrusiveList<QQmlDelegateModelGroupEmitter, &QQmlDelegateModelGroupEmitter::emitterNode> QQmlDelegateModelGroupEmitterList;

class QQmlDelegateModelGroupPrivate : public QObjectPrivate
{
public:
    Q_DECLARE_PUBLIC(QQmlDelegateModelGroup)

    QQmlDelegateModelGroupPrivate() : group(Compositor::Cache), defaultInclude(false) {}

    static QQmlDelegateModelGroupPrivate *get(QQmlDelegateModelGroup *group) {
        return static_cast<QQmlDelegateModelGroupPrivate *>(QObjectPrivate::get(group)); }

    void setModel(QQmlDelegateModel *model, Compositor::Group group);
    bool isChangedConnected();
    void emitChanges(QV4::ExecutionEngine *engine);
    void emitModelUpdated(bool reset);

    void createdPackage(int index, QQuickPackage *package);
    void initPackage(int index, QQuickPackage *package);
    void destroyingPackage(QQuickPackage *package);

    bool parseIndex(const QV4::Value &value, int *index, Compositor::Group *group) const;
    bool parseGroupArgs(
            QQmlV4FunctionPtr args, Compositor::Group *group, int *index, int *count, int *groups) const;

    Compositor::Group group;
    QPointer<QQmlDelegateModel> model;
    QQmlDelegateModelGroupEmitterList emitters;
    QQmlChangeSet changeSet;
    QString name;
    bool defaultInclude;
};

class QQmlDelegateModelParts;

class QQmlDelegateModelPrivate : public QObjectPrivate, public QQmlDelegateModelGroupEmitter
{
    Q_DECLARE_PUBLIC(QQmlDelegateModel)
public:
    QQmlDelegateModelPrivate(QQmlContext *);
    ~QQmlDelegateModelPrivate();

    static QQmlDelegateModelPrivate *get(QQmlDelegateModel *m) {
        return static_cast<QQmlDelegateModelPrivate *>(QObjectPrivate::get(m));
    }

    void init();
    void connectModel(QQmlAdaptorModel *model);
    void connectToAbstractItemModel();
    void disconnectFromAbstractItemModel();

    void requestMoreIfNecessary();
    QObject *object(Compositor::Group group, int index, QQmlIncubator::IncubationMode incubationMode);
    QQmlDelegateModel::ReleaseFlags release(QObject *object, QQmlInstanceModel::ReusableFlag reusable = QQmlInstanceModel::NotReusable);
    QVariant variantValue(Compositor::Group group, int index, const QString &name);
    void emitCreatedPackage(QQDMIncubationTask *incubationTask, QQuickPackage *package);
    void emitInitPackage(QQDMIncubationTask *incubationTask, QQuickPackage *package);
    void emitCreatedItem(QQDMIncubationTask *incubationTask, QObject *item) {
        Q_EMIT q_func()->createdItem(incubationTask->index[m_compositorGroup], item); }
    void emitInitItem(QQDMIncubationTask *incubationTask, QObject *item) {
        Q_EMIT q_func()->initItem(incubationTask->index[m_compositorGroup], item); }
    void emitDestroyingPackage(QQuickPackage *package);
    void emitDestroyingItem(QObject *item) { Q_EMIT q_func()->destroyingItem(item); }
    void addCacheItem(QQmlDelegateModelItem *item, Compositor::iterator it);
    void removeCacheItem(QQmlDelegateModelItem *cacheItem);
    void destroyCacheItem(QQmlDelegateModelItem *cacheItem);
    void updateFilterGroup();

    void reuseItem(QQmlDelegateModelItem *item, int newModelIndex, int newGroups);
    void drainReusableItemsPool(int maxPoolTime);
    QQmlComponent *resolveDelegate(int index);

    void addGroups(Compositor::iterator from, int count, Compositor::Group group, int groupFlags);
    void removeGroups(Compositor::iterator from, int count, Compositor::Group group, int groupFlags);
    void setGroups(Compositor::iterator from, int count, Compositor::Group group, int groupFlags);

    void itemsInserted(
            const QList<Compositor::Insert> &inserts,
            QVarLengthArray<QList<QQmlChangeSet::Change>, Compositor::MaximumGroupCount> *translatedInserts,
            QHash<int, QList<QQmlDelegateModelItem *> > *movedItems = nullptr);
    void itemsInserted(const QList<Compositor::Insert> &inserts);
    void itemsRemoved(
            const QList<Compositor::Remove> &removes,
            QVarLengthArray<QList<QQmlChangeSet::Change>, Compositor::MaximumGroupCount> *translatedRemoves,
            QHash<int, QList<QQmlDelegateModelItem *> > *movedItems = nullptr);
    void itemsRemoved(const QList<Compositor::Remove> &removes);
    void itemsMoved(
            const QList<Compositor::Remove> &removes, const QList<Compositor::Insert> &inserts);
    void itemsChanged(const QList<Compositor::Change> &changes);
    void emitChanges();
    void emitModelUpdated(const QQmlChangeSet &changeSet, bool reset) override;
    void delegateChanged(bool add = true, bool remove = true);

    enum class InsertionResult {
        Success,
        Error,
        Retry
    };
    InsertionResult insert(Compositor::insert_iterator &before, const QV4::Value &object, int groups);

    int adaptorModelCount() const;

    static void group_append(QQmlListProperty<QQmlDelegateModelGroup> *property, QQmlDelegateModelGroup *group);
    static qsizetype group_count(QQmlListProperty<QQmlDelegateModelGroup> *property);
    static QQmlDelegateModelGroup *group_at(QQmlListProperty<QQmlDelegateModelGroup> *property, qsizetype index);

    void releaseIncubator(QQDMIncubationTask *incubationTask);
    void incubatorStatusChanged(QQDMIncubationTask *incubationTask, QQmlIncubator::Status status);
    void setInitialState(QQDMIncubationTask *incubationTask, QObject *o);

    QQmlAdaptorModel m_adaptorModel;
    QQmlListCompositor m_compositor;
    QQmlStrongJSQObjectReference<QQmlComponent> m_delegate;
    QQmlAbstractDelegateComponent *m_delegateChooser;
    QMetaObject::Connection m_delegateChooserChanged;
    QQmlRefPointer<QQmlDelegateModelItemMetaType> m_cacheMetaType;
    QPointer<QQmlContext> m_context;
    QQmlDelegateModelParts *m_parts;
    QQmlDelegateModelGroupEmitterList m_pendingParts;

    QList<QQmlDelegateModelItem *> m_cache;
    QQmlReusableDelegateModelItemsPool m_reusableItemsPool;
    QList<QQDMIncubationTask *> m_finishedIncubating;
    QList<QByteArray> m_watchedRoles;

    QString m_filterGroup;

    int m_count;
    int m_groupCount;

    QQmlListCompositor::Group m_compositorGroup;
    bool m_complete : 1;
    bool m_delegateValidated : 1;
    bool m_reset : 1;
    bool m_transaction : 1;
    bool m_incubatorCleanupScheduled : 1;
    bool m_waitingToFetchMore : 1;

    union {
        struct {
            QQmlDelegateModelGroup *m_cacheItems;
            QQmlDelegateModelGroup *m_items;
            QQmlDelegateModelGroup *m_persistedItems;
        };
        QQmlDelegateModelGroup *m_groups[Compositor::MaximumGroupCount];
    };
};

class QQmlPartsModel : public QQmlInstanceModel, public QQmlDelegateModelGroupEmitter
{
    Q_OBJECT
    Q_PROPERTY(QString filterOnGroup READ filterGroup WRITE setFilterGroup NOTIFY filterGroupChanged RESET resetFilterGroup FINAL)
public:
    QQmlPartsModel(QQmlDelegateModel *model, const QString &part, QObject *parent = nullptr);
    ~QQmlPartsModel();

    QString filterGroup() const;
    void setFilterGroup(const QString &group);
    void resetFilterGroup();
    void updateFilterGroup();
    void updateFilterGroup(Compositor::Group group, const QQmlChangeSet &changeSet);

    int count() const override;
    bool isValid() const override;
    QObject *object(int index, QQmlIncubator::IncubationMode incubationMode = QQmlIncubator::AsynchronousIfNested) override;
    ReleaseFlags release(QObject *item, ReusableFlag reusable = NotReusable) override;
    QVariant variantValue(int index, const QString &role) override;
    QList<QByteArray> watchedRoles() const { return m_watchedRoles; }
    void setWatchedRoles(const QList<QByteArray> &roles) override;
    QQmlIncubator::Status incubationStatus(int index) override;

    int indexOf(QObject *item, QObject *objectContext) const override;

    void emitModelUpdated(const QQmlChangeSet &changeSet, bool reset) override;

    void createdPackage(int index, QQuickPackage *package) override;
    void initPackage(int index, QQuickPackage *package) override;
    void destroyingPackage(QQuickPackage *package) override;

Q_SIGNALS:
    void filterGroupChanged();

private:
    QQmlDelegateModel *m_model;
    QMultiHash<QObject *, QQuickPackage *> m_packaged;
    QString m_part;
    QString m_filterGroup;
    QList<QByteArray> m_watchedRoles;
    QList<int> m_pendingPackageInitializations; // vector holds model indices
    Compositor::Group m_compositorGroup;
    bool m_inheritGroup;
    bool m_modelUpdatePending = true;
};

class QMetaPropertyBuilder;

class QQmlDelegateModelPartsMetaObject : public QQmlOpenMetaObject
{
public:
    QQmlDelegateModelPartsMetaObject(QObject *parent)
    : QQmlOpenMetaObject(parent) {}

    void propertyCreated(int, QMetaPropertyBuilder &) override;
    QVariant initialValue(int) override;
};

class QQmlDelegateModelParts : public QObject
{
Q_OBJECT
public:
    QQmlDelegateModelParts(QQmlDelegateModel *parent);

    QQmlDelegateModel *model;
    QList<QQmlPartsModel *> models;
};

class QQmlDelegateModelAttachedMetaObject final
    : public QAbstractDynamicMetaObject,
      public QQmlRefCounted<QQmlDelegateModelAttachedMetaObject>
{
public:
    QQmlDelegateModelAttachedMetaObject(
            QQmlDelegateModelItemMetaType *metaType, QMetaObject *metaObject);
    ~QQmlDelegateModelAttachedMetaObject();

    void objectDestroyed(QObject *) override;
    int metaCall(QObject *, QMetaObject::Call, int _id, void **) override;

private:
    QQmlDelegateModelItemMetaType * const metaType;
    QMetaObject * const metaObject;
    const int memberPropertyOffset;
    const int indexPropertyOffset;
};

QT_END_NAMESPACE

#endif
