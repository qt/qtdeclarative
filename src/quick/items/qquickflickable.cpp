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

#include "qquickflickable_p.h"
#include "qquickflickable_p_p.h"
#include "qquickcanvas.h"
#include "qquickcanvas_p.h"

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


QQuickFlickablePrivate::QQuickFlickablePrivate()
  : contentItem(new QQuickItem)
    , hData(this, &QQuickFlickablePrivate::setViewportX)
    , vData(this, &QQuickFlickablePrivate::setViewportY)
    , hMoved(false), vMoved(false)
    , stealMouse(false), pressed(false), interactive(true), calcVelocity(false)
    , pixelAligned(false)
    , lastPosTime(-1)
    , lastPressTime(0)
    , deceleration(QML_FLICK_DEFAULTDECELERATION)
    , maxVelocity(QML_FLICK_DEFAULTMAXVELOCITY), reportedVelocitySmoothing(100)
    , delayedPressEvent(0), delayedPressTarget(0), pressDelay(0), fixupDuration(400)
    , fixupMode(Normal), vTime(0), visibleArea(0)
    , flickableDirection(QQuickFlickable::AutoFlickDirection)
    , boundsBehavior(QQuickFlickable::DragAndOvershootBounds)
{
}

void QQuickFlickablePrivate::init()
{
    Q_Q(QQuickFlickable);
    QDeclarative_setParent_noEvent(contentItem, q);
    contentItem->setParentItem(q);
    FAST_CONNECT(&timeline, SIGNAL(updated()), q, SLOT(ticked()))
    FAST_CONNECT(&timeline, SIGNAL(completed()), q, SLOT(movementEnding()))
    q->setAcceptedMouseButtons(Qt::LeftButton);
    q->setFiltersChildMouseEvents(true);
    QQuickItemPrivate *viewportPrivate = QQuickItemPrivate::get(contentItem);
    viewportPrivate->addItemChangeListener(this, QQuickItemPrivate::Geometry);
}

/*
    Returns the amount to overshoot by given a velocity.
    Will be roughly in range 0 - size/4
*/
qreal QQuickFlickablePrivate::overShootDistance(qreal size)
{
    if (maxVelocity <= 0)
        return 0.0;

    return qMin(qreal(QML_FLICK_OVERSHOOT), size/3);
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

void QQuickFlickablePrivate::itemGeometryChanged(QQuickItem *item, const QRectF &newGeom, const QRectF &oldGeom)
{
    Q_Q(QQuickFlickable);
    if (item == contentItem) {
        if (newGeom.x() != oldGeom.x())
            emit q->contentXChanged();
        if (newGeom.y() != oldGeom.y())
            emit q->contentYChanged();
    }
}

void QQuickFlickablePrivate::flickX(qreal velocity)
{
    Q_Q(QQuickFlickable);
    flick(hData, q->minXExtent(), q->maxXExtent(), q->width(), fixupX_callback, velocity);
}

void QQuickFlickablePrivate::flickY(qreal velocity)
{
    Q_Q(QQuickFlickable);
    flick(vData, q->minYExtent(), q->maxYExtent(), q->height(), fixupY_callback, velocity);
}

void QQuickFlickablePrivate::flick(AxisData &data, qreal minExtent, qreal maxExtent, qreal,
                                         QDeclarativeTimeLineCallback::Callback fixupCallback, qreal velocity)
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
    if (maxDistance > 0) {
        qreal v = velocity;
        if (maxVelocity != -1 && maxVelocity < qAbs(v)) {
            if (v < 0)
                v = -maxVelocity;
            else
                v = maxVelocity;
        }
        timeline.reset(data.move);
        if (boundsBehavior == QQuickFlickable::DragAndOvershootBounds)
            timeline.accel(data.move, v, deceleration);
        else
            timeline.accel(data.move, v, deceleration, maxDistance);
        timeline.callback(QDeclarativeTimeLineCallback(&data.move, fixupCallback, this));
        if (!hData.flicking && q->xflick()) {
            hData.flicking = true;
            emit q->flickingChanged();
            emit q->flickingHorizontallyChanged();
            if (!vData.flicking)
                emit q->flickStarted();
        }
        if (!vData.flicking && q->yflick()) {
            vData.flicking = true;
            emit q->flickingChanged();
            emit q->flickingVerticallyChanged();
            if (!hData.flicking)
                emit q->flickStarted();
        }
    } else {
        timeline.reset(data.move);
        fixup(data, minExtent, maxExtent);
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
    fixup(hData, q->minXExtent(), q->maxXExtent());
}

void QQuickFlickablePrivate::fixupY()
{
    Q_Q(QQuickFlickable);
    fixup(vData, q->minYExtent(), q->maxYExtent());
}

void QQuickFlickablePrivate::fixup(AxisData &data, qreal minExtent, qreal maxExtent)
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

void QQuickFlickablePrivate::updateBeginningEnd()
{
    Q_Q(QQuickFlickable);
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

    if (vData.extentsChanged) {
        vData.extentsChanged = false;
        emit q->yOriginChanged();
    }

    if (hData.extentsChanged) {
        hData.extentsChanged = false;
        emit q->xOriginChanged();
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
    \qmlclass Flickable QQuickFlickable
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
    \qmlproperty real QtQuick2::Flickable::contentX
    \qmlproperty real QtQuick2::Flickable::contentY

    These properties hold the surface coordinate currently at the top-left
    corner of the Flickable. For example, if you flick an image up 100 pixels,
    \c contentY will be 100.
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
    d->timeline.reset(d->hData.move);
    d->vTime = d->timeline.time();
    movementXEnding();
    if (-pos != d->hData.move.value()) {
        d->hData.move.setValue(-pos);
        viewportMoved();
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
        if (!interactive && (d->hData.flicking || d->vData.flicking)) {
            d->timeline.clear();
            d->vTime = d->timeline.time();
            d->hData.flicking = false;
            d->vData.flicking = false;
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
    \qmlproperty bool QtQuick2::Flickable::atXBeginning
    \qmlproperty bool QtQuick2::Flickable::atXEnd
    \qmlproperty bool QtQuick2::Flickable::atYBeginning
    \qmlproperty bool QtQuick2::Flickable::atYEnd

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

void QQuickFlickable::ticked()
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
QQuickItem *QQuickFlickable::contentItem()
{
    Q_D(QQuickFlickable);
    return d->contentItem;
}

QQuickFlickableVisibleArea *QQuickFlickable::visibleArea()
{
    Q_D(QQuickFlickable);
    if (!d->visibleArea)
        d->visibleArea = new QQuickFlickableVisibleArea(this);
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

qint64 QQuickFlickablePrivate::computeCurrentTime(QInputEvent *event)
{
    if (0 != event->timestamp() && QQuickItemPrivate::consistentTime == -1)
        return event->timestamp();

    return QQuickItemPrivate::elapsed(timer);
}

void QQuickFlickablePrivate::handleMousePressEvent(QMouseEvent *event)
{
    Q_Q(QQuickFlickable);
    QQuickItemPrivate::start(timer);
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
    lastPos = QPointF();
    lastPosTime = computeCurrentTime(event);
    pressPos = event->localPos();
    hData.pressPos = hData.move.value();
    vData.pressPos = vData.move.value();
    hData.flicking = false;
    vData.flicking = false;
    lastPressTime = computeCurrentTime(event);
    QQuickItemPrivate::start(velocityTime);
}

void QQuickFlickablePrivate::handleMouseMoveEvent(QMouseEvent *event)
{
    Q_Q(QQuickFlickable);
    if (!interactive || lastPosTime == -1)
        return;
    bool rejectY = false;
    bool rejectX = false;

    bool stealY = stealMouse;
    bool stealX = stealMouse;

    qint64 elapsed = computeCurrentTime(event) - lastPressTime;

    if (q->yflick()) {
        qreal dy = event->localPos().y() - pressPos.y();
        if (qAbs(dy) > qApp->styleHints()->startDragDistance() || elapsed > 200) {
            if (!vMoved)
                vData.dragStartOffset = dy;
            qreal newY = dy + vData.pressPos - vData.dragStartOffset;
            const qreal minY = vData.dragMinBound;
            const qreal maxY = vData.dragMaxBound;
            if (newY > minY)
                newY = minY + (newY - minY) / 2;
            if (newY < maxY && maxY - minY <= 0)
                newY = maxY + (newY - maxY) / 2;
            if (boundsBehavior == QQuickFlickable::StopAtBounds && (newY > minY || newY < maxY)) {
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
        qreal dx = event->localPos().x() - pressPos.x();
        if (qAbs(dx) > qApp->styleHints()->startDragDistance() || elapsed > 200) {
            if (!hMoved)
                hData.dragStartOffset = dx;
            qreal newX = dx + hData.pressPos - hData.dragStartOffset;
            const qreal minX = hData.dragMinBound;
            const qreal maxX = hData.dragMaxBound;
            if (newX > minX)
                newX = minX + (newX - minX) / 2;
            if (newX < maxX && maxX - minX <= 0)
                newX = maxX + (newX - maxX) / 2;
            if (boundsBehavior == QQuickFlickable::StopAtBounds && (newX > minX || newX < maxX)) {
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
        qint64 currentTimestamp = computeCurrentTime(event);
        qreal elapsed = qreal(currentTimestamp - lastPosTime) / 1000.;
        if (elapsed <= 0)
            return;
        lastPosTime = currentTimestamp;
        qreal dy = event->localPos().y()-lastPos.y();
        if (q->yflick() && !rejectY)
            vData.addVelocitySample(dy/elapsed, maxVelocity);
        qreal dx = event->localPos().x()-lastPos.x();
        if (q->xflick() && !rejectX)
            hData.addVelocitySample(dx/elapsed, maxVelocity);
    }

    lastPos = event->localPos();
}

void QQuickFlickablePrivate::handleMouseReleaseEvent(QMouseEvent *event)
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

    vTime = timeline.time();

    qreal velocity = elapsed < 100 ? vData.velocity : 0;
    if (vData.atBeginning || vData.atEnd)
        velocity /= 2;
    if (q->yflick() && qAbs(velocity) > MinimumFlickVelocity && qAbs(event->localPos().y() - pressPos.y()) > FlickThreshold) {
        velocityTimeline.reset(vData.smoothVelocity);
        vData.smoothVelocity.setValue(-velocity);
        flickY(velocity);
    } else {
        fixupY();
    }

    velocity = elapsed < 100 ? hData.velocity : 0;
    if (hData.atBeginning || hData.atEnd)
        velocity /= 2;
    if (q->xflick() && qAbs(velocity) > MinimumFlickVelocity && qAbs(event->localPos().x() - pressPos.x()) > FlickThreshold) {
        velocityTimeline.reset(hData.smoothVelocity);
        hData.smoothVelocity.setValue(-velocity);
        flickX(velocity);
    } else {
        fixupX();
    }

    if (!timeline.isActive())
        q->movementEnding();
}

void QQuickFlickable::mousePressEvent(QMouseEvent *event)
{
    Q_D(QQuickFlickable);
    if (d->interactive) {
        if (!d->pressed)
            d->handleMousePressEvent(event);
        event->accept();
    } else {
        QQuickItem::mousePressEvent(event);
    }
}

void QQuickFlickable::mouseMoveEvent(QMouseEvent *event)
{
    Q_D(QQuickFlickable);
    if (d->interactive) {
        d->handleMouseMoveEvent(event);
        event->accept();
    } else {
        QQuickItem::mouseMoveEvent(event);
    }
}

void QQuickFlickable::mouseReleaseEvent(QMouseEvent *event)
{
    Q_D(QQuickFlickable);
    if (d->interactive) {
        d->clearDelayedPress();
        d->handleMouseReleaseEvent(event);
        event->accept();
        ungrabMouse();
    } else {
        QQuickItem::mouseReleaseEvent(event);
    }
}

void QQuickFlickable::wheelEvent(QWheelEvent *event)
{
    Q_D(QQuickFlickable);
    if (!d->interactive) {
        QQuickItem::wheelEvent(event);
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
            d->vData.flicking = false;
            d->flickY(d->vData.velocity);
            if (d->vData.flicking) {
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
            d->hData.flicking = false;
            d->flickX(d->hData.velocity);
            if (d->hData.flicking) {
                d->hMoved = true;
                movementStarting();
            }
            event->accept();
        }
    } else {
        QQuickItem::wheelEvent(event);
    }
}

bool QQuickFlickablePrivate::isOutermostPressDelay() const
{
    Q_Q(const QQuickFlickable);
    QQuickItem *item = q->parentItem();
    while (item) {
        QQuickFlickable *flick = qobject_cast<QQuickFlickable*>(item);
        if (flick && flick->pressDelay() > 0 && flick->isInteractive())
            return false;
        item = item->parentItem();
    }

    return true;
}

void QQuickFlickablePrivate::captureDelayedPress(QMouseEvent *event)
{
    Q_Q(QQuickFlickable);
    if (!q->canvas() || pressDelay <= 0)
        return;
    if (!isOutermostPressDelay())
        return;
    delayedPressTarget = q->canvas()->mouseGrabberItem();
    delayedPressEvent = new QMouseEvent(*event);
    delayedPressEvent->setAccepted(false);
    delayedPressTimer.start(pressDelay, q);
}

void QQuickFlickablePrivate::clearDelayedPress()
{
    if (delayedPressEvent) {
        delayedPressTimer.stop();
        delete delayedPressEvent;
        delayedPressEvent = 0;
    }
}

//XXX pixelAligned ignores the global position of the Flickable, i.e. assumes Flickable itself is pixel aligned.
void QQuickFlickablePrivate::setViewportX(qreal x)
{
    contentItem->setX(pixelAligned ? qRound(x) : x);
}

void QQuickFlickablePrivate::setViewportY(qreal y)
{
    contentItem->setY(pixelAligned ? qRound(y) : y);
}

void QQuickFlickable::timerEvent(QTimerEvent *event)
{
    Q_D(QQuickFlickable);
    if (event->timerId() == d->delayedPressTimer.timerId()) {
        d->delayedPressTimer.stop();
        if (d->delayedPressEvent) {
            QQuickItem *grabber = canvas() ? canvas()->mouseGrabberItem() : 0;
            if (!grabber || grabber != this) {
                // We replay the mouse press but the grabber we had might not be interessted by the event (e.g. overlay)
                // so we reset the grabber
                if (canvas()->mouseGrabberItem() == d->delayedPressTarget)
                    d->delayedPressTarget->ungrabMouse();
                // Use the event handler that will take care of finding the proper item to propagate the event
                QQuickCanvasPrivate::get(canvas())->deliverMouseEvent(d->delayedPressEvent);
            }
            delete d->delayedPressEvent;
            d->delayedPressEvent = 0;
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
    return width() - vWidth() - d->hData.endMargin;
}
/* returns -ve */
qreal QQuickFlickable::maxYExtent() const
{
    Q_D(const QQuickFlickable);
    return height() - vHeight() - d->vData.endMargin;
}

void QQuickFlickable::componentComplete()
{
    Q_D(QQuickFlickable);
    QQuickItem::componentComplete();
    if (!d->hData.explicitValue && d->hData.startMargin != 0.)
        setContentX(-minXExtent());
    if (!d->vData.explicitValue && d->vData.startMargin != 0.)
        setContentY(-minYExtent());
}

void QQuickFlickable::viewportMoved()
{
    Q_D(QQuickFlickable);

    qreal prevX = d->lastFlickablePosition.x();
    qreal prevY = d->lastFlickablePosition.y();
    if (d->pressed || d->calcVelocity) {
        int elapsed = QQuickItemPrivate::restart(d->velocityTime);
        if (elapsed > 0) {
            qreal horizontalVelocity = (prevX - d->hData.move.value()) * 1000 / elapsed;
            if (qAbs(horizontalVelocity) > 0) {
                d->velocityTimeline.reset(d->hData.smoothVelocity);
                if (d->calcVelocity)
                    d->velocityTimeline.set(d->hData.smoothVelocity, horizontalVelocity);
                else
                    d->velocityTimeline.move(d->hData.smoothVelocity, horizontalVelocity, d->reportedVelocitySmoothing);
                d->velocityTimeline.move(d->hData.smoothVelocity, 0, d->reportedVelocitySmoothing);
            }
            qreal verticalVelocity = (prevY - d->vData.move.value()) * 1000 / elapsed;
            if (qAbs(verticalVelocity) > 0) {
                d->velocityTimeline.reset(d->vData.smoothVelocity);
                if (d->calcVelocity)
                    d->velocityTimeline.set(d->vData.smoothVelocity, verticalVelocity);
                else
                    d->velocityTimeline.move(d->vData.smoothVelocity, verticalVelocity, d->reportedVelocitySmoothing);
                d->velocityTimeline.move(d->vData.smoothVelocity, 0, d->reportedVelocitySmoothing);
            }
        }
    } else {
        if (d->timeline.time() > d->vTime) {
            d->velocityTimeline.clear();
            qreal horizontalVelocity = (prevX - d->hData.move.value()) * 1000 / (d->timeline.time() - d->vTime);
            qreal verticalVelocity = (prevY - d->vData.move.value()) * 1000 / (d->timeline.time() - d->vTime);
            d->hData.smoothVelocity.setValue(horizontalVelocity);
            d->vData.smoothVelocity.setValue(verticalVelocity);
        }
    }

    if (!d->vData.inOvershoot && !d->vData.fixingUp && d->vData.flicking
            && (d->vData.move.value() > minYExtent() || d->vData.move.value() < maxYExtent())
            && qAbs(d->vData.smoothVelocity.value()) > 100) {
        // Increase deceleration if we've passed a bound
        d->vData.inOvershoot = true;
        qreal maxDistance = d->overShootDistance(height());
        d->timeline.reset(d->vData.move);
        d->timeline.accel(d->vData.move, -d->vData.smoothVelocity.value(), d->deceleration*QML_FLICK_OVERSHOOTFRICTION, maxDistance);
        d->timeline.callback(QDeclarativeTimeLineCallback(&d->vData.move, d->fixupY_callback, d));
    }
    if (!d->hData.inOvershoot && !d->hData.fixingUp && d->hData.flicking
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

void QQuickFlickable::geometryChanged(const QRectF &newGeometry,
                             const QRectF &oldGeometry)
{
    Q_D(QQuickFlickable);
    QQuickItem::geometryChanged(newGeometry, oldGeometry);

    bool changed = false;
    if (newGeometry.width() != oldGeometry.width()) {
        if (xflick())
            changed = true;
        if (d->hData.viewSize < 0) {
            d->contentItem->setWidth(width());
            emit contentWidthChanged();
        }
        // Make sure that we're entirely in view.
        if (!d->pressed && !d->hData.moving && !d->vData.moving) {
            d->fixupMode = QQuickFlickablePrivate::Immediate;
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
        if (!d->pressed && !d->hData.moving && !d->vData.moving) {
            d->fixupMode = QQuickFlickablePrivate::Immediate;
            d->fixupY();
        }
    }

    if (changed)
        d->updateBeginningEnd();
}

void QQuickFlickable::cancelFlick()
{
    Q_D(QQuickFlickable);
    d->timeline.reset(d->hData.move);
    d->timeline.reset(d->vData.move);
    movementEnding();
}

void QQuickFlickablePrivate::data_append(QDeclarativeListProperty<QObject> *prop, QObject *o)
{
    QQuickItem *i = qobject_cast<QQuickItem *>(o);
    if (i) {
        i->setParentItem(static_cast<QQuickFlickablePrivate*>(prop->data)->contentItem);
    } else {
        o->setParent(prop->object); // XXX todo - do we want this?
    }
}

int QQuickFlickablePrivate::data_count(QDeclarativeListProperty<QObject> *)
{
    // XXX todo
    return 0;
}

QObject *QQuickFlickablePrivate::data_at(QDeclarativeListProperty<QObject> *, int)
{
    // XXX todo
    return 0;
}

void QQuickFlickablePrivate::data_clear(QDeclarativeListProperty<QObject> *)
{
    // XXX todo
}

QDeclarativeListProperty<QObject> QQuickFlickable::flickableData()
{
    Q_D(QQuickFlickable);
    return QDeclarativeListProperty<QObject>(this, (void *)d, QQuickFlickablePrivate::data_append,
                                             QQuickFlickablePrivate::data_count,
                                             QQuickFlickablePrivate::data_at,
                                             QQuickFlickablePrivate::data_clear);
}

QDeclarativeListProperty<QQuickItem> QQuickFlickable::flickableChildren()
{
    Q_D(QQuickFlickable);
    return QQuickItemPrivate::get(d->contentItem)->children();
}

/*!
    \qmlproperty enumeration QtQuick2::Flickable::boundsBehavior
    This property holds whether the surface may be dragged
    beyond the Flickable's boundaries, or overshoot the
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
        d->contentItem->setWidth(width());
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
        d->contentItem->setHeight(height());
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
    \qmlproperty real QtQuick2::Flickable::topMargin
    \qmlproperty real QtQuick2::Flickable::leftMargin
    \qmlproperty real QtQuick2::Flickable::bottomMargin
    \qmlproperty real QtQuick2::Flickable::rightMargin

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
    \qmlproperty real QtQuick2::Flickable::xOrigin
    \qmlproperty real QtQuick2::Flickable::yOrigin

    These properties hold the origin of the content.  This is usually (0,0), however
    ListView and GridView may have an arbitrary origin due to delegate size variation,
    or item insertion/removal outside the visible region.
*/

qreal QQuickFlickable::yOrigin() const
{
    Q_D(const QQuickFlickable);
    return -minYExtent() + d->vData.startMargin;
}

qreal QQuickFlickable::xOrigin() const
{
    Q_D(const QQuickFlickable);
    return -minXExtent() + d->hData.startMargin;
}


/*!
    \qmlmethod QtQuick2::Flickable::resizeContent(real width, real height, QPointF center)

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

bool QQuickFlickable::xflick() const
{
    Q_D(const QQuickFlickable);
    if (d->flickableDirection == QQuickFlickable::AutoFlickDirection)
        return vWidth() != width();
    return d->flickableDirection & QQuickFlickable::HorizontalFlick;
}

bool QQuickFlickable::yflick() const
{
    Q_D(const QQuickFlickable);
    if (d->flickableDirection == QQuickFlickable::AutoFlickDirection)
        return vHeight() !=  height();
    return d->flickableDirection & QQuickFlickable::VerticalFlick;
}

void QQuickFlickable::mouseUngrabEvent()
{
    Q_D(QQuickFlickable);
    if (d->pressed) {
        // if our mouse grab has been removed (probably by another Flickable),
        // fix our state
        d->pressed = false;
        d->draggingEnding();
        d->stealMouse = false;
        setKeepMouseGrab(false);
    }
}

bool QQuickFlickable::sendMouseEvent(QMouseEvent *event)
{
    Q_D(QQuickFlickable);
    QRectF myRect = mapRectToScene(QRectF(0, 0, width(), height()));

    QQuickCanvas *c = canvas();
    QQuickItem *grabber = c ? c->mouseGrabberItem() : 0;
    bool disabledItem = grabber && !grabber->isEnabled();
    bool stealThisEvent = d->stealMouse;
    if ((stealThisEvent || myRect.contains(event->windowPos())) && (!grabber || !grabber->keepMouseGrab() || disabledItem)) {
        QMouseEvent mouseEvent(event->type(), mapFromScene(event->windowPos()), event->windowPos(), event->screenPos(),
                               event->button(), event->buttons(), event->modifiers());

        mouseEvent.setTimestamp(event->timestamp());
        mouseEvent.setAccepted(false);

        switch (mouseEvent.type()) {
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
                QQuickCanvasPrivate::get(canvas())->deliverMouseEvent(d->delayedPressEvent);
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
        grabber = qobject_cast<QQuickItem*>(c->mouseGrabberItem());
        if ((grabber && stealThisEvent && !grabber->keepMouseGrab() && grabber != this) || disabledItem) {
            d->clearDelayedPress();
            grabMouse();
        }

        return stealThisEvent || d->delayedPressEvent || disabledItem;
    } else if (d->lastPosTime != -1) {
        d->lastPosTime = -1;
        returnToBounds();
    }
    if (event->type() == QEvent::MouseButtonRelease) {
        d->lastPosTime = -1;
        d->clearDelayedPress();
        d->stealMouse = false;
        d->pressed = false;
    }
    return false;
}


bool QQuickFlickable::childMouseEventFilter(QQuickItem *i, QEvent *e)
{
    Q_D(QQuickFlickable);
    if (!isVisible() || !d->interactive || !isEnabled())
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

/*!
    \qmlproperty real QtQuick2::Flickable::maximumFlickVelocity
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
    \qmlproperty real QtQuick2::Flickable::flickDeceleration
    This property holds the rate at which a flick will decelerate.

    The default value is platform dependent.
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
    d->deceleration = deceleration;
    emit flickDecelerationChanged();
}

bool QQuickFlickable::isFlicking() const
{
    Q_D(const QQuickFlickable);
    return d->hData.flicking ||  d->vData.flicking;
}

/*!
    \qmlproperty bool QtQuick2::Flickable::flicking
    \qmlproperty bool QtQuick2::Flickable::flickingHorizontally
    \qmlproperty bool QtQuick2::Flickable::flickingVertically

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
    \qmlproperty bool QtQuick2::Flickable::dragging
    \qmlproperty bool QtQuick2::Flickable::draggingHorizontally
    \qmlproperty bool QtQuick2::Flickable::draggingVertically

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
    \qmlproperty bool QtQuick2::Flickable::moving
    \qmlproperty bool QtQuick2::Flickable::movingHorizontally
    \qmlproperty bool QtQuick2::Flickable::movingVertically

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

void QQuickFlickable::movementStarting()
{
    Q_D(QQuickFlickable);
    if (d->hMoved && !d->hData.moving) {
        d->hData.moving = true;
        emit movingChanged();
        emit movingHorizontallyChanged();
        if (!d->vData.moving)
            emit movementStarted();
    }
    else if (d->vMoved && !d->vData.moving) {
        d->vData.moving = true;
        emit movingChanged();
        emit movingVerticallyChanged();
        if (!d->hData.moving)
            emit movementStarted();
    }
}

void QQuickFlickable::movementEnding()
{
    Q_D(QQuickFlickable);
    movementXEnding();
    movementYEnding();
    d->hData.smoothVelocity.setValue(0);
    d->vData.smoothVelocity.setValue(0);
}

void QQuickFlickable::movementXEnding()
{
    Q_D(QQuickFlickable);
    if (d->hData.flicking) {
        d->hData.flicking = false;
        emit flickingChanged();
        emit flickingHorizontallyChanged();
        if (!d->vData.flicking)
           emit flickEnded();
    }
    if (!d->pressed && !d->stealMouse) {
        if (d->hData.moving) {
            d->hData.moving = false;
            d->hMoved = false;
            emit movingChanged();
            emit movingHorizontallyChanged();
            if (!d->vData.moving)
                emit movementEnded();
        }
    }
    d->hData.fixingUp = false;
}

void QQuickFlickable::movementYEnding()
{
    Q_D(QQuickFlickable);
    if (d->vData.flicking) {
        d->vData.flicking = false;
        emit flickingChanged();
        emit flickingVerticallyChanged();
        if (!d->hData.flicking)
           emit flickEnded();
    }
    if (!d->pressed && !d->stealMouse) {
        if (d->vData.moving) {
            d->vData.moving = false;
            d->vMoved = false;
            emit movingChanged();
            emit movingVerticallyChanged();
            if (!d->hData.moving)
                emit movementEnded();
        }
    }
    d->vData.fixingUp = false;
}

void QQuickFlickablePrivate::updateVelocity()
{
    Q_Q(QQuickFlickable);
    emit q->horizontalVelocityChanged();
    emit q->verticalVelocityChanged();
}

QT_END_NAMESPACE
