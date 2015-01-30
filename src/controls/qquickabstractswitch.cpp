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

#include "qquickabstractswitch_p.h"
#include "qquickabstractcheckable_p_p.h"

#include <QtQuick/private/qquickwindow_p.h>

QT_BEGIN_NAMESPACE

/*!
    \qmltype Switch
    \inherits Checkable
    \instantiates QQuickAbstractSwitch
    \inqmlmodule QtQuick.Controls
    \ingroup buttons
    \brief A switch control.

    TODO
*/

class QQuickAbstractSwitchPrivate : public QQuickAbstractCheckablePrivate
{
    Q_DECLARE_PUBLIC(QQuickAbstractSwitch)

public:
    QQuickAbstractSwitchPrivate() : position(0) { }

    void updatePosition();

    qreal position;
    QPoint pressPoint;
};

void QQuickAbstractSwitchPrivate::updatePosition()
{
    Q_Q(QQuickAbstractSwitch);
    q->setPosition(checked ? 1.0 : 0.0);
}

QQuickAbstractSwitch::QQuickAbstractSwitch(QQuickItem *parent) :
    QQuickAbstractCheckable(*(new QQuickAbstractSwitchPrivate), parent)
{
    setFiltersChildMouseEvents(true);
    QObjectPrivate::connect(this, &QQuickAbstractCheckable::checkedChanged, d_func(), &QQuickAbstractSwitchPrivate::updatePosition);
}

/*!
    \qmlproperty real QtQuickControls2::Switch::position

    TODO
*/
qreal QQuickAbstractSwitch::position() const
{
    Q_D(const QQuickAbstractSwitch);
    return d->position;
}

void QQuickAbstractSwitch::setPosition(qreal position)
{
    Q_D(QQuickAbstractSwitch);
    position = qBound(0.0, position, 1.0);
    if (d->position != position) {
        d->position = position;
        emit positionChanged();
        emit visualPositionChanged();
    }
}

/*!
    \qmlproperty real QtQuickControls2::Switch::visualPosition

    TODO
*/
qreal QQuickAbstractSwitch::visualPosition() const
{
    Q_D(const QQuickAbstractSwitch);
    if (isMirrored())
        return 1.0 - d->position;
    return d->position;
}

void QQuickAbstractSwitch::mirrorChange()
{
    QQuickAbstractCheckable::mirrorChange();
    emit visualPositionChanged();
}

bool QQuickAbstractSwitch::childMouseEventFilter(QQuickItem *child, QEvent *event)
{
    if (child == indicator()) {
        switch (event->type()) {
        case QEvent::MouseButtonPress:
            return handleMousePressEvent(child, static_cast<QMouseEvent *>(event));
        case QEvent::MouseMove:
            return handleMouseMoveEvent(child, static_cast<QMouseEvent *>(event));
        case QEvent::MouseButtonRelease:
            return handleMouseReleaseEvent(child, static_cast<QMouseEvent *>(event));
        case QEvent::UngrabMouse:
            return handleMouseUngrabEvent(child);
        default:
            return false;
        }
    }
    return false;
}

bool QQuickAbstractSwitch::handleMousePressEvent(QQuickItem *child, QMouseEvent *event)
{
    Q_D(QQuickAbstractSwitch);
    Q_UNUSED(child);
    d->pressPoint = event->pos();
    setPressed(true);
    event->accept();
    return true;
}

bool QQuickAbstractSwitch::handleMouseMoveEvent(QQuickItem *child, QMouseEvent *event)
{
    Q_D(QQuickAbstractSwitch);
    if (!child->keepMouseGrab())
        child->setKeepMouseGrab(QQuickWindowPrivate::dragOverThreshold(event->pos().x() - d->pressPoint.x(), Qt::XAxis, event));
    if (child->keepMouseGrab()) {
        setPosition(positionAt(event->pos()));
        event->accept();
    }
    return true;
}

bool QQuickAbstractSwitch::handleMouseReleaseEvent(QQuickItem *child, QMouseEvent *event)
{
    Q_D(QQuickAbstractSwitch);
    d->pressPoint = QPoint();
    setPressed(false);
    if (child->keepMouseGrab()) {
        setChecked(position() > 0.5);
        setPosition(isChecked() ? 1.0 : 0.0);
        child->setKeepMouseGrab(false);
    } else {
        emit clicked();
        toggle();
    }
    event->accept();
    return true;
}

bool QQuickAbstractSwitch::handleMouseUngrabEvent(QQuickItem *child)
{
    Q_D(QQuickAbstractSwitch);
    Q_UNUSED(child);
    d->pressPoint = QPoint();
    setChecked(position() > 0.5);
    setPosition(isChecked() ? 1.0 : 0.0);
    setPressed(false);
    return true;
}

qreal QQuickAbstractSwitch::positionAt(const QPoint &point) const
{
    qreal pos = point.x() / indicator()->width();
    if (isMirrored())
        return 1.0 - pos;
    return pos;
}

QT_END_NAMESPACE
