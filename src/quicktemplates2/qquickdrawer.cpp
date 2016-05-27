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

#include "qquickdrawer_p.h"
#include "qquickpopup_p_p.h"
#include "qquickvelocitycalculator_p_p.h"

#include <QtGui/qstylehints.h>
#include <QtGui/private/qguiapplication_p.h>
#include <QtQuick/private/qquickwindow_p.h>
#include <QtQuick/private/qquickanimation_p.h>
#include <QtQuick/private/qquicktransition_p.h>
#include <QtQuick/private/qquickitemchangelistener_p.h>

QT_BEGIN_NAMESPACE

/*!
    \qmltype Drawer
    \inherits Popup
    \instantiates QQuickDrawer
    \inqmlmodule QtQuick.Controls
    \since 5.7
    \ingroup qtquickcontrols2-navigation
    \ingroup qtquickcontrols2-popups
    \brief Provides a swipe-based side panel.

    Drawer provides a swipe-based side panel, similar to those often used in
    touch interfaces to provide a central location for navigation.

    \table
    \row
        \li \image qtquickcontrols2-drawer-wireframe.png
            Drawer can be positioned at any of the four edges of the content item. \br
            In this image, it is against the left edge of the window.

        \li \image qtquickcontrols2-drawer-expanded-wireframe.png
            The drawer is then opened by \e "dragging" it out from the left edge \br
            of the window.
    \endtable

    In the image above, the application's contents are \e "pushed" across the
    screen. This is achieved by applying a translation to the contents:

    \code
    import QtQuick 2.7
    import QtQuick.Controls 2.0

    ApplicationWindow {
        id: window
        width: 200
        height: 228
        visible: true

        Drawer {
            id: drawer
            width: 0.66 * window.width
            height: window.height
        }

        Label {
            id: content

            text: "Aa"
            font.pixelSize: 96
            anchors.fill: parent
            verticalAlignment: Label.AlignVCenter
            horizontalAlignment: Label.AlignHCenter

            transform: Translate {
                x: drawer.position * content.width * 0.33
            }
        }
    }
    \endcode

    If you would like the application's contents to stay where they are when
    the drawer is opened, don't apply a translation.

    \note On some platforms, certain edges may be reserved for system
    gestures and therefore cannot be used with Drawer.

    \sa SwipeView, {Customizing Drawer}, {Navigation Controls}, {Popup Controls}
*/

class QQuickDrawerPrivate : public QQuickPopupPrivate, public QQuickItemChangeListener
{
    Q_DECLARE_PUBLIC(QQuickDrawer)

public:
    QQuickDrawerPrivate() : edge(Qt::LeftEdge), offset(0), position(0),
        dragMargin(QGuiApplication::styleHints()->startDragDistance()) { }

    qreal positionAt(const QPointF &point) const;
    void reposition() override;

    bool handleMousePressEvent(QQuickItem *item, QMouseEvent *event);
    bool handleMouseMoveEvent(QQuickItem *item, QMouseEvent *event);
    bool handleMouseReleaseEvent(QQuickItem *item, QMouseEvent *event);

    void prepareEnterTransition(bool notify = true) override;
    void prepareExitTransition() override;
    void finalizeEnterTransition() override;
    void finalizeExitTransition(bool hide = true) override;

    Qt::Edge edge;
    qreal offset;
    qreal position;
    qreal dragMargin;
    QPointF pressPoint;
    QQuickVelocityCalculator velocityCalculator;
};

qreal QQuickDrawerPrivate::positionAt(const QPointF &point) const
{
    Q_Q(const QQuickDrawer);
    QQuickWindow *window = q->window();
    if (!window)
        return 0;

    switch (edge) {
    case Qt::TopEdge:
        return point.y() / q->height();
    case Qt::LeftEdge:
        return point.x() / q->width();
    case Qt::RightEdge:
        return (window->width() - point.x()) / q->width();
    case Qt::BottomEdge:
        return (window->height() - point.y()) / q->height();
    default:
        return 0;
    }
}

void QQuickDrawerPrivate::reposition()
{
    Q_Q(QQuickDrawer);
    QQuickWindow *window = q->window();
    if (!window)
        return;

    switch (edge) {
    case Qt::LeftEdge:
        popupItem->setX((position - 1.0) * popupItem->width());
        break;
    case Qt::RightEdge:
        popupItem->setX(window->width() - position * popupItem->width());
        break;
    case Qt::TopEdge:
        popupItem->setY((position - 1.0) * popupItem->height());
        break;
    case Qt::BottomEdge:
        popupItem->setY(window->height() - position * popupItem->height());
        break;
    }
}

static bool dragOverThreshold(qreal d, Qt::Axis axis, QMouseEvent *event, int threshold = -1)
{
    return QQuickWindowPrivate::dragOverThreshold(d, axis, event, threshold);
}

bool QQuickDrawerPrivate::handleMousePressEvent(QQuickItem *item, QMouseEvent *event)
{
    pressPoint = event->windowPos();
    offset = 0;

    QQuickWindow *window = item->window();
    if (!window)
        return false;

    if (qFuzzyIsNull(position)) {
        // only accept pressing at drag margins when fully closed
        switch (edge) {
        case Qt::LeftEdge:
            event->setAccepted(dragMargin > 0 && !dragOverThreshold(event->windowPos().x(), Qt::XAxis, event, dragMargin));
            break;
        case Qt::RightEdge:
            event->setAccepted(dragMargin > 0 && !dragOverThreshold(window->width() - event->windowPos().x(), Qt::XAxis, event, dragMargin));
            break;
        case Qt::TopEdge:
            event->setAccepted(dragMargin > 0 && !dragOverThreshold(event->windowPos().y(), Qt::YAxis, event, dragMargin));
            break;
        case Qt::BottomEdge:
            event->setAccepted(dragMargin > 0 && !dragOverThreshold(window->height() - event->windowPos().y(), Qt::YAxis, event, dragMargin));
            break;
        }
    } else {
        if (modal)
            event->setAccepted(item->isAncestorOf(popupItem));
        else
            event->setAccepted(false);
    }

    velocityCalculator.startMeasuring(pressPoint, event->timestamp());

    return event->isAccepted();
}

bool QQuickDrawerPrivate::handleMouseMoveEvent(QQuickItem *item, QMouseEvent *event)
{
    Q_Q(QQuickDrawer);
    QQuickWindow *window = item->window();
    if (!window)
        return false;

    QPointF movePoint = event->windowPos();

    if (!popupItem->keepMouseGrab()) {
        // Flickable uses a hard-coded threshold of 15 for flicking, and
        // QStyleHints::startDragDistance for dragging. Drawer uses a bit
        // larger threshold to avoid being too eager to steal touch (QTBUG-50045)
        int threshold = qMax(20, QGuiApplication::styleHints()->startDragDistance() + 5);
        bool overThreshold = false;
        if (edge == Qt::LeftEdge || edge == Qt::RightEdge)
            overThreshold = dragOverThreshold(movePoint.x() - pressPoint.x(), Qt::XAxis, event, threshold);
        else
            overThreshold = dragOverThreshold(movePoint.y() - pressPoint.y(), Qt::YAxis, event, threshold);

        if (overThreshold) {
            QQuickItem *grabber = window->mouseGrabberItem();
            if (!grabber || !grabber->keepMouseGrab()) {
                popupItem->grabMouse();
                popupItem->setKeepMouseGrab(overThreshold);
                offset = qMin<qreal>(0.0, positionAt(movePoint) - position);
            }
        }
    }

    if (popupItem->keepMouseGrab())
        q->setPosition(positionAt(movePoint) - offset);
    event->accept();

    return popupItem->keepMouseGrab();
}

static const qreal openCloseVelocityThreshold = 300;

bool QQuickDrawerPrivate::handleMouseReleaseEvent(QQuickItem *item, QMouseEvent *event)
{
    Q_UNUSED(item);

    bool wasGrabbed = popupItem->keepMouseGrab();
    if (wasGrabbed) {
        velocityCalculator.stopMeasuring(event->pos(), event->timestamp());
        const qreal velocity = velocityCalculator.velocity().x();

        if (position > 0.7 || velocity > openCloseVelocityThreshold) {
            transitionManager.transitionEnter();
        } else if (position < 0.3 || velocity < -openCloseVelocityThreshold) {
            transitionManager.transitionExit();
        } else {
            switch (edge) {
            case Qt::LeftEdge:
                if (event->x() - pressPoint.x() > 0)
                    transitionManager.transitionEnter();
                else
                    transitionManager.transitionExit();
                break;
            case Qt::RightEdge:
                if (event->x() - pressPoint.x() < 0)
                    transitionManager.transitionEnter();
                else
                    transitionManager.transitionExit();
                break;
            case Qt::TopEdge:
                if (event->y() - pressPoint.y() > 0)
                    transitionManager.transitionEnter();
                else
                    transitionManager.transitionExit();
                break;
            case Qt::BottomEdge:
                if (event->y() - pressPoint.y() < 0)
                    transitionManager.transitionEnter();
                else
                    transitionManager.transitionExit();
                break;
            }
        }
        popupItem->setKeepMouseGrab(false);
    }
    pressPoint = QPoint();
    event->accept();
    return wasGrabbed;
}

static QList<QQuickStateAction> prepareTransition(QQuickDrawer *drawer, QQuickTransition *transition, qreal to)
{
    QList<QQuickStateAction> actions;
    if (!transition)
        return actions;

    qmlExecuteDeferred(transition);

    QQmlProperty defaultTarget(drawer, QLatin1String("position"));
    QQmlListProperty<QQuickAbstractAnimation> animations = transition->animations();
    int count = animations.count(&animations);
    for (int i = 0; i < count; ++i) {
        QQuickAbstractAnimation *anim = animations.at(&animations, i);
        anim->setDefaultTarget(defaultTarget);
    }

    actions << QQuickStateAction(drawer, QLatin1String("position"), to);
    return actions;
}

void QQuickDrawerPrivate::prepareEnterTransition(bool notify)
{
    Q_Q(QQuickDrawer);
    enterActions = prepareTransition(q, enter, 1.0);
    QQuickPopupPrivate::prepareEnterTransition(notify);
}

void QQuickDrawerPrivate::prepareExitTransition()
{
    Q_Q(QQuickDrawer);
    exitActions = prepareTransition(q, exit, 0.0);
    QQuickPopupPrivate::prepareExitTransition();
}

void QQuickDrawerPrivate::finalizeEnterTransition()
{
    QQuickPopupPrivate::finalizeEnterTransition();
}

void QQuickDrawerPrivate::finalizeExitTransition(bool hide)
{
    QQuickPopupPrivate::finalizeExitTransition(hide = false);
}

QQuickDrawer::QQuickDrawer(QObject *parent) :
    QQuickPopup(*(new QQuickDrawerPrivate), parent)
{
    setFocus(true);
    setModal(true);
    setFiltersChildMouseEvents(true);
    setClosePolicy(CloseOnEscape | CloseOnReleaseOutside);
}

/*!
    \qmlproperty enumeration QtQuick.Controls::Drawer::edge

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
    if (d->edge == edge)
        return;

    d->edge = edge;
    if (isComponentComplete())
        d->reposition();
    emit edgeChanged();
}

/*!
    \qmlproperty real QtQuick.Controls::Drawer::position

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
    if (qFuzzyCompare(d->position, position))
        return;

    d->position = position;
    if (isComponentComplete())
        d->reposition();
    if (d->dimmer)
        d->dimmer->setOpacity(position);
    emit positionChanged();
}

/*!
    \qmlproperty real QtQuick.Controls::Drawer::dragMargin

    This property holds the distance from the screen edge within which
    drag actions will open the drawer. Setting the value to \c 0 or less
    prevents opening the drawer by dragging.

    The default value is \c Qt.styleHints.startDragDistance.
*/
qreal QQuickDrawer::dragMargin() const
{
    Q_D(const QQuickDrawer);
    return d->dragMargin;
}

void QQuickDrawer::setDragMargin(qreal margin)
{
    Q_D(QQuickDrawer);
    if (qFuzzyCompare(d->dragMargin, margin))
        return;

    d->dragMargin = margin;
    emit dragMarginChanged();
}

void QQuickDrawer::resetDragMargin()
{
    setDragMargin(QGuiApplication::styleHints()->startDragDistance());
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
    QQuickPopup::mousePressEvent(event);
    d->handleMousePressEvent(d->popupItem, event);
}

void QQuickDrawer::mouseMoveEvent(QMouseEvent *event)
{
    Q_D(QQuickDrawer);
    QQuickPopup::mouseMoveEvent(event);
    d->handleMouseMoveEvent(d->popupItem, event);
}

void QQuickDrawer::mouseReleaseEvent(QMouseEvent *event)
{
    Q_D(QQuickDrawer);
    QQuickPopup::mouseReleaseEvent(event);
    d->handleMouseReleaseEvent(d->popupItem, event);
}

void QQuickDrawer::mouseUngrabEvent()
{
    Q_D(QQuickDrawer);
    QQuickPopup::mouseUngrabEvent();
    d->pressPoint = QPoint();
    d->velocityCalculator.reset();
}

bool QQuickDrawer::overlayEvent(QQuickItem *item, QEvent *event)
{
    Q_D(QQuickDrawer);
    switch (event->type()) {
    case QEvent::MouseButtonPress:
        d->tryClose(item, static_cast<QMouseEvent *>(event));
        return d->handleMousePressEvent(item, static_cast<QMouseEvent *>(event));
    case QEvent::MouseMove:
        return d->handleMouseMoveEvent(item, static_cast<QMouseEvent *>(event));
    case QEvent::MouseButtonRelease:
        d->tryClose(item, static_cast<QMouseEvent *>(event));
        return d->handleMouseReleaseEvent(item, static_cast<QMouseEvent *>(event));
    default:
        return false;
    }
}

void QQuickDrawer::componentComplete()
{
    Q_D(QQuickDrawer);
    QQuickPopup::componentComplete();
    bool notify = false;
    d->prepareEnterTransition(notify);
    d->reposition();
}

QT_END_NAMESPACE
