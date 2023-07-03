// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Layouts
import shared as Examples
import "components"

Rectangle {
    id: window
    width: 800
    height: 800
    visible: true
    Examples.LauncherList {
        id: ll
        objectName: "LauncherList"
        anchors.fill: parent
        Component.onCompleted: {
            addExample("tap", "TapHandler: device-agnostic tap/click detection for buttons", Qt.resolvedUrl("tapHandler.qml"))
            addExample("multibuttons", "TapHandler: gesturePolicy (99 red balloons)", Qt.resolvedUrl("multibuttons.qml"))
            addExample("pieMenu", "TapHandler: pie menu", Qt.resolvedUrl("pieMenu.qml"))
            addExample("single point handler", "PointHandler: properties such as seat, device, modifiers, velocity, pressure", Qt.resolvedUrl("singlePointHandlerProperties.qml"))
            addExample("multiflame", "PointHandler: particle flames around touchpoints", Qt.resolvedUrl("multiflame.qml"))
            addExample("hover sidebar", "HoverHandler: a hierarchy of items sharing the hover state", Qt.resolvedUrl("sidebar.qml"))
            addExample("joystick", "DragHandler: move one item inside another with any pointing device", Qt.resolvedUrl("joystick.qml"))
            addExample("mixer", "DragHandler: drag multiple sliders with multiple fingers", Qt.resolvedUrl("mixer.qml"))
            addExample("fling animation", "DragHandler: after dragging, use an animation to simulate momentum", Qt.resolvedUrl("flingAnimation.qml"))
            addExample("pinch", "PinchHandler: scale, rotate and drag", Qt.resolvedUrl("pinchHandler.qml"))
            addExample("map", "scale, pan, re-render at different resolutions", Qt.resolvedUrl("map.qml"))
            addExample("corkboards", "editable, movable sticky notes in a ListView", Qt.resolvedUrl("corkboards.qml"))
            addExample("fake Flickable", "implementation of a simplified Flickable using only Items, DragHandler and MomentumAnimation", Qt.resolvedUrl("fakeFlickable.qml"))
            addExample("tablet canvas", "PointHandler and HoverHandler with a tablet: detect the stylus, and draw", Qt.resolvedUrl("tabletCanvasDrawing.qml"))
        }
    }
    Item {
        id: glassPane
        objectName: "glassPane"
        z: 10000
        anchors.fill: parent

        Instantiator {
            model: 10
            delegate: TouchpointFeedbackSprite { parent: glassPane }
        }
    }

    MouseFeedbackSprite { }
}
