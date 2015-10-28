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

#include "qquickspinbox_p.h"
#include "qquickcontrol_p_p.h"

#include <QtGui/qguiapplication.h>
#include <QtGui/qstylehints.h>

QT_BEGIN_NAMESPACE

// copied from qabstractbutton.cpp
static const int AUTO_REPEAT_DELAY = 300;
static const int AUTO_REPEAT_INTERVAL = 100;

/*!
    \qmltype SpinBox
    \inherits Control
    \instantiates QQuickSpinBox
    \inqmlmodule Qt.labs.controls
    \ingroup input
    \brief A spinbox control.

    \image qtlabscontrols-spinbox.png

    SpinBox allows the user to choose an integer value by clicking the up
    or down indicator buttons, by pressing up or down on the keyboard, or
    by entering a text value in the input field.

    By default, SpinBox provides discrete values in the range of \c [0-99]
    with a \l stepSize of \c 1.

    \snippet qtlabscontrols-spinbox.qml 1

    \sa Tumbler, {Customizing SpinBox}
*/

class QQuickSpinBoxPrivate : public QQuickControlPrivate
{
    Q_DECLARE_PUBLIC(QQuickSpinBox)

public:
    QQuickSpinBoxPrivate() : from(0), to(99), value(0), stepSize(1),
        delayTimer(0), repeatTimer(0), up(Q_NULLPTR), down(Q_NULLPTR) { }

    int boundValue(int value) const;
    void updateValue();

    int effectiveStepSize() const;

    void startRepeatDelay();
    void startPressRepeat();
    void stopPressRepeat();

    bool handleMousePressEvent(QQuickItem *child, QMouseEvent *event);
    bool handleMouseMoveEvent(QQuickItem *child, QMouseEvent *event);
    bool handleMouseReleaseEvent(QQuickItem *child, QMouseEvent *event);
    bool handleMouseUngrabEvent(QQuickItem *child);

    int from;
    int to;
    int value;
    int stepSize;
    int delayTimer;
    int repeatTimer;
    QQuickSpinner *up;
    QQuickSpinner *down;
    QLocale locale;
};

int QQuickSpinBoxPrivate::boundValue(int value) const
{
    return from > to ? qBound(to, value, from) : qBound(from, value, to);
}

void QQuickSpinBoxPrivate::updateValue()
{
    Q_Q(QQuickSpinBox);
    if (contentItem) {
        QVariant text = contentItem->property("text");
        if (text.isValid())
            q->setValue(locale.toInt(text.toString()));
    }
}

int QQuickSpinBoxPrivate::effectiveStepSize() const
{
    return from > to ? -1 * stepSize : stepSize;
}

void QQuickSpinBoxPrivate::startRepeatDelay()
{
    Q_Q(QQuickSpinBox);
    stopPressRepeat();
    delayTimer = q->startTimer(AUTO_REPEAT_DELAY);
}

void QQuickSpinBoxPrivate::startPressRepeat()
{
    Q_Q(QQuickSpinBox);
    stopPressRepeat();
    repeatTimer = q->startTimer(AUTO_REPEAT_INTERVAL);
}

void QQuickSpinBoxPrivate::stopPressRepeat()
{
    Q_Q(QQuickSpinBox);
    if (delayTimer > 0) {
        q->killTimer(delayTimer);
        delayTimer = 0;
    }
    if (repeatTimer > 0) {
        q->killTimer(repeatTimer);
        repeatTimer = 0;
    }
}

bool QQuickSpinBoxPrivate::handleMousePressEvent(QQuickItem *child, QMouseEvent *)
{
    Q_Q(QQuickSpinBox);
    QQuickItem *ui = up->indicator();
    QQuickItem *di = down->indicator();
    up->setPressed(child == ui);
    down->setPressed(child == di);

    bool pressed = up->isPressed() || down->isPressed();
    q->setAccessibleProperty("pressed", pressed);
    if (pressed)
        startRepeatDelay();
    return child == ui || child == di;
}

bool QQuickSpinBoxPrivate::handleMouseMoveEvent(QQuickItem *child, QMouseEvent *event)
{
    Q_Q(QQuickSpinBox);
    QQuickItem *ui = up->indicator();
    QQuickItem *di = down->indicator();
    up->setPressed(child == ui && ui->contains(event->pos()));
    down->setPressed(child == di && di->contains(event->pos()));

    bool pressed = up->isPressed() || down->isPressed();
    q->setAccessibleProperty("pressed", pressed);
    stopPressRepeat();
    return child == ui || child == di;
}

bool QQuickSpinBoxPrivate::handleMouseReleaseEvent(QQuickItem *child, QMouseEvent *event)
{
    Q_Q(QQuickSpinBox);
    QQuickItem *ui = up->indicator();
    QQuickItem *di = down->indicator();
    if (child == ui) {
        up->setPressed(false);
        if (repeatTimer <= 0 && ui->contains(event->pos()))
            q->increase();
    } else if (child == di) {
        down->setPressed(false);
        if (repeatTimer <= 0 && di->contains(event->pos()))
            q->decrease();
    }

    q->setAccessibleProperty("pressed", false);
    stopPressRepeat();
    return child == ui || child == di;
}

bool QQuickSpinBoxPrivate::handleMouseUngrabEvent(QQuickItem *child)
{
    Q_Q(QQuickSpinBox);
    QQuickItem *ui = up->indicator();
    QQuickItem *di = down->indicator();
    up->setPressed(false);
    down->setPressed(false);

    q->setAccessibleProperty("pressed", false);
    stopPressRepeat();
    return child == ui || child == di;
}

QQuickSpinBox::QQuickSpinBox(QQuickItem *parent) :
    QQuickControl(*(new QQuickSpinBoxPrivate), parent)
{
    Q_D(QQuickSpinBox);
    d->up = new QQuickSpinner(this);
    d->down = new QQuickSpinner(this);

    setFlag(ItemIsFocusScope);
    setFiltersChildMouseEvents(true);
}

/*!
    \qmlproperty int Qt.labs.controls::SpinBox::from

    This property holds the starting value for the range. The default value is \c 0.

    \sa to, value
*/
int QQuickSpinBox::from() const
{
    Q_D(const QQuickSpinBox);
    return d->from;
}

void QQuickSpinBox::setFrom(int from)
{
    Q_D(QQuickSpinBox);
    if (d->from != from) {
        d->from = from;
        emit fromChanged();
        if (isComponentComplete())
            setValue(d->value);
    }
}

/*!
    \qmlproperty int Qt.labs.controls::SpinBox::to

    This property holds the end value for the range. The default value is \c 99.

    \sa from, value
*/
int QQuickSpinBox::to() const
{
    Q_D(const QQuickSpinBox);
    return d->to;
}

void QQuickSpinBox::setTo(int to)
{
    Q_D(QQuickSpinBox);
    if (d->to != to) {
        d->to = to;
        emit toChanged();
        if (isComponentComplete())
            setValue(d->value);
    }
}

/*!
    \qmlproperty int Qt.labs.controls::SpinBox::value

    This property holds the value in the range \c from - \c to. The default value is \c 0.
*/
int QQuickSpinBox::value() const
{
    Q_D(const QQuickSpinBox);
    return d->value;
}

void QQuickSpinBox::setValue(int value)
{
    Q_D(QQuickSpinBox);
    if (isComponentComplete())
        value = d->boundValue(value);

    if (d->value != value) {
        d->value = value;
        emit valueChanged();
    }
}

/*!
    \qmlproperty int Qt.labs.controls::SpinBox::stepSize

    This property holds the step size. The default value is \c 1.

    \sa increase(), decrease()
*/
int QQuickSpinBox::stepSize() const
{
    Q_D(const QQuickSpinBox);
    return d->stepSize;
}

void QQuickSpinBox::setStepSize(int step)
{
    Q_D(QQuickSpinBox);
    if (d->stepSize != step) {
        d->stepSize = step;
        emit stepSizeChanged();
    }
}

/*!
    \qmlproperty Locale Qt.labs.controls::SpinBox::locale

    This property holds the locale that is used to format the value.
*/
QLocale QQuickSpinBox::locale() const
{
    Q_D(const QQuickSpinBox);
    return d->locale;
}

void QQuickSpinBox::setLocale(const QLocale &locale)
{
    Q_D(QQuickSpinBox);
    if (d->locale != locale) {
        d->locale = locale;
        emit localeChanged();
    }
}

/*!
    \qmlpropertygroup Qt.labs.controls::SpinBox::up
    \qmlproperty bool Qt.labs.controls::SpinBox::up.pressed
    \qmlproperty Item Qt.labs.controls::SpinBox::up.indicator

    These properties hold the up indicator item and whether it is pressed.

    \sa increase()
*/
QQuickSpinner *QQuickSpinBox::up() const
{
    Q_D(const QQuickSpinBox);
    return d->up;
}

/*!
    \qmlpropertygroup Qt.labs.controls::SpinBox::down
    \qmlproperty bool Qt.labs.controls::SpinBox::down.pressed
    \qmlproperty Item Qt.labs.controls::SpinBox::down.indicator

    These properties hold the down indicator item and whether it is pressed.

    \sa decrease()
*/
QQuickSpinner *QQuickSpinBox::down() const
{
    Q_D(const QQuickSpinBox);
    return d->down;
}

/*!
    \qmlmethod void Qt.labs.controls::SpinBox::increase()

    Increases the value by \l stepSize.

    \sa stepSize
*/
void QQuickSpinBox::increase()
{
    Q_D(QQuickSpinBox);
    setValue(d->value + d->effectiveStepSize());
}

/*!
    \qmlmethod void Qt.labs.controls::SpinBox::decrease()

    Decreases the value by \l stepSize.

    \sa stepSize
*/
void QQuickSpinBox::decrease()
{
    Q_D(QQuickSpinBox);
    setValue(d->value - d->effectiveStepSize());
}

void QQuickSpinBox::keyPressEvent(QKeyEvent *event)
{
    Q_D(QQuickSpinBox);
    QQuickControl::keyPressEvent(event);

    switch (event->key()) {
    case Qt::Key_Up:
        increase();
        d->up->setPressed(true);
        event->accept();
        break;

    case Qt::Key_Down:
        decrease();
        d->down->setPressed(true);
        event->accept();
        break;

    default:
        break;
    }

    setAccessibleProperty("pressed", d->up->isPressed() || d->down->isPressed());
}

void QQuickSpinBox::keyReleaseEvent(QKeyEvent *event)
{
    Q_D(QQuickSpinBox);
    QQuickControl::keyReleaseEvent(event);

    if (event->key() == Qt::Key_Enter || event->key() == Qt::Key_Return)
        d->updateValue();

    d->up->setPressed(false);
    d->down->setPressed(false);
    setAccessibleProperty("pressed", false);
}

bool QQuickSpinBox::childMouseEventFilter(QQuickItem *child, QEvent *event)
{
    Q_D(QQuickSpinBox);
    switch (event->type()) {
    case QEvent::MouseButtonPress:
        return d->handleMousePressEvent(child, static_cast<QMouseEvent *>(event));
    case QEvent::MouseMove:
        return d->handleMouseMoveEvent(child, static_cast<QMouseEvent *>(event));
    case QEvent::MouseButtonRelease:
        return d->handleMouseReleaseEvent(child, static_cast<QMouseEvent *>(event));
    case QEvent::UngrabMouse:
        return d->handleMouseUngrabEvent(child);
    default:
        return false;
    }
}

void QQuickSpinBox::timerEvent(QTimerEvent *event)
{
    Q_D(QQuickSpinBox);
    QQuickControl::timerEvent(event);
    if (event->timerId() == d->delayTimer) {
        d->startPressRepeat();
    } else if (event->timerId() == d->repeatTimer) {
        if (d->up->isPressed())
            increase();
        else if (d->down->isPressed())
            decrease();
    }
}

void QQuickSpinBox::itemChange(ItemChange change, const ItemChangeData &value)
{
    Q_D(QQuickSpinBox);
    QQuickControl::itemChange(change, value);
    if (change == ItemActiveFocusHasChanged && !value.boolValue)
        d->updateValue();
}

void QQuickSpinBox::contentItemChange(QQuickItem *newItem, QQuickItem *oldItem)
{
    Q_UNUSED(oldItem);
    if (newItem)
        newItem->setActiveFocusOnTab(true);
}

#ifndef QT_NO_ACCESSIBILITY
QAccessible::Role QQuickSpinBox::accessibleRole() const
{
    return QAccessible::SpinBox;
}
#endif

class QQuickSpinnerPrivate : public QObjectPrivate
{
public:
    QQuickSpinnerPrivate() : pressed(false), indicator(Q_NULLPTR) { }
    bool pressed;
    QQuickItem *indicator;
};

QQuickSpinner::QQuickSpinner(QQuickSpinBox *parent) :
    QObject(*(new QQuickSpinnerPrivate), parent)
{
}

bool QQuickSpinner::isPressed() const
{
    Q_D(const QQuickSpinner);
    return d->pressed;
}

void QQuickSpinner::setPressed(bool pressed)
{
    Q_D(QQuickSpinner);
    if (d->pressed != pressed) {
        d->pressed = pressed;
        emit pressedChanged();
    }
}

QQuickItem *QQuickSpinner::indicator() const
{
    Q_D(const QQuickSpinner);
    return d->indicator;
}

void QQuickSpinner::setIndicator(QQuickItem *indicator)
{
    Q_D(QQuickSpinner);
    if (d->indicator != indicator) {
        delete d->indicator;
        d->indicator = indicator;
        if (indicator) {
            if (!indicator->parentItem())
                indicator->setParentItem(static_cast<QQuickItem *>(parent()));
            indicator->setAcceptedMouseButtons(Qt::LeftButton);
        }
        emit indicatorChanged();
    }
}

QT_END_NAMESPACE
