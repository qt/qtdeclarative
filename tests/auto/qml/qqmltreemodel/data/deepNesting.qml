// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQml
import Qt.labs.qmlmodels

QtObject {
    id: root

    property TestModel testModel: TestModel {}

    function leaf(level) {
        return {
            checked: false,
            amount: level,
            fruitType: "Fruit",
            fruitName: "Level " + level,
            fruitPrice: 1.5,
            color: "green"
        }
    }

    // Returns a row that spans `levels` levels (1 = a row without children).
    // Built iteratively, so the test itself does not depend on stack depth.
    function chain(levels, makeRow) {
        let row = makeRow(levels)
        for (let level = levels - 1; level > 0; --level) {
            const parent = makeRow(level)
            parent.rows = [ row ]
            row = parent
        }
        return row
    }

    function setRowsWithDepth(levels) {
        testModel.rows = [ chain(levels, leaf) ]
    }

    // The following return true if the conversion of the rows ran out of stack
    // space. Only the nesting matters, so the rows are kept minimal.
    function setRowsTooDeep(levels) {
        try {
            testModel.rows = [ chain(levels, level => ({ amount: level })) ]
        } catch (e) {
            return e instanceof RangeError
        }
        return false
    }

    function appendRowTooDeep(levels) {
        try {
            testModel.appendRow(testModel.index([0], 0), chain(levels, level => ({ amount: level })))
        } catch (e) {
            return e instanceof RangeError
        }
        return false
    }
}
