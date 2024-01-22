// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls

Window {
    width: 400
    height: 400

    contentItem.rotation: 90

    ComboBox {
        objectName: "first"
        x: 100
        y: 320 // is missing space, needs to unroll in the "mirrored" direction
        model: ["First", "Second", "Third", "Fourth", "Fifth"]
    }

    ComboBox {
        objectName: "second"
        x: 200
        y: 100 // has enough space to unroll
        model: ["A", "B", "C"]
    }
}
