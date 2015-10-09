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

#include "qquickabstractbutton_p.h"
#include "qquickabstractbutton_p_p.h"

#include <QtQuick/private/qquickevents_p_p.h>

QT_BEGIN_NAMESPACE

/*!
    \qmltype AbstractButton
    \inherits Control
    \instantiates QQuickAbstractButton
    \inqmlmodule Qt.labs.controls
    \qmlabstract
    \internal
*/

/*!
    \qmlsignal Qt.labs.controls::AbstractButton::pressed(MouseEvent mouse)

    This signal is emitted when the button is interactively pressed by the user.

    The mouse parameter provides information about the press, including the x
    and y position and which button was pressed.
*/

/*!
    \qmlsignal Qt.labs.controls::AbstractButton::released(MouseEvent mouse)

    This signal is emitted when the button is interactively released by the user.

    The mouse parameter provides information about the click, including the x
    and y position of the release of the click, and whether the click was held.
*/

/*!
    \qmlsignal Qt.labs.controls::AbstractButton::canceled()

    This signal is emitted when the button loses mouse grab
    while being pressed, or when it would emit the \l released
    signal but the mouse cursor is not inside the button.
*/

/*!
    \qmlsignal Qt.labs.controls::AbstractButton::clicked(MouseEvent mouse)

    This signal is emitted when the button is interactively clicked by the user.

    The mouse parameter provides information about the click, including the x
    and y position of the release of the click, and whether the click was held.
*/

/*!
    \qmlsignal Qt.labs.controls::AbstractButton::doubleClicked(MouseEvent mouse)

    This signal is emitted when the button is interactively double clicked by the user.

    The mouse parameter provides information about the click, including the x
    and y position of the release of the click, and whether the click was held.
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
    \qmlproperty string Qt.labs.controls::AbstractButton::text

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
    \qmlproperty bool Qt.labs.controls::AbstractButton::pressed

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
    \qmlproperty Item Qt.labs.controls::AbstractButton::label

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

        QQuickMouseEvent me(width() / 2, height() / 2, Qt::NoButton, Qt::NoButton, event->modifiers());
        emit pressed(&me);
        event->setAccepted(me.isAccepted());
    }
}

void QQuickAbstractButton::keyReleaseEvent(QKeyEvent *event)
{
    QQuickControl::keyReleaseEvent(event);
    if (event->key() == Qt::Key_Space) {
        setPressed(false);

        QQuickMouseEvent mre(width() / 2, height() / 2, Qt::NoButton, Qt::NoButton, event->modifiers());
        emit released(&mre);
        QQuickMouseEvent mce(width() / 2, height() / 2, Qt::NoButton, Qt::NoButton, event->modifiers(), true /* isClick */);
        emit clicked(&mce);
        event->setAccepted(mre.isAccepted() || mce.isAccepted());
    }
}

void QQuickAbstractButton::mousePressEvent(QMouseEvent *event)
{
    QQuickControl::mousePressEvent(event);
    setPressed(true);

    QQuickMouseEvent me(event->x(), event->y(), event->button(), event->buttons(), event->modifiers());
    emit pressed(&me);
    event->setAccepted(me.isAccepted());
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
        QQuickMouseEvent mre(event->x(), event->y(), event->button(), event->buttons(), event->modifiers());
        emit released(&mre);
        QQuickMouseEvent mce(event->x(), event->y(), event->button(), event->buttons(), event->modifiers(), true /* isClick */);
        emit clicked(&mce);
        event->setAccepted(mre.isAccepted() || mce.isAccepted());
    } else {
        emit canceled();
    }
}

void QQuickAbstractButton::mouseDoubleClickEvent(QMouseEvent *event)
{
    QQuickControl::mouseDoubleClickEvent(event);

    QQuickMouseEvent me(event->x(), event->y(), event->button(), event->buttons(), event->modifiers(), true /* isClick */);
    emit doubleClicked(&me);
    event->setAccepted(me.isAccepted());
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
