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
#include <QtDeclarative/private/qsequentialanimationgroup2_p.h>
#include <QtDeclarative/private/qparallelanimationgroup2_p.h>
#include <QtDeclarative/private/qpauseanimation2_p.h>

Q_DECLARE_METATYPE(QAbstractAnimation2::State)
Q_DECLARE_METATYPE(QAbstractAnimation2*)

class tst_QSequentialAnimationGroup2 : public QObject
{
    Q_OBJECT
public Q_SLOTS:
    void initTestCase();

private slots:
    void construction();
    void setCurrentTime();
    void setCurrentTimeWithUncontrolledAnimation();
    void seekingForwards();
    void seekingBackwards();
    void pauseAndResume();
    void restart();
    void looping();
    void startDelay();
    void clearGroup();
    void groupWithZeroDurationAnimations();
    void propagateGroupUpdateToChildren();
    void updateChildrenWithRunningGroup();
    void deleteChildrenWithRunningGroup();
    void startChildrenWithStoppedGroup();
    void stopGroupWithRunningChild();
    void startGroupWithRunningChild();
    void zeroDurationAnimation();
    void stopUncontrolledAnimations();
    void finishWithUncontrolledAnimation();
    void addRemoveAnimation();
    void currentAnimation();
    void currentAnimationWithZeroDuration();
    void insertAnimation();
    void clear();
    void pauseResume();
};

void tst_QSequentialAnimationGroup2::initTestCase()
{
    qRegisterMetaType<QAbstractAnimation2::State>("QAbstractAnimation2::State");
    qRegisterMetaType<QAbstractAnimation2*>("QAbstractAnimation2*");
}

void tst_QSequentialAnimationGroup2::construction()
{
    QSequentialAnimationGroup2 animationgroup;
}

class TestAnimation : public QAbstractAnimation2
{
public:
    TestAnimation(int duration = 250) : m_duration(duration) {}
    int duration() const { return m_duration; }

private:
    int m_duration;
};

class TestValueAnimation : public TestAnimation
{
public:
    TestValueAnimation(int duration = 250)
        : TestAnimation(duration), start(0), end(0), value(0) {}

    void updateCurrentTime(int msecs)
    {
        if (msecs >= duration())
            value = end;
        else
            value = start + (end - start) * (qreal(msecs) / duration());
    }

    qreal start, end;
    qreal value;
};

class UncontrolledAnimation : public QObject, public QAbstractAnimation2
{
    Q_OBJECT
public:
    int duration() const { return -1; /* not time driven */ }

protected:
    void updateCurrentTime(int currentTime)
    {
        if (currentTime >= 250)
            stop();
    }
};

class StateChangeListener: public QAnimation2ChangeListener
{
public:
    virtual void animationStateChanged(QAbstractAnimation2 *, QAbstractAnimation2::State newState, QAbstractAnimation2::State)
    {
        states << newState;
    }

    void clear() { states.clear(); }
    int count() const { return states.count(); }

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

void tst_QSequentialAnimationGroup2::setCurrentTime()
{
    // sequence operating on same object/property
    QAnimationGroup2 *sequence = new QSequentialAnimationGroup2();
    TestAnimation *a1_s_o1 = new TestAnimation;
    TestAnimation *a2_s_o1 = new TestAnimation;
    TestAnimation *a3_s_o1 = new TestAnimation;
    a2_s_o1->setLoopCount(3);
    sequence->appendAnimation(a1_s_o1);
    sequence->appendAnimation(a2_s_o1);
    sequence->appendAnimation(a3_s_o1);

    // sequence operating on different object/properties
    QAnimationGroup2 *sequence2 = new QSequentialAnimationGroup2();
    TestAnimation *a1_s_o2 = new TestAnimation;
    TestAnimation *a1_s_o3 = new TestAnimation;
    sequence2->appendAnimation(a1_s_o2);
    sequence2->appendAnimation(a1_s_o3);

    QSequentialAnimationGroup2 group;
    group.appendAnimation(sequence);
    group.appendAnimation(sequence2);

    // Current time = 1
    group.setCurrentTime(1);
    QCOMPARE(group.state(), QAnimationGroup2::Stopped);
    QCOMPARE(sequence->state(), QAnimationGroup2::Stopped);
    QCOMPARE(a1_s_o1->state(), QAnimationGroup2::Stopped);
    QCOMPARE(sequence2->state(), QAnimationGroup2::Stopped);
    QCOMPARE(a1_s_o2->state(), QAnimationGroup2::Stopped);

    QCOMPARE(group.currentLoopTime(), 1);
    QCOMPARE(sequence->currentLoopTime(), 1);
    QCOMPARE(a1_s_o1->currentLoopTime(), 1);
    QCOMPARE(a2_s_o1->currentLoopTime(), 0);
    QCOMPARE(a3_s_o1->currentLoopTime(), 0);
    QCOMPARE(a1_s_o2->currentLoopTime(), 0);
    QCOMPARE(a1_s_o3->currentLoopTime(), 0);

    // Current time = 250
    group.setCurrentTime(250);
    QCOMPARE(group.currentLoopTime(), 250);
    QCOMPARE(sequence->currentLoopTime(), 250);
    QCOMPARE(a1_s_o1->currentLoopTime(), 250);
    QCOMPARE(a2_s_o1->currentLoopTime(), 0);
    QCOMPARE(a3_s_o1->currentLoopTime(), 0);
    QCOMPARE(a1_s_o2->currentLoopTime(), 0);
    QCOMPARE(a1_s_o3->currentLoopTime(), 0);

    // Current time = 251
    group.setCurrentTime(251);
    QCOMPARE(group.currentLoopTime(), 251);
    QCOMPARE(sequence->currentLoopTime(), 251);
    QCOMPARE(a1_s_o1->currentLoopTime(), 250);
    QCOMPARE(a2_s_o1->currentLoopTime(), 1);
    QCOMPARE(a2_s_o1->currentLoop(), 0);
    QCOMPARE(a3_s_o1->currentLoopTime(), 0);
    QCOMPARE(sequence2->currentLoopTime(), 0);
    QCOMPARE(a1_s_o2->currentLoopTime(), 0);
    QCOMPARE(a1_s_o3->currentLoopTime(), 0);

    // Current time = 750
    group.setCurrentTime(750);
    QCOMPARE(group.currentLoopTime(), 750);
    QCOMPARE(sequence->currentLoopTime(), 750);
    QCOMPARE(a1_s_o1->currentLoopTime(), 250);
    QCOMPARE(a2_s_o1->currentLoopTime(), 0);
    QCOMPARE(a2_s_o1->currentLoop(), 2);
    QCOMPARE(a3_s_o1->currentLoopTime(), 0);
    QCOMPARE(sequence2->currentLoopTime(), 0);
    QCOMPARE(a1_s_o2->currentLoopTime(), 0);
    QCOMPARE(a1_s_o3->currentLoopTime(), 0);

    // Current time = 1000
    group.setCurrentTime(1000);
    QCOMPARE(group.currentLoopTime(), 1000);
    QCOMPARE(sequence->currentLoopTime(), 1000);
    QCOMPARE(a1_s_o1->currentLoopTime(), 250);
    QCOMPARE(a2_s_o1->currentLoopTime(), 250);
    QCOMPARE(a2_s_o1->currentLoop(), 2);
    QCOMPARE(a3_s_o1->currentLoopTime(), 0);
    QCOMPARE(sequence2->currentLoopTime(), 0);
    QCOMPARE(a1_s_o2->currentLoopTime(), 0);
    QCOMPARE(a1_s_o3->currentLoopTime(), 0);

    // Current time = 1010
    group.setCurrentTime(1010);
    QCOMPARE(group.currentLoopTime(), 1010);
    QCOMPARE(sequence->currentLoopTime(), 1010);
    QCOMPARE(a1_s_o1->currentLoopTime(), 250);
    QCOMPARE(a2_s_o1->currentLoopTime(), 250);
    QCOMPARE(a2_s_o1->currentLoop(), 2);
    QCOMPARE(a3_s_o1->currentLoopTime(), 10);
    QCOMPARE(sequence2->currentLoopTime(), 0);
    QCOMPARE(a1_s_o2->currentLoopTime(), 0);
    QCOMPARE(a1_s_o3->currentLoopTime(), 0);

    // Current time = 1250
    group.setCurrentTime(1250);
    QCOMPARE(group.currentLoopTime(), 1250);
    QCOMPARE(sequence->currentLoopTime(), 1250);
    QCOMPARE(a1_s_o1->currentLoopTime(), 250);
    QCOMPARE(a2_s_o1->currentLoopTime(), 250);
    QCOMPARE(a2_s_o1->currentLoop(), 2);
    QCOMPARE(a3_s_o1->currentLoopTime(), 250);
    QCOMPARE(sequence2->currentLoopTime(), 0);
    QCOMPARE(a1_s_o2->currentLoopTime(), 0);
    QCOMPARE(a1_s_o3->currentLoopTime(), 0);

    // Current time = 1500
    group.setCurrentTime(1500);
    QCOMPARE(group.currentLoopTime(), 1500);
    QCOMPARE(sequence->currentLoopTime(), 1250);
    QCOMPARE(a1_s_o1->currentLoopTime(), 250);
    QCOMPARE(a2_s_o1->currentLoopTime(), 250);
    QCOMPARE(a2_s_o1->currentLoop(), 2);
    QCOMPARE(a3_s_o1->currentLoopTime(), 250);
    QCOMPARE(sequence2->currentLoopTime(), 250);
    QCOMPARE(a1_s_o2->currentLoopTime(), 250);
    QCOMPARE(a1_s_o3->currentLoopTime(), 0);

    // Current time = 1750
    group.setCurrentTime(1750);
    QCOMPARE(group.currentLoopTime(), 1750);
    QCOMPARE(sequence->currentLoopTime(), 1250);
    QCOMPARE(a1_s_o1->currentLoopTime(), 250);
    QCOMPARE(a2_s_o1->currentLoopTime(), 250);
    QCOMPARE(a2_s_o1->currentLoop(), 2);
    QCOMPARE(a3_s_o1->currentLoopTime(), 250);
    QCOMPARE(sequence2->currentLoopTime(), 500);
    QCOMPARE(a1_s_o2->currentLoopTime(), 250);
    QCOMPARE(a1_s_o3->currentLoopTime(), 250);

    // Current time = 2000
    group.setCurrentTime(2000);
    QCOMPARE(group.currentLoopTime(), 1750);
    QCOMPARE(sequence->currentLoopTime(), 1250);
    QCOMPARE(a1_s_o1->currentLoopTime(), 250);
    QCOMPARE(a2_s_o1->currentLoopTime(), 250);
    QCOMPARE(a2_s_o1->currentLoop(), 2);
    QCOMPARE(a3_s_o1->currentLoopTime(), 250);
    QCOMPARE(sequence2->currentLoopTime(), 500);
    QCOMPARE(a1_s_o2->currentLoopTime(), 250);
    QCOMPARE(a1_s_o3->currentLoopTime(), 250);
}

void tst_QSequentialAnimationGroup2::setCurrentTimeWithUncontrolledAnimation()
{
    // sequence operating on different object/properties
    QAnimationGroup2 *sequence = new QSequentialAnimationGroup2();
    TestAnimation *a1_s_o1 = new TestAnimation;
    TestAnimation *a1_s_o2 = new TestAnimation;
    sequence->appendAnimation(a1_s_o1);
    sequence->appendAnimation(a1_s_o2);

    UncontrolledAnimation *notTimeDriven = new UncontrolledAnimation;
    QCOMPARE(notTimeDriven->totalDuration(), -1);

    TestAnimation *loopsForever = new TestAnimation;
    loopsForever->setLoopCount(-1);
    QCOMPARE(loopsForever->totalDuration(), -1);

    QSequentialAnimationGroup2 group;
    group.appendAnimation(sequence);
    group.appendAnimation(notTimeDriven);
    group.appendAnimation(loopsForever);
    group.start();
    group.pause(); // this allows the group to listen for the finish signal of its children

    // Current time = 1
    group.setCurrentTime(1);
    QCOMPARE(group.state(), QAnimationGroup2::Paused);
    QCOMPARE(sequence->state(), QAnimationGroup2::Paused);
    QCOMPARE(a1_s_o1->state(), QAnimationGroup2::Paused);
    QCOMPARE(a1_s_o2->state(), QAnimationGroup2::Stopped);
    QCOMPARE(notTimeDriven->state(), QAnimationGroup2::Stopped);
    QCOMPARE(loopsForever->state(), QAnimationGroup2::Stopped);

    QCOMPARE(group.currentLoopTime(), 1);
    QCOMPARE(sequence->currentLoopTime(), 1);
    QCOMPARE(a1_s_o1->currentLoopTime(), 1);
    QCOMPARE(a1_s_o2->currentLoopTime(), 0);
    QCOMPARE(notTimeDriven->currentLoopTime(), 0);
    QCOMPARE(loopsForever->currentLoopTime(), 0);

    // Current time = 250
    group.setCurrentTime(250);
    QCOMPARE(group.currentLoopTime(), 250);
    QCOMPARE(sequence->currentLoopTime(), 250);
    QCOMPARE(a1_s_o1->currentLoopTime(), 250);
    QCOMPARE(a1_s_o2->currentLoopTime(), 0);
    QCOMPARE(notTimeDriven->currentLoopTime(), 0);
    QCOMPARE(loopsForever->currentLoopTime(), 0);

    // Current time = 500
    group.setCurrentTime(500);
    QCOMPARE(group.currentLoopTime(), 500);
    QCOMPARE(sequence->currentLoopTime(), 500);
    QCOMPARE(a1_s_o1->currentLoopTime(), 250);
    QCOMPARE(a1_s_o2->currentLoopTime(), 250);
    QCOMPARE(notTimeDriven->currentLoopTime(), 0);
    QCOMPARE(loopsForever->currentLoopTime(), 0);
    QCOMPARE(group.currentAnimation(), static_cast<QAbstractAnimation2 *>(notTimeDriven));

    // Current time = 505
    group.setCurrentTime(505);
    QCOMPARE(group.currentLoopTime(), 505);
    QCOMPARE(sequence->currentLoopTime(), 500);
    QCOMPARE(a1_s_o1->currentLoopTime(), 250);
    QCOMPARE(a1_s_o2->currentLoopTime(), 250);
    QCOMPARE(notTimeDriven->currentLoopTime(), 5);
    QCOMPARE(loopsForever->currentLoopTime(), 0);
    QCOMPARE(group.currentAnimation(), static_cast<QAbstractAnimation2 *>(notTimeDriven));
    QCOMPARE(sequence->state(), QAnimationGroup2::Stopped);
    QCOMPARE(a1_s_o1->state(), QAnimationGroup2::Stopped);
    QCOMPARE(a1_s_o2->state(), QAnimationGroup2::Stopped);
    QCOMPARE(notTimeDriven->state(), QAnimationGroup2::Paused);
    QCOMPARE(loopsForever->state(), QAnimationGroup2::Stopped);

    // Current time = 750 (end of notTimeDriven animation)
    group.setCurrentTime(750);
    QCOMPARE(group.currentLoopTime(), 750);
    QCOMPARE(sequence->currentLoopTime(), 500);
    QCOMPARE(a1_s_o1->currentLoopTime(), 250);
    QCOMPARE(a1_s_o2->currentLoopTime(), 250);
    QCOMPARE(notTimeDriven->currentLoopTime(), 250);
    QCOMPARE(loopsForever->currentLoopTime(), 0);
    QCOMPARE(group.currentAnimation(), loopsForever);
    QCOMPARE(sequence->state(), QAnimationGroup2::Stopped);
    QCOMPARE(a1_s_o1->state(), QAnimationGroup2::Stopped);
    QCOMPARE(a1_s_o2->state(), QAnimationGroup2::Stopped);
    QCOMPARE(notTimeDriven->state(), QAnimationGroup2::Stopped);
    QCOMPARE(loopsForever->state(), QAnimationGroup2::Paused);

    // Current time = 800 (as notTimeDriven was finished at 750, loopsforever should still run)
    group.setCurrentTime(800);
    QCOMPARE(group.currentLoopTime(), 800);
    QCOMPARE(group.currentAnimation(), loopsForever);
    QCOMPARE(sequence->currentLoopTime(), 500);
    QCOMPARE(a1_s_o1->currentLoopTime(), 250);
    QCOMPARE(a1_s_o2->currentLoopTime(), 250);
    QCOMPARE(notTimeDriven->currentLoopTime(), 250);
    QCOMPARE(loopsForever->currentLoopTime(), 50);

    loopsForever->stop(); // this should stop the group

    QCOMPARE(group.state(), QAnimationGroup2::Stopped);
    QCOMPARE(sequence->state(), QAnimationGroup2::Stopped);
    QCOMPARE(a1_s_o1->state(), QAnimationGroup2::Stopped);
    QCOMPARE(a1_s_o2->state(), QAnimationGroup2::Stopped);
    QCOMPARE(notTimeDriven->state(), QAnimationGroup2::Stopped);
    QCOMPARE(loopsForever->state(), QAnimationGroup2::Stopped);
}

void tst_QSequentialAnimationGroup2::seekingForwards()
{

    // sequence operating on same object/property
    QAnimationGroup2 *sequence = new QSequentialAnimationGroup2;
    TestAnimation *a1_s_o1 = new TestAnimation;
    TestAnimation *a2_s_o1 = new TestAnimation;
    TestAnimation *a3_s_o1 = new TestAnimation;
    a2_s_o1->setLoopCount(3);
    sequence->appendAnimation(a1_s_o1);
    sequence->appendAnimation(a2_s_o1);
    sequence->appendAnimation(a3_s_o1);

    // sequence operating on different object/properties
    QAnimationGroup2 *sequence2 = new QSequentialAnimationGroup2;
    TestAnimation *a1_s_o2 = new TestAnimation;
    TestAnimation *a1_s_o3 = new TestAnimation;
    sequence2->appendAnimation(a1_s_o2);
    sequence2->appendAnimation(a1_s_o3);

    QSequentialAnimationGroup2 group;
    group.appendAnimation(sequence);
    group.appendAnimation(sequence2);

    // Current time = 1
    group.setCurrentTime(1);
    QCOMPARE(group.state(), QAnimationGroup2::Stopped);
    QCOMPARE(sequence->state(), QAnimationGroup2::Stopped);
    QCOMPARE(a1_s_o1->state(), QAnimationGroup2::Stopped);
    QCOMPARE(sequence2->state(), QAnimationGroup2::Stopped);
    QCOMPARE(a1_s_o2->state(), QAnimationGroup2::Stopped);
    QCOMPARE(a1_s_o3->state(), QAnimationGroup2::Stopped);

    QCOMPARE(group.currentLoopTime(), 1);
    QCOMPARE(sequence->currentLoopTime(), 1);
    QCOMPARE(a1_s_o1->currentLoopTime(), 1);
    QCOMPARE(a2_s_o1->currentLoopTime(), 0);
    QCOMPARE(a3_s_o1->currentLoopTime(), 0);
    QCOMPARE(sequence2->currentLoopTime(), 0);
    QCOMPARE(a1_s_o2->currentLoopTime(), 0);
    QCOMPARE(a1_s_o3->currentLoopTime(), 0);

    // Current time = 1500
    group.setCurrentTime(1500);
    QCOMPARE(group.currentLoopTime(), 1500);
    QCOMPARE(sequence->currentLoopTime(), 1250);
    QCOMPARE(a1_s_o1->currentLoopTime(), 250);
    QCOMPARE(a2_s_o1->currentLoopTime(), 250);
    QCOMPARE(a2_s_o1->currentLoop(), 2);
    QCOMPARE(a3_s_o1->currentLoopTime(), 250);
    QCOMPARE(sequence2->currentLoopTime(), 250);
    QCOMPARE(a1_s_o2->currentLoopTime(), 250);
    QCOMPARE(a1_s_o3->currentLoopTime(), 0);

    // this will restart the group
    group.start();
    group.pause();
    QCOMPARE(group.state(), QAnimationGroup2::Paused);
    QCOMPARE(sequence->state(), QAnimationGroup2::Paused);
    QCOMPARE(a1_s_o1->state(), QAnimationGroup2::Paused);
    QCOMPARE(sequence2->state(), QAnimationGroup2::Stopped);
    QCOMPARE(a1_s_o2->state(), QAnimationGroup2::Stopped);
    QCOMPARE(a1_s_o3->state(), QAnimationGroup2::Stopped);

    // Current time = 1750
    group.setCurrentTime(1750);
    QCOMPARE(group.currentLoopTime(), 1750);
    QCOMPARE(sequence->currentLoopTime(), 1250);
    QCOMPARE(a1_s_o1->currentLoopTime(), 250);
    QCOMPARE(a2_s_o1->currentLoopTime(), 250);
    QCOMPARE(a2_s_o1->currentLoop(), 2);
    QCOMPARE(a3_s_o1->currentLoopTime(), 250);
    QCOMPARE(sequence2->currentLoopTime(), 500);
    QCOMPARE(a1_s_o2->currentLoopTime(), 250);
    QCOMPARE(a1_s_o3->currentLoopTime(), 250);
}

void tst_QSequentialAnimationGroup2::seekingBackwards()
{
    // sequence operating on same object/property
    QAnimationGroup2 *sequence = new QSequentialAnimationGroup2();
    TestAnimation *a1_s_o1 = new TestAnimation;
    TestAnimation *a2_s_o1 = new TestAnimation;
    TestAnimation *a3_s_o1 = new TestAnimation;
    a2_s_o1->setLoopCount(3);
    sequence->appendAnimation(a1_s_o1);
    sequence->appendAnimation(a2_s_o1);
    sequence->appendAnimation(a3_s_o1);

    // sequence operating on different object/properties
    QAnimationGroup2 *sequence2 = new QSequentialAnimationGroup2();
    TestAnimation *a1_s_o2 = new TestAnimation;
    TestAnimation *a1_s_o3 = new TestAnimation;
    sequence2->appendAnimation(a1_s_o2);
    sequence2->appendAnimation(a1_s_o3);

    QSequentialAnimationGroup2 group;
    group.appendAnimation(sequence);
    group.appendAnimation(sequence2);

    group.start();

    // Current time = 1600
    group.setCurrentTime(1600);
    QCOMPARE(group.currentLoopTime(), 1600);
    QCOMPARE(sequence->currentLoopTime(), 1250);
    QCOMPARE(a1_s_o1->currentLoopTime(), 250);
    QCOMPARE(a2_s_o1->currentLoopTime(), 250);
    QCOMPARE(a2_s_o1->currentLoop(), 2);
    QCOMPARE(a3_s_o1->currentLoopTime(), 250);
    QCOMPARE(sequence2->currentLoopTime(), 350);
    QCOMPARE(a1_s_o2->currentLoopTime(), 250);
    QCOMPARE(a1_s_o3->currentLoopTime(), 100);

    QCOMPARE(group.state(), QAnimationGroup2::Running);
    QCOMPARE(sequence->state(), QAnimationGroup2::Stopped);
    QCOMPARE(a1_s_o1->state(), QAnimationGroup2::Stopped);
    QCOMPARE(sequence2->state(), QAnimationGroup2::Running);
    QCOMPARE(a1_s_o2->state(), QAnimationGroup2::Stopped);
    QCOMPARE(a1_s_o3->state(), QAnimationGroup2::Running);

    // Seeking backwards, current time = 1
    group.setCurrentTime(1);
    QCOMPARE(group.currentLoopTime(), 1);
    QCOMPARE(sequence->currentLoopTime(), 1);
    QCOMPARE(a1_s_o1->currentLoopTime(), 1);

    QEXPECT_FAIL("", "rewinding in nested groups is considered as a restart from the children,"
        "hence they don't reset from their current animation", Continue);
    QCOMPARE(a2_s_o1->currentLoopTime(), 0);
    QEXPECT_FAIL("", "rewinding in nested groups is considered as a restart from the children,"
        "hence they don't reset from their current animation", Continue);
    QCOMPARE(a2_s_o1->currentLoop(), 0);
    QEXPECT_FAIL("", "rewinding in nested groups is considered as a restart from the children,"
        "hence they don't reset from their current animation", Continue);
    QCOMPARE(a3_s_o1->currentLoopTime(), 0);
    QCOMPARE(sequence2->currentLoopTime(), 0);
    QCOMPARE(a1_s_o2->currentLoopTime(), 0);
    QCOMPARE(a1_s_o3->currentLoopTime(), 0);

    QCOMPARE(group.state(), QAnimationGroup2::Running);
    QCOMPARE(sequence->state(), QAnimationGroup2::Running);
    QCOMPARE(a1_s_o1->state(), QAnimationGroup2::Running);
    QCOMPARE(sequence2->state(), QAnimationGroup2::Stopped);
    QCOMPARE(a1_s_o2->state(), QAnimationGroup2::Stopped);
    QCOMPARE(a1_s_o3->state(), QAnimationGroup2::Stopped);

    // Current time = 2000
    group.setCurrentTime(2000);
    QCOMPARE(group.currentLoopTime(), 1750);
    QCOMPARE(sequence->currentLoopTime(), 1250);
    QCOMPARE(a1_s_o1->currentLoopTime(), 250);
    QCOMPARE(a2_s_o1->currentLoopTime(), 250);
    QCOMPARE(a2_s_o1->currentLoop(), 2);
    QCOMPARE(a3_s_o1->currentLoopTime(), 250);
    QCOMPARE(sequence2->currentLoopTime(), 500);
    QCOMPARE(a1_s_o2->currentLoopTime(), 250);
    QCOMPARE(a1_s_o3->currentLoopTime(), 250);

    QCOMPARE(group.state(), QAnimationGroup2::Stopped);
    QCOMPARE(sequence->state(), QAnimationGroup2::Stopped);
    QCOMPARE(a1_s_o1->state(), QAnimationGroup2::Stopped);
    QCOMPARE(sequence2->state(), QAnimationGroup2::Stopped);
    QCOMPARE(a1_s_o2->state(), QAnimationGroup2::Stopped);
    QCOMPARE(a1_s_o3->state(), QAnimationGroup2::Stopped);
}

typedef QList<QAbstractAnimation2::State> StateList;

static bool compareStates(const StateChangeListener& spy, const StateList &expectedStates)
{
    bool equals = true;
    for (int i = 0; i < qMax(expectedStates.count(), spy.count()); ++i) {
        if (i >= spy.count() || i >= expectedStates.count()) {
            equals = false;
            break;
        }
        QAbstractAnimation2::State st = expectedStates.at(i);
        QAbstractAnimation2::State actual = spy.states.at(i);
        if (equals && actual != st) {
            equals = false;
            break;
        }
    }
    if (!equals) {
        const char *stateStrings[] = {"Stopped", "Paused", "Running"};
        QString e,a;
        for (int i = 0; i < qMax(expectedStates.count(), spy.count()); ++i) {
            if (i < expectedStates.count()) {
                int exp = int(expectedStates.at(i));
                    if (!e.isEmpty())
                        e += QLatin1String(", ");
                e += QLatin1String(stateStrings[exp]);
            }
            if (i < spy.count()) {
                QAbstractAnimation2::State actual = spy.states.at(i);
                if (!a.isEmpty())
                    a += QLatin1String(", ");
                if (int(actual) >= 0 && int(actual) <= 2) {
                    a += QLatin1String(stateStrings[int(actual)]);
                } else {
                    a += QLatin1String("NaN");
                }
            }

        }
        qDebug("\n"
               "expected (count == %d): %s\n"
               "actual   (count == %d): %s\n", expectedStates.count(), qPrintable(e), spy.count(), qPrintable(a));
    }
    return equals;
}

void tst_QSequentialAnimationGroup2::pauseAndResume()
{
    // sequence operating on same object/property
    QAnimationGroup2 *sequence = new QSequentialAnimationGroup2();
    TestAnimation *a1_s_o1 = new TestAnimation;
    TestAnimation *a2_s_o1 = new TestAnimation;
    TestAnimation *a3_s_o1 = new TestAnimation;
    a2_s_o1->setLoopCount(2);
    sequence->appendAnimation(a1_s_o1);
    sequence->appendAnimation(a2_s_o1);
    sequence->appendAnimation(a3_s_o1);
    sequence->setLoopCount(2);

    StateChangeListener a1StateChangedSpy;
    a1_s_o1->addAnimationChangeListener(&a1StateChangedSpy, QAbstractAnimation2::StateChange);
    StateChangeListener seqStateChangedSpy;
    sequence->addAnimationChangeListener(&seqStateChangedSpy, QAbstractAnimation2::StateChange);

    QSequentialAnimationGroup2 group;
    group.appendAnimation(sequence);

    group.start();
    group.pause();

    // Current time = 1751
    group.setCurrentTime(1751);
    QCOMPARE(group.currentLoopTime(), 1751);
    QCOMPARE(sequence->currentLoopTime(), 751);
    QCOMPARE(sequence->currentLoop(), 1);
    QCOMPARE(a1_s_o1->currentLoopTime(), 250);
    QCOMPARE(a2_s_o1->currentLoopTime(), 250);
    QCOMPARE(a2_s_o1->currentLoop(), 1);
    QCOMPARE(a3_s_o1->currentLoop(), 0);
    QCOMPARE(a3_s_o1->currentLoopTime(), 1);

    QCOMPARE(group.state(), QAnimationGroup2::Paused);
    QCOMPARE(sequence->state(), QAnimationGroup2::Paused);
    QCOMPARE(a1_s_o1->state(), QAnimationGroup2::Stopped);
    QCOMPARE(a2_s_o1->state(), QAnimationGroup2::Stopped);
    QCOMPARE(a3_s_o1->state(), QAnimationGroup2::Paused);

    QCOMPARE(a1StateChangedSpy.count(), 5);     // Running,Paused,Stopped,Running,Stopped
    QCOMPARE(seqStateChangedSpy.count(), 2);    // Running,Paused

    QVERIFY(compareStates(a1StateChangedSpy, (StateList() << QAbstractAnimation2::Running
                                              << QAbstractAnimation2::Paused
                                              << QAbstractAnimation2::Stopped
                                              << QAbstractAnimation2::Running
                                              << QAbstractAnimation2::Stopped)));

    //### is this the same test as compareStates test above?
    QCOMPARE(a1StateChangedSpy.states.at(0), QAnimationGroup2::Running);
    QCOMPARE(a1StateChangedSpy.states.at(1), QAnimationGroup2::Paused);
    QCOMPARE(a1StateChangedSpy.states.at(2), QAnimationGroup2::Stopped);
    QCOMPARE(a1StateChangedSpy.states.at(3), QAnimationGroup2::Running);
    QCOMPARE(a1StateChangedSpy.states.at(4), QAnimationGroup2::Stopped);

    QCOMPARE(seqStateChangedSpy.states.at(0), QAnimationGroup2::Running);
    QCOMPARE(seqStateChangedSpy.states.at(1), QAnimationGroup2::Paused);

    group.resume();

    QCOMPARE(group.state(), QAnimationGroup2::Running);
    QCOMPARE(sequence->state(), QAnimationGroup2::Running);
    QCOMPARE(a1_s_o1->state(), QAnimationGroup2::Stopped);
    QCOMPARE(a2_s_o1->state(), QAnimationGroup2::Stopped);
    QCOMPARE(a3_s_o1->state(), QAnimationGroup2::Running);

    QVERIFY(group.currentLoopTime() >= 1751);
    QVERIFY(sequence->currentLoopTime() >= 751);
    QCOMPARE(sequence->currentLoop(), 1);
    QCOMPARE(a1_s_o1->currentLoopTime(), 250);
    QCOMPARE(a2_s_o1->currentLoopTime(), 250);
    QCOMPARE(a2_s_o1->currentLoop(), 1);
    QCOMPARE(a3_s_o1->currentLoop(), 0);
    QVERIFY(a3_s_o1->currentLoopTime() >= 1);

    QCOMPARE(seqStateChangedSpy.count(), 3);    // Running,Paused,Running
    QCOMPARE(seqStateChangedSpy.states.at(2), QAnimationGroup2::Running);

    group.pause();

    QCOMPARE(group.state(), QAnimationGroup2::Paused);
    QCOMPARE(sequence->state(), QAnimationGroup2::Paused);
    QCOMPARE(a1_s_o1->state(), QAnimationGroup2::Stopped);
    QCOMPARE(a2_s_o1->state(), QAnimationGroup2::Stopped);
    QCOMPARE(a3_s_o1->state(), QAnimationGroup2::Paused);

    QVERIFY(group.currentLoopTime() >= 1751);
    QVERIFY(sequence->currentLoopTime() >= 751);
    QCOMPARE(sequence->currentLoop(), 1);
    QCOMPARE(a1_s_o1->currentLoopTime(), 250);
    QCOMPARE(a2_s_o1->currentLoopTime(), 250);
    QCOMPARE(a2_s_o1->currentLoop(), 1);
    QCOMPARE(a3_s_o1->currentLoop(), 0);
    QVERIFY(a3_s_o1->currentLoopTime() >= 1);

    QCOMPARE(seqStateChangedSpy.count(), 4);    // Running,Paused,Running,Paused
    QCOMPARE(seqStateChangedSpy.states.at(3), QAnimationGroup2::Paused);

    group.stop();

    QCOMPARE(seqStateChangedSpy.count(), 5);    // Running,Paused,Running,Paused,Stopped
    QCOMPARE(seqStateChangedSpy.states.at(4), QAnimationGroup2::Stopped);
}

void tst_QSequentialAnimationGroup2::restart()
{
    // originally was sequence operating on same object/property
    QAnimationGroup2 *sequence = new QSequentialAnimationGroup2();
    //### no equivilant signal
    //QSignalSpy seqCurrentAnimChangedSpy(sequence, SIGNAL(currentAnimationChanged(QAbstractAnimation2*)));

    StateChangeListener seqStateChangedSpy;
    sequence->addAnimationChangeListener(&seqStateChangedSpy, QAbstractAnimation2::StateChange);

    TestAnimation *anims[3];
    StateChangeListener *animsStateChanged[3];

    for (int i = 0; i < 3; i++) {
        anims[i] = new TestAnimation(100);
        animsStateChanged[i] = new StateChangeListener;
        anims[i]->addAnimationChangeListener(animsStateChanged[i], QAbstractAnimation2::StateChange);
    }

    anims[1]->setLoopCount(2);
    sequence->appendAnimation(anims[0]);
    sequence->appendAnimation(anims[1]);
    sequence->appendAnimation(anims[2]);
    sequence->setLoopCount(2);

    QSequentialAnimationGroup2 group;
    group.appendAnimation(sequence);

    group.start();

    QTest::qWait(500);

    QCOMPARE(group.state(), QAnimationGroup2::Running);

    QTest::qWait(300);
    QTRY_COMPARE(group.state(), QAnimationGroup2::Stopped);

    for (int i = 0; i < 3; i++) {
        QCOMPARE(animsStateChanged[i]->count(), 4);
        QCOMPARE(animsStateChanged[i]->states.at(0), QAnimationGroup2::Running);
        QCOMPARE(animsStateChanged[i]->states.at(1), QAnimationGroup2::Stopped);
        QCOMPARE(animsStateChanged[i]->states.at(2), QAnimationGroup2::Running);
        QCOMPARE(animsStateChanged[i]->states.at(3), QAnimationGroup2::Stopped);
    }

    QCOMPARE(seqStateChangedSpy.count(), 2);
    QCOMPARE(seqStateChangedSpy.states.at(0), QAnimationGroup2::Running);
    QCOMPARE(seqStateChangedSpy.states.at(1), QAnimationGroup2::Stopped);

    //QCOMPARE(seqCurrentAnimChangedSpy.count(), 6);
    //for(int i=0; i<seqCurrentAnimChangedSpy.count(); i++)
    //        QCOMPARE(static_cast<QAbstractAnimation2*>(anims[i%3]), qVariantValue<QAbstractAnimation2*>(seqCurrentAnimChangedSpy.at(i).at(0)));

    group.start();

    QCOMPARE(animsStateChanged[0]->count(), 5);
    QCOMPARE(animsStateChanged[1]->count(), 4);
    QCOMPARE(animsStateChanged[2]->count(), 4);
    QCOMPARE(seqStateChangedSpy.count(), 3);
}

void tst_QSequentialAnimationGroup2::looping()
{
    // originally was sequence operating on same object/property
    QSequentialAnimationGroup2 *sequence = new QSequentialAnimationGroup2();
    QAbstractAnimation2 *a1_s_o1 = new TestAnimation;
    QAbstractAnimation2 *a2_s_o1 = new TestAnimation;
    QAbstractAnimation2 *a3_s_o1 = new TestAnimation;

    StateChangeListener a1Spy;
    a1_s_o1->addAnimationChangeListener(&a1Spy, QAbstractAnimation2::StateChange);
    StateChangeListener a2Spy;
    a2_s_o1->addAnimationChangeListener(&a2Spy, QAbstractAnimation2::StateChange);
    StateChangeListener a3Spy;
    a3_s_o1->addAnimationChangeListener(&a3Spy, QAbstractAnimation2::StateChange);
    StateChangeListener seqSpy;
    sequence->addAnimationChangeListener(&seqSpy, QAbstractAnimation2::StateChange);

    a2_s_o1->setLoopCount(2);
    sequence->appendAnimation(a1_s_o1);
    sequence->appendAnimation(a2_s_o1);
    sequence->appendAnimation(a3_s_o1);
    sequence->setLoopCount(2);

    QSequentialAnimationGroup2 group;
    StateChangeListener groupSpy;
    group.addAnimationChangeListener(&groupSpy, QAbstractAnimation2::StateChange);

    group.appendAnimation(sequence);
    group.setLoopCount(2);

    group.start();
    group.pause();

    // Current time = 1750
    group.setCurrentTime(1750);
    QCOMPARE(group.currentLoopTime(), 1750);
    QCOMPARE(sequence->currentLoopTime(), 750);
    QCOMPARE(sequence->currentLoop(), 1);
    QCOMPARE(a1_s_o1->currentLoopTime(), 250);
    QCOMPARE(a2_s_o1->currentLoopTime(), 250);
    QCOMPARE(a2_s_o1->currentLoop(), 1);
    // this animation is at the beginning because it is the current one inside sequence
    QCOMPARE(a3_s_o1->currentLoop(), 0);
    QCOMPARE(a3_s_o1->currentLoopTime(), 0);
    QCOMPARE(sequence->currentAnimation(), a3_s_o1);

    QCOMPARE(group.state(), QAnimationGroup2::Paused);
    QCOMPARE(sequence->state(), QAnimationGroup2::Paused);
    QCOMPARE(a1_s_o1->state(), QAnimationGroup2::Stopped);
    QCOMPARE(a2_s_o1->state(), QAnimationGroup2::Stopped);
    QCOMPARE(a3_s_o1->state(), QAnimationGroup2::Paused);

    QCOMPARE(a1Spy.count(), 5);     // Running,Paused,Stopped,Running,Stopped
    QVERIFY(compareStates(a1Spy, (StateList() << QAbstractAnimation2::Running
                                              << QAbstractAnimation2::Paused
                                              << QAbstractAnimation2::Stopped
                                              << QAbstractAnimation2::Running
                                              << QAbstractAnimation2::Stopped)));

    QCOMPARE(a2Spy.count(), 4);     // Running,Stopped,Running,Stopped
    QVERIFY(compareStates(a3Spy, (StateList() << QAbstractAnimation2::Running
                                              << QAbstractAnimation2::Stopped
                                              << QAbstractAnimation2::Running
                                              << QAbstractAnimation2::Paused)));

    QCOMPARE(seqSpy.count(), 2);    // Running,Paused
    QCOMPARE(groupSpy.count(), 2);  // Running,Paused

    // Looping, current time = duration + 1
    group.setCurrentTime(group.duration() + 1);
    QCOMPARE(group.currentLoopTime(), 1);
    QCOMPARE(group.currentLoop(), 1);
    QCOMPARE(sequence->currentLoopTime(), 1);
    QCOMPARE(sequence->currentLoop(), 0);
    QCOMPARE(a1_s_o1->currentLoopTime(), 1);
    QCOMPARE(a2_s_o1->currentLoopTime(), 250);
    QCOMPARE(a2_s_o1->currentLoop(), 1);
    // this animation is at the end because it was run on the previous loop
    QCOMPARE(a3_s_o1->currentLoop(), 0);
    QCOMPARE(a3_s_o1->currentLoopTime(), 250);

    QCOMPARE(group.state(), QAnimationGroup2::Paused);
    QCOMPARE(sequence->state(), QAnimationGroup2::Paused);
    QCOMPARE(a1_s_o1->state(), QAnimationGroup2::Paused);
    QCOMPARE(a2_s_o1->state(), QAnimationGroup2::Stopped);
    QCOMPARE(a3_s_o1->state(), QAnimationGroup2::Stopped);

    QCOMPARE(a1Spy.count(), 7); // Running,Paused,Stopped,Running,Stopped,Running,Stopped
    QCOMPARE(a2Spy.count(), 4); // Running, Stopped, Running, Stopped
    QVERIFY(compareStates(a3Spy, (StateList() << QAbstractAnimation2::Running
                                              << QAbstractAnimation2::Stopped
                                              << QAbstractAnimation2::Running
                                              << QAbstractAnimation2::Paused
                                              << QAbstractAnimation2::Stopped)));
    QVERIFY(compareStates(seqSpy, (StateList() << QAbstractAnimation2::Running
                                               << QAbstractAnimation2::Paused
                                               << QAbstractAnimation2::Stopped
                                               << QAbstractAnimation2::Running
                                               << QAbstractAnimation2::Paused)));
    QCOMPARE(groupSpy.count(), 2);

    //cleanup
    a1_s_o1->removeAnimationChangeListener(&a1Spy, QAbstractAnimation2::StateChange);
    a2_s_o1->removeAnimationChangeListener(&a2Spy, QAbstractAnimation2::StateChange);
    a3_s_o1->removeAnimationChangeListener(&a3Spy, QAbstractAnimation2::StateChange);
    sequence->removeAnimationChangeListener(&seqSpy, QAbstractAnimation2::StateChange);
    group.removeAnimationChangeListener(&groupSpy, QAbstractAnimation2::StateChange);
}

void tst_QSequentialAnimationGroup2::startDelay()
{
    QSequentialAnimationGroup2 group;
    group.appendAnimation(new QPauseAnimation2(250));
    group.appendAnimation(new QPauseAnimation2(125));
    QCOMPARE(group.totalDuration(), 375);

    group.start();
    QCOMPARE(group.state(), QAnimationGroup2::Running);

    QTest::qWait(500);

    QTRY_COMPARE(group.state(), QAnimationGroup2::Stopped);
    QVERIFY(group.currentLoopTime() == 375);
}

void tst_QSequentialAnimationGroup2::clearGroup()
{
    QSequentialAnimationGroup2 group;

    static const int animationCount = 20;

    for (int i = 0; i < animationCount/2; ++i) {
        QSequentialAnimationGroup2 *subGroup = new QSequentialAnimationGroup2;
        group.appendAnimation(subGroup);
        group.appendAnimation(new QPauseAnimation2(100));
        subGroup->appendAnimation(new QPauseAnimation2(10));
    }

    int count = 0;
    for (QAbstractAnimation2 *anim = group.firstChild(); anim; anim = anim->nextSibling())
        ++count;
    QCOMPARE(count, animationCount);

    group.clear();

    QVERIFY(!group.firstChild() && !group.lastChild());
    QCOMPARE(group.currentLoopTime(), 0);
}

void tst_QSequentialAnimationGroup2::groupWithZeroDurationAnimations()
{
    QSequentialAnimationGroup2 group;

    TestValueAnimation *a1 = new TestValueAnimation(0);
    a1->start = 42;
    a1->end = 43;
    group.appendAnimation(a1);

    //this should just run fine and change nothing
    group.setCurrentTime(0);
    QCOMPARE(group.currentAnimation(), static_cast<QAbstractAnimation2*>(a1));

    TestValueAnimation *a2 = new TestValueAnimation(500);
    a2->start = 13;
    a2->end = 31;
    group.appendAnimation(a2);

    TestValueAnimation *a3 = new TestValueAnimation(0);
    a3->start = 43;
    a3->end = 44;
    group.appendAnimation(a3);

    TestValueAnimation *a4 = new TestValueAnimation(250);
    a4->start = 13;
    a4->end = 75;
    group.appendAnimation(a4);

    TestValueAnimation *a5 = new TestValueAnimation(0);
    a5->start = 42;
    a5->end = 12;
    group.appendAnimation(a5);

    QCOMPARE((int)a1->value, 43);   //### is this actually the behavior we want?
    QCOMPARE((int)a2->value, 0);
    QCOMPARE((int)a3->value, 0);
    QCOMPARE((int)a4->value, 0);
    QCOMPARE((int)a5->value, 0);

    group.start();

    QCOMPARE((int)a1->value, 43);   //### is this actually the behavior we want?
    QCOMPARE((int)a2->value, 13);
    QCOMPARE((int)a3->value, 0);
    QCOMPARE((int)a4->value, 0);
    QCOMPARE((int)a5->value, 0);

    QTest::qWait(100);

    QCOMPARE((int)a1->value, 43);
    QVERIFY(a2->value > 13 && a2->value < 31);
    QCOMPARE((int)a3->value, 0);
    QCOMPARE((int)a4->value, 0);
    QCOMPARE((int)a5->value, 0);

    QTest::qWait(500);

    QTRY_COMPARE((int)a3->value, 44);
    QCOMPARE((int)a1->value, 43);
    QCOMPARE((int)a2->value, 31);
    //QCOMPARE((int)a4->value, 36);
    QCOMPARE((int)a5->value, 0);
    QCOMPARE(a1->state(), QAnimationGroup2::Stopped);
    QCOMPARE(a2->state(), QAnimationGroup2::Stopped);
    QCOMPARE(a3->state(), QAnimationGroup2::Stopped);
    QCOMPARE(a4->state(), QAnimationGroup2::Running);
    QCOMPARE(a5->state(), QAnimationGroup2::Stopped);
    QCOMPARE(group.state(), QAnimationGroup2::Running);
    QTest::qWait(500);

    QTRY_COMPARE(group.state(), QAnimationGroup2::Stopped);
    QCOMPARE((int)a1->value, 43);
    QCOMPARE((int)a2->value, 31);
    QCOMPARE((int)a3->value, 44);
    QCOMPARE((int)a4->value, 75);
    QCOMPARE((int)a5->value, 12);
    QCOMPARE(a1->state(), QAnimationGroup2::Stopped);
    QCOMPARE(a2->state(), QAnimationGroup2::Stopped);
    QCOMPARE(a3->state(), QAnimationGroup2::Stopped);
    QCOMPARE(a4->state(), QAnimationGroup2::Stopped);
    QCOMPARE(a5->state(), QAnimationGroup2::Stopped);
}

void tst_QSequentialAnimationGroup2::propagateGroupUpdateToChildren()
{
    // this test verifies if group state changes are updating its children correctly
    QSequentialAnimationGroup2 group;

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
    QCOMPARE(anim2.state(), QAnimationGroup2::Stopped);

    group.pause();

    QCOMPARE(group.state(), QAnimationGroup2::Paused);
    QCOMPARE(anim1.state(), QAnimationGroup2::Paused);
    QCOMPARE(anim2.state(), QAnimationGroup2::Stopped);

    group.stop();

    QCOMPARE(group.state(), QAnimationGroup2::Stopped);
    QCOMPARE(anim1.state(), QAnimationGroup2::Stopped);
    QCOMPARE(anim2.state(), QAnimationGroup2::Stopped);
}

void tst_QSequentialAnimationGroup2::updateChildrenWithRunningGroup()
{
    // assert that its possible to modify a child's state directly while their group is running
    QSequentialAnimationGroup2 group;

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

void tst_QSequentialAnimationGroup2::deleteChildrenWithRunningGroup()
{
    // test if children can be activated when their group is stopped
    QSequentialAnimationGroup2 group;

    TestAnimation *anim1 = new TestAnimation(200);
    group.appendAnimation(anim1);

    QCOMPARE(group.duration(), anim1->duration());

    group.start();
    QCOMPARE(group.state(), QAnimationGroup2::Running);
    QCOMPARE(anim1->state(), QAnimationGroup2::Running);

    QTest::qWait(100);
    QTRY_VERIFY(group.currentLoopTime() > 0);

    delete anim1;
    QVERIFY(!group.firstChild());
    QCOMPARE(group.duration(), 0);
    QCOMPARE(group.state(), QAnimationGroup2::Stopped);
    QCOMPARE(group.currentLoopTime(), 0); //that's the invariant
}

void tst_QSequentialAnimationGroup2::startChildrenWithStoppedGroup()
{
    // test if children can be activated when their group is stopped
    QSequentialAnimationGroup2 group;

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

void tst_QSequentialAnimationGroup2::stopGroupWithRunningChild()
{
    // children that started independently will not be affected by a group stop
    QSequentialAnimationGroup2 group;

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

void tst_QSequentialAnimationGroup2::startGroupWithRunningChild()
{
    // as the group has precedence over its children, starting a group will restart all the children
    QSequentialAnimationGroup2 group;

    TestAnimation *anim1 = new TestAnimation(200);
    TestAnimation *anim2 = new TestAnimation(200);

    StateChangeListener stateChangedSpy1;
    anim1->addAnimationChangeListener(&stateChangedSpy1, QAbstractAnimation2::StateChange);
    StateChangeListener stateChangedSpy2;
    anim2->addAnimationChangeListener(&stateChangedSpy2, QAbstractAnimation2::StateChange);

    QCOMPARE(stateChangedSpy1.count(), 0);
    QCOMPARE(stateChangedSpy2.count(), 0);
    QCOMPARE(group.state(), QAnimationGroup2::Stopped);
    QCOMPARE(anim1->state(), QAnimationGroup2::Stopped);
    QCOMPARE(anim2->state(), QAnimationGroup2::Stopped);

    group.appendAnimation(anim1);
    group.appendAnimation(anim2);

    anim1->start();
    anim2->start();
    anim2->pause();

    QVERIFY(compareStates(stateChangedSpy1, (StateList() << QAbstractAnimation2::Running)));

    QVERIFY(compareStates(stateChangedSpy2, (StateList() << QAbstractAnimation2::Running
                                                         << QAbstractAnimation2::Paused)));

    QCOMPARE(group.state(), QAnimationGroup2::Stopped);
    QCOMPARE(anim1->state(), QAnimationGroup2::Running);
    QCOMPARE(anim2->state(), QAnimationGroup2::Paused);

    group.start();

    QVERIFY(compareStates(stateChangedSpy1, (StateList() << QAbstractAnimation2::Running
                                                         << QAbstractAnimation2::Stopped
                                                         << QAbstractAnimation2::Running)));
    QVERIFY(compareStates(stateChangedSpy2, (StateList() << QAbstractAnimation2::Running
                                                         << QAbstractAnimation2::Paused)));

    QCOMPARE(group.state(), QAnimationGroup2::Running);
    QCOMPARE(anim1->state(), QAnimationGroup2::Running);
    QCOMPARE(anim2->state(), QAnimationGroup2::Paused);

    QTest::qWait(300);

    QCOMPARE(group.state(), QAnimationGroup2::Running);
    QCOMPARE(anim1->state(), QAnimationGroup2::Stopped);
    QCOMPARE(anim2->state(), QAnimationGroup2::Running);

    QCOMPARE(stateChangedSpy2.count(), 4);
    QCOMPARE(stateChangedSpy2.states.at(2), QAnimationGroup2::Stopped);
    QCOMPARE(stateChangedSpy2.states.at(3), QAnimationGroup2::Running);

    group.stop();

    QCOMPARE(group.state(), QAnimationGroup2::Stopped);
    QCOMPARE(anim1->state(), QAnimationGroup2::Stopped);
    QCOMPARE(anim2->state(), QAnimationGroup2::Stopped);

    anim1->removeAnimationChangeListener(&stateChangedSpy1, QAbstractAnimation2::StateChange);
    anim2->removeAnimationChangeListener(&stateChangedSpy2, QAbstractAnimation2::StateChange);
}

void tst_QSequentialAnimationGroup2::zeroDurationAnimation()
{
    QSequentialAnimationGroup2 group;

    TestAnimation *anim1 = new TestAnimation(0);
    TestAnimation *anim2 = new TestAnimation(100);
    TestValueAnimation *anim3 = new TestValueAnimation(0);
    anim3->end = 100;

    StateChangeListener stateChangedSpy;
    anim1->addAnimationChangeListener(&stateChangedSpy, QAbstractAnimation2::StateChange);

    group.appendAnimation(anim1);
    group.appendAnimation(anim2);
    group.appendAnimation(anim3);
    group.setLoopCount(2);
    group.start();

    QCOMPARE(stateChangedSpy.count(), 2);
    QCOMPARE(stateChangedSpy.states.at(0), QAnimationGroup2::Running);
    QCOMPARE(stateChangedSpy.states.at(1), QAnimationGroup2::Stopped);

    QCOMPARE(anim1->state(), QAnimationGroup2::Stopped);
    QCOMPARE(anim2->state(), QAnimationGroup2::Running);
    QCOMPARE(group.state(), QAnimationGroup2::Running);

    //now let's try to seek to the next loop
    group.setCurrentTime(group.duration() + 1);
    QCOMPARE(anim1->state(), QAnimationGroup2::Stopped);
    QCOMPARE(anim2->state(), QAnimationGroup2::Running);
    QCOMPARE(anim3->state(), QAnimationGroup2::Stopped);
    QCOMPARE(group.state(), QAnimationGroup2::Running);
    //TODO: test that anim3 was run
    QCOMPARE(anim3->value, qreal(100)); //anim3 should have been run

    anim1->removeAnimationChangeListener(&stateChangedSpy, QAbstractAnimation2::StateChange);
}

void tst_QSequentialAnimationGroup2::stopUncontrolledAnimations()
{
    QSequentialAnimationGroup2 group;

    UncontrolledAnimation notTimeDriven;
    QCOMPARE(notTimeDriven.totalDuration(), -1);

    TestAnimation loopsForever(100);
    loopsForever.setLoopCount(-1);

    group.appendAnimation(&notTimeDriven);
    group.appendAnimation(&loopsForever);

    group.start();

    QCOMPARE(group.state(), QAnimationGroup2::Running);
    QCOMPARE(notTimeDriven.state(), QAnimationGroup2::Running);
    QCOMPARE(loopsForever.state(), QAnimationGroup2::Stopped);

    notTimeDriven.stop();

    QCOMPARE(group.state(), QAnimationGroup2::Running);
    QCOMPARE(notTimeDriven.state(), QAnimationGroup2::Stopped);
    QCOMPARE(loopsForever.state(), QAnimationGroup2::Running);

    loopsForever.stop();

    QCOMPARE(group.state(), QAnimationGroup2::Stopped);
    QCOMPARE(notTimeDriven.state(), QAnimationGroup2::Stopped);
    QCOMPARE(loopsForever.state(), QAnimationGroup2::Stopped);
}

void tst_QSequentialAnimationGroup2::finishWithUncontrolledAnimation()
{
    //1st case:
    //first we test a group with one uncontrolled animation
    QSequentialAnimationGroup2 group;
    UncontrolledAnimation notTimeDriven;
    group.appendAnimation(&notTimeDriven);
    FinishedListener spy;
    group.addAnimationChangeListener(&spy, QAbstractAnimation2::Completion);

    group.start();
    QCOMPARE(group.state(), QAnimationGroup2::Running);
    QCOMPARE(notTimeDriven.state(), QAnimationGroup2::Running);
    QCOMPARE(group.currentLoopTime(), 0);
    QCOMPARE(notTimeDriven.currentLoopTime(), 0);

    QTest::qWait(300); //wait for the end of notTimeDriven
    QTRY_COMPARE(notTimeDriven.state(), QAnimationGroup2::Stopped);
    const int actualDuration = notTimeDriven.currentLoopTime();
    QCOMPARE(group.state(), QAnimationGroup2::Stopped);
    QCOMPARE(group.currentLoopTime(), actualDuration);
    QCOMPARE(spy.count(), 1);

    //2nd case:
    // lets make sure the seeking will work again
    spy.clear();
    TestAnimation anim;
    group.appendAnimation(&anim);
    StateChangeListener animStateChangedSpy;
    anim.addAnimationChangeListener(&animStateChangedSpy, QAbstractAnimation2::StateChange);

    group.setCurrentTime(300);
    QCOMPARE(group.state(), QAnimationGroup2::Stopped);
    QCOMPARE(notTimeDriven.currentLoopTime(), actualDuration);
    QCOMPARE(group.currentAnimation(), static_cast<QAbstractAnimation2*>(&anim));

    //3rd case:
    //now let's add a perfectly defined animation at the end
    QCOMPARE(animStateChangedSpy.count(), 0);
    group.start();
    QCOMPARE(group.state(), QAnimationGroup2::Running);
    QCOMPARE(notTimeDriven.state(), QAnimationGroup2::Running);
    QCOMPARE(group.currentLoopTime(), 0);
    QCOMPARE(notTimeDriven.currentLoopTime(), 0);

    QCOMPARE(animStateChangedSpy.count(), 0);

    QTest::qWait(300); //wait for the end of notTimeDriven
    QTRY_COMPARE(notTimeDriven.state(), QAnimationGroup2::Stopped);
    QCOMPARE(group.state(), QAnimationGroup2::Running);
    QCOMPARE(anim.state(), QAnimationGroup2::Running);
    QCOMPARE(group.currentAnimation(), static_cast<QAbstractAnimation2*>(&anim));
    QCOMPARE(animStateChangedSpy.count(), 1);
    QTest::qWait(300); //wait for the end of anim

    QTRY_COMPARE(anim.state(), QAnimationGroup2::Stopped);
    QCOMPARE(anim.currentLoopTime(), anim.duration());

    //we should simply be at the end
    QCOMPARE(spy.count(), 1);
    QCOMPARE(animStateChangedSpy.count(), 2);
    QCOMPARE(group.currentLoopTime(), notTimeDriven.currentLoopTime() + anim.currentLoopTime());

    //cleanup
    group.removeAnimationChangeListener(&spy, QAbstractAnimation2::Completion);
    anim.removeAnimationChangeListener(&animStateChangedSpy, QAbstractAnimation2::StateChange);
}

void tst_QSequentialAnimationGroup2::addRemoveAnimation()
{
    //this test is specific to the sequential animation group
    QSequentialAnimationGroup2 group;

    QCOMPARE(group.duration(), 0);
    QCOMPARE(group.currentLoopTime(), 0);
    QAbstractAnimation2 *anim1 = new TestAnimation;
    group.appendAnimation(anim1);
    QCOMPARE(group.duration(), 250);
    QCOMPARE(group.currentLoopTime(), 0);
    QCOMPARE(group.currentAnimation(), anim1);

    //let's append an animation
    QAbstractAnimation2 *anim2 = new TestAnimation;
    group.appendAnimation(anim2);
    QCOMPARE(group.duration(), 500);
    QCOMPARE(group.currentLoopTime(), 0);
    QCOMPARE(group.currentAnimation(), anim1);

    //let's prepend an animation
    QAbstractAnimation2 *anim0 = new TestAnimation;
    group.prependAnimation(anim0);
    QCOMPARE(group.duration(), 750);
    QCOMPARE(group.currentLoopTime(), 0);
    QCOMPARE(group.currentAnimation(), anim0); //anim0 has become the new currentAnimation

    group.setCurrentTime(300); //anim0 | anim1 | anim2
    QCOMPARE(group.currentLoopTime(), 300);
    QCOMPARE(group.currentAnimation(), anim1);
    QCOMPARE(anim1->currentLoopTime(), 50);

    group.removeAnimation(anim0); //anim1 | anim2
    QCOMPARE(group.currentLoopTime(), 50);
    QCOMPARE(group.currentAnimation(), anim1);
    QCOMPARE(anim1->currentLoopTime(), 50);

    group.setCurrentTime(0);
    group.prependAnimation(anim0); //anim0 | anim1 | anim2
    group.setCurrentTime(300);
    QCOMPARE(group.currentLoopTime(), 300);
    QCOMPARE(group.currentAnimation(), anim1);
    QCOMPARE(anim1->currentLoopTime(), 50);

    group.removeAnimation(anim1); //anim0 | anim2
    QCOMPARE(group.currentLoopTime(), 250);
    QCOMPARE(group.currentAnimation(), anim2);
    QCOMPARE(anim0->currentLoopTime(), 250);
}

void tst_QSequentialAnimationGroup2::currentAnimation()
{
    QSequentialAnimationGroup2 group;
    QVERIFY(group.currentAnimation() == 0);

    TestAnimation anim(0);
    group.appendAnimation(&anim);
    QCOMPARE(group.currentAnimation(), static_cast<QAbstractAnimation2*>(&anim));
}

void tst_QSequentialAnimationGroup2::currentAnimationWithZeroDuration()
{
    QSequentialAnimationGroup2 group;
    QVERIFY(group.currentAnimation() == 0);

    TestAnimation zero1(0);
    TestAnimation zero2(0);

    TestAnimation anim;

    TestAnimation zero3(0);
    TestAnimation zero4(0);

    group.appendAnimation(&zero1);
    group.appendAnimation(&zero2);
    group.appendAnimation(&anim);
    group.appendAnimation(&zero3);
    group.appendAnimation(&zero4);

    QCOMPARE(group.currentAnimation(), static_cast<QAbstractAnimation2*>(&zero1));

    group.setCurrentTime(0);
    QCOMPARE(group.currentAnimation(), static_cast<QAbstractAnimation2*>(&anim));

    group.setCurrentTime(group.duration());
    QCOMPARE(group.currentAnimation(), static_cast<QAbstractAnimation2*>(&zero4));

    group.setDirection(QAbstractAnimation2::Backward);

    group.setCurrentTime(0);
    QCOMPARE(group.currentAnimation(), static_cast<QAbstractAnimation2*>(&zero1));

    group.setCurrentTime(group.duration());
    QCOMPARE(group.currentAnimation(), static_cast<QAbstractAnimation2*>(&anim));
}

void tst_QSequentialAnimationGroup2::insertAnimation()
{
    QSequentialAnimationGroup2 group;
    group.setLoopCount(2);
    TestAnimation *anim = new TestAnimation;
    group.appendAnimation(anim);
    QCOMPARE(group.duration(), anim->duration());
    group.setCurrentTime(300);
    QCOMPARE(group.currentLoop(), 1);

    //this will crash if the sequential group calls duration on the created animation
    group.appendAnimation(new TestAnimation);
}

class ClearFinishedListener: public QAnimation2ChangeListener
{
public:
    ClearFinishedListener(QSequentialAnimationGroup2 *g) : group(g) {}

    virtual void animationFinished(QAbstractAnimation2 *)
    {
        group->clear();
    }

    QSequentialAnimationGroup2 *group;
};

class RefillFinishedListener: public QAnimation2ChangeListener
{
public:
    RefillFinishedListener(QSequentialAnimationGroup2 *g) : group(g) {}

    virtual void animationFinished(QAbstractAnimation2 *)
    {
        group->stop();
        group->clear();
        group->appendAnimation(new TestAnimation);
        group->start();
    }

    QSequentialAnimationGroup2 *group;
};

void tst_QSequentialAnimationGroup2::clear()
{
    QSKIP("deleting an animation when finished is not currently supported");
    QSequentialAnimationGroup2 group;
    TestAnimation *anim1 = new TestAnimation;
    group.appendAnimation(anim1);
    ClearFinishedListener clearListener(&group);
    anim1->addAnimationChangeListener(&clearListener, QAbstractAnimation2::Completion);

    TestAnimation *anim2 = new TestAnimation;
    group.appendAnimation(anim2);
    QCOMPARE(group.firstChild(), anim1);
    QCOMPARE(group.lastChild(), anim2);

    group.start();
    QTest::qWait(anim1->duration() + 100);
    QTRY_VERIFY(!group.firstChild());
    QCOMPARE(group.state(), QAbstractAnimation2::Stopped);
    QCOMPARE(group.currentLoopTime(), 0);

    anim1 = new TestAnimation;
    group.appendAnimation(anim1);
    RefillFinishedListener refillListener(&group);
    anim1->addAnimationChangeListener(&refillListener, QAbstractAnimation2::Completion);
    group.start();
    QTest::qWait(anim1->duration() + 100);
    QTRY_COMPARE(group.state(), QAbstractAnimation2::Running);
}

void tst_QSequentialAnimationGroup2::pauseResume()
{
    QParallelAnimationGroup2 group;
    TestAnimation *anim = new TestAnimation;
    group.appendAnimation(anim);
    StateChangeListener spy;
    anim->addAnimationChangeListener(&spy, QAbstractAnimation2::StateChange);
    QCOMPARE(group.duration(), 250);
    group.start();
    QTest::qWait(100);
    QTRY_COMPARE(group.state(), QAnimationGroup2::Running);
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

    anim->removeAnimationChangeListener(&spy, QAbstractAnimation2::StateChange);
}

QTEST_MAIN(tst_QSequentialAnimationGroup2)
#include "tst_qsequentialanimationgroup2.moc"
