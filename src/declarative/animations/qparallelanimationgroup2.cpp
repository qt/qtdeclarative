/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/
#include "private/qparallelanimationgroup2_p.h"
#include "private/qparallelanimationgroup2_p_p.h"
//#define QANIMATION_DEBUG



QT_BEGIN_NAMESPACE

QParallelAnimationGroup2::QParallelAnimationGroup2(QDeclarativeAbstractAnimation *animation)
    : QAnimationGroup2(new QParallelAnimationGroup2Private, animation)
{
}
QParallelAnimationGroup2::QParallelAnimationGroup2(QParallelAnimationGroup2Private *dd, QDeclarativeAbstractAnimation *animation)
    :QAnimationGroup2(dd, animation)
{
}
QParallelAnimationGroup2::~QParallelAnimationGroup2()
{
}

int QParallelAnimationGroup2::duration() const
{
    int ret = 0;

    for (int i = 0; i < d_func()->animations.size(); ++i) {
        QAbstractAnimation2 *animation = d_func()->animations.at(i);
        const int currentDuration = animation->totalDuration();
        if (currentDuration == -1)
            return -1; // Undetermined length

        ret = qMax(ret, currentDuration);
    }

    return ret;
}

void QParallelAnimationGroup2::updateCurrentTime(int currentTime)
{
    if (d_func()->animations.isEmpty())
        return;

    if (d_func()->currentLoop > d_func()->lastLoop) {
        // simulate completion of the loop
        int dura = duration();
        if (dura > 0) {
            for (int i = 0; i < d_func()->animations.size(); ++i) {
                QAbstractAnimation2 *animation = d_func()->animations.at(i);
                if (animation->state() != QAbstractAnimation2::Stopped)
                    d_func()->animations.at(i)->setCurrentTime(dura);   // will stop
            }
        }
    } else if (d_func()->currentLoop < d_func()->lastLoop) {
        // simulate completion of the loop seeking backwards
        for (int i = 0; i < d_func()->animations.size(); ++i) {
            QAbstractAnimation2 *animation = d_func()->animations.at(i);
            //we need to make sure the animation is in the right state
            //and then rewind it
            d_func()->applyGroupState(animation);
            animation->setCurrentTime(0);
            animation->stop();
        }
    }

#ifdef QANIMATION_DEBUG
    qDebug("QParallellAnimationGroup %5d: setCurrentTime(%d), loop:%d, last:%d, timeFwd:%d, lastcurrent:%d, %d",
        __LINE__, d_func()->currentTime, d_func()->currentLoop, d_func()->lastLoop, timeFwd, d_func()->lastCurrentTime, state());
#endif
    // finally move into the actual time of the current loop
    for (int i = 0; i < d_func()->animations.size(); ++i) {
        QAbstractAnimation2 *animation = d_func()->animations.at(i);
        const int dura = animation->totalDuration();
        //if the loopcount is bigger we should always start all animations
        if (d_func()->currentLoop > d_func()->lastLoop
            //if we're at the end of the animation, we need to start it if it wasn't already started in this loop
            //this happens in Backward direction where not all animations are started at the same time
            || d_func()->shouldAnimationStart(animation, d_func()->lastCurrentTime > dura /*startIfAtEnd*/)) {
            d_func()->applyGroupState(animation);
        }

        if (animation->state() == state()) {
            animation->setCurrentTime(currentTime);
            if (dura > 0 && currentTime > dura)
                animation->stop();
        }
    }
    d_func()->lastLoop = d_func()->currentLoop;
    d_func()->lastCurrentTime = currentTime;
}

void QParallelAnimationGroup2::updateState(QAbstractAnimation2::State newState,
                                          QAbstractAnimation2::State oldState)
{
    QAnimationGroup2::updateState(newState, oldState);

    switch (newState) {
    case Stopped:
        for (int i = 0; i < d_func()->animations.size(); ++i)
            d_func()->animations.at(i)->stop();
        d_func()->disconnectUncontrolledAnimations();
        break;
    case Paused:
        for (int i = 0; i < d_func()->animations.size(); ++i)
            if (d_func()->animations.at(i)->state() == Running)
                d_func()->animations.at(i)->pause();
        break;
    case Running:
        d_func()->connectUncontrolledAnimations();
        for (int i = 0; i < d_func()->animations.size(); ++i) {
            QAbstractAnimation2 *animation = d_func()->animations.at(i);
            if (oldState == Stopped)
                animation->stop();
            animation->setDirection(d_func()->direction);
            if (d_func()->shouldAnimationStart(animation, oldState == Stopped))
                animation->start();
        }
        break;
    }
}

bool QParallelAnimationGroup2Private::shouldAnimationStart(QAbstractAnimation2 *animation, bool startIfAtEnd) const
{
    const int dura = animation->totalDuration();

    if (dura == -1)
        return !isUncontrolledAnimationFinished(animation);

    if (startIfAtEnd)
        return currentTime <= dura;
    if (direction == QAbstractAnimation2::Forward)
        return currentTime < dura;
    else //direction == QAbstractAnimation2::Backward
        return currentTime && currentTime <= dura;
}

void QParallelAnimationGroup2Private::applyGroupState(QAbstractAnimation2 *animation)
{
    switch (state)
    {
    case QAbstractAnimation2::Running:
        animation->start();
        break;
    case QAbstractAnimation2::Paused:
        animation->pause();
        break;
    case QAbstractAnimation2::Stopped:
    default:
        break;
    }
}

void QParallelAnimationGroup2Private::animationRemoved(int index, QAbstractAnimation2 *anim)
{
    QAnimationGroup2Private::animationRemoved(index, anim);
    disconnectUncontrolledAnimation(anim);
}

void QParallelAnimationGroup2::updateDirection(QAbstractAnimation2::Direction direction)
{
    //we need to update the direction of the current animation
    if (state() != Stopped) {
        for (int i = 0; i < d_func()->animations.size(); ++i) {
            QAbstractAnimation2 *animation = d_func()->animations.at(i);
            animation->setDirection(direction);
        }
    } else {
        if (direction == Forward) {
            d_func()->lastLoop = 0;
            d_func()->lastCurrentTime = 0;
        } else {
            // Looping backwards with loopCount == -1 does not really work well...
            d_func()->lastLoop = (d_func()->loopCount == -1 ? 0 : d_func()->loopCount - 1);
            d_func()->lastCurrentTime = duration();
        }
    }
}

void QParallelAnimationGroup2::uncontrolledAnimationFinished(QAbstractAnimation2* animation)
{
    if (d_func()->isAnimationConnected(animation)) {
        Q_ASSERT(animation && animation->duration() == -1 || animation->loopCount() < 0);
        int uncontrolledRunningCount = 0;
        QHash<QAbstractAnimation2 *, int>::iterator it = d_func()->uncontrolledFinishTime.begin();
        while (it != d_func()->uncontrolledFinishTime.end()) {
            if (it.key() == animation) {
                *it = animation->currentTime();
            }
            if (it.value() == -1)
                ++uncontrolledRunningCount;
            ++it;
        }
        if (uncontrolledRunningCount > 0)
            return;

        int maxDuration = 0;
        for (int i = 0; i < d_func()->animations.size(); ++i)
            maxDuration = qMax(maxDuration, d_func()->animations.at(i)->totalDuration());

        if (d_func()->currentTime >= maxDuration)
            stop();
    }
}

QT_END_NAMESPACE

