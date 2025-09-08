// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtQml/qqmlengine.h>
#include <QtTest/qtest.h>

#include <private/qabstractanimationjob_p.h>
#include <private/qanimationgroupjob_p.h>
#include <private/qcoreapplication_p.h>

class tst_QAbstractAnimationJob : public QObject
{
  Q_OBJECT
private slots:
    void construction();
    void destruction();
    void currentLoop();
    void currentLoopTime();
    void currentTime();
    void direction();
    void group();
    void loopCount();
    void state();
    void totalDuration();
    void avoidJumpAtStart();
    void avoidJumpAtStartWithStop();
    void avoidJumpAtStartWithRunning();
    void deleteAnimationTimer();
};

class TestableQAbstractAnimation : public QAbstractAnimationJob
{
public:
    TestableQAbstractAnimation() {}
    ~TestableQAbstractAnimation() override {};

    int duration() const override { return m_duration; }
    void updateCurrentTime(int) override {}

    void setDuration(int duration) { m_duration = duration; }
private:
    int m_duration = 10;
};

class DummyQAnimationGroup : public QAnimationGroupJob
{
public:
    int duration() const override { return 10; }
    void updateCurrentTime(int) override {}
};

void tst_QAbstractAnimationJob::construction()
{
    TestableQAbstractAnimation anim;
}

void tst_QAbstractAnimationJob::destruction()
{
    std::unique_ptr<TestableQAbstractAnimation> anim = std::make_unique<TestableQAbstractAnimation>();
}

void tst_QAbstractAnimationJob::currentLoop()
{
    TestableQAbstractAnimation anim;
    QCOMPARE(anim.currentLoop(), 0);
}

void tst_QAbstractAnimationJob::currentLoopTime()
{
    TestableQAbstractAnimation anim;
    QCOMPARE(anim.currentLoopTime(), 0);
}

void tst_QAbstractAnimationJob::currentTime()
{
    TestableQAbstractAnimation anim;
    QCOMPARE(anim.currentTime(), 0);
    anim.setCurrentTime(10);
    QCOMPARE(anim.currentTime(), 10);
}

void tst_QAbstractAnimationJob::direction()
{
    TestableQAbstractAnimation anim;
    QCOMPARE(anim.direction(), QAbstractAnimationJob::Forward);
    anim.setDirection(QAbstractAnimationJob::Backward);
    QCOMPARE(anim.direction(), QAbstractAnimationJob::Backward);
    anim.setDirection(QAbstractAnimationJob::Forward);
    QCOMPARE(anim.direction(), QAbstractAnimationJob::Forward);
}

void tst_QAbstractAnimationJob::group()
{
    TestableQAbstractAnimation *anim = new TestableQAbstractAnimation;
    DummyQAnimationGroup group;
    group.appendAnimation(anim);
    QCOMPARE(anim->group(), &group);
}

void tst_QAbstractAnimationJob::loopCount()
{
    TestableQAbstractAnimation anim;
    QCOMPARE(anim.loopCount(), 1);
    anim.setLoopCount(10);
    QCOMPARE(anim.loopCount(), 10);
}

void tst_QAbstractAnimationJob::state()
{
    TestableQAbstractAnimation anim;
    QCOMPARE(anim.state(), QAbstractAnimationJob::Stopped);
}

void tst_QAbstractAnimationJob::totalDuration()
{
    TestableQAbstractAnimation anim;
    QCOMPARE(anim.duration(), 10);
    anim.setLoopCount(5);
    QCOMPARE(anim.totalDuration(), 50);
}

void tst_QAbstractAnimationJob::avoidJumpAtStart()
{
    TestableQAbstractAnimation anim;
    anim.setDuration(1000);

    /*
        the timer shouldn't actually start until we hit the event loop,
        so the sleep should have no effect
    */
    anim.start();
    QTest::qSleep(300);
    QCoreApplication::processEvents();
    QVERIFY(anim.currentTime() < 50);
}

void tst_QAbstractAnimationJob::avoidJumpAtStartWithStop()
{
    TestableQAbstractAnimation anim;
    anim.setDuration(1000);

    TestableQAbstractAnimation anim2;
    anim2.setDuration(1000);

    TestableQAbstractAnimation anim3;
    anim3.setDuration(1000);

    anim.start();
    QTest::qWait(300);
    anim.stop();

    /*
        same test as avoidJumpAtStart, but after there is a
        running animation that is stopped
    */
    anim2.start();
    QTest::qSleep(300);
    anim3.start();
    QCoreApplication::processEvents();
    QVERIFY(anim2.currentTime() < 50);
    QVERIFY(anim3.currentTime() < 50);
}

void tst_QAbstractAnimationJob::avoidJumpAtStartWithRunning()
{
    TestableQAbstractAnimation anim;
    anim.setDuration(2000);

    TestableQAbstractAnimation anim2;
    anim2.setDuration(1000);

    TestableQAbstractAnimation anim3;
    anim3.setDuration(1000);

    anim.start();
    QTest::qWait(300);  //make sure timer has started

    /*
        same test as avoidJumpAtStart, but with an
        existing running animation
    */
    anim2.start();
    QTest::qSleep(300); //force large delta for next tick
    anim3.start();
    QCoreApplication::processEvents();
    QVERIFY(anim2.currentTime() < 50);
    QVERIFY(anim3.currentTime() < 50);
}

void tst_QAbstractAnimationJob::deleteAnimationTimer()
{
    for (int i = 0; i < 10; ++i) {
        {
            TestableQAbstractAnimation mainAnimation;
            mainAnimation.setDuration(100);
            mainAnimation.start();
            QTRY_VERIFY(mainAnimation.isRunning());
            QTRY_VERIFY(mainAnimation.currentTime() > 50);
        }

        // We can tear down the thread data that holds the animation timer. This does not cause
        // the QUnifiedTimer to hold a dangling pointer next timer around.
        static_cast<QCoreApplicationPrivate *>(QObjectPrivate::get(QCoreApplication::instance()))
                ->cleanupThreadData();
    }
}


QTEST_MAIN(tst_QAbstractAnimationJob)

#include "tst_qabstractanimationjob.moc"
