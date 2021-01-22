/****************************************************************************
**
** Copyright (C) 2016 Jolla Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
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

#include "private/qcontinuinganimationgroupjob_p.h"
#include "private/qanimationjobutil_p.h"

QT_BEGIN_NAMESPACE

QContinuingAnimationGroupJob::QContinuingAnimationGroupJob()
{
}

QContinuingAnimationGroupJob::~QContinuingAnimationGroupJob()
{
}

void QContinuingAnimationGroupJob::updateCurrentTime(int /*currentTime*/)
{
    Q_ASSERT(firstChild());

    for (QAbstractAnimationJob *animation = firstChild(); animation; animation = animation->nextSibling()) {
        if (animation->state() == state()) {
            RETURN_IF_DELETED(animation->setCurrentTime(m_currentTime));
        }
    }
}

void QContinuingAnimationGroupJob::updateState(QAbstractAnimationJob::State newState,
                                          QAbstractAnimationJob::State oldState)
{
    QAnimationGroupJob::updateState(newState, oldState);

    switch (newState) {
    case Stopped:
        for (QAbstractAnimationJob *animation = firstChild(); animation; animation = animation->nextSibling())
            animation->stop();
        break;
    case Paused:
        for (QAbstractAnimationJob *animation = firstChild(); animation; animation = animation->nextSibling())
            if (animation->isRunning())
                animation->pause();
        break;
    case Running:
        if (!firstChild()) {
            stop();
            return;
        }
        for (QAbstractAnimationJob *animation = firstChild(); animation; animation = animation->nextSibling()) {
            resetUncontrolledAnimationFinishTime(animation);
            animation->setDirection(m_direction);
            animation->start();
        }
        break;
    }
}

void QContinuingAnimationGroupJob::updateDirection(QAbstractAnimationJob::Direction direction)
{
    if (!isStopped()) {
        for (QAbstractAnimationJob *animation = firstChild(); animation; animation = animation->nextSibling()) {
            animation->setDirection(direction);
        }
    }
}

void QContinuingAnimationGroupJob::uncontrolledAnimationFinished(QAbstractAnimationJob *animation)
{
    Q_ASSERT(animation && (animation->duration() == -1));
    int uncontrolledRunningCount = 0;

    for (QAbstractAnimationJob *child = firstChild(); child; child = child->nextSibling()) {
        if (child == animation)
            setUncontrolledAnimationFinishTime(animation, animation->currentTime());
        else if (uncontrolledAnimationFinishTime(child) == -1)
            ++uncontrolledRunningCount;
    }

    if (uncontrolledRunningCount > 0)
        return;

    setUncontrolledAnimationFinishTime(this, currentTime());
    stop();
}

void QContinuingAnimationGroupJob::debugAnimation(QDebug d) const
{
    d << "ContinuingAnimationGroupJob(" << Qt::hex << (const void *) this << Qt::dec << ")";

    debugChildren(d);
}

QT_END_NAMESPACE

