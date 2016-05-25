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

#include "qquickscrollbar_p.h"
#include "qquickcontrol_p_p.h"

#include <QtQml/qqmlinfo.h>
#include <QtQuick/private/qquickflickable_p.h>
#include <QtQuick/private/qquickitemchangelistener_p.h>

QT_BEGIN_NAMESPACE

/*!
    \qmltype ScrollBar
    \inherits Control
    \instantiates QQuickScrollBar
    \inqmlmodule QtQuick.Controls
    \since 5.7
    \ingroup qtquickcontrols2-indicators
    \brief An interactive scroll bar control.

    ScrollBar is an interactive bar that can be used to scroll to a specific
    position. A scroll bar can be either \l vertical or \l horizontal, and can
    be attached to any \l Flickable, such as \l ListView and \l GridView.

    \image qtquickcontrols2-scrollbar.png

    \code
    Flickable {
        // ...
        ScrollBar.vertical: ScrollBar { }
    }
    \endcode

    \note When ScrollBar is attached \l {ScrollBar::vertical}{vertically} or
    \l {ScrollBar::horizontal}{horizontally} to a Flickable, its geometry and
    the following properties are automatically set and updated as appropriate:
    \list
    \li \l orientation
    \li \l position
    \li \l size
    \li \l active
    \endlist

    Notice that ScrollBar does not filter key events of the Flickable it is
    attached to. The following example illustrates how to implement scrolling
    with up and down keys:

    \code
    Flickable {
        focus: true

        Keys.onUpPressed: scrollBar.decrease()
        Keys.onDownPressed: scrollBar.increase()

        ScrollBar.vertical: ScrollBar { id: scrollBar }
    }
    \endcode

    Horizontal and vertical scroll bars do not share the \l active state with
    each other by default. In order to keep both bars visible whilst scrolling
    to either direction, establish a two-way binding between the active states
    as presented by the following example:

    \snippet qtquickcontrols2-scrollbar-active.qml 1

    \sa ScrollIndicator, {Customizing ScrollBar}, {Indicator Controls}
*/

class QQuickScrollBarPrivate : public QQuickControlPrivate
{
    Q_DECLARE_PUBLIC(QQuickScrollBar)

public:
    QQuickScrollBarPrivate() : size(0), position(0), stepSize(0), offset(0),
        active(false), pressed(false), moving(false),
        orientation(Qt::Vertical)
    {
    }

    static QQuickScrollBarPrivate *get(QQuickScrollBar *bar)
    {
        return bar->d_func();
    }

    qreal positionAt(const QPoint &point) const;

    void resizeContent() override;

    qreal size;
    qreal position;
    qreal stepSize;
    qreal offset;
    bool active;
    bool pressed;
    bool moving;
    Qt::Orientation orientation;
};

qreal QQuickScrollBarPrivate::positionAt(const QPoint &point) const
{
    Q_Q(const QQuickScrollBar);
    if (orientation == Qt::Horizontal)
        return (point.x() - q->leftPadding()) / q->availableWidth();
    else
        return (point.y() - q->topPadding()) / q->availableHeight();
}

void QQuickScrollBarPrivate::resizeContent()
{
    Q_Q(QQuickScrollBar);
    if (!contentItem)
        return;

    if (orientation == Qt::Horizontal) {
        contentItem->setPosition(QPointF(q->leftPadding() + position * q->availableWidth(), q->topPadding()));
        contentItem->setSize(QSizeF(q->availableWidth() * size, q->availableHeight()));
    } else {
        contentItem->setPosition(QPointF(q->leftPadding(), q->topPadding() + position * q->availableHeight()));
        contentItem->setSize(QSizeF(q->availableWidth(), q->availableHeight() * size));
    }
}

QQuickScrollBar::QQuickScrollBar(QQuickItem *parent) :
    QQuickControl(*(new QQuickScrollBarPrivate), parent)
{
    setKeepMouseGrab(true);
    setAcceptedMouseButtons(Qt::LeftButton);
}

QQuickScrollBarAttached *QQuickScrollBar::qmlAttachedProperties(QObject *object)
{
    QQuickFlickable *flickable = qobject_cast<QQuickFlickable *>(object);
    if (!flickable)
        qmlInfo(object) << "ScrollBar must be attached to a Flickable";

    return new QQuickScrollBarAttached(flickable);
}

/*!
    \qmlproperty real QtQuick.Controls::ScrollBar::size

    This property holds the size of the scroll bar, scaled to \c {0.0 - 1.0}.

    \sa {Flickable::visibleArea.heightRatio}{Flickable::visibleArea}
*/
qreal QQuickScrollBar::size() const
{
    Q_D(const QQuickScrollBar);
    return d->size;
}

void QQuickScrollBar::setSize(qreal size)
{
    Q_D(QQuickScrollBar);
    size = qBound<qreal>(0.0, size, 1.0 - d->position);
    if (qFuzzyCompare(d->size, size))
        return;

    d->size = size;
    if (isComponentComplete())
        d->resizeContent();
    emit sizeChanged();
}

/*!
    \qmlproperty real QtQuick.Controls::ScrollBar::position

    This property holds the position of the scroll bar, scaled to \c {0.0 - 1.0}.

    \sa {Flickable::visibleArea.yPosition}{Flickable::visibleArea}
*/
qreal QQuickScrollBar::position() const
{
    Q_D(const QQuickScrollBar);
    return d->position;
}

void QQuickScrollBar::setPosition(qreal position)
{
    Q_D(QQuickScrollBar);
    position = qBound<qreal>(0.0, position, 1.0 - d->size);
    if (qFuzzyCompare(d->position, position))
        return;

    d->position = position;
    if (isComponentComplete())
        d->resizeContent();
    emit positionChanged();
}

/*!
    \qmlproperty real QtQuick.Controls::ScrollBar::stepSize

    This property holds the step size. The default value is \c 0.0.

    \sa increase(), decrease()
*/
qreal QQuickScrollBar::stepSize() const
{
    Q_D(const QQuickScrollBar);
    return d->stepSize;
}

void QQuickScrollBar::setStepSize(qreal step)
{
    Q_D(QQuickScrollBar);
    if (qFuzzyCompare(d->stepSize, step))
        return;

    d->stepSize = step;
    emit stepSizeChanged();
}

/*!
    \qmlproperty bool QtQuick.Controls::ScrollBar::active

    This property holds whether the scroll bar is active, i.e. when it's \l pressed
    or the attached Flickable is \l {Flickable::moving}{moving}.
*/
bool QQuickScrollBar::isActive() const
{
    Q_D(const QQuickScrollBar);
    return d->active;
}

void QQuickScrollBar::setActive(bool active)
{
    Q_D(QQuickScrollBar);
    if (d->active == active)
        return;

    d->active = active;
    emit activeChanged();
}

/*!
    \qmlproperty bool QtQuick.Controls::ScrollBar::pressed

    This property holds whether the scroll bar is pressed.
*/
bool QQuickScrollBar::isPressed() const
{
    Q_D(const QQuickScrollBar);
    return d->pressed;
}

void QQuickScrollBar::setPressed(bool pressed)
{
    Q_D(QQuickScrollBar);
    if (d->pressed == pressed)
        return;

    d->pressed = pressed;
    setAccessibleProperty("pressed", pressed);
    setActive(d->pressed || d->moving);
    emit pressedChanged();
}

/*!
    \qmlproperty enumeration QtQuick.Controls::ScrollBar::orientation

    This property holds the orientation of the scroll bar.

    Possible values:
    \value Qt.Horizontal Horizontal
    \value Qt.Vertical Vertical (default)
*/
Qt::Orientation QQuickScrollBar::orientation() const
{
    Q_D(const QQuickScrollBar);
    return d->orientation;
}

void QQuickScrollBar::setOrientation(Qt::Orientation orientation)
{
    Q_D(QQuickScrollBar);
    if (d->orientation == orientation)
        return;

    d->orientation = orientation;
    if (isComponentComplete())
        d->resizeContent();
    emit orientationChanged();
}

/*!
    \qmlmethod void QtQuick.Controls::ScrollBar::increase()

    Increases the position by \l stepSize or \c 0.1 if stepSize is \c 0.0.

    \sa stepSize
*/
void QQuickScrollBar::increase()
{
    Q_D(QQuickScrollBar);
    qreal step = qFuzzyIsNull(d->stepSize) ? 0.1 : d->stepSize;
    setActive(true);
    setPosition(d->position + step);
    setActive(false);
}

/*!
    \qmlmethod void QtQuick.Controls::ScrollBar::decrease()

    Decreases the position by \l stepSize or \c 0.1 if stepSize is \c 0.0.

    \sa stepSize
*/
void QQuickScrollBar::decrease()
{
    Q_D(QQuickScrollBar);
    qreal step = qFuzzyIsNull(d->stepSize) ? 0.1 : d->stepSize;
    setActive(true);
    setPosition(d->position - step);
    setActive(false);
}

void QQuickScrollBar::mousePressEvent(QMouseEvent *event)
{
    Q_D(QQuickScrollBar);
    QQuickControl::mousePressEvent(event);
    d->offset = d->positionAt(event->pos()) - d->position;
    if (d->offset < 0 || d->offset > d->size)
        d->offset = d->size / 2;
    setPressed(true);
}

void QQuickScrollBar::mouseMoveEvent(QMouseEvent *event)
{
    Q_D(QQuickScrollBar);
    QQuickControl::mouseMoveEvent(event);
    setPosition(qBound<qreal>(0.0, d->positionAt(event->pos()) - d->offset, 1.0 - d->size));
}

void QQuickScrollBar::mouseReleaseEvent(QMouseEvent *event)
{
    Q_D(QQuickScrollBar);
    QQuickControl::mouseReleaseEvent(event);
    setPosition(qBound<qreal>(0.0, d->positionAt(event->pos()) - d->offset, 1.0 - d->size));
    d->offset = 0.0;
    setPressed(false);
}

#ifndef QT_NO_ACCESSIBILITY
void QQuickScrollBar::accessibilityActiveChanged(bool active)
{
    QQuickControl::accessibilityActiveChanged(active);

    Q_D(QQuickScrollBar);
    if (active)
        setAccessibleProperty("pressed", d->pressed);
}

QAccessible::Role QQuickScrollBar::accessibleRole() const
{
    return QAccessible::ScrollBar;
}
#endif

class QQuickScrollBarAttachedPrivate : public QObjectPrivate, public QQuickItemChangeListener
{
public:
    QQuickScrollBarAttachedPrivate(QQuickFlickable *flickable) : flickable(flickable), horizontal(nullptr), vertical(nullptr) { }

    void activateHorizontal();
    void activateVertical();
    void scrollHorizontal();
    void scrollVertical();

    void layoutHorizontal(bool move = true);
    void layoutVertical(bool move = true);

    void itemGeometryChanged(QQuickItem *item, const QRectF &newGeometry, const QRectF &oldGeometry) override;

    QQuickFlickable *flickable;
    QQuickScrollBar *horizontal;
    QQuickScrollBar *vertical;
};

void QQuickScrollBarAttachedPrivate::activateHorizontal()
{
    QQuickScrollBarPrivate *p = QQuickScrollBarPrivate::get(horizontal);
    p->moving = flickable->isMovingHorizontally();
    horizontal->setActive(p->moving || p->pressed);
}

void QQuickScrollBarAttachedPrivate::activateVertical()
{
    QQuickScrollBarPrivate *p = QQuickScrollBarPrivate::get(vertical);
    p->moving = flickable->isMovingVertically();
    vertical->setActive(p->moving || p->pressed);
}

// TODO: QQuickFlickable::maxXYExtent()
class QQuickFriendlyFlickable : public QQuickFlickable
{
    friend class QQuickScrollBarAttachedPrivate;
};

void QQuickScrollBarAttachedPrivate::scrollHorizontal()
{
    QQuickFriendlyFlickable *f = reinterpret_cast<QQuickFriendlyFlickable *>(flickable);

    const qreal viewwidth = f->width();
    const qreal maxxextent = -f->maxXExtent() + f->minXExtent();
    qreal cx = horizontal->position() * (maxxextent + viewwidth) - f->minXExtent();
    if (!qIsNaN(cx) && !qFuzzyCompare(cx, flickable->contentX()))
        flickable->setContentX(cx);
}

void QQuickScrollBarAttachedPrivate::scrollVertical()
{
    QQuickFriendlyFlickable *f = reinterpret_cast<QQuickFriendlyFlickable *>(flickable);

    const qreal viewheight = f->height();
    const qreal maxyextent = -f->maxYExtent() + f->minYExtent();
    qreal cy = vertical->position() * (maxyextent + viewheight) - f->minYExtent();
    if (!qIsNaN(cy) && !qFuzzyCompare(cy, flickable->contentY()))
        flickable->setContentY(cy);
}

void QQuickScrollBarAttachedPrivate::layoutHorizontal(bool move)
{
    Q_ASSERT(horizontal && flickable);
    horizontal->setWidth(flickable->width());
    if (move)
        horizontal->setY(flickable->height() - horizontal->height());
}

void QQuickScrollBarAttachedPrivate::layoutVertical(bool move)
{
    Q_ASSERT(vertical && flickable);
    vertical->setHeight(flickable->height());
    if (move && !QQuickItemPrivate::get(vertical)->isMirrored())
        vertical->setX(flickable->width() - vertical->width());
}

void QQuickScrollBarAttachedPrivate::itemGeometryChanged(QQuickItem *item, const QRectF &newGeometry, const QRectF &oldGeometry)
{
    Q_UNUSED(item);
    Q_UNUSED(newGeometry);
    if (horizontal && horizontal->height() > 0) {
        bool move = qFuzzyIsNull(horizontal->y()) || qFuzzyCompare(horizontal->y(), oldGeometry.height() - horizontal->height());
        layoutHorizontal(move);
    }
    if (vertical && vertical->width() > 0) {
        bool move = qFuzzyIsNull(vertical->x()) || qFuzzyCompare(vertical->x(), oldGeometry.width() - vertical->width());
        layoutVertical(move);
    }
}

QQuickScrollBarAttached::QQuickScrollBarAttached(QQuickFlickable *flickable) :
    QObject(*(new QQuickScrollBarAttachedPrivate(flickable)), flickable)
{
    Q_D(QQuickScrollBarAttached);
    if (flickable) {
        QQuickItemPrivate *p = QQuickItemPrivate::get(flickable);
        p->updateOrAddGeometryChangeListener(d, QQuickItemPrivate::SizeChange);
    }
}

QQuickScrollBarAttached::~QQuickScrollBarAttached()
{
    Q_D(QQuickScrollBarAttached);
    if (d->flickable) {
        if (d->horizontal)
            QQuickItemPrivate::get(d->horizontal)->removeItemChangeListener(d, QQuickItemPrivate::Geometry);
        if (d->vertical)
            QQuickItemPrivate::get(d->vertical)->removeItemChangeListener(d, QQuickItemPrivate::Geometry);
    }
}

/*!
    \qmlattachedproperty ScrollBar QtQuick.Controls::ScrollBar::horizontal

    This property attaches a horizontal scroll bar to a \l Flickable.

    \code
    Flickable {
        contentWidth: 2000
        ScrollBar.horizontal: ScrollBar { }
    }
    \endcode
*/
QQuickScrollBar *QQuickScrollBarAttached::horizontal() const
{
    Q_D(const QQuickScrollBarAttached);
    return d->horizontal;
}

void QQuickScrollBarAttached::setHorizontal(QQuickScrollBar *horizontal)
{
    Q_D(QQuickScrollBarAttached);
    if (d->horizontal == horizontal)
        return;

    if (d->horizontal && d->flickable) {
        QQuickItemPrivate::get(d->horizontal)->removeItemChangeListener(d, QQuickItemPrivate::Geometry);
        QObjectPrivate::disconnect(d->horizontal, &QQuickScrollBar::positionChanged, d, &QQuickScrollBarAttachedPrivate::scrollHorizontal);
        QObjectPrivate::disconnect(d->flickable, &QQuickFlickable::movingHorizontallyChanged, d, &QQuickScrollBarAttachedPrivate::activateHorizontal);

        // TODO: export QQuickFlickableVisibleArea
        QObject *area = d->flickable->property("visibleArea").value<QObject *>();
        disconnect(area, SIGNAL(widthRatioChanged(qreal)), d->horizontal, SLOT(setSize(qreal)));
        disconnect(area, SIGNAL(xPositionChanged(qreal)), d->horizontal, SLOT(setPosition(qreal)));
    }

    d->horizontal = horizontal;

    if (horizontal && d->flickable) {
        if (!horizontal->parentItem())
            horizontal->setParentItem(d->flickable);
        horizontal->setOrientation(Qt::Horizontal);

        QQuickItemPrivate::get(horizontal)->updateOrAddGeometryChangeListener(d, QQuickItemPrivate::SizeChange);
        QObjectPrivate::connect(horizontal, &QQuickScrollBar::positionChanged, d, &QQuickScrollBarAttachedPrivate::scrollHorizontal);
        QObjectPrivate::connect(d->flickable, &QQuickFlickable::movingHorizontallyChanged, d, &QQuickScrollBarAttachedPrivate::activateHorizontal);

        // TODO: export QQuickFlickableVisibleArea
        QObject *area = d->flickable->property("visibleArea").value<QObject *>();
        connect(area, SIGNAL(widthRatioChanged(qreal)), horizontal, SLOT(setSize(qreal)));
        connect(area, SIGNAL(xPositionChanged(qreal)), horizontal, SLOT(setPosition(qreal)));

        d->layoutHorizontal();
        horizontal->setSize(area->property("widthRatio").toReal());
        horizontal->setPosition(area->property("xPosition").toReal());
    }
    emit horizontalChanged();
}

/*!
    \qmlattachedproperty ScrollBar QtQuick.Controls::ScrollBar::vertical

    This property attaches a vertical scroll bar to a \l Flickable.

    \code
    Flickable {
        contentHeight: 2000
        ScrollBar.vertical: ScrollBar { }
    }
    \endcode
*/
QQuickScrollBar *QQuickScrollBarAttached::vertical() const
{
    Q_D(const QQuickScrollBarAttached);
    return d->vertical;
}

void QQuickScrollBarAttached::setVertical(QQuickScrollBar *vertical)
{
    Q_D(QQuickScrollBarAttached);
    if (d->vertical == vertical)
        return;

    if (d->vertical && d->flickable) {
        QQuickItemPrivate::get(d->vertical)->removeItemChangeListener(d, QQuickItemPrivate::Geometry);
        QObjectPrivate::disconnect(d->vertical, &QQuickScrollBar::positionChanged, d, &QQuickScrollBarAttachedPrivate::scrollVertical);
        QObjectPrivate::disconnect(d->flickable, &QQuickFlickable::movingVerticallyChanged, d, &QQuickScrollBarAttachedPrivate::activateVertical);

        // TODO: export QQuickFlickableVisibleArea
        QObject *area = d->flickable->property("visibleArea").value<QObject *>();
        disconnect(area, SIGNAL(heightRatioChanged(qreal)), d->vertical, SLOT(setSize(qreal)));
        disconnect(area, SIGNAL(yPositionChanged(qreal)), d->vertical, SLOT(setPosition(qreal)));
    }

    d->vertical = vertical;

    if (vertical && d->flickable) {
        if (!vertical->parentItem())
            vertical->setParentItem(d->flickable);
        vertical->setOrientation(Qt::Vertical);

        QQuickItemPrivate::get(vertical)->updateOrAddGeometryChangeListener(d, QQuickItemPrivate::SizeChange);
        QObjectPrivate::connect(vertical, &QQuickScrollBar::positionChanged, d, &QQuickScrollBarAttachedPrivate::scrollVertical);
        QObjectPrivate::connect(d->flickable, &QQuickFlickable::movingVerticallyChanged, d, &QQuickScrollBarAttachedPrivate::activateVertical);

        // TODO: export QQuickFlickableVisibleArea
        QObject *area = d->flickable->property("visibleArea").value<QObject *>();
        connect(area, SIGNAL(heightRatioChanged(qreal)), vertical, SLOT(setSize(qreal)));
        connect(area, SIGNAL(yPositionChanged(qreal)), vertical, SLOT(setPosition(qreal)));

        d->layoutVertical();
        vertical->setSize(area->property("heightRatio").toReal());
        vertical->setPosition(area->property("yPosition").toReal());
    }
    emit verticalChanged();
}

QT_END_NAMESPACE
