// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Window
import QtQuick.Controls

Item {
    id: root
    width: slider.width
    height: slider.height * 2.0

    Binding { target: slider.anchors; property: "centerIn"; value: root }
    Binding { target: slider.anchors; property: "verticalCenterOffset"; value: slider.height / 2 }
    Binding { target: slider; property: "pressed"; value: root.Window.active }

    //! [1]
    Slider {
        id: slider
        value: 0.5

        ToolTip {
            parent: slider.handle
            visible: slider.pressed
            text: slider.value.toFixed(1)
        }
    }
    //! [1]
}
