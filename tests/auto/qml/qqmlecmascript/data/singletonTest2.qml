// Copyright (C) 2013 Canonical Limited and/or its subsidiary(-ies).
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick 2.0
import Test 1.0

Item {
    property bool myInheritedQmlObjectTest1: false
    property bool myInheritedQmlObjectTest2: false
    property bool myInheritedQmlObjectTest3: false
    property bool myQmlObjectTest1: false
    property bool myQmlObjectTest2: false
    property bool myQmlObjectTest3: false
    property bool qobjectTest1: false
    property bool qobjectTest2: false
    property bool qobjectTest3: false
    property bool singletonEqualToItself: true
    property int testFoo: -1

    Component.onCompleted: {
        MyInheritedQmlObjectSingleton.myInheritedQmlObjectProperty = MyInheritedQmlObjectSingleton;
        myInheritedQmlObjectTest1 = MyInheritedQmlObjectSingleton.myInheritedQmlObjectProperty == MyInheritedQmlObjectSingleton;
        myInheritedQmlObjectTest2 = MyInheritedQmlObjectSingleton.myInheritedQmlObjectProperty == MyInheritedQmlObjectSingleton.myInheritedQmlObjectProperty;
        myInheritedQmlObjectTest3 = MyInheritedQmlObjectSingleton == MyInheritedQmlObjectSingleton.myInheritedQmlObjectProperty;

        MyInheritedQmlObjectSingleton.myQmlObjectProperty = MyInheritedQmlObjectSingleton;
        myQmlObjectTest1 = MyInheritedQmlObjectSingleton.myQmlObjectProperty == MyInheritedQmlObjectSingleton;
        myQmlObjectTest2 = MyInheritedQmlObjectSingleton.myQmlObjectProperty == MyInheritedQmlObjectSingleton.myQmlObjectProperty;
        myQmlObjectTest3 = MyInheritedQmlObjectSingleton == MyInheritedQmlObjectSingleton.myQmlObjectProperty;

        MyInheritedQmlObjectSingleton.qobjectProperty = MyInheritedQmlObjectSingleton;
        qobjectTest1 = MyInheritedQmlObjectSingleton.qobjectProperty == MyInheritedQmlObjectSingleton;
        qobjectTest2 = MyInheritedQmlObjectSingleton.qobjectProperty == MyInheritedQmlObjectSingleton.qobjectProperty;
        qobjectTest3 = MyInheritedQmlObjectSingleton == MyInheritedQmlObjectSingleton.qobjectProperty;

        singletonEqualToItself = MyInheritedQmlObjectSingleton == MyInheritedQmlObjectSingleton;
        testFoo = TestTypeCppSingleton.foo
    }
}
