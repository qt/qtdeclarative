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

/*!
    \class QParallelAnimationGroup2
    \brief The QParallelAnimationGroup2 class provides a parallel group of animations.
    \since 4.6
    \ingroup animation

    QParallelAnimationGroup2--a \l{QAnimationGroup2}{container for
    animations}--starts all its animations when it is
    \l{QAbstractAnimation2::start()}{started} itself, i.e., runs all
    animations in parallel. The animation group finishes when the
    longest lasting animation has finished.

    You can treat QParallelAnimation as any other QAbstractAnimation2,
    e.g., pause, resume, or add it to other animation groups.

    \code
        QParallelAnimationGroup2 *group = new QParallelAnimationGroup2;
        group->addAnimation(anim1);
        group->addAnimation(anim2);

        group->start();
    \endcode

    In this example, \c anim1 and \c anim2 are two
    \l{QPropertyAnimation2}s that have already been set up.

    \sa QAnimationGroup2, QPropertyAnimation2, {The Animation Framework}
*/


#include "private/qparallelanimationgroup2_p.h"
#include "private/qparallelanimationgroup2_p_p.h"
//#define QANIMATION_DEBUG



QT_BEGIN_NAMESPACE

/*!
    Constructs a QParallelAnimationGroup2.
    \a parent is passed to QObject's constructor.
*/
QParallelAnimationGroup2::QParallelAnimationGroup2(QObject *parent)
    : QAnimationGroup2(*new QParallelAnimationGroup2Private, parent)
{
}

/*!
    \internal
*/
QParallelAnimationGroup2::QParallelAnimationGroup2(QParallelAnimationGroup2Private &dd,
                                                 QObject *parent)
    : QAnimationGroup2(dd, parent)
{
}

/*!
    Destroys the animation group. It will also destroy all its animations.
*/
QParallelAnimationGroup2::~QParallelAnimationGroup2()
{
}

/*!
    \reimp
*/
int QParallelAnimationGroup2::duration() const
{
    Q_D(const QParallelAnimationGroup2);
    int ret = 0;

    for (int i = 0; i < d->animations.size(); ++i) {
        QAbstractAnimation2 *animation = d->animations.at(i);
        const int currentDuration = animation->totalDuration();
        if (currentDuration == -1)
            return -1; // Undetermined length

        ret = qMax(ret, currentDuration);
    }

    return ret;
}

/*!
    \reimp
*/
void QParallelAnimationGroup2::updateCurrentTime(int currentTime)
{
    Q_D(QParallelAnimationGroup2);
    if (d->animations.isEmpty())
        return;

    if (d->currentLoop > d->lastLoop) {
        // simulate completion of the loop
        int dura = duration();
        if (dura > 0) {
            for (int i = 0; i < d->animations.size(); ++i) {
                QAbstractAnimation2 *animation = d->animations.at(i);
                if (animation->state() != QAbstractAnimation2::Stopped)
                    d->animations.at(i)->setCurrentTime(dura);   // will stop
            }
        }
    } else if (d->currentLoop < d->lastLoop) {
        // simulate completion of the loop seeking backwards
        for (int i = 0; i < d->animations.size(); ++i) {
            QAbstractAnimation2 *animation = d->animations.at(i);
            //we need to make sure the animation is in the right state
            //and then rewind it
            d->applyGroupState(animation);
            animation->setCurrentTime(0);
            animation->stop();
        }
    }

#ifdef QANIMATION_DEBUG
    qDebug("QParallellAnimationGroup %5d: setCurrentTime(%d), loop:%d, last:%d, timeFwd:%d, lastcurrent:%d, %d",
        __LINE__, d->currentTime, d->currentLoop, d->lastLoop, timeFwd, d->lastCurrentTime, state());
#endif
    // finally move into the actual time of the current loop
    for (int i = 0; i < d->animations.size(); ++i) {
        QAbstractAnimation2 *animation = d->animations.at(i);
        const int dura = animation->totalDuration();
        //if the loopcount is bigger we should always start all animations
        if (d->currentLoop > d->lastLoop
            //if we're at the end of the animation, we need to start it if it wasn't already started in this loop
            //this happens in Backward direction where not all animations are started at the same time
            || d->shouldAnimationStart(animation, d->lastCurrentTime > dura /*startIfAtEnd*/)) {
            d->applyGroupState(animation);
        }

        if (animation->state() == state()) {
            animation->setCurrentTime(currentTime);
            if (dura > 0 && currentTime > dura)
                animation->stop();
        }
    }
    d->lastLoop = d->currentLoop;
    d->lastCurrentTime = currentTime;
}

/*!
    \reimp
*/
void QParallelAnimationGroup2::updateState(QAbstractAnimation2::State newState,
                                          QAbstractAnimation2::State oldState)
{
    Q_D(QParallelAnimationGroup2);
    QAnimationGroup2::updateState(newState, oldState);

    switch (newState) {
    case Stopped:
        for (int i = 0; i < d->animations.size(); ++i)
            d->animations.at(i)->stop();
        d->disconnectUncontrolledAnimations();
        break;
    case Paused:
        for (int i = 0; i < d->animations.size(); ++i)
            if (d->animations.at(i)->state() == Running)
                d->animations.at(i)->pause();
        break;
    case Running:
        d->connectUncontrolledAnimations();
        for (int i = 0; i < d->animations.size(); ++i) {
            QAbstractAnimation2 *animation = d->animations.at(i);
            if (oldState == Stopped)
                animation->stop();
            animation->setDirection(d->direction);
            if (d->shouldAnimationStart(animation, oldState == Stopped))
                animation->start();
        }
        break;
    }
}

void QParallelAnimationGroup2Private::_q_uncontrolledAnimationFinished()
{
    Q_Q(QParallelAnimationGroup2);

    QAbstractAnimation2 *animation = qobject_cast<QAbstractAnimation2 *>(q->sender());
    Q_ASSERT(animation);

    int uncontrolledRunningCount = 0;
    if (animation->duration() == -1 || animation->loopCount() < 0) {
        QHash<QAbstractAnimation2 *, int>::iterator it = uncontrolledFinishTime.begin();
        while (it != uncontrolledFinishTime.end()) {
            if (it.key() == animation) {
                *it = animation->currentTime();
            }
            if (it.value() == -1)
                ++uncontrolledRunningCount;
            ++it;
        }
    }

    if (uncontrolledRunningCount > 0)
        return;

    int maxDuration = 0;
    for (int i = 0; i < animations.size(); ++i)
        maxDuration = qMax(maxDuration, animations.at(i)->totalDuration());

    if (currentTime >= maxDuration)
        q->stop();
}

void QParallelAnimationGroup2Private::disconnectUncontrolledAnimations()
{
    QHash<QAbstractAnimation2 *, int>::iterator it = uncontrolledFinishTime.begin();
    while (it != uncontrolledFinishTime.end()) {
        disconnectUncontrolledAnimation(it.key());
        ++it;
    }

    uncontrolledFinishTime.clear();
}

void QParallelAnimationGroup2Private::connectUncontrolledAnimations()
{
    for (int i = 0; i < animations.size(); ++i) {
        QAbstractAnimation2 *animation = animations.at(i);
        if (animation->duration() == -1 || animation->loopCount() < 0) {
            uncontrolledFinishTime[animation] = -1;
            connectUncontrolledAnimation(animation);
        }
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


bool QParallelAnimationGroup2Private::isUncontrolledAnimationFinished(QAbstractAnimation2 *anim) const
{
    return uncontrolledFinishTime.value(anim, -1) >= 0;
}

void QParallelAnimationGroup2Private::animationRemoved(int index, QAbstractAnimation2 *anim)
{
    QAnimationGroup2Private::animationRemoved(index, anim);
    disconnectUncontrolledAnimation(anim);
    uncontrolledFinishTime.remove(anim);
}

/*!
    \reimp
*/
void QParallelAnimationGroup2::updateDirection(QAbstractAnimation2::Direction direction)
{
    Q_D(QParallelAnimationGroup2);
    //we need to update the direction of the current animation
    if (state() != Stopped) {
        for (int i = 0; i < d->animations.size(); ++i) {
            QAbstractAnimation2 *animation = d->animations.at(i);
            animation->setDirection(direction);
        }
    } else {
        if (direction == Forward) {
            d->lastLoop = 0;
            d->lastCurrentTime = 0;
        } else {
            // Looping backwards with loopCount == -1 does not really work well...
            d->lastLoop = (d->loopCount == -1 ? 0 : d->loopCount - 1);
            d->lastCurrentTime = duration();
        }
    }
}

/*!
    \reimp
*/
bool QParallelAnimationGroup2::event(QEvent *event)
{
    return QAnimationGroup2::event(event);
}

QT_END_NAMESPACE

#include "moc_qparallelanimationgroup2_p.cpp"


