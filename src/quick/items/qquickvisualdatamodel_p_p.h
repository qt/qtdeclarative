/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QQUICKVISUALDATAMODEL_P_P_H
#define QQUICKVISUALDATAMODEL_P_P_H

#include "qquickvisualdatamodel_p.h"

#include "qquickvisualadaptormodel_p.h"

#include <QtQml/qqmlcontext.h>
#include <QtQml/qqmlincubator.h>

#include <private/qqmlopenmetaobject_p.h>

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

typedef QQuickListCompositor Compositor;

class QQuickVisualDataModelAttachedMetaObject;

class QQuickVisualDataModelItemMetaType : public QQmlRefCount
{
public:
    QQuickVisualDataModelItemMetaType(QV8Engine *engine, QQuickVisualDataModel *model, const QStringList &groupNames);
    ~QQuickVisualDataModelItemMetaType();

    void initializeMetaObject();
    void initializeConstructor();

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

    QQmlGuard<QQuickVisualDataModel> model;
    const int groupCount;
    QV8Engine * const v8Engine;
    QQuickVisualDataModelAttachedMetaObject *metaObject;
    const QStringList groupNames;
    v8::Persistent<v8::ObjectTemplate> constructor;
};

class QQuickVisualAdaptorModel;
class QVDMIncubationTask;

class QQuickVisualDataModelItem : public QObject, public QV8ObjectResource
{
    Q_OBJECT
    Q_PROPERTY(int index READ modelIndex NOTIFY modelIndexChanged)
    Q_PROPERTY(QObject *model READ modelObject CONSTANT)
    V8_RESOURCE_TYPE(VisualDataItemType)
public:
    QQuickVisualDataModelItem(QQuickVisualDataModelItemMetaType *metaType, int modelIndex);
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

    QObject *modelObject() { return this; }

    void incubateObject(
            QQmlComponent *component,
            QQmlEngine *engine,
            QQmlContextData *context,
            QQmlContextData *forContext);
    void destroyObject();

    static QQuickVisualDataModelItem *dataForObject(QObject *object);

    int groupIndex(Compositor::Group group);

    int modelIndex() const { return index; }
    void setModelIndex(int idx) { index = idx; emit modelIndexChanged(); }

    virtual v8::Handle<v8::Value> get() { return engine->newQObject(this); }

    virtual void setValue(const QString &role, const QVariant &value) { Q_UNUSED(role); Q_UNUSED(value); }
    virtual bool resolveIndex(const QQuickVisualAdaptorModel &, int) { return false; }

    QQuickVisualDataModelItemMetaType * const metaType;
    QQmlContextData *contextData;
    QObject *object;
    QQuickVisualDataModelAttached *attached;
    QVDMIncubationTask *incubationTask;
    int objectRef;
    int scriptRef;
    int groups;
    int index;


Q_SIGNALS:
    void modelIndexChanged();

protected:
    void objectDestroyed(QObject *);
};


class QQuickVisualDataModelPrivate;
class QVDMIncubationTask : public QQmlIncubator
{
public:
    QVDMIncubationTask(QQuickVisualDataModelPrivate *l, IncubationMode mode)
        : QQmlIncubator(mode)
        , incubating(0)
        , vdm(l) {}

    virtual void statusChanged(Status);
    virtual void setInitialState(QObject *);

    QQuickVisualDataModelItem *incubating;
    QQuickVisualDataModelPrivate *vdm;
    int index[QQuickListCompositor::MaximumGroupCount];
};


class QQuickVisualDataGroupEmitter
{
public:
    virtual ~QQuickVisualDataGroupEmitter() {}
    virtual void emitModelUpdated(const QQuickChangeSet &changeSet, bool reset) = 0;
    virtual void createdPackage(int, QQuickPackage *) {}
    virtual void initPackage(int, QQuickPackage *) {}
    virtual void destroyingPackage(QQuickPackage *) {}

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
    bool isChangedConnected();
    void emitChanges(QV8Engine *engine);
    void emitModelUpdated(bool reset);

    void createdPackage(int index, QQuickPackage *package);
    void initPackage(int index, QQuickPackage *package);
    void destroyingPackage(QQuickPackage *package);

    bool parseIndex(const v8::Local<v8::Value> &value, int *index, Compositor::Group *group) const;
    bool parseGroupArgs(
            QQmlV8Function *args, Compositor::Group *group, int *index, int *count, int *groups) const;

    Compositor::Group group;
    QQmlGuard<QQuickVisualDataModel> model;
    QQuickVisualDataGroupEmitterList emitters;
    QQuickChangeSet changeSet;
    QString name;
    bool defaultInclude;
};

class QQuickVisualDataModelParts;

class QQuickVisualDataModelPrivate : public QObjectPrivate, public QQuickVisualDataGroupEmitter
{
    Q_DECLARE_PUBLIC(QQuickVisualDataModel)
public:
    QQuickVisualDataModelPrivate(QQmlContext *);
    ~QQuickVisualDataModelPrivate();

    static QQuickVisualDataModelPrivate *get(QQuickVisualDataModel *m) {
        return static_cast<QQuickVisualDataModelPrivate *>(QObjectPrivate::get(m));
    }

    void init();
    void connectModel(QQuickVisualAdaptorModel *model);

    QObject *object(Compositor::Group group, int index, bool asynchronous);
    QQuickVisualDataModel::ReleaseFlags release(QObject *object);
    QString stringValue(Compositor::Group group, int index, const QString &name);
    void emitCreatedPackage(QVDMIncubationTask *incubationTask, QQuickPackage *package);
    void emitInitPackage(QVDMIncubationTask *incubationTask, QQuickPackage *package);
    void emitCreatedItem(QVDMIncubationTask *incubationTask, QQuickItem *item) {
        emit q_func()->createdItem(incubationTask->index[m_compositorGroup], item); }
    void emitInitItem(QVDMIncubationTask *incubationTask, QQuickItem *item) {
        emit q_func()->initItem(incubationTask->index[m_compositorGroup], item); }
    void emitDestroyingPackage(QQuickPackage *package);
    void emitDestroyingItem(QQuickItem *item) { emit q_func()->destroyingItem(item); }
    void removeCacheItem(QQuickVisualDataModelItem *cacheItem);

    void updateFilterGroup();

    void addGroups(Compositor::iterator from, int count, Compositor::Group group, int groupFlags);
    void removeGroups(Compositor::iterator from, int count, Compositor::Group group, int groupFlags);
    void setGroups(Compositor::iterator from, int count, Compositor::Group group, int groupFlags);

    void itemsInserted(
            const QVector<Compositor::Insert> &inserts,
            QVarLengthArray<QVector<QQuickChangeSet::Insert>, Compositor::MaximumGroupCount> *translatedInserts,
            QHash<int, QList<QQuickVisualDataModelItem *> > *movedItems = 0);
    void itemsInserted(const QVector<Compositor::Insert> &inserts);
    void itemsRemoved(
            const QVector<Compositor::Remove> &removes,
            QVarLengthArray<QVector<QQuickChangeSet::Remove>, Compositor::MaximumGroupCount> *translatedRemoves,
            QHash<int, QList<QQuickVisualDataModelItem *> > *movedItems = 0);
    void itemsRemoved(const QVector<Compositor::Remove> &removes);
    void itemsMoved(
            const QVector<Compositor::Remove> &removes, const QVector<Compositor::Insert> &inserts);
    void itemsChanged(const QVector<Compositor::Change> &changes);
    template <typename T> static v8::Local<v8::Array> buildChangeList(const QVector<T> &changes);
    void emitChanges();
    void emitModelUpdated(const QQuickChangeSet &changeSet, bool reset);

    bool insert(Compositor::insert_iterator &before, const v8::Local<v8::Object> &object, int groups);

    static void group_append(QQmlListProperty<QQuickVisualDataGroup> *property, QQuickVisualDataGroup *group);
    static int group_count(QQmlListProperty<QQuickVisualDataGroup> *property);
    static QQuickVisualDataGroup *group_at(QQmlListProperty<QQuickVisualDataGroup> *property, int index);

    void releaseIncubator(QVDMIncubationTask *incubationTask);
    void incubatorStatusChanged(QVDMIncubationTask *incubationTask, QQmlIncubator::Status status);
    void setInitialState(QVDMIncubationTask *incubationTask, QObject *o);

    QQuickVisualAdaptorModel m_adaptorModel;
    QQuickListCompositor m_compositor;
    QQmlComponent *m_delegate;
    QQuickVisualDataModelItemMetaType *m_cacheMetaType;
    QQmlContext *m_context;
    QQuickVisualDataModelParts *m_parts;
    QQuickVisualDataGroupEmitterList m_pendingParts;

    QList<QQuickVisualDataModelItem *> m_cache;
    QList<QVDMIncubationTask *> m_finishedIncubating;
    QList<QByteArray> m_watchedRoles;

    QString m_filterGroup;

    int m_count;
    int m_groupCount;

    QQuickListCompositor::Group m_compositorGroup;
    bool m_complete : 1;
    bool m_delegateValidated : 1;
    bool m_reset : 1;
    bool m_transaction : 1;
    bool m_incubatorCleanupScheduled : 1;

    union {
        struct {
            QQuickVisualDataGroup *m_cacheItems;
            QQuickVisualDataGroup *m_items;
            QQuickVisualDataGroup *m_persistedItems;
        };
        QQuickVisualDataGroup *m_groups[Compositor::MaximumGroupCount];
    };
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
    void updateFilterGroup(Compositor::Group group, const QQuickChangeSet &changeSet);

    int count() const;
    bool isValid() const;
    QQuickItem *item(int index, bool asynchronous=false);
    ReleaseFlags release(QQuickItem *item);
    QString stringValue(int index, const QString &role);
    QList<QByteArray> watchedRoles() const { return m_watchedRoles; }
    void setWatchedRoles(QList<QByteArray> roles);

    int indexOf(QQuickItem *item, QObject *objectContext) const;

    void emitModelUpdated(const QQuickChangeSet &changeSet, bool reset);

    void createdPackage(int index, QQuickPackage *package);
    void initPackage(int index, QQuickPackage *package);
    void destroyingPackage(QQuickPackage *package);

Q_SIGNALS:
    void filterGroupChanged();

private:
    QQuickVisualDataModel *m_model;
    QHash<QObject *, QQuickPackage *> m_packaged;
    QString m_part;
    QString m_filterGroup;
    QList<QByteArray> m_watchedRoles;
    Compositor::Group m_compositorGroup;
    bool m_inheritGroup;
};

class QMetaPropertyBuilder;

class QQuickVisualDataModelPartsMetaObject : public QQmlOpenMetaObject
{
public:
    QQuickVisualDataModelPartsMetaObject(QObject *parent)
    : QQmlOpenMetaObject(parent) {}

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

class QQuickVisualDataModelAttachedMetaObject : public QAbstractDynamicMetaObject, public QQmlRefCount
{
public:
    QQuickVisualDataModelAttachedMetaObject(
            QQuickVisualDataModelItemMetaType *metaType, QMetaObject *metaObject);
    ~QQuickVisualDataModelAttachedMetaObject();

    void objectDestroyed(QObject *);
    int metaCall(QObject *, QMetaObject::Call, int _id, void **);

private:
    QQuickVisualDataModelItemMetaType * const metaType;
    QMetaObject * const metaObject;
    const int memberPropertyOffset;
    const int indexPropertyOffset;
};

QT_END_NAMESPACE

#endif
