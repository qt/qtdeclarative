// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick

Item {
    property var p: f()
    // crash if arguments are wrongly collected, e.g. because they are stolen from other unimplemented scriptelements
    property var p2: f(1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20)

    // dummy test: if these ones fail, then the deconstructing parameters cannot possibly work (they share some
    // common code).
    function deconstruct() {
        let { a } = { a: 32, b: 42}, { b, c } = { b: 32, c: 42}, [ d, e, f ] = [ 111, 222, 333 ];
        let x = [,,,,[1,2,3],,];
    }

    function f(q,w,e,r,t,y) {
        let helloF = 32
        return 42
    }

    function fWithDefault(q = 1, w = 2, e, r = 4, t, y = 6) {
        let helloFWithDefault = {}
        return 42
    }

    function marmelade(...onTheBread) {
        let helloMarmelade = 123
        return 42
    }

    function marmelade2(spread, it,...onTheBread) {
        let helloMarmelade2 = 123
        return 42
    }

    // check if nothing crashes for empty stuff
    function empty({}, []) {
        let {} = {};
        let [] = [];
    }

    component MyType: Item{}

    function withTypes(a: int, b: MyType) {}
    function empty() {}
    signal mySignal()


    property var p3: evil({ hello: "World", y: "yyy"}, [1,2,3], { is: {a: 111, lot: 222, of: 333, fun: 444, really: ["!",]}})

    function evil({ hello = "world", x = 42 },
                  [n = 42, m = 43, o = 44],
                  { destructuring, is = {a, lot, of}, fun = 42 } = {destructuring : 123, is : {x : 123}, fun : 456}) {
        const helloEvil = "asdf"
        return 42
    }


}
