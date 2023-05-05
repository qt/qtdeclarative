// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick

Item {
    property var p: f()
    // crash if arguments are wrongly collected, e.g. because they are stolen from other unimplemented scriptelements
    property var p2: f(1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20)
    // test destructuring patterns
    property var p3: evil({ hello: "World", y: "yyy"}, [1,2,3], { is: {a: 111, lot: 222, of: 333, fun: 444, really: ["!",]}})

    // function definitions not supported yet
    function f(q,w,e,r,t,y) {
        return 42
    }


    // function definitions not supported yet
    function evil({ hello = "world", x = 42 },
                  [n = 42, m = 43, o = 44],
                  { destructuring, is = {a, lot, of}, fun = 42 } = {destructuring : 123, is : {x : 123}, fun : 456}) {
        return 42
    }


}
