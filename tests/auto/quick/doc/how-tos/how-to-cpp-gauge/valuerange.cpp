// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "valuerange.h"

QT_BEGIN_NAMESPACE

ValueRange::ValueRange(QObject *parent)
    : QObject(parent)
    , mInverted(false)
{
}

/*!
    \property ValueRange::minimumValue
    \brief the minimum value that \l value can assume

    The default value is \c 0.
*/
qreal ValueRange::minimumValue() const
{
    return mMinimumValue;
}

void ValueRange::setMinimumValue(qreal min)
{
    if (qFuzzyCompare(mMinimumValue, min))
        return;

    mMinimumValue = min;
    emit minimumValueChanged();
    emit constraintsChanged();
    if (mIsComplete) {
        setValue(mValue);
        updatePosition();
    }
}

/*!
    \property ValueRange::maximumValue
    \brief the maximum value that \l value can assume

    The default value is \c 100.
*/
qreal ValueRange::maximumValue() const
{
    return mMaximumValue;
}

void ValueRange::setMaximumValue(qreal max)
{
    if (qFuzzyCompare(mMaximumValue, max))
        return;

    mMaximumValue = max;
    emit maximumValueChanged();
    emit constraintsChanged();
    if (mIsComplete) {
        setValue(mMaximumValue);
        updatePosition();
    }
}

/*!
    \property ValueRange::stepSize

    This property holds the step size. The default value is \c 0.0.

    For example, if a user sets \l minimumValue to \c 0, \l maximumValue to
    \c 100, and stepSize to \c 30, the valid values for \l value would be:
    \c 0, \c 30, \c 60, \c 90, and \c 100.
*/

void ValueRange::setStepSize(qreal stepSize)
{
    stepSize = qMax(qreal(0.0), stepSize);
    if (qFuzzyCompare(mStepSize, stepSize))
        return;

    mStepSize = stepSize;
    emit constraintsChanged();
    emit stepSizeChanged();
}

qreal ValueRange::stepSize() const
{
    return mStepSize;
}


void ValueRange::classBegin()
{
}

void ValueRange::componentComplete()
{
    mIsComplete = true;
}

/*!
    \qmlproperty real ValueRange::value

    This property holds the value in the range \c from - \c to. The default value is \c 0.0.

    \sa position
*/
qreal ValueRange::value() const
{
    return mValue;
}

void ValueRange::setValue(qreal value)
{
    if (mIsComplete)
        value = mMinimumValue > mMaximumValue ? qBound(mMaximumValue, value, mMinimumValue) : qBound(mMinimumValue, value, mMaximumValue);

    if (qFuzzyCompare(mValue, value))
        return;

    mValue = value;
    updatePosition();
    emit valueChanged();
}

/*!
    \qmlproperty real ValueRange::position
    \readonly

    This property holds the logical position of the range's value.

    The position is expressed as a fraction of the control's size, in the range
    \c {0.0 - 1.0}. For visualizing a slider, the right-to-left aware
    \l visualPosition should be used instead.

    \sa value, visualPosition, valueAt()
*/
qreal ValueRange::position() const
{
    return mPosition;
}

/*!
    \qmlproperty real ValueRange::visualPosition
    \readonly

    This property holds the visual position of the range's value.

    The position is expressed as a fraction of the control's size, in the range
    \c {0.0 - 1.0}. When the control is \l {Control::mirrored}{mirrored}, the
    value is equal to \c {1.0 - position}. This makes the value suitable for
    visualizing the slider, taking right-to-left support into account.

    \sa position
*/
qreal ValueRange::visualPosition() const
{
    if (mInverted)
        return 1.0 - mPosition;
    return mPosition;
}

void ValueRange::setPosition(qreal pos)
{
    pos = qBound<qreal>(0.0, pos, 1.0);
    if (qFuzzyCompare(mPosition, pos))
        return;

    mPosition = pos;
    emit positionChanged();
    emit visualPositionChanged();
}


void ValueRange::updatePosition()
{
    qreal pos = 0;
    if (!qFuzzyCompare(mMinimumValue, mMaximumValue))
        pos = (mValue - mMinimumValue) / (mMaximumValue - mMinimumValue);
    setPosition(pos);
}

/*!
    \property ValueRange::inverted

    This property holds whether the \l minimumValue and \l maximumValue should be visually inverted.
*/

void ValueRange::setInverted(bool inverted)
{
    if (inverted == mInverted)
        return;

    mInverted = inverted;
    emit visualPositionChanged();
    emit invertedChanged();
}

bool ValueRange::inverted() const
{
    return mInverted;
}

QT_END_NAMESPACE
