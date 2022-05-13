// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls

Column {
    id: root
    spacing: 8

    Label {
        text: "Total number of screens: " + screenInfo.count
        font.bold: true
    }

    Flow {
        spacing: 12
        width: parent.width

        Repeater {
            id: screenInfo
            model: (Qt.application as Application).screens
            Label {
                required property string name
                required property int virtualX
                required property int virtualY
                required property var modelData // avoid shadowing Label.width and height

                lineHeight: 1.5
                text: name + "\n" + virtualX + ", " + virtualY + " " + modelData.width + "x" + modelData.height
            }
        }
    }

    Component.onCompleted: {
        var screens = (Qt.application as Application).screens;
        for (var i = 0; i < screens.length; ++i)
            console.log("screen " + screens[i].name + " has geometry " +
                        screens[i].virtualX + ", " + screens[i].virtualY + " " +
                        screens[i].width + "x" + screens[i].height)
    }
}
