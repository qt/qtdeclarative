/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQuick module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qquickboundaryrule_p.h"

#include <qqmlcontext.h>
#include <qqmlinfo.h>
#include <private/qqmlproperty_p.h>
#include <private/qqmlengine_p.h>
#include <private/qobject_p.h>
#include <private/qquickanimation_p_p.h>
#include <QtCore/qloggingcategory.h>

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(lcBR, "qt.quick.boundaryrule")

class QQuickBoundaryReturnJob;
class QQuickBoundaryRulePrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QQuickBoundaryRule)
public:
    QQuickBoundaryRulePrivate() {}

    QQmlProperty property;
    QEasingCurve easing = QEasingCurve(QEasingCurve::OutQuad);
    QQuickBoundaryReturnJob *returnAnimationJob = nullptr;
    // read-only properties, updated on each write()
    qreal targetValue = 0; // after easing was applied
    qreal peakOvershoot = 0;
    qreal currentOvershoot = 0;
    // settable properties
    qreal minimum = 0;
    qreal maximum = 0;
    qreal minimumOvershoot = 0;
    qreal maximumOvershoot = 0;
    qreal overshootScale = 0.5;
    int returnDuration = 100;
    QQuickBoundaryRule::OvershootFilter overshootFilter = QQuickBoundaryRule::OvershootFilter::None;
    bool enabled = true;
    bool finalized = false;

    qreal easedOvershoot(qreal overshootingValue);
    void resetOvershoot();
};

class QQuickBoundaryReturnJob : public QAbstractAnimationJob
{
public:
    QQuickBoundaryReturnJob(QQuickBoundaryRulePrivate *br, qreal to)
        : QAbstractAnimationJob()
        , boundaryRule(br)
        , fromValue(br->targetValue)
        , toValue(to) {}

    int duration() const override { return boundaryRule->returnDuration; }

    void updateCurrentTime(int) override;

    void updateState(QAbstractAnimationJob::State newState,
                     QAbstractAnimationJob::State oldState) override;

    QQuickBoundaryRulePrivate *boundaryRule;
    qreal fromValue;    // snapshot of initial value from which we're returning
    qreal toValue;      // target property value to which we're returning
};

void QQuickBoundaryReturnJob::updateCurrentTime(int t)
{
    // The easing property tells how to behave when the property is being
    // externally manipulated beyond the bounds.  During returnToBounds()
    // we run it in reverse, by reversing time.
    qreal progress = (duration() - t) / qreal(duration());
    qreal easingValue = boundaryRule->easing.valueForProgress(progress);
    qreal delta = qAbs(fromValue - toValue) * easingValue;
    qreal value = (fromValue > toValue ? toValue + delta : toValue - delta);
    qCDebug(lcBR) << t << "ms" << qRound(progress * 100) << "% easing" << easingValue << "->" << value;
    QQmlPropertyPrivate::write(boundaryRule->property, value,
                               QQmlPropertyData::BypassInterceptor | QQmlPropertyData::DontRemoveBinding);
}

void QQuickBoundaryReturnJob::updateState(QAbstractAnimationJob::State newState, QAbstractAnimationJob::State oldState)
{
    Q_UNUSED(oldState)
    if (newState == QAbstractAnimationJob::Stopped) {
        qCDebug(lcBR) << "return animation done";
        boundaryRule->resetOvershoot();
        boundaryRule->returnAnimationJob = nullptr;
        delete this;
    }
}

/*!
    \qmltype BoundaryRule
    \instantiates QQuickBoundaryRule
    \inqmlmodule Qt.labs.animation
    \ingroup qtquick-transitions-animations
    \ingroup qtquick-interceptors
    \brief Defines a restriction on the range of values that can be set on a numeric property.
    \since 5.14

    A BoundaryRule defines the range of values that a particular property is
    allowed to have.  When an out-of-range value would otherwise be set,
    it applies "resistance" via an easing curve.

    For example, the following BoundaryRule prevents DragHandler from dragging
    the Rectangle too far:

    \snippet qml/boundaryRule.qml 0

    Note that a property cannot have more than one assigned BoundaryRule.

    \sa {Animation and Transitions in Qt Quick}, {Qt Quick Examples - Animation#Behaviors}{Behavior example}, {Qt QML}
*/

QQuickBoundaryRule::QQuickBoundaryRule(QObject *parent)
    : QObject(*(new QQuickBoundaryRulePrivate), parent)
    , QQmlPropertyValueInterceptor()
{
}

QQuickBoundaryRule::~QQuickBoundaryRule()
{
    Q_D(QQuickBoundaryRule);
    // stop any running animation and
    // prevent QQuickBoundaryReturnJob::updateState() from accessing QQuickBoundaryRulePrivate
    delete d->returnAnimationJob;
}

/*!
    \qmlproperty bool QtQuick::BoundaryRule::enabled

    This property holds whether the rule will be enforced when the tracked
    property changes value.

    By default a BoundaryRule is enabled.
*/
bool QQuickBoundaryRule::enabled() const
{
    Q_D(const QQuickBoundaryRule);
    return d->enabled;
}

void QQuickBoundaryRule::setEnabled(bool enabled)
{
    Q_D(QQuickBoundaryRule);
    if (d->enabled == enabled)
        return;
    d->enabled = enabled;
    emit enabledChanged();
}

/*!
    \qmlproperty qreal QtQuick::BoundaryRule::minimum

    This property holds the smallest unconstrained value that the property is
    allowed to have.  If the property is set to a smaller value, it will be
    constrained by \l easing and \l minimumOvershoot.

    The default is \c 0.
*/
qreal QQuickBoundaryRule::minimum() const
{
    Q_D(const QQuickBoundaryRule);
    return d->minimum;
}

void QQuickBoundaryRule::setMinimum(qreal minimum)
{
    Q_D(QQuickBoundaryRule);
    if (qFuzzyCompare(d->minimum, minimum))
        return;
    d->minimum = minimum;
    emit minimumChanged();
}

/*!
    \qmlproperty qreal QtQuick::BoundaryRule::minimumOvershoot

    This property holds the amount that the property is allowed to be
    less than \l minimum.  Whenever the value is less than \l minimum
    and greater than \c {minimum - minimumOvershoot}, it is constrained
    by the \l easing curve.  When the value attempts to go under
    \c {minimum - minimumOvershoots} there is a hard stop.

    The default is \c 0.
*/
qreal QQuickBoundaryRule::minimumOvershoot() const
{
    Q_D(const QQuickBoundaryRule);
    return d->minimumOvershoot;
}

void QQuickBoundaryRule::setMinimumOvershoot(qreal minimumOvershoot)
{
    Q_D(QQuickBoundaryRule);
    if (qFuzzyCompare(d->minimumOvershoot, minimumOvershoot))
        return;
    d->minimumOvershoot = minimumOvershoot;
    emit minimumOvershootChanged();
}

/*!
    \qmlproperty qreal QtQuick::BoundaryRule::maximum

    This property holds the largest unconstrained value that the property is
    allowed to have.  If the property is set to a larger value, it will be
    constrained by \l easing and \l maximumOvershoot.

    The default is \c 1.
*/
qreal QQuickBoundaryRule::maximum() const
{
    Q_D(const QQuickBoundaryRule);
    return d->maximum;
}

void QQuickBoundaryRule::setMaximum(qreal maximum)
{
    Q_D(QQuickBoundaryRule);
    if (qFuzzyCompare(d->maximum, maximum))
        return;
    d->maximum = maximum;
    emit maximumChanged();
}

/*!
    \qmlproperty qreal QtQuick::BoundaryRule::maximumOvershoot

    This property holds the amount that the property is allowed to be
    more than \l maximum.  Whenever the value is greater than \l maximum
    and less than \c {maximum + maximumOvershoot}, it is constrained
    by the \l easing curve.  When the value attempts to exceed
    \c {maximum + maximumOvershoot} there is a hard stop.

    The default is 0.
*/
qreal QQuickBoundaryRule::maximumOvershoot() const
{
    Q_D(const QQuickBoundaryRule);
    return d->maximumOvershoot;
}

void QQuickBoundaryRule::setMaximumOvershoot(qreal maximumOvershoot)
{
    Q_D(QQuickBoundaryRule);
    if (qFuzzyCompare(d->maximumOvershoot, maximumOvershoot))
        return;
    d->maximumOvershoot = maximumOvershoot;
    emit maximumOvershootChanged();
}

/*!
    \qmlproperty qreal QtQuick::BoundaryRule::overshootScale

    This property holds the amount by which the \l easing is scaled during the
    overshoot condition. For example if an Item is restricted from moving more
    than 100 pixels beyond some limit, and the user (by means of some Input
    Handler) is trying to drag it 100 pixels past the limit, if overshootScale
    is set to 1, the user will succeed: the only effect of the easing curve is
    to change the rate at which the item moves from overshoot 0 to overshoot
    100. But if it is set to 0.5, the BoundaryRule provides resistance such
    that when the user tries to move 100 pixels, the Item will only move 50
    pixels; and the easing curve modulates the rate of movement such that it
    may move in sync with the user's attempted movement at the beginning, and
    then slows down, depending on the shape of the easing curve.

    The default is 0.5.
*/
qreal QQuickBoundaryRule::overshootScale() const
{
    Q_D(const QQuickBoundaryRule);
    return d->overshootScale;
}

void QQuickBoundaryRule::setOvershootScale(qreal overshootScale)
{
    Q_D(QQuickBoundaryRule);
    if (qFuzzyCompare(d->overshootScale, overshootScale))
        return;
    d->overshootScale = overshootScale;
    emit overshootScaleChanged();
}

/*!
    \qmlproperty qreal QtQuick::BoundaryRule::currentOvershoot

    This property holds the amount by which the most recently set value of the
    intercepted property exceeds \l maximum or is less than \l minimum.

    It is positive if the property value exceeds \l maximum, negative if the
    property value is less than \l minimum, or 0 if the property value is
    within both boundaries.
*/
qreal QQuickBoundaryRule::currentOvershoot() const
{
    Q_D(const QQuickBoundaryRule);
    return d->currentOvershoot;
}

/*!
    \qmlproperty qreal QtQuick::BoundaryRule::peakOvershoot

    This property holds the most-positive or most-negative value of
    \l currentOvershoot that has been seen, until \l returnToBounds() is called.

    This can be useful when the intercepted property value is known to
    fluctuate, and you want to find and react to the maximum amount of
    overshoot rather than to the fluctuations.

    \sa overshootFilter
*/
qreal QQuickBoundaryRule::peakOvershoot() const
{
    Q_D(const QQuickBoundaryRule);
    return d->peakOvershoot;
}

/*!
    \qmlproperty enum QtQuick::BoundaryRule::overshootFilter

    This property specifies the aggregation function that will be applied to
    the intercepted property value.

    If this is set to \c BoundaryRule.None (the default), the intercepted
    property will hold a value whose overshoot is limited to \l currentOvershoot.
    If this is set to \c BoundaryRule.Peak, the intercepted property will hold
    a value whose overshoot is limited to \l peakOvershoot.
*/
QQuickBoundaryRule::OvershootFilter QQuickBoundaryRule::overshootFilter() const
{
    Q_D(const QQuickBoundaryRule);
    return d->overshootFilter;
}

void QQuickBoundaryRule::setOvershootFilter(OvershootFilter overshootFilter)
{
    Q_D(QQuickBoundaryRule);
    if (d->overshootFilter == overshootFilter)
        return;
    d->overshootFilter = overshootFilter;
    emit overshootFilterChanged();
}

/*!
    \qmlmethod bool QtQuick::BoundaryRule::returnToBounds

    Returns the intercepted property to a value between \l minimum and
    \l maximum, such that \l currentOvershoot and \l peakOvershoot are both
    zero. This will be animated if \l returnDuration is greater than zero.

    Returns true if the value needed to be adjusted, or false if it was already
    within bounds.
*/
bool QQuickBoundaryRule::returnToBounds()
{
    Q_D(QQuickBoundaryRule);
    if (d->returnAnimationJob) {
        qCDebug(lcBR) << "animation already in progress";
        return true;
    }
    if (currentOvershoot() > 0) {
        if (d->returnDuration > 0)
            d->returnAnimationJob = new QQuickBoundaryReturnJob(d, maximum());
        else
            write(maximum());
    } else if (currentOvershoot() < 0) {
        if (d->returnDuration > 0)
            d->returnAnimationJob = new QQuickBoundaryReturnJob(d, minimum());
        else
            write(minimum());
    } else {
        return false;
    }
    if (d->returnAnimationJob) {
        qCDebug(lcBR) << "animating from" << d->returnAnimationJob->fromValue << "to" << d->returnAnimationJob->toValue;
        d->returnAnimationJob->start();
    } else {
        d->resetOvershoot();
        qCDebug(lcBR) << "returned to" << d->property.read();
    }
    return true;
}

/*!
    \qmlproperty qreal QtQuick::BoundaryRule::easing

    This property holds the easing curve to be applied in overshoot mode
    (whenever the \l minimum or \l maximum constraint is violated, while
    the value is still within the respective overshoot range).

    The default easing curve is \l QEasingCurve::OutQuad.
*/
QEasingCurve QQuickBoundaryRule::easing() const
{
    Q_D(const QQuickBoundaryRule);
    return d->easing;
}

void QQuickBoundaryRule::setEasing(const QEasingCurve &easing)
{
    Q_D(QQuickBoundaryRule);
    if (d->easing == easing)
        return;
    d->easing = easing;
    emit easingChanged();
}

/*!
    \qmlproperty int QtQuick::BoundaryRule::returnDuration

    This property holds the amount of time in milliseconds that
    \l returnToBounds() will take to return the target property to the nearest bound.
    If it is set to 0, returnToBounds() will set the property immediately
    rather than creating an animation job.

    The default is 100 ms.
*/
int QQuickBoundaryRule::returnDuration() const
{
    Q_D(const QQuickBoundaryRule);
    return d->returnDuration;
}

void QQuickBoundaryRule::setReturnDuration(int duration)
{
    Q_D(QQuickBoundaryRule);
    if (d->returnDuration == duration)
        return;
    d->returnDuration = duration;
    emit returnDurationChanged();
}

void QQuickBoundaryRule::write(const QVariant &value)
{
    bool conversionOk = false;
    qreal rValue = value.toReal(&conversionOk);
    if (!conversionOk) {
        qWarning() << "BoundaryRule doesn't work with non-numeric values:" << value;
        return;
    }
    Q_D(QQuickBoundaryRule);
    bool bypass = !d->enabled || !d->finalized || QQmlEnginePrivate::designerMode();
    if (bypass) {
        QQmlPropertyPrivate::write(d->property, value,
                                   QQmlPropertyData::BypassInterceptor | QQmlPropertyData::DontRemoveBinding);
        return;
    }

    qmlExecuteDeferred(this);
    d->targetValue = d->easedOvershoot(rValue);
    QQmlPropertyPrivate::write(d->property, d->targetValue,
                               QQmlPropertyData::BypassInterceptor | QQmlPropertyData::DontRemoveBinding);
}

void QQuickBoundaryRule::setTarget(const QQmlProperty &property)
{
    Q_D(QQuickBoundaryRule);
    d->property = property;

    QQmlEnginePrivate *engPriv = QQmlEnginePrivate::get(qmlEngine(this));
    static int finalizedIdx = -1;
    if (finalizedIdx < 0)
        finalizedIdx = metaObject()->indexOfSlot("componentFinalized()");
    engPriv->registerFinalizeCallback(this, finalizedIdx);
}

void QQuickBoundaryRule::componentFinalized()
{
    Q_D(QQuickBoundaryRule);
    d->finalized = true;
}

/*!
    \internal
    Given that something is trying to set the target property to \a value,
    this function applies the easing curve and returns the value that the
    property should actually get instead.
*/
qreal QQuickBoundaryRulePrivate::easedOvershoot(qreal value)
{
    qreal ret = value;
    Q_Q(QQuickBoundaryRule);
    if (value > maximum) {
        qreal overshootWas = currentOvershoot;
        currentOvershoot = value - maximum;
        if (!qFuzzyCompare(overshootWas, currentOvershoot))
            emit q->currentOvershootChanged();
        overshootWas = peakOvershoot;
        peakOvershoot = qMax(currentOvershoot, peakOvershoot);
        if (!qFuzzyCompare(overshootWas, peakOvershoot))
            emit q->peakOvershootChanged();
        ret = maximum + maximumOvershoot * easing.valueForProgress(
                    (overshootFilter == QQuickBoundaryRule::OvershootFilter::Peak ? peakOvershoot : currentOvershoot)
                    * overshootScale / maximumOvershoot);
        qCDebug(lcBR).nospace() << value << " overshoots maximum " << maximum << " by "
                                << currentOvershoot << " (peak " << peakOvershoot << "): eased to " << ret;
    } else if (value < minimum) {
        qreal overshootWas = currentOvershoot;
        currentOvershoot = value - minimum;
        if (!qFuzzyCompare(overshootWas, currentOvershoot))
            emit q->currentOvershootChanged();
        overshootWas = peakOvershoot;
        peakOvershoot = qMin(currentOvershoot, peakOvershoot);
        if (!qFuzzyCompare(overshootWas, peakOvershoot))
            emit q->peakOvershootChanged();
        ret = minimum - minimumOvershoot * easing.valueForProgress(
                    -(overshootFilter == QQuickBoundaryRule::OvershootFilter::Peak ? peakOvershoot : currentOvershoot)
                    * overshootScale / minimumOvershoot);
        qCDebug(lcBR).nospace() << value << " overshoots minimum " << minimum << " by "
                                << currentOvershoot << " (peak " << peakOvershoot << "): eased to " << ret;
    } else {
        resetOvershoot();
    }
    return ret;
}

/*!
    \internal
    Resets the currentOvershoot and peakOvershoot
    properties to zero.
*/
void QQuickBoundaryRulePrivate::resetOvershoot()
{
    Q_Q(QQuickBoundaryRule);
    if (!qFuzzyCompare(peakOvershoot, 0)) {
        peakOvershoot = 0;
        emit q->peakOvershootChanged();
    }
    if (!qFuzzyCompare(currentOvershoot, 0)) {
        currentOvershoot = 0;
        emit q->currentOvershootChanged();
    }
}

QT_END_NAMESPACE

#include "moc_qquickboundaryrule_p.cpp"
