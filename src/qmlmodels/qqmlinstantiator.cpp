// Copyright (C) 2016 Research In Motion.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#include "qqmlinstantiator_p.h"
#include "qqmlinstantiator_p_p.h"
#include <QtQml/QQmlContext>
#include <QtQml/QQmlComponent>
#include <QtQml/QQmlInfo>
#include <QtQml/QQmlError>
#include <QtQmlModels/private/qqmlobjectmodel_p.h>
#include <QtQmlModels/private/qqmldelegatemodel_p.h>

QT_BEGIN_NAMESPACE

QQmlInstantiatorPrivate::QQmlInstantiatorPrivate()
    : componentComplete(true)
    , active(true)
    , async(false)
    , ownModel(false)
{
}

void QQmlInstantiatorPrivate::clear()
{
    Q_Q(QQmlInstantiator);
    if (!model)
        return;

    if (objects.isEmpty())
        return;

    for (int i=0; i < objects.size(); i++) {
        QObject *object = objects[i];
        emit q->objectRemoved(i, object);
        model->release(object);
        if (object && object->parent() == q)
            object->setParent(nullptr);
    }

    objects.clear();
    emit q->objectChanged();
}

QObject *QQmlInstantiatorPrivate::modelObject(int index, bool async)
{
    requestedIndex = index;
    QObject *o = model->object(index, async ? QQmlIncubator::Asynchronous : QQmlIncubator::AsynchronousIfNested);
    requestedIndex = -1;
    return o;
}


void QQmlInstantiatorPrivate::regenerate()
{
    Q_Q(QQmlInstantiator);
    if (!componentComplete)
        return;

    int prevCount = q->count();

    clear();

    if (!active || !model || !model->count() || !model->isValid()) {
        if (prevCount)
            q->countChanged();
        return;
    }

    for (int i = 0; i < model->count(); i++) {
        QObject *object = modelObject(i, async);
        // If the item was already created we won't get a createdItem
        if (object)
            _q_createdItem(i, object);
    }
    if (q->count() != prevCount)
        q->countChanged();
}

void QQmlInstantiatorPrivate::_q_createdItem(int idx, QObject* item)
{
    Q_Q(QQmlInstantiator);
    if (objects.contains(item)) //Case when it was created synchronously in regenerate
        return;
    if (requestedIndex != idx) // Asynchronous creation, reference the object
        (void)model->object(idx);
    if (!item->parent())
        item->setParent(q);
    if (objects.size() < idx + 1) {
        int modelCount = model->count();
        if (objects.capacity() < modelCount)
            objects.reserve(modelCount);
        objects.resize(idx + 1);
    }
    if (QObject *o = objects.at(idx))
        model->release(o);
    objects.replace(idx, item);
    if (objects.size() == 1)
        q->objectChanged();
    q->objectAdded(idx, item);
}

void QQmlInstantiatorPrivate::_q_modelUpdated(const QQmlChangeSet &changeSet, bool reset)
{
    Q_Q(QQmlInstantiator);

    if (!componentComplete || !active)
        return;

    if (reset) {
        regenerate();
        if (changeSet.difference() != 0)
            q->countChanged();
        return;
    }

    int difference = 0;
    QHash<int, QList<QPointer<QObject> > > moved;
    const QList<QQmlChangeSet::Change> &removes = changeSet.removes();
    for (const QQmlChangeSet::Change &remove : removes) {
        int index = qMin(remove.index, objects.size());
        int count = qMin(remove.index + remove.count, objects.size()) - index;
        if (remove.isMove()) {
            moved.insert(remove.moveId, objects.mid(index, count));
            objects.erase(
                    objects.begin() + index,
                    objects.begin() + index + count);
        } else while (count--) {
            QObject *obj = objects.at(index);
            objects.remove(index);
            q->objectRemoved(index, obj);
            if (obj)
                model->release(obj);
        }

        difference -= remove.count;
    }

    const QList<QQmlChangeSet::Change> &inserts = changeSet.inserts();
    for (const QQmlChangeSet::Change &insert : inserts) {
        int index = qMin(insert.index, objects.size());
        if (insert.isMove()) {
            QList<QPointer<QObject> > movedObjects = moved.value(insert.moveId);
            objects = objects.mid(0, index) + movedObjects + objects.mid(index);
        } else {
            if (insert.index <= objects.size())
                objects.insert(insert.index, insert.count, nullptr);
            for (int i = 0; i < insert.count; ++i) {
                int modelIndex = index + i;
                QObject* obj = modelObject(modelIndex, async);
                if (obj)
                    _q_createdItem(modelIndex, obj);
            }
        }
        difference += insert.count;
    }

    if (difference != 0)
        q->countChanged();
}

/*!
    \qmltype Instantiator
    \nativetype QQmlInstantiator
    \inqmlmodule QtQml.Models
    \ingroup qtquick-models
    \brief Dynamically creates objects.

    A Instantiator can be used to control the dynamic creation of objects, or to dynamically
    create multiple objects from a template.

    The Instantiator element will manage the objects it creates. Those objects are parented to the
    Instantiator and can also be deleted by the Instantiator if the Instantiator's properties change. Objects
    can also be destroyed dynamically through other means, and the Instantiator will not recreate
    them unless the properties of the Instantiator change.

    \note Instantiator is part of QtQml.Models since version 2.14 and part of QtQml since
    version 2.1. Importing Instantiator via QtQml is deprecated since Qt 5.14.
*/
QQmlInstantiator::QQmlInstantiator(QObject *parent)
    : QObject(*(new QQmlInstantiatorPrivate), parent)
{
}

QQmlInstantiator::~QQmlInstantiator()
{
    Q_D(QQmlInstantiator);
    d->clear();
    QQmlDelegateModelPointer model(d->model);
    d->disconnectModel(this, &model);
}

/*!
    \qmlsignal QtQml.Models::Instantiator::objectAdded(int index, QtObject object)

    This signal is emitted when an object is added to the Instantiator. The \a index
    parameter holds the index which the object has been given, and the \a object
    parameter holds the \l QtObject that has been added.
*/

/*!
    \qmlsignal QtQml.Models::Instantiator::objectRemoved(int index, QtObject object)

    This signal is emitted when an object is removed from the Instantiator. The \a index
    parameter holds the index which the object had been given, and the \a object
    parameter holds the \l QtObject that has been removed.

    Do not keep a reference to \a object if it was created by this Instantiator, as
    in these cases it will be deleted shortly after the signal is handled.
*/
/*!
    \qmlproperty bool QtQml.Models::Instantiator::active

    When active is true, and the delegate component is ready, the Instantiator will
    create objects according to the model. When active is false, no objects
    will be created and any previously created objects will be destroyed.

    Default is true.
*/
bool QQmlInstantiator::isActive() const
{
    Q_D(const QQmlInstantiator);
    return d->active;
}

void QQmlInstantiator::setActive(bool newVal)
{
    Q_D(QQmlInstantiator);
    if (newVal == d->active)
        return;
    d->active = newVal;
    emit activeChanged();
    d->regenerate();
}

/*!
    \qmlproperty bool QtQml.Models::Instantiator::asynchronous

    When asynchronous is true the Instantiator will attempt to create objects
    asynchronously. This means that objects may not be available immediately,
    even if active is set to true.

    You can use the objectAdded signal to respond to items being created.

    Default is false.
*/
bool QQmlInstantiator::isAsync() const
{
    Q_D(const QQmlInstantiator);
    return d->async;
}

void QQmlInstantiator::setAsync(bool newVal)
{
    Q_D(QQmlInstantiator);
    if (newVal == d->async)
        return;
    d->async = newVal;
    emit asynchronousChanged();
}


/*!
    \qmlproperty int QtQml.Models::Instantiator::count

    The number of objects the Instantiator is currently managing.
*/

int QQmlInstantiator::count() const
{
    Q_D(const QQmlInstantiator);
    return d->objects.size();
}

/*!
    \qmlproperty QtQml::Component QtQml.Models::Instantiator::delegate
    \qmldefault

    The component used to create all objects.

    Note that an extra variable, index, will be available inside instances of the
    delegate. This variable refers to the index of the instance inside the Instantiator,
    and can be used to obtain the object through the objectAt method of the Instantiator.

    If this property is changed, all instances using the old delegate will be destroyed
    and new instances will be created using the new delegate.
*/
QQmlComponent* QQmlInstantiator::delegate()
{
    Q_D(QQmlInstantiator);
    return d->delegate;
}

void QQmlInstantiator::setDelegate(QQmlComponent* c)
{
    Q_D(QQmlInstantiator);
    if (c == d->delegate)
        return;

    d->delegate = c;
    emit delegateChanged();

    if (!d->ownModel)
        return;

    if (QQmlDelegateModel *dModel = qobject_cast<QQmlDelegateModel*>(d->model))
        dModel->setDelegate(c);
    else if (d->componentComplete)
        d->regenerate();
}

/*!
    \qmlproperty variant QtQml.Models::Instantiator::model

    This property can be set to any of the supported \l {qml-data-models}{data models}:

    \list
    \li A number that indicates the number of delegates to be created by the repeater
    \li A model (e.g. a ListModel item, or a QAbstractItemModel subclass)
    \li A string list
    \li An object list
    \endlist

    The type of model affects the properties that are exposed to the \l delegate.

    Default value is 1, which creates a single delegate instance.

    \sa {qml-data-models}{Data Models}
*/

QVariant QQmlInstantiator::model() const
{
    Q_D(const QQmlInstantiator);

    if (d->ownModel)
        return static_cast<QQmlDelegateModel *>(d->model.data())->model();
    if (d->model)
        return QVariant::fromValue(d->model);
    return QVariant();
}

void QQmlInstantiator::setModel(const QVariant &m)
{
    Q_D(QQmlInstantiator);
    QVariant model = m;
    if (model.userType() == qMetaTypeId<QJSValue>())
        model = model.value<QJSValue>().toVariant();

    QQmlDelegateModelPointer oldModel(d->model);
    if (d->ownModel) {
        if (oldModel.delegateModel()->model() == model)
            return;
    } else if (QVariant::fromValue(d->model) == model) {
        return;
    }

    d->clear();

    d->disconnectModel(this, &oldModel);
    d->model = nullptr;

    QObject *object = qvariant_cast<QObject *>(model);

    QQmlDelegateModelPointer newModel(qobject_cast<QQmlInstanceModel *>(object));
    if (newModel) {
        if (d->ownModel) {
            delete oldModel.instanceModel();
            d->ownModel = false;
        }
        d->model = newModel.instanceModel();
    } else if (d->ownModel) {
        // d->ownModel can only be set if the old model is a QQmlDelegateModel.
        Q_ASSERT(oldModel.delegateModel());
        newModel = oldModel;
        d->model = newModel.instanceModel();
        newModel.delegateModel()->setModel(model);
    } else {
        QQmlDelegateModel *own = QQmlDelegateModel::createForView(this, d);
        own->setDelegate(d->delegate);
        own->setDelegateModelAccess(d->delegateModelAccess);
        own->setModel(model);
        newModel = own;
    }

    d->connectModel(this, &newModel);

    emit modelChanged();
}

/*!
    \qmlproperty enumeration QtQml.Models::Instantiator::delegateModelAccess
    \since 6.10

    \include delegatemodelaccess.qdocinc
*/

QQmlDelegateModel::DelegateModelAccess QQmlInstantiator::delegateModelAccess() const
{
    Q_D(const QQmlInstantiator);
    return d->delegateModelAccess;
}

void QQmlInstantiator::setDelegateModelAccess(
        QQmlDelegateModel::DelegateModelAccess delegateModelAccess)
{
    Q_D(QQmlInstantiator);
    if (delegateModelAccess == d->delegateModelAccess)
        return;

    d->delegateModelAccess = delegateModelAccess;
    emit delegateModelAccessChanged();

    if (!d->ownModel)
        return;

    if (QQmlDelegateModel *dModel = qobject_cast<QQmlDelegateModel*>(d->model))
        dModel->setDelegateModelAccess(delegateModelAccess);
    if (d->componentComplete)
        d->regenerate();
}

/*!
    \qmlproperty QtObject QtQml.Models::Instantiator::object

    This is a reference to the first created object, intended as a convenience
    for the case where only one object has been created.
*/
QObject *QQmlInstantiator::object() const
{
    Q_D(const QQmlInstantiator);
    if (d->objects.size())
        return d->objects[0];
    return nullptr;
}

/*!
    \qmlmethod QtObject QtQml.Models::Instantiator::objectAt(int index)

    Returns a reference to the object with the given \a index.
*/
QObject *QQmlInstantiator::objectAt(int index) const
{
    Q_D(const QQmlInstantiator);
    if (index >= 0 && index < d->objects.size())
        return d->objects[index];
    return nullptr;
}

/*!
 \internal
*/
void QQmlInstantiator::classBegin()
{
    Q_D(QQmlInstantiator);
    d->componentComplete = false;
}

/*!
 \internal
*/
void QQmlInstantiator::componentComplete()
{
    Q_D(QQmlInstantiator);
    d->componentComplete = true;

    if (d->model && d->ownModel)
        static_cast<QQmlDelegateModel *>(d->model.data())->componentComplete();
    else if (!d->ownModel && !d->model)
        setModel(QVariant(1));
    else
        d->regenerate();
}

QT_END_NAMESPACE

#include "moc_qqmlinstantiator_p.cpp"
