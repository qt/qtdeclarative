// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
#include <qtest.h>
#include <QtQml/qqmlengine.h>
#include <QtQml/qqmlcomponent.h>
#include <QtQuick/qquickview.h>
#include <private/qquickflipable_p.h>
#include <private/qqmlvaluetype_p.h>
#include <QFontMetrics>
#include <QtQuick/private/qquickrectangle_p.h>
#include <math.h>
#include <QtQuickTestUtils/private/qmlutils_p.h>

class tst_qquickflipable : public QQmlDataTest
{
    Q_OBJECT
public:
    tst_qquickflipable();

private slots:
    void create();
    void checkFrontAndBack();
    void setFrontAndBack();
    void flipFlipable();

    // below here task issues
    void QTBUG_9161_crash();
    void QTBUG_8474_qgv_abort();

    void flipRotationAngle_data();
    void flipRotationAngle();

private:
    QQmlEngine engine;
};

tst_qquickflipable::tst_qquickflipable()
    : QQmlDataTest(QT_QMLTEST_DATADIR)
{
}

void tst_qquickflipable::create()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, testFileUrl("test-flipable.qml"));
    QQuickFlipable *obj = qobject_cast<QQuickFlipable*>(c.create());

    QVERIFY(obj != nullptr);
    delete obj;
}

void tst_qquickflipable::checkFrontAndBack()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, testFileUrl("test-flipable.qml"));
    QQuickFlipable *obj = qobject_cast<QQuickFlipable*>(c.create());

    QVERIFY(obj != nullptr);
    QVERIFY(obj->front() != nullptr);
    QVERIFY(obj->back() != nullptr);
    delete obj;
}

void tst_qquickflipable::setFrontAndBack()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, testFileUrl("test-flipable.qml"));
    QQuickFlipable *obj = qobject_cast<QQuickFlipable*>(c.create());

    QVERIFY(obj != nullptr);
    QVERIFY(obj->front() != nullptr);
    QVERIFY(obj->back() != nullptr);

    QString message = c.url().toString() + ":3:1: QML Flipable: front is a write-once property";
    QTest::ignoreMessage(QtWarningMsg, qPrintable(message));
    obj->setFront(new QQuickRectangle());

    message = c.url().toString() + ":3:1: QML Flipable: back is a write-once property";
    QTest::ignoreMessage(QtWarningMsg, qPrintable(message));
    obj->setBack(new QQuickRectangle());
    delete obj;
}

void tst_qquickflipable::flipFlipable()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, testFileUrl("flip-flipable.qml"));
    QQuickFlipable *obj = qobject_cast<QQuickFlipable*>(c.create());
    QVERIFY(obj != nullptr);
    QCOMPARE(obj->side(), QQuickFlipable::Front);
    obj->setProperty("flipped", QVariant(true));
    QTRY_COMPARE(obj->side(), QQuickFlipable::Back);
    QTRY_COMPARE(obj->side(), QQuickFlipable::Front);
    QTRY_COMPARE(obj->side(), QQuickFlipable::Back);
    delete obj;
}

void tst_qquickflipable::QTBUG_9161_crash()
{
    QQuickView *window = new QQuickView;
    window->setSource(testFileUrl("crash.qml"));
    QQuickItem *root = window->rootObject();
    QVERIFY(root != nullptr);
    window->show();
    delete window;
}

void tst_qquickflipable::QTBUG_8474_qgv_abort()
{
    QQuickView *window = new QQuickView;
    window->setSource(testFileUrl("flipable-abort.qml"));
    QQuickItem *root = window->rootObject();
    QVERIFY(root != nullptr);
    window->show();
    delete window;
}

void tst_qquickflipable::flipRotationAngle_data()
{
    QTest::addColumn<int>("angle");
    QTest::addColumn<QQuickFlipable::Side>("side");

    QTest::newRow("89") << 89 << QQuickFlipable::Front;
    QTest::newRow("91") << 91 << QQuickFlipable::Back;
    QTest::newRow("-89") << -89 << QQuickFlipable::Front;
    QTest::newRow("-91") << -91 << QQuickFlipable::Back;
}

void tst_qquickflipable::flipRotationAngle() // QTBUG-75954
{
    QFETCH(int, angle);
    QFETCH(QQuickFlipable::Side, side);

    QQmlEngine engine;
    QQmlComponent c(&engine, testFileUrl("flip-y-axis-flipable.qml"));
    QQuickFlipable *obj = qobject_cast<QQuickFlipable*>(c.create());
    QVERIFY(obj != nullptr);
    obj->setProperty("angle", angle);
    QCOMPARE(obj->side(), side);
    delete obj;
}

QTEST_MAIN(tst_qquickflipable)

#include "tst_qquickflipable.moc"
