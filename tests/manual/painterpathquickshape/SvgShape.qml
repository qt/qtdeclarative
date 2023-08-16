// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Shapes
import QtQuick.Controls
import QtQuick.Dialogs
import io.qt

Item {
    id: topLevel
    property var boundingRect: Qt.rect(0, 0, 100, 100)

    Text {
        id: fileNameLabel
        x: boundingRect.x
        y: boundingRect.bottom * controlPanel.scale  + font.pixelSize*2
        text: "Filename: " + pathLoader.source
    }

    ToolButton {
        anchors.left: fileNameLabel.right
        anchors.top: fileNameLabel.top
        anchors.leftMargin: 10
        text: "..."

        onClicked: {
            fileDialog.visible = true
        }
    }

    property var children: []
    SvgPathLoader {
        id: pathLoader
        function reload()
        {
            for (var j = 0; j < children.length; ++j) {
                children[j].destroy()
            }
            children = []

            let first = true
            let pickShape = controlPanel.subShape
            let pickOne = pickShape >= 0 && !controlPanel.subShapeGreaterThan
            let pickGreater = pickShape >= 0 && controlPanel.subShapeGreaterThan
            if (pickOne)
                console.log("Creating SVG item", pickShape, "out of", pathLoader.paths.length)
            else if (pickGreater)
                console.debug("Creating " + (pathLoader.paths.length - pickShape) + " SVG items")
            else
                console.debug("Creating " + pathLoader.paths.length + " SVG items")
            for (var i = 0; i < pathLoader.paths.length; ++i) {
                if ((pickOne && pickShape !== i ) || (pickGreater && i < pickShape))
                    continue
                var s = pathLoader.paths[i]
                var fillColor = pathLoader.fillColors[i]
                let strokeText = "";
                let strokeColor = pathLoader.strokeColors[i]
                let strokeWidth = pathLoader.strokeWidths[i]
                let transform = pathLoader.transforms[i]
                if (strokeColor) {
                    if (!strokeWidth)
                        strokeWidth = "1.0" // default value defined by SVG standard
                    strokeText = "strokeColor: \"" + strokeColor + "\"; strokeWidth: " + strokeWidth + ";"
                }

                let fillText = "";
                if (fillColor)
                    fillText = "fillColor: \"" + fillColor + "\";\n";

                var obj = Qt.createQmlObject("import QtQuick\nimport QtQuick.Shapes\n ControlledShape { \n"
                                             + fillText
                                             + "shapeTransform: Matrix4x4 { matrix: Qt.matrix4x4(" + transform + "); }\n"
                                             + strokeText + "\n"
                                             + "fillRule: ShapePath.WindingFill; delegate: [ PathSvg { path: \"" + s + "\";  } ] }\n",
                                             topLevel, "SvgPathComponent_" + i)


                children.push(obj)
                if (first) {
                    topLevel.boundingRect = obj.boundingRect
                    first = false
                } else {
                    var minX = Math.min(topLevel.boundingRect.x, obj.boundingRect.x)
                    var minY = Math.min(topLevel.boundingRect.y, obj.boundingRect.y)
                    var maxX = Math.max(topLevel.boundingRect.x + topLevel.boundingRect.width,
                                        obj.boundingRect.x + obj.boundingRect.width)
                    var maxY = Math.max(topLevel.boundingRect.y + topLevel.boundingRect.height,
                                        obj.boundingRect.y + obj.boundingRect.height)

                    topLevel.boundingRect = Qt.rect(minX, minY, maxX - minX, maxY - minY)
                }
            }
            console.debug("Finished SVG")
        }
        onPathsChanged: reload()

        Component.onCompleted: {
            pathLoader.source = "qrc:/1535737773.svg"
        }
    }

    Connections {
        target: controlPanel
        function onSubShapeChanged() { pathLoader.reload() }
    }

    FileDialog {
        id: fileDialog
        title: "Please choose a file"
        onAccepted: {
            pathLoader.source = fileDialog.selectedFile
            fileDialog.visible = false
        }
    }
}
