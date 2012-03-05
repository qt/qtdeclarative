/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
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
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <qtest.h>
#include <QQmlEngine>
#include <QQmlContext>
#include <QNetworkAccessManager>
#include <QPointer>
#include <QDir>
#include <QStandardPaths>
#include <QDebug>
#include <QQmlComponent>
#include <QQmlNetworkAccessManagerFactory>
#include <QQmlExpression>

class tst_qqmlengine : public QObject
{
    Q_OBJECT
public:
    tst_qqmlengine() {}

private slots:
    void rootContext();
    void networkAccessManager();
    void baseUrl();
    void contextForObject();
    void offlineStoragePath();
    void clearComponentCache();
    void outputWarningsToStandardError();
    void objectOwnership();
    void multipleEngines();
};

void tst_qqmlengine::rootContext()
{
    QQmlEngine engine;

    QVERIFY(engine.rootContext());

    QCOMPARE(engine.rootContext()->engine(), &engine);
    QVERIFY(engine.rootContext()->parentContext() == 0);
}

class NetworkAccessManagerFactory : public QQmlNetworkAccessManagerFactory
{
public:
    NetworkAccessManagerFactory() : manager(0) {}

    QNetworkAccessManager *create(QObject *parent) {
        manager = new QNetworkAccessManager(parent);
        return manager;
    }

    QNetworkAccessManager *manager;
};

void tst_qqmlengine::networkAccessManager()
{
    QQmlEngine *engine = new QQmlEngine;

    // Test QQmlEngine created manager
    QPointer<QNetworkAccessManager> manager = engine->networkAccessManager();
    QVERIFY(manager != 0);
    delete engine;

    // Test factory created manager
    engine = new QQmlEngine;
    NetworkAccessManagerFactory factory;
    engine->setNetworkAccessManagerFactory(&factory);
    QVERIFY(engine->networkAccessManagerFactory() == &factory);
    QVERIFY(engine->networkAccessManager() == factory.manager);
    delete engine;
}

void tst_qqmlengine::baseUrl()
{
    QQmlEngine engine;

    QUrl cwd = QUrl::fromLocalFile(QDir::currentPath() + QDir::separator());

    QCOMPARE(engine.baseUrl(), cwd);
    QCOMPARE(engine.rootContext()->resolvedUrl(QUrl("main.qml")), cwd.resolved(QUrl("main.qml")));

    QDir dir = QDir::current();
    dir.cdUp();
    QVERIFY(dir != QDir::current());
    QDir::setCurrent(dir.path());
    QVERIFY(QDir::current() == dir);

    QUrl cwd2 = QUrl::fromLocalFile(QDir::currentPath() + QDir::separator());
    QCOMPARE(engine.baseUrl(), cwd2);
    QCOMPARE(engine.rootContext()->resolvedUrl(QUrl("main.qml")), cwd2.resolved(QUrl("main.qml")));

    engine.setBaseUrl(cwd);
    QCOMPARE(engine.baseUrl(), cwd);
    QCOMPARE(engine.rootContext()->resolvedUrl(QUrl("main.qml")), cwd.resolved(QUrl("main.qml")));
}

void tst_qqmlengine::contextForObject()
{
    QQmlEngine *engine = new QQmlEngine;

    // Test null-object
    QVERIFY(QQmlEngine::contextForObject(0) == 0);

    // Test an object with no context
    QObject object;
    QVERIFY(QQmlEngine::contextForObject(&object) == 0);

    // Test setting null-object
    QQmlEngine::setContextForObject(0, engine->rootContext());

    // Test setting null-context
    QQmlEngine::setContextForObject(&object, 0);

    // Test setting context
    QQmlEngine::setContextForObject(&object, engine->rootContext());
    QVERIFY(QQmlEngine::contextForObject(&object) == engine->rootContext());

    QQmlContext context(engine->rootContext());

    // Try changing context
    QTest::ignoreMessage(QtWarningMsg, "QQmlEngine::setContextForObject(): Object already has a QQmlContext");
    QQmlEngine::setContextForObject(&object, &context);
    QVERIFY(QQmlEngine::contextForObject(&object) == engine->rootContext());

    // Delete context
    delete engine; engine = 0;
    QVERIFY(QQmlEngine::contextForObject(&object) == 0);
}

void tst_qqmlengine::offlineStoragePath()
{
    // Without these set, QDesktopServices::storageLocation returns
    // strings with extra "//" at the end. We set them to ignore this problem.
    qApp->setApplicationName("tst_qqmlengine");
    qApp->setOrganizationName("Nokia");
    qApp->setOrganizationDomain("nokia.com");

    QQmlEngine engine;

    QDir dir(QStandardPaths::writableLocation(QStandardPaths::DataLocation));
    dir.mkpath("QML");
    dir.cd("QML");
    dir.mkpath("OfflineStorage");
    dir.cd("OfflineStorage");

    QCOMPARE(QDir::fromNativeSeparators(engine.offlineStoragePath()), dir.path());

    engine.setOfflineStoragePath(QDir::homePath());
    QCOMPARE(engine.offlineStoragePath(), QDir::homePath());
}

void tst_qqmlengine::clearComponentCache()
{
    QQmlEngine engine;

    // Create original qml file
    {
        QFile file("temp.qml");
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write("import QtQuick 2.0\nQtObject {\nproperty int test: 10\n}\n");
        file.close();
    }

    // Test "test" property
    {
        QQmlComponent component(&engine, "temp.qml");
        QObject *obj = component.create();
        QVERIFY(obj != 0);
        QCOMPARE(obj->property("test").toInt(), 10);
        delete obj;
    }

    // Modify qml file
    {
        QFile file("temp.qml");
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write("import QtQuick 2.0\nQtObject {\nproperty int test: 11\n}\n");
        file.close();
    }

    // Test cache hit
    {
        QQmlComponent component(&engine, "temp.qml");
        QObject *obj = component.create();
        QVERIFY(obj != 0);
        QCOMPARE(obj->property("test").toInt(), 10);
        delete obj;
    }

    // Clear cache
    engine.clearComponentCache();

    // Test cache refresh
    {
        QQmlComponent component(&engine, "temp.qml");
        QObject *obj = component.create();
        QVERIFY(obj != 0);
        QCOMPARE(obj->property("test").toInt(), 11);
        delete obj;
    }
}

static QStringList warnings;
static void msgHandler(QtMsgType, const char *warning)
{
    warnings << QString::fromUtf8(warning);
}

void tst_qqmlengine::outputWarningsToStandardError()
{
    QQmlEngine engine;

    QCOMPARE(engine.outputWarningsToStandardError(), true);

    QQmlComponent c(&engine);
    c.setData("import QtQuick 2.0; QtObject { property int a: undefined }", QUrl());

    QVERIFY(c.isReady() == true);

    warnings.clear();
    QtMsgHandler old = qInstallMsgHandler(msgHandler);

    QObject *o = c.create();

    qInstallMsgHandler(old);

    QVERIFY(o != 0);
    delete o;

    QCOMPARE(warnings.count(), 1);
    QCOMPARE(warnings.at(0), QLatin1String("<Unknown File>:1: Unable to assign [undefined] to int"));
    warnings.clear();


    engine.setOutputWarningsToStandardError(false);
    QCOMPARE(engine.outputWarningsToStandardError(), false);


    old = qInstallMsgHandler(msgHandler);

    o = c.create();

    qInstallMsgHandler(old);

    QVERIFY(o != 0);
    delete o;

    QCOMPARE(warnings.count(), 0);
}

void tst_qqmlengine::objectOwnership()
{
    {
    QCOMPARE(QQmlEngine::objectOwnership(0), QQmlEngine::CppOwnership);
    QQmlEngine::setObjectOwnership(0, QQmlEngine::JavaScriptOwnership);
    QCOMPARE(QQmlEngine::objectOwnership(0), QQmlEngine::CppOwnership);
    }

    {
    QObject o;
    QCOMPARE(QQmlEngine::objectOwnership(&o), QQmlEngine::CppOwnership);
    QQmlEngine::setObjectOwnership(&o, QQmlEngine::CppOwnership);
    QCOMPARE(QQmlEngine::objectOwnership(&o), QQmlEngine::CppOwnership);
    QQmlEngine::setObjectOwnership(&o, QQmlEngine::JavaScriptOwnership);
    QCOMPARE(QQmlEngine::objectOwnership(&o), QQmlEngine::JavaScriptOwnership);
    QQmlEngine::setObjectOwnership(&o, QQmlEngine::CppOwnership);
    QCOMPARE(QQmlEngine::objectOwnership(&o), QQmlEngine::CppOwnership);
    }

    {
    QQmlEngine engine;
    QQmlComponent c(&engine);
    c.setData("import QtQuick 2.0; QtObject { property QtObject object: QtObject {} }", QUrl());

    QObject *o = c.create();
    QVERIFY(o != 0);

    QCOMPARE(QQmlEngine::objectOwnership(o), QQmlEngine::CppOwnership);

    QObject *o2 = qvariant_cast<QObject *>(o->property("object"));
    QCOMPARE(QQmlEngine::objectOwnership(o2), QQmlEngine::JavaScriptOwnership);

    delete o;
    }

}

// Test an object can be accessed by multiple engines
void tst_qqmlengine::multipleEngines()
{
    QObject o;
    o.setObjectName("TestName");

    // Simultaneous engines
    {
        QQmlEngine engine1;
        QQmlEngine engine2;
        engine1.rootContext()->setContextProperty("object", &o);
        engine2.rootContext()->setContextProperty("object", &o);

        QQmlExpression expr1(engine1.rootContext(), 0, QString("object.objectName"));
        QQmlExpression expr2(engine2.rootContext(), 0, QString("object.objectName"));

        QCOMPARE(expr1.evaluate().toString(), QString("TestName"));
        QCOMPARE(expr2.evaluate().toString(), QString("TestName"));
    }

    // Serial engines
    {
        QQmlEngine engine1;
        engine1.rootContext()->setContextProperty("object", &o);
        QQmlExpression expr1(engine1.rootContext(), 0, QString("object.objectName"));
        QCOMPARE(expr1.evaluate().toString(), QString("TestName"));
    }
    {
        QQmlEngine engine1;
        engine1.rootContext()->setContextProperty("object", &o);
        QQmlExpression expr1(engine1.rootContext(), 0, QString("object.objectName"));
        QCOMPARE(expr1.evaluate().toString(), QString("TestName"));
    }
}

QTEST_MAIN(tst_qqmlengine)

#include "tst_qqmlengine.moc"
