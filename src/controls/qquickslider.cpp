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

#include "qquickslider_p.h"
#include "qquickcontrol_p_p.h"

#include <QtQuick/private/qquickwindow_p.h>

QT_BEGIN_NAMESPACE

/*!
    \qmltype Slider
    \inherits Control
    \instantiates QQuickSlider
    \inqmlmodule QtQuick.Controls
    \ingroup sliders
    \brief A slider control.

    TODO
*/

class QQuickSliderPrivate : public QQuickControlPrivate
{
    Q_DECLARE_PUBLIC(QQuickSlider)

public:
    QQuickSliderPrivate() : value(0), position(0), stepSize(0),
        pressed(false), orientation(Qt::Horizontal), layoutDirection(Qt::LeftToRight),
        snapMode(QQuickSlider::NoSnap), handle(Q_NULLPTR), track(Q_NULLPTR) { }

    qreal snapPosition(qreal position) const;
    qreal positionAt(const QPoint &point) const;

    qreal value;
    qreal position;
    qreal stepSize;
    bool pressed;
    QPoint pressPoint;
    Qt::Orientation orientation;
    Qt::LayoutDirection layoutDirection;
    QQuickSlider::SnapMode snapMode;
    QQuickItem *handle;
    QQuickItem *track;
};

qreal QQuickSliderPrivate::snapPosition(qreal position) const
{
    if (qFuzzyIsNull(stepSize))
        return position;
    return qRound(position / stepSize) * stepSize;
}

qreal QQuickSliderPrivate::positionAt(const QPoint &point) const
{
    Q_Q(const QQuickSlider);
    if (orientation == Qt::Horizontal) {
        const qreal hw = handle ? handle->width() : 0;
        const qreal offset = hw / 2;
        const qreal extent = q->width() - hw;
        if (!qFuzzyIsNull(extent)) {
            const qreal pos = (point.x() - offset) / extent;
            if (q->effectiveLayoutDirection() == Qt::RightToLeft)
                return 1.0 - pos;
            return pos;
        }
    } else {
        const qreal hh = handle ? handle->height() : 0;
        const qreal offset = hh / 2;
        const qreal extent = q->height() - hh;
        if (!qFuzzyIsNull(extent))
            return (point.y() - offset) / extent;
    }
    return 0;
}

QQuickSlider::QQuickSlider(QQuickItem *parent) :
    QQuickControl(*(new QQuickSliderPrivate), parent)
{
    setActiveFocusOnTab(true);
    setAcceptedMouseButtons(Qt::LeftButton);
}

/*!
    \qmlproperty real QtQuickControls2::Slider::value

    TODO
*/
qreal QQuickSlider::value() const
{
    Q_D(const QQuickSlider);
    return d->value;
}

void QQuickSlider::setValue(qreal value)
{
    Q_D(QQuickSlider);
    value = qBound(0.0, value, 1.0);
    if (!qFuzzyCompare(d->value, value)) {
        d->value = value;
        setPosition(value);
        emit valueChanged();
    }
}

/*!
    \qmlproperty real QtQuickControls2::Slider::position

    TODO
*/
qreal QQuickSlider::position() const
{
    Q_D(const QQuickSlider);
    return d->position;
}

/*!
    \qmlproperty real QtQuickControls2::Slider::visualPosition

    TODO
*/
qreal QQuickSlider::visualPosition() const
{
    Q_D(const QQuickSlider);
    if (d->orientation == Qt::Vertical || effectiveLayoutDirection() == Qt::RightToLeft)
        return 1.0 - d->position;
    return d->position;
}

void QQuickSlider::setPosition(qreal position)
{
    Q_D(QQuickSlider);
    position = qBound(0.0, position, 1.0);
    if (!qFuzzyCompare(d->position, position)) {
        d->position = position;
        emit positionChanged();
        emit visualPositionChanged();
    }
}

/*!
    \qmlproperty real QtQuickControls2::Slider::stepSize

    TODO
*/
qreal QQuickSlider::stepSize() const
{
    Q_D(const QQuickSlider);
    return d->stepSize;
}

void QQuickSlider::setStepSize(qreal step)
{
    Q_D(QQuickSlider);
    if (!qFuzzyCompare(d->stepSize, step)) {
        d->stepSize = step;
        emit stepSizeChanged();
    }
}

/*!
    \qmlproperty enumeration QtQuickControls2::Slider::snapMode

    TODO
*/
QQuickSlider::SnapMode QQuickSlider::snapMode() const
{
    Q_D(const QQuickSlider);
    return d->snapMode;
}

void QQuickSlider::setSnapMode(SnapMode mode)
{
    Q_D(QQuickSlider);
    if (d->snapMode != mode) {
        d->snapMode = mode;
        emit snapModeChanged();
    }
}

/*!
    \qmlproperty bool QtQuickControls2::Slider::pressed

    TODO
*/
bool QQuickSlider::isPressed() const
{
    Q_D(const QQuickSlider);
    return d->pressed;
}

void QQuickSlider::setPressed(bool pressed)
{
    Q_D(QQuickSlider);
    if (d->pressed != pressed) {
        d->pressed = pressed;
        emit pressedChanged();
    }
}

/*!
    \qmlproperty enumeration QtQuickControls2::Slider::orientation

    TODO
*/
Qt::Orientation QQuickSlider::orientation() const
{
    Q_D(const QQuickSlider);
    return d->orientation;
}

void QQuickSlider::setOrientation(Qt::Orientation orientation)
{
    Q_D(QQuickSlider);
    if (d->orientation != orientation) {
        d->orientation = orientation;
        emit orientationChanged();
    }
}

/*!
    \qmlproperty enumeration QtQuickControls2::Slider::layoutDirection

    TODO
*/
Qt::LayoutDirection QQuickSlider::layoutDirection() const
{
    Q_D(const QQuickSlider);
    return d->layoutDirection;
}

/*!
    \qmlproperty enumeration QtQuickControls2::Slider::effectiveLayoutDirection

    TODO
*/
Qt::LayoutDirection QQuickSlider::effectiveLayoutDirection() const
{
    Q_D(const QQuickSlider);
    if (isMirrored())
        return d->layoutDirection == Qt::RightToLeft ? Qt::LeftToRight : Qt::RightToLeft;
    return d->layoutDirection;
}

void QQuickSlider::setLayoutDirection(Qt::LayoutDirection direction)
{
    Q_D(QQuickSlider);
    if (d->layoutDirection != direction) {
        d->layoutDirection = direction;
        emit layoutDirectionChanged();
        emit effectiveLayoutDirectionChanged();
    }
}

/*!
    \qmlproperty Item QtQuickControls2::Slider::handle

    TODO
*/
QQuickItem *QQuickSlider::handle() const
{
    Q_D(const QQuickSlider);
    return d->handle;
}

void QQuickSlider::setHandle(QQuickItem *handle)
{
    Q_D(QQuickSlider);
    if (d->handle != handle) {
        delete d->handle;
        d->handle = handle;
        if (handle && !handle->parentItem())
            handle->setParentItem(this);
        emit handleChanged();
    }
}

/*!
    \qmlproperty Item QtQuickControls2::Slider::track

    TODO
*/
QQuickItem *QQuickSlider::track() const
{
    Q_D(const QQuickSlider);
    return d->track;
}

void QQuickSlider::setTrack(QQuickItem *track)
{
    Q_D(QQuickSlider);
    if (d->track != track) {
        delete d->track;
        d->track = track;
        if (track && !track->parentItem())
            track->setParentItem(this);
        emit trackChanged();
    }
}

/*!
    \qmlmethod void QtQuickControls2::Slider::increase()

    TODO
*/
void QQuickSlider::increase()
{
    Q_D(QQuickSlider);
    qreal step = qFuzzyIsNull(d->stepSize) ? 0.1 : d->stepSize;
    setValue(d->value + step);
}

/*!
    \qmlmethod void QtQuickControls2::Slider::decrease()

    TODO
*/
void QQuickSlider::decrease()
{
    Q_D(QQuickSlider);
    qreal step = qFuzzyIsNull(d->stepSize) ? 0.1 : d->stepSize;
    setValue(d->value - step);
}

void QQuickSlider::keyPressEvent(QKeyEvent *event)
{
    Q_D(QQuickSlider);
    QQuickControl::keyPressEvent(event);
    if (d->orientation == Qt::Horizontal) {
        if (event->key() == Qt::Key_Left) {
            setPressed(true);
            if (effectiveLayoutDirection() == Qt::RightToLeft)
                increase();
            else
                decrease();
            event->accept();
        } else if (event->key() == Qt::Key_Right) {
            setPressed(true);
            if (effectiveLayoutDirection() == Qt::RightToLeft)
                decrease();
            else
                increase();
            event->accept();
        }
    } else {
        if (event->key() == Qt::Key_Up) {
            setPressed(true);
            increase();
            event->accept();
        } else if (event->key() == Qt::Key_Down) {
            setPressed(true);
            decrease();
            event->accept();
        }
    }
}

void QQuickSlider::keyReleaseEvent(QKeyEvent *event)
{
    QQuickControl::keyReleaseEvent(event);
    setPressed(false);
}

void QQuickSlider::mousePressEvent(QMouseEvent *event)
{
    Q_D(QQuickSlider);
    QQuickControl::mousePressEvent(event);
    d->pressPoint = event->pos();
    setPressed(true);
    event->accept();
}

void QQuickSlider::mouseMoveEvent(QMouseEvent *event)
{
    Q_D(QQuickSlider);
    QQuickControl::mouseMoveEvent(event);
    if (!keepMouseGrab()) {
        if (d->orientation == Qt::Horizontal)
            setKeepMouseGrab(QQuickWindowPrivate::dragOverThreshold(event->pos().x() - d->pressPoint.x(), Qt::XAxis, event));
        else
            setKeepMouseGrab(QQuickWindowPrivate::dragOverThreshold(event->pos().y() - d->pressPoint.y(), Qt::YAxis, event));
    }
    if (keepMouseGrab()) {
        qreal pos = d->positionAt(event->pos());
        if (d->snapMode == SnapAlways)
            pos = d->snapPosition(pos);
        setPosition(pos);
    }
    event->accept();
}

void QQuickSlider::mouseReleaseEvent(QMouseEvent *event)
{
    Q_D(QQuickSlider);
    QQuickControl::mouseReleaseEvent(event);
    d->pressPoint = QPoint();
    if (keepMouseGrab()) {
        qreal pos = d->positionAt(event->pos());
        if (d->snapMode != NoSnap)
            pos = d->snapPosition(pos);
        setValue(pos);
        setKeepMouseGrab(false);
    }
    setPressed(false);
    event->accept();
}

void QQuickSlider::mouseUngrabEvent()
{
    Q_D(QQuickSlider);
    QQuickControl::mouseUngrabEvent();
    d->pressPoint = QPoint();
    setPressed(false);
}

void QQuickSlider::mirrorChange()
{
    emit effectiveLayoutDirectionChanged();
    emit visualPositionChanged();
}

QT_END_NAMESPACE
