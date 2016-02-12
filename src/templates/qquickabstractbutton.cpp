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
#include "qquickbuttongroup_p.h"

#include <QtGui/qstylehints.h>
#include <QtGui/qguiapplication.h>
#include <QtQuick/private/qquickevents_p_p.h>
#include <QtQml/qqmllist.h>

QT_BEGIN_NAMESPACE

// copied from qabstractbutton.cpp
static const int AUTO_REPEAT_DELAY = 300;
static const int AUTO_REPEAT_INTERVAL = 100;

/*!
    \qmltype AbstractButton
    \inherits Control
    \instantiates QQuickAbstractButton
    \inqmlmodule Qt.labs.controls
    \ingroup qtlabscontrols-buttons
    \brief The base of all button controls.

    AbstractButton provides the interface for controls with button-like
    behavior; for example, push buttons and checkable controls like
    radio buttons and check boxes. As an abstract control, it has no delegate
    implementations, leaving them to the types that derive from it.

    TODO: ButtonGroup usage

    \labs
*/

/*!
    \qmlsignal Qt.labs.controls::AbstractButton::pressed()

    This signal is emitted when the button is interactively pressed by the user.
*/

/*!
    \qmlsignal Qt.labs.controls::AbstractButton::released()

    This signal is emitted when the button is interactively released by the user.
*/

/*!
    \qmlsignal Qt.labs.controls::AbstractButton::canceled()

    This signal is emitted when the button loses mouse grab
    while being pressed, or when it would emit the \l released
    signal but the mouse cursor is not inside the button.
*/

/*!
    \qmlsignal Qt.labs.controls::AbstractButton::clicked()

    This signal is emitted when the button is interactively clicked by the user.
*/

/*!
    \qmlsignal Qt.labs.controls::AbstractButton::pressAndHold()

    This signal is emitted when the button is interactively perssed and held down by the user.
*/

/*!
    \qmlsignal Qt.labs.controls::AbstractButton::doubleClicked()

    This signal is emitted when the button is interactively double clicked by the user.
*/

QQuickAbstractButtonPrivate::QQuickAbstractButtonPrivate() :
    pressed(false), checked(false), checkable(false), highlighted(false), autoExclusive(false), autoRepeat(false), wasHeld(false),
    holdTimer(0), delayTimer(0), repeatTimer(0), repeatButton(Qt::NoButton),
    label(Q_NULLPTR), indicator(Q_NULLPTR), group(Q_NULLPTR)
{
}

bool QQuickAbstractButtonPrivate::isPressAndHoldConnected()
{
    Q_Q(QQuickAbstractButton);
    IS_SIGNAL_CONNECTED(q, QQuickAbstractButton, pressAndHold, ());
}

void QQuickAbstractButtonPrivate::startPressAndHold()
{
    Q_Q(QQuickAbstractButton);
    wasHeld = false;
    stopPressAndHold();
    if (isPressAndHoldConnected())
        holdTimer = q->startTimer(QGuiApplication::styleHints()->mousePressAndHoldInterval());
}

void QQuickAbstractButtonPrivate::stopPressAndHold()
{
    Q_Q(QQuickAbstractButton);
    if (holdTimer > 0) {
        q->killTimer(holdTimer);
        holdTimer = 0;
    }
}

void QQuickAbstractButtonPrivate::startRepeatDelay()
{
    Q_Q(QQuickAbstractButton);
    stopPressRepeat();
    delayTimer = q->startTimer(AUTO_REPEAT_DELAY);
}

void QQuickAbstractButtonPrivate::startPressRepeat()
{
    Q_Q(QQuickAbstractButton);
    stopPressRepeat();
    repeatTimer = q->startTimer(AUTO_REPEAT_INTERVAL);
}

void QQuickAbstractButtonPrivate::stopPressRepeat()
{
    Q_Q(QQuickAbstractButton);
    if (delayTimer > 0) {
        q->killTimer(delayTimer);
        delayTimer = 0;
    }
    if (repeatTimer > 0) {
        q->killTimer(repeatTimer);
        repeatTimer = 0;
    }
}

QQuickAbstractButton *QQuickAbstractButtonPrivate::findCheckedButton() const
{
    Q_Q(const QQuickAbstractButton);
    if (group)
        return qobject_cast<QQuickAbstractButton *>(group->checkedButton());

    QList<QQuickAbstractButton *> buttons = findExclusiveButtons();
    // TODO: A singular QRadioButton can be unchecked, which seems logical,
    // because there's nothing to be exclusive with. However, a RadioButton
    // from QtQuick.Controls 1.x can never be unchecked, which is the behavior
    // that QQuickRadioButton adopted. Uncommenting the following count check
    // gives the QRadioButton behavior. Notice that tst_radiobutton.qml needs
    // to be updated.
    if (!autoExclusive /*|| buttons.count() == 1*/)
        return Q_NULLPTR;

    foreach (QQuickAbstractButton *button, buttons) {
        if (button->isChecked() && button != q)
            return button;
    }
    return checked ? const_cast<QQuickAbstractButton *>(q) : Q_NULLPTR;
}

QList<QQuickAbstractButton *> QQuickAbstractButtonPrivate::findExclusiveButtons() const
{
    QList<QQuickAbstractButton *> buttons;
    if (group) {
        QQmlListProperty<QQuickAbstractButton> groupButtons = group->buttons();
        int count = groupButtons.count(&groupButtons);
        for (int i = 0; i < count; ++i) {
            QQuickAbstractButton *button = qobject_cast<QQuickAbstractButton *>(groupButtons.at(&groupButtons, i));
            if (button)
                buttons += button;
        }
    } else if (parentItem) {
        foreach (QQuickItem *child, parentItem->childItems()) {
            QQuickAbstractButton *button = qobject_cast<QQuickAbstractButton *>(child);
            if (button && button->autoExclusive() && !QQuickAbstractButtonPrivate::get(button)->group)
                buttons += button;
        }
    }
    return buttons;
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

QQuickAbstractButton::~QQuickAbstractButton()
{
    Q_D(QQuickAbstractButton);
    if (d->group)
        d->group->removeButton(this);
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
    \qmlproperty bool Qt.labs.controls::AbstractButton::checked

    This property holds whether the button is checked.
*/
bool QQuickAbstractButton::isChecked() const
{
    Q_D(const QQuickAbstractButton);
    return d->checked;
}

void QQuickAbstractButton::setChecked(bool checked)
{
    Q_D(QQuickAbstractButton);
    if (checked && !d->checkable)
        setCheckable(true);

    if (d->checked != checked) {
        d->checked = checked;
        setAccessibleProperty("checked", checked);
        checkStateSet();
        emit checkedChanged();
    }
}

/*!
    \qmlproperty bool Qt.labs.controls::AbstractButton::checkable

    This property holds whether the button is checkable.
*/
bool QQuickAbstractButton::isCheckable() const
{
    Q_D(const QQuickAbstractButton);
    return d->checkable;
}

void QQuickAbstractButton::setCheckable(bool checkable)
{
    Q_D(QQuickAbstractButton);
    if (d->checkable != checkable) {
        d->checkable = checkable;
        setAccessibleProperty("checkable", checkable);
        emit checkableChanged();
    }
}

/*!
    \qmlproperty bool Qt.labs.controls::AbstractButton::highlighted

    This property holds whether the button is highlighted.

    A button can be highlighted in order to draw the user's attention towards
    it. It has no effect on keyboard interaction.

    The default value is \c false.
*/
bool QQuickAbstractButton::isHighlighted() const
{
    Q_D(const QQuickAbstractButton);
    return d->highlighted;
}

void QQuickAbstractButton::setHighlighted(bool highlighted)
{
    Q_D(QQuickAbstractButton);
    if (highlighted != d->highlighted) {
        d->highlighted = highlighted;
        emit highlightedChanged();
    }
}

/*!
    \qmlproperty bool Qt.labs.controls::AbstractButton::autoExclusive

    This property holds whether auto-exclusivity is enabled.

    If auto-exclusivity is enabled, checkable buttons that belong to the same
    parent item behave as if they were part of the same ButtonGroup. Only
    one button can be checked at any time; checking another button automatically
    unchecks the previously checked one.

    \note The property has no effect on buttons that belong to an ButtonGroup.

    RadioButton and TabButton are auto-exclusive by default.
*/
bool QQuickAbstractButton::autoExclusive() const
{
    Q_D(const QQuickAbstractButton);
    return d->autoExclusive;
}

void QQuickAbstractButton::setAutoExclusive(bool exclusive)
{
    Q_D(QQuickAbstractButton);
    if (d->autoExclusive != exclusive) {
        d->autoExclusive = exclusive;
        emit autoExclusiveChanged();
    }
}

/*!
    \qmlproperty bool Qt.labs.controls::AbstractButton::autoRepeat

    This property holds whether the button repeats pressed(), released()
    and clicked() signals while the button is pressed and held down.

    The default value is \c false.
*/
bool QQuickAbstractButton::autoRepeat() const
{
    Q_D(const QQuickAbstractButton);
    return d->autoRepeat;
}

void QQuickAbstractButton::setAutoRepeat(bool repeat)
{
    Q_D(QQuickAbstractButton);
    if (d->autoRepeat != repeat) {
        d->stopPressRepeat();
        d->autoRepeat = repeat;
        emit autoRepeatChanged();
    }
}

/*!
    \qmlproperty Item Qt.labs.controls::AbstractButton::indicator

    This property holds the indicator item.
*/
QQuickItem *QQuickAbstractButton::indicator() const
{
    Q_D(const QQuickAbstractButton);
    return d->indicator;
}

void QQuickAbstractButton::setIndicator(QQuickItem *indicator)
{
    Q_D(QQuickAbstractButton);
    if (d->indicator != indicator) {
        delete d->indicator;
        d->indicator = indicator;
        if (indicator) {
            if (!indicator->parentItem())
                indicator->setParentItem(this);
            indicator->setAcceptedMouseButtons(Qt::LeftButton);
        }
        emit indicatorChanged();
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

/*!
    \qmlmethod void Qt.labs.controls::Button::toggle()

    Toggles the checked state of the button.
*/
void QQuickAbstractButton::toggle()
{
    Q_D(QQuickAbstractButton);
    setChecked(!d->checked);
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
    Q_D(QQuickAbstractButton);
    QQuickControl::keyPressEvent(event);
    if (event->key() == Qt::Key_Space) {
        setPressed(true);
        d->pressPoint = QPoint(qRound(width() / 2), qRound(height() / 2));

        if (d->autoRepeat) {
            d->startRepeatDelay();
            d->repeatButton = Qt::NoButton;
        }

        emit pressed();
    }
}

void QQuickAbstractButton::keyReleaseEvent(QKeyEvent *event)
{
    Q_D(QQuickAbstractButton);
    QQuickControl::keyReleaseEvent(event);
    if (event->key() == Qt::Key_Space) {
        setPressed(false);

        emit released();
        emit clicked();
        nextCheckState();

        if (d->autoRepeat)
            d->stopPressRepeat();
    }
}

void QQuickAbstractButton::mousePressEvent(QMouseEvent *event)
{
    Q_D(QQuickAbstractButton);
    QQuickControl::mousePressEvent(event);
    setPressed(true);
    d->pressPoint = event->pos();

    emit pressed();

    if (d->autoRepeat) {
        d->startRepeatDelay();
        d->repeatButton = event->button();
    } else if (Qt::LeftButton == (event->buttons() & Qt::LeftButton)) {
        d->startPressAndHold();
    } else {
        d->stopPressAndHold();
    }
}

void QQuickAbstractButton::mouseMoveEvent(QMouseEvent *event)
{
    Q_D(QQuickAbstractButton);
    QQuickControl::mouseMoveEvent(event);
    setPressed(contains(event->pos()));

    if (d->autoRepeat)
        d->stopPressRepeat();
    else if (d->holdTimer > 0 && QLineF(d->pressPoint, event->localPos()).length() > QGuiApplication::styleHints()->startDragDistance())
        d->stopPressAndHold();
}

void QQuickAbstractButton::mouseReleaseEvent(QMouseEvent *event)
{
    Q_D(QQuickAbstractButton);
    QQuickControl::mouseReleaseEvent(event);
    bool wasPressed = d->pressed;
    setPressed(false);

    if (wasPressed) {
        emit released();
        if (!d->wasHeld)
            emit clicked();
    } else {
        emit canceled();
    }

    if (contains(event->pos()))
        nextCheckState();

    if (d->autoRepeat)
        d->stopPressRepeat();
    else
        d->stopPressAndHold();
}

void QQuickAbstractButton::mouseDoubleClickEvent(QMouseEvent *event)
{
    QQuickControl::mouseDoubleClickEvent(event);
    emit doubleClicked();
}

void QQuickAbstractButton::mouseUngrabEvent()
{
    Q_D(QQuickAbstractButton);
    QQuickControl::mouseUngrabEvent();
    if (d->pressed) {
        setPressed(false);
        d->stopPressRepeat();
        d->stopPressAndHold();
        emit canceled();
    }
}

void QQuickAbstractButton::timerEvent(QTimerEvent *event)
{
    Q_D(QQuickAbstractButton);
    QQuickControl::timerEvent(event);
    if (event->timerId() == d->holdTimer) {
        d->stopPressAndHold();
        d->wasHeld = true;
        emit pressAndHold();
    } else if (event->timerId() == d->delayTimer) {
        d->startPressRepeat();
    } else if (event->timerId() == d->repeatTimer) {
        emit released();
        emit clicked();
        emit pressed();
    }
}

void QQuickAbstractButton::checkStateSet()
{
    Q_D(QQuickAbstractButton);
    if (d->checked) {
        QQuickAbstractButton *button = d->findCheckedButton();
        if (button && button != this)
            button->setChecked(false);
    }
}

void QQuickAbstractButton::nextCheckState()
{
    Q_D(QQuickAbstractButton);
    if (d->checkable && (!d->checked || d->findCheckedButton() != this))
        setChecked(!d->checked);
}

#ifndef QT_NO_ACCESSIBILITY
void QQuickAbstractButton::accessibilityActiveChanged(bool active)
{
    QQuickControl::accessibilityActiveChanged(active);

    Q_D(QQuickAbstractButton);
    if (active) {
        setAccessibleName(d->text);
        setAccessibleProperty("pressed", d->pressed);
        setAccessibleProperty("checked", d->checked);
        setAccessibleProperty("checkable", d->checkable);
    }
}

QAccessible::Role QQuickAbstractButton::accessibleRole() const
{
    return QAccessible::Button;
}
#endif

QT_END_NAMESPACE
