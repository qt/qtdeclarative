// Copyright (C) 2013 Canonical Limited and/or its subsidiary(-ies).
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick 2.0
import Test 1.0

Item {
    property bool qobjectTest: MyInheritedQmlObjectSingleton.isItYouQObject(MyInheritedQmlObjectSingleton)
    property bool myQmlObjectTest: MyInheritedQmlObjectSingleton.isItYouMyQmlObject(MyInheritedQmlObjectSingleton)
    property bool myInheritedQmlObjectTest: MyInheritedQmlObjectSingleton.isItYouMyInheritedQmlObject(MyInheritedQmlObjectSingleton)
    property int testFoo: TestTypeCppSingleton.foo
}
