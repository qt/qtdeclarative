// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickmultipointtoucharea_p.h"
#include <QtQuick/qquickwindow.h>
#include <private/qsgadaptationlayer_p.h>
#include <private/qquickitem_p.h>
#include <private/qquickwindow_p.h>
#include <private/qguiapplication_p.h>
#include <QtGui/private/qevent_p.h>
#include <QtGui/private/qeventpoint_p.h>
#include <QtGui/private/qpointingdevice_p.h>
#include <QEvent>
#include <QMouseEvent>
#include <QDebug>
#include <qpa/qplatformnativeinterface.h>

QT_BEGIN_NAMESPACE

DEFINE_BOOL_CONFIG_OPTION(qmlMptaVisualTouchDebugging, QML_VISUAL_TOUCH_DEBUGGING)

/*!
    \qmltype TouchPoint
    \instantiates QQuickTouchPoint
    \inqmlmodule QtQuick
    \ingroup qtquick-input-events
    \brief Describes a touch point in a MultiPointTouchArea.

    The TouchPoint type contains information about a touch point, such as the current
    position, pressure, and area.

    \image touchpoint-metrics.png
*/

/*!
    \qmlproperty int QtQuick::TouchPoint::pointId

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
    \qmlproperty real QtQuick::TouchPoint::x
    \qmlproperty real QtQuick::TouchPoint::y

    These properties hold the current position of the touch point.
*/

void QQuickTouchPoint::setPosition(QPointF p)
{
    bool xch = (_x != p.x());
    bool ych = (_y != p.y());
    if (!xch && !ych)
        return;
    _x = p.x();
    _y = p.y();
    if (xch)
        emit xChanged();
    if (ych)
        emit yChanged();
}

/*!
    \qmlproperty size QtQuick::TouchPoint::ellipseDiameters
    \since 5.9

    This property holds the major and minor axes of the ellipse representing
    the covered area of the touch point.
*/
void QQuickTouchPoint::setEllipseDiameters(const QSizeF &d)
{
    if (_ellipseDiameters == d)
        return;
    _ellipseDiameters = d;
    emit ellipseDiametersChanged();
}

/*!
    \qmlproperty real QtQuick::TouchPoint::pressure
    \qmlproperty vector2d QtQuick::TouchPoint::velocity

    These properties hold additional information about the current state of the touch point.

    \list
    \li \c pressure is a value in the range of 0.0 to 1.0.
    \li \c velocity is a vector with magnitude reported in pixels per second.
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

/*!
    \qmlproperty real QtQuick::TouchPoint::rotation
    \since 5.9

    This property holds the angular orientation of this touch point. The return
    value is in degrees, where zero (the default) indicates the finger or token
    is pointing upwards, a negative angle means it's rotated to the left, and a
    positive angle means it's rotated to the right. Most touchscreens do not
    detect rotation, so zero is the most common value.

    \sa QEventPoint::rotation()
*/
void QQuickTouchPoint::setRotation(qreal r)
{
    if (_rotation == r)
        return;
    _rotation = r;
    emit rotationChanged();
}

void QQuickTouchPoint::setVelocity(const QVector2D &velocity)
{
    if (_velocity == velocity)
        return;
    _velocity = velocity;
    emit velocityChanged();
}

/*!
    \deprecated
    \qmlproperty rectangle QtQuick::TouchPoint::area

    A rectangle covering the area of the touch point, centered on the current
    position of the touch point.

    It is deprecated because a touch point is more correctly modeled as an ellipse,
    whereas this rectangle represents the outer bounds of the ellipse after \l rotation.
*/
void QQuickTouchPoint::setArea(const QRectF &area)
{
    if (_area == area)
        return;
    _area = area;
    emit areaChanged();
}

/*!
    \qmlproperty bool QtQuick::TouchPoint::pressed

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
    \qmlproperty real QtQuick::TouchPoint::startX
    \qmlproperty real QtQuick::TouchPoint::startY

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
    \qmlproperty real QtQuick::TouchPoint::previousX
    \qmlproperty real QtQuick::TouchPoint::previousY

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
    \qmlproperty real QtQuick::TouchPoint::sceneX
    \qmlproperty real QtQuick::TouchPoint::sceneY

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
    \qmlproperty pointingDeviceUniqueId QtQuick::TouchPoint::uniqueId
    \since 5.9

    This property holds the unique ID of the touch point or token.

    It is normally empty, because touchscreens cannot uniquely identify fingers.
    But when it is set, it is expected to uniquely identify a specific token
    (fiducial object).

    Interpreting the contents of this ID requires knowledge of the hardware and
    drivers in use (e.g. various TUIO-based touch surfaces).
*/
void QQuickTouchPoint::setUniqueId(const QPointingDeviceUniqueId &id)
{
    _uniqueId = id;
    emit uniqueIdChanged();
}


/*!
    \qmltype GestureEvent
    \instantiates QQuickGrabGestureEvent
    \inqmlmodule QtQuick
    \ingroup qtquick-input-events
    \brief The parameter given with the gestureStarted signal.

    The GestureEvent object has the current touch points, which you may choose
    to interpret as a gesture, and an invokable method to grab the involved
    points exclusively.
*/

/*!
    \qmlproperty real QtQuick::GestureEvent::dragThreshold

    This property holds the system setting for the distance a finger must move
    before it is interpreted as a drag. It comes from
    QStyleHints::startDragDistance().
*/

/*!
    \qmlproperty list<TouchPoint> QtQuick::GestureEvent::touchPoints

    This property holds the set of current touch points.
*/

/*!
    \qmlmethod QtQuick::GestureEvent::grab()

    Acquires an exclusive grab of the mouse and all the \l touchPoints, and
    calls \l {QQuickItem::setKeepTouchGrab()}{setKeepTouchGrab()} and
    \l {QQuickItem::setKeepMouseGrab()}{setKeepMouseGrab()} so that any
    parent Item that \l {QQuickItem::filtersChildMouseEvents()}{filters} its
    children's events will not be allowed to take over the grabs.
*/

/*!
    \qmltype MultiPointTouchArea
    \instantiates QQuickMultiPointTouchArea
    \inqmlmodule QtQuick
    \inherits Item
    \ingroup qtquick-input
    \brief Enables handling of multiple touch points.


    A MultiPointTouchArea is an invisible item that is used to track multiple touch points.

    The \l Item::enabled property is used to enable and disable touch handling. When disabled,
    the touch area becomes transparent to mouse and touch events.

    By default, the mouse will be handled the same way as a single touch point,
    and items under the touch area will not receive mouse events because the
    touch area is handling them. But if the \l mouseEnabled property is set to
    false, it becomes transparent to mouse events so that another
    mouse-sensitive Item (such as a MouseArea) can be used to handle mouse
    interaction separately.

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
    \qmlsignal QtQuick::MultiPointTouchArea::pressed(list<TouchPoint> touchPoints)

    This signal is emitted when new touch points are added. \a touchPoints is a list of these new points.

    If minimumTouchPoints is set to a value greater than one, this signal will not be emitted until the minimum number
    of required touch points has been reached.

    \note If you use the \c touchPoints argument in your signal handler code,
    it's best to rename it in your formal parameter to avoid confusion with the
    \c touchPoints property (see \l{QML Coding Conventions}):
    \qml
    onPressed: (points) => console.log("pressed", points.length)
    \endqml
*/

/*!
    \qmlsignal QtQuick::MultiPointTouchArea::updated(list<TouchPoint> touchPoints)

    This signal is emitted when existing touch points are updated. \a touchPoints is a list of these updated points.

    \note If you use the \c touchPoints argument in your signal handler code,
    it's best to rename it in your formal parameter to avoid confusion with the
    \c touchPoints property (see \l{QML Coding Conventions}):
    \qml
    onUpdated: (points) => console.log("updated", points.length)
    \endqml
*/

/*!
    \qmlsignal QtQuick::MultiPointTouchArea::released(list<TouchPoint> touchPoints)

    This signal is emitted when existing touch points are removed. \a touchPoints is a list of these removed points.

    \note If you use the \c touchPoints argument in your signal handler code,
    it's best to rename it in your formal parameter to avoid confusion with the
    \c touchPoints property (see \l{QML Coding Conventions}):
    \qml
    onReleased: (points) => console.log("released", points.length)
    \endqml
*/

/*!
    \qmlsignal QtQuick::MultiPointTouchArea::canceled(list<TouchPoint> touchPoints)

    This signal is emitted when new touch events have been canceled because another item stole the touch event handling.

    This signal is for advanced use: it is useful when there is more than one MultiPointTouchArea
    that is handling input, or when there is a MultiPointTouchArea inside a \l Flickable. In the latter
    case, if you execute some logic in the \c onPressed signal handler and then start dragging, the
    \l Flickable may steal the touch handling from the MultiPointTouchArea. In these cases, to reset
    the logic when the MultiPointTouchArea has lost the touch handling to the \l Flickable,
    \c canceled should be handled in addition to \l released.

    \a touchPoints is the list of canceled points.

    \note If you use the \c touchPoints argument in your signal handler code,
    it's best to rename it in your formal parameter to avoid confusion with the
    \c touchPoints property (see \l{QML Coding Conventions}):
    \qml
    onCanceled: (points) => console.log("canceled", points.length)
    \endqml
*/

// TODO Qt 7: remove the notes above about the signal touchPoints arguments

/*!
    \qmlsignal QtQuick::MultiPointTouchArea::gestureStarted(GestureEvent gesture)

    This signal is emitted when the global drag threshold has been reached.

    This signal is typically used when a MultiPointTouchArea has been nested in a Flickable or another MultiPointTouchArea.
    When the threshold has been reached and the signal is handled, you can determine whether or not the touch
    area should grab the current touch points. By default they will not be grabbed; to grab them call \c gesture.grab(). If the
    gesture is not grabbed, the nesting Flickable, for example, would also have an opportunity to grab.

    The \a gesture object also includes information on the current set of \c touchPoints and the \c dragThreshold.
*/

/*!
    \qmlsignal QtQuick::MultiPointTouchArea::touchUpdated(list<TouchPoint> touchPoints)

    This signal is emitted when the touch points handled by the MultiPointTouchArea change. This includes adding new touch points,
    removing or canceling previous touch points, as well as updating current touch point data. \a touchPoints is the list of all current touch
    points.
*/

/*!
    \qmlproperty list<TouchPoint> QtQuick::MultiPointTouchArea::touchPoints

    This property holds a set of user-defined touch point objects that can be bound to.

    If mouseEnabled is true (the default) and the left mouse button is pressed
    while the mouse is over the touch area, the current mouse position will be
    one of these touch points.

    In the following example, we have two small rectangles that follow our touch points.

    \snippet qml/multipointtoucharea/multipointtoucharea.qml 0

    By default this property holds an empty list.

    \sa TouchPoint
*/

QQuickMultiPointTouchArea::QQuickMultiPointTouchArea(QQuickItem *parent)
    : QQuickItem(parent),
      _minimumTouchPoints(0),
      _maximumTouchPoints(INT_MAX),
      _touchMouseDevice(nullptr),
      _stealMouse(false),
      _mouseEnabled(true)
{
    setAcceptedMouseButtons(Qt::LeftButton);
    setFiltersChildMouseEvents(true);
    if (qmlMptaVisualTouchDebugging()) {
        setFlag(QQuickItem::ItemHasContents);
    }
    setAcceptTouchEvents(true);
#ifdef Q_OS_MACOS
    setAcceptHoverEvents(true); // needed to enable touch events on mouse hover.
#endif
}

QQuickMultiPointTouchArea::~QQuickMultiPointTouchArea()
{
    clearTouchLists();
    for (QObject *obj : std::as_const(_touchPoints)) {
        QQuickTouchPoint *dtp = static_cast<QQuickTouchPoint*>(obj);
        if (!dtp->isQmlDefined())
            delete dtp;
    }
}

/*!
    \qmlproperty int QtQuick::MultiPointTouchArea::minimumTouchPoints
    \qmlproperty int QtQuick::MultiPointTouchArea::maximumTouchPoints

    These properties hold the range of touch points to be handled by the touch area.

    These are convenience that allow you to, for example, have nested MultiPointTouchAreas,
    one handling two finger touches, and another handling three finger touches.

    By default, all touch points within the touch area are handled.

    If mouseEnabled is true, the mouse acts as a touch point, so it is also
    subject to these constraints: for example if maximumTouchPoints is two, you
    can use the mouse as one touch point and a finger as another touch point
    for a total of two.
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

/*!
    \qmlproperty bool QtQuick::MultiPointTouchArea::mouseEnabled

    This property controls whether the MultiPointTouchArea will handle mouse
    events too. If it is true (the default), the touch area will treat the
    mouse the same as a single touch point; if it is false, the touch area will
    ignore mouse events and allow them to "pass through" so that they can be
    handled by other items underneath.
*/
void QQuickMultiPointTouchArea::setMouseEnabled(bool arg)
{
    if (_mouseEnabled != arg) {
        _mouseEnabled = arg;
        if (_mouseTouchPoint && !arg)
            _mouseTouchPoint = nullptr;
        emit mouseEnabledChanged();
    }
}

void QQuickMultiPointTouchArea::touchEvent(QTouchEvent *event)
{
    switch (event->type()) {
    case QEvent::TouchBegin:
    case QEvent::TouchUpdate:
    case QEvent::TouchEnd: {
        //if e.g. a parent Flickable has the mouse grab, don't process the touch events
        QQuickWindow *c = window();
        QQuickItem *grabber = c ? c->mouseGrabberItem() : nullptr;
        if (grabber && grabber != this && grabber->keepMouseGrab() && grabber->isEnabled()) {
            QQuickItem *item = this;
            while ((item = item->parentItem())) {
                if (item == grabber)
                    return;
            }
        }
        updateTouchData(event);
        if (event->type() == QEvent::TouchEnd)
            ungrab(true);
        break;
    }
    case QEvent::TouchCancel:
        ungrab();
        break;
    default:
        QQuickItem::touchEvent(event);
        break;
    }
}

void QQuickMultiPointTouchArea::grabGesture(QPointingDevice *dev)
{
    _stealMouse = true;

    grabMouse();
    setKeepMouseGrab(true);

    QPointingDevicePrivate *devPriv = QPointingDevicePrivate::get(dev);
    for (auto it = _touchPoints.keyBegin(), end = _touchPoints.keyEnd(); it != end; ++it) {
        if (*it != -1) // -1 might be the mouse-point, but we already grabbed the mouse above.
            if (auto pt = devPriv->queryPointById(*it))
                pt->exclusiveGrabber = this;
    }
    setKeepTouchGrab(true);
}

void QQuickMultiPointTouchArea::updateTouchData(QEvent *event, RemapEventPoints remap)
{
    bool ended = false;
    bool moved = false;
    bool started = false;

    clearTouchLists();
    QList<QEventPoint> touchPoints;
    bool touchPointsFromEvent = false;
    QPointingDevice *dev = nullptr;

    switch (event->type()) {
    case QEvent::TouchBegin:
    case QEvent::TouchUpdate:
    case QEvent::TouchEnd: {
        QTouchEvent* te = static_cast<QTouchEvent*>(event);
        touchPoints = te->points();
        touchPointsFromEvent = true;
        dev = const_cast<QPointingDevice *>(te->pointingDevice());
        break;
    }
    case QEvent::MouseButtonPress: {
        auto da = QQuickItemPrivate::get(this)->deliveryAgentPrivate();
        _mouseQpaTouchPoint = QEventPoint(da->touchMouseId);
        _touchMouseDevice = da->touchMouseDevice;
        Q_FALLTHROUGH();
    }
    case QEvent::MouseMove:
    case QEvent::MouseButtonRelease: {
        QMouseEvent *me = static_cast<QMouseEvent*>(event);
        _mouseQpaTouchPoint = me->points().first();
        dev = const_cast<QPointingDevice *>(me->pointingDevice());
        if (event->type() == QEvent::MouseButtonPress) {
            addTouchPoint(me);
            started = true;
        }
        touchPoints << _mouseQpaTouchPoint;
        break;
    }
    default:
        qWarning("updateTouchData: unhandled event type %d", event->type());
        break;
    }

    int numTouchPoints = touchPoints.size();
    //always remove released touches, and make sure we handle all releases before adds.
    for (const QEventPoint &p : std::as_const(touchPoints)) {
        QEventPoint::State touchPointState = p.state();
        int id = p.id();
        if (touchPointState & QEventPoint::State::Released) {
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
        for (QEventPoint &p : touchPoints) {
            QPointF oldPos = p.position();
            auto transformBack = qScopeGuard([&] { QMutableEventPoint::setPosition(p, oldPos); });
            if (touchPointsFromEvent && remap == RemapEventPoints::ToLocal)
                QMutableEventPoint::setPosition(p, mapFromScene(p.scenePosition()));
            QEventPoint::State touchPointState = p.state();
            int id = p.id();
            if (touchPointState & QEventPoint::State::Released) {
                //handled above
            } else if (!_touchPoints.contains(id)) { //could be pressed, moved, or stationary
                // (we may have just obtained enough points to start tracking them -- in that case moved or stationary count as newly pressed)
                addTouchPoint(&p);
                started = true;
            } else if ((touchPointState & QEventPoint::State::Updated) ||
                       (touchPointState & QEventPoint::State::Stationary)) {
                // React to a stationary point as if the point moved. (QTBUG-77142)
                QQuickTouchPoint* dtp = static_cast<QQuickTouchPoint*>(_touchPoints.value(id));
                Q_ASSERT(dtp);
                _movedTouchPoints.append(dtp);
                updateTouchPoint(dtp,&p);
                moved = true;
            } else {
                QQuickTouchPoint* dtp = static_cast<QQuickTouchPoint*>(_touchPoints.value(id));
                Q_ASSERT(dtp);
                updateTouchPoint(dtp,&p);
            }
        }

        //see if we should be grabbing the gesture
        if (!_stealMouse /* !ignoring gesture*/) {
            bool offerGrab = false;
            const int dragThreshold = QGuiApplication::styleHints()->startDragDistance();
            for (const QEventPoint &p : std::as_const(touchPoints)) {
                if (p.state() == QEventPoint::State::Released)
                    continue;
                const QPointF &currentPos = p.scenePosition();
                const QPointF &startPos = p.scenePressPosition();
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
                if (event.wantsGrab() && dev)
                    grabGesture(dev);
            }
        }

        if (ended)
            emit released(_releasedTouchPoints);
        if (moved)
            emit updated(_movedTouchPoints);
        if (started && !_pressedTouchPoints.isEmpty())
            emit pressed(_pressedTouchPoints);
        if (ended || moved || started) emit touchUpdated(_touchPoints.values());
    }
}

void QQuickMultiPointTouchArea::clearTouchLists()
{
    for (QObject *obj : std::as_const(_releasedTouchPoints)) {
        QQuickTouchPoint *dtp = static_cast<QQuickTouchPoint*>(obj);
        if (!dtp->isQmlDefined()) {
            _touchPoints.remove(dtp->pointId());
            delete dtp;
        } else {
            dtp->setInUse(false);
        }
    }
    _releasedTouchPoints.clear();
    _pressedTouchPoints.clear();
    _movedTouchPoints.clear();
}

void QQuickMultiPointTouchArea::addTouchPoint(const QEventPoint *p)
{
    QQuickTouchPoint *dtp = nullptr;
    for (QQuickTouchPoint* tp : std::as_const(_touchPrototypes)) {
        if (!tp->inUse()) {
            tp->setInUse(true);
            dtp = tp;
            break;
        }
    }

    if (dtp == nullptr)
        dtp = new QQuickTouchPoint(false);
    dtp->setPointId(p->id());
    updateTouchPoint(dtp,p);
    dtp->setPressed(true);
    _touchPoints.insert(p->id(),dtp);
    _pressedTouchPoints.append(dtp);
}

void QQuickMultiPointTouchArea::addTouchPoint(const QMouseEvent *e)
{
    QQuickTouchPoint *dtp = nullptr;
    for (QQuickTouchPoint *tp : std::as_const(_touchPrototypes)) {
        if (!tp->inUse()) {
            tp->setInUse(true);
            dtp = tp;
            break;
        } else if (_mouseTouchPoint == tp) {
            return; // do not allow more than one touchpoint to react to the mouse (QTBUG-83662)
        }
    }

    if (dtp == nullptr)
        dtp = new QQuickTouchPoint(false);
    updateTouchPoint(dtp, e);
    dtp->setPressed(true);
    _touchPoints.insert(_mouseQpaTouchPoint.id(), dtp);
    _pressedTouchPoints.append(dtp);
    _mouseTouchPoint = dtp;
}

#ifdef Q_OS_MACOS
void QQuickMultiPointTouchArea::hoverEnterEvent(QHoverEvent *event)
{
    setTouchEventsEnabled(isEnabled());
    QQuickItem::hoverEnterEvent(event);
}

void QQuickMultiPointTouchArea::hoverLeaveEvent(QHoverEvent *event)
{
    setTouchEventsEnabled(false);
    QQuickItem::hoverLeaveEvent(event);
}

void QQuickMultiPointTouchArea::setTouchEventsEnabled(bool enable)
{
    // Resolve function for enabling touch events from the (cocoa) platform plugin.
    typedef void (*RegisterTouchWindowFunction)(QWindow *, bool);
    RegisterTouchWindowFunction registerTouchWindow = reinterpret_cast<RegisterTouchWindowFunction>(
        QGuiApplication::platformNativeInterface()->nativeResourceFunctionForIntegration("registertouchwindow"));
    if (!registerTouchWindow)
        return; // Not necessarily an error, Qt might be using a different platform plugin.

    registerTouchWindow(window(), enable);
}

void QQuickMultiPointTouchArea::itemChange(ItemChange change, const ItemChangeData &data)
{
    if (change == ItemEnabledHasChanged)
        setAcceptHoverEvents(data.boolValue);
    QQuickItem::itemChange(change, data);
}
#endif // Q_OS_MACOS

void QQuickMultiPointTouchArea::addTouchPrototype(QQuickTouchPoint *prototype)
{
    int id = _touchPrototypes.size();
    prototype->setPointId(id);
    _touchPrototypes.insert(id, prototype);
}

void QQuickMultiPointTouchArea::updateTouchPoint(QQuickTouchPoint *dtp, const QEventPoint *p)
{
    //TODO: if !qmlDefined, could bypass setters.
    //      also, should only emit signals after all values have been set
    dtp->setUniqueId(p->uniqueId());
    dtp->setPosition(p->position());
    dtp->setEllipseDiameters(p->ellipseDiameters());
    dtp->setPressure(p->pressure());
    dtp->setRotation(p->rotation());
    dtp->setVelocity(p->velocity());
    QRectF area(QPointF(), p->ellipseDiameters());
    area.moveCenter(p->position());
    dtp->setArea(area);
    dtp->setStartX(p->pressPosition().x());
    dtp->setStartY(p->pressPosition().y());
    dtp->setPreviousX(p->lastPosition().x());
    dtp->setPreviousY(p->lastPosition().y());
    dtp->setSceneX(p->scenePosition().x());
    dtp->setSceneY(p->scenePosition().y());
}

void QQuickMultiPointTouchArea::updateTouchPoint(QQuickTouchPoint *dtp, const QMouseEvent *e)
{
    dtp->setPreviousX(dtp->x());
    dtp->setPreviousY(dtp->y());
    dtp->setPosition(e->position());
    if (e->type() == QEvent::MouseButtonPress) {
        dtp->setStartX(e->position().x());
        dtp->setStartY(e->position().y());
    }
    dtp->setSceneX(e->scenePosition().x());
    dtp->setSceneY(e->scenePosition().y());
}

void QQuickMultiPointTouchArea::mousePressEvent(QMouseEvent *event)
{
    if (!isEnabled() || !_mouseEnabled || event->button() != Qt::LeftButton) {
        QQuickItem::mousePressEvent(event);
        return;
    }

    _stealMouse = false;
    setKeepMouseGrab(false);
    event->setAccepted(true);
    _mousePos = event->position();
    if (event->source() != Qt::MouseEventNotSynthesized && event->source() != Qt::MouseEventSynthesizedByQt)
        return;

    if (_touchPoints.size() >= _minimumTouchPoints - 1 && _touchPoints.size() < _maximumTouchPoints) {
        updateTouchData(event);
    }
}

void QQuickMultiPointTouchArea::mouseMoveEvent(QMouseEvent *event)
{
    if (!isEnabled() || !_mouseEnabled) {
        QQuickItem::mouseMoveEvent(event);
        return;
    }

    if (event->source() != Qt::MouseEventNotSynthesized && event->source() != Qt::MouseEventSynthesizedByQt)
        return;

    _movedTouchPoints.clear();
    updateTouchData(event);
}

void QQuickMultiPointTouchArea::mouseReleaseEvent(QMouseEvent *event)
{
    _stealMouse = false;
    if (!isEnabled() || !_mouseEnabled) {
        QQuickItem::mouseReleaseEvent(event);
        return;
    }

    if (event->source() != Qt::MouseEventNotSynthesized && event->source() != Qt::MouseEventSynthesizedByQt)
        return;

    if (_mouseTouchPoint) {
        updateTouchData(event);
        _mouseTouchPoint->setInUse(false);
        _releasedTouchPoints.removeAll(_mouseTouchPoint);
        _mouseTouchPoint = nullptr;
    }

    setKeepMouseGrab(false);
}

void QQuickMultiPointTouchArea::ungrab(bool normalRelease)
{
    _stealMouse = false;
    setKeepMouseGrab(false);
    setKeepTouchGrab(false);
    if (!normalRelease)
        ungrabTouchPoints();

    if (_touchPoints.size()) {
        for (QObject *obj : std::as_const(_touchPoints))
            static_cast<QQuickTouchPoint*>(obj)->setPressed(false);
        if (!normalRelease)
            emit canceled(_touchPoints.values());
        clearTouchLists();
        for (QObject *obj : std::as_const(_touchPoints)) {
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
    const QPointF localPos = mapFromScene(event->scenePosition());

    QQuickWindow *c = window();
    QQuickItem *grabber = c ? c->mouseGrabberItem() : nullptr;
    bool stealThisEvent = _stealMouse;
    if ((stealThisEvent || contains(localPos)) && (!grabber || !grabber->keepMouseGrab())) {
        QMutableSinglePointEvent mouseEvent(*event);
        const auto oldPosition = mouseEvent.position();
        QMutableEventPoint::setPosition(mouseEvent.point(0), localPos);
        mouseEvent.setSource(Qt::MouseEventSynthesizedByQt);
        mouseEvent.setAccepted(false);
        QMouseEvent *pmouseEvent = static_cast<QMouseEvent *>(static_cast<QSinglePointEvent *>(&mouseEvent));

        switch (mouseEvent.type()) {
        case QEvent::MouseMove:
            mouseMoveEvent(pmouseEvent);
            break;
        case QEvent::MouseButtonPress:
            mousePressEvent(pmouseEvent);
            break;
        case QEvent::MouseButtonRelease:
            mouseReleaseEvent(pmouseEvent);
            break;
        default:
            break;
        }
        grabber = c ? c->mouseGrabberItem() : nullptr;
        if (grabber && stealThisEvent && !grabber->keepMouseGrab() && grabber != this)
            grabMouse();

        QMutableEventPoint::setPosition(mouseEvent.point(0), oldPosition);
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

bool QQuickMultiPointTouchArea::childMouseEventFilter(QQuickItem *receiver, QEvent *event)
{
    if (!isEnabled() || !isVisible())
        return QQuickItem::childMouseEventFilter(receiver, event);
    switch (event->type()) {
    case QEvent::MouseButtonPress: {
        auto da = QQuickItemPrivate::get(this)->deliveryAgentPrivate();
        // If we already got a chance to filter the touchpoint that generated this synth-mouse-press,
        // and chose not to filter it, ignore it now, too.
        if (static_cast<QMouseEvent *>(event)->source() == Qt::MouseEventSynthesizedByQt &&
                _lastFilterableTouchPointIds.contains(da->touchMouseId))
            return false;
        } Q_FALLTHROUGH();
    case QEvent::MouseMove:
    case QEvent::MouseButtonRelease:
        return sendMouseEvent(static_cast<QMouseEvent *>(event));
    case QEvent::TouchBegin:
        _lastFilterableTouchPointIds.clear();
        Q_FALLTHROUGH();
    case QEvent::TouchUpdate:
        for (const auto &tp : static_cast<QTouchEvent*>(event)->points()) {
            if (tp.state() == QEventPoint::State::Pressed)
                _lastFilterableTouchPointIds << tp.id();
        }
        if (!shouldFilter(event))
            return false;
        updateTouchData(event, RemapEventPoints::ToLocal);
        return _stealMouse;
    case QEvent::TouchEnd: {
            if (!shouldFilter(event))
                return false;
            updateTouchData(event, RemapEventPoints::ToLocal);
            ungrab(true);
        }
        break;
    default:
        break;
    }
    return QQuickItem::childMouseEventFilter(receiver, event);
}

bool QQuickMultiPointTouchArea::shouldFilter(QEvent *event)
{
    QQuickWindow *c = window();
    QQuickItem *grabber = c ? c->mouseGrabberItem() : nullptr;
    bool disabledItem = grabber && !grabber->isEnabled();
    bool stealThisEvent = _stealMouse;
    bool containsPoint = false;
    if (!stealThisEvent) {
        switch (event->type()) {
        case QEvent::MouseButtonPress:
        case QEvent::MouseMove:
        case QEvent::MouseButtonRelease: {
                QMouseEvent *me = static_cast<QMouseEvent*>(event);
                containsPoint = contains(mapFromScene(me->scenePosition()));
            }
            break;
        case QEvent::TouchBegin:
        case QEvent::TouchUpdate:
        case QEvent::TouchEnd: {
                QTouchEvent *te = static_cast<QTouchEvent*>(event);
                for (const QEventPoint &point : te->points()) {
                    if (contains(mapFromScene(point.scenePosition()))) {
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

    if (!qmlMptaVisualTouchDebugging())
        return nullptr;

    QSGInternalRectangleNode *rectangle = static_cast<QSGInternalRectangleNode *>(oldNode);
    if (!rectangle) rectangle = QQuickItemPrivate::get(this)->sceneGraphContext()->createInternalRectangleNode();

    rectangle->setRect(QRectF(0, 0, width(), height()));
    rectangle->setColor(QColor(255, 0, 0, 50));
    rectangle->update();
    return rectangle;
}

QT_END_NAMESPACE

#include "moc_qquickmultipointtoucharea_p.cpp"
