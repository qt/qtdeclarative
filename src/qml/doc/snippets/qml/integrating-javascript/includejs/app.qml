// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
//![0]
import QtQuick
import "script.mjs" as MyScript

Item {
    width: 100; height: 100

    MouseArea {
        anchors.fill: parent
        onClicked: {
            MyScript.showCalculations(10)
            console.log("Call factorial() from QML:",
                MyScript.factorial(10))
        }
    }
}
//![0]
