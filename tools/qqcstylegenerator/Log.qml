// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Qt.labs.qmlmodels
import QtCore

GroupBox {
    implicitWidth: parent.width
    implicitHeight: 200
    title: "Log"

    Connections {
        target: window
        function onGeneratingChanged() {
            if (window.generating) {
                outputModel.clear()
                filteredModel.clear()
            }
        }
    }

    ListModel {
        id: outputModel
    }

    ListModel {
        id: filteredModel
    }

    Connections {
        target: bridge

        function onDebug(msg) {
            output(msg)
        }

        function onWarning(msg) {
            outputFilter.text = "(warning|error)"
            updateOutputFilter();
            output("Warning: " + msg)
        }

        function onError(msg) {
            outputFilter.text = "(warning|error)"
            updateOutputFilter();
            output("Error: " + msg)
        }

        function onProgressLabelChanged(label) {
            output(label)
        }

        function onFinished() {
            output(window.stopping ? "Stopped!" : "Finished!")
        }

        function onFigmaFileNameChanged(name) {
            output("Figma name: " + name)
        }
    }

    function output(msg)
    {
        outputModel.append({"msg": msg})
        const regex = new RegExp(outputFilter.text, "i");
        if (regex.test(msg))
            filteredModel.append({"msg" : msg})
    }

    function updateOutputFilter() {
        filteredModel.clear()
        const regex = new RegExp(outputFilter.text, "i");
        let rows = outputModel.rowCount()
        for (let row = 0; row < rows; ++row) {
            let index = outputModel.index(row, 0)
            let msg = outputModel.data(index)
            if (regex.test(msg))
                filteredModel.append({"msg" : msg})
        }
    }

    ColumnLayout {
        anchors.fill: parent
        ScrollView {
            id: outputScrollView
            Layout.preferredWidth: parent.width
            Layout.fillHeight: true

            property bool sticky: true

            TableView {
                id: outputView
                clip: true
                animate: false
                model: filteredModel

                delegate: Label {
                    text: msg
                    onImplicitWidthChanged: {
                        if (implicitWidth > outputView.contentWidth)
                            outputView.contentWidth = implicitWidth
                    }
                }

                onRowsChanged: {
                    if (outputScrollView.sticky)
                        positionViewAtRow(rows - 1, TableView.AlignBottom)
                }
            }

            Connections {
                target: outputScrollView.ScrollBar.vertical
                function onPressedChanged() {
                    outputScrollView.sticky = target.pressed ? false : target.position > 0.9
                }
            }
        }

        TextField {
            id: outputFilter
            placeholderText: "Filter (regexp)"
            Layout.preferredWidth: parent.width
            onAccepted: updateOutputFilter()
        }
    }
}

