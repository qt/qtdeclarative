// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQml
import Test

BindablePoint {
    id: root
    property int aaChanges: 0
    property int bbChanges: 0
    property alias aa: root.point.x
    property alias bb: root.point.y
    onAaChanged: ++aaChanges
    bb: objectName
    onBbChanged: ++bbChanges
    objectName: "14"

    function reassign() {
        bb = Qt.binding(function() { return 16 + 0.5 });
    }
}
