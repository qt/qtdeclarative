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

    Button presents a push-button control that can be pushed or clicked by
    the user. Buttons are normally used to perform an action, or to answer
    a question. Typical buttons are \e OK, \e Apply, \e Cancel, \e Close,
    \e Yes, \e No and \e Help.

    \table
    \row \li \image qtquickcontrols2-button-normal.png
         \li A button in its normal state.
    \row \li \image qtquickcontrols2-button-pressed.png
         \li A button that is pressed.
    \row \li \image qtquickcontrols2-button-focused.png
         \li A button that has active focus.
    \row \li \image qtquickcontrols2-button-disabled.png
         \li A button that is disabled.
    \endtable

    \code
    RowLayout {
        Button {
            text: "Ok"
            onClicked: model.submit()
        }
        Button {
            text: "Cancel"
            onClicked: model.revert()
        }
    }
    \endcode

    \section1 Structure

    Button consists of two parts, \l {Control::background}{background} and
    \l {Button::label}{label}. Their implicit sizes are used to calculate
    the implicit size of the control.

    \section3 Background

    \image qtquickcontrols2-button-background.png

    The following snippet presents the default background item implementation.

    \snippet Button.qml background

    \section3 Label

    \image qtquickcontrols2-button-label.png

    The following snippet presents the default label item implementation.

    \snippet Button.qml label

    \section1 Theming

    \note ### TODO: Document what is used and how to customize.
*/

/*!
    \qmlsignal QtQuickControls2::Button::pressed()

    This signal is emitted when the button is interactively pressed by the user.
*/

/*!
    \qmlsignal QtQuickControls2::Button::released()

    This signal is emitted when the button is interactively released by the user.
*/

/*!
    \qmlsignal QtQuickControls2::Button::canceled()

    This signal is emitted when the button loses mouse grab
    while being pressed, or when it would emit the \l release
    signal but the mouse cursor is not inside the button.
*/

/*!
    \qmlsignal QtQuickControls2::Button::clicked()

    This signal is emitted when the button is clicked.
*/

QQuickButtonPrivate::QQuickButtonPrivate() : pressed(false), label(Q_NULLPTR)
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

    This property holds a textual description of the button.

    \note The text is used for accessibility purposes, so it makes sense to
          set a textual description even if the label item is an image.

    \sa label
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

    This property holds whether the button is pressed.
*/
bool QQuickButton::isPressed() const
{
    Q_D(const QQuickButton);
    return d->pressed;
}

void QQuickButton::setPressed(bool isPressed)
{
    Q_D(QQuickButton);
    if (d->pressed != isPressed) {
        d->pressed = isPressed;
        emit pressedChanged();
    }
}

/*!
    \qmlproperty Item QtQuickControls2::Button::label

    This property holds the label item.

    \sa text
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
    Q_D(QQuickButton);
    QQuickControl::focusOutEvent(event);
    if (d->pressed) {
        setPressed(false);
        emit canceled();
    }
}

void QQuickButton::keyPressEvent(QKeyEvent *event)
{
    QQuickControl::keyPressEvent(event);
    if (event->key() == Qt::Key_Space) {
        setPressed(true);
        emit pressed();
        event->accept();
    }
}

void QQuickButton::keyReleaseEvent(QKeyEvent *event)
{
    QQuickControl::keyReleaseEvent(event);
    if (event->key() == Qt::Key_Space) {
        setPressed(false);
        emit released();
        emit clicked();
        event->accept();
    }
}

void QQuickButton::mousePressEvent(QMouseEvent *event)
{
    QQuickControl::mousePressEvent(event);
    setPressed(true);
    emit pressed();
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
    Q_D(QQuickButton);
    QQuickControl::mouseReleaseEvent(event);
    bool wasPressed = d->pressed;
    setPressed(false);
    if (wasPressed) {
        emit released();
        emit clicked();
    } else {
        emit canceled();
    }
    event->accept();
}

void QQuickButton::mouseUngrabEvent()
{
    Q_D(QQuickButton);
    QQuickControl::mouseUngrabEvent();
    if (d->pressed) {
        setPressed(false);
        emit canceled();
    }
}

QT_END_NAMESPACE
