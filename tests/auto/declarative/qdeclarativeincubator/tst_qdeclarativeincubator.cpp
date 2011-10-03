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
#include <QDeclarativeComponent>
#include <QDeclarativeIncubator>

inline QUrl TEST_FILE(const QString &filename)
{
    QFileInfo fileInfo(__FILE__);
    return QUrl::fromLocalFile(fileInfo.absoluteDir().filePath("data/" + filename));
}

inline QUrl TEST_FILE(const char *filename)
{
    return TEST_FILE(QLatin1String(filename));
}

class tst_qdeclarativeincubator : public QObject
{
    Q_OBJECT
public:
    tst_qdeclarativeincubator() {}

private slots:
    void initTestCase();

    void incubationMode();
    void objectDeleted();
    void clear();

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
        QFile file(QLatin1String(SRCDIR) + QLatin1String("/data/") + QLatin1String(errorfile)); \
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

    QDeclarativeComponent component(&engine, TEST_FILE("objectDeleted.qml"));
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

    QDeclarativeComponent component(&engine, TEST_FILE("clear.qml"));
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

    // XXX Clear in error state
}

QTEST_MAIN(tst_qdeclarativeincubator)

#include "tst_qdeclarativeincubator.moc"
