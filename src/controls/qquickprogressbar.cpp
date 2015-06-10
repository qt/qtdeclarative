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
    \brief A progress bar control.

    ProgressBar indicates the progress of an operation.

    \table
    \row \li \image qtquickcontrols2-progressbar-normal.png
         \li A progress bar in its normal state.
    \row \li \image qtquickcontrols2-progressbar-disabled.png
         \li A progress bar that is disabled.
    \endtable

    \code
    ProgressBar {
        value: 0.5
    }
    \endcode

    \sa {Customizing ProgressBar}
*/

class QQuickProgressBarPrivate : public QQuickControlPrivate
{
public:
    QQuickProgressBarPrivate() : value(0), indeterminate(false), indicator(Q_NULLPTR) { }

    qreal value;
    bool indeterminate;
    QQuickItem *indicator;
};

QQuickProgressBar::QQuickProgressBar(QQuickItem *parent) :
    QQuickControl(*(new QQuickProgressBarPrivate), parent)
{
}

/*!
    \qmlproperty real QtQuickControls2::ProgressBar::value

    This property holds the value in the range \c 0.0 - \c 1.0. The default value is \c 0.0.

    \sa visualPosition
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
    \readonly

    This property holds the visual position of the progress.

    The position is defined as a percentage of the control's size, scaled to
    \c 0.0 - \c 1.0. When the control is \l mirrored, the value is equal to
    \c {1.0 - value}. This makes the value suitable for visualizing the
    progress, taking right-to-left support into account.

    \sa value
*/
qreal QQuickProgressBar::visualPosition() const
{
    Q_D(const QQuickProgressBar);
    if (isMirrored())
        return 1.0 - d->value;
    return d->value;
}

/*!
    \qmlproperty bool QtQuickControls2::ProgressBar::indeterminate

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
    \qmlproperty Item QtQuickControls2::ProgressBar::indicator

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
    emit visualPositionChanged();
}

QT_END_NAMESPACE
