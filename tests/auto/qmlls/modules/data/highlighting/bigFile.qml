// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick

Item {
    id: rootId
    function f() {
        let sum = 0, sum2 = 0;
        for (let i = 1; i < 42; i = i + 2) {
            sum = sum + i;
            {
                let sum = 42; // another unrelated sum
            }
        }
    }

    readonly property int helloProperty: 0
    property int p2: 1

    function withProperty() {
        let sum = 0, sum2 = 0;
        for (const i = 1; i < 42; i = i + 2) {
            sum = sum + i;
            helloProperty = helloProperty + sum - i * p2;
            {
                let helloProperty = "evil";
            }
        }
    }
    Item {
        function f() {
            return helloProperty + p2;
        }
        property string helloProperty
    }
    component IC: Item {
        property var helloProperty
        function f() {
            return helloProperty + p2;
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
        let a = myNested.p2 + p2;
        let b = myNested.inner.p2 + p2;
        let c = myNested.inner.inner.p2 + p2;
        let d = myNested.inner.inner.inner.p2 + p2;
    }

    function recursive(n: int): int {
        if (n > 3)
            return 1 + recursive(recursive(x - 1) + recursive(x - 2) - recursive(x - 3));
        else
            return recursive(0);
    }

    property int helloRecursive: recursive(42)
    Rectangle {
        function f() {
            return rootId.recursive(123);
        }
    }

    signal helloSignal

    function callSignals() {
        helloSignal();
        if (false) {
            helloSignal();
        } else {
            // helloSignal() // not a usage btw
            if (true)
                helloSignal();
        }
    }
    function callSignals2() {
        helloSignal();
        if (false) {
            widthChanged();
        } else {
            // helloSignal() // not a usage btw
            if (true)
                widthChanged();
            rootId.widthChanged();
        }
    }
    Item {
        function callSignalsInChild() {
            widthChanged();
            rootId.widthChanged();
        }
    }

    function myHelloHandler() {
        let x = 32;
    }
    onHelloSignal: myHelloHandler

    property int helloPropertyBinding
    helloPropertyBinding: 123

    property int checkHandlers
    onCheckHandlersChanged: myHelloHandler
    onChildrenChanged: myHelloHandler
    function callChanged() {
        checkHandlersChanged();
        childrenChanged();
    }
    property int _: 48
    property int ______42: 48
    property int _123a: 48
    on_Changed: myHelloHandler
    on______42Changed: myHelloHandler
    on_123AChanged: myHelloHandler
    function weirdPropertynames() {
        _Changed();
        ______42Changed();
        _123aChanged();
    }

    TapHandler {
        onTapped: myHelloHandler
        function f() {
            tapped();
        }
    }

    function anotherF() {
        helloPropertyChanged();
    }
    onHelloPropertyChanged: myHelloHandler
    // Type {}
    function foo(mouse) {
    }

    MouseArea {
        id: area1
        onClicked: foo
        property int insideMouseArea1
    }

    MouseArea {
        id: area2
        Connections {
            function onClicked(mouse) {
                area1.clicked();
                area3.clicked();
            }
        }
        property int insideMouseArea2

        MouseArea {
            id: area3
        }
    }

    property Connections c: Connections {
        target: area3
        onClicked: function (mouse) {}
    }
    function useMouseAreas() {
        area1.clicked();
        area2.clicked();
        area3.clicked();
    }

    function checkParameters(a: int, b: double, {
        x,
        y = {},
        z = [x, y]
    }) {
        return a + b + c + x + y + z;
    }

    function deconstructingUsages(xxx) {
        let {
            a,
            b
        } = xxx;
        let c = a + b;
    }

    function k() {
    }

    function mafik() {
        var patron = 34;
        const upperLimit = 42;
        do {
            ++patron;
            if (patron < 2)
                continue;
            else
                ++patron;
        } while (patron < upperLimit)
        switch (patron) {
        case 1:
            return 23;
        default:
            break;
        }
        try {
            {}
        } catch (error) {
            {}
        } finally {}
        for (const a in [1, 2, 3]) {
            throw 2;
        }
    }

    enum Test {
        LOG
    }

    readonly property int t: 34
    signal tt
    required property int k

    signal kkk(string a)
    signal yyy(string a)

    function ttt() {
    }

    function createComplexExpression(...objects) {
        // Create an object that holds some data
        let data = {
            a: 5,
            b: 10,
            c: 3
        };

        // Create a complex expression using the data object
        let expression = ((data.a + data.b * data.c) / (data.a - data.b)) ** data.c;

        return expression;
    }

    function set1() {
        const array = [1, 2, 3, 4];
        const [a, b] = [1, 2];
        const [aa, , bb] = array;
        const [aaa = 23, bbb] = array;
        const [a1, b1, ...rest1] = array;
        const [a2, , b2, ...rest2] = array;
        const [a3, b3, ...{
                pop,
                push
            }] = array;
        const [a4, b4, ...[c, d]] = array;

        const obj = {
            _a: 1,
            _b: 2
        };
        const {
            a5,
            b5
        } = obj;
        const {
            a6: a_,
            b6: b1_
        } = obj;
        const {
            a7: a11 = 4,
            b11 = 34,
            c1: b111,
            d1
        } = obj;
        let key = a;
        const {
            [key]: a___
        } = obj;
    }

    function set2(s: int): int {
        // declare first
        let a, b, a1, b1, c, d, rest, pop, push;
        const array = [1, 2, 3, 4];
        [a, b] = array;
        [a, , b] = array;
        [a = aDefault, b] = array;
        [a, b, ...rest] = array;
        [a, , b, ...rest] = array;
        [a, b, ...{
                pop,
                push
            }] = array;
        [a, b, ...[c, d]] = array;

        const obj = {
            _a: 1,
            _b: 2
        };
        ({
                a,
                b
            } = obj); // brackets are required
        ({
                a: a1,
                b: b1
            } = obj);

        const complicatedObject = {
            a: 1,
            b: {
                c: 2,
                d: {
                    e: 3,
                    f: [4, 5, 6]
                }
            },
            g: [7, 8, 9]
        };

        const {
            patron,
            b: {
                mafik,
                d: {
                    e,
                    f: [, secondF, ...restF]
                }
            },
            g: [firstG, ...restG]
        } = complicatedObject;
    }
}
