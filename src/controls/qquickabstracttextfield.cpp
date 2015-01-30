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

#include "qquickabstracttextfield_p.h"
#include "qquickcontrol_p_p.h"

#include <QtQuick/private/qquicktext_p.h>
#include <QtQuick/private/qquicktextinput_p.h>

QT_BEGIN_NAMESPACE

class QQuickAbstractTextFieldPrivate : public QQuickControlPrivate
{
    Q_DECLARE_PUBLIC(QQuickAbstractTextField)

public:
    QQuickAbstractTextFieldPrivate() : input(Q_NULLPTR), placeholder(Q_NULLPTR) { }

    void updateText();

    QString text;
    QQuickTextInput *input;
    QQuickText *placeholder;
};


void QQuickAbstractTextFieldPrivate::updateText()
{
    Q_Q(QQuickAbstractTextField);
    q->setText(input->text());
}

QQuickAbstractTextField::QQuickAbstractTextField(QQuickItem *parent) :
    QQuickControl(*(new QQuickAbstractTextFieldPrivate), parent)
{
    setFlag(ItemIsFocusScope);
    setActiveFocusOnTab(true);
}

QString QQuickAbstractTextField::text() const
{
    Q_D(const QQuickAbstractTextField);
    return d->text;
}

void QQuickAbstractTextField::setText(const QString &text)
{
    Q_D(QQuickAbstractTextField);
    if (d->text != text) {
        d->text = text;
        if (d->input)
            d->input->setText(text);
        emit textChanged();
    }
}

QQuickTextInput *QQuickAbstractTextField::input() const
{
    Q_D(const QQuickAbstractTextField);
    return d->input;
}

void QQuickAbstractTextField::setInput(QQuickTextInput *input)
{
    Q_D(QQuickAbstractTextField);
    if (d->input != input) {
        delete d->input;
        d->input = input;
        if (input) {
            if (!input->parentItem())
                input->setParentItem(this);
            input->setText(d->text);
            QObjectPrivate::connect(input, &QQuickTextInput::textChanged, d, &QQuickAbstractTextFieldPrivate::updateText);
        }
        emit inputChanged();
    }
}

QQuickText *QQuickAbstractTextField::placeholder() const
{
    Q_D(const QQuickAbstractTextField);
    return d->placeholder;
}

void QQuickAbstractTextField::setPlaceholder(QQuickText *placeholder)
{
    Q_D(QQuickAbstractTextField);
    if (d->placeholder != placeholder) {
        delete d->placeholder;
        d->placeholder = placeholder;
        if (placeholder && !placeholder->parentItem())
            placeholder->setParentItem(this);
        emit placeholderChanged();
    }
}

QT_END_NAMESPACE
