// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [document]
import QtQuick

Rectangle {
    width: 400; height: 400; color: "black"

    Grid {
        x: 5; y: 5
        rows: 5; columns: 5; spacing: 10

        Repeater { model: 24
                   Rectangle { width: 70; height: 70
                               color: "lightgreen"

                               Text { text: index
                                      font.pointSize: 30
                                      anchors.centerIn: parent } }
        }
    }
}
//! [document]
