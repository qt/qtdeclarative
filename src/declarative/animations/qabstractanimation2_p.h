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
#include <QtCore/private/qabstractanimation_p.h>
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

class Q_DECLARATIVE_EXPORT QAbstractAnimation2 : public QDeclarativeRefCount
{
public:
    enum Direction {
        Forward,
        Backward
    };

    enum State {
        Stopped,
        Paused,
        Running
    };

    QAbstractAnimation2();
    QAbstractAnimation2(const QAbstractAnimation2& other);
    ~QAbstractAnimation2();

    //definition
    inline QAnimationGroup2 *group() const {return m_group;}
    void setGroup(QAnimationGroup2* group) {m_group = group;}   //### remove from old group, add to new

    inline int loopCount() const {return m_loopCount;}
    void setLoopCount(int loopCount);

    int totalDuration() const;

    virtual int duration() const {return 0;}

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
    QAbstractAnimation2::Direction m_direction;

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

    friend class QDeclarativeAnimationTimer;
    friend class QAnimationGroup2;
};

typedef QDeclarativeRefPointer<QAbstractAnimation2> QAbstractAnimation2Pointer;
uint Q_DECLARATIVE_EXPORT qHash(const QAbstractAnimation2Pointer& value);

class Q_DECLARATIVE_EXPORT QDeclarativeAnimationTimer : public QAbstractAnimationTimer
{
    Q_OBJECT
private:
    QDeclarativeAnimationTimer();

public:
    static QDeclarativeAnimationTimer *instance();
    static QDeclarativeAnimationTimer *instance(bool create);

    static void registerAnimation(QAbstractAnimation2 *animation, bool isTopLevel);
    static void unregisterAnimation(QAbstractAnimation2 *animation);

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

    void restartAnimationTimer();
    void updateAnimationsTime(qint64 timeStep);

    int currentDelta() { return lastDelta; }

    //useful for profiling/debugging
    int runningAnimationCount() { return animations.count(); }

private Q_SLOTS:
    void startAnimations();
    void stopTimer();

private:
    qint64 lastTick;
    int lastDelta;
    int currentAnimationIdx;
    bool insideTick;
    bool startAnimationPending;
    bool stopTimerPending;

    QList<QAbstractAnimation2*> animations, animationsToStart;

    // this is the count of running animations that are not a group neither a pause animation
    int runningLeafAnimations;
    QList<QAbstractAnimation2*> runningPauseAnimations;

    void registerRunningAnimation(QAbstractAnimation2 *animation);
    void unregisterRunningAnimation(QAbstractAnimation2 *animation);

    int closestPauseAnimationTimeToFinish();
};

QT_END_NAMESPACE

QT_END_HEADER

#endif // QABSTRACTANIMATION2_P_H
