/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Qt Quick Controls module of the Qt Toolkit.
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

#include "qquickscrollindicator_p.h"
#include "qquickcontrol_p_p.h"

#include <QtQuick/private/qquickflickable_p.h>
#include <QtQuick/private/qquickitemchangelistener_p.h>

QT_BEGIN_NAMESPACE

/*!
    \qmltype ScrollIndicator
    \inherits Control
    \instantiates QQuickScrollIndicator
    \inqmlmodule QtQuick.Controls
    \ingroup indicators
    \brief A scroll indicator.

    TODO
*/

class QQuickScrollIndicatorPrivate : public QQuickControlPrivate
{
public:
    QQuickScrollIndicatorPrivate() : size(0), position(0),
        active(false), orientation(Qt::Vertical), indicator(Q_NULLPTR) { }

    qreal size;
    qreal position;
    bool active;
    Qt::Orientation orientation;
    QQuickItem *indicator;
};

QQuickScrollIndicator::QQuickScrollIndicator(QQuickItem *parent) :
    QQuickControl(*(new QQuickScrollIndicatorPrivate), parent)
{
}

QQuickScrollIndicatorAttached *QQuickScrollIndicator::qmlAttachedProperties(QObject *object)
{
    QQuickFlickable *flickable = qobject_cast<QQuickFlickable *>(object);
    if (flickable)
        return new QQuickScrollIndicatorAttached(flickable);

    qWarning() << "ScrollIndicator must be attached to a Flickable" << object;
    return Q_NULLPTR;
}

/*!
    \qmlproperty real QtQuickControls2::ScrollIndicator::size

    TODO
*/
qreal QQuickScrollIndicator::size() const
{
    Q_D(const QQuickScrollIndicator);
    return d->size;
}

void QQuickScrollIndicator::setSize(qreal size)
{
    Q_D(QQuickScrollIndicator);
    if (!qFuzzyCompare(d->size, size)) {
        d->size = size;
        emit sizeChanged();
    }
}

/*!
    \qmlproperty real QtQuickControls2::ScrollIndicator::position

    TODO
*/
qreal QQuickScrollIndicator::position() const
{
    Q_D(const QQuickScrollIndicator);
    return d->position;
}

void QQuickScrollIndicator::setPosition(qreal position)
{
    Q_D(QQuickScrollIndicator);
    if (!qFuzzyCompare(d->position, position)) {
        d->position = position;
        emit positionChanged();
    }
}

/*!
    \qmlproperty bool QtQuickControls2::ScrollIndicator::active

    TODO
*/
bool QQuickScrollIndicator::isActive() const
{
    Q_D(const QQuickScrollIndicator);
    return d->active;
}

void QQuickScrollIndicator::setActive(bool active)
{
    Q_D(QQuickScrollIndicator);
    if (d->active != active) {
        d->active = active;
        emit activeChanged();
    }
}

/*!
    \qmlproperty enumeration QtQuickControls2::ScrollIndicator::orientation

    TODO
*/
Qt::Orientation QQuickScrollIndicator::orientation() const
{
    Q_D(const QQuickScrollIndicator);
    return d->orientation;
}

void QQuickScrollIndicator::setOrientation(Qt::Orientation orientation)
{
    Q_D(QQuickScrollIndicator);
    if (d->orientation != orientation) {
        d->orientation = orientation;
        emit orientationChanged();
    }
}

/*!
    \qmlproperty Item QtQuickControls2::ScrollIndicator::indicator

    TODO
*/
QQuickItem *QQuickScrollIndicator::indicator() const
{
    Q_D(const QQuickScrollIndicator);
    return d->indicator;
}

void QQuickScrollIndicator::setIndicator(QQuickItem *indicator)
{
    Q_D(QQuickScrollIndicator);
    if (d->indicator != indicator) {
        delete d->indicator;
        d->indicator = indicator;
        if (indicator && !indicator->parentItem())
            indicator->setParentItem(this);
        emit indicatorChanged();
    }
}

class QQuickScrollIndicatorAttachedPrivate : public QObjectPrivate, public QQuickItemChangeListener
{
public:
    QQuickScrollIndicatorAttachedPrivate(QQuickFlickable *flickable) : flickable(flickable), horizontal(Q_NULLPTR), vertical(Q_NULLPTR) { }

    void activateHorizontal();
    void activateVertical();

    void itemGeometryChanged(QQuickItem *item, const QRectF &newGeometry, const QRectF &oldGeometry) Q_DECL_OVERRIDE;

    QQuickFlickable *flickable;
    QQuickScrollIndicator *horizontal;
    QQuickScrollIndicator *vertical;
};

void QQuickScrollIndicatorAttachedPrivate::activateHorizontal()
{
    horizontal->setActive(flickable->isMovingHorizontally());
}

void QQuickScrollIndicatorAttachedPrivate::activateVertical()
{
    vertical->setActive(flickable->isMovingVertically());
}

void QQuickScrollIndicatorAttachedPrivate::itemGeometryChanged(QQuickItem *item, const QRectF &newGeometry, const QRectF &oldGeometry)
{
    Q_ASSERT(item == flickable);
    if (horizontal) {
        QQuickItemPrivate *p = QQuickItemPrivate::get(horizontal);
        if (!p->widthValid) {
            horizontal->setWidth(newGeometry.width());
            p->widthValid = false;
        }
        if (qFuzzyIsNull(horizontal->y()) || qFuzzyCompare(horizontal->y(), oldGeometry.height() - horizontal->height()))
            horizontal->setY(newGeometry.height() - horizontal->height());
    }
    if (vertical) {
        QQuickItemPrivate *p = QQuickItemPrivate::get(vertical);
        if (!p->heightValid) {
            vertical->setHeight(newGeometry.height());
            p->heightValid = false;
        }
        if (!p->isMirrored() && (qFuzzyIsNull(vertical->x()) || qFuzzyCompare(vertical->x(), oldGeometry.width() - vertical->width())))
            vertical->setX(newGeometry.width() - vertical->width());
    }
}

QQuickScrollIndicatorAttached::QQuickScrollIndicatorAttached(QQuickFlickable *flickable) :
    QObject(*(new QQuickScrollIndicatorAttachedPrivate(flickable)), flickable)
{
    Q_D(QQuickScrollIndicatorAttached);
    QQuickItemPrivate *p = QQuickItemPrivate::get(flickable);
    p->addItemChangeListener(d, QQuickItemPrivate::Geometry);
}

QQuickScrollIndicator *QQuickScrollIndicatorAttached::horizontal() const
{
    Q_D(const QQuickScrollIndicatorAttached);
    return d->horizontal;
}

void QQuickScrollIndicatorAttached::setHorizontal(QQuickScrollIndicator *horizontal)
{
    Q_D(QQuickScrollIndicatorAttached);
    if (d->horizontal != horizontal) {
        if (d->horizontal) {
            QObjectPrivate::disconnect(d->flickable, &QQuickFlickable::movingHorizontallyChanged, d, &QQuickScrollIndicatorAttachedPrivate::activateHorizontal);

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

            QObjectPrivate::connect(d->flickable, &QQuickFlickable::movingHorizontallyChanged, d, &QQuickScrollIndicatorAttachedPrivate::activateHorizontal);

            // TODO: export QQuickFlickableVisibleArea
            QObject *area = d->flickable->property("visibleArea").value<QObject *>();
            connect(area, SIGNAL(widthRatioChanged(qreal)), horizontal, SLOT(setSize(qreal)));
            connect(area, SIGNAL(xPositionChanged(qreal)), horizontal, SLOT(setPosition(qreal)));

            horizontal->setSize(area->property("widthRatio").toReal());
            horizontal->setPosition(area->property("xPosition").toReal());
        }
        emit horizontalChanged();
    }
}

QQuickScrollIndicator *QQuickScrollIndicatorAttached::vertical() const
{
    Q_D(const QQuickScrollIndicatorAttached);
    return d->vertical;
}

void QQuickScrollIndicatorAttached::setVertical(QQuickScrollIndicator *vertical)
{
    Q_D(QQuickScrollIndicatorAttached);
    if (d->vertical != vertical) {
        if (d->vertical) {
            QObjectPrivate::disconnect(d->flickable, &QQuickFlickable::movingVerticallyChanged, d, &QQuickScrollIndicatorAttachedPrivate::activateVertical);

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

            QObjectPrivate::connect(d->flickable, &QQuickFlickable::movingVerticallyChanged, d, &QQuickScrollIndicatorAttachedPrivate::activateVertical);

            // TODO: export QQuickFlickableVisibleArea
            QObject *area = d->flickable->property("visibleArea").value<QObject *>();
            connect(area, SIGNAL(heightRatioChanged(qreal)), vertical, SLOT(setSize(qreal)));
            connect(area, SIGNAL(yPositionChanged(qreal)), vertical, SLOT(setPosition(qreal)));

            vertical->setSize(area->property("heightRatio").toReal());
            vertical->setPosition(area->property("yPosition").toReal());
        }
        emit verticalChanged();
    }
}

QT_END_NAMESPACE
