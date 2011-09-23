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

#include "private/qdeclarativeglobal_p.h"

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Declarative)



class QAnimationGroup2;
class QSequentialAnimationGroup2;
class QAnimationDriver2;
class QDeclarativeAbstractAnimation;

class QAbstractAnimation2Private;
class Q_DECLARATIVE_EXPORT QAbstractAnimation2
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

    enum DeletionPolicy {
        KeepWhenStopped = 0,
        DeleteWhenStopped
    };

    explicit QAbstractAnimation2(QDeclarativeAbstractAnimation *animation=0);
    explicit QAbstractAnimation2(QAbstractAnimation2Private *dd, QDeclarativeAbstractAnimation *animation=0);

    virtual ~QAbstractAnimation2();

    State state() const;

    QAnimationGroup2 *group() const;
    QDeclarativeAbstractAnimation *animation() const;

    Direction direction() const;
    void setDirection(Direction direction);

    int currentTime() const;
    int currentLoopTime() const;

    int loopCount() const;
    void setLoopCount(int loopCount);
    int currentLoop() const;

    virtual int duration() const = 0;
    int totalDuration() const;

    void start(QAbstractAnimation2::DeletionPolicy policy = KeepWhenStopped);
    void pause();
    void resume();
    void setPaused(bool);
    void stop();
    void setCurrentTime(int msecs);

    void registerFinished(QObject* object, const char* method);
    void registerStateChanged(QObject* object, const char* method);
    void registerCurrentLoopChanged(QObject* object, const char* method);
    void registerDirectionChanged(QObject* object, const char* method);

protected:
    virtual void updateCurrentTime(int currentTime) = 0;
    virtual void updateState(QAbstractAnimation2::State newState, QAbstractAnimation2::State oldState);
    virtual void updateDirection(QAbstractAnimation2::Direction direction);
    void finished();
    void stateChanged(QAbstractAnimation2::State newState, QAbstractAnimation2::State oldState);
    void currentLoopChanged(int currentLoop);
    void directionChanged(QAbstractAnimation2::Direction);

    Q_DISABLE_COPY(QAbstractAnimation2)
    friend class QAbstractAnimation2Private;
    QAbstractAnimation2Private* d;
};

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

QT_END_NAMESPACE

QT_END_HEADER

#endif // QABSTRACTANIMATION2_P_H
