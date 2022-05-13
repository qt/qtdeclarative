// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "avatarExample.h"
#include <QQmlEngine>
#include <QQmlComponent>
#include <QGuiApplication>

struct Expectations
{
    QQmlEngine engine;

    void expectOne()
    {
//![1]
        QQmlComponent component(&engine, "qrc:/exampleOne.qml");
        QObject *object = component.create();
        // The scarce resource will have been released automatically
        // by this point, after the binding expression was evaluated.
        bool expectedResult = (object->property("avatarWidth").toInt() == 100);
        delete object;
//![1]
        Q_ASSERT(expectedResult);
    }

    void expectTwo()
    {
//![2]
        QQmlComponent component(&engine, "qrc:/exampleTwo.qml");
        QObject *object = component.create();
        // The scarce resource will not have been released automatically
        // after the binding expression was evaluated.
        // Since the scarce resource was not released explicitly prior
        // to the binding expression being evaluated, we get:
        bool expectedResult = (object->property("avatar").isValid() == true);
        delete object;
//![2]
        Q_ASSERT(expectedResult);
    }

    void expectThree()
    {
//![3]
        QQmlComponent component(&engine, "qrc:/exampleThree.qml");
        QObject *object = component.create();
        // The resource was preserved explicitly during evaluation of the
        // JavaScript expression.  Thus, during property assignment, the
        // scarce resource was still valid, and so we get:
        bool expectedResult = (object->property("avatar").isValid() == true);
        // The scarce resource will not be released until all references to
        // the resource are released, and the JavaScript garbage collector runs.
        delete object;
//![3]
        Q_ASSERT(expectedResult);
    }

    void expectFour()
    {
//![4]
        QQmlComponent component(&engine, "qrc:/exampleFour.qml");
        QObject *object = component.create();
        // The scarce resource was explicitly preserved by the client during
        // the importAvatar() function, and so the scarce resource
        // remains valid until the explicit call to releaseAvatar().  As such,
        // we get the expected results:
        bool expectedResultOne = (object->property("avatarOne").isValid() == true);
        bool expectedResultTwo = (object->property("avatarTwo").isValid() == false);
        // Because the scarce resource referenced by avatarTwo was released explicitly,
        // it will no longer be consuming any system resources (beyond what a normal
        // JS Object would; that small overhead will exist until the JS GC runs, as per
        // any other JavaScript object).
        delete object;
//![4]
        Q_ASSERT(expectedResultOne);
        Q_ASSERT(expectedResultTwo);
    }

    void expectFive()
    {
//![5]
        QQmlComponent component(&engine, "qrc:/exampleFive.qml");
        QObject *object = component.create();
        // We have the expected results:
        bool expectedResultOne = (object->property("avatarOne").isValid() == false);
        bool expectedResultTwo = (object->property("avatarTwo").isValid() == false);
        // Because although only avatarTwo was explicitly released,
        // avatarOne and avatarTwo were referencing the same
        // scarce resource.
        delete object;
//![5]
        Q_ASSERT(expectedResultOne);
        Q_ASSERT(expectedResultTwo);
    }
};

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    Expectations expectations;
    expectations.expectOne();
    expectations.expectTwo();
    expectations.expectThree();
    expectations.expectFour();
    expectations.expectFive();
}
