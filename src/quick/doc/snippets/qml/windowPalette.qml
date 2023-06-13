// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//![entire]
import QtQuick
import QtQuick.Controls

//![declaration-and-color]
Window {
    visible: true

    // here we use the Window.active and Window.palette ordinary properties
    color: active ? palette.active.window : palette.inactive.window
//![declaration-and-color]

    // colors that are not customized here come from SystemPalette
    palette.active.window: "peachpuff"
    palette.windowText: "brown"

    Text {
        anchors.centerIn: parent
        // here we use the Window.active attached property and the Item.palette property
        color: Window.active ? palette.active.windowText : palette.inactive.windowText
        text: Window.active ? "active" : "inactive"
    }

    Button {
        text: "Button"
        anchors {
            bottom: parent.bottom
            bottomMargin: 6
            horizontalCenter: parent.horizontalCenter
        }
    }
//![closing-brace]
}
//![closing-brace]
//![entire]
