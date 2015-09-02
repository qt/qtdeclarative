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
    \qmltype AbstractButton
    \inherits Control
    \instantiates QQuickAbstractButton
    \inqmlmodule QtQuick.Controls
    \qmlabstract
    \internal
*/

/*!
    \qmlsignal QtQuickControls2::AbstractButton::pressed()

    This signal is emitted when the button is interactively pressed by the user.
*/

/*!
    \qmlsignal QtQuickControls2::AbstractButton::released()

    This signal is emitted when the button is interactively released by the user.
*/

/*!
    \qmlsignal QtQuickControls2::AbstractButton::canceled()

    This signal is emitted when the button loses mouse grab
    while being pressed, or when it would emit the \l released
    signal but the mouse cursor is not inside the button.
*/

/*!
    \qmlsignal QtQuickControls2::AbstractButton::clicked()

    This signal is emitted when the button is clicked.
*/

QQuickAbstractButtonPrivate::QQuickAbstractButtonPrivate() : pressed(false), label(Q_NULLPTR)
{
}

QQuickAbstractButton::QQuickAbstractButton(QQuickItem *parent) :
    QQuickControl(*(new QQuickAbstractButtonPrivate), parent)
{
    setActiveFocusOnTab(true);
    setAcceptedMouseButtons(Qt::LeftButton);
    setAccessibleRole(0x0000002B); //QAccessible::Button
}

QQuickAbstractButton::QQuickAbstractButton(QQuickAbstractButtonPrivate &dd, QQuickItem *parent) :
    QQuickControl(dd, parent)
{
    setActiveFocusOnTab(true);
    setAcceptedMouseButtons(Qt::LeftButton);
    setAccessibleRole(0x0000002B); //QAccessible::Button
}

/*!
    \qmlproperty string QtQuickControls2::AbstractButton::text

    This property holds a textual description of the button.

    \note The text is used for accessibility purposes, so it makes sense to
          set a textual description even if the label item is an image.

    \sa label
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
        setAccessibleName(text);
        emit textChanged();
    }
}

/*!
    \qmlproperty bool QtQuickControls2::AbstractButton::pressed

    This property holds whether the button is pressed.
*/
bool QQuickAbstractButton::isPressed() const
{
    Q_D(const QQuickAbstractButton);
    return d->pressed;
}

void QQuickAbstractButton::setPressed(bool isPressed)
{
    Q_D(QQuickAbstractButton);
    if (d->pressed != isPressed) {
        d->pressed = isPressed;
        setAccessibleProperty("pressed", isPressed);
        emit pressedChanged();
    }
}

/*!
    \qmlproperty Item QtQuickControls2::AbstractButton::label

    This property holds the label item.

    \sa text
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
    Q_D(QQuickAbstractButton);
    QQuickControl::focusOutEvent(event);
    if (d->pressed) {
        setPressed(false);
        emit canceled();
    }
}

void QQuickAbstractButton::keyPressEvent(QKeyEvent *event)
{
    QQuickControl::keyPressEvent(event);
    if (event->key() == Qt::Key_Space) {
        setPressed(true);
        emit pressed();
        event->accept();
    }
}

void QQuickAbstractButton::keyReleaseEvent(QKeyEvent *event)
{
    QQuickControl::keyReleaseEvent(event);
    if (event->key() == Qt::Key_Space) {
        setPressed(false);
        emit released();
        emit clicked();
        event->accept();
    }
}

void QQuickAbstractButton::mousePressEvent(QMouseEvent *event)
{
    QQuickControl::mousePressEvent(event);
    setPressed(true);
    emit pressed();
}

void QQuickAbstractButton::mouseMoveEvent(QMouseEvent *event)
{
    QQuickControl::mouseMoveEvent(event);
    setPressed(contains(event->pos()));
}

void QQuickAbstractButton::mouseReleaseEvent(QMouseEvent *event)
{
    Q_D(QQuickAbstractButton);
    QQuickControl::mouseReleaseEvent(event);
    bool wasPressed = d->pressed;
    setPressed(false);
    if (wasPressed) {
        emit released();
        emit clicked();
    } else {
        emit canceled();
    }
}

void QQuickAbstractButton::mouseUngrabEvent()
{
    Q_D(QQuickAbstractButton);
    QQuickControl::mouseUngrabEvent();
    if (d->pressed) {
        setPressed(false);
        emit canceled();
    }
}

QT_END_NAMESPACE
