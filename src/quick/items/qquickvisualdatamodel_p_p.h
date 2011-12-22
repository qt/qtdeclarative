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

#ifndef QQUICKVISUALDATAMODEL_P_P_H
#define QQUICKVISUALDATAMODEL_P_P_H

#include "qquickvisualdatamodel_p.h"

#include <QtDeclarative/qdeclarativecontext.h>
#include <QtDeclarative/qdeclarativeincubator.h>

#include <private/qdeclarativeopenmetaobject_p.h>

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

QT_BEGIN_NAMESPACE

typedef QDeclarativeListCompositor Compositor;

class QQuickVisualDataModelItemMetaType : public QDeclarativeRefCount
{
public:
    QQuickVisualDataModelItemMetaType(QV8Engine *engine, QQuickVisualDataModel *model, const QStringList &groupNames);
    ~QQuickVisualDataModelItemMetaType();

    int parseGroups(const QStringList &groupNames) const;
    int parseGroups(const v8::Local<v8::Value> &groupNames) const;

    static void release_index(v8::Persistent<v8::Value> object, void *parameter);
    static void release_model(v8::Persistent<v8::Value> object, void *parameter);

    static v8::Handle<v8::Value> get_model(v8::Local<v8::String>, const v8::AccessorInfo &info);
    static v8::Handle<v8::Value> get_groups(v8::Local<v8::String>, const v8::AccessorInfo &info);
    static void set_groups(
            v8::Local<v8::String>, v8::Local<v8::Value> value, const v8::AccessorInfo &info);
    static v8::Handle<v8::Value> get_member(v8::Local<v8::String>, const v8::AccessorInfo &info);
    static void set_member(
            v8::Local<v8::String>, v8::Local<v8::Value> value, const v8::AccessorInfo &info);
    static v8::Handle<v8::Value> get_index(v8::Local<v8::String>, const v8::AccessorInfo &info);

    QDeclarativeGuard<QQuickVisualDataModel> model;
    const int groupCount;
    const int memberPropertyOffset;
    const int indexPropertyOffset;
    QV8Engine * const v8Engine;
    QMetaObject *metaObject;
    const QStringList groupNames;
    v8::Persistent<v8::Function> constructor;
};

class QQuickVisualAdaptorModel;
class QVDMIncubationTask;

class QQuickVisualDataModelItem : public QObject, public QV8ObjectResource
{
    Q_OBJECT
    Q_PROPERTY(int index READ modelIndex NOTIFY modelIndexChanged)
    V8_RESOURCE_TYPE(VisualDataItemType)
public:
    QQuickVisualDataModelItem(
            QQuickVisualDataModelItemMetaType *metaType, QQuickVisualAdaptorModel *model, int modelIndex);
    ~QQuickVisualDataModelItem();

    void referenceObject() { ++objectRef; }
    bool releaseObject() { return --objectRef == 0 && !(groups & Compositor::PersistedFlag); }
    bool isObjectReferenced() const { return objectRef != 0 || (groups & Compositor::PersistedFlag); }

    bool isReferenced() const {
        return scriptRef
                || incubationTask
                || ((groups & Compositor::UnresolvedFlag) && (groups & Compositor::GroupMask));
    }

    void Dispose();

    int modelIndex() const { return index[0]; }
    void setModelIndex(int idx) { index[0] = idx; emit modelIndexChanged(); }

    virtual v8::Handle<v8::Value> get() { return engine->newQObject(this); }

    virtual void setValue(const QString &role, const QVariant &value) { Q_UNUSED(role); Q_UNUSED(value); }
    virtual bool resolveIndex(int) { return false; }

Q_SIGNALS:
    void modelIndexChanged();

public:
    QQuickVisualDataModelItemMetaType * const metaType;
    QDeclarativeGuard<QQuickVisualAdaptorModel> model;
    QDeclarativeGuard<QObject> object;
    QQuickVisualDataModelAttached *attached;
    v8::Persistent<v8::Object> indexHandle;
    v8::Persistent<v8::Value> modelHandle;
    QIntrusiveListNode cacheNode;
    int objectRef;
    int scriptRef;
    int groups;
    int index[QDeclarativeListCompositor::MaximumGroupCount];
    QVDMIncubationTask *incubationTask;
};


class QQuickVisualDataModelPrivate;
class QVDMIncubationTask : public QDeclarativeIncubator
{
public:
    QVDMIncubationTask(QQuickVisualDataModelPrivate *l, IncubationMode mode)
        : QDeclarativeIncubator(mode)
        , incubating(0)
        , incubatingContext(0)
        , vdm(l) {}

    virtual void statusChanged(Status);
    virtual void setInitialState(QObject *);

    QQuickVisualDataModelItem *incubating;
    QDeclarativeContext *incubatingContext;

private:
    QQuickVisualDataModelPrivate *vdm;
};


class QQuickVisualDataGroupEmitter
{
public:
    virtual void emitModelUpdated(const QDeclarativeChangeSet &changeSet, bool reset) = 0;
    virtual void createdPackage(int, QDeclarativePackage *) {}
    virtual void initPackage(int, QDeclarativePackage *) {}
    virtual void destroyingPackage(QDeclarativePackage *) {}

    QIntrusiveListNode emitterNode;
};

typedef QIntrusiveList<QQuickVisualDataGroupEmitter, &QQuickVisualDataGroupEmitter::emitterNode> QQuickVisualDataGroupEmitterList;

class QQuickVisualDataGroupPrivate : public QObjectPrivate
{
public:
    Q_DECLARE_PUBLIC(QQuickVisualDataGroup)

    QQuickVisualDataGroupPrivate() : group(Compositor::Cache), defaultInclude(false) {}

    static QQuickVisualDataGroupPrivate *get(QQuickVisualDataGroup *group) {
        return static_cast<QQuickVisualDataGroupPrivate *>(QObjectPrivate::get(group)); }

    void setModel(QQuickVisualDataModel *model, Compositor::Group group);
    void emitChanges(QV8Engine *engine);
    void emitModelUpdated(bool reset);

    void createdPackage(int index, QDeclarativePackage *package);
    void initPackage(int index, QDeclarativePackage *package);
    void destroyingPackage(QDeclarativePackage *package);

    bool parseIndex(const v8::Local<v8::Value> &value, int *index, Compositor::Group *group) const;
    bool parseGroupArgs(
            QDeclarativeV8Function *args, Compositor::Group *group, int *index, int *count, int *groups) const;

    Compositor::Group group;
    QDeclarativeGuard<QQuickVisualDataModel> model;
    QQuickVisualDataGroupEmitterList emitters;
    QDeclarativeChangeSet changeSet;
    QString name;
    bool defaultInclude;
};

class QQuickVisualDataModelParts;

class QQuickVisualDataModelPrivate : public QObjectPrivate, public QQuickVisualDataGroupEmitter
{
    Q_DECLARE_PUBLIC(QQuickVisualDataModel)
public:
    QQuickVisualDataModelPrivate(QDeclarativeContext *);
    ~QQuickVisualDataModelPrivate();

    static QQuickVisualDataModelPrivate *get(QQuickVisualDataModel *m) {
        return static_cast<QQuickVisualDataModelPrivate *>(QObjectPrivate::get(m));
    }

    void init();
    void connectModel(QQuickVisualAdaptorModel *model);

    QObject *object(Compositor::Group group, int index, bool asynchronous, bool reference);
    void destroy(QObject *object);
    QQuickVisualDataModel::ReleaseFlags release(QObject *object);
    QString stringValue(Compositor::Group group, int index, const QString &name);
    int cacheIndexOf(QObject *object) const;
    void emitCreatedPackage(QQuickVisualDataModelItem *cacheItem, QDeclarativePackage *package);
    void emitInitPackage(QQuickVisualDataModelItem *cacheItem, QDeclarativePackage *package);
    void emitCreatedItem(QQuickVisualDataModelItem *cacheItem, QQuickItem *item) {
        emit q_func()->createdItem(cacheItem->index[m_compositorGroup], item); }
    void emitInitItem(QQuickVisualDataModelItem *cacheItem, QQuickItem *item) {
        emit q_func()->initItem(cacheItem->index[m_compositorGroup], item); }
    void emitDestroyingPackage(QDeclarativePackage *package);
    void emitDestroyingItem(QQuickItem *item) { emit q_func()->destroyingItem(item); }

    void updateFilterGroup();

    void addGroups(Compositor::iterator from, int count, Compositor::Group group, int groupFlags);
    void removeGroups(Compositor::iterator from, int count, Compositor::Group group, int groupFlags);
    void setGroups(Compositor::iterator from, int count, Compositor::Group group, int groupFlags);

    void itemsInserted(
            const QVector<Compositor::Insert> &inserts,
            QVarLengthArray<QVector<QDeclarativeChangeSet::Insert>, Compositor::MaximumGroupCount> *translatedInserts,
            QHash<int, QList<QQuickVisualDataModelItem *> > *movedItems = 0);
    void itemsInserted(const QVector<Compositor::Insert> &inserts);
    void itemsRemoved(
            const QVector<Compositor::Remove> &removes,
            QVarLengthArray<QVector<QDeclarativeChangeSet::Remove>, Compositor::MaximumGroupCount> *translatedRemoves,
            QHash<int, QList<QQuickVisualDataModelItem *> > *movedItems = 0);
    void itemsRemoved(const QVector<Compositor::Remove> &removes);
    void itemsMoved(
            const QVector<Compositor::Remove> &removes, const QVector<Compositor::Insert> &inserts);
    void itemsChanged(const QVector<Compositor::Change> &changes);
    template <typename T> static v8::Local<v8::Array> buildChangeList(const QVector<T> &changes);
    void emitChanges();
    void emitModelUpdated(const QDeclarativeChangeSet &changeSet, bool reset);

    bool insert(Compositor::insert_iterator &before, const v8::Local<v8::Object> &object, int groups);

    static void group_append(QDeclarativeListProperty<QQuickVisualDataGroup> *property, QQuickVisualDataGroup *group);
    static int group_count(QDeclarativeListProperty<QQuickVisualDataGroup> *property);
    static QQuickVisualDataGroup *group_at(QDeclarativeListProperty<QQuickVisualDataGroup> *property, int index);

    void releaseIncubator(QVDMIncubationTask *incubationTask);
    void incubatorStatusChanged(QVDMIncubationTask *incubationTask, QDeclarativeIncubator::Status status);
    void setInitialState(QVDMIncubationTask *incubationTask, QObject *o);

    QQuickVisualAdaptorModel *m_adaptorModel;
    QDeclarativeComponent *m_delegate;
    QQuickVisualDataModelItemMetaType *m_cacheMetaType;
    QDeclarativeGuard<QDeclarativeContext> m_context;

    QList<QQuickVisualDataModelItem *> m_cache;
    QQuickVisualDataModelParts *m_parts;
    QQuickVisualDataGroupEmitterList m_pendingParts;

    QDeclarativeListCompositor m_compositor;
    QDeclarativeListCompositor::Group m_compositorGroup;
    bool m_complete : 1;
    bool m_delegateValidated : 1;
    bool m_reset : 1;
    bool m_transaction : 1;
    bool m_incubatorCleanupScheduled : 1;

    QString m_filterGroup;
    QList<QByteArray> watchedRoles;

    union {
        struct {
            QQuickVisualDataGroup *m_cacheItems;
            QQuickVisualDataGroup *m_items;
            QQuickVisualDataGroup *m_persistedItems;
        };
        QQuickVisualDataGroup *m_groups[Compositor::MaximumGroupCount];
    };
    int m_groupCount;

    QList<QVDMIncubationTask *> m_finishedIncubating;
};

class QQuickVisualPartsModel : public QQuickVisualModel, public QQuickVisualDataGroupEmitter
{
    Q_OBJECT
    Q_PROPERTY(QString filterOnGroup READ filterGroup WRITE setFilterGroup NOTIFY filterGroupChanged RESET resetFilterGroup)
public:
    QQuickVisualPartsModel(QQuickVisualDataModel *model, const QString &part, QObject *parent = 0);
    ~QQuickVisualPartsModel();

    QString filterGroup() const;
    void setFilterGroup(const QString &group);
    void resetFilterGroup();
    void updateFilterGroup();
    void updateFilterGroup(Compositor::Group group, const QDeclarativeChangeSet &changeSet);

    int count() const;
    bool isValid() const;
    QQuickItem *item(int index, bool asynchronous=false);
    ReleaseFlags release(QQuickItem *item);
    QString stringValue(int index, const QString &role);
    void setWatchedRoles(QList<QByteArray> roles);

    int indexOf(QQuickItem *item, QObject *objectContext) const;

    void emitModelUpdated(const QDeclarativeChangeSet &changeSet, bool reset);

    void createdPackage(int index, QDeclarativePackage *package);
    void initPackage(int index, QDeclarativePackage *package);
    void destroyingPackage(QDeclarativePackage *package);

Q_SIGNALS:
    void filterGroupChanged();

private:
    QQuickVisualDataModel *m_model;
    QHash<QObject *, QDeclarativePackage *> m_packaged;
    QString m_part;
    QString m_filterGroup;
    QList<QByteArray> m_watchedRoles;
    Compositor::Group m_compositorGroup;
    bool m_inheritGroup;
};

class QMetaPropertyBuilder;

class QQuickVisualDataModelPartsMetaObject : public QDeclarativeOpenMetaObject
{
public:
    QQuickVisualDataModelPartsMetaObject(QObject *parent)
    : QDeclarativeOpenMetaObject(parent) {}

    virtual void propertyCreated(int, QMetaPropertyBuilder &);
    virtual QVariant initialValue(int);
};

class QQuickVisualDataModelParts : public QObject
{
Q_OBJECT
public:
    QQuickVisualDataModelParts(QQuickVisualDataModel *parent);

    QQuickVisualDataModel *model;
    QList<QQuickVisualPartsModel *> models;
};

class QQuickVisualDataModelAttachedMetaObject : public QAbstractDynamicMetaObject
{
public:
    QQuickVisualDataModelAttachedMetaObject(
            QQuickVisualDataModelAttached *attached, QQuickVisualDataModelItemMetaType *metaType);
    ~QQuickVisualDataModelAttachedMetaObject();

    int metaCall(QMetaObject::Call, int _id, void **);

private:
    QQuickVisualDataModelAttached *attached;
    QQuickVisualDataModelItemMetaType *metaType;
};

class QQuickVisualDataModelContext : public QDeclarativeContext
{
    Q_OBJECT
public:
    QQuickVisualDataModelContext(
            QQuickVisualDataModelItem *cacheItem,
            QDeclarativeContext *parentContext,
            QObject *parent = 0)
        : QDeclarativeContext(parentContext, parent)
        , cacheItem(cacheItem)
    {
        ++cacheItem->scriptRef;
    }

    ~QQuickVisualDataModelContext()
    {
        cacheItem->Dispose();
    }

    QQuickVisualDataModelItem *cacheItem;
};

QT_END_NAMESPACE

#endif
