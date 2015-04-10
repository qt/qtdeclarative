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

#include "qquickbutton_p.h"
#include "qquickbutton_p_p.h"

QT_BEGIN_NAMESPACE

/*!
    \qmltype Button
    \inherits Control
    \instantiates QQuickButton
    \inqmlmodule QtQuick.Controls
    \ingroup buttons
    \brief A button control.

    TODO
*/

QQuickButtonPrivate::QQuickButtonPrivate() :
    pressed(false), label(Q_NULLPTR)
{
}

QQuickButton::QQuickButton(QQuickItem *parent) :
    QQuickControl(*(new QQuickButtonPrivate), parent)
{
    setActiveFocusOnTab(true);
    setAcceptedMouseButtons(Qt::LeftButton);
}

QQuickButton::QQuickButton(QQuickButtonPrivate &dd, QQuickItem *parent) :
    QQuickControl(dd, parent)
{
    setActiveFocusOnTab(true);
    setAcceptedMouseButtons(Qt::LeftButton);
}

/*!
    \qmlproperty string QtQuickControls2::Button::text

    TODO
*/
QString QQuickButton::text() const
{
    Q_D(const QQuickButton);
    return d->text;
}

void QQuickButton::setText(const QString &text)
{
    Q_D(QQuickButton);
    if (d->text != text) {
        d->text = text;
        emit textChanged();
    }
}

/*!
    \qmlproperty bool QtQuickControls2::Button::pressed

    TODO
*/
bool QQuickButton::isPressed() const
{
    Q_D(const QQuickButton);
    return d->pressed;
}

void QQuickButton::setPressed(bool pressed)
{
    Q_D(QQuickButton);
    if (d->pressed != pressed) {
        d->pressed = pressed;
        emit pressedChanged();
    }
}

/*!
    \qmlproperty Item QtQuickControls2::Button::label

    TODO
*/
QQuickItem *QQuickButton::label() const
{
    Q_D(const QQuickButton);
    return d->label;
}

void QQuickButton::setLabel(QQuickItem *label)
{
    Q_D(QQuickButton);
    if (d->label != label) {
        delete d->label;
        d->label = label;
        if (label && !label->parentItem())
            label->setParentItem(this);
        emit labelChanged();
    }
}

void QQuickButton::focusOutEvent(QFocusEvent *event)
{
    QQuickControl::focusOutEvent(event);
    setPressed(false);
}

void QQuickButton::keyPressEvent(QKeyEvent *event)
{
    QQuickControl::keyPressEvent(event);
    if (event->key() == Qt::Key_Space) {
        setPressed(true);
        event->accept();
    }
}

void QQuickButton::keyReleaseEvent(QKeyEvent *event)
{
    QQuickControl::keyReleaseEvent(event);
    if (event->key() == Qt::Key_Space) {
        emit clicked();
        setPressed(false);
        event->accept();
    }
}

void QQuickButton::mousePressEvent(QMouseEvent *event)
{
    QQuickControl::mousePressEvent(event);
    setPressed(true);
    event->accept();
}

void QQuickButton::mouseMoveEvent(QMouseEvent *event)
{
    QQuickControl::mouseMoveEvent(event);
    setPressed(contains(event->pos()));
    event->accept();
}

void QQuickButton::mouseReleaseEvent(QMouseEvent *event)
{
    QQuickControl::mouseReleaseEvent(event);
    if (contains(event->pos()))
        emit clicked();
    setPressed(false);
    event->accept();
}

void QQuickButton::mouseUngrabEvent()
{
    QQuickControl::mouseUngrabEvent();
    setPressed(false);
}

QT_END_NAMESPACE
