// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickdial_p.h"
#include "qquickdeferredexecute_p_p.h"

#include <QtCore/qmath.h>
#include <QtQuick/private/qquickflickable_p.h>
#include <QtQuickTemplates2/private/qquickcontrol_p_p.h>

#include <cmath>

QT_BEGIN_NAMESPACE

/*!
    \qmltype Dial
    \inherits Control
//!     \nativetype QQuickDial
    \inqmlmodule QtQuick.Controls
    \since 5.7
    \ingroup qtquickcontrols-input
    \brief Circular dial that is rotated to set a value.

    The Dial is similar to a traditional dial knob that is found on devices
    such as stereos or industrial equipment. It allows the user to specify a
    value within a range.

    \image qtquickcontrols-dial-no-wrap.gif

    The value of the dial is set with the \l value property. The range is
    set with the \l from and \l to properties. To enable or disable wrapping,
    use the \l wrap property.

    The dial can be manipulated with a keyboard. It supports the following
    actions:

    \table
    \header \li \b {Action} \li \b {Key}
    \row \li Decrease \l value by \l stepSize \li \c Qt.Key_Left
    \row \li Decrease \l value by \l stepSize \li \c Qt.Key_Down
    \row \li Set \l value to \l from \li \c Qt.Key_Home
    \row \li Increase \l value by \l stepSize \li \c Qt.Key_Right
    \row \li Increase \l value by \l stepSize \li \c Qt.Key_Up
    \row \li Set \l value to \l to \li \c Qt.Key_End
    \endtable

    \include qquickdial.qdocinc inputMode

    \sa {Customizing Dial}, {Input Controls}
*/

/*!
    \since QtQuick.Controls 2.2 (Qt 5.9)
    \qmlsignal QtQuick.Controls::Dial::moved()

    This signal is emitted when the dial has been interactively moved
    by the user by either touch, mouse, or keys.
*/

/*!
    \qmlsignal QtQuick.Controls::Dial::wrapped(Dial.WrapDirection direction)
    \since 6.6

    This signal is emitted when the dial wraps around, i.e. goes beyond its
    maximum value to its minimum value, or vice versa. It is only emitted when
    \l wrap is \c true.
    The \a direction argument specifies the direction of the full rotation and
    will be one of the following arguments:

    \value Dial.Clockwise           The dial wrapped in clockwise direction.
    \value Dial.CounterClockwise    The dial wrapped in counterclockwise direction.
*/

// The user angle is the clockwise angle between the position and the vertical
// y-axis (12 o clock position).
// Using radians for logic (atan2(...)) and degree for user interface
constexpr qreal toUserAngleDeg(qreal logicAngleRad) {
    // minus to turn clockwise, add 90 deg clockwise
    return -logicAngleRad / M_PI * 180. + 90;
}

static const qreal defaultStartAngle = -140;
static const qreal defaultEndAngle = 140;

class QQuickDialPrivate : public QQuickControlPrivate
{
    Q_DECLARE_PUBLIC(QQuickDial)

public:
    qreal valueAt(qreal position) const;
    qreal snapPosition(qreal position) const;
    qreal positionAt(const QPointF &point) const;
    qreal circularPositionAt(const QPointF &point) const;
    qreal linearPositionAt(const QPointF &point) const;
    void setPosition(qreal position);
    void updatePosition();
    bool isLargeChange(qreal proposedPosition) const;
    bool isHorizontalOrVertical() const;

    bool handlePress(const QPointF &point, ulong timestamp) override;
    bool handleMove(const QPointF &point, ulong timestamp) override;
    bool handleRelease(const QPointF &point, ulong timestamp) override;
    void handleUngrab() override;

    void cancelHandle();
    void executeHandle(bool complete = false);

    void updateAllValuesAreInteger();

    void maybeEmitWrapAround(qreal pos);

    qreal from = 0;
    qreal to = 1;
    qreal value = 0;
    qreal position = 0;
    qreal startAngle = defaultStartAngle;
    qreal endAngle = defaultEndAngle;
    qreal angle = startAngle;
    qreal stepSize = 0;
    QPointF pressPoint;
    qreal positionBeforePress = 0;
    QQuickDial::SnapMode snapMode = QQuickDial::NoSnap;
    QQuickDial::InputMode inputMode = QQuickDial::Circular;
    QQuickDeferredPointer<QQuickItem> handle;
    bool wrap = false;
    bool live = true;
    bool pressed = false;
    bool allValuesAreInteger = false;
};

qreal QQuickDialPrivate::valueAt(qreal position) const
{
    qreal value = from + (to - from) * position;

    /* play nice with users expecting that integer from, to and stepSize leads to
       integer values - given that we are using floating point internally (and in
       the API of value), this does not hold, but it is easy enough to handle
    */
    if (allValuesAreInteger)
        value = qRound(value);

    return value;
}

qreal QQuickDialPrivate::snapPosition(qreal position) const
{
    const qreal range = to - from;
    if (qFuzzyIsNull(range))
        return position;

    const qreal effectiveStep = stepSize / range;
    if (qFuzzyIsNull(effectiveStep))
        return position;

    return qRound(position / effectiveStep) * effectiveStep;
}

qreal QQuickDialPrivate::positionAt(const QPointF &point) const
{
    return inputMode == QQuickDial::Circular ? circularPositionAt(point) : linearPositionAt(point);
}

qreal QQuickDialPrivate::circularPositionAt(const QPointF &point) const
{
    qreal yy = height / 2.0 - point.y();
    qreal xx = point.x() - width / 2.0;
    qreal alpha = (xx || yy) ? toUserAngleDeg(std::atan2(yy, xx)) : 0;

    // Move around the circle to reach the interval.
    if (alpha < startAngle && alpha + 360. < endAngle)
        alpha += 360.;
    else if (alpha >= endAngle && alpha - 360. >= startAngle)
        alpha -= 360.;

    // If wrap is on and we are out of the interval [startAngle, endAngle],
    // we want to jump to the closest border to make it feel nice and responsive
    if ((alpha < startAngle || alpha > endAngle) && wrap) {
        if (abs(alpha - startAngle) > abs(endAngle - alpha - 360.))
            alpha += 360.;
        else if (abs(alpha - startAngle - 360.) < abs(endAngle - alpha))
            alpha -= 360.;
    }

    // If wrap is off,
    // we want to stay as close as possible to the current angle.
    // This is important to allow easy setting of boundary values (0,1)
    if (!wrap) {
        if (abs(angle - alpha) > abs(angle - (alpha + 360.)))
            alpha += 360.;
        if (abs(angle - alpha) > abs(angle - (alpha - 360.)))
            alpha -= 360.;
    }

    return (alpha - startAngle) / (endAngle - startAngle);
}

qreal QQuickDialPrivate::linearPositionAt(const QPointF &point) const
{
    // This value determines the range (either horizontal or vertical)
    // within which the dial can be dragged.
    // The larger this value is, the further the drag distance
    // must be to go from a position of e.g. 0.0 to 1.0.
    qreal dragArea = 0;

    // The linear input mode uses a "relative" input system,
    // where the distance from the press point is used to calculate
    // the change in position. Moving the mouse above the press
    // point increases the position (when inputMode is Vertical),
    // and vice versa. This prevents the dial from jumping when clicked.
    qreal dragDistance = 0;

    if (inputMode == QQuickDial::Horizontal) {
        dragArea = width * 2;
        dragDistance = pressPoint.x() - point.x();
    } else {
        dragArea = height * 2;
        dragDistance = point.y() - pressPoint.y();
    }
    const qreal normalisedDifference = dragDistance / dragArea;
    return qBound(qreal(0), positionBeforePress - normalisedDifference, qreal(1));
}

void QQuickDialPrivate::setPosition(qreal pos)
{
    Q_Q(QQuickDial);
    pos = qBound<qreal>(qreal(0), pos, qreal(1));
    const qreal alpha = startAngle + pos * qAbs(endAngle - startAngle);
    if (qFuzzyCompare(position, pos) && qFuzzyCompare(angle, alpha))
        return;

    angle = alpha;
    position = pos;


    emit q->positionChanged();
    emit q->angleChanged();
}

void QQuickDialPrivate::updatePosition()
{
    qreal pos = 0;
    if (!qFuzzyCompare(from, to))
        pos = (value - from) / (to - from);
    setPosition(pos);
}

bool QQuickDialPrivate::isLargeChange(qreal proposedPosition) const
{
    if (endAngle - startAngle < 180.0)
        return false;
    return qAbs(proposedPosition - position) > qreal(0.5);
}

bool QQuickDialPrivate::isHorizontalOrVertical() const
{
    return inputMode == QQuickDial::Horizontal || inputMode == QQuickDial::Vertical;
}

bool QQuickDialPrivate::handlePress(const QPointF &point, ulong timestamp)
{
    Q_Q(QQuickDial);
    QQuickControlPrivate::handlePress(point, timestamp);
    pressPoint = point;
    positionBeforePress = position;
    q->setPressed(true);
    return true;
}

bool QQuickDialPrivate::handleMove(const QPointF &point, ulong timestamp)
{
    Q_Q(QQuickDial);
    QQuickControlPrivate::handleMove(point, timestamp);
    const qreal oldPos = position;
    qreal pos = qBound(0.0, positionAt(point), 1.0);
    if (snapMode == QQuickDial::SnapAlways)
        pos = snapPosition(pos);

    maybeEmitWrapAround(pos);

    if (wrap || isHorizontalOrVertical() || !isLargeChange(pos)) {
        if (live)
            q->setValue(valueAt(pos));
        else
            setPosition(pos);
        if (!qFuzzyCompare(pos, oldPos))
            emit q->moved();
    }
    return true;
}

bool QQuickDialPrivate::handleRelease(const QPointF &point, ulong timestamp)
{
    Q_Q(QQuickDial);
    QQuickControlPrivate::handleRelease(point, timestamp);
    if (q->keepMouseGrab() || q->keepTouchGrab()) {
        const qreal oldPos = position;
        qreal pos = positionAt(point);
        if (snapMode != QQuickDial::NoSnap)
            pos = snapPosition(pos);

        maybeEmitWrapAround(pos);

        if (wrap || isHorizontalOrVertical() || !isLargeChange(pos))
            q->setValue(valueAt(pos));
        if (!qFuzzyCompare(pos, oldPos))
            emit q->moved();

        q->setKeepMouseGrab(false);
        q->setKeepTouchGrab(false);
    }

    q->setPressed(false);
    pressPoint = QPointF();
    positionBeforePress = 0;
    return true;
}

void QQuickDialPrivate::handleUngrab()
{
    Q_Q(QQuickDial);
    QQuickControlPrivate::handleUngrab();
    pressPoint = QPointF();
    positionBeforePress = 0;
    q->setPressed(false);
}

void QQuickDialPrivate::cancelHandle()
{
    Q_Q(QQuickDial);
    quickCancelDeferred(q, handleName());
}

void QQuickDialPrivate::executeHandle(bool complete)
{
    Q_Q(QQuickDial);
    if (handle.wasExecuted())
        return;

    if (!handle || complete)
        quickBeginDeferred(q, handleName(), handle);
    if (complete)
        quickCompleteDeferred(q, handleName(), handle);
}

template<typename ...Real>
static bool areRepresentableAsInteger(Real... numbers) {
    auto check = [](qreal number) -> bool { return std::nearbyint(number) == number; };
    return (... && check(numbers));
}

void QQuickDialPrivate::updateAllValuesAreInteger()
{
    allValuesAreInteger = areRepresentableAsInteger(to, from, stepSize) && stepSize != 0.0;
}

void QQuickDialPrivate::maybeEmitWrapAround(qreal pos)
{
    Q_Q(QQuickDial);

    if (wrap && isLargeChange(pos))
        emit q->wrapped((pos < q->position()) ? QQuickDial::Clockwise : QQuickDial::CounterClockwise);
}

QQuickDial::QQuickDial(QQuickItem *parent)
    : QQuickControl(*(new QQuickDialPrivate), parent)
{
    setActiveFocusOnTab(true);
    setAcceptedMouseButtons(Qt::LeftButton);
#if QT_CONFIG(quicktemplates2_multitouch)
    setAcceptTouchEvents(true);
#endif
#if QT_CONFIG(cursor)
    setCursor(Qt::ArrowCursor);
#endif
    Q_D(QQuickDial);
    d->setSizePolicy(QLayoutPolicy::Preferred, QLayoutPolicy::Preferred);
}

/*!
    \qmlproperty real QtQuick.Controls::Dial::from

    This property holds the starting value for the range. The default value is \c 0.0.

    \sa to, value
*/
qreal QQuickDial::from() const
{
    Q_D(const QQuickDial);
    return d->from;
}

void QQuickDial::setFrom(qreal from)
{
    Q_D(QQuickDial);
    if (qFuzzyCompare(d->from, from))
        return;

    d->from = from;
    emit fromChanged();
    d->updateAllValuesAreInteger();
    if (isComponentComplete()) {
        setValue(d->value);
        d->updatePosition();
    }
}

/*!
    \qmlproperty real QtQuick.Controls::Dial::to

    This property holds the end value for the range. The default value is
    \c 1.0.

    \sa from, value
*/
qreal QQuickDial::to() const
{
    Q_D(const QQuickDial);
    return d->to;
}

void QQuickDial::setTo(qreal to)
{
    Q_D(QQuickDial);
    if (qFuzzyCompare(d->to, to))
        return;

    d->to = to;
    d->updateAllValuesAreInteger();
    emit toChanged();
    if (isComponentComplete()) {
        setValue(d->value);
        d->updatePosition();
    }
}

/*!
    \qmlproperty real QtQuick.Controls::Dial::value

    This property holds the value in the range \c from - \c to. The default
    value is \c 0.0.

    \sa position, live
*/
qreal QQuickDial::value() const
{
    Q_D(const QQuickDial);
    return d->value;
}

void QQuickDial::setValue(qreal value)
{
    Q_D(QQuickDial);
    if (isComponentComplete())
        value = d->from > d->to ? qBound(d->to, value, d->from) : qBound(d->from, value, d->to);

    if (qFuzzyCompare(d->value, value))
        return;

    d->value = value;
    d->updatePosition();
    emit valueChanged();
}

/*!
    \qmlproperty real QtQuick.Controls::Dial::position
    \readonly

    This property holds the logical position of the handle.

    The position is expressed as a fraction of the control's angle range (the
    range within which the handle can be moved) in the range \c {0.0 - 1.0}.

    \sa value, angle
*/
qreal QQuickDial::position() const
{
    Q_D(const QQuickDial);
    return d->position;
}

/*!
    \qmlproperty real QtQuick.Controls::Dial::angle
    \readonly

    This property holds the clockwise angle of the handle in degrees.

    The angle is zero at the 12 o'clock position and the range is from
    \l startAngle to \c endAngle.

    \sa position, startAngle, endAngle
*/
qreal QQuickDial::angle() const
{
    Q_D(const QQuickDial);
    return d->angle;
}

/*!
    \qmlproperty real QtQuick.Controls::Dial::stepSize

    This property holds the step size.

    The step size determines the amount by which the dial's value
    is increased and decreased when interacted with via the keyboard.
    For example, a step size of \c 0.2, will result in the dial's
    value increasing and decreasing in increments of \c 0.2.

    The step size is only respected for touch and mouse interaction
    when \l snapMode is set to a value other than \c Dial.NoSnap.

    The default value is \c 0.0, which results in an effective step
    size of \c 0.1 for keyboard interaction.

    \sa snapMode, increase(), decrease()
*/
qreal QQuickDial::stepSize() const
{
    Q_D(const QQuickDial);
    return d->stepSize;
}

void QQuickDial::setStepSize(qreal step)
{
    Q_D(QQuickDial);
    if (qFuzzyCompare(d->stepSize, step))
        return;

    d->stepSize = step;
    d->updateAllValuesAreInteger();
    emit stepSizeChanged();
}


/*!
    \qmlproperty real QtQuick.Controls::Dial::startAngle
    \since 6.6

    This property holds the starting angle of the dial in degrees.

    This is the \l angle the dial will have for its minimum value, i.e. \l from.
    The \l startAngle has to be smaller than the \l endAngle, larger than -360
    and larger or equal to the \l endAngle - 360 degrees.

    \sa endAngle, angle
*/
qreal QQuickDial::startAngle() const
{
    Q_D(const QQuickDial);
    return d->startAngle;
}

void QQuickDial::setStartAngle(qreal startAngle)
{
    Q_D(QQuickDial);
    if (!d->componentComplete) {
        // Binding evaluation order can cause warnings with certain combinations
        // of start and end angles, so delay the actual setting until after component completion.
        // Store the requested value in the existing member to avoid the need for an extra one.
        d->startAngle = startAngle;
        return;
    }

    if (qFuzzyCompare(d->startAngle, startAngle))
        return;

    // do not allow to change direction
    if (startAngle >= d->endAngle) {
        qmlWarning(this) << "startAngle (" << startAngle
            << ") cannot be greater than or equal to endAngle (" << d->endAngle << ")";
        return;
    }

    // Keep the interval around 0
    if (startAngle <= -360.) {
        qmlWarning(this) << "startAngle (" << startAngle << ") cannot be less than or equal to -360";
        return;
    }

    // keep the interval [startAngle, endAngle] unique
    if (startAngle < d->endAngle - 360.) {
        qmlWarning(this) << "Difference between startAngle (" << startAngle
            << ") and endAngle (" << d->endAngle << ") cannot be greater than 360."
            << " Changing endAngle to avoid overlaps.";
        d->endAngle = startAngle + 360.;
        emit endAngleChanged();
    }

    d->startAngle = startAngle;
    // changing the startAngle will change the angle
    // if the value is kept constant
    d->updatePosition();
    emit startAngleChanged();
}

/*!
    \qmlproperty real QtQuick.Controls::Dial::endAngle
    \since 6.6

    This property holds the end angle of the dial in degrees.

    This is the \l angle the dial will have for its maximum value, i.e. \l to.
    The \l endAngle has to be bigger than the \l startAngle, smaller than 720
    and smaller or equal than the \l startAngle + 360 degrees.

    \sa startAngle, angle
*/
qreal QQuickDial::endAngle() const
{
    Q_D(const QQuickDial);
    return d->endAngle;

}

void QQuickDial::setEndAngle(qreal endAngle)
{
    Q_D(QQuickDial);
    if (!d->componentComplete) {
        // Binding evaluation order can cause warnings with certain combinations
        // of start and end angles, so delay the actual setting until after component completion.
        // Store the requested value in the existing member to avoid the need for an extra one.
        d->endAngle = endAngle;
        return;
    }

    if (qFuzzyCompare(d->endAngle, endAngle))
        return;

    if (endAngle <= d->startAngle) {
        qmlWarning(this) << "endAngle (" << endAngle
            << ") cannot be less than or equal to startAngle (" << d->startAngle << ")";
        return;
    }

    // Keep the interval around 0
    if (endAngle >= 720.) {
        qmlWarning(this) << "endAngle (" << endAngle << ") cannot be greater than or equal to 720";
        return;
    }

    // keep the interval [startAngle, endAngle] unique
    if (endAngle > d->startAngle + 360.) {
        qmlWarning(this) << "Difference between startAngle (" << d->startAngle
            << ") and endAngle (" << endAngle << ") cannot be greater than 360."
            << " Changing startAngle to avoid overlaps.";
        d->startAngle = endAngle - 360.;
        emit startAngleChanged();
    }

    d->endAngle = endAngle;
    // changing the startAngle will change the angle
    // if the value is kept constant
    d->updatePosition();
    emit endAngleChanged();
}

/*!
    \qmlproperty enumeration QtQuick.Controls::Dial::snapMode

    This property holds the snap mode.

    The snap mode works with the \l stepSize to allow the handle to snap to
    certain points along the dial.

    Possible values:
    \value Dial.NoSnap The dial does not snap (default).
    \value Dial.SnapAlways The dial snaps while the handle is dragged.
    \value Dial.SnapOnRelease The dial does not snap while being dragged, but only after the handle is released.

    \sa stepSize
*/
QQuickDial::SnapMode QQuickDial::snapMode() const
{
    Q_D(const QQuickDial);
    return d->snapMode;
}

void QQuickDial::setSnapMode(SnapMode mode)
{
    Q_D(QQuickDial);
    if (d->snapMode == mode)
        return;

    d->snapMode = mode;
    emit snapModeChanged();
}

/*!
    \since QtQuick.Controls 2.5 (Qt 5.12)
    \qmlproperty enumeration QtQuick.Controls::Dial::inputMode

    This property holds the input mode.

    \include qquickdial.qdocinc inputMode

    The default value is \c Dial.Circular.
*/
QQuickDial::InputMode QQuickDial::inputMode() const
{
    Q_D(const QQuickDial);
    return d->inputMode;
}

void QQuickDial::setInputMode(QQuickDial::InputMode mode)
{
    Q_D(QQuickDial);
    if (d->inputMode == mode)
        return;

    d->inputMode = mode;
    emit inputModeChanged();
}

/*!
    \qmlproperty bool QtQuick.Controls::Dial::wrap

    This property holds whether the dial wraps when dragged.

    For example, when this property is set to \c true, dragging the dial past
    the \l to position will result in the handle being positioned at the
    \l from position, and vice versa:

    \image qtquickcontrols-dial-wrap.gif

    When this property is \c false, it's not possible to drag the dial across
    the from and to values.

    \image qtquickcontrols-dial-no-wrap.gif

    The default value is \c false.
*/
bool QQuickDial::wrap() const
{
    Q_D(const QQuickDial);
    return d->wrap;
}

void QQuickDial::setWrap(bool wrap)
{
    Q_D(QQuickDial);
    if (d->wrap == wrap)
        return;

    d->wrap = wrap;
    emit wrapChanged();
}

/*!
    \qmlproperty bool QtQuick.Controls::Dial::pressed

    This property holds whether the dial is pressed.

    The dial will be pressed when either the mouse is pressed over it, or a key
    such as \c Qt.Key_Left is held down. If you'd prefer not to have the dial
    be pressed upon key presses (due to styling reasons, for example), you can
    use the \l {Keys}{Keys attached property}:

    \code
    Dial {
        Keys.onLeftPressed: {}
    }
    \endcode

    This will result in pressed only being \c true upon mouse presses.
*/
bool QQuickDial::isPressed() const
{
    Q_D(const QQuickDial);
    return d->pressed;
}

void QQuickDial::setPressed(bool pressed)
{
    Q_D(QQuickDial);
    if (d->pressed == pressed)
        return;

    d->pressed = pressed;
    setAccessibleProperty("pressed", pressed);
    emit pressedChanged();
}

/*!
    \qmlproperty Item QtQuick.Controls::Dial::handle

    This property holds the handle of the dial.

    The handle acts as a visual indicator of the position of the dial.

    \sa {Customizing Dial}
*/
QQuickItem *QQuickDial::handle() const
{
    QQuickDialPrivate *d = const_cast<QQuickDialPrivate *>(d_func());
    if (!d->handle)
        d->executeHandle();
    return d->handle;
}

void QQuickDial::setHandle(QQuickItem *handle)
{
    Q_D(QQuickDial);
    if (handle == d->handle)
        return;

    QQuickControlPrivate::warnIfCustomizationNotSupported(this, handle, QStringLiteral("handle"));

    if (!d->handle.isExecuting())
        d->cancelHandle();

    QQuickControlPrivate::hideOldItem(d->handle);
    d->handle = handle;
    if (d->handle && !d->handle->parentItem())
        d->handle->setParentItem(this);
    if (!d->handle.isExecuting())
        emit handleChanged();
}

/*!
    \since QtQuick.Controls 2.2 (Qt 5.9)
    \qmlproperty bool QtQuick.Controls::Dial::live

    This property holds whether the dial provides live updates for the \l value
    property while the handle is dragged.

    The default value is \c true.

    \sa value
*/
bool QQuickDial::live() const
{
    Q_D(const QQuickDial);
    return d->live;
}

void QQuickDial::setLive(bool live)
{
    Q_D(QQuickDial);
    if (d->live == live)
        return;

    d->live = live;
    emit liveChanged();
}

/*!
    \qmlmethod void QtQuick.Controls::Dial::increase()

    Increases the value by \l stepSize, or \c 0.1 if stepSize is not defined.

    \sa stepSize
*/
void QQuickDial::increase()
{
    Q_D(QQuickDial);
    qreal step = qFuzzyIsNull(d->stepSize) ? 0.1 : d->stepSize;
    setValue(d->value + step);
}

/*!
    \qmlmethod void QtQuick.Controls::Dial::decrease()

    Decreases the value by \l stepSize, or \c 0.1 if stepSize is not defined.

    \sa stepSize
*/
void QQuickDial::decrease()
{
    Q_D(QQuickDial);
    qreal step = qFuzzyIsNull(d->stepSize) ? 0.1 : d->stepSize;
    setValue(d->value - step);
}

void QQuickDial::keyPressEvent(QKeyEvent *event)
{
    Q_D(QQuickDial);
    const qreal oldValue = d->value;
    switch (event->key()) {
    case Qt::Key_Left:
    case Qt::Key_Down:
        setPressed(true);
        if (isMirrored())
            increase();
        else
            decrease();
        break;

    case Qt::Key_Right:
    case Qt::Key_Up:
        setPressed(true);
        if (isMirrored())
            decrease();
        else
            increase();
        break;

    case Qt::Key_Home:
        setPressed(true);
        setValue(isMirrored() ? d->to : d->from);
        break;

    case Qt::Key_End:
        setPressed(true);
        setValue(isMirrored() ? d->from : d->to);
        break;

    default:
        event->ignore();
        QQuickControl::keyPressEvent(event);
        break;
    }
    if (!qFuzzyCompare(d->value, oldValue))
        emit moved();
}

void QQuickDial::keyReleaseEvent(QKeyEvent *event)
{
    QQuickControl::keyReleaseEvent(event);
    setPressed(false);
}

void QQuickDial::mousePressEvent(QMouseEvent *event)
{
    Q_D(QQuickDial);
    QQuickControl::mousePressEvent(event);
    d->handleMove(event->position(), event->timestamp());
    setKeepMouseGrab(true);
}

#if QT_CONFIG(quicktemplates2_multitouch)
void QQuickDial::touchEvent(QTouchEvent *event)
{
    Q_D(QQuickDial);
    switch (event->type()) {
    case QEvent::TouchUpdate:
        for (const QTouchEvent::TouchPoint &point : event->points()) {
            if (!d->acceptTouch(point))
                continue;

            switch (point.state()) {
            case QEventPoint::Updated:
                if (!keepTouchGrab()) {
                    bool overXDragThreshold = QQuickDeliveryAgentPrivate::dragOverThreshold(point.position().x() - d->pressPoint.x(),
                                                                            Qt::XAxis, point);
                    setKeepTouchGrab(overXDragThreshold);

                    if (!overXDragThreshold) {
                        bool overYDragThreshold = QQuickDeliveryAgentPrivate::dragOverThreshold(point.position().y() - d->pressPoint.y(),
                                                                            Qt::YAxis, point);
                        setKeepTouchGrab(overYDragThreshold);
                    }
                }
                if (keepTouchGrab())
                    d->handleMove(point.position(), event->timestamp());
                break;

            default:
                QQuickControl::touchEvent(event);
                break;
            }
        }
        break;

    default:
        QQuickControl::touchEvent(event);
        break;
    }
}
#endif

#if QT_CONFIG(wheelevent)
void QQuickDial::wheelEvent(QWheelEvent *event)
{
    Q_D(QQuickDial);
    QQuickControl::wheelEvent(event);
    if (d->wheelEnabled) {
        const qreal oldValue = d->value;
        const QPointF angle = event->angleDelta();
        const qreal delta = (qFuzzyIsNull(angle.y()) ? angle.x() : (event->inverted() ? -angle.y() : angle.y())) / int(QWheelEvent::DefaultDeltasPerStep);
        const qreal step = qFuzzyIsNull(d->stepSize) ? 0.1 : d->stepSize;
        setValue(oldValue + step * delta);
        event->setAccepted(!qFuzzyCompare(d->value, oldValue));
    }
}
#endif

void QQuickDial::mirrorChange()
{
    QQuickControl::mirrorChange();
    emit angleChanged();
}

void QQuickDial::componentComplete()
{
    Q_D(QQuickDial);
    d->executeHandle(true);
    QQuickControl::componentComplete();

    // Set the (delayed) start and end angles, if necessary (see the setters for more info).
    if (!qFuzzyCompare(d->startAngle, defaultStartAngle)) {
        const qreal startAngle = d->startAngle;
        // Temporarily set it to something else so that it sees that it has changed.
        d->startAngle = defaultStartAngle;
        setStartAngle(startAngle);
    }

    if (!qFuzzyCompare(d->endAngle, defaultEndAngle)) {
        const qreal endAngle = d->endAngle;
        d->endAngle = defaultEndAngle;
        setEndAngle(endAngle);
    }

    setValue(d->value);
    d->updatePosition();
}

#if QT_CONFIG(accessibility)
void QQuickDial::accessibilityActiveChanged(bool active)
{
    QQuickControl::accessibilityActiveChanged(active);

    Q_D(QQuickDial);
    if (active)
        setAccessibleProperty("pressed", d->pressed);
}

QAccessible::Role QQuickDial::accessibleRole() const
{
    return QAccessible::Dial;
}
#endif

QT_END_NAMESPACE

#include "moc_qquickdial_p.cpp"
