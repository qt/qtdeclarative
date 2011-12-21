/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
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
#include "testtypes.h"

#include <QUrl>
#include <QDir>
#include <QDebug>
#include <qtest.h>
#include <QPointer>
#include <QFileInfo>
#include <QDeclarativeEngine>
#include <QDeclarativeContext>
#include <QDeclarativeProperty>
#include <QDeclarativeComponent>
#include <QDeclarativeIncubator>
#include "../../shared/util.h"

class tst_qdeclarativeincubator : public QDeclarativeDataTest
{
    Q_OBJECT
public:
    tst_qdeclarativeincubator() {}

private slots:
    void initTestCase();

    void incubationMode();
    void objectDeleted();
    void clear();
    void noIncubationController();
    void forceCompletion();
    void setInitialState();
    void clearDuringCompletion();
    void objectDeletionAfterInit();
    void recursiveClear();
    void statusChanged();
    void asynchronousIfNested();
    void nestedComponent();
    void chainedAsynchronousIfNested();
    void chainedAsynchronousIfNestedOnCompleted();
    void selfDelete();
    void contextDelete();

private:
    QDeclarativeIncubationController controller;
    QDeclarativeEngine engine;
};

#define VERIFY_ERRORS(component, errorfile) \
    if (!errorfile) { \
        if (qgetenv("DEBUG") != "" && !component.errors().isEmpty()) \
            qWarning() << "Unexpected Errors:" << component.errors(); \
        QVERIFY(!component.isError()); \
        QVERIFY(component.errors().isEmpty()); \
    } else { \
        QFile file(QDeclarativeDataTest::instance()->testFile(errorfile)); \
        QVERIFY(file.open(QIODevice::ReadOnly | QIODevice::Text)); \
        QByteArray data = file.readAll(); \
        file.close(); \
        QList<QByteArray> expected = data.split('\n'); \
        expected.removeAll(QByteArray("")); \
        QList<QDeclarativeError> errors = component.errors(); \
        QList<QByteArray> actual; \
        for (int ii = 0; ii < errors.count(); ++ii) { \
            const QDeclarativeError &error = errors.at(ii); \
            QByteArray errorStr = QByteArray::number(error.line()) + ":" +  \
                                  QByteArray::number(error.column()) + ":" + \
                                  error.description().toUtf8(); \
            actual << errorStr; \
        } \
        if (qgetenv("DEBUG") != "" && expected != actual) \
            qWarning() << "Expected:" << expected << "Actual:" << actual;  \
        QCOMPARE(expected, actual); \
    }

void tst_qdeclarativeincubator::initTestCase()
{
    QDeclarativeDataTest::initTestCase();
    registerTypes();
    engine.setIncubationController(&controller);
}

void tst_qdeclarativeincubator::incubationMode()
{
    {
    QDeclarativeIncubator incubator;
    QCOMPARE(incubator.incubationMode(), QDeclarativeIncubator::Asynchronous);
    }
    {
    QDeclarativeIncubator incubator(QDeclarativeIncubator::Asynchronous);
    QCOMPARE(incubator.incubationMode(), QDeclarativeIncubator::Asynchronous);
    }
    {
    QDeclarativeIncubator incubator(QDeclarativeIncubator::Synchronous);
    QCOMPARE(incubator.incubationMode(), QDeclarativeIncubator::Synchronous);
    }
    {
    QDeclarativeIncubator incubator(QDeclarativeIncubator::AsynchronousIfNested);
    QCOMPARE(incubator.incubationMode(), QDeclarativeIncubator::AsynchronousIfNested);
    }
}

void tst_qdeclarativeincubator::objectDeleted()
{
    SelfRegisteringType::clearMe();

    QDeclarativeComponent component(&engine, testFileUrl("objectDeleted.qml"));
    QVERIFY(component.isReady());

    QDeclarativeIncubator incubator;
    component.create(incubator);

    QCOMPARE(incubator.status(), QDeclarativeIncubator::Loading);
    QVERIFY(SelfRegisteringType::me() == 0);

    while (SelfRegisteringType::me() == 0 && incubator.isLoading()) {
        bool b = false;
        controller.incubateWhile(&b);
    }

    QVERIFY(SelfRegisteringType::me() != 0);
    QVERIFY(incubator.isLoading());

    delete SelfRegisteringType::me();

    {
    bool b = true;
    controller.incubateWhile(&b);
    }

    QVERIFY(incubator.isError());
    VERIFY_ERRORS(incubator, "objectDeleted.errors.txt");
    QVERIFY(incubator.object() == 0);
}

void tst_qdeclarativeincubator::clear()
{
    SelfRegisteringType::clearMe();

    QDeclarativeComponent component(&engine, testFileUrl("clear.qml"));
    QVERIFY(component.isReady());

    // Clear in null state
    {
    QDeclarativeIncubator incubator;
    QVERIFY(incubator.isNull());
    incubator.clear(); // no effect
    QVERIFY(incubator.isNull());
    }

    // Clear in loading state
    {
    QDeclarativeIncubator incubator;
    component.create(incubator);
    QVERIFY(incubator.isLoading());
    incubator.clear();
    QVERIFY(incubator.isNull());
    }

    // Clear mid load
    {
    QDeclarativeIncubator incubator;
    component.create(incubator);

    while (SelfRegisteringType::me() == 0 && incubator.isLoading()) {
        bool b = false;
        controller.incubateWhile(&b);
    }

    QVERIFY(incubator.isLoading());
    QVERIFY(SelfRegisteringType::me() != 0);
    QPointer<SelfRegisteringType> srt = SelfRegisteringType::me();

    incubator.clear();
    QVERIFY(incubator.isNull());
    QVERIFY(srt.isNull());
    }

    // Clear in ready state
    {
    QDeclarativeIncubator incubator;
    component.create(incubator);

    {
        bool b = true;
        controller.incubateWhile(&b);
    }

    QVERIFY(incubator.isReady());
    QVERIFY(incubator.object() != 0);
    QPointer<QObject> obj = incubator.object();

    incubator.clear();
    QVERIFY(incubator.isNull());
    QVERIFY(incubator.object() == 0);
    QVERIFY(!obj.isNull());

    delete obj;
    QVERIFY(obj.isNull());
    }
}

void tst_qdeclarativeincubator::noIncubationController()
{
    // All incubators should behave synchronously when there is no controller

    QDeclarativeEngine engine;
    QDeclarativeComponent component(&engine, testFileUrl("noIncubationController.qml"));

    QVERIFY(component.isReady());

    {
    QDeclarativeIncubator incubator(QDeclarativeIncubator::Asynchronous);
    component.create(incubator);
    QVERIFY(incubator.isReady());
    QVERIFY(incubator.object());
    QCOMPARE(incubator.object()->property("testValue").toInt(), 1913);
    delete incubator.object();
    }

    {
    QDeclarativeIncubator incubator(QDeclarativeIncubator::AsynchronousIfNested);
    component.create(incubator);
    QVERIFY(incubator.isReady());
    QVERIFY(incubator.object());
    QCOMPARE(incubator.object()->property("testValue").toInt(), 1913);
    delete incubator.object();
    }

    {
    QDeclarativeIncubator incubator(QDeclarativeIncubator::Synchronous);
    component.create(incubator);
    QVERIFY(incubator.isReady());
    QVERIFY(incubator.object());
    QCOMPARE(incubator.object()->property("testValue").toInt(), 1913);
    delete incubator.object();
    }
}

void tst_qdeclarativeincubator::forceCompletion()
{
    QDeclarativeComponent component(&engine, testFileUrl("forceCompletion.qml"));
    QVERIFY(component.isReady());

    {
    // forceCompletion on a null incubator does nothing
    QDeclarativeIncubator incubator;
    QVERIFY(incubator.isNull());
    incubator.forceCompletion();
    QVERIFY(incubator.isNull());
    }

    {
    // forceCompletion immediately after creating an asynchronous object completes it
    QDeclarativeIncubator incubator;
    QVERIFY(incubator.isNull());
    component.create(incubator);
    QVERIFY(incubator.isLoading());

    incubator.forceCompletion();

    QVERIFY(incubator.isReady());
    QVERIFY(incubator.object() != 0);
    QCOMPARE(incubator.object()->property("testValue").toInt(), 3499);

    delete incubator.object();
    }

    {
    // forceCompletion during creation completes it
    SelfRegisteringType::clearMe();

    QDeclarativeIncubator incubator;
    QVERIFY(incubator.isNull());
    component.create(incubator);
    QVERIFY(incubator.isLoading());

    while (SelfRegisteringType::me() == 0 && incubator.isLoading()) {
        bool b = false;
        controller.incubateWhile(&b);
    }

    QVERIFY(SelfRegisteringType::me() != 0);
    QVERIFY(incubator.isLoading());

    incubator.forceCompletion();

    QVERIFY(incubator.isReady());
    QVERIFY(incubator.object() != 0);
    QCOMPARE(incubator.object()->property("testValue").toInt(), 3499);

    delete incubator.object();
    }

    {
    // forceCompletion on a ready incubator has no effect
    QDeclarativeIncubator incubator;
    QVERIFY(incubator.isNull());
    component.create(incubator);
    QVERIFY(incubator.isLoading());

    incubator.forceCompletion();

    QVERIFY(incubator.isReady());
    QVERIFY(incubator.object() != 0);
    QCOMPARE(incubator.object()->property("testValue").toInt(), 3499);

    incubator.forceCompletion();

    QVERIFY(incubator.isReady());
    QVERIFY(incubator.object() != 0);
    QCOMPARE(incubator.object()->property("testValue").toInt(), 3499);

    delete incubator.object();
    }
}

void tst_qdeclarativeincubator::setInitialState()
{
    QDeclarativeComponent component(&engine, testFileUrl("setInitialState.qml"));
    QVERIFY(component.isReady());

    struct MyIncubator : public QDeclarativeIncubator
    {
        MyIncubator(QDeclarativeIncubator::IncubationMode mode)
        : QDeclarativeIncubator(mode) {}

        virtual void setInitialState(QObject *o) {
            QDeclarativeProperty::write(o, "test2", 19);
            QDeclarativeProperty::write(o, "testData1", 201);
        }
    };

    {
    MyIncubator incubator(QDeclarativeIncubator::Asynchronous);
    component.create(incubator);
    QVERIFY(incubator.isLoading());
    bool b = true;
    controller.incubateWhile(&b);
    QVERIFY(incubator.isReady());
    QVERIFY(incubator.object());
    QCOMPARE(incubator.object()->property("myValueFunctionCalled").toBool(), false);
    QCOMPARE(incubator.object()->property("test1").toInt(), 502);
    QCOMPARE(incubator.object()->property("test2").toInt(), 19);
    delete incubator.object();
    }

    {
    MyIncubator incubator(QDeclarativeIncubator::Synchronous);
    component.create(incubator);
    QVERIFY(incubator.isReady());
    QVERIFY(incubator.object());
    QCOMPARE(incubator.object()->property("myValueFunctionCalled").toBool(), false);
    QCOMPARE(incubator.object()->property("test1").toInt(), 502);
    QCOMPARE(incubator.object()->property("test2").toInt(), 19);
    delete incubator.object();
    }
}

void tst_qdeclarativeincubator::clearDuringCompletion()
{
    CompletionRegisteringType::clearMe();
    SelfRegisteringType::clearMe();

    QDeclarativeComponent component(&engine, testFileUrl("clearDuringCompletion.qml"));
    QVERIFY(component.isReady());

    QDeclarativeIncubator incubator;
    component.create(incubator);

    QCOMPARE(incubator.status(), QDeclarativeIncubator::Loading);
    QVERIFY(CompletionRegisteringType::me() == 0);

    while (CompletionRegisteringType::me() == 0 && incubator.isLoading()) {
        bool b = false;
        controller.incubateWhile(&b);
    }

    QVERIFY(CompletionRegisteringType::me() != 0);
    QVERIFY(SelfRegisteringType::me() != 0);
    QVERIFY(incubator.isLoading());

    QPointer<QObject> srt = SelfRegisteringType::me();

    incubator.clear();
    QCoreApplication::processEvents(QEventLoop::DeferredDeletion);
    QVERIFY(incubator.isNull());
    QVERIFY(srt.isNull());
}

void tst_qdeclarativeincubator::objectDeletionAfterInit()
{
    QDeclarativeComponent component(&engine, testFileUrl("clear.qml"));
    QVERIFY(component.isReady());

    struct MyIncubator : public QDeclarativeIncubator
    {
        MyIncubator(QDeclarativeIncubator::IncubationMode mode)
        : QDeclarativeIncubator(mode), obj(0) {}

        virtual void setInitialState(QObject *o) {
            obj = o;
        }

        QObject *obj;
    };

    SelfRegisteringType::clearMe();
    MyIncubator incubator(QDeclarativeIncubator::Asynchronous);
    component.create(incubator);

    while (!incubator.obj && incubator.isLoading()) {
        bool b = false;
        controller.incubateWhile(&b);
    }

    QVERIFY(incubator.isLoading());
    QVERIFY(SelfRegisteringType::me() != 0);

    delete incubator.obj;

    incubator.clear();
    QCoreApplication::processEvents(QEventLoop::DeferredDeletion);
    QVERIFY(incubator.isNull());
}

class Switcher : public QObject
{
    Q_OBJECT
public:
    Switcher(QDeclarativeEngine *e) : QObject(), engine(e) { }

    struct MyIncubator : public QDeclarativeIncubator
    {
        MyIncubator(QDeclarativeIncubator::IncubationMode mode, QObject *s)
        : QDeclarativeIncubator(mode), switcher(s) {}

        virtual void setInitialState(QObject *o) {
            if (o->objectName() == "switchMe")
                connect(o, SIGNAL(switchMe()), switcher, SLOT(switchIt()));
        }

        QObject *switcher;
    };

    void start()
    {
        incubator = new MyIncubator(QDeclarativeIncubator::Synchronous, this);
        component = new QDeclarativeComponent(engine, QDeclarativeDataTest::instance()->testFileUrl("recursiveClear.1.qml"));
        component->create(*incubator);
    }

    QDeclarativeEngine *engine;
    MyIncubator *incubator;
    QDeclarativeComponent *component;

public slots:
    void switchIt() {
        component->deleteLater();
        incubator->clear();
        component = new QDeclarativeComponent(engine, QDeclarativeDataTest::instance()->testFileUrl("recursiveClear.2.qml"));
        component->create(*incubator);
    }
};

void tst_qdeclarativeincubator::recursiveClear()
{
    Switcher switcher(&engine);
    switcher.start();
}

void tst_qdeclarativeincubator::statusChanged()
{
    class MyIncubator : public QDeclarativeIncubator
    {
    public:
        MyIncubator(QDeclarativeIncubator::IncubationMode mode = QDeclarativeIncubator::Asynchronous)
        : QDeclarativeIncubator(mode) {}

        QList<int> statuses;
    protected:
        virtual void statusChanged(Status s) { statuses << s; }
        virtual void setInitialState(QObject *) { statuses << -1; }
    };

    {
    QDeclarativeComponent component(&engine, testFileUrl("statusChanged.qml"));
    QVERIFY(component.isReady());

    MyIncubator incubator(QDeclarativeIncubator::Synchronous);
    component.create(incubator);
    QVERIFY(incubator.isReady());
    QCOMPARE(incubator.statuses.count(), 3);
    QCOMPARE(incubator.statuses.at(0), int(QDeclarativeIncubator::Loading));
    QCOMPARE(incubator.statuses.at(1), -1);
    QCOMPARE(incubator.statuses.at(2), int(QDeclarativeIncubator::Ready));
    delete incubator.object();
    }

    {
    QDeclarativeComponent component(&engine, testFileUrl("statusChanged.qml"));
    QVERIFY(component.isReady());

    MyIncubator incubator(QDeclarativeIncubator::Asynchronous);
    component.create(incubator);
    QVERIFY(incubator.isLoading());
    QCOMPARE(incubator.statuses.count(), 1);
    QCOMPARE(incubator.statuses.at(0), int(QDeclarativeIncubator::Loading));

    {
    bool b = true;
    controller.incubateWhile(&b);
    }

    QCOMPARE(incubator.statuses.count(), 3);
    QCOMPARE(incubator.statuses.at(0), int(QDeclarativeIncubator::Loading));
    QCOMPARE(incubator.statuses.at(1), -1);
    QCOMPARE(incubator.statuses.at(2), int(QDeclarativeIncubator::Ready));
    delete incubator.object();
    }

    {
    QDeclarativeComponent component2(&engine, testFileUrl("statusChanged.nested.qml"));
    QVERIFY(component2.isReady());

    MyIncubator incubator(QDeclarativeIncubator::Asynchronous);
    component2.create(incubator);
    QVERIFY(incubator.isLoading());
    QCOMPARE(incubator.statuses.count(), 1);
    QCOMPARE(incubator.statuses.at(0), int(QDeclarativeIncubator::Loading));

    {
    bool b = true;
    controller.incubateWhile(&b);
    }

    QVERIFY(incubator.isReady());
    QCOMPARE(incubator.statuses.count(), 3);
    QCOMPARE(incubator.statuses.at(0), int(QDeclarativeIncubator::Loading));
    QCOMPARE(incubator.statuses.at(1), -1);
    QCOMPARE(incubator.statuses.at(2), int(QDeclarativeIncubator::Ready));
    delete incubator.object();
    }
}

void tst_qdeclarativeincubator::asynchronousIfNested()
{
    // Asynchronous if nested within a finalized context behaves synchronously
    {
    QDeclarativeComponent component(&engine, testFileUrl("asynchronousIfNested.1.qml"));
    QVERIFY(component.isReady());

    QObject *object = component.create();
    QVERIFY(object != 0);
    QCOMPARE(object->property("a").toInt(), 10);

    QDeclarativeIncubator incubator(QDeclarativeIncubator::AsynchronousIfNested);
    component.create(incubator, 0, qmlContext(object));

    QVERIFY(incubator.isReady());
    QVERIFY(incubator.object());
    QCOMPARE(incubator.object()->property("a").toInt(), 10);
    delete incubator.object();
    delete object;
    }

    // Asynchronous if nested within an executing context behaves asynchronously, but prevents
    // the parent from finishing
    {
    SelfRegisteringType::clearMe();

    QDeclarativeComponent component(&engine, testFileUrl("asynchronousIfNested.2.qml"));
    QVERIFY(component.isReady());

    QDeclarativeIncubator incubator;
    component.create(incubator);

    QVERIFY(incubator.isLoading());
    QVERIFY(SelfRegisteringType::me() == 0);
    while (SelfRegisteringType::me() == 0 && incubator.isLoading()) {
        bool b = false;
        controller.incubateWhile(&b);
    }

    QVERIFY(SelfRegisteringType::me() != 0);
    QVERIFY(incubator.isLoading());

    QDeclarativeIncubator nested(QDeclarativeIncubator::AsynchronousIfNested);
    component.create(nested, 0, qmlContext(SelfRegisteringType::me()));
    QVERIFY(nested.isLoading());

    while (nested.isLoading()) {
        QVERIFY(incubator.isLoading());
        bool b = false;
        controller.incubateWhile(&b);
    }

    QVERIFY(nested.isReady());
    QVERIFY(incubator.isLoading());

    {
        bool b = true;
        controller.incubateWhile(&b);
    }

    QVERIFY(nested.isReady());
    QVERIFY(incubator.isReady());

    delete nested.object();
    delete incubator.object();
    }

    // AsynchronousIfNested within a synchronous AsynchronousIfNested behaves synchronously
    {
    SelfRegisteringType::clearMe();

    QDeclarativeComponent component(&engine, testFileUrl("asynchronousIfNested.3.qml"));
    QVERIFY(component.isReady());

    struct CallbackData {
        CallbackData(QDeclarativeEngine *e) : engine(e), pass(false) {}
        QDeclarativeEngine *engine;
        bool pass;
        static void callback(CallbackRegisteringType *o, void *data) {
            CallbackData *d = (CallbackData *)data;

            QDeclarativeComponent c(d->engine, QDeclarativeDataTest::instance()->testFileUrl("asynchronousIfNested.1.qml"));
            if (!c.isReady()) return;

            QDeclarativeIncubator incubator(QDeclarativeIncubator::AsynchronousIfNested);
            c.create(incubator, 0, qmlContext(o));

            if (!incubator.isReady()) return;

            if (incubator.object()->property("a").toInt() != 10) return;

            d->pass = true;
        }
    };

    CallbackData cd(&engine);
    CallbackRegisteringType::registerCallback(&CallbackData::callback, &cd);

    QDeclarativeIncubator incubator(QDeclarativeIncubator::AsynchronousIfNested);
    component.create(incubator);

    QVERIFY(incubator.isReady());
    QCOMPARE(cd.pass, true);

    delete incubator.object();
    }
}

void tst_qdeclarativeincubator::nestedComponent()
{
    QDeclarativeComponent component(&engine, testFileUrl("nestedComponent.qml"));
    QVERIFY(component.isReady());

    QObject *object = component.create();

    QDeclarativeComponent *nested = object->property("c").value<QDeclarativeComponent*>();
    QVERIFY(nested);
    QVERIFY(nested->isReady());

    // Test without incubator
    {
    QObject *nestedObject = nested->create();
    QCOMPARE(nestedObject->property("value").toInt(), 19988);
    delete nestedObject;
    }

    // Test with incubator
    {
    QDeclarativeIncubator incubator(QDeclarativeIncubator::Synchronous);
    nested->create(incubator);
    QVERIFY(incubator.isReady());
    QVERIFY(incubator.object());
    QCOMPARE(incubator.object()->property("value").toInt(), 19988);
    delete incubator.object();
    }

    delete object;
}

// Checks that a new AsynchronousIfNested incubator can be correctly started in the
// statusChanged() callback of another.
void tst_qdeclarativeincubator::chainedAsynchronousIfNested()
{
    SelfRegisteringType::clearMe();

    QDeclarativeComponent component(&engine, testFileUrl("chainedAsynchronousIfNested.qml"));
    QVERIFY(component.isReady());

    QDeclarativeIncubator incubator(QDeclarativeIncubator::Asynchronous);
    component.create(incubator);

    QVERIFY(incubator.isLoading());
    QVERIFY(SelfRegisteringType::me() == 0);

    while (SelfRegisteringType::me() == 0 && incubator.isLoading()) {
        bool b = false;
        controller.incubateWhile(&b);
    }

    QVERIFY(SelfRegisteringType::me() != 0);
    QVERIFY(incubator.isLoading());

    struct MyIncubator : public QDeclarativeIncubator {
        MyIncubator(MyIncubator *next, QDeclarativeComponent *component, QDeclarativeContext *ctxt)
        : QDeclarativeIncubator(AsynchronousIfNested), next(next), component(component), ctxt(ctxt) {}

    protected:
        virtual void statusChanged(Status s) {
            if (s == Ready && next)
                component->create(*next, 0, ctxt);
        }

    private:
        MyIncubator *next;
        QDeclarativeComponent *component;
        QDeclarativeContext *ctxt;
    };

    MyIncubator incubator2(0, &component, 0);
    MyIncubator incubator1(&incubator2, &component, qmlContext(SelfRegisteringType::me()));

    component.create(incubator1, 0, qmlContext(SelfRegisteringType::me()));

    QVERIFY(incubator.isLoading());
    QVERIFY(incubator1.isLoading());
    QVERIFY(incubator2.isNull());

    while (incubator1.isLoading()) {
        QVERIFY(incubator.isLoading());
        QVERIFY(incubator1.isLoading());
        QVERIFY(incubator2.isNull());

        bool b = false;
        controller.incubateWhile(&b);
    }

    QVERIFY(incubator.isLoading());
    QVERIFY(incubator1.isReady());
    QVERIFY(incubator2.isLoading());

    while (incubator2.isLoading()) {
        QVERIFY(incubator.isLoading());
        QVERIFY(incubator1.isReady());
        QVERIFY(incubator2.isLoading());

        bool b = false;
        controller.incubateWhile(&b);
    }

    QVERIFY(incubator.isLoading());
    QVERIFY(incubator1.isReady());
    QVERIFY(incubator2.isReady());

    {
    bool b = true;
    controller.incubateWhile(&b);
    }

    QVERIFY(incubator.isReady());
    QVERIFY(incubator1.isReady());
    QVERIFY(incubator2.isReady());
}

// Checks that new AsynchronousIfNested incubators can be correctly chained if started in
// componentCompleted().
void tst_qdeclarativeincubator::chainedAsynchronousIfNestedOnCompleted()
{
    SelfRegisteringType::clearMe();

    QDeclarativeComponent component(&engine, testFileUrl("chainInCompletion.qml"));
    QVERIFY(component.isReady());

    QDeclarativeComponent c1(&engine, testFileUrl("chainedAsynchronousIfNested.qml"));
    QVERIFY(c1.isReady());

    struct MyIncubator : public QDeclarativeIncubator {
        MyIncubator(MyIncubator *next, QDeclarativeComponent *component, QDeclarativeContext *ctxt)
        : QDeclarativeIncubator(AsynchronousIfNested), next(next), component(component), ctxt(ctxt) {}

    protected:
        virtual void statusChanged(Status s) {
            if (s == Ready && next) {
                component->create(*next, 0, ctxt);
            }
        }

    private:
        MyIncubator *next;
        QDeclarativeComponent *component;
        QDeclarativeContext *ctxt;
    };

    struct CallbackData {
        CallbackData(QDeclarativeComponent *c, MyIncubator *i, QDeclarativeContext *ct)
            : component(c), incubator(i), ctxt(ct) {}
        QDeclarativeComponent *component;
        MyIncubator *incubator;
        QDeclarativeContext *ctxt;
        static void callback(CompletionCallbackType *, void *data) {
            CallbackData *d = (CallbackData *)data;
            d->component->create(*d->incubator, 0, d->ctxt);
        }
    };

    QDeclarativeIncubator incubator(QDeclarativeIncubator::Asynchronous);
    component.create(incubator);

    QVERIFY(incubator.isLoading());
    QVERIFY(SelfRegisteringType::me() == 0);

    while (SelfRegisteringType::me() == 0 && incubator.isLoading()) {
        bool b = false;
        controller.incubateWhile(&b);
    }

    QVERIFY(SelfRegisteringType::me() != 0);
    QVERIFY(incubator.isLoading());

    MyIncubator incubator3(0, &c1, qmlContext(SelfRegisteringType::me()));
    MyIncubator incubator2(&incubator3, &c1, qmlContext(SelfRegisteringType::me()));
    MyIncubator incubator1(&incubator2, &c1, qmlContext(SelfRegisteringType::me()));

    // start incubator1 in componentComplete
    CallbackData cd(&c1, &incubator1, qmlContext(SelfRegisteringType::me()));
    CompletionCallbackType::registerCallback(&CallbackData::callback, &cd);

    while (!incubator1.isLoading()) {
        QVERIFY(incubator.isLoading());
        QVERIFY(incubator2.isNull());
        QVERIFY(incubator3.isNull());

        bool b = false;
        controller.incubateWhile(&b);
    }

    QVERIFY(incubator.isLoading());
    QVERIFY(incubator1.isLoading());
    QVERIFY(incubator2.isNull());
    QVERIFY(incubator3.isNull());

    while (incubator1.isLoading()) {
        QVERIFY(incubator.isLoading());
        QVERIFY(incubator1.isLoading());
        QVERIFY(incubator2.isNull());
        QVERIFY(incubator3.isNull());

        bool b = false;
        controller.incubateWhile(&b);
    }

    QVERIFY(incubator.isLoading());
    QVERIFY(incubator1.isReady());
    QVERIFY(incubator2.isLoading());
    QVERIFY(incubator3.isNull());

    while (incubator2.isLoading()) {
        QVERIFY(incubator.isLoading());
        QVERIFY(incubator1.isReady());
        QVERIFY(incubator2.isLoading());
        QVERIFY(incubator3.isNull());

        bool b = false;
        controller.incubateWhile(&b);
    }

    QVERIFY(incubator.isLoading());
    QVERIFY(incubator1.isReady());
    QVERIFY(incubator2.isReady());
    QVERIFY(incubator3.isLoading());

    while (incubator3.isLoading()) {
        QVERIFY(incubator.isLoading());
        QVERIFY(incubator1.isReady());
        QVERIFY(incubator2.isReady());
        QVERIFY(incubator3.isLoading());

        bool b = false;
        controller.incubateWhile(&b);
    }

    {
    bool b = true;
    controller.incubateWhile(&b);
    }

    QVERIFY(incubator.isReady());
    QVERIFY(incubator1.isReady());
    QVERIFY(incubator2.isReady());
    QVERIFY(incubator3.isReady());
}

void tst_qdeclarativeincubator::selfDelete()
{
    struct MyIncubator : public QDeclarativeIncubator {
        MyIncubator(bool *done, Status status, IncubationMode mode)
        : QDeclarativeIncubator(mode), done(done), status(status) {}

    protected:
        virtual void statusChanged(Status s) {
            if (s == status) {
                *done = true;
                if (s == Ready) delete object();
                delete this;
            }
        }

    private:
        bool *done;
        Status status;
    };

    {
    QDeclarativeComponent component(&engine, testFileUrl("selfDelete.qml"));

#define DELETE_TEST(status, mode) { \
    bool done = false; \
    component.create(*(new MyIncubator(&done, status, mode))); \
    bool True = true; \
    controller.incubateWhile(&True); \
    QVERIFY(done == true); \
    }

    DELETE_TEST(QDeclarativeIncubator::Loading, QDeclarativeIncubator::Synchronous);
    DELETE_TEST(QDeclarativeIncubator::Ready, QDeclarativeIncubator::Synchronous);
    DELETE_TEST(QDeclarativeIncubator::Loading, QDeclarativeIncubator::Asynchronous);
    DELETE_TEST(QDeclarativeIncubator::Ready, QDeclarativeIncubator::Asynchronous);

#undef DELETE_TEST
    }

    // Delete within error status
    {
    SelfRegisteringType::clearMe();

    QDeclarativeComponent component(&engine, testFileUrl("objectDeleted.qml"));
    QVERIFY(component.isReady());

    bool done = false;
    MyIncubator *incubator = new MyIncubator(&done, QDeclarativeIncubator::Error,
                                             QDeclarativeIncubator::Asynchronous);
    component.create(*incubator);

    QCOMPARE(incubator->QDeclarativeIncubator::status(), QDeclarativeIncubator::Loading);
    QVERIFY(SelfRegisteringType::me() == 0);

    while (SelfRegisteringType::me() == 0 && incubator->isLoading()) {
        bool b = false;
        controller.incubateWhile(&b);
    }

    QVERIFY(SelfRegisteringType::me() != 0);
    QVERIFY(incubator->isLoading());

    delete SelfRegisteringType::me();

    {
    bool b = true;
    controller.incubateWhile(&b);
    }

    QVERIFY(done);
    }
}

// Test that QML doesn't crash if the context is deleted prior to the incubator
// first executing.
void tst_qdeclarativeincubator::contextDelete()
{
    QDeclarativeContext *context = new QDeclarativeContext(engine.rootContext());
    QDeclarativeComponent component(&engine, testFileUrl("contextDelete.qml"));

    QDeclarativeIncubator incubator;
    component.create(incubator, context);

    delete context;

    {
        bool b = false;
        controller.incubateWhile(&b);
    }
}

QTEST_MAIN(tst_qdeclarativeincubator)

#include "tst_qdeclarativeincubator.moc"
