// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [document]
import QtQuick

Rectangle {
    width: 320; height: 110
    color: "#c0c0c0"

    Row {
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.verticalCenter: parent.verticalCenter

        spacing: 5

        Rectangle { width: 100; height: 100; radius: 20.0
                    color: "#024c1c" }
        Rectangle { width: 100; height: 100; radius: 20.0
                    color: "#42a51c" }
        Rectangle { width: 100; height: 100; radius: 20.0
                    color: "white" }
    }
}
//! [document]
