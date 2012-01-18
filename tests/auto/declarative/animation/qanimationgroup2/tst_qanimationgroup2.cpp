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

#include <QtDeclarative/private/qanimationgroup2_p.h>
#include <QtDeclarative/private/qsequentialanimationgroup2_p.h>
#include <QtDeclarative/private/qparallelanimationgroup2_p.h>

Q_DECLARE_METATYPE(QAbstractAnimation2::State)

class tst_QAnimationGroup2 : public QObject
{
    Q_OBJECT
public Q_SLOTS:
    void initTestCase();

private slots:
    void construction();
    void emptyGroup();
    void setCurrentTime();
    void addChildTwice();
};

void tst_QAnimationGroup2::initTestCase()
{
    qRegisterMetaType<QAbstractAnimation2::State>("QAbstractAnimation2::State");
}

void tst_QAnimationGroup2::construction()
{
    QSequentialAnimationGroup2 animationgroup;
}

class TestableGenericAnimation : public QAbstractAnimation2
{
public:
    TestableGenericAnimation(int duration = 250) : m_duration(duration) {}
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

    int count()
    {
        return states.count();
    }

    QList<QAbstractAnimation2::State> states;
};

void tst_QAnimationGroup2::emptyGroup()
{
    QSequentialAnimationGroup2 group;
    StateChangeListener groupStateChangedSpy;
    group.addAnimationChangeListener(&groupStateChangedSpy, QAbstractAnimation2::StateChange);

    QCOMPARE(group.state(), QAnimationGroup2::Stopped);
    group.start();

    QCOMPARE(groupStateChangedSpy.count(), 2);

    QCOMPARE(groupStateChangedSpy.states.at(0), QAnimationGroup2::Running);
    QCOMPARE(groupStateChangedSpy.states.at(1), QAnimationGroup2::Stopped);

    QCOMPARE(group.state(), QAnimationGroup2::Stopped);

    QTest::ignoreMessage(QtWarningMsg, "QAbstractAnimation2::pause: Cannot pause a stopped animation");
    group.pause();

    QCOMPARE(groupStateChangedSpy.count(), 2);
    QCOMPARE(group.state(), QAnimationGroup2::Stopped);

    group.start();

    QCOMPARE(groupStateChangedSpy.states.at(2),
             QAnimationGroup2::Running);
    QCOMPARE(groupStateChangedSpy.states.at(3),
             QAnimationGroup2::Stopped);

    QCOMPARE(group.state(), QAnimationGroup2::Stopped);

    group.stop();

    QCOMPARE(groupStateChangedSpy.count(), 4);
    QCOMPARE(group.state(), QAnimationGroup2::Stopped);
}

void tst_QAnimationGroup2::setCurrentTime()
{
    // was originally sequence operating on same object/property
    QSequentialAnimationGroup2 *sequence = new QSequentialAnimationGroup2();
    QAbstractAnimation2 *a1_s_o1 = new TestableGenericAnimation;
    QAbstractAnimation2 *a2_s_o1 = new TestableGenericAnimation;
    QAbstractAnimation2 *a3_s_o1 = new TestableGenericAnimation;
    a2_s_o1->setLoopCount(3);
    sequence->appendAnimation(a1_s_o1);
    sequence->appendAnimation(a2_s_o1);
    sequence->appendAnimation(a3_s_o1);

    // was originally sequence operating on different object/properties
    QAnimationGroup2 *sequence2 = new QSequentialAnimationGroup2();
    QAbstractAnimation2 *a1_s_o2 = new TestableGenericAnimation;
    QAbstractAnimation2 *a1_s_o3 = new TestableGenericAnimation;
    sequence2->appendAnimation(a1_s_o2);
    sequence2->appendAnimation(a1_s_o3);

    // was originally parallel operating on different object/properties
    QAnimationGroup2 *parallel = new QParallelAnimationGroup2();
    QAbstractAnimation2 *a1_p_o1 = new TestableGenericAnimation;
    QAbstractAnimation2 *a1_p_o2 = new TestableGenericAnimation;
    QAbstractAnimation2 *a1_p_o3 = new TestableGenericAnimation;
    a1_p_o2->setLoopCount(3);
    parallel->appendAnimation(a1_p_o1);
    parallel->appendAnimation(a1_p_o2);
    parallel->appendAnimation(a1_p_o3);

    QAbstractAnimation2 *notTimeDriven = new UncontrolledAnimation;
    QCOMPARE(notTimeDriven->totalDuration(), -1);

    QAbstractAnimation2 *loopsForever = new TestableGenericAnimation;
    loopsForever->setLoopCount(-1);
    QCOMPARE(loopsForever->totalDuration(), -1);

    QParallelAnimationGroup2 group;
    group.appendAnimation(sequence);
    group.appendAnimation(sequence2);
    group.appendAnimation(parallel);
    group.appendAnimation(notTimeDriven);
    group.appendAnimation(loopsForever);

    // Current time = 1
    group.setCurrentTime(1);
    QCOMPARE(group.state(), QAnimationGroup2::Stopped);
    QCOMPARE(sequence->state(), QAnimationGroup2::Stopped);
    QCOMPARE(a1_s_o1->state(), QAnimationGroup2::Stopped);
    QCOMPARE(sequence2->state(), QAnimationGroup2::Stopped);
    QCOMPARE(a1_s_o2->state(), QAnimationGroup2::Stopped);
    QCOMPARE(parallel->state(), QAnimationGroup2::Stopped);
    QCOMPARE(a1_p_o1->state(), QAnimationGroup2::Stopped);
    QCOMPARE(a1_p_o2->state(), QAnimationGroup2::Stopped);
    QCOMPARE(a1_p_o3->state(), QAnimationGroup2::Stopped);
    QCOMPARE(notTimeDriven->state(), QAnimationGroup2::Stopped);
    QCOMPARE(loopsForever->state(), QAnimationGroup2::Stopped);

    QCOMPARE(group.currentLoopTime(), 1);
    QCOMPARE(sequence->currentLoopTime(), 1);
    QCOMPARE(a1_s_o1->currentLoopTime(), 1);
    QCOMPARE(a2_s_o1->currentLoopTime(), 0);
    QCOMPARE(a3_s_o1->currentLoopTime(), 0);
    QCOMPARE(a1_s_o2->currentLoopTime(), 1);
    QCOMPARE(a1_s_o3->currentLoopTime(), 0);
    QCOMPARE(a1_p_o1->currentLoopTime(), 1);
    QCOMPARE(a1_p_o2->currentLoopTime(), 1);
    QCOMPARE(a1_p_o3->currentLoopTime(), 1);
    QCOMPARE(notTimeDriven->currentLoopTime(), 1);
    QCOMPARE(loopsForever->currentLoopTime(), 1);

    // Current time = 250
    group.setCurrentTime(250);
    QCOMPARE(group.currentLoopTime(), 250);
    QCOMPARE(sequence->currentLoopTime(), 250);
    QCOMPARE(a1_s_o1->currentLoopTime(), 250);
    QCOMPARE(a2_s_o1->currentLoopTime(), 0);
    QCOMPARE(a3_s_o1->currentLoopTime(), 0);
    QCOMPARE(a1_s_o2->currentLoopTime(), 250);
    QCOMPARE(a1_s_o3->currentLoopTime(), 0);
    QCOMPARE(a1_p_o1->currentLoopTime(), 250);
    QCOMPARE(a1_p_o2->currentLoopTime(), 0);
    QCOMPARE(a1_p_o2->currentLoop(), 1);
    QCOMPARE(a1_p_o3->currentLoopTime(), 250);
    QCOMPARE(notTimeDriven->currentLoopTime(), 250);
    QCOMPARE(loopsForever->currentLoopTime(), 0);
    QCOMPARE(loopsForever->currentLoop(), 1);
    QCOMPARE(sequence->currentAnimation(), a2_s_o1);

    // Current time = 251
    group.setCurrentTime(251);
    QCOMPARE(group.currentLoopTime(), 251);
    QCOMPARE(sequence->currentLoopTime(), 251);
    QCOMPARE(a1_s_o1->currentLoopTime(), 250);
    QCOMPARE(a2_s_o1->currentLoopTime(), 1);
    QCOMPARE(a2_s_o1->currentLoop(), 0);
    QCOMPARE(a3_s_o1->currentLoopTime(), 0);
    QCOMPARE(sequence2->currentLoopTime(), 251);
    QCOMPARE(a1_s_o2->currentLoopTime(), 250);
    QCOMPARE(a1_s_o3->currentLoopTime(), 1);
    QCOMPARE(a1_p_o1->currentLoopTime(), 250);
    QCOMPARE(a1_p_o2->currentLoopTime(), 1);
    QCOMPARE(a1_p_o2->currentLoop(), 1);
    QCOMPARE(a1_p_o3->currentLoopTime(), 250);
    QCOMPARE(notTimeDriven->currentLoopTime(), 251);
    QCOMPARE(loopsForever->currentLoopTime(), 1);
    QCOMPARE(sequence->currentAnimation(), a2_s_o1);
}

void tst_QAnimationGroup2::addChildTwice()
{
    QAbstractAnimation2 *subGroup;
    QAbstractAnimation2 *subGroup2;
    QAnimationGroup2 *parent = new QSequentialAnimationGroup2();

    subGroup = new QAbstractAnimation2;
    parent->appendAnimation(subGroup);
    parent->appendAnimation(subGroup);
    QVERIFY(parent->firstChild() && !parent->firstChild()->nextSibling());

    parent->clear();

    QVERIFY(!parent->firstChild());

    // adding the same item twice to a group will remove the item from its current position
    // and append it to the end
    subGroup = new QAbstractAnimation2;
    parent->appendAnimation(subGroup);
    subGroup2 = new QAbstractAnimation2;
    parent->appendAnimation(subGroup2);

    QCOMPARE(parent->firstChild(), subGroup);
    QCOMPARE(parent->lastChild(), subGroup2);

    parent->appendAnimation(subGroup);

    QCOMPARE(parent->firstChild(), subGroup2);
    QCOMPARE(parent->lastChild(), subGroup);

    delete parent;
}

QTEST_MAIN(tst_QAnimationGroup2)
#include "tst_qanimationgroup2.moc"
