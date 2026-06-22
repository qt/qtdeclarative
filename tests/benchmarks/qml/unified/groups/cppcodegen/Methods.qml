// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import TestTypes

BirthdayParty {
    id: party
    host: Person {
        name: "Bob Jones"
        shoeSize: 12
    }
    guests: [
        Person { name: "Leo Hodges" },
        Person { name: "Jack Smith" },
        Person { name: "Anne Brown" }
    ]

    property var dresses: [0, 0, 0, 0]

    property var foo: party
    property var bar: console

    property var numeric
    property var stringly

    Component.onCompleted: {
        invite("William Green")
        foo.invite("The Foo")
        numeric = 1;
        stringly = "name";
    }

    function getEnum(): int {
        return Test.CC
    }

    function getCppEnum(): int {
        return Item.BottomLeft
    }

    function storeElement() {
        ++host.shoeSize
        party.dresses[2] = [1, 2, 3]
    }

    function stuff(sn) {
        if (sn.substr(0, 2) === "on" && sn[2] === sn[2].toUpperCase())
            return sn
        return "on" + sn.substr(0, 1).toUpperCase() + sn.substr(1)
    }

    function retrieveVar() {
        return guests[numeric][stringly];
    }

    function retrieveString() : string {
        return guests[numeric][stringly];
    }

    function sumTyped(n: int) : int {
        var x = 0;
        for (var i=0; i<n; ++i)
            x = x + x;
        return x;
    }

    function sumUntyped(n) {
        var x = 0;
        for (var i=0; i<n; ++i)
            x = x + x;
        return x;
    }

    property var n1: stuff("onGurk")
    property var n2: stuff("semmeln")
    property var n3: stuff("12")

    property bool corner: false
    property int enumValue: corner ? Item.TopRight : Item.BottomLeft
}
