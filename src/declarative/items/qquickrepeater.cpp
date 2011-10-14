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

#include "qquickrepeater_p.h"
#include "qquickrepeater_p_p.h"
#include "qquickvisualdatamodel_p.h"

#include <private/qdeclarativeglobal_p.h>
#include <private/qdeclarativelistaccessor_p.h>
#include <private/qlistmodelinterface_p.h>
#include <private/qdeclarativechangeset_p.h>

QT_BEGIN_NAMESPACE

QQuickRepeaterPrivate::QQuickRepeaterPrivate()
: model(0), ownModel(false)
{
}

QQuickRepeaterPrivate::~QQuickRepeaterPrivate()
{
    if (ownModel)
        delete model;
}

/*!
    \qmlclass Repeater QQuickRepeater
    \inqmlmodule QtQuick 2
    \ingroup qml-utility-elements
    \inherits Item

    \brief The Repeater element allows you to repeat an Item-based component using a model.

    The Repeater element is used to create a large number of
    similar items. Like other view elements, a Repeater has a \l model and a \l delegate:
    for each entry in the model, the delegate is instantiated
    in a context seeded with data from the model. A Repeater item is usually
    enclosed in a positioner element such as \l Row or \l Column to visually
    position the multiple delegate items created by the Repeater.

    The following Repeater creates three instances of a \l Rectangle item within
    a \l Row:

    \snippet doc/src/snippets/declarative/repeaters/repeater.qml import
    \codeline
    \snippet doc/src/snippets/declarative/repeaters/repeater.qml simple

    \image repeater-simple.png

    A Repeater's \l model can be any of the supported \l {qmlmodels}{data models}.
    Additionally, like delegates for other views, a Repeater delegate can access
    its index within the repeater, as well as the model data relevant to the
    delegate. See the \l delegate property documentation for details.

    Items instantiated by the Repeater are inserted, in order, as
    children of the Repeater's parent.  The insertion starts immediately after
    the repeater's position in its parent stacking list.  This allows
    a Repeater to be used inside a layout. For example, the following Repeater's
    items are stacked between a red rectangle and a blue rectangle:

    \snippet doc/src/snippets/declarative/repeaters/repeater.qml layout

    \image repeater.png


    \note A Repeater item owns all items it instantiates. Removing or dynamically destroying
    an item created by a Repeater results in unpredictable behavior.


    \section2 Considerations when using Repeater

    The Repeater element creates all of its delegate items when the repeater is first
    created. This can be inefficient if there are a large number of delegate items and
    not all of the items are required to be visible at the same time. If this is the case,
    consider using other view elements like ListView (which only creates delegate items
    when they are scrolled into view) or use the \l {Dynamic Object Creation} methods to
    create items as they are required.

    Also, note that Repeater is \l {Item}-based, and can only repeat \l {Item}-derived objects.
    For example, it cannot be used to repeat QtObjects:
    \badcode
    Item {
        //XXX does not work! Can't repeat QtObject as it doesn't derive from Item.
        Repeater {
            model: 10
            QtObject {}
        }
    }
    \endcode
 */

/*!
    \qmlsignal QtQuick2::Repeater::onItemAdded(int index, Item item)

    This handler is called when an item is added to the repeater. The \a index
    parameter holds the index at which the item has been inserted within the
    repeater, and the \a item parameter holds the \l Item that has been added.
*/

/*!
    \qmlsignal QtQuick2::Repeater::onItemRemoved(int index, Item item)

    This handler is called when an item is removed from the repeater. The \a index
    parameter holds the index at which the item was removed from the repeater,
    and the \a item parameter holds the \l Item that was removed.

    Do not keep a reference to \a item if it was created by this repeater, as
    in these cases it will be deleted shortly after the handler is called.
*/
QQuickRepeater::QQuickRepeater(QQuickItem *parent)
  : QQuickItem(*(new QQuickRepeaterPrivate), parent)
{
}

QQuickRepeater::~QQuickRepeater()
{
}

/*!
    \qmlproperty any QtQuick2::Repeater::model

    The model providing data for the repeater.

    This property can be set to any of the supported \l {qmlmodels}{data models}:

    \list
    \o A number that indicates the number of delegates to be created by the repeater
    \o A model (e.g. a ListModel item, or a QAbstractItemModel subclass)
    \o A string list
    \o An object list
    \endlist

    The type of model affects the properties that are exposed to the \l delegate.

    \sa {qmlmodels}{Data Models}
*/
QVariant QQuickRepeater::model() const
{
    Q_D(const QQuickRepeater);
    return d->dataSource;
}

void QQuickRepeater::setModel(const QVariant &model)
{
    Q_D(QQuickRepeater);
    if (d->dataSource == model)
        return;

    clear();
    if (d->model) {
        disconnect(d->model, SIGNAL(modelUpdated(QDeclarativeChangeSet,bool)),
                this, SLOT(modelUpdated(QDeclarativeChangeSet,bool)));
        /*
        disconnect(d->model, SIGNAL(createdItem(int,QQuickItem*)), this, SLOT(createdItem(int,QQuickItem*)));
        disconnect(d->model, SIGNAL(destroyingItem(QQuickItem*)), this, SLOT(destroyingItem(QQuickItem*)));
    */
    }
    d->dataSource = model;
    QObject *object = qvariant_cast<QObject*>(model);
    QQuickVisualModel *vim = 0;
    if (object && (vim = qobject_cast<QQuickVisualModel *>(object))) {
        if (d->ownModel) {
            delete d->model;
            d->ownModel = false;
        }
        d->model = vim;
    } else {
        if (!d->ownModel) {
            d->model = new QQuickVisualDataModel(qmlContext(this));
            d->ownModel = true;
            if (isComponentComplete())
                static_cast<QQuickVisualDataModel *>(d->model)->componentComplete();
        }
        if (QQuickVisualDataModel *dataModel = qobject_cast<QQuickVisualDataModel*>(d->model))
            dataModel->setModel(model);
    }
    if (d->model) {
        connect(d->model, SIGNAL(modelUpdated(QDeclarativeChangeSet,bool)),
                this, SLOT(modelUpdated(QDeclarativeChangeSet,bool)));
        /*
        connect(d->model, SIGNAL(createdItem(int,QQuickItem*)), this, SLOT(createdItem(int,QQuickItem*)));
        connect(d->model, SIGNAL(destroyingItem(QQuickItem*)), this, SLOT(destroyingItem(QQuickItem*)));
        */
        regenerate();
    }
    emit modelChanged();
    emit countChanged();
}

/*!
    \qmlproperty Component QtQuick2::Repeater::delegate
    \default

    The delegate provides a template defining each item instantiated by the repeater.

    Delegates are exposed to a read-only \c index property that indicates the index
    of the delegate within the repeater. For example, the following \l Text delegate
    displays the index of each repeated item:

    \table
    \row
    \o \snippet doc/src/snippets/declarative/repeaters/repeater.qml index
    \o \image repeater-index.png
    \endtable

    If the \l model is a \l{QStringList-based model}{string list} or
    \l{QObjectList-based model}{object list}, the delegate is also exposed to
    a read-only \c modelData property that holds the string or object data. For
    example:

    \table
    \row
    \o \snippet doc/src/snippets/declarative/repeaters/repeater.qml modeldata
    \o \image repeater-modeldata.png
    \endtable

    If the \l model is a model object (such as a \l ListModel) the delegate
    can access all model roles as named properties, in the same way that delegates
    do for view classes like ListView.

    \sa {QML Data Models}
 */
QDeclarativeComponent *QQuickRepeater::delegate() const
{
    Q_D(const QQuickRepeater);
    if (d->model) {
        if (QQuickVisualDataModel *dataModel = qobject_cast<QQuickVisualDataModel*>(d->model))
            return dataModel->delegate();
    }

    return 0;
}

void QQuickRepeater::setDelegate(QDeclarativeComponent *delegate)
{
    Q_D(QQuickRepeater);
    if (QQuickVisualDataModel *dataModel = qobject_cast<QQuickVisualDataModel*>(d->model))
       if (delegate == dataModel->delegate())
           return;

    if (!d->ownModel) {
        d->model = new QQuickVisualDataModel(qmlContext(this));
        d->ownModel = true;
    }
    if (QQuickVisualDataModel *dataModel = qobject_cast<QQuickVisualDataModel*>(d->model)) {
        dataModel->setDelegate(delegate);
        regenerate();
        emit delegateChanged();
    }
}

/*!
    \qmlproperty int QtQuick2::Repeater::count

    This property holds the number of items in the repeater.
*/
int QQuickRepeater::count() const
{
    Q_D(const QQuickRepeater);
    if (d->model)
        return d->model->count();
    return 0;
}

/*!
    \qmlmethod Item QtQuick2::Repeater::itemAt(index)

    Returns the \l Item that has been created at the given \a index, or \c null
    if no item exists at \a index.
*/
QQuickItem *QQuickRepeater::itemAt(int index) const
{
    Q_D(const QQuickRepeater);
    if (index >= 0 && index < d->deletables.count())
        return d->deletables[index];
    return 0;
}

void QQuickRepeater::componentComplete()
{
    Q_D(QQuickRepeater);
    if (d->model && d->ownModel)
        static_cast<QQuickVisualDataModel *>(d->model)->componentComplete();
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
        while (d->deletables.count() > 0) {
            QQuickItem *item = d->deletables.takeLast();
            if (complete)
                emit itemRemoved(d->deletables.count()-1, item);
            d->model->release(item);
        }
    }
    d->deletables.clear();
}

void QQuickRepeater::regenerate()
{
    Q_D(QQuickRepeater);
    if (!isComponentComplete())
        return;

    clear();

    if (!d->model || !d->model->count() || !d->model->isValid() || !parentItem() || !isComponentComplete())
        return;

    for (int ii = 0; ii < count(); ++ii) {
        QQuickItem *item = d->model->item(ii);
        if (item) {
            QDeclarative_setParent_noEvent(item, parentItem());
            item->setParentItem(parentItem());
            item->stackBefore(this);
            d->deletables << item;
            emit itemAdded(ii, item);
        }
    }
}

void QQuickRepeater::modelUpdated(const QDeclarativeChangeSet &changeSet, bool reset)
{
    Q_D(QQuickRepeater);

    if (!isComponentComplete())
        return;

    if (reset) {
        regenerate();
        emit countChanged();
    }

    int difference = 0;
    QHash<int, QList<QPointer<QQuickItem> > > moved;
    foreach (const QDeclarativeChangeSet::Remove &remove, changeSet.removes()) {
        int index = qMin(remove.index, d->deletables.count());
        int count = qMin(remove.index + remove.count, d->deletables.count()) - index;
        if (remove.isMove()) {
            moved.insert(remove.moveId, d->deletables.mid(index, count));
            d->deletables.erase(
                    d->deletables.begin() + index,
                    d->deletables.begin() + index + count);
        } else while (count--) {
            QQuickItem *item = d->deletables.takeAt(index);
            emit itemRemoved(index, item);
            if (item)
                d->model->release(item);
        }

        difference -= remove.count;
    }

    foreach (const QDeclarativeChangeSet::Insert &insert, changeSet.inserts()) {
        int index = qMin(insert.index, d->deletables.count());
        if (insert.isMove()) {
            QList<QPointer<QQuickItem> > items = moved.value(insert.moveId);
            d->deletables = d->deletables.mid(0, index) + items + d->deletables.mid(index);
            QQuickItem *stackBefore = index + items.count() < d->deletables.count()
                    ? d->deletables.at(index + items.count())
                    : this;
            for (int i = index; i < index + items.count(); ++i)
                d->deletables.at(i)->stackBefore(stackBefore);
        } else for (int i = 0; i < insert.count; ++i) {
            int modelIndex = index + i;
            QQuickItem *item = d->model->item(modelIndex);
            if (item) {
                QDeclarative_setParent_noEvent(item, parentItem());
                item->setParentItem(parentItem());
                if (modelIndex < d->deletables.count())
                    item->stackBefore(d->deletables.at(modelIndex));
                else
                    item->stackBefore(this);
                d->deletables.insert(modelIndex, item);
                emit itemAdded(modelIndex, item);
            }
        }
        difference += insert.count;
    }

    if (difference != 0)
        emit countChanged();
}

QT_END_NAMESPACE
