// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QtTest/QtTest>
#include <QtQuickTest/quicktest.h>

#include <QtQuick/qquickview.h>
#include <QtQuick/private/qquickanimator_p.h>
#include <QtQuick/private/qquicktransition_p.h>
#include <QtQuick/private/qquickrepeater_p.h>
#include <QtQuick/private/qquickanimatorcontroller_p.h>
#include <QtQuick/private/qquickanimation_p_p.h>
#include <QtQuick/private/qquickitem_p.h>

#include <QtQuickTestUtils/private/qmlutils_p.h>
#include <QtQuickTestUtils/private/viewtestutils_p.h>
#include <QQmlComponent>

using namespace QQuickViewTestUtils;

QT_BEGIN_NAMESPACE

class tst_Animators: public QQmlDataTest
{
    Q_OBJECT

public:
    tst_Animators();

private slots:
    void testMultiWinAnimator_data();
    void testMultiWinAnimator();
    void testTransitions();
    void testTransitionsWithImplicitFrom();
};

tst_Animators::tst_Animators()
    : QQmlDataTest(QT_QMLTEST_DATADIR)
{
}

void tst_Animators::testMultiWinAnimator_data()
{
    QTest::addColumn<int>("count");

    QTest::newRow("1") << 1;
    QTest::newRow("10") << 10;
}

void tst_Animators::testMultiWinAnimator()
{
    QFETCH(int, count);

    QQmlEngine engine;
    QQmlComponent component(&engine, testFileUrl("windowWithAnimator.qml"));

    QList<QQuickWindow *> windows;
    for (int i = 0; i < count; ++i) {
        QQuickWindow *win = qobject_cast<QQuickWindow *>(component.create());
        windows << win;

        // As the windows are all the same size, if they are positioned at the
        // same place only the top-most one will strictly be "exposed" and rendering
        // for all the others will be disabled. Move the windows a little bit
        // to ensure they are exposed and actually rendering.
        if (i > 0) {
            QPoint pos = win->position();
            if (pos == windows.first()->position()) {
                pos += QPoint(10 * i, 10 * i);
                win->setPosition(pos);
            }
        }
    }

    // let all animations run their course...
    while (true) {
        QTest::qWait(200);
        bool allDone = true;
        for (int i=0; i<count; ++i) {
            QQuickWindow *win = windows.at(i);
            allDone = win->isExposed() && win->property("animationDone").toBool();
        }

        if (allDone) {
            for (int i=0; i<count; ++i) {
                QQuickWindow *win = windows.at(i);
                delete win;
            }
            break;
        }
    }
    QVERIFY(true);
}

void tst_Animators::testTransitions()
{
    QScopedPointer<QQuickView> view(createView());
    view->setSource(testFileUrl("positionerWithAnimator.qml"));
    view->show();
    QVERIFY(QTest::qWaitForWindowExposed(view.data()));
    QQuickItem *root = view->rootObject();
    QVERIFY(root);

    QQuickRepeater *repeater = root->property("repeater").value<QQuickRepeater *>();
    QVERIFY(repeater);

    QQuickItem *child = repeater->itemAt(0);
    QVERIFY(child);
    QCOMPARE(child->scale(), qreal(0));

    QQuickTransition *transition = root->property("transition").value<QQuickTransition *>();
    QVERIFY(transition);

    QTRY_VERIFY(transition->running());
    QTRY_VERIFY(!transition->running());
    QCOMPARE(child->scale(), qreal(1));
}

void tst_Animators::testTransitionsWithImplicitFrom()
{
    QScopedPointer<QQuickView> view(createView());
    view->setSource(testFileUrl("animatorImplicitFrom.qml"));
    view->show();
    QVERIFY(QTest::qWaitForWindowExposed(view.data()));
    QQuickItem *root = view->rootObject();
    QVERIFY(root);

    QQuickItem *rectangle = root->property("rectangle").value<QQuickItem *>();
    QVERIFY(rectangle);

    // the controller has access to actual AnimatorJob instances
    QQuickAnimatorController *controller = QQuickWindowPrivate::get(view.data())->animationController.data();
    QVERIFY(controller);

    // verify initial state
    QCOMPARE(rectangle->x(), 0);
    QCOMPARE(rectangle->state(), "left");
    QVERIFY(controller->m_runningAnimators.isEmpty());

    // transition to the "right" state
    rectangle->setState("right");
    QTRY_VERIFY(!controller->m_runningAnimators.isEmpty());
    auto r_job = *controller->m_runningAnimators.constBegin();
    QVERIFY(r_job);
    QCOMPARE(r_job->from(), 0);
    QCOMPARE(r_job->to(), 100);
    QTRY_VERIFY(controller->m_runningAnimators.isEmpty());

    // verify state after first transition
    QTRY_COMPARE(rectangle->x(), 100); // the render thread has to sync first
    QCOMPARE(rectangle->state(), "right");
    QVERIFY(controller->m_runningAnimators.isEmpty());

    // transition back to the "left" state
    rectangle->setState("left");
    QTRY_VERIFY(!controller->m_runningAnimators.isEmpty());
    auto l_job = *controller->m_runningAnimators.constBegin();
    QVERIFY(l_job);
    QCOMPARE(l_job->from(), 100); // this was not working in older Qt versions
    QCOMPARE(l_job->to(), 0);
    QTRY_VERIFY(controller->m_runningAnimators.isEmpty());

    // verify the final state
    QTRY_COMPARE(rectangle->x(), 0); // the render thread has to sync first
    QCOMPARE(rectangle->state(), "left");
    QVERIFY(controller->m_runningAnimators.isEmpty());
}

QT_END_NAMESPACE

QTEST_MAIN(tst_Animators)

#include "tst_qquickanimators.moc"
