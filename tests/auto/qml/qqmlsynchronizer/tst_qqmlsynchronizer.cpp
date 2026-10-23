// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtQuickTestUtils/private/qmlutils_p.h>
#include <QtQmlModels/private/qqmldelegatemodel_p.h>

#include <QtQml/qqmlcomponent.h>
#include <QtQml/qqmlengine.h>

#include <QtTest/qtest.h>

#include <QtCore/qproperty.h>

class tst_qqmlsynchronizer : public QQmlDataTest
{
    Q_OBJECT
public:
    tst_qqmlsynchronizer();

private slots:
    void basics_data();
    void basics();

    void ignored();
    void valueTypes();
    void modelObject();
    void warnings();
    void unnotified();
};

class Unnotified : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(int unnotified READ unnotified CONSTANT)
public:
    Unnotified(QObject *parent = nullptr) : QObject(parent) {}
    int unnotified() const { return 12; }
};

class BindablePoint : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QPointF point BINDABLE bindablePoint READ default WRITE default)
public:
    BindablePoint(QObject *parent = nullptr) : QObject(parent), m_point(QPointF(101, 102)) {}

    QBindable<QPointF> bindablePoint() { return QBindable<QPointF>(&m_point); }

private:
    QProperty<QPointF> m_point;
};

tst_qqmlsynchronizer::tst_qqmlsynchronizer()
    : QQmlDataTest(QT_QMLTEST_DATADIR)
{
    qmlRegisterTypesAndRevisions<BindablePoint>("Test", 1);
    qmlRegisterTypesAndRevisions<Unnotified>("Test", 1);
}

void tst_qqmlsynchronizer::basics_data()
{
    QTest::addColumn<QUrl>("url");
    QTest::addColumn<bool>("initializesOther");
    QTest::addRow("onVsAlias") << testFileUrl("onVsAlias.qml") << true;
    QTest::addRow("onVsTargetProperty") << testFileUrl("onVsTargetProperty.qml") << true;
    QTest::addRow("aliasVsTargetProperty") << testFileUrl("aliasVsTargetProperty.qml") << false;
    QTest::addRow("onVsSourceProperty") << testFileUrl("onVsSourceProperty.qml") << false;
    QTest::addRow("aliasVsSourceProperty") << testFileUrl("aliasVsSourceProperty.qml") << true;
}

void tst_qqmlsynchronizer::basics()
{
    QFETCH(QUrl, url);
    QFETCH(bool, initializesOther);

    QQmlEngine engine;
    QQmlComponent c(&engine, url);
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));
    QScopedPointer<QObject> o(c.create());
    QVERIFY(!o.isNull());

    QObject *other = o->property("other").value<QObject *>();
    QVERIFY(other);

    QCOMPARE(o->objectName(), initializesOther ? "bar" : "foo");
    QCOMPARE(other->objectName(), initializesOther ? "bar" : "foo");

    o->setObjectName("baz");
    QCOMPARE(o->objectName(), "baz");
    QCOMPARE(other->objectName(), "baz");

    other->setObjectName("foo");
    QCOMPARE(o->objectName(), "foo");
    QCOMPARE(other->objectName(), "foo");

    QCOMPARE(o->property("unbindable").toString(), initializesOther ? "a" : "b");
    QCOMPARE(other->property("unbindable").toString(), initializesOther ? "a" : "b");

    o->setProperty("unbindable", QStringLiteral("c"));

    QCOMPARE(o->property("unbindable").toString(), "c");
    QCOMPARE(other->property("unbindable").toString(), "c");

    other->setProperty("unbindable", QStringLiteral("d"));

    QCOMPARE(o->property("unbindable").toString(), "d");
    QCOMPARE(other->property("unbindable").toString(), "d");

    QCOMPARE(o->property("count").toInt(), initializesOther ? 12 : 13);
    QCOMPARE(other->property("count").toDouble(), 13.5);
    int countBounces = initializesOther ? 1 : 0;
    QCOMPARE(o->property("countBounces").toInt(), countBounces);
    QCOMPARE(o->property("countIgnores").toInt(), 0);

    o->setProperty("count", 17);
    ++countBounces;
    QCOMPARE(o->property("count").toInt(), 17);
    QCOMPARE(other->property("count").toDouble(), 13.5);
    QCOMPARE(o->property("countBounces").toInt(), countBounces);
    QCOMPARE(o->property("countIgnores").toInt(), 0);

    other->setProperty("count", 18);
    QCOMPARE(o->property("count").toInt(), 13);
    QCOMPARE(other->property("count").toDouble(), 13.5);
    QCOMPARE(o->property("countBounces").toInt(), countBounces);
    QCOMPARE(o->property("countIgnores").toInt(), 0);
}

void tst_qqmlsynchronizer::ignored()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, testFileUrl("ignored.qml"));
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));

    QScopedPointer<QObject> o(c.create());
    QVERIFY(!o.isNull());

    QObject *other = o->property("other").value<QObject *>();
    QVERIFY(other);

    QCOMPARE(o->property("count").toInt(), 12);
    QCOMPARE(other->property("count").toDouble(), 12);

    QCOMPARE(o->property("countBounces").toInt(), 0);
    QCOMPARE(o->property("countIgnores").toInt(), 0);

    other->setProperty("count", 12.4);
    QCOMPARE(o->property("count").toInt(), 12);
    QCOMPARE(o->property("countBounces").toInt(), 0);
    QCOMPARE(o->property("countIgnores").toInt(), 1);
}

void tst_qqmlsynchronizer::valueTypes()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, testFileUrl("valueTypes.qml"));
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));

    QScopedPointer<QObject> o(c.create());
    QVERIFY(!o.isNull());

    QObject *other = o->property("other").value<QObject *>();
    QVERIFY(other);

    QCOMPARE(o->property("point").value<QPointF>(), QPointF(101, 102));
    QCOMPARE(other->property("point").value<QPointF>(), QPointF(12, 11));
    QCOMPARE(o->objectName(), "11");
    QCOMPARE(other->objectName(), "102");
    QCOMPARE(other->property("count"), 101);
    QCOMPARE(o->property("count"), 12);

    // Since point can only be summarily signaled, this updates the other synchronizer, too.
    o->setProperty("count", 77);
    QCOMPARE(other->property("point").value<QPointF>(), QPointF(77, 11));
    QCOMPARE(o->objectName(), "11");

    other->setProperty("point", QPointF(97, 98));
    QCOMPARE(o->property("count"), 97);
    QCOMPARE(o->objectName(), "98");

    o->setObjectName("54");
    QCOMPARE(other->property("point").value<QPointF>(), QPointF(97, 54));

    o->setProperty("point", QPointF(33.25, 34.8));
    QCOMPARE(other->property("count").toDouble(), 33.25);
    QCOMPARE(other->objectName(), "34.8");
    other->setProperty("count", 55.5);
    QCOMPARE(o->property("point").value<QPointF>(), QPointF(55.5, 34.8));
    QCOMPARE(other->objectName(), "34.8");
    other->setObjectName("77.2");
    QCOMPARE(o->property("point").value<QPointF>(), QPointF(55.5, 77.2));
}

void tst_qqmlsynchronizer::modelObject()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, testFileUrl("modelObject.qml"));
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));

    QScopedPointer<QObject> o(c.create());
    QVERIFY(!o.isNull());

    QQmlDelegateModel *delegateModel = qobject_cast<QQmlDelegateModel *>(o.data());
    QVERIFY(delegateModel);

    QObject *delegate = delegateModel->object(0);
    QVERIFY(delegate);

    QAbstractListModel *model = delegateModel->property("listModel").value<QAbstractListModel *>();
    QVERIFY(model);

    const auto roleNames = model->roleNames();
    int role = -1;
    for (auto it = roleNames.begin(), end = roleNames.end(); it != end; ++it) {
        if (it.value() == "a") {
            role = it.key();
            break;
        }
    }

    const QModelIndex modelIndex = model->index(0);

    QCOMPARE(delegate->objectName(), "foo");
    QCOMPARE(model->data(modelIndex, role), "foo");

    model->setData(modelIndex, "c", role);
    QCOMPARE(model->data(modelIndex, role), "c");
    QCOMPARE(delegate->objectName(), "c");

    delegate->setObjectName("d");
    QCOMPARE(model->data(modelIndex, role), "d");
}

void tst_qqmlsynchronizer::warnings()
{
    QQmlEngine engine;
    const QUrl url = testFileUrl("warnings.qml");
    QQmlComponent c(&engine, url);
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));

    QTest::ignoreMessage(
            QtWarningMsg,
            qPrintable(url.toString()
                + ":31:44: QML Synchronizer: Target object has no property called doesNotExist"));
    QTest::ignoreMessage(
            QtWarningMsg,
            qPrintable(url.toString()
                + ":37:39: QML Synchronizer: Member doThings of target object is a function"));

    QScopedPointer<QObject> o(c.create());
    QVERIFY(!o.isNull());

    QTest::ignoreMessage(
            QtWarningMsg,
            qPrintable(url.toString()
                + ":25:43: QML Synchronizer: Cannot convert from QPointF to QObject*"));

    o->setProperty("point", QPointF(13, 14));
}

void tst_qqmlsynchronizer::unnotified()
{
    QQmlEngine engine;
    const QUrl url = testFileUrl("unnotified.qml");
    QQmlComponent c(&engine, url);
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));
    QScopedPointer<QObject> o(c.create());
    QVERIFY(!o.isNull());
    QCOMPARE(o->property("count").toInt(), 12);

    // Don't crash when disconnecting
    o->setProperty("sourceProperty", QStringLiteral("objectName"));
    o->setObjectName("17");
    QCOMPARE(o->property("count").toInt(), 17);
}

QTEST_MAIN(tst_qqmlsynchronizer)

#include "tst_qqmlsynchronizer.moc"
