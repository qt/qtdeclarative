// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Shapes
import QtQuick.Controls

Rectangle {
    id: root
    width: 1024
    height: 768

    readonly property color col: "lightsteelblue"
    gradient: Gradient {
        GradientStop {
            position: 0.0
            color: Qt.tint(root.col, "#20FFFFFF")
        }
        GradientStop {
            position: 0.1
            color: Qt.tint(root.col, "#20AAAAAA")
        }
        GradientStop {
            position: 0.9
            color: Qt.tint(root.col, "#20666666")
        }
        GradientStop {
            position: 1.0
            color: Qt.tint(root.col, "#20000000")
        }
    }

    layer.enabled: msaaCheckBox.checked
    layer.samples: 4

    Item {
        id: zoomView
        anchors.fill: parent
        anchors.margins: 10
        anchors.leftMargin: 100
        property bool zoomedIn: false
        property rect zoomTarget: "0, 0, 100x100"
        property rect zoomRect: zoomTarget
        Behavior on zoomRect {
            id: zoomBehavior
            PropertyAnimation { id: zoomAnimation; duration: 800 }
        }
        property real zoomScale: Math.min(width / zoomRect.width, height / zoomRect.height)

        Item {
            width: loader1.width
            height: loader1.height
            scale: zoomView.zoomScale
            transformOrigin: Item.TopLeft
            Loader {
                id: loader1
                x: -zoomView.zoomRect.x
                y: -zoomView.zoomRect.y
                property rect br: item ? item.boundingRect : "0,0,100x100"
                source: "tiger.qml"
                asynchronous: true
                visible: status === Loader.Ready
                onLoaded: {
                    item.highlightOnTap = false
                    zoomView.zoomTarget = br
                    width = br
                    height = br
                }
            }
        }
    }
    MouseArea {
        anchors.fill: parent

        property vector2d clickPoint
        property vector2d zoomPoint
        property bool dragged: false
        property real targetSize: 30
        enabled: !zoomAnimation.running
        onPressed: (event) =>  {
                       clickPoint = Qt.vector2d(event.x, event.y)
                       zoomPoint = Qt.vector2d(zoomView.zoomRect.x, zoomView.zoomRect.y)
                       dragged = false
                       zoomBehavior.enabled = false
                   }
        onReleased: (event) => {
                        if (!dragged)
                        doTap(Qt.point(event.x, event.y))
                    }
        onPositionChanged: (event) => {
                               if (!dragged) {
                                   let delta = Qt.vector2d(event.x, event.y).minus(clickPoint)
                                   if (delta.length() < drag.threshold)
                                   return
                               }
                               dragged = true
                               if (zoomView.zoomedIn) {
                                   doDrag(Qt.point(event.x, event.y))
                               }
                           }
        function doDrag(pos) {
            let r = zoomView.zoomTarget
            let xx = zoomPoint.x - (pos.x - clickPoint.x) / zoomView.zoomScale
            let yy = zoomPoint.y - (pos.y - clickPoint.y) / zoomView.zoomScale
            zoomView.zoomTarget = Qt.rect(xx, yy, r.width, r.height)
        }
        function doTap(pos) {
            if (zoomView.zoomedIn) {
                zoomBehavior.enabled = true
                zoomView.zoomTarget = loader1.item.boundingRect
                zoomView.zoomedIn = false
            } else {
                let localPos = loader1.mapFromItem(root, pos)
                let xx = localPos.x - loader1.br.left
                let yy = localPos.y - loader1.br.top
                if (loader1.item.contains(Qt.point(xx,yy))) {
                    zoomBehavior.enabled = true
                    zoomView.zoomTarget = Qt.rect(xx - 184 - targetSize/2, yy - 144 - targetSize/2, targetSize, targetSize)
                    zoomView.zoomedIn = true
                }
            }
        }
    }

    Rectangle {
        opacity: 0.5
        color: "white"
        x: settings.childrenRect.x
        y: settings.childrenRect.y
        width: settings.childrenRect.width
        height:  settings.childrenRect.height
    }
    Column {
        id: settings
        RadioButton {
            id: curveRendererCheckBox
            text: "Curve Renderer [tech preview]"
            onCheckedChanged: {
                loader1.item.preferredRendererType = checked ? Shape.CurveRenderer : Shape.GeometryRenderer
            }
        }
        RadioButton {
            id: geometryRendererCheckBox
            text: "Geometry Renderer"
            checked: true
        }
        RadioButton {
            id: msaaCheckBox
            text: "Geometry Renderer 4x MSAA"
        }
    }
}
