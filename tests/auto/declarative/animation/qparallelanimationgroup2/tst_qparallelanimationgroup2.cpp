/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the test suite of the Qt Toolkit.
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

#include <QtTest/QtTest>

#include <QtDeclarative/private/qparallelanimationgroup2_p.h>

Q_DECLARE_METATYPE(QAbstractAnimation2::State)

class tst_QParallelAnimationGroup2 : public QObject
{
    Q_OBJECT
public Q_SLOTS:
    void initTestCase();

private slots:
    void construction();
    void setCurrentTime();
    void stateChanged();
    void clearGroup();
    void propagateGroupUpdateToChildren();
    void updateChildrenWithRunningGroup();
    void deleteChildrenWithRunningGroup();
    void startChildrenWithStoppedGroup();
    void stopGroupWithRunningChild();
    void startGroupWithRunningChild();
    void zeroDurationAnimation();
    void stopUncontrolledAnimations();
    void loopCount_data();
    void loopCount();
    void addAndRemoveDuration();
    void pauseResume();

    void crashWhenRemovingUncontrolledAnimation();
};

void tst_QParallelAnimationGroup2::initTestCase()
{
    qRegisterMetaType<QAbstractAnimation2::State>("QAbstractAnimation2::State");
#if defined(Q_OS_MAC) || defined(Q_OS_WINCE)
    // give the mac/wince app start event queue time to clear
    QTest::qWait(1000);
#endif
}

void tst_QParallelAnimationGroup2::construction()
{
    QParallelAnimationGroup2 animationgroup;
}

class TestAnimation : public QAbstractAnimation2
{
public:
    TestAnimation(int duration = 250) : m_duration(duration) {}
    int duration() const { return m_duration; }

private:
    int m_duration;
};

class UncontrolledAnimation : public QObject, public QAbstractAnimation2
{
    Q_OBJECT
public:
    UncontrolledAnimation()
        : id(0)
    {
    }

    int duration() const { return -1; /* not time driven */ }

protected:
    void timerEvent(QTimerEvent *event)
    {
        if (event->timerId() == id)
            stop();
    }

    void updateRunning(bool running)
    {
        if (running) {
            id = startTimer(500);
        } else {
            killTimer(id);
            id = 0;
        }
    }

private:
    int id;
};

class StateChangeListener: public QAnimation2ChangeListener
{
public:
    virtual void animationStateChanged(QAbstractAnimation2 *, QAbstractAnimation2::State newState, QAbstractAnimation2::State)
    {
        states << newState;
    }

    void clear() { states.clear(); }
    int count() { return states.count(); }

    QList<QAbstractAnimation2::State> states;
};

class FinishedListener: public QAnimation2ChangeListener
{
public:
    FinishedListener() : m_count(0) {}

    virtual void animationFinished(QAbstractAnimation2 *) { ++m_count; }
    void clear() { m_count = 0; }
    int count() { return m_count; }

private:
    int m_count;
};

void tst_QParallelAnimationGroup2::setCurrentTime()
{
    // originally was parallel operating on different object/properties
    QAnimationGroup2 *parallel = new QParallelAnimationGroup2();
    TestAnimation *a1_p_o1 = new TestAnimation;
    TestAnimation *a1_p_o2 = new TestAnimation;
    TestAnimation *a1_p_o3 = new TestAnimation;
    a1_p_o2->setLoopCount(3);
    parallel->appendAnimation(a1_p_o1);
    parallel->appendAnimation(a1_p_o2);
    parallel->appendAnimation(a1_p_o3);

    UncontrolledAnimation *notTimeDriven = new UncontrolledAnimation;
    QCOMPARE(notTimeDriven->totalDuration(), -1);

    TestAnimation *loopsForever = new TestAnimation;
    loopsForever->setLoopCount(-1);
    QCOMPARE(loopsForever->totalDuration(), -1);

    QParallelAnimationGroup2 group;
    group.appendAnimation(parallel);
    group.appendAnimation(notTimeDriven);
    group.appendAnimation(loopsForever);

    // Current time = 1
    group.setCurrentTime(1);
    QCOMPARE(group.state(), QAnimationGroup2::Stopped);
    QCOMPARE(parallel->state(), QAnimationGroup2::Stopped);
    QCOMPARE(a1_p_o1->state(), QAnimationGroup2::Stopped);
    QCOMPARE(a1_p_o2->state(), QAnimationGroup2::Stopped);
    QCOMPARE(a1_p_o3->state(), QAnimationGroup2::Stopped);
    QCOMPARE(notTimeDriven->state(), QAnimationGroup2::Stopped);
    QCOMPARE(loopsForever->state(), QAnimationGroup2::Stopped);

    QCOMPARE(group.currentLoopTime(), 1);
    QCOMPARE(a1_p_o1->currentLoopTime(), 1);
    QCOMPARE(a1_p_o2->currentLoopTime(), 1);
    QCOMPARE(a1_p_o3->currentLoopTime(), 1);
    QCOMPARE(notTimeDriven->currentLoopTime(), 1);
    QCOMPARE(loopsForever->currentLoopTime(), 1);

    // Current time = 250
    group.setCurrentTime(250);
    QCOMPARE(group.currentLoopTime(), 250);
    QCOMPARE(a1_p_o1->currentLoopTime(), 250);
    QCOMPARE(a1_p_o2->currentLoopTime(), 0);
    QCOMPARE(a1_p_o2->currentLoop(), 1);
    QCOMPARE(a1_p_o3->currentLoopTime(), 250);
    QCOMPARE(notTimeDriven->currentLoopTime(), 250);
    QCOMPARE(loopsForever->currentLoopTime(), 0);
    QCOMPARE(loopsForever->currentLoop(), 1);

    // Current time = 251
    group.setCurrentTime(251);
    QCOMPARE(group.currentLoopTime(), 251);
    QCOMPARE(a1_p_o1->currentLoopTime(), 250);
    QCOMPARE(a1_p_o2->currentLoopTime(), 1);
    QCOMPARE(a1_p_o2->currentLoop(), 1);
    QCOMPARE(a1_p_o3->currentLoopTime(), 250);
    QCOMPARE(notTimeDriven->currentLoopTime(), 251);
    QCOMPARE(loopsForever->currentLoopTime(), 1);
}

void tst_QParallelAnimationGroup2::stateChanged()
{
    //this ensures that the correct animations are started when starting the group
    TestAnimation *anim1 = new TestAnimation(1000);
    TestAnimation *anim2 = new TestAnimation(2000);
    TestAnimation *anim3 = new TestAnimation(3000);
    TestAnimation *anim4 = new TestAnimation(3000);

    QParallelAnimationGroup2 group;
    group.appendAnimation(anim1);
    group.appendAnimation(anim2);
    group.appendAnimation(anim3);
    group.appendAnimation(anim4);

    StateChangeListener spy1;
    anim1->addAnimationChangeListener(&spy1, QAbstractAnimation2::StateChange);
    StateChangeListener spy2;
    anim2->addAnimationChangeListener(&spy2, QAbstractAnimation2::StateChange);
    StateChangeListener spy3;
    anim3->addAnimationChangeListener(&spy3, QAbstractAnimation2::StateChange);
    StateChangeListener spy4;
    anim4->addAnimationChangeListener(&spy4, QAbstractAnimation2::StateChange);

    //first; let's start forward
    group.start();
    //all the animations should be started
    QCOMPARE(spy1.count(), 1);
    QCOMPARE(spy1.states.last(), TestAnimation::Running);
    QCOMPARE(spy2.count(), 1);
    QCOMPARE(spy2.states.last(), TestAnimation::Running);
    QCOMPARE(spy3.count(), 1);
    QCOMPARE(spy3.states.last(), TestAnimation::Running);
    QCOMPARE(spy4.count(), 1);
    QCOMPARE(spy4.states.last(), TestAnimation::Running);

    group.setCurrentTime(1500); //anim1 should be finished
    QCOMPARE(group.state(), QAnimationGroup2::Running);
    QCOMPARE(spy1.count(), 2);
    QCOMPARE(spy1.states.last(), TestAnimation::Stopped);
    QCOMPARE(spy2.count(), 1); //no change
    QCOMPARE(spy3.count(), 1); //no change
    QCOMPARE(spy4.count(), 1); //no change

    group.setCurrentTime(2500); //anim2 should be finished
    QCOMPARE(group.state(), QAnimationGroup2::Running);
    QCOMPARE(spy1.count(), 2); //no change
    QCOMPARE(spy2.count(), 2);
    QCOMPARE(spy2.states.last(), TestAnimation::Stopped);
    QCOMPARE(spy3.count(), 1); //no change
    QCOMPARE(spy4.count(), 1); //no change

    group.setCurrentTime(3500); //everything should be finished
    QCOMPARE(group.state(), QAnimationGroup2::Stopped);
    QCOMPARE(spy1.count(), 2); //no change
    QCOMPARE(spy2.count(), 2); //no change
    QCOMPARE(spy3.count(), 2);
    QCOMPARE(spy3.states.last(), TestAnimation::Stopped);
    QCOMPARE(spy4.count(), 2);
    QCOMPARE(spy4.states.last(), TestAnimation::Stopped);

    //cleanup
    spy1.clear();
    spy2.clear();
    spy3.clear();
    spy4.clear();

    //now let's try to reverse that
    group.setDirection(QAbstractAnimation2::Backward);
    group.start();

    //only anim3 and anim4 should be started
    QCOMPARE(group.state(), QAnimationGroup2::Running);
    QCOMPARE(spy1.count(), 0);
    QCOMPARE(spy2.count(), 0);
    QCOMPARE(spy3.count(), 1);
    QCOMPARE(spy3.states.last(), TestAnimation::Running);
    QCOMPARE(spy4.count(), 1);
    QCOMPARE(spy4.states.last(), TestAnimation::Running);

    group.setCurrentTime(1500); //anim2 should be started
    QCOMPARE(group.state(), QAnimationGroup2::Running);
    QCOMPARE(spy1.count(), 0); //no change
    QCOMPARE(spy2.count(), 1);
    QCOMPARE(spy2.states.last(), TestAnimation::Running);
    QCOMPARE(spy3.count(), 1); //no change
    QCOMPARE(spy4.count(), 1); //no change

    group.setCurrentTime(500); //anim1 is finally also started
    QCOMPARE(group.state(), QAnimationGroup2::Running);
    QCOMPARE(spy1.count(), 1);
    QCOMPARE(spy1.states.last(), TestAnimation::Running);
    QCOMPARE(spy2.count(), 1); //no change
    QCOMPARE(spy3.count(), 1); //no change
    QCOMPARE(spy4.count(), 1); //no change

    group.setCurrentTime(0); //everything should be stopped
    QCOMPARE(group.state(), QAnimationGroup2::Stopped);
    QCOMPARE(spy1.count(), 2);
    QCOMPARE(spy1.states.last(), TestAnimation::Stopped);
    QCOMPARE(spy2.count(), 2);
    QCOMPARE(spy2.states.last(), TestAnimation::Stopped);
    QCOMPARE(spy3.count(), 2);
    QCOMPARE(spy3.states.last(), TestAnimation::Stopped);
    QCOMPARE(spy4.count(), 2);
    QCOMPARE(spy4.states.last(), TestAnimation::Stopped);
}

void tst_QParallelAnimationGroup2::clearGroup()
{
    QParallelAnimationGroup2 group;
    static const int animationCount = 10;

    for (int i = 0; i < animationCount; ++i) {
        group.appendAnimation(new QParallelAnimationGroup2);
    }

    int count = 0;
    for (QAbstractAnimation2 *anim = group.firstChild(); anim; anim = anim->nextSibling())
        ++count;
    QCOMPARE(count, animationCount);

    group.clear();

    QVERIFY(!group.firstChild() && !group.lastChild());
    QCOMPARE(group.currentLoopTime(), 0);
}

void tst_QParallelAnimationGroup2::propagateGroupUpdateToChildren()
{
    // this test verifies if group state changes are updating its children correctly
    QParallelAnimationGroup2 group;

    TestAnimation anim1(100);
    TestAnimation anim2(200);

    QCOMPARE(group.state(), QAnimationGroup2::Stopped);
    QCOMPARE(anim1.state(), QAnimationGroup2::Stopped);
    QCOMPARE(anim2.state(), QAnimationGroup2::Stopped);

    group.appendAnimation(&anim1);
    group.appendAnimation(&anim2);

    group.start();

    QCOMPARE(group.state(), QAnimationGroup2::Running);
    QCOMPARE(anim1.state(), QAnimationGroup2::Running);
    QCOMPARE(anim2.state(), QAnimationGroup2::Running);

    group.pause();

    QCOMPARE(group.state(), QAnimationGroup2::Paused);
    QCOMPARE(anim1.state(), QAnimationGroup2::Paused);
    QCOMPARE(anim2.state(), QAnimationGroup2::Paused);

    group.stop();

    QCOMPARE(group.state(), QAnimationGroup2::Stopped);
    QCOMPARE(anim1.state(), QAnimationGroup2::Stopped);
    QCOMPARE(anim2.state(), QAnimationGroup2::Stopped);
}

void tst_QParallelAnimationGroup2::updateChildrenWithRunningGroup()
{
    // assert that its possible to modify a child's state directly while their group is running
    QParallelAnimationGroup2 group;

    TestAnimation anim(200);

    StateChangeListener groupStateChangedSpy;
    group.addAnimationChangeListener(&groupStateChangedSpy, QAbstractAnimation2::StateChange);
    StateChangeListener childStateChangedSpy;
    anim.addAnimationChangeListener(&childStateChangedSpy, QAbstractAnimation2::StateChange);

    QCOMPARE(groupStateChangedSpy.count(), 0);
    QCOMPARE(childStateChangedSpy.count(), 0);
    QCOMPARE(group.state(), QAnimationGroup2::Stopped);
    QCOMPARE(anim.state(), QAnimationGroup2::Stopped);

    group.appendAnimation(&anim);

    group.start();

    QCOMPARE(group.state(), QAnimationGroup2::Running);
    QCOMPARE(anim.state(), QAnimationGroup2::Running);

    QCOMPARE(groupStateChangedSpy.count(), 1);
    QCOMPARE(childStateChangedSpy.count(), 1);

    QCOMPARE(groupStateChangedSpy.states.at(0), QAnimationGroup2::Running);
    QCOMPARE(childStateChangedSpy.states.at(0), QAnimationGroup2::Running);

    // starting directly a running child will not have any effect
    anim.start();

    QCOMPARE(groupStateChangedSpy.count(), 1);
    QCOMPARE(childStateChangedSpy.count(), 1);

    anim.pause();

    QCOMPARE(group.state(), QAnimationGroup2::Running);
    QCOMPARE(anim.state(), QAnimationGroup2::Paused);

    // in the animation stops directly, the group will still be running
    anim.stop();

    QCOMPARE(group.state(), QAnimationGroup2::Running);
    QCOMPARE(anim.state(), QAnimationGroup2::Stopped);

    //cleanup
    group.removeAnimationChangeListener(&groupStateChangedSpy, QAbstractAnimation2::StateChange);
    anim.removeAnimationChangeListener(&childStateChangedSpy, QAbstractAnimation2::StateChange);
}

void tst_QParallelAnimationGroup2::deleteChildrenWithRunningGroup()
{
    // test if children can be activated when their group is stopped
    QParallelAnimationGroup2 group;

    TestAnimation *anim1 = new TestAnimation(200);
    group.appendAnimation(anim1);

    QCOMPARE(group.duration(), anim1->duration());

    group.start();
    QCOMPARE(group.state(), QAnimationGroup2::Running);
    QCOMPARE(anim1->state(), QAnimationGroup2::Running);

    QTest::qWait(80);
    QVERIFY(group.currentLoopTime() > 0);

    delete anim1;
    QVERIFY(!group.firstChild());
    QCOMPARE(group.duration(), 0);
    QCOMPARE(group.state(), QAnimationGroup2::Stopped);
    QCOMPARE(group.currentLoopTime(), 0); //that's the invariant
}

void tst_QParallelAnimationGroup2::startChildrenWithStoppedGroup()
{
    // test if children can be activated when their group is stopped
    QParallelAnimationGroup2 group;

    TestAnimation anim1(200);
    TestAnimation anim2(200);

    QCOMPARE(group.state(), QAnimationGroup2::Stopped);
    QCOMPARE(anim1.state(), QAnimationGroup2::Stopped);
    QCOMPARE(anim2.state(), QAnimationGroup2::Stopped);

    group.appendAnimation(&anim1);
    group.appendAnimation(&anim2);

    group.stop();

    QCOMPARE(group.state(), QAnimationGroup2::Stopped);
    QCOMPARE(anim1.state(), QAnimationGroup2::Stopped);
    QCOMPARE(anim2.state(), QAnimationGroup2::Stopped);

    anim1.start();
    anim2.start();
    anim2.pause();

    QCOMPARE(group.state(), QAnimationGroup2::Stopped);
    QCOMPARE(anim1.state(), QAnimationGroup2::Running);
    QCOMPARE(anim2.state(), QAnimationGroup2::Paused);
}

void tst_QParallelAnimationGroup2::stopGroupWithRunningChild()
{
    // children that started independently will not be affected by a group stop
    QParallelAnimationGroup2 group;

    TestAnimation anim1(200);
    TestAnimation anim2(200);

    QCOMPARE(group.state(), QAnimationGroup2::Stopped);
    QCOMPARE(anim1.state(), QAnimationGroup2::Stopped);
    QCOMPARE(anim2.state(), QAnimationGroup2::Stopped);

    group.appendAnimation(&anim1);
    group.appendAnimation(&anim2);

    anim1.start();
    anim2.start();
    anim2.pause();

    QCOMPARE(group.state(), QAnimationGroup2::Stopped);
    QCOMPARE(anim1.state(), QAnimationGroup2::Running);
    QCOMPARE(anim2.state(), QAnimationGroup2::Paused);

    group.stop();

    QCOMPARE(group.state(), QAnimationGroup2::Stopped);
    QCOMPARE(anim1.state(), QAnimationGroup2::Running);
    QCOMPARE(anim2.state(), QAnimationGroup2::Paused);

    anim1.stop();
    anim2.stop();

    QCOMPARE(group.state(), QAnimationGroup2::Stopped);
    QCOMPARE(anim1.state(), QAnimationGroup2::Stopped);
    QCOMPARE(anim2.state(), QAnimationGroup2::Stopped);
}

void tst_QParallelAnimationGroup2::startGroupWithRunningChild()
{
    // as the group has precedence over its children, starting a group will restart all the children
    QParallelAnimationGroup2 group;

    TestAnimation anim1(200);
    TestAnimation anim2(200);

    StateChangeListener stateChangedSpy1;
    anim1.addAnimationChangeListener(&stateChangedSpy1, QAbstractAnimation2::StateChange);
    StateChangeListener stateChangedSpy2;
    anim2.addAnimationChangeListener(&stateChangedSpy2, QAbstractAnimation2::StateChange);

    QCOMPARE(stateChangedSpy1.count(), 0);
    QCOMPARE(stateChangedSpy2.count(), 0);
    QCOMPARE(group.state(), QAnimationGroup2::Stopped);
    QCOMPARE(anim1.state(), QAnimationGroup2::Stopped);
    QCOMPARE(anim2.state(), QAnimationGroup2::Stopped);

    group.appendAnimation(&anim1);
    group.appendAnimation(&anim2);

    anim1.start();
    anim2.start();
    anim2.pause();

    QCOMPARE(stateChangedSpy1.states.at(0), QAnimationGroup2::Running);
    QCOMPARE(stateChangedSpy2.states.at(0), QAnimationGroup2::Running);
    QCOMPARE(stateChangedSpy2.states.at(1), QAnimationGroup2::Paused);

    QCOMPARE(group.state(), QAnimationGroup2::Stopped);
    QCOMPARE(anim1.state(), QAnimationGroup2::Running);
    QCOMPARE(anim2.state(), QAnimationGroup2::Paused);

    group.start();

    QCOMPARE(stateChangedSpy1.count(), 3);
    QCOMPARE(stateChangedSpy1.states.at(1), QAnimationGroup2::Stopped);
    QCOMPARE(stateChangedSpy1.states.at(2), QAnimationGroup2::Running);

    QCOMPARE(stateChangedSpy2.count(), 4);
    QCOMPARE(stateChangedSpy2.states.at(2), QAnimationGroup2::Stopped);
    QCOMPARE(stateChangedSpy2.states.at(3), QAnimationGroup2::Running);

    QCOMPARE(group.state(), QAnimationGroup2::Running);
    QCOMPARE(anim1.state(), QAnimationGroup2::Running);
    QCOMPARE(anim2.state(), QAnimationGroup2::Running);

    //cleanup
    anim1.removeAnimationChangeListener(&stateChangedSpy1, QAbstractAnimation2::StateChange);
    anim2.removeAnimationChangeListener(&stateChangedSpy2, QAbstractAnimation2::StateChange);
}

void tst_QParallelAnimationGroup2::zeroDurationAnimation()
{
    QParallelAnimationGroup2 group;

    TestAnimation anim1(0);
    TestAnimation anim2(100);
    TestAnimation anim3(10);

    StateChangeListener stateChangedSpy1;
    anim1.addAnimationChangeListener(&stateChangedSpy1, QAbstractAnimation2::StateChange);
    FinishedListener finishedSpy1;
    anim1.addAnimationChangeListener(&finishedSpy1, QAbstractAnimation2::Completion);

    StateChangeListener stateChangedSpy2;
    anim2.addAnimationChangeListener(&stateChangedSpy2, QAbstractAnimation2::StateChange);
    FinishedListener finishedSpy2;
    anim2.addAnimationChangeListener(&finishedSpy2, QAbstractAnimation2::Completion);

    StateChangeListener stateChangedSpy3;
    anim3.addAnimationChangeListener(&stateChangedSpy3, QAbstractAnimation2::StateChange);
    FinishedListener finishedSpy3;
    anim3.addAnimationChangeListener(&finishedSpy3, QAbstractAnimation2::Completion);

    group.appendAnimation(&anim1);
    group.appendAnimation(&anim2);
    group.appendAnimation(&anim3);
    QCOMPARE(stateChangedSpy1.count(), 0);
    group.start();
    QCOMPARE(stateChangedSpy1.count(), 2);
    QCOMPARE(finishedSpy1.count(), 1);
    QCOMPARE(stateChangedSpy1.states.at(0), QAnimationGroup2::Running);
    QCOMPARE(stateChangedSpy1.states.at(1), QAnimationGroup2::Stopped);

    QCOMPARE(stateChangedSpy2.count(), 1);
    QCOMPARE(finishedSpy2.count(), 0);
    QCOMPARE(stateChangedSpy1.states.at(0), QAnimationGroup2::Running);

    QCOMPARE(stateChangedSpy3.count(), 1);
    QCOMPARE(finishedSpy3.count(), 0);
    QCOMPARE(stateChangedSpy3.states.at(0), QAnimationGroup2::Running);

    QCOMPARE(anim1.state(), QAnimationGroup2::Stopped);
    QCOMPARE(anim2.state(), QAnimationGroup2::Running);
    QCOMPARE(anim3.state(), QAnimationGroup2::Running);
    QCOMPARE(group.state(), QAnimationGroup2::Running);

    group.stop();
    group.setLoopCount(4);
    stateChangedSpy1.clear();
    stateChangedSpy2.clear();
    stateChangedSpy3.clear();

    group.start();
    QCOMPARE(stateChangedSpy1.count(), 2);
    QCOMPARE(stateChangedSpy2.count(), 1);
    QCOMPARE(stateChangedSpy3.count(), 1);
    group.setCurrentTime(50);
    QCOMPARE(stateChangedSpy1.count(), 2);
    QCOMPARE(stateChangedSpy2.count(), 1);
    QCOMPARE(stateChangedSpy3.count(), 2);
    group.setCurrentTime(150);
    QCOMPARE(stateChangedSpy1.count(), 4);
    QCOMPARE(stateChangedSpy2.count(), 3);
    QCOMPARE(stateChangedSpy3.count(), 4);
    group.setCurrentTime(50);
    QCOMPARE(stateChangedSpy1.count(), 6);
    QCOMPARE(stateChangedSpy2.count(), 5);
    QCOMPARE(stateChangedSpy3.count(), 6);

    //cleanup
    anim1.removeAnimationChangeListener(&stateChangedSpy1, QAbstractAnimation2::StateChange);
    anim1.removeAnimationChangeListener(&finishedSpy1, QAbstractAnimation2::Completion);
    anim2.removeAnimationChangeListener(&stateChangedSpy2, QAbstractAnimation2::StateChange);
    anim2.removeAnimationChangeListener(&finishedSpy2, QAbstractAnimation2::Completion);
    anim3.removeAnimationChangeListener(&stateChangedSpy3, QAbstractAnimation2::StateChange);
    anim3.removeAnimationChangeListener(&finishedSpy3, QAbstractAnimation2::Completion);
}

void tst_QParallelAnimationGroup2::stopUncontrolledAnimations()
{
    QParallelAnimationGroup2 group;

    TestAnimation anim1(0);

    UncontrolledAnimation notTimeDriven;
    QCOMPARE(notTimeDriven.totalDuration(), -1);

    TestAnimation loopsForever(100);
    loopsForever.setLoopCount(-1);

    StateChangeListener stateChangedSpy;
    anim1.addAnimationChangeListener(&stateChangedSpy, QAbstractAnimation2::StateChange);

    group.appendAnimation(&anim1);
    group.appendAnimation(&notTimeDriven);
    group.appendAnimation(&loopsForever);

    group.start();

    QCOMPARE(stateChangedSpy.count(), 2);
    QCOMPARE(stateChangedSpy.states.at(0), QAnimationGroup2::Running);
    QCOMPARE(stateChangedSpy.states.at(1), QAnimationGroup2::Stopped);

    QCOMPARE(group.state(), QAnimationGroup2::Running);
    QCOMPARE(notTimeDriven.state(), QAnimationGroup2::Running);
    QCOMPARE(loopsForever.state(), QAnimationGroup2::Running);
    QCOMPARE(anim1.state(), QAnimationGroup2::Stopped);

    notTimeDriven.stop();

    QCOMPARE(group.state(), QAnimationGroup2::Running);
    QCOMPARE(notTimeDriven.state(), QAnimationGroup2::Stopped);
    QCOMPARE(loopsForever.state(), QAnimationGroup2::Running);

    loopsForever.stop();

    QCOMPARE(group.state(), QAnimationGroup2::Stopped);
    QCOMPARE(notTimeDriven.state(), QAnimationGroup2::Stopped);
    QCOMPARE(loopsForever.state(), QAnimationGroup2::Stopped);
}

struct AnimState {
    AnimState(int time = -1) : time(time), state(-1) {}
    AnimState(int time, int state) : time(time), state(state) {}
    int time;
    int state;
};

#define Running QAbstractAnimation2::Running
#define Stopped QAbstractAnimation2::Stopped

Q_DECLARE_METATYPE(AnimState)
void tst_QParallelAnimationGroup2::loopCount_data()
{
    QTest::addColumn<bool>("directionBackward");
    QTest::addColumn<int>("setLoopCount");
    QTest::addColumn<int>("initialGroupTime");
    QTest::addColumn<int>("currentGroupTime");
    QTest::addColumn<AnimState>("expected1");
    QTest::addColumn<AnimState>("expected2");
    QTest::addColumn<AnimState>("expected3");

    //                                                                                  D U R A T I O N
    //                                                              100                           60*2                           0
    // direction = Forward
    QTest::newRow("50")  << false << 3 << 0 <<  50 << AnimState( 50, Running) << AnimState( 50, Running) << AnimState(  0, Stopped);
    QTest::newRow("100") << false << 3 << 0 << 100 << AnimState(100         ) << AnimState( 40, Running) << AnimState(  0, Stopped);
    QTest::newRow("110") << false << 3 << 0 << 110 << AnimState(100, Stopped) << AnimState( 50, Running) << AnimState(  0, Stopped);
    QTest::newRow("120") << false << 3 << 0 << 120 << AnimState(  0, Running) << AnimState(  0, Running) << AnimState(  0, Stopped);

    QTest::newRow("170") << false << 3 << 0 << 170 << AnimState( 50, Running) << AnimState( 50, Running) << AnimState(  0, Stopped);
    QTest::newRow("220") << false << 3 << 0 << 220 << AnimState(100         ) << AnimState( 40, Running) << AnimState(  0, Stopped);
    QTest::newRow("230") << false << 3 << 0 << 230 << AnimState(100, Stopped) << AnimState( 50, Running) << AnimState(  0, Stopped);
    QTest::newRow("240") << false << 3 << 0 << 240 << AnimState(  0, Running) << AnimState(  0, Running) << AnimState(  0, Stopped);

    QTest::newRow("290") << false << 3 << 0 << 290 << AnimState( 50, Running) << AnimState( 50, Running) << AnimState(  0, Stopped);
    QTest::newRow("340") << false << 3 << 0 << 340 << AnimState(100         ) << AnimState( 40, Running) << AnimState(  0, Stopped);
    QTest::newRow("350") << false << 3 << 0 << 350 << AnimState(100, Stopped) << AnimState( 50, Running) << AnimState(  0, Stopped);
    QTest::newRow("360") << false << 3 << 0 << 360 << AnimState(100, Stopped) << AnimState( 60         ) << AnimState(  0, Stopped);

    QTest::newRow("410") << false << 3 << 0 << 410 << AnimState(100, Stopped) << AnimState( 60, Stopped) << AnimState(  0, Stopped);
    QTest::newRow("460") << false << 3 << 0 << 460 << AnimState(100, Stopped) << AnimState( 60, Stopped) << AnimState(  0, Stopped);
    QTest::newRow("470") << false << 3 << 0 << 470 << AnimState(100, Stopped) << AnimState( 60, Stopped) << AnimState(  0, Stopped);
    QTest::newRow("480") << false << 3 << 0 << 480 << AnimState(100, Stopped) << AnimState( 60, Stopped) << AnimState(  0, Stopped);

    // direction = Forward, rewind
    QTest::newRow("120-110") << false << 3 << 120 << 110 << AnimState(   0, Stopped) << AnimState( 50, Running) << AnimState(  0, Stopped);
    QTest::newRow("120-50")  << false << 3 << 120 <<  50 << AnimState(  50, Running) << AnimState( 50, Running) << AnimState(  0, Stopped);
    QTest::newRow("120-0")   << false << 3 << 120 <<  0  << AnimState(   0, Running) << AnimState(  0, Running) << AnimState(  0, Stopped);
    QTest::newRow("300-110") << false << 3 << 300 << 110 << AnimState(   0, Stopped) << AnimState( 50, Running) << AnimState(  0, Stopped);
    QTest::newRow("300-50")  << false << 3 << 300 <<  50 << AnimState(  50, Running) << AnimState( 50, Running) << AnimState(  0, Stopped);
    QTest::newRow("300-0")   << false << 3 << 300 <<  0  << AnimState(   0, Running) << AnimState(  0, Running) << AnimState(  0, Stopped);
    QTest::newRow("115-105") << false << 3 << 115 << 105 << AnimState(  42, Stopped) << AnimState( 45, Running) << AnimState(  0, Stopped);

    // direction = Backward
    QTest::newRow("b120-120") << true << 3 << 120 << 120 << AnimState( 42, Stopped) << AnimState( 60, Running) << AnimState(  0, Stopped);
    QTest::newRow("b120-110") << true << 3 << 120 << 110 << AnimState( 42, Stopped) << AnimState( 50, Running) << AnimState(  0, Stopped);
    QTest::newRow("b120-100") << true << 3 << 120 << 100 << AnimState(100, Running) << AnimState( 40, Running) << AnimState(  0, Stopped);
    QTest::newRow("b120-50")  << true << 3 << 120 <<  50 << AnimState( 50, Running) << AnimState( 50, Running) << AnimState(  0, Stopped);
    QTest::newRow("b120-0")   << true << 3 << 120 <<   0 << AnimState(  0, Stopped) << AnimState(  0, Stopped) << AnimState(  0, Stopped);
    QTest::newRow("b360-170") << true << 3 << 360 << 170 << AnimState( 50, Running) << AnimState( 50, Running) << AnimState(  0, Stopped);
    QTest::newRow("b360-220") << true << 3 << 360 << 220 << AnimState(100, Running) << AnimState( 40, Running) << AnimState(  0, Stopped);
    QTest::newRow("b360-210") << true << 3 << 360 << 210 << AnimState( 90, Running) << AnimState( 30, Running) << AnimState(  0, Stopped);
    QTest::newRow("b360-120") << true << 3 << 360 << 120 << AnimState(  0, Stopped) << AnimState( 60, Running) << AnimState(  0, Stopped);

    // rewind, direction = Backward
    QTest::newRow("b50-110")  << true << 3 <<  50 << 110 << AnimState(100, Stopped) << AnimState( 50, Running) << AnimState(  0, Stopped);
    QTest::newRow("b50-120")  << true << 3 <<  50 << 120 << AnimState(100, Stopped) << AnimState( 60, Running) << AnimState(  0, Stopped);
    QTest::newRow("b50-140")  << true << 3 <<  50 << 140 << AnimState( 20, Running) << AnimState( 20, Running) << AnimState(  0, Stopped);
    QTest::newRow("b50-240")  << true << 3 <<  50 << 240 << AnimState(100, Stopped) << AnimState( 60, Running) << AnimState(  0, Stopped);
    QTest::newRow("b50-260")  << true << 3 <<  50 << 260 << AnimState( 20, Running) << AnimState( 20, Running) << AnimState(  0, Stopped);
    QTest::newRow("b50-350")  << true << 3 <<  50 << 350 << AnimState(100, Stopped) << AnimState( 50, Running) << AnimState(  0, Stopped);

    // infinite looping
    QTest::newRow("inf1220")  << false << -1 <<  0 << 1220 << AnimState( 20, Running) << AnimState( 20, Running) << AnimState(  0, Stopped);
    QTest::newRow("inf1310")  << false << -1 <<  0 << 1310 << AnimState( 100, Stopped) << AnimState( 50, Running) << AnimState(  0, Stopped);
    // infinite looping, direction = Backward (will only loop once)
    QTest::newRow("b.inf120-120") << true  << -1 << 120 << 120 << AnimState( 42, Stopped) << AnimState( 60, Running) << AnimState(  0, Stopped);
    QTest::newRow("b.inf120-20")  << true  << -1 << 120 <<  20 << AnimState( 20, Running) << AnimState( 20, Running) << AnimState(  0, Stopped);
    QTest::newRow("b.inf120-110") << true  << -1 << 120 << 110 << AnimState( 42, Stopped) << AnimState( 50, Running) << AnimState(  0, Stopped);


}

void tst_QParallelAnimationGroup2::loopCount()
{
    QFETCH(bool, directionBackward);
    QFETCH(int, setLoopCount);
    QFETCH(int, initialGroupTime);
    QFETCH(int, currentGroupTime);
    QFETCH(AnimState, expected1);
    QFETCH(AnimState, expected2);
    QFETCH(AnimState, expected3);

    QParallelAnimationGroup2 group;

    TestAnimation anim1(100);
    TestAnimation anim2(60);  //total 120
    anim2.setLoopCount(2);
    TestAnimation anim3(0);

    group.appendAnimation(&anim1);
    group.appendAnimation(&anim2);
    group.appendAnimation(&anim3);

    group.setLoopCount(setLoopCount);
    if (initialGroupTime >= 0)
        group.setCurrentTime(initialGroupTime);
    if (directionBackward)
        group.setDirection(QAbstractAnimation2::Backward);

    group.start();
    if (initialGroupTime >= 0)
        group.setCurrentTime(initialGroupTime);

    anim1.setCurrentTime(42);   // 42 is "untouched"
    anim2.setCurrentTime(42);

    group.setCurrentTime(currentGroupTime);

    QCOMPARE(anim1.currentLoopTime(), expected1.time);
    QCOMPARE(anim2.currentLoopTime(), expected2.time);
    QCOMPARE(anim3.currentLoopTime(), expected3.time);

    if (expected1.state >=0)
        QCOMPARE(int(anim1.state()), expected1.state);
    if (expected2.state >=0)
        QCOMPARE(int(anim2.state()), expected2.state);
    if (expected3.state >=0)
        QCOMPARE(int(anim3.state()), expected3.state);

}

void tst_QParallelAnimationGroup2::addAndRemoveDuration()
{
    QParallelAnimationGroup2 group;
    QCOMPARE(group.duration(), 0);
    TestAnimation *test = new TestAnimation(250);      // 0, duration = 250;
    group.appendAnimation(test);
    QCOMPARE(test->group(), static_cast<QAnimationGroup2*>(&group));
    QCOMPARE(test->duration(), 250);
    QCOMPARE(group.duration(), 250);

    TestAnimation *test2 = new TestAnimation(750);     // 1
    group.appendAnimation(test2);
    QCOMPARE(test2->group(), static_cast<QAnimationGroup2*>(&group));
    QCOMPARE(group.duration(), 750);

    TestAnimation *test3 = new TestAnimation(500);     // 2
    group.appendAnimation(test3);
    QCOMPARE(test3->group(), static_cast<QAnimationGroup2*>(&group));
    QCOMPARE(group.duration(), 750);

    group.removeAnimation(test2);    // remove the one with duration = 750
    delete test2;
    QCOMPARE(group.duration(), 500);

    group.removeAnimation(test3);    // remove the one with duration = 500
    delete test3;
    QCOMPARE(group.duration(), 250);

    group.removeAnimation(test);    // remove the last one (with duration = 250)
    QCOMPARE(test->group(), static_cast<QAnimationGroup2*>(0));
    QCOMPARE(group.duration(), 0);
    delete test;
}

void tst_QParallelAnimationGroup2::pauseResume()
{
    QParallelAnimationGroup2 group;
    TestAnimation *anim = new TestAnimation(250);      // 0, duration = 250;
    group.appendAnimation(anim);
    StateChangeListener spy;
    anim->addAnimationChangeListener(&spy, QAbstractAnimation2::StateChange);
    QCOMPARE(group.duration(), 250);
    group.start();
    QTest::qWait(100);
    QCOMPARE(group.state(), QAnimationGroup2::Running);
    QCOMPARE(anim->state(), QAnimationGroup2::Running);
    QCOMPARE(spy.count(), 1);
    spy.clear();
    const int currentTime = group.currentLoopTime();
    QCOMPARE(anim->currentLoopTime(), currentTime);

    group.pause();
    QCOMPARE(group.state(), QAnimationGroup2::Paused);
    QCOMPARE(group.currentLoopTime(), currentTime);
    QCOMPARE(anim->state(), QAnimationGroup2::Paused);
    QCOMPARE(anim->currentLoopTime(), currentTime);
    QCOMPARE(spy.count(), 1);
    spy.clear();

    group.resume();
    QCOMPARE(group.state(), QAnimationGroup2::Running);
    QCOMPARE(group.currentLoopTime(), currentTime);
    QCOMPARE(anim->state(), QAnimationGroup2::Running);
    QCOMPARE(anim->currentLoopTime(), currentTime);
    QCOMPARE(spy.count(), 1);

    group.stop();
    spy.clear();
    group.appendAnimation(new TestAnimation(500));
    group.start();
    QCOMPARE(spy.count(), 1); //the animation should have been started
    QCOMPARE(spy.states.at(0), TestAnimation::Running);
    group.setCurrentTime(250); //end of first animation
    QCOMPARE(spy.count(), 2); //the animation should have been stopped
    QCOMPARE(spy.states.at(1), TestAnimation::Stopped);
    group.pause();
    QCOMPARE(spy.count(), 2); //this shouldn't have changed
    group.resume();
    QCOMPARE(spy.count(), 2); //this shouldn't have changed
}

// This is a regression test for QTBUG-8910, where a crash occurred when the
// last animation was removed from a group.
void tst_QParallelAnimationGroup2::crashWhenRemovingUncontrolledAnimation()
{
    QParallelAnimationGroup2 group;
    TestAnimation *anim = new TestAnimation;
    anim->setLoopCount(-1);
    TestAnimation *anim2 = new TestAnimation;
    anim2->setLoopCount(-1);
    group.appendAnimation(anim);
    group.appendAnimation(anim2);
    group.start();
    delete anim;
    // it would crash here because the internals of the group would still have a reference to anim
    delete anim2;
}


QTEST_MAIN(tst_QParallelAnimationGroup2)
#include "tst_qparallelanimationgroup2.moc"
