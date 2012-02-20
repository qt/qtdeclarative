/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
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
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qquickitemview_p_p.h"
#include <QtQuick/private/qdeclarativetransition_p.h>

QT_BEGIN_NAMESPACE


FxViewItem::FxViewItem(QQuickItem *i, bool own)
    : item(i), ownItem(own), index(-1), releaseAfterTransition(false)
    , transition(0)
    , nextTransitionType(FxViewItemTransitionManager::NoTransition)
    , isTransitionTarget(false)
    , nextTransitionToSet(false)
{
}

FxViewItem::~FxViewItem()
{
    if (transition)
        transition->m_item = 0;
    delete transition;

    if (ownItem && item) {
        item->setParentItem(0);
        item->deleteLater();
        item = 0;
    }
}

qreal FxViewItem::itemX() const
{
    if (nextTransitionType != FxViewItemTransitionManager::NoTransition)
        return nextTransitionToSet ? nextTransitionTo.x() : item->x();
    else if (transition && transition->isActive())
        return transition->m_toPos.x();
    else
        return item->x();
}

qreal FxViewItem::itemY() const
{
    // If item is transitioning to some pos, return that dest pos.
    // If item was redirected to some new pos before the current transition finished,
    // return that new pos.
    if (nextTransitionType != FxViewItemTransitionManager::NoTransition)
        return nextTransitionToSet ? nextTransitionTo.y() : item->y();
    else if (transition && transition->isActive())
        return transition->m_toPos.y();
    else
        return item->y();
}

void FxViewItem::setVisible(bool visible)
{
    if (!visible && transitionScheduledOrRunning())
        return;
    item->setVisible(visible);
}

void FxViewItem::setNextTransition(FxViewItemTransitionManager::TransitionType type, bool isTargetItem)
{
    // Don't reset nextTransitionToSet - once it is set, it cannot be changed
    // until the animation finishes since the itemX() and itemY() may be used
    // to calculate positions for transitions for other items in the view.
    nextTransitionType = type;
    isTransitionTarget = isTargetItem;
}

bool FxViewItem::transitionScheduledOrRunning() const
{
    return (transition && transition->isActive())
            || nextTransitionType != FxViewItemTransitionManager::NoTransition;
}

bool FxViewItem::prepareTransition(const QRectF &viewBounds)
{
    bool doTransition = false;

    switch (nextTransitionType) {
    case FxViewItemTransitionManager::NoTransition:
    {
        return false;
    }
    case FxViewItemTransitionManager::PopulateTransition:
    {
        return true;
    }
    case FxViewItemTransitionManager::AddTransition:
    case FxViewItemTransitionManager::RemoveTransition:
        // For Add targets, do transition if item is moving into visible area
        // For Remove targets, do transition if item is currently in visible area
        if (isTransitionTarget) {
            doTransition = (nextTransitionType == FxViewItemTransitionManager::AddTransition)
                    ? viewBounds.intersects(QRectF(nextTransitionTo.x(), nextTransitionTo.y(), item->width(), item->height()))
                    : viewBounds.intersects(QRectF(item->x(), item->y(), item->width(), item->height()));
            if (!doTransition)
                item->setPos(nextTransitionTo);
        } else {
            if (viewBounds.intersects(QRectF(item->x(), item->y(), item->width(), item->height()))
                    || viewBounds.intersects(QRectF(nextTransitionTo.x(), nextTransitionTo.y(), item->width(), item->height()))) {
                doTransition = (nextTransitionTo != item->pos());
            } else {
                item->setPos(nextTransitionTo);
            }
        }
        break;
    case FxViewItemTransitionManager::MoveTransition:
        // do transition if moving from or into visible area
        if (nextTransitionTo != item->pos()) {
            doTransition = viewBounds.intersects(QRectF(item->x(), item->y(), item->width(), item->height()))
                    || viewBounds.intersects(QRectF(nextTransitionTo.x(), nextTransitionTo.y(), item->width(), item->height()));
            if (!doTransition)
                item->setPos(nextTransitionTo);
        }
        break;
    }

    if (!doTransition)
        resetTransitionData();
    return doTransition;
}

void FxViewItem::startTransition()
{
    if (nextTransitionType == FxViewItemTransitionManager::NoTransition)
        return;

    if (!transition || transition->m_type != nextTransitionType || transition->m_type != isTransitionTarget) {
        delete transition;
        transition = new FxViewItemTransitionManager;
    }

    // if item is not already moving somewhere, set it to not move anywhere
    // so that removed items do not move to the default (0,0)
    if (!nextTransitionToSet)
        moveTo(item->pos());

    transition->startTransition(this, nextTransitionType, nextTransitionTo, isTransitionTarget);
    nextTransitionType = FxViewItemTransitionManager::NoTransition;
}

void FxViewItem::stopTransition()
{
    if (transition) {
        transition->cancel();
        delete transition;
        transition = 0;
    }
    resetTransitionData();
    finishedTransition();
}

void FxViewItem::finishedTransition()
{
    nextTransitionToSet = false;
    nextTransitionTo = QPointF();

    if (releaseAfterTransition) {
        QQuickItemViewPrivate *vp = static_cast<QQuickItemViewPrivate*>(QObjectPrivate::get(itemView()));
        vp->releasePendingTransition.removeOne(this);
        vp->releaseItem(this);
    }
}

void FxViewItem::resetTransitionData()
{
    nextTransitionType = FxViewItemTransitionManager::NoTransition;
    isTransitionTarget = false;
    nextTransitionTo = QPointF();
    nextTransitionToSet = false;
}

bool FxViewItem::isPendingRemoval() const
{
    if (nextTransitionType == FxViewItemTransitionManager::RemoveTransition)
        return isTransitionTarget;
    if (transition && transition->isActive() && transition->m_type == FxViewItemTransitionManager::RemoveTransition)
        return transition->m_isTarget;
    return false;
}

void FxViewItem::moveTo(const QPointF &pos)
{
    if (transitionScheduledOrRunning()) {
        nextTransitionTo = pos;
        nextTransitionToSet = true;
    } else {
        item->setPos(pos);
    }
}


FxViewItemTransitionManager::FxViewItemTransitionManager()
    : m_active(false), m_item(0), m_type(FxViewItemTransitionManager::NoTransition), m_isTarget(false)
{
}

FxViewItemTransitionManager::~FxViewItemTransitionManager()
{
}

bool FxViewItemTransitionManager::isActive() const
{
    return m_active;
}

void FxViewItemTransitionManager::startTransition(FxViewItem *item, FxViewItemTransitionManager::TransitionType type, const QPointF &to, bool isTargetItem)
{
    if (!item) {
        qWarning("startTransition(): invalid item");
        return;
    }

    QQuickItemViewPrivate *vp = static_cast<QQuickItemViewPrivate*>(QObjectPrivate::get(item->itemView()));

    QDeclarativeTransition *trans = 0;
    switch (type) {
    case NoTransition:
        break;
    case PopulateTransition:
        trans = vp->populateTransition;
        break;
    case AddTransition:
        trans = isTargetItem ? vp->addTransition : vp->addDisplacedTransition;
        break;
    case MoveTransition:
        trans = isTargetItem ? vp->moveTransition : vp->moveDisplacedTransition;
        break;
    case RemoveTransition:
        trans = isTargetItem ? vp->removeTransition : vp->removeDisplacedTransition;
        break;
    }

    if (!trans) {
        qWarning("QQuickItemView: invalid view transition!");
        return;
    }

    m_active = true;
    m_item = item;
    m_toPos = to;
    m_type = type;
    m_isTarget = isTargetItem;

    QQuickViewTransitionAttached *attached =
            static_cast<QQuickViewTransitionAttached*>(qmlAttachedPropertiesObject<QQuickViewTransitionAttached>(trans));
    if (attached) {
        attached->m_index = item->index;
        attached->m_item = item->item;
        attached->m_destination = to;
        switch (type) {
        case NoTransition:
            break;
        case PopulateTransition:
        case AddTransition:
            attached->m_targetIndexes = vp->addTransitionIndexes;
            attached->m_targetItems = vp->addTransitionTargets;
            break;
        case MoveTransition:
            attached->m_targetIndexes = vp->moveTransitionIndexes;
            attached->m_targetItems = vp->moveTransitionTargets;
            break;
        case RemoveTransition:
            attached->m_targetIndexes = vp->removeTransitionIndexes;
            attached->m_targetItems = vp->removeTransitionTargets;
            break;
        }
        emit attached->indexChanged();
        emit attached->itemChanged();
        emit attached->destinationChanged();
        emit attached->targetIndexesChanged();
        emit attached->targetItemsChanged();
    }

    QDeclarativeStateOperation::ActionList actions;
    actions << QDeclarativeAction(item->item, QLatin1String("x"), QVariant(to.x()));
    actions << QDeclarativeAction(item->item, QLatin1String("y"), QVariant(to.y()));

    QDeclarativeTransitionManager::transition(actions, trans, item->item);
}

void FxViewItemTransitionManager::finished()
{
    QDeclarativeTransitionManager::finished();

    m_active = false;

    if (m_item)
        m_item->finishedTransition();
    m_item = 0;
    m_toPos.setX(0);
    m_toPos.setY(0);
    m_type = NoTransition;
    m_isTarget = false;
}


QQuickItemViewChangeSet::QQuickItemViewChangeSet()
    : active(false)
{
    reset();
}

bool QQuickItemViewChangeSet::hasPendingChanges() const
{
    return !pendingChanges.isEmpty();
}

void QQuickItemViewChangeSet::applyChanges(const QDeclarativeChangeSet &changeSet)
{
    pendingChanges.apply(changeSet);

    int moveId = -1;
    int moveOffset = 0;

    foreach (const QDeclarativeChangeSet::Remove &r, changeSet.removes()) {
        itemCount -= r.count;
        if (moveId == -1 && newCurrentIndex >= r.index + r.count) {
            newCurrentIndex -= r.count;
            currentChanged = true;
        } else if (moveId == -1 && newCurrentIndex >= r.index && newCurrentIndex < r.index + r.count) {
            // current item has been removed.
            if (r.isMove()) {
                moveId = r.moveId;
                moveOffset = newCurrentIndex - r.index;
            } else {
                currentRemoved = true;
                newCurrentIndex = -1;
                if (itemCount)
                    newCurrentIndex = qMin(r.index, itemCount - 1);
            }
            currentChanged = true;
        }
    }
    foreach (const QDeclarativeChangeSet::Insert &i, changeSet.inserts()) {
        if (moveId == -1) {
            if (itemCount && newCurrentIndex >= i.index) {
                newCurrentIndex += i.count;
                currentChanged = true;
            } else if (newCurrentIndex < 0) {
                newCurrentIndex = 0;
                currentChanged = true;
            } else if (newCurrentIndex == 0 && !itemCount) {
                // this is the first item, set the initial current index
                currentChanged = true;
            }
        } else if (moveId == i.moveId) {
            newCurrentIndex = i.index + moveOffset;
        }
        itemCount += i.count;
    }
}

void QQuickItemViewChangeSet::prepare(int currentIndex, int count)
{
    if (active)
        return;
    reset();
    active = true;
    itemCount = count;
    newCurrentIndex = currentIndex;
}

void QQuickItemViewChangeSet::reset()
{
    itemCount = 0;
    newCurrentIndex = -1;
    pendingChanges.clear();
    removedItems.clear();
    active = false;
    currentChanged = false;
    currentRemoved = false;
}


QQuickViewTransitionAttached::QQuickViewTransitionAttached(QObject *parent)
    : QObject(parent), m_index(-1), m_item(0)
{
}
/*!
    \qmlclass ViewTransition QQuickViewTransitionAttached
    \inqmlmodule QtQuick 2
    \ingroup qml-view-elements
    \brief The ViewTransition attached property provides details on items under transition in a view.

    With ListView and GridView, it is possible to specify transitions that should be applied whenever
    the items in the view change as a result of modifications to the view's model. They both have the
    following properties that can be set to the appropriate transitions to be run for various
    operations:

    \list
    \o \c add and \c addDisplaced - the transitions to run when items are added to the view
    \o \c remove and \c removeDisplaced - the transitions to run when items are removed from the view
    \o \c move and \c moveDisplaced - the transitions to run when items are moved within the view
       (i.e. as a result of a move operation in the model)
    \o \c populate - the transition to run when a view is created, or when the model changes
    \endlist

    Such view transitions additionally have access to a ViewTransition attached property that
    provides details of the items that are under transition and the operation that triggered the
    transition. Since view transitions are run once per item, these details can be used to customise
    each transition for each individual item.

    The ViewTransition attached property provides the following properties specific to the item to
    which the transition is applied:

    \list
    \o ViewTransition.item - the item that is under transition
    \o ViewTransition.index - the index of this item
    \o ViewTransition.destination - the (x,y) point to which this item is moving for the relevant view operation
    \endlist

    In addition, ViewTransition provides properties specific to the items which are the target
    of the operation that triggered the transition:

    \list
    \o ViewTransition.targetIndexes - the indexes of the target items
    \o ViewTransition.targetItems - the target items themselves
    \endlist

    View transitions can be written without referring to any of the attributes listed
    above. These attributes merely provide extra details that are useful for customising view
    transitions.

    Following is an introduction to view transitions and the ways in which the ViewTransition
    attached property can be used to augment view transitions.


    \section2 View transitions: a simple example

    Here is a basic example of the use of view transitions. The view below specifies transitions for
    the \c add and \c addDisplaced properties, which will be run when items are added to the view:

    \snippet doc/src/snippets/declarative/viewtransitions/viewtransitions-basic.qml 0

    When the space key is pressed, adding an item to the model, the new item will fade in and
    increase in scale over 400 milliseconds as it is added to the view. Also, any item that is
    displaced by the addition of a new item will animate to its new position in the view over
    400 milliseconds, as specified by the \c addDisplaced transition.

    If five items were inserted in succession at index 0, the effect would be this:

    \image viewtransitions-basic.gif

    Notice that the NumberAnimation objects above do not need to specify a \c target to animate
    the appropriate item. Also, the NumberAnimation in the \c addTransition does not need to specify
    the \c to value to move the item to its correct position in the view. This is because the view
    implicitly sets the \c target and \c to values with the correct item and final item position
    values if these properties are not explicitly defined.

    At its simplest, a view transition may just animate an item to its new position following a
    view operation, just as the \c addDisplaced transition does above, or animate some item properties,
    as in the \c add transition above. Additionally, a view transition may make use of the
    ViewTransition attached property to customise animation behavior for different items. Following
    are some examples of how this can be achieved.


    \section2 Using the ViewTransition attached property

    As stated, the various ViewTransition properties provide details specific to the individual item
    being transitioned as well as the operation that triggered the transition. In the animation above,
    five items are inserted in succession at index 0. When the fifth and final insertion takes place,
    adding "Item 4" to the view, the \c add transition is run once (for the inserted item) and the
    \c addDisplaced transition is run four times (once for each of the four existing items in the view).

    At this point, if we examined the \c addDisplaced transition that was run for the bottom displaced
    item ("Item 0"), the ViewTransition property values provided to this transition would be as follows:

    \table
    \header
        \o Property
        \o Value
        \o Explanation
    \row
        \o ViewTransition.item
        \o "Item 0" delegate instance
        \o The "Item 0" \l Rectangle object itself
    \row
        \o ViewTransition.index
        \o \c int value of 4
        \o The index of "Item 0" within the model following the add operation
    \row
        \o ViewTransition.destination
        \o \l point value of (0, 120)
        \o The position that "Item 0" is moving to
    \row
        \o ViewTransition.targetIndexes
        \o \c int array, just contains the integer "0" (zero)
        \o The index of "Item 4", the new item added to the view
    \row
        \o ViewTransition.targetItems
        \o object array, just contains the "Item 4" delegate instance
        \o The "Item 4" \l Rectangle object - the new item added to the view
    \endtable

    The ViewTransition.targetIndexes and ViewTransition.targetItems lists provide the items and
    indexes of all delegate instances that are the targets of the relevant operation. For an add
    operation, these are all the items that are added into the view; for a remove, these are all
    the items removed from the view, and so on. (Note these lists will only contain references to
    items that have been created within the view or its cached items; targets that are not within
    the visible area of the view or within the item cache will not be accessible.)

    So, while the ViewTransition.item, ViewTransition.index and ViewTransition.destination values
    vary for each individual transition that is run, the ViewTransition.targetIndexes and
    ViewTransition.targetItems values are the same for every \c add and \c addDisplaced transition
    that is triggered by a particular add operation.


    \section3 Delaying animations based on index

    Since each view transition is run once for each item affected by the transition, the ViewTransition
    properties can be used within a transition to define custom behavior for each item's transition.
    For example, the ListView in the previous example could use this information to create a ripple-type
    effect on the movement of the displaced items.

    This can be achieved by modifying the \c addDisplaced transition so that it delays the animation of
    each displaced item based on the difference between its index (provided by ViewTransition.index)
    and the first removed index (provided by ViewTransition.targetIndexes):

    \snippet doc/src/snippets/declarative/viewtransitions/viewtransitions-delayedbyindex.qml 0

    Each displaced item delays its animation by an additional 100 milliseconds, producing a subtle
    ripple-type effect when items are displaced by the add, like this:

    \image viewtransitions-delayedbyindex.gif


    \section3 Animating items to intermediate positions

    The ViewTransition.item property gives a reference to the item to which the transition is being
    applied. This can be used to access any of the item's attributes, custom \c property values,
    and so on.

    Below is a modification of the \c addDisplaced transition from the previous example. It adds a
    ParallelAnimation with nested NumberAnimation objects that reference ViewTransition.item to access
    each item's \c x and \c y values at the start of their transitions. This allows each item to
    animate to an intermediate position relative to its starting point for the transition, before
    animating to its final position in the view:

    \snippet doc/src/snippets/declarative/viewtransitions/viewtransitions-intermediatemove.qml 0

    Now, a displaced item will first move to a position of (20, 50) relative to its starting
    position, and then to its final, correct position in the view:

    \image viewtransitions-intermediatemove.gif

    Since the final NumberAnimation does not specify a \c to value, the view implicitly sets this
    value to the item's final position in the view, and so this last animation will move this item
    to the correct place. If the transition requires the final position of the item for some calculation,
    this is accessible through ViewTransition.destination.

    Instead of using multiple NumberAnimations, you could use a PathAnimation to animate an item over
    a curved path. For example, the \c add transition in the previous example could be augmented with
    a PathAnimation as follows: to animate newly added items along a path:

    \snippet doc/src/snippets/declarative/viewtransitions/viewtransitions-pathanim.qml 0

    This animates newly added items along a path. Notice that each path is specified relative to
    each item's final destination point, so that items inserted at different indexes start their
    paths from different positions:

    \image viewtransitions-pathanim.gif


    \section2 Handling interrupted animations

    A view transition may be interrupted at any time if a different view transition needs to be
    applied while the original transition is in progress. For example, say Item A is inserted at index 0
    and undergoes an "add" transition; then, Item B is inserted at index 0 in quick succession before
    Item A's transition has finished. Since Item B is inserted before Item A, it will displace Item
    A, causing the view to interrupt Item A's "add" transition mid-way and start an "addDisplaced"
    transition on Item A instead.

    For simple animations that simply animate an item's movement to its final destination, this
    interruption is unlikely to require additional consideration. However, if a transition changes other
    properties, this interruption may cause unwanted side effects. Consider the first example on this
    page, repeated below for convenience:

    \snippet doc/src/snippets/declarative/viewtransitions/viewtransitions-basic.qml 0

    If multiple items are added in rapid succession, without waiting for a previous transition
    to finish, this is the result:

    \image viewtransitions-interruptedbad.gif

    Each newly added item undergoes an \c add transition, but before the transition can finish,
    another item is added, displacing the previously added item. Because of this, the \c add
    transition on the previously added item is interrupted and an \c addDisplaced transition is
    started on the item instead. Due to the interruption, the \c opacity and \c scale animations
    have not completed, thus producing items with opacity and scale that are below 1.0.

    To fix this, the \c addDisplaced transition should additionally ensure the item properties are
    set to the end values specified in the \c add transition, effectively resetting these values
    whenever an item is displaced. In this case, it means setting the item opacity and scale to 1.0:

    \snippet doc/src/snippets/declarative/viewtransitions/viewtransitions-interruptedgood.qml 0

    Now, when an item's \c add transition is interrupted, its opacity and scale are animated to 1.0
    upon displacement, avoiding the erroneous visual effects from before:

    \image viewtransitions-interruptedgood.gif

    The same principle applies to any combination of view transitions. An added item may be moved
    before its add transition finishes, or a moved item may be removed before its moved transition
    finishes, and so on; so, the rule of thumb is that every transition should handle the same set of
    properties.


    \section2 Restrictions regarding ScriptAction

    When a view transition is initialized, any property bindings that refer to the ViewTransition
    attached property are evaluated in preparation for the transition. Due to the nature of the
    internal construction of a view transition, the attributes of the ViewTransition attached
    property are only valid for the relevant item when the transition is initialized, and may not be
    valid when the transition is actually run.

    Therefore, a ScriptAction within a view transition should not refer to the ViewTransition
    attached property, as it may not refer to the expected values at the time that the ScriptAction
    is actually invoked. Consider the following example:

    \snippet doc/src/snippets/declarative/viewtransitions/viewtransitions-scriptactionbad.qml 0

    When the space key is pressed, three items are moved from index 5 to index 1. For each moved
    item, the \c moveTransition sequence presumably animates the item's color to "yellow", then
    animates it to its final position, then changes the item color back to "lightsteelblue" using a
    ScriptAction. However, when run, the transition does not produce the intended result:

    \image viewtransitions-scriptactionbad.gif

    Only the last moved item is returned to the "lightsteelblue" color; the others remain yellow. This
    is because the ScriptAction is not run until after the transition has already been initialized, by
    which time the ViewTransition.item value has changed to refer to a different item; the item that
    the script had intended to refer to is not the one held by ViewTransition.item at the time the
    ScriptAction is actually invoked.

    In this instance, to avoid this issue, the view could set the property using a PropertyAction
    instead:

    \snippet doc/src/snippets/declarative/viewtransitions/viewtransitions-scriptactiongood.qml 0

    When the transition is initialized, the PropertyAction \c target will be set to the respective
    ViewTransition.item for the transition and will later run with the correct item target as
    expected.
  */

/*!
    \qmlattachedproperty list QtQuick2::ViewTransition::index

    This attached property holds the index of the item that is being
    transitioned.

    Note that if the item is being moved, this property holds the index that
    the item is moving to, not from.
*/

/*!
    \qmlattachedproperty list QtQuick2::ViewTransition::item

    This attached property holds the the item that is being transitioned.

    \warning This item should not be kept and referred to outside of the transition
    as it may become invalid as the view changes.
*/

/*!
    \qmlattachedproperty list QtQuick2::ViewTransition::destination

    This attached property holds the final destination position for the transitioned
    item within the view.

    This property value is a \l point with \c x and \c y properties.
*/

/*!
    \qmlattachedproperty list QtQuick2::ViewTransition::targetIndexes

    This attached property holds a list of the indexes of the items in view
    that are the target of the relevant operation.

    The targets are the items that are the subject of the operation. For
    an add operation, these are the items being added; for a remove, these
    are the items being removed; for a move, these are the items being
    moved.

    For example, if the transition was triggered by an insert operation
    that added two items at index 1 and 2, this targetIndexes list would
    have the value [1,2].

    \note The targetIndexes list only contains the indexes of items that are actually
    in view, or will be in the view once the relevant operation completes.

    \sa QtQuick2::ViewTransition::targetIndexes
*/

/*!
    \qmlattachedproperty list QtQuick2::ViewTransition::targetItems

    This attached property holds the list of items in view that are the
    target of the relevant operation.

    The targets are the items that are the subject of the operation. For
    an add operation, these are the items being added; for a remove, these
    are the items being removed; for a move, these are the items being
    moved.

    For example, if the transition was triggered by an insert operation
    that added two items at index 1 and 2, this targetItems list would
    contain these two items.

    \note The targetItems list only contains items that are actually
    in view, or will be in the view once the relevant operation completes.

    \warning The objects in this list should not be kept and referred to
    outside of the transition as the items may become invalid. The targetItems
    are only valid when the Transition is initially created; this also means
    they should not be used by ScriptAction objects in the Transition, which are
    not evaluated until the transition is run.

    \sa QtQuick2::ViewTransition::targetIndexes
*/
QDeclarativeListProperty<QObject> QQuickViewTransitionAttached::targetItems()
{
    return QDeclarativeListProperty<QObject>(this, m_targetItems);
}

QQuickViewTransitionAttached *QQuickViewTransitionAttached::qmlAttachedProperties(QObject *obj)
{
    return new QQuickViewTransitionAttached(obj);
}


//-----------------------------------

QQuickItemView::QQuickItemView(QQuickFlickablePrivate &dd, QQuickItem *parent)
    : QQuickFlickable(dd, parent)
{
    Q_D(QQuickItemView);
    d->init();
}

QQuickItemView::~QQuickItemView()
{
    Q_D(QQuickItemView);
    d->clear();
    if (d->ownModel)
        delete d->model;
    delete d->header;
    delete d->footer;
}


QQuickItem *QQuickItemView::currentItem() const
{
    Q_D(const QQuickItemView);
    if (!d->currentItem)
        return 0;
    const_cast<QQuickItemViewPrivate*>(d)->applyPendingChanges();
    return d->currentItem->item;
}

QVariant QQuickItemView::model() const
{
    Q_D(const QQuickItemView);
    return d->modelVariant;
}

void QQuickItemView::setModel(const QVariant &model)
{
    Q_D(QQuickItemView);
    if (d->modelVariant == model)
        return;
    if (d->model) {
        disconnect(d->model, SIGNAL(modelUpdated(QDeclarativeChangeSet,bool)),
                this, SLOT(modelUpdated(QDeclarativeChangeSet,bool)));
        disconnect(d->model, SIGNAL(initItem(int,QQuickItem*)), this, SLOT(initItem(int,QQuickItem*)));
        disconnect(d->model, SIGNAL(createdItem(int,QQuickItem*)), this, SLOT(createdItem(int,QQuickItem*)));
        disconnect(d->model, SIGNAL(destroyingItem(QQuickItem*)), this, SLOT(destroyingItem(QQuickItem*)));
    }

    QQuickVisualModel *oldModel = d->model;

    d->clear();
    d->setPosition(d->contentStartOffset());
    d->model = 0;
    d->modelVariant = model;

    QObject *object = qvariant_cast<QObject*>(model);
    QQuickVisualModel *vim = 0;
    if (object && (vim = qobject_cast<QQuickVisualModel *>(object))) {
        if (d->ownModel) {
            delete oldModel;
            d->ownModel = false;
        }
        d->model = vim;
    } else {
        if (!d->ownModel) {
            d->model = new QQuickVisualDataModel(qmlContext(this), this);
            d->ownModel = true;
            if (isComponentComplete())
                static_cast<QQuickVisualDataModel *>(d->model.data())->componentComplete();
        } else {
            d->model = oldModel;
        }
        if (QQuickVisualDataModel *dataModel = qobject_cast<QQuickVisualDataModel*>(d->model))
            dataModel->setModel(model);
    }

    if (d->model) {
        d->bufferMode = QQuickItemViewPrivate::BufferBefore | QQuickItemViewPrivate::BufferAfter;
        connect(d->model, SIGNAL(createdItem(int,QQuickItem*)), this, SLOT(createdItem(int,QQuickItem*)));
        connect(d->model, SIGNAL(initItem(int,QQuickItem*)), this, SLOT(initItem(int,QQuickItem*)));
        connect(d->model, SIGNAL(destroyingItem(QQuickItem*)), this, SLOT(destroyingItem(QQuickItem*)));
        if (isComponentComplete()) {
            updateSections();
            d->refill();
            if ((d->currentIndex >= d->model->count() || d->currentIndex < 0) && !d->currentIndexCleared) {
                setCurrentIndex(0);
            } else {
                d->moveReason = QQuickItemViewPrivate::SetIndex;
                d->updateCurrent(d->currentIndex);
                if (d->highlight && d->currentItem) {
                    if (d->autoHighlight)
                        d->resetHighlightPosition();
                    d->updateTrackedItem();
                }
                d->moveReason = QQuickItemViewPrivate::Other;
            }
            d->updateViewport();

            if (d->populateTransition) {
                d->forceLayout = true;
                d->usePopulateTransition = true;
                polish();
            }
        }
        connect(d->model, SIGNAL(modelUpdated(QDeclarativeChangeSet,bool)),
                this, SLOT(modelUpdated(QDeclarativeChangeSet,bool)));
        emit countChanged();
    }
    emit modelChanged();
}

QDeclarativeComponent *QQuickItemView::delegate() const
{
    Q_D(const QQuickItemView);
    if (d->model) {
        if (QQuickVisualDataModel *dataModel = qobject_cast<QQuickVisualDataModel*>(d->model))
            return dataModel->delegate();
    }

    return 0;
}

void QQuickItemView::setDelegate(QDeclarativeComponent *delegate)
{
    Q_D(QQuickItemView);
    if (delegate == this->delegate())
        return;
    if (!d->ownModel) {
        d->model = new QQuickVisualDataModel(qmlContext(this));
        d->ownModel = true;
    }
    if (QQuickVisualDataModel *dataModel = qobject_cast<QQuickVisualDataModel*>(d->model)) {
        int oldCount = dataModel->count();
        dataModel->setDelegate(delegate);
        if (isComponentComplete()) {
            for (int i = 0; i < d->visibleItems.count(); ++i)
                d->releaseItem(d->visibleItems.at(i));
            d->visibleItems.clear();
            d->releaseItem(d->currentItem);
            d->currentItem = 0;
            updateSections();
            d->refill();
            d->moveReason = QQuickItemViewPrivate::SetIndex;
            d->updateCurrent(d->currentIndex);
            if (d->highlight && d->currentItem) {
                if (d->autoHighlight)
                    d->resetHighlightPosition();
                d->updateTrackedItem();
            }
            d->moveReason = QQuickItemViewPrivate::Other;
            d->updateViewport();
        }
        if (oldCount != dataModel->count())
            emit countChanged();
    }
    emit delegateChanged();
}


int QQuickItemView::count() const
{
    Q_D(const QQuickItemView);
    if (!d->model)
        return 0;
    const_cast<QQuickItemViewPrivate*>(d)->applyPendingChanges();
    return d->model->count();
}

int QQuickItemView::currentIndex() const
{
    Q_D(const QQuickItemView);
    const_cast<QQuickItemViewPrivate*>(d)->applyPendingChanges();
    return d->currentIndex;
}

void QQuickItemView::setCurrentIndex(int index)
{
    Q_D(QQuickItemView);
    if (d->requestedIndex >= 0 && !d->requestedAsync)  // currently creating item
        return;
    d->currentIndexCleared = (index == -1);

    d->applyPendingChanges();
    if (index == d->currentIndex)
        return;
    if (isComponentComplete() && d->isValid()) {
        d->moveReason = QQuickItemViewPrivate::SetIndex;
        d->updateCurrent(index);
    } else if (d->currentIndex != index) {
        d->currentIndex = index;
        emit currentIndexChanged();
    }
}


bool QQuickItemView::isWrapEnabled() const
{
    Q_D(const QQuickItemView);
    return d->wrap;
}

void QQuickItemView::setWrapEnabled(bool wrap)
{
    Q_D(QQuickItemView);
    if (d->wrap == wrap)
        return;
    d->wrap = wrap;
    emit keyNavigationWrapsChanged();
}

int QQuickItemView::cacheBuffer() const
{
    Q_D(const QQuickItemView);
    return d->buffer;
}

void QQuickItemView::setCacheBuffer(int b)
{
    Q_D(QQuickItemView);
    if (d->buffer != b) {
        d->buffer = b;
        if (isComponentComplete()) {
            d->bufferMode = QQuickItemViewPrivate::BufferBefore | QQuickItemViewPrivate::BufferAfter;
            d->refill();
        }
        emit cacheBufferChanged();
    }
}


Qt::LayoutDirection QQuickItemView::layoutDirection() const
{
    Q_D(const QQuickItemView);
    return d->layoutDirection;
}

void QQuickItemView::setLayoutDirection(Qt::LayoutDirection layoutDirection)
{
    Q_D(QQuickItemView);
    if (d->layoutDirection != layoutDirection) {
        d->layoutDirection = layoutDirection;
        d->regenerate();
        emit layoutDirectionChanged();
        emit effectiveLayoutDirectionChanged();
    }
}

Qt::LayoutDirection QQuickItemView::effectiveLayoutDirection() const
{
    Q_D(const QQuickItemView);
    if (d->effectiveLayoutMirror)
        return d->layoutDirection == Qt::RightToLeft ? Qt::LeftToRight : Qt::RightToLeft;
    else
        return d->layoutDirection;
}


QDeclarativeComponent *QQuickItemView::header() const
{
    Q_D(const QQuickItemView);
    return d->headerComponent;
}

QQuickItem *QQuickItemView::headerItem() const
{
    Q_D(const QQuickItemView);
    const_cast<QQuickItemViewPrivate*>(d)->applyPendingChanges();
    return d->header ? d->header->item : 0;
}

void QQuickItemView::setHeader(QDeclarativeComponent *headerComponent)
{
    Q_D(QQuickItemView);
    if (d->headerComponent != headerComponent) {
        d->applyPendingChanges();
        delete d->header;
        d->header = 0;
        d->headerComponent = headerComponent;

        d->markExtentsDirty();

        if (isComponentComplete()) {
            d->updateHeader();
            d->updateFooter();
            d->updateViewport();
            d->fixupPosition();
        } else {
            emit headerItemChanged();
        }
        emit headerChanged();
    }
}

QDeclarativeComponent *QQuickItemView::footer() const
{
    Q_D(const QQuickItemView);
    return d->footerComponent;
}

QQuickItem *QQuickItemView::footerItem() const
{
    Q_D(const QQuickItemView);
    const_cast<QQuickItemViewPrivate*>(d)->applyPendingChanges();
    return d->footer ? d->footer->item : 0;
}

void QQuickItemView::setFooter(QDeclarativeComponent *footerComponent)
{
    Q_D(QQuickItemView);
    if (d->footerComponent != footerComponent) {
        d->applyPendingChanges();
        delete d->footer;
        d->footer = 0;
        d->footerComponent = footerComponent;

        if (isComponentComplete()) {
            d->updateFooter();
            d->updateViewport();
            d->fixupPosition();
        } else {
            emit footerItemChanged();
        }
        emit footerChanged();
    }
}

QDeclarativeComponent *QQuickItemView::highlight() const
{
    Q_D(const QQuickItemView);
    const_cast<QQuickItemViewPrivate*>(d)->applyPendingChanges();
    return d->highlightComponent;
}

void QQuickItemView::setHighlight(QDeclarativeComponent *highlightComponent)
{
    Q_D(QQuickItemView);
    if (highlightComponent != d->highlightComponent) {
        d->applyPendingChanges();
        d->highlightComponent = highlightComponent;
        d->createHighlight();
        if (d->currentItem)
            d->updateHighlight();
        emit highlightChanged();
    }
}

QQuickItem *QQuickItemView::highlightItem() const
{
    Q_D(const QQuickItemView);
    const_cast<QQuickItemViewPrivate*>(d)->applyPendingChanges();
    return d->highlight ? d->highlight->item : 0;
}

bool QQuickItemView::highlightFollowsCurrentItem() const
{
    Q_D(const QQuickItemView);
    return d->autoHighlight;
}

void QQuickItemView::setHighlightFollowsCurrentItem(bool autoHighlight)
{
    Q_D(QQuickItemView);
    if (d->autoHighlight != autoHighlight) {
        d->autoHighlight = autoHighlight;
        if (autoHighlight)
            d->updateHighlight();
        emit highlightFollowsCurrentItemChanged();
    }
}

QQuickItemView::HighlightRangeMode QQuickItemView::highlightRangeMode() const
{
    Q_D(const QQuickItemView);
    return static_cast<QQuickItemView::HighlightRangeMode>(d->highlightRange);
}

void QQuickItemView::setHighlightRangeMode(HighlightRangeMode mode)
{
    Q_D(QQuickItemView);
    if (d->highlightRange == mode)
        return;
    d->highlightRange = mode;
    d->haveHighlightRange = d->highlightRange != NoHighlightRange && d->highlightRangeStart <= d->highlightRangeEnd;
    emit highlightRangeModeChanged();
}

//###Possibly rename these properties, since they are very useful even without a highlight?
qreal QQuickItemView::preferredHighlightBegin() const
{
    Q_D(const QQuickItemView);
    return d->highlightRangeStart;
}

void QQuickItemView::setPreferredHighlightBegin(qreal start)
{
    Q_D(QQuickItemView);
    d->highlightRangeStartValid = true;
    if (d->highlightRangeStart == start)
        return;
    d->highlightRangeStart = start;
    d->haveHighlightRange = d->highlightRange != NoHighlightRange && d->highlightRangeStart <= d->highlightRangeEnd;
    emit preferredHighlightBeginChanged();
}

void QQuickItemView::resetPreferredHighlightBegin()
{
    Q_D(QQuickItemView);
    d->highlightRangeStartValid = false;
    if (d->highlightRangeStart == 0)
        return;
    d->highlightRangeStart = 0;
    emit preferredHighlightBeginChanged();
}

qreal QQuickItemView::preferredHighlightEnd() const
{
    Q_D(const QQuickItemView);
    return d->highlightRangeEnd;
}

void QQuickItemView::setPreferredHighlightEnd(qreal end)
{
    Q_D(QQuickItemView);
    d->highlightRangeEndValid = true;
    if (d->highlightRangeEnd == end)
        return;
    d->highlightRangeEnd = end;
    d->haveHighlightRange = d->highlightRange != NoHighlightRange && d->highlightRangeStart <= d->highlightRangeEnd;
    emit preferredHighlightEndChanged();
}

void QQuickItemView::resetPreferredHighlightEnd()
{
    Q_D(QQuickItemView);
    d->highlightRangeEndValid = false;
    if (d->highlightRangeEnd == 0)
        return;
    d->highlightRangeEnd = 0;
    emit preferredHighlightEndChanged();
}

int QQuickItemView::highlightMoveDuration() const
{
    Q_D(const QQuickItemView);
    return d->highlightMoveDuration;
}

void QQuickItemView::setHighlightMoveDuration(int duration)
{
    Q_D(QQuickItemView);
    if (d->highlightMoveDuration != duration) {
        d->highlightMoveDuration = duration;
        emit highlightMoveDurationChanged();
    }
}

QDeclarativeTransition *QQuickItemView::populateTransition() const
{
    Q_D(const QQuickItemView);
    return d->populateTransition;
}

void QQuickItemView::setPopulateTransition(QDeclarativeTransition *transition)
{
    Q_D(QQuickItemView);
    if (d->populateTransition != transition) {
        d->populateTransition = transition;
        emit populateTransitionChanged();
    }
}

QDeclarativeTransition *QQuickItemView::addTransition() const
{
    Q_D(const QQuickItemView);
    return d->addTransition;
}

void QQuickItemView::setAddTransition(QDeclarativeTransition *transition)
{
    Q_D(QQuickItemView);
    if (d->addTransition != transition) {
        d->addTransition = transition;
        emit addTransitionChanged();
    }
}

QDeclarativeTransition *QQuickItemView::addDisplacedTransition() const
{
    Q_D(const QQuickItemView);
    return d->addDisplacedTransition;
}

void QQuickItemView::setAddDisplacedTransition(QDeclarativeTransition *transition)
{
    Q_D(QQuickItemView);
    if (d->addDisplacedTransition != transition) {
        d->addDisplacedTransition = transition;
        emit addDisplacedTransitionChanged();
    }
}

QDeclarativeTransition *QQuickItemView::moveTransition() const
{
    Q_D(const QQuickItemView);
    return d->moveTransition;
}

void QQuickItemView::setMoveTransition(QDeclarativeTransition *transition)
{
    Q_D(QQuickItemView);
    if (d->moveTransition != transition) {
        d->moveTransition = transition;
        emit moveTransitionChanged();
    }
}

QDeclarativeTransition *QQuickItemView::moveDisplacedTransition() const
{
    Q_D(const QQuickItemView);
    return d->moveDisplacedTransition;
}

void QQuickItemView::setMoveDisplacedTransition(QDeclarativeTransition *transition)
{
    Q_D(QQuickItemView);
    if (d->moveDisplacedTransition != transition) {
        d->moveDisplacedTransition = transition;
        emit moveDisplacedTransitionChanged();
    }
}

QDeclarativeTransition *QQuickItemView::removeTransition() const
{
    Q_D(const QQuickItemView);
    return d->removeTransition;
}

void QQuickItemView::setRemoveTransition(QDeclarativeTransition *transition)
{
    Q_D(QQuickItemView);
    if (d->removeTransition != transition) {
        d->removeTransition = transition;
        emit removeTransitionChanged();
    }
}

QDeclarativeTransition *QQuickItemView::removeDisplacedTransition() const
{
    Q_D(const QQuickItemView);
    return d->removeDisplacedTransition;
}

void QQuickItemView::setRemoveDisplacedTransition(QDeclarativeTransition *transition)
{
    Q_D(QQuickItemView);
    if (d->removeDisplacedTransition != transition) {
        d->removeDisplacedTransition = transition;
        emit removeDisplacedTransitionChanged();
    }
}

void QQuickItemViewPrivate::positionViewAtIndex(int index, int mode)
{
    Q_Q(QQuickItemView);
    if (!isValid())
        return;
    if (mode < QQuickItemView::Beginning || mode > QQuickItemView::Contain)
        return;

    applyPendingChanges();
    int idx = qMax(qMin(index, model->count()-1), 0);

    qreal pos = isContentFlowReversed() ? -position() - size() : position();
    FxViewItem *item = visibleItem(idx);
    qreal maxExtent;
    if (layoutOrientation() == Qt::Vertical)
        maxExtent = -q->maxYExtent();
    else
        maxExtent = isContentFlowReversed() ? q->minXExtent()-size(): -q->maxXExtent();
    if (!item) {
        int itemPos = positionAt(idx);
        changedVisibleIndex(idx);
        // save the currently visible items in case any of them end up visible again
        QList<FxViewItem *> oldVisible = visibleItems;
        visibleItems.clear();
        setPosition(qMin(qreal(itemPos), maxExtent));
        // now release the reference to all the old visible items.
        for (int i = 0; i < oldVisible.count(); ++i)
            releaseItem(oldVisible.at(i));
        item = visibleItem(idx);
    }
    if (item) {
        const qreal itemPos = item->position();
        switch (mode) {
        case QQuickItemView::Beginning:
            pos = itemPos;
            if (index < 0 && header)
                pos -= headerSize();
            break;
        case QQuickItemView::Center:
            pos = itemPos - (size() - item->size())/2;
            break;
        case QQuickItemView::End:
            pos = itemPos - size() + item->size();
            if (index >= model->count() && footer)
                pos += footerSize();
            break;
        case QQuickItemView::Visible:
            if (itemPos > pos + size())
                pos = itemPos - size() + item->size();
            else if (item->endPosition() <= pos)
                pos = itemPos;
            break;
        case QQuickItemView::Contain:
            if (item->endPosition() >= pos + size())
                pos = itemPos - size() + item->size();
            if (itemPos < pos)
                pos = itemPos;
        }
        pos = qMin(pos, maxExtent);
        qreal minExtent;
        if (layoutOrientation() == Qt::Vertical)
            minExtent = -q->minYExtent();
        else
            minExtent = isContentFlowReversed() ? q->maxXExtent()-size(): -q->minXExtent();
        pos = qMax(pos, minExtent);
        moveReason = QQuickItemViewPrivate::Other;
        q->cancelFlick();
        setPosition(pos);

        if (highlight) {
            if (autoHighlight)
                resetHighlightPosition();
            updateHighlight();
        }
    }
    fixupPosition();
}

void QQuickItemView::positionViewAtIndex(int index, int mode)
{
    Q_D(QQuickItemView);
    if (!d->isValid() || index < 0 || index >= d->model->count())
        return;
    d->positionViewAtIndex(index, mode);
}


void QQuickItemView::positionViewAtBeginning()
{
    Q_D(QQuickItemView);
    if (!d->isValid())
        return;
    d->positionViewAtIndex(-1, Beginning);
}

void QQuickItemView::positionViewAtEnd()
{
    Q_D(QQuickItemView);
    if (!d->isValid())
        return;
    d->positionViewAtIndex(d->model->count(), End);
}

int QQuickItemView::indexAt(qreal x, qreal y) const
{
    Q_D(const QQuickItemView);
    for (int i = 0; i < d->visibleItems.count(); ++i) {
        const FxViewItem *item = d->visibleItems.at(i);
        if (item->contains(x, y))
            return item->index;
    }

    return -1;
}

QQuickItem *QQuickItemView::itemAt(qreal x, qreal y) const
{
    Q_D(const QQuickItemView);
    for (int i = 0; i < d->visibleItems.count(); ++i) {
        const FxViewItem *item = d->visibleItems.at(i);
        if (item->contains(x, y))
            return item->item;
    }

    return 0;
}

void QQuickItemViewPrivate::applyPendingChanges()
{
    Q_Q(QQuickItemView);
    if (q->isComponentComplete() && currentChanges.hasPendingChanges())
        layout();
}

bool QQuickItemViewPrivate::canTransition(FxViewItemTransitionManager::TransitionType type, bool asTarget) const
{
    switch (type) {
    case FxViewItemTransitionManager::NoTransition:
        break;
    case FxViewItemTransitionManager::PopulateTransition:
        return usePopulateTransition
                && populateTransition && populateTransition->enabled();
    case FxViewItemTransitionManager::AddTransition:
        if (asTarget)
            return addTransition && addTransition->enabled();
        else
            return addDisplacedTransition && addDisplacedTransition->enabled();
    case FxViewItemTransitionManager::MoveTransition:
        if (asTarget)
            return moveTransition && moveTransition->enabled();
        else
            return moveDisplacedTransition && moveDisplacedTransition->enabled();
    case FxViewItemTransitionManager::RemoveTransition:
        if (asTarget)
            return removeTransition && removeTransition->enabled();
        else
            return removeDisplacedTransition && removeDisplacedTransition->enabled();
    }
    return false;
}

bool QQuickItemViewPrivate::hasItemTransitions() const
{
    return canTransition(FxViewItemTransitionManager::PopulateTransition, true)
            || canTransition(FxViewItemTransitionManager::AddTransition, true)
            || canTransition(FxViewItemTransitionManager::AddTransition, false)
            || canTransition(FxViewItemTransitionManager::MoveTransition, true)
            || canTransition(FxViewItemTransitionManager::MoveTransition, false)
            || canTransition(FxViewItemTransitionManager::RemoveTransition, true)
            || canTransition(FxViewItemTransitionManager::RemoveTransition, false);
}

void QQuickItemViewPrivate::transitionNextReposition(FxViewItem *item, FxViewItemTransitionManager::TransitionType type, bool isTarget)
{
    bool matchedTransition = false;
    if (type == FxViewItemTransitionManager::AddTransition) {
        // don't run add transitions for added items while populating
        matchedTransition = !usePopulateTransition && canTransition(type, isTarget);
    } else {
        matchedTransition = canTransition(type, isTarget);
    }

    if (matchedTransition) {
        item->setNextTransition(type, isTarget);
    } else {
        // the requested transition type is not valid, but the item is scheduled/in another
        // transition, so cancel it to allow the item to move directly to the correct pos
        if (item->transitionScheduledOrRunning())
            item->stopTransition();
    }
}

int QQuickItemViewPrivate::findMoveKeyIndex(QDeclarativeChangeSet::MoveKey key, const QVector<QDeclarativeChangeSet::Remove> &changes) const
{
    for (int i=0; i<changes.count(); i++) {
        for (int j=changes[i].index; j<changes[i].index + changes[i].count; j++) {
            if (changes[i].moveKey(j) == key)
                return j;
        }
    }
    return -1;
}

// for debugging only
void QQuickItemViewPrivate::checkVisible() const
{
    int skip = 0;
    for (int i = 0; i < visibleItems.count(); ++i) {
        FxViewItem *item = visibleItems.at(i);
        if (item->index == -1) {
            ++skip;
        } else if (item->index != visibleIndex + i - skip) {
            qFatal("index %d %d %d", visibleIndex, i, item->index);
        }
    }
}

// for debugging only
void QQuickItemViewPrivate::showVisibleItems() const
{
    qDebug() << "Visible items:";
    for (int i = 0; i < visibleItems.count(); ++i) {
        qDebug() << "\t" << visibleItems[i]->index
                 << visibleItems[i]->item->objectName()
                 << visibleItems[i]->position();
    }
}

void QQuickItemViewPrivate::itemGeometryChanged(QQuickItem *item, const QRectF &newGeometry, const QRectF &oldGeometry)
{
    Q_Q(QQuickItemView);
    QQuickFlickablePrivate::itemGeometryChanged(item, newGeometry, oldGeometry);
    if (!q->isComponentComplete())
        return;

    if (header && header->item == item) {
        updateHeader();
        markExtentsDirty();
        if (!q->isMoving() && !q->isFlicking())
            fixupPosition();
    } else if (footer && footer->item == item) {
        updateFooter();
        markExtentsDirty();
        if (!q->isMoving() && !q->isFlicking())
            fixupPosition();
    }

    if (currentItem && currentItem->item == item) {
        // don't allow item movement transitions to trigger a re-layout and
        // start new transitions
        bool prevDisableLayout = disableLayout;
        if (!disableLayout) {
            FxViewItem *actualItem = hasItemTransitions() ? visibleItem(currentIndex) : 0;
            if (actualItem && actualItem->transition && actualItem->transition->isRunning())
                disableLayout = true;
        }
        updateHighlight();
        disableLayout = prevDisableLayout;
    }

    if (trackedItem && trackedItem->item == item)
        q->trackedPositionChanged();
}

void QQuickItemView::destroyRemoved()
{
    Q_D(QQuickItemView);
    for (QList<FxViewItem*>::Iterator it = d->visibleItems.begin();
            it != d->visibleItems.end();) {
        FxViewItem *item = *it;
        if (item->index == -1 && item->attached->delayRemove() == false) {
            if (d->canTransition(FxViewItemTransitionManager::RemoveTransition, true)) {
                // don't remove from visibleItems until next layout()
                d->runDelayedRemoveTransition = true;
                QObject::disconnect(item->attached, SIGNAL(delayRemoveChanged()), this, SLOT(destroyRemoved()));
                ++it;
            } else {
                d->releaseItem(item);
                it = d->visibleItems.erase(it);
            }
        } else {
            ++it;
        }
    }

    // Correct the positioning of the items
    d->updateSections();
    d->forceLayout = true;
    polish();
}

void QQuickItemView::modelUpdated(const QDeclarativeChangeSet &changeSet, bool reset)
{
    Q_D(QQuickItemView);
    if (reset) {
        d->usePopulateTransition = true;
        d->moveReason = QQuickItemViewPrivate::SetIndex;
        d->regenerate();
        if (d->highlight && d->currentItem) {
            if (d->autoHighlight)
                d->resetHighlightPosition();
            d->updateTrackedItem();
        }
        d->moveReason = QQuickItemViewPrivate::Other;
        emit countChanged();
        if (d->populateTransition) {
            d->forceLayout = true;
            polish();
        }
    } else {
        d->currentChanges.prepare(d->currentIndex, d->itemCount);
        d->currentChanges.applyChanges(changeSet);
        polish();
    }
}

void QQuickItemView::animStopped()
{
    Q_D(QQuickItemView);
    d->bufferMode = QQuickItemViewPrivate::BufferBefore | QQuickItemViewPrivate::BufferAfter;
    d->refill();
    if (d->haveHighlightRange && d->highlightRange == QQuickItemView::StrictlyEnforceRange)
        d->updateHighlight();
}


void QQuickItemView::trackedPositionChanged()
{
    Q_D(QQuickItemView);
    if (!d->trackedItem || !d->currentItem)
        return;
    if (d->moveReason == QQuickItemViewPrivate::SetIndex) {
        qreal trackedPos = d->trackedItem->position();
        qreal trackedSize = d->trackedItem->size();
        qreal viewPos = d->isContentFlowReversed() ? -d->position()-d->size() : d->position();
        qreal pos = viewPos;
        if (d->haveHighlightRange) {
            if (trackedPos > pos + d->highlightRangeEnd - trackedSize)
                pos = trackedPos - d->highlightRangeEnd + trackedSize;
            if (trackedPos < pos + d->highlightRangeStart)
                pos = trackedPos - d->highlightRangeStart;
            if (d->highlightRange != StrictlyEnforceRange) {
                if (pos > d->endPosition() - d->size())
                    pos = d->endPosition() - d->size();
                if (pos < d->startPosition())
                    pos = d->startPosition();
            }
        } else {
            if (d->trackedItem != d->currentItem) {
                // also make section header visible
                trackedPos -= d->currentItem->sectionSize();
                trackedSize += d->currentItem->sectionSize();
            }
            qreal trackedEndPos = d->trackedItem->endPosition();
            qreal toItemPos = d->currentItem->position();
            qreal toItemEndPos = d->currentItem->endPosition();
            if (d->showHeaderForIndex(d->currentIndex)) {
                qreal startOffset = -d->contentStartOffset();
                trackedPos -= startOffset;
                trackedEndPos -= startOffset;
                toItemPos -= startOffset;
                toItemEndPos -= startOffset;
            } else if (d->showFooterForIndex(d->currentIndex)) {
                qreal endOffset = d->footerSize();
                if (d->layoutOrientation() == Qt::Vertical)
                    endOffset += d->vData.endMargin;
                else if (d->isContentFlowReversed())
                    endOffset += d->hData.endMargin;
                else
                    endOffset += d->hData.startMargin;
                trackedPos += endOffset;
                trackedEndPos += endOffset;
                toItemPos += endOffset;
                toItemEndPos += endOffset;
            }

            if (trackedEndPos >= viewPos + d->size()
                && toItemEndPos >= viewPos + d->size()) {
                if (trackedEndPos <= toItemEndPos) {
                    pos = trackedEndPos - d->size();
                    if (trackedSize > d->size())
                        pos = trackedPos;
                } else {
                    pos = toItemEndPos - d->size();
                    if (d->currentItem->size() > d->size())
                        pos = d->currentItem->position();
                }
            }
            if (trackedPos < pos && toItemPos < pos)
                pos = qMax(trackedPos, toItemPos);
        }
        if (viewPos != pos) {
            cancelFlick();
            d->calcVelocity = true;
            d->setPosition(pos);
            d->calcVelocity = false;
        }
    }
}

void QQuickItemView::geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    Q_D(QQuickItemView);
    d->markExtentsDirty();
    if (isComponentComplete() && d->isValid()) {
        d->forceLayout = true;
        polish();
    }
    QQuickFlickable::geometryChanged(newGeometry, oldGeometry);
}


qreal QQuickItemView::minYExtent() const
{
    Q_D(const QQuickItemView);
    if (d->layoutOrientation() == Qt::Horizontal)
        return QQuickFlickable::minYExtent();

    if (d->vData.minExtentDirty) {
        d->minExtent = d->vData.startMargin-d->startPosition();
        if (d->header)
            d->minExtent += d->headerSize();
        if (d->haveHighlightRange && d->highlightRange == StrictlyEnforceRange) {
            d->minExtent += d->highlightRangeStart;
            if (d->visibleItem(0))
                d->minExtent -= d->visibleItem(0)->sectionSize();
            d->minExtent = qMax(d->minExtent, -(d->endPositionAt(0) - d->highlightRangeEnd));
        }
        d->vData.minExtentDirty = false;
    }

    return d->minExtent;
}

qreal QQuickItemView::maxYExtent() const
{
    Q_D(const QQuickItemView);
    if (d->layoutOrientation() == Qt::Horizontal)
        return height();

    if (d->vData.maxExtentDirty) {
        if (!d->model || !d->model->count()) {
            d->maxExtent = d->header ? -d->headerSize() : 0;
            d->maxExtent += height();
        } else if (d->haveHighlightRange && d->highlightRange == StrictlyEnforceRange) {
            d->maxExtent = -(d->positionAt(d->model->count()-1) - d->highlightRangeStart);
            if (d->highlightRangeEnd != d->highlightRangeStart)
                d->maxExtent = qMin(d->maxExtent, -(d->endPosition() - d->highlightRangeEnd));
        } else {
            d->maxExtent = -(d->endPosition() - height());
        }

        if (d->footer)
            d->maxExtent -= d->footerSize();
        d->maxExtent -= d->vData.endMargin;
        qreal minY = minYExtent();
        if (d->maxExtent > minY)
            d->maxExtent = minY;
        d->vData.maxExtentDirty = false;
    }
    return d->maxExtent;
}

qreal QQuickItemView::minXExtent() const
{
    Q_D(const QQuickItemView);
    if (d->layoutOrientation() == Qt::Vertical)
        return QQuickFlickable::minXExtent();

    if (d->hData.minExtentDirty) {
        d->minExtent = -d->startPosition();
        qreal highlightStart;
        qreal highlightEnd;
        qreal endPositionFirstItem = 0;
        if (d->isContentFlowReversed()) {
            d->minExtent += d->hData.endMargin;
            if (d->model && d->model->count())
                endPositionFirstItem = d->positionAt(d->model->count()-1);
            else if (d->header)
                d->minExtent += d->headerSize();
            highlightStart = d->highlightRangeEndValid ? d->size() - d->highlightRangeEnd : d->size();
            highlightEnd = d->highlightRangeStartValid ? d->size() - d->highlightRangeStart : d->size();
            if (d->footer)
                d->minExtent += d->footerSize();
            qreal maxX = maxXExtent();
            if (d->minExtent < maxX)
                d->minExtent = maxX;
        } else {
            d->minExtent += d->hData.startMargin;
            endPositionFirstItem = d->endPositionAt(0);
            highlightStart = d->highlightRangeStart;
            highlightEnd = d->highlightRangeEnd;
            if (d->header)
                d->minExtent += d->headerSize();
        }
        if (d->haveHighlightRange && d->highlightRange == StrictlyEnforceRange) {
            d->minExtent += highlightStart;
            d->minExtent = d->isContentFlowReversed()
                                ? qMin(d->minExtent, endPositionFirstItem + highlightEnd)
                                : qMax(d->minExtent, -(endPositionFirstItem - highlightEnd));
        }
        d->hData.minExtentDirty = false;
    }

    return d->minExtent;
}

qreal QQuickItemView::maxXExtent() const
{
    Q_D(const QQuickItemView);
    if (d->layoutOrientation() == Qt::Vertical)
        return width();

    if (d->hData.maxExtentDirty) {
        qreal highlightStart;
        qreal highlightEnd;
        qreal lastItemPosition = 0;
        d->maxExtent = 0;
        if (d->isContentFlowReversed()) {
            highlightStart = d->highlightRangeEndValid ? d->size() - d->highlightRangeEnd : d->size();
            highlightEnd = d->highlightRangeStartValid ? d->size() - d->highlightRangeStart : d->size();
            lastItemPosition = d->endPosition();
        } else {
            highlightStart = d->highlightRangeStart;
            highlightEnd = d->highlightRangeEnd;
            if (d->model && d->model->count())
                lastItemPosition = d->positionAt(d->model->count()-1);
        }
        if (!d->model || !d->model->count()) {
            if (!d->isContentFlowReversed())
                d->maxExtent = d->header ? -d->headerSize() : 0;
            d->maxExtent += width();
        } else if (d->haveHighlightRange && d->highlightRange == StrictlyEnforceRange) {
            d->maxExtent = -(lastItemPosition - highlightStart);
            if (highlightEnd != highlightStart) {
                d->maxExtent = d->isContentFlowReversed()
                        ? qMax(d->maxExtent, -(d->endPosition() - highlightEnd))
                        : qMin(d->maxExtent, -(d->endPosition() - highlightEnd));
            }
        } else {
            d->maxExtent = -(d->endPosition() - width());
        }
        if (d->isContentFlowReversed()) {
            if (d->header)
                d->maxExtent -= d->headerSize();
            d->maxExtent -= d->hData.startMargin;
        } else {
            if (d->footer)
                d->maxExtent -= d->footerSize();
            d->maxExtent -= d->hData.endMargin;
            qreal minX = minXExtent();
            if (d->maxExtent > minX)
                d->maxExtent = minX;
        }
        d->hData.maxExtentDirty = false;
    }

    return d->maxExtent;
}

void QQuickItemView::setContentX(qreal pos)
{
    Q_D(QQuickItemView);
    // Positioning the view manually should override any current movement state
    d->moveReason = QQuickItemViewPrivate::Other;
    QQuickFlickable::setContentX(pos);
}

void QQuickItemView::setContentY(qreal pos)
{
    Q_D(QQuickItemView);
    // Positioning the view manually should override any current movement state
    d->moveReason = QQuickItemViewPrivate::Other;
    QQuickFlickable::setContentY(pos);
}

qreal QQuickItemView::xOrigin() const
{
    Q_D(const QQuickItemView);
    if (d->isContentFlowReversed())
        return -maxXExtent() + d->size() - d->hData.startMargin;
    else
        return -minXExtent() + d->hData.startMargin;
}

void QQuickItemView::updatePolish()
{
    Q_D(QQuickItemView);
    QQuickFlickable::updatePolish();
    d->layout();
}

void QQuickItemView::componentComplete()
{
    Q_D(QQuickItemView);
    if (d->model && d->ownModel)
        static_cast<QQuickVisualDataModel *>(d->model.data())->componentComplete();

    QQuickFlickable::componentComplete();

    updateSections();
    d->updateHeader();
    d->updateFooter();
    d->updateViewport();
    d->setPosition(d->contentStartOffset());
    d->usePopulateTransition = true;

    if (d->isValid()) {
        d->refill();
        d->moveReason = QQuickItemViewPrivate::SetIndex;
        if (d->currentIndex < 0 && !d->currentIndexCleared)
            d->updateCurrent(0);
        else
            d->updateCurrent(d->currentIndex);
        if (d->highlight && d->currentItem) {
            if (d->autoHighlight)
                d->resetHighlightPosition();
            d->updateTrackedItem();
        }
        d->moveReason = QQuickItemViewPrivate::Other;
        d->fixupPosition();
    }
    if (d->model && d->model->count())
        emit countChanged();
}



QQuickItemViewPrivate::QQuickItemViewPrivate()
    : itemCount(0)
    , buffer(0), bufferMode(BufferBefore | BufferAfter)
    , layoutDirection(Qt::LeftToRight)
    , moveReason(Other)
    , visibleIndex(0)
    , currentIndex(-1), currentItem(0)
    , trackedItem(0), requestedIndex(-1), requestedItem(0)
    , highlightComponent(0), highlight(0)
    , highlightRange(QQuickItemView::NoHighlightRange)
    , highlightRangeStart(0), highlightRangeEnd(0)
    , highlightMoveDuration(150)
    , headerComponent(0), header(0), footerComponent(0), footer(0)
    , populateTransition(0)
    , addTransition(0), addDisplacedTransition(0)
    , moveTransition(0), moveDisplacedTransition(0)
    , removeTransition(0), removeDisplacedTransition(0)
    , minExtent(0), maxExtent(0)
    , ownModel(false), wrap(false)
    , disableLayout(false), inViewportMoved(false), forceLayout(false), currentIndexCleared(false)
    , haveHighlightRange(false), autoHighlight(true), highlightRangeStartValid(false), highlightRangeEndValid(false)
    , fillCacheBuffer(false), inRequest(false), requestedAsync(false)
    , usePopulateTransition(false), runDelayedRemoveTransition(false)
{
}

bool QQuickItemViewPrivate::isValid() const
{
    return model && model->count() && model->isValid();
}

qreal QQuickItemViewPrivate::position() const
{
    Q_Q(const QQuickItemView);
    return layoutOrientation() == Qt::Vertical ? q->contentY() : q->contentX();
}

qreal QQuickItemViewPrivate::size() const
{
    Q_Q(const QQuickItemView);
    return layoutOrientation() == Qt::Vertical ? q->height() : q->width();
}

qreal QQuickItemViewPrivate::startPosition() const
{
    return isContentFlowReversed() ? -lastPosition() : originPosition();
}

qreal QQuickItemViewPrivate::endPosition() const
{
    return isContentFlowReversed() ? -originPosition() : lastPosition();
}

qreal QQuickItemViewPrivate::contentStartOffset() const
{
    qreal pos = -headerSize();
    if (layoutOrientation() == Qt::Vertical)
        pos -= vData.startMargin;
    else if (isContentFlowReversed())
        pos -= hData.endMargin;
    else
        pos -= hData.startMargin;

    return pos;
}

int QQuickItemViewPrivate::findLastVisibleIndex(int defaultValue) const
{
    if (visibleItems.count()) {
        int i = visibleItems.count() - 1;
        while (i > 0 && visibleItems.at(i)->index == -1)
            --i;
        if (visibleItems.at(i)->index != -1)
            return visibleItems.at(i)->index;
    }
    return defaultValue;
}

FxViewItem *QQuickItemViewPrivate::visibleItem(int modelIndex) const {
    if (modelIndex >= visibleIndex && modelIndex < visibleIndex + visibleItems.count()) {
        for (int i = modelIndex - visibleIndex; i < visibleItems.count(); ++i) {
            FxViewItem *item = visibleItems.at(i);
            if (item->index == modelIndex)
                return item;
        }
    }
    return 0;
}

// should rename to firstItemInView() to avoid confusion with other "*visible*" methods
// that don't look at the view position and size
FxViewItem *QQuickItemViewPrivate::firstVisibleItem() const {
    const qreal pos = isContentFlowReversed() ? -position()-size() : position();
    for (int i = 0; i < visibleItems.count(); ++i) {
        FxViewItem *item = visibleItems.at(i);
        if (item->index != -1 && item->endPosition() > pos)
            return item;
    }
    return visibleItems.count() ? visibleItems.first() : 0;
}

int QQuickItemViewPrivate::findLastIndexInView() const
{
    const qreal viewEndPos = isContentFlowReversed() ? -position() : position() + size();
    for (int i=visibleItems.count() - 1; i>=0; i--) {
        if (visibleItems.at(i)->position() <= viewEndPos && visibleItems.at(i)->index != -1)
            return visibleItems.at(i)->index;
    }
    return -1;
}

// Map a model index to visibleItems list index.
// These may differ if removed items are still present in the visible list,
// e.g. doing a removal animation
int QQuickItemViewPrivate::mapFromModel(int modelIndex) const
{
    if (modelIndex < visibleIndex || modelIndex >= visibleIndex + visibleItems.count())
        return -1;
    for (int i = 0; i < visibleItems.count(); ++i) {
        FxViewItem *item = visibleItems.at(i);
        if (item->index == modelIndex)
            return i;
        if (item->index > modelIndex)
            return -1;
    }
    return -1; // Not in visibleList
}

void QQuickItemViewPrivate::init()
{
    Q_Q(QQuickItemView);
    QQuickItemPrivate::get(contentItem)->childrenDoNotOverlap = true;
    q->setFlag(QQuickItem::ItemIsFocusScope);
    QObject::connect(q, SIGNAL(movementEnded()), q, SLOT(animStopped()));
    q->setFlickableDirection(QQuickFlickable::VerticalFlick);
}

void QQuickItemViewPrivate::updateCurrent(int modelIndex)
{
    Q_Q(QQuickItemView);
    applyPendingChanges();
    if (!q->isComponentComplete() || !isValid() || modelIndex < 0 || modelIndex >= model->count()) {
        if (currentItem) {
            currentItem->attached->setIsCurrentItem(false);
            releaseItem(currentItem);
            currentItem = 0;
            currentIndex = modelIndex;
            emit q->currentIndexChanged();
            emit q->currentItemChanged();
            updateHighlight();
        } else if (currentIndex != modelIndex) {
            currentIndex = modelIndex;
            emit q->currentIndexChanged();
        }
        return;
    }

    if (currentItem && currentIndex == modelIndex) {
        updateHighlight();
        return;
    }

    FxViewItem *oldCurrentItem = currentItem;
    int oldCurrentIndex = currentIndex;
    currentIndex = modelIndex;
    currentItem = createItem(modelIndex, false);
    if (oldCurrentItem && (!currentItem || oldCurrentItem->item != currentItem->item))
        oldCurrentItem->attached->setIsCurrentItem(false);
    if (currentItem) {
        currentItem->item->setFocus(true);
        currentItem->attached->setIsCurrentItem(true);
        initializeCurrentItem();
    }

    updateHighlight();
    if (oldCurrentIndex != currentIndex)
        emit q->currentIndexChanged();
    if (oldCurrentItem != currentItem)
        emit q->currentItemChanged();
    releaseItem(oldCurrentItem);
}

void QQuickItemViewPrivate::clear()
{
    currentChanges.reset();
    timeline.clear();

    for (int i = 0; i < visibleItems.count(); ++i)
        releaseItem(visibleItems.at(i));
    visibleItems.clear();
    visibleIndex = 0;

    for (int i = 0; i < releasePendingTransition.count(); ++i) {
        releasePendingTransition.at(i)->releaseAfterTransition = false;
        releaseItem(releasePendingTransition.at(i));
    }
    releasePendingTransition.clear();

    releaseItem(currentItem);
    currentItem = 0;
    createHighlight();
    trackedItem = 0;

    markExtentsDirty();
    itemCount = 0;
}


void QQuickItemViewPrivate::mirrorChange()
{
    Q_Q(QQuickItemView);
    regenerate();
    emit q->effectiveLayoutDirectionChanged();
}

void QQuickItemViewPrivate::refill()
{
    qreal s = qMax(size(), qreal(0.));
    if (isContentFlowReversed())
        refill(-position()-s, -position());
    else
        refill(position(), position()+s);
}

void QQuickItemViewPrivate::refill(qreal from, qreal to)
{
    Q_Q(QQuickItemView);
    if (!isValid() || !q->isComponentComplete())
        return;

    currentChanges.reset();

    int prevCount = itemCount;
    itemCount = model->count();
    qreal bufferFrom = from - buffer;
    qreal bufferTo = to + buffer;
    qreal fillFrom = from;
    qreal fillTo = to;

    bool added = addVisibleItems(fillFrom, fillTo, false);
    bool removed = removeNonVisibleItems(bufferFrom, bufferTo);

    if (buffer && bufferMode != NoBuffer) {
        if (bufferMode & BufferAfter)
            fillTo = bufferTo;
        if (bufferMode & BufferBefore)
            fillFrom = bufferFrom;
        added |= addVisibleItems(fillFrom, fillTo, true);
    }

    if (added || removed) {
        markExtentsDirty();
        updateBeginningEnd();
        visibleItemsChanged();
        updateHeader();
        updateFooter();
        updateViewport();
    }

    if (prevCount != itemCount)
        emit q->countChanged();
}

void QQuickItemViewPrivate::regenerate()
{
    Q_Q(QQuickItemView);
    if (q->isComponentComplete()) {
        currentChanges.reset();
        delete header;
        header = 0;
        delete footer;
        footer = 0;
        updateHeader();
        updateFooter();
        clear();
        updateViewport();
        setPosition(contentStartOffset());
        refill();
        updateCurrent(currentIndex);
    }
}

void QQuickItemViewPrivate::updateViewport()
{
    Q_Q(QQuickItemView);
    if (isValid()) {
        if (layoutOrientation() == Qt::Vertical)
            q->setContentHeight(endPosition() - startPosition());
        else
            q->setContentWidth(endPosition() - startPosition());
    }
}

void QQuickItemViewPrivate::layout()
{
    Q_Q(QQuickItemView);
    if (disableLayout)
        return;

    if (!isValid() && !visibleItems.count()) {
        clear();
        setPosition(contentStartOffset());
        usePopulateTransition = false;
        return;
    }

    if (runDelayedRemoveTransition && canTransition(FxViewItemTransitionManager::RemoveTransition, false)) {
        // assume that any items moving now are moving due to the remove - if they schedule
        // a different transition, that will override this one anyway
        for (int i=0; i<visibleItems.count(); i++)
            transitionNextReposition(visibleItems[i], FxViewItemTransitionManager::RemoveTransition, false);
    }

    ChangeResult insertionPosChanges;
    ChangeResult removalPosChanges;
    if (!applyModelChanges(&insertionPosChanges, &removalPosChanges) && !forceLayout) {
        if (fillCacheBuffer) {
            fillCacheBuffer = false;
            refill();
        }
        return;
    }
    forceLayout = false;

    if (canTransition(FxViewItemTransitionManager::PopulateTransition, true)) {
        for (int i=0; i<visibleItems.count(); i++)
            transitionNextReposition(visibleItems.at(i), FxViewItemTransitionManager::PopulateTransition, true);
    }
    layoutVisibleItems();

    int lastIndexInView = findLastIndexInView();
    refill();
    markExtentsDirty();
    updateHighlight();

    if (!q->isMoving() && !q->isFlicking()) {
        fixupPosition();
        refill();
    }

    updateHeader();
    updateFooter();
    updateViewport();
    updateUnrequestedPositions();

    if (hasItemTransitions()) {
        // items added in the last refill() may need to be transitioned in - e.g. a remove
        // causes items to slide up into view
        if (canTransition(FxViewItemTransitionManager::MoveTransition, false)
                || canTransition(FxViewItemTransitionManager::RemoveTransition, false)) {
            translateAndTransitionItemsAfter(lastIndexInView, insertionPosChanges, removalPosChanges);
        }

        prepareVisibleItemTransitions();

        QRectF viewBounds(0, position(), q->width(), q->height());
        for (QList<FxViewItem*>::Iterator it = releasePendingTransition.begin();
             it != releasePendingTransition.end(); ) {
            FxViewItem *item = *it;
            if ( (item->transition && item->transition->isActive())
                 || prepareNonVisibleItemTransition(item, viewBounds)) {
                ++it;
            } else {
                releaseItem(item);
                it = releasePendingTransition.erase(it);
            }
        }

        for (int i=0; i<visibleItems.count(); i++)
            visibleItems[i]->startTransition();
        for (int i=0; i<releasePendingTransition.count(); i++)
            releasePendingTransition[i]->startTransition();
    }
    usePopulateTransition = false;
    runDelayedRemoveTransition = false;
}

bool QQuickItemViewPrivate::applyModelChanges(ChangeResult *totalInsertionResult, ChangeResult *totalRemovalResult)
{
    Q_Q(QQuickItemView);
    if (!q->isComponentComplete() || (!currentChanges.hasPendingChanges() && !runDelayedRemoveTransition) || disableLayout)
        return false;

    disableLayout = true;

    updateUnrequestedIndexes();
    moveReason = QQuickItemViewPrivate::Other;

    FxViewItem *prevVisibleItemsFirst = visibleItems.count() ? *visibleItems.constBegin() : 0;
    int prevItemCount = itemCount;
    int prevVisibleItemsCount = visibleItems.count();
    bool visibleAffected = false;
    bool viewportChanged = !currentChanges.pendingChanges.removes().isEmpty()
            || !currentChanges.pendingChanges.inserts().isEmpty();

    FxViewItem *prevFirstVisible = firstVisibleItem();
    QDeclarativeNullableValue<qreal> prevViewPos;
    int prevFirstVisibleIndex = -1;
    if (prevFirstVisible) {
        prevViewPos = prevFirstVisible->position();
        prevFirstVisibleIndex = prevFirstVisible->index;
    }
    qreal prevVisibleItemsFirstPos = visibleItems.count() ? visibleItems.first()->position() : 0.0;

    totalInsertionResult->visiblePos = prevViewPos;
    totalRemovalResult->visiblePos = prevViewPos;

    const QVector<QDeclarativeChangeSet::Remove> &removals = currentChanges.pendingChanges.removes();
    const QVector<QDeclarativeChangeSet::Insert> &insertions = currentChanges.pendingChanges.inserts();
    ChangeResult insertionResult(prevViewPos);
    ChangeResult removalResult(prevViewPos);

    int removedCount = 0;
    for (int i=0; i<removals.count(); i++) {
        itemCount -= removals[i].count;
        if (applyRemovalChange(removals[i], &removalResult, &removedCount))
            visibleAffected = true;
        if (!visibleAffected && needsRefillForAddedOrRemovedIndex(removals[i].index))
            visibleAffected = true;
        if (prevFirstVisibleIndex >= 0 && removals[i].index < prevFirstVisibleIndex) {
            if (removals[i].index + removals[i].count < prevFirstVisibleIndex)
                removalResult.countChangeBeforeVisible += removals[i].count;
            else
                removalResult.countChangeBeforeVisible += (prevFirstVisibleIndex - removals[i].index);
        }
    }
    if (runDelayedRemoveTransition) {
        QDeclarativeChangeSet::Remove removal;
        for (QList<FxViewItem*>::Iterator it = visibleItems.begin(); it != visibleItems.end();) {
            FxViewItem *item = *it;
            if (item->index == -1 && !item->attached->delayRemove()) {
                removeItem(item, removal, &removalResult);
                removedCount++;
                it = visibleItems.erase(it);
            } else {
               ++it;
            }
        }
    }
    *totalRemovalResult += removalResult;
    if (!removals.isEmpty()) {
        updateVisibleIndex();

        // set positions correctly for the next insertion
        if (!insertions.isEmpty()) {
            repositionFirstItem(prevVisibleItemsFirst, prevVisibleItemsFirstPos, prevFirstVisible, &insertionResult, &removalResult);
            layoutVisibleItems(removals.first().index);
        }
    }

    QList<FxViewItem *> newItems;
    QList<MovedItem> movingIntoView;

    for (int i=0; i<insertions.count(); i++) {
        bool wasEmpty = visibleItems.isEmpty();
        if (applyInsertionChange(insertions[i], &insertionResult, &newItems, &movingIntoView))
            visibleAffected = true;
        if (!visibleAffected && needsRefillForAddedOrRemovedIndex(insertions[i].index))
            visibleAffected = true;
        if (wasEmpty && !visibleItems.isEmpty())
            resetFirstItemPosition();
        *totalInsertionResult += insertionResult;

        // set positions correctly for the next insertion
        if (i < insertions.count() - 1) {
            repositionFirstItem(prevVisibleItemsFirst, prevVisibleItemsFirstPos, prevFirstVisible, &insertionResult, &removalResult);
            layoutVisibleItems(insertions[i].index);
        }
        itemCount += insertions[i].count;
    }
    for (int i=0; i<newItems.count(); i++)
        newItems.at(i)->attached->emitAdd();

    // for each item that was moved directly into the view as a result of a move(),
    // find the index it was moved from in order to set its initial position, so that we
    // can transition it from this "original" position to its new position in the view
    if (canTransition(FxViewItemTransitionManager::MoveTransition, true)) {
        for (int i=0; i<movingIntoView.count(); i++) {
            int fromIndex = findMoveKeyIndex(movingIntoView[i].moveKey, removals);
            if (fromIndex >= 0) {
                if (prevFirstVisibleIndex >= 0 && fromIndex < prevFirstVisibleIndex)
                    repositionItemAt(movingIntoView[i].item, fromIndex, -totalInsertionResult->sizeChangesAfterVisiblePos);
                else
                    repositionItemAt(movingIntoView[i].item, fromIndex, totalInsertionResult->sizeChangesAfterVisiblePos);
                transitionNextReposition(movingIntoView[i].item, FxViewItemTransitionManager::MoveTransition, true);
            }
        }
    }

    // reposition visibleItems.first() correctly so that the content y doesn't jump
    if (removedCount != prevVisibleItemsCount)
        repositionFirstItem(prevVisibleItemsFirst, prevVisibleItemsFirstPos, prevFirstVisible, &insertionResult, &removalResult);

    // Whatever removed/moved items remain are no longer visible items.
    prepareRemoveTransitions(&currentChanges.removedItems);
    for (QHash<QDeclarativeChangeSet::MoveKey, FxViewItem *>::Iterator it = currentChanges.removedItems.begin();
         it != currentChanges.removedItems.end(); ++it) {
        releaseItem(it.value());
    }
    currentChanges.removedItems.clear();

    if (currentChanges.currentChanged) {
        if (currentChanges.currentRemoved && currentItem) {
            currentItem->attached->setIsCurrentItem(false);
            releaseItem(currentItem);
            currentItem = 0;
        }
        if (!currentIndexCleared)
            updateCurrent(currentChanges.newCurrentIndex);
    }

    if (!visibleAffected)
        visibleAffected = !currentChanges.pendingChanges.changes().isEmpty();
    currentChanges.reset();

    updateSections();
    if (prevItemCount != itemCount)
        emit q->countChanged();
    if (!visibleAffected && viewportChanged)
        updateViewport();

    disableLayout = false;
    return visibleAffected;
}

bool QQuickItemViewPrivate::applyRemovalChange(const QDeclarativeChangeSet::Remove &removal, ChangeResult *removeResult, int *removedCount)
{
    Q_Q(QQuickItemView);
    bool visibleAffected = false;

    if (visibleItems.count() && removal.index + removal.count > visibleItems.last()->index) {
        if (removal.index > visibleItems.last()->index)
            removeResult->countChangeAfterVisibleItems += removal.count;
        else
            removeResult->countChangeAfterVisibleItems += ((removal.index + removal.count - 1) - visibleItems.last()->index);
    }

    QList<FxViewItem*>::Iterator it = visibleItems.begin();
    while (it != visibleItems.end()) {
        FxViewItem *item = *it;
        if (item->index == -1 || item->index < removal.index) {
            // already removed, or before removed items
            if (!visibleAffected && item->index < removal.index)
                visibleAffected = true;
            ++it;
        } else if (item->index >= removal.index + removal.count) {
            // after removed items
            item->index -= removal.count;
            if (removal.isMove())
                transitionNextReposition(item, FxViewItemTransitionManager::MoveTransition, false);
            else
                transitionNextReposition(item, FxViewItemTransitionManager::RemoveTransition, false);
            ++it;
        } else {
            // removed item
            visibleAffected = true;
            if (!removal.isMove())
                item->attached->emitRemove();

            if (item->attached->delayRemove() && !removal.isMove()) {
                item->index = -1;
                QObject::connect(item->attached, SIGNAL(delayRemoveChanged()), q, SLOT(destroyRemoved()), Qt::QueuedConnection);
                ++it;
            } else {
                removeItem(item, removal, removeResult);
                if (!removal.isMove())
                    (*removedCount)++;
                it = visibleItems.erase(it);
            }
        }
    }

    return visibleAffected;
}

void QQuickItemViewPrivate::removeItem(FxViewItem *item, const QDeclarativeChangeSet::Remove &removal, ChangeResult *removeResult)
{
    if (removeResult->visiblePos.isValid()) {
        if (item->position() < removeResult->visiblePos)
            removeResult->sizeChangesBeforeVisiblePos += item->size();
        else
            removeResult->sizeChangesAfterVisiblePos += item->size();
    }
    if (removal.isMove()) {
        currentChanges.removedItems.insert(removal.moveKey(item->index), item);
        transitionNextReposition(item, FxViewItemTransitionManager::MoveTransition, true);
    } else {
        // track item so it is released later
        currentChanges.removedItems.insertMulti(QDeclarativeChangeSet::MoveKey(), item);
    }
    if (!removeResult->changedFirstItem && item == *visibleItems.constBegin())
        removeResult->changedFirstItem = true;
}

void QQuickItemViewPrivate::repositionFirstItem(FxViewItem *prevVisibleItemsFirst,
                                                   qreal prevVisibleItemsFirstPos,
                                                   FxViewItem *prevFirstVisible,
                                                   ChangeResult *insertionResult,
                                                   ChangeResult *removalResult)
{
    const QDeclarativeNullableValue<qreal> prevViewPos = insertionResult->visiblePos;

    // reposition visibleItems.first() correctly so that the content y doesn't jump
    if (visibleItems.count()) {
        if (prevVisibleItemsFirst && insertionResult->changedFirstItem)
            resetFirstItemPosition(prevVisibleItemsFirstPos);

        if (prevFirstVisible && prevVisibleItemsFirst == prevFirstVisible
                && prevFirstVisible != *visibleItems.constBegin()) {
            // the previous visibleItems.first() was also the first visible item, and it has been
            // moved/removed, so move the new visibleItems.first() to the pos of the previous one
            if (!insertionResult->changedFirstItem)
                resetFirstItemPosition(prevVisibleItemsFirstPos);

        } else if (prevViewPos.isValid()) {
            qreal moveForwardsBy = 0;
            qreal moveBackwardsBy = 0;

            // shift visibleItems.first() relative to the number of added/removed items
            if (visibleItems.first()->position() > prevViewPos) {
                moveForwardsBy = insertionResult->sizeChangesAfterVisiblePos;
                moveBackwardsBy = removalResult->sizeChangesAfterVisiblePos;
            } else if (visibleItems.first()->position() < prevViewPos) {
                moveForwardsBy = removalResult->sizeChangesBeforeVisiblePos;
                moveBackwardsBy = insertionResult->sizeChangesBeforeVisiblePos;
            }
            adjustFirstItem(moveForwardsBy, moveBackwardsBy, insertionResult->countChangeBeforeVisible - removalResult->countChangeBeforeVisible);
        }
        insertionResult->reset();
        removalResult->reset();
    }
}

void QQuickItemViewPrivate::prepareVisibleItemTransitions()
{
    Q_Q(QQuickItemView);
    if (!hasItemTransitions())
        return;

    addTransitionIndexes.clear();
    addTransitionTargets.clear();
    moveTransitionIndexes.clear();
    moveTransitionTargets.clear();

    QRectF viewBounds(0, position(), q->width(), q->height());
    for (int i=0; i<visibleItems.count(); i++) {
        // must call for every visible item to init or discard transitions
        if (!visibleItems[i]->prepareTransition(viewBounds))
            continue;
        if (visibleItems[i]->isTransitionTarget) {
            switch (visibleItems[i]->nextTransitionType) {
            case FxViewItemTransitionManager::NoTransition:
                break;
            case FxViewItemTransitionManager::PopulateTransition:
            case FxViewItemTransitionManager::AddTransition:
                addTransitionIndexes.append(visibleItems[i]->index);
                addTransitionTargets.append(visibleItems[i]->item);
                break;
            case FxViewItemTransitionManager::MoveTransition:
                moveTransitionIndexes.append(visibleItems[i]->index);
                moveTransitionTargets.append(visibleItems[i]->item);
                break;
            case FxViewItemTransitionManager::RemoveTransition:
                // removed targets won't be in visibleItems, handle these
                // in prepareNonVisibleItemTransition()
                break;
            }
        }
    }
}

void QQuickItemViewPrivate::prepareRemoveTransitions(QHash<QDeclarativeChangeSet::MoveKey, FxViewItem *> *removedItems)
{
    removeTransitionIndexes.clear();
    removeTransitionTargets.clear();

    if (canTransition(FxViewItemTransitionManager::RemoveTransition, true)) {
        for (QHash<QDeclarativeChangeSet::MoveKey, FxViewItem *>::Iterator it = removedItems->begin();
             it != removedItems->end(); ) {
            bool isRemove = it.key().moveId < 0;
            if (isRemove) {
                FxViewItem *item = *it;
                item->releaseAfterTransition = true;
                releasePendingTransition.append(item);
                transitionNextReposition(item, FxViewItemTransitionManager::RemoveTransition, true);
                it = removedItems->erase(it);
            } else {
                ++it;
            }
        }
    }
}

bool QQuickItemViewPrivate::prepareNonVisibleItemTransition(FxViewItem *item, const QRectF &viewBounds)
{
    // Called for items that have been removed from visibleItems and may now be
    // transitioned out of the view. This applies to items that are being directly
    // removed, or moved to outside of the view, as well as those that are
    // displaced to a position outside of the view due to an insert or move.

    if (item->nextTransitionType == FxViewItemTransitionManager::MoveTransition)
        repositionItemAt(item, item->index, 0);
    if (!item->prepareTransition(viewBounds))
        return false;

    if (item->isTransitionTarget) {
        if (item->nextTransitionType == FxViewItemTransitionManager::MoveTransition) {
            moveTransitionIndexes.append(item->index);
            moveTransitionTargets.append(item->item);
        } else if (item->nextTransitionType == FxViewItemTransitionManager::RemoveTransition) {
            removeTransitionIndexes.append(item->index);
            removeTransitionTargets.append(item->item);
        }
    }

    item->releaseAfterTransition = true;
    return true;
}

/*
  This may return 0 if the item is being created asynchronously.
  When the item becomes available, refill() will be called and the item
  will be returned on the next call to createItem().
*/
FxViewItem *QQuickItemViewPrivate::createItem(int modelIndex, bool asynchronous)
{
    Q_Q(QQuickItemView);

    if (requestedIndex == modelIndex && (asynchronous || requestedAsync == asynchronous))
        return 0;

    if (requestedIndex != -1 && requestedIndex != modelIndex) {
        if (requestedItem && requestedItem->item)
            requestedItem->item->setParentItem(0);
        delete requestedItem;
        requestedItem = 0;
    }

    for (int i=0; i<releasePendingTransition.count(); i++) {
        if (releasePendingTransition[i]->index == modelIndex
                && !releasePendingTransition[i]->isPendingRemoval()) {
            releasePendingTransition[i]->releaseAfterTransition = false;
            return releasePendingTransition.takeAt(i);
        }
    }

    requestedIndex = modelIndex;
    requestedAsync = asynchronous;
    inRequest = true;

    if (QQuickItem *item = model->item(modelIndex, asynchronous)) {
        item->setParentItem(q->contentItem());
        QDeclarative_setParent_noEvent(item, q->contentItem());
        requestedIndex = -1;
        FxViewItem *viewItem = requestedItem;
        if (!viewItem)
            viewItem = newViewItem(modelIndex, item); // already in cache, so viewItem not initialized in initItem()
        if (viewItem) {
            viewItem->index = modelIndex;
            // do other set up for the new item that should not happen
            // until after bindings are evaluated
            initializeViewItem(viewItem);
            unrequestedItems.remove(item);
        }
        requestedItem = 0;
        inRequest = false;
        return viewItem;
    }

    inRequest = false;
    return 0;
}

void QQuickItemView::createdItem(int index, QQuickItem *item)
{
    Q_D(QQuickItemView);
    if (d->requestedIndex != index) {
        item->setParentItem(contentItem());
        d->unrequestedItems.insert(item, index);
        item->setVisible(false);
        d->repositionPackageItemAt(item, index);
    } else {
        d->requestedIndex = -1;
        if (!d->inRequest) {
            if (index == d->currentIndex)
                d->updateCurrent(index);
            d->refill();
        }
    }
}

void QQuickItemView::initItem(int index, QQuickItem *item)
{
    Q_D(QQuickItemView);
    item->setZ(1);
    if (d->requestedIndex == index) {
        item->setParentItem(contentItem());
        QDeclarative_setParent_noEvent(item, contentItem());
        d->requestedItem = d->newViewItem(index, item);
    }
}

void QQuickItemView::destroyingItem(QQuickItem *item)
{
    Q_D(QQuickItemView);
    d->unrequestedItems.remove(item);
}

void QQuickItemViewPrivate::releaseItem(FxViewItem *item)
{
    Q_Q(QQuickItemView);
    if (!item || !model)
        return;
    if (trackedItem == item)
        trackedItem = 0;
    QQuickItemPrivate *itemPrivate = QQuickItemPrivate::get(item->item);
    itemPrivate->removeItemChangeListener(this, QQuickItemPrivate::Geometry);
    if (model->release(item->item) == 0) {
        // item was not destroyed, and we no longer reference it.
        item->item->setVisible(false);
        unrequestedItems.insert(item->item, model->indexOf(item->item, q));
    }
    delete item;
}

QQuickItem *QQuickItemViewPrivate::createHighlightItem()
{
    return createComponentItem(highlightComponent, true, true);
}

QQuickItem *QQuickItemViewPrivate::createComponentItem(QDeclarativeComponent *component, bool receiveItemGeometryChanges, bool createDefault)
{
    Q_Q(QQuickItemView);

    QQuickItem *item = 0;
    if (component) {
        QDeclarativeContext *creationContext = component->creationContext();
        QDeclarativeContext *context = new QDeclarativeContext(
                creationContext ? creationContext : qmlContext(q));
        QObject *nobj = component->create(context);
        if (nobj) {
            QDeclarative_setParent_noEvent(context, nobj);
            item = qobject_cast<QQuickItem *>(nobj);
            if (!item)
                delete nobj;
        } else {
            delete context;
        }
    } else if (createDefault) {
        item = new QQuickItem;
    }
    if (item) {
        QDeclarative_setParent_noEvent(item, q->contentItem());
        item->setParentItem(q->contentItem());
        if (receiveItemGeometryChanges) {
            QQuickItemPrivate *itemPrivate = QQuickItemPrivate::get(item);
            itemPrivate->addItemChangeListener(this, QQuickItemPrivate::Geometry);
        }
    }
    return item;
}

void QQuickItemViewPrivate::updateTrackedItem()
{
    Q_Q(QQuickItemView);
    FxViewItem *item = currentItem;
    if (highlight)
        item = highlight;
    trackedItem = item;

    if (trackedItem)
        q->trackedPositionChanged();
}

void QQuickItemViewPrivate::updateUnrequestedIndexes()
{
    Q_Q(QQuickItemView);
    for (QHash<QQuickItem*,int>::iterator it = unrequestedItems.begin(); it != unrequestedItems.end(); ++it)
        *it = model->indexOf(it.key(), q);
}

void QQuickItemViewPrivate::updateUnrequestedPositions()
{
    for (QHash<QQuickItem*,int>::const_iterator it = unrequestedItems.begin(); it != unrequestedItems.end(); ++it)
        repositionPackageItemAt(it.key(), it.value());
}

void QQuickItemViewPrivate::updateVisibleIndex()
{
    visibleIndex = 0;
    for (QList<FxViewItem*>::Iterator it = visibleItems.begin(); it != visibleItems.end(); ++it) {
        if ((*it)->index != -1) {
            visibleIndex = (*it)->index;
            break;
        }
    }
}

QT_END_NAMESPACE
