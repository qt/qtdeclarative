// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Shapes
import QtQuick.Controls
import QtQuick.Layouts

Window {
    id: mainWindow

    property real designWindowWidthRatio: topLevel.zoomRect.width / 997
    property real designWindowHeightRatio: topLevel.zoomRect.height / 1024
    property int preferredRendererType: curveRendererCheckBox.checked ? Shape.CurveRenderer : Shape.GeometryRenderer

    title: qsTr("Weather Forecast")
    width: 1280
    height: 960
    visible: true
    color: "white"

    FontLoader {
        id: workSansRegular
        source: "assets/WorkSans-Regular.ttf"
    }

    Rectangle {
        id: bgFill
        color: "#9FCBF9"
        anchors.fill: parent
    }

    Item {
        id: mapContainer
        anchors.left: !settingsDrawer.isLandscape ? parent.left : settingsDrawer.right
        anchors.top: !settingsDrawer.isLandscape ? settingsDrawer.bottom : parent.top
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        layer.enabled: msaaCheckBox.checked
        layer.samples: 4

        Item {
            property rect selectedRect
            property rect zoomTarget: Qt.rect(0, 0, mapshape.width, mapshape.height)
            property rect zoomRect: zoomTarget
            property real zoomScale:  Math.min(width / zoomRect.width, height / zoomRect.height)
            property real subScale: 1
            property rect overlayRect
            property real overlayRotation: 0

            anchors.fill: parent
            anchors.margins: 10
            id: topLevel
            transformOrigin: Item.TopLeft
            scale: zoomScale > 0 ?  zoomScale : 1
            Behavior on zoomRect { PropertyAnimation { duration: 500 } }

            Europe {
                id: mapshape
                x: (topLevel.width / topLevel.scale - topLevel.zoomRect.width) / 2  - topLevel.zoomRect.x
                y: (topLevel.height / topLevel.scale - topLevel.zoomRect.height) / 2 - topLevel.zoomRect.y
                onZoomTo: (child, name, childRect, textRect) => {
                              if (!child) {
                                  topLevel.zoomTarget = Qt.rect(0, 0, mapshape.width, mapshape.height)
                                  return
                              }
                              let x_in = childRect.x
                              let y_in = childRect.y
                              let w_in = childRect.width
                              let h_in = childRect.height

                              // try to center the target in the map
                              var x, y, w, h
                              let ar_in = h_in / w_in
                              let ar = height / width
                              if (ar_in < ar) {
                                  // in rect too short
                                  h = w_in * ar
                                  y = y_in - (h - h_in) / 2
                                  x = x_in
                                  w = w_in
                              } else {
                                  // in rect too narrow
                                  w = h_in / ar
                                  x = x_in - (w - w_in) / 2
                                  y = y_in
                                  h = h_in
                              }
                              topLevel.selectedRect = Qt.rect(x_in, y_in, w_in, h_in)
                              topLevel.subScale = Math.min(mapshape.width / w, mapshape.height / h)
                              topLevel.zoomTarget = Qt.rect(x, y, w, h)
                              if (textRect) {
                                  topLevel.overlayRect = Qt.rect(textRect.x, textRect.y, textRect.width, textRect.height)
                                  topLevel.overlayRotation = textRect.rotation
                              } else {
                                  topLevel.overlayRect = topLevel.zoomTarget
                                  topLevel.overlayRotation = 0
                              }
                              textLayer.label = name
                          }
                Item {
                    id: globalLayer
                    anchors.fill: parent
                    opacity: mapshape.zoomedIn ? 0 : 1
                    z: 2
                    Behavior on opacity {
                        NumberAnimation { duration: 500 }
                    }

                    Repeater {
                        model: ListModel {
                            ListElement {
                                itemSource: "Cloud.qml"
                                temperature: 22
                                placeName: "Oslo"
                                itemX: 0.440
                                itemY: 0.320
                            }

                            ListElement {
                                itemSource: "CloudWithLightning.qml"
                                placeName: "Helsinki"
                                temperature: 20
                                itemX: 0.615
                                itemY: 0.300
                            }

                            ListElement {
                                itemSource: "SunBehindCloud.qml"
                                placeName: "Berlin"
                                temperature: 24
                                itemX: 0.490
                                itemY: 0.540
                            }

                            ListElement {
                                itemSource: "SunBehindCloud.qml"
                                placeName: "London"
                                temperature: 22
                                itemX: 0.249
                                itemY: 0.550
                            }
                            ListElement {
                                itemSource: "SunBehindCloud.qml"
                                placeName: "Paris"
                                temperature: 22
                                itemX: 0.293
                                itemY: 0.630
                            }
                            ListElement {
                                itemSource: "SunBehindCloud.qml"
                                placeName: "Rome"
                                temperature: 22
                                itemX: 0.496
                                itemY: 0.827
                            }
                            ListElement {
                                itemSource: "SunBehindCloud.qml"
                                placeName: "Budapest"
                                temperature: 22
                                itemX: 0.612
                                itemY: 0.666
                            }
                            ListElement {
                                itemSource: "SunBehindCloud.qml"
                                placeName: "Sofia"
                                temperature: 22
                                itemX: 0.719
                                itemY: 0.781
                            }
                            ListElement {
                                itemSource: "SunBehindCloud.qml"
                                placeName: "Kyiv"
                                temperature: 22
                                itemX: 0.796
                                itemY: 0.543
                            }
                            ListElement {
                                itemSource: "Sun.qml"
                                placeName: "Madrid"
                                temperature: 28
                                itemX: 0.140
                                itemY: 0.830
                            }
                            ListElement {
                                itemSource: "CloudWithSnow.qml"
                                placeName: "Reykjavik"
                                temperature: 12
                                itemX: 0.050
                                itemY: 0.120
                            }
                        }

                        delegate: BouncyShape {
                            x: itemX * mapshape.width
                            y: itemY * mapshape.height
                            amount: 1.5
                            MapLabel {
                                iconSource: itemSource
                                label: placeName
                                degrees: temperature
                            }
                        }
                    }
                } // end globalLayer
            } // end Europe

            Item {
                id: textLayer
                opacity: mapshape.zoomedIn ? 1 : 0
                z: 2
                x: mapshape.x + topLevel.overlayRect.x
                y: mapshape.y + topLevel.overlayRect.y
                width: topLevel.overlayRect.width
                height: topLevel.overlayRect.height
                rotation: topLevel.overlayRotation
                property string label: ""
                property real localScale: 10 / topLevel.subScale

                Behavior on opacity {
                    NumberAnimation { duration: 800}
                }

                Item {
                    anchors.fill: parent
                    DemoShape {
                        id: textShape
                        property real s: Math.min(parent.width / boundingRect.width,
                                                  parent.height / boundingRect.height)

                        anchors.centerIn: parent
                        scale: Number.isNaN(s) ? 1 : 0.5 * s

//! [textShape]
                        ShapePath {
                            strokeColor:  "transparent"
                            strokeWidth: 1
                            joinStyle: ShapePath.RoundJoin
                            fillRule: ShapePath.WindingFill

                            fillGradient: RadialGradient {
                                centerX: textShape.width / 2
                                centerY: textShape.height / 2
                                centerRadius: textShape.width / 2
                                focalX: centerX; focalY: centerY
                                GradientStop { position: 0; color: "#b0ab9d7f" }
                                GradientStop { position: 1; color: "#5cab9d7f" }
                            }
                            PathText {
                                id: textPath
                                text: textLayer.label
                                font.family: workSansRegular.font.family
                                font.pixelSize: 64
                            }
                        }
//! [textShape]
                    }
                }
            }
            Item {
                id: localLayer
                property real localScale: 10 / topLevel.subScale

                opacity: mapshape.zoomedIn ? 1 : 0
                z: 2
                x: mapshape.x + topLevel.selectedRect.x
                y: mapshape.y + topLevel.selectedRect.y
                width: topLevel.selectedRect.width
                height: topLevel.selectedRect.height

                Item {
                    scale: localLayer.localScale
                    x: 2 * parent.width / 3
                    y: 2 * parent.height / 3
                    BouncyShape {
                        hoverEnabled: mapshape.zoomedIn
                        Sun {
                            width: 10
                            height: 10
                        }
                    }
                }
                Item {
                    scale: localLayer.localScale
                    x: parent.width / 10
                    y: parent.height / 10
                    BouncyShape {
                        hoverEnabled: mapshape.zoomedIn
                        CloudWithLightning {
                            width: 10
                            height: 10
                        }
                    }
                }
                Behavior on opacity { NumberAnimation{ duration: 800 } }
            }
        }
    }

    Rectangle {
        id: gear
        anchors.top: mapContainer.top
        anchors.left: mapContainer.left
        anchors.leftMargin: 30
        anchors.topMargin: 30
        radius: 8
        width: 44
        height: width
        color: Qt.rgba(0, 0, 0, hoverHandler.hovered ? 0.21 : 0.07)
//! [msaa]
        layer.enabled: msaaCheckBox.checked
        layer.samples: 4
//! [msaa]

        Gear {
            anchors.centerIn: parent
            width: 24
            height: width
        }

        HoverHandler {
            id: hoverHandler
        }

        TapHandler {
            onTapped: (eventPoint, button) => {
              settingsDrawer.isOpen = !settingsDrawer.isOpen
            }
        }
    }

    SettingsDrawer {
        id: settingsDrawer
        isLandscape: mainWindow.width > mainWindow.height
        width: isLandscape ? implicitWidth : mainWindow.width
        height: isLandscape ? mainWindow.height : mainWindow.height * 0.33

        RadioButton {
            id: curveRendererCheckBox
            text: qsTr("Curve Renderer")
            checked: true
        }

        RadioButton {
            id: geometryRendererCheckBox
            text: qsTr("Geometry Renderer")
        }
        RadioButton {
            id: msaaCheckBox
            text: qsTr("Geometry Renderer 4x MSAA")
        }
    }
}
