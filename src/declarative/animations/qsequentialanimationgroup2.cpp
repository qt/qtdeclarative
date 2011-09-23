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
#include "private/qsequentialanimationgroup2_p_p.h"

#include "private/qpauseanimation2_p.h"

#include <QtCore/qdebug.h>



QT_BEGIN_NAMESPACE

bool QSequentialAnimationGroup2Private::atEnd() const
{
    // we try to detect if we're at the end of the group
    //this is true if the following conditions are true:
    // 1. we're in the last loop
    // 2. the direction is forward
    // 3. the current animation is the last one
    // 4. the current animation has reached its end
    const int animTotalCurrentTime = QAbstractAnimation2Private::get(currentAnimation)->totalCurrentTime;
    return (currentLoop == loopCount - 1
        && direction == QAbstractAnimation2::Forward
        && currentAnimation == animations.last()
        && animTotalCurrentTime == animationActualTotalDuration(currentAnimationIndex));
}

int QSequentialAnimationGroup2Private::animationActualTotalDuration(int index) const
{
    QAbstractAnimation2 *anim = animations.at(index);
    int ret = anim->totalDuration();
    if (ret == -1 && actualDuration.size() > index)
        ret = actualDuration.at(index); //we can try the actual duration there
    return ret;
}

QSequentialAnimationGroup2Private::AnimationIndex QSequentialAnimationGroup2Private::indexForCurrentTime() const
{
    Q_ASSERT(!animations.isEmpty());

    AnimationIndex ret;
    int duration = 0;

    for (int i = 0; i < animations.size(); ++i) {
        duration = animationActualTotalDuration(i);

        // 'animation' is the current animation if one of these reasons is true:
        // 1. it's duration is undefined
        // 2. it ends after msecs
        // 3. it is the last animation (this can happen in case there is at least 1 uncontrolled animation)
        // 4. it ends exactly in msecs and the direction is backwards
        if (duration == -1 || currentTime < (ret.timeOffset + duration)
            || (currentTime == (ret.timeOffset + duration) && direction == QAbstractAnimation2::Backward)) {
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
    ret.index = animations.size() - 1;
    return ret;
}

void QSequentialAnimationGroup2Private::restart()
{
    // restarting the group by making the first/last animation the current one
    if (direction == QAbstractAnimation2::Forward) {
        lastLoop = 0;
        if (currentAnimationIndex == 0)
            activateCurrentAnimation();
        else
            setCurrentAnimation(0);
    } else { // direction == QAbstractAnimation2::Backward
        lastLoop = loopCount - 1;
        int index = animations.size() - 1;
        if (currentAnimationIndex == index)
            activateCurrentAnimation();
        else
            setCurrentAnimation(index);
    }
}

void QSequentialAnimationGroup2Private::advanceForwards(const AnimationIndex &newAnimationIndex)
{
    if (lastLoop < currentLoop) {
        // we need to fast forward to the end
        for (int i = currentAnimationIndex; i < animations.size(); ++i) {
            QAbstractAnimation2 *anim = animations.at(i);
            setCurrentAnimation(i, true);
            anim->setCurrentTime(animationActualTotalDuration(i));
        }
        // this will make sure the current animation is reset to the beginning
        if (animations.size() == 1)
            // we need to force activation because setCurrentAnimation will have no effect
            activateCurrentAnimation();
        else
            setCurrentAnimation(0, true);
    }

    // and now we need to fast forward from the current position to
    for (int i = currentAnimationIndex; i < newAnimationIndex.index; ++i) {     //### WRONG,
        QAbstractAnimation2 *anim = animations.at(i);
        setCurrentAnimation(i, true);
        anim->setCurrentTime(animationActualTotalDuration(i));
    }
    // setting the new current animation will happen later
}

void QSequentialAnimationGroup2Private::rewindForwards(const AnimationIndex &newAnimationIndex)
{
    if (lastLoop > currentLoop) {
        // we need to fast rewind to the beginning
        for (int i = currentAnimationIndex; i >= 0 ; --i) {
            QAbstractAnimation2 *anim = animations.at(i);
            setCurrentAnimation(i, true);
            anim->setCurrentTime(0);
        }
        // this will make sure the current animation is reset to the end
        if (animations.size() == 1)
            // we need to force activation because setCurrentAnimation will have no effect
            activateCurrentAnimation();
        else
            setCurrentAnimation(animations.count() - 1, true);
    }

    // and now we need to fast rewind from the current position to
    for (int i = currentAnimationIndex; i > newAnimationIndex.index; --i) {
        QAbstractAnimation2 *anim = animations.at(i);
        setCurrentAnimation(i, true);
        anim->setCurrentTime(0);
    }
    // setting the new current animation will happen later
}

QSequentialAnimationGroup2::QSequentialAnimationGroup2(QDeclarativeAbstractAnimation *animation)
    : QAnimationGroup2(new QSequentialAnimationGroup2Private, animation)
{
}

QSequentialAnimationGroup2::QSequentialAnimationGroup2(QSequentialAnimationGroup2Private *dd,
                                                     QDeclarativeAbstractAnimation *animation)
    : QAnimationGroup2(dd, animation)
{
}

QSequentialAnimationGroup2::~QSequentialAnimationGroup2()
{
}

QPauseAnimation2 *QSequentialAnimationGroup2::addPause(int msecs)
{
    QPauseAnimation2 *pause = new QPauseAnimation2(msecs);
    addAnimation(pause);
    return pause;
}

QPauseAnimation2 *QSequentialAnimationGroup2::insertPause(int index, int msecs)
{
    if (index < 0 || index > d_func()->animations.size()) {
        qWarning("QSequentialAnimationGroup2::insertPause: index is out of bounds");
        return 0;
    }

    QPauseAnimation2 *pause = new QPauseAnimation2(msecs);
    insertAnimation(index, pause);
    return pause;
}


QAbstractAnimation2 *QSequentialAnimationGroup2::currentAnimation() const
{
    return d_func()->currentAnimation;
}

int QSequentialAnimationGroup2::duration() const
{
    int ret = 0;

    for (int i = 0; i < d_func()->animations.size(); ++i) {
        QAbstractAnimation2 *animation = d_func()->animations.at(i);
        const int currentDuration = animation->totalDuration();
        if (currentDuration == -1)
            return -1; // Undetermined length

        ret += currentDuration;
    }

    return ret;
}

//void QSequentialAnimationGroup2::currentAnimationChanged(QAbstractAnimation2 *current)
//{
//    Q_UNUSED(current);
//    qDebug() << "currentAnimationChanged";
//}

void QSequentialAnimationGroup2::updateCurrentTime(int currentTime)
{
    if (!d_func()->currentAnimation)
        return;

    const QSequentialAnimationGroup2Private::AnimationIndex newAnimationIndex = d_func()->indexForCurrentTime();

    // remove unneeded animations from actualDuration list
    while (newAnimationIndex.index < d_func()->actualDuration.size())
        d_func()->actualDuration.removeLast();

    // newAnimationIndex.index is the new current animation
    if (d_func()->lastLoop < d_func()->currentLoop
        || (d_func()->lastLoop == d_func()->currentLoop && d_func()->currentAnimationIndex < newAnimationIndex.index)) {
            // advancing with forward direction is the same as rewinding with backwards direction
            d_func()->advanceForwards(newAnimationIndex);
    } else if (d_func()->lastLoop > d_func()->currentLoop
        || (d_func()->lastLoop == d_func()->currentLoop && d_func()->currentAnimationIndex > newAnimationIndex.index)) {
            // rewinding with forward direction is the same as advancing with backwards direction
            d_func()->rewindForwards(newAnimationIndex);
    }

    d_func()->setCurrentAnimation(newAnimationIndex.index);

    const int newCurrentTime = currentTime - newAnimationIndex.timeOffset;

    if (d_func()->currentAnimation) {
        d_func()->currentAnimation->setCurrentTime(newCurrentTime);
        if (d_func()->atEnd()) {
            //we make sure that we don't exceed the duration here
            d_func()->currentTime += QAbstractAnimation2Private::get(d_func()->currentAnimation)->totalCurrentTime - newCurrentTime;
            stop();
        }
    } else {
        //the only case where currentAnimation could be null
        //is when all animations have been removed
        Q_ASSERT(d_func()->animations.isEmpty());
        d_func()->currentTime = 0;
        stop();
    }

    d_func()->lastLoop = d_func()->currentLoop;
}

void QSequentialAnimationGroup2::updateState(QAbstractAnimation2::State newState,
                                            QAbstractAnimation2::State oldState)
{
    QAnimationGroup2::updateState(newState, oldState);

    if (!d_func()->currentAnimation)
        return;

    switch (newState) {
    case Stopped:
        d_func()->currentAnimation->stop();
        break;
    case Paused:
        if (oldState == d_func()->currentAnimation->state()
            && oldState == QSequentialAnimationGroup2::Running) {
                d_func()->currentAnimation->pause();
            }
        else
            d_func()->restart();
        break;
    case Running:
        if (oldState == d_func()->currentAnimation->state()
            && oldState == QSequentialAnimationGroup2::Paused)
            d_func()->currentAnimation->start();
        else
            d_func()->restart();
        break;
    }
}

void QSequentialAnimationGroup2::updateDirection(QAbstractAnimation2::Direction direction)
{
    // we need to update the direction of the current animation
    if (state() != Stopped && d_func()->currentAnimation)
        d_func()->currentAnimation->setDirection(direction);
}

void QSequentialAnimationGroup2Private::setCurrentAnimation(int index, bool intermediate)
{
    index = qMin(index, animations.count() - 1);

    if (index == -1) {
        Q_ASSERT(animations.isEmpty());
        currentAnimationIndex = -1;
        currentAnimation = 0;
        return;
    }

    // need these two checks below because this func can be called after the current animation
    // has been removed
    if (index == currentAnimationIndex && animations.at(index) == currentAnimation)
        return;

    // stop the old current animation
    if (currentAnimation)
        currentAnimation->stop();

    currentAnimation = animations.at(index);
    currentAnimationIndex = index;

//    q->currentAnimationChanged(currentAnimation);

    activateCurrentAnimation(intermediate);
}

void QSequentialAnimationGroup2Private::activateCurrentAnimation(bool intermediate)
{
    if (!currentAnimation || state == QSequentialAnimationGroup2::Stopped)
        return;

    currentAnimation->stop();

    // we ensure the direction is consistent with the group's direction
    currentAnimation->setDirection(direction);

    // connects to the finish signal of uncontrolled animations
    if (currentAnimation->totalDuration() == -1)
        connectUncontrolledAnimation(currentAnimation);

    currentAnimation->start();
    if (!intermediate && state == QSequentialAnimationGroup2::Paused)
        currentAnimation->pause();
}

void QSequentialAnimationGroup2::uncontrolledAnimationFinished(QAbstractAnimation2* animation)
{
    if (d_func()->isAnimationConnected(animation)) {
        Q_ASSERT(animation == d_func()->currentAnimation);

        // we trust the duration returned by the animation
        while (d_func()->actualDuration.size() < (d_func()->currentAnimationIndex + 1))
            d_func()->actualDuration.append(-1);
        d_func()->actualDuration[d_func()->currentAnimationIndex] = d_func()->currentAnimation->currentTime();

        d_func()->disconnectUncontrolledAnimation(d_func()->currentAnimation);

        if ((d_func()->direction == QAbstractAnimation2::Forward && d_func()->currentAnimation == d_func()->animations.last())
            || (d_func()->direction == QAbstractAnimation2::Backward && d_func()->currentAnimationIndex == 0)) {
            // we don't handle looping of a group with undefined duration
            stop();
        } else if (d_func()->direction == QAbstractAnimation2::Forward) {
            // set the current animation to be the next one
            d_func()->setCurrentAnimation(d_func()->currentAnimationIndex + 1);
        } else {
            // set the current animation to be the previous one
            d_func()->setCurrentAnimation(d_func()->currentAnimationIndex - 1);
        }
    }
}

void QSequentialAnimationGroup2Private::animationInsertedAt(int index)
{
    if (currentAnimation == 0)
        setCurrentAnimation(0); // initialize the current animation

    if (currentAnimationIndex == index
        && currentAnimation->currentTime() == 0 && currentAnimation->currentLoop() == 0) {
            //in this case we simply insert an animation before the current one has actually started
            setCurrentAnimation(index);
    }

    //we update currentAnimationIndex in case it has changed (the animation pointer is still valid)
    currentAnimationIndex = animations.indexOf(currentAnimation);

    if (index < currentAnimationIndex || currentLoop != 0) {
        qWarning("QSequentialGroup::insertAnimation only supports to add animations after the current one.");
        return; //we're not affected because it is added after the current one
    }
}

void QSequentialAnimationGroup2Private::animationRemoved(int index, QAbstractAnimation2 *anim)
{
    QAnimationGroup2Private::animationRemoved(index, anim);

    Q_ASSERT(currentAnimation); // currentAnimation should always be set

    if (actualDuration.size() > index)
        actualDuration.removeAt(index);

    const int currentIndex = animations.indexOf(currentAnimation);
    if (currentIndex == -1) {
        //we're removing the current animation

        disconnectUncontrolledAnimation(currentAnimation);

        if (index < animations.count())
            setCurrentAnimation(index); //let's try to take the next one
        else if (index > 0)
            setCurrentAnimation(index - 1);
        else// case all animations were removed
            setCurrentAnimation(-1);
    } else if (currentAnimationIndex > index) {
        currentAnimationIndex--;
    }

    // duration of the previous animations up to the current animation
    currentTime = 0;
    for (int i = 0; i < currentAnimationIndex; ++i) {
        const int current = animationActualTotalDuration(i);
        currentTime += current;
    }

    if (currentIndex != -1) {
        //the current animation is not the one being removed
        //so we add its current time to the current time of this group
        currentTime += QAbstractAnimation2Private::get(currentAnimation)->totalCurrentTime;
    }

    //let's also update the total current time
    totalCurrentTime = currentTime + loopCount * q->duration();
}

QT_END_NAMESPACE
