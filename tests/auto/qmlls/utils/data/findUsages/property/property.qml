// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick

Item {
    property int p2: 1
    property int helloProperty: 0
    function withProperty() {
        let sum = 0, sum2 = 0
        for(let i = 1; i < 42; i = i + 2) {
            sum = sum + i
            helloProperty = helloProperty + sum - i * p2;
            {
                let helloProperty = "evil"
            }
        }
    }
    function signalEmitter() {
        helloProperty = 23;
        helloPropertyChanged()
    }
    onHelloPropertyChanged: {}

    // inline component
    component IC: Item {
        property var helloProperty
        function f() {
            return helloProperty + p2
        }
    }

    //sub item
    Item {
        function f() {
            return helloProperty + p2
        }
        property string helloProperty
    }

    PropertyFromAnotherFile {
        helloProperty: 42
        function f() {
            return helloProperty + 53;
        }
        onHelloPropertyChanged: f()
    }
}
