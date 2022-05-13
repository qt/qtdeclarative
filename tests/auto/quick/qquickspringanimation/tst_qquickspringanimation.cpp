// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#include <qtest.h>
#include <QtQml/qqmlengine.h>
#include <QtQml/qqmlcomponent.h>
#include <QtQuick/qquickview.h>
#include <private/qquickspringanimation_p.h>
#include <private/qqmlvaluetype_p.h>
#include <QtQuickTestUtils/private/qmlutils_p.h>

class tst_qquickspringanimation : public QQmlDataTest
{
    Q_OBJECT
public:
    tst_qquickspringanimation();

private slots:
    void defaultValues();
    void values();
    void disabled();
    void inTransition();

private:
    QQmlEngine engine;
};

tst_qquickspringanimation::tst_qquickspringanimation()
    : QQmlDataTest(QT_QMLTEST_DATADIR)
{
}

void tst_qquickspringanimation::defaultValues()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, testFileUrl("springanimation1.qml"));
    QQuickSpringAnimation *obj = qobject_cast<QQuickSpringAnimation*>(c.create());

    QVERIFY(obj != nullptr);

    QCOMPARE(obj->to(), 0.);
    QCOMPARE(obj->velocity(), 0.);
    QCOMPARE(obj->spring(), 0.);
    QCOMPARE(obj->damping(), 0.);
    QCOMPARE(obj->epsilon(), 0.01);
    QCOMPARE(obj->modulus(), 0.);
    QCOMPARE(obj->mass(), 1.);
    QCOMPARE(obj->isRunning(), false);

    delete obj;
}

void tst_qquickspringanimation::values()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, testFileUrl("springanimation2.qml"));
    QObject *root = c.create();

    QQuickSpringAnimation *obj = root->findChild<QQuickSpringAnimation*>();

    QVERIFY(obj != nullptr);

    QCOMPARE(obj->to(), 1.44);
    QCOMPARE(obj->velocity(), 0.9);
    QCOMPARE(obj->spring(), 1.0);
    QCOMPARE(obj->damping(), 0.5);
    QCOMPARE(obj->epsilon(), 0.25);
    QCOMPARE(obj->modulus(), 360.0);
    QCOMPARE(obj->mass(), 2.0);
    QCOMPARE(obj->isRunning(), true);

    QTRY_COMPARE(obj->isRunning(), false);

    delete obj;
}

void tst_qquickspringanimation::disabled()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, testFileUrl("springanimation3.qml"));
    QQuickSpringAnimation *obj = qobject_cast<QQuickSpringAnimation*>(c.create());

    QVERIFY(obj != nullptr);

    QCOMPARE(obj->to(), 1.44);
    QCOMPARE(obj->velocity(), 0.9);
    QCOMPARE(obj->spring(), 1.0);
    QCOMPARE(obj->damping(), 0.5);
    QCOMPARE(obj->epsilon(), 0.25);
    QCOMPARE(obj->modulus(), 360.0);
    QCOMPARE(obj->mass(), 2.0);
    QCOMPARE(obj->isRunning(), false);

    delete obj;
}

void tst_qquickspringanimation::inTransition()
{
    QQuickView view(testFileUrl("inTransition.qml"));
    view.show();
    // this used to crash after ~1 sec, once the spring animation was done
    QTest::qWait(2000);
}

QTEST_MAIN(tst_qquickspringanimation)

#include "tst_qquickspringanimation.moc"
