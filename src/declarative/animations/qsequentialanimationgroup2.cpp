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

#include "private/qsequentialanimationgroup2_p.h"
#include "private/qpauseanimation2_p.h"
#include <QtCore/qdebug.h>



QT_BEGIN_NAMESPACE

bool QSequentialAnimationGroup2::atEnd() const
{
    // we try to detect if we're at the end of the group
    //this is true if the following conditions are true:
    // 1. we're in the last loop
    // 2. the direction is forward
    // 3. the current animation is the last one
    // 4. the current animation has reached its end
    const int animTotalCurrentTime = m_currentAnimation->currentTime();
    return (m_currentLoop == m_loopCount - 1
        && m_direction == QAbstractAnimation2::Forward
        && m_currentAnimation == m_animations.last()
        && animTotalCurrentTime == animationActualTotalDuration(m_currentAnimationIndex));
}

int QSequentialAnimationGroup2::animationActualTotalDuration(int index) const
{
    QAbstractAnimation2Pointer anim = m_animations.at(index);
    int ret = anim->totalDuration();
    if (ret == -1 && m_actualDuration.size() > index)
        ret = m_actualDuration.at(index); //we can try the actual duration there
    return ret;
}

QSequentialAnimationGroup2::AnimationIndex QSequentialAnimationGroup2::indexForCurrentTime() const
{
    Q_ASSERT(!m_animations.isEmpty());

    AnimationIndex ret;
    int duration = 0;

    for (int i = 0; i < m_animations.size(); ++i) {
        duration = animationActualTotalDuration(i);

        // 'animation' is the current animation if one of these reasons is true:
        // 1. it's duration is undefined
        // 2. it ends after msecs
        // 3. it is the last animation (this can happen in case there is at least 1 uncontrolled animation)
        // 4. it ends exactly in msecs and the direction is backwards
        if (duration == -1 || m_currentTime < (ret.timeOffset + duration)
            || (m_currentTime == (ret.timeOffset + duration) && m_direction == QAbstractAnimation2::Backward)) {
            ret.index = i;
            return ret;
        }

        // 'animation' has a non-null defined duration and is not the one at time 'msecs'.
        ret.timeOffset += duration;
    }

    // this can only happen when one of those conditions is true:
    // 1. the duration of the group is undefined and we passed its actual duration
    // 2. there are only 0-duration animations in the group
    ret.timeOffset -= duration;
    ret.index = m_animations.size() - 1;
    return ret;
}

void QSequentialAnimationGroup2::restart()
{
    // restarting the group by making the first/last animation the current one
    if (m_direction == QAbstractAnimation2::Forward) {
        m_lastLoop = 0;
        if (m_currentAnimationIndex == 0)
            activateCurrentAnimation();
        else
            setCurrentAnimation(0);
    } else { // direction == QAbstractAnimation2::Backward
        m_lastLoop = m_loopCount - 1;
        int index = m_animations.size() - 1;
        if (m_currentAnimationIndex == index)
            activateCurrentAnimation();
        else
            setCurrentAnimation(index);
    }
}

void QSequentialAnimationGroup2::advanceForwards(const AnimationIndex &newAnimationIndex)
{
    if (m_lastLoop < m_currentLoop) {
        // we need to fast forward to the end
        for (int i = m_currentAnimationIndex; i < m_animations.size(); ++i) {
            QAbstractAnimation2Pointer anim = m_animations.at(i);
            setCurrentAnimation(i, true);
            anim->setCurrentTime(animationActualTotalDuration(i));
        }
        // this will make sure the current animation is reset to the beginning
        if (m_animations.size() == 1)
            // we need to force activation because setCurrentAnimation will have no effect
            activateCurrentAnimation();
        else
            setCurrentAnimation(0, true);
    }

    // and now we need to fast forward from the current position to
    for (int i = m_currentAnimationIndex; i < newAnimationIndex.index; ++i) {     //### WRONG,
        QAbstractAnimation2Pointer anim = m_animations.at(i);
        setCurrentAnimation(i, true);
        anim->setCurrentTime(animationActualTotalDuration(i));
    }
    // setting the new current animation will happen later
}

void QSequentialAnimationGroup2::rewindForwards(const AnimationIndex &newAnimationIndex)
{
    if (m_lastLoop > m_currentLoop) {
        // we need to fast rewind to the beginning
        for (int i = m_currentAnimationIndex; i >= 0 ; --i) {
            QAbstractAnimation2Pointer anim = m_animations.at(i);
            setCurrentAnimation(i, true);
            anim->setCurrentTime(0);
        }
        // this will make sure the current animation is reset to the end
        if (m_animations.size() == 1)
            // we need to force activation because setCurrentAnimation will have no effect
            activateCurrentAnimation();
        else
            setCurrentAnimation(m_animations.count() - 1, true);
    }

    // and now we need to fast rewind from the current position to
    for (int i = m_currentAnimationIndex; i > newAnimationIndex.index; --i) {
        QAbstractAnimation2Pointer anim = m_animations.at(i);
        setCurrentAnimation(i, true);
        anim->setCurrentTime(0);
    }
    // setting the new current animation will happen later
}

QSequentialAnimationGroup2::QSequentialAnimationGroup2(QDeclarativeAbstractAnimation *animation)
    : QAnimationGroup2(animation)
    , m_currentAnimation(0)
    , m_currentAnimationIndex(-1)
    , m_lastLoop(0)
{
}

QSequentialAnimationGroup2::QSequentialAnimationGroup2(const QSequentialAnimationGroup2 &other)
    : QAnimationGroup2(other)
    , m_currentAnimation(other.m_currentAnimation)
    , m_currentAnimationIndex(other.m_currentAnimationIndex)
    , m_lastLoop(other.m_lastLoop)
{
}
QSequentialAnimationGroup2::~QSequentialAnimationGroup2()
{
}

QAbstractAnimation2Pointer QSequentialAnimationGroup2::currentAnimation() const
{
    return m_currentAnimation;
}

//only calculate once
int QSequentialAnimationGroup2::duration() const
{
    int ret = 0;

    for (int i = 0; i < m_animations.size(); ++i) {
        QAbstractAnimation2Pointer animation = m_animations.at(i);
        const int currentDuration = animation->totalDuration();
        if (currentDuration == -1)
            return -1; // Undetermined length

        ret += currentDuration;
    }

    return ret;
}

void QSequentialAnimationGroup2::updateCurrentTime(int currentTime)
{
    if (!m_currentAnimation)
        return;

    const QSequentialAnimationGroup2::AnimationIndex newAnimationIndex = indexForCurrentTime();

    // remove unneeded animations from actualDuration list
    while (newAnimationIndex.index < m_actualDuration.size())
        m_actualDuration.removeLast();

    // newAnimationIndex.index is the new current animation
    if (m_lastLoop < m_currentLoop
        || (m_lastLoop == m_currentLoop && m_currentAnimationIndex < newAnimationIndex.index)) {
            // advancing with forward direction is the same as rewinding with backwards direction
            advanceForwards(newAnimationIndex);
    } else if (m_lastLoop > m_currentLoop
        || (m_lastLoop == m_currentLoop && m_currentAnimationIndex > newAnimationIndex.index)) {
            // rewinding with forward direction is the same as advancing with backwards direction
            rewindForwards(newAnimationIndex);
    }

    setCurrentAnimation(newAnimationIndex.index);

    const int newCurrentTime = currentTime - newAnimationIndex.timeOffset;

    if (m_currentAnimation) {
        m_currentAnimation->setCurrentTime(newCurrentTime);
        if (atEnd()) {
            //we make sure that we don't exceed the duration here
            m_currentTime += m_currentAnimation->currentTime() - newCurrentTime;
            stop();
        }
    } else {
        //the only case where currentAnimation could be null
        //is when all animations have been removed
        Q_ASSERT(m_animations.isEmpty());
        m_currentTime = 0;
        stop();
    }

    m_lastLoop = m_currentLoop;
}

void QSequentialAnimationGroup2::updateState(QAbstractAnimation2::State newState,
                                            QAbstractAnimation2::State oldState)
{
    QAnimationGroup2::updateState(newState, oldState);

    if (!m_currentAnimation)
        return;

    switch (newState) {
    case Stopped:
        m_currentAnimation->stop();
        break;
    case Paused:
        if (oldState == m_currentAnimation->state()
            && oldState == QSequentialAnimationGroup2::Running) {
                m_currentAnimation->pause();
            }
        else
            restart();
        break;
    case Running:
        if (oldState == m_currentAnimation->state()
            && oldState == QSequentialAnimationGroup2::Paused)
            m_currentAnimation->start();
        else
            restart();
        break;
    }
}

void QSequentialAnimationGroup2::updateDirection(QAbstractAnimation2::Direction direction)
{
    // we need to update the direction of the current animation
    if (state() != Stopped && m_currentAnimation)
        m_currentAnimation->setDirection(direction);
}

void QSequentialAnimationGroup2::setCurrentAnimation(int index, bool intermediate)
{
    index = qMin(index, m_animations.count() - 1);

    if (index == -1) {
        Q_ASSERT(m_animations.isEmpty());
        m_currentAnimationIndex = -1;
        m_currentAnimation = 0;
        return;
    }

    // need these two checks below because this func can be called after the current animation
    // has been removed
    if (index == m_currentAnimationIndex && m_animations.at(index) == m_currentAnimation)
        return;

    // stop the old current animation
    if (m_currentAnimation)
        m_currentAnimation->stop();

    m_currentAnimation = m_animations.at(index);
    m_currentAnimationIndex = index;

//    currentAnimationChanged(currentAnimation);

    activateCurrentAnimation(intermediate);
}

void QSequentialAnimationGroup2::activateCurrentAnimation(bool intermediate)
{
    if (!m_currentAnimation || m_state == QSequentialAnimationGroup2::Stopped)
        return;

    m_currentAnimation->stop();

    // we ensure the direction is consistent with the group's direction
    m_currentAnimation->setDirection(m_direction);

    // connects to the finish signal of uncontrolled animations
    if (m_currentAnimation->totalDuration() == -1)
        connectUncontrolledAnimation(m_currentAnimation);

    m_currentAnimation->start();
    if (!intermediate && state() == QSequentialAnimationGroup2::Paused)
        m_currentAnimation->pause();
}

void QSequentialAnimationGroup2::uncontrolledAnimationFinished(QAbstractAnimation2Pointer animation)
{
    if (isAnimationConnected(animation)) {
        Q_ASSERT(animation == m_currentAnimation);

        // we trust the duration returned by the animation
        while (m_actualDuration.size() < (m_currentAnimationIndex + 1))
            m_actualDuration.append(-1);
        m_actualDuration[m_currentAnimationIndex] = m_currentAnimation->currentTime();

        disconnectUncontrolledAnimation(m_currentAnimation);

        if ((m_direction == QAbstractAnimation2::Forward && m_currentAnimation == m_animations.last())
            || (m_direction == QAbstractAnimation2::Backward && m_currentAnimationIndex == 0)) {
            // we don't handle looping of a group with undefined duration
            stop();
        } else if (m_direction == QAbstractAnimation2::Forward) {
            // set the current animation to be the next one
            setCurrentAnimation(m_currentAnimationIndex + 1);
        } else {
            // set the current animation to be the previous one
            setCurrentAnimation(m_currentAnimationIndex - 1);
        }
    }
}

void QSequentialAnimationGroup2::animationInsertedAt(int index)
{
    if (m_currentAnimation == 0)
        setCurrentAnimation(0); // initialize the current animation

    if (m_currentAnimationIndex == index
        && m_currentAnimation->currentTime() == 0 && m_currentAnimation->currentLoop() == 0) {
            //in this case we simply insert an animation before the current one has actually started
            setCurrentAnimation(index);
    }

    //we update m_currentAnimationIndex in case it has changed (the animation pointer is still valid)
    m_currentAnimationIndex = m_animations.indexOf(m_currentAnimation);

    if (index < m_currentAnimationIndex || m_currentLoop != 0) {
        qWarning("QSequentialGroup::insertAnimation only supports to add animations after the current one.");
        return; //we're not affected because it is added after the current one
    }
}

void QSequentialAnimationGroup2::animationRemoved(int index, QAbstractAnimation2Pointer anim)
{
    QAnimationGroup2::animationRemoved(index, anim);

    Q_ASSERT(m_currentAnimation); // currentAnimation should always be set

    if (m_actualDuration.size() > index)
        m_actualDuration.removeAt(index);

    const int currentIndex = m_animations.indexOf(m_currentAnimation);
    if (currentIndex == -1) {
        //we're removing the current animation

        disconnectUncontrolledAnimation(m_currentAnimation);

        if (index < m_animations.count())
            setCurrentAnimation(index); //let's try to take the next one
        else if (index > 0)
            setCurrentAnimation(index - 1);
        else// case all animations were removed
            setCurrentAnimation(-1);
    } else if (m_currentAnimationIndex > index) {
        m_currentAnimationIndex--;
    }

    // duration of the previous animations up to the current animation
    m_currentTime = 0;
    for (int i = 0; i < m_currentAnimationIndex; ++i) {
        const int current = animationActualTotalDuration(i);
        m_currentTime += current;
    }

    if (currentIndex != -1) {
        //the current animation is not the one being removed
        //so we add its current time to the current time of this group
        m_currentTime += m_currentAnimation->currentTime();
    }

    //let's also update the total current time
    m_totalCurrentTime =m_currentTime + m_loopCount * duration();
}

QT_END_NAMESPACE
