/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Qt Labs Templates module of the Qt Toolkit.
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

#include <QtQuick/private/qquickflickable_p.h>
#include <QtQuick/private/qquickitemchangelistener_p.h>

QT_BEGIN_NAMESPACE

/*!
    \qmltype ScrollBar
    \inherits Control
    \instantiates QQuickScrollBar
    \inqmlmodule Qt.labs.controls
    \ingroup qtlabscontrols-indicators
    \brief An interactive scroll bar control.

    ScrollBar is an interactive bar that can be used to scroll to a specific
    position. A scroll bar can be either \l vertical or \l horizontal, and can
    be attached to any \l Flickable, such as \l ListView and \l GridView.

    \image qtlabscontrols-scrollbar.png

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

    \labs

    \sa ScrollIndicator, {Customizing ScrollBar}, {Indicator Controls}
*/

class QQuickScrollBarPrivate : public QQuickControlPrivate
{
public:
    QQuickScrollBarPrivate() : size(0), position(0), offset(0),
        active(false), pressed(false), moving(false),
        orientation(Qt::Vertical), handle(Q_NULLPTR)
    {
    }

    static QQuickScrollBarPrivate *get(QQuickScrollBar *bar)
    {
        return bar->d_func();
    }

    qreal size;
    qreal position;
    qreal offset;
    bool active;
    bool pressed;
    bool moving;
    Qt::Orientation orientation;
    QQuickItem *handle;
};

QQuickScrollBar::QQuickScrollBar(QQuickItem *parent) :
    QQuickControl(*(new QQuickScrollBarPrivate), parent)
{
    setKeepMouseGrab(true);
    setAcceptedMouseButtons(Qt::LeftButton);
}

QQuickScrollBarAttached *QQuickScrollBar::qmlAttachedProperties(QObject *object)
{
    QQuickFlickable *flickable = qobject_cast<QQuickFlickable *>(object);
    if (flickable)
        return new QQuickScrollBarAttached(flickable);

    qWarning() << "ScrollBar must be attached to a Flickable" << object;
    return Q_NULLPTR;
}

/*!
    \qmlproperty real Qt.labs.controls::ScrollBar::size

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
    if (!qFuzzyCompare(d->size, size)) {
        d->size = size;
        emit sizeChanged();
    }
}

/*!
    \qmlproperty real Qt.labs.controls::ScrollBar::position

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
    if (!qFuzzyCompare(d->position, position)) {
        d->position = position;
        emit positionChanged();
    }
}

/*!
    \qmlproperty bool Qt.labs.controls::ScrollBar::active

    This property holds whether the scroll bar is active ie. when its \l pressed
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
    if (d->active != active) {
        d->active = active;
        emit activeChanged();
    }
}

/*!
    \qmlproperty bool Qt.labs.controls::ScrollBar::pressed

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
    if (d->pressed != pressed) {
        d->pressed = pressed;
        setAccessibleProperty("pressed", pressed);
        setActive(d->pressed || d->moving);
        emit pressedChanged();
    }
}

/*!
    \qmlproperty enumeration Qt.labs.controls::ScrollBar::orientation

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
    if (d->orientation != orientation) {
        d->orientation = orientation;
        emit orientationChanged();
    }
}

/*!
    \qmlproperty Item Qt.labs.controls::ScrollBar::handle

    This property holds the handle item.

    \sa {Customizing ScrollBar}
*/
QQuickItem *QQuickScrollBar::handle() const
{
    Q_D(const QQuickScrollBar);
    return d->handle;
}

void QQuickScrollBar::setHandle(QQuickItem *handle)
{
    Q_D(QQuickScrollBar);
    if (d->handle != handle) {
        delete d->handle;
        d->handle = handle;
        if (handle && !handle->parentItem())
            handle->setParentItem(this);
        emit handleChanged();
    }
}

void QQuickScrollBar::mousePressEvent(QMouseEvent *event)
{
    Q_D(QQuickScrollBar);
    QQuickControl::mousePressEvent(event);
    d->offset = positionAt(event->pos()) - d->position;
    if (d->offset < 0 || d->offset > d->size)
        d->offset = d->size / 2;
    setPressed(true);
}

void QQuickScrollBar::mouseMoveEvent(QMouseEvent *event)
{
    Q_D(QQuickScrollBar);
    QQuickControl::mouseMoveEvent(event);
    setPosition(qBound<qreal>(0.0, positionAt(event->pos()) - d->offset, 1.0 - d->size));
}

void QQuickScrollBar::mouseReleaseEvent(QMouseEvent *event)
{
    Q_D(QQuickScrollBar);
    QQuickControl::mouseReleaseEvent(event);
    setPosition(qBound<qreal>(0.0, positionAt(event->pos()) - d->offset, 1.0 - d->size));
    d->offset = 0.0;
    setPressed(false);
}

qreal QQuickScrollBar::positionAt(const QPoint &point) const
{
    Q_D(const QQuickScrollBar);
    if (d->orientation == Qt::Horizontal)
        return (point.x() - leftPadding()) / availableWidth();
    else
        return (point.y() - topPadding()) / availableHeight();
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
    QQuickScrollBarAttachedPrivate(QQuickFlickable *flickable) : flickable(flickable), horizontal(Q_NULLPTR), vertical(Q_NULLPTR) { }

    void activateHorizontal();
    void activateVertical();
    void scrollHorizontal();
    void scrollVertical();

    void layoutHorizontal(bool move = true);
    void layoutVertical(bool move = true);

    void itemGeometryChanged(QQuickItem *item, const QRectF &newGeometry, const QRectF &oldGeometry) Q_DECL_OVERRIDE;

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
    QQuickItemPrivate *p = QQuickItemPrivate::get(flickable);
    p->updateOrAddGeometryChangeListener(d, QQuickItemPrivate::SizeChange);
}

QQuickScrollBarAttached::~QQuickScrollBarAttached()
{
    Q_D(QQuickScrollBarAttached);
    if (d->horizontal)
        QQuickItemPrivate::get(d->horizontal)->removeItemChangeListener(d, QQuickItemPrivate::Geometry);
    if (d->vertical)
        QQuickItemPrivate::get(d->vertical)->removeItemChangeListener(d, QQuickItemPrivate::Geometry);
}

/*!
    \qmlattachedproperty ScrollBar Qt.labs.controls::ScrollBar::horizontal

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
    if (d->horizontal != horizontal) {
        if (d->horizontal) {
            QQuickItemPrivate::get(d->horizontal)->removeItemChangeListener(d, QQuickItemPrivate::Geometry);
            QObjectPrivate::disconnect(d->horizontal, &QQuickScrollBar::positionChanged, d, &QQuickScrollBarAttachedPrivate::scrollHorizontal);
            QObjectPrivate::disconnect(d->flickable, &QQuickFlickable::movingHorizontallyChanged, d, &QQuickScrollBarAttachedPrivate::activateHorizontal);

            // TODO: export QQuickFlickableVisibleArea
            QObject *area = d->flickable->property("visibleArea").value<QObject *>();
            disconnect(area, SIGNAL(widthRatioChanged(qreal)), d->horizontal, SLOT(setSize(qreal)));
            disconnect(area, SIGNAL(xPositionChanged(qreal)), d->horizontal, SLOT(setPosition(qreal)));
        }

        d->horizontal = horizontal;

        if (horizontal) {
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
}

/*!
    \qmlattachedproperty ScrollBar Qt.labs.controls::ScrollBar::vertical

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
    if (d->vertical != vertical) {
        if (d->vertical) {
            QQuickItemPrivate::get(d->vertical)->removeItemChangeListener(d, QQuickItemPrivate::Geometry);
            QObjectPrivate::disconnect(d->vertical, &QQuickScrollBar::positionChanged, d, &QQuickScrollBarAttachedPrivate::scrollVertical);
            QObjectPrivate::disconnect(d->flickable, &QQuickFlickable::movingVerticallyChanged, d, &QQuickScrollBarAttachedPrivate::activateVertical);

            // TODO: export QQuickFlickableVisibleArea
            QObject *area = d->flickable->property("visibleArea").value<QObject *>();
            disconnect(area, SIGNAL(heightRatioChanged(qreal)), d->vertical, SLOT(setSize(qreal)));
            disconnect(area, SIGNAL(yPositionChanged(qreal)), d->vertical, SLOT(setPosition(qreal)));
        }

        d->vertical = vertical;

        if (vertical) {
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
}

QT_END_NAMESPACE
