// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.14
import Qt.labs.animation 1.0
import "content"

Rectangle {
    id: root
    width: 1024; height: 600
    color: "#eee"

    CheckBox {
        id: cbTouchpadEnabled
        x: 10; y: 6
        label: "Touchpad wheel emulation enabled"
    }

    CheckBox {
        id: cbHorzWheelEnabled
        x: cbTouchpadEnabled.width + 20; y: 6
        label: "Horizontal wheel rotation"
    }

    function getTransformationDetails(item, pinchhandler) {
        return "\n\npinch.scale:" + pinchhandler.scale.toFixed(2)
                + "\npinch.rotation:" + pinchhandler.rotation.toFixed(2)
                + "°\npinch.activeTranslation:" + "(" + pinchhandler.activeTranslation.x.toFixed(2) + "," + pinchhandler.activeTranslation.y.toFixed(2) + ")"
                + "\npinch.persistentTranslation:" + "(" + pinchhandler.persistentTranslation.x.toFixed(2) + "," + pinchhandler.persistentTranslation.y.toFixed(2) + ")"
                + " item pos " + "(" + transformable.x.toFixed(2) + "," + transformable.y.toFixed(2) + ")"
                + "\nscale wheel.rotation:" + scaleWheelHandler.rotation.toFixed(2)
                + "°\nhorizontal wheel.rotation:" + horizontalRotationWheelHandler.rotation.toFixed(2)
                + "°\ncontrol-rotation wheel.rotation:" + controlRotationWheelHandler.rotation.toFixed(2)
                + "°\nrect.scale: " + item.scale.toFixed(2)
                + "\nrect.rotation: " + item.rotation.toFixed(2)
                + "°\nrect.position: " + "(" + item.x.toFixed(2) + "," + item.y.toFixed(2) + ")"
    }

    Rectangle {
        id: transformable
        width: parent.width - 100; height: parent.height - 100; x: 50; y: 50
        color: "#ffe0e0e0"
        antialiasing: true

        PinchHandler {
            id: parentPinch
            objectName: "parent pinch"
            minimumScale: 0.5
            maximumScale: 3
        }

        WheelHandler {
            id: scaleWheelHandler
            objectName: "mouse wheel for scaling"
            acceptedDevices: cbTouchpadEnabled.checked ? PointerDevice.Mouse | PointerDevice.TouchPad : PointerDevice.Mouse
            acceptedModifiers: Qt.NoModifier
            property: "scale"
            onActiveChanged: if (!active) sbr.returnToBounds();
            onWheel: console.log(objectName + ": rotation " + event.angleDelta.y + " scaled " + rotation + " @ " + point.position + " => " + parent.scale)
        }

        BoundaryRule on scale {
            id: sbr
            minimum: 0.1
            maximum: 2
            minimumOvershoot: 0.05
            maximumOvershoot: 0.05
        }

        BoundaryRule on rotation {
            id: rbr
            minimum: -90
            maximum: 360
            minimumOvershoot: 10
            maximumOvershoot: 10
        }

        WheelHandler {
            id: horizontalRotationWheelHandler
            enabled: cbHorzWheelEnabled.checked
            objectName: "horizontal mouse wheel for rotation"
            acceptedDevices: cbTouchpadEnabled.checked ? PointerDevice.Mouse | PointerDevice.TouchPad : PointerDevice.Mouse
            acceptedModifiers: Qt.NoModifier
            property: "rotation"
            orientation: Qt.Horizontal
            onActiveChanged: if (!active) rbr.returnToBounds()
            onWheel: console.log(objectName + ": rotation " + event.angleDelta.y + " scaled " + rotation + " @ " + point.position + " => " + parent.rotation)
        }

        WheelHandler {
            id: controlRotationWheelHandler
            objectName: "control-mouse wheel for rotation"
            acceptedDevices: cbTouchpadEnabled.checked ? PointerDevice.Mouse | PointerDevice.TouchPad : PointerDevice.Mouse
            acceptedModifiers: Qt.ControlModifier
            property: "rotation"
            orientation: Qt.Vertical // already the default
            // TODO returnToBounds() causes trouble because position isn't being adjusted when this happens
            onActiveChanged: if (!active) rbr.returnToBounds()
            onWheel: console.log(objectName + ": rotation " + event.angleDelta.y + " scaled " + rotation + " @ " + point.position + " => " + parent.rotation)
        }

        HoverHandler {
            id: hover
            acceptedDevices: PointerDevice.AllDevices
            property var scenePoint: transformable.mapToItem(root, point.position.x, point.position.y)
        }

        Text {
            text: "Pinch with 2 fingers to scale, rotate and translate\nMouse wheel to scale, Ctrl+mouse wheel or horizontal wheel to rotate"
                  + getTransformationDetails(parent, parentPinch)
        }
    }

    Rectangle {
        width: 1; height: parent.height
        color: "blue"
        x: hover.scenePoint.x
        Text {
            x: implicitWidth / -2; style: Text.Outline; styleColor: "white"
            y: 30
            color: "blue"
            text: "outer " + parent.x.toFixed(2) + " inner " + hover.point.position.x.toFixed(2)
        }
    }

    Rectangle {
        width: parent.width; height: 1
        color: "blue"
        y: hover.scenePoint.y
        Text {
            x: 45
            y: implicitHeight / -2; style: Text.Outline; styleColor: "white"
            color: "blue"
            text: "outer " + parent.y.toFixed(2) + " inner " + hover.point.position.y.toFixed(2)
        }
    }
}
