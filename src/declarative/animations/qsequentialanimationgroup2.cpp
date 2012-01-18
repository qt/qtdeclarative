/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtDeclarative module of the Qt Toolkit.
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

QT_BEGIN_NAMESPACE

QSequentialAnimationGroup2::QSequentialAnimationGroup2()
    : QAnimationGroup2()
    , m_currentAnimation(0)
    , m_previousLoop(0)
{
}

QSequentialAnimationGroup2::~QSequentialAnimationGroup2()
{
}

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
        && m_direction == Forward
        && !m_currentAnimation->nextSibling()
        && animTotalCurrentTime == animationActualTotalDuration(m_currentAnimation));
}

int QSequentialAnimationGroup2::animationActualTotalDuration(QAbstractAnimation2 *anim) const
{
    int ret = anim->totalDuration();
    if (ret == -1)
        ret = uncontrolledAnimationFinishTime(anim); //we can try the actual duration there
    return ret;
}

QSequentialAnimationGroup2::AnimationIndex QSequentialAnimationGroup2::indexForCurrentTime() const
{
    Q_ASSERT(firstChild());

    AnimationIndex ret;
    QAbstractAnimation2 *anim = 0;
    int duration = 0;

    for (anim = firstChild(); anim; anim = anim->nextSibling()) {
        duration = animationActualTotalDuration(anim);

        // 'animation' is the current animation if one of these reasons is true:
        // 1. it's duration is undefined
        // 2. it ends after msecs
        // 3. it is the last animation (this can happen in case there is at least 1 uncontrolled animation)
        // 4. it ends exactly in msecs and the direction is backwards
        if (duration == -1 || m_currentTime < (ret.timeOffset + duration)
            || (m_currentTime == (ret.timeOffset + duration) && m_direction == QAbstractAnimation2::Backward)) {
            ret.animation = anim;
            return ret;
        }

        if (anim == m_currentAnimation)
            ret.afterCurrent = true;

        // 'animation' has a non-null defined duration and is not the one at time 'msecs'.
        ret.timeOffset += duration;
    }

    // this can only happen when one of those conditions is true:
    // 1. the duration of the group is undefined and we passed its actual duration
    // 2. there are only 0-duration animations in the group
    ret.timeOffset -= duration;
    ret.animation = lastChild();
    return ret;
}

void QSequentialAnimationGroup2::restart()
{
    // restarting the group by making the first/last animation the current one
    if (m_direction == Forward) {
        m_previousLoop = 0;
        if (m_currentAnimation == firstChild())
            activateCurrentAnimation();
        else
            setCurrentAnimation(firstChild());
    }
    else { // direction == Backward
        m_previousLoop = m_loopCount - 1;
        if (m_currentAnimation == lastChild())
            activateCurrentAnimation();
        else
            setCurrentAnimation(lastChild());
    }
}

void QSequentialAnimationGroup2::advanceForwards(const AnimationIndex &newAnimationIndex)
{
    if (m_previousLoop < m_currentLoop) {
        // we need to fast forward to the end
        for (QAbstractAnimation2 *anim = m_currentAnimation; anim; anim = anim->nextSibling()) {
            setCurrentAnimation(anim, true);
            anim->setCurrentTime(animationActualTotalDuration(anim));
        }
        // this will make sure the current animation is reset to the beginning
        if (firstChild() && !firstChild()->nextSibling())   //count == 1
            // we need to force activation because setCurrentAnimation will have no effect
            activateCurrentAnimation();
        else
            setCurrentAnimation(firstChild(), true);
    }

    // and now we need to fast forward from the current position to
    for (QAbstractAnimation2 *anim = m_currentAnimation; anim && anim != newAnimationIndex.animation; anim = anim->nextSibling()) {     //### WRONG,
        setCurrentAnimation(anim, true);
        anim->setCurrentTime(animationActualTotalDuration(anim));
    }
    // setting the new current animation will happen later
}

void QSequentialAnimationGroup2::rewindForwards(const AnimationIndex &newAnimationIndex)
{
    if (m_previousLoop > m_currentLoop) {
        // we need to fast rewind to the beginning
        for (QAbstractAnimation2 *anim = m_currentAnimation; anim; anim = anim->previousSibling()) {
            setCurrentAnimation(anim, true);
            anim->setCurrentTime(0);
        }
        // this will make sure the current animation is reset to the end
        if (lastChild() && !lastChild()->previousSibling())   //count == 1
            // we need to force activation because setCurrentAnimation will have no effect
            activateCurrentAnimation();
        else {
            setCurrentAnimation(lastChild(), true);
        }
    }

    // and now we need to fast rewind from the current position to
    for (QAbstractAnimation2 *anim = m_currentAnimation; anim && anim != newAnimationIndex.animation; anim = anim->previousSibling()) {
        setCurrentAnimation(anim, true);
        anim->setCurrentTime(0);
    }
    // setting the new current animation will happen later
}

int QSequentialAnimationGroup2::duration() const
{
    int ret = 0;

    for (QAbstractAnimation2 *anim = firstChild(); anim; anim = anim->nextSibling()) {
        const int currentDuration = anim->totalDuration();
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

    // newAnimationIndex.index is the new current animation
    if (m_previousLoop < m_currentLoop
        || (m_previousLoop == m_currentLoop && m_currentAnimation != newAnimationIndex.animation && newAnimationIndex.afterCurrent)) {
            // advancing with forward direction is the same as rewinding with backwards direction
            advanceForwards(newAnimationIndex);
    } else if (m_previousLoop > m_currentLoop
        || (m_previousLoop == m_currentLoop && m_currentAnimation != newAnimationIndex.animation && !newAnimationIndex.afterCurrent)) {
            // rewinding with forward direction is the same as advancing with backwards direction
            rewindForwards(newAnimationIndex);
    }

    setCurrentAnimation(newAnimationIndex.animation);

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
        Q_ASSERT(!firstChild());
        m_currentTime = 0;
        stop();
    }

    m_previousLoop = m_currentLoop;
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
        if (oldState == m_currentAnimation->state() && oldState == Running)
            m_currentAnimation->pause();
        else
            restart();
        break;
    case Running:
        if (oldState == m_currentAnimation->state() && oldState == Paused)
            m_currentAnimation->start();
        else
            restart();
        break;
    }
}

void QSequentialAnimationGroup2::updateDirection(QAbstractAnimation2::Direction direction)
{
    // we need to update the direction of the current animation
    if (!isStopped() && m_currentAnimation)
        m_currentAnimation->setDirection(direction);
}

void QSequentialAnimationGroup2::setCurrentAnimation(QAbstractAnimation2 *anim, bool intermediate)
{
    if (!anim) {
        Q_ASSERT(!firstChild());
        m_currentAnimation = 0;
        return;
    }

    if (anim == m_currentAnimation)
        return;

    // stop the old current animation
    if (m_currentAnimation)
        m_currentAnimation->stop();

    m_currentAnimation = anim;

    activateCurrentAnimation(intermediate);
}

void QSequentialAnimationGroup2::activateCurrentAnimation(bool intermediate)
{
    if (!m_currentAnimation || isStopped())
        return;

    m_currentAnimation->stop();

    // we ensure the direction is consistent with the group's direction
    m_currentAnimation->setDirection(m_direction);

    // reset the finish time of the animation if it is uncontrolled
    if (m_currentAnimation->totalDuration() == -1)
        resetUncontrolledAnimationFinishTime(m_currentAnimation);

    m_currentAnimation->start();
    if (!intermediate && isPaused())
        m_currentAnimation->pause();
}

void QSequentialAnimationGroup2::uncontrolledAnimationFinished(QAbstractAnimation2 *animation)
{
    Q_ASSERT(animation == m_currentAnimation);

    setUncontrolledAnimationFinishTime(m_currentAnimation, m_currentAnimation->currentTime());

    if ((m_direction == Forward && m_currentAnimation == lastChild())
        || (m_direction == Backward && m_currentAnimation == firstChild())) {
        // we don't handle looping of a group with undefined duration
        stop();
    } else if (m_direction == Forward) {
        // set the current animation to be the next one
        setCurrentAnimation(m_currentAnimation->nextSibling());
    } else {
        // set the current animation to be the previous one
        setCurrentAnimation(m_currentAnimation->previousSibling());
    }
}

void QSequentialAnimationGroup2::animationInserted(QAbstractAnimation2 *anim)
{
    if (m_currentAnimation == 0)
        setCurrentAnimation(firstChild()); // initialize the current animation

    if (m_currentAnimation == anim->nextSibling()
        && m_currentAnimation->currentTime() == 0 && m_currentAnimation->currentLoop() == 0) {
            //in this case we simply insert the animation before the current one has actually started
            setCurrentAnimation(anim);
    }

//    TODO
//    if (index < m_currentAnimationIndex || m_currentLoop != 0) {
//        qWarning("QSequentialGroup::insertAnimation only supports to add animations after the current one.");
//        return; //we're not affected because it is added after the current one
//    }
}

void QSequentialAnimationGroup2::animationRemoved(QAbstractAnimation2 *anim, QAbstractAnimation2 *prev, QAbstractAnimation2 *next)
{
    QAnimationGroup2::animationRemoved(anim, prev, next);

    Q_ASSERT(m_currentAnimation); // currentAnimation should always be set

    bool removingCurrent = anim == m_currentAnimation;
    if (removingCurrent) {
        if (next)
            setCurrentAnimation(next); //let's try to take the next one
        else if (prev)
            setCurrentAnimation(prev);
        else// case all animations were removed
            setCurrentAnimation(0);
    }

    // duration of the previous animations up to the current animation
    m_currentTime = 0;
    for (QAbstractAnimation2 *anim = firstChild(); anim; anim = anim->nextSibling()) {
        if (anim == m_currentAnimation)
            break;
        m_currentTime += animationActualTotalDuration(anim);

    }

    if (!removingCurrent) {
        //the current animation is not the one being removed
        //so we add its current time to the current time of this group
        m_currentTime += m_currentAnimation->currentTime();
    }

    //let's also update the total current time
    m_totalCurrentTime = m_currentTime + m_loopCount * duration();
}

QT_END_NAMESPACE
