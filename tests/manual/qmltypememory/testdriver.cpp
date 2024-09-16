// Copyright (C) 2016 Research In Motion.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "testdriver.h"
#include <QCoreApplication>
const int interval = 0;

QObject* singleFactory(QQmlEngine *, QJSEngine*)
{
    QObject* ret = new QObject; //NOTE: No parent - but also shouldn't need to be created
    return ret;
}

TestDriver::TestDriver(const QUrl &componentUrl, int maxIter)
    : QObject(0), e(0), c(0), i(maxIter)
{
    testFile = componentUrl;
    QTimer::singleShot(interval, this, SLOT(setUp()));
}

void TestDriver::setUp()
{
    qmlRegisterType<TestType>("Test", 2, 0, "TestTypeCpp");
    for (int j=2; j<1000; j++) {
        qmlRegisterType<TestType>("Test", j, 0, "TestType");
        qmlRegisterType<TestType>("Test", j, 1, "TestType");
        qmlRegisterType<TestType>("Test", j, 2, "TestType");
        qmlRegisterType<TestType>("Test", j, 3, "TestType");
        qmlRegisterType<TestType>("Test", j, 4, "TestType");
        qmlRegisterType<TestType>("Test", j, 5, "TestType");
        qmlRegisterType<TestType>("Test", j, 6, "TestType");
        qmlRegisterType<TestType>("Test", j, 7, "TestType");
        qmlRegisterType<TestType>("Test", j, 8, "TestType");
        qmlRegisterType<TestType>("Test", j, 9, "TestType");
    }
    qmlRegisterType<TestType>("Test2", 1, 0, "TestType");
    qmlRegisterType<TestType>("Test2", 3, 0, "TestType");
    qmlRegisterType<TestType>("Test3", 9, 0, "TestType");
    qmlRegisterType(QUrl::fromLocalFile(QDir::currentPath() + QLatin1String("TestType.qml")), "Test", 2, 0, "TestType2");
    qmlRegisterUncreatableType<TestType2>("Test", 2, 0, "CannotDoThis", QLatin1String("Just don't"));
    qmlRegisterType<TestType3>();
    qmlRegisterSingletonType<QObject>("Test", 2, 0, "Singlet", singleFactory);

    e = new QQmlEngine(this);
    c = new QQmlComponent(e, testFile);
    QObject* unused = c->create();
    if (!unused) {
        qDebug() << "Creation failure" << c->errorString();
        exit(1);
    }
    unused->setParent(c); //Not testing leaky objects in the tree
    QTimer::singleShot(interval, this, SLOT(tearDown()));
}

void TestDriver::tearDown()
{
    delete c;
    e->collectGarbage();
    delete e;
    qmlClearTypeRegistrations();
    //Note that i < 0 will effectively never terminate. This is deliberate as a prime use is to run indefinitely and watch the memory *not* grow
    i--;
    if (!i)
        qApp->quit();
    else
        QTimer::singleShot(interval, this, SLOT(setUp()));
}
