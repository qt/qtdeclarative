// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "qquickrepeater_p.h"
#include "qquickrepeater_p_p.h"

#include <private/qqmlglobal_p.h>
#include <private/qqmlchangeset_p.h>
#include <private/qqmldelegatemodel_p.h>

#include <QtQml/QQmlInfo>
#include <QtQml/qqmlcomponent.h>

QT_BEGIN_NAMESPACE

QQuickRepeaterPrivate::QQuickRepeaterPrivate()
    : model(nullptr)
    , ownModel(false)
    , delegateValidated(false)
    , explicitDelegate(false)
    , explicitDelegateModelAccess(false)
    , itemCount(0)
{
    setTransparentForPositioner(true);
}

QQuickRepeaterPrivate::~QQuickRepeaterPrivate()
{
    if (ownModel)
        delete model;
}

/*!
    \qmltype Repeater
    \nativetype QQuickRepeater
    \inqmlmodule QtQuick
    \ingroup qtquick-models
    \ingroup qtquick-positioning
    \inherits Item
    \brief Instantiates a number of Item-based components using a provided model.

    The Repeater type is used to create a large number of
    similar items. Like other view types, a Repeater has a \l model and a \l delegate:
    for each entry in the model, the delegate is instantiated
    in a context seeded with data from the model. A Repeater item is usually
    enclosed in a positioner type such as \l Row or \l Column to visually
    position the multiple delegate items created by the Repeater.

    The following Repeater creates three instances of a \l Rectangle item within
    a \l Row:

    \snippet qml/repeaters/repeater.qml import
    \codeline
    \snippet qml/repeaters/repeater.qml simple

    \image repeater-simple.png

    A Repeater's \l model can be any of the supported \l {qml-data-models}{data models}.
    Additionally, like delegates for other views, a Repeater delegate can access
    its index within the repeater, as well as the model data relevant to the
    delegate. See the \l delegate property documentation for details.

    Items instantiated by the Repeater are inserted, in order, as
    children of the Repeater's parent.  The insertion starts immediately after
    the repeater's position in its parent stacking list.  This allows
    a Repeater to be used inside a layout. For example, the following Repeater's
    items are stacked between a red rectangle and a blue rectangle:

    \snippet qml/repeaters/repeater.qml layout

    \image repeater.png


    \note A Repeater item owns all items it instantiates. Removing or dynamically destroying
    an item created by a Repeater results in unpredictable behavior.


    \section2 Considerations when using Repeater

    The Repeater type creates all of its delegate items when the repeater is first
    created. This can be inefficient if there are a large number of delegate items and
    not all of the items are required to be visible at the same time. If this is the case,
    consider using other view types like ListView (which only creates delegate items
    when they are scrolled into view) or use the \l {Dynamic Object Creation} methods to
    create items as they are required.

    Also, note that Repeater is \l {Item}-based, and can only repeat \l {Item}-derived objects.
    For example, it cannot be used to repeat QtObjects:

    \qml
    // bad code:
    Item {
        // Can't repeat QtObject as it doesn't derive from Item.
        Repeater {
            model: 10
            QtObject {}
        }
    }
    \endqml
 */

/*!
    \qmlsignal QtQuick::Repeater::itemAdded(int index, Item item)

    This signal is emitted when an item is added to the repeater. The \a index
    parameter holds the index at which the item has been inserted within the
    repeater, and the \a item parameter holds the \l Item that has been added.
*/

/*!
    \qmlsignal QtQuick::Repeater::itemRemoved(int index, Item item)

    This signal is emitted when an item is removed from the repeater. The \a index
    parameter holds the index at which the item was removed from the repeater,
    and the \a item parameter holds the \l Item that was removed.

    Do not keep a reference to \a item if it was created by this repeater, as
    in these cases it will be deleted shortly after the signal is handled.
*/
QQuickRepeater::QQuickRepeater(QQuickItem *parent)
  : QQuickItem(*(new QQuickRepeaterPrivate), parent)
{
}

QQuickRepeater::~QQuickRepeater()
{
    Q_D(QQuickRepeater);
    QQmlDelegateModelPointer model(d->model);
    d->disconnectModel(this, &model);
}

/*!
    \qmlproperty var QtQuick::Repeater::model

    The model providing data for the repeater.

    This property can be set to any of the supported \l {qml-data-models}{data models}:

    \list
    \li A number that indicates the number of delegates to be created by the repeater
    \li A model (e.g. a ListModel item, or a QAbstractItemModel subclass)
    \li A string list
    \li An object list
    \endlist

    The type of model affects the properties that are exposed to the \l delegate.

    \sa {qml-data-models}{Data Models}
*/
QVariant QQuickRepeater::model() const
{
    Q_D(const QQuickRepeater);

    if (d->ownModel)
        return static_cast<QQmlDelegateModel *>(d->model.data())->model();
    if (d->model)
        return QVariant::fromValue(d->model.data());
    return QVariant();
}

void QQuickRepeater::setModel(const QVariant &m)
{
    Q_D(QQuickRepeater);
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

    clear();

    d->disconnectModel(this, &oldModel);
    d->model = nullptr;

    QObject *object = qvariant_cast<QObject *>(model);

    QQmlDelegateModelPointer newModel(qobject_cast<QQmlInstanceModel *>(object));
    if (newModel) {
        if (d->explicitDelegate) {
            QQmlComponent *delegate = nullptr;
            if (QQmlDelegateModel *old = oldModel.delegateModel())
                delegate = old->delegate();

            if (QQmlDelegateModel *delegateModel = newModel.delegateModel()) {
                delegateModel->setDelegate(delegate);
            } else if (delegate) {
                qmlWarning(this) << "Cannot retain explicitly set delegate on non-DelegateModel";
                d->explicitDelegate = false;
            }
        }

        if (d->explicitDelegateModelAccess) {
            QQmlDelegateModel::DelegateModelAccess access = QQmlDelegateModel::Qt5ReadWrite;
            if (QQmlDelegateModel *old = oldModel.delegateModel())
                access = old->delegateModelAccess();

            if (QQmlDelegateModel *delegateModel = newModel.delegateModel()) {
                delegateModel->setDelegateModelAccess(access);
            } else if (access != QQmlDelegateModel::Qt5ReadWrite) {
                qmlWarning(this) << "Cannot retain explicitly set delegate model access "
                                    "on non-DelegateModel";
                d->explicitDelegateModelAccess = false;
            }
        }

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
        newModel = QQmlDelegateModel::createForView(this, d);
        if (d->explicitDelegate) {
            QQmlComponent *delegate = nullptr;
            if (QQmlDelegateModel *old = oldModel.delegateModel())
                delegate = old->delegate();
            newModel.delegateModel()->setDelegate(delegate);
        }

        if (d->explicitDelegateModelAccess) {
            QQmlDelegateModel::DelegateModelAccess access = QQmlDelegateModel::Qt5ReadWrite;
            if (QQmlDelegateModel *old = oldModel.delegateModel())
                access = old->delegateModelAccess();
            newModel.delegateModel()->setDelegateModelAccess(access);
        }

        newModel.delegateModel()->setModel(model);
    }

    d->connectModel(this, &newModel);
    emit modelChanged();
    emit countChanged();
}

/*!
    \qmlproperty Component QtQuick::Repeater::delegate
    \qmldefault

    The delegate provides a template defining each item instantiated by the repeater.

    Delegates are exposed to a read-only \c index property that indicates the index
    of the delegate within the repeater. For example, the following \l Text delegate
    displays the index of each repeated item:

    \table
    \row
    \li \snippet qml/repeaters/repeater.qml index
    \li \image repeater-index.png
    \endtable

    If the \l model is a \l{QStringList-based model}{string list} or
    \l{QObjectList-based model}{object list}, the delegate is also exposed to
    a read-only \c modelData property that holds the string or object data. For
    example:

    \table
    \row
    \li \snippet qml/repeaters/repeater.qml modeldata
    \li \image repeater-modeldata.png
    \endtable

    If the \l model is a model object (such as a \l ListModel) the delegate
    can access all model roles as named properties, in the same way that delegates
    do for view classes like ListView.

    \sa {QML Data Models}
 */
QQmlComponent *QQuickRepeater::delegate() const
{
    Q_D(const QQuickRepeater);
    if (d->model) {
        if (QQmlDelegateModel *dataModel = qobject_cast<QQmlDelegateModel*>(d->model))
            return dataModel->delegate();
    }

    return nullptr;
}

void QQuickRepeater::setDelegate(QQmlComponent *delegate)
{
    Q_D(QQuickRepeater);
    const auto setExplicitDelegate = [&](QQmlDelegateModel *delegateModel) {
        if (delegateModel->delegate() == delegate) {
            d->explicitDelegate = true;
            return;
        }

        const int oldCount = delegateModel->count();
        delegateModel->setDelegate(delegate);
        regenerate();
        if (oldCount != delegateModel->count())
            emit countChanged();
        d->explicitDelegate = true;
        d->delegateValidated = false;
    };

    if (!d->model) {
        if (!delegate) {
            // Explicitly set a null delegate. We can do this without model.
            d->explicitDelegate = true;
            return;
        }

        setExplicitDelegate(QQmlDelegateModel::createForView(this, d));
        // The new model is not connected to applyDelegateChange, yet. We only do this once
        // there is actual data, via an explicit setModel(). So we have to manually emit the
        // delegateChanged() here.
        emit delegateChanged();
        return;
    }

    if (QQmlDelegateModel *delegateModel = qobject_cast<QQmlDelegateModel *>(d->model)) {
        // Disable the warning in applyDelegateChange since the new delegate is also explicit.
        d->explicitDelegate = false;
        setExplicitDelegate(delegateModel);
        return;
    }

    if (delegate)
        qmlWarning(this) << "Cannot set a delegate on an explicitly provided non-DelegateModel";
    else
        d->explicitDelegate = true; // Explicitly set null delegate always works
}

/*!
    \qmlproperty int QtQuick::Repeater::count

    This property holds the number of items in the \l model.

    The value of \c count does not always match the number of instantiated
    \l {delegate}{delegates}; use \l itemAt() to check if a delegate at a given
    index exists. It returns \c null if the delegate is not instantiated.

    \list
    \li While the Repeater is in the process of instantiating delegates (at
        startup, or because of \c model changes), the \l itemAdded signal is
        emitted for each delegate created, and the \c count property changes
        afterwards.
    \li If the Repeater is not part of a completed
        \l {Concepts - Visual Parent in Qt Quick}{visual hierarchy},
        \c count reflects the model size, but no delegates are created.
    \li If the Repeater destroys delegates because of \c model changes,
        the \l itemRemoved() signal is emitted for each, and the \c count
        property changes afterwards.
    \li If the Repeater is taken out of the visual hierarchy (for example by
        setting \c {parent = null}), delegates are destroyed, the \l itemRemoved()
        signal is emitted for each, but \c count does not change.
    \endlist

    \sa itemAt(), itemAdded(), itemRemoved()
*/
int QQuickRepeater::count() const
{
    Q_D(const QQuickRepeater);
    if (d->model)
        return d->model->count();
    return 0;
}

/*!
    \qmlmethod Item QtQuick::Repeater::itemAt(index)

    Returns the \l Item that has been created at the given \a index, or \c null
    if no item exists at \a index.
*/
QQuickItem *QQuickRepeater::itemAt(int index) const
{
    Q_D(const QQuickRepeater);
    if (index >= 0 && index < d->deletables.size())
        return d->deletables[index];
    return nullptr;
}

void QQuickRepeater::componentComplete()
{
    Q_D(QQuickRepeater);
    if (d->model && d->ownModel)
        static_cast<QQmlDelegateModel *>(d->model.data())->componentComplete();
    QQuickItem::componentComplete();
    regenerate();
    if (d->model && d->model->count())
        emit countChanged();
}

void QQuickRepeater::itemChange(ItemChange change, const ItemChangeData &value)
{
    QQuickItem::itemChange(change, value);
    if (change == ItemParentHasChanged) {
        regenerate();
    }
}

void QQuickRepeater::clear()
{
    Q_D(QQuickRepeater);
    bool complete = isComponentComplete();

    if (d->model) {
        // We remove in reverse order deliberately; so that signals are emitted
        // with sensible indices.
        for (int i = d->deletables.size() - 1; i >= 0; --i) {
            if (QQuickItem *item = d->deletables.at(i)) {
                if (complete)
                    emit itemRemoved(i, item);
                d->model->release(item);
            }
        }
        for (QQuickItem *item : std::as_const(d->deletables)) {
            if (item)
                item->setParentItem(nullptr);
        }
    }
    d->deletables.clear();
    d->itemCount = 0;
}

void QQuickRepeater::regenerate()
{
    Q_D(QQuickRepeater);
    if (!isComponentComplete())
        return;

    clear();

    if (!d->model || !d->model->count() || !d->model->isValid() || !parentItem() || !isComponentComplete())
        return;

    d->itemCount = count();
    d->deletables.resize(d->itemCount);
    d->requestItems();
}

void QQuickRepeaterPrivate::requestItems()
{
    for (int i = 0; i < itemCount; i++) {
        QObject *object = model->object(i, QQmlIncubator::AsynchronousIfNested);
        if (object)
            model->release(object);
    }
}

void QQuickRepeaterPrivate::connectModel(QQuickRepeater *q, QQmlDelegateModelPointer *model)
{
    QQmlInstanceModel *instanceModel = model->instanceModel();
    if (!instanceModel)
        return;

    QObject::connect(instanceModel, &QQmlInstanceModel::modelUpdated,
                     q, &QQuickRepeater::modelUpdated);
    QObject::connect(instanceModel, &QQmlInstanceModel::createdItem,
                     q, &QQuickRepeater::createdItem);
    QObject::connect(instanceModel, &QQmlInstanceModel::initItem,
                     q, &QQuickRepeater::initItem);
    if (QQmlDelegateModel *dataModel = model->delegateModel()) {
        QObjectPrivate::connect(
                dataModel, &QQmlDelegateModel::delegateChanged,
                this, &QQuickRepeaterPrivate::applyDelegateChange);
        QObjectPrivate::connect(
                dataModel, &QQmlDelegateModel::delegateModelAccessChanged,
                this, &QQuickRepeaterPrivate::applyDelegateModelAccessChange);
        if (ownModel) {
            QObject::connect(dataModel, &QQmlDelegateModel::modelChanged,
                             q, &QQuickRepeater::modelChanged);
        }
    }
    q->regenerate();
}

void QQuickRepeaterPrivate::disconnectModel(QQuickRepeater *q, QQmlDelegateModelPointer *model)
{
    QQmlInstanceModel *instanceModel = model->instanceModel();
    if (!instanceModel)
        return;

    QObject::disconnect(instanceModel, &QQmlInstanceModel::modelUpdated,
                        q, &QQuickRepeater::modelUpdated);
    QObject::disconnect(instanceModel, &QQmlInstanceModel::createdItem,
                        q, &QQuickRepeater::createdItem);
    QObject::disconnect(instanceModel, &QQmlInstanceModel::initItem,
                        q, &QQuickRepeater::initItem);
    if (QQmlDelegateModel *delegateModel = model->delegateModel()) {
        QObjectPrivate::disconnect(
                delegateModel, &QQmlDelegateModel::delegateChanged,
                this, &QQuickRepeaterPrivate::applyDelegateChange);
        QObjectPrivate::disconnect(
                delegateModel, &QQmlDelegateModel::delegateModelAccessChanged,
                this, &QQuickRepeaterPrivate::applyDelegateModelAccessChange);
        if (ownModel) {
            QObject::disconnect(delegateModel, &QQmlDelegateModel::modelChanged,
                                q, &QQuickRepeater::modelChanged);
        }
    }
}

void QQuickRepeater::createdItem(int index, QObject *)
{
    Q_D(QQuickRepeater);
    QObject *object = d->model->object(index, QQmlIncubator::AsynchronousIfNested);
    QQuickItem *item = qmlobject_cast<QQuickItem*>(object);
    emit itemAdded(index, item);
}

void QQuickRepeater::initItem(int index, QObject *object)
{
    Q_D(QQuickRepeater);
    if (index >= d->deletables.size()) {
        // this can happen when Package is used
        // calling regenerate does too much work, all we need is to call resize
        // so that d->deletables[index] = item below works
        d->deletables.resize(d->model->count() + 1);
    }
    QQuickItem *item = qmlobject_cast<QQuickItem*>(object);

    if (!d->deletables.at(index)) {
        if (!item) {
            if (object) {
                d->model->release(object);
                if (!d->delegateValidated) {
                    d->delegateValidated = true;
                    QObject* delegate = this->delegate();
                    qmlWarning(delegate ? delegate : this) << QQuickRepeater::tr("Delegate must be of Item type");
                }
            }
            return;
        }
        d->deletables[index] = item;
        item->setParentItem(parentItem());

        // If the item comes from an ObjectModel, it might be used as
        // ComboBox/Menu/TabBar's contentItem. These types unconditionally cull items
        // that are inserted, so account for that here.
        if (d->model && !d->ownModel)
            QQuickItemPrivate::get(item)->setCulled(false);
        if (index > 0 && d->deletables.at(index-1)) {
            item->stackAfter(d->deletables.at(index-1));
        } else {
            QQuickItem *after = this;
            for (int si = index+1; si < d->itemCount; ++si) {
                if (d->deletables.at(si)) {
                    after = d->deletables.at(si);
                    break;
                }
            }
            item->stackBefore(after);
        }
    }
}

void QQuickRepeater::modelUpdated(const QQmlChangeSet &changeSet, bool reset)
{
    Q_D(QQuickRepeater);

    if (!isComponentComplete())
        return;

    if (reset) {
        regenerate();
        if (changeSet.difference() != 0)
            emit countChanged();
        return;
    }

    int difference = 0;
    QHash<int, QList<QPointer<QQuickItem> > > moved;
    for (const QQmlChangeSet::Change &remove : changeSet.removes()) {
        int index = qMin(remove.index, d->deletables.size());
        int count = qMin(remove.index + remove.count, d->deletables.size()) - index;
        if (remove.isMove()) {
            moved.insert(remove.moveId, d->deletables.mid(index, count));
            d->deletables.erase(
                    d->deletables.begin() + index,
                    d->deletables.begin() + index + count);
        } else while (count--) {
            QQuickItem *item = d->deletables.at(index);
            d->deletables.remove(index);
            emit itemRemoved(index, item);
            if (item) {
                d->model->release(item);
                item->setParentItem(nullptr);
            }
            --d->itemCount;
        }

        difference -= remove.count;
    }

    for (const QQmlChangeSet::Change &insert : changeSet.inserts()) {
        int index = qMin(insert.index, d->deletables.size());
        if (insert.isMove()) {
            QList<QPointer<QQuickItem> > items = moved.value(insert.moveId);
            d->deletables = d->deletables.mid(0, index) + items + d->deletables.mid(index);
            QQuickItem *stackBefore = index + items.size() < d->deletables.size()
                    ? d->deletables.at(index + items.size())
                    : this;
            if (stackBefore) {
                for (int i = index; i < index + items.size(); ++i) {
                    if (i < d->deletables.size()) {
                        QPointer<QQuickItem> item = d->deletables.at(i);
                        if (item)
                            item->stackBefore(stackBefore);
                    }
                }
            }
        } else for (int i = 0; i < insert.count; ++i) {
            int modelIndex = index + i;
            ++d->itemCount;
            d->deletables.insert(modelIndex, nullptr);
            QObject *object = d->model->object(modelIndex, QQmlIncubator::AsynchronousIfNested);
            if (object)
                d->model->release(object);
        }
        difference += insert.count;
    }

    if (difference != 0)
        emit countChanged();
}

/*!
    \qmlproperty enumeration QtQuick::Repeater::delegateModelAccess
    \since 6.10

    \include delegatemodelaccess.qdocinc
*/
QQmlDelegateModel::DelegateModelAccess QQuickRepeater::delegateModelAccess() const
{
    Q_D(const QQuickRepeater);
    if (QQmlDelegateModel *dataModel = qobject_cast<QQmlDelegateModel *>(d->model))
        return dataModel->delegateModelAccess();
    return QQmlDelegateModel::Qt5ReadWrite;
}

void QQuickRepeater::setDelegateModelAccess(
        QQmlDelegateModel::DelegateModelAccess delegateModelAccess)
{
    Q_D(QQuickRepeater);
    const auto setExplicitDelegateModelAccess = [&](QQmlDelegateModel *delegateModel) {
        delegateModel->setDelegateModelAccess(delegateModelAccess);
        d->explicitDelegateModelAccess = true;
    };

    if (!d->model) {
        if (delegateModelAccess == QQmlDelegateModel::Qt5ReadWrite) {
            // Explicitly set delegateModelAccess to Legacy. We can do this without model.
            d->explicitDelegateModelAccess = true;
            return;
        }

        setExplicitDelegateModelAccess(QQmlDelegateModel::createForView(this, d));

        // The new model is not connected to applyDelegateModelAccessChange, yet. We only do this
        // once there is actual data, via an explicit setModel(). So we have to manually emit the
        // delegateModelAccessChanged() here.
        emit delegateModelAccessChanged();
        return;
    }

    if (QQmlDelegateModel *delegateModel = qobject_cast<QQmlDelegateModel *>(d->model)) {
        // Disable the warning in applyDelegateModelAccessChange since the new delegate model
        // access is also explicit.
        d->explicitDelegateModelAccess = false;
        setExplicitDelegateModelAccess(delegateModel);
        return;
    }

    if (delegateModelAccess == QQmlDelegateModel::Qt5ReadWrite) {
        d->explicitDelegateModelAccess = true; // Explicitly set null delegate always works
    } else {
        qmlWarning(this) << "Cannot set a delegateModelAccess on an explicitly provided "
                            "non-DelegateModel";
    }
}

QT_END_NAMESPACE

#include "moc_qquickrepeater_p.cpp"
