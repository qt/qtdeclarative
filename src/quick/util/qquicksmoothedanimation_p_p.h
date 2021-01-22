/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQuick module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

#ifndef QQUICKSMOOTHEDANIMATION2_P_H
#define QQUICKSMOOTHEDANIMATION2_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "qquicksmoothedanimation_p.h"
#include "qquickanimation_p.h"

#include "qquickanimation_p_p.h"

#include <private/qobject_p.h>
#include <QBasicTimer>

QT_BEGIN_NAMESPACE
class QSmoothedAnimation;
class QSmoothedAnimationTimer : public QTimer
{
    Q_OBJECT
public:
    explicit QSmoothedAnimationTimer(QSmoothedAnimation *animation, QObject *parent = nullptr);
    ~QSmoothedAnimationTimer();
public Q_SLOTS:
    void stopAnimation();
private:
    QSmoothedAnimation *m_animation;
};

class QQuickSmoothedAnimationPrivate;
class Q_AUTOTEST_EXPORT QSmoothedAnimation : public QAbstractAnimationJob
{
    Q_DISABLE_COPY(QSmoothedAnimation)
public:
    QSmoothedAnimation(QQuickSmoothedAnimationPrivate * = nullptr);

    ~QSmoothedAnimation();
    qreal to;
    qreal velocity;
    int userDuration;

    int maximumEasingTime;
    QQuickSmoothedAnimation::ReversingMode reversingMode;

    qreal initialVelocity;
    qreal trackVelocity;

    QQmlProperty target;

    int duration() const override;
    void restart();
    void init();

    void prepareForRestart();
    void clearTemplate() { animationTemplate = nullptr; }

protected:
    void updateCurrentTime(int) override;
    void updateState(QAbstractAnimationJob::State, QAbstractAnimationJob::State) override;
    void debugAnimation(QDebug d) const override;

private:
    qreal easeFollow(qreal);
    qreal initialValue;

    bool invert;

    int finalDuration;

    // Parameters for use in updateCurrentTime()
    qreal a;  // Acceleration
    qreal d;  // Deceleration
    qreal tf; // Total time
    qreal tp; // Time at which peak velocity occurs
    qreal td; // Time at which deceleration begins
    qreal vp; // Velocity at tp
    qreal sp; // Displacement at tp
    qreal sd; // Displacement at td
    qreal vi; // "Normalized" initialvelocity
    qreal s;  // Total s

    int lastTime;
    bool skipUpdate;

    bool recalc();
    void delayedStop();
    QSmoothedAnimationTimer *delayedStopTimer;
    QQuickSmoothedAnimationPrivate *animationTemplate;
};

class QQuickSmoothedAnimationPrivate : public QQuickPropertyAnimationPrivate
{
    Q_DECLARE_PUBLIC(QQuickSmoothedAnimation)
public:
    QQuickSmoothedAnimationPrivate();
    ~QQuickSmoothedAnimationPrivate();
    void updateRunningAnimations();

    QSmoothedAnimation *anim;
    QHash<QQmlProperty, QSmoothedAnimation*> activeAnimations;
};

QT_END_NAMESPACE

#endif // QQUICKSMOOTHEDANIMATION2_P_H
