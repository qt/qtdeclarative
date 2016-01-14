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

#include "qquickprogressbar_p.h"
#include "qquickcontrol_p_p.h"

QT_BEGIN_NAMESPACE

/*!
    \qmltype ProgressBar
    \inherits Control
    \instantiates QQuickProgressBar
    \inqmlmodule Qt.labs.controls
    \ingroup qtlabscontrols-indicators
    \brief A progress bar control.

    ProgressBar indicates the progress of an operation.

    \table
    \row \li \image qtlabscontrols-progressbar-normal.png
         \li A progress bar in its normal state.
    \row \li \image qtlabscontrols-progressbar-disabled.png
         \li A progress bar that is disabled.
    \endtable

    \code
    ProgressBar {
        value: 0.5
    }
    \endcode

    \labs

    \sa {Customizing ProgressBar}
*/

class QQuickProgressBarPrivate : public QQuickControlPrivate
{
public:
    QQuickProgressBarPrivate() : from(0), to(1.0), value(0), indeterminate(false), indicator(Q_NULLPTR)
    {
    }

    qreal from;
    qreal to;
    qreal value;
    bool indeterminate;
    QQuickItem *indicator;
};

QQuickProgressBar::QQuickProgressBar(QQuickItem *parent) :
    QQuickControl(*(new QQuickProgressBarPrivate), parent)
{
}

/*!
    \qmlproperty real Qt.labs.controls::ProgressBar::from

    This property holds the starting value for the progress. The default value is \c 0.0.

    \sa to, value
*/
qreal QQuickProgressBar::from() const
{
    Q_D(const QQuickProgressBar);
    return d->from;
}

void QQuickProgressBar::setFrom(qreal from)
{
    Q_D(QQuickProgressBar);
    if (!qFuzzyCompare(d->from, from)) {
        d->from = from;
        emit fromChanged();
        emit positionChanged();
        emit visualPositionChanged();
        if (isComponentComplete())
            setValue(d->value);
    }
}

/*!
    \qmlproperty real Qt.labs.controls::ProgressBar::to

    This property holds the end value for the progress. The default value is \c 1.0.

    \sa from, value
*/
qreal QQuickProgressBar::to() const
{
    Q_D(const QQuickProgressBar);
    return d->to;
}

void QQuickProgressBar::setTo(qreal to)
{
    Q_D(QQuickProgressBar);
    if (!qFuzzyCompare(d->to, to)) {
        d->to = to;
        emit toChanged();
        emit positionChanged();
        emit visualPositionChanged();
        if (isComponentComplete())
            setValue(d->value);
    }
}

/*!
    \qmlproperty real Qt.labs.controls::ProgressBar::value

    This property holds the progress value. The default value is \c 0.0.

    \sa from, to, position
*/
qreal QQuickProgressBar::value() const
{
    Q_D(const QQuickProgressBar);
    return d->value;
}

void QQuickProgressBar::setValue(qreal value)
{
    Q_D(QQuickProgressBar);
    if (isComponentComplete())
        value = d->from > d->to ? qBound(d->to, value, d->from) : qBound(d->from, value, d->to);

    if (!qFuzzyCompare(d->value, value)) {
        d->value = value;
        emit valueChanged();
        emit positionChanged();
        emit visualPositionChanged();
    }
}

/*!
    \qmlproperty real Qt.labs.controls::ProgressBar::position
    \readonly

    This property holds the logical position of the progress.

    The position is defined as a percentage of the value, scaled to
    \c {0.0 - 1.0}. For visualizing the progress, the right-to-left
    aware \l visualPosition should be used instead.

    \sa value, visualPosition
*/
qreal QQuickProgressBar::position() const
{
    Q_D(const QQuickProgressBar);
    if (qFuzzyCompare(d->from, d->to))
        return 0;
    return (d->value - d->from) / (d->to - d->from);
}

/*!
    \qmlproperty real Qt.labs.controls::ProgressBar::visualPosition
    \readonly

    This property holds the visual position of the progress.

    The position is defined as a percentage of the value, scaled to \c {0.0 - 1.0}.
    When the control is \l {Control::mirrored}{mirrored}, \c visuaPosition is equal
    to \c {1.0 - position}. This makes \c visualPosition suitable for visualizing
    the progress, taking right-to-left support into account.

    \sa position, value
*/
qreal QQuickProgressBar::visualPosition() const
{
    if (isMirrored())
        return 1.0 - position();
    return position();
}

/*!
    \qmlproperty bool Qt.labs.controls::ProgressBar::indeterminate

    This property holds whether the progress bar is in an indeterminate mode.
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
    \qmlproperty Item Qt.labs.controls::ProgressBar::indicator

    This property holds the indicator item.

    \sa {Customizing ProgressBar}
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
    QQuickControl::mirrorChange();
    if (!qFuzzyCompare(position(), qreal(0.5)))
        emit visualPositionChanged();
}

void QQuickProgressBar::componentComplete()
{
    Q_D(QQuickProgressBar);
    QQuickControl::componentComplete();
    setValue(d->value);
}

#ifndef QT_NO_ACCESSIBILITY
QAccessible::Role QQuickProgressBar::accessibleRole() const
{
    return QAccessible::ProgressBar;
}
#endif

QT_END_NAMESPACE
