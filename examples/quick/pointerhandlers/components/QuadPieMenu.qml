/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
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

import QtQuick
import QtQuick.Shapes

TapHandler {
    property var labels: [ "upperLeft", "upperRight", "lowerRight", "lowerLeft" ]
    signal triggered(string text)

    id: menuTap
    gesturePolicy: TapHandler.DragWithinBounds
    onPressedChanged: if (pressed) {
        impl.x = point.position.x - impl.width / 2
        impl.y = point.position.y - impl.width / 2
    } else {
        if (impl.highlightedShape)
            menuTap.triggered(impl.highlightedShape.text)
    }

    property Item impl: Item {
        parent: menuTap.parent
        width: 100
        height: 100
        scale: Math.min(1, Math.max(0, menuTap.timeHeld * 4))
        opacity: scale * 2
        visible: menuTap.pressed
        property Shape highlightedShape: null

        component PieSegment : Shape {
            id: shape
            property int orientation: Qt.TopRightCorner
            property alias text: text.text

            width: 100
            height: 100
            containsMode: Shape.FillContains

            property bool highlighted: menuTap.pressed &&
                    shape.contains(shape.mapFromItem(menuTap.parent, menuTap.point.position))
            onHighlightedChanged: {
                if (highlighted)
                    impl.highlightedShape = shape
                else if (impl.highlightedShape === shape)
                    impl.highlightedShape = null
            }

            ShapePath {
                fillColor: highlighted ? "darkturquoise" : "aliceblue"
                PathSvg {
                    id: svgPath
                    path: switch (orientation) {
                        case Qt.TopRightCorner:
                            return "M75,50 l 25,0 a50,50 0 0,0 -50,-50 l 0,25 a25,25 0 0,1 25,25";
                        case Qt.BottomRightCorner:
                            return "M75,50 l 25,0 a50,50 0 0,1 -50,50 l 0,-25 a25,25 0 0,0 25,-25";
                        case Qt.TopLeftCorner:
                            return "M50,25 l 0,-25 a50,50 0 0,0 -50,50 l 25,0 a25,25 0 0,1 25,-25";
                        case Qt.BottomLeftCorner:
                            return "M50,75 l 0,25 a50,50 0 0,1 -50,-50 l 25,0 a25,25 0 0,0 25,25";
                    }
                }
            }
            Text {
                id: text
                anchors {
                    centerIn: parent
                    horizontalCenterOffset: switch (orientation) {
                        case Qt.TopRightCorner:
                        case Qt.BottomRightCorner:
                            return 25;
                        default:
                            return -25;
                    }
                    verticalCenterOffset: switch (orientation) {
                        case Qt.BottomLeftCorner:
                        case Qt.BottomRightCorner:
                            return 25;
                        default:
                            return -25;
                    }
                }
                horizontalAlignment: Text.AlignHCenter
                rotation: switch (orientation) {
                    case Qt.TopRightCorner:
                    case Qt.BottomLeftCorner:
                        return 45;
                    default:
                        return -45;
                }
            }
        }

        PieSegment {
            orientation: Qt.TopLeftCorner
            text: labels[0]
        }
        PieSegment {
            orientation: Qt.TopRightCorner
            text: labels[1]
        }
        PieSegment {
            orientation: Qt.BottomRightCorner
            text: labels[2]
        }
        PieSegment {
            orientation: Qt.BottomLeftCorner
            text: labels[3]
        }
    }
}
