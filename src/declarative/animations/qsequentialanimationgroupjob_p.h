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

#ifndef QSEQUENTIALANIMATIONGROUPJOB_P_H
#define QSEQUENTIALANIMATIONGROUPJOB_P_H

#include <private/qanimationgroupjob_p.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Declarative)

class QPauseAnimationJob;
class Q_DECLARATIVE_EXPORT QSequentialAnimationGroupJob : public QAnimationGroupJob
{
    Q_DISABLE_COPY(QSequentialAnimationGroupJob)
public:
    QSequentialAnimationGroupJob();
    ~QSequentialAnimationGroupJob();

    int duration() const;

    QAbstractAnimationJob *currentAnimation() const { return m_currentAnimation; }

protected:
    void updateCurrentTime(int);
    void updateState(QAbstractAnimationJob::State newState, QAbstractAnimationJob::State oldState);
    void updateDirection(QAbstractAnimationJob::Direction direction);
    void uncontrolledAnimationFinished(QAbstractAnimationJob *animation);

private:
    struct AnimationIndex
    {
        AnimationIndex() : afterCurrent(false), timeOffset(0), animation(0) {}
        // AnimationIndex points to the animation at timeOffset, skipping 0 duration animations.
        // Note that the index semantic is slightly different depending on the direction.
        bool afterCurrent;  //whether animation is before or after m_currentAnimation   //TODO: make enum Before/After/Same
        int timeOffset; // time offset when the animation at index starts.
        QAbstractAnimationJob *animation; //points to the animation at timeOffset
    };

    int animationActualTotalDuration(QAbstractAnimationJob *anim) const;
    AnimationIndex indexForCurrentTime() const;

    void setCurrentAnimation(QAbstractAnimationJob *anim, bool intermediate = false);
    void activateCurrentAnimation(bool intermediate = false);

    void animationInserted(QAbstractAnimationJob *anim);
    void animationRemoved(QAbstractAnimationJob *anim,QAbstractAnimationJob*,QAbstractAnimationJob*);

    bool atEnd() const;

    void restart();

    // handle time changes
    void rewindForwards(const AnimationIndex &newAnimationIndex);
    void advanceForwards(const AnimationIndex &newAnimationIndex);

    //state
    QAbstractAnimationJob *m_currentAnimation;
    int m_previousLoop;
};

QT_END_NAMESPACE

QT_END_HEADER

#endif //QSEQUENTIALANIMATIONGROUPJOB_P_H
