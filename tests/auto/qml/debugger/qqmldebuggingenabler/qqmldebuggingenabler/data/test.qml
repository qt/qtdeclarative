// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick 2.0

//DO NOT CHANGE

Item {
    Component.onCompleted: {
            var a = [1, 2]
            var b = {a: "hello", d: 1 }
            var c
            var d = 12
        console.log("Component.onCompleted");
    }
    function foo() {
        var a = [1, 2]
        var b = {a: "hello", d: 1 }
        var c
        var d = 12
    }
}

