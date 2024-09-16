// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick

Derived2 {
    id: self
    function f() {
        helloMethod()
        let x = helloProperty;
        helloSignal()
        let y = HelloEnum.HelloEnumValue1
    }

    property var someMethod: helloMethod()
    property int someProperty: helloProperty
    property var someEnum: HelloEnum.HelloEnumValue2
    function someHandler() { helloSignal(); }
    onHelloSignal: someHandler

    Item {
        property var someMethod: self.helloMethod()
        property int someProperty: self.helloProperty
        function f() {
            self.helloSignal()
        }
    }
    helloFont.family: "helloFamily"
    Keys.onBackPressed: someHandler

    helloProperty: 42
}
