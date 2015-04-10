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

#include "qquickprogressbar_p.h"
#include "qquickcontrol_p_p.h"

QT_BEGIN_NAMESPACE

/*!
    \qmltype ProgressBar
    \inherits Control
    \instantiates QQuickProgressBar
    \inqmlmodule QtQuick.Controls
    \ingroup indicators
    \brief A progress bar.

    TODO
*/

class QQuickProgressBarPrivate : public QQuickControlPrivate
{
public:
    QQuickProgressBarPrivate() : value(0), indeterminate(false),
        layoutDirection(Qt::LeftToRight), indicator(Q_NULLPTR) { }

    qreal value;
    bool indeterminate;
    Qt::LayoutDirection layoutDirection;
    QQuickItem *indicator;
};

QQuickProgressBar::QQuickProgressBar(QQuickItem *parent) :
    QQuickControl(*(new QQuickProgressBarPrivate), parent)
{
}

/*!
    \qmlproperty real QtQuickControls2::ProgressBar::value

    TODO
*/
qreal QQuickProgressBar::value() const
{
    Q_D(const QQuickProgressBar);
    return d->value;
}

void QQuickProgressBar::setValue(qreal value)
{
    Q_D(QQuickProgressBar);
    value = qBound(0.0, value, 1.0);
    if (!qFuzzyCompare(d->value, value)) {
        d->value = value;
        emit valueChanged();
        emit visualPositionChanged();
    }
}

/*!
    \qmlproperty real QtQuickControls2::ProgressBar::visualPosition

    TODO
*/
qreal QQuickProgressBar::visualPosition() const
{
    Q_D(const QQuickProgressBar);
    if (effectiveLayoutDirection() == Qt::RightToLeft)
        return 1.0 - d->value;
    return d->value;
}

/*!
    \qmlproperty bool QtQuickControls2::ProgressBar::indeterminate

    TODO
*/
bool QQuickProgressBar::isIndeterminate() const
{
    Q_D(const QQuickProgressBar);
    return d->indeterminate;
}

void QQuickProgressBar::setIndeterminate(bool indeterminate)
{
    Q_D(QQuickProgressBar);
    if (d->indeterminate != indeterminate) {
        d->indeterminate = indeterminate;
        emit indeterminateChanged();
    }
}

/*!
    \qmlproperty enumeration QtQuickControls2::ProgressBar::layoutDirection

    TODO
*/
Qt::LayoutDirection QQuickProgressBar::layoutDirection() const
{
    Q_D(const QQuickProgressBar);
    return d->layoutDirection;
}

/*!
    \qmlproperty enumeration QtQuickControls2::ProgressBar::effectiveLayoutDirection

    TODO
*/
Qt::LayoutDirection QQuickProgressBar::effectiveLayoutDirection() const
{
    Q_D(const QQuickProgressBar);
    if (isMirrored())
        return d->layoutDirection == Qt::RightToLeft ? Qt::LeftToRight : Qt::RightToLeft;
    return d->layoutDirection;
}

void QQuickProgressBar::setLayoutDirection(Qt::LayoutDirection direction)
{
    Q_D(QQuickProgressBar);
    if (d->layoutDirection != direction) {
        d->layoutDirection = direction;
        emit layoutDirectionChanged();
        emit effectiveLayoutDirectionChanged();
    }
}

/*!
    \qmlproperty Item QtQuickControls2::ProgressBar::indicator

    TODO
*/
QQuickItem *QQuickProgressBar::indicator() const
{
    Q_D(const QQuickProgressBar);
    return d->indicator;
}

void QQuickProgressBar::setIndicator(QQuickItem *indicator)
{
    Q_D(QQuickProgressBar);
    if (d->indicator != indicator) {
        delete d->indicator;
        d->indicator = indicator;
        if (indicator && !indicator->parentItem())
            indicator->setParentItem(this);
        emit indicatorChanged();
    }
}

void QQuickProgressBar::mirrorChange()
{
    emit effectiveLayoutDirectionChanged();
    emit visualPositionChanged();
}

QT_END_NAMESPACE
