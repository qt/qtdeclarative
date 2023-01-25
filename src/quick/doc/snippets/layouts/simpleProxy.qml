// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Layouts
import QtQuick.Window
import QtQuick.Controls

Window {
    visible: true

    width: 350
    //! [document]
    //! [item definition]
    Rectangle {
        id: rectangle1
        color: "tomato"
        Layout.fillHeight: true
        Layout.fillWidth: true
    }

    Rectangle {
        id: rectangle2
        color: "lightskyblue"
        Layout.fillHeight: true
        Layout.fillWidth: true
    }
    //! [item definition]

    //! [layout definition]
    GridLayout {
        id: l1
        columns: 1
        visible: false
        anchors.fill: parent
        LayoutItemProxy { target: rectangle1 }
        LayoutItemProxy { target: rectangle2 }
    }

    GridLayout {
        id: l2
        columns: 2
        visible: true
        anchors.fill: parent
        LayoutItemProxy { target: rectangle1 }
        LayoutItemProxy { target: rectangle2 }
    }
    //! [layout definition]

    //! [layout choice]
    onWidthChanged: {
        if (width < 300) {
            l2.visible = false
            l1.visible = true
        } else {
            l1.visible = false
            l2.visible = true
        }
    }
    //! [layout choice]
    //! [document]
}
