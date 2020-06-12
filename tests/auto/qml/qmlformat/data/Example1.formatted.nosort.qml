/* This file is licensed under the not a license license
	1. You may not comply
	2. Goodbye
*/

// Importing this is very important
import QtQuick 5.15
// Muddling the waters!
import QtQuick.Models 3.14 as muddle
// Importing that is important too
import Z
import That
import This // THIS IS VERY IMPORTANT!
import Y
import X.Z
import X.Y
import A.LLOHA
import A.B.B.A

// This comment is related to Item
Item {
    // Orphan comment
    // Another orphan
    // More orphans

    // This to id
    // Also id. (line 2)
    // This is the third id
    // fourth id comment
    id: foo

    // This to enum
    enum Foo {
        A = 3, // This is A
        B, // This is B
        C = 4, // This is C
        D // This is D
    }

    property bool some_bool: false
    property variant some_array_literal: [30, 20, Math["PI"], [4, 3, 2], "foo", 0.3]
    property bool something_computed: function(x) {
        // This is an orphan inside something_computed
        // Are these getting duplicated?
        // Another orphan inside something_computed

        const PI = 3, DAYS_PER_YEAR = 365.25;
        var x = 3 + 2;
        x["bla"] = 50;
        // This one to var few!
        var few = new WhatEver();
        x += Math.sin(3);
        x--;
        --x;
        x++;
        ++x;
        for (var x = 0; x < 100; x++) {
            x++;
            console.log("Foo");
        }
        for (var x in [3, 2, 1]) {
            y++;
            console.log("Bar");
        }
        while (true)
            console.log("Wee");

        with (foo) {
            bar;
            x += 5;
        } // This is related to with!
        x3:
        do {
            console.log("Hello");
        } while (3 == 0);
        try {
            dangerous();
        } catch (e) {
            console.log(e);
        } finally {
            dangerous();
        }
        switch (x) {
        case 0:
            x = 1;
            break;
        case 1:
            x = 5;
            break;
        case 4:
            x = 100;
            break;
        }
        if (x == 50)
            console.log("true");
        else if (x == 50)
            console.log("other thing");
        else
            console.log("false");
        if (x == 50) {
            console.log("true");
        } else if (x == 50) {
            console.log("other thing");
            x--;
        } else {
            console.log("false");
        }
        return "foobar";
    }()
    default property bool some_default_bool: 500 % 5 !== 0 // some_default_bool
    // some_read_only_bool
    readonly property bool some_read_only_bool: Math.sin(3) && (aFunc()[30] + 5) | 2 != 0

    signal say(string name, bool caps)

    // This one to aFunc()
    function aFunc() {
        var x = 3;
        return x;
    }

    x: 3 // Very cool
    Component.onCompleted: console.log("Foo!")
    myFavouriteThings: [
        // This is an orphan

        // This is a cool text
        Text {
        },
        // This is a cool rectangle
        Rectangle {
        }
    ]

    Text {
        required property string batman

        signal boo(int count, int times, real duration)

        text: "Bla"
    }

    // This comment is related to the property animation
    PropertyAnimation on x {
        id: foo

        x: 3
        y: x + 3
    }

}
