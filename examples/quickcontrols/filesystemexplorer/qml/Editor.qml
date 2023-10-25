// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import FileSystemModule

pragma ComponentBehavior: Bound

// This is the text editor that displays the currently open file, including
// their corresponding line numbers.
Rectangle {
    id: root

    required property string currentFilePath
    required property bool showLineNumbers
    property alias text: textArea
    property int currentLineNumber: -1
    property int rowHeight: Math.ceil(fontMetrics.lineSpacing)

    color: Colors.background

    onWidthChanged: textArea.update()
    onHeightChanged: textArea.update()

    RowLayout {
        anchors.fill: parent
        // We use a flickable to synchronize the position of the editor and
        // the line numbers. This is necessary because the line numbers can
        // extend the available height.
        Flickable {
            id: lineNumbers

            // Calculate the width based on the logarithmic scale.
            Layout.preferredWidth: fontMetrics.averageCharacterWidth
                * (Math.floor(Math.log10(textArea.lineCount)) + 1) + 10
            Layout.fillHeight: true

            interactive: false
            contentY: editorFlickable.contentY
            visible: textArea.text !== "" && root.showLineNumbers

            Column {
                anchors.fill: parent
                Repeater {
                    id: repeatedLineNumbers

                    model: LineNumberModel {
                        lineCount: textArea.text !== "" ? textArea.lineCount : 0
                    }

                    delegate: Item {
                        required property int index

                        width: parent.width
                        height: root.rowHeight
                        Label {
                            id: numbers

                            text: parent.index + 1

                            width: parent.width
                            height: parent.height
                            horizontalAlignment: Text.AlignLeft
                            verticalAlignment: Text.AlignVCenter

                            color: (root.currentLineNumber === parent.index)
                                    ? Colors.iconIndicator : Qt.darker(Colors.text, 2)
                            font: textArea.font
                        }
                        Rectangle {
                            id: indicator

                            anchors.left: numbers.right
                            width: 1
                            height: parent.height
                            color: Qt.darker(Colors.text, 3)
                        }
                    }
                }
            }
        }

        Flickable {
            id: editorFlickable

            property alias textArea: textArea

            // We use an inline component to customize the horizontal and vertical
            // scroll-bars. This is convenient when the component is only used in one file.
            component MyScrollBar: ScrollBar {
                id: scrollBar
                background: Rectangle {
                    implicitWidth: scrollBar.interactive ? 8 : 4
                    implicitHeight: scrollBar.interactive ? 8 : 4

                    opacity: scrollBar.active && scrollBar.size < 1.0 ? 1.0 : 0.0
                    color: Colors.background
                    Behavior on opacity {
                        OpacityAnimator {
                            duration: 500
                        }
                    }
                }
                contentItem: Rectangle {
                    implicitWidth: scrollBar.interactive ? 8 : 4
                    implicitHeight: scrollBar.interactive ? 8 : 4
                    opacity: scrollBar.active && scrollBar.size < 1.0 ? 1.0 : 0.0
                    color: Colors.color1
                    Behavior on opacity {
                        OpacityAnimator {
                            duration: 1000
                        }
                    }
                }
            }

            Layout.fillHeight: true
            Layout.fillWidth: true
            ScrollBar.horizontal: MyScrollBar {}
            ScrollBar.vertical: MyScrollBar {}

            boundsBehavior: Flickable.StopAtBounds

            TextArea.flickable: TextArea {
                id: textArea
                anchors.fill: parent

                focus: false
                topPadding: 0
                leftPadding: 10

                text: FileSystemModel.readFile(root.currentFilePath)
                tabStopDistance: fontMetrics.averageCharacterWidth * 4

                // Grab the current line number from the C++ interface.
                onCursorPositionChanged: {
                    root.currentLineNumber = FileSystemModel.currentLineNumber(
                        textArea.textDocument, textArea.cursorPosition)
                }

                color: Colors.textFile
                selectedTextColor: Colors.textFile
                selectionColor: Colors.selection

                textFormat: TextEdit.PlainText
                renderType: Text.QtRendering
                selectByMouse: true
                antialiasing: true
                background: null
            }

            FontMetrics {
                id: fontMetrics
                font: textArea.font
            }
        }
    }
}
