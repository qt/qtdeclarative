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

#include "qquickabstractspinbox_p.h"
#include "qquickcontrol_p_p.h"

#include <QtQuick/private/qquicktextinput_p.h>

QT_BEGIN_NAMESPACE

class QQuickAbstractSpinBoxPrivate : public QQuickControlPrivate
{
    Q_DECLARE_PUBLIC(QQuickAbstractSpinBox)

public:
    QQuickAbstractSpinBoxPrivate() : value(0), step(1), input(Q_NULLPTR), validator(Q_NULLPTR),
        upButton(Q_NULLPTR), downButton(Q_NULLPTR), pressed(QQuickAbstractSpinBox::None) { }

    void updateText();
    void updateValue();

    qreal value;
    qreal step;
    QQuickTextInput *input;
    QValidator *validator;
    QQuickItem *upButton;
    QQuickItem *downButton;
    QQuickAbstractSpinBox::SubControl pressed;
};

void QQuickAbstractSpinBoxPrivate::updateText()
{
    if (input) {
        QString text = QString::number(value);
        if (validator) {
            validator->fixup(text);
            int cursor = input->cursorPosition();
            if (validator->validate(text, cursor) == QValidator::Acceptable)
                input->setCursorPosition(cursor);
            input->setText(text);
        }
    }
}

void QQuickAbstractSpinBoxPrivate::updateValue()
{
    Q_Q(QQuickAbstractSpinBox);
    if (validator) {
        QString text = input->text();
        validator->fixup(text);
        int cursor = input->cursorPosition();
        if (validator->validate(text, cursor) == QValidator::Acceptable)
            q->setValue(text.toDouble());
    }
}

QQuickAbstractSpinBox::QQuickAbstractSpinBox(QQuickItem *parent) :
    QQuickControl(*(new QQuickAbstractSpinBoxPrivate), parent)
{
    setFlag(ItemIsFocusScope);
    setActiveFocusOnTab(true);
    setAcceptedMouseButtons(Qt::LeftButton);
}

qreal QQuickAbstractSpinBox::value() const
{
    Q_D(const QQuickAbstractSpinBox);
    return d->value;
}

void QQuickAbstractSpinBox::setValue(qreal value)
{
    Q_D(QQuickAbstractSpinBox);
    if (!qFuzzyCompare(d->value, value)) {
        d->value = value;
        d->updateText();
        emit valueChanged();
    }
}

qreal QQuickAbstractSpinBox::step() const
{
    Q_D(const QQuickAbstractSpinBox);
    return d->step;
}

void QQuickAbstractSpinBox::setStep(qreal step)
{
    Q_D(QQuickAbstractSpinBox);
    if (!qFuzzyCompare(d->step, step)) {
        d->step = step;
        emit stepChanged();
    }
}

QQuickTextInput *QQuickAbstractSpinBox::input() const
{
    Q_D(const QQuickAbstractSpinBox);
    return d->input;
}

void QQuickAbstractSpinBox::setInput(QQuickTextInput *input)
{
    Q_D(QQuickAbstractSpinBox);
    if (d->input != input) {
        delete d->input;
        d->input = input;
        if (input) {
            if (!input->parentItem())
                input->setParentItem(this);
            input->setValidator(d->validator);
            input->setFocus(true);
            QObjectPrivate::connect(input, &QQuickTextInput::textChanged, d, &QQuickAbstractSpinBoxPrivate::updateValue);
        }
        emit inputChanged();
    }
}

QValidator *QQuickAbstractSpinBox::validator() const
{
    Q_D(const QQuickAbstractSpinBox);
    return d->validator;
}

void QQuickAbstractSpinBox::setValidator(QValidator *validator)
{
    Q_D(QQuickAbstractSpinBox);
    if (d->validator != validator) {
        delete d->validator;
        d->validator = validator;
        if (d->input)
            d->input->setValidator(validator);
        emit validatorChanged();
    }
}

QQuickItem *QQuickAbstractSpinBox::upButton() const
{
    Q_D(const QQuickAbstractSpinBox);
    return d->upButton;
}

void QQuickAbstractSpinBox::setUpButton(QQuickItem *button)
{
    Q_D(QQuickAbstractSpinBox);
    if (d->upButton != button) {
        delete d->upButton;
        d->upButton = button;
        if (button && !button->parentItem())
            button->setParentItem(this);
        emit upButtonChanged();
    }
}

QQuickItem *QQuickAbstractSpinBox::downButton() const
{
    Q_D(const QQuickAbstractSpinBox);
    return d->downButton;
}

void QQuickAbstractSpinBox::setDownButton(QQuickItem *button)
{
    Q_D(QQuickAbstractSpinBox);
    if (d->downButton != button) {
        delete d->downButton;
        d->downButton = button;
        if (button && !button->parentItem())
            button->setParentItem(this);
        emit downButtonChanged();
    }
}

QQuickAbstractSpinBox::SubControl QQuickAbstractSpinBox::pressed() const
{
    Q_D(const QQuickAbstractSpinBox);
    return d->pressed;
}

void QQuickAbstractSpinBox::setPressed(SubControl control)
{
    Q_D(QQuickAbstractSpinBox);
    if (d->pressed != control) {
        d->pressed = control;
        emit pressedChanged();
    }
}

void QQuickAbstractSpinBox::increment()
{
    Q_D(QQuickAbstractSpinBox);
    setValue(d->value + d->step);
}

void QQuickAbstractSpinBox::decrement()
{
    Q_D(QQuickAbstractSpinBox);
    setValue(d->value - d->step);
}

void QQuickAbstractSpinBox::keyPressEvent(QKeyEvent *event)
{
    switch (event->key()) {
    case Qt::Key_Up:
        increment();
        event->accept();
        break;
    case Qt::Key_Down:
        decrement();
        event->accept();
        break;
    default:
        event->ignore();
        break;
    }
}

void QQuickAbstractSpinBox::mousePressEvent(QMouseEvent *event)
{
    Q_D(QQuickAbstractSpinBox);
    QQuickControl::mousePressEvent(event);
    if (d->upButton && d->upButton->contains(mapToItem(d->upButton, event->pos())))
        setPressed(UpButton);
    else if (d->downButton && d->downButton->contains(mapToItem(d->downButton, event->pos())))
        setPressed(DownButton);
    else
        setPressed(None);
    event->accept();
}

void QQuickAbstractSpinBox::mouseMoveEvent(QMouseEvent *event)
{
    Q_D(QQuickAbstractSpinBox);
    QQuickControl::mouseMoveEvent(event);
    if (d->upButton && d->upButton->contains(mapToItem(d->upButton, event->pos())))
        setPressed(UpButton);
    else if (d->downButton && d->downButton->contains(mapToItem(d->downButton, event->pos())))
        setPressed(DownButton);
    else
        setPressed(None);
    event->accept();
}

void QQuickAbstractSpinBox::mouseReleaseEvent(QMouseEvent *event)
{
    Q_D(QQuickAbstractSpinBox);
    QQuickControl::mouseReleaseEvent(event);
    if (d->pressed == UpButton)
        increment();
    else if (d->pressed == DownButton)
        decrement();
    setPressed(None);
    event->accept();
}

void QQuickAbstractSpinBox::mouseUngrabEvent()
{
    setPressed(None);
}

void QQuickAbstractSpinBox::componentComplete()
{
    Q_D(QQuickAbstractSpinBox);
    QQuickControl::componentComplete();
    d->updateText();
}

QT_END_NAMESPACE
