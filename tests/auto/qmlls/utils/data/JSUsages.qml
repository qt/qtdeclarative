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

    function recursive(n: int): int {
        if (n > 3)
            return 1 + recursive(recursive(x-1) + recursive(x-2) - recursive(x-3));
        else
            return recursive(0);
    }

    property int helloRecursive: recursive(42)

    id: rootId
    Rectangle {
        function f() {
            return rootId.recursive(123)
        }
    }

    signal helloSignal()

    function callSignals() {
        helloSignal()
        if (false) {
            helloSignal()
        } else {
        // helloSignal() // not a usage btw
            if (true)
                helloSignal()
        }
    }
    function callSignals2() {
        helloSignal()
        if (false) {
            widthChanged()
        } else {
        // helloSignal() // not a usage btw
            if (true)
                widthChanged()
            rootId.widthChanged()
        }
    }
    Item {
        function callSignalsInChild() {
            widthChanged()
            rootId.widthChanged()
        }
    }

    function myHelloHandler() { let x = 32; }
    onHelloSignal: myHelloHandler

    property int helloPropertyBinding
    helloPropertyBinding: 123

    property int checkHandlers
    onCheckHandlersChanged: myHelloHandler
    onChildrenChanged: myHelloHandler
    function callChanged() {
        checkHandlersChanged()
        childrenChanged()
    }
    property int _: 48
    property int ______42: 48
    property int _123a: 48
    on_Changed: myHelloHandler
    on______42Changed: myHelloHandler
    on_123AChanged: myHelloHandler
    function weirdPropertynames() {
        _Changed()
        ______42Changed()
        _123aChanged()
    }

    TapHandler {
        onTapped: myHelloHandler
        function f() {
            tapped()
        }
    }

    function anotherF() {
        helloPropertyChanged()
    }
    onHelloPropertyChanged: myHelloHandler
    Type {}
}
