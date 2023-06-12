// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.15

Item {
    id: rootId

    property int i // (1)
    function f(a /*(2)*/ , b) {return a /*  go to definition on a leads to (2) */  > b}  // (4)

    Component.onCompleted: {
        let x = 42 // (3)
        f(x, i) // goto definition on f goes to 4, on x goes to (3) and on i goes to (1)
        f(x, rootId.i) // goto definition on f goes to 4, on x goes to (3) and on i goes to (1)
    }

    function ffff() {
        let scoped = 42;
        {
            let scoped = 666;
            f(scoped, i);
            {
                let a = 12345, i = 32;
                f(scoped, i);
            }
        }
        f(scoped, i);
    }

    Rectangle {
        id: nested

        property int i

        function f(n: int): int {
            let x = i, y = nested.i, z = rootId.i;
            if (x > 3)
                return 1 + f(f(x-1) + f(x-2) - f(x-3));
            else
                return f(0);
        }
        function fff(n: int, m: int): int {
            return f(n + m) / 42 + ffff()
        }
    }
    function abc() {
        return nested.f(42);
    }

    component MyIC: Rectangle {
        id: helloIC

        property int data: 42
        Item {
            property int data: helloIC.data
        }
    }

    property MyIC ic: MyIC {}
    function icProperty() {
        return ic.data
    }
    property int propertyInBinding: i
    property int propertyInBinding2: i * 42
    property int propertyInBinding3: abc()[rootId.i ** 42 - 7]

    property BaseType bt: BaseType {}
    property int helloProperty: 1234567890 // BaseType also has a property helloProperty
    function helloFunction() {} // BaseType also has a method helloFunction
    function fromDifferentFiles() {
        let x = bt.helloProperty + bt.helloFunction()
    }
}
