/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the QtQml module of the Qt Toolkit.
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

#include "qquickmousearea_p.h"
#include "qquickmousearea_p_p.h"
#include "qquickcanvas.h"
#include "qquickevents_p_p.h"
#include "qquickdrag_p.h"

#include <private/qqmldata_p.h>

#include <QtGui/qevent.h>
#include <QtGui/qguiapplication.h>
#include <QtGui/qstylehints.h>

#include <float.h>

QT_BEGIN_NAMESPACE

DEFINE_BOOL_CONFIG_OPTION(qmlVisualTouchDebugging, QML_VISUAL_TOUCH_DEBUGGING)

static const int PressAndHoldDelay = 800;

QQuickDrag::QQuickDrag(QObject *parent)
: QObject(parent), _target(0), _axis(XandYAxis), _xmin(-FLT_MAX),
_xmax(FLT_MAX), _ymin(-FLT_MAX), _ymax(FLT_MAX), _active(false), _filterChildren(false)
{
}

QQuickDrag::~QQuickDrag()
{
}

QQuickItem *QQuickDrag::target() const
{
    return _target;
}

void QQuickDrag::setTarget(QQuickItem *t)
{
    if (_target == t)
        return;
    _target = t;
    emit targetChanged();
}

void QQuickDrag::resetTarget()
{
    if (_target == 0)
        return;
    _target = 0;
    emit targetChanged();
}

QQuickDrag::Axis QQuickDrag::axis() const
{
    return _axis;
}

void QQuickDrag::setAxis(QQuickDrag::Axis a)
{
    if (_axis == a)
        return;
    _axis = a;
    emit axisChanged();
}

qreal QQuickDrag::xmin() const
{
    return _xmin;
}

void QQuickDrag::setXmin(qreal m)
{
    if (_xmin == m)
        return;
    _xmin = m;
    emit minimumXChanged();
}

qreal QQuickDrag::xmax() const
{
    return _xmax;
}

void QQuickDrag::setXmax(qreal m)
{
    if (_xmax == m)
        return;
    _xmax = m;
    emit maximumXChanged();
}

qreal QQuickDrag::ymin() const
{
    return _ymin;
}

void QQuickDrag::setYmin(qreal m)
{
    if (_ymin == m)
        return;
    _ymin = m;
    emit minimumYChanged();
}

qreal QQuickDrag::ymax() const
{
    return _ymax;
}

void QQuickDrag::setYmax(qreal m)
{
    if (_ymax == m)
        return;
    _ymax = m;
    emit maximumYChanged();
}

bool QQuickDrag::active() const
{
    return _active;
}

void QQuickDrag::setActive(bool drag)
{
    if (_active == drag)
        return;
    _active = drag;
    emit activeChanged();
}

bool QQuickDrag::filterChildren() const
{
    return _filterChildren;
}

void QQuickDrag::setFilterChildren(bool filter)
{
    if (_filterChildren == filter)
        return;
    _filterChildren = filter;
    emit filterChildrenChanged();
}

QQuickDragAttached *QQuickDrag::qmlAttachedProperties(QObject *obj)
{
    return new QQuickDragAttached(obj);
}

QQuickMouseAreaPrivate::QQuickMouseAreaPrivate()
: enabled(true), hovered(false), pressed(false), longPress(false),
  moved(false), dragX(true), dragY(true), stealMouse(false), doubleClick(false), preventStealing(false),
  propagateComposedEvents(false), drag(0)
{
}

QQuickMouseAreaPrivate::~QQuickMouseAreaPrivate()
{
    delete drag;
}

void QQuickMouseAreaPrivate::init()
{
    Q_Q(QQuickMouseArea);
    q->setAcceptedMouseButtons(Qt::LeftButton);
    q->setFiltersChildMouseEvents(true);
    if (qmlVisualTouchDebugging()) {
        q->setFlag(QQuickItem::ItemHasContents);
    }
}

void QQuickMouseAreaPrivate::saveEvent(QMouseEvent *event)
{
    lastPos = event->localPos();
    lastScenePos = event->windowPos();
    lastButton = event->button();
    lastButtons = event->buttons();
    lastModifiers = event->modifiers();
}

bool QQuickMouseAreaPrivate::isPressAndHoldConnected()
{
    Q_Q(QQuickMouseArea);
    IS_SIGNAL_CONNECTED(q, "pressAndHold(QQuickMouseEvent*)");
}

bool QQuickMouseAreaPrivate::isDoubleClickConnected()
{
    Q_Q(QQuickMouseArea);
    IS_SIGNAL_CONNECTED(q, "doubleClicked(QQuickMouseEvent*)");
}

bool QQuickMouseAreaPrivate::isClickConnected()
{
    Q_Q(QQuickMouseArea);
    IS_SIGNAL_CONNECTED(q, "clicked(QQuickMouseEvent*)");
}

bool QQuickMouseAreaPrivate::isWheelConnected()
{
    Q_Q(QQuickMouseArea);
    IS_SIGNAL_CONNECTED(q, "wheel(QQuickWheelEvent*)");
}

void QQuickMouseAreaPrivate::propagate(QQuickMouseEvent* event, PropagateType t)
{
    Q_Q(QQuickMouseArea);
    if (!propagateComposedEvents)
        return;
    QPointF scenePos = q->mapToScene(QPointF(event->x(), event->y()));
    propagateHelper(event, canvas->rootItem(), scenePos, t);
}

bool QQuickMouseAreaPrivate::propagateHelper(QQuickMouseEvent *ev, QQuickItem *item,const QPointF &sp, PropagateType sig)
{
    //Based off of QQuickCanvas::deliverInitialMousePressEvent
    //But specific to MouseArea, so doesn't belong in canvas
    Q_Q(const QQuickMouseArea);
    QQuickItemPrivate *itemPrivate = QQuickItemPrivate::get(item);
    if (itemPrivate->opacity() == 0.0)
        return false;

    if (itemPrivate->flags & QQuickItem::ItemClipsChildrenToShape) {
        QPointF p = item->mapFromScene(sp);
        if (!item->contains(p))
            return false;
    }

    QList<QQuickItem *> children = itemPrivate->paintOrderChildItems();
    for (int ii = children.count() - 1; ii >= 0; --ii) {
        QQuickItem *child = children.at(ii);
        if (!child->isVisible() || !child->isEnabled())
            continue;
        if (propagateHelper(ev, child, sp, sig))
            return true;
    }

    QQuickMouseArea* ma = qobject_cast<QQuickMouseArea*>(item);
    if (ma && ma != q && itemPrivate->acceptedMouseButtons() & ev->button()) {
        switch (sig) {
        case Click:
            if (!ma->d_func()->isClickConnected())
                return false;
            break;
        case DoubleClick:
            if (!ma->d_func()->isDoubleClickConnected())
                return false;
            break;
        case PressAndHold:
            if (!ma->d_func()->isPressAndHoldConnected())
                return false;
            break;
        }
        QPointF p = item->mapFromScene(sp);
        if (item->contains(p)) {
            ev->setX(p.x());
            ev->setY(p.y());
            ev->setAccepted(true);//It is connected, they have to explicitly ignore to let it slide
            switch (sig) {
            case Click: emit ma->clicked(ev); break;
            case DoubleClick: emit ma->doubleClicked(ev); break;
            case PressAndHold: emit ma->pressAndHold(ev); break;
            }
            if (ev->isAccepted())
                return true;
        }
    }
    return false;

}

/*!
    \qmlclass MouseArea QQuickMouseArea
    \inqmlmodule QtQuick 2
    \ingroup qtquick-interaction
    \brief Enables simple mouse handling
    \inherits Item

    A MouseArea is an invisible item that is typically used in conjunction with
    a visible item in order to provide mouse handling for that item.
    By effectively acting as a proxy, the logic for mouse handling can be
    contained within a MouseArea item.

    For basic key handling, see the \l{Keys}{Keys attached property}.

    The \l enabled property is used to enable and disable mouse handling for
    the proxied item. When disabled, the mouse area becomes transparent to
    mouse events.

    The \l pressed read-only property indicates whether or not the user is
    holding down a mouse button over the mouse area. This property is often
    used in bindings between properties in a user interface. The containsMouse
    read-only property indicates the presence of the mouse cursor over the
    mouse area but, by default, only when a mouse button is held down; see below
    for further details.

    Information about the mouse position and button clicks are provided via
    signals for which event handler properties are defined. The most commonly
    used involved handling mouse presses and clicks: onClicked, onDoubleClicked,
    onPressed, onReleased and onPressAndHold. It's also possible to handle mouse
    wheel events via the onWheel signal.

    By default, MouseArea items only report mouse clicks and not changes to the
    position of the mouse cursor. Setting the hoverEnabled property ensures that
    handlers defined for onPositionChanged, onEntered and onExited are used and
    that the containsMouse property is updated even when no mouse buttons are
    pressed.

    \section1 Example Usage

    \div {class="float-right"}
    \inlineimage qml-mousearea-snippet.png
    \enddiv

    The following example uses a MouseArea in a \l Rectangle that changes
    the \l Rectangle color to red when clicked:

    \snippet qml/mousearea/mousearea.qml import
    \codeline
    \snippet qml/mousearea/mousearea.qml intro

    \clearfloat
    Many MouseArea signals pass a \l{MouseEvent}{mouse} parameter that contains
    additional information about the mouse event, such as the position, button,
    and any key modifiers.

    Here is an extension of the previous example that produces a different
    color when the area is right clicked:

    \snippet qml/mousearea/mousearea.qml intro-extended

  Behavioral Change in QtQuick 2.0

  From QtQuick 2.0, the signals clicked, doubleClicked and pressAndHold have a different interaction
  model with regards to the delivery of events to multiple overlapping MouseAreas. These signals can now propagate
  to all MouseAreas in the area, in painting order, until accepted by one of them. A signal is accepted by
  default if there is a signal handler for it, use mouse.accepted = false; to ignore. This propagation
  can send the signal to MouseAreas other than the one which accepted the press event, although that MouseArea
  will receive the signal first. This behavior can be enabled by setting propagateComposedEvents to true.

    \sa MouseEvent, {declarative/touchinteraction/mousearea}{MouseArea example}
*/

/*!
    \qmlsignal QtQuick2::MouseArea::onEntered()

    This handler is called when the mouse enters the mouse area.

    By default the onEntered handler is only called while a button is
    pressed. Setting hoverEnabled to true enables handling of
    onEntered when no mouse button is pressed.

    \sa hoverEnabled
*/

/*!
    \qmlsignal QtQuick2::MouseArea::onExited()

    This handler is called when the mouse exits the mouse area.

    By default the onExited handler is only called while a button is
    pressed. Setting hoverEnabled to true enables handling of
    onExited when no mouse button is pressed.

    The example below shows a fairly typical relationship between
    two MouseAreas, with \c mouseArea2 on top of \c mouseArea1. Moving the
    mouse into \c mouseArea2 from \c mouseArea1 will cause \c onExited
    to be called for \c mouseArea1.
    \qml
    Rectangle {
        width: 400; height: 400
        MouseArea {
            id: mouseArea1
            anchors.fill: parent
            hoverEnabled: true
        }
        MouseArea {
            id: mouseArea2
            width: 100; height: 100
            anchors.centerIn: parent
            hoverEnabled: true
        }
    }
    \endqml

    If instead you give the two mouseAreas a parent-child relationship,
    moving the mouse into \c mouseArea2 from \c mouseArea1 will \b not
    cause \c onExited to be called for \c mouseArea1. Instead, they will
    both be considered to be simultaneously hovered.

    \sa hoverEnabled
*/

/*!
    \qmlsignal QtQuick2::MouseArea::onPositionChanged(MouseEvent mouse)

    This handler is called when the mouse position changes.

    The \l {MouseEvent}{mouse} parameter provides information about the mouse, including the x and y
    position, and any buttons currently pressed.

    The \e accepted property of the MouseEvent parameter is ignored in this handler.

    By default the onPositionChanged handler is only called while a button is
    pressed.  Setting hoverEnabled to true enables handling of
    onPositionChanged when no mouse button is pressed.
*/

/*!
    \qmlsignal QtQuick2::MouseArea::onClicked(MouseEvent mouse)

    This handler is called when there is a click. A click is defined as a press followed by a release,
    both inside the MouseArea (pressing, moving outside the MouseArea, and then moving back inside and
    releasing is also considered a click).

    The \l {MouseEvent}{mouse} parameter provides information about the click, including the x and y
    position of the release of the click, and whether the click was held.

    The \e accepted property of the MouseEvent parameter is ignored in this handler.
*/

/*!
    \qmlsignal QtQuick2::MouseArea::onPressed(MouseEvent mouse)

    This handler is called when there is a press.
    The \l {MouseEvent}{mouse} parameter provides information about the press, including the x and y
    position and which button was pressed.

    The \e accepted property of the MouseEvent parameter determines whether this MouseArea
    will handle the press and all future mouse events until release.  The default is to accept
    the event and not allow other MouseArea beneath this one to handle the event.  If \e accepted
    is set to false, no further events will be sent to this MouseArea until the button is next
    pressed.
*/

/*!
    \qmlsignal QtQuick2::MouseArea::onReleased(MouseEvent mouse)

    This handler is called when there is a release.
    The \l {MouseEvent}{mouse} parameter provides information about the click, including the x and y
    position of the release of the click, and whether the click was held.

    The \e accepted property of the MouseEvent parameter is ignored in this handler.

    \sa onCanceled
*/

/*!
    \qmlsignal QtQuick2::MouseArea::onPressAndHold(MouseEvent mouse)

    This handler is called when there is a long press (currently 800ms).
    The \l {MouseEvent}{mouse} parameter provides information about the press, including the x and y
    position of the press, and which button is pressed.

    The \e accepted property of the MouseEvent parameter is ignored in this handler.
*/

/*!
    \qmlsignal QtQuick2::MouseArea::onDoubleClicked(MouseEvent mouse)

    This handler is called when there is a double-click (a press followed by a release followed by a press).
    The \l {MouseEvent}{mouse} parameter provides information about the click, including the x and y
    position of the release of the click, and whether the click was held.

    If the \e accepted property of the \l {MouseEvent}{mouse} parameter is set to false
    in the handler, the onPressed/onReleased/onClicked handlers will be called for the second
    click; otherwise they are suppressed.  The accepted property defaults to true.
*/

/*!
    \qmlsignal QtQuick2::MouseArea::onCanceled()

    This handler is called when mouse events have been canceled, either because an event was not accepted, or
    because another element stole the mouse event handling.

    This signal is for advanced use: it is useful when there is more than one MouseArea
    that is handling input, or when there is a MouseArea inside a \l Flickable. In the latter
    case, if you execute some logic on the pressed signal and then start dragging, the
    \l Flickable will steal the mouse handling from the MouseArea. In these cases, to reset
    the logic when the MouseArea has lost the mouse handling to the \l Flickable,
    \c onCanceled should be used in addition to onReleased.
*/

/*!
    \qmlsignal QtQuick2::MouseArea::onWheel(WheelEvent mouse)

    This handler is called in response to both mouse wheel and trackpad scroll gestures.

    The \l {WheelEvent}{wheel} parameter provides information about the event, including the x and y
    position, any buttons currently pressed, and information about the wheel movement, including
    angleDelta and pixelDelta.
*/

QQuickMouseArea::QQuickMouseArea(QQuickItem *parent)
  : QQuickItem(*(new QQuickMouseAreaPrivate), parent)
{
    Q_D(QQuickMouseArea);
    d->init();
}

QQuickMouseArea::~QQuickMouseArea()
{
}

/*!
    \qmlproperty real QtQuick2::MouseArea::mouseX
    \qmlproperty real QtQuick2::MouseArea::mouseY
    These properties hold the coordinates of the mouse cursor.

    If the hoverEnabled property is false then these properties will only be valid
    while a button is pressed, and will remain valid as long as the button is held
    down even if the mouse is moved outside the area.

    By default, this property is false.

    If hoverEnabled is true then these properties will be valid when:
    \list
        \li no button is pressed, but the mouse is within the MouseArea (containsMouse is true).
        \li a button is pressed and held, even if it has since moved out of the area.
    \endlist

    The coordinates are relative to the MouseArea.
*/
qreal QQuickMouseArea::mouseX() const
{
    Q_D(const QQuickMouseArea);
    return d->lastPos.x();
}

qreal QQuickMouseArea::mouseY() const
{
    Q_D(const QQuickMouseArea);
    return d->lastPos.y();
}

/*!
    \qmlproperty bool QtQuick2::MouseArea::enabled
    This property holds whether the item accepts mouse events.

    By default, this property is true.
*/
bool QQuickMouseArea::isEnabled() const
{
    Q_D(const QQuickMouseArea);
    return d->enabled;
}

void QQuickMouseArea::setEnabled(bool a)
{
    Q_D(QQuickMouseArea);
    if (a != d->enabled) {
        d->enabled = a;
        emit enabledChanged();
    }
}

/*!
    \qmlproperty bool QtQuick2::MouseArea::preventStealing
    This property holds whether the mouse events may be stolen from this
    MouseArea.

    If a MouseArea is placed within an item that filters child mouse
    events, such as Flickable, the mouse
    events may be stolen from the MouseArea if a gesture is recognized
    by the parent element, e.g. a flick gesture.  If preventStealing is
    set to true, no element will steal the mouse events.

    Note that setting preventStealing to true once an element has started
    stealing events will have no effect until the next press event.

    By default this property is false.
*/
bool QQuickMouseArea::preventStealing() const
{
    Q_D(const QQuickMouseArea);
    return d->preventStealing;
}

void QQuickMouseArea::setPreventStealing(bool prevent)
{
    Q_D(QQuickMouseArea);
    if (prevent != d->preventStealing) {
        d->preventStealing = prevent;
        setKeepMouseGrab(d->preventStealing && d->enabled);
        emit preventStealingChanged();
    }
}


/*!
    \qmlproperty bool QtQuick2::MouseArea::propagateComposedEvents
    This property holds whether composed mouse events will automatically propagate to
    other MouseAreas.

    MouseArea contains several composed events, clicked, doubleClicked,
    and pressAndHold. These can propagate via a separate mechanism to basic
    mouse events, like pressed, which they are composed of.

    If propagateComposedEvents is set to true, then composed events will be automatically
    propagated to other MouseAreas in the same location in the scene. They are propagated
    in painting order until an item accepts them. Unlike pressed handling, events will
    not be automatically accepted if no handler is present.

    This property greatly simplifies the usecase of when you want to have overlapping MouseAreas
    handling the composed events together. For example: if you want one MouseArea to handle click
    signals and the other to handle pressAndHold, or if you want one MouseArea to handle click most
    of the time, but pass it through when certain conditions are met.

    By default this property is false.
*/
bool QQuickMouseArea::propagateComposedEvents() const
{
    Q_D(const QQuickMouseArea);
    return d->propagateComposedEvents;
}

void QQuickMouseArea::setPropagateComposedEvents(bool prevent)
{
    Q_D(QQuickMouseArea);
    if (prevent != d->propagateComposedEvents) {
        d->propagateComposedEvents = prevent;
        setKeepMouseGrab(d->propagateComposedEvents && d->enabled);
        emit propagateComposedEventsChanged();
    }
}

/*!
    \qmlproperty MouseButtons QtQuick2::MouseArea::pressedButtons
    This property holds the mouse buttons currently pressed.

    It contains a bitwise combination of:
    \list
    \li Qt.LeftButton
    \li Qt.RightButton
    \li Qt.MiddleButton
    \endlist

    The code below displays "right" when the right mouse buttons is pressed:

    \snippet qml/mousearea/mousearea.qml mousebuttons

    \sa acceptedButtons
*/
Qt::MouseButtons QQuickMouseArea::pressedButtons() const
{
    Q_D(const QQuickMouseArea);
    return d->lastButtons;
}

void QQuickMouseArea::mousePressEvent(QMouseEvent *event)
{
    Q_D(QQuickMouseArea);
    d->moved = false;
    d->stealMouse = d->preventStealing;
    if (!d->enabled)
        QQuickItem::mousePressEvent(event);
    else {
        d->longPress = false;
        d->saveEvent(event);
        if (d->drag)
            d->drag->setActive(false);
        setHovered(true);
        d->startScene = event->windowPos();
        d->pressAndHoldTimer.start(PressAndHoldDelay, this);
        setKeepMouseGrab(d->stealMouse);
        event->setAccepted(setPressed(true));

        if (d->drag) {
            d->dragX = drag()->axis() & QQuickDrag::XAxis;
            d->dragY = drag()->axis() & QQuickDrag::YAxis;
        }
    }
}

void QQuickMouseArea::mouseMoveEvent(QMouseEvent *event)
{
    Q_D(QQuickMouseArea);
    if (!d->enabled && !d->pressed) {
        QQuickItem::mouseMoveEvent(event);
        return;
    }

    d->saveEvent(event);

    // ### we should skip this if these signals aren't used
    // ### can GV handle this for us?
    const bool isInside = contains(d->lastPos);
    if (d->hovered && !isInside)
        setHovered(false);
    else if (!d->hovered && isInside)
        setHovered(true);

    if (d->drag && d->drag->target()) {
        if (!d->moved) {
            d->targetStartPos = d->drag->target()->parentItem()
                    ? d->drag->target()->parentItem()->mapToScene(d->drag->target()->pos())
                    : d->drag->target()->pos();
        }

        QPointF startLocalPos;
        QPointF curLocalPos;
        if (drag()->target()->parentItem()) {
            startLocalPos = drag()->target()->parentItem()->mapFromScene(d->startScene);
            curLocalPos = drag()->target()->parentItem()->mapFromScene(event->windowPos());
        } else {
            startLocalPos = d->startScene;
            curLocalPos = event->windowPos();
        }

        qreal dx = qAbs(curLocalPos.x() - startLocalPos.x());
        qreal dy = qAbs(curLocalPos.y() - startLocalPos.y());

        if (keepMouseGrab() && d->stealMouse && !d->drag->active())
            d->drag->setActive(true);

        QPointF startPos = d->drag->target()->parentItem()
                ? d->drag->target()->parentItem()->mapFromScene(d->targetStartPos)
                : d->targetStartPos;

        QPointF dragPos = d->drag->target()->pos();

        if (d->dragX && d->drag->active()) {
            qreal x = (curLocalPos.x() - startLocalPos.x()) + startPos.x();
            if (x < drag()->xmin())
                x = drag()->xmin();
            else if (x > drag()->xmax())
                x = drag()->xmax();
            dragPos.setX(x);
        }
        if (d->dragY && d->drag->active()) {
            qreal y = (curLocalPos.y() - startLocalPos.y()) + startPos.y();
            if (y < drag()->ymin())
                y = drag()->ymin();
            else if (y > drag()->ymax())
                y = drag()->ymax();
            dragPos.setY(y);
        }
        d->drag->target()->setPos(dragPos);

        if (!keepMouseGrab()) {
            bool xDragged = QQuickCanvasPrivate::dragOverThreshold(dx, Qt::XAxis, event);
            bool yDragged = QQuickCanvasPrivate::dragOverThreshold(dy, Qt::YAxis, event);
            if ((!d->dragY && !yDragged && d->dragX && xDragged)
                || (!d->dragX && !xDragged && d->dragY && yDragged)
                || (d->dragX && d->dragY && (xDragged || yDragged))) {
                setKeepMouseGrab(true);
                d->stealMouse = true;
            }
        }

        d->moved = true;
    }
    QQuickMouseEvent me(d->lastPos.x(), d->lastPos.y(), d->lastButton, d->lastButtons, d->lastModifiers, false, d->longPress);
    emit mouseXChanged(&me);
    me.setPosition(d->lastPos);
    emit mouseYChanged(&me);
    me.setPosition(d->lastPos);
    emit positionChanged(&me);
}

void QQuickMouseArea::mouseReleaseEvent(QMouseEvent *event)
{
    Q_D(QQuickMouseArea);
    d->stealMouse = false;
    if (!d->enabled && !d->pressed) {
        QQuickItem::mouseReleaseEvent(event);
    } else {
        d->saveEvent(event);
        setPressed(false);
        if (d->drag)
            d->drag->setActive(false);
        // If we don't accept hover, we need to reset containsMouse.
        if (!acceptHoverEvents())
            setHovered(false);
        QQuickCanvas *c = canvas();
        if (c && c->mouseGrabberItem() == this)
            ungrabMouse();
        setKeepMouseGrab(false);

    }
    d->doubleClick = false;
}

void QQuickMouseArea::mouseDoubleClickEvent(QMouseEvent *event)
{
    Q_D(QQuickMouseArea);
    if (d->enabled) {
        d->saveEvent(event);
        QQuickMouseEvent me(d->lastPos.x(), d->lastPos.y(), d->lastButton, d->lastButtons, d->lastModifiers, true, false);
        me.setAccepted(d->isDoubleClickConnected());
        emit this->doubleClicked(&me);
        if (!me.isAccepted())
            d->propagate(&me, QQuickMouseAreaPrivate::DoubleClick);
        d->doubleClick = d->isDoubleClickConnected() || me.isAccepted();
    }
    QQuickItem::mouseDoubleClickEvent(event);
}

void QQuickMouseArea::hoverEnterEvent(QHoverEvent *event)
{
    Q_D(QQuickMouseArea);
    if (!d->enabled && !d->pressed) {
        QQuickItem::hoverEnterEvent(event);
    } else {
        d->lastPos = event->posF();
        d->lastModifiers = event->modifiers();
        setHovered(true);
        QQuickMouseEvent me(d->lastPos.x(), d->lastPos.y(), Qt::NoButton, Qt::NoButton, d->lastModifiers, false, false);
        emit mouseXChanged(&me);
        me.setPosition(d->lastPos);
        emit mouseYChanged(&me);
        me.setPosition(d->lastPos);
    }
}

void QQuickMouseArea::hoverMoveEvent(QHoverEvent *event)
{
    Q_D(QQuickMouseArea);
    if (!d->enabled && !d->pressed) {
        QQuickItem::hoverMoveEvent(event);
    } else {
        d->lastPos = event->posF();
        d->lastModifiers = event->modifiers();
        QQuickMouseEvent me(d->lastPos.x(), d->lastPos.y(), Qt::NoButton, Qt::NoButton, d->lastModifiers, false, false);
        emit mouseXChanged(&me);
        me.setPosition(d->lastPos);
        emit mouseYChanged(&me);
        me.setPosition(d->lastPos);
        emit positionChanged(&me);
    }
}

void QQuickMouseArea::hoverLeaveEvent(QHoverEvent *event)
{
    Q_D(QQuickMouseArea);
    if (!d->enabled && !d->pressed)
        QQuickItem::hoverLeaveEvent(event);
    else
        setHovered(false);
}

void QQuickMouseArea::wheelEvent(QWheelEvent *event)
{
    Q_D(QQuickMouseArea);
    if (!d->enabled) {
        QQuickItem::wheelEvent(event);
        return;
    }

    QQuickWheelEvent we(event->posF().x(), event->posF().y(), event->angleDelta(),
                        event->pixelDelta(), event->buttons(), event->modifiers());
    we.setAccepted(d->isWheelConnected());
    emit wheel(&we);
    if (!we.isAccepted())
        QQuickItem::wheelEvent(event);
}

void QQuickMouseArea::ungrabMouse()
{
    Q_D(QQuickMouseArea);
    if (d->pressed) {
        // if our mouse grab has been removed (probably by Flickable), fix our
        // state
        d->pressed = false;
        d->stealMouse = false;
        setKeepMouseGrab(false);
        emit canceled();
        emit pressedChanged();
        if (d->hovered) {
            d->hovered = false;
            emit hoveredChanged();
        }
    }
}

void QQuickMouseArea::mouseUngrabEvent()
{
    ungrabMouse();
}

bool QQuickMouseArea::sendMouseEvent(QMouseEvent *event)
{
    Q_D(QQuickMouseArea);
    QPointF localPos = mapFromScene(event->windowPos());

    QQuickCanvas *c = canvas();
    QQuickItem *grabber = c ? c->mouseGrabberItem() : 0;
    bool stealThisEvent = d->stealMouse;
    if ((stealThisEvent || contains(localPos)) && (!grabber || !grabber->keepMouseGrab())) {
        QMouseEvent mouseEvent(event->type(), localPos, event->windowPos(), event->screenPos(),
                               event->button(), event->buttons(), event->modifiers());
        mouseEvent.setAccepted(false);

        switch (event->type()) {
        case QEvent::MouseMove:
            mouseMoveEvent(&mouseEvent);
            break;
        case QEvent::MouseButtonPress:
            mousePressEvent(&mouseEvent);
            break;
        case QEvent::MouseButtonRelease:
            mouseReleaseEvent(&mouseEvent);
            break;
        default:
            break;
        }
        grabber = c->mouseGrabberItem();
        if (grabber && stealThisEvent && !grabber->keepMouseGrab() && grabber != this)
            grabMouse();

        return stealThisEvent;
    }
    if (event->type() == QEvent::MouseButtonRelease) {
        if (d->pressed) {
            d->pressed = false;
            d->stealMouse = false;
            if (c && c->mouseGrabberItem() == this)
                ungrabMouse();
            emit canceled();
            emit pressedChanged();
            if (d->hovered) {
                d->hovered = false;
                emit hoveredChanged();
            }
        }
    }
    return false;
}

bool QQuickMouseArea::childMouseEventFilter(QQuickItem *i, QEvent *e)
{
    Q_D(QQuickMouseArea);
    if (!d->pressed && (!d->enabled || !isVisible() || !d->drag || !d->drag->filterChildren()))
        return QQuickItem::childMouseEventFilter(i, e);
    switch (e->type()) {
    case QEvent::MouseButtonPress:
    case QEvent::MouseMove:
    case QEvent::MouseButtonRelease:
        return sendMouseEvent(static_cast<QMouseEvent *>(e));
    default:
        break;
    }

    return QQuickItem::childMouseEventFilter(i, e);
}

void QQuickMouseArea::timerEvent(QTimerEvent *event)
{
    Q_D(QQuickMouseArea);
    if (event->timerId() == d->pressAndHoldTimer.timerId()) {
        d->pressAndHoldTimer.stop();
        bool dragged = d->drag && d->drag->active();
        if (d->pressed && dragged == false && d->hovered == true) {
            d->longPress = true;
            QQuickMouseEvent me(d->lastPos.x(), d->lastPos.y(), d->lastButton, d->lastButtons, d->lastModifiers, false, d->longPress);
            me.setAccepted(d->isPressAndHoldConnected());
            emit pressAndHold(&me);
            if (!me.isAccepted())
                d->propagate(&me, QQuickMouseAreaPrivate::PressAndHold);
            if (!me.isAccepted()) // no one handled the long press - allow click
                d->longPress = false;
        }
    }
}

void QQuickMouseArea::windowDeactivateEvent()
{
    ungrabMouse();
    QQuickItem::windowDeactivateEvent();
}

void QQuickMouseArea::geometryChanged(const QRectF &newGeometry,
                                            const QRectF &oldGeometry)
{
    Q_D(QQuickMouseArea);
    QQuickItem::geometryChanged(newGeometry, oldGeometry);

    if (d->lastScenePos.isNull)
        d->lastScenePos = mapToScene(d->lastPos);
    else if (newGeometry.x() != oldGeometry.x() || newGeometry.y() != oldGeometry.y())
        d->lastPos = mapFromScene(d->lastScenePos);
}

void QQuickMouseArea::itemChange(ItemChange change, const ItemChangeData &value)
{
    Q_D(QQuickMouseArea);
    switch (change) {
    case ItemVisibleHasChanged:
        if (acceptHoverEvents() && d->hovered != (isVisible() && isUnderMouse()))
            setHovered(!d->hovered);
        break;
    default:
        break;
    }

    QQuickItem::itemChange(change, value);
}

/*!
    \qmlproperty bool QtQuick2::MouseArea::hoverEnabled
    This property holds whether hover events are handled.

    By default, mouse events are only handled in response to a button event, or when a button is
    pressed.  Hover enables handling of all mouse events even when no mouse button is
    pressed.

    This property affects the containsMouse property and the onEntered, onExited and
    onPositionChanged signals.
*/
bool QQuickMouseArea::hoverEnabled() const
{
    return acceptHoverEvents();
}

void QQuickMouseArea::setHoverEnabled(bool h)
{
    if (h == acceptHoverEvents())
        return;

    setAcceptHoverEvents(h);
    emit hoverEnabledChanged();
}


/*!
    \qmlproperty bool QtQuick2::MouseArea::containsMouse
    This property holds whether the mouse is currently inside the mouse area.

    \warning This property is not updated if the area moves under the mouse: \e containsMouse will not change.
    In addition, if hoverEnabled is false, containsMouse will only be valid when the mouse is pressed.
*/
bool QQuickMouseArea::hovered() const
{
    Q_D(const QQuickMouseArea);
    return d->hovered;
}

/*!
    \qmlproperty bool QtQuick2::MouseArea::pressed
    This property holds whether the mouse area is currently pressed.
*/
bool QQuickMouseArea::pressed() const
{
    Q_D(const QQuickMouseArea);
    return d->pressed;
}

void QQuickMouseArea::setHovered(bool h)
{
    Q_D(QQuickMouseArea);
    if (d->hovered != h) {
        d->hovered = h;
        emit hoveredChanged();
        d->hovered ? emit entered() : emit exited();
    }
}
/*!
    \qmlproperty QtQuick2::Qt::MouseButtons MouseArea::acceptedButtons
    This property holds the mouse buttons that the mouse area reacts to.

    The available buttons are:
    \list
    \li Qt.LeftButton
    \li Qt.RightButton
    \li Qt.MiddleButton
    \endlist

    To accept more than one button the flags can be combined with the
    "|" (or) operator:

    \code
    MouseArea { acceptedButtons: Qt.LeftButton | Qt.RightButton }
    \endcode

    The default value is \c Qt.LeftButton.
*/
Qt::MouseButtons QQuickMouseArea::acceptedButtons() const
{
    return acceptedMouseButtons();
}

void QQuickMouseArea::setAcceptedButtons(Qt::MouseButtons buttons)
{
    if (buttons != acceptedMouseButtons()) {
        setAcceptedMouseButtons(buttons);
        emit acceptedButtonsChanged();
    }
}

bool QQuickMouseArea::setPressed(bool p)
{
    Q_D(QQuickMouseArea);
    bool dragged = d->drag && d->drag->active();
    bool isclick = d->pressed == true && p == false && dragged == false && d->hovered == true;

    if (d->pressed != p) {
        d->pressed = p;
        QQuickMouseEvent me(d->lastPos.x(), d->lastPos.y(), d->lastButton, d->lastButtons, d->lastModifiers, isclick, d->longPress);
        if (d->pressed) {
            if (!d->doubleClick)
                emit pressed(&me);
            me.setPosition(d->lastPos);
            emit mouseXChanged(&me);
            me.setPosition(d->lastPos);
            emit mouseYChanged(&me);
            emit pressedChanged();
        } else {
            emit released(&me);
            me.setPosition(d->lastPos);
            emit pressedChanged();
            if (isclick && !d->longPress && !d->doubleClick){
                me.setAccepted(d->isClickConnected());
                emit clicked(&me);
                if (!me.isAccepted())
                    d->propagate(&me, QQuickMouseAreaPrivate::Click);
            }
        }

        return me.isAccepted();
    }
    return false;
}

/*!
    \qmlproperty Item QtQuick2::MouseArea::drag.target
    \qmlproperty bool QtQuick2::MouseArea::drag.active
    \qmlproperty enumeration QtQuick2::MouseArea::drag.axis
    \qmlproperty real QtQuick2::MouseArea::drag.minimumX
    \qmlproperty real QtQuick2::MouseArea::drag.maximumX
    \qmlproperty real QtQuick2::MouseArea::drag.minimumY
    \qmlproperty real QtQuick2::MouseArea::drag.maximumY
    \qmlproperty bool QtQuick2::MouseArea::drag.filterChildren

    \c drag provides a convenient way to make an item draggable.

    \list
    \li \c drag.target specifies the id of the item to drag.
    \li \c drag.active specifies if the target item is currently being dragged.
    \li \c drag.axis specifies whether dragging can be done horizontally (\c Drag.XAxis), vertically (\c Drag.YAxis), or both (\c Drag.XandYAxis)
    \li \c drag.minimum and \c drag.maximum limit how far the target can be dragged along the corresponding axes.
    \endlist

    The following example displays a \l Rectangle that can be dragged along the X-axis. The opacity
    of the rectangle is reduced when it is dragged to the right.

    \snippet qml/mousearea/mousearea.qml drag

    \note Items cannot be dragged if they are anchored for the requested
    \c drag.axis. For example, if \c anchors.left or \c anchors.right was set
    for \c rect in the above example, it cannot be dragged along the X-axis.
    This can be avoided by settng the anchor value to \c undefined in
    an \l onPressed handler.

    If \c drag.filterChildren is set to true, a drag can override descendant MouseAreas.  This
    enables a parent MouseArea to handle drags, for example, while descendants handle clicks:

    \snippet qml/mousearea/mouseareadragfilter.qml dragfilter

*/

QQuickDrag *QQuickMouseArea::drag()
{
    Q_D(QQuickMouseArea);
    if (!d->drag)
        d->drag = new QQuickDrag;
    return d->drag;
}

QSGNode *QQuickMouseArea::updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *data)
{
    Q_UNUSED(data);
    Q_D(QQuickMouseArea);

    if (!qmlVisualTouchDebugging())
        return 0;

    QSGRectangleNode *rectangle = static_cast<QSGRectangleNode *>(oldNode);
    if (!rectangle) rectangle = d->sceneGraphContext()->createRectangleNode();

    rectangle->setRect(QRectF(0, 0, width(), height()));
    rectangle->setColor(QColor(255, 0, 0, 50));
    rectangle->update();
    return rectangle;
}

QT_END_NAMESPACE
