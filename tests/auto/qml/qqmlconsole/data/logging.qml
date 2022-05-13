// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0


import QtQuick 2.0

QtObject {
    id: root

    required property var customObject
    required property var stringListProperty

    function consoleCount() {
        console.count("console.count", "Ignore additional argument");
        console.count();
    }

    Component.onCompleted: {
        console.debug("console.debug");
        console.log("console.log");
        console.info("console.info");
        console.warn("console.warn");
        console.error("console.error");

        consoleCount();
        consoleCount();

        const a = [1, 2];
        const b = { a: "hello", d: 1 };
        b.toString = function() { return JSON.stringify(b); }
        let c;
        const d = 12;
        const e = function() { return 5; };
        const f = true;
        const g = { toString: function() { throw new Error('toString'); } };

        console.log(a);
        console.log(b);
        console.log(c);
        console.log(d);
        console.log(e);
        console.log(f);
        console.log(root);
        console.log(g);
        console.log(1, "pong!", new Object);
        console.log(1, ["ping","pong"], new Object, 2);

        console.log(stringListProperty);
        console.log(customObject);
        console.log([[1,2,3,[2,2,2,2],4],[5,6,7,8]]);

        try {
            console.log(exception);
        } catch (e) {
            return;
        }

        throw "console.log(exception) should have raised an exception";
    }
}
