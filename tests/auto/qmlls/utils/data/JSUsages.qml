// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick

Item {
    function f() {
        let sum = 0, sum2 = 0
        for(let i = 1; i < 42; i = i + 2) {
            sum = sum + i
            {
                let sum = 42; // another unrelated sum
            }
        }
    }

    property int helloProperty: 0
    property int p2: 1

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
    Item {
        function f() {
            return helloProperty + p2
        }
        property string helloProperty
    }
    component IC: Item {
        property var helloProperty
        function f() {
            return helloProperty + p2
        }
    }
    component NestedComponent: Item {
        property NestedComponent2 inner: NestedComponent2 {}
        property int p2
    }
    component NestedComponent2: Item {
        property NestedComponent3 inner
        property int p2
        inner: NestedComponent3 {}
    }
    component NestedComponent3: Item {
        property NestedComponent4 inner
        property int p2
        inner: NestedComponent4 {}

    }
    component NestedComponent4: Item {
        property int helloProperty
        property int p2
    }
    NestedComponent {
        id: myNested
    }
    function nestedUsages() {
        let x = myNested.inner.inner.inner.helloProperty + helloProperty;
        let a = myNested.p2 + p2
        let b = myNested.inner.p2 + p2
        let c = myNested.inner.inner.p2 + p2
        let d = myNested.inner.inner.inner.p2 + p2
    }
}
