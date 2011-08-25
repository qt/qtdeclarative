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

#include "QtQuick1/private/qdeclarativemousearea_p.h"
#include "QtQuick1/private/qdeclarativemousearea_p_p.h"

#include "QtQuick1/private/qdeclarativeevents_p_p.h"

#include <QGraphicsSceneMouseEvent>

#include <float.h>

QT_BEGIN_NAMESPACE


static const int PressAndHoldDelay = 800;

QDeclarative1Drag::QDeclarative1Drag(QObject *parent)
: QObject(parent), _target(0), _axis(XandYAxis), _xmin(-FLT_MAX), _xmax(FLT_MAX), _ymin(-FLT_MAX), _ymax(FLT_MAX),
_active(false), _filterChildren(false)
{
}

QDeclarative1Drag::~QDeclarative1Drag()
{
}

QGraphicsObject *QDeclarative1Drag::target() const
{
    return _target;
}

void QDeclarative1Drag::setTarget(QGraphicsObject *t)
{
    if (_target == t)
        return;
    _target = t;
    emit targetChanged();
}

void QDeclarative1Drag::resetTarget()
{
    if (!_target)
        return;
    _target = 0;
    emit targetChanged();
}

QDeclarative1Drag::Axis QDeclarative1Drag::axis() const
{
    return _axis;
}

void QDeclarative1Drag::setAxis(QDeclarative1Drag::Axis a)
{
    if (_axis == a)
        return;
    _axis = a;
    emit axisChanged();
}

qreal QDeclarative1Drag::xmin() const
{
    return _xmin;
}

void QDeclarative1Drag::setXmin(qreal m)
{
    if (_xmin == m)
        return;
    _xmin = m;
    emit minimumXChanged();
}

qreal QDeclarative1Drag::xmax() const
{
    return _xmax;
}

void QDeclarative1Drag::setXmax(qreal m)
{
    if (_xmax == m)
        return;
    _xmax = m;
    emit maximumXChanged();
}

qreal QDeclarative1Drag::ymin() const
{
    return _ymin;
}

void QDeclarative1Drag::setYmin(qreal m)
{
    if (_ymin == m)
        return;
    _ymin = m;
    emit minimumYChanged();
}

qreal QDeclarative1Drag::ymax() const
{
    return _ymax;
}

void QDeclarative1Drag::setYmax(qreal m)
{
    if (_ymax == m)
        return;
    _ymax = m;
    emit maximumYChanged();
}

bool QDeclarative1Drag::active() const
{
    return _active;
}

void QDeclarative1Drag::setActive(bool drag)
{
    if (_active == drag)
        return;
    _active = drag;
    emit activeChanged();
}

bool QDeclarative1Drag::filterChildren() const
{
    return _filterChildren;
}

void QDeclarative1Drag::setFilterChildren(bool filter)
{
    if (_filterChildren == filter)
        return;
    _filterChildren = filter;
    emit filterChildrenChanged();
}

QDeclarative1MouseAreaPrivate::~QDeclarative1MouseAreaPrivate()
{
    delete drag;
}

/*!
    \qmlclass MouseArea QDeclarative1MouseArea
    \inqmlmodule QtQuick 1
    \ingroup qml-basic-interaction-elements
    \since QtQuick 1.0
    \brief The MouseArea item enables simple mouse handling.
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
    onPressed, onReleased and onPressAndHold.

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

    \snippet doc/src/snippets/qtquick1/mousearea/mousearea.qml import
    \codeline
    \snippet doc/src/snippets/qtquick1/mousearea/mousearea.qml intro

    \clearfloat
    Many MouseArea signals pass a \l{MouseEvent}{mouse} parameter that contains
    additional information about the mouse event, such as the position, button,
    and any key modifiers.

    Here is an extension of the previous example that produces a different
    color when the area is right clicked:

    \snippet doc/src/snippets/qtquick1/mousearea/mousearea.qml intro-extended

    \sa MouseEvent, {declarative/touchinteraction/mousearea}{MouseArea example}
*/

/*!
    \qmlsignal QtQuick1::MouseArea::onEntered()

    This handler is called when the mouse enters the mouse area.

    By default the onEntered handler is only called while a button is
    pressed. Setting hoverEnabled to true enables handling of
    onEntered when no mouse button is pressed.

    \sa hoverEnabled
*/

/*!
    \qmlsignal QtQuick1::MouseArea::onExited()

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
    \qmlsignal QtQuick1::MouseArea::onPositionChanged(MouseEvent mouse)

    This handler is called when the mouse position changes.

    The \l {MouseEvent}{mouse} parameter provides information about the mouse, including the x and y
    position, and any buttons currently pressed.

    The \e accepted property of the MouseEvent parameter is ignored in this handler.

    By default the onPositionChanged handler is only called while a button is
    pressed.  Setting hoverEnabled to true enables handling of
    onPositionChanged when no mouse button is pressed.
*/

/*!
    \qmlsignal QtQuick1::MouseArea::onClicked(MouseEvent mouse)

    This handler is called when there is a click. A click is defined as a press followed by a release,
    both inside the MouseArea (pressing, moving outside the MouseArea, and then moving back inside and
    releasing is also considered a click).

    The \l {MouseEvent}{mouse} parameter provides information about the click, including the x and y
    position of the release of the click, and whether the click was held.

    The \e accepted property of the MouseEvent parameter is ignored in this handler.
*/

/*!
    \qmlsignal QtQuick1::MouseArea::onPressed(MouseEvent mouse)

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
    \qmlsignal QtQuick1::MouseArea::onReleased(MouseEvent mouse)

    This handler is called when there is a release.
    The \l {MouseEvent}{mouse} parameter provides information about the click, including the x and y
    position of the release of the click, and whether the click was held.

    The \e accepted property of the MouseEvent parameter is ignored in this handler.

    \sa onCanceled
*/

/*!
    \qmlsignal QtQuick1::MouseArea::onPressAndHold(MouseEvent mouse)

    This handler is called when there is a long press (currently 800ms).
    The \l {MouseEvent}{mouse} parameter provides information about the press, including the x and y
    position of the press, and which button is pressed.

    The \e accepted property of the MouseEvent parameter is ignored in this handler.
*/

/*!
    \qmlsignal QtQuick1::MouseArea::onDoubleClicked(MouseEvent mouse)

    This handler is called when there is a double-click (a press followed by a release followed by a press).
    The \l {MouseEvent}{mouse} parameter provides information about the click, including the x and y
    position of the release of the click, and whether the click was held.

    If the \e accepted property of the \l {MouseEvent}{mouse} parameter is set to false
    in the handler, the onPressed/onReleased/onClicked handlers will be called for the second
    click; otherwise they are suppressed.  The accepted property defaults to true.
*/

/*!
    \qmlsignal QtQuick1::MouseArea::onCanceled()

    This handler is called when mouse events have been canceled, either because an event was not accepted, or
    because another element stole the mouse event handling.

    This signal is for advanced use: it is useful when there is more than one MouseArea
    that is handling input, or when there is a MouseArea inside a \l Flickable. In the latter
    case, if you execute some logic on the pressed signal and then start dragging, the
    \l Flickable will steal the mouse handling from the MouseArea. In these cases, to reset
    the logic when the MouseArea has lost the mouse handling to the \l Flickable,
    \c onCanceled should be used in addition to onReleased.
*/

QDeclarative1MouseArea::QDeclarative1MouseArea(QDeclarativeItem *parent)
  : QDeclarativeItem(*(new QDeclarative1MouseAreaPrivate), parent)
{
    Q_D(QDeclarative1MouseArea);
    d->init();
}

QDeclarative1MouseArea::~QDeclarative1MouseArea()
{
}

/*!
    \qmlproperty real QtQuick1::MouseArea::mouseX
    \qmlproperty real QtQuick1::MouseArea::mouseY
    These properties hold the coordinates of the mouse cursor.

    If the hoverEnabled property is false then these properties will only be valid
    while a button is pressed, and will remain valid as long as the button is held
    down even if the mouse is moved outside the area.

    By default, this property is false.

    If hoverEnabled is true then these properties will be valid when:
    \list
        \i no button is pressed, but the mouse is within the MouseArea (containsMouse is true).
        \i a button is pressed and held, even if it has since moved out of the area.
    \endlist

    The coordinates are relative to the MouseArea.
*/
qreal QDeclarative1MouseArea::mouseX() const
{
    Q_D(const QDeclarative1MouseArea);
    return d->lastPos.x();
}

qreal QDeclarative1MouseArea::mouseY() const
{
    Q_D(const QDeclarative1MouseArea);
    return d->lastPos.y();
}

/*!
    \qmlproperty bool QtQuick1::MouseArea::enabled
    This property holds whether the item accepts mouse events.

    By default, this property is true.
*/
bool QDeclarative1MouseArea::isEnabled() const
{
    Q_D(const QDeclarative1MouseArea);
    return d->absorb;
}

void QDeclarative1MouseArea::setEnabled(bool a)
{
    Q_D(QDeclarative1MouseArea);
    if (a != d->absorb) {
        d->absorb = a;
        emit enabledChanged();
    }
}

/*!
    \qmlproperty bool QtQuick1::MouseArea::preventStealing
    \since Quick 1.1
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
bool QDeclarative1MouseArea::preventStealing() const
{
    Q_D(const QDeclarative1MouseArea);
    return d->preventStealing;
}

void QDeclarative1MouseArea::setPreventStealing(bool prevent)
{
    Q_D(QDeclarative1MouseArea);
    if (prevent != d->preventStealing) {
        d->preventStealing = prevent;
        setKeepMouseGrab(d->preventStealing && d->absorb);
        emit preventStealingChanged();
    }
}

/*!
    \qmlproperty MouseButtons QtQuick1::MouseArea::pressedButtons
    This property holds the mouse buttons currently pressed.

    It contains a bitwise combination of:
    \list
    \o Qt.LeftButton
    \o Qt.RightButton
    \o Qt.MiddleButton
    \endlist

    The code below displays "right" when the right mouse buttons is pressed:

    \snippet doc/src/snippets/qtquick1/mousearea/mousearea.qml mousebuttons

    \sa acceptedButtons
*/
Qt::MouseButtons QDeclarative1MouseArea::pressedButtons() const
{
    Q_D(const QDeclarative1MouseArea);
    return d->lastButtons;
}

void QDeclarative1MouseArea::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    Q_D(QDeclarative1MouseArea);
    d->moved = false;
    d->stealMouse = d->preventStealing;
    if (!d->absorb)
        QDeclarativeItem::mousePressEvent(event);
    else {
        d->longPress = false;
        d->saveEvent(event);
        if (d->drag) {
            d->dragX = drag()->axis() & QDeclarative1Drag::XAxis;
            d->dragY = drag()->axis() & QDeclarative1Drag::YAxis;
        }
        if (d->drag)
            d->drag->setActive(false);
        setHovered(true);
        d->startScene = event->scenePos();
        // we should only start timer if pressAndHold is connected to.
        if (d->isPressAndHoldConnected())
            d->pressAndHoldTimer.start(PressAndHoldDelay, this);
        setKeepMouseGrab(d->stealMouse);
        event->setAccepted(setPressed(true));
    }
}

void QDeclarative1MouseArea::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    Q_D(QDeclarative1MouseArea);
    if (!d->absorb) {
        QDeclarativeItem::mouseMoveEvent(event);
        return;
    }

    d->saveEvent(event);

    // ### we should skip this if these signals aren't used
    // ### can GV handle this for us?
    bool contains = boundingRect().contains(d->lastPos);
    if (d->hovered && !contains)
        setHovered(false);
    else if (!d->hovered && contains)
        setHovered(true);

    if (d->drag && d->drag->target()) {
        if (!d->moved) {
            d->startX = drag()->target()->x();
            d->startY = drag()->target()->y();
        }

        QPointF startLocalPos;
        QPointF curLocalPos;
        if (drag()->target()->parentItem()) {
            startLocalPos = drag()->target()->parentItem()->mapFromScene(d->startScene);
            curLocalPos = drag()->target()->parentItem()->mapFromScene(event->scenePos());
        } else {
            startLocalPos = d->startScene;
            curLocalPos = event->scenePos();
        }

        const int dragThreshold = QApplication::startDragDistance();
        qreal dx = qAbs(curLocalPos.x() - startLocalPos.x());
        qreal dy = qAbs(curLocalPos.y() - startLocalPos.y());

        if (keepMouseGrab() && d->stealMouse)
            d->drag->setActive(true);

        if (d->dragX && d->drag->active()) {
            qreal x = (curLocalPos.x() - startLocalPos.x()) + d->startX;
            if (x < drag()->xmin())
                x = drag()->xmin();
            else if (x > drag()->xmax())
                x = drag()->xmax();
            drag()->target()->setX(x);
        }
        if (d->dragY && d->drag->active()) {
            qreal y = (curLocalPos.y() - startLocalPos.y()) + d->startY;
            if (y < drag()->ymin())
                y = drag()->ymin();
            else if (y > drag()->ymax())
                y = drag()->ymax();
            drag()->target()->setY(y);
        }

        if (!keepMouseGrab()) {
            if ((!d->dragY && dy < dragThreshold && d->dragX && dx > dragThreshold)
                || (!d->dragX && dx < dragThreshold && d->dragY && dy > dragThreshold)
                || (d->dragX && d->dragY && (dx > dragThreshold || dy > dragThreshold))) {
                setKeepMouseGrab(true);
                d->stealMouse = true;
            }
        }

        d->moved = true;
    }
    QDeclarative1MouseEvent me(d->lastPos.x(), d->lastPos.y(), d->lastButton, d->lastButtons, d->lastModifiers, false, d->longPress);
    emit mousePositionChanged(&me);
    me.setX(d->lastPos.x());
    me.setY(d->lastPos.y());
    emit positionChanged(&me);
}


void QDeclarative1MouseArea::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    Q_D(QDeclarative1MouseArea);
    d->stealMouse = false;
    if (!d->absorb) {
        QDeclarativeItem::mouseReleaseEvent(event);
    } else {
        d->saveEvent(event);
        setPressed(false);
        if (d->drag)
            d->drag->setActive(false);
        // If we don't accept hover, we need to reset containsMouse.
        if (!acceptHoverEvents())
            setHovered(false);
        QGraphicsScene *s = scene();
        if (s && s->mouseGrabberItem() == this)
            ungrabMouse();
        setKeepMouseGrab(false);
    }
    d->doubleClick = false;
}

void QDeclarative1MouseArea::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event)
{
    Q_D(QDeclarative1MouseArea);
    if (!d->absorb) {
        QDeclarativeItem::mouseDoubleClickEvent(event);
    } else {
        if (d->isDoubleClickConnected())
            d->doubleClick = true;
        d->saveEvent(event);
        QDeclarative1MouseEvent me(d->lastPos.x(), d->lastPos.y(), d->lastButton, d->lastButtons, d->lastModifiers, true, false);
        me.setAccepted(d->isDoubleClickConnected());
        emit this->doubleClicked(&me);
        QDeclarativeItem::mouseDoubleClickEvent(event);
    }
}

void QDeclarative1MouseArea::hoverEnterEvent(QGraphicsSceneHoverEvent *event)
{
    Q_D(QDeclarative1MouseArea);
    if (!d->absorb)
        QDeclarativeItem::hoverEnterEvent(event);
    else {
        d->lastPos = event->pos();
        setHovered(true);
        QDeclarative1MouseEvent me(d->lastPos.x(), d->lastPos.y(), Qt::NoButton, Qt::NoButton, event->modifiers(), false, false);
        emit mousePositionChanged(&me);
    }
}

void QDeclarative1MouseArea::hoverMoveEvent(QGraphicsSceneHoverEvent *event)
{
    Q_D(QDeclarative1MouseArea);
    if (!d->absorb) {
        QDeclarativeItem::hoverMoveEvent(event);
    } else {
        d->lastPos = event->pos();
        QDeclarative1MouseEvent me(d->lastPos.x(), d->lastPos.y(), Qt::NoButton, Qt::NoButton, event->modifiers(), false, false);
        emit mousePositionChanged(&me);
        me.setX(d->lastPos.x());
        me.setY(d->lastPos.y());
        emit positionChanged(&me);
    }
}

void QDeclarative1MouseArea::hoverLeaveEvent(QGraphicsSceneHoverEvent *event)
{
    Q_D(QDeclarative1MouseArea);
    if (!d->absorb)
        QDeclarativeItem::hoverLeaveEvent(event);
    else
        setHovered(false);
}

#ifndef QT_NO_CONTEXTMENU
void QDeclarative1MouseArea::contextMenuEvent(QGraphicsSceneContextMenuEvent *event)
{
    bool acceptsContextMenuButton;
#if defined(Q_OS_SYMBIAN)
    // In Symbian a Long Tap on the screen will trigger. See QSymbianControl::HandleLongTapEventL().
    acceptsContextMenuButton = acceptedButtons() & Qt::LeftButton;
#elif defined(Q_WS_WINCE)
    // ### WinCE can trigger context menu event with a gesture in the left button or a
    // click with the right button. Since we have no way here to differentiate them when
    // event happens, accepting either of the them will block the event.
    acceptsContextMenuButton = acceptedButtons() & (Qt::LeftButton | Qt::RightButton);
#else
    acceptsContextMenuButton = acceptedButtons() & Qt::RightButton;
#endif

    if (isEnabled() && event->reason() == QGraphicsSceneContextMenuEvent::Mouse
        && acceptsContextMenuButton) {
        // Do not let the context menu event propagate to items behind.
        return;
    }

    QDeclarativeItem::contextMenuEvent(event);
}
#endif // QT_NO_CONTEXTMENU

bool QDeclarative1MouseArea::sceneEvent(QEvent *event)
{
    bool rv = QDeclarativeItem::sceneEvent(event);
    if (event->type() == QEvent::UngrabMouse) {
        Q_D(QDeclarative1MouseArea);
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
    return rv;
}

bool QDeclarative1MouseArea::sendMouseEvent(QGraphicsSceneMouseEvent *event)
{
    Q_D(QDeclarative1MouseArea);
    QGraphicsSceneMouseEvent mouseEvent(event->type());
    QRectF myRect = mapToScene(QRectF(0, 0, width(), height())).boundingRect();

    QGraphicsScene *s = scene();
    QDeclarativeItem *grabber = s ? qobject_cast<QDeclarativeItem*>(s->mouseGrabberItem()) : 0;
    bool stealThisEvent = d->stealMouse;
    if ((stealThisEvent || myRect.contains(event->scenePos().toPoint())) && (!grabber || !grabber->keepMouseGrab())) {
        mouseEvent.setAccepted(false);
        for (int i = 0x1; i <= 0x10; i <<= 1) {
            if (event->buttons() & i) {
                Qt::MouseButton button = Qt::MouseButton(i);
                mouseEvent.setButtonDownPos(button, mapFromScene(event->buttonDownPos(button)));
            }
        }
        mouseEvent.setScenePos(event->scenePos());
        mouseEvent.setLastScenePos(event->lastScenePos());
        mouseEvent.setPos(mapFromScene(event->scenePos()));
        mouseEvent.setLastPos(mapFromScene(event->lastScenePos()));

        switch(mouseEvent.type()) {
        case QEvent::GraphicsSceneMouseMove:
            mouseMoveEvent(&mouseEvent);
            break;
        case QEvent::GraphicsSceneMousePress:
            mousePressEvent(&mouseEvent);
            break;
        case QEvent::GraphicsSceneMouseRelease:
            mouseReleaseEvent(&mouseEvent);
            break;
        default:
            break;
        }
        grabber = qobject_cast<QDeclarativeItem*>(s->mouseGrabberItem());
        if (grabber && stealThisEvent && !grabber->keepMouseGrab() && grabber != this)
            grabMouse();

        return stealThisEvent;
    }
    if (mouseEvent.type() == QEvent::GraphicsSceneMouseRelease) {
        if (d->pressed) {
            d->pressed = false;
            d->stealMouse = false;
            if (s && s->mouseGrabberItem() == this)
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

bool QDeclarative1MouseArea::sceneEventFilter(QGraphicsItem *i, QEvent *e)
{
    Q_D(QDeclarative1MouseArea);
    if (!d->absorb || !isVisible() || !d->drag || !d->drag->filterChildren())
        return QDeclarativeItem::sceneEventFilter(i, e);
    switch (e->type()) {
    case QEvent::GraphicsSceneMousePress:
    case QEvent::GraphicsSceneMouseMove:
    case QEvent::GraphicsSceneMouseRelease:
        return sendMouseEvent(static_cast<QGraphicsSceneMouseEvent *>(e));
    default:
        break;
    }

    return QDeclarativeItem::sceneEventFilter(i, e);
}

void QDeclarative1MouseArea::timerEvent(QTimerEvent *event)
{
    Q_D(QDeclarative1MouseArea);
    if (event->timerId() == d->pressAndHoldTimer.timerId()) {
        d->pressAndHoldTimer.stop();
        bool dragged = d->drag && d->drag->active();
        if (d->pressed && dragged == false && d->hovered == true) {
            d->longPress = true;
            QDeclarative1MouseEvent me(d->lastPos.x(), d->lastPos.y(), d->lastButton, d->lastButtons, d->lastModifiers, false, d->longPress);
            emit pressAndHold(&me);
        }
    }
}

void QDeclarative1MouseArea::geometryChanged(const QRectF &newGeometry,
                                            const QRectF &oldGeometry)
{
    Q_D(QDeclarative1MouseArea);
    QDeclarativeItem::geometryChanged(newGeometry, oldGeometry);

    if (d->lastScenePos.isNull)
        d->lastScenePos = mapToScene(d->lastPos);
    else if (newGeometry.x() != oldGeometry.x() || newGeometry.y() != oldGeometry.y())
        d->lastPos = mapFromScene(d->lastScenePos);
}

QVariant QDeclarative1MouseArea::itemChange(GraphicsItemChange change,
                                       const QVariant &value)
{
    Q_D(QDeclarative1MouseArea);
    switch (change) {
    case ItemVisibleHasChanged:
        if (acceptHoverEvents() && d->hovered != (isVisible() && isUnderMouse()))
            setHovered(!d->hovered);
        break;
    default:
        break;
    }

    return QDeclarativeItem::itemChange(change, value);
}

/*!
    \qmlproperty bool QtQuick1::MouseArea::hoverEnabled
    This property holds whether hover events are handled.

    By default, mouse events are only handled in response to a button event, or when a button is
    pressed.  Hover enables handling of all mouse events even when no mouse button is
    pressed.

    This property affects the containsMouse property and the onEntered, onExited and
    onPositionChanged signals.
*/
bool QDeclarative1MouseArea::hoverEnabled() const
{
    return acceptHoverEvents();
}

void QDeclarative1MouseArea::setHoverEnabled(bool h)
{
    Q_D(QDeclarative1MouseArea);
    if (h == acceptHoverEvents())
        return;

    setAcceptHoverEvents(h);
    emit hoverEnabledChanged();
    if (d->hovered != isUnderMouse())
        setHovered(!d->hovered);
}

/*!
    \qmlproperty bool QtQuick1::MouseArea::containsMouse
    This property holds whether the mouse is currently inside the mouse area.

    \warning This property is not updated if the area moves under the mouse: \e containsMouse will not change.
    In addition, if hoverEnabled is false, containsMouse will only be valid when the mouse is pressed.
*/
bool QDeclarative1MouseArea::hovered() const
{
    Q_D(const QDeclarative1MouseArea);
    return d->hovered;
}

/*!
    \qmlproperty bool QtQuick1::MouseArea::pressed
    This property holds whether the mouse area is currently pressed.
*/
bool QDeclarative1MouseArea::pressed() const
{
    Q_D(const QDeclarative1MouseArea);
    return d->pressed;
}

void QDeclarative1MouseArea::setHovered(bool h)
{
    Q_D(QDeclarative1MouseArea);
    if (d->hovered != h) {
        d->hovered = h;
        emit hoveredChanged();
        d->hovered ? emit entered() : emit exited();
    }
}

/*!
    \qmlproperty QtQuick1::Qt::MouseButtons MouseArea::acceptedButtons
    This property holds the mouse buttons that the mouse area reacts to.

    The available buttons are:
    \list
    \o Qt.LeftButton
    \o Qt.RightButton
    \o Qt.MiddleButton
    \endlist

    To accept more than one button the flags can be combined with the
    "|" (or) operator:

    \code
    MouseArea { acceptedButtons: Qt.LeftButton | Qt.RightButton }
    \endcode

    The default value is \c Qt.LeftButton.
*/
Qt::MouseButtons QDeclarative1MouseArea::acceptedButtons() const
{
    return acceptedMouseButtons();
}

void QDeclarative1MouseArea::setAcceptedButtons(Qt::MouseButtons buttons)
{
    if (buttons != acceptedMouseButtons()) {
        setAcceptedMouseButtons(buttons);
        emit acceptedButtonsChanged();
    }
}

bool QDeclarative1MouseArea::setPressed(bool p)
{
    Q_D(QDeclarative1MouseArea);
    bool dragged = d->drag && d->drag->active();
    bool isclick = d->pressed == true && p == false && dragged == false && d->hovered == true;

    if (d->pressed != p) {
        d->pressed = p;
        QDeclarative1MouseEvent me(d->lastPos.x(), d->lastPos.y(), d->lastButton, d->lastButtons, d->lastModifiers, isclick, d->longPress);
        if (d->pressed) {
            if (!d->doubleClick)
                emit pressed(&me);
            me.setX(d->lastPos.x());
            me.setY(d->lastPos.y());
            emit mousePositionChanged(&me);
            emit pressedChanged();
        } else {
            emit released(&me);
            me.setX(d->lastPos.x());
            me.setY(d->lastPos.y());
            emit pressedChanged();
            if (isclick && !d->longPress && !d->doubleClick)
                emit clicked(&me);
        }

        return me.isAccepted();
    }
    return false;
}

QDeclarative1Drag *QDeclarative1MouseArea::drag()
{
    Q_D(QDeclarative1MouseArea);
    if (!d->drag)
        d->drag = new QDeclarative1Drag;
    return d->drag;
}

/*!
    \qmlproperty Item QtQuick1::MouseArea::drag.target
    \qmlproperty bool QtQuick1::MouseArea::drag.active
    \qmlproperty enumeration QtQuick1::MouseArea::drag.axis
    \qmlproperty real QtQuick1::MouseArea::drag.minimumX
    \qmlproperty real QtQuick1::MouseArea::drag.maximumX
    \qmlproperty real QtQuick1::MouseArea::drag.minimumY
    \qmlproperty real QtQuick1::MouseArea::drag.maximumY
    \qmlproperty bool QtQuick1::MouseArea::drag.filterChildren

    \c drag provides a convenient way to make an item draggable.

    \list
    \i \c drag.target specifies the id of the item to drag.
    \i \c drag.active specifies if the target item is currently being dragged.
    \i \c drag.axis specifies whether dragging can be done horizontally (\c Drag.XAxis), vertically (\c Drag.YAxis), or both (\c Drag.XandYAxis)
    \i \c drag.minimum and \c drag.maximum limit how far the target can be dragged along the corresponding axes.
    \endlist

    The following example displays a \l Rectangle that can be dragged along the X-axis. The opacity
    of the rectangle is reduced when it is dragged to the right.

    \snippet doc/src/snippets/qtquick1/mousearea/mousearea.qml drag

    \note Items cannot be dragged if they are anchored for the requested 
    \c drag.axis. For example, if \c anchors.left or \c anchors.right was set
    for \c rect in the above example, it cannot be dragged along the X-axis.
    This can be avoided by settng the anchor value to \c undefined in 
    an \l onPressed handler.

    If \c drag.filterChildren is set to true, a drag can override descendant MouseAreas.  This
    enables a parent MouseArea to handle drags, for example, while descendants handle clicks:

    \snippet doc/src/snippets/qtquick1/mousearea/mouseareadragfilter.qml dragfilter

*/

QT_END_NAMESPACE
