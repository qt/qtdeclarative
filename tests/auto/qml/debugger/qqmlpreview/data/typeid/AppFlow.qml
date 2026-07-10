// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick

Item {
    id: flow
    property int v: 0

    Component { id: pageComp; Page {} }

    function run() {
        var page = pageComp.createObject(flow, { owner: flow })
        if (page) {
            console.log("typeid run ok v=" + flow.v)
            page.destroy()
        } else {
            console.log("typeid run FAILED to create page")
        }
    }

    Timer {
        interval: 200
        running: true
        repeat: true
        onTriggered: flow.run()
    }
}
