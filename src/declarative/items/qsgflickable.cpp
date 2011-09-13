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

#include "qsgflickable_p.h"
#include "qsgflickable_p_p.h"
#include "qsgcanvas.h"
#include "qsgcanvas_p.h"

#include <QtDeclarative/qdeclarativeinfo.h>
#include <QtGui/qevent.h>
#include <QtGui/qguiapplication.h>
#include <QtGui/qstylehints.h>
#include "qplatformdefs.h"

QT_BEGIN_NAMESPACE

// The maximum number of pixels a flick can overshoot
#ifndef QML_FLICK_OVERSHOOT
#define QML_FLICK_OVERSHOOT 200
#endif

// The number of samples to use in calculating the velocity of a flick
#ifndef QML_FLICK_SAMPLEBUFFER
#define QML_FLICK_SAMPLEBUFFER 3
#endif

// The number of samples to discard when calculating the flick velocity.
// Touch panels often produce inaccurate results as the finger is lifted.
#ifndef QML_FLICK_DISCARDSAMPLES
#define QML_FLICK_DISCARDSAMPLES 1
#endif

// The default maximum velocity of a flick.
#ifndef QML_FLICK_DEFAULTMAXVELOCITY
#define QML_FLICK_DEFAULTMAXVELOCITY 2500
#endif

// The default deceleration of a flick.
#ifndef QML_FLICK_DEFAULTDECELERATION
#define QML_FLICK_DEFAULTDECELERATION 1500
#endif

// How much faster to decelerate when overshooting
#ifndef QML_FLICK_OVERSHOOTFRICTION
#define QML_FLICK_OVERSHOOTFRICTION 8
#endif

// FlickThreshold determines how far the "mouse" must have moved
// before we perform a flick.
static const int FlickThreshold = 20;

// RetainGrabVelocity is the maxmimum instantaneous velocity that
// will ensure the Flickable retains the grab on consecutive flicks.
static const int RetainGrabVelocity = 15;

QSGFlickableVisibleArea::QSGFlickableVisibleArea(QSGFlickable *parent)
    : QObject(parent), flickable(parent), m_xPosition(0.), m_widthRatio(0.)
    , m_yPosition(0.), m_heightRatio(0.)
{
}

qreal QSGFlickableVisibleArea::widthRatio() const
{
    return m_widthRatio;
}

qreal QSGFlickableVisibleArea::xPosition() const
{
    return m_xPosition;
}

qreal QSGFlickableVisibleArea::heightRatio() const
{
    return m_heightRatio;
}

qreal QSGFlickableVisibleArea::yPosition() const
{
    return m_yPosition;
}

void QSGFlickableVisibleArea::updateVisible()
{
    QSGFlickablePrivate *p = QSGFlickablePrivate::get(flickable);

    bool changeX = false;
    bool changeY = false;
    bool changeWidth = false;
    bool changeHeight = false;

    // Vertical
    const qreal viewheight = flickable->height();
    const qreal maxyextent = -flickable->maxYExtent() + flickable->minYExtent();
    qreal pagePos = (-p->vData.move.value() + flickable->minYExtent()) / (maxyextent + viewheight);
    qreal pageSize = viewheight / (maxyextent + viewheight);

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
    pagePos = (-p->hData.move.value() + flickable->minXExtent()) / (maxxextent + viewwidth);
    pageSize = viewwidth / (maxxextent + viewwidth);

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


QSGFlickablePrivate::QSGFlickablePrivate()
  : contentItem(new QSGItem)
    , hData(this, &QSGFlickablePrivate::setViewportX)
    , vData(this, &QSGFlickablePrivate::setViewportY)
    , flickingHorizontally(false), flickingVertically(false)
    , hMoved(false), vMoved(false)
    , movingHorizontally(false), movingVertically(false)
    , stealMouse(false), pressed(false), interactive(true), calcVelocity(false)
    , pixelAligned(false)
    , deceleration(QML_FLICK_DEFAULTDECELERATION)
    , maxVelocity(QML_FLICK_DEFAULTMAXVELOCITY), reportedVelocitySmoothing(100)
    , delayedPressEvent(0), delayedPressTarget(0), pressDelay(0), fixupDuration(400)
    , fixupMode(Normal), vTime(0), visibleArea(0)
    , flickableDirection(QSGFlickable::AutoFlickDirection)
    , boundsBehavior(QSGFlickable::DragAndOvershootBounds)
{
}

void QSGFlickablePrivate::init()
{
    Q_Q(QSGFlickable);
    QDeclarative_setParent_noEvent(contentItem, q);
    contentItem->setParentItem(q);
    static int timelineUpdatedIdx = -1;
    static int timelineCompletedIdx = -1;
    static int flickableTickedIdx = -1;
    static int flickableMovementEndingIdx = -1;
    if (timelineUpdatedIdx == -1) {
        timelineUpdatedIdx = QDeclarativeTimeLine::staticMetaObject.indexOfSignal("updated()");
        timelineCompletedIdx = QDeclarativeTimeLine::staticMetaObject.indexOfSignal("completed()");
        flickableTickedIdx = QSGFlickable::staticMetaObject.indexOfSlot("ticked()");
        flickableMovementEndingIdx = QSGFlickable::staticMetaObject.indexOfSlot("movementEnding()");
    }
    QMetaObject::connect(&timeline, timelineUpdatedIdx,
                         q, flickableTickedIdx, Qt::DirectConnection);
    QMetaObject::connect(&timeline, timelineCompletedIdx,
                         q, flickableMovementEndingIdx, Qt::DirectConnection);
    q->setAcceptedMouseButtons(Qt::LeftButton);
    q->setFiltersChildMouseEvents(true);
    QSGItemPrivate *viewportPrivate = QSGItemPrivate::get(contentItem);
    viewportPrivate->addItemChangeListener(this, QSGItemPrivate::Geometry);
    lastPosTime.invalidate();
}

/*
    Returns the amount to overshoot by given a velocity.
    Will be roughly in range 0 - size/4
*/
qreal QSGFlickablePrivate::overShootDistance(qreal size)
{
    if (maxVelocity <= 0)
        return 0.0;

    return qMin(qreal(QML_FLICK_OVERSHOOT), size/3);
}

void QSGFlickablePrivate::AxisData::addVelocitySample(qreal v, qreal maxVelocity)
{
    if (v > maxVelocity)
        v = maxVelocity;
    else if (v < -maxVelocity)
        v = -maxVelocity;
    velocityBuffer.append(v);
    if (velocityBuffer.count() > QML_FLICK_SAMPLEBUFFER)
        velocityBuffer.remove(0);
}

void QSGFlickablePrivate::AxisData::updateVelocity()
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

void QSGFlickablePrivate::itemGeometryChanged(QSGItem *item, const QRectF &newGeom, const QRectF &oldGeom)
{
    Q_Q(QSGFlickable);
    if (item == contentItem) {
        if (newGeom.x() != oldGeom.x())
            emit q->contentXChanged();
        if (newGeom.y() != oldGeom.y())
            emit q->contentYChanged();
    }
}

void QSGFlickablePrivate::flickX(qreal velocity)
{
    Q_Q(QSGFlickable);
    flick(hData, q->minXExtent(), q->maxXExtent(), q->width(), fixupX_callback, velocity);
}

void QSGFlickablePrivate::flickY(qreal velocity)
{
    Q_Q(QSGFlickable);
    flick(vData, q->minYExtent(), q->maxYExtent(), q->height(), fixupY_callback, velocity);
}

void QSGFlickablePrivate::flick(AxisData &data, qreal minExtent, qreal maxExtent, qreal,
                                         QDeclarativeTimeLineCallback::Callback fixupCallback, qreal velocity)
{
    Q_Q(QSGFlickable);
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
    if (maxDistance > 0) {
        qreal v = velocity;
        if (maxVelocity != -1 && maxVelocity < qAbs(v)) {
            if (v < 0)
                v = -maxVelocity;
            else
                v = maxVelocity;
        }
        timeline.reset(data.move);
        if (boundsBehavior == QSGFlickable::DragAndOvershootBounds)
            timeline.accel(data.move, v, deceleration);
        else
            timeline.accel(data.move, v, deceleration, maxDistance);
        timeline.callback(QDeclarativeTimeLineCallback(&data.move, fixupCallback, this));
        if (!flickingHorizontally && q->xflick()) {
            flickingHorizontally = true;
            emit q->flickingChanged();
            emit q->flickingHorizontallyChanged();
            if (!flickingVertically)
                emit q->flickStarted();
        }
        if (!flickingVertically && q->yflick()) {
            flickingVertically = true;
            emit q->flickingChanged();
            emit q->flickingVerticallyChanged();
            if (!flickingHorizontally)
                emit q->flickStarted();
        }
    } else {
        timeline.reset(data.move);
        fixup(data, minExtent, maxExtent);
    }
}

void QSGFlickablePrivate::fixupY_callback(void *data)
{
    ((QSGFlickablePrivate *)data)->fixupY();
}

void QSGFlickablePrivate::fixupX_callback(void *data)
{
    ((QSGFlickablePrivate *)data)->fixupX();
}

void QSGFlickablePrivate::fixupX()
{
    Q_Q(QSGFlickable);
    fixup(hData, q->minXExtent(), q->maxXExtent());
}

void QSGFlickablePrivate::fixupY()
{
    Q_Q(QSGFlickable);
    fixup(vData, q->minYExtent(), q->maxYExtent());
}

void QSGFlickablePrivate::fixup(AxisData &data, qreal minExtent, qreal maxExtent)
{
    if (data.move.value() > minExtent || maxExtent > minExtent) {
        timeline.reset(data.move);
        if (data.move.value() != minExtent) {
            switch (fixupMode) {
            case Immediate:
                timeline.set(data.move, minExtent);
                break;
            case ExtentChanged:
                // The target has changed. Don't start from the beginning; just complete the
                // second half of the animation using the new extent.
                timeline.move(data.move, minExtent, QEasingCurve(QEasingCurve::OutExpo), 3*fixupDuration/4);
                data.fixingUp = true;
                break;
            default: {
                    qreal dist = minExtent - data.move;
                    timeline.move(data.move, minExtent - dist/2, QEasingCurve(QEasingCurve::InQuad), fixupDuration/4);
                    timeline.move(data.move, minExtent, QEasingCurve(QEasingCurve::OutExpo), 3*fixupDuration/4);
                    data.fixingUp = true;
                }
            }
        }
    } else if (data.move.value() < maxExtent) {
        timeline.reset(data.move);
        switch (fixupMode) {
        case Immediate:
            timeline.set(data.move, maxExtent);
            break;
        case ExtentChanged:
            // The target has changed. Don't start from the beginning; just complete the
            // second half of the animation using the new extent.
            timeline.move(data.move, maxExtent, QEasingCurve(QEasingCurve::OutExpo), 3*fixupDuration/4);
            data.fixingUp = true;
            break;
        default: {
                qreal dist = maxExtent - data.move;
                timeline.move(data.move, maxExtent - dist/2, QEasingCurve(QEasingCurve::InQuad), fixupDuration/4);
                timeline.move(data.move, maxExtent, QEasingCurve(QEasingCurve::OutExpo), 3*fixupDuration/4);
                data.fixingUp = true;
            }
        }
    }
    data.inOvershoot = false;
    fixupMode = Normal;
    vTime = timeline.time();
}

void QSGFlickablePrivate::updateBeginningEnd()
{
    Q_Q(QSGFlickable);
    bool atBoundaryChange = false;

    // Vertical
    const int maxyextent = int(-q->maxYExtent());
    const qreal ypos = -vData.move.value();
    bool atBeginning = (ypos <= -q->minYExtent());
    bool atEnd = (maxyextent <= ypos);

    if (atBeginning != vData.atBeginning) {
        vData.atBeginning = atBeginning;
        atBoundaryChange = true;
    }
    if (atEnd != vData.atEnd) {
        vData.atEnd = atEnd;
        atBoundaryChange = true;
    }

    // Horizontal
    const int maxxextent = int(-q->maxXExtent());
    const qreal xpos = -hData.move.value();
    atBeginning = (xpos <= -q->minXExtent());
    atEnd = (maxxextent <= xpos);

    if (atBeginning != hData.atBeginning) {
        hData.atBeginning = atBeginning;
        atBoundaryChange = true;
    }
    if (atEnd != hData.atEnd) {
        hData.atEnd = atEnd;
        atBoundaryChange = true;
    }

    if (atBoundaryChange)
        emit q->isAtBoundaryChanged();

    if (visibleArea)
        visibleArea->updateVisible();
}

/*
XXXTODO add docs describing moving, dragging, flicking properties, e.g.

When the user starts dragging the Flickable, the dragging and moving properties
will be true.

If the velocity is sufficient when the drag is ended, flicking may begin.

The moving properties will remain true until all dragging and flicking
is finished.
*/

/*!
    \qmlsignal QtQuick2::Flickable::onDragStarted()

    This handler is called when the view starts to be dragged due to user
    interaction.
*/

/*!
    \qmlsignal QtQuick2::Flickable::onDragEnded()

    This handler is called when the user stops dragging the view.

    If the velocity of the drag is suffient at the time the
    touch/mouse button is released then a flick will start.
*/

/*!
    \qmlclass Flickable QSGFlickable
    \inqmlmodule QtQuick 2
    \ingroup qml-basic-interaction-elements

    \brief The Flickable item provides a surface that can be "flicked".
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

    \snippet doc/src/snippets/declarative/flickable.qml document

    \clearfloat

    Items declared as children of a Flickable are automatically parented to the
    Flickable's \l contentItem.  This should be taken into account when
    operating on the children of the Flickable; it is usually the children of
    \c contentItem that are relevant.  For example, the bound of Items added
    to the Flickable will be available by \c contentItem.childrenRect

    \section1 Limitations

    \note Due to an implementation detail, items placed inside a Flickable cannot anchor to it by
    \c id. Use \c parent instead.
*/

/*!
    \qmlsignal QtQuick2::Flickable::onMovementStarted()

    This handler is called when the view begins moving due to user
    interaction.
*/

/*!
    \qmlsignal QtQuick2::Flickable::onMovementEnded()

    This handler is called when the view stops moving due to user
    interaction.  If a flick was generated, this handler will
    be triggered once the flick stops.  If a flick was not
    generated, the handler will be triggered when the
    user stops dragging - i.e. a mouse or touch release.
*/

/*!
    \qmlsignal QtQuick2::Flickable::onFlickStarted()

    This handler is called when the view is flicked.  A flick
    starts from the point that the mouse or touch is released,
    while still in motion.
*/

/*!
    \qmlsignal QtQuick2::Flickable::onFlickEnded()

    This handler is called when the view stops moving due to a flick.
*/

/*!
    \qmlproperty real QtQuick2::Flickable::visibleArea.xPosition
    \qmlproperty real QtQuick2::Flickable::visibleArea.widthRatio
    \qmlproperty real QtQuick2::Flickable::visibleArea.yPosition
    \qmlproperty real QtQuick2::Flickable::visibleArea.heightRatio

    These properties describe the position and size of the currently viewed area.
    The size is defined as the percentage of the full view currently visible,
    scaled to 0.0 - 1.0.  The page position is usually in the range 0.0 (beginning) to
    1.0 minus size ratio (end), i.e. \c yPosition is in the range 0.0 to 1.0-\c heightRatio.
    However, it is possible for the contents to be dragged outside of the normal
    range, resulting in the page positions also being outside the normal range.

    These properties are typically used to draw a scrollbar. For example:

    \snippet doc/src/snippets/declarative/flickableScrollbar.qml 0
    \dots 8
    \snippet doc/src/snippets/declarative/flickableScrollbar.qml 1

    \sa {declarative/ui-components/scrollbar}{scrollbar example}
*/
QSGFlickable::QSGFlickable(QSGItem *parent)
  : QSGItem(*(new QSGFlickablePrivate), parent)
{
    Q_D(QSGFlickable);
    d->init();
}

QSGFlickable::QSGFlickable(QSGFlickablePrivate &dd, QSGItem *parent)
  : QSGItem(dd, parent)
{
    Q_D(QSGFlickable);
    d->init();
}

QSGFlickable::~QSGFlickable()
{
}

/*!
    \qmlproperty real QtQuick2::Flickable::contentX
    \qmlproperty real QtQuick2::Flickable::contentY

    These properties hold the surface coordinate currently at the top-left
    corner of the Flickable. For example, if you flick an image up 100 pixels,
    \c contentY will be 100.
*/
qreal QSGFlickable::contentX() const
{
    Q_D(const QSGFlickable);
    return -d->contentItem->x();
}

void QSGFlickable::setContentX(qreal pos)
{
    Q_D(QSGFlickable);
    d->timeline.reset(d->hData.move);
    d->vTime = d->timeline.time();
    movementXEnding();
    if (-pos != d->hData.move.value()) {
        d->hData.move.setValue(-pos);
        viewportMoved();
    }
}

qreal QSGFlickable::contentY() const
{
    Q_D(const QSGFlickable);
    return -d->contentItem->y();
}

void QSGFlickable::setContentY(qreal pos)
{
    Q_D(QSGFlickable);
    d->timeline.reset(d->vData.move);
    d->vTime = d->timeline.time();
    movementYEnding();
    if (-pos != d->vData.move.value()) {
        d->vData.move.setValue(-pos);
        viewportMoved();
    }
}

/*!
    \qmlproperty bool QtQuick2::Flickable::interactive

    This property describes whether the user can interact with the Flickable.
    A user cannot drag or flick a Flickable that is not interactive.

    By default, this property is true.

    This property is useful for temporarily disabling flicking. This allows
    special interaction with Flickable's children; for example, you might want
    to freeze a flickable map while scrolling through a pop-up dialog that
    is a child of the Flickable.
*/
bool QSGFlickable::isInteractive() const
{
    Q_D(const QSGFlickable);
    return d->interactive;
}

void QSGFlickable::setInteractive(bool interactive)
{
    Q_D(QSGFlickable);
    if (interactive != d->interactive) {
        d->interactive = interactive;
        if (!interactive && (d->flickingHorizontally || d->flickingVertically)) {
            d->timeline.clear();
            d->vTime = d->timeline.time();
            d->flickingHorizontally = false;
            d->flickingVertically = false;
            emit flickingChanged();
            emit flickingHorizontallyChanged();
            emit flickingVerticallyChanged();
            emit flickEnded();
        }
        emit interactiveChanged();
    }
}

/*!
    \qmlproperty real QtQuick2::Flickable::horizontalVelocity
    \qmlproperty real QtQuick2::Flickable::verticalVelocity

    The instantaneous velocity of movement along the x and y axes, in pixels/sec.

    The reported velocity is smoothed to avoid erratic output.
*/
qreal QSGFlickable::horizontalVelocity() const
{
    Q_D(const QSGFlickable);
    return d->hData.smoothVelocity.value();
}

qreal QSGFlickable::verticalVelocity() const
{
    Q_D(const QSGFlickable);
    return d->vData.smoothVelocity.value();
}

/*!
    \qmlproperty bool QtQuick2::Flickable::atXBeginning
    \qmlproperty bool QtQuick2::Flickable::atXEnd
    \qmlproperty bool QtQuick2::Flickable::atYBeginning
    \qmlproperty bool QtQuick2::Flickable::atYEnd

    These properties are true if the flickable view is positioned at the beginning,
    or end respecively.
*/
bool QSGFlickable::isAtXEnd() const
{
    Q_D(const QSGFlickable);
    return d->hData.atEnd;
}

bool QSGFlickable::isAtXBeginning() const
{
    Q_D(const QSGFlickable);
    return d->hData.atBeginning;
}

bool QSGFlickable::isAtYEnd() const
{
    Q_D(const QSGFlickable);
    return d->vData.atEnd;
}

bool QSGFlickable::isAtYBeginning() const
{
    Q_D(const QSGFlickable);
    return d->vData.atBeginning;
}

void QSGFlickable::ticked()
{
    viewportMoved();
}

/*!
    \qmlproperty Item QtQuick2::Flickable::contentItem

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
QSGItem *QSGFlickable::contentItem()
{
    Q_D(QSGFlickable);
    return d->contentItem;
}

QSGFlickableVisibleArea *QSGFlickable::visibleArea()
{
    Q_D(QSGFlickable);
    if (!d->visibleArea)
        d->visibleArea = new QSGFlickableVisibleArea(this);
    return d->visibleArea;
}

/*!
    \qmlproperty enumeration QtQuick2::Flickable::flickableDirection

    This property determines which directions the view can be flicked.

    \list
    \o Flickable.AutoFlickDirection (default) - allows flicking vertically if the
    \e contentHeight is not equal to the \e height of the Flickable.
    Allows flicking horizontally if the \e contentWidth is not equal
    to the \e width of the Flickable.
    \o Flickable.HorizontalFlick - allows flicking horizontally.
    \o Flickable.VerticalFlick - allows flicking vertically.
    \o Flickable.HorizontalAndVerticalFlick - allows flicking in both directions.
    \endlist
*/
QSGFlickable::FlickableDirection QSGFlickable::flickableDirection() const
{
    Q_D(const QSGFlickable);
    return d->flickableDirection;
}

void QSGFlickable::setFlickableDirection(FlickableDirection direction)
{
    Q_D(QSGFlickable);
    if (direction != d->flickableDirection) {
        d->flickableDirection = direction;
        emit flickableDirectionChanged();
    }
}

bool QSGFlickable::pixelAligned() const
{
    Q_D(const QSGFlickable);
    return d->pixelAligned;
}

void QSGFlickable::setPixelAligned(bool align)
{
    Q_D(QSGFlickable);
    if (align != d->pixelAligned) {
        d->pixelAligned = align;
        emit pixelAlignedChanged();
    }
}

void QSGFlickablePrivate::handleMousePressEvent(QMouseEvent *event)
{
    Q_Q(QSGFlickable);
    if (interactive && timeline.isActive()
        && (qAbs(hData.smoothVelocity.value()) > RetainGrabVelocity
            || qAbs(vData.smoothVelocity.value()) > RetainGrabVelocity)) {
        stealMouse = true; // If we've been flicked then steal the click.
    } else {
        stealMouse = false;
    }
    q->setKeepMouseGrab(stealMouse);
    pressed = true;
    timeline.clear();
    hData.reset();
    vData.reset();
    hData.dragMinBound = q->minXExtent();
    vData.dragMinBound = q->minYExtent();
    hData.dragMaxBound = q->maxXExtent();
    vData.dragMaxBound = q->maxYExtent();
    fixupMode = Normal;
    lastPos = QPoint();
    QSGItemPrivate::start(lastPosTime);
    pressPos = event->localPos();
    hData.pressPos = hData.move.value();
    vData.pressPos = vData.move.value();
    flickingHorizontally = false;
    flickingVertically = false;
    QSGItemPrivate::start(pressTime);
    QSGItemPrivate::start(velocityTime);
}

void QSGFlickablePrivate::handleMouseMoveEvent(QMouseEvent *event)
{
    Q_Q(QSGFlickable);
    if (!interactive || !lastPosTime.isValid())
        return;
    bool rejectY = false;
    bool rejectX = false;

    bool stealY = stealMouse;
    bool stealX = stealMouse;

    if (q->yflick()) {
        int dy = int(event->localPos().y() - pressPos.y());
        if (qAbs(dy) > qApp->styleHints()->startDragDistance() || QSGItemPrivate::elapsed(pressTime) > 200) {
            if (!vMoved)
                vData.dragStartOffset = dy;
            qreal newY = dy + vData.pressPos - vData.dragStartOffset;
            const qreal minY = vData.dragMinBound;
            const qreal maxY = vData.dragMaxBound;
            if (newY > minY)
                newY = minY + (newY - minY) / 2;
            if (newY < maxY && maxY - minY <= 0)
                newY = maxY + (newY - maxY) / 2;
            if (boundsBehavior == QSGFlickable::StopAtBounds && (newY > minY || newY < maxY)) {
                rejectY = true;
                if (newY < maxY) {
                    newY = maxY;
                    rejectY = false;
                }
                if (newY > minY) {
                    newY = minY;
                    rejectY = false;
                }
            }
            if (!rejectY && stealMouse) {
                vData.move.setValue(qRound(newY));
                vMoved = true;
            }
            if (qAbs(dy) > qApp->styleHints()->startDragDistance())
                stealY = true;
        }
    }

    if (q->xflick()) {
        int dx = int(event->localPos().x() - pressPos.x());
        if (qAbs(dx) > qApp->styleHints()->startDragDistance() || QSGItemPrivate::elapsed(pressTime) > 200) {
            if (!hMoved)
                hData.dragStartOffset = dx;
            qreal newX = dx + hData.pressPos - hData.dragStartOffset;
            const qreal minX = hData.dragMinBound;
            const qreal maxX = hData.dragMaxBound;
            if (newX > minX)
                newX = minX + (newX - minX) / 2;
            if (newX < maxX && maxX - minX <= 0)
                newX = maxX + (newX - maxX) / 2;
            if (boundsBehavior == QSGFlickable::StopAtBounds && (newX > minX || newX < maxX)) {
                rejectX = true;
                if (newX < maxX) {
                    newX = maxX;
                    rejectX = false;
                }
                if (newX > minX) {
                    newX = minX;
                    rejectX = false;
                }
            }
            if (!rejectX && stealMouse) {
                hData.move.setValue(qRound(newX));
                hMoved = true;
            }

            if (qAbs(dx) > qApp->styleHints()->startDragDistance())
                stealX = true;
        }
    }

    stealMouse = stealX || stealY;
    if (stealMouse)
        q->setKeepMouseGrab(true);

    if (rejectY) {
        vData.velocityBuffer.clear();
        vData.velocity = 0;
    }
    if (rejectX) {
        hData.velocityBuffer.clear();
        hData.velocity = 0;
    }

    if (hMoved || vMoved) {
        draggingStarting();
        q->movementStarting();
        q->viewportMoved();
    }

    if (!lastPos.isNull()) {
        qreal elapsed = qreal(QSGItemPrivate::elapsed(lastPosTime)) / 1000.;
        if (elapsed <= 0)
            return;
        QSGItemPrivate::restart(lastPosTime);
        qreal dy = event->localPos().y()-lastPos.y();
        if (q->yflick() && !rejectY)
            vData.addVelocitySample(dy/elapsed, maxVelocity);
        qreal dx = event->localPos().x()-lastPos.x();
        if (q->xflick() && !rejectX)
            hData.addVelocitySample(dx/elapsed, maxVelocity);
    }

    lastPos = event->localPos();
}

void QSGFlickablePrivate::handleMouseReleaseEvent(QMouseEvent *event)
{
    Q_Q(QSGFlickable);
    stealMouse = false;
    q->setKeepMouseGrab(false);
    pressed = false;

    // if we drag then pause before release we should not cause a flick.
    if (QSGItemPrivate::elapsed(lastPosTime) < 100) {
        vData.updateVelocity();
        hData.updateVelocity();
    } else {
        hData.velocity = 0.0;
        vData.velocity = 0.0;
    }

    draggingEnding();

    if (!lastPosTime.isValid())
        return;

    vTime = timeline.time();

    qreal velocity = vData.velocity;
    if (vData.atBeginning || vData.atEnd)
        velocity /= 2;
    if (qAbs(velocity) > MinimumFlickVelocity && qAbs(event->localPos().y() - pressPos.y()) > FlickThreshold)
        flickY(velocity);
    else
        fixupY();

    velocity = hData.velocity;
    if (hData.atBeginning || hData.atEnd)
        velocity /= 2;
    if (qAbs(velocity) > MinimumFlickVelocity && qAbs(event->localPos().x() - pressPos.x()) > FlickThreshold)
        flickX(velocity);
    else
        fixupX();

    if (!timeline.isActive())
        q->movementEnding();
}

void QSGFlickable::mousePressEvent(QMouseEvent *event)
{
    Q_D(QSGFlickable);
    if (d->interactive) {
        if (!d->pressed)
            d->handleMousePressEvent(event);
        event->accept();
    } else {
        QSGItem::mousePressEvent(event);
    }
}

void QSGFlickable::mouseMoveEvent(QMouseEvent *event)
{
    Q_D(QSGFlickable);
    if (d->interactive) {
        d->handleMouseMoveEvent(event);
        event->accept();
    } else {
        QSGItem::mouseMoveEvent(event);
    }
}

void QSGFlickable::mouseReleaseEvent(QMouseEvent *event)
{
    Q_D(QSGFlickable);
    if (d->interactive) {
        d->clearDelayedPress();
        d->handleMouseReleaseEvent(event);
        event->accept();
        ungrabMouse();
    } else {
        QSGItem::mouseReleaseEvent(event);
    }
}

void QSGFlickable::wheelEvent(QWheelEvent *event)
{
    Q_D(QSGFlickable);
    if (!d->interactive) {
        QSGItem::wheelEvent(event);
    } else if (yflick() && event->orientation() == Qt::Vertical) {
        bool valid = false;
        if (event->delta() > 0 && contentY() > -minYExtent()) {
            d->vData.velocity = qMax(event->delta()*2 - d->vData.smoothVelocity.value(), qreal(d->maxVelocity/4));
            valid = true;
        } else if (event->delta() < 0 && contentY() < -maxYExtent()) {
            d->vData.velocity = qMin(event->delta()*2 - d->vData.smoothVelocity.value(), qreal(-d->maxVelocity/4));
            valid = true;
        }
        if (valid) {
            d->flickingVertically = false;
            d->flickY(d->vData.velocity);
            if (d->flickingVertically) {
                d->vMoved = true;
                movementStarting();
            }
            event->accept();
        }
    } else if (xflick() && event->orientation() == Qt::Horizontal) {
        bool valid = false;
        if (event->delta() > 0 && contentX() > -minXExtent()) {
            d->hData.velocity = qMax(event->delta()*2 - d->hData.smoothVelocity.value(), qreal(d->maxVelocity/4));
            valid = true;
        } else if (event->delta() < 0 && contentX() < -maxXExtent()) {
            d->hData.velocity = qMin(event->delta()*2 - d->hData.smoothVelocity.value(), qreal(-d->maxVelocity/4));
            valid = true;
        }
        if (valid) {
            d->flickingHorizontally = false;
            d->flickX(d->hData.velocity);
            if (d->flickingHorizontally) {
                d->hMoved = true;
                movementStarting();
            }
            event->accept();
        }
    } else {
        QSGItem::wheelEvent(event);
    }
}

bool QSGFlickablePrivate::isOutermostPressDelay() const
{
    Q_Q(const QSGFlickable);
    QSGItem *item = q->parentItem();
    while (item) {
        QSGFlickable *flick = qobject_cast<QSGFlickable*>(item);
        if (flick && flick->pressDelay() > 0 && flick->isInteractive())
            return false;
        item = item->parentItem();
    }

    return true;
}

void QSGFlickablePrivate::captureDelayedPress(QMouseEvent *event)
{
    Q_Q(QSGFlickable);
    if (!q->canvas() || pressDelay <= 0)
        return;
    if (!isOutermostPressDelay())
        return;
    delayedPressTarget = q->canvas()->mouseGrabberItem();
    delayedPressEvent = new QMouseEvent(*event);
    delayedPressEvent->setAccepted(false);
    delayedPressTimer.start(pressDelay, q);
}

void QSGFlickablePrivate::clearDelayedPress()
{
    if (delayedPressEvent) {
        delayedPressTimer.stop();
        delete delayedPressEvent;
        delayedPressEvent = 0;
    }
}

//XXX pixelAligned ignores the global position of the Flickable, i.e. assumes Flickable itself is pixel aligned.
void QSGFlickablePrivate::setViewportX(qreal x)
{
    contentItem->setX(pixelAligned ? qRound(x) : x);
}

void QSGFlickablePrivate::setViewportY(qreal y)
{
    contentItem->setY(pixelAligned ? qRound(y) : y);
}

void QSGFlickable::timerEvent(QTimerEvent *event)
{
    Q_D(QSGFlickable);
    if (event->timerId() == d->delayedPressTimer.timerId()) {
        d->delayedPressTimer.stop();
        if (d->delayedPressEvent) {
            QSGItem *grabber = canvas() ? canvas()->mouseGrabberItem() : 0;
            if (!grabber || grabber != this) {
                // We replay the mouse press but the grabber we had might not be interessted by the event (e.g. overlay)
                // so we reset the grabber
                if (canvas()->mouseGrabberItem() == d->delayedPressTarget)
                    d->delayedPressTarget->ungrabMouse();
                // Use the event handler that will take care of finding the proper item to propagate the event
                QSGCanvasPrivate::get(canvas())->deliverMouseEvent(d->delayedPressEvent);
            }
            delete d->delayedPressEvent;
            d->delayedPressEvent = 0;
        }
    }
}

qreal QSGFlickable::minYExtent() const
{
    return 0.0;
}

qreal QSGFlickable::minXExtent() const
{
    return 0.0;
}

/* returns -ve */
qreal QSGFlickable::maxXExtent() const
{
    return width() - vWidth();
}
/* returns -ve */
qreal QSGFlickable::maxYExtent() const
{
    return height() - vHeight();
}

void QSGFlickable::viewportMoved()
{
    Q_D(QSGFlickable);

    qreal prevX = d->lastFlickablePosition.x();
    qreal prevY = d->lastFlickablePosition.y();
    d->velocityTimeline.clear();
    if (d->pressed || d->calcVelocity) {
        int elapsed = QSGItemPrivate::restart(d->velocityTime);
        if (elapsed > 0) {
            qreal horizontalVelocity = (prevX - d->hData.move.value()) * 1000 / elapsed;
            qreal verticalVelocity = (prevY - d->vData.move.value()) * 1000 / elapsed;
            d->velocityTimeline.move(d->hData.smoothVelocity, horizontalVelocity, d->reportedVelocitySmoothing);
            d->velocityTimeline.move(d->hData.smoothVelocity, 0, d->reportedVelocitySmoothing);
            d->velocityTimeline.move(d->vData.smoothVelocity, verticalVelocity, d->reportedVelocitySmoothing);
            d->velocityTimeline.move(d->vData.smoothVelocity, 0, d->reportedVelocitySmoothing);
        }
    } else {
        if (d->timeline.time() > d->vTime) {
            qreal horizontalVelocity = (prevX - d->hData.move.value()) * 1000 / (d->timeline.time() - d->vTime);
            qreal verticalVelocity = (prevY - d->vData.move.value()) * 1000 / (d->timeline.time() - d->vTime);
            d->hData.smoothVelocity.setValue(horizontalVelocity);
            d->vData.smoothVelocity.setValue(verticalVelocity);
        }
    }

    if (!d->vData.inOvershoot && !d->vData.fixingUp && d->flickingVertically
            && (d->vData.move.value() > minYExtent() || d->vData.move.value() < maxYExtent())
            && qAbs(d->vData.smoothVelocity.value()) > 100) {
        // Increase deceleration if we've passed a bound
        d->vData.inOvershoot = true;
        qreal maxDistance = d->overShootDistance(height());
        d->timeline.reset(d->vData.move);
        d->timeline.accel(d->vData.move, -d->vData.smoothVelocity.value(), d->deceleration*QML_FLICK_OVERSHOOTFRICTION, maxDistance);
        d->timeline.callback(QDeclarativeTimeLineCallback(&d->vData.move, d->fixupY_callback, d));
    }
    if (!d->hData.inOvershoot && !d->hData.fixingUp && d->flickingHorizontally
            && (d->hData.move.value() > minXExtent() || d->hData.move.value() < maxXExtent())
            && qAbs(d->hData.smoothVelocity.value()) > 100) {
        // Increase deceleration if we've passed a bound
        d->hData.inOvershoot = true;
        qreal maxDistance = d->overShootDistance(width());
        d->timeline.reset(d->hData.move);
        d->timeline.accel(d->hData.move, -d->hData.smoothVelocity.value(), d->deceleration*QML_FLICK_OVERSHOOTFRICTION, maxDistance);
        d->timeline.callback(QDeclarativeTimeLineCallback(&d->hData.move, d->fixupX_callback, d));
    }

    d->lastFlickablePosition = QPointF(d->hData.move.value(), d->vData.move.value());

    d->vTime = d->timeline.time();
    d->updateBeginningEnd();
}

void QSGFlickable::geometryChanged(const QRectF &newGeometry,
                             const QRectF &oldGeometry)
{
    Q_D(QSGFlickable);
    QSGItem::geometryChanged(newGeometry, oldGeometry);

    bool changed = false;
    if (newGeometry.width() != oldGeometry.width()) {
        if (xflick())
            changed = true;
        if (d->hData.viewSize < 0) {
            d->contentItem->setWidth(width());
            emit contentWidthChanged();
        }
        // Make sure that we're entirely in view.
        if (!d->pressed && !d->movingHorizontally && !d->movingVertically) {
            d->fixupMode = QSGFlickablePrivate::Immediate;
            d->fixupX();
        }
    }
    if (newGeometry.height() != oldGeometry.height()) {
        if (yflick())
            changed = true;
        if (d->vData.viewSize < 0) {
            d->contentItem->setHeight(height());
            emit contentHeightChanged();
        }
        // Make sure that we're entirely in view.
        if (!d->pressed && !d->movingHorizontally && !d->movingVertically) {
            d->fixupMode = QSGFlickablePrivate::Immediate;
            d->fixupY();
        }
    }

    if (changed)
        d->updateBeginningEnd();
}

void QSGFlickable::cancelFlick()
{
    Q_D(QSGFlickable);
    d->timeline.reset(d->hData.move);
    d->timeline.reset(d->vData.move);
    movementEnding();
}

void QSGFlickablePrivate::data_append(QDeclarativeListProperty<QObject> *prop, QObject *o)
{
    QSGItem *i = qobject_cast<QSGItem *>(o);
    if (i) {
        i->setParentItem(static_cast<QSGFlickablePrivate*>(prop->data)->contentItem);
    } else {
        o->setParent(prop->object); // XXX todo - do we want this?
    }
}

int QSGFlickablePrivate::data_count(QDeclarativeListProperty<QObject> *)
{
    // XXX todo
    return 0;
}

QObject *QSGFlickablePrivate::data_at(QDeclarativeListProperty<QObject> *, int)
{
    // XXX todo
    return 0;
}

void QSGFlickablePrivate::data_clear(QDeclarativeListProperty<QObject> *)
{
    // XXX todo
}

QDeclarativeListProperty<QObject> QSGFlickable::flickableData()
{
    Q_D(QSGFlickable);
    return QDeclarativeListProperty<QObject>(this, (void *)d, QSGFlickablePrivate::data_append,
                                             QSGFlickablePrivate::data_count,
                                             QSGFlickablePrivate::data_at,
                                             QSGFlickablePrivate::data_clear);
}

QDeclarativeListProperty<QSGItem> QSGFlickable::flickableChildren()
{
    Q_D(QSGFlickable);
    return QSGItemPrivate::get(d->contentItem)->children();
}

/*!
    \qmlproperty enumeration QtQuick2::Flickable::boundsBehavior
    This property holds whether the surface may be dragged
    beyond the Fickable's boundaries, or overshoot the
    Flickable's boundaries when flicked.

    This enables the feeling that the edges of the view are soft,
    rather than a hard physical boundary.

    The \c boundsBehavior can be one of:

    \list
    \o Flickable.StopAtBounds - the contents can not be dragged beyond the boundary
    of the flickable, and flicks will not overshoot.
    \o Flickable.DragOverBounds - the contents can be dragged beyond the boundary
    of the Flickable, but flicks will not overshoot.
    \o Flickable.DragAndOvershootBounds (default) - the contents can be dragged
    beyond the boundary of the Flickable, and can overshoot the
    boundary when flicked.
    \endlist
*/
QSGFlickable::BoundsBehavior QSGFlickable::boundsBehavior() const
{
    Q_D(const QSGFlickable);
    return d->boundsBehavior;
}

void QSGFlickable::setBoundsBehavior(BoundsBehavior b)
{
    Q_D(QSGFlickable);
    if (b == d->boundsBehavior)
        return;
    d->boundsBehavior = b;
    emit boundsBehaviorChanged();
}

/*!
    \qmlproperty real QtQuick2::Flickable::contentWidth
    \qmlproperty real QtQuick2::Flickable::contentHeight

    The dimensions of the content (the surface controlled by Flickable).
    This should typically be set to the combined size of the items placed in the
    Flickable.

    The following snippet shows how these properties are used to display
    an image that is larger than the Flickable item itself:

    \snippet doc/src/snippets/declarative/flickable.qml document

    In some cases, the the content dimensions can be automatically set
    using the \l {Item::childrenRect.width}{childrenRect.width}
    and \l {Item::childrenRect.height}{childrenRect.height} properties.
*/
qreal QSGFlickable::contentWidth() const
{
    Q_D(const QSGFlickable);
    return d->hData.viewSize;
}

void QSGFlickable::setContentWidth(qreal w)
{
    Q_D(QSGFlickable);
    if (d->hData.viewSize == w)
        return;
    d->hData.viewSize = w;
    if (w < 0)
        d->contentItem->setWidth(width());
    else
        d->contentItem->setWidth(w);
    // Make sure that we're entirely in view.
    if (!d->pressed && !d->movingHorizontally && !d->movingVertically) {
        d->fixupMode = QSGFlickablePrivate::Immediate;
        d->fixupX();
    } else if (!d->pressed && d->hData.fixingUp) {
        d->fixupMode = QSGFlickablePrivate::ExtentChanged;
        d->fixupX();
    }
    emit contentWidthChanged();
    d->updateBeginningEnd();
}

qreal QSGFlickable::contentHeight() const
{
    Q_D(const QSGFlickable);
    return d->vData.viewSize;
}

void QSGFlickable::setContentHeight(qreal h)
{
    Q_D(QSGFlickable);
    if (d->vData.viewSize == h)
        return;
    d->vData.viewSize = h;
    if (h < 0)
        d->contentItem->setHeight(height());
    else
        d->contentItem->setHeight(h);
    // Make sure that we're entirely in view.
    if (!d->pressed && !d->movingHorizontally && !d->movingVertically) {
        d->fixupMode = QSGFlickablePrivate::Immediate;
        d->fixupY();
    } else if (!d->pressed && d->vData.fixingUp) {
        d->fixupMode = QSGFlickablePrivate::ExtentChanged;
        d->fixupY();
    }
    emit contentHeightChanged();
    d->updateBeginningEnd();
}

/*!
    \qmlmethod QtQuick2::Flickable::resizeContent(real width, real height, QPointF center)
    \preliminary

    Resizes the content to \a width x \a height about \a center.

    This does not scale the contents of the Flickable - it only resizes the \l contentWidth
    and \l contentHeight.

    Resizing the content may result in the content being positioned outside
    the bounds of the Flickable.  Calling \l returnToBounds() will
    move the content back within legal bounds.
*/
void QSGFlickable::resizeContent(qreal w, qreal h, QPointF center)
{
    Q_D(QSGFlickable);
    if (w != d->hData.viewSize) {
        qreal oldSize = d->hData.viewSize;
        d->hData.viewSize = w;
        d->contentItem->setWidth(w);
        emit contentWidthChanged();
        if (center.x() != 0) {
            qreal pos = center.x() * w / oldSize;
            setContentX(contentX() + pos - center.x());
        }
    }
    if (h != d->vData.viewSize) {
        qreal oldSize = d->vData.viewSize;
        d->vData.viewSize = h;
        d->contentItem->setHeight(h);
        emit contentHeightChanged();
        if (center.y() != 0) {
            qreal pos = center.y() * h / oldSize;
            setContentY(contentY() + pos - center.y());
        }
    }
    d->updateBeginningEnd();
}

/*!
    \qmlmethod QtQuick2::Flickable::returnToBounds()
    \preliminary

    Ensures the content is within legal bounds.

    This may be called to ensure that the content is within legal bounds
    after manually positioning the content.
*/
void QSGFlickable::returnToBounds()
{
    Q_D(QSGFlickable);
    d->fixupX();
    d->fixupY();
}

qreal QSGFlickable::vWidth() const
{
    Q_D(const QSGFlickable);
    if (d->hData.viewSize < 0)
        return width();
    else
        return d->hData.viewSize;
}

qreal QSGFlickable::vHeight() const
{
    Q_D(const QSGFlickable);
    if (d->vData.viewSize < 0)
        return height();
    else
        return d->vData.viewSize;
}

bool QSGFlickable::xflick() const
{
    Q_D(const QSGFlickable);
    if (d->flickableDirection == QSGFlickable::AutoFlickDirection)
        return vWidth() != width();
    return d->flickableDirection & QSGFlickable::HorizontalFlick;
}

bool QSGFlickable::yflick() const
{
    Q_D(const QSGFlickable);
    if (d->flickableDirection == QSGFlickable::AutoFlickDirection)
        return vHeight() !=  height();
    return d->flickableDirection & QSGFlickable::VerticalFlick;
}

void QSGFlickable::mouseUngrabEvent()
{
    Q_D(QSGFlickable);
    if (d->pressed) {
        // if our mouse grab has been removed (probably by another Flickable),
        // fix our state
        d->pressed = false;
        d->draggingEnding();
        d->stealMouse = false;
        setKeepMouseGrab(false);
    }
}

bool QSGFlickable::sendMouseEvent(QMouseEvent *event)
{
    Q_D(QSGFlickable);
    QRectF myRect = mapRectToScene(QRectF(0, 0, width(), height()));

    QSGCanvas *c = canvas();
    QSGItem *grabber = c ? c->mouseGrabberItem() : 0;
    bool disabledItem = grabber && !grabber->isEnabled();
    bool stealThisEvent = d->stealMouse;
    if ((stealThisEvent || myRect.contains(event->windowPos())) && (!grabber || !grabber->keepMouseGrab() || disabledItem)) {
        QMouseEvent mouseEvent(event->type(), mapFromScene(event->windowPos()), event->windowPos(), event->screenPos(),
                               event->button(), event->buttons(), event->modifiers());

        mouseEvent.setAccepted(false);

        switch(mouseEvent.type()) {
        case QEvent::MouseMove:
            d->handleMouseMoveEvent(&mouseEvent);
            break;
        case QEvent::MouseButtonPress:
            if (d->pressed) // we are already pressed - this is a delayed replay
                return false;

            d->handleMousePressEvent(&mouseEvent);
            d->captureDelayedPress(event);
            stealThisEvent = d->stealMouse;   // Update stealThisEvent in case changed by function call above
            break;
        case QEvent::MouseButtonRelease:
            if (d->delayedPressEvent) {
                // We replay the mouse press but the grabber we had might not be interessted by the event (e.g. overlay)
                // so we reset the grabber
                if (c->mouseGrabberItem() == d->delayedPressTarget)
                    d->delayedPressTarget->ungrabMouse();
                //Use the event handler that will take care of finding the proper item to propagate the event
                QSGCanvasPrivate::get(canvas())->deliverMouseEvent(d->delayedPressEvent);
                d->clearDelayedPress();
                // We send the release
                canvas()->sendEvent(c->mouseGrabberItem(), event);
                // And the event has been consumed
                d->stealMouse = false;
                d->pressed = false;
                return true;
            }
            d->handleMouseReleaseEvent(&mouseEvent);
            break;
        default:
            break;
        }
        grabber = qobject_cast<QSGItem*>(c->mouseGrabberItem());
        if ((grabber && stealThisEvent && !grabber->keepMouseGrab() && grabber != this) || disabledItem) {
            d->clearDelayedPress();
            grabMouse();
        }

        return stealThisEvent || d->delayedPressEvent || disabledItem;
    } else if (d->lastPosTime.isValid()) {
        d->lastPosTime.invalidate();
        returnToBounds();
    }
    if (event->type() == QEvent::MouseButtonRelease) {
        d->lastPosTime.invalidate();
        d->clearDelayedPress();
        d->stealMouse = false;
        d->pressed = false;
    }
    return false;
}


bool QSGFlickable::childMouseEventFilter(QSGItem *i, QEvent *e)
{
    Q_D(QSGFlickable);
    if (!isVisible() || !d->interactive || !isEnabled())
        return QSGItem::childMouseEventFilter(i, e);
    switch (e->type()) {
    case QEvent::MouseButtonPress:
    case QEvent::MouseMove:
    case QEvent::MouseButtonRelease:
        return sendMouseEvent(static_cast<QMouseEvent *>(e));
    default:
        break;
    }

    return QSGItem::childMouseEventFilter(i, e);
}

/*!
    \qmlproperty real QtQuick2::Flickable::maximumFlickVelocity
    This property holds the maximum velocity that the user can flick the view in pixels/second.

    The default value is platform dependent.
*/
qreal QSGFlickable::maximumFlickVelocity() const
{
    Q_D(const QSGFlickable);
    return d->maxVelocity;
}

void QSGFlickable::setMaximumFlickVelocity(qreal v)
{
    Q_D(QSGFlickable);
    if (v == d->maxVelocity)
        return;
    d->maxVelocity = v;
    emit maximumFlickVelocityChanged();
}

/*!
    \qmlproperty real QtQuick2::Flickable::flickDeceleration
    This property holds the rate at which a flick will decelerate.

    The default value is platform dependent.
*/
qreal QSGFlickable::flickDeceleration() const
{
    Q_D(const QSGFlickable);
    return d->deceleration;
}

void QSGFlickable::setFlickDeceleration(qreal deceleration)
{
    Q_D(QSGFlickable);
    if (deceleration == d->deceleration)
        return;
    d->deceleration = deceleration;
    emit flickDecelerationChanged();
}

bool QSGFlickable::isFlicking() const
{
    Q_D(const QSGFlickable);
    return d->flickingHorizontally ||  d->flickingVertically;
}

/*!
    \qmlproperty bool QtQuick2::Flickable::flicking
    \qmlproperty bool QtQuick2::Flickable::flickingHorizontally
    \qmlproperty bool QtQuick2::Flickable::flickingVertically

    These properties describe whether the view is currently moving horizontally,
    vertically or in either direction, due to the user flicking the view.
*/
bool QSGFlickable::isFlickingHorizontally() const
{
    Q_D(const QSGFlickable);
    return d->flickingHorizontally;
}

bool QSGFlickable::isFlickingVertically() const
{
    Q_D(const QSGFlickable);
    return d->flickingVertically;
}

/*!
    \qmlproperty bool QtQuick2::Flickable::dragging
    \qmlproperty bool QtQuick2::Flickable::draggingHorizontally
    \qmlproperty bool QtQuick2::Flickable::draggingVertically

    These properties describe whether the view is currently moving horizontally,
    vertically or in either direction, due to the user dragging the view.
*/
bool QSGFlickable::isDragging() const
{
    Q_D(const QSGFlickable);
    return d->hData.dragging ||  d->vData.dragging;
}

bool QSGFlickable::isDraggingHorizontally() const
{
    Q_D(const QSGFlickable);
    return d->hData.dragging;
}

bool QSGFlickable::isDraggingVertically() const
{
    Q_D(const QSGFlickable);
    return d->vData.dragging;
}

void QSGFlickablePrivate::draggingStarting()
{
    Q_Q(QSGFlickable);
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

void QSGFlickablePrivate::draggingEnding()
{
    Q_Q(QSGFlickable);
    bool wasDragging = hData.dragging || vData.dragging;
    if (hData.dragging) {
        hData.dragging = false;
        emit q->draggingHorizontallyChanged();
    }
    if (vData.dragging) {
        vData.dragging = false;
        emit q->draggingVerticallyChanged();
    }
    if (wasDragging && !hData.dragging && !vData.dragging) {
        emit q->draggingChanged();
        emit q->dragEnded();
    }
}

/*!
    \qmlproperty int QtQuick2::Flickable::pressDelay

    This property holds the time to delay (ms) delivering a press to
    children of the Flickable.  This can be useful where reacting
    to a press before a flicking action has undesirable effects.

    If the flickable is dragged/flicked before the delay times out
    the press event will not be delivered.  If the button is released
    within the timeout, both the press and release will be delivered.

    Note that for nested Flickables with pressDelay set, the pressDelay of
    inner Flickables is overridden by the outermost Flickable.
*/
int QSGFlickable::pressDelay() const
{
    Q_D(const QSGFlickable);
    return d->pressDelay;
}

void QSGFlickable::setPressDelay(int delay)
{
    Q_D(QSGFlickable);
    if (d->pressDelay == delay)
        return;
    d->pressDelay = delay;
    emit pressDelayChanged();
}

/*!
    \qmlproperty bool QtQuick2::Flickable::moving
    \qmlproperty bool QtQuick2::Flickable::movingHorizontally
    \qmlproperty bool QtQuick2::Flickable::movingVertically

    These properties describe whether the view is currently moving horizontally,
    vertically or in either direction, due to the user either dragging or
    flicking the view.
*/

bool QSGFlickable::isMoving() const
{
    Q_D(const QSGFlickable);
    return d->movingHorizontally || d->movingVertically;
}

bool QSGFlickable::isMovingHorizontally() const
{
    Q_D(const QSGFlickable);
    return d->movingHorizontally;
}

bool QSGFlickable::isMovingVertically() const
{
    Q_D(const QSGFlickable);
    return d->movingVertically;
}

void QSGFlickable::movementStarting()
{
    Q_D(QSGFlickable);
    if (d->hMoved && !d->movingHorizontally) {
        d->movingHorizontally = true;
        emit movingChanged();
        emit movingHorizontallyChanged();
        if (!d->movingVertically)
            emit movementStarted();
    }
    else if (d->vMoved && !d->movingVertically) {
        d->movingVertically = true;
        emit movingChanged();
        emit movingVerticallyChanged();
        if (!d->movingHorizontally)
            emit movementStarted();
    }
}

void QSGFlickable::movementEnding()
{
    Q_D(QSGFlickable);
    movementXEnding();
    movementYEnding();
    d->hData.smoothVelocity.setValue(0);
    d->vData.smoothVelocity.setValue(0);
}

void QSGFlickable::movementXEnding()
{
    Q_D(QSGFlickable);
    if (d->flickingHorizontally) {
        d->flickingHorizontally = false;
        emit flickingChanged();
        emit flickingHorizontallyChanged();
        if (!d->flickingVertically)
           emit flickEnded();
    }
    if (!d->pressed && !d->stealMouse) {
        if (d->movingHorizontally) {
            d->movingHorizontally = false;
            d->hMoved = false;
            emit movingChanged();
            emit movingHorizontallyChanged();
            if (!d->movingVertically)
                emit movementEnded();
        }
    }
    d->hData.fixingUp = false;
}

void QSGFlickable::movementYEnding()
{
    Q_D(QSGFlickable);
    if (d->flickingVertically) {
        d->flickingVertically = false;
        emit flickingChanged();
        emit flickingVerticallyChanged();
        if (!d->flickingHorizontally)
           emit flickEnded();
    }
    if (!d->pressed && !d->stealMouse) {
        if (d->movingVertically) {
            d->movingVertically = false;
            d->vMoved = false;
            emit movingChanged();
            emit movingVerticallyChanged();
            if (!d->movingHorizontally)
                emit movementEnded();
        }
    }
    d->vData.fixingUp = false;
}

void QSGFlickablePrivate::updateVelocity()
{
    Q_Q(QSGFlickable);
    emit q->horizontalVelocityChanged();
    emit q->verticalVelocityChanged();
}

QT_END_NAMESPACE
