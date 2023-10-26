// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickflickable_p.h"
#include "qquickflickable_p_p.h"
#include "qquickflickablebehavior_p.h"
#include "qquickwindow.h"
#include "qquickwindow_p.h"
#include "qquickmousearea_p.h"
#if QT_CONFIG(quick_draganddrop)
#include "qquickdrag_p.h"
#endif

#include <QtQuick/private/qquickpointerhandler_p.h>
#include <QtQuick/private/qquicktransition_p.h>
#include <private/qqmlglobal_p.h>

#include <QtQml/qqmlinfo.h>
#include <QtGui/qevent.h>
#include <QtGui/qguiapplication.h>
#include <QtGui/private/qguiapplication_p.h>
#include <QtGui/private/qeventpoint_p.h>
#include <QtGui/qstylehints.h>
#include <QtCore/qmath.h>
#include <qpa/qplatformintegration.h>

#include <math.h>
#include <cmath>

QT_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(lcHandlerParent)
Q_LOGGING_CATEGORY(lcFlickable, "qt.quick.flickable")
Q_LOGGING_CATEGORY(lcFilter, "qt.quick.flickable.filter")
Q_LOGGING_CATEGORY(lcReplay, "qt.quick.flickable.replay")
Q_LOGGING_CATEGORY(lcWheel, "qt.quick.flickable.wheel")
Q_LOGGING_CATEGORY(lcVel, "qt.quick.flickable.velocity")

// RetainGrabVelocity is the maxmimum instantaneous velocity that
// will ensure the Flickable retains the grab on consecutive flicks.
static const int RetainGrabVelocity = 100;

// Currently std::round can't be used on Android when using ndk g++, so
// use C version instead. We could just define two versions of Round, one
// for float and one for double, but then only one of them would be used
// and compiler would trigger a warning about unused function.
//
// See https://code.google.com/p/android/issues/detail?id=54418
template<typename T>
static T Round(T t) {
    return round(t);
}
template<>
Q_DECL_UNUSED float Round<float>(float f) {
    return roundf(f);
}

static qreal EaseOvershoot(qreal t) {
    return qAtan(t);
}

QQuickFlickableVisibleArea::QQuickFlickableVisibleArea(QQuickFlickable *parent)
    : QObject(parent), flickable(parent), m_xPosition(0.), m_widthRatio(0.)
    , m_yPosition(0.), m_heightRatio(0.)
{
}

qreal QQuickFlickableVisibleArea::widthRatio() const
{
    return m_widthRatio;
}

qreal QQuickFlickableVisibleArea::xPosition() const
{
    return m_xPosition;
}

qreal QQuickFlickableVisibleArea::heightRatio() const
{
    return m_heightRatio;
}

qreal QQuickFlickableVisibleArea::yPosition() const
{
    return m_yPosition;
}

void QQuickFlickableVisibleArea::updateVisible()
{
    QQuickFlickablePrivate *p = QQuickFlickablePrivate::get(flickable);

    bool changeX = false;
    bool changeY = false;
    bool changeWidth = false;
    bool changeHeight = false;

    // Vertical
    const qreal viewheight = flickable->height();
    const qreal maxyextent = -flickable->maxYExtent() + flickable->minYExtent();
    const qreal maxYBounds = maxyextent + viewheight;
    qreal pagePos = 0;
    qreal pageSize = 0;
    if (!qFuzzyIsNull(maxYBounds)) {
        qreal y = p->pixelAligned ? Round(p->vData.move.value()) : p->vData.move.value();
        pagePos = (-y + flickable->minYExtent()) / maxYBounds;
        pageSize = viewheight / maxYBounds;
    }

    if (pageSize != m_heightRatio) {
        m_heightRatio = pageSize;
        changeHeight = true;
    }
    if (pagePos != m_yPosition) {
        m_yPosition = pagePos;
        changeY = true;
    }

    // Horizontal
    const qreal viewwidth = flickable->width();
    const qreal maxxextent = -flickable->maxXExtent() + flickable->minXExtent();
    const qreal maxXBounds = maxxextent + viewwidth;
    if (!qFuzzyIsNull(maxXBounds)) {
        qreal x = p->pixelAligned ? Round(p->hData.move.value()) : p->hData.move.value();
        pagePos = (-x + flickable->minXExtent()) / maxXBounds;
        pageSize = viewwidth / maxXBounds;
    } else {
        pagePos = 0;
        pageSize = 0;
    }

    if (pageSize != m_widthRatio) {
        m_widthRatio = pageSize;
        changeWidth = true;
    }
    if (pagePos != m_xPosition) {
        m_xPosition = pagePos;
        changeX = true;
    }

    if (changeX)
        emit xPositionChanged(m_xPosition);
    if (changeY)
        emit yPositionChanged(m_yPosition);
    if (changeWidth)
        emit widthRatioChanged(m_widthRatio);
    if (changeHeight)
        emit heightRatioChanged(m_heightRatio);
}


class QQuickFlickableReboundTransition : public QQuickTransitionManager
{
public:
    QQuickFlickableReboundTransition(QQuickFlickable *f, const QString &name)
        : flickable(f), axisData(nullptr), propName(name), active(false)
    {
    }

    ~QQuickFlickableReboundTransition()
    {
        flickable = nullptr;
    }

    bool startTransition(QQuickFlickablePrivate::AxisData *data, qreal toPos) {
        QQuickFlickablePrivate *fp = QQuickFlickablePrivate::get(flickable);
        if (!fp->rebound || !fp->rebound->enabled())
            return false;
        active = true;
        axisData = data;
        axisData->transitionTo = toPos;
        axisData->transitionToSet = true;

        actions.clear();
        actions << QQuickStateAction(fp->contentItem, propName, toPos);
        QQuickTransitionManager::transition(actions, fp->rebound, fp->contentItem);
        return true;
    }

    bool isActive() const {
        return active;
    }

    void stopTransition() {
        if (!flickable || !isRunning())
            return;
        QQuickFlickablePrivate *fp = QQuickFlickablePrivate::get(flickable);
        if (axisData == &fp->hData)
            axisData->move.setValue(-flickable->contentX());
        else
            axisData->move.setValue(-flickable->contentY());
        active = false;
        cancel();
    }

protected:
    void finished() override {
        if (!flickable)
            return;
        axisData->move.setValue(axisData->transitionTo);
        QQuickFlickablePrivate *fp = QQuickFlickablePrivate::get(flickable);
        active = false;

        if (!fp->hData.transitionToBounds->isActive()
                && !fp->vData.transitionToBounds->isActive()) {
            flickable->movementEnding();
        }
    }

private:
    QQuickStateOperation::ActionList actions;
    QQuickFlickable *flickable;
    QQuickFlickablePrivate::AxisData *axisData;
    QString propName;
    bool active;
};

QQuickFlickablePrivate::AxisData::~AxisData()
{
    delete transitionToBounds;
}

class QQuickFlickableContentItem : public QQuickItem
{
    /*!
        \internal
        The flickable area inside the viewport can be bigger than the bounds of the
        content item itself, if the flickable is using non-zero extents (as returned
        by e.g minXExtent()). Since the default implementation in QQuickItem::contains()
        only checks if the point is inside the bounds of the item, we need to override it
        to check the extents as well. The easist way to do this is to simply check if the
        point is inside the bounds of the flickable rather than the content item.
    */
    bool contains(const QPointF &point) const override
    {
        const QQuickItem *flickable = parentItem();
        const QPointF posInFlickable = flickable->mapFromItem(this, point);
        return flickable->contains(posInFlickable);
    }
};

QQuickFlickablePrivate::QQuickFlickablePrivate()
  : contentItem(new QQuickFlickableContentItem)
    , hData(this, &QQuickFlickablePrivate::setViewportX)
    , vData(this, &QQuickFlickablePrivate::setViewportY)
    , hMoved(false), vMoved(false)
    , stealMouse(false), pressed(false)
    , scrollingPhase(false), interactive(true), calcVelocity(false)
    , pixelAligned(false)
    , syncDrag(false)
    , lastPosTime(-1)
    , lastPressTime(0)
    , deceleration(QGuiApplicationPrivate::platformIntegration()->styleHint(QPlatformIntegration::FlickDeceleration).toReal())
    , wheelDeceleration(15000)
    , maxVelocity(QGuiApplicationPrivate::platformIntegration()->styleHint(QPlatformIntegration::FlickMaximumVelocity).toReal())
    , delayedPressEvent(nullptr), pressDelay(0), fixupDuration(400)
    , flickBoost(1.0), initialWheelFlickDistance(qApp->styleHints()->wheelScrollLines() * 24)
    , fixupMode(Normal), vTime(0), visibleArea(nullptr)
    , flickableDirection(QQuickFlickable::AutoFlickDirection)
    , boundsBehavior(QQuickFlickable::DragAndOvershootBounds)
    , boundsMovement(QQuickFlickable::FollowBoundsBehavior)
    , rebound(nullptr)
{
    const int wheelDecelerationEnv = qEnvironmentVariableIntValue("QT_QUICK_FLICKABLE_WHEEL_DECELERATION");
    if (wheelDecelerationEnv > 0)
        wheelDeceleration = wheelDecelerationEnv;
}

void QQuickFlickablePrivate::init()
{
    Q_Q(QQuickFlickable);
    QQml_setParent_noEvent(contentItem, q);
    contentItem->setParentItem(q);
    qmlobject_connect(&timeline, QQuickTimeLine, SIGNAL(completed()),
                      q, QQuickFlickable, SLOT(timelineCompleted()));
    qmlobject_connect(&velocityTimeline, QQuickTimeLine, SIGNAL(completed()),
                      q, QQuickFlickable, SLOT(velocityTimelineCompleted()));
    q->setAcceptedMouseButtons(Qt::LeftButton);
    q->setAcceptTouchEvents(true);
    q->setFiltersChildMouseEvents(true);
    q->setFlag(QQuickItem::ItemIsViewport);
    QQuickItemPrivate *viewportPrivate = QQuickItemPrivate::get(contentItem);
    viewportPrivate->addItemChangeListener(this, QQuickItemPrivate::Geometry);
}

/*!
    \internal
    Returns the distance to overshoot, given \a velocity.
    Will be in range 0 - velocity / 3, but limited to a max of QML_FLICK_OVERSHOOT
*/
qreal QQuickFlickablePrivate::overShootDistance(qreal velocity) const
{
    if (maxVelocity <= 0)
        return 0;

    return qMin(qreal(QML_FLICK_OVERSHOOT), velocity / 3);
}

void QQuickFlickablePrivate::AxisData::addVelocitySample(qreal v, qreal maxVelocity)
{
    if (v > maxVelocity)
        v = maxVelocity;
    else if (v < -maxVelocity)
        v = -maxVelocity;
    velocityBuffer.append(v);
    if (velocityBuffer.count() > QML_FLICK_SAMPLEBUFFER)
        velocityBuffer.remove(0);
}

void QQuickFlickablePrivate::AxisData::updateVelocity()
{
    velocity = 0;
    if (velocityBuffer.count() > QML_FLICK_DISCARDSAMPLES) {
        int count = velocityBuffer.count()-QML_FLICK_DISCARDSAMPLES;
        for (int i = 0; i < count; ++i) {
            qreal v = velocityBuffer.at(i);
            velocity += v;
        }
        velocity /= count;
    }
}

void QQuickFlickablePrivate::itemGeometryChanged(QQuickItem *item, QQuickGeometryChange change, const QRectF &oldGeom)
{
    Q_Q(QQuickFlickable);
    if (item == contentItem) {
        Qt::Orientations orient;
        if (change.xChange())
            orient |= Qt::Horizontal;
        if (change.yChange())
            orient |= Qt::Vertical;
        if (orient) {
            q->viewportMoved(orient);
            const QPointF deltaMoved = item->position() - oldGeom.topLeft();
            if (hData.contentPositionChangedExternallyDuringDrag)
                hData.pressPos += deltaMoved.x();
            if (vData.contentPositionChangedExternallyDuringDrag)
                vData.pressPos += deltaMoved.y();
        }
        if (orient & Qt::Horizontal)
            emit q->contentXChanged();
        if (orient & Qt::Vertical)
            emit q->contentYChanged();
    }
}

bool QQuickFlickablePrivate::flickX(QEvent::Type eventType, qreal velocity)
{
    Q_Q(QQuickFlickable);
    return flick(hData, q->minXExtent(), q->maxXExtent(), q->width(), fixupX_callback, eventType, velocity);
}

bool QQuickFlickablePrivate::flickY(QEvent::Type eventType, qreal velocity)
{
    Q_Q(QQuickFlickable);
    return flick(vData, q->minYExtent(), q->maxYExtent(), q->height(), fixupY_callback, eventType, velocity);
}

bool QQuickFlickablePrivate::flick(AxisData &data, qreal minExtent, qreal maxExtent, qreal,
                                   QQuickTimeLineCallback::Callback fixupCallback,
                                   QEvent::Type eventType, qreal velocity)
{
    Q_Q(QQuickFlickable);
    qreal maxDistance = -1;
    data.fixingUp = false;
    // -ve velocity means list is moving up
    if (velocity > 0) {
        maxDistance = qAbs(minExtent - data.move.value());
        data.flickTarget = minExtent;
    } else {
        maxDistance = qAbs(maxExtent - data.move.value());
        data.flickTarget = maxExtent;
    }
    if (maxDistance > 0 || boundsBehavior & QQuickFlickable::OvershootBounds) {
        qreal v = velocity;
        if (maxVelocity != -1 && maxVelocity < qAbs(v)) {
            if (v < 0)
                v = -maxVelocity;
            else
                v = maxVelocity;
        }

        qreal accel = eventType == QEvent::Wheel ? wheelDeceleration : deceleration;
        qCDebug(lcFlickable) << "choosing deceleration" << accel << "for" << eventType;
        // adjust accel so that we hit a full pixel
        qreal v2 = v * v;
        qreal dist = v2 / (accel * 2.0);
        if (v > 0)
            dist = -dist;
        qreal target = -Round(-(data.move.value() - dist));
        dist = -target + data.move.value();
        accel = v2 / (2.0f * qAbs(dist));

        resetTimeline(data);
        if (!data.inOvershoot) {
            if (boundsBehavior & QQuickFlickable::OvershootBounds)
                timeline.accel(data.move, v, accel);
            else
                timeline.accel(data.move, v, accel, maxDistance);
        }
        timeline.callback(QQuickTimeLineCallback(&data.move, fixupCallback, this));

        if (&data == &hData)
            return !hData.flicking && q->xflick();
        else if (&data == &vData)
            return !vData.flicking && q->yflick();
        return false;
    } else {
        resetTimeline(data);
        fixup(data, minExtent, maxExtent);
        return false;
    }
}

void QQuickFlickablePrivate::fixupY_callback(void *data)
{
    ((QQuickFlickablePrivate *)data)->fixupY();
}

void QQuickFlickablePrivate::fixupX_callback(void *data)
{
    ((QQuickFlickablePrivate *)data)->fixupX();
}

void QQuickFlickablePrivate::fixupX()
{
    Q_Q(QQuickFlickable);
    if (!q->isComponentComplete())
        return; //Do not fixup from initialization values
    fixup(hData, q->minXExtent(), q->maxXExtent());
}

void QQuickFlickablePrivate::fixupY()
{
    Q_Q(QQuickFlickable);
    if (!q->isComponentComplete())
        return; //Do not fixup from initialization values
    fixup(vData, q->minYExtent(), q->maxYExtent());
}

/*!
    \internal

    Adjusts the contentItem's position via the timeline.
    This function is used by QQuickFlickablePrivate::fixup in order to
    position the contentItem back into the viewport, in case flicking,
    dragging or geometry adjustments moved it outside of bounds.
*/
void QQuickFlickablePrivate::adjustContentPos(AxisData &data, qreal toPos)
{
    Q_Q(QQuickFlickable);
    switch (fixupMode) {
    case Immediate:
        timeline.set(data.move, toPos);
        break;
    case ExtentChanged:
        // The target has changed. Don't start from the beginning; just complete the
        // second half of the animation using the new extent.
        timeline.move(data.move, toPos, QEasingCurve(QEasingCurve::OutExpo), 3*fixupDuration/4);
        data.fixingUp = true;
        break;
    default: {
            if (data.transitionToBounds && data.transitionToBounds->startTransition(&data, toPos)) {
                q->movementStarting();
                data.fixingUp = true;
            } else {
                qreal dist = toPos - data.move;
                timeline.move(data.move, toPos - dist/2, QEasingCurve(QEasingCurve::InQuad), fixupDuration/4);
                timeline.move(data.move, toPos, QEasingCurve(QEasingCurve::OutExpo), 3*fixupDuration/4);
                data.fixingUp = true;
            }
        }
    }
}

void QQuickFlickablePrivate::resetTimeline(AxisData &data)
{
    timeline.reset(data.move);
    if (data.transitionToBounds)
        data.transitionToBounds->stopTransition();
}

void QQuickFlickablePrivate::clearTimeline()
{
    timeline.clear();
    if (hData.transitionToBounds)
        hData.transitionToBounds->stopTransition();
    if (vData.transitionToBounds)
        vData.transitionToBounds->stopTransition();
}

/*!
    \internal

    This function should be called after the contentItem has been moved, either programmatically,
    or by the timeline (as a result of a flick).
    It ensures that the contentItem will be moved back into bounds,
    in case it was flicked outside of the visible area.

    The positional adjustment will usually be animated by the timeline, unless the fixupMode is set to Immediate.
*/
void QQuickFlickablePrivate::fixup(AxisData &data, qreal minExtent, qreal maxExtent)
{
    if (data.move.value() >= minExtent || maxExtent > minExtent) {
        resetTimeline(data);
        if (data.move.value() != minExtent) {
            adjustContentPos(data, minExtent);
        }
    } else if (data.move.value() <= maxExtent) {
        resetTimeline(data);
        adjustContentPos(data, maxExtent);
    } else if (-Round(-data.move.value()) != data.move.value()) {
        // We could animate, but since it is less than 0.5 pixel it's probably not worthwhile.
        resetTimeline(data);
        qreal val = data.move.value();
        if (std::abs(-Round(-val) - val) < 0.25) // round small differences
            val = -Round(-val);
        else if (data.smoothVelocity.value() > 0) // continue direction of motion for larger
            val = -std::floor(-val);
        else if (data.smoothVelocity.value() < 0)
            val = -std::ceil(-val);
        else // otherwise round
            val = -Round(-val);
        timeline.set(data.move, val);
    }
    data.inOvershoot = false;
    fixupMode = Normal;
    data.vTime = timeline.time();
}

static bool fuzzyLessThanOrEqualTo(qreal a, qreal b)
{
    if (a == 0.0 || b == 0.0) {
        // qFuzzyCompare is broken
        a += 1.0;
        b += 1.0;
    }
    return a <= b || qFuzzyCompare(a, b);
}

/*!
    \internal

    This function's main purpose is to update the atBeginning and atEnd flags
    in hData and vData. It should be called when the contentItem has moved,
    to ensure that hData and vData are up to date.

    The origin will also be updated, if AxisData::markExtentsDirty has been called
*/
void QQuickFlickablePrivate::updateBeginningEnd()
{
    Q_Q(QQuickFlickable);
    bool atXBeginningChange = false, atXEndChange = false;
    bool atYBeginningChange = false, atYEndChange = false;

    // Vertical
    const qreal maxyextent = -q->maxYExtent();
    const qreal minyextent = -q->minYExtent();
    const qreal ypos = pixelAligned ? -std::round(vData.move.value()) : -vData.move.value();
    bool atBeginning = fuzzyLessThanOrEqualTo(ypos, std::ceil(minyextent));
    bool atEnd = fuzzyLessThanOrEqualTo(std::floor(maxyextent), ypos);

    if (atBeginning != vData.atBeginning) {
        vData.atBeginning = atBeginning;
        atYBeginningChange = true;
        if (!vData.moving && atBeginning)
            vData.smoothVelocity.setValue(0);
    }
    if (atEnd != vData.atEnd) {
        vData.atEnd = atEnd;
        atYEndChange = true;
        if (!vData.moving && atEnd)
            vData.smoothVelocity.setValue(0);
    }

    // Horizontal
    const qreal maxxextent = -q->maxXExtent();
    const qreal minxextent = -q->minXExtent();
    const qreal xpos = pixelAligned ? -std::round(hData.move.value()) : -hData.move.value();
    atBeginning = fuzzyLessThanOrEqualTo(xpos, std::ceil(minxextent));
    atEnd = fuzzyLessThanOrEqualTo(std::floor(maxxextent), xpos);

    if (atBeginning != hData.atBeginning) {
        hData.atBeginning = atBeginning;
        atXBeginningChange = true;
        if (!hData.moving && atBeginning)
            hData.smoothVelocity.setValue(0);
    }
    if (atEnd != hData.atEnd) {
        hData.atEnd = atEnd;
        atXEndChange = true;
        if (!hData.moving && atEnd)
            hData.smoothVelocity.setValue(0);
    }

    if (vData.extentsChanged) {
        vData.extentsChanged = false;
        qreal originY = q->originY();
        if (vData.origin != originY) {
            vData.origin = originY;
            emit q->originYChanged();
        }
    }

    if (hData.extentsChanged) {
        hData.extentsChanged = false;
        qreal originX = q->originX();
        if (hData.origin != originX) {
            hData.origin = originX;
            emit q->originXChanged();
        }
    }

    if (atXEndChange || atYEndChange || atXBeginningChange || atYBeginningChange)
        emit q->isAtBoundaryChanged();
    if (atXEndChange)
        emit q->atXEndChanged();
    if (atXBeginningChange)
        emit q->atXBeginningChanged();
    if (atYEndChange)
        emit q->atYEndChanged();
    if (atYBeginningChange)
        emit q->atYBeginningChanged();

    if (visibleArea)
        visibleArea->updateVisible();
}

/*!
    \qmlsignal QtQuick::Flickable::dragStarted()

    This signal is emitted when the view starts to be dragged due to user
    interaction.
*/

/*!
    \qmlsignal QtQuick::Flickable::dragEnded()

    This signal is emitted when the user stops dragging the view.

    If the velocity of the drag is sufficient at the time the
    touch/mouse button is released then a flick will start.
*/

/*!
    \qmltype Flickable
    \instantiates QQuickFlickable
    \inqmlmodule QtQuick
    \ingroup qtquick-input
    \ingroup qtquick-containers

    \brief Provides a surface that can be "flicked".
    \inherits Item

    The Flickable item places its children on a surface that can be dragged
    and flicked, causing the view onto the child items to scroll. This
    behavior forms the basis of Items that are designed to show large numbers
    of child items, such as \l ListView and \l GridView.

    In traditional user interfaces, views can be scrolled using standard
    controls, such as scroll bars and arrow buttons. In some situations, it
    is also possible to drag the view directly by pressing and holding a
    mouse button while moving the cursor. In touch-based user interfaces,
    this dragging action is often complemented with a flicking action, where
    scrolling continues after the user has stopped touching the view.

    Flickable does not automatically clip its contents. If it is not used as
    a full-screen item, you should consider setting the \l{Item::}{clip} property
    to true.

    \section1 Example Usage

    \div {class="float-right"}
    \inlineimage flickable.gif
    \enddiv

    The following example shows a small view onto a large image in which the
    user can drag or flick the image in order to view different parts of it.

    \snippet qml/flickable.qml document

    \clearfloat

    Items declared as children of a Flickable are automatically parented to the
    Flickable's \l contentItem.  This should be taken into account when
    operating on the children of the Flickable; it is usually the children of
    \c contentItem that are relevant.  For example, the bound of Items added
    to the Flickable will be available by \c contentItem.childrenRect

    \section1 Examples of contentX and contentY

    The following images demonstrate a flickable being flicked in various
    directions and the resulting \l contentX and \l contentY values.
    The blue square represents the flickable's content, and the black
    border represents the bounds of the flickable.

    \table
        \row
            \li \image flickable-contentXY-resting.png
            \li The \c contentX and \c contentY are both \c 0.
        \row
            \li \image flickable-contentXY-top-left.png
            \li The \c contentX and the \c contentY are both \c 50.
        \row
            \li \image flickable-contentXY-top-right.png
            \li The \c contentX is \c -50 and the \c contentY is \c 50.
        \row
            \li \image flickable-contentXY-bottom-right.png
            \li The \c contentX and the \c contentY are both \c -50.
        \row
            \li \image flickable-contentXY-bottom-left.png
            \li The \c contentX is \c 50 and the \c contentY is \c -50.
    \endtable

    \section1 Limitations

    \note Due to an implementation detail, items placed inside a Flickable
    cannot anchor to the Flickable. Instead, use \l {Item::}{parent}, which
    refers to the Flickable's \l contentItem. The size of the content item is
    determined by \l contentWidth and \l contentHeight.
*/

/*!
    \qmlsignal QtQuick::Flickable::movementStarted()

    This signal is emitted when the view begins moving due to user
    interaction or a generated flick().
*/

/*!
    \qmlsignal QtQuick::Flickable::movementEnded()

    This signal is emitted when the view stops moving due to user
    interaction or a generated flick().  If a flick was active, this signal will
    be emitted once the flick stops.  If a flick was not
    active, this signal will be emitted when the
    user stops dragging - i.e. a mouse or touch release.
*/

/*!
    \qmlsignal QtQuick::Flickable::flickStarted()

    This signal is emitted when the view is flicked.  A flick
    starts from the point that the mouse or touch is released,
    while still in motion.
*/

/*!
    \qmlsignal QtQuick::Flickable::flickEnded()

    This signal is emitted when the view stops moving after a flick
    or a series of flicks.
*/

/*!
    \qmlpropertygroup QtQuick::Flickable::visibleArea
    \qmlproperty real QtQuick::Flickable::visibleArea.xPosition
    \qmlproperty real QtQuick::Flickable::visibleArea.widthRatio
    \qmlproperty real QtQuick::Flickable::visibleArea.yPosition
    \qmlproperty real QtQuick::Flickable::visibleArea.heightRatio

    These properties describe the position and size of the currently viewed area.
    The size is defined as the percentage of the full view currently visible,
    scaled to 0.0 - 1.0.  The page position is usually in the range 0.0 (beginning) to
    1.0 minus size ratio (end), i.e. \c yPosition is in the range 0.0 to 1.0-\c heightRatio.
    However, it is possible for the contents to be dragged outside of the normal
    range, resulting in the page positions also being outside the normal range.

    These properties are typically used to draw a scrollbar. For example:

    \snippet qml/flickableScrollbar.qml 0
    \dots 8
    \snippet qml/flickableScrollbar.qml 1
*/
QQuickFlickable::QQuickFlickable(QQuickItem *parent)
  : QQuickItem(*(new QQuickFlickablePrivate), parent)
{
    Q_D(QQuickFlickable);
    d->init();
}

QQuickFlickable::QQuickFlickable(QQuickFlickablePrivate &dd, QQuickItem *parent)
  : QQuickItem(dd, parent)
{
    Q_D(QQuickFlickable);
    d->init();
}

QQuickFlickable::~QQuickFlickable()
{
}

/*!
    \qmlproperty real QtQuick::Flickable::contentX
    \qmlproperty real QtQuick::Flickable::contentY

    These properties hold the surface coordinate currently at the top-left
    corner of the Flickable. For example, if you flick an image up 100 pixels,
    \c contentY will increase by 100.

    \note If you flick back to the origin (the top-left corner), after the
    rebound animation, \c contentX will settle to the same value as \c originX,
    and \c contentY to \c originY. These are usually (0,0), however ListView
    and GridView may have an arbitrary origin due to delegate size variation,
    or item insertion/removal outside the visible region. So if you want to
    implement something like a vertical scrollbar, one way is to use
    \c {y: (contentY - originY) * (height / contentHeight)}
    for the position; another way is to use the normalized values in
    \l {QtQuick::Flickable::visibleArea}{visibleArea}.

    \sa {Examples of contentX and contentY}, originX, originY
*/
qreal QQuickFlickable::contentX() const
{
    Q_D(const QQuickFlickable);
    return -d->contentItem->x();
}

void QQuickFlickable::setContentX(qreal pos)
{
    Q_D(QQuickFlickable);
    d->hData.explicitValue = true;
    d->resetTimeline(d->hData);
    d->hData.vTime = d->timeline.time();
    if (isMoving() || isFlicking())
        movementEnding(true, false);
    if (!qFuzzyCompare(-pos, d->hData.move.value())) {
        d->hData.contentPositionChangedExternallyDuringDrag = d->hData.dragging;
        d->hData.move.setValue(-pos);
        d->hData.contentPositionChangedExternallyDuringDrag = false;
    }
}

qreal QQuickFlickable::contentY() const
{
    Q_D(const QQuickFlickable);
    return -d->contentItem->y();
}

void QQuickFlickable::setContentY(qreal pos)
{
    Q_D(QQuickFlickable);
    d->vData.explicitValue = true;
    d->resetTimeline(d->vData);
    d->vData.vTime = d->timeline.time();
    if (isMoving() || isFlicking())
        movementEnding(false, true);
    if (!qFuzzyCompare(-pos, d->vData.move.value())) {
        d->vData.contentPositionChangedExternallyDuringDrag = d->vData.dragging;
        d->vData.move.setValue(-pos);
        d->vData.contentPositionChangedExternallyDuringDrag = false;
    }
}

/*!
    \qmlproperty bool QtQuick::Flickable::interactive

    This property describes whether the user can interact with the Flickable.
    A user cannot drag or flick a Flickable that is not interactive.

    By default, this property is true.

    This property is useful for temporarily disabling flicking. This allows
    special interaction with Flickable's children; for example, you might want
    to freeze a flickable map while scrolling through a pop-up dialog that
    is a child of the Flickable.
*/
bool QQuickFlickable::isInteractive() const
{
    Q_D(const QQuickFlickable);
    return d->interactive;
}

void QQuickFlickable::setInteractive(bool interactive)
{
    Q_D(QQuickFlickable);
    if (interactive != d->interactive) {
        d->interactive = interactive;
        if (!interactive) {
            d->cancelInteraction();
        }
        emit interactiveChanged();
    }
}

/*!
    \qmlproperty real QtQuick::Flickable::horizontalVelocity
    \qmlproperty real QtQuick::Flickable::verticalVelocity

    The instantaneous velocity of movement along the x and y axes, in pixels/sec.

    The reported velocity is smoothed to avoid erratic output.

    Note that for views with a large content size (more than 10 times the view size),
    the velocity of the flick may exceed the velocity of the touch in the case
    of multiple quick consecutive flicks.  This allows the user to flick faster
    through large content.
*/
qreal QQuickFlickable::horizontalVelocity() const
{
    Q_D(const QQuickFlickable);
    return d->hData.smoothVelocity.value();
}

qreal QQuickFlickable::verticalVelocity() const
{
    Q_D(const QQuickFlickable);
    return d->vData.smoothVelocity.value();
}

/*!
    \qmlproperty bool QtQuick::Flickable::atXBeginning
    \qmlproperty bool QtQuick::Flickable::atXEnd
    \qmlproperty bool QtQuick::Flickable::atYBeginning
    \qmlproperty bool QtQuick::Flickable::atYEnd

    These properties are true if the flickable view is positioned at the beginning,
    or end respectively.
*/
bool QQuickFlickable::isAtXEnd() const
{
    Q_D(const QQuickFlickable);
    return d->hData.atEnd;
}

bool QQuickFlickable::isAtXBeginning() const
{
    Q_D(const QQuickFlickable);
    return d->hData.atBeginning;
}

bool QQuickFlickable::isAtYEnd() const
{
    Q_D(const QQuickFlickable);
    return d->vData.atEnd;
}

bool QQuickFlickable::isAtYBeginning() const
{
    Q_D(const QQuickFlickable);
    return d->vData.atBeginning;
}

/*!
    \qmlproperty Item QtQuick::Flickable::contentItem

    The internal item that contains the Items to be moved in the Flickable.

    Items declared as children of a Flickable are automatically parented to the Flickable's contentItem.

    Items created dynamically need to be explicitly parented to the \e contentItem:
    \code
    Flickable {
        id: myFlickable
        function addItem(file) {
            var component = Qt.createComponent(file)
            component.createObject(myFlickable.contentItem);
        }
    }
    \endcode
*/
QQuickItem *QQuickFlickable::contentItem() const
{
    Q_D(const QQuickFlickable);
    return d->contentItem;
}

QQuickFlickableVisibleArea *QQuickFlickable::visibleArea()
{
    Q_D(QQuickFlickable);
    if (!d->visibleArea) {
        d->visibleArea = new QQuickFlickableVisibleArea(this);
        d->visibleArea->updateVisible(); // calculate initial ratios
    }
    return d->visibleArea;
}

/*!
    \qmlproperty enumeration QtQuick::Flickable::flickableDirection

    This property determines which directions the view can be flicked.

    \list
    \li Flickable.AutoFlickDirection (default) - allows flicking vertically if the
    \e contentHeight is not equal to the \e height of the Flickable.
    Allows flicking horizontally if the \e contentWidth is not equal
    to the \e width of the Flickable.
    \li Flickable.AutoFlickIfNeeded - allows flicking vertically if the
    \e contentHeight is greater than the \e height of the Flickable.
    Allows flicking horizontally if the \e contentWidth is greater than
    to the \e width of the Flickable. (since \c{QtQuick 2.7})
    \li Flickable.HorizontalFlick - allows flicking horizontally.
    \li Flickable.VerticalFlick - allows flicking vertically.
    \li Flickable.HorizontalAndVerticalFlick - allows flicking in both directions.
    \endlist
*/
QQuickFlickable::FlickableDirection QQuickFlickable::flickableDirection() const
{
    Q_D(const QQuickFlickable);
    return d->flickableDirection;
}

void QQuickFlickable::setFlickableDirection(FlickableDirection direction)
{
    Q_D(QQuickFlickable);
    if (direction != d->flickableDirection) {
        d->flickableDirection = direction;
        emit flickableDirectionChanged();
    }
}

/*!
    \qmlproperty bool QtQuick::Flickable::pixelAligned

    This property sets the alignment of \l contentX and \l contentY to
    pixels (\c true) or subpixels (\c false).

    Enable pixelAligned to optimize for still content or moving content with
    high constrast edges, such as one-pixel-wide lines, text or vector graphics.
    Disable pixelAligned when optimizing for animation quality.

    The default is \c false.
*/
bool QQuickFlickable::pixelAligned() const
{
    Q_D(const QQuickFlickable);
    return d->pixelAligned;
}

void QQuickFlickable::setPixelAligned(bool align)
{
    Q_D(QQuickFlickable);
    if (align != d->pixelAligned) {
        d->pixelAligned = align;
        emit pixelAlignedChanged();
    }
}

/*!
    \qmlproperty bool QtQuick::Flickable::synchronousDrag
    \since 5.12

    If this property is set to true, then when the mouse or touchpoint moves
    far enough to begin dragging the content, the content will jump, such that
    the content pixel which was under the cursor or touchpoint when pressed
    remains under that point.

    The default is \c false, which provides a smoother experience (no jump)
    at the cost that some of the drag distance is "lost" at the beginning.
*/
bool QQuickFlickable::synchronousDrag() const
{
    Q_D(const QQuickFlickable);
    return d->syncDrag;
}

void QQuickFlickable::setSynchronousDrag(bool v)
{
    Q_D(QQuickFlickable);
    if (v != d->syncDrag) {
        d->syncDrag = v;
        emit synchronousDragChanged();
    }
}

/*! \internal
    Take the velocity of the first point from the given \a event and transform
    it to the local coordinate system (taking scale and rotation into account).
*/
QVector2D QQuickFlickablePrivate::firstPointLocalVelocity(QPointerEvent *event)
{
    QTransform transform = windowToItemTransform();
    // rotate and scale the velocity vector from scene to local
    return QVector2D(transform.map(event->point(0).velocity().toPointF()) - transform.map(QPointF()));
}

qint64 QQuickFlickablePrivate::computeCurrentTime(QInputEvent *event) const
{
    if (0 != event->timestamp())
        return event->timestamp();
    if (!timer.isValid())
        return 0LL;
    return timer.elapsed();
}

qreal QQuickFlickablePrivate::devicePixelRatio() const
{
    return (window ? window->effectiveDevicePixelRatio() : qApp->devicePixelRatio());
}

void QQuickFlickablePrivate::handlePressEvent(QPointerEvent *event)
{
    Q_Q(QQuickFlickable);
    timer.start();
    if (interactive && timeline.isActive()
        && ((qAbs(hData.smoothVelocity.value()) > RetainGrabVelocity && !hData.fixingUp && !hData.inOvershoot)
            || (qAbs(vData.smoothVelocity.value()) > RetainGrabVelocity && !vData.fixingUp && !vData.inOvershoot))) {
        stealMouse = true; // If we've been flicked then steal the click.
        int flickTime = timeline.time();
        if (flickTime > 600) {
            // too long between flicks - cancel boost
            hData.continuousFlickVelocity = 0;
            vData.continuousFlickVelocity = 0;
            flickBoost = 1.0;
        } else {
            hData.continuousFlickVelocity = -hData.smoothVelocity.value();
            vData.continuousFlickVelocity = -vData.smoothVelocity.value();
            if (flickTime > 300) // slower flicking - reduce boost
                flickBoost = qMax(1.0, flickBoost - 0.5);
        }
    } else {
        stealMouse = false;
        hData.continuousFlickVelocity = 0;
        vData.continuousFlickVelocity = 0;
        flickBoost = 1.0;
    }
    q->setKeepMouseGrab(stealMouse);

    maybeBeginDrag(computeCurrentTime(event), event->points().first().position());
}

void QQuickFlickablePrivate::maybeBeginDrag(qint64 currentTimestamp, const QPointF &pressPosn)
{
    Q_Q(QQuickFlickable);
    clearDelayedPress();
    pressed = true;

    if (hData.transitionToBounds)
        hData.transitionToBounds->stopTransition();
    if (vData.transitionToBounds)
        vData.transitionToBounds->stopTransition();
    if (!hData.fixingUp)
        resetTimeline(hData);
    if (!vData.fixingUp)
        resetTimeline(vData);

    hData.reset();
    vData.reset();
    hData.dragMinBound = q->minXExtent() - hData.startMargin;
    vData.dragMinBound = q->minYExtent() - vData.startMargin;
    hData.dragMaxBound = q->maxXExtent() + hData.endMargin;
    vData.dragMaxBound = q->maxYExtent() + vData.endMargin;
    fixupMode = Normal;
    lastPos = QPointF();
    pressPos = pressPosn;
    hData.pressPos = hData.move.value();
    vData.pressPos = vData.move.value();
    const bool wasFlicking = hData.flicking || vData.flicking;
    hData.flickingWhenDragBegan = hData.flicking;
    vData.flickingWhenDragBegan = vData.flicking;
    if (hData.flicking) {
        hData.flicking = false;
        emit q->flickingHorizontallyChanged();
    }
    if (vData.flicking) {
        vData.flicking = false;
        emit q->flickingVerticallyChanged();
    }
    if (wasFlicking)
        emit q->flickingChanged();
    lastPosTime = lastPressTime = currentTimestamp;
    vData.velocityTime.start();
    hData.velocityTime.start();
}

void QQuickFlickablePrivate::drag(qint64 currentTimestamp, QEvent::Type eventType, const QPointF &localPos,
                                  const QVector2D &deltas, bool overThreshold, bool momentum,
                                  bool velocitySensitiveOverBounds, const QVector2D &velocity)
{
    Q_Q(QQuickFlickable);
    bool rejectY = false;
    bool rejectX = false;

    bool keepY = q->yflick();
    bool keepX = q->xflick();

    bool stealY = false;
    bool stealX = false;
    if (eventType == QEvent::MouseMove) {
        stealX = stealY = stealMouse;
    } else if (eventType == QEvent::Wheel) {
        stealX = stealY = scrollingPhase;
    }

    bool prevHMoved = hMoved;
    bool prevVMoved = vMoved;

    qint64 elapsedSincePress = currentTimestamp - lastPressTime;
    qCDebug(lcFlickable).nospace() << currentTimestamp << ' ' << eventType << " drag @ " << localPos.x() << ',' << localPos.y()
                                   << " \u0394 " << deltas.x() << ',' << deltas.y() << " vel " << velocity.x() << ',' << velocity.y()
                                   << " thrsld? " << overThreshold << " momentum? " << momentum << " velSens? " << velocitySensitiveOverBounds
                                   << " sincePress " << elapsedSincePress;

    if (q->yflick()) {
        qreal dy = deltas.y();
        if (overThreshold || elapsedSincePress > 200) {
            if (!vMoved)
                vData.dragStartOffset = dy;
            qreal newY = dy + vData.pressPos - (syncDrag ? 0 : vData.dragStartOffset);
            // Recalculate bounds in case margins have changed, but use the content
            // size estimate taken at the start of the drag in case the drag causes
            // the estimate to be altered
            const qreal minY = vData.dragMinBound + vData.startMargin;
            const qreal maxY = vData.dragMaxBound - vData.endMargin;
            if (!(boundsBehavior & QQuickFlickable::DragOverBounds)) {
                if (fuzzyLessThanOrEqualTo(newY, maxY)) {
                    newY = maxY;
                    rejectY = vData.pressPos == maxY && vData.move.value() == maxY && dy < 0;
                }
                if (fuzzyLessThanOrEqualTo(minY, newY)) {
                    newY = minY;
                    rejectY |= vData.pressPos == minY && vData.move.value() == minY && dy > 0;
                }
            } else {
                qreal vel = velocity.y() / QML_FLICK_OVERSHOOTFRICTION;
                if (vel > 0. && vel > vData.velocity)
                    vData.velocity = qMin(velocity.y() / QML_FLICK_OVERSHOOTFRICTION, maxVelocity);
                else if (vel < 0. && vel < vData.velocity)
                    vData.velocity = qMax(velocity.y() / QML_FLICK_OVERSHOOTFRICTION, -maxVelocity);
                if (newY > minY) {
                    // Overshoot beyond the top.  But don't wait for momentum phase to end before returning to bounds.
                    if (momentum && vData.atBeginning) {
                        if (!vData.inRebound) {
                            vData.inRebound = true;
                            q->returnToBounds();
                        }
                        return;
                    }
                    if (velocitySensitiveOverBounds) {
                        qreal overshoot = (newY - minY) * vData.velocity / maxVelocity / QML_FLICK_OVERSHOOTFRICTION;
                        overshoot = QML_FLICK_OVERSHOOT * devicePixelRatio() * EaseOvershoot(overshoot / QML_FLICK_OVERSHOOT / devicePixelRatio());
                        newY = minY + overshoot;
                    } else {
                        newY = minY + (newY - minY) / 2;
                    }
                } else if (newY < maxY && maxY - minY <= 0) {
                    // Overshoot beyond the bottom.  But don't wait for momentum phase to end before returning to bounds.
                    if (momentum && vData.atEnd) {
                        if (!vData.inRebound) {
                            vData.inRebound = true;
                            q->returnToBounds();
                        }
                        return;
                    }
                    if (velocitySensitiveOverBounds) {
                        qreal overshoot = (newY - maxY) * vData.velocity / maxVelocity / QML_FLICK_OVERSHOOTFRICTION;
                        overshoot = QML_FLICK_OVERSHOOT * devicePixelRatio() * EaseOvershoot(overshoot / QML_FLICK_OVERSHOOT / devicePixelRatio());
                        newY = maxY - overshoot;
                    } else {
                        newY = maxY + (newY - maxY) / 2;
                    }
                }
            }
            if (!rejectY && stealMouse && dy != 0.0 && dy != vData.previousDragDelta) {
                clearTimeline();
                vData.move.setValue(newY);
                vMoved = true;
            }
            if (!rejectY && overThreshold)
                stealY = true;

            if ((newY >= minY && vData.pressPos == minY && vData.move.value() == minY && dy > 0)
                        || (newY <= maxY && vData.pressPos == maxY && vData.move.value() == maxY && dy < 0)) {
                keepY = false;
            }
        }
        vData.previousDragDelta = dy;
    }

    if (q->xflick()) {
        qreal dx = deltas.x();
        if (overThreshold || elapsedSincePress > 200) {
            if (!hMoved)
                hData.dragStartOffset = dx;
            qreal newX = dx + hData.pressPos - (syncDrag ? 0 : hData.dragStartOffset);
            const qreal minX = hData.dragMinBound + hData.startMargin;
            const qreal maxX = hData.dragMaxBound - hData.endMargin;
            if (!(boundsBehavior & QQuickFlickable::DragOverBounds)) {
                if (fuzzyLessThanOrEqualTo(newX, maxX)) {
                    newX = maxX;
                    rejectX = hData.pressPos == maxX && hData.move.value() == maxX && dx < 0;
                }
                if (fuzzyLessThanOrEqualTo(minX, newX)) {
                    newX = minX;
                    rejectX |= hData.pressPos == minX && hData.move.value() == minX && dx > 0;
                }
            } else {
                qreal vel = velocity.x() / QML_FLICK_OVERSHOOTFRICTION;
                if (vel > 0. && vel > hData.velocity)
                    hData.velocity = qMin(velocity.x() / QML_FLICK_OVERSHOOTFRICTION, maxVelocity);
                else if (vel < 0. && vel < hData.velocity)
                    hData.velocity = qMax(velocity.x() / QML_FLICK_OVERSHOOTFRICTION, -maxVelocity);
                if (newX > minX) {
                    // Overshoot beyond the left.  But don't wait for momentum phase to end before returning to bounds.
                    if (momentum && hData.atBeginning) {
                        if (!hData.inRebound) {
                            hData.inRebound = true;
                            q->returnToBounds();
                        }
                        return;
                    }
                    if (velocitySensitiveOverBounds) {
                        qreal overshoot = (newX - minX) * hData.velocity / maxVelocity / QML_FLICK_OVERSHOOTFRICTION;
                        overshoot = QML_FLICK_OVERSHOOT * devicePixelRatio() * EaseOvershoot(overshoot / QML_FLICK_OVERSHOOT / devicePixelRatio());
                        newX = minX + overshoot;
                    } else {
                        newX = minX + (newX - minX) / 2;
                    }
                } else if (newX < maxX && maxX - minX <= 0) {
                    // Overshoot beyond the right.  But don't wait for momentum phase to end before returning to bounds.
                    if (momentum && hData.atEnd) {
                        if (!hData.inRebound) {
                            hData.inRebound = true;
                            q->returnToBounds();
                        }
                        return;
                    }
                    if (velocitySensitiveOverBounds) {
                        qreal overshoot = (newX - maxX) * hData.velocity / maxVelocity / QML_FLICK_OVERSHOOTFRICTION;
                        overshoot = QML_FLICK_OVERSHOOT * devicePixelRatio() * EaseOvershoot(overshoot / QML_FLICK_OVERSHOOT / devicePixelRatio());
                        newX = maxX - overshoot;
                    } else {
                        newX = maxX + (newX - maxX) / 2;
                    }
                }
            }

            if (!rejectX && stealMouse && dx != 0.0 && dx != hData.previousDragDelta) {
                clearTimeline();
                hData.move.setValue(newX);
                hMoved = true;
            }

            if (!rejectX && overThreshold)
                stealX = true;

            if ((newX >= minX && vData.pressPos == minX && vData.move.value() == minX && dx > 0)
                        || (newX <= maxX && vData.pressPos == maxX && vData.move.value() == maxX && dx < 0)) {
                keepX = false;
            }
        }
        hData.previousDragDelta = dx;
    }

    stealMouse = stealX || stealY;
    if (stealMouse) {
        if ((stealX && keepX) || (stealY && keepY))
            q->setKeepMouseGrab(true);
        clearDelayedPress();
    }

    if (rejectY) {
        vData.velocityBuffer.clear();
        vData.velocity = 0;
    }
    if (rejectX) {
        hData.velocityBuffer.clear();
        hData.velocity = 0;
    }

    if (momentum && !hData.flicking && !vData.flicking)
        flickingStarted(hData.velocity != 0, vData.velocity != 0);
    draggingStarting();

    if ((hMoved && !prevHMoved) || (vMoved && !prevVMoved))
        q->movementStarting();

    lastPosTime = currentTimestamp;
    if (q->yflick() && !rejectY)
        vData.addVelocitySample(velocity.y(), maxVelocity);
    if (q->xflick() && !rejectX)
        hData.addVelocitySample(velocity.x(), maxVelocity);
    lastPos = localPos;
}

void QQuickFlickablePrivate::handleMoveEvent(QPointerEvent *event)
{
    Q_Q(QQuickFlickable);
    if (!interactive || lastPosTime == -1 ||
            (event->isSinglePointEvent() && !static_cast<QSinglePointEvent *>(event)->buttons().testFlag(Qt::LeftButton)))
        return;

    qint64 currentTimestamp = computeCurrentTime(event);
    const auto &firstPoint = event->points().first();
    const auto &pos = firstPoint.position();
    const QVector2D deltas = QVector2D(pos - q->mapFromGlobal(firstPoint.globalPressPosition()));
    const QVector2D velocity = firstPointLocalVelocity(event);
    bool overThreshold = false;

    if (event->pointCount() == 1) {
        if (q->yflick())
            overThreshold |= QQuickDeliveryAgentPrivate::dragOverThreshold(deltas.y(), Qt::YAxis, firstPoint);
        if (q->xflick())
            overThreshold |= QQuickDeliveryAgentPrivate::dragOverThreshold(deltas.x(), Qt::XAxis, firstPoint);
    } else {
        qCDebug(lcFilter) << q->objectName() << "ignoring multi-touch" << event;
    }

    drag(currentTimestamp, event->type(), pos, deltas, overThreshold, false, false, velocity);
}

void QQuickFlickablePrivate::handleReleaseEvent(QPointerEvent *event)
{
    Q_Q(QQuickFlickable);
    stealMouse = false;
    q->setKeepMouseGrab(false);
    pressed = false;

    // if we drag then pause before release we should not cause a flick.
    qint64 elapsed = computeCurrentTime(event) - lastPosTime;

    vData.updateVelocity();
    hData.updateVelocity();

    draggingEnding();

    if (lastPosTime == -1)
        return;

    hData.vTime = vData.vTime = timeline.time();

    bool canBoost = false;
    const auto pos = event->points().first().position();
    const auto pressPos = q->mapFromGlobal(event->points().first().globalPressPosition());
    const QVector2D eventVelocity = firstPointLocalVelocity(event);
    qCDebug(lcVel) << event->deviceType() << event->type() << "velocity" << event->points().first().velocity() << "transformed to local" << eventVelocity;

    qreal vVelocity = 0;
    if (elapsed < 100 && vData.velocity != 0.) {
        vVelocity = (event->device()->capabilities().testFlag(QInputDevice::Capability::Velocity)
                ? eventVelocity.y() : vData.velocity);
    }
    if ((vData.atBeginning && vVelocity > 0.) || (vData.atEnd && vVelocity < 0.)) {
        vVelocity /= 2;
    } else if (vData.continuousFlickVelocity != 0.0
               && vData.viewSize/q->height() > QML_FLICK_MULTIFLICK_RATIO
               && ((vVelocity > 0) == (vData.continuousFlickVelocity > 0))
               && qAbs(vVelocity) > QML_FLICK_MULTIFLICK_THRESHOLD) {
        // accelerate flick for large view flicked quickly
        canBoost = true;
    }

    qreal hVelocity = 0;
    if (elapsed < 100 && hData.velocity != 0.) {
        hVelocity = (event->device()->capabilities().testFlag(QInputDevice::Capability::Velocity)
                     ? eventVelocity.x() : hData.velocity);
    }
    if ((hData.atBeginning && hVelocity > 0.) || (hData.atEnd && hVelocity < 0.)) {
        hVelocity /= 2;
    } else if (hData.continuousFlickVelocity != 0.0
               && hData.viewSize/q->width() > QML_FLICK_MULTIFLICK_RATIO
               && ((hVelocity > 0) == (hData.continuousFlickVelocity > 0))
               && qAbs(hVelocity) > QML_FLICK_MULTIFLICK_THRESHOLD) {
        // accelerate flick for large view flicked quickly
        canBoost = true;
    }

    flickBoost = canBoost ? qBound(1.0, flickBoost+0.25, QML_FLICK_MULTIFLICK_MAXBOOST) : 1.0;
    const int flickThreshold = QGuiApplicationPrivate::platformIntegration()->styleHint(QPlatformIntegration::FlickStartDistance).toInt();

    bool flickedVertically = false;
    vVelocity *= flickBoost;
    bool isVerticalFlickAllowed = q->yflick() && qAbs(vVelocity) > _q_MinimumFlickVelocity && qAbs(pos.y() - pressPos.y()) > flickThreshold;
    if (isVerticalFlickAllowed) {
        velocityTimeline.reset(vData.smoothVelocity);
        vData.smoothVelocity.setValue(-vVelocity);
        flickedVertically = flickY(event->type(), vVelocity);
    }

    bool flickedHorizontally = false;
    hVelocity *= flickBoost;
    bool isHorizontalFlickAllowed = q->xflick() && qAbs(hVelocity) > _q_MinimumFlickVelocity && qAbs(pos.x() - pressPos.x()) > flickThreshold;
    if (isHorizontalFlickAllowed) {
        velocityTimeline.reset(hData.smoothVelocity);
        hData.smoothVelocity.setValue(-hVelocity);
        flickedHorizontally = flickX(event->type(), hVelocity);
    }

    if (!isVerticalFlickAllowed)
        fixupY();

    if (!isHorizontalFlickAllowed)
        fixupX();

    flickingStarted(flickedHorizontally, flickedVertically);
    if (!isViewMoving()) {
        q->movementEnding();
    } else {
        if (flickedVertically)
            vMoved = true;
        if (flickedHorizontally)
            hMoved = true;
        q->movementStarting();
    }
}

void QQuickFlickable::mousePressEvent(QMouseEvent *event)
{
    Q_D(QQuickFlickable);
    if (d->interactive && !d->replayingPressEvent && d->wantsPointerEvent(event)) {
        if (!d->pressed)
            d->handlePressEvent(event);
        event->accept();
    } else {
        QQuickItem::mousePressEvent(event);
    }
}

void QQuickFlickable::mouseMoveEvent(QMouseEvent *event)
{
    Q_D(QQuickFlickable);
    if (d->interactive && d->wantsPointerEvent(event)) {
        d->handleMoveEvent(event);
        event->accept();
    } else {
        QQuickItem::mouseMoveEvent(event);
    }
}

void QQuickFlickable::mouseReleaseEvent(QMouseEvent *event)
{
    Q_D(QQuickFlickable);
    if (d->interactive && d->wantsPointerEvent(event)) {
        if (d->delayedPressEvent) {
            d->replayDelayedPress();

            auto &firstPoint = event->point(0);
            if (const auto *grabber = event->exclusiveGrabber(firstPoint); grabber && grabber->isQuickItemType()) {
                // Since we sent the delayed press to the window, we need to resend the release to the window too.
                // We're not copying or detaching, so restore the original event position afterwards.
                const auto oldPosition = firstPoint.position();
                QMutableEventPoint::setPosition(firstPoint, event->scenePosition());
                QCoreApplication::sendEvent(window(), event);
                QMutableEventPoint::setPosition(firstPoint, oldPosition);
            }

            // And the event has been consumed
            d->stealMouse = false;
            d->pressed = false;
            return;
        }

        d->handleReleaseEvent(event);
        event->accept();
    } else {
        QQuickItem::mouseReleaseEvent(event);
    }
}

void QQuickFlickable::touchEvent(QTouchEvent *event)
{
    Q_D(QQuickFlickable);

    if (event->type() == QEvent::TouchCancel) {
        if (d->interactive && d->wantsPointerEvent(event))
            d->cancelInteraction();
        else
            QQuickItem::touchEvent(event);
        return;
    }

    bool unhandled = false;
    const auto &firstPoint = event->points().first();
    switch (firstPoint.state()) {
    case QEventPoint::State::Pressed:
        if (d->interactive && !d->replayingPressEvent && d->wantsPointerEvent(event)) {
            if (!d->pressed)
                d->handlePressEvent(event);
            event->accept();
        } else {
            unhandled = true;
        }
        break;
    case QEventPoint::State::Updated:
        if (d->interactive && d->wantsPointerEvent(event)) {
            d->handleMoveEvent(event);
            event->accept();
        } else {
            unhandled = true;
        }
        break;
    case QEventPoint::State::Released:
        if (d->interactive && d->wantsPointerEvent(event)) {
            if (d->delayedPressEvent) {
                d->replayDelayedPress();

                const auto &firstPoint = event->point(0);
                if (const auto *grabber = event->exclusiveGrabber(firstPoint); grabber && grabber->isQuickItemType()) {
                    // Since we sent the delayed press to the window, we need to resend the release to the window too.
                    QScopedPointer<QPointerEvent> localizedEvent(
                            QQuickDeliveryAgentPrivate::clonePointerEvent(event, firstPoint.scenePosition()));
                    QCoreApplication::sendEvent(window(), localizedEvent.data());
                }

                // And the event has been consumed
                d->stealMouse = false;
                d->pressed = false;
                return;
            }

            d->handleReleaseEvent(event);
            event->accept();
        } else {
            unhandled = true;
        }
        break;
    case QEventPoint::State::Stationary:
    case QEventPoint::State::Unknown:
        break;
    }
    if (unhandled)
        QQuickItem::touchEvent(event);
}

#if QT_CONFIG(wheelevent)
void QQuickFlickable::wheelEvent(QWheelEvent *event)
{
    Q_D(QQuickFlickable);
    if (!d->interactive || !d->wantsPointerEvent(event)) {
        QQuickItem::wheelEvent(event);
        return;
    }
    qCDebug(lcWheel) << event->device() << event << event->source();
    event->setAccepted(false);
    qint64 currentTimestamp = d->computeCurrentTime(event);
    switch (event->phase()) {
    case Qt::ScrollBegin:
        d->scrollingPhase = true;
        d->accumulatedWheelPixelDelta = QVector2D();
        d->vData.velocity = 0;
        d->hData.velocity = 0;
        d->timer.start();
        d->maybeBeginDrag(currentTimestamp, event->position());
        d->lastPosTime = -1;
        break;
    case Qt::NoScrollPhase: // default phase with an ordinary wheel mouse
    case Qt::ScrollUpdate:
        if (d->scrollingPhase)
            d->pressed = true;
        break;
    case Qt::ScrollMomentum:
        d->pressed = false;
        d->scrollingPhase = false;
        d->draggingEnding();
        if (isMoving())
            event->accept();
        d->lastPosTime = -1;
        break;
    case Qt::ScrollEnd:
        d->pressed = false;
        d->scrollingPhase = false;
        d->draggingEnding();
        event->accept();
        returnToBounds();
        d->lastPosTime = -1;
        d->stealMouse = false;
        if (!d->velocityTimeline.isActive() && !d->timeline.isActive())
            movementEnding(true, true);
        return;
    }

    qreal elapsed = qreal(currentTimestamp - d->lastPosTime) / qreal(1000);
    if (elapsed <= 0) {
        d->lastPosTime = currentTimestamp;
        qCDebug(lcWheel) << "insufficient elapsed time: can't calculate velocity" << elapsed;
        return;
    }

    if (event->source() == Qt::MouseEventNotSynthesized || event->pixelDelta().isNull() || event->phase() == Qt::NoScrollPhase) {
        // no pixel delta (physical mouse wheel, or "dumb" touchpad), so use angleDelta
        int xDelta = event->angleDelta().x();
        int yDelta = event->angleDelta().y();

        if (d->wheelDeceleration > _q_MaximumWheelDeceleration) {
            const qreal wheelScroll = -qApp->styleHints()->wheelScrollLines() * 24;
            // If wheelDeceleration is very large, i.e. the user or the platform does not want to have any mouse wheel
            // acceleration behavior, we want to move a distance proportional to QStyleHints::wheelScrollLines()
            if (yflick() && yDelta != 0) {
                d->moveReason = QQuickFlickablePrivate::Mouse; // ItemViews will set fixupMode to Immediate in fixup() without this.
                d->vMoved = true;
                qreal scrollPixel = (-yDelta / 120.0 * wheelScroll);
                if (d->boundsBehavior == QQuickFlickable::StopAtBounds) {
                    const qreal estContentPos = scrollPixel + d->vData.move.value();
                    if (scrollPixel > 0) { // Forward direction (away from user)
                        if (d->vData.move.value() >= minYExtent())
                            d->vMoved = false;
                        else if (estContentPos > minYExtent())
                            scrollPixel = minYExtent() - d->vData.move.value();
                    } else { // Backward direction (towards user)
                        if (d->vData.move.value() <= maxYExtent())
                            d->vMoved = false;
                        else if (estContentPos < maxYExtent())
                            scrollPixel = maxYExtent() - d->vData.move.value();
                    }
                }
                if (d->vMoved) {
                    d->resetTimeline(d->vData);
                    movementStarting();
                    d->timeline.moveBy(d->vData.move, scrollPixel, QEasingCurve(QEasingCurve::OutExpo), 3*d->fixupDuration/4);
                    d->vData.fixingUp = true;
                    d->timeline.callback(QQuickTimeLineCallback(&d->vData.move, QQuickFlickablePrivate::fixupY_callback, d));
                }
                event->accept();
            }
            if (xflick() && xDelta != 0) {
                d->moveReason = QQuickFlickablePrivate::Mouse; // ItemViews will set fixupMode to Immediate in fixup() without this.
                d->hMoved = true;
                qreal scrollPixel = (-xDelta / 120.0 * wheelScroll);
                if (d->boundsBehavior == QQuickFlickable::StopAtBounds) {
                    const qreal estContentPos = scrollPixel + d->hData.move.value();
                    if (scrollPixel > 0) { // Forward direction (away from user)
                        if (d->hData.move.value() >= minXExtent())
                            d->hMoved = false;
                        else if (estContentPos > minXExtent())
                            scrollPixel = minXExtent() - d->hData.move.value();
                    } else { // Backward direction (towards user)
                        if (d->hData.move.value() <= maxXExtent())
                            d->hMoved = false;
                        else if (estContentPos < maxXExtent())
                            scrollPixel = maxXExtent() - d->hData.move.value();
                    }
                }
                if (d->hMoved) {
                    d->resetTimeline(d->hData);
                    movementStarting();
                    d->timeline.moveBy(d->hData.move, scrollPixel, QEasingCurve(QEasingCurve::OutExpo), 3*d->fixupDuration/4);
                    d->hData.fixingUp = true;
                    d->timeline.callback(QQuickTimeLineCallback(&d->hData.move, QQuickFlickablePrivate::fixupX_callback, d));
                }
                event->accept();
            }
        } else {
            // wheelDeceleration is set to some reasonable value: the user or the platform wants to have
            // the classic Qt Quick mouse wheel acceleration behavior.
            // For a single "clicky" wheel event (angleDelta +/- 120),
            // we want flick() to end up moving a distance proportional to QStyleHints::wheelScrollLines().
            // The decel algo from there is
            // qreal dist = v2 / (accel * 2.0);
            // i.e. initialWheelFlickDistance = (120 / dt)^2 / (deceleration * 2)
            // now solve for dt:
            // dt = 120 / sqrt(deceleration * 2 * initialWheelFlickDistance)
            if (!isMoving())
                elapsed = 120 / qSqrt(d->wheelDeceleration * 2 * d->initialWheelFlickDistance);
            if (yflick() && yDelta != 0) {
                qreal instVelocity = yDelta / elapsed;
                // if the direction has changed, start over with filtering, to allow instant movement in the opposite direction
                if ((instVelocity < 0 && d->vData.velocity > 0) || (instVelocity > 0 && d->vData.velocity < 0))
                    d->vData.velocityBuffer.clear();
                d->vData.addVelocitySample(instVelocity, d->maxVelocity);
                d->vData.updateVelocity();
                if ((yDelta > 0 && contentY() > -minYExtent()) || (yDelta < 0 && contentY() < -maxYExtent())) {
                    const bool newFlick = d->flickY(event->type(), d->vData.velocity);
                    if (newFlick && (d->vData.atBeginning != (yDelta > 0) || d->vData.atEnd != (yDelta < 0))) {
                        d->flickingStarted(false, true);
                        d->vMoved = true;
                        movementStarting();
                    }
                    event->accept();
                }
            }
            if (xflick() && xDelta != 0) {
                qreal instVelocity = xDelta / elapsed;
                // if the direction has changed, start over with filtering, to allow instant movement in the opposite direction
                if ((instVelocity < 0 && d->hData.velocity > 0) || (instVelocity > 0 && d->hData.velocity < 0))
                    d->hData.velocityBuffer.clear();
                d->hData.addVelocitySample(instVelocity, d->maxVelocity);
                d->hData.updateVelocity();
                if ((xDelta > 0 && contentX() > -minXExtent()) || (xDelta < 0 && contentX() < -maxXExtent())) {
                    const bool newFlick = d->flickX(event->type(), d->hData.velocity);
                    if (newFlick && (d->hData.atBeginning != (xDelta > 0) || d->hData.atEnd != (xDelta < 0))) {
                        d->flickingStarted(true, false);
                        d->hMoved = true;
                        movementStarting();
                    }
                    event->accept();
                }
            }
        }
    } else {
        // use pixelDelta (probably from a trackpad): this is where we want to be on most platforms eventually
        int xDelta = event->pixelDelta().x();
        int yDelta = event->pixelDelta().y();

        QVector2D velocity(xDelta / elapsed, yDelta / elapsed);
        d->accumulatedWheelPixelDelta += QVector2D(event->pixelDelta());
        // Try to drag if 1) we already are dragging or flicking, or
        // 2) the flickable is free to flick both directions, or
        // 3) the movement so far has been mostly horizontal AND it's free to flick horizontally, or
        // 4) the movement so far has been mostly vertical AND it's free to flick vertically.
        // Otherwise, wait until the next event.  Wheel events with pixel deltas tend to come frequently.
        if (isMoving() || isFlicking() || (yflick() && xflick())
                || (xflick() && qAbs(d->accumulatedWheelPixelDelta.x()) > qAbs(d->accumulatedWheelPixelDelta.y() * 2))
                || (yflick() && qAbs(d->accumulatedWheelPixelDelta.y()) > qAbs(d->accumulatedWheelPixelDelta.x() * 2))) {
            d->drag(currentTimestamp, event->type(), event->position(), d->accumulatedWheelPixelDelta,
                    true, !d->scrollingPhase, true, velocity);
            event->accept();
        } else {
            qCDebug(lcWheel) << "not dragging: accumulated deltas" << d->accumulatedWheelPixelDelta <<
                                "moving?" << isMoving() << "can flick horizontally?" << xflick() << "vertically?" << yflick();
        }
    }
    d->lastPosTime = currentTimestamp;

    if (!event->isAccepted())
        QQuickItem::wheelEvent(event);
}
#endif

bool QQuickFlickablePrivate::isInnermostPressDelay(QQuickItem *i) const
{
    Q_Q(const QQuickFlickable);
    QQuickItem *item = i;
    while (item) {
        QQuickFlickable *flick = qobject_cast<QQuickFlickable*>(item);
        if (flick && flick->pressDelay() > 0 && flick->isInteractive()) {
            // Found the innermost flickable with press delay - is it me?
            return (flick == q);
        }
        item = item->parentItem();
    }
    return false;
}

void QQuickFlickablePrivate::captureDelayedPress(QQuickItem *item, QPointerEvent *event)
{
    Q_Q(QQuickFlickable);
    if (!q->window() || pressDelay <= 0)
        return;

    // Only the innermost flickable should handle the delayed press; this allows
    // flickables up the parent chain to all see the events in their filter functions
    if (!isInnermostPressDelay(item))
        return;

    delayedPressEvent = QQuickDeliveryAgentPrivate::clonePointerEvent(event);
    delayedPressEvent->setAccepted(false);
    delayedPressTimer.start(pressDelay, q);
    qCDebug(lcReplay) << "begin press delay" << pressDelay << "ms with" << delayedPressEvent;
}

void QQuickFlickablePrivate::clearDelayedPress()
{
    if (delayedPressEvent) {
        delayedPressTimer.stop();
        qCDebug(lcReplay) << "clear delayed press" << delayedPressEvent;
        delete delayedPressEvent;
        delayedPressEvent = nullptr;
    }
}

void QQuickFlickablePrivate::replayDelayedPress()
{
    Q_Q(QQuickFlickable);
    if (delayedPressEvent) {
        // Losing the grab will clear the delayed press event; take control of it here
        QScopedPointer<QPointerEvent> event(delayedPressEvent);
        delayedPressEvent = nullptr;
        delayedPressTimer.stop();

        // If we have the grab, release before delivering the event
        if (QQuickWindow *window = q->window()) {
            auto da = deliveryAgentPrivate();
            da->allowChildEventFiltering = false; // don't allow re-filtering during replay
            replayingPressEvent = true;
            auto &firstPoint = event->point(0);
            // At first glance, it's weird for delayedPressEvent to already have a grabber;
            // but on press, filterMouseEvent() took the exclusive grab, and that's stored
            // in the device-specific EventPointData instance in QPointingDevicePrivate::activePoints,
            // not in the event itself.  If this Flickable is still the grabber of that point on that device,
            // that's the reason; but now it doesn't need that grab anymore.
            if (event->exclusiveGrabber(firstPoint) == q)
                event->setExclusiveGrabber(firstPoint, nullptr);

            qCDebug(lcReplay) << "replaying" << event.data();
            // Put scenePosition into position, for the sake of QQuickWindowPrivate::translateTouchEvent()
            // TODO remove this if we remove QQuickWindowPrivate::translateTouchEvent()
            QMutableEventPoint::setPosition(firstPoint, firstPoint.scenePosition());
            // Send it through like a fresh press event, and let QQuickWindow
            // (more specifically, QQuickWindowPrivate::deliverPressOrReleaseEvent)
            // find the item or handler that should receive it, as usual.
            QCoreApplication::sendEvent(window, event.data());
            qCDebug(lcReplay) << "replay done";

            // We're done with replay, go back to normal delivery behavior
            replayingPressEvent = false;
            da->allowChildEventFiltering = true;
        }
    }
}

//XXX pixelAligned ignores the global position of the Flickable, i.e. assumes Flickable itself is pixel aligned.

/*!
    \internal

    This function is called from the timeline,
    when advancement in the timeline is modifying the hData.move value.
    The \a x argument is the newly updated value in hData.move.
    The purpose of the function is to update the x position of the contentItem.
*/
void QQuickFlickablePrivate::setViewportX(qreal x)
{
    Q_Q(QQuickFlickable);
    qreal effectiveX = pixelAligned ? -Round(-x) : x;

    const qreal maxX = q->maxXExtent();
    const qreal minX = q->minXExtent();

    if (boundsMovement == int(QQuickFlickable::StopAtBounds))
        effectiveX = qBound(maxX, effectiveX, minX);

    contentItem->setX(effectiveX);
    if (contentItem->x() != effectiveX)
        return; // reentered

    qreal overshoot = 0.0;
    if (x <= maxX)
        overshoot = maxX - x;
    else if (x >= minX)
        overshoot = minX - x;

    if (overshoot != hData.overshoot) {
        hData.overshoot = overshoot;
        emit q->horizontalOvershootChanged();
    }
}

/*!
    \internal

    This function is called from the timeline,
    when advancement in the timeline is modifying the vData.move value.
    The \a y argument is the newly updated value in vData.move.
    The purpose of the function is to update the y position of the contentItem.
*/
void QQuickFlickablePrivate::setViewportY(qreal y)
{
    Q_Q(QQuickFlickable);
    qreal effectiveY = pixelAligned ? -Round(-y) : y;

    const qreal maxY = q->maxYExtent();
    const qreal minY = q->minYExtent();

    if (boundsMovement == int(QQuickFlickable::StopAtBounds))
        effectiveY = qBound(maxY, effectiveY, minY);

    contentItem->setY(effectiveY);
    if (contentItem->y() != effectiveY)
        return; // reentered

    qreal overshoot = 0.0;
    if (y <= maxY)
        overshoot = maxY - y;
    else if (y >= minY)
        overshoot = minY - y;

    if (overshoot != vData.overshoot) {
        vData.overshoot = overshoot;
        emit q->verticalOvershootChanged();
    }
}

void QQuickFlickable::timerEvent(QTimerEvent *event)
{
    Q_D(QQuickFlickable);
    if (event->timerId() == d->delayedPressTimer.timerId()) {
        d->delayedPressTimer.stop();
        if (d->delayedPressEvent) {
            d->replayDelayedPress();
        }
    }
}

qreal QQuickFlickable::minYExtent() const
{
    Q_D(const QQuickFlickable);
    return d->vData.startMargin;
}

qreal QQuickFlickable::minXExtent() const
{
    Q_D(const QQuickFlickable);
    return d->hData.startMargin;
}

/* returns -ve */
qreal QQuickFlickable::maxXExtent() const
{
    Q_D(const QQuickFlickable);
    return qMin<qreal>(minXExtent(), width() - vWidth() - d->hData.endMargin);
}
/* returns -ve */
qreal QQuickFlickable::maxYExtent() const
{
    Q_D(const QQuickFlickable);
    return qMin<qreal>(minYExtent(), height() - vHeight() - d->vData.endMargin);
}

void QQuickFlickable::componentComplete()
{
    Q_D(QQuickFlickable);
    QQuickItem::componentComplete();
    if (!d->hData.explicitValue && d->hData.startMargin != 0.)
        setContentX(-minXExtent());
    if (!d->vData.explicitValue && d->vData.startMargin != 0.)
        setContentY(-minYExtent());
    if (lcWheel().isDebugEnabled() || lcVel().isDebugEnabled()) {
        d->timeline.setObjectName(QLatin1String("timeline for Flickable ") + objectName());
        d->velocityTimeline.setObjectName(QLatin1String("velocity timeline for Flickable ") + objectName());
    }
}

void QQuickFlickable::viewportMoved(Qt::Orientations orient)
{
    Q_D(QQuickFlickable);
    if (orient & Qt::Vertical)
        d->viewportAxisMoved(d->vData, minYExtent(), maxYExtent(), d->fixupY_callback);
    if (orient & Qt::Horizontal)
        d->viewportAxisMoved(d->hData, minXExtent(), maxXExtent(), d->fixupX_callback);
    d->updateBeginningEnd();
}

void QQuickFlickablePrivate::viewportAxisMoved(AxisData &data, qreal minExtent, qreal maxExtent,
                                           QQuickTimeLineCallback::Callback fixupCallback)
{
    if (!scrollingPhase && (pressed || calcVelocity)) {
        int elapsed = data.velocityTime.restart();
        if (elapsed > 0) {
            qreal velocity = (data.lastPos - data.move.value()) * 1000 / elapsed;
            if (qAbs(velocity) > 0) {
                velocityTimeline.reset(data.smoothVelocity);
                velocityTimeline.set(data.smoothVelocity, velocity);
                qCDebug(lcVel) << "touchpad scroll phase: velocity" << velocity;
            }
        }
    } else {
        if (timeline.time() > data.vTime) {
            velocityTimeline.reset(data.smoothVelocity);
            int dt = timeline.time() - data.vTime;
            if (dt > 2) {
                qreal velocity = (data.lastPos - data.move.value()) * 1000 / dt;
                if (!qFuzzyCompare(data.smoothVelocity.value(), velocity))
                    qCDebug(lcVel) << "velocity" << data.smoothVelocity.value() << "->" << velocity
                                   << "computed as (" << data.lastPos << "-" << data.move.value() << ") * 1000 / ("
                                   << timeline.time() << "-" << data.vTime << ")";
                data.smoothVelocity.setValue(velocity);
            }
        }
    }

    if (!data.inOvershoot && !data.fixingUp && data.flicking
            && (data.move.value() > minExtent || data.move.value() < maxExtent)
            && qAbs(data.smoothVelocity.value()) > 10) {
        // Increase deceleration if we've passed a bound
        qreal overBound = data.move.value() > minExtent
                ? data.move.value() - minExtent
                : maxExtent - data.move.value();
        data.inOvershoot = true;
        qreal maxDistance = overShootDistance(qAbs(data.smoothVelocity.value())) - overBound;
        resetTimeline(data);
        if (maxDistance > 0)
            timeline.accel(data.move, -data.smoothVelocity.value(), deceleration*QML_FLICK_OVERSHOOTFRICTION, maxDistance);
        timeline.callback(QQuickTimeLineCallback(&data.move, fixupCallback, this));
    }

    data.lastPos = data.move.value();
    data.vTime = timeline.time();
}

void QQuickFlickable::geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    Q_D(QQuickFlickable);
    QQuickItem::geometryChange(newGeometry, oldGeometry);

    bool changed = false;
    if (newGeometry.width() != oldGeometry.width()) {
        changed = true; // we must update visualArea.widthRatio
        if (d->hData.viewSize < 0)
            d->contentItem->setWidth(width() - d->hData.startMargin - d->hData.endMargin);
        // Make sure that we're entirely in view.
        if (!d->pressed && !d->hData.moving && !d->vData.moving) {
            d->fixupMode = QQuickFlickablePrivate::Immediate;
            d->fixupX();
        }
    }
    if (newGeometry.height() != oldGeometry.height()) {
        changed = true; // we must update visualArea.heightRatio
        if (d->vData.viewSize < 0)
            d->contentItem->setHeight(height() - d->vData.startMargin - d->vData.endMargin);
        // Make sure that we're entirely in view.
        if (!d->pressed && !d->hData.moving && !d->vData.moving) {
            d->fixupMode = QQuickFlickablePrivate::Immediate;
            d->fixupY();
        }
    }

    if (changed)
        d->updateBeginningEnd();
}

/*!
    \qmlmethod QtQuick::Flickable::flick(qreal xVelocity, qreal yVelocity)

    Flicks the content with \a xVelocity horizontally and \a yVelocity vertically in pixels/sec.

    Calling this method will update the corresponding moving and flicking properties and signals,
    just like a real touchscreen flick.
*/

void QQuickFlickable::flick(qreal xVelocity, qreal yVelocity)
{
    Q_D(QQuickFlickable);
    d->hData.reset();
    d->vData.reset();
    d->hData.velocity = xVelocity;
    d->vData.velocity = yVelocity;
    d->hData.vTime = d->vData.vTime = d->timeline.time();

    const bool flickedX = xflick() && !qFuzzyIsNull(xVelocity) && d->flickX(QEvent::TouchUpdate, xVelocity);
    const bool flickedY = yflick() && !qFuzzyIsNull(yVelocity) && d->flickY(QEvent::TouchUpdate, yVelocity);

    if (flickedX)
        d->hMoved = true;
    if (flickedY)
        d->vMoved = true;
    movementStarting();
    d->flickingStarted(flickedX, flickedY);
}

void QQuickFlickablePrivate::flickingStarted(bool flickingH, bool flickingV)
{
    Q_Q(QQuickFlickable);
    if (!flickingH && !flickingV)
        return;

    bool wasFlicking = hData.flicking || vData.flicking;
    if (flickingH && !hData.flicking) {
        hData.flicking = true;
        emit q->flickingHorizontallyChanged();
    }
    if (flickingV && !vData.flicking) {
        vData.flicking = true;
        emit q->flickingVerticallyChanged();
    }
    if (!wasFlicking && (hData.flicking || vData.flicking)) {
        emit q->flickingChanged();
        emit q->flickStarted();
    }
}

/*!
    \qmlmethod QtQuick::Flickable::cancelFlick()

    Cancels the current flick animation.
*/

void QQuickFlickable::cancelFlick()
{
    Q_D(QQuickFlickable);
    d->resetTimeline(d->hData);
    d->resetTimeline(d->vData);
    movementEnding();
}

void QQuickFlickablePrivate::data_append(QQmlListProperty<QObject> *prop, QObject *o)
{
    if (!prop || !prop->data)
        return;

    if (QQuickItem *i = qmlobject_cast<QQuickItem *>(o)) {
        i->setParentItem(static_cast<QQuickFlickablePrivate*>(prop->data)->contentItem);
    } else if (QQuickPointerHandler *pointerHandler = qmlobject_cast<QQuickPointerHandler *>(o)) {
        static_cast<QQuickFlickablePrivate*>(prop->data)->addPointerHandler(pointerHandler);
    } else {
        o->setParent(prop->object); // XXX todo - do we want this?
    }
}

qsizetype QQuickFlickablePrivate::data_count(QQmlListProperty<QObject> *)
{
    // XXX todo
    return 0;
}

QObject *QQuickFlickablePrivate::data_at(QQmlListProperty<QObject> *, qsizetype)
{
    // XXX todo
    return nullptr;
}

void QQuickFlickablePrivate::data_clear(QQmlListProperty<QObject> *)
{
    // XXX todo
}

QQmlListProperty<QObject> QQuickFlickable::flickableData()
{
    Q_D(QQuickFlickable);
    return QQmlListProperty<QObject>(this, (void *)d, QQuickFlickablePrivate::data_append,
                                             QQuickFlickablePrivate::data_count,
                                             QQuickFlickablePrivate::data_at,
                                             QQuickFlickablePrivate::data_clear);
}

QQmlListProperty<QQuickItem> QQuickFlickable::flickableChildren()
{
    Q_D(QQuickFlickable);
    return QQuickItemPrivate::get(d->contentItem)->children();
}

/*!
    \qmlproperty enumeration QtQuick::Flickable::boundsBehavior
    This property holds whether the surface may be dragged
    beyond the Flickable's boundaries, or overshoot the
    Flickable's boundaries when flicked.

    When the \l boundsMovement is \c Flickable.FollowBoundsBehavior, a value
    other than \c Flickable.StopAtBounds will give a feeling that the edges of
    the view are soft, rather than a hard physical boundary.

    The \c boundsBehavior can be one of:

    \list
    \li Flickable.StopAtBounds - the contents can not be dragged beyond the boundary
    of the flickable, and flicks will not overshoot.
    \li Flickable.DragOverBounds - the contents can be dragged beyond the boundary
    of the Flickable, but flicks will not overshoot.
    \li Flickable.OvershootBounds - the contents can overshoot the boundary when flicked,
    but the content cannot be dragged beyond the boundary of the flickable. (since \c{QtQuick 2.5})
    \li Flickable.DragAndOvershootBounds (default) - the contents can be dragged
    beyond the boundary of the Flickable, and can overshoot the
    boundary when flicked.
    \endlist

    \sa horizontalOvershoot, verticalOvershoot, boundsMovement
*/
QQuickFlickable::BoundsBehavior QQuickFlickable::boundsBehavior() const
{
    Q_D(const QQuickFlickable);
    return d->boundsBehavior;
}

void QQuickFlickable::setBoundsBehavior(BoundsBehavior b)
{
    Q_D(QQuickFlickable);
    if (b == d->boundsBehavior)
        return;
    d->boundsBehavior = b;
    emit boundsBehaviorChanged();
}

/*!
    \qmlproperty Transition QtQuick::Flickable::rebound

    This holds the transition to be applied to the content view when
    it snaps back to the bounds of the flickable. The transition is
    triggered when the view is flicked or dragged past the edge of the
    content area, or when returnToBounds() is called.

    \qml
    import QtQuick 2.0

    Flickable {
        width: 150; height: 150
        contentWidth: 300; contentHeight: 300

        rebound: Transition {
            NumberAnimation {
                properties: "x,y"
                duration: 1000
                easing.type: Easing.OutBounce
            }
        }

        Rectangle {
            width: 300; height: 300
            gradient: Gradient {
                GradientStop { position: 0.0; color: "lightsteelblue" }
                GradientStop { position: 1.0; color: "blue" }
            }
        }
    }
    \endqml

    When the above view is flicked beyond its bounds, it will return to its
    bounds using the transition specified:

    \image flickable-rebound.gif

    If this property is not set, a default animation is applied.
  */
QQuickTransition *QQuickFlickable::rebound() const
{
    Q_D(const QQuickFlickable);
    return d->rebound;
}

void QQuickFlickable::setRebound(QQuickTransition *transition)
{
    Q_D(QQuickFlickable);
    if (transition) {
        if (!d->hData.transitionToBounds)
            d->hData.transitionToBounds = new QQuickFlickableReboundTransition(this, QLatin1String("x"));
        if (!d->vData.transitionToBounds)
            d->vData.transitionToBounds = new QQuickFlickableReboundTransition(this, QLatin1String("y"));
    }
    if (d->rebound != transition) {
        d->rebound = transition;
        emit reboundChanged();
    }
}

/*!
    \qmlproperty real QtQuick::Flickable::contentWidth
    \qmlproperty real QtQuick::Flickable::contentHeight

    The dimensions of the content (the surface controlled by Flickable).
    This should typically be set to the combined size of the items placed in the
    Flickable.

    The following snippet shows how these properties are used to display
    an image that is larger than the Flickable item itself:

    \snippet qml/flickable.qml document

    In some cases, the content dimensions can be automatically set
    based on the \l {Item::childrenRect.width}{childrenRect.width}
    and \l {Item::childrenRect.height}{childrenRect.height} properties
    of the \l contentItem. For example, the previous snippet could be rewritten with:

    \code
    contentWidth: contentItem.childrenRect.width; contentHeight: contentItem.childrenRect.height
    \endcode

    Though this assumes that the origin of the childrenRect is 0,0.
*/
qreal QQuickFlickable::contentWidth() const
{
    Q_D(const QQuickFlickable);
    return d->hData.viewSize;
}

void QQuickFlickable::setContentWidth(qreal w)
{
    Q_D(QQuickFlickable);
    if (d->hData.viewSize == w)
        return;
    d->hData.viewSize = w;
    if (w < 0)
        d->contentItem->setWidth(width() - d->hData.startMargin - d->hData.endMargin);
    else
        d->contentItem->setWidth(w);
    d->hData.markExtentsDirty();
    // Make sure that we're entirely in view.
    if (!d->pressed && !d->hData.moving && !d->vData.moving) {
        d->fixupMode = QQuickFlickablePrivate::Immediate;
        d->fixupX();
    } else if (!d->pressed && d->hData.fixingUp) {
        d->fixupMode = QQuickFlickablePrivate::ExtentChanged;
        d->fixupX();
    }
    emit contentWidthChanged();
    d->updateBeginningEnd();
}

qreal QQuickFlickable::contentHeight() const
{
    Q_D(const QQuickFlickable);
    return d->vData.viewSize;
}

void QQuickFlickable::setContentHeight(qreal h)
{
    Q_D(QQuickFlickable);
    if (d->vData.viewSize == h)
        return;
    d->vData.viewSize = h;
    if (h < 0)
        d->contentItem->setHeight(height() - d->vData.startMargin - d->vData.endMargin);
    else
        d->contentItem->setHeight(h);
    d->vData.markExtentsDirty();
    // Make sure that we're entirely in view.
    if (!d->pressed && !d->hData.moving && !d->vData.moving) {
        d->fixupMode = QQuickFlickablePrivate::Immediate;
        d->fixupY();
    } else if (!d->pressed && d->vData.fixingUp) {
        d->fixupMode = QQuickFlickablePrivate::ExtentChanged;
        d->fixupY();
    }
    emit contentHeightChanged();
    d->updateBeginningEnd();
}

/*!
    \qmlproperty real QtQuick::Flickable::topMargin
    \qmlproperty real QtQuick::Flickable::leftMargin
    \qmlproperty real QtQuick::Flickable::bottomMargin
    \qmlproperty real QtQuick::Flickable::rightMargin

    These properties hold the margins around the content.  This space is reserved
    in addition to the contentWidth and contentHeight.
*/


qreal QQuickFlickable::topMargin() const
{
    Q_D(const QQuickFlickable);
    return d->vData.startMargin;
}

void QQuickFlickable::setTopMargin(qreal m)
{
    Q_D(QQuickFlickable);
    if (d->vData.startMargin == m)
        return;
    d->vData.startMargin = m;
    d->vData.markExtentsDirty();
    if (!d->pressed && !d->hData.moving && !d->vData.moving) {
        d->fixupMode = QQuickFlickablePrivate::Immediate;
        d->fixupY();
    }
    emit topMarginChanged();
    d->updateBeginningEnd();
}

qreal QQuickFlickable::bottomMargin() const
{
    Q_D(const QQuickFlickable);
    return d->vData.endMargin;
}

void QQuickFlickable::setBottomMargin(qreal m)
{
    Q_D(QQuickFlickable);
    if (d->vData.endMargin == m)
        return;
    d->vData.endMargin = m;
    d->vData.markExtentsDirty();
    if (!d->pressed && !d->hData.moving && !d->vData.moving) {
        d->fixupMode = QQuickFlickablePrivate::Immediate;
        d->fixupY();
    }
    emit bottomMarginChanged();
    d->updateBeginningEnd();
}

qreal QQuickFlickable::leftMargin() const
{
    Q_D(const QQuickFlickable);
    return d->hData.startMargin;
}

void QQuickFlickable::setLeftMargin(qreal m)
{
    Q_D(QQuickFlickable);
    if (d->hData.startMargin == m)
        return;
    d->hData.startMargin = m;
    d->hData.markExtentsDirty();
    if (!d->pressed && !d->hData.moving && !d->vData.moving) {
        d->fixupMode = QQuickFlickablePrivate::Immediate;
        d->fixupX();
    }
    emit leftMarginChanged();
    d->updateBeginningEnd();
}

qreal QQuickFlickable::rightMargin() const
{
    Q_D(const QQuickFlickable);
    return d->hData.endMargin;
}

void QQuickFlickable::setRightMargin(qreal m)
{
    Q_D(QQuickFlickable);
    if (d->hData.endMargin == m)
        return;
    d->hData.endMargin = m;
    d->hData.markExtentsDirty();
    if (!d->pressed && !d->hData.moving && !d->vData.moving) {
        d->fixupMode = QQuickFlickablePrivate::Immediate;
        d->fixupX();
    }
    emit rightMarginChanged();
    d->updateBeginningEnd();
}

/*!
    \qmlproperty real QtQuick::Flickable::originX
    \qmlproperty real QtQuick::Flickable::originY

    These properties hold the origin of the content. This value always refers
    to the top-left position of the content regardless of layout direction.

    This is usually (0,0), however ListView and GridView may have an arbitrary
    origin due to delegate size variation, or item insertion/removal outside
    the visible region.

    \sa contentX, contentY
*/

qreal QQuickFlickable::originY() const
{
    Q_D(const QQuickFlickable);
    return -minYExtent() + d->vData.startMargin;
}

qreal QQuickFlickable::originX() const
{
    Q_D(const QQuickFlickable);
    return -minXExtent() + d->hData.startMargin;
}


/*!
    \qmlmethod QtQuick::Flickable::resizeContent(real width, real height, QPointF center)

    Resizes the content to \a width x \a height about \a center.

    This does not scale the contents of the Flickable - it only resizes the \l contentWidth
    and \l contentHeight.

    Resizing the content may result in the content being positioned outside
    the bounds of the Flickable.  Calling \l returnToBounds() will
    move the content back within legal bounds.
*/
void QQuickFlickable::resizeContent(qreal w, qreal h, QPointF center)
{
    Q_D(QQuickFlickable);
    const qreal oldHSize = d->hData.viewSize;
    const qreal oldVSize = d->vData.viewSize;
    const bool needToUpdateWidth = w != oldHSize;
    const bool needToUpdateHeight = h != oldVSize;
    d->hData.viewSize = w;
    d->vData.viewSize = h;
    d->contentItem->setSize(QSizeF(w, h));
    if (needToUpdateWidth)
        emit contentWidthChanged();
    if (needToUpdateHeight)
        emit contentHeightChanged();

    if (center.x() != 0) {
        qreal pos = center.x() * w / oldHSize;
        setContentX(contentX() + pos - center.x());
    }
    if (center.y() != 0) {
        qreal pos = center.y() * h / oldVSize;
        setContentY(contentY() + pos - center.y());
    }
    d->updateBeginningEnd();
}

/*!
    \qmlmethod QtQuick::Flickable::returnToBounds()

    Ensures the content is within legal bounds.

    This may be called to ensure that the content is within legal bounds
    after manually positioning the content.
*/
void QQuickFlickable::returnToBounds()
{
    Q_D(QQuickFlickable);
    d->fixupX();
    d->fixupY();
}

qreal QQuickFlickable::vWidth() const
{
    Q_D(const QQuickFlickable);
    if (d->hData.viewSize < 0)
        return width();
    else
        return d->hData.viewSize;
}

qreal QQuickFlickable::vHeight() const
{
    Q_D(const QQuickFlickable);
    if (d->vData.viewSize < 0)
        return height();
    else
        return d->vData.viewSize;
}

/*!
    \internal

    The setFlickableDirection function can be used to set constraints on which axis the contentItem can be flicked along.

    \return true if the flickable is allowed to flick in the horizontal direction, otherwise returns false
*/
bool QQuickFlickable::xflick() const
{
    Q_D(const QQuickFlickable);
    const int contentWidthWithMargins = d->contentItem->width() + d->hData.startMargin + d->hData.endMargin;
    if ((d->flickableDirection & QQuickFlickable::AutoFlickIfNeeded) && (contentWidthWithMargins > width()))
        return true;
    if (d->flickableDirection == QQuickFlickable::AutoFlickDirection)
        return std::floor(qAbs(contentWidthWithMargins - width()));
    return d->flickableDirection & QQuickFlickable::HorizontalFlick;
}

/*!
    \internal

    The setFlickableDirection function can be used to set constraints on which axis the contentItem can be flicked along.

    \return true if the flickable is allowed to flick in the vertical direction, otherwise returns false.
*/
bool QQuickFlickable::yflick() const
{
    Q_D(const QQuickFlickable);
    const int contentHeightWithMargins = d->contentItem->height() + d->vData.startMargin + d->vData.endMargin;
    if ((d->flickableDirection & QQuickFlickable::AutoFlickIfNeeded) && (contentHeightWithMargins > height()))
        return true;
    if (d->flickableDirection == QQuickFlickable::AutoFlickDirection)
        return std::floor(qAbs(contentHeightWithMargins - height()));
    return d->flickableDirection & QQuickFlickable::VerticalFlick;
}

void QQuickFlickable::mouseUngrabEvent()
{
    Q_D(QQuickFlickable);
    // if our mouse grab has been removed (probably by another Flickable),
    // fix our state
    if (!d->replayingPressEvent)
        d->cancelInteraction();
}

void QQuickFlickablePrivate::cancelInteraction()
{
    Q_Q(QQuickFlickable);
    if (pressed) {
        clearDelayedPress();
        pressed = false;
        draggingEnding();
        stealMouse = false;
        q->setKeepMouseGrab(false);
        fixupX();
        fixupY();
        if (!isViewMoving())
            q->movementEnding();
    }
}

void QQuickFlickablePrivate::addPointerHandler(QQuickPointerHandler *h)
{
    Q_Q(const QQuickFlickable);
    qCDebug(lcHandlerParent) << "reparenting handler" << h << "to contentItem of" << q;
    h->setParent(contentItem);
    QQuickItemPrivate::get(contentItem)->addPointerHandler(h);
}

/*! \internal
    QQuickFlickable::filterPointerEvent filters pointer events intercepted on the way
    to the child \a receiver, and potentially steals the exclusive grab.

    This is how flickable takes over the handling of events from child items.

    Returns true if the event will be stolen and should <em>not</em> be delivered to the \a receiver.
*/
bool QQuickFlickable::filterPointerEvent(QQuickItem *receiver, QPointerEvent *event)
{
    const bool isTouch = QQuickDeliveryAgentPrivate::isTouchEvent(event);
    if (!(QQuickDeliveryAgentPrivate::isMouseEvent(event) || isTouch ||
          QQuickDeliveryAgentPrivate::isTabletEvent(event)))
        return false; // don't filter hover events or wheel events, for example
    Q_ASSERT_X(receiver != this, "", "Flickable received a filter event for itself");
    Q_D(QQuickFlickable);
    // If a touch event contains a new press point, don't steal right away: watch the movements for a while
    if (isTouch && static_cast<QTouchEvent *>(event)->touchPointStates().testFlag(QEventPoint::State::Pressed))
        d->stealMouse = false;
    // If multiple touchpoints are within bounds, don't grab: it's probably meant for multi-touch interaction in some child
    if (event->pointCount() > 1) {
        qCDebug(lcFilter) << objectName() << "ignoring multi-touch" << event << "for" << receiver;
        d->stealMouse = false;
    } else {
        qCDebug(lcFilter) << objectName() << "filtering" << event << "for" << receiver;
    }

    const auto &firstPoint = event->points().first();

    if (event->pointCount() == 1 && event->exclusiveGrabber(firstPoint) == this) {
        // We have an exclusive grab (since we're e.g dragging), but at the same time, we have
        // a child with a passive grab (which is why this filter is being called). And because
        // of that, we end up getting the same pointer events twice; First in our own event
        // handlers (because of the grab), then once more in here, since we filter the child.
        // To avoid processing the event twice (e.g avoid calling handleReleaseEvent once more
        // from below), we mark the event as filtered, and simply return.
        event->setAccepted(true);
        return true;
    }

    QPointF localPos = mapFromScene(firstPoint.scenePosition());
    bool receiverDisabled = receiver && !receiver->isEnabled();
    bool stealThisEvent = d->stealMouse;
    bool receiverKeepsGrab = receiver && (receiver->keepMouseGrab() || receiver->keepTouchGrab());
    bool receiverRelinquishGrab = false;

    // Special case for MouseArea, try to guess what it does with the event
    if (auto *mouseArea = qmlobject_cast<QQuickMouseArea *>(receiver)) {
        bool preventStealing = mouseArea->preventStealing();
#if QT_CONFIG(quick_draganddrop)
        if (mouseArea->drag() && mouseArea->drag()->target())
            preventStealing = true;
#endif
        if (!preventStealing && receiverKeepsGrab) {
            receiverRelinquishGrab = !receiverDisabled
                    || (QQuickDeliveryAgentPrivate::isMouseEvent(event)
                        && firstPoint.state() == QEventPoint::State::Pressed
                        && (receiver->acceptedMouseButtons() & static_cast<QMouseEvent *>(event)->button()));
            if (receiverRelinquishGrab)
                receiverKeepsGrab = false;
        }
    }

    if ((stealThisEvent || contains(localPos)) && (!receiver || !receiverKeepsGrab || receiverDisabled)) {
        QScopedPointer<QPointerEvent> localizedEvent(QQuickDeliveryAgentPrivate::clonePointerEvent(event, localPos));
        localizedEvent->setAccepted(false);
        switch (firstPoint.state()) {
        case QEventPoint::State::Updated:
            d->handleMoveEvent(localizedEvent.data());
            break;
        case QEventPoint::State::Pressed:
            d->handlePressEvent(localizedEvent.data());
            d->captureDelayedPress(receiver, event);
            // never grab the pointing device on press during filtering: do it later, during a move
            d->stealMouse = false;
            stealThisEvent = false;
            break;
        case QEventPoint::State::Released:
            d->handleReleaseEvent(localizedEvent.data());
            stealThisEvent = d->stealMouse;
            break;
        case QEventPoint::State::Stationary:
        case QEventPoint::State::Unknown:
            break;
        }
        if ((receiver && stealThisEvent && !receiverKeepsGrab && receiver != this) || receiverDisabled) {
            d->clearDelayedPress();
            event->setExclusiveGrabber(firstPoint, this);
        } else if (d->delayedPressEvent) {
            event->setExclusiveGrabber(firstPoint, this);
        }

        const bool filtered = !receiverRelinquishGrab && (stealThisEvent || d->delayedPressEvent || receiverDisabled);
        if (filtered) {
            event->setAccepted(true);
        }
        return filtered;
    } else if (d->lastPosTime != -1) {
        d->lastPosTime = -1;
        returnToBounds();
    }
    if (firstPoint.state() == QEventPoint::State::Released || (receiverKeepsGrab && !receiverDisabled)) {
        // mouse released, or another item has claimed the grab
        d->lastPosTime = -1;
        d->clearDelayedPress();
        d->stealMouse = false;
        d->pressed = false;
    }
    return false;
}

/*! \internal
    Despite the name, this function filters all pointer events on their way to any child within.
    Returns true if the event will be stolen and should <em>not</em> be delivered to the \a receiver.
*/
bool QQuickFlickable::childMouseEventFilter(QQuickItem *i, QEvent *e)
{
    Q_D(QQuickFlickable);
    QPointerEvent *pointerEvent = e->isPointerEvent() ? static_cast<QPointerEvent *>(e) : nullptr;

    auto wantsPointerEvent_helper = [this, d, i, pointerEvent]() {
        Q_ASSERT(pointerEvent);
        QQuickDeliveryAgentPrivate::localizePointerEvent(pointerEvent, this);
        const bool wants = d->wantsPointerEvent(pointerEvent);
        // re-localize event back to \a i before returning
        QQuickDeliveryAgentPrivate::localizePointerEvent(pointerEvent, i);
        return wants;
    };

    if (!isVisible() || !isEnabled() || !isInteractive() ||
            (pointerEvent && !wantsPointerEvent_helper())) {
        d->cancelInteraction();
        return QQuickItem::childMouseEventFilter(i, e);
    }

    if (e->type() == QEvent::UngrabMouse) {
        Q_ASSERT(e->isSinglePointEvent());
        auto spe = static_cast<QSinglePointEvent *>(e);
        const QObject *grabber = spe->exclusiveGrabber(spe->points().first());
        qCDebug(lcFilter) << "filtering UngrabMouse" << spe->points().first() << "for" << i << "grabber is" << grabber;
        if (grabber != this)
            mouseUngrabEvent(); // A child has been ungrabbed
    } else if (pointerEvent) {
        return filterPointerEvent(i, pointerEvent);
    }

    return QQuickItem::childMouseEventFilter(i, e);
}

/*!
    \qmlproperty real QtQuick::Flickable::maximumFlickVelocity
    This property holds the maximum velocity that the user can flick the view in pixels/second.

    The default value is platform dependent.
*/
qreal QQuickFlickable::maximumFlickVelocity() const
{
    Q_D(const QQuickFlickable);
    return d->maxVelocity;
}

void QQuickFlickable::setMaximumFlickVelocity(qreal v)
{
    Q_D(QQuickFlickable);
    if (v == d->maxVelocity)
        return;
    d->maxVelocity = v;
    emit maximumFlickVelocityChanged();
}

/*!
    \qmlproperty real QtQuick::Flickable::flickDeceleration
    This property holds the rate at which a flick will decelerate:
    the higher the number, the faster it slows down when the user stops
    flicking via touch. For example 0.0001 is nearly
    "frictionless", and 10000 feels quite "sticky".

    The default value is platform dependent. Values of zero or less are not allowed.
*/
qreal QQuickFlickable::flickDeceleration() const
{
    Q_D(const QQuickFlickable);
    return d->deceleration;
}

void QQuickFlickable::setFlickDeceleration(qreal deceleration)
{
    Q_D(QQuickFlickable);
    if (deceleration == d->deceleration)
        return;
    d->deceleration = qMax(0.001, deceleration);
    emit flickDecelerationChanged();
}

bool QQuickFlickable::isFlicking() const
{
    Q_D(const QQuickFlickable);
    return d->hData.flicking ||  d->vData.flicking;
}

/*!
    \qmlproperty bool QtQuick::Flickable::flicking
    \qmlproperty bool QtQuick::Flickable::flickingHorizontally
    \qmlproperty bool QtQuick::Flickable::flickingVertically

    These properties describe whether the view is currently moving horizontally,
    vertically or in either direction, due to the user flicking the view.
*/
bool QQuickFlickable::isFlickingHorizontally() const
{
    Q_D(const QQuickFlickable);
    return d->hData.flicking;
}

bool QQuickFlickable::isFlickingVertically() const
{
    Q_D(const QQuickFlickable);
    return d->vData.flicking;
}

/*!
    \qmlproperty bool QtQuick::Flickable::dragging
    \qmlproperty bool QtQuick::Flickable::draggingHorizontally
    \qmlproperty bool QtQuick::Flickable::draggingVertically

    These properties describe whether the view is currently moving horizontally,
    vertically or in either direction, due to the user dragging the view.
*/
bool QQuickFlickable::isDragging() const
{
    Q_D(const QQuickFlickable);
    return d->hData.dragging ||  d->vData.dragging;
}

bool QQuickFlickable::isDraggingHorizontally() const
{
    Q_D(const QQuickFlickable);
    return d->hData.dragging;
}

bool QQuickFlickable::isDraggingVertically() const
{
    Q_D(const QQuickFlickable);
    return d->vData.dragging;
}

void QQuickFlickablePrivate::draggingStarting()
{
    Q_Q(QQuickFlickable);
    bool wasDragging = hData.dragging || vData.dragging;
    if (hMoved && !hData.dragging) {
        hData.dragging = true;
        emit q->draggingHorizontallyChanged();
    }
    if (vMoved && !vData.dragging) {
        vData.dragging = true;
        emit q->draggingVerticallyChanged();
    }
    if (!wasDragging && (hData.dragging || vData.dragging)) {
        emit q->draggingChanged();
        emit q->dragStarted();
    }
}

void QQuickFlickablePrivate::draggingEnding()
{
    Q_Q(QQuickFlickable);
    const bool wasDragging = hData.dragging || vData.dragging;
    if (hData.dragging) {
        hData.dragging = false;
        emit q->draggingHorizontallyChanged();
    }
    if (vData.dragging) {
        vData.dragging = false;
        emit q->draggingVerticallyChanged();
    }
    if (wasDragging) {
        if (!hData.dragging && !vData.dragging) {
            emit q->draggingChanged();
            emit q->dragEnded();
        }
        hData.inRebound = false;
        vData.inRebound = false;
    }
}

bool QQuickFlickablePrivate::isViewMoving() const
{
    if (timeline.isActive()
            || (hData.transitionToBounds && hData.transitionToBounds->isActive())
            || (vData.transitionToBounds && vData.transitionToBounds->isActive()) ) {
        return true;
    }
    return false;
}

/*!
    \qmlproperty int QtQuick::Flickable::pressDelay

    This property holds the time to delay (ms) delivering a press to
    children of the Flickable.  This can be useful where reacting
    to a press before a flicking action has undesirable effects.

    If the flickable is dragged/flicked before the delay times out
    the press event will not be delivered.  If the button is released
    within the timeout, both the press and release will be delivered.

    Note that for nested Flickables with pressDelay set, the pressDelay of
    outer Flickables is overridden by the innermost Flickable. If the drag
    exceeds the platform drag threshold, the press event will be delivered
    regardless of this property.

    \sa QStyleHints
*/
int QQuickFlickable::pressDelay() const
{
    Q_D(const QQuickFlickable);
    return d->pressDelay;
}

void QQuickFlickable::setPressDelay(int delay)
{
    Q_D(QQuickFlickable);
    if (d->pressDelay == delay)
        return;
    d->pressDelay = delay;
    emit pressDelayChanged();
}

/*!
    \qmlproperty bool QtQuick::Flickable::moving
    \qmlproperty bool QtQuick::Flickable::movingHorizontally
    \qmlproperty bool QtQuick::Flickable::movingVertically

    These properties describe whether the view is currently moving horizontally,
    vertically or in either direction, due to the user either dragging or
    flicking the view.
*/

bool QQuickFlickable::isMoving() const
{
    Q_D(const QQuickFlickable);
    return d->hData.moving || d->vData.moving;
}

bool QQuickFlickable::isMovingHorizontally() const
{
    Q_D(const QQuickFlickable);
    return d->hData.moving;
}

bool QQuickFlickable::isMovingVertically() const
{
    Q_D(const QQuickFlickable);
    return d->vData.moving;
}

void QQuickFlickable::velocityTimelineCompleted()
{
    Q_D(QQuickFlickable);
    if ( (d->hData.transitionToBounds && d->hData.transitionToBounds->isActive())
         || (d->vData.transitionToBounds && d->vData.transitionToBounds->isActive()) ) {
        return;
    }
    // With subclasses such as GridView, velocityTimeline.completed is emitted repeatedly:
    // for example setting currentIndex results in a visual "flick" which the user
    // didn't initiate directly. We don't want to end movement repeatedly, and in
    // that case movementEnding will happen after the sequence of movements ends.
    if (d->vData.flicking)
        movementEnding();
    d->updateBeginningEnd();
}

void QQuickFlickable::timelineCompleted()
{
    Q_D(QQuickFlickable);
    if ( (d->hData.transitionToBounds && d->hData.transitionToBounds->isActive())
         || (d->vData.transitionToBounds && d->vData.transitionToBounds->isActive()) ) {
        return;
    }
    movementEnding();
    d->updateBeginningEnd();
}

void QQuickFlickable::movementStarting()
{
    Q_D(QQuickFlickable);
    bool wasMoving = d->hData.moving || d->vData.moving;
    if (d->hMoved && !d->hData.moving) {
        d->hData.moving = true;
        emit movingHorizontallyChanged();
    }
    if (d->vMoved && !d->vData.moving) {
        d->vData.moving = true;
        emit movingVerticallyChanged();
    }

    if (!wasMoving && (d->hData.moving || d->vData.moving)) {
        emit movingChanged();
        emit movementStarted();
#if QT_CONFIG(accessibility)
        if (QAccessible::isActive()) {
            QAccessibleEvent ev(this, QAccessible::ScrollingStart);
            QAccessible::updateAccessibility(&ev);
        }
#endif
    }
}

void QQuickFlickable::movementEnding()
{
    movementEnding(true, true);
}

void QQuickFlickable::movementEnding(bool hMovementEnding, bool vMovementEnding)
{
    Q_D(QQuickFlickable);

    // emit flicking signals
    const bool wasFlicking = d->hData.flicking || d->vData.flicking;
    if (hMovementEnding && d->hData.flicking) {
        d->hData.flicking = false;
        emit flickingHorizontallyChanged();
    }
    if (vMovementEnding && d->vData.flicking) {
        d->vData.flicking = false;
        emit flickingVerticallyChanged();
    }
    if (wasFlicking && (!d->hData.flicking || !d->vData.flicking)) {
        emit flickingChanged();
        emit flickEnded();
    } else if (d->hData.flickingWhenDragBegan || d->vData.flickingWhenDragBegan) {
        d->hData.flickingWhenDragBegan = !hMovementEnding;
        d->vData.flickingWhenDragBegan = !vMovementEnding;
        emit flickEnded();
    }

    // emit moving signals
    bool wasMoving = isMoving();
    if (hMovementEnding && d->hData.moving
            && (!d->pressed && !d->stealMouse)) {
        d->hData.moving = false;
        d->hMoved = false;
        emit movingHorizontallyChanged();
    }
    if (vMovementEnding && d->vData.moving
            && (!d->pressed && !d->stealMouse)) {
        d->vData.moving = false;
        d->vMoved = false;
        emit movingVerticallyChanged();
    }
    if (wasMoving && !isMoving()) {
        emit movingChanged();
        emit movementEnded();
#if QT_CONFIG(accessibility)
        if (QAccessible::isActive()) {
            QAccessibleEvent ev(this, QAccessible::ScrollingEnd);
            QAccessible::updateAccessibility(&ev);
        }
#endif
    }

    if (hMovementEnding) {
        d->hData.fixingUp = false;
        d->hData.smoothVelocity.setValue(0);
        d->hData.previousDragDelta = 0.0;
    }
    if (vMovementEnding) {
        d->vData.fixingUp = false;
        d->vData.smoothVelocity.setValue(0);
        d->vData.previousDragDelta = 0.0;
    }
}

void QQuickFlickablePrivate::updateVelocity()
{
    Q_Q(QQuickFlickable);
    emit q->horizontalVelocityChanged();
    emit q->verticalVelocityChanged();
}

/*!
    \qmlproperty real QtQuick::Flickable::horizontalOvershoot
    \since 5.9

    This property holds the horizontal overshoot, that is, the horizontal distance by
    which the contents has been dragged or flicked past the bounds of the flickable.
    The value is negative when the content is dragged or flicked beyond the beginning,
    and positive when beyond the end; \c 0.0 otherwise.

    Whether the values are reported for dragging and/or flicking is determined by
    \l boundsBehavior. The overshoot distance is reported even when \l boundsMovement
    is \c Flickable.StopAtBounds.

    \sa verticalOvershoot, boundsBehavior, boundsMovement
*/
qreal QQuickFlickable::horizontalOvershoot() const
{
    Q_D(const QQuickFlickable);
    return d->hData.overshoot;
}

/*!
    \qmlproperty real QtQuick::Flickable::verticalOvershoot
    \since 5.9

    This property holds the vertical overshoot, that is, the vertical distance by
    which the contents has been dragged or flicked past the bounds of the flickable.
    The value is negative when the content is dragged or flicked beyond the beginning,
    and positive when beyond the end; \c 0.0 otherwise.

    Whether the values are reported for dragging and/or flicking is determined by
    \l boundsBehavior. The overshoot distance is reported even when \l boundsMovement
    is \c Flickable.StopAtBounds.

    \sa horizontalOvershoot, boundsBehavior, boundsMovement
*/
qreal QQuickFlickable::verticalOvershoot() const
{
    Q_D(const QQuickFlickable);
    return d->vData.overshoot;
}

/*!
    \qmlproperty enumeration QtQuick::Flickable::boundsMovement
    \since 5.10

    This property holds whether the flickable will give a feeling that the edges of the
    view are soft, rather than a hard physical boundary.

    The \c boundsMovement can be one of:

    \list
    \li Flickable.StopAtBounds - this allows implementing custom edge effects where the
    contents do not follow drags or flicks beyond the bounds of the flickable. The values
    of \l horizontalOvershoot and \l verticalOvershoot can be utilized to implement custom
    edge effects.
    \li Flickable.FollowBoundsBehavior (default) - whether the contents follow drags or
    flicks beyond the bounds of the flickable is determined by \l boundsBehavior.
    \endlist

    The following example keeps the contents within bounds and instead applies a flip
    effect when flicked over horizontal bounds:
    \code
    Flickable {
        id: flickable
        boundsMovement: Flickable.StopAtBounds
        boundsBehavior: Flickable.DragAndOvershootBounds
        transform: Rotation {
            axis { x: 0; y: 1; z: 0 }
            origin.x: flickable.width / 2
            origin.y: flickable.height / 2
            angle: Math.min(30, Math.max(-30, flickable.horizontalOvershoot))
        }
    }
    \endcode

    The following example keeps the contents within bounds and instead applies an opacity
    effect when dragged over vertical bounds:
    \code
    Flickable {
        boundsMovement: Flickable.StopAtBounds
        boundsBehavior: Flickable.DragOverBounds
        opacity: Math.max(0.5, 1.0 - Math.abs(verticalOvershoot) / height)
    }
    \endcode

    \sa boundsBehavior, verticalOvershoot, horizontalOvershoot
*/
QQuickFlickable::BoundsMovement QQuickFlickable::boundsMovement() const
{
    Q_D(const QQuickFlickable);
    return d->boundsMovement;
}

void QQuickFlickable::setBoundsMovement(BoundsMovement movement)
{
    Q_D(QQuickFlickable);
    if (d->boundsMovement == movement)
        return;

    d->boundsMovement = movement;
    emit boundsMovementChanged();
}

QT_END_NAMESPACE

#include "moc_qquickflickable_p_p.cpp"

#include "moc_qquickflickable_p.cpp"
