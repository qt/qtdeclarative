// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick 2.0

Rectangle {
    id: root
    width: 200
    height: 200
    color: "white"

    property int acc: 0

    function loadPuzzle() {
        console.log("js_func loading level " + acc)
    }

    function nextPuzzle() {
        acc = (acc + 1) % 10;
        loadPuzzle();
    }

    Timer {
        repeat: true
        interval: 300
        running: true
        onTriggered: root.nextPuzzle()
    }
}
