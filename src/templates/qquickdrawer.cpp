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

#include "qquickdrawer_p.h"

#include <QtGui/qstylehints.h>
#include <QtGui/private/qguiapplication_p.h>
#include <QtQuick/private/qquickwindow_p.h>
#include <QtQuick/private/qquickanimation_p.h>
#include <QtQuick/private/qquickitemchangelistener_p.h>
#include <QtLabsTemplates/private/qquickcontrol_p_p.h>

QT_BEGIN_NAMESPACE

/*!
    \qmltype Drawer
    \inherits Container
    \instantiates QQuickDrawer
    \inqmlmodule Qt.labs.controls
    \ingroup qtlabscontrols-navigation
    \ingroup qtlabscontrols-containers
    \brief A side panel control.

    Drawer provides a swipe-based side panel, similar to those often used in
    touch interfaces to provide a central location for navigation.

    \table
    \row
        \li \image qtlabscontrols-drawer-wireframe.png
            Drawer can be positioned at any of the four edges of the content item. \br
            In this image, it is against the left edge of the window.

        \li \image qtlabscontrols-drawer-expanded-wireframe.png
            The drawer is then opened by \e "dragging" it out from the left edge \br
            of the window.
    \endtable

    In the image above, the application's contents are \e "pushed" across the
    screen. This is achieved by applying a translation to the contents:

    \code
    transform: Translate {
        x: (1.0 - drawer.position) * listview.width
    }
    \endcode

    If you would like the application's contents to stay where they are when
    the drawer is opened, don't apply a translation.

    \labs

    \sa SwipeView, {Customizing Drawer}, {Navigation Controls}, {Container Controls}
*/

/*!
    \qmlsignal Qt.labs.controls::Drawer::clicked()

    This signal is emitted when the drawer is clicked.
*/

class QQuickDrawerPrivate : public QQuickControlPrivate, public QQuickItemChangeListener
{
    Q_DECLARE_PUBLIC(QQuickDrawer)

public:
    QQuickDrawerPrivate() : edge(Qt::LeftEdge), offset(0), position(0),
        content(Q_NULLPTR), animation(Q_NULLPTR) { }

    void updateContent();
    bool handleMousePressEvent(QQuickItem *item, QMouseEvent *event);
    bool handleMouseMoveEvent(QQuickItem *item, QMouseEvent *event);
    bool handleMouseReleaseEvent(QQuickItem *item, QMouseEvent *event);

    void itemGeometryChanged(QQuickItem *, const QRectF &, const QRectF &) Q_DECL_OVERRIDE;

    Qt::Edge edge;
    qreal offset;
    qreal position;
    QPointF pressPoint;
    QQuickItem *content;
    QQuickPropertyAnimation *animation;
};

void QQuickDrawerPrivate::updateContent()
{
    Q_Q(QQuickDrawer);
    if (!content)
        return;

    switch (edge) {
    case Qt::LeftEdge:
        content->setX((position - 1.0) * content->width());
        break;
    case Qt::RightEdge:
        content->setX(q->width() + - position * content->width());
        break;
    case Qt::TopEdge:
        content->setY((position - 1.0) * content->height());
        break;
    case Qt::BottomEdge:
        content->setY(q->height() + - position * content->height());
        break;
    }
}

static bool dragOverThreshold(qreal d, Qt::Axis axis, QMouseEvent *event, int threshold = -1)
{
    return QQuickWindowPrivate::dragOverThreshold(d, axis, event, threshold);
}

bool QQuickDrawerPrivate::handleMousePressEvent(QQuickItem *item, QMouseEvent *event)
{
    Q_Q(QQuickDrawer);
    pressPoint = event->windowPos();
    offset = 0;

    if (qFuzzyIsNull(position)) {
        // only accept pressing at drag margins when fully closed
        switch (edge) {
        case Qt::LeftEdge:
            event->setAccepted(!dragOverThreshold(event->windowPos().x(), Qt::XAxis, event));
            break;
        case Qt::RightEdge:
            event->setAccepted(!dragOverThreshold(q->width() - event->windowPos().x(), Qt::XAxis, event));
            break;
        case Qt::TopEdge:
            event->setAccepted(!dragOverThreshold(event->windowPos().y(), Qt::YAxis, event));
            break;
        case Qt::BottomEdge:
            event->setAccepted(!dragOverThreshold(q->height() - event->windowPos().y(), Qt::YAxis, event));
            break;
        }
    } else {
        event->accept();
    }

    return item == q;
}

bool QQuickDrawerPrivate::handleMouseMoveEvent(QQuickItem *item, QMouseEvent *event)
{
    Q_Q(QQuickDrawer);
    Q_UNUSED(item);
    QPointF movePoint = event->windowPos();

    if (!q->keepMouseGrab()) {
        // Flickable uses a hard-coded threshold of 15 for flicking, and
        // QStyleHints::startDragDistance for dragging. Drawer uses a bit
        // larger threshold to avoid being too eager to steal touch (QTBUG-50045)
        int threshold = qMax(20, QGuiApplication::styleHints()->startDragDistance() + 5);
        bool overThreshold = false;
        if (edge == Qt::LeftEdge || edge == Qt::RightEdge)
            overThreshold = dragOverThreshold(movePoint.x() - pressPoint.x(), Qt::XAxis, event, threshold);
        else
            overThreshold = dragOverThreshold(movePoint.y() - pressPoint.y(), Qt::YAxis, event, threshold);

        if (window && overThreshold) {
            QQuickItem *grabber = q->window()->mouseGrabberItem();
            if (!grabber || !grabber->keepMouseGrab()) {
                q->grabMouse();
                q->setKeepMouseGrab(overThreshold);
                offset = qMin<qreal>(0.0, q->positionAt(movePoint) - position);
            }
        }
    }

    if (q->keepMouseGrab())
        q->setPosition(q->positionAt(movePoint) - offset);
    event->accept();

    return q->keepMouseGrab();
}

bool QQuickDrawerPrivate::handleMouseReleaseEvent(QQuickItem *item, QMouseEvent *event)
{
    Q_Q(QQuickDrawer);
    bool wasGrabbed = q->keepMouseGrab();
    if (wasGrabbed) {
//        int startDragVelocity = QGuiApplication::styleHints()->startDragVelocity();
//        if (startDragVelocity && QGuiApplicationPrivate::mouseEventCaps(event) & QTouchDevice::Velocity) {
//            QVector2D velocity = QGuiApplicationPrivate::mouseEventVelocity(event);
//            qreal vel = (edge == Qt::LeftEdge || edge == Qt::RightEdge) ? velocity.x() : velocity.y();
//            qDebug() << vel << "vs." << startDragVelocity;
//        }
        if (position < 0.3) {
            q->close();
        } else if (position > 0.7) {
            q->open();
        } else {
            switch (edge) {
            case Qt::LeftEdge:
                if (event->x() - pressPoint.x() > 0)
                    q->open();
                else
                    q->close();
                break;
            case Qt::RightEdge:
                if (event->x() - pressPoint.x() < 0)
                    q->open();
                else
                    q->close();
                break;
            case Qt::TopEdge:
                if (event->y() - pressPoint.y() > 0)
                    q->open();
                else
                    q->close();
                break;
            case Qt::BottomEdge:
                if (event->y() - pressPoint.y() < 0)
                    q->open();
                else
                    q->close();
                break;
            }
        }
        q->setKeepMouseGrab(false);
    } else {
        if (item == q)
            emit q->clicked();
    }
    pressPoint = QPoint();
    event->accept();
    return wasGrabbed;
}

void QQuickDrawerPrivate::itemGeometryChanged(QQuickItem *, const QRectF &, const QRectF &)
{
    updateContent();
}

QQuickDrawer::QQuickDrawer(QQuickItem *parent) :
    QQuickControl(*(new QQuickDrawerPrivate), parent)
{
    setZ(1);
    setFiltersChildMouseEvents(true);
    setAcceptedMouseButtons(Qt::LeftButton);
}

/*!
    \qmlproperty enumeration Qt.labs.controls::Drawer::edge

    This property holds the edge of the content item at which the drawer will
    open from. The acceptable values are:

    \value Qt.TopEdge     The top edge of the content item.
    \value Qt.LeftEdge    The left edge of the content item (default).
    \value Qt.RightEdge   The right edge of the content item.
    \value Qt.BottomEdge  The bottom edge of the content item.
*/
Qt::Edge QQuickDrawer::edge() const
{
    Q_D(const QQuickDrawer);
    return d->edge;
}

void QQuickDrawer::setEdge(Qt::Edge edge)
{
    Q_D(QQuickDrawer);
    if (d->edge != edge) {
        d->edge = edge;
        if (isComponentComplete())
            d->updateContent();
        emit edgeChanged();
    }
}

/*!
    \qmlproperty real Qt.labs.controls::Drawer::position

    This property holds the position of the drawer relative to its final
    destination. That is, the position will be \c 0 when the drawer
    is fully closed, and \c 1 when fully open.
*/
qreal QQuickDrawer::position() const
{
    Q_D(const QQuickDrawer);
    return d->position;
}

void QQuickDrawer::setPosition(qreal position)
{
    Q_D(QQuickDrawer);
    position = qBound<qreal>(0.0, position, 1.0);
    if (!qFuzzyCompare(d->position, position)) {
        d->position = position;
        if (isComponentComplete())
            d->updateContent();
        emit positionChanged();
    }
}


/*!
    \qmlproperty Item Qt.labs.controls::Drawer::contentItem

    This property holds the content item that the drawer displays when opened.
*/
QQuickItem *QQuickDrawer::contentItem() const
{
    Q_D(const QQuickDrawer);
    return d->content;
}

void QQuickDrawer::setContentItem(QQuickItem *item)
{
    Q_D(QQuickDrawer);
    if (d->content != item) {
        if (d->content) {
            QQuickItemPrivate::get(d->content)->removeItemChangeListener(d, QQuickItemPrivate::Geometry);
            delete d->content;
        }
        d->content = item;
        if (item) {
            item->setParentItem(this);
            QQuickItemPrivate::get(item)->updateOrAddGeometryChangeListener(d, QQuickItemPrivate::SizeChange);
            if (isComponentComplete())
                d->updateContent();
        }
        emit contentItemChanged();
    }
}

/*!
    \qmlproperty Animation Qt.labs.controls::Drawer::animation

    This property holds the animation for the \l position property. It is used
    to animate the drawer's movement.

    If no valid animation is set, the drawer's movement will not be animated.
*/
QQuickPropertyAnimation *QQuickDrawer::animation() const
{
    Q_D(const QQuickDrawer);
    return d->animation;
}

void QQuickDrawer::setAnimation(QQuickPropertyAnimation *animation)
{
    Q_D(QQuickDrawer);
    if (d->animation != animation) {
        delete d->animation;
        d->animation = animation;
        if (animation) {
            animation->setTargetObject(this);
            animation->setProperty(QStringLiteral("position"));
        }
        emit animationChanged();
    }
}

/*!
    \qmlmethod void Qt.labs.controls::Drawer::open()

    This method opens the drawer, animating the movement if a valid
    \l animation has been set.
*/
void QQuickDrawer::open()
{
    Q_D(QQuickDrawer);
    if (d->animation) {
        d->animation->stop();
        d->animation->setFrom(d->position);
        d->animation->setTo(1.0);
        d->animation->start();
    } else {
        setPosition(1.0);
    }
}

/*!
    \qmlmethod void Qt.labs.controls::Drawer::close()

    This method closes the drawer, animating the movement if a valid
    \l animation has been set.
*/
void QQuickDrawer::close()
{
    Q_D(QQuickDrawer);
    if (d->animation) {
        d->animation->stop();
        d->animation->setFrom(d->position);
        d->animation->setTo(0.0);
        d->animation->start();
    } else {
        setPosition(0.0);
    }
}

bool QQuickDrawer::childMouseEventFilter(QQuickItem *child, QEvent *event)
{
    Q_D(QQuickDrawer);
    switch (event->type()) {
    case QEvent::MouseButtonPress:
        return d->handleMousePressEvent(child, static_cast<QMouseEvent *>(event));
    case QEvent::MouseMove:
        return d->handleMouseMoveEvent(child, static_cast<QMouseEvent *>(event));
    case QEvent::MouseButtonRelease:
        return d->handleMouseReleaseEvent(child, static_cast<QMouseEvent *>(event));
    default:
        return false;
    }
}

void QQuickDrawer::mousePressEvent(QMouseEvent *event)
{
    Q_D(QQuickDrawer);
    QQuickControl::mousePressEvent(event);
    d->handleMousePressEvent(this, event);
}

void QQuickDrawer::mouseMoveEvent(QMouseEvent *event)
{
    Q_D(QQuickDrawer);
    QQuickControl::mouseMoveEvent(event);
    d->handleMouseMoveEvent(this, event);
}

void QQuickDrawer::mouseReleaseEvent(QMouseEvent *event)
{
    Q_D(QQuickDrawer);
    QQuickControl::mouseReleaseEvent(event);
    d->handleMouseReleaseEvent(this, event);
}

void QQuickDrawer::mouseUngrabEvent()
{
    Q_D(QQuickDrawer);
    QQuickControl::mouseUngrabEvent();
    d->pressPoint = QPoint();
}

void QQuickDrawer::geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    Q_D(QQuickDrawer);
    QQuickControl::geometryChanged(newGeometry, oldGeometry);
    if (isComponentComplete())
        d->updateContent();
}

void QQuickDrawer::componentComplete()
{
    Q_D(QQuickDrawer);
    QQuickControl::componentComplete();
    d->updateContent();
}

qreal QQuickDrawer::positionAt(const QPointF &point) const
{
    Q_D(const QQuickDrawer);
    if (!d->content)
        return 0.0;

    switch (d->edge) {
    case Qt::TopEdge:
        return point.y() / d->content->height();
    case Qt::LeftEdge:
        return point.x() / d->content->width();
    case Qt::RightEdge:
        return (width() - point.x()) / d->content->width();
    case Qt::BottomEdge:
        return (height() - point.y()) / d->content->height();
    default:
        return 0;
    }
}

QT_END_NAMESPACE
