/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/
#include <qtest.h>
#include <QtCore/qdebug.h>
#include <QtCore/qtimer.h>
#include <QtCore/qdir.h>
#include <QtCore/qfileinfo.h>
#include <QtCore/qregularexpression.h>
#include <QtQml/qjsengine.h>

#include <QtQml/qqmlcomponent.h>
#include <QtQml/qqmlengine.h>

#include <private/qquickworkerscript_p.h>
#include <private/qqmlengine_p.h>
#include "../../shared/util.h"

class tst_QQuickWorkerScript : public QQmlDataTest
{
    Q_OBJECT
public:
    tst_QQuickWorkerScript() {}
private slots:
    void source();
    void ready();
    void messaging();
    void messaging_data();
    void messaging_sendQObjectList();
    void messaging_sendJsObject();
    void messaging_sendExternalObject();
    void script_with_pragma();
    void script_included();
    void scriptError_onLoad();
    void scriptError_onCall();
    void script_function();
    void script_var();
    void stressDispose();
    void xmlHttpRequest();

private:
    void waitForEchoMessage(QQuickWorkerScript *worker) {
        QEventLoop loop;
        QVERIFY(connect(worker, SIGNAL(done()), &loop, SLOT(quit())));
        QTimer timer;
        timer.setSingleShot(true);
        connect(&timer, SIGNAL(timeout()), &loop, SLOT(quit()));
        timer.start(10000);
        loop.exec();
        QVERIFY(timer.isActive());
    }

    QQmlEngine m_engine;
};

void tst_QQuickWorkerScript::source()
{
    QQmlComponent component(&m_engine, testFileUrl("worker.qml"));
    QScopedPointer<QQuickWorkerScript>worker(qobject_cast<QQuickWorkerScript*>(component.create()));
    QVERIFY(worker != nullptr);
    const QMetaObject *mo = worker->metaObject();

    QVariant value(100);
    QVERIFY(QMetaObject::invokeMethod(worker.data(), "testSend", Q_ARG(QVariant, value)));
    waitForEchoMessage(worker.data());
    QCOMPARE(mo->property(mo->indexOfProperty("response")).read(worker.data()).value<QVariant>(), value);

    QUrl source = testFileUrl("script_fixed_return.js");
    worker->setSource(source);
    QCOMPARE(worker->source(), source);
    QVERIFY(QMetaObject::invokeMethod(worker.data(), "testSend", Q_ARG(QVariant, value)));
    waitForEchoMessage(worker.data());
    QCOMPARE(mo->property(mo->indexOfProperty("response")).read(worker.data()).value<QVariant>(), QVariant::fromValue(QString("Hello_World")));

    source = testFileUrl("script_module.mjs");
    worker->setSource(source);
    QCOMPARE(worker->source(), source);
    QVERIFY(QMetaObject::invokeMethod(worker.data(), "testSend", Q_ARG(QVariant, value)));
    waitForEchoMessage(worker.data());
    QCOMPARE(mo->property(mo->indexOfProperty("response")).read(worker.data()).value<QVariant>(), QVariant::fromValue(QString("Hello from the module")));

    qApp->processEvents();
}

void tst_QQuickWorkerScript::ready()
{
    QQmlComponent component(&m_engine, testFileUrl("worker.qml"));
    QScopedPointer<QQuickWorkerScript>worker(qobject_cast<QQuickWorkerScript*>(component.create()));
    QVERIFY(worker != nullptr);

    const QMetaObject *mo = worker->metaObject();

    QTRY_VERIFY(worker->ready());

    QVariant readyChangedCalled = mo->property(mo->indexOfProperty("readyChangedCalled")).read(worker.data()).value<QVariant>();

    QVERIFY(!readyChangedCalled.isNull());
    QVERIFY(readyChangedCalled.toBool());
}

void tst_QQuickWorkerScript::messaging()
{
    QFETCH(QVariant, value);

    QQmlComponent component(&m_engine, testFileUrl("worker.qml"));
    QQuickWorkerScript *worker = qobject_cast<QQuickWorkerScript*>(component.create());
    QVERIFY(worker != nullptr);

    QVERIFY(QMetaObject::invokeMethod(worker, "testSend", Q_ARG(QVariant, value)));
    waitForEchoMessage(worker);

    const QMetaObject *mo = worker->metaObject();
    QVariant response = mo->property(mo->indexOfProperty("response")).read(worker).value<QVariant>();
    if (response.userType() == qMetaTypeId<QJSValue>())
        response = response.value<QJSValue>().toVariant();

    if (value.type() == QMetaType::QRegExp && response.type() == QMetaType::QRegularExpression) {
        // toVariant() doesn't know if we want QRegExp or QRegularExpression. It always creates
        // a QRegularExpression from a JavaScript regular expression.
        const QRegularExpression responseRegExp = response.toRegularExpression();
        const QRegExp valueRegExp = value.toRegExp();
        QCOMPARE(responseRegExp.pattern(), valueRegExp.pattern());
        QCOMPARE(bool(responseRegExp.patternOptions() & QRegularExpression::CaseInsensitiveOption),
                 bool(valueRegExp.caseSensitivity() == Qt::CaseInsensitive));
    } else {
        QCOMPARE(response, value);
    }

    qApp->processEvents();
    delete worker;
}

void tst_QQuickWorkerScript::messaging_data()
{
    QTest::addColumn<QVariant>("value");

    QTest::newRow("invalid") << QVariant();
    QTest::newRow("bool") << QVariant::fromValue(true);
    QTest::newRow("int") << QVariant::fromValue(1001);
    QTest::newRow("real") << QVariant::fromValue(10334.375);
    QTest::newRow("string") << QVariant::fromValue(QString("More cheeeese, Gromit!"));
    QTest::newRow("variant list") << QVariant::fromValue((QVariantList() << "a" << "b" << "c"));
    QTest::newRow("date time") << QVariant::fromValue(QDateTime::currentDateTime());
    QTest::newRow("regexp") << QVariant::fromValue(QRegExp("^\\d\\d?$", Qt::CaseInsensitive,
                                                         QRegExp::RegExp2));
    QTest::newRow("regularexpression") << QVariant::fromValue(QRegularExpression(
            "^\\d\\d?$", QRegularExpression::CaseInsensitiveOption));
    QTest::newRow("url") << QVariant::fromValue(QUrl("http://example.com/foo/bar"));
}

void tst_QQuickWorkerScript::messaging_sendQObjectList()
{
    // Not allowed to send QObjects other than QQmlListModelWorkerAgent
    // instances. If objects are sent in a list, they will be sent as 'undefined'
    // js values.

    QQmlComponent component(&m_engine, testFileUrl("worker.qml"));
    QQuickWorkerScript *worker = qobject_cast<QQuickWorkerScript*>(component.create());
    QVERIFY(worker != nullptr);

    QVariantList objects;
    for (int i=0; i<3; i++)
        objects << QVariant::fromValue(new QObject(this));

    QVERIFY(QMetaObject::invokeMethod(worker, "testSend", Q_ARG(QVariant, QVariant::fromValue(objects))));
    waitForEchoMessage(worker);

    const QMetaObject *mo = worker->metaObject();
    QVariantList result = mo->property(mo->indexOfProperty("response")).read(worker).value<QVariantList>();
    QCOMPARE(result, (QVariantList() << QVariant() << QVariant() << QVariant()));

    qApp->processEvents();
    delete worker;
}

void tst_QQuickWorkerScript::messaging_sendJsObject()
{
    QQmlComponent component(&m_engine, testFileUrl("worker.qml"));
    QQuickWorkerScript *worker = qobject_cast<QQuickWorkerScript*>(component.create());
    QVERIFY(worker != nullptr);

    // Properties are in alphabetical order to enable string-based comparison after
    // QVariant roundtrip, since the properties will be stored in a QVariantMap.
    QString jsObject = "{'haste': 1125, 'name': 'zyz', 'spell power': 3101}";

    QVariantMap map;
    map.insert("haste", 1125);
    map.insert("name", "zyz");
    map.insert("spell power", 3101);

    QVERIFY(QMetaObject::invokeMethod(worker, "testSend", Q_ARG(QVariant, QVariant::fromValue(map))));
    waitForEchoMessage(worker);

    QVariant result = QVariant::fromValue(false);
    QVERIFY(QMetaObject::invokeMethod(worker, "compareLiteralResponse", Qt::DirectConnection,
            Q_RETURN_ARG(QVariant, result), Q_ARG(QVariant, jsObject)));
    QVERIFY(result.toBool());

    qApp->processEvents();
    delete worker;
}

void tst_QQuickWorkerScript::messaging_sendExternalObject()
{
    QQmlComponent component(&m_engine, testFileUrl("externalObjectWorker.qml"));
    QObject *obj = component.create();
    QVERIFY(obj);
    QMetaObject::invokeMethod(obj, "testExternalObject");
    QTest::qWait(100); // shouldn't crash.
    delete obj;
}

void tst_QQuickWorkerScript::script_with_pragma()
{
    QVariant value(100);

    QQmlComponent component(&m_engine, testFileUrl("worker_pragma.qml"));
    QQuickWorkerScript *worker = qobject_cast<QQuickWorkerScript*>(component.create());
    QVERIFY(worker != nullptr);

    QVERIFY(QMetaObject::invokeMethod(worker, "testSend", Q_ARG(QVariant, value)));
    waitForEchoMessage(worker);

    const QMetaObject *mo = worker->metaObject();
    QCOMPARE(mo->property(mo->indexOfProperty("response")).read(worker).value<QVariant>(), value);

    qApp->processEvents();
    delete worker;
}

void tst_QQuickWorkerScript::script_included()
{
    QQmlComponent component(&m_engine, testFileUrl("worker_include.qml"));
    QQuickWorkerScript *worker = qobject_cast<QQuickWorkerScript*>(component.create());
    QVERIFY(worker != nullptr);

    QString value("Hello");

    QVERIFY(QMetaObject::invokeMethod(worker, "testSend", Q_ARG(QVariant, value)));
    waitForEchoMessage(worker);

    const QMetaObject *mo = worker->metaObject();
    QCOMPARE(mo->property(mo->indexOfProperty("response")).read(worker).toString(), value + " World");

    qApp->processEvents();
    delete worker;
}

static QString qquickworkerscript_lastWarning;
static void qquickworkerscript_warningsHandler(QtMsgType type, const QMessageLogContext &, const QString &msg)
{
    if (type == QtWarningMsg)
         qquickworkerscript_lastWarning = msg;
}

void tst_QQuickWorkerScript::scriptError_onLoad()
{
    QQmlComponent component(&m_engine, testFileUrl("worker_error_onLoad.qml"));

    QtMessageHandler previousMsgHandler = qInstallMessageHandler(qquickworkerscript_warningsHandler);
    QQuickWorkerScript *worker = qobject_cast<QQuickWorkerScript*>(component.create());
    QVERIFY(worker != nullptr);

    QTRY_COMPARE(qquickworkerscript_lastWarning,
            testFileUrl("script_error_onLoad.js").toString() + QLatin1String(":3:10: SyntaxError: Expected token `,'"));

    qInstallMessageHandler(previousMsgHandler);
    qApp->processEvents();
    delete worker;
}

void tst_QQuickWorkerScript::scriptError_onCall()
{
    QQmlComponent component(&m_engine, testFileUrl("worker_error_onCall.qml"));
    QQuickWorkerScript *worker = qobject_cast<QQuickWorkerScript*>(component.create());
    QVERIFY(worker != nullptr);

    QtMessageHandler previousMsgHandler = qInstallMessageHandler(qquickworkerscript_warningsHandler);
    QVariant value;
    QVERIFY(QMetaObject::invokeMethod(worker, "testSend", Q_ARG(QVariant, value)));

    QTRY_COMPARE(qquickworkerscript_lastWarning,
            testFileUrl("script_error_onCall.js").toString() + QLatin1String(":4: ReferenceError: getData is not defined"));

    qInstallMessageHandler(previousMsgHandler);
    qApp->processEvents();
    delete worker;
}

void tst_QQuickWorkerScript::script_function()
{
    QQmlComponent component(&m_engine, testFileUrl("worker_function.qml"));
    QQuickWorkerScript *worker = qobject_cast<QQuickWorkerScript*>(component.create());
    QVERIFY(worker != nullptr);

    QString value("Hello");

    QVERIFY(QMetaObject::invokeMethod(worker, "testSend", Q_ARG(QVariant, value)));
    waitForEchoMessage(worker);

    const QMetaObject *mo = worker->metaObject();
    QCOMPARE(mo->property(mo->indexOfProperty("response")).read(worker).toString(), value + " World");

    qApp->processEvents();
    delete worker;
}

void tst_QQuickWorkerScript::script_var()
{
    QQmlComponent component(&m_engine, testFileUrl("worker_var.qml"));
    QQuickWorkerScript *worker = qobject_cast<QQuickWorkerScript*>(component.create());
    QVERIFY(worker != nullptr);

    QString value("Hello");

    QVERIFY(QMetaObject::invokeMethod(worker, "testSend", Q_ARG(QVariant, value)));
    waitForEchoMessage(worker);

    const QMetaObject *mo = worker->metaObject();
    QCOMPARE(mo->property(mo->indexOfProperty("response")).read(worker).toString(), value + " World");

    qApp->processEvents();
    delete worker;
}

// Rapidly create and destroy worker scripts to test resources are being disposed
// in the correct isolate
void tst_QQuickWorkerScript::stressDispose()
{
    for (int ii = 0; ii < 100; ++ii) {
        QQmlEngine engine;
        QQmlComponent component(&engine, testFileUrl("stressDispose.qml"));
        QObject *o = component.create();
        QVERIFY(o);
        delete o;
    }
}

void tst_QQuickWorkerScript::xmlHttpRequest()
{
    QQmlComponent component(&m_engine, testFileUrl("xmlHttpRequest.qml"));
    QScopedPointer<QObject> root{component.create()}; // should not crash
    QVERIFY(root);
}

QTEST_MAIN(tst_QQuickWorkerScript)

#include "tst_qquickworkerscript.moc"
