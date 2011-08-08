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

#include "qsgrepeater_p.h"
#include "qsgrepeater_p_p.h"
#include "qsgvisualitemmodel_p.h"

#include <private/qdeclarativeglobal_p.h>
#include <private/qdeclarativelistaccessor_p.h>
#include <private/qlistmodelinterface_p.h>

QT_BEGIN_NAMESPACE

QSGRepeaterPrivate::QSGRepeaterPrivate()
: model(0), ownModel(false)
{
}

QSGRepeaterPrivate::~QSGRepeaterPrivate()
{
    if (ownModel)
        delete model;
}

/*!
    \qmlclass Repeater QSGRepeater
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
QSGRepeater::QSGRepeater(QSGItem *parent)
  : QSGItem(*(new QSGRepeaterPrivate), parent)
{
}

QSGRepeater::~QSGRepeater()
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
QVariant QSGRepeater::model() const
{
    Q_D(const QSGRepeater);
    return d->dataSource;
}

void QSGRepeater::setModel(const QVariant &model)
{
    Q_D(QSGRepeater);
    if (d->dataSource == model)
        return;

    clear();
    if (d->model) {
        disconnect(d->model, SIGNAL(itemsInserted(int,int)), this, SLOT(itemsInserted(int,int)));
        disconnect(d->model, SIGNAL(itemsRemoved(int,int)), this, SLOT(itemsRemoved(int,int)));
        disconnect(d->model, SIGNAL(itemsMoved(int,int,int)), this, SLOT(itemsMoved(int,int,int)));
        disconnect(d->model, SIGNAL(modelReset()), this, SLOT(modelReset()));
        /*
        disconnect(d->model, SIGNAL(createdItem(int,QSGItem*)), this, SLOT(createdItem(int,QSGItem*)));
        disconnect(d->model, SIGNAL(destroyingItem(QSGItem*)), this, SLOT(destroyingItem(QSGItem*)));
    */
    }
    d->dataSource = model;
    QObject *object = qvariant_cast<QObject*>(model);
    QSGVisualModel *vim = 0;
    if (object && (vim = qobject_cast<QSGVisualModel *>(object))) {
        if (d->ownModel) {
            delete d->model;
            d->ownModel = false;
        }
        d->model = vim;
    } else {
        if (!d->ownModel) {
            d->model = new QSGVisualDataModel(qmlContext(this));
            d->ownModel = true;
        }
        if (QSGVisualDataModel *dataModel = qobject_cast<QSGVisualDataModel*>(d->model))
            dataModel->setModel(model);
    }
    if (d->model) {
        connect(d->model, SIGNAL(itemsInserted(int,int)), this, SLOT(itemsInserted(int,int)));
        connect(d->model, SIGNAL(itemsRemoved(int,int)), this, SLOT(itemsRemoved(int,int)));
        connect(d->model, SIGNAL(itemsMoved(int,int,int)), this, SLOT(itemsMoved(int,int,int)));
        connect(d->model, SIGNAL(modelReset()), this, SLOT(modelReset()));
        /*
        connect(d->model, SIGNAL(createdItem(int,QSGItem*)), this, SLOT(createdItem(int,QSGItem*)));
        connect(d->model, SIGNAL(destroyingItem(QSGItem*)), this, SLOT(destroyingItem(QSGItem*)));
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
QDeclarativeComponent *QSGRepeater::delegate() const
{
    Q_D(const QSGRepeater);
    if (d->model) {
        if (QSGVisualDataModel *dataModel = qobject_cast<QSGVisualDataModel*>(d->model))
            return dataModel->delegate();
    }

    return 0;
}

void QSGRepeater::setDelegate(QDeclarativeComponent *delegate)
{
    Q_D(QSGRepeater);
    if (QSGVisualDataModel *dataModel = qobject_cast<QSGVisualDataModel*>(d->model))
       if (delegate == dataModel->delegate())
           return;

    if (!d->ownModel) {
        d->model = new QSGVisualDataModel(qmlContext(this));
        d->ownModel = true;
    }
    if (QSGVisualDataModel *dataModel = qobject_cast<QSGVisualDataModel*>(d->model)) {
        dataModel->setDelegate(delegate);
        regenerate();
        emit delegateChanged();
    }
}

/*!
    \qmlproperty int QtQuick2::Repeater::count

    This property holds the number of items in the repeater.
*/
int QSGRepeater::count() const
{
    Q_D(const QSGRepeater);
    if (d->model)
        return d->model->count();
    return 0;
}

/*!
    \qmlmethod Item QtQuick2::Repeater::itemAt(index)

    Returns the \l Item that has been created at the given \a index, or \c null
    if no item exists at \a index.
*/
QSGItem *QSGRepeater::itemAt(int index) const
{
    Q_D(const QSGRepeater);
    if (index >= 0 && index < d->deletables.count())
        return d->deletables[index];
    return 0;
}

void QSGRepeater::componentComplete()
{
    QSGItem::componentComplete();
    regenerate();
}

void QSGRepeater::itemChange(ItemChange change, const ItemChangeData &value)
{
    QSGItem::itemChange(change, value);
    if (change == ItemParentHasChanged) {
        regenerate();
    }
}

void QSGRepeater::clear()
{
    Q_D(QSGRepeater);
    bool complete = isComponentComplete();

    if (d->model) {
        while (d->deletables.count() > 0) {
            QSGItem *item = d->deletables.takeLast();
            if (complete)
                emit itemRemoved(d->deletables.count()-1, item);
            d->model->release(item);
        }
    }
    d->deletables.clear();
}

void QSGRepeater::regenerate()
{
    Q_D(QSGRepeater);
    if (!isComponentComplete())
        return;

    clear();

    if (!d->model || !d->model->count() || !d->model->isValid() || !parentItem() || !isComponentComplete())
        return;

    for (int ii = 0; ii < count(); ++ii) {
        QSGItem *item = d->model->item(ii);
        if (item) {
            QDeclarative_setParent_noEvent(item, parentItem());
            item->setParentItem(parentItem());
            item->stackBefore(this);
            d->deletables << item;
            emit itemAdded(ii, item);
        }
    }
}

void QSGRepeater::itemsInserted(int index, int count)
{
    Q_D(QSGRepeater);
    if (!isComponentComplete())
        return;
    for (int i = 0; i < count; ++i) {
        int modelIndex = index + i;
        QSGItem *item = d->model->item(modelIndex);
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
    emit countChanged();
}

void QSGRepeater::itemsRemoved(int index, int count)
{
    Q_D(QSGRepeater);
    if (!isComponentComplete() || count <= 0)
        return;
    while (count--) {
        QSGItem *item = d->deletables.takeAt(index);
        emit itemRemoved(index, item);
        if (item)
            d->model->release(item);
        else
            break;
    }
    emit countChanged();
}

void QSGRepeater::itemsMoved(int from, int to, int count)
{
    Q_D(QSGRepeater);
    if (!isComponentComplete() || count <= 0)
        return;
    if (from + count > d->deletables.count()) {
        regenerate();
        return;
    }
    QList<QSGItem*> removed;
    int removedCount = count;
    while (removedCount--)
        removed << d->deletables.takeAt(from);
    for (int i = 0; i < count; ++i)
        d->deletables.insert(to + i, removed.at(i));
    d->deletables.last()->stackBefore(this);
    for (int i = d->model->count()-1; i > 0; --i) {
        QSGItem *item = d->deletables.at(i-1);
        item->stackBefore(d->deletables.at(i));
    }
}

void QSGRepeater::modelReset()
{
    if (!isComponentComplete())
        return;
    regenerate();
    emit countChanged();
}

QT_END_NAMESPACE
