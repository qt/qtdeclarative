// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls

Item {
    width: 360
    height: bar.height

//! [1]
TabBar {
    id: bar
    width: parent.width

    Repeater {
        model: ["First", "Second", "Third", "Fourth", "Fifth"]

        TabButton {
            text: modelData
            width: Math.max(100, bar.width / 5)
        }
    }
}
//! [1]
}
