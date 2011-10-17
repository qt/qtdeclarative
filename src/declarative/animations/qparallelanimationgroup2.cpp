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
//#define QANIMATION_DEBUG



QT_BEGIN_NAMESPACE

QParallelAnimationGroup2::QParallelAnimationGroup2(QDeclarativeAbstractAnimation *animation)
    : QAnimationGroup2(animation)
    , m_lastLoop(0)
    , m_lastCurrentTime(0)
{
}

QParallelAnimationGroup2::QParallelAnimationGroup2(const QParallelAnimationGroup2 &other)
    : QAnimationGroup2(other)
    , m_lastLoop(other.m_lastLoop)
    , m_lastCurrentTime(other.m_lastCurrentTime)
{
}

QParallelAnimationGroup2::~QParallelAnimationGroup2()
{
}


//only calculate once
int QParallelAnimationGroup2::duration() const
{
    int ret = 0;

    for (int i = 0; i < m_animations.size(); ++i) {
        QAbstractAnimation2Pointer animation = m_animations.at(i);
        const int currentDuration = animation->totalDuration();
        if (currentDuration == -1)
            return -1; // Undetermined length

        ret = qMax(ret, currentDuration);
    }

    return ret;
}

void QParallelAnimationGroup2::updateCurrentTime(int currentTime)
{
    if (m_animations.isEmpty())
        return;

    if (m_currentLoop > m_lastLoop) {
        // simulate completion of the loop
        int dura = duration();
        if (dura > 0) {
            for (int i = 0; i < m_animations.size(); ++i) {
                QAbstractAnimation2Pointer animation = m_animations.at(i);
                if (animation->state() != QAbstractAnimation2::Stopped)
                    m_animations.at(i)->setCurrentTime(dura);   // will stop
            }
        }
    } else if (m_currentLoop < m_lastLoop) {
        // simulate completion of the loop seeking backwards
        for (int i = 0; i < m_animations.size(); ++i) {
            QAbstractAnimation2Pointer animation = m_animations.at(i);
            //we need to make sure the animation is in the right state
            //and then rewind it
            applyGroupState(animation);
            animation->setCurrentTime(0);
            animation->stop();
        }
    }

#ifdef QANIMATION_DEBUG
    qDebug("QParallellAnimationGroup %5d: setCurrentTime(%d), loop:%d, last:%d, timeFwd:%d, lastcurrent:%d, %d",
        __LINE__, m_currentTime, m_currentLoop, m_lastLoop, timeFwd, m_lastCurrentTime, state());
#endif
    // finally move into the actual time of the current loop
    for (int i = 0; i < m_animations.size(); ++i) {
        QAbstractAnimation2Pointer animation = m_animations.at(i);
        const int dura = animation->totalDuration();
        //if the loopcount is bigger we should always start all animations
        if (m_currentLoop > m_lastLoop
            //if we're at the end of the animation, we need to start it if it wasn't already started in this loop
            //this happens in Backward direction where not all animations are started at the same time
            || shouldAnimationStart(animation, m_lastCurrentTime > dura /*startIfAtEnd*/)) {
            applyGroupState(animation);
        }

        if (animation->state() == state()) {
            animation->setCurrentTime(m_currentTime);
            if (dura > 0 && m_currentTime > dura)
                animation->stop();
        }
    }
    m_lastLoop = m_currentLoop;
    m_lastCurrentTime = m_currentTime;
}

void QParallelAnimationGroup2::updateState(QAbstractAnimation2::State newState,
                                          QAbstractAnimation2::State oldState)
{
    QAnimationGroup2::updateState(newState, oldState);

    switch (newState) {
    case Stopped:
        for (int i = 0; i < m_animations.size(); ++i)
            m_animations.at(i)->stop();
        disconnectUncontrolledAnimations();
        break;
    case Paused:
        for (int i = 0; i < m_animations.size(); ++i)
            if (m_animations.at(i)->state() == Running)
                m_animations.at(i)->pause();
        break;
    case Running:
        connectUncontrolledAnimations();
        for (int i = 0; i < m_animations.size(); ++i) {
            QAbstractAnimation2Pointer animation = m_animations.at(i);
            if (oldState == Stopped)
                animation->stop();
            animation->setDirection(m_direction);
            if (shouldAnimationStart(animation, oldState == Stopped))
                animation->start();
        }
        break;
    }
}

bool QParallelAnimationGroup2::shouldAnimationStart(QAbstractAnimation2Pointer animation, bool startIfAtEnd) const
{
    const int dura = animation->totalDuration();

    if (dura == -1)
        return !isUncontrolledAnimationFinished(animation);

    if (startIfAtEnd)
        return m_currentTime <= dura;
    if (m_direction == QAbstractAnimation2::Forward)
        return m_currentTime < dura;
    else //direction == QAbstractAnimation2::Backward
        return m_currentTime && m_currentTime <= dura;
}

void QParallelAnimationGroup2::applyGroupState(QAbstractAnimation2Pointer animation)
{
    switch (m_state)
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

void QParallelAnimationGroup2::animationRemoved(int index, QAbstractAnimation2Pointer anim)
{
    QAnimationGroup2::animationRemoved(index, anim);
    disconnectUncontrolledAnimation(anim);
}

void QParallelAnimationGroup2::updateDirection(QAbstractAnimation2::Direction direction)
{
    //we need to update the direction of the current animation
    if (state() != Stopped) {
        for (int i = 0; i < m_animations.size(); ++i) {
            QAbstractAnimation2Pointer animation = m_animations.at(i);
            animation->setDirection(direction);
        }
    } else {
        if (direction == Forward) {
            m_lastLoop = 0;
            m_lastCurrentTime = 0;
        } else {
            // Looping backwards with loopCount == -1 does not really work well...
            m_lastLoop = (m_loopCount == -1 ? 0 : m_loopCount - 1);
            m_lastCurrentTime = duration();
        }
    }
}

void QParallelAnimationGroup2::uncontrolledAnimationFinished(QAbstractAnimation2Pointer animation)
{
    if (isAnimationConnected(animation)) {
        Q_ASSERT(animation && animation->duration() == -1 || animation->loopCount() < 0);
        int uncontrolledRunningCount = 0;
        QHash<QAbstractAnimation2Pointer , int>::iterator it = m_uncontrolledFinishTime.begin();
        while (it != m_uncontrolledFinishTime.end()) {
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
        for (int i = 0; i < m_animations.size(); ++i)
            maxDuration = qMax(maxDuration, m_animations.at(i)->totalDuration());

        if (m_currentTime >= maxDuration)
            stop();
    }
}

QT_END_NAMESPACE

