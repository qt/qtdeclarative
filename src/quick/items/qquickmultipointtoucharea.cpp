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

#include "qquickmultipointtoucharea_p.h"
#include <QtQuick/qquickwindow.h>
#include <private/qsgadaptationlayer_p.h>
#include <private/qquickitem_p.h>
#include <QEvent>
#include <QMouseEvent>
#include <math.h>
#include <QDebug>
#include <qpa/qplatformnativeinterface.h>

QT_BEGIN_NAMESPACE

DEFINE_BOOL_CONFIG_OPTION(qmlVisualTouchDebugging, QML_VISUAL_TOUCH_DEBUGGING)

/*!
    \qmltype TouchPoint
    \instantiates QQuickTouchPoint
    \inqmlmodule QtQuick 2
    \ingroup qtquick-input-events
    \brief Describes a touch point in a MultiPointTouchArea

    The TouchPoint type contains information about a touch point, such as the current
    position, pressure, and area.
*/

/*!
    \qmlproperty int QtQuick2::TouchPoint::pointId

    This property holds the point id of the touch point.

    Each touch point within a MultiPointTouchArea will have a unique id.
*/
void QQuickTouchPoint::setPointId(int id)
{
    if (_id == id)
        return;
    _id = id;
    emit pointIdChanged();
}

/*!
    \qmlproperty real QtQuick2::TouchPoint::x
    \qmlproperty real QtQuick2::TouchPoint::y

    These properties hold the current position of the touch point.
*/

void QQuickTouchPoint::setX(qreal x)
{
    if (_x == x)
        return;
    _x = x;
    emit xChanged();
}

void QQuickTouchPoint::setY(qreal y)
{
    if (_y == y)
        return;
    _y = y;
    emit yChanged();
}

/*!
    \qmlproperty real QtQuick2::TouchPoint::pressure
    \qmlproperty vector2d QtQuick2::TouchPoint::velocity
    \qmlproperty rectangle QtQuick2::TouchPoint::area

    These properties hold additional information about the current state of the touch point.

    \list
    \li \c pressure is a value in the range of 0.0 to 1.0.
    \li \c velocity is a vector with magnitude reported in pixels per second.
    \li \c area is a rectangle covering the area of the touch point, centered on the current position of the touch point.
    \endlist

    Not all touch devices support velocity. If velocity is not supported, it will be reported
    as 0,0.
*/
void QQuickTouchPoint::setPressure(qreal pressure)
{
    if (_pressure == pressure)
        return;
    _pressure = pressure;
    emit pressureChanged();
}

void QQuickTouchPoint::setVelocity(const QVector2D &velocity)
{
    if (_velocity == velocity)
        return;
    _velocity = velocity;
    emit velocityChanged();
}

void QQuickTouchPoint::setArea(const QRectF &area)
{
    if (_area == area)
        return;
    _area = area;
    emit areaChanged();
}

/*!
    \qmlproperty bool QtQuick2::TouchPoint::pressed

    This property holds whether the touch point is currently pressed.
*/
void QQuickTouchPoint::setPressed(bool pressed)
{
    if (_pressed == pressed)
        return;
    _pressed = pressed;
    emit pressedChanged();
}

/*!
    \qmlproperty real QtQuick2::TouchPoint::startX
    \qmlproperty real QtQuick2::TouchPoint::startY

    These properties hold the starting position of the touch point.
*/

void QQuickTouchPoint::setStartX(qreal startX)
{
    if (_startX == startX)
        return;
    _startX = startX;
    emit startXChanged();
}

void QQuickTouchPoint::setStartY(qreal startY)
{
    if (_startY == startY)
        return;
    _startY = startY;
    emit startYChanged();
}

/*!
    \qmlproperty real QtQuick2::TouchPoint::previousX
    \qmlproperty real QtQuick2::TouchPoint::previousY

    These properties hold the previous position of the touch point.
*/
void QQuickTouchPoint::setPreviousX(qreal previousX)
{
    if (_previousX == previousX)
        return;
    _previousX = previousX;
    emit previousXChanged();
}

void QQuickTouchPoint::setPreviousY(qreal previousY)
{
    if (_previousY == previousY)
        return;
    _previousY = previousY;
    emit previousYChanged();
}

/*!
    \qmlproperty real QtQuick2::TouchPoint::sceneX
    \qmlproperty real QtQuick2::TouchPoint::sceneY

    These properties hold the current position of the touch point in scene coordinates.
*/

void QQuickTouchPoint::setSceneX(qreal sceneX)
{
    if (_sceneX == sceneX)
        return;
    _sceneX = sceneX;
    emit sceneXChanged();
}

void QQuickTouchPoint::setSceneY(qreal sceneY)
{
    if (_sceneY == sceneY)
        return;
    _sceneY = sceneY;
    emit sceneYChanged();
}

/*!
    \qmltype MultiPointTouchArea
    \instantiates QQuickMultiPointTouchArea
    \inqmlmodule QtQuick 2
    \inherits Item
    \ingroup qtquick-input
    \brief Enables handling of multiple touch points


    A MultiPointTouchArea is an invisible item that is used to track multiple touch points.

    The \l Item::enabled property is used to enable and disable touch handling. When disabled,
    the touch area becomes transparent to mouse/touch events.

    MultiPointTouchArea can be used in two ways:

    \list
    \li setting \c touchPoints to provide touch point objects with properties that can be bound to
    \li using the onTouchUpdated or onPressed, onUpdated and onReleased handlers
    \endlist

    While a MultiPointTouchArea \e can take exclusive ownership of certain touch points, it is also possible to have
    multiple MultiPointTouchAreas active at the same time, each operating on a different set of touch points.

    \sa TouchPoint
*/

/*!
    \qmlsignal QtQuick2::MultiPointTouchArea::onPressed(list<TouchPoint> touchPoints)

    This handler is called when new touch points are added. \a touchPoints is a list of these new points.

    If minimumTouchPoints is set to a value greater than one, this handler will not be called until the minimum number
    of required touch points has been reached. At that point, onPressed will be called with all the current touch points.
*/

/*!
    \qmlsignal QtQuick2::MultiPointTouchArea::onUpdated(list<TouchPoint> touchPoints)

    This handler is called when existing touch points are updated. \a touchPoints is a list of these updated points.
*/

/*!
    \qmlsignal QtQuick2::MultiPointTouchArea::onReleased(list<TouchPoint> touchPoints)

    This handler is called when existing touch points are removed. \a touchPoints is a list of these removed points.
*/

/*!
    \qmlsignal QtQuick2::MultiPointTouchArea::onCanceled(list<TouchPoint> touchPoints)

    This handler is called when new touch events have been canceled because another item stole the touch event handling.

    This signal is for advanced use: it is useful when there is more than one MultiPointTouchArea
    that is handling input, or when there is a MultiPointTouchArea inside a \l Flickable. In the latter
    case, if you execute some logic on the onPressed signal and then start dragging, the
    \l Flickable may steal the touch handling from the MultiPointTouchArea. In these cases, to reset
    the logic when the MultiPointTouchArea has lost the touch handling to the \l Flickable,
    \c onCanceled should be used in addition to onReleased.

    \a touchPoints is the list of canceled points.
*/

/*!
    \qmlsignal QtQuick2::MultiPointTouchArea::onGestureStarted(GestureEvent gesture)

    This handler is called when the global drag threshold has been reached.

    This function is typically used when a MultiPointTouchAreas has been nested in a Flickable or another MultiPointTouchArea.
    When the threshold has been reached, and the handler called, you can determine whether or not the touch
    area should grab the current touch points. By default they will not be grabbed; to grab them call \c gesture.grab(). If the
    gesture is not grabbed, the nesting Flickable, for example, would also have an opportunity to grab.

    The gesture object also includes information on the current set of \c touchPoints and the \c dragThreshold.
*/

/*!
    \qmlsignal QtQuick2::MultiPointTouchArea::onTouchUpdated(list<TouchPoint> touchPoints)

    This handler is called when the touch points handled by the MultiPointTouchArea change. This includes adding new touch points,
    removing or canceling previous touch points, as well as updating current touch point data. \a touchPoints is the list of all current touch
    points.
*/

/*!
    \qmlproperty list<TouchPoint> QtQuick2::MultiPointTouchArea::touchPoints

    This property holds a set of user-defined touch point objects that can be bound to.

    In the following example, we have two small rectangles that follow our touch points.

    \snippet qml/multipointtoucharea/multipointtoucharea.qml 0

    By default this property holds an empty list.

    \sa TouchPoint
*/

QQuickMultiPointTouchArea::QQuickMultiPointTouchArea(QQuickItem *parent)
    : QQuickItem(parent),
      _currentWindow(0),
      _minimumTouchPoints(0),
      _maximumTouchPoints(INT_MAX),
      _stealMouse(false)
{
    setAcceptedMouseButtons(Qt::LeftButton);
    setFiltersChildMouseEvents(true);
    if (qmlVisualTouchDebugging()) {
        setFlag(QQuickItem::ItemHasContents);
    }
#ifdef Q_OS_MAC
    connect(this, &QQuickItem::windowChanged, this, &QQuickMultiPointTouchArea::setTouchEventsEnabledForWindow);
#endif
}

QQuickMultiPointTouchArea::~QQuickMultiPointTouchArea()
{
    clearTouchLists();
    foreach (QObject *obj, _touchPoints) {
        QQuickTouchPoint *dtp = static_cast<QQuickTouchPoint*>(obj);
        if (!dtp->isQmlDefined())
            delete dtp;
    }
}

/*!
    \qmlproperty int QtQuick2::MultiPointTouchArea::minimumTouchPoints
    \qmlproperty int QtQuick2::MultiPointTouchArea::maximumTouchPoints

    These properties hold the range of touch points to be handled by the touch area.

    These are convenience that allow you to, for example, have nested MultiPointTouchAreas,
    one handling two finger touches, and another handling three finger touches.

    By default, all touch points within the touch area are handled.
*/

int QQuickMultiPointTouchArea::minimumTouchPoints() const
{
    return _minimumTouchPoints;
}

void QQuickMultiPointTouchArea::setMinimumTouchPoints(int num)
{
    if (_minimumTouchPoints == num)
        return;
    _minimumTouchPoints = num;
    emit minimumTouchPointsChanged();
}

int QQuickMultiPointTouchArea::maximumTouchPoints() const
{
    return _maximumTouchPoints;
}

void QQuickMultiPointTouchArea::setMaximumTouchPoints(int num)
{
    if (_maximumTouchPoints == num)
        return;
    _maximumTouchPoints = num;
    emit maximumTouchPointsChanged();
}

void QQuickMultiPointTouchArea::touchEvent(QTouchEvent *event)
{
    switch (event->type()) {
    case QEvent::TouchBegin:
    case QEvent::TouchUpdate:
    case QEvent::TouchEnd: {
        //if e.g. a parent Flickable has the mouse grab, don't process the touch events
        QQuickWindow *c = window();
        QQuickItem *grabber = c ? c->mouseGrabberItem() : 0;
        if (grabber && grabber != this && grabber->keepMouseGrab() && grabber->isEnabled()) {
            QQuickItem *item = this;
            while ((item = item->parentItem())) {
                if (item == grabber)
                    return;
            }
        }
        updateTouchData(event);
        if (event->type() == QEvent::TouchEnd) {
            //TODO: move to window
            _stealMouse = false;
            setKeepMouseGrab(false);
            setKeepTouchGrab(false);
            ungrabTouchPoints();
        }
        break;
    }
    default:
        QQuickItem::touchEvent(event);
        break;
    }
}

void QQuickMultiPointTouchArea::grabGesture()
{
    _stealMouse = true;

    grabMouse();
    setKeepMouseGrab(true);

    grabTouchPoints(_touchPoints.keys().toVector());
    setKeepTouchGrab(true);
}

void QQuickMultiPointTouchArea::updateTouchData(QEvent *event)
{
    bool ended = false;
    bool moved = false;
    bool started = false;

    clearTouchLists();
    QTouchEvent *e = static_cast<QTouchEvent*>(event);
    QList<QTouchEvent::TouchPoint> touchPoints = e->touchPoints();
    int numTouchPoints = touchPoints.count();
    //always remove released touches, and make sure we handle all releases before adds.
    foreach (const QTouchEvent::TouchPoint &p, touchPoints) {
        Qt::TouchPointState touchPointState = p.state();
        int id = p.id();
        if (touchPointState & Qt::TouchPointReleased) {
            QQuickTouchPoint* dtp = static_cast<QQuickTouchPoint*>(_touchPoints.value(id));
            if (!dtp)
                continue;
            updateTouchPoint(dtp, &p);
            dtp->setPressed(false);
            _releasedTouchPoints.append(dtp);
            _touchPoints.remove(id);
            ended = true;
        }
    }
    if (numTouchPoints >= _minimumTouchPoints && numTouchPoints <= _maximumTouchPoints) {
        foreach (const QTouchEvent::TouchPoint &p, touchPoints) {
            Qt::TouchPointState touchPointState = p.state();
            int id = p.id();
            if (touchPointState & Qt::TouchPointReleased) {
                //handled above
            } else if (!_touchPoints.contains(id)) { //could be pressed, moved, or stationary
                // (we may have just obtained enough points to start tracking them -- in that case moved or stationary count as newly pressed)
                addTouchPoint(&p);
                started = true;
            } else if (touchPointState & Qt::TouchPointMoved) {
                QQuickTouchPoint* dtp = static_cast<QQuickTouchPoint*>(_touchPoints[id]);
                Q_ASSERT(dtp);
                _movedTouchPoints.append(dtp);
                updateTouchPoint(dtp,&p);
                moved = true;
            } else {
                QQuickTouchPoint* dtp = static_cast<QQuickTouchPoint*>(_touchPoints[id]);
                Q_ASSERT(dtp);
                updateTouchPoint(dtp,&p);
            }
        }

        //see if we should be grabbing the gesture
        if (!_stealMouse /* !ignoring gesture*/) {
            bool offerGrab = false;
            const int dragThreshold = qApp->styleHints()->startDragDistance();
            foreach (const QTouchEvent::TouchPoint &p, touchPoints) {
                if (p.state() == Qt::TouchPointReleased)
                    continue;
                const QPointF &currentPos = p.scenePos();
                const QPointF &startPos = p.startScenePos();
                if (qAbs(currentPos.x() - startPos.x()) > dragThreshold)
                    offerGrab = true;
                else if (qAbs(currentPos.y() - startPos.y()) > dragThreshold)
                    offerGrab = true;
                if (offerGrab)
                    break;
            }

            if (offerGrab) {
                QQuickGrabGestureEvent event;
                event._touchPoints = _touchPoints.values();
                emit gestureStarted(&event);
                if (event.wantsGrab())
                    grabGesture();
            }
        }

        if (ended)
            emit released(_releasedTouchPoints);
        if (moved)
            emit updated(_movedTouchPoints);
        if (started)
            emit pressed(_pressedTouchPoints);
        if (ended || moved || started) emit touchUpdated(_touchPoints.values());
    }
}

void QQuickMultiPointTouchArea::clearTouchLists()
{
    foreach (QObject *obj, _releasedTouchPoints) {
        QQuickTouchPoint *dtp = static_cast<QQuickTouchPoint*>(obj);
        if (!dtp->isQmlDefined())
            delete dtp;
        else
            dtp->setInUse(false);
    }
    _releasedTouchPoints.clear();
    _pressedTouchPoints.clear();
    _movedTouchPoints.clear();
}

void QQuickMultiPointTouchArea::addTouchPoint(const QTouchEvent::TouchPoint *p)
{
    QQuickTouchPoint *dtp = 0;
    foreach (QQuickTouchPoint* tp, _touchPrototypes) {
        if (!tp->inUse()) {
            tp->setInUse(true);
            dtp = tp;
            break;
        }
    }

    if (dtp == 0)
        dtp = new QQuickTouchPoint(false);
    dtp->setPointId(p->id());
    updateTouchPoint(dtp,p);
    dtp->setPressed(true);
    _touchPoints.insert(p->id(),dtp);
    _pressedTouchPoints.append(dtp);
}

void QQuickMultiPointTouchArea::setTouchEventsEnabledForWindow(QWindow *window)
{
#ifdef Q_OS_MAC
    // Resolve function for enabling touch events from the (cocoa) platform plugin.
    typedef void (*RegisterTouchWindowFunction)(QWindow *, bool);
    RegisterTouchWindowFunction registerTouchWindow = reinterpret_cast<RegisterTouchWindowFunction>(
        QGuiApplication::platformNativeInterface()->nativeResourceFunctionForIntegration("registertouchwindow"));
    if (!registerTouchWindow)
        return; // Not necessarily an error, Qt migh be using a different platform plugin.

    // Disable touch on the old window, enable on the new window.
    if (_currentWindow)
        registerTouchWindow(_currentWindow, false);
    if (window)
        registerTouchWindow(window, true);
    // Save the current window, setTouchEventsEnabledForWindow will be called
    // with a null window on disable.
    _currentWindow = window;
#else // Q_OS_MAC
    Q_UNUSED(window)
#endif
}

void QQuickMultiPointTouchArea::addTouchPrototype(QQuickTouchPoint *prototype)
{
    int id = _touchPrototypes.count();
    prototype->setPointId(id);
    _touchPrototypes.insert(id, prototype);
}

void QQuickMultiPointTouchArea::updateTouchPoint(QQuickTouchPoint *dtp, const QTouchEvent::TouchPoint *p)
{
    //TODO: if !qmlDefined, could bypass setters.
    //      also, should only emit signals after all values have been set
    dtp->setX(p->pos().x());
    dtp->setY(p->pos().y());
    dtp->setPressure(p->pressure());
    dtp->setVelocity(p->velocity());
    dtp->setArea(p->rect());
    dtp->setStartX(p->startPos().x());
    dtp->setStartY(p->startPos().y());
    dtp->setPreviousX(p->lastPos().x());
    dtp->setPreviousY(p->lastPos().y());
    dtp->setSceneX(p->scenePos().x());
    dtp->setSceneY(p->scenePos().y());
}

void QQuickMultiPointTouchArea::mousePressEvent(QMouseEvent *event)
{
    if (!isEnabled()) {
        QQuickItem::mousePressEvent(event);
        return;
    }

    _stealMouse = false;
    setKeepMouseGrab(false);
    event->setAccepted(true);
}

void QQuickMultiPointTouchArea::mouseMoveEvent(QMouseEvent *event)
{
    if (!isEnabled()) {
        QQuickItem::mouseMoveEvent(event);
        return;
    }

    //do nothing
}

void QQuickMultiPointTouchArea::mouseReleaseEvent(QMouseEvent *event)
{
    _stealMouse = false;
    if (!isEnabled()) {
        QQuickItem::mouseReleaseEvent(event);
        return;
    }
    QQuickWindow *c = window();
    if (c && c->mouseGrabberItem() == this)
        ungrabMouse();
    setKeepMouseGrab(false);
}

void QQuickMultiPointTouchArea::ungrab()
{
    if (_touchPoints.count()) {
        QQuickWindow *c = window();
        if (c && c->mouseGrabberItem() == this) {
            _stealMouse = false;
            setKeepMouseGrab(false);
        }
        setKeepTouchGrab(false);
        foreach (QObject *obj, _touchPoints)
            static_cast<QQuickTouchPoint*>(obj)->setPressed(false);
        emit canceled(_touchPoints.values());
        clearTouchLists();
        foreach (QObject *obj, _touchPoints) {
            QQuickTouchPoint *dtp = static_cast<QQuickTouchPoint*>(obj);
            if (!dtp->isQmlDefined())
                delete dtp;
            else
                dtp->setInUse(false);
        }
        _touchPoints.clear();
        emit touchUpdated(QList<QObject*>());
    }
}

void QQuickMultiPointTouchArea::mouseUngrabEvent()
{
    ungrab();
}

void QQuickMultiPointTouchArea::touchUngrabEvent()
{
    ungrab();
}

bool QQuickMultiPointTouchArea::sendMouseEvent(QMouseEvent *event)
{
    QPointF localPos = mapFromScene(event->windowPos());

    QQuickWindow *c = window();
    QQuickItem *grabber = c ? c->mouseGrabberItem() : 0;
    bool stealThisEvent = _stealMouse;
    if ((stealThisEvent || contains(localPos)) && (!grabber || !grabber->keepMouseGrab())) {
        QMouseEvent mouseEvent(event->type(), localPos, event->windowPos(), event->screenPos(),
                               event->button(), event->buttons(), event->modifiers());
        mouseEvent.setAccepted(false);

        switch (mouseEvent.type()) {
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
        _stealMouse = false;
        if (c && c->mouseGrabberItem() == this)
            ungrabMouse();
        setKeepMouseGrab(false);
    }
    return false;
}

bool QQuickMultiPointTouchArea::childMouseEventFilter(QQuickItem *i, QEvent *event)
{
    if (!isEnabled() || !isVisible())
        return QQuickItem::childMouseEventFilter(i, event);
    switch (event->type()) {
    case QEvent::MouseButtonPress:
    case QEvent::MouseMove:
    case QEvent::MouseButtonRelease:
        return sendMouseEvent(static_cast<QMouseEvent *>(event));
        break;
    case QEvent::TouchBegin:
    case QEvent::TouchUpdate:
        if (!shouldFilter(event))
            return false;
        updateTouchData(event);
        return _stealMouse;
    case QEvent::TouchEnd: {
            if (!shouldFilter(event))
                return false;
            updateTouchData(event);
            //TODO: verify this behavior
            _stealMouse = false;
            setKeepMouseGrab(false);
            setKeepTouchGrab(false);
            ungrabTouchPoints();
        }
        break;
    default:
        break;
    }
    return QQuickItem::childMouseEventFilter(i, event);
}

bool QQuickMultiPointTouchArea::shouldFilter(QEvent *event)
{
    QQuickWindow *c = window();
    QQuickItem *grabber = c ? c->mouseGrabberItem() : 0;
    bool disabledItem = grabber && !grabber->isEnabled();
    bool stealThisEvent = _stealMouse;
    bool containsPoint = false;
    if (!stealThisEvent) {
        switch (event->type()) {
        case QEvent::MouseButtonPress:
        case QEvent::MouseMove:
        case QEvent::MouseButtonRelease: {
                QMouseEvent *me = static_cast<QMouseEvent*>(event);
                containsPoint = contains(mapFromScene(me->windowPos()));
            }
            break;
        case QEvent::TouchBegin:
        case QEvent::TouchUpdate:
        case QEvent::TouchEnd: {
                QTouchEvent *te = static_cast<QTouchEvent*>(event);
                foreach (const QTouchEvent::TouchPoint &point, te->touchPoints()) {
                    if (contains(mapFromScene(point.scenePos()))) {
                        containsPoint = true;
                        break;
                    }
                }
            }
            break;
        default:
            break;
        }
    }
    if ((stealThisEvent || containsPoint) && (!grabber || !grabber->keepMouseGrab() || disabledItem)) {
        return true;
    }
    ungrab();
    return false;
}

QSGNode *QQuickMultiPointTouchArea::updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *data)
{
    Q_UNUSED(data);

    if (!qmlVisualTouchDebugging())
        return 0;

    QSGRectangleNode *rectangle = static_cast<QSGRectangleNode *>(oldNode);
    if (!rectangle) rectangle = QQuickItemPrivate::get(this)->sceneGraphContext()->createRectangleNode();

    rectangle->setRect(QRectF(0, 0, width(), height()));
    rectangle->setColor(QColor(255, 0, 0, 50));
    rectangle->update();
    return rectangle;
}

QT_END_NAMESPACE
