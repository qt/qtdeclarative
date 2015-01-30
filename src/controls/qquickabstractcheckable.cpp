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

#include "qquickabstractcheckable_p.h"
#include "qquickabstractcheckable_p_p.h"

QT_BEGIN_NAMESPACE

/*!
    \qmltype Checkable
    \inherits Button
    \instantiates QQuickAbstractCheckable
    \inqmlmodule QtQuick.Controls
    \qmlabstract
    \internal
*/

QQuickAbstractCheckablePrivate::QQuickAbstractCheckablePrivate() :
    checked(false), exclusive(false), indicator(Q_NULLPTR), layoutDirection(Qt::LeftToRight)
{
}

QQuickAbstractCheckable::QQuickAbstractCheckable(QQuickItem *parent) :
    QQuickAbstractButton(*(new QQuickAbstractCheckablePrivate), parent)
{
}

QQuickAbstractCheckable::QQuickAbstractCheckable(QQuickAbstractCheckablePrivate &dd, QQuickItem *parent) :
    QQuickAbstractButton(dd, parent)
{
}

/*!
    \qmlproperty bool QtQuickControls2::Checkable::checked

    TODO
*/
bool QQuickAbstractCheckable::isChecked() const
{
    Q_D(const QQuickAbstractCheckable);
    return d->checked;
}

void QQuickAbstractCheckable::setChecked(bool checked)
{
    Q_D(QQuickAbstractCheckable);
    if (d->checked != checked) {
        d->checked = checked;
        emit checkedChanged();
    }
}

bool QQuickAbstractCheckable::isExclusive() const
{
    Q_D(const QQuickAbstractCheckable);
    return d->exclusive;
}

void QQuickAbstractCheckable::setExclusive(bool exclusive)
{
    Q_D(QQuickAbstractCheckable);
    d->exclusive = exclusive;
}

/*!
    \qmlproperty Item QtQuickControls2::Checkable::indicator

    TODO
*/
QQuickItem *QQuickAbstractCheckable::indicator() const
{
    Q_D(const QQuickAbstractCheckable);
    return d->indicator;
}

void QQuickAbstractCheckable::setIndicator(QQuickItem *indicator)
{
    Q_D(QQuickAbstractCheckable);
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
    \qmlproperty enumeration QtQuickControls2::Checkable::layoutDirection

    TODO
*/
Qt::LayoutDirection QQuickAbstractCheckable::layoutDirection() const
{
    Q_D(const QQuickAbstractCheckable);
    return d->layoutDirection;
}

/*!
    \qmlproperty enumeration QtQuickControls2::Checkable::effectiveLayoutDirection

    TODO
*/
Qt::LayoutDirection QQuickAbstractCheckable::effectiveLayoutDirection() const
{
    Q_D(const QQuickAbstractCheckable);
    if (isMirrored())
        return d->layoutDirection == Qt::RightToLeft ? Qt::LeftToRight : Qt::RightToLeft;
    return d->layoutDirection;
}

void QQuickAbstractCheckable::setLayoutDirection(Qt::LayoutDirection direction)
{
    Q_D(QQuickAbstractCheckable);
    if (d->layoutDirection != direction) {
        d->layoutDirection = direction;
        emit layoutDirectionChanged();
        emit effectiveLayoutDirectionChanged();
    }
}

/*!
    \qmlmethod void QtQuickControls2::Checkable::toggle()

    TODO
*/
void QQuickAbstractCheckable::toggle()
{
    Q_D(QQuickAbstractCheckable);
    setChecked(!d->checked);
}

void QQuickAbstractCheckable::keyReleaseEvent(QKeyEvent *event)
{
    Q_D(QQuickAbstractCheckable);
    QQuickAbstractButton::keyReleaseEvent(event);
    if (event->key() == Qt::Key_Space)
        setChecked(d->exclusive || !d->checked);
}

void QQuickAbstractCheckable::mouseReleaseEvent(QMouseEvent *event)
{
    Q_D(QQuickAbstractCheckable);
    QQuickAbstractButton::mouseReleaseEvent(event);
    if (contains(event->pos()))
        setChecked(d->exclusive || !d->checked);
}

void QQuickAbstractCheckable::mirrorChange()
{
    emit effectiveLayoutDirectionChanged();
}

QT_END_NAMESPACE
