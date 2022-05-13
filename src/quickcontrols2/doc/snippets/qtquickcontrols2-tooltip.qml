// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GFDL-1.3-no-invariants-only

import QtQuick
import QtQuick.Window
import QtQuick.Controls

Item {
    id: root
    width: 360
    height: button.height * 2

    property Button button: children[0]

    Binding { target: root.button; property: "down"; value: root.Window.active }
    Binding { target: root.button.anchors; property: "bottom"; value: root.bottom }
    Binding { target: root.button.anchors; property: "horizontalCenter"; value: root.horizontalCenter }

    //! [1]
    Button {
        text: qsTr("Save")

        ToolTip.visible: down
        ToolTip.text: qsTr("Save the active project")
    }
    //! [1]
}
