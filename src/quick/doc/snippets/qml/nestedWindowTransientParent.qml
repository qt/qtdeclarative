// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick

//![0]
Window {
    // visible is false by default
    Window {
        transientParent: null
        visible: true
    }
//![0]

    id: outer
    Timer {
        interval: 2000
        running: true
        onTriggered: outer.visible = true
    }
//![1]
}
//![1]
