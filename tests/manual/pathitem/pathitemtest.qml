/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQuick module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

import QtQuick 2.9
import Qt.labs.pathitem 1.0

Rectangle {
    id: root
    width: 1024
    height: 768

    property color col: "lightsteelblue"
    gradient: Gradient {
        GradientStop { position: 0.0; color: Qt.tint(root.col, "#20FFFFFF") }
        GradientStop { position: 0.1; color: Qt.tint(root.col, "#20AAAAAA") }
        GradientStop { position: 0.9; color: Qt.tint(root.col, "#20666666") }
        GradientStop { position: 1.0; color: Qt.tint(root.col, "#20000000") }
    }

    Row {
        anchors.top: parent.top
        anchors.centerIn: parent
        spacing: 20

        Column {
            spacing: 20

            Rectangle {
                border.color: "purple"
                color: "transparent"
                width: 200
                height: 100
                PathItem {
                    id: triangle
                    anchors.fill: parent
                    VisualPath {
                        strokeWidth: 4
                        strokeColor: "red"
                        fillGradient: PathLinearGradient {
                            x1: 0; y1: 0
                            x2: 200; y2: 100
                            PathGradientStop { position: 0; color: "blue" }
                            PathGradientStop { position: 0.2; color: "green" }
                            PathGradientStop { position: 0.4; color: "red" }
                            PathGradientStop { position: 0.6; color: "yellow" }
                            PathGradientStop { position: 1; color: "cyan" }
                        }
                        fillColor: "blue" // ignored with the gradient set
                        strokeStyle: VisualPath.DashLine
                        dashPattern: [ 1, 4 ]
                        Path {
                            PathLine { x: 200; y: 100 }
                            PathLine { x: 0; y: 100 }
                            PathLine { x: 0; y: 0 }
                        }
                    }
                    transform: Rotation { origin.x: 100; origin.y: 50; axis { x: 0; y: 1; z: 0 }
                        SequentialAnimation on angle {
                            NumberAnimation { from: 0; to: 75; duration: 2000 }
                            NumberAnimation { from: 75; to: -75; duration: 4000 }
                            NumberAnimation { from: -75; to: 0; duration: 2000 }
                            loops: Animation.Infinite
                        }
                    }
                }
            }

            Rectangle {
                border.color: "purple"
                color: "transparent"
                width: 200
                height: 100
                PathItem {
                    anchors.fill: parent
                    VisualPath {
                        id: someCurve
                        property color sc: "gray"
                        strokeColor: sc
                        property color fc: "yellow"
                        fillColor: fc
                        Path {
                            startX: 20; startY: 10
                            PathQuad { x: 50; y: 80; controlX: 0; controlY: 80 }
                            PathLine { x: 150; y: 80 }
                            PathQuad { x: 180; y: 10; controlX: 200; controlY: 80 }
                            PathLine { x: 20; y: 10 }
                        }
                        // Dynamic changes via property bindings etc. all work but can
                        // be computationally expense with the generic backend for properties
                        // that need retriangulating on every change. Should be cheap with NVPR.
                        NumberAnimation on strokeWidth {
                            from: 1; to: 20; duration: 10000
                        }
                    }
                }
                // Changing colors for a solid stroke or fill is simple and
                // (relatively) cheap. However, changing to/from transparent
                // stroke/fill color and stroke width 0 are special as these
                // change the scenegraph node tree (with the generic backend).
                Timer {
                    interval: 2000
                    running: true
                    repeat: true
                    onTriggered: someCurve.fillColor = (someCurve.fillColor === someCurve.fc ? "transparent" : someCurve.fc)
                }
                Timer {
                    interval: 1000
                    running: true
                    repeat: true
                    onTriggered: someCurve.strokeColor = (someCurve.strokeColor === someCurve.sc ? "transparent" : someCurve.sc)
                }
            }

            Rectangle {
                border.color: "purple"
                color: "transparent"
                width: 300
                height: 100
                PathItem {
                    id: linesAndMoves
                    anchors.fill: parent
                    VisualPath {
                        strokeColor: "black"
                        Path {
                            startX: 0; startY: 50
                            PathLine { relativeX: 100; y: 50 }
                            PathMove { relativeX: 100; y: 50 }
                            PathLine { relativeX: 100; y: 50 }
                        }
                    }
                }
            }

            Rectangle {
                border.color: "purple"
                color: "transparent"
                width: 200
                height: 120
                PathItem {
                    anchors.fill: parent
                    VisualPath {
                        id: joinTest
                        strokeColor: "black"
                        strokeWidth: 16
                        fillColor: "transparent"
                        capStyle: VisualPath.RoundCap
                        Path {
                            startX: 30
                            startY: 30
                            PathLine { x: 100; y: 100 }
                            PathLine { x: 30; y: 100 }
                        }
                    }
                }
                Timer {
                    interval: 1000
                    repeat: true
                    running: true
                    property variant styles: [ VisualPath.BevelJoin, VisualPath.MiterJoin, VisualPath.RoundJoin ]
                    onTriggered: {
                        for (var i = 0; i < styles.length; ++i)
                            if (styles[i] === joinTest.joinStyle) {
                                joinTest.joinStyle = styles[(i + 1) % styles.length];
                                break;
                            }
                    }
                }
            }

            Rectangle {
                border.color: "purple"
                color: "transparent"
                width: 200
                height: 100
                PathItem {
                    anchors.fill: parent
                    VisualPath {
                        id: star
                        strokeColor: "blue"
                        fillColor: "lightGray"
                        strokeWidth: 2
                        Path {
                            PathMove { x: 90; y: 50 }
                            PathLine { x: 50 + 40 * Math.cos(0.8 * 1 * Math.PI); y: 50 + 40 * Math.sin(0.8 * 1 * Math.PI) }
                            PathLine { x: 50 + 40 * Math.cos(0.8 * 2 * Math.PI); y: 50 + 40 * Math.sin(0.8 * 2 * Math.PI) }
                            PathLine { x: 50 + 40 * Math.cos(0.8 * 3 * Math.PI); y: 50 + 40 * Math.sin(0.8 * 3 * Math.PI) }
                            PathLine { x: 50 + 40 * Math.cos(0.8 * 4 * Math.PI); y: 50 + 40 * Math.sin(0.8 * 4 * Math.PI) }
                            PathLine { x: 90; y: 50 }
                        }
                    }
                }
                Timer {
                    interval: 1000
                    onTriggered: star.fillRule = (star.fillRule === VisualPath.OddEvenFill ? VisualPath.WindingFill : VisualPath.OddEvenFill)
                    repeat: true
                    running: true
                }
            }

            Rectangle {
                border.color: "purple"
                color: "transparent"
                width: 200
                height: 100
                PathItem {
                    anchors.fill: parent
                    VisualPath {
                        strokeWidth: 4
                        strokeColor: "black"
                        fillColor: "transparent"
                        Path {
                            startX: 20; startY: 10
                            PathCubic {
                                id: cb
                                x: 180; y: 10
                                control1X: -10; control1Y: 90; control2Y: 90
                                NumberAnimation on control2X { from: 400; to: 0; duration: 5000; loops: Animation.Infinite }
                            }
                        }
                    }
                }
            }
        }

        Column {
            spacing: 20

            Rectangle {
                border.color: "purple"
                color: "transparent"
                width: 200
                height: 100
                PathItem {
                    anchors.fill: parent
                    VisualPath {
                        fillColor: "transparent"
                        strokeColor: "red"
                        strokeWidth: 4
                        Path {
                            startX: 10; startY: 40
                            PathArc {
                                x: 10; y: 60
                                radiusX: 40; radiusY: 40
                                useLargeArc: true
                            }
                        }
                    }
                }
            }

            Rectangle {
                border.color: "purple"
                color: "transparent"
                width: 200
                height: 200
                Rectangle {
                    anchors.centerIn: parent
                    // have a size smaller than 150x150
                    width: 100
                    height: 100
                    // and enable clipping. Normally this goes via scissoring, unless
                    // some transform triggers the stencil-based path. Ensure this via rotation.
                    clip: true
                    NumberAnimation on rotation {
                        from: 0; to: 360; duration: 5000; loops: Animation.Infinite
                    }

                    PathItem {
                        width: 150
                        height: 150

                        VisualPath {
                            fillColor: "blue"
                            strokeColor: "red"
                            strokeWidth: 4
                            Path {
                                startX: 10; startY: 10
                                PathLine { x: 140; y: 140 }
                                PathLine { x: 10; y: 140 }
                                PathLine { x: 10; y: 10 }
                            }
                        }
                    }
                }
            }

            // stencil clip test #2, something more complicated:
            Rectangle {
                border.color: "purple"
                color: "transparent"
                width: 150
                height: 150
                Rectangle {
                    anchors.centerIn: parent
                    width: 60
                    height: 60
                    clip: true
                    NumberAnimation on rotation {
                        from: 0; to: 360; duration: 5000; loops: Animation.Infinite
                    }
                    PathItem {
                        width: 100
                        height: 100
                        VisualPath {
                            id: clippedStar
                            strokeColor: "blue"
                            fillColor: "lightGray"
                            strokeWidth: 2
                            Path {
                                PathMove { x: 90; y: 50 }
                                PathLine { x: 50 + 40 * Math.cos(0.8 * 1 * Math.PI); y: 50 + 40 * Math.sin(0.8 * 1 * Math.PI) }
                                PathLine { x: 50 + 40 * Math.cos(0.8 * 2 * Math.PI); y: 50 + 40 * Math.sin(0.8 * 2 * Math.PI) }
                                PathLine { x: 50 + 40 * Math.cos(0.8 * 3 * Math.PI); y: 50 + 40 * Math.sin(0.8 * 3 * Math.PI) }
                                PathLine { x: 50 + 40 * Math.cos(0.8 * 4 * Math.PI); y: 50 + 40 * Math.sin(0.8 * 4 * Math.PI) }
                                PathLine { x: 90; y: 50 }
                            }
                        }
                    }
                    Timer {
                        interval: 1000
                        onTriggered: clippedStar.fillRule = (clippedStar.fillRule === VisualPath.OddEvenFill ? VisualPath.WindingFill : VisualPath.OddEvenFill)
                        repeat: true
                        running: true
                    }
                }
            }

            Rectangle {
                border.color: "purple"
                color: "transparent"
                width: 100
                height: 100
                PathItem {
                    anchors.fill: parent
                    VisualPath {
                        strokeColor: "red"
                        Path {
                            PathLine { x: 100; y: 100 }
                        }
                    }
                    VisualPath {
                        strokeColor: "blue"
                        Path {
                            startX: 100; startY: 0
                            PathLine { x: 0; y: 100 }
                        }
                    }
                }
            }

            Rectangle {
                border.color: "purple"
                color: "transparent"
                width: 100
                height: 100

                PathLinearGradient {
                    id: refGrad
                    x1: 0; y1: 0
                    x2: 200; y2: 100
                    PathGradientStop { position: 0; color: "blue" }
                    PathGradientStop { position: 0.2; color: "green" }
                    PathGradientStop { position: 0.4; color: "red" }
                    PathGradientStop { position: 0.6; color: "yellow" }
                    PathGradientStop { position: 1; color: "cyan" }
                }

                PathItem {
                    id: jsApiItem
                    anchors.fill: parent
                    asynchronous: true

                    Component.onCompleted: {
                        clearVisualPaths();

                        var path = newPath();
                        var sfp = newStrokeFillParams();
                        sfp.strokeColor = "red";
                        path.lineTo(100, 100);
                        appendVisualPath(path, sfp)

                        path.clear();
                        sfp.clear();
                        sfp.strokeColor = "blue";
                        path.moveTo(100, 0);
                        path.lineTo(0, 100);
                        appendVisualPath(path, sfp)

                        path.clear();
                        sfp.clear();
                        sfp.strokeColor = "red"
                        sfp.strokeWidth = 4;
                        sfp.fillGradient = refGrad;
                        path.moveTo(10, 40);
                        path.arcTo(40, 40, 0, 10, 60, true, true);
                        appendVisualPath(path, sfp);

                        commitVisualPaths();
                    }
                }
            }
        }
    }

    Rectangle {
        id: stackTestRect
        SequentialAnimation on opacity {
            NumberAnimation { from: 0; to: 1; duration: 5000 }
            PauseAnimation { duration: 2000 }
            NumberAnimation { from: 1; to: 0; duration: 5000 }
            PauseAnimation { duration: 2000 }
            loops: Animation.Infinite
            id: opAnim
        }
        color: "blue"
        anchors.margins: 10
        anchors.fill: parent
    }
    MouseArea {
        anchors.fill: parent
        onClicked: stackTestRect.visible = !stackTestRect.visible
    }

    MouseArea {
        width: 200
        height: 200
        anchors.right: parent.right
        anchors.top: parent.top
        onClicked: {
            console.log("Roar!");

            jsApiItem.clearVisualPaths();

            var p = jsApiItem.newPath(); var sfp = jsApiItem.newStrokeFillParams();
            sfp.fillColor =  "#ffffff";
            sfp.strokeColor = "#000000";
            sfp.strokeWidth = 0.172;
            p.moveTo(-122.304, 84.285);
            p.cubicTo(-122.304, 84.285, -122.203, 86.179, -123.027, 86.16);
            p.cubicTo(-123.851, 86.141, -140.305, 38.066, -160.833, 40.309);
            p.cubicTo(-160.833, 40.309, -143.05, 32.956, -122.304, 84.285);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#ffffff";
            sfp.strokeColor = "#000000";
            sfp.strokeWidth = 0.172;
            p.moveTo(-118.774, 81.262);
            p.cubicTo(-118.774, 81.262, -119.323, 83.078, -120.092, 82.779);
            p.cubicTo(-120.86, 82.481, -119.977, 31.675, -140.043, 26.801);
            p.cubicTo(-140.043, 26.801, -120.82, 25.937, -118.774, 81.262);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#ffffff";
            sfp.strokeColor = "#000000";
            sfp.strokeWidth = 0.172;
            p.moveTo(-91.284, 123.59);
            p.cubicTo(-91.284, 123.59, -89.648, 124.55, -90.118, 125.227);
            p.cubicTo(-90.589, 125.904, -139.763, 113.102, -149.218, 131.459);
            p.cubicTo(-149.218, 131.459, -145.539, 112.572, -91.284, 123.59);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#ffffff";
            sfp.strokeColor = "#000000";
            sfp.strokeWidth = 0.172;
            p.moveTo(-94.093, 133.801);
            p.cubicTo(-94.093, 133.801, -92.237, 134.197, -92.471, 134.988);
            p.cubicTo(-92.704, 135.779, -143.407, 139.121, -146.597, 159.522);
            p.cubicTo(-146.597, 159.522, -149.055, 140.437, -94.093, 133.801);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#ffffff";
            sfp.strokeColor = "#000000";
            sfp.strokeWidth = 0.172;
            p.moveTo(-98.304, 128.276);
            p.cubicTo(-98.304, 128.276, -96.526, 128.939, -96.872, 129.687);
            p.cubicTo(-97.218, 130.435, -147.866, 126.346, -153.998, 146.064);
            p.cubicTo(-153.998, 146.064, -153.646, 126.825, -98.304, 128.276);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#ffffff";
            sfp.strokeColor = "#000000";
            sfp.strokeWidth = 0.172;
            p.moveTo(-109.009, 110.072);
            p.cubicTo(-109.009, 110.072, -107.701, 111.446, -108.34, 111.967);
            p.cubicTo(-108.979, 112.488, -152.722, 86.634, -166.869, 101.676);
            p.cubicTo(-166.869, 101.676, -158.128, 84.533, -109.009, 110.072);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#ffffff";
            sfp.strokeColor = "#000000";
            sfp.strokeWidth = 0.172;
            p.moveTo(-116.554, 114.263);
            p.cubicTo(-116.554, 114.263, -115.098, 115.48, -115.674, 116.071);
            p.cubicTo(-116.25, 116.661, -162.638, 95.922, -174.992, 112.469);
            p.cubicTo(-174.992, 112.469, -168.247, 94.447, -116.554, 114.263);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#ffffff";
            sfp.strokeColor = "#000000";
            sfp.strokeWidth = 0.172;
            p.moveTo(-119.154, 118.335);
            p.cubicTo(-119.154, 118.335, -117.546, 119.343, -118.036, 120.006);
            p.cubicTo(-118.526, 120.669, -167.308, 106.446, -177.291, 124.522);
            p.cubicTo(-177.291, 124.522, -173.066, 105.749, -119.154, 118.335);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#ffffff";
            sfp.strokeColor = "#000000";
            sfp.strokeWidth = 0.172;
            p.moveTo(-108.42, 118.949);
            p.cubicTo(-108.42, 118.949, -107.298, 120.48, -107.999, 120.915);
            p.cubicTo(-108.7, 121.35, -148.769, 90.102, -164.727, 103.207);
            p.cubicTo(-164.727, 103.207, -153.862, 87.326, -108.42, 118.949);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#ffffff";
            sfp.strokeColor = "#000000";
            sfp.strokeWidth = 0.172;
            p.moveTo(-128.2, 90);
            p.cubicTo(-128.2, 90, -127.6, 91.8, -128.4, 92);
            p.cubicTo(-129.2, 92.2, -157.8, 50.2, -177.001, 57.8);
            p.cubicTo(-177.001, 57.8, -161.8, 46, -128.2, 90);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#ffffff";
            sfp.strokeColor = "#000000";
            sfp.strokeWidth = 0.172;
            p.moveTo(-127.505, 96.979);
            p.cubicTo(-127.505, 96.979, -126.53, 98.608, -127.269, 98.975);
            p.cubicTo(-128.007, 99.343, -164.992, 64.499, -182.101, 76.061);
            p.cubicTo(-182.101, 76.061, -169.804, 61.261, -127.505, 96.979);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#ffffff";
            sfp.strokeColor = "#000000";
            sfp.strokeWidth = 0.172;
            p.moveTo(-127.62, 101.349);
            p.cubicTo(-127.62, 101.349, -126.498, 102.88, -127.199, 103.315);
            p.cubicTo(-127.9, 103.749, -167.969, 72.502, -183.927, 85.607);
            p.cubicTo(-183.927, 85.607, -173.062, 69.726, -127.62, 101.349);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#ffffff";
            sfp.strokeColor = "#000000";
            sfp.strokeWidth = 1;
            p.moveTo(-129.83, 103.065);
            p.cubicTo(-129.327, 109.113, -128.339, 115.682, -126.6, 118.801);
            p.cubicTo(-126.6, 118.801, -130.2, 131.201, -121.4, 144.401);
            p.cubicTo(-121.4, 144.401, -121.8, 151.601, -120.2, 154.801);
            p.cubicTo(-120.2, 154.801, -116.2, 163.201, -111.4, 164.001);
            p.cubicTo(-107.516, 164.648, -98.793, 167.717, -88.932, 169.121);
            p.cubicTo(-88.932, 169.121, -71.8, 183.201, -75, 196.001);
            p.cubicTo(-75, 196.001, -75.4, 212.401, -79, 214.001);
            p.cubicTo(-79, 214.001, -67.4, 202.801, -77, 219.601);
            p.lineTo(-81.4, 238.401);
            p.cubicTo(-81.4, 238.401, -55.8, 216.801, -71.4, 235.201);
            p.lineTo(-81.4, 261.201);
            p.cubicTo(-81.4, 261.201, -61.8, 242.801, -69, 251.201);
            p.lineTo(-72.2, 260.001);
            p.cubicTo(-72.2, 260.001, -29, 232.801, -59.8, 262.401);
            p.cubicTo(-59.8, 262.401, -51.8, 258.801, -47.4, 261.601);
            p.cubicTo(-47.4, 261.601, -40.6, 260.401, -41.4, 262.001);
            p.cubicTo(-41.4, 262.001, -62.2, 272.401, -65.8, 290.801);
            p.cubicTo(-65.8, 290.801, -57.4, 280.801, -60.6, 291.601);
            p.lineTo(-60.2, 303.201);
            p.cubicTo(-60.2, 303.201, -56.2, 281.601, -56.6, 319.201);
            p.cubicTo(-56.6, 319.201, -37.4, 301.201, -49, 322.001);
            p.lineTo(-49, 338.801);
            p.cubicTo(-49, 338.801, -33.8, 322.401, -40.2, 335.201);
            p.cubicTo(-40.2, 335.201, -30.2, 326.401, -34.2, 341.601);
            p.cubicTo(-34.2, 341.601, -35, 352.001, -30.6, 340.801);
            p.cubicTo(-30.6, 340.801, -14.6, 310.201, -20.6, 336.401);
            p.cubicTo(-20.6, 336.401, -21.4, 355.601, -16.6, 340.801);
            p.cubicTo(-16.6, 340.801, -16.2, 351.201, -7, 358.401);
            p.cubicTo(-7, 358.401, -8.2, 307.601, 4.6, 343.601);
            p.lineTo(8.6, 360.001);
            p.cubicTo(8.6, 360.001, 11.4, 350.801, 11, 345.601);
            p.cubicTo(11, 345.601, 25.8, 329.201, 19, 353.601);
            p.cubicTo(19, 353.601, 34.2, 330.801, 31, 344.001);
            p.cubicTo(31, 344.001, 23.4, 360.001, 25, 364.801);
            p.cubicTo(25, 364.801, 41.8, 330.001, 43, 328.401);
            p.cubicTo(43, 328.401, 41, 370.802, 51.8, 334.801);
            p.cubicTo(51.8, 334.801, 57.4, 346.801, 54.6, 351.201);
            p.cubicTo(54.6, 351.201, 62.6, 343.201, 61.8, 340.001);
            p.cubicTo(61.8, 340.001, 66.4, 331.801, 69.2, 345.401);
            p.cubicTo(69.2, 345.401, 71, 354.801, 72.6, 351.601);
            p.cubicTo(72.6, 351.601, 76.6, 375.602, 77.8, 352.801);
            p.cubicTo(77.8, 352.801, 79.4, 339.201, 72.2, 327.601);
            p.cubicTo(72.2, 327.601, 73, 324.401, 70.2, 320.401);
            p.cubicTo(70.2, 320.401, 83.8, 342.001, 76.6, 313.201);
            p.cubicTo(76.6, 313.201, 87.801, 321.201, 89.001, 321.201);
            p.cubicTo(89.001, 321.201, 75.4, 298.001, 84.2, 302.801);
            p.cubicTo(84.2, 302.801, 79, 292.401, 97.001, 304.401);
            p.cubicTo(97.001, 304.401, 81, 288.401, 98.601, 298.001);
            p.cubicTo(98.601, 298.001, 106.601, 304.401, 99.001, 294.401);
            p.cubicTo(99.001, 294.401, 84.6, 278.401, 106.601, 296.401);
            p.cubicTo(106.601, 296.401, 118.201, 312.801, 119.001, 315.601);
            p.cubicTo(119.001, 315.601, 109.001, 286.401, 104.601, 283.601);
            p.cubicTo(104.601, 283.601, 113.001, 247.201, 154.201, 262.801);
            p.cubicTo(154.201, 262.801, 161.001, 280.001, 165.401, 261.601);
            p.cubicTo(165.401, 261.601, 178.201, 255.201, 189.401, 282.801);
            p.cubicTo(189.401, 282.801, 193.401, 269.201, 192.601, 266.401);
            p.cubicTo(192.601, 266.401, 199.401, 267.601, 198.601, 266.401);
            p.cubicTo(198.601, 266.401, 211.801, 270.801, 213.001, 270.001);
            p.cubicTo(213.001, 270.001, 219.801, 276.801, 220.201, 273.201);
            p.cubicTo(220.201, 273.201, 229.401, 276.001, 227.401, 272.401);
            p.cubicTo(227.401, 272.401, 236.201, 288.001, 236.601, 291.601);
            p.lineTo(239.001, 277.601);
            p.lineTo(241.001, 280.401);
            p.cubicTo(241.001, 280.401, 242.601, 272.801, 241.801, 271.601);
            p.cubicTo(241.001, 270.401, 261.801, 278.401, 266.601, 299.201);
            p.lineTo(268.601, 307.601);
            p.cubicTo(268.601, 307.601, 274.601, 292.801, 273.001, 288.801);
            p.cubicTo(273.001, 288.801, 278.201, 289.601, 278.601, 294.001);
            p.cubicTo(278.601, 294.001, 282.601, 270.801, 277.801, 264.801);
            p.cubicTo(277.801, 264.801, 282.201, 264.001, 283.401, 267.601);
            p.lineTo(283.401, 260.401);
            p.cubicTo(283.401, 260.401, 290.601, 261.201, 290.601, 258.801);
            p.cubicTo(290.601, 258.801, 295.001, 254.801, 297.001, 259.601);
            p.cubicTo(297.001, 259.601, 284.601, 224.401, 303.001, 243.601);
            p.cubicTo(303.001, 243.601, 310.201, 254.401, 306.601, 235.601);
            p.cubicTo(303.001, 216.801, 299.001, 215.201, 303.801, 214.801);
            p.cubicTo(303.801, 214.801, 304.601, 211.201, 302.601, 209.601);
            p.cubicTo(300.601, 208.001, 303.801, 209.601, 303.801, 209.601);
            p.cubicTo(303.801, 209.601, 308.601, 213.601, 303.401, 191.601);
            p.cubicTo(303.401, 191.601, 309.801, 193.201, 297.801, 164.001);
            p.cubicTo(297.801, 164.001, 300.601, 161.601, 296.601, 153.201);
            p.cubicTo(296.601, 153.201, 304.601, 157.601, 307.401, 156.001);
            p.cubicTo(307.401, 156.001, 307.001, 154.401, 303.801, 150.401);
            p.cubicTo(303.801, 150.401, 282.201, 95.6, 302.601, 117.601);
            p.cubicTo(302.601, 117.601, 314.451, 131.151, 308.051, 108.351);
            p.cubicTo(308.051, 108.351, 298.94, 84.341, 299.717, 80.045);
            p.lineTo(-129.83, 103.065);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#cc7226";
            sfp.strokeColor = "#000000";
            sfp.strokeWidth = 1;
            p.moveTo(299.717, 80.245);
            p.cubicTo(300.345, 80.426, 302.551, 81.55, 303.801, 83.2);
            p.cubicTo(303.801, 83.2, 310.601, 94, 305.401, 75.6);
            p.cubicTo(305.401, 75.6, 296.201, 46.8, 305.001, 58);
            p.cubicTo(305.001, 58, 311.001, 65.2, 307.801, 51.6);
            p.cubicTo(303.936, 35.173, 301.401, 28.8, 301.401, 28.8);
            p.cubicTo(301.401, 28.8, 313.001, 33.6, 286.201, -6);
            p.lineTo(295.001, -2.4);
            p.cubicTo(295.001, -2.4, 275.401, -42, 253.801, -47.2);
            p.lineTo(245.801, -53.2);
            p.cubicTo(245.801, -53.2, 284.201, -91.2, 271.401, -128);
            p.cubicTo(271.401, -128, 264.601, -133.2, 255.001, -124);
            p.cubicTo(255.001, -124, 248.601, -119.2, 242.601, -120.8);
            p.cubicTo(242.601, -120.8, 211.801, -119.6, 209.801, -119.6);
            p.cubicTo(207.801, -119.6, 173.001, -156.8, 107.401, -139.2);
            p.cubicTo(107.401, -139.2, 102.201, -137.2, 97.801, -138.4);
            p.cubicTo(97.801, -138.4, 79.4, -154.4, 30.6, -131.6);
            p.cubicTo(30.6, -131.6, 20.6, -129.6, 19, -129.6);
            p.cubicTo(17.4, -129.6, 14.6, -129.6, 6.6, -123.2);
            p.cubicTo(-1.4, -116.8, -1.8, -116, -3.8, -114.4);
            p.cubicTo(-3.8, -114.4, -20.2, -103.2, -25, -102.4);
            p.cubicTo(-25, -102.4, -36.6, -96, -41, -86);
            p.lineTo(-44.6, -84.8);
            p.cubicTo(-44.6, -84.8, -46.2, -77.6, -46.6, -76.4);
            p.cubicTo(-46.6, -76.4, -51.4, -72.8, -52.2, -67.2);
            p.cubicTo(-52.2, -67.2, -61, -61.2, -60.6, -56.8);
            p.cubicTo(-60.6, -56.8, -62.2, -51.6, -63, -46.8);
            p.cubicTo(-63, -46.8, -70.2, -42, -69.4, -39.2);
            p.cubicTo(-69.4, -39.2, -77, -25.2, -75.8, -18.4);
            p.cubicTo(-75.8, -18.4, -82.2, -18.8, -85, -16.4);
            p.cubicTo(-85, -16.4, -85.8, -11.6, -87.4, -11.2);
            p.cubicTo(-87.4, -11.2, -90.2, -10, -87.8, -6);
            p.cubicTo(-87.8, -6, -89.4, -3.2, -89.8, -1.6);
            p.cubicTo(-89.8, -1.6, -89, 1.2, -93.4, 6.8);
            p.cubicTo(-93.4, 6.8, -99.8, 25.6, -97.8, 30.8);
            p.cubicTo(-97.8, 30.8, -97.4, 35.6, -100.2, 37.2);
            p.cubicTo(-100.2, 37.2, -103.8, 36.8, -95.4, 48.8);
            p.cubicTo(-95.4, 48.8, -94.6, 50, -97.8, 52.4);
            p.cubicTo(-97.8, 52.4, -115, 56, -117.4, 72.4);
            p.cubicTo(-117.4, 72.4, -131, 87.2, -131, 92.4);
            p.cubicTo(-131, 94.705, -130.729, 97.852, -130.03, 102.465);
            p.cubicTo(-130.03, 102.465, -130.6, 110.801, -103, 111.601);
            p.cubicTo(-75.4, 112.401, 299.717, 80.245, 299.717, 80.245);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#cc7226";
            sfp.strokeWidth =  -1;
            p.moveTo(-115.6, 102.6);
            p.cubicTo(-140.6, 63.2, -126.2, 119.601, -126.2, 119.601);
            p.cubicTo(-117.4, 154.001, 12.2, 116.401, 12.2, 116.401);
            p.cubicTo(12.2, 116.401, 181.001, 86, 192.201, 82);
            p.cubicTo(203.401, 78, 298.601, 84.4, 298.601, 84.4);
            p.lineTo(293.001, 67.6);
            p.cubicTo(228.201, 21.2, 209.001, 44.4, 195.401, 40.4);
            p.cubicTo(181.801, 36.4, 184.201, 46, 181.001, 46.8);
            p.cubicTo(177.801, 47.6, 138.601, 22.8, 132.201, 23.6);
            p.cubicTo(125.801, 24.4, 100.459, 0.649, 115.401, 32.4);
            p.cubicTo(131.401, 66.4, 57, 71.6, 40.2, 60.4);
            p.cubicTo(23.4, 49.2, 47.4, 78.8, 47.4, 78.8);
            p.cubicTo(65.8, 98.8, 31.4, 82, 31.4, 82);
            p.cubicTo(-3, 69.2, -27, 94.8, -30.2, 95.6);
            p.cubicTo(-33.4, 96.4, -38.2, 99.6, -39, 93.2);
            p.cubicTo(-39.8, 86.8, -47.31, 70.099, -79, 96.4);
            p.cubicTo(-99, 113.001, -112.8, 91, -112.8, 91);
            p.lineTo(-115.6, 102.6);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#e87f3a";
            sfp.strokeWidth =  -1;
            p.moveTo(133.51, 25.346);
            p.cubicTo(127.11, 26.146, 101.743, 2.407, 116.71, 34.146);
            p.cubicTo(133.31, 69.346, 58.31, 73.346, 41.51, 62.146);
            p.cubicTo(24.709, 50.946, 48.71, 80.546, 48.71, 80.546);
            p.cubicTo(67.11, 100.546, 32.709, 83.746, 32.709, 83.746);
            p.cubicTo(-1.691, 70.946, -25.691, 96.546, -28.891, 97.346);
            p.cubicTo(-32.091, 98.146, -36.891, 101.346, -37.691, 94.946);
            p.cubicTo(-38.491, 88.546, -45.87, 72.012, -77.691, 98.146);
            p.cubicTo(-98.927, 115.492, -112.418, 94.037, -112.418, 94.037);
            p.lineTo(-115.618, 104.146);
            p.cubicTo(-140.618, 64.346, -125.546, 122.655, -125.546, 122.655);
            p.cubicTo(-116.745, 157.056, 13.509, 118.146, 13.509, 118.146);
            p.cubicTo(13.509, 118.146, 182.31, 87.746, 193.51, 83.746);
            p.cubicTo(204.71, 79.746, 299.038, 86.073, 299.038, 86.073);
            p.lineTo(293.51, 68.764);
            p.cubicTo(228.71, 22.364, 210.31, 46.146, 196.71, 42.146);
            p.cubicTo(183.11, 38.146, 185.51, 47.746, 182.31, 48.546);
            p.cubicTo(179.11, 49.346, 139.91, 24.546, 133.51, 25.346);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#ea8c4d";
            sfp.strokeWidth =  -1;
            p.moveTo(134.819, 27.091);
            p.cubicTo(128.419, 27.891, 103.685, 3.862, 118.019, 35.891);
            p.cubicTo(134.219, 72.092, 59.619, 75.092, 42.819, 63.892);
            p.cubicTo(26.019, 52.692, 50.019, 82.292, 50.019, 82.292);
            p.cubicTo(68.419, 102.292, 34.019, 85.492, 34.019, 85.492);
            p.cubicTo(-0.381, 72.692, -24.382, 98.292, -27.582, 99.092);
            p.cubicTo(-30.782, 99.892, -35.582, 103.092, -36.382, 96.692);
            p.cubicTo(-37.182, 90.292, -44.43, 73.925, -76.382, 99.892);
            p.cubicTo(-98.855, 117.983, -112.036, 97.074, -112.036, 97.074);
            p.lineTo(-115.636, 105.692);
            p.cubicTo(-139.436, 66.692, -124.891, 125.71, -124.891, 125.71);
            p.cubicTo(-116.091, 160.11, 14.819, 119.892, 14.819, 119.892);
            p.cubicTo(14.819, 119.892, 183.619, 89.492, 194.819, 85.492);
            p.cubicTo(206.019, 81.492, 299.474, 87.746, 299.474, 87.746);
            p.lineTo(294.02, 69.928);
            p.cubicTo(229.219, 23.528, 211.619, 47.891, 198.019, 43.891);
            p.cubicTo(184.419, 39.891, 186.819, 49.491, 183.619, 50.292);
            p.cubicTo(180.419, 51.092, 141.219, 26.291, 134.819, 27.091);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#ec9961";
            sfp.strokeWidth =  -1;
            p.moveTo(136.128, 28.837);
            p.cubicTo(129.728, 29.637, 104.999, 5.605, 119.328, 37.637);
            p.cubicTo(136.128, 75.193, 60.394, 76.482, 44.128, 65.637);
            p.cubicTo(27.328, 54.437, 51.328, 84.037, 51.328, 84.037);
            p.cubicTo(69.728, 104.037, 35.328, 87.237, 35.328, 87.237);
            p.cubicTo(0.928, 74.437, -23.072, 100.037, -26.272, 100.837);
            p.cubicTo(-29.472, 101.637, -34.272, 104.837, -35.072, 98.437);
            p.cubicTo(-35.872, 92.037, -42.989, 75.839, -75.073, 101.637);
            p.cubicTo(-98.782, 120.474, -111.655, 100.11, -111.655, 100.11);
            p.lineTo(-115.655, 107.237);
            p.cubicTo(-137.455, 70.437, -124.236, 128.765, -124.236, 128.765);
            p.cubicTo(-115.436, 163.165, 16.128, 121.637, 16.128, 121.637);
            p.cubicTo(16.128, 121.637, 184.928, 91.237, 196.129, 87.237);
            p.cubicTo(207.329, 83.237, 299.911, 89.419, 299.911, 89.419);
            p.lineTo(294.529, 71.092);
            p.cubicTo(229.729, 24.691, 212.929, 49.637, 199.329, 45.637);
            p.cubicTo(185.728, 41.637, 188.128, 51.237, 184.928, 52.037);
            p.cubicTo(181.728, 52.837, 142.528, 28.037, 136.128, 28.837);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#eea575";
            sfp.strokeWidth =  -1;
            p.moveTo(137.438, 30.583);
            p.cubicTo(131.037, 31.383, 106.814, 7.129, 120.637, 39.383);
            p.cubicTo(137.438, 78.583, 62.237, 78.583, 45.437, 67.383);
            p.cubicTo(28.637, 56.183, 52.637, 85.783, 52.637, 85.783);
            p.cubicTo(71.037, 105.783, 36.637, 88.983, 36.637, 88.983);
            p.cubicTo(2.237, 76.183, -21.763, 101.783, -24.963, 102.583);
            p.cubicTo(-28.163, 103.383, -32.963, 106.583, -33.763, 100.183);
            p.cubicTo(-34.563, 93.783, -41.548, 77.752, -73.763, 103.383);
            p.cubicTo(-98.709, 122.965, -111.273, 103.146, -111.273, 103.146);
            p.lineTo(-115.673, 108.783);
            p.cubicTo(-135.473, 73.982, -123.582, 131.819, -123.582, 131.819);
            p.cubicTo(-114.782, 166.22, 17.437, 123.383, 17.437, 123.383);
            p.cubicTo(17.437, 123.383, 186.238, 92.983, 197.438, 88.983);
            p.cubicTo(208.638, 84.983, 300.347, 91.092, 300.347, 91.092);
            p.lineTo(295.038, 72.255);
            p.cubicTo(230.238, 25.855, 214.238, 51.383, 200.638, 47.383);
            p.cubicTo(187.038, 43.383, 189.438, 52.983, 186.238, 53.783);
            p.cubicTo(183.038, 54.583, 143.838, 29.783, 137.438, 30.583);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#f1b288";
            sfp.strokeWidth =  -1;
            p.moveTo(138.747, 32.328);
            p.cubicTo(132.347, 33.128, 106.383, 9.677, 121.947, 41.128);
            p.cubicTo(141.147, 79.928, 63.546, 80.328, 46.746, 69.128);
            p.cubicTo(29.946, 57.928, 53.946, 87.528, 53.946, 87.528);
            p.cubicTo(72.346, 107.528, 37.946, 90.728, 37.946, 90.728);
            p.cubicTo(3.546, 77.928, -20.454, 103.528, -23.654, 104.328);
            p.cubicTo(-26.854, 105.128, -31.654, 108.328, -32.454, 101.928);
            p.cubicTo(-33.254, 95.528, -40.108, 79.665, -72.454, 105.128);
            p.cubicTo(-98.636, 125.456, -110.891, 106.183, -110.891, 106.183);
            p.lineTo(-115.691, 110.328);
            p.cubicTo(-133.691, 77.128, -122.927, 134.874, -122.927, 134.874);
            p.cubicTo(-114.127, 169.274, 18.746, 125.128, 18.746, 125.128);
            p.cubicTo(18.746, 125.128, 187.547, 94.728, 198.747, 90.728);
            p.cubicTo(209.947, 86.728, 300.783, 92.764, 300.783, 92.764);
            p.lineTo(295.547, 73.419);
            p.cubicTo(230.747, 27.019, 215.547, 53.128, 201.947, 49.128);
            p.cubicTo(188.347, 45.128, 190.747, 54.728, 187.547, 55.528);
            p.cubicTo(184.347, 56.328, 145.147, 31.528, 138.747, 32.328);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#f3bf9c";
            sfp.strokeWidth =  -1;
            p.moveTo(140.056, 34.073);
            p.cubicTo(133.655, 34.873, 107.313, 11.613, 123.255, 42.873);
            p.cubicTo(143.656, 82.874, 64.855, 82.074, 48.055, 70.874);
            p.cubicTo(31.255, 59.674, 55.255, 89.274, 55.255, 89.274);
            p.cubicTo(73.655, 109.274, 39.255, 92.474, 39.255, 92.474);
            p.cubicTo(4.855, 79.674, -19.145, 105.274, -22.345, 106.074);
            p.cubicTo(-25.545, 106.874, -30.345, 110.074, -31.145, 103.674);
            p.cubicTo(-31.945, 97.274, -38.668, 81.578, -71.145, 106.874);
            p.cubicTo(-98.564, 127.947, -110.509, 109.219, -110.509, 109.219);
            p.lineTo(-115.709, 111.874);
            p.cubicTo(-131.709, 81.674, -122.273, 137.929, -122.273, 137.929);
            p.cubicTo(-113.473, 172.329, 20.055, 126.874, 20.055, 126.874);
            p.cubicTo(20.055, 126.874, 188.856, 96.474, 200.056, 92.474);
            p.cubicTo(211.256, 88.474, 301.22, 94.437, 301.22, 94.437);
            p.lineTo(296.056, 74.583);
            p.cubicTo(231.256, 28.183, 216.856, 54.874, 203.256, 50.874);
            p.cubicTo(189.656, 46.873, 192.056, 56.474, 188.856, 57.274);
            p.cubicTo(185.656, 58.074, 146.456, 33.273, 140.056, 34.073);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#f5ccb0";
            sfp.strokeWidth =  -1;
            p.moveTo(141.365, 35.819);
            p.cubicTo(134.965, 36.619, 107.523, 13.944, 124.565, 44.619);
            p.cubicTo(146.565, 84.219, 66.164, 83.819, 49.364, 72.619);
            p.cubicTo(32.564, 61.419, 56.564, 91.019, 56.564, 91.019);
            p.cubicTo(74.964, 111.019, 40.564, 94.219, 40.564, 94.219);
            p.cubicTo(6.164, 81.419, -17.836, 107.019, -21.036, 107.819);
            p.cubicTo(-24.236, 108.619, -29.036, 111.819, -29.836, 105.419);
            p.cubicTo(-30.636, 99.019, -37.227, 83.492, -69.836, 108.619);
            p.cubicTo(-98.491, 130.438, -110.127, 112.256, -110.127, 112.256);
            p.lineTo(-115.727, 113.419);
            p.cubicTo(-130.128, 85.019, -121.618, 140.983, -121.618, 140.983);
            p.cubicTo(-112.818, 175.384, 21.364, 128.619, 21.364, 128.619);
            p.cubicTo(21.364, 128.619, 190.165, 98.219, 201.365, 94.219);
            p.cubicTo(212.565, 90.219, 301.656, 96.11, 301.656, 96.11);
            p.lineTo(296.565, 75.746);
            p.cubicTo(231.765, 29.346, 218.165, 56.619, 204.565, 52.619);
            p.cubicTo(190.965, 48.619, 193.365, 58.219, 190.165, 59.019);
            p.cubicTo(186.965, 59.819, 147.765, 35.019, 141.365, 35.819);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#f8d8c4";
            sfp.strokeWidth =  -1;
            p.moveTo(142.674, 37.565);
            p.cubicTo(136.274, 38.365, 108.832, 15.689, 125.874, 46.365);
            p.cubicTo(147.874, 85.965, 67.474, 85.565, 50.674, 74.365);
            p.cubicTo(33.874, 63.165, 57.874, 92.765, 57.874, 92.765);
            p.cubicTo(76.274, 112.765, 41.874, 95.965, 41.874, 95.965);
            p.cubicTo(7.473, 83.165, -16.527, 108.765, -19.727, 109.565);
            p.cubicTo(-22.927, 110.365, -27.727, 113.565, -28.527, 107.165);
            p.cubicTo(-29.327, 100.765, -35.786, 85.405, -68.527, 110.365);
            p.cubicTo(-98.418, 132.929, -109.745, 115.293, -109.745, 115.293);
            p.lineTo(-115.745, 114.965);
            p.cubicTo(-129.346, 88.564, -120.963, 144.038, -120.963, 144.038);
            p.cubicTo(-112.163, 178.438, 22.673, 130.365, 22.673, 130.365);
            p.cubicTo(22.673, 130.365, 191.474, 99.965, 202.674, 95.965);
            p.cubicTo(213.874, 91.965, 302.093, 97.783, 302.093, 97.783);
            p.lineTo(297.075, 76.91);
            p.cubicTo(232.274, 30.51, 219.474, 58.365, 205.874, 54.365);
            p.cubicTo(192.274, 50.365, 194.674, 59.965, 191.474, 60.765);
            p.cubicTo(188.274, 61.565, 149.074, 36.765, 142.674, 37.565);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#fae5d7";
            sfp.strokeWidth =  -1;
            p.moveTo(143.983, 39.31);
            p.cubicTo(137.583, 40.11, 110.529, 17.223, 127.183, 48.11);
            p.cubicTo(149.183, 88.91, 68.783, 87.31, 51.983, 76.11);
            p.cubicTo(35.183, 64.91, 59.183, 94.51, 59.183, 94.51);
            p.cubicTo(77.583, 114.51, 43.183, 97.71, 43.183, 97.71);
            p.cubicTo(8.783, 84.91, -15.217, 110.51, -18.417, 111.31);
            p.cubicTo(-21.618, 112.11, -26.418, 115.31, -27.218, 108.91);
            p.cubicTo(-28.018, 102.51, -34.346, 87.318, -67.218, 112.11);
            p.cubicTo(-98.345, 135.42, -109.363, 118.329, -109.363, 118.329);
            p.lineTo(-115.764, 116.51);
            p.cubicTo(-128.764, 92.51, -120.309, 147.093, -120.309, 147.093);
            p.cubicTo(-111.509, 181.493, 23.983, 132.11, 23.983, 132.11);
            p.cubicTo(23.983, 132.11, 192.783, 101.71, 203.983, 97.71);
            p.cubicTo(215.183, 93.71, 302.529, 99.456, 302.529, 99.456);
            p.lineTo(297.583, 78.074);
            p.cubicTo(232.783, 31.673, 220.783, 60.11, 207.183, 56.11);
            p.cubicTo(193.583, 52.11, 195.983, 61.71, 192.783, 62.51);
            p.cubicTo(189.583, 63.31, 150.383, 38.51, 143.983, 39.31);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#fcf2eb";
            sfp.strokeWidth =  -1;
            p.moveTo(145.292, 41.055);
            p.cubicTo(138.892, 41.855, 112.917, 18.411, 128.492, 49.855);
            p.cubicTo(149.692, 92.656, 70.092, 89.056, 53.292, 77.856);
            p.cubicTo(36.492, 66.656, 60.492, 96.256, 60.492, 96.256);
            p.cubicTo(78.892, 116.256, 44.492, 99.456, 44.492, 99.456);
            p.cubicTo(10.092, 86.656, -13.908, 112.256, -17.108, 113.056);
            p.cubicTo(-20.308, 113.856, -25.108, 117.056, -25.908, 110.656);
            p.cubicTo(-26.708, 104.256, -32.905, 89.232, -65.908, 113.856);
            p.cubicTo(-98.273, 137.911, -108.982, 121.365, -108.982, 121.365);
            p.lineTo(-115.782, 118.056);
            p.cubicTo(-128.582, 94.856, -119.654, 150.147, -119.654, 150.147);
            p.cubicTo(-110.854, 184.547, 25.292, 133.856, 25.292, 133.856);
            p.cubicTo(25.292, 133.856, 194.093, 103.456, 205.293, 99.456);
            p.cubicTo(216.493, 95.456, 302.965, 101.128, 302.965, 101.128);
            p.lineTo(298.093, 79.237);
            p.cubicTo(233.292, 32.837, 222.093, 61.856, 208.493, 57.856);
            p.cubicTo(194.893, 53.855, 197.293, 63.456, 194.093, 64.256);
            p.cubicTo(190.892, 65.056, 151.692, 40.255, 145.292, 41.055);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#ffffff";
            sfp.strokeWidth =  -1;
            p.moveTo(-115.8, 119.601);
            p.cubicTo(-128.6, 97.6, -119, 153.201, -119, 153.201);
            p.cubicTo(-110.2, 187.601, 26.6, 135.601, 26.6, 135.601);
            p.cubicTo(26.6, 135.601, 195.401, 105.2, 206.601, 101.2);
            p.cubicTo(217.801, 97.2, 303.401, 102.8, 303.401, 102.8);
            p.lineTo(298.601, 80.4);
            p.cubicTo(233.801, 34, 223.401, 63.6, 209.801, 59.6);
            p.cubicTo(196.201, 55.6, 198.601, 65.2, 195.401, 66);
            p.cubicTo(192.201, 66.8, 153.001, 42, 146.601, 42.8);
            p.cubicTo(140.201, 43.6, 114.981, 19.793, 129.801, 51.6);
            p.cubicTo(152.028, 99.307, 69.041, 89.227, 54.6, 79.6);
            p.cubicTo(37.8, 68.4, 61.8, 98, 61.8, 98);
            p.cubicTo(80.2, 118.001, 45.8, 101.2, 45.8, 101.2);
            p.cubicTo(11.4, 88.4, -12.6, 114.001, -15.8, 114.801);
            p.cubicTo(-19, 115.601, -23.8, 118.801, -24.6, 112.401);
            p.cubicTo(-25.4, 106, -31.465, 91.144, -64.6, 115.601);
            p.cubicTo(-98.2, 140.401, -108.6, 124.401, -108.6, 124.401);
            p.lineTo(-115.8, 119.601);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#000000";
            sfp.strokeWidth =  -1;
            p.moveTo(-74.2, 149.601);
            p.cubicTo(-74.2, 149.601, -81.4, 161.201, -60.6, 174.401);
            p.cubicTo(-60.6, 174.401, -59.2, 175.801, -77.2, 171.601);
            p.cubicTo(-77.2, 171.601, -83.4, 169.601, -85, 159.201);
            p.cubicTo(-85, 159.201, -89.8, 154.801, -94.6, 149.201);
            p.cubicTo(-99.4, 143.601, -74.2, 149.601, -74.2, 149.601);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#cccccc";
            sfp.strokeWidth =  -1;
            p.moveTo(65.8, 102);
            p.cubicTo(65.8, 102, 83.498, 128.821, 82.9, 133.601);
            p.cubicTo(81.6, 144.001, 81.4, 153.601, 84.6, 157.601);
            p.cubicTo(87.801, 161.601, 96.601, 194.801, 96.601, 194.801);
            p.cubicTo(96.601, 194.801, 96.201, 196.001, 108.601, 158.001);
            p.cubicTo(108.601, 158.001, 120.201, 142.001, 100.201, 123.601);
            p.cubicTo(100.201, 123.601, 65, 94.8, 65.8, 102);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#000000";
            sfp.strokeWidth =  -1;
            p.moveTo(-54.2, 176.401);
            p.cubicTo(-54.2, 176.401, -43, 183.601, -57.4, 214.801);
            p.lineTo(-51, 212.401);
            p.cubicTo(-51, 212.401, -51.8, 223.601, -55, 226.001);
            p.lineTo(-47.8, 222.801);
            p.cubicTo(-47.8, 222.801, -43, 230.801, -47, 235.601);
            p.cubicTo(-47, 235.601, -30.2, 243.601, -31, 250.001);
            p.cubicTo(-31, 250.001, -24.6, 242.001, -28.6, 235.601);
            p.cubicTo(-32.6, 229.201, -39.8, 233.201, -39, 214.801);
            p.lineTo(-47.8, 218.001);
            p.cubicTo(-47.8, 218.001, -42.2, 209.201, -42.2, 202.801);
            p.lineTo(-50.2, 205.201);
            p.cubicTo(-50.2, 205.201, -34.731, 178.623, -45.4, 177.201);
            p.cubicTo(-51.4, 176.401, -54.2, 176.401, -54.2, 176.401);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#cccccc";
            sfp.strokeWidth =  -1;
            p.moveTo(-21.8, 193.201);
            p.cubicTo(-21.8, 193.201, -19, 188.801, -21.8, 189.601);
            p.cubicTo(-24.6, 190.401, -55.8, 205.201, -61.8, 214.801);
            p.cubicTo(-61.8, 214.801, -27.4, 190.401, -21.8, 193.201);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#cccccc";
            sfp.strokeWidth =  -1;
            p.moveTo(-11.4, 201.201);
            p.cubicTo(-11.4, 201.201, -8.6, 196.801, -11.4, 197.601);
            p.cubicTo(-14.2, 198.401, -45.4, 213.201, -51.4, 222.801);
            p.cubicTo(-51.4, 222.801, -17, 198.401, -11.4, 201.201);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#cccccc";
            sfp.strokeWidth =  -1;
            p.moveTo(1.8, 186.001);
            p.cubicTo(1.8, 186.001, 4.6, 181.601, 1.8, 182.401);
            p.cubicTo(-1, 183.201, -32.2, 198.001, -38.2, 207.601);
            p.cubicTo(-38.2, 207.601, -3.8, 183.201, 1.8, 186.001);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#cccccc";
            sfp.strokeWidth =  -1;
            p.moveTo(-21.4, 229.601);
            p.cubicTo(-21.4, 229.601, -21.4, 223.601, -24.2, 224.401);
            p.cubicTo(-27, 225.201, -63, 242.801, -69, 252.401);
            p.cubicTo(-69, 252.401, -27, 226.801, -21.4, 229.601);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#cccccc";
            sfp.strokeWidth =  -1;
            p.moveTo(-20.2, 218.801);
            p.cubicTo(-20.2, 218.801, -19, 214.001, -21.8, 214.801);
            p.cubicTo(-23.8, 214.801, -50.2, 226.401, -56.2, 236.001);
            p.cubicTo(-56.2, 236.001, -26.6, 214.401, -20.2, 218.801);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#cccccc";
            sfp.strokeWidth =  -1;
            p.moveTo(-34.6, 266.401);
            p.lineTo(-44.6, 274.001);
            p.cubicTo(-44.6, 274.001, -34.2, 266.401, -30.6, 267.601);
            p.cubicTo(-30.6, 267.601, -37.4, 278.801, -38.2, 284.001);
            p.cubicTo(-38.2, 284.001, -27.8, 271.201, -22.2, 271.601);
            p.cubicTo(-22.2, 271.601, -14.6, 272.001, -14.6, 282.801);
            p.cubicTo(-14.6, 282.801, -9, 272.401, -5.8, 272.801);
            p.cubicTo(-5.8, 272.801, -4.6, 279.201, -5.8, 286.001);
            p.cubicTo(-5.8, 286.001, -1.8, 278.401, 2.2, 280.001);
            p.cubicTo(2.2, 280.001, 8.6, 278.001, 7.8, 289.601);
            p.cubicTo(7.8, 289.601, 7.8, 300.001, 7, 302.801);
            p.cubicTo(7, 302.801, 12.6, 276.401, 15, 276.001);
            p.cubicTo(15, 276.001, 23, 274.801, 27.8, 283.601);
            p.cubicTo(27.8, 283.601, 23.8, 276.001, 28.6, 278.001);
            p.cubicTo(28.6, 278.001, 39.4, 279.601, 42.6, 286.401);
            p.cubicTo(42.6, 286.401, 35.8, 274.401, 41.4, 277.601);
            p.cubicTo(41.4, 277.601, 48.2, 277.601, 49.4, 284.001);
            p.cubicTo(49.4, 284.001, 57.8, 305.201, 59.8, 306.801);
            p.cubicTo(59.8, 306.801, 52.2, 285.201, 53.8, 285.201);
            p.cubicTo(53.8, 285.201, 51.8, 273.201, 57, 288.001);
            p.cubicTo(57, 288.001, 53.8, 274.001, 59.4, 274.801);
            p.cubicTo(65, 275.601, 69.4, 285.601, 77.8, 283.201);
            p.cubicTo(77.8, 283.201, 87.401, 288.801, 89.401, 219.601);
            p.lineTo(-34.6, 266.401);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#000000";
            sfp.strokeWidth =  -1;
            p.moveTo(-29.8, 173.601);
            p.cubicTo(-29.8, 173.601, -15, 167.601, 25, 173.601);
            p.cubicTo(25, 173.601, 32.2, 174.001, 39, 165.201);
            p.cubicTo(45.8, 156.401, 72.6, 149.201, 79, 151.201);
            p.lineTo(88.601, 157.601);
            p.lineTo(89.401, 158.801);
            p.cubicTo(89.401, 158.801, 101.801, 169.201, 102.201, 176.801);
            p.cubicTo(102.601, 184.401, 87.801, 232.401, 78.2, 248.401);
            p.cubicTo(68.6, 264.401, 59, 276.801, 39.8, 274.401);
            p.cubicTo(39.8, 274.401, 19, 270.401, -6.6, 274.401);
            p.cubicTo(-6.6, 274.401, -35.8, 272.801, -38.6, 264.801);
            p.cubicTo(-41.4, 256.801, -27.4, 241.601, -27.4, 241.601);
            p.cubicTo(-27.4, 241.601, -23, 233.201, -24.2, 218.801);
            p.cubicTo(-25.4, 204.401, -25, 176.401, -29.8, 173.601);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#e5668c";
            sfp.strokeWidth =  -1;
            p.moveTo(-7.8, 175.601);
            p.cubicTo(0.6, 194.001, -29, 259.201, -29, 259.201);
            p.cubicTo(-31, 260.801, -16.34, 266.846, -6.2, 264.401);
            p.cubicTo(4.746, 261.763, 45, 266.001, 45, 266.001);
            p.cubicTo(68.6, 250.401, 81.4, 206.001, 81.4, 206.001);
            p.cubicTo(81.4, 206.001, 91.801, 182.001, 74.2, 178.801);
            p.cubicTo(56.6, 175.601, -7.8, 175.601, -7.8, 175.601);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#b23259";
            sfp.strokeWidth =  -1;
            p.moveTo(-9.831, 206.497);
            p.cubicTo(-6.505, 193.707, -4.921, 181.906, -7.8, 175.601);
            p.cubicTo(-7.8, 175.601, 54.6, 182.001, 65.8, 161.201);
            p.cubicTo(70.041, 153.326, 84.801, 184.001, 84.4, 193.601);
            p.cubicTo(84.4, 193.601, 21.4, 208.001, 6.6, 196.801);
            p.lineTo(-9.831, 206.497);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#a5264c";
            sfp.strokeWidth =  -1;
            p.moveTo(-5.4, 222.801);
            p.cubicTo(-5.4, 222.801, -3.4, 230.001, -5.8, 234.001);
            p.cubicTo(-5.8, 234.001, -7.4, 234.801, -8.6, 235.201);
            p.cubicTo(-8.6, 235.201, -7.4, 238.801, -1.4, 240.401);
            p.cubicTo(-1.4, 240.401, 0.6, 244.801, 3, 245.201);
            p.cubicTo(5.4, 245.601, 10.2, 251.201, 14.2, 250.001);
            p.cubicTo(18.2, 248.801, 29.4, 244.801, 29.4, 244.801);
            p.cubicTo(29.4, 244.801, 35, 241.601, 43.8, 245.201);
            p.cubicTo(43.8, 245.201, 46.175, 244.399, 46.6, 240.401);
            p.cubicTo(47.1, 235.701, 50.2, 232.001, 52.2, 230.001);
            p.cubicTo(54.2, 228.001, 63.8, 215.201, 62.6, 214.801);
            p.cubicTo(61.4, 214.401, -5.4, 222.801, -5.4, 222.801);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#ff727f";
            sfp.strokeColor = "#000000";
            sfp.strokeWidth = 1;
            p.moveTo(-9.8, 174.401);
            p.cubicTo(-9.8, 174.401, -12.6, 196.801, -9.4, 205.201);
            p.cubicTo(-6.2, 213.601, -7, 215.601, -7.8, 219.601);
            p.cubicTo(-8.6, 223.601, -4.2, 233.601, 1.4, 239.601);
            p.lineTo(13.4, 241.201);
            p.cubicTo(13.4, 241.201, 28.6, 237.601, 37.8, 240.401);
            p.cubicTo(37.8, 240.401, 46.794, 241.744, 50.2, 226.801);
            p.cubicTo(50.2, 226.801, 55, 220.401, 62.2, 217.601);
            p.cubicTo(69.4, 214.801, 76.6, 173.201, 72.6, 165.201);
            p.cubicTo(68.6, 157.201, 54.2, 152.801, 38.2, 168.401);
            p.cubicTo(22.2, 184.001, 20.2, 167.201, -9.8, 174.401);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#ffffcc";
            sfp.strokeColor = "#000000";
            sfp.strokeWidth = 0.5;
            p.moveTo(-8.2, 249.201);
            p.cubicTo(-8.2, 249.201, -9, 247.201, -13.4, 246.801);
            p.cubicTo(-13.4, 246.801, -35.8, 243.201, -44.2, 230.801);
            p.cubicTo(-44.2, 230.801, -51, 225.201, -46.6, 236.801);
            p.cubicTo(-46.6, 236.801, -36.2, 257.201, -29.4, 260.001);
            p.cubicTo(-29.4, 260.001, -13, 264.001, -8.2, 249.201);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#cc3f4c";
            sfp.strokeWidth =  -1;
            p.moveTo(71.742, 185.229);
            p.cubicTo(72.401, 177.323, 74.354, 168.709, 72.6, 165.201);
            p.cubicTo(66.154, 152.307, 49.181, 157.695, 38.2, 168.401);
            p.cubicTo(22.2, 184.001, 20.2, 167.201, -9.8, 174.401);
            p.cubicTo(-9.8, 174.401, -11.545, 188.364, -10.705, 198.376);
            p.cubicTo(-10.705, 198.376, 26.6, 186.801, 27.4, 192.401);
            p.cubicTo(27.4, 192.401, 29, 189.201, 38.2, 189.201);
            p.cubicTo(47.4, 189.201, 70.142, 188.029, 71.742, 185.229);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor = "transparent";
            sfp.strokeColor = "#a51926";
            sfp.strokeWidth = 2;
            p.moveTo(28.6, 175.201);
            p.cubicTo(28.6, 175.201, 33.4, 180.001, 29.8, 189.601);
            p.cubicTo(29.8, 189.601, 15.4, 205.601, 17.4, 219.601);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#ffffcc";
            sfp.strokeColor = "#000000";
            sfp.strokeWidth = 0.5;
            p.moveTo(-19.4, 260.001);
            p.cubicTo(-19.4, 260.001, -23.8, 247.201, -15, 254.001);
            p.cubicTo(-15, 254.001, -10.2, 256.001, -11.4, 257.601);
            p.cubicTo(-12.6, 259.201, -18.2, 263.201, -19.4, 260.001);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#ffffcc";
            sfp.strokeColor = "#000000";
            sfp.strokeWidth = 0.5;
            p.moveTo(-14.36, 261.201);
            p.cubicTo(-14.36, 261.201, -17.88, 250.961, -10.84, 256.401);
            p.cubicTo(-10.84, 256.401, -6.419, 258.849, -7.96, 259.281);
            p.cubicTo(-12.52, 260.561, -7.96, 263.121, -14.36, 261.201);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#ffffcc";
            sfp.strokeColor = "#000000";
            sfp.strokeWidth = 0.5;
            p.moveTo(-9.56, 261.201);
            p.cubicTo(-9.56, 261.201, -13.08, 250.961, -6.04, 256.401);
            p.cubicTo(-6.04, 256.401, -1.665, 258.711, -3.16, 259.281);
            p.cubicTo(-6.52, 260.561, -3.16, 263.121, -9.56, 261.201);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#ffffcc";
            sfp.strokeColor = "#000000";
            sfp.strokeWidth = 0.5;
            p.moveTo(-2.96, 261.401);
            p.cubicTo(-2.96, 261.401, -6.48, 251.161, 0.56, 256.601);
            p.cubicTo(0.56, 256.601, 4.943, 258.933, 3.441, 259.481);
            p.cubicTo(0.48, 260.561, 3.441, 263.321, -2.96, 261.401);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#ffffcc";
            sfp.strokeColor = "#000000";
            sfp.strokeWidth = 0.5;
            p.moveTo(3.52, 261.321);
            p.cubicTo(3.52, 261.321, 0, 251.081, 7.041, 256.521);
            p.cubicTo(7.041, 256.521, 10.881, 258.121, 9.921, 259.401);
            p.cubicTo(8.961, 260.681, 9.921, 263.241, 3.52, 261.321);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#ffffcc";
            sfp.strokeColor = "#000000";
            sfp.strokeWidth = 0.5;
            p.moveTo(10.2, 262.001);
            p.cubicTo(10.2, 262.001, 5.4, 249.601, 14.6, 256.001);
            p.cubicTo(14.6, 256.001, 19.4, 258.001, 18.2, 259.601);
            p.cubicTo(17, 261.201, 18.2, 264.401, 10.2, 262.001);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor = "transparent";
            sfp.strokeColor = "#a5264c";
            sfp.strokeWidth = 2;
            p.moveTo(-18.2, 244.801);
            p.cubicTo(-18.2, 244.801, -5, 242.001, 1, 245.201);
            p.cubicTo(1, 245.201, 7, 246.401, 8.2, 246.001);
            p.cubicTo(9.4, 245.601, 12.6, 245.201, 12.6, 245.201);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor = "transparent";
            sfp.strokeColor = "#a5264c";
            sfp.strokeWidth = 2;
            p.moveTo(15.8, 253.601);
            p.cubicTo(15.8, 253.601, 27.8, 240.001, 39.8, 244.401);
            p.cubicTo(46.816, 246.974, 45.8, 243.601, 46.6, 240.801);
            p.cubicTo(47.4, 238.001, 47.6, 233.801, 52.6, 230.801);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#ffffcc";
            sfp.strokeColor = "#000000";
            sfp.strokeWidth = 0.5;
            p.moveTo(33, 237.601);
            p.cubicTo(33, 237.601, 29, 226.801, 26.2, 239.601);
            p.cubicTo(23.4, 252.401, 20.2, 256.001, 18.6, 258.801);
            p.cubicTo(18.6, 258.801, 18.6, 264.001, 27, 263.601);
            p.cubicTo(27, 263.601, 37.8, 263.201, 38.2, 260.401);
            p.cubicTo(38.6, 257.601, 37, 246.001, 33, 237.601);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor = "transparent";
            sfp.strokeColor = "#a5264c";
            sfp.strokeWidth = 2;
            p.moveTo(47, 244.801);
            p.cubicTo(47, 244.801, 50.6, 242.401, 53, 243.601);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor = "transparent";
            sfp.strokeColor = "#a5264c";
            sfp.strokeWidth = 2;
            p.moveTo(53.5, 228.401);
            p.cubicTo(53.5, 228.401, 56.4, 223.501, 61.2, 222.701);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#b2b2b2";
            sfp.strokeWidth =  -1;
            p.moveTo(-25.8, 265.201);
            p.cubicTo(-25.8, 265.201, -7.8, 268.401, -3.4, 266.801);
            p.cubicTo(-3.4, 266.801, 5.4, 266.801, -3, 268.801);
            p.cubicTo(-3, 268.801, -15.8, 268.801, -23.8, 267.601);
            p.cubicTo(-23.8, 267.601, -35.4, 262.001, -25.8, 265.201);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#ffffcc";
            sfp.strokeColor = "#000000";
            sfp.strokeWidth = 0.5;
            p.moveTo(-11.8, 172.001);
            p.cubicTo(-11.8, 172.001, 5.8, 172.001, 7.8, 172.801);
            p.cubicTo(7.8, 172.801, 15, 203.601, 11.4, 211.201);
            p.cubicTo(11.4, 211.201, 10.2, 214.001, 7.4, 208.401);
            p.cubicTo(7.4, 208.401, -11, 175.601, -14.2, 173.601);
            p.cubicTo(-17.4, 171.601, -13, 172.001, -11.8, 172.001);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#ffffcc";
            sfp.strokeColor = "#000000";
            sfp.strokeWidth = 0.5;
            p.moveTo(-88.9, 169.301);
            p.cubicTo(-88.9, 169.301, -80, 171.001, -67.4, 173.601);
            p.cubicTo(-67.4, 173.601, -62.6, 196.001, -59.4, 200.801);
            p.cubicTo(-56.2, 205.601, -59.8, 205.601, -63.4, 202.801);
            p.cubicTo(-67, 200.001, -81.8, 186.001, -83.8, 181.601);
            p.cubicTo(-85.8, 177.201, -88.9, 169.301, -88.9, 169.301);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#ffffcc";
            sfp.strokeColor = "#000000";
            sfp.strokeWidth = 0.5;
            p.moveTo(-67.039, 173.818);
            p.cubicTo(-67.039, 173.818, -61.239, 175.366, -60.23, 177.581);
            p.cubicTo(-59.222, 179.795, -61.432, 183.092, -61.432, 183.092);
            p.cubicTo(-61.432, 183.092, -62.432, 186.397, -63.634, 184.235);
            p.cubicTo(-64.836, 182.072, -67.708, 174.412, -67.039, 173.818);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#000000";
            sfp.strokeWidth =  -1;
            p.moveTo(-67, 173.601);
            p.cubicTo(-67, 173.601, -63.4, 178.801, -59.8, 178.801);
            p.cubicTo(-56.2, 178.801, -55.818, 178.388, -53, 179.001);
            p.cubicTo(-48.4, 180.001, -48.8, 178.001, -42.2, 179.201);
            p.cubicTo(-39.56, 179.681, -37, 178.801, -34.2, 180.001);
            p.cubicTo(-31.4, 181.201, -28.2, 180.401, -27, 178.401);
            p.cubicTo(-25.8, 176.401, -21, 172.201, -21, 172.201);
            p.cubicTo(-21, 172.201, -33.8, 174.001, -36.6, 174.801);
            p.cubicTo(-36.6, 174.801, -59, 176.001, -67, 173.601);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#ffffcc";
            sfp.strokeColor = "#000000";
            sfp.strokeWidth = 0.5;
            p.moveTo(-22.4, 173.801);
            p.cubicTo(-22.4, 173.801, -28.85, 177.301, -29.25, 179.701);
            p.cubicTo(-29.65, 182.101, -24, 185.801, -24, 185.801);
            p.cubicTo(-24, 185.801, -21.25, 190.401, -20.65, 188.001);
            p.cubicTo(-20.05, 185.601, -21.6, 174.201, -22.4, 173.801);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#ffffcc";
            sfp.strokeColor = "#000000";
            sfp.strokeWidth = 0.5;
            p.moveTo(-59.885, 179.265);
            p.cubicTo(-59.885, 179.265, -52.878, 190.453, -52.661, 179.242);
            p.cubicTo(-52.661, 179.242, -52.104, 177.984, -53.864, 177.962);
            p.cubicTo(-59.939, 177.886, -58.418, 173.784, -59.885, 179.265);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#ffffcc";
            sfp.strokeColor = "#000000";
            sfp.strokeWidth = 0.5;
            p.moveTo(-52.707, 179.514);
            p.cubicTo(-52.707, 179.514, -44.786, 190.701, -45.422, 179.421);
            p.cubicTo(-45.422, 179.421, -45.415, 179.089, -47.168, 178.936);
            p.cubicTo(-51.915, 178.522, -51.57, 174.004, -52.707, 179.514);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#ffffcc";
            sfp.strokeColor = "#000000";
            sfp.strokeWidth = 0.5;
            p.moveTo(-45.494, 179.522);
            p.cubicTo(-45.494, 179.522, -37.534, 190.15, -38.203, 180.484);
            p.cubicTo(-38.203, 180.484, -38.084, 179.251, -39.738, 178.95);
            p.cubicTo(-43.63, 178.244, -43.841, 174.995, -45.494, 179.522);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#ffffcc";
            sfp.strokeColor = "#000000";
            sfp.strokeWidth = 0.5;
            p.moveTo(-38.618, 179.602);
            p.cubicTo(-38.618, 179.602, -30.718, 191.163, -30.37, 181.382);
            p.cubicTo(-30.37, 181.382, -28.726, 180.004, -30.472, 179.782);
            p.cubicTo(-36.29, 179.042, -35.492, 174.588, -38.618, 179.602);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#e5e5b2";
            sfp.strokeWidth =  -1;
            p.moveTo(-74.792, 183.132);
            p.lineTo(-82.45, 181.601);
            p.cubicTo(-85.05, 176.601, -87.15, 170.451, -87.15, 170.451);
            p.cubicTo(-87.15, 170.451, -80.8, 171.451, -68.3, 174.251);
            p.cubicTo(-68.3, 174.251, -67.424, 177.569, -65.952, 183.364);
            p.lineTo(-74.792, 183.132);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#e5e5b2";
            sfp.strokeWidth =  -1;
            p.moveTo(-9.724, 178.47);
            p.cubicTo(-11.39, 175.964, -12.707, 174.206, -13.357, 173.8);
            p.cubicTo(-16.37, 171.917, -12.227, 172.294, -11.098, 172.294);
            p.cubicTo(-11.098, 172.294, 5.473, 172.294, 7.356, 173.047);
            p.cubicTo(7.356, 173.047, 7.88, 175.289, 8.564, 178.68);
            p.cubicTo(8.564, 178.68, -1.524, 176.67, -9.724, 178.47);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#cc7226";
            sfp.strokeWidth =  -1;
            p.moveTo(43.88, 40.321);
            p.cubicTo(71.601, 44.281, 97.121, 8.641, 98.881, -1.04);
            p.cubicTo(100.641, -10.72, 90.521, -22.6, 90.521, -22.6);
            p.cubicTo(91.841, -25.68, 87.001, -39.76, 81.721, -49);
            p.cubicTo(76.441, -58.24, 60.54, -57.266, 43, -58.24);
            p.cubicTo(27.16, -59.12, 8.68, -35.8, 7.36, -34.04);
            p.cubicTo(6.04, -32.28, 12.2, 6.001, 13.52, 11.721);
            p.cubicTo(14.84, 17.441, 12.2, 43.841, 12.2, 43.841);
            p.cubicTo(46.44, 34.741, 16.16, 36.361, 43.88, 40.321);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#ea8e51";
            sfp.strokeWidth =  -1;
            p.moveTo(8.088, -33.392);
            p.cubicTo(6.792, -31.664, 12.84, 5.921, 14.136, 11.537);
            p.cubicTo(15.432, 17.153, 12.84, 43.073, 12.84, 43.073);
            p.cubicTo(45.512, 34.193, 16.728, 35.729, 43.944, 39.617);
            p.cubicTo(71.161, 43.505, 96.217, 8.513, 97.945, -0.992);
            p.cubicTo(99.673, -10.496, 89.737, -22.16, 89.737, -22.16);
            p.cubicTo(91.033, -25.184, 86.281, -39.008, 81.097, -48.08);
            p.cubicTo(75.913, -57.152, 60.302, -56.195, 43.08, -57.152);
            p.cubicTo(27.528, -58.016, 9.384, -35.12, 8.088, -33.392);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#efaa7c";
            sfp.strokeWidth =  -1;
            p.moveTo(8.816, -32.744);
            p.cubicTo(7.544, -31.048, 13.48, 5.841, 14.752, 11.353);
            p.cubicTo(16.024, 16.865, 13.48, 42.305, 13.48, 42.305);
            p.cubicTo(44.884, 33.145, 17.296, 35.097, 44.008, 38.913);
            p.cubicTo(70.721, 42.729, 95.313, 8.385, 97.009, -0.944);
            p.cubicTo(98.705, -10.272, 88.953, -21.72, 88.953, -21.72);
            p.cubicTo(90.225, -24.688, 85.561, -38.256, 80.473, -47.16);
            p.cubicTo(75.385, -56.064, 60.063, -55.125, 43.16, -56.064);
            p.cubicTo(27.896, -56.912, 10.088, -34.44, 8.816, -32.744);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#f4c6a8";
            sfp.strokeWidth =  -1;
            p.moveTo(9.544, -32.096);
            p.cubicTo(8.296, -30.432, 14.12, 5.761, 15.368, 11.169);
            p.cubicTo(16.616, 16.577, 14.12, 41.537, 14.12, 41.537);
            p.cubicTo(43.556, 32.497, 17.864, 34.465, 44.072, 38.209);
            p.cubicTo(70.281, 41.953, 94.409, 8.257, 96.073, -0.895);
            p.cubicTo(97.737, -10.048, 88.169, -21.28, 88.169, -21.28);
            p.cubicTo(89.417, -24.192, 84.841, -37.504, 79.849, -46.24);
            p.cubicTo(74.857, -54.976, 59.824, -54.055, 43.24, -54.976);
            p.cubicTo(28.264, -55.808, 10.792, -33.76, 9.544, -32.096);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#f9e2d3";
            sfp.strokeWidth =  -1;
            p.moveTo(10.272, -31.448);
            p.cubicTo(9.048, -29.816, 14.76, 5.681, 15.984, 10.985);
            p.cubicTo(17.208, 16.289, 14.76, 40.769, 14.76, 40.769);
            p.cubicTo(42.628, 31.849, 18.432, 33.833, 44.136, 37.505);
            p.cubicTo(69.841, 41.177, 93.505, 8.129, 95.137, -0.848);
            p.cubicTo(96.769, -9.824, 87.385, -20.84, 87.385, -20.84);
            p.cubicTo(88.609, -23.696, 84.121, -36.752, 79.225, -45.32);
            p.cubicTo(74.329, -53.888, 59.585, -52.985, 43.32, -53.888);
            p.cubicTo(28.632, -54.704, 11.496, -33.08, 10.272, -31.448);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#ffffff";
            sfp.strokeWidth =  -1;
            p.moveTo(44.2, 36.8);
            p.cubicTo(69.4, 40.4, 92.601, 8, 94.201, -0.8);
            p.cubicTo(95.801, -9.6, 86.601, -20.4, 86.601, -20.4);
            p.cubicTo(87.801, -23.2, 83.4, -36, 78.6, -44.4);
            p.cubicTo(73.8, -52.8, 59.346, -51.914, 43.4, -52.8);
            p.cubicTo(29, -53.6, 12.2, -32.4, 11, -30.8);
            p.cubicTo(9.8, -29.2, 15.4, 5.6, 16.6, 10.8);
            p.cubicTo(17.8, 16, 15.4, 40, 15.4, 40);
            p.cubicTo(40.9, 31.4, 19, 33.2, 44.2, 36.8);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#cccccc";
            sfp.strokeWidth =  -1;
            p.moveTo(90.601, 2.8);
            p.cubicTo(90.601, 2.8, 62.8, 10.4, 51.2, 8.8);
            p.cubicTo(51.2, 8.8, 35.4, 2.2, 26.6, 24);
            p.cubicTo(26.6, 24, 23, 31.2, 21, 33.2);
            p.cubicTo(19, 35.2, 90.601, 2.8, 90.601, 2.8);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#000000";
            sfp.strokeWidth =  -1;
            p.moveTo(94.401, 0.6);
            p.cubicTo(94.401, 0.6, 65.4, 12.8, 55.4, 12.4);
            p.cubicTo(55.4, 12.4, 39, 7.8, 30.6, 22.4);
            p.cubicTo(30.6, 22.4, 22.2, 31.6, 19, 33.2);
            p.cubicTo(19, 33.2, 18.6, 34.8, 25, 30.8);
            p.lineTo(35.4, 36);
            p.cubicTo(35.4, 36, 50.2, 45.6, 59.8, 29.6);
            p.cubicTo(59.8, 29.6, 63.8, 18.4, 63.8, 16.4);
            p.cubicTo(63.8, 14.4, 85, 8.8, 86.601, 8.4);
            p.cubicTo(88.201, 8, 94.801, 3.8, 94.401, 0.6);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#99cc32";
            sfp.strokeWidth =  -1;
            p.moveTo(47, 36.514);
            p.cubicTo(40.128, 36.514, 31.755, 32.649, 31.755, 26.4);
            p.cubicTo(31.755, 20.152, 40.128, 13.887, 47, 13.887);
            p.cubicTo(53.874, 13.887, 59.446, 18.952, 59.446, 25.2);
            p.cubicTo(59.446, 31.449, 53.874, 36.514, 47, 36.514);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#659900";
            sfp.strokeWidth =  -1;
            p.moveTo(43.377, 19.83);
            p.cubicTo(38.531, 20.552, 33.442, 22.055, 33.514, 21.839);
            p.cubicTo(35.054, 17.22, 41.415, 13.887, 47, 13.887);
            p.cubicTo(51.296, 13.887, 55.084, 15.865, 57.32, 18.875);
            p.cubicTo(57.32, 18.875, 52.004, 18.545, 43.377, 19.83);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#ffffff";
            sfp.strokeWidth =  -1;
            p.moveTo(55.4, 19.6);
            p.cubicTo(55.4, 19.6, 51, 16.4, 51, 18.6);
            p.cubicTo(51, 18.6, 54.6, 23, 55.4, 19.6);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#000000";
            sfp.strokeWidth =  -1;
            p.moveTo(45.4, 27.726);
            p.cubicTo(42.901, 27.726, 40.875, 25.7, 40.875, 23.2);
            p.cubicTo(40.875, 20.701, 42.901, 18.675, 45.4, 18.675);
            p.cubicTo(47.9, 18.675, 49.926, 20.701, 49.926, 23.2);
            p.cubicTo(49.926, 25.7, 47.9, 27.726, 45.4, 27.726);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#cc7226";
            sfp.strokeWidth =  -1;
            p.moveTo(-58.6, 14.4);
            p.cubicTo(-58.6, 14.4, -61.8, -6.8, -59.4, -11.2);
            p.cubicTo(-59.4, -11.2, -48.6, -21.2, -49, -24.8);
            p.cubicTo(-49, -24.8, -49.4, -42.8, -50.6, -43.6);
            p.cubicTo(-51.8, -44.4, -59.4, -50.4, -65.4, -44);
            p.cubicTo(-65.4, -44, -75.8, -26, -75, -19.6);
            p.lineTo(-75, -17.6);
            p.cubicTo(-75, -17.6, -82.6, -18, -84.2, -16);
            p.cubicTo(-84.2, -16, -85.4, -10.8, -86.6, -10.4);
            p.cubicTo(-86.6, -10.4, -89.4, -8, -87.4, -5.2);
            p.cubicTo(-87.4, -5.2, -89.4, -2.8, -89, 1.2);
            p.lineTo(-81.4, 5.2);
            p.cubicTo(-81.4, 5.2, -79.4, 19.6, -68.6, 24.8);
            p.cubicTo(-63.764, 27.129, -60.6, 20.4, -58.6, 14.4);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#ffffff";
            sfp.strokeWidth =  -1;
            p.moveTo(-59.6, 12.56);
            p.cubicTo(-59.6, 12.56, -62.48, -6.52, -60.32, -10.48);
            p.cubicTo(-60.32, -10.48, -50.6, -19.48, -50.96, -22.72);
            p.cubicTo(-50.96, -22.72, -51.32, -38.92, -52.4, -39.64);
            p.cubicTo(-53.48, -40.36, -60.32, -45.76, -65.72, -40);
            p.cubicTo(-65.72, -40, -75.08, -23.8, -74.36, -18.04);
            p.lineTo(-74.36, -16.24);
            p.cubicTo(-74.36, -16.24, -81.2, -16.6, -82.64, -14.8);
            p.cubicTo(-82.64, -14.8, -83.72, -10.12, -84.8, -9.76);
            p.cubicTo(-84.8, -9.76, -87.32, -7.6, -85.52, -5.08);
            p.cubicTo(-85.52, -5.08, -87.32, -2.92, -86.96, 0.68);
            p.lineTo(-80.12, 4.28);
            p.cubicTo(-80.12, 4.28, -78.32, 17.24, -68.6, 21.92);
            p.cubicTo(-64.248, 24.015, -61.4, 17.96, -59.6, 12.56);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#eb955c";
            sfp.strokeWidth =  -1;
            p.moveTo(-51.05, -42.61);
            p.cubicTo(-52.14, -43.47, -59.63, -49.24, -65.48, -43);
            p.cubicTo(-65.48, -43, -75.62, -25.45, -74.84, -19.21);
            p.lineTo(-74.84, -17.26);
            p.cubicTo(-74.84, -17.26, -82.25, -17.65, -83.81, -15.7);
            p.cubicTo(-83.81, -15.7, -84.98, -10.63, -86.15, -10.24);
            p.cubicTo(-86.15, -10.24, -88.88, -7.9, -86.93, -5.17);
            p.cubicTo(-86.93, -5.17, -88.88, -2.83, -88.49, 1.07);
            p.lineTo(-81.08, 4.97);
            p.cubicTo(-81.08, 4.97, -79.13, 19.01, -68.6, 24.08);
            p.cubicTo(-63.886, 26.35, -60.8, 19.79, -58.85, 13.94);
            p.cubicTo(-58.85, 13.94, -61.97, -6.73, -59.63, -11.02);
            p.cubicTo(-59.63, -11.02, -49.1, -20.77, -49.49, -24.28);
            p.cubicTo(-49.49, -24.28, -49.88, -41.83, -51.05, -42.61);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#f2b892";
            sfp.strokeWidth =  -1;
            p.moveTo(-51.5, -41.62);
            p.cubicTo(-52.48, -42.54, -59.86, -48.08, -65.56, -42);
            p.cubicTo(-65.56, -42, -75.44, -24.9, -74.68, -18.82);
            p.lineTo(-74.68, -16.92);
            p.cubicTo(-74.68, -16.92, -81.9, -17.3, -83.42, -15.4);
            p.cubicTo(-83.42, -15.4, -84.56, -10.46, -85.7, -10.08);
            p.cubicTo(-85.7, -10.08, -88.36, -7.8, -86.46, -5.14);
            p.cubicTo(-86.46, -5.14, -88.36, -2.86, -87.98, 0.94);
            p.lineTo(-80.76, 4.74);
            p.cubicTo(-80.76, 4.74, -78.86, 18.42, -68.6, 23.36);
            p.cubicTo(-64.006, 25.572, -61, 19.18, -59.1, 13.48);
            p.cubicTo(-59.1, 13.48, -62.14, -6.66, -59.86, -10.84);
            p.cubicTo(-59.86, -10.84, -49.6, -20.34, -49.98, -23.76);
            p.cubicTo(-49.98, -23.76, -50.36, -40.86, -51.5, -41.62);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#f8dcc8";
            sfp.strokeWidth =  -1;
            p.moveTo(-51.95, -40.63);
            p.cubicTo(-52.82, -41.61, -60.09, -46.92, -65.64, -41);
            p.cubicTo(-65.64, -41, -75.26, -24.35, -74.52, -18.43);
            p.lineTo(-74.52, -16.58);
            p.cubicTo(-74.52, -16.58, -81.55, -16.95, -83.03, -15.1);
            p.cubicTo(-83.03, -15.1, -84.14, -10.29, -85.25, -9.92);
            p.cubicTo(-85.25, -9.92, -87.84, -7.7, -85.99, -5.11);
            p.cubicTo(-85.99, -5.11, -87.84, -2.89, -87.47, 0.81);
            p.lineTo(-80.44, 4.51);
            p.cubicTo(-80.44, 4.51, -78.59, 17.83, -68.6, 22.64);
            p.cubicTo(-64.127, 24.794, -61.2, 18.57, -59.35, 13.02);
            p.cubicTo(-59.35, 13.02, -62.31, -6.59, -60.09, -10.66);
            p.cubicTo(-60.09, -10.66, -50.1, -19.91, -50.47, -23.24);
            p.cubicTo(-50.47, -23.24, -50.84, -39.89, -51.95, -40.63);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#ffffff";
            sfp.strokeWidth =  -1;
            p.moveTo(-59.6, 12.46);
            p.cubicTo(-59.6, 12.46, -62.48, -6.52, -60.32, -10.48);
            p.cubicTo(-60.32, -10.48, -50.6, -19.48, -50.96, -22.72);
            p.cubicTo(-50.96, -22.72, -51.32, -38.92, -52.4, -39.64);
            p.cubicTo(-53.16, -40.68, -60.32, -45.76, -65.72, -40);
            p.cubicTo(-65.72, -40, -75.08, -23.8, -74.36, -18.04);
            p.lineTo(-74.36, -16.24);
            p.cubicTo(-74.36, -16.24, -81.2, -16.6, -82.64, -14.8);
            p.cubicTo(-82.64, -14.8, -83.72, -10.12, -84.8, -9.76);
            p.cubicTo(-84.8, -9.76, -87.32, -7.6, -85.52, -5.08);
            p.cubicTo(-85.52, -5.08, -87.32, -2.92, -86.96, 0.68);
            p.lineTo(-80.12, 4.28);
            p.cubicTo(-80.12, 4.28, -78.32, 17.24, -68.6, 21.92);
            p.cubicTo(-64.248, 24.015, -61.4, 17.86, -59.6, 12.46);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#cccccc";
            sfp.strokeWidth =  -1;
            p.moveTo(-62.7, 6.2);
            p.cubicTo(-62.7, 6.2, -84.3, -4, -85.2, -4.8);
            p.cubicTo(-85.2, -4.8, -76.1, 3.4, -75.3, 3.4);
            p.cubicTo(-74.5, 3.4, -62.7, 6.2, -62.7, 6.2);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#000000";
            sfp.strokeWidth =  -1;
            p.moveTo(-79.8, 0);
            p.cubicTo(-79.8, 0, -61.4, 3.6, -61.4, 8);
            p.cubicTo(-61.4, 10.912, -61.643, 24.331, -67, 22.8);
            p.cubicTo(-75.4, 20.4, -71.8, 6, -79.8, 0);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#99cc32";
            sfp.strokeWidth =  -1;
            p.moveTo(-71.4, 3.8);
            p.cubicTo(-71.4, 3.8, -62.422, 5.274, -61.4, 8);
            p.cubicTo(-60.8, 9.6, -60.137, 17.908, -65.6, 19);
            p.cubicTo(-70.152, 19.911, -72.382, 9.69, -71.4, 3.8);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#000000";
            sfp.strokeWidth =  -1;
            p.moveTo(14.595, 46.349);
            p.cubicTo(14.098, 44.607, 15.409, 44.738, 17.2, 44.2);
            p.cubicTo(19.2, 43.6, 31.4, 39.8, 32.2, 37.2);
            p.cubicTo(33, 34.6, 46.2, 39, 46.2, 39);
            p.cubicTo(48, 39.8, 52.4, 42.4, 52.4, 42.4);
            p.cubicTo(57.2, 43.6, 63.8, 44, 63.8, 44);
            p.cubicTo(66.2, 45, 69.6, 47.8, 69.6, 47.8);
            p.cubicTo(84.2, 58, 96.601, 50.8, 96.601, 50.8);
            p.cubicTo(116.601, 44.2, 110.601, 27, 110.601, 27);
            p.cubicTo(107.601, 18, 110.801, 14.6, 110.801, 14.6);
            p.cubicTo(111.001, 10.8, 118.201, 17.2, 118.201, 17.2);
            p.cubicTo(120.801, 21.4, 121.601, 26.4, 121.601, 26.4);
            p.cubicTo(129.601, 37.6, 126.201, 19.8, 126.201, 19.8);
            p.cubicTo(126.401, 18.8, 123.601, 15.2, 123.601, 14);
            p.cubicTo(123.601, 12.8, 121.801, 9.4, 121.801, 9.4);
            p.cubicTo(118.801, 6, 121.201, -1, 121.201, -1);
            p.cubicTo(123.001, -14.8, 120.801, -13, 120.801, -13);
            p.cubicTo(119.601, -14.8, 110.401, -4.8, 110.401, -4.8);
            p.cubicTo(108.201, -1.4, 102.201, 0.2, 102.201, 0.2);
            p.cubicTo(99.401, 2, 96.001, 0.6, 96.001, 0.6);
            p.cubicTo(93.401, 0.2, 87.801, 7.2, 87.801, 7.2);
            p.cubicTo(90.601, 7, 93.001, 11.4, 95.401, 11.6);
            p.cubicTo(97.801, 11.8, 99.601, 9.2, 101.201, 8.6);
            p.cubicTo(102.801, 8, 105.601, 13.8, 105.601, 13.8);
            p.cubicTo(106.001, 16.4, 100.401, 21.2, 100.401, 21.2);
            p.cubicTo(100.001, 25.8, 98.401, 24.2, 98.401, 24.2);
            p.cubicTo(95.401, 23.6, 94.201, 27.4, 93.201, 32);
            p.cubicTo(92.201, 36.6, 88.001, 37, 88.001, 37);
            p.cubicTo(86.401, 44.4, 85.2, 41.4, 85.2, 41.4);
            p.cubicTo(85, 35.8, 79, 41.6, 79, 41.6);
            p.cubicTo(77.8, 43.6, 73.2, 41.4, 73.2, 41.4);
            p.cubicTo(66.4, 39.4, 68.8, 37.4, 68.8, 37.4);
            p.cubicTo(70.6, 35.2, 81.8, 37.4, 81.8, 37.4);
            p.cubicTo(84, 35.8, 76, 31.8, 76, 31.8);
            p.cubicTo(75.4, 30, 76.4, 25.6, 76.4, 25.6);
            p.cubicTo(77.6, 22.4, 84.4, 16.8, 84.4, 16.8);
            p.cubicTo(93.801, 15.6, 91.001, 14, 91.001, 14);
            p.cubicTo(84.801, 8.8, 79, 16.4, 79, 16.4);
            p.cubicTo(76.8, 22.6, 59.4, 37.6, 59.4, 37.6);
            p.cubicTo(54.6, 41, 57.2, 34.2, 53.2, 37.6);
            p.cubicTo(49.2, 41, 28.6, 32, 28.6, 32);
            p.cubicTo(17.038, 30.807, 14.306, 46.549, 10.777, 43.429);
            p.cubicTo(10.777, 43.429, 16.195, 51.949, 14.595, 46.349);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#000000";
            sfp.strokeWidth =  -1;
            p.moveTo(209.401, -120);
            p.cubicTo(209.401, -120, 183.801, -112, 181.001, -93.2);
            p.cubicTo(181.001, -93.2, 178.601, -70.4, 199.001, -52.8);
            p.cubicTo(199.001, -52.8, 199.401, -46.4, 201.401, -43.2);
            p.cubicTo(201.401, -43.2, 199.801, -38.4, 218.601, -46);
            p.lineTo(245.801, -54.4);
            p.cubicTo(245.801, -54.4, 252.201, -56.8, 257.401, -65.6);
            p.cubicTo(262.601, -74.4, 277.801, -93.2, 274.201, -118.4);
            p.cubicTo(274.201, -118.4, 275.401, -129.6, 269.401, -130);
            p.cubicTo(269.401, -130, 261.001, -131.6, 253.801, -124);
            p.cubicTo(253.801, -124, 247.001, -120.8, 244.601, -121.2);
            p.lineTo(209.401, -120);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#000000";
            sfp.strokeWidth =  -1;
            p.moveTo(264.022, -120.99);
            p.cubicTo(264.022, -120.99, 266.122, -129.92, 261.282, -125.08);
            p.cubicTo(261.282, -125.08, 254.242, -119.36, 246.761, -119.36);
            p.cubicTo(246.761, -119.36, 232.241, -117.16, 227.841, -103.96);
            p.cubicTo(227.841, -103.96, 223.881, -77.12, 231.801, -71.4);
            p.cubicTo(231.801, -71.4, 236.641, -63.92, 243.681, -70.52);
            p.cubicTo(250.722, -77.12, 266.222, -107.35, 264.022, -120.99);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#323232";
            sfp.strokeWidth =  -1;
            p.moveTo(263.648, -120.632);
            p.cubicTo(263.648, -120.632, 265.738, -129.376, 260.986, -124.624);
            p.cubicTo(260.986, -124.624, 254.074, -119.008, 246.729, -119.008);
            p.cubicTo(246.729, -119.008, 232.473, -116.848, 228.153, -103.888);
            p.cubicTo(228.153, -103.888, 224.265, -77.536, 232.041, -71.92);
            p.cubicTo(232.041, -71.92, 236.793, -64.576, 243.705, -71.056);
            p.cubicTo(250.618, -77.536, 265.808, -107.24, 263.648, -120.632);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#666666";
            sfp.strokeWidth =  -1;
            p.moveTo(263.274, -120.274);
            p.cubicTo(263.274, -120.274, 265.354, -128.832, 260.69, -124.168);
            p.cubicTo(260.69, -124.168, 253.906, -118.656, 246.697, -118.656);
            p.cubicTo(246.697, -118.656, 232.705, -116.536, 228.465, -103.816);
            p.cubicTo(228.465, -103.816, 224.649, -77.952, 232.281, -72.44);
            p.cubicTo(232.281, -72.44, 236.945, -65.232, 243.729, -71.592);
            p.cubicTo(250.514, -77.952, 265.394, -107.13, 263.274, -120.274);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#999999";
            sfp.strokeWidth =  -1;
            p.moveTo(262.9, -119.916);
            p.cubicTo(262.9, -119.916, 264.97, -128.288, 260.394, -123.712);
            p.cubicTo(260.394, -123.712, 253.738, -118.304, 246.665, -118.304);
            p.cubicTo(246.665, -118.304, 232.937, -116.224, 228.777, -103.744);
            p.cubicTo(228.777, -103.744, 225.033, -78.368, 232.521, -72.96);
            p.cubicTo(232.521, -72.96, 237.097, -65.888, 243.753, -72.128);
            p.cubicTo(250.41, -78.368, 264.98, -107.02, 262.9, -119.916);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#cccccc";
            sfp.strokeWidth =  -1;
            p.moveTo(262.526, -119.558);
            p.cubicTo(262.526, -119.558, 264.586, -127.744, 260.098, -123.256);
            p.cubicTo(260.098, -123.256, 253.569, -117.952, 246.633, -117.952);
            p.cubicTo(246.633, -117.952, 233.169, -115.912, 229.089, -103.672);
            p.cubicTo(229.089, -103.672, 225.417, -78.784, 232.761, -73.48);
            p.cubicTo(232.761, -73.48, 237.249, -66.544, 243.777, -72.664);
            p.cubicTo(250.305, -78.784, 264.566, -106.91, 262.526, -119.558);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#ffffff";
            sfp.strokeWidth =  -1;
            p.moveTo(262.151, -119.2);
            p.cubicTo(262.151, -119.2, 264.201, -127.2, 259.801, -122.8);
            p.cubicTo(259.801, -122.8, 253.401, -117.6, 246.601, -117.6);
            p.cubicTo(246.601, -117.6, 233.401, -115.6, 229.401, -103.6);
            p.cubicTo(229.401, -103.6, 225.801, -79.2, 233.001, -74);
            p.cubicTo(233.001, -74, 237.401, -67.2, 243.801, -73.2);
            p.cubicTo(250.201, -79.2, 264.151, -106.8, 262.151, -119.2);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#992600";
            sfp.strokeWidth =  -1;
            p.moveTo(50.6, 84);
            p.cubicTo(50.6, 84, 30.2, 64.8, 22.2, 64);
            p.cubicTo(22.2, 64, -12.2, 60, -27, 78);
            p.cubicTo(-27, 78, -9.4, 57.6, 18.2, 63.2);
            p.cubicTo(18.2, 63.2, -3.4, 58.8, -15.8, 62);
            p.cubicTo(-15.8, 62, -32.6, 62, -42.2, 76);
            p.lineTo(-45, 80.8);
            p.cubicTo(-45, 80.8, -41, 66, -22.6, 60);
            p.cubicTo(-22.6, 60, 0.2, 55.2, 11, 60);
            p.cubicTo(11, 60, -10.6, 53.2, -20.6, 55.2);
            p.cubicTo(-20.6, 55.2, -51, 52.8, -63.8, 79.2);
            p.cubicTo(-63.8, 79.2, -59.8, 64.8, -45, 57.6);
            p.cubicTo(-45, 57.6, -31.4, 48.8, -11, 51.6);
            p.cubicTo(-11, 51.6, 3.4, 54.8, 8.6, 57.2);
            p.cubicTo(13.8, 59.6, 12.6, 56.8, 4.2, 52);
            p.cubicTo(4.2, 52, -1.4, 42, -15.4, 42.4);
            p.cubicTo(-15.4, 42.4, -58.2, 46, -68.6, 58);
            p.cubicTo(-68.6, 58, -55, 46.8, -44.6, 44);
            p.cubicTo(-44.6, 44, -22.2, 36, -13.8, 36.8);
            p.cubicTo(-13.8, 36.8, 11, 37.8, 18.6, 33.8);
            p.cubicTo(18.6, 33.8, 7.4, 38.8, 10.6, 42);
            p.cubicTo(13.8, 45.2, 20.6, 52.8, 20.6, 54);
            p.cubicTo(20.6, 55.2, 44.8, 77.3, 48.4, 81.7);
            p.lineTo(50.6, 84);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#cccccc";
            sfp.strokeWidth =  -1;
            p.moveTo(189, 278);
            p.cubicTo(189, 278, 173.5, 241.5, 161, 232);
            p.cubicTo(161, 232, 187, 248, 190.5, 266);
            p.cubicTo(190.5, 266, 190.5, 276, 189, 278);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#cccccc";
            sfp.strokeWidth =  -1;
            p.moveTo(236, 285.5);
            p.cubicTo(236, 285.5, 209.5, 230.5, 191, 206.5);
            p.cubicTo(191, 206.5, 234.5, 244, 239.5, 270.5);
            p.lineTo(240, 276);
            p.lineTo(237, 273.5);
            p.cubicTo(237, 273.5, 236.5, 282.5, 236, 285.5);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#cccccc";
            sfp.strokeWidth =  -1;
            p.moveTo(292.5, 237);
            p.cubicTo(292.5, 237, 230, 177.5, 228.5, 175);
            p.cubicTo(228.5, 175, 289, 241, 292, 248.5);
            p.cubicTo(292, 248.5, 290, 239.5, 292.5, 237);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#cccccc";
            sfp.strokeWidth =  -1;
            p.moveTo(104, 280.5);
            p.cubicTo(104, 280.5, 123.5, 228.5, 142.5, 251);
            p.cubicTo(142.5, 251, 157.5, 261, 157, 264);
            p.cubicTo(157, 264, 153, 257.5, 135, 258);
            p.cubicTo(135, 258, 116, 255, 104, 280.5);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#cccccc";
            sfp.strokeWidth =  -1;
            p.moveTo(294.5, 153);
            p.cubicTo(294.5, 153, 249.5, 124.5, 242, 123);
            p.cubicTo(230.193, 120.639, 291.5, 152, 296.5, 162.5);
            p.cubicTo(296.5, 162.5, 298.5, 160, 294.5, 153);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#000000";
            sfp.strokeWidth =  -1;
            p.moveTo(143.801, 259.601);
            p.cubicTo(143.801, 259.601, 164.201, 257.601, 171.001, 250.801);
            p.lineTo(175.401, 254.401);
            p.lineTo(193.001, 216.001);
            p.lineTo(196.601, 221.201);
            p.cubicTo(196.601, 221.201, 211.001, 206.401, 210.201, 198.401);
            p.cubicTo(209.401, 190.401, 223.001, 204.401, 223.001, 204.401);
            p.cubicTo(223.001, 204.401, 222.201, 192.801, 229.401, 199.601);
            p.cubicTo(229.401, 199.601, 227.001, 184.001, 235.401, 192.001);
            p.cubicTo(235.401, 192.001, 224.864, 161.844, 247.401, 187.601);
            p.cubicTo(253.001, 194.001, 248.601, 187.201, 248.601, 187.201);
            p.cubicTo(248.601, 187.201, 222.601, 139.201, 244.201, 153.601);
            p.cubicTo(244.201, 153.601, 246.201, 130.801, 245.001, 126.401);
            p.cubicTo(243.801, 122.001, 241.801, 99.6, 237.001, 94.4);
            p.cubicTo(232.201, 89.2, 237.401, 87.6, 243.001, 92.8);
            p.cubicTo(243.001, 92.8, 231.801, 68.8, 245.001, 80.8);
            p.cubicTo(245.001, 80.8, 241.401, 65.6, 237.001, 62.8);
            p.cubicTo(237.001, 62.8, 231.401, 45.6, 246.601, 56.4);
            p.cubicTo(246.601, 56.4, 242.201, 44, 239.001, 40.8);
            p.cubicTo(239.001, 40.8, 227.401, 13.2, 234.601, 18);
            p.lineTo(239.001, 21.6);
            p.cubicTo(239.001, 21.6, 232.201, 7.6, 238.601, 12);
            p.cubicTo(245.001, 16.4, 245.001, 16, 245.001, 16);
            p.cubicTo(245.001, 16, 223.801, -17.2, 244.201, 0.4);
            p.cubicTo(244.201, 0.4, 236.042, -13.518, 232.601, -20.4);
            p.cubicTo(232.601, -20.4, 213.801, -40.8, 228.201, -34.4);
            p.lineTo(233.001, -32.8);
            p.cubicTo(233.001, -32.8, 224.201, -42.8, 216.201, -44.4);
            p.cubicTo(208.201, -46, 218.601, -52.4, 225.001, -50.4);
            p.cubicTo(231.401, -48.4, 247.001, -40.8, 247.001, -40.8);
            p.cubicTo(247.001, -40.8, 259.801, -22, 263.801, -21.6);
            p.cubicTo(263.801, -21.6, 243.801, -29.2, 249.801, -21.2);
            p.cubicTo(249.801, -21.2, 264.201, -7.2, 257.001, -7.6);
            p.cubicTo(257.001, -7.6, 251.001, -0.4, 255.801, 8.4);
            p.cubicTo(255.801, 8.4, 237.342, -9.991, 252.201, 15.6);
            p.lineTo(259.001, 32);
            p.cubicTo(259.001, 32, 234.601, 7.2, 245.801, 29.2);
            p.cubicTo(245.801, 29.2, 263.001, 52.8, 265.001, 53.2);
            p.cubicTo(267.001, 53.6, 271.401, 62.4, 271.401, 62.4);
            p.lineTo(267.001, 60.4);
            p.lineTo(272.201, 69.2);
            p.cubicTo(272.201, 69.2, 261.001, 57.2, 267.001, 70.4);
            p.lineTo(272.601, 84.8);
            p.cubicTo(272.601, 84.8, 252.201, 62.8, 265.801, 92.4);
            p.cubicTo(265.801, 92.4, 249.401, 87.2, 258.201, 104.4);
            p.cubicTo(258.201, 104.4, 256.601, 120.401, 257.001, 125.601);
            p.cubicTo(257.401, 130.801, 258.601, 159.201, 254.201, 167.201);
            p.cubicTo(249.801, 175.201, 260.201, 194.401, 262.201, 198.401);
            p.cubicTo(264.201, 202.401, 267.801, 213.201, 259.001, 204.001);
            p.cubicTo(250.201, 194.801, 254.601, 200.401, 256.601, 209.201);
            p.cubicTo(258.601, 218.001, 264.601, 233.601, 263.801, 239.201);
            p.cubicTo(263.801, 239.201, 262.601, 240.401, 259.401, 236.801);
            p.cubicTo(259.401, 236.801, 244.601, 214.001, 246.201, 228.401);
            p.cubicTo(246.201, 228.401, 245.001, 236.401, 241.801, 245.201);
            p.cubicTo(241.801, 245.201, 238.601, 256.001, 238.601, 247.201);
            p.cubicTo(238.601, 247.201, 235.401, 230.401, 232.601, 238.001);
            p.cubicTo(229.801, 245.601, 226.201, 251.601, 223.401, 254.001);
            p.cubicTo(220.601, 256.401, 215.401, 233.601, 214.201, 244.001);
            p.cubicTo(214.201, 244.001, 202.201, 231.601, 197.401, 248.001);
            p.lineTo(185.801, 264.401);
            p.cubicTo(185.801, 264.401, 185.401, 252.001, 184.201, 258.001);
            p.cubicTo(184.201, 258.001, 154.201, 264.001, 143.801, 259.601);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#000000";
            sfp.strokeWidth =  -1;
            p.moveTo(109.401, -97.2);
            p.cubicTo(109.401, -97.2, 97.801, -105.2, 93.801, -104.8);
            p.cubicTo(89.801, -104.4, 121.401, -113.6, 162.601, -86);
            p.cubicTo(162.601, -86, 167.401, -83.2, 171.001, -83.6);
            p.cubicTo(171.001, -83.6, 174.201, -81.2, 171.401, -77.6);
            p.cubicTo(171.401, -77.6, 162.601, -68, 173.801, -56.8);
            p.cubicTo(173.801, -56.8, 192.201, -50, 186.601, -58.8);
            p.cubicTo(186.601, -58.8, 197.401, -54.8, 199.801, -50.8);
            p.cubicTo(202.201, -46.8, 201.001, -50.8, 201.001, -50.8);
            p.cubicTo(201.001, -50.8, 194.601, -58, 188.601, -63.2);
            p.cubicTo(188.601, -63.2, 183.401, -65.2, 180.601, -73.6);
            p.cubicTo(177.801, -82, 175.401, -92, 179.801, -95.2);
            p.cubicTo(179.801, -95.2, 175.801, -90.8, 176.601, -94.8);
            p.cubicTo(177.401, -98.8, 181.001, -102.4, 182.601, -102.8);
            p.cubicTo(184.201, -103.2, 200.601, -119, 207.401, -119.4);
            p.cubicTo(207.401, -119.4, 198.201, -118, 195.201, -119);
            p.cubicTo(192.201, -120, 165.601, -131.4, 159.601, -132.6);
            p.cubicTo(159.601, -132.6, 142.801, -139.2, 154.801, -137.2);
            p.cubicTo(154.801, -137.2, 190.601, -133.4, 208.801, -120.2);
            p.cubicTo(208.801, -120.2, 201.601, -128.6, 183.201, -135.6);
            p.cubicTo(183.201, -135.6, 161.001, -148.2, 125.801, -143.2);
            p.cubicTo(125.801, -143.2, 108.001, -140, 100.201, -138.2);
            p.cubicTo(100.201, -138.2, 97.601, -138.8, 97.001, -139.2);
            p.cubicTo(96.401, -139.6, 84.6, -148.6, 57, -141.6);
            p.cubicTo(57, -141.6, 40, -137, 31.4, -132.2);
            p.cubicTo(31.4, -132.2, 16.2, -131, 12.6, -127.8);
            p.cubicTo(12.6, -127.8, -6, -113.2, -8, -112.4);
            p.cubicTo(-10, -111.6, -21.4, -104, -22.2, -103.6);
            p.cubicTo(-22.2, -103.6, 2.4, -110.2, 4.8, -112.6);
            p.cubicTo(7.2, -115, 24.6, -117.6, 27, -116.2);
            p.cubicTo(29.4, -114.8, 37.8, -115.4, 28.2, -114.8);
            p.cubicTo(28.2, -114.8, 103.801, -100, 104.601, -98);
            p.cubicTo(105.401, -96, 109.401, -97.2, 109.401, -97.2);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#cc7226";
            sfp.strokeWidth =  -1;
            p.moveTo(180.801, -106.4);
            p.cubicTo(180.801, -106.4, 170.601, -113.8, 168.601, -113.8);
            p.cubicTo(166.601, -113.8, 154.201, -124, 150.001, -123.6);
            p.cubicTo(145.801, -123.2, 133.601, -133.2, 106.201, -125);
            p.cubicTo(106.201, -125, 105.601, -127, 109.201, -127.8);
            p.cubicTo(109.201, -127.8, 115.601, -130, 116.001, -130.6);
            p.cubicTo(116.001, -130.6, 136.201, -134.8, 143.401, -131.2);
            p.cubicTo(143.401, -131.2, 152.601, -128.6, 158.801, -122.4);
            p.cubicTo(158.801, -122.4, 170.001, -119.2, 173.201, -120.2);
            p.cubicTo(173.201, -120.2, 182.001, -118, 182.401, -116.2);
            p.cubicTo(182.401, -116.2, 188.201, -113.2, 186.401, -110.6);
            p.cubicTo(186.401, -110.6, 186.801, -109, 180.801, -106.4);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#cc7226";
            sfp.strokeWidth =  -1;
            p.moveTo(168.33, -108.509);
            p.cubicTo(169.137, -107.877, 170.156, -107.779, 170.761, -106.97);
            p.cubicTo(170.995, -106.656, 170.706, -106.33, 170.391, -106.233);
            p.cubicTo(169.348, -105.916, 168.292, -106.486, 167.15, -105.898);
            p.cubicTo(166.748, -105.691, 166.106, -105.873, 165.553, -106.022);
            p.cubicTo(163.921, -106.463, 162.092, -106.488, 160.401, -105.8);
            p.cubicTo(158.416, -106.929, 156.056, -106.345, 153.975, -107.346);
            p.cubicTo(153.917, -107.373, 153.695, -107.027, 153.621, -107.054);
            p.cubicTo(150.575, -108.199, 146.832, -107.916, 144.401, -110.2);
            p.cubicTo(141.973, -110.612, 139.616, -111.074, 137.188, -111.754);
            p.cubicTo(135.37, -112.263, 133.961, -113.252, 132.341, -114.084);
            p.cubicTo(130.964, -114.792, 129.507, -115.314, 127.973, -115.686);
            p.cubicTo(126.11, -116.138, 124.279, -116.026, 122.386, -116.546);
            p.cubicTo(122.293, -116.571, 122.101, -116.227, 122.019, -116.254);
            p.cubicTo(121.695, -116.362, 121.405, -116.945, 121.234, -116.892);
            p.cubicTo(119.553, -116.37, 118.065, -117.342, 116.401, -117);
            p.cubicTo(115.223, -118.224, 113.495, -117.979, 111.949, -118.421);
            p.cubicTo(108.985, -119.269, 105.831, -117.999, 102.801, -119);
            p.cubicTo(106.914, -120.842, 111.601, -119.61, 115.663, -121.679);
            p.cubicTo(117.991, -122.865, 120.653, -121.763, 123.223, -122.523);
            p.cubicTo(123.71, -122.667, 124.401, -122.869, 124.801, -122.2);
            p.cubicTo(124.935, -122.335, 125.117, -122.574, 125.175, -122.546);
            p.cubicTo(127.625, -121.389, 129.94, -120.115, 132.422, -119.049);
            p.cubicTo(132.763, -118.903, 133.295, -119.135, 133.547, -118.933);
            p.cubicTo(135.067, -117.717, 137.01, -117.82, 138.401, -116.6);
            p.cubicTo(140.099, -117.102, 141.892, -116.722, 143.621, -117.346);
            p.cubicTo(143.698, -117.373, 143.932, -117.032, 143.965, -117.054);
            p.cubicTo(145.095, -117.802, 146.25, -117.531, 147.142, -117.227);
            p.cubicTo(147.48, -117.112, 148.143, -116.865, 148.448, -116.791);
            p.cubicTo(149.574, -116.515, 150.43, -116.035, 151.609, -115.852);
            p.cubicTo(151.723, -115.834, 151.908, -116.174, 151.98, -116.146);
            p.cubicTo(153.103, -115.708, 154.145, -115.764, 154.801, -114.6);
            p.cubicTo(154.936, -114.735, 155.101, -114.973, 155.183, -114.946);
            p.cubicTo(156.21, -114.608, 156.859, -113.853, 157.96, -113.612);
            p.cubicTo(158.445, -113.506, 159.057, -112.88, 159.633, -112.704);
            p.cubicTo(162.025, -111.973, 163.868, -110.444, 166.062, -109.549);
            p.cubicTo(166.821, -109.239, 167.697, -109.005, 168.33, -108.509);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#cc7226";
            sfp.strokeWidth =  -1;
            p.moveTo(91.696, -122.739);
            p.cubicTo(89.178, -124.464, 86.81, -125.57, 84.368, -127.356);
            p.cubicTo(84.187, -127.489, 83.827, -127.319, 83.625, -127.441);
            p.cubicTo(82.618, -128.05, 81.73, -128.631, 80.748, -129.327);
            p.cubicTo(80.209, -129.709, 79.388, -129.698, 78.88, -129.956);
            p.cubicTo(76.336, -131.248, 73.707, -131.806, 71.2, -133);
            p.cubicTo(71.882, -133.638, 73.004, -133.394, 73.6, -134.2);
            p.cubicTo(73.795, -133.92, 74.033, -133.636, 74.386, -133.827);
            p.cubicTo(76.064, -134.731, 77.914, -134.884, 79.59, -134.794);
            p.cubicTo(81.294, -134.702, 83.014, -134.397, 84.789, -134.125);
            p.cubicTo(85.096, -134.078, 85.295, -133.555, 85.618, -133.458);
            p.cubicTo(87.846, -132.795, 90.235, -133.32, 92.354, -132.482);
            p.cubicTo(93.945, -131.853, 95.515, -131.03, 96.754, -129.755);
            p.cubicTo(97.006, -129.495, 96.681, -129.194, 96.401, -129);
            p.cubicTo(96.789, -129.109, 97.062, -128.903, 97.173, -128.59);
            p.cubicTo(97.257, -128.351, 97.257, -128.049, 97.173, -127.81);
            p.cubicTo(97.061, -127.498, 96.782, -127.397, 96.408, -127.346);
            p.cubicTo(95.001, -127.156, 96.773, -128.536, 96.073, -128.088);
            p.cubicTo(94.8, -127.274, 95.546, -125.868, 94.801, -124.6);
            p.cubicTo(94.521, -124.794, 94.291, -125.012, 94.401, -125.4);
            p.cubicTo(94.635, -124.878, 94.033, -124.588, 93.865, -124.272);
            p.cubicTo(93.48, -123.547, 92.581, -122.132, 91.696, -122.739);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#cc7226";
            sfp.strokeWidth =  -1;
            p.moveTo(59.198, -115.391);
            p.cubicTo(56.044, -116.185, 52.994, -116.07, 49.978, -117.346);
            p.cubicTo(49.911, -117.374, 49.688, -117.027, 49.624, -117.054);
            p.cubicTo(48.258, -117.648, 47.34, -118.614, 46.264, -119.66);
            p.cubicTo(45.351, -120.548, 43.693, -120.161, 42.419, -120.648);
            p.cubicTo(42.095, -120.772, 41.892, -121.284, 41.591, -121.323);
            p.cubicTo(40.372, -121.48, 39.445, -122.429, 38.4, -123);
            p.cubicTo(40.736, -123.795, 43.147, -123.764, 45.609, -124.148);
            p.cubicTo(45.722, -124.166, 45.867, -123.845, 46, -123.845);
            p.cubicTo(46.136, -123.845, 46.266, -124.066, 46.4, -124.2);
            p.cubicTo(46.595, -123.92, 46.897, -123.594, 47.154, -123.848);
            p.cubicTo(47.702, -124.388, 48.258, -124.198, 48.798, -124.158);
            p.cubicTo(48.942, -124.148, 49.067, -123.845, 49.2, -123.845);
            p.cubicTo(49.336, -123.845, 49.467, -124.156, 49.6, -124.156);
            p.cubicTo(49.736, -124.155, 49.867, -123.845, 50, -123.845);
            p.cubicTo(50.136, -123.845, 50.266, -124.066, 50.4, -124.2);
            p.cubicTo(51.092, -123.418, 51.977, -123.972, 52.799, -123.793);
            p.cubicTo(53.837, -123.566, 54.104, -122.418, 55.178, -122.12);
            p.cubicTo(59.893, -120.816, 64.03, -118.671, 68.393, -116.584);
            p.cubicTo(68.7, -116.437, 68.91, -116.189, 68.8, -115.8);
            p.cubicTo(69.067, -115.8, 69.38, -115.888, 69.57, -115.756);
            p.cubicTo(70.628, -115.024, 71.669, -114.476, 72.366, -113.378);
            p.cubicTo(72.582, -113.039, 72.253, -112.632, 72.02, -112.684);
            p.cubicTo(67.591, -113.679, 63.585, -114.287, 59.198, -115.391);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#cc7226";
            sfp.strokeWidth =  -1;
            p.moveTo(45.338, -71.179);
            p.cubicTo(43.746, -72.398, 43.162, -74.429, 42.034, -76.221);
            p.cubicTo(41.82, -76.561, 42.094, -76.875, 42.411, -76.964);
            p.cubicTo(42.971, -77.123, 43.514, -76.645, 43.923, -76.443);
            p.cubicTo(45.668, -75.581, 47.203, -74.339, 49.2, -74.2);
            p.cubicTo(51.19, -71.966, 55.45, -71.581, 55.457, -68.2);
            p.cubicTo(55.458, -67.341, 54.03, -68.259, 53.6, -67.4);
            p.cubicTo(51.149, -68.403, 48.76, -68.3, 46.38, -69.767);
            p.cubicTo(45.763, -70.148, 46.093, -70.601, 45.338, -71.179);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#cc7226";
            sfp.strokeWidth =  -1;
            p.moveTo(17.8, -123.756);
            p.cubicTo(17.935, -123.755, 24.966, -123.522, 24.949, -123.408);
            p.cubicTo(24.904, -123.099, 17.174, -122.05, 16.81, -122.22);
            p.cubicTo(16.646, -122.296, 9.134, -119.866, 9, -120);
            p.cubicTo(9.268, -120.135, 17.534, -123.756, 17.8, -123.756);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#000000";
            sfp.strokeWidth =  -1;
            p.moveTo(33.2, -114);
            p.cubicTo(33.2, -114, 18.4, -112.2, 14, -111);
            p.cubicTo(9.6, -109.8, -9, -102.2, -12, -100.2);
            p.cubicTo(-12, -100.2, -25.4, -94.8, -42.4, -74.8);
            p.cubicTo(-42.4, -74.8, -34.8, -78.2, -32.6, -81);
            p.cubicTo(-32.6, -81, -19, -93.6, -19.2, -91);
            p.cubicTo(-19.2, -91, -7, -99.6, -7.6, -97.4);
            p.cubicTo(-7.6, -97.4, 16.8, -108.6, 14.8, -105.4);
            p.cubicTo(14.8, -105.4, 36.4, -110, 35.4, -108);
            p.cubicTo(35.4, -108, 54.2, -103.6, 51.4, -103.4);
            p.cubicTo(51.4, -103.4, 45.6, -102.2, 52, -98.6);
            p.cubicTo(52, -98.6, 48.6, -94.2, 43.2, -98.2);
            p.cubicTo(37.8, -102.2, 40.8, -100, 35.8, -99);
            p.cubicTo(35.8, -99, 33.2, -98.2, 28.6, -102.2);
            p.cubicTo(28.6, -102.2, 23, -106.8, 14.2, -103.2);
            p.cubicTo(14.2, -103.2, -16.4, -90.6, -18.4, -90);
            p.cubicTo(-18.4, -90, -22, -87.2, -24.4, -83.6);
            p.cubicTo(-24.4, -83.6, -30.2, -79.2, -33.2, -77.8);
            p.cubicTo(-33.2, -77.8, -46, -66.2, -47.2, -64.8);
            p.cubicTo(-47.2, -64.8, -50.6, -59.6, -51.4, -59.2);
            p.cubicTo(-51.4, -59.2, -45, -63, -43, -65);
            p.cubicTo(-43, -65, -29, -75, -23.6, -75.8);
            p.cubicTo(-23.6, -75.8, -19.2, -78.8, -18.4, -80.2);
            p.cubicTo(-18.4, -80.2, -4, -89.4, 0.2, -89.4);
            p.cubicTo(0.2, -89.4, 9.4, -84.2, 11.8, -91.2);
            p.cubicTo(11.8, -91.2, 17.6, -93, 23.2, -91.8);
            p.cubicTo(23.2, -91.8, 26.4, -94.4, 25.6, -96.6);
            p.cubicTo(25.6, -96.6, 27.2, -98.4, 28.2, -94.6);
            p.cubicTo(28.2, -94.6, 31.6, -91, 36.4, -93);
            p.cubicTo(36.4, -93, 40.4, -93.2, 38.4, -90.8);
            p.cubicTo(38.4, -90.8, 34, -87, 22.2, -86.8);
            p.cubicTo(22.2, -86.8, 9.8, -86.2, -6.6, -78.6);
            p.cubicTo(-6.6, -78.6, -36.4, -68.2, -45.6, -57.8);
            p.cubicTo(-45.6, -57.8, -52, -49, -57.4, -47.8);
            p.cubicTo(-57.4, -47.8, -63.2, -47, -69.2, -39.6);
            p.cubicTo(-69.2, -39.6, -59.4, -45.4, -50.4, -45.4);
            p.cubicTo(-50.4, -45.4, -46.4, -47.8, -50.2, -44.2);
            p.cubicTo(-50.2, -44.2, -53.8, -36.6, -52.2, -31.2);
            p.cubicTo(-52.2, -31.2, -52.8, -26, -53.6, -24.4);
            p.cubicTo(-53.6, -24.4, -61.4, -11.6, -61.4, -9.2);
            p.cubicTo(-61.4, -6.8, -60.2, 3, -59.8, 3.6);
            p.cubicTo(-59.4, 4.2, -60.8, 2, -57, 4.4);
            p.cubicTo(-53.2, 6.8, -50.4, 8.4, -49.6, 11.2);
            p.cubicTo(-48.8, 14, -51.6, 5.8, -51.8, 4);
            p.cubicTo(-52, 2.2, -56.2, -5, -55.4, -7.4);
            p.cubicTo(-55.4, -7.4, -54.4, -6.4, -53.6, -5);
            p.cubicTo(-53.6, -5, -54.2, -5.6, -53.6, -9.2);
            p.cubicTo(-53.6, -9.2, -52.8, -14.4, -51.4, -17.6);
            p.cubicTo(-50, -20.8, -48, -24.6, -47.6, -25.4);
            p.cubicTo(-47.2, -26.2, -47.2, -32, -45.8, -29.4);
            p.lineTo(-42.4, -26.8);
            p.cubicTo(-42.4, -26.8, -45.2, -29.4, -43, -31.6);
            p.cubicTo(-43, -31.6, -44, -37.2, -42.2, -39.8);
            p.cubicTo(-42.2, -39.8, -35.2, -48.2, -33.6, -49.2);
            p.cubicTo(-32, -50.2, -33.4, -49.8, -33.4, -49.8);
            p.cubicTo(-33.4, -49.8, -27.4, -54, -33.2, -52.4);
            p.cubicTo(-33.2, -52.4, -37.2, -50.8, -40.2, -50.8);
            p.cubicTo(-40.2, -50.8, -47.8, -48.8, -43.8, -53);
            p.cubicTo(-39.8, -57.2, -29.8, -62.6, -26, -62.4);
            p.lineTo(-25.2, -60.8);
            p.lineTo(-14, -63.2);
            p.lineTo(-15.2, -62.4);
            p.cubicTo(-15.2, -62.4, -15.4, -62.6, -11.2, -63);
            p.cubicTo(-7, -63.4, -1.2, -62, 0.2, -63.8);
            p.cubicTo(1.6, -65.6, 5, -66.6, 4.6, -65.2);
            p.cubicTo(4.2, -63.8, 4, -61.8, 4, -61.8);
            p.cubicTo(4, -61.8, 9, -67.6, 8.4, -65.4);
            p.cubicTo(7.8, -63.2, -0.4, -58, -1.8, -51.8);
            p.lineTo(8.6, -60);
            p.lineTo(12.2, -63);
            p.cubicTo(12.2, -63, 15.8, -60.8, 16, -62.4);
            p.cubicTo(16.2, -64, 20.8, -69.8, 22, -69.6);
            p.cubicTo(23.2, -69.4, 25.2, -72.2, 25, -69.6);
            p.cubicTo(24.8, -67, 32.4, -61.6, 32.4, -61.6);
            p.cubicTo(32.4, -61.6, 35.6, -63.4, 37, -62);
            p.cubicTo(38.4, -60.6, 42.6, -81.8, 42.6, -81.8);
            p.lineTo(67.6, -92.4);
            p.lineTo(111.201, -95.8);
            p.lineTo(94.201, -102.6);
            p.lineTo(33.2, -114);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor = "transparent";
            sfp.strokeColor = "#4c0000";
            sfp.strokeWidth = 2;
            p.moveTo(51.4, 85);
            p.cubicTo(51.4, 85, 36.4, 68.2, 28, 65.6);
            p.cubicTo(28, 65.6, 14.6, 58.8, -10, 66.6);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor = "transparent";
            sfp.strokeColor = "#4c0000";
            sfp.strokeWidth = 2;
            p.moveTo(24.8, 64.2);
            p.cubicTo(24.8, 64.2, -0.4, 56.2, -15.8, 60.4);
            p.cubicTo(-15.8, 60.4, -34.2, 62.4, -42.6, 76.2);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor = "transparent";
            sfp.strokeColor = "#4c0000";
            sfp.strokeWidth = 2;
            p.moveTo(21.2, 63);
            p.cubicTo(21.2, 63, 4.2, 55.8, -10.6, 53.6);
            p.cubicTo(-10.6, 53.6, -27.2, 51, -43.8, 58.2);
            p.cubicTo(-43.8, 58.2, -56, 64.2, -61.4, 74.4);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor = "transparent";
            sfp.strokeColor = "#4c0000";
            sfp.strokeWidth = 2;
            p.moveTo(22.2, 63.4);
            p.cubicTo(22.2, 63.4, 6.8, 52.4, 5.8, 51);
            p.cubicTo(5.8, 51, -1.2, 40, -14.2, 39.6);
            p.cubicTo(-14.2, 39.6, -35.6, 40.4, -52.8, 48.4);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#000000";
            sfp.strokeWidth =  -1;
            p.moveTo(20.895, 54.407);
            p.cubicTo(22.437, 55.87, 49.4, 84.8, 49.4, 84.8);
            p.cubicTo(84.6, 121.401, 56.6, 87.2, 56.6, 87.2);
            p.cubicTo(49, 82.4, 39.8, 63.6, 39.8, 63.6);
            p.cubicTo(38.6, 60.8, 53.8, 70.8, 53.8, 70.8);
            p.cubicTo(57.8, 71.6, 71.4, 90.8, 71.4, 90.8);
            p.cubicTo(64.6, 88.4, 69.4, 95.6, 69.4, 95.6);
            p.cubicTo(72.2, 97.6, 92.601, 113.201, 92.601, 113.201);
            p.cubicTo(96.201, 117.201, 100.201, 118.801, 100.201, 118.801);
            p.cubicTo(114.201, 113.601, 107.801, 126.801, 107.801, 126.801);
            p.cubicTo(110.201, 133.601, 115.801, 122.001, 115.801, 122.001);
            p.cubicTo(127.001, 105.2, 110.601, 107.601, 110.601, 107.601);
            p.cubicTo(80.6, 110.401, 73.8, 94.4, 73.8, 94.4);
            p.cubicTo(71.4, 92, 80.2, 94.4, 80.2, 94.4);
            p.cubicTo(88.601, 96.4, 73, 82, 73, 82);
            p.cubicTo(75.4, 82, 84.6, 88.8, 84.6, 88.8);
            p.cubicTo(95.001, 98, 97.001, 96, 97.001, 96);
            p.cubicTo(115.001, 87.2, 125.401, 94.8, 125.401, 94.8);
            p.cubicTo(127.401, 96.4, 121.801, 103.2, 123.401, 108.401);
            p.cubicTo(125.001, 113.601, 129.801, 126.001, 129.801, 126.001);
            p.cubicTo(127.401, 127.601, 127.801, 138.401, 127.801, 138.401);
            p.cubicTo(144.601, 161.601, 135.001, 159.601, 135.001, 159.601);
            p.cubicTo(119.401, 159.201, 134.201, 166.801, 134.201, 166.801);
            p.cubicTo(137.401, 168.801, 146.201, 176.001, 146.201, 176.001);
            p.cubicTo(143.401, 174.801, 141.801, 180.001, 141.801, 180.001);
            p.cubicTo(146.601, 184.001, 143.801, 188.801, 143.801, 188.801);
            p.cubicTo(137.801, 190.001, 136.601, 194.001, 136.601, 194.001);
            p.cubicTo(143.401, 202.001, 133.401, 202.401, 133.401, 202.401);
            p.cubicTo(137.001, 206.801, 132.201, 218.801, 132.201, 218.801);
            p.cubicTo(127.401, 218.801, 121.001, 224.401, 121.001, 224.401);
            p.cubicTo(123.401, 229.201, 113.001, 234.801, 113.001, 234.801);
            p.cubicTo(104.601, 236.401, 107.401, 243.201, 107.401, 243.201);
            p.cubicTo(99.401, 249.201, 97.001, 265.201, 97.001, 265.201);
            p.cubicTo(96.201, 275.601, 93.801, 278.801, 99.001, 276.801);
            p.cubicTo(104.201, 274.801, 103.401, 262.401, 103.401, 262.401);
            p.cubicTo(98.601, 246.801, 141.401, 230.801, 141.401, 230.801);
            p.cubicTo(145.401, 229.201, 146.201, 224.001, 146.201, 224.001);
            p.cubicTo(148.201, 224.401, 157.001, 232.001, 157.001, 232.001);
            p.cubicTo(164.601, 243.201, 165.001, 234.001, 165.001, 234.001);
            p.cubicTo(166.201, 230.401, 164.601, 224.401, 164.601, 224.401);
            p.cubicTo(170.601, 202.801, 156.601, 196.401, 156.601, 196.401);
            p.cubicTo(146.601, 162.801, 160.601, 171.201, 160.601, 171.201);
            p.cubicTo(163.401, 176.801, 174.201, 182.001, 174.201, 182.001);
            p.lineTo(177.801, 179.601);
            p.cubicTo(176.201, 174.801, 184.601, 168.801, 184.601, 168.801);
            p.cubicTo(187.401, 175.201, 193.401, 167.201, 193.401, 167.201);
            p.cubicTo(197.001, 142.801, 209.401, 157.201, 209.401, 157.201);
            p.cubicTo(213.401, 158.401, 214.601, 151.601, 214.601, 151.601);
            p.cubicTo(218.201, 141.201, 214.601, 127.601, 214.601, 127.601);
            p.cubicTo(218.201, 127.201, 227.801, 133.201, 227.801, 133.201);
            p.cubicTo(230.601, 129.601, 221.401, 112.801, 225.401, 115.201);
            p.cubicTo(229.401, 117.601, 233.801, 119.201, 233.801, 119.201);
            p.cubicTo(234.601, 117.201, 224.601, 104.801, 224.601, 104.801);
            p.cubicTo(220.201, 102, 215.001, 81.6, 215.001, 81.6);
            p.cubicTo(222.201, 85.2, 212.201, 70, 212.201, 70);
            p.cubicTo(212.201, 66.8, 218.201, 55.6, 218.201, 55.6);
            p.cubicTo(217.401, 48.8, 218.201, 49.2, 218.201, 49.2);
            p.cubicTo(221.001, 50.4, 229.001, 52, 222.201, 45.6);
            p.cubicTo(215.401, 39.2, 223.001, 34.4, 223.001, 34.4);
            p.cubicTo(227.401, 31.6, 213.801, 32, 213.801, 32);
            p.cubicTo(208.601, 27.6, 209.001, 23.6, 209.001, 23.6);
            p.cubicTo(217.001, 25.6, 202.601, 11.2, 200.201, 7.6);
            p.cubicTo(197.801, 4, 207.401, -1.2, 207.401, -1.2);
            p.cubicTo(220.601, -4.8, 209.001, -8, 209.001, -8);
            p.cubicTo(189.401, -7.6, 200.201, -18.4, 200.201, -18.4);
            p.cubicTo(206.201, -18, 204.601, -20.4, 204.601, -20.4);
            p.cubicTo(199.401, -21.6, 189.801, -28, 189.801, -28);
            p.cubicTo(185.801, -31.6, 189.401, -30.8, 189.401, -30.8);
            p.cubicTo(206.201, -29.6, 177.401, -40.8, 177.401, -40.8);
            p.cubicTo(185.401, -40.8, 167.401, -51.2, 167.401, -51.2);
            p.cubicTo(165.401, -52.8, 162.201, -60.4, 162.201, -60.4);
            p.cubicTo(156.201, -65.6, 151.401, -72.4, 151.401, -72.4);
            p.cubicTo(151.001, -76.8, 146.201, -81.6, 146.201, -81.6);
            p.cubicTo(134.601, -95.2, 129.001, -94.8, 129.001, -94.8);
            p.cubicTo(114.201, -98.4, 109.001, -97.6, 109.001, -97.6);
            p.lineTo(56.2, -93.2);
            p.cubicTo(29.8, -80.4, 37.6, -59.4, 37.6, -59.4);
            p.cubicTo(44, -51, 53.2, -54.8, 53.2, -54.8);
            p.cubicTo(57.8, -61, 69.4, -58.8, 69.4, -58.8);
            p.cubicTo(89.801, -55.6, 87.201, -59.2, 87.201, -59.2);
            p.cubicTo(84.801, -63.8, 68.6, -70, 68.4, -70.6);
            p.cubicTo(68.2, -71.2, 59.4, -74.6, 59.4, -74.6);
            p.cubicTo(56.4, -75.8, 52, -85, 52, -85);
            p.cubicTo(48.8, -88.4, 64.6, -82.6, 64.6, -82.6);
            p.cubicTo(63.4, -81.6, 70.8, -77.6, 70.8, -77.6);
            p.cubicTo(88.201, -78.6, 98.801, -67.8, 98.801, -67.8);
            p.cubicTo(109.601, -51.2, 109.801, -59.4, 109.801, -59.4);
            p.cubicTo(112.601, -68.8, 100.801, -90, 100.801, -90);
            p.cubicTo(101.201, -92, 109.401, -85.4, 109.401, -85.4);
            p.cubicTo(110.801, -87.4, 111.601, -81.6, 111.601, -81.6);
            p.cubicTo(111.801, -79.2, 115.601, -71.2, 115.601, -71.2);
            p.cubicTo(118.401, -58.2, 122.001, -65.6, 122.001, -65.6);
            p.lineTo(126.601, -56.2);
            p.cubicTo(128.001, -53.6, 122.001, -46, 122.001, -46);
            p.cubicTo(121.801, -43.2, 122.601, -43.4, 117.001, -35.8);
            p.cubicTo(111.401, -28.2, 114.801, -23.8, 114.801, -23.8);
            p.cubicTo(113.401, -17.2, 122.201, -17.6, 122.201, -17.6);
            p.cubicTo(124.801, -15.4, 128.201, -15.4, 128.201, -15.4);
            p.cubicTo(130.001, -13.4, 132.401, -14, 132.401, -14);
            p.cubicTo(134.001, -17.8, 140.201, -15.8, 140.201, -15.8);
            p.cubicTo(141.601, -18.2, 149.801, -18.6, 149.801, -18.6);
            p.cubicTo(150.801, -21.2, 151.201, -22.8, 154.601, -23.4);
            p.cubicTo(158.001, -24, 133.401, -67, 133.401, -67);
            p.cubicTo(139.801, -67.8, 131.601, -80.2, 131.601, -80.2);
            p.cubicTo(129.401, -86.8, 140.801, -72.2, 143.001, -70.8);
            p.cubicTo(145.201, -69.4, 146.201, -67.2, 144.601, -67.4);
            p.cubicTo(143.001, -67.6, 141.201, -65.4, 142.601, -65.2);
            p.cubicTo(144.001, -65, 157.001, -50, 160.401, -39.8);
            p.cubicTo(163.801, -29.6, 169.801, -25.6, 176.001, -19.6);
            p.cubicTo(182.201, -13.6, 181.401, 10.6, 181.401, 10.6);
            p.cubicTo(181.001, 19.4, 187.001, 30, 187.001, 30);
            p.cubicTo(189.001, 33.8, 184.801, 52, 184.801, 52);
            p.cubicTo(182.801, 54.2, 184.201, 55, 184.201, 55);
            p.cubicTo(185.201, 56.2, 192.001, 69.4, 192.001, 69.4);
            p.cubicTo(190.201, 69.2, 193.801, 72.8, 193.801, 72.8);
            p.cubicTo(199.001, 78.8, 192.601, 75.8, 192.601, 75.8);
            p.cubicTo(186.601, 74.2, 193.601, 84, 193.601, 84);
            p.cubicTo(194.801, 85.8, 185.801, 81.2, 185.801, 81.2);
            p.cubicTo(176.601, 80.6, 188.201, 87.8, 188.201, 87.8);
            p.cubicTo(196.801, 95, 185.401, 90.6, 185.401, 90.6);
            p.cubicTo(180.801, 88.8, 184.001, 95.6, 184.001, 95.6);
            p.cubicTo(187.201, 97.2, 204.401, 104.2, 204.401, 104.2);
            p.cubicTo(204.801, 108.001, 201.801, 113.001, 201.801, 113.001);
            p.cubicTo(202.201, 117.001, 200.001, 120.401, 200.001, 120.401);
            p.cubicTo(198.801, 128.601, 198.201, 129.401, 198.201, 129.401);
            p.cubicTo(194.001, 129.601, 186.601, 143.401, 186.601, 143.401);
            p.cubicTo(184.801, 146.001, 174.601, 158.001, 174.601, 158.001);
            p.cubicTo(172.601, 165.001, 154.601, 157.801, 154.601, 157.801);
            p.cubicTo(148.001, 161.201, 150.001, 157.801, 150.001, 157.801);
            p.cubicTo(149.601, 155.601, 154.401, 149.601, 154.401, 149.601);
            p.cubicTo(161.401, 147.001, 158.801, 136.201, 158.801, 136.201);
            p.cubicTo(162.801, 134.801, 151.601, 132.001, 151.801, 130.801);
            p.cubicTo(152.001, 129.601, 157.801, 128.201, 157.801, 128.201);
            p.cubicTo(165.801, 126.201, 161.401, 123.801, 161.401, 123.801);
            p.cubicTo(160.801, 119.801, 163.801, 114.201, 163.801, 114.201);
            p.cubicTo(175.401, 113.401, 163.801, 97.2, 163.801, 97.2);
            p.cubicTo(153.001, 89.6, 152.001, 83.8, 152.001, 83.8);
            p.cubicTo(164.601, 75.6, 156.401, 63.2, 156.601, 59.6);
            p.cubicTo(156.801, 56, 158.001, 34.4, 158.001, 34.4);
            p.cubicTo(156.001, 28.2, 153.001, 14.6, 153.001, 14.6);
            p.cubicTo(155.201, 9.4, 162.601, -3.2, 162.601, -3.2);
            p.cubicTo(165.401, -7.4, 174.201, -12.2, 172.001, -15.2);
            p.cubicTo(169.801, -18.2, 162.001, -16.4, 162.001, -16.4);
            p.cubicTo(154.201, -17.8, 154.801, -12.6, 154.801, -12.6);
            p.cubicTo(153.201, -11.6, 152.401, -6.6, 152.401, -6.6);
            p.cubicTo(151.68, 1.333, 142.801, 7.6, 142.801, 7.6);
            p.cubicTo(131.601, 13.8, 140.801, 17.8, 140.801, 17.8);
            p.cubicTo(146.801, 24.4, 137.001, 24.6, 137.001, 24.6);
            p.cubicTo(126.001, 22.8, 134.201, 33, 134.201, 33);
            p.cubicTo(145.001, 45.8, 142.001, 48.6, 142.001, 48.6);
            p.cubicTo(131.801, 49.6, 144.401, 58.8, 144.401, 58.8);
            p.cubicTo(144.401, 58.8, 143.601, 56.8, 143.801, 58.6);
            p.cubicTo(144.001, 60.4, 147.001, 64.6, 147.801, 66.6);
            p.cubicTo(148.601, 68.6, 144.601, 68.8, 144.601, 68.8);
            p.cubicTo(145.201, 78.4, 129.801, 74.2, 129.801, 74.2);
            p.cubicTo(129.801, 74.2, 129.801, 74.2, 128.201, 74.4);
            p.cubicTo(126.601, 74.6, 115.401, 73.8, 109.601, 71.6);
            p.cubicTo(103.801, 69.4, 97.001, 69.4, 97.001, 69.4);
            p.cubicTo(97.001, 69.4, 93.001, 71.2, 85.4, 71);
            p.cubicTo(77.8, 70.8, 69.8, 73.6, 69.8, 73.6);
            p.cubicTo(65.4, 73.2, 74, 68.8, 74.2, 69);
            p.cubicTo(74.4, 69.2, 80, 63.6, 72, 64.2);
            p.cubicTo(50.203, 65.835, 39.4, 55.6, 39.4, 55.6);
            p.cubicTo(37.4, 54.2, 34.8, 51.4, 34.8, 51.4);
            p.cubicTo(24.8, 49.4, 36.2, 63.8, 36.2, 63.8);
            p.cubicTo(37.4, 65.2, 36, 66.2, 36, 66.2);
            p.cubicTo(35.2, 64.6, 27.4, 59.2, 27.4, 59.2);
            p.cubicTo(24.589, 58.227, 23.226, 56.893, 20.895, 54.407);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#4c0000";
            sfp.strokeWidth =  -1;
            p.moveTo(-3, 42.8);
            p.cubicTo(-3, 42.8, 8.6, 48.4, 11.2, 51.2);
            p.cubicTo(13.8, 54, 27.8, 65.4, 27.8, 65.4);
            p.cubicTo(27.8, 65.4, 22.4, 63.4, 19.8, 61.6);
            p.cubicTo(17.2, 59.8, 6.4, 51.6, 6.4, 51.6);
            p.cubicTo(6.4, 51.6, 2.6, 45.6, -3, 42.8);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#99cc32";
            sfp.strokeWidth =  -1;
            p.moveTo(-61.009, 11.603);
            p.cubicTo(-60.672, 11.455, -61.196, 8.743, -61.4, 8.2);
            p.cubicTo(-62.422, 5.474, -71.4, 4, -71.4, 4);
            p.cubicTo(-71.627, 5.365, -71.682, 6.961, -71.576, 8.599);
            p.cubicTo(-71.576, 8.599, -66.708, 14.118, -61.009, 11.603);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#659900";
            sfp.strokeWidth =  -1;
            p.moveTo(-61.009, 11.403);
            p.cubicTo(-61.458, 11.561, -61.024, 8.669, -61.2, 8.2);
            p.cubicTo(-62.222, 5.474, -71.4, 3.9, -71.4, 3.9);
            p.cubicTo(-71.627, 5.265, -71.682, 6.861, -71.576, 8.499);
            p.cubicTo(-71.576, 8.499, -67.308, 13.618, -61.009, 11.403);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#000000";
            sfp.strokeWidth =  -1;
            p.moveTo(-65.4, 11.546);
            p.cubicTo(-66.025, 11.546, -66.531, 10.406, -66.531, 9);
            p.cubicTo(-66.531, 7.595, -66.025, 6.455, -65.4, 6.455);
            p.cubicTo(-64.775, 6.455, -64.268, 7.595, -64.268, 9);
            p.cubicTo(-64.268, 10.406, -64.775, 11.546, -65.4, 11.546);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#000000";
            sfp.strokeWidth =  -1;
            p.moveTo(-65.4, 9);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#000000";
            sfp.strokeWidth =  -1;
            p.moveTo(-111, 109.601);
            p.cubicTo(-111, 109.601, -116.6, 119.601, -91.8, 113.601);
            p.cubicTo(-91.8, 113.601, -77.8, 112.401, -75.4, 110.001);
            p.cubicTo(-74.2, 110.801, -65.834, 113.734, -63, 114.401);
            p.cubicTo(-56.2, 116.001, -47.8, 106, -47.8, 106);
            p.cubicTo(-47.8, 106, -43.2, 95.5, -40.4, 95.5);
            p.cubicTo(-37.6, 95.5, -40.8, 97.1, -40.8, 97.1);
            p.cubicTo(-40.8, 97.1, -47.4, 107.201, -47, 108.801);
            p.cubicTo(-47, 108.801, -52.2, 128.801, -68.2, 129.601);
            p.cubicTo(-68.2, 129.601, -84.35, 130.551, -83, 136.401);
            p.cubicTo(-83, 136.401, -74.2, 134.001, -71.8, 136.401);
            p.cubicTo(-71.8, 136.401, -61, 136.001, -69, 142.401);
            p.lineTo(-75.8, 154.001);
            p.cubicTo(-75.8, 154.001, -75.66, 157.919, -85.8, 154.401);
            p.cubicTo(-95.6, 151.001, -105.9, 138.101, -105.9, 138.101);
            p.cubicTo(-105.9, 138.101, -121.85, 123.551, -111, 109.601);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#e59999";
            sfp.strokeWidth =  -1;
            p.moveTo(-112.2, 113.601);
            p.cubicTo(-112.2, 113.601, -114.2, 123.201, -77.4, 112.801);
            p.cubicTo(-77.4, 112.801, -73, 112.801, -70.6, 113.601);
            p.cubicTo(-68.2, 114.401, -56.2, 117.201, -54.2, 116.001);
            p.cubicTo(-54.2, 116.001, -61.4, 129.601, -73, 128.001);
            p.cubicTo(-73, 128.001, -86.2, 129.601, -85.8, 134.401);
            p.cubicTo(-85.8, 134.401, -81.8, 141.601, -77, 144.001);
            p.cubicTo(-77, 144.001, -74.2, 146.401, -74.6, 149.601);
            p.cubicTo(-75, 152.801, -77.8, 154.401, -79.8, 155.201);
            p.cubicTo(-81.8, 156.001, -85, 152.801, -86.6, 152.801);
            p.cubicTo(-88.2, 152.801, -96.6, 146.401, -101, 141.601);
            p.cubicTo(-105.4, 136.801, -113.8, 124.801, -113.4, 122.001);
            p.cubicTo(-113, 119.201, -112.2, 113.601, -112.2, 113.601);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#b26565";
            sfp.strokeWidth =  -1;
            p.moveTo(-109, 131.051);
            p.cubicTo(-106.4, 135.001, -103.2, 139.201, -101, 141.601);
            p.cubicTo(-96.6, 146.401, -88.2, 152.801, -86.6, 152.801);
            p.cubicTo(-85, 152.801, -81.8, 156.001, -79.8, 155.201);
            p.cubicTo(-77.8, 154.401, -75, 152.801, -74.6, 149.601);
            p.cubicTo(-74.2, 146.401, -77, 144.001, -77, 144.001);
            p.cubicTo(-80.066, 142.468, -82.806, 138.976, -84.385, 136.653);
            p.cubicTo(-84.385, 136.653, -84.2, 139.201, -89.4, 138.401);
            p.cubicTo(-94.6, 137.601, -99.8, 134.801, -101.4, 131.601);
            p.cubicTo(-103, 128.401, -105.4, 126.001, -103.8, 129.601);
            p.cubicTo(-102.2, 133.201, -99.8, 136.801, -98.2, 137.201);
            p.cubicTo(-96.6, 137.601, -97, 138.801, -99.4, 138.401);
            p.cubicTo(-101.8, 138.001, -104.6, 137.601, -109, 132.401);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#992600";
            sfp.strokeWidth =  -1;
            p.moveTo(-111.6, 110.001);
            p.cubicTo(-111.6, 110.001, -109.8, 96.4, -108.6, 92.4);
            p.cubicTo(-108.6, 92.4, -109.4, 85.6, -107, 81.4);
            p.cubicTo(-104.6, 77.2, -102.6, 71, -99.6, 65.6);
            p.cubicTo(-96.6, 60.2, -96.4, 56.2, -92.4, 54.6);
            p.cubicTo(-88.4, 53, -82.4, 44.4, -79.6, 43.4);
            p.cubicTo(-76.8, 42.4, -77, 43.2, -77, 43.2);
            p.cubicTo(-77, 43.2, -70.2, 28.4, -56.6, 32.4);
            p.cubicTo(-56.6, 32.4, -72.8, 29.6, -57, 20.2);
            p.cubicTo(-57, 20.2, -61.8, 21.3, -58.5, 14.3);
            p.cubicTo(-56.299, 9.632, -56.8, 16.4, -67.8, 28.2);
            p.cubicTo(-67.8, 28.2, -72.8, 36.8, -78, 39.8);
            p.cubicTo(-83.2, 42.8, -95.2, 49.8, -96.4, 53.6);
            p.cubicTo(-97.6, 57.4, -100.8, 63.2, -102.8, 64.8);
            p.cubicTo(-104.8, 66.4, -107.6, 70.6, -108, 74);
            p.cubicTo(-108, 74, -109.2, 78, -110.6, 79.2);
            p.cubicTo(-112, 80.4, -112.2, 83.6, -112.2, 85.6);
            p.cubicTo(-112.2, 87.6, -114.2, 90.4, -114, 92.8);
            p.cubicTo(-114, 92.8, -113.2, 111.801, -113.6, 113.801);
            p.lineTo(-111.6, 110.001);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#ffffff";
            sfp.strokeWidth =  -1;
            p.moveTo(-120.2, 114.601);
            p.cubicTo(-120.2, 114.601, -122.2, 113.201, -126.6, 119.201);
            p.cubicTo(-126.6, 119.201, -119.3, 152.201, -119.3, 153.601);
            p.cubicTo(-119.3, 153.601, -118.2, 151.501, -119.5, 144.301);
            p.cubicTo(-120.8, 137.101, -121.7, 124.401, -121.7, 124.401);
            p.lineTo(-120.2, 114.601);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#992600";
            sfp.strokeWidth =  -1;
            p.moveTo(-98.6, 54);
            p.cubicTo(-98.6, 54, -116.2, 57.2, -115.8, 86.4);
            p.lineTo(-116.6, 111.201);
            p.cubicTo(-116.6, 111.201, -117.8, 85.6, -119, 84);
            p.cubicTo(-120.2, 82.4, -116.2, 71.2, -119.4, 77.2);
            p.cubicTo(-119.4, 77.2, -133.4, 91.2, -125.4, 112.401);
            p.cubicTo(-125.4, 112.401, -123.9, 115.701, -126.9, 111.101);
            p.cubicTo(-126.9, 111.101, -131.5, 98.5, -130.4, 92.1);
            p.cubicTo(-130.4, 92.1, -130.2, 89.9, -128.3, 87.1);
            p.cubicTo(-128.3, 87.1, -119.7, 75.4, -117, 73.1);
            p.cubicTo(-117, 73.1, -115.2, 58.7, -99.8, 53.5);
            p.cubicTo(-99.8, 53.5, -94.1, 51.2, -98.6, 54);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#000000";
            sfp.strokeWidth =  -1;
            p.moveTo(40.8, -12.2);
            p.cubicTo(41.46, -12.554, 41.451, -13.524, 42.031, -13.697);
            p.cubicTo(43.18, -14.041, 43.344, -15.108, 43.862, -15.892);
            p.cubicTo(44.735, -17.211, 44.928, -18.744, 45.51, -20.235);
            p.cubicTo(45.782, -20.935, 45.809, -21.89, 45.496, -22.55);
            p.cubicTo(44.322, -25.031, 43.62, -27.48, 42.178, -29.906);
            p.cubicTo(41.91, -30.356, 41.648, -31.15, 41.447, -31.748);
            p.cubicTo(40.984, -33.132, 39.727, -34.123, 38.867, -35.443);
            p.cubicTo(38.579, -35.884, 39.104, -36.809, 38.388, -36.893);
            p.cubicTo(37.491, -36.998, 36.042, -37.578, 35.809, -36.552);
            p.cubicTo(35.221, -33.965, 36.232, -31.442, 37.2, -29);
            p.cubicTo(36.418, -28.308, 36.752, -27.387, 36.904, -26.62);
            p.cubicTo(37.614, -23.014, 36.416, -19.662, 35.655, -16.188);
            p.cubicTo(35.632, -16.084, 35.974, -15.886, 35.946, -15.824);
            p.cubicTo(34.724, -13.138, 33.272, -10.693, 31.453, -8.312);
            p.cubicTo(30.695, -7.32, 29.823, -6.404, 29.326, -5.341);
            p.cubicTo(28.958, -4.554, 28.55, -3.588, 28.8, -2.6);
            p.cubicTo(25.365, 0.18, 23.115, 4.025, 20.504, 7.871);
            p.cubicTo(20.042, 8.551, 20.333, 9.76, 20.884, 10.029);
            p.cubicTo(21.697, 10.427, 22.653, 9.403, 23.123, 8.557);
            p.cubicTo(23.512, 7.859, 23.865, 7.209, 24.356, 6.566);
            p.cubicTo(24.489, 6.391, 24.31, 5.972, 24.445, 5.851);
            p.cubicTo(27.078, 3.504, 28.747, 0.568, 31.2, -1.8);
            p.cubicTo(33.15, -2.129, 34.687, -3.127, 36.435, -4.14);
            p.cubicTo(36.743, -4.319, 37.267, -4.07, 37.557, -4.265);
            p.cubicTo(39.31, -5.442, 39.308, -7.478, 39.414, -9.388);
            p.cubicTo(39.464, -10.272, 39.66, -11.589, 40.8, -12.2);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#000000";
            sfp.strokeWidth =  -1;
            p.moveTo(31.959, -16.666);
            p.cubicTo(32.083, -16.743, 31.928, -17.166, 32.037, -17.382);
            p.cubicTo(32.199, -17.706, 32.602, -17.894, 32.764, -18.218);
            p.cubicTo(32.873, -18.434, 32.71, -18.814, 32.846, -18.956);
            p.cubicTo(35.179, -21.403, 35.436, -24.427, 34.4, -27.4);
            p.cubicTo(35.424, -28.02, 35.485, -29.282, 35.06, -30.129);
            p.cubicTo(34.207, -31.829, 34.014, -33.755, 33.039, -35.298);
            p.cubicTo(32.237, -36.567, 30.659, -37.811, 29.288, -36.508);
            p.cubicTo(28.867, -36.108, 28.546, -35.321, 28.824, -34.609);
            p.cubicTo(28.888, -34.446, 29.173, -34.3, 29.146, -34.218);
            p.cubicTo(29.039, -33.894, 28.493, -33.67, 28.487, -33.398);
            p.cubicTo(28.457, -31.902, 27.503, -30.391, 28.133, -29.062);
            p.cubicTo(28.905, -27.433, 29.724, -25.576, 30.4, -23.8);
            p.cubicTo(29.166, -21.684, 30.199, -19.235, 28.446, -17.358);
            p.cubicTo(28.31, -17.212, 28.319, -16.826, 28.441, -16.624);
            p.cubicTo(28.733, -16.138, 29.139, -15.732, 29.625, -15.44);
            p.cubicTo(29.827, -15.319, 30.175, -15.317, 30.375, -15.441);
            p.cubicTo(30.953, -15.803, 31.351, -16.29, 31.959, -16.666);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#000000";
            sfp.strokeWidth =  -1;
            p.moveTo(94.771, -26.977);
            p.cubicTo(96.16, -25.185, 96.45, -22.39, 94.401, -21);
            p.cubicTo(94.951, -17.691, 98.302, -19.67, 100.401, -20.2);
            p.cubicTo(100.292, -20.588, 100.519, -20.932, 100.802, -20.937);
            p.cubicTo(101.859, -20.952, 102.539, -21.984, 103.601, -21.8);
            p.cubicTo(104.035, -23.357, 105.673, -24.059, 106.317, -25.439);
            p.cubicTo(108.043, -29.134, 107.452, -33.407, 104.868, -36.653);
            p.cubicTo(104.666, -36.907, 104.883, -37.424, 104.759, -37.786);
            p.cubicTo(104.003, -39.997, 101.935, -40.312, 100.001, -41);
            p.cubicTo(98.824, -44.875, 98.163, -48.906, 96.401, -52.6);
            p.cubicTo(94.787, -52.85, 94.089, -54.589, 92.752, -55.309);
            p.cubicTo(91.419, -56.028, 90.851, -54.449, 90.892, -53.403);
            p.cubicTo(90.899, -53.198, 91.351, -52.974, 91.181, -52.609);
            p.cubicTo(91.105, -52.445, 90.845, -52.334, 90.845, -52.2);
            p.cubicTo(90.846, -52.065, 91.067, -51.934, 91.201, -51.8);
            p.cubicTo(90.283, -50.98, 88.86, -50.503, 88.565, -49.358);
            p.cubicTo(87.611, -45.648, 90.184, -42.523, 91.852, -39.322);
            p.cubicTo(92.443, -38.187, 91.707, -36.916, 90.947, -35.708);
            p.cubicTo(90.509, -35.013, 90.617, -33.886, 90.893, -33.03);
            p.cubicTo(91.645, -30.699, 93.236, -28.96, 94.771, -26.977);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#000000";
            sfp.strokeWidth =  -1;
            p.moveTo(57.611, -8.591);
            p.cubicTo(56.124, -6.74, 52.712, -4.171, 55.629, -2.243);
            p.cubicTo(55.823, -2.114, 56.193, -2.11, 56.366, -2.244);
            p.cubicTo(58.387, -3.809, 60.39, -4.712, 62.826, -5.294);
            p.cubicTo(62.95, -5.323, 63.224, -4.856, 63.593, -5.017);
            p.cubicTo(65.206, -5.72, 67.216, -5.662, 68.4, -7);
            p.cubicTo(72.167, -6.776, 75.732, -7.892, 79.123, -9.2);
            p.cubicTo(80.284, -9.648, 81.554, -10.207, 82.755, -10.709);
            p.cubicTo(84.131, -11.285, 85.335, -12.213, 86.447, -13.354);
            p.cubicTo(86.58, -13.49, 86.934, -13.4, 87.201, -13.4);
            p.cubicTo(87.161, -14.263, 88.123, -14.39, 88.37, -15.012);
            p.cubicTo(88.462, -15.244, 88.312, -15.64, 88.445, -15.742);
            p.cubicTo(90.583, -17.372, 91.503, -19.39, 90.334, -21.767);
            p.cubicTo(90.049, -22.345, 89.8, -22.963, 89.234, -23.439);
            p.cubicTo(88.149, -24.35, 87.047, -23.496, 86, -23.8);
            p.cubicTo(85.841, -23.172, 85.112, -23.344, 84.726, -23.146);
            p.cubicTo(83.867, -22.707, 82.534, -23.292, 81.675, -22.854);
            p.cubicTo(80.313, -22.159, 79.072, -21.99, 77.65, -21.613);
            p.cubicTo(77.338, -21.531, 76.56, -21.627, 76.4, -21);
            p.cubicTo(76.266, -21.134, 76.118, -21.368, 76.012, -21.346);
            p.cubicTo(74.104, -20.95, 72.844, -20.736, 71.543, -19.044);
            p.cubicTo(71.44, -18.911, 70.998, -19.09, 70.839, -18.955);
            p.cubicTo(69.882, -18.147, 69.477, -16.913, 68.376, -16.241);
            p.cubicTo(68.175, -16.118, 67.823, -16.286, 67.629, -16.157);
            p.cubicTo(66.983, -15.726, 66.616, -15.085, 65.974, -14.638);
            p.cubicTo(65.645, -14.409, 65.245, -14.734, 65.277, -14.99);
            p.cubicTo(65.522, -16.937, 66.175, -18.724, 65.6, -20.6);
            p.cubicTo(67.677, -23.12, 70.194, -25.069, 72, -27.8);
            p.cubicTo(72.015, -29.966, 72.707, -32.112, 72.594, -34.189);
            p.cubicTo(72.584, -34.382, 72.296, -35.115, 72.17, -35.462);
            p.cubicTo(71.858, -36.316, 72.764, -37.382, 71.92, -38.106);
            p.cubicTo(70.516, -39.309, 69.224, -38.433, 68.4, -37);
            p.cubicTo(66.562, -36.61, 64.496, -35.917, 62.918, -37.151);
            p.cubicTo(61.911, -37.938, 61.333, -38.844, 60.534, -39.9);
            p.cubicTo(59.549, -41.202, 59.884, -42.638, 59.954, -44.202);
            p.cubicTo(59.96, -44.33, 59.645, -44.466, 59.645, -44.6);
            p.cubicTo(59.646, -44.735, 59.866, -44.866, 60, -45);
            p.cubicTo(59.294, -45.626, 59.019, -46.684, 58, -47);
            p.cubicTo(58.305, -48.092, 57.629, -48.976, 56.758, -49.278);
            p.cubicTo(54.763, -49.969, 53.086, -48.057, 51.194, -47.984);
            p.cubicTo(50.68, -47.965, 50.213, -49.003, 49.564, -49.328);
            p.cubicTo(49.132, -49.544, 48.428, -49.577, 48.066, -49.311);
            p.cubicTo(47.378, -48.807, 46.789, -48.693, 46.031, -48.488);
            p.cubicTo(44.414, -48.052, 43.136, -46.958, 41.656, -46.103);
            p.cubicTo(40.171, -45.246, 39.216, -43.809, 38.136, -42.489);
            p.cubicTo(37.195, -41.337, 37.059, -38.923, 38.479, -38.423);
            p.cubicTo(40.322, -37.773, 41.626, -40.476, 43.592, -40.15);
            p.cubicTo(43.904, -40.099, 44.11, -39.788, 44, -39.4);
            p.cubicTo(44.389, -39.291, 44.607, -39.52, 44.8, -39.8);
            p.cubicTo(45.658, -38.781, 46.822, -38.444, 47.76, -37.571);
            p.cubicTo(48.73, -36.667, 50.476, -37.085, 51.491, -36.088);
            p.cubicTo(53.02, -34.586, 52.461, -31.905, 54.4, -30.6);
            p.cubicTo(53.814, -29.287, 53.207, -28.01, 52.872, -26.583);
            p.cubicTo(52.59, -25.377, 53.584, -24.18, 54.795, -24.271);
            p.cubicTo(56.053, -24.365, 56.315, -25.124, 56.8, -26.2);
            p.cubicTo(57.067, -25.933, 57.536, -25.636, 57.495, -25.42);
            p.cubicTo(57.038, -23.033, 56.011, -21.04, 55.553, -18.609);
            p.cubicTo(55.494, -18.292, 55.189, -18.09, 54.8, -18.2);
            p.cubicTo(54.332, -14.051, 50.28, -11.657, 47.735, -8.492);
            p.cubicTo(47.332, -7.99, 47.328, -6.741, 47.737, -6.338);
            p.cubicTo(49.14, -4.951, 51.1, -6.497, 52.8, -7);
            p.cubicTo(53.013, -8.206, 53.872, -9.148, 55.204, -9.092);
            p.cubicTo(55.46, -9.082, 55.695, -9.624, 56.019, -9.754);
            p.cubicTo(56.367, -9.892, 56.869, -9.668, 57.155, -9.866);
            p.cubicTo(58.884, -11.061, 60.292, -12.167, 62.03, -13.356);
            p.cubicTo(62.222, -13.487, 62.566, -13.328, 62.782, -13.436);
            p.cubicTo(63.107, -13.598, 63.294, -13.985, 63.617, -14.17);
            p.cubicTo(63.965, -14.37, 64.207, -14.08, 64.4, -13.8);
            p.cubicTo(63.754, -13.451, 63.75, -12.494, 63.168, -12.292);
            p.cubicTo(62.393, -12.024, 61.832, -11.511, 61.158, -11.064);
            p.cubicTo(60.866, -10.871, 60.207, -11.119, 60.103, -10.94);
            p.cubicTo(59.505, -9.912, 58.321, -9.474, 57.611, -8.591);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#000000";
            sfp.strokeWidth =  -1;
            p.moveTo(2.2, -58);
            p.cubicTo(2.2, -58, -7.038, -60.872, -18.2, -35.2);
            p.cubicTo(-18.2, -35.2, -20.6, -30, -23, -28);
            p.cubicTo(-25.4, -26, -36.6, -22.4, -38.6, -18.4);
            p.lineTo(-49, -2.4);
            p.cubicTo(-49, -2.4, -34.2, -18.4, -31, -20.8);
            p.cubicTo(-31, -20.8, -23, -29.2, -26.2, -22.4);
            p.cubicTo(-26.2, -22.4, -40.2, -11.6, -39, -2.4);
            p.cubicTo(-39, -2.4, -44.6, 12, -45.4, 14);
            p.cubicTo(-45.4, 14, -29.4, -18, -27, -19.2);
            p.cubicTo(-24.6, -20.4, -23.4, -20.4, -24.6, -16.8);
            p.cubicTo(-25.8, -13.2, -26.2, 3.2, -29, 5.2);
            p.cubicTo(-29, 5.2, -21, -15.2, -21.8, -18.4);
            p.cubicTo(-21.8, -18.4, -18.6, -22, -16.2, -16.8);
            p.lineTo(-17.4, -0.8);
            p.lineTo(-13, 11.2);
            p.cubicTo(-13, 11.2, -15.4, 0, -13.8, -15.6);
            p.cubicTo(-13.8, -15.6, -15.8, -26, -11.8, -20.4);
            p.cubicTo(-7.8, -14.8, 1.8, -8.8, 1.8, -4);
            p.cubicTo(1.8, -4, -3.4, -21.6, -12.6, -26.4);
            p.lineTo(-16.6, -20.4);
            p.lineTo(-17.8, -22.4);
            p.cubicTo(-17.8, -22.4, -21.4, -23.2, -17, -30);
            p.cubicTo(-12.6, -36.8, -13, -37.6, -13, -37.6);
            p.cubicTo(-13, -37.6, -6.6, -30.4, -5, -30.4);
            p.cubicTo(-5, -30.4, 8.2, -38, 9.4, -13.6);
            p.cubicTo(9.4, -13.6, 16.2, -28, 7, -34.8);
            p.cubicTo(7, -34.8, -7.8, -36.8, -6.6, -42);
            p.lineTo(0.6, -54.4);
            p.cubicTo(4.2, -59.6, 2.6, -56.8, 2.6, -56.8);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#000000";
            sfp.strokeWidth =  -1;
            p.moveTo(-17.8, -41.6);
            p.cubicTo(-17.8, -41.6, -30.6, -41.6, -33.8, -36.4);
            p.lineTo(-41, -26.8);
            p.cubicTo(-41, -26.8, -23.8, -36.8, -19.8, -38);
            p.cubicTo(-15.8, -39.2, -17.8, -41.6, -17.8, -41.6);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#000000";
            sfp.strokeWidth =  -1;
            p.moveTo(-57.8, -35.2);
            p.cubicTo(-57.8, -35.2, -59.8, -34, -60.2, -31.2);
            p.cubicTo(-60.6, -28.4, -63, -28, -62.2, -25.2);
            p.cubicTo(-61.4, -22.4, -59.4, -20, -59.4, -24);
            p.cubicTo(-59.4, -28, -57.8, -30, -57, -31.2);
            p.cubicTo(-56.2, -32.4, -54.6, -36.8, -57.8, -35.2);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#000000";
            sfp.strokeWidth =  -1;
            p.moveTo(-66.6, 26);
            p.cubicTo(-66.6, 26, -75, 22, -78.2, 18.4);
            p.cubicTo(-81.4, 14.8, -80.948, 19.966, -85.8, 19.6);
            p.cubicTo(-91.647, 19.159, -90.6, 3.2, -90.6, 3.2);
            p.lineTo(-94.6, 10.8);
            p.cubicTo(-94.6, 10.8, -95.8, 25.2, -87.8, 22.8);
            p.cubicTo(-83.893, 21.628, -82.6, 23.2, -84.2, 24);
            p.cubicTo(-85.8, 24.8, -78.6, 25.2, -81.4, 26.8);
            p.cubicTo(-84.2, 28.4, -69.8, 23.2, -72.2, 33.6);
            p.lineTo(-66.6, 26);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#000000";
            sfp.strokeWidth =  -1;
            p.moveTo(-79.2, 40.4);
            p.cubicTo(-79.2, 40.4, -94.6, 44.8, -98.2, 35.2);
            p.cubicTo(-98.2, 35.2, -103, 37.6, -100.8, 40.6);
            p.cubicTo(-98.6, 43.6, -97.4, 44, -97.4, 44);
            p.cubicTo(-97.4, 44, -92, 45.2, -92.6, 46);
            p.cubicTo(-93.2, 46.8, -95.6, 50.2, -95.6, 50.2);
            p.cubicTo(-95.6, 50.2, -85.4, 44.2, -79.2, 40.4);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#ffffff";
            sfp.strokeWidth =  -1;
            p.moveTo(149.201, 118.601);
            p.cubicTo(148.774, 120.735, 147.103, 121.536, 145.201, 122.201);
            p.cubicTo(143.284, 121.243, 140.686, 118.137, 138.801, 120.201);
            p.cubicTo(138.327, 119.721, 137.548, 119.661, 137.204, 118.999);
            p.cubicTo(136.739, 118.101, 137.011, 117.055, 136.669, 116.257);
            p.cubicTo(136.124, 114.985, 135.415, 113.619, 135.601, 112.201);
            p.cubicTo(137.407, 111.489, 138.002, 109.583, 137.528, 107.82);
            p.cubicTo(137.459, 107.563, 137.03, 107.366, 137.23, 107.017);
            p.cubicTo(137.416, 106.694, 137.734, 106.467, 138.001, 106.2);
            p.cubicTo(137.866, 106.335, 137.721, 106.568, 137.61, 106.548);
            p.cubicTo(137, 106.442, 137.124, 105.805, 137.254, 105.418);
            p.cubicTo(137.839, 103.672, 139.853, 103.408, 141.201, 104.6);
            p.cubicTo(141.457, 104.035, 141.966, 104.229, 142.401, 104.2);
            p.cubicTo(142.351, 103.621, 142.759, 103.094, 142.957, 102.674);
            p.cubicTo(143.475, 101.576, 145.104, 102.682, 145.901, 102.07);
            p.cubicTo(146.977, 101.245, 148.04, 100.546, 149.118, 101.149);
            p.cubicTo(150.927, 102.162, 152.636, 103.374, 153.835, 105.115);
            p.cubicTo(154.41, 105.949, 154.65, 107.23, 154.592, 108.188);
            p.cubicTo(154.554, 108.835, 153.173, 108.483, 152.83, 109.412);
            p.cubicTo(152.185, 111.16, 154.016, 111.679, 154.772, 113.017);
            p.cubicTo(154.97, 113.366, 154.706, 113.67, 154.391, 113.768);
            p.cubicTo(153.98, 113.896, 153.196, 113.707, 153.334, 114.16);
            p.cubicTo(154.306, 117.353, 151.55, 118.031, 149.201, 118.601);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#ffffff";
            sfp.strokeWidth =  -1;
            p.moveTo(139.6, 138.201);
            p.cubicTo(139.593, 136.463, 137.992, 134.707, 139.201, 133.001);
            p.cubicTo(139.336, 133.135, 139.467, 133.356, 139.601, 133.356);
            p.cubicTo(139.736, 133.356, 139.867, 133.135, 140.001, 133.001);
            p.cubicTo(141.496, 135.217, 145.148, 136.145, 145.006, 138.991);
            p.cubicTo(144.984, 139.438, 143.897, 140.356, 144.801, 141.001);
            p.cubicTo(142.988, 142.349, 142.933, 144.719, 142.001, 146.601);
            p.cubicTo(140.763, 146.315, 139.551, 145.952, 138.401, 145.401);
            p.cubicTo(138.753, 143.915, 138.636, 142.231, 139.456, 140.911);
            p.cubicTo(139.89, 140.213, 139.603, 139.134, 139.6, 138.201);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#cccccc";
            sfp.strokeWidth =  -1;
            p.moveTo(-26.6, 129.201);
            p.cubicTo(-26.6, 129.201, -43.458, 139.337, -29.4, 124.001);
            p.cubicTo(-20.6, 114.401, -10.6, 108.801, -10.6, 108.801);
            p.cubicTo(-10.6, 108.801, -0.2, 104.4, 3.4, 103.2);
            p.cubicTo(7, 102, 22.2, 96.8, 25.4, 96.4);
            p.cubicTo(28.6, 96, 38.2, 92, 45, 96);
            p.cubicTo(51.8, 100, 59.8, 104.4, 59.8, 104.4);
            p.cubicTo(59.8, 104.4, 43.4, 96, 39.8, 98.4);
            p.cubicTo(36.2, 100.8, 29, 100.4, 23, 103.6);
            p.cubicTo(23, 103.6, 8.2, 108.001, 5, 110.001);
            p.cubicTo(1.8, 112.001, -8.6, 123.601, -10.2, 122.801);
            p.cubicTo(-11.8, 122.001, -9.8, 121.601, -8.6, 118.801);
            p.cubicTo(-7.4, 116.001, -9.4, 114.401, -17.4, 120.801);
            p.cubicTo(-25.4, 127.201, -26.6, 129.201, -26.6, 129.201);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#000000";
            sfp.strokeWidth =  -1;
            p.moveTo(-19.195, 123.234);
            p.cubicTo(-19.195, 123.234, -17.785, 110.194, -9.307, 111.859);
            p.cubicTo(-9.307, 111.859, -1.081, 107.689, 1.641, 105.721);
            p.cubicTo(1.641, 105.721, 9.78, 104.019, 11.09, 103.402);
            p.cubicTo(29.569, 94.702, 44.288, 99.221, 44.835, 98.101);
            p.cubicTo(45.381, 96.982, 65.006, 104.099, 68.615, 108.185);
            p.cubicTo(69.006, 108.628, 58.384, 102.588, 48.686, 100.697);
            p.cubicTo(40.413, 99.083, 18.811, 100.944, 7.905, 106.48);
            p.cubicTo(4.932, 107.989, -4.013, 113.773, -6.544, 113.662);
            p.cubicTo(-9.075, 113.55, -19.195, 123.234, -19.195, 123.234);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#cccccc";
            sfp.strokeWidth =  -1;
            p.moveTo(-23, 148.801);
            p.cubicTo(-23, 148.801, -38.2, 146.401, -21.4, 144.801);
            p.cubicTo(-21.4, 144.801, -3.4, 142.801, 0.6, 137.601);
            p.cubicTo(0.6, 137.601, 14.2, 128.401, 17, 128.001);
            p.cubicTo(19.8, 127.601, 49.8, 120.401, 50.2, 118.001);
            p.cubicTo(50.6, 115.601, 56.2, 115.601, 57.8, 116.401);
            p.cubicTo(59.4, 117.201, 58.6, 118.401, 55.8, 119.201);
            p.cubicTo(53, 120.001, 21.8, 136.401, 15.4, 137.601);
            p.cubicTo(9, 138.801, -2.6, 146.401, -7.4, 147.601);
            p.cubicTo(-12.2, 148.801, -23, 148.801, -23, 148.801);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#000000";
            sfp.strokeWidth =  -1;
            p.moveTo(-3.48, 141.403);
            p.cubicTo(-3.48, 141.403, -12.062, 140.574, -3.461, 139.755);
            p.cubicTo(-3.461, 139.755, 5.355, 136.331, 7.403, 133.668);
            p.cubicTo(7.403, 133.668, 14.367, 128.957, 15.8, 128.753);
            p.cubicTo(17.234, 128.548, 31.194, 124.861, 31.399, 123.633);
            p.cubicTo(31.604, 122.404, 65.67, 109.823, 70.09, 113.013);
            p.cubicTo(73.001, 115.114, 63.1, 113.437, 53.466, 117.847);
            p.cubicTo(52.111, 118.467, 18.258, 133.054, 14.981, 133.668);
            p.cubicTo(11.704, 134.283, 5.765, 138.174, 3.307, 138.788);
            p.cubicTo(0.85, 139.403, -3.48, 141.403, -3.48, 141.403);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#000000";
            sfp.strokeWidth =  -1;
            p.moveTo(-11.4, 143.601);
            p.cubicTo(-11.4, 143.601, -6.2, 143.201, -7.4, 144.801);
            p.cubicTo(-8.6, 146.401, -11, 145.601, -11, 145.601);
            p.lineTo(-11.4, 143.601);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#000000";
            sfp.strokeWidth =  -1;
            p.moveTo(-18.6, 145.201);
            p.cubicTo(-18.6, 145.201, -13.4, 144.801, -14.6, 146.401);
            p.cubicTo(-15.8, 148.001, -18.2, 147.201, -18.2, 147.201);
            p.lineTo(-18.6, 145.201);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#000000";
            sfp.strokeWidth =  -1;
            p.moveTo(-29, 146.801);
            p.cubicTo(-29, 146.801, -23.8, 146.401, -25, 148.001);
            p.cubicTo(-26.2, 149.601, -28.6, 148.801, -28.6, 148.801);
            p.lineTo(-29, 146.801);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#000000";
            sfp.strokeWidth =  -1;
            p.moveTo(-36.6, 147.601);
            p.cubicTo(-36.6, 147.601, -31.4, 147.201, -32.6, 148.801);
            p.cubicTo(-33.8, 150.401, -36.2, 149.601, -36.2, 149.601);
            p.lineTo(-36.6, 147.601);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#000000";
            sfp.strokeWidth =  -1;
            p.moveTo(1.8, 108.001);
            p.cubicTo(1.8, 108.001, 6.2, 108.001, 5, 109.601);
            p.cubicTo(3.8, 111.201, 0.6, 110.801, 0.6, 110.801);
            p.lineTo(1.8, 108.001);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#000000";
            sfp.strokeWidth =  -1;
            p.moveTo(-8.2, 113.601);
            p.cubicTo(-8.2, 113.601, -1.694, 111.46, -4.2, 114.801);
            p.cubicTo(-5.4, 116.401, -7.8, 115.601, -7.8, 115.601);
            p.lineTo(-8.2, 113.601);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#000000";
            sfp.strokeWidth =  -1;
            p.moveTo(-19.4, 118.401);
            p.cubicTo(-19.4, 118.401, -14.2, 118.001, -15.4, 119.601);
            p.cubicTo(-16.6, 121.201, -19, 120.401, -19, 120.401);
            p.lineTo(-19.4, 118.401);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#000000";
            sfp.strokeWidth =  -1;
            p.moveTo(-27, 124.401);
            p.cubicTo(-27, 124.401, -21.8, 124.001, -23, 125.601);
            p.cubicTo(-24.2, 127.201, -26.6, 126.401, -26.6, 126.401);
            p.lineTo(-27, 124.401);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#000000";
            sfp.strokeWidth =  -1;
            p.moveTo(-33.8, 129.201);
            p.cubicTo(-33.8, 129.201, -28.6, 128.801, -29.8, 130.401);
            p.cubicTo(-31, 132.001, -33.4, 131.201, -33.4, 131.201);
            p.lineTo(-33.8, 129.201);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#000000";
            sfp.strokeWidth =  -1;
            p.moveTo(5.282, 135.598);
            p.cubicTo(5.282, 135.598, 12.203, 135.066, 10.606, 137.195);
            p.cubicTo(9.009, 139.325, 5.814, 138.26, 5.814, 138.26);
            p.lineTo(5.282, 135.598);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#000000";
            sfp.strokeWidth =  -1;
            p.moveTo(15.682, 130.798);
            p.cubicTo(15.682, 130.798, 22.603, 130.266, 21.006, 132.395);
            p.cubicTo(19.409, 134.525, 16.214, 133.46, 16.214, 133.46);
            p.lineTo(15.682, 130.798);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#000000";
            sfp.strokeWidth =  -1;
            p.moveTo(26.482, 126.398);
            p.cubicTo(26.482, 126.398, 33.403, 125.866, 31.806, 127.995);
            p.cubicTo(30.209, 130.125, 27.014, 129.06, 27.014, 129.06);
            p.lineTo(26.482, 126.398);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#000000";
            sfp.strokeWidth =  -1;
            p.moveTo(36.882, 121.598);
            p.cubicTo(36.882, 121.598, 43.803, 121.066, 42.206, 123.195);
            p.cubicTo(40.609, 125.325, 37.414, 124.26, 37.414, 124.26);
            p.lineTo(36.882, 121.598);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#000000";
            sfp.strokeWidth =  -1;
            p.moveTo(9.282, 103.598);
            p.cubicTo(9.282, 103.598, 16.203, 103.066, 14.606, 105.195);
            p.cubicTo(13.009, 107.325, 9.014, 107.06, 9.014, 107.06);
            p.lineTo(9.282, 103.598);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#000000";
            sfp.strokeWidth =  -1;
            p.moveTo(19.282, 100.398);
            p.cubicTo(19.282, 100.398, 26.203, 99.866, 24.606, 101.995);
            p.cubicTo(23.009, 104.125, 18.614, 103.86, 18.614, 103.86);
            p.lineTo(19.282, 100.398);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#000000";
            sfp.strokeWidth =  -1;
            p.moveTo(-3.4, 140.401);
            p.cubicTo(-3.4, 140.401, 1.8, 140.001, 0.6, 141.601);
            p.cubicTo(-0.6, 143.201, -3, 142.401, -3, 142.401);
            p.lineTo(-3.4, 140.401);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#992600";
            sfp.strokeWidth =  -1;
            p.moveTo(-76.6, 41.2);
            p.cubicTo(-76.6, 41.2, -81, 50, -81.4, 53.2);
            p.cubicTo(-81.4, 53.2, -80.6, 44.4, -79.4, 42.4);
            p.cubicTo(-78.2, 40.4, -76.6, 41.2, -76.6, 41.2);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#992600";
            sfp.strokeWidth =  -1;
            p.moveTo(-95, 55.2);
            p.cubicTo(-95, 55.2, -98.2, 69.6, -97.8, 72.4);
            p.cubicTo(-97.8, 72.4, -99, 60.8, -98.6, 59.6);
            p.cubicTo(-98.2, 58.4, -95, 55.2, -95, 55.2);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#cccccc";
            sfp.strokeWidth =  -1;
            p.moveTo(-74.2, -19.4);
            p.lineTo(-74.4, -16.2);
            p.lineTo(-76.6, -16);
            p.cubicTo(-76.6, -16, -62.4, -3.4, -61.8, 4.2);
            p.cubicTo(-61.8, 4.2, -61, -4, -74.2, -19.4);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#000000";
            sfp.strokeWidth =  -1;
            p.moveTo(-70.216, -18.135);
            p.cubicTo(-70.647, -18.551, -70.428, -19.296, -70.836, -19.556);
            p.cubicTo(-71.645, -20.072, -69.538, -20.129, -69.766, -20.845);
            p.cubicTo(-70.149, -22.051, -69.962, -22.072, -70.084, -23.348);
            p.cubicTo(-70.141, -23.946, -69.553, -25.486, -69.168, -25.926);
            p.cubicTo(-67.722, -27.578, -69.046, -30.51, -67.406, -32.061);
            p.cubicTo(-67.102, -32.35, -66.726, -32.902, -66.441, -33.32);
            p.cubicTo(-65.782, -34.283, -64.598, -34.771, -63.648, -35.599);
            p.cubicTo(-63.33, -35.875, -63.531, -36.702, -62.962, -36.61);
            p.cubicTo(-62.248, -36.495, -61.007, -36.625, -61.052, -35.784);
            p.cubicTo(-61.165, -33.664, -62.494, -31.944, -63.774, -30.276);
            p.cubicTo(-63.323, -29.572, -63.781, -28.937, -64.065, -28.38);
            p.cubicTo(-65.4, -25.76, -65.211, -22.919, -65.385, -20.079);
            p.cubicTo(-65.39, -19.994, -65.697, -19.916, -65.689, -19.863);
            p.cubicTo(-65.336, -17.528, -64.752, -15.329, -63.873, -13.1);
            p.cubicTo(-63.507, -12.17, -63.036, -11.275, -62.886, -10.348);
            p.cubicTo(-62.775, -9.662, -62.672, -8.829, -63.08, -8.124);
            p.cubicTo(-61.045, -5.234, -62.354, -2.583, -61.185, 0.948);
            p.cubicTo(-60.978, 1.573, -59.286, 3.487, -59.749, 3.326);
            p.cubicTo(-62.262, 2.455, -62.374, 2.057, -62.551, 1.304);
            p.cubicTo(-62.697, 0.681, -63.027, -0.696, -63.264, -1.298);
            p.cubicTo(-63.328, -1.462, -63.499, -3.346, -63.577, -3.468);
            p.cubicTo(-65.09, -5.85, -63.732, -5.674, -65.102, -8.032);
            p.cubicTo(-66.53, -8.712, -67.496, -9.816, -68.619, -10.978);
            p.cubicTo(-68.817, -11.182, -67.674, -11.906, -67.855, -12.119);
            p.cubicTo(-68.947, -13.408, -70.1, -14.175, -69.764, -15.668);
            p.cubicTo(-69.609, -16.358, -69.472, -17.415, -70.216, -18.135);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#000000";
            sfp.strokeWidth =  -1;
            p.moveTo(-73.8, -16.4);
            p.cubicTo(-73.8, -16.4, -73.4, -9.6, -71, -8);
            p.cubicTo(-68.6, -6.4, -69.8, -7.2, -73, -8.4);
            p.cubicTo(-76.2, -9.6, -75, -10.4, -75, -10.4);
            p.cubicTo(-75, -10.4, -77.8, -10, -75.4, -8);
            p.cubicTo(-73, -6, -69.4, -3.6, -71, -3.6);
            p.cubicTo(-72.6, -3.6, -80.2, -7.6, -80.2, -10.4);
            p.cubicTo(-80.2, -13.2, -81.2, -17.3, -81.2, -17.3);
            p.cubicTo(-81.2, -17.3, -80.1, -18.1, -75.3, -18);
            p.cubicTo(-75.3, -18, -73.9, -17.3, -73.8, -16.4);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#ffffff";
            sfp.strokeColor = "#000000";
            sfp.strokeWidth = 0.1;
            p.moveTo(-74.6, 2.2);
            p.cubicTo(-74.6, 2.2, -83.12, -0.591, -101.6, 2.8);
            p.cubicTo(-101.6, 2.8, -92.569, 0.722, -73.8, 3);
            p.cubicTo(-63.5, 4.25, -74.6, 2.2, -74.6, 2.2);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#ffffff";
            sfp.strokeColor = "#000000";
            sfp.strokeWidth = 0.1;
            p.moveTo(-72.502, 2.129);
            p.cubicTo(-72.502, 2.129, -80.748, -1.389, -99.453, 0.392);
            p.cubicTo(-99.453, 0.392, -90.275, -0.897, -71.774, 2.995);
            p.cubicTo(-61.62, 5.131, -72.502, 2.129, -72.502, 2.129);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#ffffff";
            sfp.strokeColor = "#000000";
            sfp.strokeWidth = 0.1;
            p.moveTo(-70.714, 2.222);
            p.cubicTo(-70.714, 2.222, -78.676, -1.899, -97.461, -1.514);
            p.cubicTo(-97.461, -1.514, -88.213, -2.118, -70.052, 3.14);
            p.cubicTo(-60.086, 6.025, -70.714, 2.222, -70.714, 2.222);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#ffffff";
            sfp.strokeColor = "#000000";
            sfp.strokeWidth = 0.1;
            p.moveTo(-69.444, 2.445);
            p.cubicTo(-69.444, 2.445, -76.268, -1.862, -93.142, -2.96);
            p.cubicTo(-93.142, -2.96, -84.803, -2.79, -68.922, 3.319);
            p.cubicTo(-60.206, 6.672, -69.444, 2.445, -69.444, 2.445);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#ffffff";
            sfp.strokeColor = "#000000";
            sfp.strokeWidth = 0.1;
            p.moveTo(45.84, 12.961);
            p.cubicTo(45.84, 12.961, 44.91, 13.605, 45.124, 12.424);
            p.cubicTo(45.339, 11.243, 73.547, -1.927, 77.161, -1.677);
            p.cubicTo(77.161, -1.677, 46.913, 11.529, 45.84, 12.961);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#ffffff";
            sfp.strokeColor = "#000000";
            sfp.strokeWidth = 0.1;
            p.moveTo(42.446, 13.6);
            p.cubicTo(42.446, 13.6, 41.57, 14.315, 41.691, 13.121);
            p.cubicTo(41.812, 11.927, 68.899, -3.418, 72.521, -3.452);
            p.cubicTo(72.521, -3.452, 43.404, 12.089, 42.446, 13.6);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#ffffff";
            sfp.strokeColor = "#000000";
            sfp.strokeWidth = 0.1;
            p.moveTo(39.16, 14.975);
            p.cubicTo(39.16, 14.975, 38.332, 15.747, 38.374, 14.547);
            p.cubicTo(38.416, 13.348, 58.233, -2.149, 68.045, -4.023);
            p.cubicTo(68.045, -4.023, 50.015, 4.104, 39.16, 14.975);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#ffffff";
            sfp.strokeColor = "#000000";
            sfp.strokeWidth = 0.1;
            p.moveTo(36.284, 16.838);
            p.cubicTo(36.284, 16.838, 35.539, 17.532, 35.577, 16.453);
            p.cubicTo(35.615, 15.373, 53.449, 1.426, 62.28, -0.26);
            p.cubicTo(62.28, -0.26, 46.054, 7.054, 36.284, 16.838);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#cccccc";
            sfp.strokeWidth =  -1;
            p.moveTo(4.6, 164.801);
            p.cubicTo(4.6, 164.801, -10.6, 162.401, 6.2, 160.801);
            p.cubicTo(6.2, 160.801, 24.2, 158.801, 28.2, 153.601);
            p.cubicTo(28.2, 153.601, 41.8, 144.401, 44.6, 144.001);
            p.cubicTo(47.4, 143.601, 63.8, 140.001, 64.2, 137.601);
            p.cubicTo(64.6, 135.201, 70.6, 132.801, 72.2, 133.601);
            p.cubicTo(73.8, 134.401, 73.8, 143.601, 71, 144.401);
            p.cubicTo(68.2, 145.201, 49.4, 152.401, 43, 153.601);
            p.cubicTo(36.6, 154.801, 25, 162.401, 20.2, 163.601);
            p.cubicTo(15.4, 164.801, 4.6, 164.801, 4.6, 164.801);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#000000";
            sfp.strokeWidth =  -1;
            p.moveTo(77.6, 127.401);
            p.cubicTo(77.6, 127.401, 74.6, 129.001, 73.4, 131.601);
            p.cubicTo(73.4, 131.601, 67, 142.201, 52.8, 145.401);
            p.cubicTo(52.8, 145.401, 29.8, 154.401, 22, 156.401);
            p.cubicTo(22, 156.401, 8.6, 161.401, 1.2, 160.601);
            p.cubicTo(1.2, 160.601, -5.8, 160.801, 0.4, 162.401);
            p.cubicTo(0.4, 162.401, 20.6, 160.401, 24, 158.601);
            p.cubicTo(24, 158.601, 39.6, 153.401, 42.6, 150.801);
            p.cubicTo(45.6, 148.201, 63.8, 143.201, 66, 141.201);
            p.cubicTo(68.2, 139.201, 78, 130.801, 77.6, 127.401);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#000000";
            sfp.strokeWidth =  -1;
            p.moveTo(18.882, 158.911);
            p.cubicTo(18.882, 158.911, 24.111, 158.685, 22.958, 160.234);
            p.cubicTo(21.805, 161.784, 19.357, 160.91, 19.357, 160.91);
            p.lineTo(18.882, 158.911);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#000000";
            sfp.strokeWidth =  -1;
            p.moveTo(11.68, 160.263);
            p.cubicTo(11.68, 160.263, 16.908, 160.037, 15.756, 161.586);
            p.cubicTo(14.603, 163.136, 12.155, 162.263, 12.155, 162.263);
            p.lineTo(11.68, 160.263);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#000000";
            sfp.strokeWidth =  -1;
            p.moveTo(1.251, 161.511);
            p.cubicTo(1.251, 161.511, 6.48, 161.284, 5.327, 162.834);
            p.cubicTo(4.174, 164.383, 1.726, 163.51, 1.726, 163.51);
            p.lineTo(1.251, 161.511);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#000000";
            sfp.strokeWidth =  -1;
            p.moveTo(-6.383, 162.055);
            p.cubicTo(-6.383, 162.055, -1.154, 161.829, -2.307, 163.378);
            p.cubicTo(-3.46, 164.928, -5.908, 164.054, -5.908, 164.054);
            p.lineTo(-6.383, 162.055);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#000000";
            sfp.strokeWidth =  -1;
            p.moveTo(35.415, 151.513);
            p.cubicTo(35.415, 151.513, 42.375, 151.212, 40.84, 153.274);
            p.cubicTo(39.306, 155.336, 36.047, 154.174, 36.047, 154.174);
            p.lineTo(35.415, 151.513);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#000000";
            sfp.strokeWidth =  -1;
            p.moveTo(45.73, 147.088);
            p.cubicTo(45.73, 147.088, 51.689, 143.787, 51.155, 148.849);
            p.cubicTo(50.885, 151.405, 46.362, 149.749, 46.362, 149.749);
            p.lineTo(45.73, 147.088);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#000000";
            sfp.strokeWidth =  -1;
            p.moveTo(54.862, 144.274);
            p.cubicTo(54.862, 144.274, 62.021, 140.573, 60.287, 146.035);
            p.cubicTo(59.509, 148.485, 55.493, 146.935, 55.493, 146.935);
            p.lineTo(54.862, 144.274);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#000000";
            sfp.strokeWidth =  -1;
            p.moveTo(64.376, 139.449);
            p.cubicTo(64.376, 139.449, 68.735, 134.548, 69.801, 141.21);
            p.cubicTo(70.207, 143.748, 65.008, 142.11, 65.008, 142.11);
            p.lineTo(64.376, 139.449);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#000000";
            sfp.strokeWidth =  -1;
            p.moveTo(26.834, 155.997);
            p.cubicTo(26.834, 155.997, 32.062, 155.77, 30.91, 157.32);
            p.cubicTo(29.757, 158.869, 27.308, 157.996, 27.308, 157.996);
            p.lineTo(26.834, 155.997);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#ffffff";
            sfp.strokeColor = "#000000";
            sfp.strokeWidth = 0.1;
            p.moveTo(62.434, 34.603);
            p.cubicTo(62.434, 34.603, 61.708, 35.268, 61.707, 34.197);
            p.cubicTo(61.707, 33.127, 79.191, 19.863, 88.034, 18.479);
            p.cubicTo(88.034, 18.479, 71.935, 25.208, 62.434, 34.603);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#000000";
            sfp.strokeWidth =  -1;
            p.moveTo(65.4, 98.4);
            p.cubicTo(65.4, 98.4, 87.401, 120.801, 96.601, 124.401);
            p.cubicTo(96.601, 124.401, 105.801, 135.601, 101.801, 161.601);
            p.cubicTo(101.801, 161.601, 98.601, 169.201, 95.401, 148.401);
            p.cubicTo(95.401, 148.401, 98.601, 123.201, 87.401, 139.201);
            p.cubicTo(87.401, 139.201, 79, 129.301, 85.4, 129.601);
            p.cubicTo(85.4, 129.601, 88.601, 131.601, 89.001, 130.001);
            p.cubicTo(89.401, 128.401, 81.4, 114.801, 64.2, 100.4);
            p.cubicTo(47, 86, 65.4, 98.4, 65.4, 98.4);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#ffffff";
            sfp.strokeColor = "#000000";
            sfp.strokeWidth = 0.1;
            p.moveTo(7, 137.201);
            p.cubicTo(7, 137.201, 6.8, 135.401, 8.6, 136.201);
            p.cubicTo(10.4, 137.001, 104.601, 143.201, 136.201, 167.201);
            p.cubicTo(136.201, 167.201, 91.001, 144.001, 7, 137.201);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#ffffff";
            sfp.strokeColor = "#000000";
            sfp.strokeWidth = 0.1;
            p.moveTo(17.4, 132.801);
            p.cubicTo(17.4, 132.801, 17.2, 131.001, 19, 131.801);
            p.cubicTo(20.8, 132.601, 157.401, 131.601, 181.001, 164.001);
            p.cubicTo(181.001, 164.001, 159.001, 138.801, 17.4, 132.801);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#ffffff";
            sfp.strokeColor = "#000000";
            sfp.strokeWidth = 0.1;
            p.moveTo(29, 128.801);
            p.cubicTo(29, 128.801, 28.8, 127.001, 30.6, 127.801);
            p.cubicTo(32.4, 128.601, 205.801, 115.601, 229.401, 148.001);
            p.cubicTo(229.401, 148.001, 219.801, 122.401, 29, 128.801);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#ffffff";
            sfp.strokeColor = "#000000";
            sfp.strokeWidth = 0.1;
            p.moveTo(39, 124.001);
            p.cubicTo(39, 124.001, 38.8, 122.201, 40.6, 123.001);
            p.cubicTo(42.4, 123.801, 164.601, 85.2, 188.201, 117.601);
            p.cubicTo(188.201, 117.601, 174.801, 93, 39, 124.001);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#ffffff";
            sfp.strokeColor = "#000000";
            sfp.strokeWidth = 0.1;
            p.moveTo(-19, 146.801);
            p.cubicTo(-19, 146.801, -19.2, 145.001, -17.4, 145.801);
            p.cubicTo(-15.6, 146.601, 2.2, 148.801, 4.2, 187.601);
            p.cubicTo(4.2, 187.601, -3, 145.601, -19, 146.801);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#ffffff";
            sfp.strokeColor = "#000000";
            sfp.strokeWidth = 0.1;
            p.moveTo(-27.8, 148.401);
            p.cubicTo(-27.8, 148.401, -28, 146.601, -26.2, 147.401);
            p.cubicTo(-24.4, 148.201, -10.2, 143.601, -13, 182.401);
            p.cubicTo(-13, 182.401, -11.8, 147.201, -27.8, 148.401);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#ffffff";
            sfp.strokeColor = "#000000";
            sfp.strokeWidth = 0.1;
            p.moveTo(-35.8, 148.801);
            p.cubicTo(-35.8, 148.801, -36, 147.001, -34.2, 147.801);
            p.cubicTo(-32.4, 148.601, -17, 149.201, -29.4, 171.601);
            p.cubicTo(-29.4, 171.601, -19.8, 147.601, -35.8, 148.801);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#ffffff";
            sfp.strokeColor = "#000000";
            sfp.strokeWidth = 0.1;
            p.moveTo(11.526, 104.465);
            p.cubicTo(11.526, 104.465, 11.082, 106.464, 12.631, 105.247);
            p.cubicTo(28.699, 92.622, 61.141, 33.72, 116.826, 28.086);
            p.cubicTo(116.826, 28.086, 78.518, 15.976, 11.526, 104.465);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#ffffff";
            sfp.strokeColor = "#000000";
            sfp.strokeWidth = 0.1;
            p.moveTo(22.726, 102.665);
            p.cubicTo(22.726, 102.665, 21.363, 101.472, 23.231, 100.847);
            p.cubicTo(25.099, 100.222, 137.541, 27.72, 176.826, 35.686);
            p.cubicTo(176.826, 35.686, 149.719, 28.176, 22.726, 102.665);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#ffffff";
            sfp.strokeColor = "#000000";
            sfp.strokeWidth = 0.1;
            p.moveTo(1.885, 108.767);
            p.cubicTo(1.885, 108.767, 1.376, 110.366, 3.087, 109.39);
            p.cubicTo(12.062, 104.27, 15.677, 47.059, 59.254, 45.804);
            p.cubicTo(59.254, 45.804, 26.843, 31.09, 1.885, 108.767);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#ffffff";
            sfp.strokeColor = "#000000";
            sfp.strokeWidth = 0.1;
            p.moveTo(-18.038, 119.793);
            p.cubicTo(-18.038, 119.793, -19.115, 121.079, -17.162, 120.825);
            p.cubicTo(-6.916, 119.493, 14.489, 78.222, 58.928, 83.301);
            p.cubicTo(58.928, 83.301, 26.962, 68.955, -18.038, 119.793);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#ffffff";
            sfp.strokeColor = "#000000";
            sfp.strokeWidth = 0.1;
            p.moveTo(-6.8, 113.667);
            p.cubicTo(-6.8, 113.667, -7.611, 115.136, -5.742, 114.511);
            p.cubicTo(4.057, 111.237, 17.141, 66.625, 61.729, 63.078);
            p.cubicTo(61.729, 63.078, 27.603, 55.135, -6.8, 113.667);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#ffffff";
            sfp.strokeColor = "#000000";
            sfp.strokeWidth = 0.1;
            p.moveTo(-25.078, 124.912);
            p.cubicTo(-25.078, 124.912, -25.951, 125.954, -24.369, 125.748);
            p.cubicTo(-16.07, 124.669, 1.268, 91.24, 37.264, 95.354);
            p.cubicTo(37.264, 95.354, 11.371, 83.734, -25.078, 124.912);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#ffffff";
            sfp.strokeColor = "#000000";
            sfp.strokeWidth = 0.1;
            p.moveTo(-32.677, 130.821);
            p.cubicTo(-32.677, 130.821, -33.682, 131.866, -32.091, 131.748);
            p.cubicTo(-27.923, 131.439, 2.715, 98.36, 21.183, 113.862);
            p.cubicTo(21.183, 113.862, 9.168, 95.139, -32.677, 130.821);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#ffffff";
            sfp.strokeColor = "#000000";
            sfp.strokeWidth = 0.1;
            p.moveTo(36.855, 98.898);
            p.cubicTo(36.855, 98.898, 35.654, 97.543, 37.586, 97.158);
            p.cubicTo(39.518, 96.774, 160.221, 39.061, 198.184, 51.927);
            p.cubicTo(198.184, 51.927, 172.243, 41.053, 36.855, 98.898);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#ffffff";
            sfp.strokeColor = "#000000";
            sfp.strokeWidth = 0.1;
            p.moveTo(3.4, 163.201);
            p.cubicTo(3.4, 163.201, 3.2, 161.401, 5, 162.201);
            p.cubicTo(6.8, 163.001, 22.2, 163.601, 9.8, 186.001);
            p.cubicTo(9.8, 186.001, 19.4, 162.001, 3.4, 163.201);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#ffffff";
            sfp.strokeColor = "#000000";
            sfp.strokeWidth = 0.1;
            p.moveTo(13.8, 161.601);
            p.cubicTo(13.8, 161.601, 13.6, 159.801, 15.4, 160.601);
            p.cubicTo(17.2, 161.401, 35, 163.601, 37, 202.401);
            p.cubicTo(37, 202.401, 29.8, 160.401, 13.8, 161.601);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#ffffff";
            sfp.strokeColor = "#000000";
            sfp.strokeWidth = 0.1;
            p.moveTo(20.6, 160.001);
            p.cubicTo(20.6, 160.001, 20.4, 158.201, 22.2, 159.001);
            p.cubicTo(24, 159.801, 48.6, 163.201, 72.2, 195.601);
            p.cubicTo(72.2, 195.601, 36.6, 158.801, 20.6, 160.001);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#ffffff";
            sfp.strokeColor = "#000000";
            sfp.strokeWidth = 0.1;
            p.moveTo(28.225, 157.972);
            p.cubicTo(28.225, 157.972, 27.788, 156.214, 29.678, 156.768);
            p.cubicTo(31.568, 157.322, 52.002, 155.423, 90.099, 189.599);
            p.cubicTo(90.099, 189.599, 43.924, 154.656, 28.225, 157.972);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#ffffff";
            sfp.strokeColor = "#000000";
            sfp.strokeWidth = 0.1;
            p.moveTo(38.625, 153.572);
            p.cubicTo(38.625, 153.572, 38.188, 151.814, 40.078, 152.368);
            p.cubicTo(41.968, 152.922, 76.802, 157.423, 128.499, 192.399);
            p.cubicTo(128.499, 192.399, 54.324, 150.256, 38.625, 153.572);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#ffffff";
            sfp.strokeColor = "#000000";
            sfp.strokeWidth = 0.1;
            p.moveTo(-1.8, 142.001);
            p.cubicTo(-1.8, 142.001, -2, 140.201, -0.2, 141.001);
            p.cubicTo(1.6, 141.801, 55, 144.401, 85.4, 171.201);
            p.cubicTo(85.4, 171.201, 50.499, 146.426, -1.8, 142.001);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#ffffff";
            sfp.strokeColor = "#000000";
            sfp.strokeWidth = 0.1;
            p.moveTo(-11.8, 146.001);
            p.cubicTo(-11.8, 146.001, -12, 144.201, -10.2, 145.001);
            p.cubicTo(-8.4, 145.801, 16.2, 149.201, 39.8, 181.601);
            p.cubicTo(39.8, 181.601, 4.2, 144.801, -11.8, 146.001);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#ffffff";
            sfp.strokeColor = "#000000";
            sfp.strokeWidth = 0.1;
            p.moveTo(49.503, 148.962);
            p.cubicTo(49.503, 148.962, 48.938, 147.241, 50.864, 147.655);
            p.cubicTo(52.79, 148.068, 87.86, 150.004, 141.981, 181.098);
            p.cubicTo(141.981, 181.098, 64.317, 146.704, 49.503, 148.962);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#ffffff";
            sfp.strokeColor = "#000000";
            sfp.strokeWidth = 0.1;
            p.moveTo(57.903, 146.562);
            p.cubicTo(57.903, 146.562, 57.338, 144.841, 59.264, 145.255);
            p.cubicTo(61.19, 145.668, 96.26, 147.604, 150.381, 178.698);
            p.cubicTo(150.381, 178.698, 73.317, 143.904, 57.903, 146.562);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#ffffff";
            sfp.strokeColor = "#000000";
            sfp.strokeWidth = 0.1;
            p.moveTo(67.503, 141.562);
            p.cubicTo(67.503, 141.562, 66.938, 139.841, 68.864, 140.255);
            p.cubicTo(70.79, 140.668, 113.86, 145.004, 203.582, 179.298);
            p.cubicTo(203.582, 179.298, 82.917, 138.904, 67.503, 141.562);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#000000";
            sfp.strokeWidth =  -1;
            p.moveTo(-43.8, 148.401);
            p.cubicTo(-43.8, 148.401, -38.6, 148.001, -39.8, 149.601);
            p.cubicTo(-41, 151.201, -43.4, 150.401, -43.4, 150.401);
            p.lineTo(-43.8, 148.401);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#000000";
            sfp.strokeWidth =  -1;
            p.moveTo(-13, 162.401);
            p.cubicTo(-13, 162.401, -7.8, 162.001, -9, 163.601);
            p.cubicTo(-10.2, 165.201, -12.6, 164.401, -12.6, 164.401);
            p.lineTo(-13, 162.401);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#000000";
            sfp.strokeWidth =  -1;
            p.moveTo(-21.8, 162.001);
            p.cubicTo(-21.8, 162.001, -16.6, 161.601, -17.8, 163.201);
            p.cubicTo(-19, 164.801, -21.4, 164.001, -21.4, 164.001);
            p.lineTo(-21.8, 162.001);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#000000";
            sfp.strokeWidth =  -1;
            p.moveTo(-117.169, 150.182);
            p.cubicTo(-117.169, 150.182, -112.124, 151.505, -113.782, 152.624);
            p.cubicTo(-115.439, 153.744, -117.446, 152.202, -117.446, 152.202);
            p.lineTo(-117.169, 150.182);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#000000";
            sfp.strokeWidth =  -1;
            p.moveTo(-115.169, 140.582);
            p.cubicTo(-115.169, 140.582, -110.124, 141.905, -111.782, 143.024);
            p.cubicTo(-113.439, 144.144, -115.446, 142.602, -115.446, 142.602);
            p.lineTo(-115.169, 140.582);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#000000";
            sfp.strokeWidth =  -1;
            p.moveTo(-122.369, 136.182);
            p.cubicTo(-122.369, 136.182, -117.324, 137.505, -118.982, 138.624);
            p.cubicTo(-120.639, 139.744, -122.646, 138.202, -122.646, 138.202);
            p.lineTo(-122.369, 136.182);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#cccccc";
            sfp.strokeWidth =  -1;
            p.moveTo(-42.6, 211.201);
            p.cubicTo(-42.6, 211.201, -44.2, 211.201, -48.2, 213.201);
            p.cubicTo(-50.2, 213.201, -61.4, 216.801, -67, 226.801);
            p.cubicTo(-67, 226.801, -54.6, 217.201, -42.6, 211.201);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#cccccc";
            sfp.strokeWidth =  -1;
            p.moveTo(45.116, 303.847);
            p.cubicTo(45.257, 304.105, 45.312, 304.525, 45.604, 304.542);
            p.cubicTo(46.262, 304.582, 47.495, 304.883, 47.37, 304.247);
            p.cubicTo(46.522, 299.941, 45.648, 295.004, 41.515, 293.197);
            p.cubicTo(40.876, 292.918, 39.434, 293.331, 39.36, 294.215);
            p.cubicTo(39.233, 295.739, 39.116, 297.088, 39.425, 298.554);
            p.cubicTo(39.725, 299.975, 41.883, 299.985, 42.8, 298.601);
            p.cubicTo(43.736, 300.273, 44.168, 302.116, 45.116, 303.847);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#cccccc";
            sfp.strokeWidth =  -1;
            p.moveTo(34.038, 308.581);
            p.cubicTo(34.786, 309.994, 34.659, 311.853, 36.074, 312.416);
            p.cubicTo(36.814, 312.71, 38.664, 311.735, 38.246, 310.661);
            p.cubicTo(37.444, 308.6, 37.056, 306.361, 35.667, 304.55);
            p.cubicTo(35.467, 304.288, 35.707, 303.755, 35.547, 303.427);
            p.cubicTo(34.953, 302.207, 33.808, 301.472, 32.4, 301.801);
            p.cubicTo(31.285, 304.004, 32.433, 306.133, 33.955, 307.842);
            p.cubicTo(34.091, 307.994, 33.925, 308.37, 34.038, 308.581);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#cccccc";
            sfp.strokeWidth =  -1;
            p.moveTo(-5.564, 303.391);
            p.cubicTo(-5.672, 303.014, -5.71, 302.551, -5.545, 302.23);
            p.cubicTo(-5.014, 301.197, -4.221, 300.075, -4.558, 299.053);
            p.cubicTo(-4.906, 297.997, -6.022, 298.179, -6.672, 298.748);
            p.cubicTo(-7.807, 299.742, -7.856, 301.568, -8.547, 302.927);
            p.cubicTo(-8.743, 303.313, -8.692, 303.886, -9.133, 304.277);
            p.cubicTo(-9.607, 304.698, -10.047, 306.222, -9.951, 306.793);
            p.cubicTo(-9.898, 307.106, -10.081, 317.014, -9.859, 316.751);
            p.cubicTo(-9.24, 316.018, -6.19, 306.284, -6.121, 305.392);
            p.cubicTo(-6.064, 304.661, -5.332, 304.196, -5.564, 303.391);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#cccccc";
            sfp.strokeWidth =  -1;
            p.moveTo(-31.202, 296.599);
            p.cubicTo(-28.568, 294.1, -25.778, 291.139, -26.22, 287.427);
            p.cubicTo(-26.336, 286.451, -28.111, 286.978, -28.298, 287.824);
            p.cubicTo(-29.1, 291.449, -31.139, 294.11, -33.707, 296.502);
            p.cubicTo(-35.903, 298.549, -37.765, 304.893, -38, 305.401);
            p.cubicTo(-34.303, 300.145, -32.046, 297.399, -31.202, 296.599);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#cccccc";
            sfp.strokeWidth =  -1;
            p.moveTo(-44.776, 290.635);
            p.cubicTo(-44.253, 290.265, -44.555, 289.774, -44.338, 289.442);
            p.cubicTo(-43.385, 287.984, -42.084, 286.738, -42.066, 285);
            p.cubicTo(-42.063, 284.723, -42.441, 284.414, -42.776, 284.638);
            p.cubicTo(-43.053, 284.822, -43.395, 284.952, -43.503, 285.082);
            p.cubicTo(-45.533, 287.531, -46.933, 290.202, -48.376, 293.014);
            p.cubicTo(-48.559, 293.371, -49.703, 297.862, -49.39, 297.973);
            p.cubicTo(-49.151, 298.058, -47.431, 293.877, -47.221, 293.763);
            p.cubicTo(-45.958, 293.077, -45.946, 291.462, -44.776, 290.635);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#cccccc";
            sfp.strokeWidth =  -1;
            p.moveTo(-28.043, 310.179);
            p.cubicTo(-27.599, 309.31, -26.023, 308.108, -26.136, 307.219);
            p.cubicTo(-26.254, 306.291, -25.786, 304.848, -26.698, 305.536);
            p.cubicTo(-27.955, 306.484, -31.404, 307.833, -31.674, 313.641);
            p.cubicTo(-31.7, 314.212, -28.726, 311.519, -28.043, 310.179);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#cccccc";
            sfp.strokeWidth =  -1;
            p.moveTo(-13.6, 293.001);
            p.cubicTo(-13.2, 292.333, -12.492, 292.806, -12.033, 292.543);
            p.cubicTo(-11.385, 292.171, -10.774, 291.613, -10.482, 290.964);
            p.cubicTo(-9.512, 288.815, -7.743, 286.995, -7.6, 284.601);
            p.cubicTo(-9.091, 283.196, -9.77, 285.236, -10.4, 286.201);
            p.cubicTo(-11.723, 284.554, -12.722, 286.428, -14.022, 286.947);
            p.cubicTo(-14.092, 286.975, -14.305, 286.628, -14.38, 286.655);
            p.cubicTo(-15.557, 287.095, -16.237, 288.176, -17.235, 288.957);
            p.cubicTo(-17.406, 289.091, -17.811, 288.911, -17.958, 289.047);
            p.cubicTo(-18.61, 289.65, -19.583, 289.975, -19.863, 290.657);
            p.cubicTo(-20.973, 293.364, -24.113, 295.459, -26, 303.001);
            p.cubicTo(-25.619, 303.91, -21.488, 296.359, -21.001, 295.661);
            p.cubicTo(-20.165, 294.465, -20.047, 297.322, -18.771, 296.656);
            p.cubicTo(-18.72, 296.629, -18.534, 296.867, -18.4, 297.001);
            p.cubicTo(-18.206, 296.721, -17.988, 296.492, -17.6, 296.601);
            p.cubicTo(-17.6, 296.201, -17.734, 295.645, -17.533, 295.486);
            p.cubicTo(-16.296, 294.509, -16.38, 293.441, -15.6, 292.201);
            p.cubicTo(-15.142, 292.99, -14.081, 292.271, -13.6, 293.001);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#cccccc";
            sfp.strokeWidth =  -1;
            p.moveTo(46.2, 347.401);
            p.cubicTo(46.2, 347.401, 53.6, 327.001, 49.2, 315.801);
            p.cubicTo(49.2, 315.801, 60.6, 337.401, 56, 348.601);
            p.cubicTo(56, 348.601, 55.6, 338.201, 51.6, 333.201);
            p.cubicTo(51.6, 333.201, 47.6, 346.001, 46.2, 347.401);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#cccccc";
            sfp.strokeWidth =  -1;
            p.moveTo(31.4, 344.801);
            p.cubicTo(31.4, 344.801, 36.8, 336.001, 28.8, 317.601);
            p.cubicTo(28.8, 317.601, 28, 338.001, 21.2, 349.001);
            p.cubicTo(21.2, 349.001, 35.4, 328.801, 31.4, 344.801);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#cccccc";
            sfp.strokeWidth =  -1;
            p.moveTo(21.4, 342.801);
            p.cubicTo(21.4, 342.801, 21.2, 322.801, 21.6, 319.801);
            p.cubicTo(21.6, 319.801, 17.8, 336.401, 7.6, 346.001);
            p.cubicTo(7.6, 346.001, 22, 334.001, 21.4, 342.801);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#cccccc";
            sfp.strokeWidth =  -1;
            p.moveTo(11.8, 310.801);
            p.cubicTo(11.8, 310.801, 17.8, 324.401, 7.8, 342.801);
            p.cubicTo(7.8, 342.801, 14.2, 330.601, 9.4, 323.601);
            p.cubicTo(9.4, 323.601, 12, 320.201, 11.8, 310.801);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#cccccc";
            sfp.strokeWidth =  -1;
            p.moveTo(-7.4, 342.401);
            p.cubicTo(-7.4, 342.401, -8.4, 326.801, -6.6, 324.601);
            p.cubicTo(-6.6, 324.601, -6.4, 318.201, -6.8, 317.201);
            p.cubicTo(-6.8, 317.201, -2.8, 311.001, -2.6, 318.401);
            p.cubicTo(-2.6, 318.401, -1.2, 326.201, 1.6, 330.801);
            p.cubicTo(1.6, 330.801, 5.2, 336.201, 5, 342.601);
            p.cubicTo(5, 342.601, -5, 312.401, -7.4, 342.401);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#cccccc";
            sfp.strokeWidth =  -1;
            p.moveTo(-11, 314.801);
            p.cubicTo(-11, 314.801, -17.6, 325.601, -19.4, 344.601);
            p.cubicTo(-19.4, 344.601, -20.8, 338.401, -17, 324.001);
            p.cubicTo(-17, 324.001, -12.8, 308.601, -11, 314.801);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#cccccc";
            sfp.strokeWidth =  -1;
            p.moveTo(-32.8, 334.601);
            p.cubicTo(-32.8, 334.601, -27.8, 329.201, -26.4, 324.201);
            p.cubicTo(-26.4, 324.201, -22.8, 308.401, -29.2, 317.001);
            p.cubicTo(-29.2, 317.001, -29, 325.001, -37.2, 332.401);
            p.cubicTo(-37.2, 332.401, -32.4, 330.001, -32.8, 334.601);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#cccccc";
            sfp.strokeWidth =  -1;
            p.moveTo(-38.6, 329.601);
            p.cubicTo(-38.6, 329.601, -35.2, 312.201, -34.4, 311.401);
            p.cubicTo(-34.4, 311.401, -32.6, 308.001, -35.4, 311.201);
            p.cubicTo(-35.4, 311.201, -44.2, 330.401, -48.2, 337.001);
            p.cubicTo(-48.2, 337.001, -40.2, 327.801, -38.6, 329.601);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#cccccc";
            sfp.strokeWidth =  -1;
            p.moveTo(-44.4, 313.001);
            p.cubicTo(-44.4, 313.001, -32.8, 290.601, -54.6, 316.401);
            p.cubicTo(-54.6, 316.401, -43.6, 306.601, -44.4, 313.001);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#cccccc";
            sfp.strokeWidth =  -1;
            p.moveTo(-59.8, 298.401);
            p.cubicTo(-59.8, 298.401, -55, 279.601, -52.4, 279.801);
            p.cubicTo(-52.4, 279.801, -44.2, 270.801, -50.8, 281.401);
            p.cubicTo(-50.8, 281.401, -56.8, 291.001, -56.2, 300.801);
            p.cubicTo(-56.2, 300.801, -56.8, 291.201, -59.8, 298.401);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#cccccc";
            sfp.strokeWidth =  -1;
            p.moveTo(270.5, 287);
            p.cubicTo(270.5, 287, 258.5, 277, 256, 273.5);
            p.cubicTo(256, 273.5, 269.5, 292, 269.5, 299);
            p.cubicTo(269.5, 299, 272, 291.5, 270.5, 287);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#cccccc";
            sfp.strokeWidth =  -1;
            p.moveTo(276, 265);
            p.cubicTo(276, 265, 255, 250, 251.5, 242.5);
            p.cubicTo(251.5, 242.5, 278, 272, 278, 276.5);
            p.cubicTo(278, 276.5, 278.5, 267.5, 276, 265);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#cccccc";
            sfp.strokeWidth =  -1;
            p.moveTo(293, 111);
            p.cubicTo(293, 111, 281, 103, 279.5, 105);
            p.cubicTo(279.5, 105, 290, 111.5, 292.5, 120);
            p.cubicTo(292.5, 120, 291, 111, 293, 111);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor =  "#cccccc";
            sfp.strokeWidth =  -1;
            p.moveTo(301.5, 191.5);
            p.lineTo(284, 179.5);
            p.cubicTo(284, 179.5, 303, 196.5, 303.5, 200.5);
            p.lineTo(301.5, 191.5);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor = "transparent";
            sfp.strokeColor = "#000000";
            sfp.strokeWidth = 1;
            p.moveTo(-89.25, 169);
            p.lineTo(-67.25, 173.75);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor = "transparent";
            sfp.strokeColor = "#000000";
            sfp.strokeWidth = 1;
            p.moveTo(-39, 331);
            p.cubicTo(-39, 331, -39.5, 327.5, -48.5, 338);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor = "transparent";
            sfp.strokeColor = "#000000";
            sfp.strokeWidth = 1;
            p.moveTo(-33.5, 336);
            p.cubicTo(-33.5, 336, -31.5, 329.5, -38, 334);
            jsApiItem.appendVisualPath(p, sfp);

            p.clear(); sfp.clear();
            sfp.fillColor = "transparent";
            sfp.strokeColor = "#000000";
            sfp.strokeWidth = 1;
            p.moveTo(20.5, 344.5);
            p.cubicTo(20.5, 344.5, 22, 333.5, 10.5, 346.5);
            jsApiItem.appendVisualPath(p, sfp);

            jsApiItem.commitVisualPaths();
        }
    }
}
