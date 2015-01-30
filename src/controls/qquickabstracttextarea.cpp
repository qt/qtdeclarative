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

#include "qquickabstracttextarea_p.h"
#include "qquickcontrol_p_p.h"

#include <QtQuick/private/qquicktext_p.h>
#include <QtQuick/private/qquicktextedit_p.h>

QT_BEGIN_NAMESPACE

class QQuickAbstractTextAreaPrivate : public QQuickControlPrivate
{
    Q_DECLARE_PUBLIC(QQuickAbstractTextArea)

public:
    QQuickAbstractTextAreaPrivate() : edit(Q_NULLPTR), placeholder(Q_NULLPTR) { }

    void updateText();

    QString text;
    QQuickTextEdit *edit;
    QQuickText *placeholder;
};

void QQuickAbstractTextAreaPrivate::updateText()
{
    Q_Q(QQuickAbstractTextArea);
    q->setText(edit->text());
}

QQuickAbstractTextArea::QQuickAbstractTextArea(QQuickItem *parent) :
    QQuickControl(*(new QQuickAbstractTextAreaPrivate), parent)
{
    setFlag(ItemIsFocusScope);
    setActiveFocusOnTab(true);
}

QString QQuickAbstractTextArea::text() const
{
    Q_D(const QQuickAbstractTextArea);
    return d->text;
}

void QQuickAbstractTextArea::setText(const QString &text)
{
    Q_D(QQuickAbstractTextArea);
    if (d->text != text) {
        d->text = text;
        if (d->edit)
            d->edit->setText(text);
        emit textChanged();
    }
}

QQuickTextEdit *QQuickAbstractTextArea::edit() const
{
    Q_D(const QQuickAbstractTextArea);
    return d->edit;
}

void QQuickAbstractTextArea::setEdit(QQuickTextEdit *edit)
{
    Q_D(QQuickAbstractTextArea);
    if (d->edit != edit) {
        delete d->edit;
        d->edit = edit;
        if (edit) {
            if (!edit->parentItem())
                edit->setParentItem(this);
            edit->setText(d->text);
            QObjectPrivate::connect(edit, &QQuickTextEdit::textChanged, d, &QQuickAbstractTextAreaPrivate::updateText);
        }
        emit editChanged();
    }
}

QQuickText *QQuickAbstractTextArea::placeholder() const
{
    Q_D(const QQuickAbstractTextArea);
    return d->placeholder;
}

void QQuickAbstractTextArea::setPlaceholder(QQuickText *placeholder)
{
    Q_D(QQuickAbstractTextArea);
    if (d->placeholder != placeholder) {
        delete d->placeholder;
        d->placeholder = placeholder;
        if (placeholder && !placeholder->parentItem())
            placeholder->setParentItem(this);
        emit placeholderChanged();
    }
}

QT_END_NAMESPACE
