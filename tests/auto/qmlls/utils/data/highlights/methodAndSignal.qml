// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick

Item {
    signal p()
    signal q(int a)
    signal r(a: int)
    function a(b: int) : int {}

    // lambda
    signal helloSignal
    function method() {
        console.log("helloMethod");
        let myLambda = function(a, b) { return a + b };
        let myArrow = (v, w) => f + b;
    }
    onHelloSignal: function(x, y, z) { console.log("HelloLambda"); }
    function testNestedFunctions() {
        function nested(tic, tac, toe) { return tic + tac/3 + toe/2}
        nested()
    }
    function generators() {
        function *myGeneratorDeclaration(a, b) { yield 5 };
        let myGenerator = function*(tic, tac, toe) { yield tic + tac - toe };
    }

    function *generatorInQmlObject() {
        function nested(q,w,e,r) { return q + w + e - r; }
        function *nested2(a,z,e,r) { yield a + z + e - r; yield 42; }
        yield 4;
        yield* nested2(1,2,3,4);
        const t = (function (a) {
            return a + 100;
        });
    }
    function traditionalLambda() {
        const tradition = (function (a) {
            return a + 100;
        });
    }
}
