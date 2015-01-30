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

#include "qquickabstractbutton_p.h"
#include "qquickabstractbutton_p_p.h"

QT_BEGIN_NAMESPACE

/*!
    \qmltype Button
    \inherits Control
    \instantiates QQuickAbstractButton
    \inqmlmodule QtQuick.Controls
    \ingroup buttons
    \brief A button control.

    TODO
*/

QQuickAbstractButtonPrivate::QQuickAbstractButtonPrivate() :
    pressed(false), label(Q_NULLPTR)
{
}

QQuickAbstractButton::QQuickAbstractButton(QQuickItem *parent) :
    QQuickControl(*(new QQuickAbstractButtonPrivate), parent)
{
    setActiveFocusOnTab(true);
    setAcceptedMouseButtons(Qt::LeftButton);
}

QQuickAbstractButton::QQuickAbstractButton(QQuickAbstractButtonPrivate &dd, QQuickItem *parent) :
    QQuickControl(dd, parent)
{
    setActiveFocusOnTab(true);
    setAcceptedMouseButtons(Qt::LeftButton);
}

/*!
    \qmlproperty string QtQuickControls2::Button::text

    TODO
*/
QString QQuickAbstractButton::text() const
{
    Q_D(const QQuickAbstractButton);
    return d->text;
}

void QQuickAbstractButton::setText(const QString &text)
{
    Q_D(QQuickAbstractButton);
    if (d->text != text) {
        d->text = text;
        emit textChanged();
    }
}

/*!
    \qmlproperty bool QtQuickControls2::Button::pressed

    TODO
*/
bool QQuickAbstractButton::isPressed() const
{
    Q_D(const QQuickAbstractButton);
    return d->pressed;
}

void QQuickAbstractButton::setPressed(bool pressed)
{
    Q_D(QQuickAbstractButton);
    if (d->pressed != pressed) {
        d->pressed = pressed;
        emit pressedChanged();
    }
}

/*!
    \qmlproperty Item QtQuickControls2::Button::label

    TODO
*/
QQuickItem *QQuickAbstractButton::label() const
{
    Q_D(const QQuickAbstractButton);
    return d->label;
}

void QQuickAbstractButton::setLabel(QQuickItem *label)
{
    Q_D(QQuickAbstractButton);
    if (d->label != label) {
        delete d->label;
        d->label = label;
        if (label && !label->parentItem())
            label->setParentItem(this);
        emit labelChanged();
    }
}

void QQuickAbstractButton::focusOutEvent(QFocusEvent *event)
{
    QQuickControl::focusOutEvent(event);
    setPressed(false);
}

void QQuickAbstractButton::keyPressEvent(QKeyEvent *event)
{
    QQuickControl::keyPressEvent(event);
    if (event->key() == Qt::Key_Space) {
        setPressed(true);
        event->accept();
    }
}

void QQuickAbstractButton::keyReleaseEvent(QKeyEvent *event)
{
    QQuickControl::keyReleaseEvent(event);
    if (event->key() == Qt::Key_Space) {
        emit clicked();
        setPressed(false);
        event->accept();
    }
}

void QQuickAbstractButton::mousePressEvent(QMouseEvent *event)
{
    QQuickControl::mousePressEvent(event);
    setPressed(true);
    event->accept();
}

void QQuickAbstractButton::mouseMoveEvent(QMouseEvent *event)
{
    QQuickControl::mouseMoveEvent(event);
    setPressed(contains(event->pos()));
    event->accept();
}

void QQuickAbstractButton::mouseReleaseEvent(QMouseEvent *event)
{
    QQuickControl::mouseReleaseEvent(event);
    if (contains(event->pos()))
        emit clicked();
    setPressed(false);
    event->accept();
}

void QQuickAbstractButton::mouseUngrabEvent()
{
    QQuickControl::mouseUngrabEvent();
    setPressed(false);
}

QT_END_NAMESPACE
