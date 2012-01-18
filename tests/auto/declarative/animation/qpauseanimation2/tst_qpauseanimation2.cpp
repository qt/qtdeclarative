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

#include <QtDeclarative/private/qpauseanimation2_p.h>
#include <QtDeclarative/private/qsequentialanimationgroup2_p.h>
#include <QtDeclarative/private/qparallelanimationgroup2_p.h>

#ifdef Q_OS_WIN
static const char winTimerError[] = "On windows, consistent timing is not working properly due to bad timer resolution";
#endif

class TestablePauseAnimation : public QPauseAnimation2
{
public:
    TestablePauseAnimation()
        : m_updateCurrentTimeCount(0)
    {
    }

    TestablePauseAnimation(int duration)
        : QPauseAnimation2(duration), m_updateCurrentTimeCount(0)
    {
    }

    int m_updateCurrentTimeCount;
protected:
    void updateCurrentTime(int currentTime)
    {
        QPauseAnimation2::updateCurrentTime(currentTime);
        ++m_updateCurrentTimeCount;
    }
};

class TestableGenericAnimation : public QAbstractAnimation2
{
public:
    TestableGenericAnimation(int duration = 250) : m_duration(duration) {}
    int duration() const { return m_duration; }

private:
    int m_duration;
};

class EnableConsistentTiming
{
public:
    EnableConsistentTiming()
    {
        QUnifiedTimer *timer = QUnifiedTimer::instance();
        timer->setConsistentTiming(true);
    }
    ~EnableConsistentTiming()
    {
        QUnifiedTimer *timer = QUnifiedTimer::instance();
        timer->setConsistentTiming(false);
    }
};

class tst_QPauseAnimation2 : public QObject
{
  Q_OBJECT
public Q_SLOTS:
    void initTestCase();

private slots:
    void changeDirectionWhileRunning();
    void noTimerUpdates_data();
    void noTimerUpdates();
    void multiplePauseAnimations();
    void pauseAndPropertyAnimations();
    void pauseResume();
    void sequentialPauseGroup();
    void sequentialGroupWithPause();
    void multipleSequentialGroups();
    void zeroDuration();
};

void tst_QPauseAnimation2::initTestCase()
{
//    qRegisterMetaType<QAbstractAnimation2::State>("QAbstractAnimation2::State");
}

void tst_QPauseAnimation2::changeDirectionWhileRunning()
{
    EnableConsistentTiming enabled;

    TestablePauseAnimation animation;
    animation.setDuration(400);
    animation.start();
    QTest::qWait(100);
    QVERIFY(animation.state() == QAbstractAnimation2::Running);
    animation.setDirection(QAbstractAnimation2::Backward);
    QTest::qWait(animation.totalDuration() + 50);
    QVERIFY(animation.state() == QAbstractAnimation2::Stopped);
}

void tst_QPauseAnimation2::noTimerUpdates_data()
{
    QTest::addColumn<int>("duration");
    QTest::addColumn<int>("loopCount");

    QTest::newRow("0") << 200 << 1;
    QTest::newRow("1") << 160 << 1;
    QTest::newRow("2") << 160 << 2;
    QTest::newRow("3") << 200 << 3;
}

void tst_QPauseAnimation2::noTimerUpdates()
{
    EnableConsistentTiming enabled;

    QFETCH(int, duration);
    QFETCH(int, loopCount);

    TestablePauseAnimation animation;
    animation.setDuration(duration);
    animation.setLoopCount(loopCount);
    animation.start();
    QTest::qWait(animation.totalDuration() + 100);

#ifdef Q_OS_WIN
    if (animation.state() != QAbstractAnimation2::Stopped)
        QEXPECT_FAIL("", winTimerError, Abort);
#endif

    QVERIFY(animation.state() == QAbstractAnimation2::Stopped);
    const int expectedLoopCount = 1 + loopCount;

#ifdef Q_OS_WIN
    if (animation.m_updateCurrentTimeCount != expectedLoopCount)
        QEXPECT_FAIL("", winTimerError, Abort);
#endif
    QCOMPARE(animation.m_updateCurrentTimeCount, expectedLoopCount);
}

void tst_QPauseAnimation2::multiplePauseAnimations()
{
    EnableConsistentTiming enabled;

    TestablePauseAnimation animation;
    animation.setDuration(200);

    TestablePauseAnimation animation2;
    animation2.setDuration(800);

    animation.start();
    animation2.start();
    QTest::qWait(animation.totalDuration() + 100);

#ifdef Q_OS_WIN
    if (animation.state() != QAbstractAnimation2::Stopped)
        QEXPECT_FAIL("", winTimerError, Abort);
#endif
    QVERIFY(animation.state() == QAbstractAnimation2::Stopped);

#ifdef Q_OS_WIN
    if (animation2.state() != QAbstractAnimation2::Running)
        QEXPECT_FAIL("", winTimerError, Abort);
#endif
    QVERIFY(animation2.state() == QAbstractAnimation2::Running);

#ifdef Q_OS_WIN
    if (animation.m_updateCurrentTimeCount != 2)
        QEXPECT_FAIL("", winTimerError, Abort);
#endif
    QCOMPARE(animation.m_updateCurrentTimeCount, 2);

#ifdef Q_OS_WIN
    if (animation2.m_updateCurrentTimeCount != 2)
        QEXPECT_FAIL("", winTimerError, Abort);
#endif
    QCOMPARE(animation2.m_updateCurrentTimeCount, 2);

    QTest::qWait(550);

#ifdef Q_OS_WIN
    if (animation2.state() != QAbstractAnimation2::Stopped)
        QEXPECT_FAIL("", winTimerError, Abort);
#endif
    QVERIFY(animation2.state() == QAbstractAnimation2::Stopped);

#ifdef Q_OS_WIN
    if (animation2.m_updateCurrentTimeCount != 3)
        QEXPECT_FAIL("", winTimerError, Abort);
#endif
    QCOMPARE(animation2.m_updateCurrentTimeCount, 3);
}

void tst_QPauseAnimation2::pauseAndPropertyAnimations()
{
    EnableConsistentTiming enabled;

    TestablePauseAnimation pause;
    pause.setDuration(200);

    TestableGenericAnimation animation;

    pause.start();

    QTest::qWait(100);
    animation.start();

    QVERIFY(animation.state() == QAbstractAnimation2::Running);
    QVERIFY(pause.state() == QAbstractAnimation2::Running);
    QCOMPARE(pause.m_updateCurrentTimeCount, 2);

    QTest::qWait(animation.totalDuration() + 100);

#ifdef Q_OS_WIN
    if (animation.state() != QAbstractAnimation2::Stopped)
        QEXPECT_FAIL("", winTimerError, Abort);
#endif
    QVERIFY(animation.state() == QAbstractAnimation2::Stopped);
    QVERIFY(pause.state() == QAbstractAnimation2::Stopped);
    QVERIFY(pause.m_updateCurrentTimeCount > 3);
}

void tst_QPauseAnimation2::pauseResume()
{
    TestablePauseAnimation animation;
    animation.setDuration(400);
    animation.start();
    QVERIFY(animation.state() == QAbstractAnimation2::Running);
    QTest::qWait(200);
    animation.pause();
    QVERIFY(animation.state() == QAbstractAnimation2::Paused);
    animation.start();
    QTest::qWait(300);
    QVERIFY(animation.state() == QAbstractAnimation2::Stopped);

#ifdef Q_OS_WIN
    if (animation.m_updateCurrentTimeCount != 3)
        QEXPECT_FAIL("", winTimerError, Abort);
#endif
    QCOMPARE(animation.m_updateCurrentTimeCount, 3);
}

void tst_QPauseAnimation2::sequentialPauseGroup()
{
    QSequentialAnimationGroup2 group;

    TestablePauseAnimation animation1(200);
    group.appendAnimation(&animation1);
    TestablePauseAnimation animation2(200);
    group.appendAnimation(&animation2);
    TestablePauseAnimation animation3(200);
    group.appendAnimation(&animation3);

    group.start();
    QCOMPARE(animation1.m_updateCurrentTimeCount, 1);
    QCOMPARE(animation2.m_updateCurrentTimeCount, 0);
    QCOMPARE(animation3.m_updateCurrentTimeCount, 0);

    QVERIFY(group.state() == QAbstractAnimation2::Running);
    QVERIFY(animation1.state() == QAbstractAnimation2::Running);
    QVERIFY(animation2.state() == QAbstractAnimation2::Stopped);
    QVERIFY(animation3.state() == QAbstractAnimation2::Stopped);

    group.setCurrentTime(250);
    QCOMPARE(animation1.m_updateCurrentTimeCount, 2);
    QCOMPARE(animation2.m_updateCurrentTimeCount, 1);
    QCOMPARE(animation3.m_updateCurrentTimeCount, 0);

    QVERIFY(group.state() == QAbstractAnimation2::Running);
    QVERIFY(animation1.state() == QAbstractAnimation2::Stopped);
    QCOMPARE((QAbstractAnimation2*)&animation2, group.currentAnimation());
    QVERIFY(animation2.state() == QAbstractAnimation2::Running);
    QVERIFY(animation3.state() == QAbstractAnimation2::Stopped);

    group.setCurrentTime(500);
    QCOMPARE(animation1.m_updateCurrentTimeCount, 2);
    QCOMPARE(animation2.m_updateCurrentTimeCount, 2);
    QCOMPARE(animation3.m_updateCurrentTimeCount, 1);

    QVERIFY(group.state() == QAbstractAnimation2::Running);
    QVERIFY(animation1.state() == QAbstractAnimation2::Stopped);
    QVERIFY(animation2.state() == QAbstractAnimation2::Stopped);
    QCOMPARE((QAbstractAnimation2*)&animation3, group.currentAnimation());
    QVERIFY(animation3.state() == QAbstractAnimation2::Running);

    group.setCurrentTime(750);

    QVERIFY(group.state() == QAbstractAnimation2::Stopped);
    QVERIFY(animation1.state() == QAbstractAnimation2::Stopped);
    QVERIFY(animation2.state() == QAbstractAnimation2::Stopped);
    QVERIFY(animation3.state() == QAbstractAnimation2::Stopped);

    QCOMPARE(animation1.m_updateCurrentTimeCount, 2);
    QCOMPARE(animation2.m_updateCurrentTimeCount, 2);
    QCOMPARE(animation3.m_updateCurrentTimeCount, 2);
}

void tst_QPauseAnimation2::sequentialGroupWithPause()
{
    QSequentialAnimationGroup2 group;

    TestableGenericAnimation animation;
    group.appendAnimation(&animation);

    TestablePauseAnimation pause;
    pause.setDuration(250);
    group.appendAnimation(&pause);

    group.start();

    QVERIFY(group.state() == QAbstractAnimation2::Running);
    QVERIFY(animation.state() == QAbstractAnimation2::Running);
    QVERIFY(pause.state() == QAbstractAnimation2::Stopped);

    group.setCurrentTime(300);

    QVERIFY(group.state() == QAbstractAnimation2::Running);
    QVERIFY(animation.state() == QAbstractAnimation2::Stopped);
    QCOMPARE((QAbstractAnimation2*)&pause, group.currentAnimation());
    QVERIFY(pause.state() == QAbstractAnimation2::Running);

    group.setCurrentTime(600);

    QVERIFY(group.state() == QAbstractAnimation2::Stopped);
    QVERIFY(animation.state() == QAbstractAnimation2::Stopped);
    QVERIFY(pause.state() == QAbstractAnimation2::Stopped);

    QCOMPARE(pause.m_updateCurrentTimeCount, 2);
}

void tst_QPauseAnimation2::multipleSequentialGroups()
{
    EnableConsistentTiming enabled;

    QParallelAnimationGroup2 group;
    group.setLoopCount(2);

    QSequentialAnimationGroup2 subgroup1;
    group.appendAnimation(&subgroup1);

    TestableGenericAnimation animation(300);
    subgroup1.appendAnimation(&animation);

    TestablePauseAnimation pause(200);
    subgroup1.appendAnimation(&pause);

    QSequentialAnimationGroup2 subgroup2;
    group.appendAnimation(&subgroup2);

    TestableGenericAnimation animation2(200);
    subgroup2.appendAnimation(&animation2);

    TestablePauseAnimation pause2(250);
    subgroup2.appendAnimation(&pause2);

    QSequentialAnimationGroup2 subgroup3;
    group.appendAnimation(&subgroup3);

    TestablePauseAnimation pause3(400);
    subgroup3.appendAnimation(&pause3);

    TestableGenericAnimation animation3(200);
    subgroup3.appendAnimation(&animation3);

    QSequentialAnimationGroup2 subgroup4;
    group.appendAnimation(&subgroup4);

    TestablePauseAnimation pause4(310);
    subgroup4.appendAnimation(&pause4);

    TestablePauseAnimation pause5(60);
    subgroup4.appendAnimation(&pause5);

    group.start();

    QVERIFY(group.state() == QAbstractAnimation2::Running);
    QVERIFY(subgroup1.state() == QAbstractAnimation2::Running);
    QVERIFY(subgroup2.state() == QAbstractAnimation2::Running);
    QVERIFY(subgroup3.state() == QAbstractAnimation2::Running);
    QVERIFY(subgroup4.state() == QAbstractAnimation2::Running);

    // This is a pretty long animation so it tends to get rather out of sync
    // when using the consistent timer, so run for an extra half second for good
    // measure...
    QTest::qWait(group.totalDuration() + 500);

#ifdef Q_OS_WIN
    if (group.state() != QAbstractAnimation2::Stopped)
        QEXPECT_FAIL("", winTimerError, Abort);
#endif
    QVERIFY(group.state() == QAbstractAnimation2::Stopped);

#ifdef Q_OS_WIN
    if (subgroup1.state() != QAbstractAnimation2::Stopped)
        QEXPECT_FAIL("", winTimerError, Abort);
#endif
    QVERIFY(subgroup1.state() == QAbstractAnimation2::Stopped);

#ifdef Q_OS_WIN
    if (subgroup2.state() != QAbstractAnimation2::Stopped)
        QEXPECT_FAIL("", winTimerError, Abort);
#endif
    QVERIFY(subgroup2.state() == QAbstractAnimation2::Stopped);

#ifdef Q_OS_WIN
    if (subgroup3.state() != QAbstractAnimation2::Stopped)
        QEXPECT_FAIL("", winTimerError, Abort);
#endif
    QVERIFY(subgroup3.state() == QAbstractAnimation2::Stopped);

#ifdef Q_OS_WIN
    if (subgroup4.state() != QAbstractAnimation2::Stopped)
        QEXPECT_FAIL("", winTimerError, Abort);
#endif
    QVERIFY(subgroup4.state() == QAbstractAnimation2::Stopped);

#ifdef Q_OS_WIN
    if (pause5.m_updateCurrentTimeCount != 4)
        QEXPECT_FAIL("", winTimerError, Abort);
#endif
    QCOMPARE(pause5.m_updateCurrentTimeCount, 4);
}

void tst_QPauseAnimation2::zeroDuration()
{
    TestablePauseAnimation animation;
    animation.setDuration(0);
    animation.start();
    QTest::qWait(animation.totalDuration() + 100);
    QVERIFY(animation.state() == QAbstractAnimation2::Stopped);
    QCOMPARE(animation.m_updateCurrentTimeCount, 1);
}

QTEST_MAIN(tst_QPauseAnimation2)
#include "tst_qpauseanimation2.moc"
