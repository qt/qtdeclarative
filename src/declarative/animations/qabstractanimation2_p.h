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

#ifndef QABSTRACTANIMATION2_P_H
#define QABSTRACTANIMATION2_P_H


#include <QtCore/qbasictimer.h>
#include <QtCore/qdatetime.h>
#include <QtCore/qtimer.h>
#include <QtCore/qelapsedtimer.h>
#include <private/qobject_p.h>
#include <QtCore/QObject>
#include "private/qdeclarativerefcount_p.h"
#include "private/qdeclarativeguard_p.h"
#ifdef Q_OS_WIN
#include <qt_windows.h>
#endif

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Declarative)

class QAnimationGroup2;
class QSequentialAnimationGroup2;
class QAnimationDriver2;
class QDeclarativeAbstractAnimation;

class Q_DECLARATIVE_EXPORT QAbstractAnimation2 : public QDeclarativeRefCount
{
public:
    enum AnimationType {
        DefaultAnimation,
        GroupAnimation,
        ParallelAnimation,
        SequentialAnimation,
        PauseAnimation,
        SmoothedAnimation,
        ActionAnimation,
        BulkValueAnimation,
        ParticleSystemAnimation,
        SpringAnimation
    };

    enum Direction {
        Forward,
        Backward
    };

    enum State {
        Stopped,
        Paused,
        Running
    };

    QAbstractAnimation2(QDeclarativeAbstractAnimation* animation = 0);
    QAbstractAnimation2(const QAbstractAnimation2& other);
    ~QAbstractAnimation2();

    //definition
    inline QAnimationGroup2 *group() const {return m_group;}
    void setGroup(QAnimationGroup2* group) {m_group = group;}   //### remove from old group, add to new

    inline QDeclarativeAbstractAnimation *animation() const;
    void setAnimation(QObject *animation);

    inline int loopCount() const {return m_loopCount;}
    void setLoopCount(int loopCount);

    int totalDuration() const;

    virtual int duration() const {return 0;}
    virtual QAbstractAnimation2::AnimationType type() const;

    inline QAbstractAnimation2::Direction direction() const {return m_direction;}
    void setDirection(QAbstractAnimation2::Direction direction);

    //state
    inline int currentTime() const {return m_totalCurrentTime;}
    inline int currentLoopTime() const {return m_currentTime;}
    inline int currentLoop() const {return m_currentLoop;}
    inline QAbstractAnimation2::State state() const {return m_state;}

    void setPaused(bool);
    void setCurrentTime(int msecs);

    void start();
    void pause();
    void resume();
    void stop();

    void registerFinished(QObject* object, const char* method);
    void registerStateChanged(QObject* object, const char* method);
    void registerCurrentLoopChanged(QObject* object, const char* method);
    void registerDirectionChanged(QObject* object, const char* method);
protected:
    virtual void updateCurrentTime(int) {}
    virtual void updateState(QAbstractAnimation2::State newState, QAbstractAnimation2::State oldState);
    virtual void updateDirection(QAbstractAnimation2::Direction direction);
    virtual void topLevelAnimationLoopChanged() {}

    void setState(QAbstractAnimation2::State state);

    void finished();
    void stateChanged(QAbstractAnimation2::State newState, QAbstractAnimation2::State oldState);
    void currentLoopChanged(int currentLoop);
    void directionChanged(QAbstractAnimation2::Direction);

    //definition
    int m_loopCount;
    bool m_isPause;
    bool m_isGroup;
    QAnimationGroup2 *m_group;
    QDeclarativeGuard<QObject> m_animationGuard;
    AnimationType m_type;
    QAbstractAnimation2::Direction m_direction; //???

    //state
    QAbstractAnimation2::State m_state;
    int m_totalCurrentTime;
    int m_currentTime;
    int m_currentLoop;
    bool m_hasRegisteredTimer;

    QList<QPair<QDeclarativeGuard<QObject>,int> > m_finishedSlots;
    QList<QPair<QDeclarativeGuard<QObject>,int> > m_stateChangedSlots;
    QList<QPair<QDeclarativeGuard<QObject>,int> > m_currentLoopChangedSlots;
    QList<QPair<QDeclarativeGuard<QObject>,int> > m_directionChangedSlots;

    friend class QUnifiedTimer2;
    friend class QAnimationGroup2;
};

typedef QDeclarativeRefPointer<QAbstractAnimation2> QAbstractAnimation2Pointer;
uint qHash(const QAbstractAnimation2Pointer& value);

class QAnimationDriver2Private;
class Q_DECLARATIVE_EXPORT QAnimationDriver2 : public QObject
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QAnimationDriver2)

public:
    QAnimationDriver2(QObject *parent = 0);
    ~QAnimationDriver2();

    virtual void advance();

    void install();
    void uninstall();

    bool isRunning() const;

    qint64 elapsed() const;

Q_SIGNALS:
    void started();
    void stopped();

protected:
    void advanceAnimation(qint64 timeStep = -1);
    virtual void start();
    virtual void stop();

    QAnimationDriver2(QAnimationDriver2Private &dd, QObject *parent = 0);
private:
    friend class QUnifiedTimer2;

};

class Q_DECLARATIVE_EXPORT QAnimationDriver2Private : public QObjectPrivate
{
public:
    QAnimationDriver2Private() : running(false) {}
    bool running;
};

class QUnifiedTimer2;
class QDefaultAnimationDriver2 : public QAnimationDriver2
{
    Q_OBJECT
public:
    QDefaultAnimationDriver2(QUnifiedTimer2 *timer);
    void timerEvent(QTimerEvent *e);

private Q_SLOTS:
    void startTimer();
    void stopTimer();

private:
    QBasicTimer m_timer;
    QUnifiedTimer2 *m_unified_timer;
};

typedef QElapsedTimer ElapsedTimer;

class Q_DECLARATIVE_EXPORT QUnifiedTimer2 : public QObject
{
private:
    QUnifiedTimer2();

public:
    //XXX this is needed by dui
    static QUnifiedTimer2 *instance();
    static QUnifiedTimer2 *instance(bool create);

    static void registerAnimation(QAbstractAnimation2* animation, bool isTopLevel);

    //the parameter needs to be "QAbstractAnimation2 *" here, as it will be
    //called in the QAbstractAnimation2's dtor
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
    friend class QDefaultAnimationDriver2;
    friend class QAnimationDriver2;

    QAnimationDriver2 *driver;
    QDefaultAnimationDriver2 defaultDriver;

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

    void registerRunningAnimation(QAbstractAnimation2* animation);
    void unregisterRunningAnimation(QAbstractAnimation2 *animation);

    int closestPauseAnimationTimeToFinish();
};

QT_END_NAMESPACE

QT_END_HEADER

#endif // QABSTRACTANIMATION2_P_H
