// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [entire]
import QtQuick
import QtQuick.Controls

Window {
    id: win
    flags: Qt.Window | Qt.WindowFullscreenButtonHint
    visibility: fullscreenButton.checked ? Window.FullScreen : Window.Windowed

    Button {
        id: fullscreenButton
        anchors {
            right: parent.right
            top: parent.top
            margins: 6
        }
        width: height
        checkable: true
        Binding on checked { value: win.visibility === Window.FullScreen }
        text: "⛶"
        ToolTip.visible: hovered
        ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
        ToolTip.text: win.visibility === Window.FullScreen ? qsTr("restore") : qsTr("fill screen")
    }
}
//! [entire]
