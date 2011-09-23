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

#ifndef QABSTRACTANIMATION2_P_P_H
#define QABSTRACTANIMATION2_P_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of QIODevice. This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/qbasictimer.h>
#include <QtCore/qdatetime.h>
#include <QtCore/qtimer.h>
#include <QtCore/qelapsedtimer.h>
#include <private/qobject_p.h>
#include "private/qabstractanimation2_p.h"

#ifdef Q_OS_WIN
#include <qt_windows.h>
#endif



QT_BEGIN_NAMESPACE

class QAnimationGroup2;
class QAbstractAnimation2;
class QAbstractAnimation2Private : public QObjectPrivate
{
public:
    QAbstractAnimation2Private()
        : state(QAbstractAnimation2::Stopped),
          direction(QAbstractAnimation2::Forward),
          totalCurrentTime(0),
          currentTime(0),
          loopCount(1),
          currentLoop(0),
          deleteWhenStopped(false),
          hasRegisteredTimer(false),
          isPause(false),
          isGroup(false),
          group(0)
    {
    }

    virtual ~QAbstractAnimation2Private() {}

    static QAbstractAnimation2Private *get(QAbstractAnimation2 *q)
    {
        return q->d_func();
    }

    QAbstractAnimation2::State state;
    QAbstractAnimation2::Direction direction;
    void setState(QAbstractAnimation2::State state);

    int totalCurrentTime;
    int currentTime;
    int loopCount;
    int currentLoop;

    bool deleteWhenStopped;
    bool hasRegisteredTimer;
    bool isPause;
    bool isGroup;

    QAnimationGroup2 *group;

private:
    Q_DECLARE_PUBLIC(QAbstractAnimation2)
};


class QUnifiedTimer;
class QDefaultAnimationDriver : public QAnimationDriver2
{
    Q_OBJECT
public:
    QDefaultAnimationDriver(QUnifiedTimer *timer);
    void timerEvent(QTimerEvent *e);

private Q_SLOTS:
    void startTimer();
    void stopTimer();

private:
    QBasicTimer m_timer;
    QUnifiedTimer *m_unified_timer;
};

class Q_CORE_EXPORT QAnimationDriver2Private : public QObjectPrivate
{
public:
    QAnimationDriver2Private() : running(false) {}
    bool running;
};

typedef QElapsedTimer ElapsedTimer;

class Q_CORE_EXPORT QUnifiedTimer : public QObject
{
private:
    QUnifiedTimer();

public:
    //XXX this is needed by dui
    static QUnifiedTimer *instance();
    static QUnifiedTimer *instance(bool create);

    static void registerAnimation(QAbstractAnimation2 *animation, bool isTopLevel);
    static void unregisterAnimation(QAbstractAnimation2 *animation);

    //defines the timing interval. Default is DEFAULT_TIMER_INTERVAL
    void setTimingInterval(int interval);

    /*
       this allows to have a consistent timer interval at each tick from the timer
       not taking the real time that passed into account.
    */
    void setConsistentTiming(bool consistent) { consistentTiming = consistent; }

    //these facilitate fine-tuning of complex animations
    void setSlowModeEnabled(bool enabled) { slowMode = enabled; }
    void setSlowdownFactor(qreal factor) { slowdownFactor = factor; }

    /*
        this is used for updating the currentTime of all animations in case the pause
        timer is active or, otherwise, only of the animation passed as parameter.
    */
    static void ensureTimerUpdate();

    /*
        this will evaluate the need of restarting the pause timer in case there is still
        some pause animations running.
    */
    static void updateAnimationTimer();

    void installAnimationDriver(QAnimationDriver2 *driver);
    void uninstallAnimationDriver(QAnimationDriver2 *driver);
    bool canUninstallAnimationDriver(QAnimationDriver2 *driver);

    void restartAnimationTimer();
    void updateAnimationsTime(qint64 timeStep);

    //useful for profiling/debugging
    int runningAnimationCount() { return animations.count(); }

protected:
    void timerEvent(QTimerEvent *);

private:
    friend class QDefaultAnimationDriver;
    friend class QAnimationDriver2;

    QAnimationDriver2 *driver;
    QDefaultAnimationDriver defaultDriver;

    QBasicTimer animationTimer;
    // timer used to delay the check if we should start/stop the animation timer
    QBasicTimer startStopAnimationTimer;

    ElapsedTimer time;

    qint64 lastTick;
    int timingInterval;
    int currentAnimationIdx;
    bool insideTick;
    bool consistentTiming;
    bool slowMode;

    // This factor will be used to divide the DEFAULT_TIMER_INTERVAL at each tick
    // when slowMode is enabled. Setting it to 0 or higher than DEFAULT_TIMER_INTERVAL (16)
    // stops all animations.
    qreal slowdownFactor;

    // bool to indicate that only pause animations are active
    bool isPauseTimerActive;

    QList<QAbstractAnimation2*> animations, animationsToStart;

    // this is the count of running animations that are not a group neither a pause animation
    int runningLeafAnimations;
    QList<QAbstractAnimation2*> runningPauseAnimations;

    void registerRunningAnimation(QAbstractAnimation2 *animation);
    void unregisterRunningAnimation(QAbstractAnimation2 *animation);

    int closestPauseAnimationTimeToFinish();
};

QT_END_NAMESPACE



#endif //QABSTRACTANIMATION2_P_P_H
