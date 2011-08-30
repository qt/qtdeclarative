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
#include <private/qobject_p.h>

#include <QtCore/qhash.h>
#include <QtCore/qlist.h>

QT_BEGIN_NAMESPACE

class QSGVisualDataModelParts;
class QSGVisualDataModelData;
class QSGVisualDataModelDataMetaObject;
class QSGVisualDataModelPrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QSGVisualDataModel)
public:
    QSGVisualDataModelPrivate(QDeclarativeContext *);

    static QSGVisualDataModelPrivate *get(QSGVisualDataModel *m) {
        return static_cast<QSGVisualDataModelPrivate *>(QObjectPrivate::get(m));
    }

    void init();
    void connectModel(QSGVisualAdaptorModel *model);

    QObject *object(int index, bool complete);
    QSGVisualDataModel::ReleaseFlags release(QObject *object);
    QString stringValue(int index, const QString &name);
    void emitCreatedPackage(int index, QDeclarativePackage *package) {
        emit q_func()->createdPackage(index, package); }
    void emitDestroyingPackage(QDeclarativePackage *package) {
        emit q_func()->destroyingPackage(package); }

    QSGVisualAdaptorModel *m_adaptorModel;
    QDeclarativeComponent *m_delegate;
    QDeclarativeGuard<QDeclarativeContext> m_context;

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

    Cache m_cache;
    QHash<QObject *, QDeclarativePackage*> m_packaged;

    QSGVisualDataModelParts *m_parts;
    friend class QSGVisualItemParts;

    friend class QSGVisualDataModelData;
    bool m_delegateValidated : 1;
    bool m_completePending : 1;

    QList<QByteArray> watchedRoles;
};

//---------------------------------------------------------------------------

class QSGVisualPartsModel : public QSGVisualModel
{
    Q_OBJECT

public:
    QSGVisualPartsModel(QSGVisualDataModel *model, const QString &part, QObject *parent = 0);
    ~QSGVisualPartsModel();

    int count() const;
    bool isValid() const;
    QSGItem *item(int index, bool complete=true);
    ReleaseFlags release(QSGItem *item);
    bool completePending() const;
    void completeItem();
    QString stringValue(int index, const QString &role);
    void setWatchedRoles(QList<QByteArray> roles);

    int indexOf(QSGItem *item, QObject *objectContext) const;

public Q_SLOTS:
    void createdPackage(int index, QDeclarativePackage *package);
    void destroyingPackage(QDeclarativePackage *package);

private:
    QSGVisualDataModel *m_model;
    QHash<QObject *, QDeclarativePackage *> m_packaged;
    QString m_part;
    QList<QByteArray> m_watchedRoles;
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
    QSGVisualPartsModel *m = new QSGVisualPartsModel(
            static_cast<QSGVisualDataModelParts *>(object())->model,
            QString::fromUtf8(name(id)),
            object());
    return QVariant::fromValue(static_cast<QObject *>(m));
}

QSGVisualDataModelParts::QSGVisualDataModelParts(QSGVisualDataModel *parent)
: QObject(parent), model(parent)
{
    new QSGVisualDataModelPartsMetaObject(this);
}

QSGVisualDataModelPrivate::QSGVisualDataModelPrivate(QDeclarativeContext *ctxt)
    : m_adaptorModel(0)
    , m_delegate(0)
    , m_context(ctxt)
    , m_parts(0)
    , m_delegateValidated(false)
    , m_completePending(false)
{
}

//---------------------------------------------------------------------------

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
    m_adaptorModel = new QSGVisualAdaptorModel;
    QObject::connect(m_adaptorModel, SIGNAL(rootIndexChanged()), q, SIGNAL(rootIndexChanged()));
    connectModel(m_adaptorModel);
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
    delete d->m_adaptorModel;
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
    if (d->m_adaptorModel->canFetchMore())
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
    bool wasValid = d->m_delegate != 0;
    d->m_delegate = delegate;
    d->m_delegateValidated = false;
    if (!wasValid && d->m_adaptorModel->count() && d->m_delegate) {
        emit itemsInserted(0, d->m_adaptorModel->count());
        emit countChanged();
    }
    if (wasValid && !d->m_delegate && d->m_adaptorModel->count()) {
        _q_itemsRemoved(0, d->m_adaptorModel->count());
        emit countChanged();
    }
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

int QSGVisualDataModel::count() const
{
    Q_D(const QSGVisualDataModel);
    if (!d->m_delegate)
        return 0;
    return d->m_adaptorModel->count();
}

QSGVisualDataModel::ReleaseFlags QSGVisualDataModelPrivate::release(QObject *object)
{
    QSGVisualDataModel::ReleaseFlags stat = 0;

    if (m_cache.releaseItem(object)) {
        // Remove any bindings to avoid warnings due to parent change.
        QObjectPrivate *p = QObjectPrivate::get(object);
        Q_ASSERT(p->declarativeData);
        QDeclarativeData *d = static_cast<QDeclarativeData*>(p->declarativeData);
        if (d->ownContext && d->context)
            d->context->clearContext();
        stat |= QSGVisualDataModel::Destroyed;
        object->deleteLater();
    } else {
        stat |= QSGVisualDataModel::Referenced;
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

QObject *QSGVisualDataModelPrivate::object(int index, bool complete)
{
    Q_Q(QSGVisualDataModel);
    if (m_adaptorModel->count() <= 0 || !m_delegate)
        return 0;
    QObject *nobj = m_cache.getItem(index);
    bool needComplete = false;
    if (!nobj) {
        QObject *data = m_adaptorModel->data(index);

        QDeclarativeContext *rootContext = new QDeclarativeContext(
                m_context ? m_context.data() : qmlContext(q));
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
        nobj = m_delegate->beginCreate(ctxt);
        if (complete) {
            m_delegate->completeCreate();
        } else {
            m_completePending = true;
            needComplete = true;
        }
        if (nobj) {
            QDeclarative_setParent_noEvent(rootContext, nobj);
            m_cache.insertItem(index, nobj);
            if (QDeclarativePackage *package = qobject_cast<QDeclarativePackage *>(nobj))
                emitCreatedPackage(index, package);
        } else {
            delete rootContext;
            qmlInfo(q, m_delegate->errors()) << "Error creating delegate";
        }
    }

    if (index == m_adaptorModel->count() -1 && m_adaptorModel->canFetchMore())
        QCoreApplication::postEvent(q, new QEvent(QEvent::UpdateRequest));

    return nobj;
}

QSGItem *QSGVisualDataModel::item(int index, bool complete)
{
    Q_D(QSGVisualDataModel);
    QObject *object = d->object(index, complete);
    if (QSGItem *item = qobject_cast<QSGItem *>(object))
        return item;

    if (completePending())
        completeItem();
    d->release(object);
    if (!d->m_delegateValidated) {
        qmlInfo(d->m_delegate) << tr("Delegate component must be Item type.");
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

QString QSGVisualDataModelPrivate::stringValue(int index, const QString &name)
{
    return m_adaptorModel->stringValue(index, name);
}

QString QSGVisualDataModel::stringValue(int index, const QString &name)
{
    Q_D(QSGVisualDataModel);
    return d->stringValue(index, name);
}

int QSGVisualDataModel::indexOf(QSGItem *item, QObject *) const
{
    Q_D(const QSGVisualDataModel);
    return d->m_adaptorModel->indexOf(item);
}

void QSGVisualDataModel::setWatchedRoles(QList<QByteArray> roles)
{
    Q_D(QSGVisualDataModel);
    d->m_adaptorModel->replaceWatchedRoles(d->watchedRoles, roles);
    d->watchedRoles = roles;
}

bool QSGVisualDataModel::event(QEvent *e)
{
    Q_D(QSGVisualDataModel);
    if (e->type() == QEvent::UpdateRequest)
        d->m_adaptorModel->fetchMore();
    return QSGVisualModel::event(e);
}

void QSGVisualDataModel::_q_itemsChanged(int index, int count)
{
    Q_D(QSGVisualDataModel);
    if (!d->m_delegate)
        return;
    emit itemsChanged(index, count);
}

void QSGVisualDataModel::_q_itemsInserted(int index, int count)
{
    Q_D(QSGVisualDataModel);
    if (!d->m_delegate)
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
        } else {
            ++iter;
        }
    }
    d->m_cache.unite(items);

    emit itemsInserted(index, count);
    emit countChanged();
}

void QSGVisualDataModel::_q_itemsRemoved(int index, int count)
{
    Q_D(QSGVisualDataModel);
    if (!d->m_delegate)
        return;
        // XXX - highly inefficient
    QHash<int, QSGVisualDataModelPrivate::ObjectRef> items;
    for (QHash<int, QSGVisualDataModelPrivate::ObjectRef>::Iterator iter = d->m_cache.begin();
        iter != d->m_cache.end(); ) {
        if (iter.key() >= index && iter.key() < index + count) {
            QSGVisualDataModelPrivate::ObjectRef objRef = *iter;
            iter = d->m_cache.erase(iter);
            items.insertMulti(-1, objRef); //XXX perhaps better to maintain separately
        } else if (iter.key() >= index + count) {
            QSGVisualDataModelPrivate::ObjectRef objRef = *iter;
            int index = iter.key() - count;
            iter = d->m_cache.erase(iter);
            items.insert(index, objRef);
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
    if (!d->m_delegate)
        return;

    // XXX - highly inefficient
    QHash<int,QSGVisualDataModelPrivate::ObjectRef> items;
    for (QHash<int,QSGVisualDataModelPrivate::ObjectRef>::Iterator iter = d->m_cache.begin();
        iter != d->m_cache.end(); ) {
        if (iter.key() >= from && iter.key() < from + count) {
            QSGVisualDataModelPrivate::ObjectRef objRef = *iter;
            int index = iter.key() - from + to;
            items.insert(index, objRef);
            iter = d->m_cache.erase(iter);
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
        } else {
            ++iter;
        }
    }
    d->m_cache.unite(items);
    emit itemsMoved(from, to, count);
}

void QSGVisualDataModel::_q_modelReset(int, int)
{
    Q_D(QSGVisualDataModel);
    if (!d->m_delegate)
        return;
    emit modelReset();
    emit countChanged();
}

//============================================================================

QSGVisualPartsModel::QSGVisualPartsModel(QSGVisualDataModel *model, const QString &part, QObject *parent)
    : QSGVisualModel(*new QObjectPrivate, parent)
    , m_model(model)
    , m_part(part)
{
    connect(m_model, SIGNAL(modelReset()), this, SIGNAL(modelReset()));
    connect(m_model, SIGNAL(itemsInserted(int,int)), this, SIGNAL(itemsInserted(int,int)));
    connect(m_model, SIGNAL(itemsRemoved(int,int)), this, SIGNAL(itemsRemoved(int,int)));
    connect(m_model, SIGNAL(itemsChanged(int,int)), this, SIGNAL(itemsChanged(int,int)));
    connect(m_model, SIGNAL(itemsMoved(int,int,int)), this, SIGNAL(itemsMoved(int,int,int)));
    connect(m_model, SIGNAL(createdPackage(int,QDeclarativePackage*)),
            this, SLOT(createdPackage(int,QDeclarativePackage*)));
    connect(m_model, SIGNAL(destroyingPackage(QDeclarativePackage*)),
            this, SLOT(destroyingPackage(QDeclarativePackage*)));
}

QSGVisualPartsModel::~QSGVisualPartsModel()
{
}

int QSGVisualPartsModel::count() const
{
    return m_model->count();
}

bool QSGVisualPartsModel::isValid() const
{
    return m_model->isValid();
}

QSGItem *QSGVisualPartsModel::item(int index, bool complete)
{
    QSGVisualDataModelPrivate *model = QSGVisualDataModelPrivate::get(m_model);

    QObject *object = model->object(index, complete);

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
    return QSGVisualDataModelPrivate::get(m_model)->stringValue(index, role);
}

void QSGVisualPartsModel::setWatchedRoles(QList<QByteArray> roles)
{
    QSGVisualDataModelPrivate *model = QSGVisualDataModelPrivate::get(m_model);
    model->m_adaptorModel->replaceWatchedRoles(m_watchedRoles, roles);
    m_watchedRoles = roles;
}

int QSGVisualPartsModel::indexOf(QSGItem *item, QObject *) const
{
    const QSGVisualDataModelPrivate *model = QSGVisualDataModelPrivate::get(m_model);
    QHash<QObject *, QDeclarativePackage *>::const_iterator it = m_packaged.find(item);
    return it != m_packaged.end()
            ? model->m_adaptorModel->indexOf(*it)
            : -1;
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

QT_END_NAMESPACE

#include <qsgvisualdatamodel.moc>
