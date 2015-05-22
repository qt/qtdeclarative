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

#include "qquickcheckable_p.h"
#include "qquickcheckable_p_p.h"

QT_BEGIN_NAMESPACE

/*!
    \qmltype Checkable
    \inherits Button
    \instantiates QQuickCheckable
    \inqmlmodule QtQuick.Controls
    \qmlabstract
    \internal
*/

QQuickCheckablePrivate::QQuickCheckablePrivate() :
    checked(false), exclusive(false), indicator(Q_NULLPTR)
{
}

QQuickCheckable::QQuickCheckable(QQuickItem *parent) :
    QQuickButton(*(new QQuickCheckablePrivate), parent)
{
}

QQuickCheckable::QQuickCheckable(QQuickCheckablePrivate &dd, QQuickItem *parent) :
    QQuickButton(dd, parent)
{
}

/*!
    \qmlproperty bool QtQuickControls2::Checkable::checked

    TODO
*/
bool QQuickCheckable::isChecked() const
{
    Q_D(const QQuickCheckable);
    return d->checked;
}

void QQuickCheckable::setChecked(bool checked)
{
    Q_D(QQuickCheckable);
    if (d->checked != checked) {
        d->checked = checked;
        emit checkedChanged();
    }
}

bool QQuickCheckable::isExclusive() const
{
    Q_D(const QQuickCheckable);
    return d->exclusive;
}

void QQuickCheckable::setExclusive(bool exclusive)
{
    Q_D(QQuickCheckable);
    d->exclusive = exclusive;
}

/*!
    \qmlproperty Item QtQuickControls2::Checkable::indicator

    TODO
*/
QQuickItem *QQuickCheckable::indicator() const
{
    Q_D(const QQuickCheckable);
    return d->indicator;
}

void QQuickCheckable::setIndicator(QQuickItem *indicator)
{
    Q_D(QQuickCheckable);
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
    \qmlmethod void QtQuickControls2::Checkable::toggle()

    TODO
*/
void QQuickCheckable::toggle()
{
    Q_D(QQuickCheckable);
    setChecked(!d->checked);
}

void QQuickCheckable::keyReleaseEvent(QKeyEvent *event)
{
    Q_D(QQuickCheckable);
    QQuickButton::keyReleaseEvent(event);
    if (event->key() == Qt::Key_Space)
        setChecked(d->exclusive || !d->checked);
}

void QQuickCheckable::mouseReleaseEvent(QMouseEvent *event)
{
    Q_D(QQuickCheckable);
    QQuickButton::mouseReleaseEvent(event);
    if (contains(event->pos()))
        setChecked(d->exclusive || !d->checked);
}

QT_END_NAMESPACE
