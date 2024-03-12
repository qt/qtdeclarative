// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick 2.8
import QtTest 1.1

TestCase {
    name: "tst_tryVerify"

    Item {
        id: item
    }

    QtObject {
        id: itemContainer
        property Item i
    }

    Timer {
        id: timer
        interval: 100
        onTriggered: itemContainer.i = item
    }

    function resetTimer() {
        itemContainer.i = null;
        timer.restart();
    }

    function test_tryVerify() {
        timer.start();
        tryVerify(function(){ return itemContainer.i; }, 200, "string");
        compare(itemContainer.i, item);

        resetTimer();
        tryVerify(function(){ return itemContainer.i; }, 200);
        compare(itemContainer.i, item);

        resetTimer();
        tryVerify(function(){ return itemContainer.i; });
        compare(itemContainer.i, item);

        resetTimer();
        tryVerify(function(){ return !itemContainer.i; }, 0, "string");
        verify(!itemContainer.i);
    }
}
