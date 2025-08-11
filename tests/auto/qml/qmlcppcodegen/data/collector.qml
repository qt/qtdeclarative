// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

pragma Strict
import QtQml
import TestTypes

Collector {
    id: self
    property Component c: QtObject { objectName: "dynamic" }
    property QtObject o: c.createObject()
    property QtObject o2
    property int gcRun: 0

    // Pass the object through a property once so that we know
    // it has a wrapper (missing wrapper will be fixed independently)
    property QtObject hidden: c.createObject()
    function os() : list<var> {
        let result = [hidden]
        hidden = null
        return result
    }

    Component.onCompleted: {
        var oo = o
        o = null
        var oso = os()

        // Hide this behind a property so that the side effect
        // can't be detected.
        var one = self.gc

        // Don't rely on QV4::Sequence to check its members, yet
        o2 = oso[0]
        o = oo

        // Actually use the value retrieved to trigger the gc
        // Otherwise the access is optimized out.
        gcRun = one
    }
}
