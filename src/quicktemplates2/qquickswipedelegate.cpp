/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Qt Quick Templates 2 module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL3$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or later as published by the Free
** Software Foundation and appearing in the file LICENSE.GPL included in
** the packaging of this file. Please review the following information to
** ensure the GNU General Public License version 2.0 requirements will be
** met: http://www.gnu.org/licenses/gpl-2.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qquickswipedelegate_p.h"
#include "qquickcontrol_p_p.h"
#include "qquickitemdelegate_p_p.h"
#include "qquickvelocitycalculator_p_p.h"

#include <QtGui/qstylehints.h>
#include <QtGui/private/qguiapplication_p.h>
#include <QtGui/qpa/qplatformtheme.h>
#include <QtQml/qqmlinfo.h>

QT_BEGIN_NAMESPACE

/*!
    \qmltype SwipeDelegate
    \inherits ItemDelegate
    \instantiates QQuickSwipeDelegate
    \inqmlmodule QtQuick.Controls
    \since 5.7
    \brief A swipable item delegate.

    SwipeDelegate presents a view item that can be swiped left or right to
    expose more options or information. It is used as a delegate in views such
    as \l ListView.

    SwipeDelegate inherits its API from AbstractButton. For instance, you can set
    \l {AbstractButton::text}{text} and react to
    \l {AbstractButton::clicked}{clicks} using the AbstractButton API.

    Information regarding the progress of a swipe, as well as the components
    that should be shown upon swiping, are both available through the
    \l {SwipeDelegate::}{swipe} grouped property object. For example,
    \c swipe.position holds the position of the
    swipe within the range \c -1.0 to \c 1.0. The \c swipe.left
    property determines which item will be displayed when the control is swiped
    to the right, and vice versa for \c swipe.right. The positioning of these
    components is left to applications to decide. For example, without specifying
    any position for \c swipe.left or \c swipe.right, the following will
    occur:

    \image qtquickcontrols2-swipedelegate.gif

    If \c swipe.left and \c swipe.right are anchored to the left and
    right of the \l {Control::}{background} item (respectively), they'll behave like this:

    \image qtquickcontrols2-swipedelegate-leading-trailing.gif

    When using \c swipe.left and \c swipe.right, the control cannot be
    swiped past the left and right edges. To achieve this type of "wrapping"
    behavior, set \c swipe.behind instead. This will result in the same
    item being shown regardless of which direction the control is swiped. For
    example, in the image below, we set \c swipe.behind and then swipe the
    control repeatedly in both directions:

    \image qtquickcontrols2-swipedelegate-behind.gif

    \sa {Customizing SwipeDelegate}
*/

class QQuickSwipePrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QQuickSwipe)

public:
    QQuickSwipePrivate(QQuickSwipeDelegate *control) :
        control(control),
        positionBeforePress(0),
        position(0),
        wasComplete(false),
        complete(false),
        left(nullptr),
        behind(nullptr),
        right(nullptr),
        leftItem(nullptr),
        behindItem(nullptr),
        rightItem(nullptr)
    {
    }

    static QQuickSwipePrivate *get(QQuickSwipe *swipe);

    QQuickItem *createDelegateItem(QQmlComponent *component);
    QQuickItem *showRelevantItemForPosition(qreal position);
    QQuickItem *createRelevantItemForDistance(qreal distance);
    void createLeftItem();
    void createBehindItem();
    void createRightItem();
    void createAndShowLeftItem();
    void createAndShowBehindItem();
    void createAndShowRightItem();

    void warnAboutMixingDelegates();
    void warnAboutSettingDelegatesWhileVisible();

    QQuickSwipeDelegate *control;
    // Same range as position, but is set before press events so that we can
    // keep track of which direction the user must swipe when using left and right delegates.
    qreal positionBeforePress;
    qreal position;
    // A "less strict" version of complete that is true if complete was true
    // before the last press event.
    bool wasComplete;
    bool complete;
    QQuickVelocityCalculator velocityCalculator;
    QQmlComponent *left;
    QQmlComponent *behind;
    QQmlComponent *right;
    QQuickItem *leftItem;
    QQuickItem *behindItem;
    QQuickItem *rightItem;
};

QQuickSwipePrivate *QQuickSwipePrivate::get(QQuickSwipe *swipe)
{
    return swipe->d_func();
}

QQuickItem *QQuickSwipePrivate::createDelegateItem(QQmlComponent *component)
{
    // If we don't use the correct context, it won't be possible to refer to
    // the control's id from within the delegates.
    QQmlContext *creationContext = component->creationContext();
    // The component might not have been created in QML, in which case
    // the creation context will be null and we have to create it ourselves.
    if (!creationContext)
        creationContext = qmlContext(control);
    QQmlContext *context = new QQmlContext(creationContext);
    context->setContextObject(control);
    QQuickItem *item = qobject_cast<QQuickItem*>(component->beginCreate(context));
    if (item) {
        item->setParentItem(control);
        component->completeCreate();
    }
    return item;
}

QQuickItem *QQuickSwipePrivate::showRelevantItemForPosition(qreal position)
{
    if (qFuzzyIsNull(position))
        return nullptr;

    if (behind) {
        createAndShowBehindItem();
        return behindItem;
    }

    if (right && position < 0.0) {
        createAndShowRightItem();
        return rightItem;
    }

    if (left && position > 0.0) {
        createAndShowLeftItem();
        return leftItem;
    }

    return nullptr;
}

QQuickItem *QQuickSwipePrivate::createRelevantItemForDistance(qreal distance)
{
    if (qFuzzyIsNull(distance))
        return nullptr;

    if (behind) {
        createBehindItem();
        return behindItem;
    }

    // a) If the position before the press was 0.0, we know that *any* movement
    // whose distance is negative will result in the right item being shown and
    // vice versa.
    // b) Once the control has been exposed (that is, swiped to the left or right,
    // and hence the position is either -1.0 or 1.0), we must use the width of the
    // relevant item to determine if the distance is larger than that item,
    // in order to know whether or not to display it.
    // c) If the control has been exposed, and the swipe is larger than the width
    // of the relevant item from which the swipe started from, we must show the
    // item on the other side (if any).

    if (right) {
        if ((distance < 0.0 && positionBeforePress == 0.0) /* a) */
            || (rightItem && positionBeforePress == -1.0 && distance < rightItem->width()) /* b) */
            || (leftItem && positionBeforePress == 1.0 && qAbs(distance) > leftItem->width())) /* c) */ {
            createRightItem();
            return rightItem;
        }
    }

    if (left) {
        if ((distance > 0.0 && positionBeforePress == 0.0) /* a) */
            || (leftItem && positionBeforePress == 1.0 && qAbs(distance) < leftItem->width()) /* b) */
            || (rightItem && positionBeforePress == -1.0 && qAbs(distance) > rightItem->width())) /* c) */ {
            createLeftItem();
            return leftItem;
        }
    }

    return nullptr;
}

void QQuickSwipePrivate::createLeftItem()
{
    if (!leftItem) {
        Q_Q(QQuickSwipe);
        q->setLeftItem(createDelegateItem(left));
        if (!leftItem)
            qmlInfo(control) << "Failed to create left item:" << left->errors();
    }
}

void QQuickSwipePrivate::createBehindItem()
{
    if (!behindItem) {
        Q_Q(QQuickSwipe);
        q->setBehindItem(createDelegateItem(behind));
        if (!behindItem)
            qmlInfo(control) << "Failed to create behind item:" << behind->errors();
    }
}

void QQuickSwipePrivate::createRightItem()
{
    if (!rightItem) {
        Q_Q(QQuickSwipe);
        q->setRightItem(createDelegateItem(right));
        if (!rightItem)
            qmlInfo(control) << "Failed to create right item:" << right->errors();
    }
}

void QQuickSwipePrivate::createAndShowLeftItem()
{
    createLeftItem();

    if (leftItem)
        leftItem->setVisible(true);

    if (rightItem)
        rightItem->setVisible(false);
}

void QQuickSwipePrivate::createAndShowBehindItem()
{
    createBehindItem();

    if (behindItem)
        behindItem->setVisible(true);
}

void QQuickSwipePrivate::createAndShowRightItem()
{
    createRightItem();

    // This item may have already existed but was hidden.
    if (rightItem)
        rightItem->setVisible(true);

    // The left item isn't visible when the right item is visible, so save rendering effort by hiding it.
    if (leftItem)
        leftItem->setVisible(false);
}

void QQuickSwipePrivate::warnAboutMixingDelegates()
{
    qmlInfo(control) << "cannot set both behind and left/right properties";
}

void QQuickSwipePrivate::warnAboutSettingDelegatesWhileVisible()
{
    qmlInfo(control) << "left/right/behind properties may only be set when swipe.position is 0";
}

QQuickSwipe::QQuickSwipe(QQuickSwipeDelegate *control) :
    QObject(*(new QQuickSwipePrivate(control)))
{
}

QQmlComponent *QQuickSwipe::left() const
{
    Q_D(const QQuickSwipe);
    return d->left;
}

void QQuickSwipe::setLeft(QQmlComponent *left)
{
    Q_D(QQuickSwipe);
    if (left == d->left)
        return;

    if (d->behind) {
        d->warnAboutMixingDelegates();
        return;
    }

    if (!qFuzzyIsNull(d->position)) {
        d->warnAboutSettingDelegatesWhileVisible();
        return;
    }

    d->left = left;

    if (!d->left) {
        delete d->leftItem;
        d->leftItem = nullptr;
    }

    emit leftChanged();
}

QQmlComponent *QQuickSwipe::behind() const
{
    Q_D(const QQuickSwipe);
    return d->behind;
}

void QQuickSwipe::setBehind(QQmlComponent *behind)
{
    Q_D(QQuickSwipe);
    if (behind == d->behind)
        return;

    if (d->left || d->right) {
        d->warnAboutMixingDelegates();
        return;
    }

    if (!qFuzzyIsNull(d->position)) {
        d->warnAboutSettingDelegatesWhileVisible();
        return;
    }

    d->behind = behind;

    if (!d->behind) {
        delete d->behindItem;
        d->behindItem = nullptr;
    }

    emit behindChanged();
}

QQmlComponent *QQuickSwipe::right() const
{
    Q_D(const QQuickSwipe);
    return d->right;
}

void QQuickSwipe::setRight(QQmlComponent *right)
{
    Q_D(QQuickSwipe);
    if (right == d->right)
        return;

    if (d->behind) {
        d->warnAboutMixingDelegates();
        return;
    }

    if (!qFuzzyIsNull(d->position)) {
        d->warnAboutSettingDelegatesWhileVisible();
        return;
    }

    d->right = right;

    if (!d->right) {
        delete d->rightItem;
        d->rightItem = nullptr;
    }

    emit rightChanged();
}

QQuickItem *QQuickSwipe::leftItem() const
{
    Q_D(const QQuickSwipe);
    return d->leftItem;
}

void QQuickSwipe::setLeftItem(QQuickItem *item)
{
    Q_D(QQuickSwipe);
    if (item == d->leftItem)
        return;

    delete d->leftItem;
    d->leftItem = item;

    if (d->leftItem) {
        d->leftItem->setParentItem(d->control);

        if (qFuzzyIsNull(d->leftItem->z()))
            d->leftItem->setZ(-2);
    }

    emit leftItemChanged();
}

QQuickItem *QQuickSwipe::behindItem() const
{
    Q_D(const QQuickSwipe);
    return d->behindItem;
}

void QQuickSwipe::setBehindItem(QQuickItem *item)
{
    Q_D(QQuickSwipe);
    if (item == d->behindItem)
        return;

    delete d->behindItem;
    d->behindItem = item;

    if (d->behindItem) {
        d->behindItem->setParentItem(d->control);

        if (qFuzzyIsNull(d->behindItem->z()))
            d->behindItem->setZ(-2);
    }

    emit behindItemChanged();
}

QQuickItem *QQuickSwipe::rightItem() const
{
    Q_D(const QQuickSwipe);
    return d->rightItem;
}

void QQuickSwipe::setRightItem(QQuickItem *item)
{
    Q_D(QQuickSwipe);
    if (item == d->rightItem)
        return;

    delete d->rightItem;
    d->rightItem = item;

    if (d->rightItem) {
        d->rightItem->setParentItem(d->control);

        if (qFuzzyIsNull(d->rightItem->z()))
            d->rightItem->setZ(-2);
    }

    emit rightItemChanged();
}

qreal QQuickSwipe::position() const
{
    Q_D(const QQuickSwipe);
    return d->position;
}

void QQuickSwipe::setPosition(qreal position)
{
    Q_D(QQuickSwipe);
    const qreal adjustedPosition = qBound<qreal>(-1.0, position, 1.0);
    if (adjustedPosition == d->position)
        return;

    d->position = adjustedPosition;

    QQuickItem *relevantItem = d->showRelevantItemForPosition(d->position);
    const qreal relevantWidth = relevantItem ? relevantItem->width() : 0.0;
    d->control->contentItem()->setProperty("x", d->position * relevantWidth + d->control->leftPadding());
    if (QQuickItem *background = d->control->background())
        background->setProperty("x", d->position * relevantWidth);

    emit positionChanged();
}

bool QQuickSwipe::isComplete() const
{
    Q_D(const QQuickSwipe);
    return d->complete;
}

void QQuickSwipe::setComplete(bool complete)
{
    Q_D(QQuickSwipe);
    if (complete == d->complete)
        return;

    d->complete = complete;
    emit completeChanged();
}

class QQuickSwipeDelegatePrivate : public QQuickItemDelegatePrivate
{
    Q_DECLARE_PUBLIC(QQuickSwipeDelegate)

public:
    QQuickSwipeDelegatePrivate(QQuickSwipeDelegate *control) :
        swipe(control)
    {
    }

    bool handleMousePressEvent(QQuickItem *item, QMouseEvent *event);
    bool handleMouseMoveEvent(QQuickItem *item, QMouseEvent *event);
    bool handleMouseReleaseEvent(QQuickItem *item, QMouseEvent *event);

    void resizeContent() override;

    QQuickSwipe swipe;
};

bool QQuickSwipeDelegatePrivate::handleMousePressEvent(QQuickItem *item, QMouseEvent *event)
{
    Q_Q(QQuickSwipeDelegate);
    QQuickSwipePrivate *swipePrivate = QQuickSwipePrivate::get(&swipe);
    // If the position is 0, we want to handle events ourself - we don't want child items to steal them.
    // This code will only get called when a child item has been created;
    // events will go through the regular channels (mousePressEvent()) until then.
    if (qFuzzyIsNull(swipePrivate->position)) {
        q->mousePressEvent(event);
        return true;
    }

    swipePrivate->positionBeforePress = swipePrivate->position;
    swipePrivate->velocityCalculator.startMeasuring(event->pos(), event->timestamp());
    pressPoint = item->mapToItem(q, event->pos());
    return false;
}

bool QQuickSwipeDelegatePrivate::handleMouseMoveEvent(QQuickItem *item, QMouseEvent *event)
{
    Q_Q(QQuickSwipeDelegate);

    if (holdTimer > 0) {
        if (QLineF(pressPoint, event->localPos()).length() > QGuiApplication::styleHints()->startDragDistance())
            stopPressAndHold();
    }

    // Protect against division by zero.
    if (width == 0)
        return false;

    // Don't bother reacting to events if we don't have any delegates.
    QQuickSwipePrivate *swipePrivate = QQuickSwipePrivate::get(&swipe);
    if (!swipePrivate->left && !swipePrivate->right && !swipePrivate->behind)
        return false;

    // Don't handle move events for the control if it wasn't pressed.
    if (item == q && !pressed)
        return false;

    const qreal distance = (event->pos() - pressPoint).x();
    if (!q->keepMouseGrab()) {
        // Taken from QQuickDrawer::handleMouseMoveEvent; see comments there.
        int threshold = qMax(20, QGuiApplication::styleHints()->startDragDistance() + 5);
        const bool overThreshold = QQuickWindowPrivate::dragOverThreshold(distance, Qt::XAxis, event, threshold);
        if (window && overThreshold) {
            QQuickItem *grabber = q->window()->mouseGrabberItem();
            if (!grabber || !grabber->keepMouseGrab()) {
                q->grabMouse();
                q->setKeepMouseGrab(overThreshold);
                q->setPressed(true);
                swipe.setComplete(false);
            }
        }
    }

    if (q->keepMouseGrab()) {
        // Ensure we don't try to calculate a position when the user tried to drag
        // to the left when the left item is already exposed, and vice versa.
        // The code below assumes that the drag is valid, so if we don't have this check,
        // the wrong items are visible and the swiping wraps.
        if (swipePrivate->behind
            || ((swipePrivate->left || swipePrivate->right)
                && (qFuzzyIsNull(swipePrivate->positionBeforePress)
                    || (swipePrivate->positionBeforePress == -1.0 && distance >= 0.0)
                    || (swipePrivate->positionBeforePress == 1.0 && distance <= 0.0)))) {

            // We must instantiate the items here so that we can calculate the
            // position against the width of the relevant item.
            QQuickItem *relevantItem = swipePrivate->createRelevantItemForDistance(distance);
            // If there isn't any relevant item, the user may have swiped back to the 0 position,
            // or they swiped back to a position that is equal to positionBeforePress.
            const qreal normalizedDistance = relevantItem ? distance / relevantItem->width() : 0.0;
            qreal position = 0;

            // If the control was exposed before the drag begun, the distance should be inverted.
            // For example, if the control had been swiped to the right, the position would be 1.0.
            // If the control was then swiped the left by a distance of -20 pixels, the normalized
            // distance might be -0.2, for example, which cannot be used as the position; the swipe
            // started from the right, so we account for that by adding the position.
            if (qFuzzyIsNull(normalizedDistance)) {
                // There are two cases when the normalizedDistance can be 0,
                // and we must distinguish between them:
                //
                // a) The swipe returns to the position that it was at before the press event.
                // In this case, the distance will be 0.
                // There would have been many position changes in the meantime, so we can't just
                // ignore the move event; we have to set position to what it was before the press.
                //
                // b) If the position was at, 1.0, for example, and the control was then swiped
                // to the left by the exact width of the left item, there won't be any relevant item
                // (because the swipe's position would be at 0.0). In turn, the normalizedDistance
                // would be 0 (because of the lack of a relevant item), but the distance will be non-zero.
                position = qFuzzyIsNull(distance) ? swipePrivate->positionBeforePress : 0;
            } else if (!swipePrivate->wasComplete) {
                position = normalizedDistance;
            } else {
                position = distance > 0 ? normalizedDistance - 1.0 : normalizedDistance + 1.0;
            }

            swipe.setPosition(position);
        }
    }

    event->accept();

    return q->keepMouseGrab();
}

static const qreal exposeVelocityThreshold = 300.0;

bool QQuickSwipeDelegatePrivate::handleMouseReleaseEvent(QQuickItem *, QMouseEvent *event)
{
    Q_Q(QQuickSwipeDelegate);
    QQuickSwipePrivate *swipePrivate = QQuickSwipePrivate::get(&swipe);
    swipePrivate->velocityCalculator.stopMeasuring(event->pos(), event->timestamp());

    // The control can be exposed by either swiping past the halfway mark, or swiping fast enough.
    const qreal swipeVelocity = swipePrivate->velocityCalculator.velocity().x();
    if (swipePrivate->position > 0.5 ||
        (swipePrivate->position > 0.0 && swipeVelocity > exposeVelocityThreshold)) {
        swipe.setPosition(1.0);
        swipe.setComplete(true);
        swipePrivate->wasComplete = true;
    } else if (swipePrivate->position < -0.5 ||
        (swipePrivate->position < 0.0 && swipeVelocity < -exposeVelocityThreshold)) {
        swipe.setPosition(-1.0);
        swipe.setComplete(true);
        swipePrivate->wasComplete = true;
    } else {
        swipe.setPosition(0.0);
        swipe.setComplete(false);
        swipePrivate->wasComplete = false;
    }

    q->setKeepMouseGrab(false);

    return true;
}

void QQuickSwipeDelegatePrivate::resizeContent()
{
    // If the background and contentItem are outside the visible bounds
    // of the control (we clip anything outside the bounds), we don't want
    // to call QQuickControlPrivate's implementation of this function,
    // as it repositions the contentItem to be visible.
    QQuickSwipePrivate *swipePrivate = QQuickSwipePrivate::get(&swipe);
    if (!swipePrivate->complete) {
        QQuickAbstractButtonPrivate::resizeContent();
    }
}

QQuickSwipeDelegate::QQuickSwipeDelegate(QQuickItem *parent) :
    QQuickItemDelegate(*(new QQuickSwipeDelegatePrivate(this)), parent)
{
    setFiltersChildMouseEvents(true);
}

/*!
    \qmlpropertygroup QtQuick.Controls::SwipeDelegate::swipe
    \qmlproperty real QtQuick.Controls::SwipeDelegate::swipe.position
    \qmlproperty bool QtQuick.Controls::SwipeDelegate::swipe.complete
    \qmlproperty Component QtQuick.Controls::SwipeDelegate::swipe.left
    \qmlproperty Component QtQuick.Controls::SwipeDelegate::swipe.behind
    \qmlproperty Component QtQuick.Controls::SwipeDelegate::swipe.right
    \qmlproperty Item QtQuick.Controls::SwipeDelegate::swipe.leftItem
    \qmlproperty Item QtQuick.Controls::SwipeDelegate::swipe.behindItem
    \qmlproperty Item QtQuick.Controls::SwipeDelegate::swipe.rightItem

    \table
    \header
        \li Property
        \li Description
    \row
        \li position
        \li This property holds the position of the swipe relative to either
            side of the control. When this value reaches either
            \c -1.0 (left side) or \c 1.0 (right side) and the mouse button is
            released, \c complete will be \c true.
    \row
        \li complete
        \li This property holds whether the control is fully exposed after
            having been swiped to the left or right.

            When complete is \c true, any interactive items declared in \c left,
            \c right, or \c behind will receive mouse events.
    \row
        \li left
        \li This property holds the left delegate.

            The left delegate sits behind both \l {Control::}{contentItem} and
            \l {Control::}{background}. When the SwipeDelegate is swiped to the right,
            this item will be gradually revealed.
    \row
        \li behind
        \li This property holds the delegate that is shown when the
            SwipeDelegate is swiped to both the left and right.

            As with the \c left and \c right delegates, it sits behind both
            \l {Control::}{contentItem} and \l {Control::}{background}. However, a
            SwipeDelegate whose \c behind has been set can be continuously swiped
            from either side, and will always show the same item.
    \row
        \li right
        \li This property holds the right delegate.

            The right delegate sits behind both \l {Control::}{contentItem} and
            \l {Control::}{background}. When the SwipeDelegate is swiped to the left,
            this item will be gradually revealed.
    \row
        \li leftItem
        \li This property holds the item instantiated from the \c left component.

            If \c left has not been set, or the position hasn't changed since
            creation of the SwipeDelegate, this property will be \c null.
    \row
        \li behindItem
        \li This property holds the item instantiated from the \c behind component.

            If \c behind has not been set, or the position hasn't changed since
            creation of the SwipeDelegate, this property will be \c null.
    \row
        \li rightItem
        \li This property holds the item instantiated from the \c right component.

            If \c right has not been set, or the position hasn't changed since
            creation of the SwipeDelegate, this property will be \c null.
    \endtable

    \sa {Control::}{contentItem}, {Control::}{background}
*/
QQuickSwipe *QQuickSwipeDelegate::swipe() const
{
    Q_D(const QQuickSwipeDelegate);
    return const_cast<QQuickSwipe*>(&d->swipe);
}

static bool isChildOrGrandchildOf(QQuickItem *child, QQuickItem *item)
{
    return item && (child == item || item->isAncestorOf(child));
}

bool QQuickSwipeDelegate::childMouseEventFilter(QQuickItem *child, QEvent *event)
{
    Q_D(QQuickSwipeDelegate);
    // The contentItem is, by default, usually a non-interactive item like Text, and
    // the same applies to the background. This means that simply stacking the left/right/behind
    // items before these items won't allow us to get mouse events when the control is not currently exposed
    // but has been previously. Therefore, we instead call setFiltersChildMouseEvents(true) in the constructor
    // and filter out child events only when the child is the left/right/behind item.
    const QQuickSwipePrivate *swipePrivate = QQuickSwipePrivate::get(&d->swipe);
    if (!isChildOrGrandchildOf(child, swipePrivate->leftItem) && !isChildOrGrandchildOf(child, swipePrivate->behindItem)
        && !isChildOrGrandchildOf(child, swipePrivate->rightItem)) {
        return false;
    }

    switch (event->type()) {
    case QEvent::MouseButtonPress: {
        return d->handleMousePressEvent(child, static_cast<QMouseEvent *>(event));
    } case QEvent::MouseMove: {
        return d->handleMouseMoveEvent(child, static_cast<QMouseEvent *>(event));
    } case QEvent::MouseButtonRelease: {
        // Make sure that the control gets release events if it has created child
        // items that are stealing events from it.
        QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);
        QQuickItemDelegate::mouseReleaseEvent(mouseEvent);
        return d->handleMouseReleaseEvent(child, mouseEvent);
    } default:
        return false;
    }
}

// We only override this to set positionBeforePress;
// otherwise, it's the same as the base class implementation.
void QQuickSwipeDelegate::mousePressEvent(QMouseEvent *event)
{
    Q_D(QQuickSwipeDelegate);
    QQuickItemDelegate::mousePressEvent(event);
    QQuickSwipePrivate *swipePrivate = QQuickSwipePrivate::get(&d->swipe);
    swipePrivate->positionBeforePress = swipePrivate->position;
    swipePrivate->velocityCalculator.startMeasuring(event->pos(), event->timestamp());
}

void QQuickSwipeDelegate::mouseMoveEvent(QMouseEvent *event)
{
    Q_D(QQuickSwipeDelegate);
    d->handleMouseMoveEvent(this, event);
}

void QQuickSwipeDelegate::mouseReleaseEvent(QMouseEvent *event)
{
    Q_D(QQuickSwipeDelegate);
    QQuickItemDelegate::mouseReleaseEvent(event);
    d->handleMouseReleaseEvent(this, event);
}

QFont QQuickSwipeDelegate::defaultFont() const
{
    return QQuickControlPrivate::themeFont(QPlatformTheme::ListViewFont);
}

#ifndef QT_NO_ACCESSIBILITY
QAccessible::Role QQuickSwipeDelegate::accessibleRole() const
{
    return QAccessible::ListItem;
}
#endif

QT_END_NAMESPACE
