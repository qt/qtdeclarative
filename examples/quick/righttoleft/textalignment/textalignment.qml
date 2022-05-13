// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick

Rectangle {
    id: root
    color: "white"
    //width: containerColumn.width
    //height: containerColumn.height + containerColumn.anchors.topMargin
    width: 320
    height: 480

    property bool mirror: false
    property int pxSz: 18
    property variant horizontalAlignment: undefined

    property variant editorType: ["Plain Text", "Styled Text", "Plain Rich Text", "Italic Rich Text", "Plain TextEdit", "Italic TextEdit", "TextInput"]
    property variant text: ["", " ", "Hello world!", "مرحبا العالم!", "Hello world! Hello!\nHello world! Hello!", "مرحبا العالم! مرحبا! مرحبا العالم! مرحبا!" ,"مرحبا العالم! مرحبا! مرحبا Hello world!\nالعالم! مرحبا!"]
    property variant description: ["empty text", "white-space-only text", "left-to-right text", "right-to-left text", "multi-line left-to-right text", "multi-line right-to-left text", "multi-line bidi text"]
    property variant textComponents: [plainTextComponent, styledTextComponent, richTextComponent, italicRichTextComponent, plainTextEdit, italicTextEdit, textInput]

    function shortText(horizontalAlignment) {

        // all the different QML editors have
        // the same alignment values
        switch (horizontalAlignment) {
        case Text.AlignLeft:
            return "L";
        case Text.AlignRight:
            return "R";
        case Text.AlignHCenter:
            return "C";
        case Text.AlignJustify:
            return "J";
        default:
            return "Error";
        }
    }
    Column {
        id: containerColumn
        spacing: 10
        width: editorTypeRow.width
        anchors { top: parent.top; topMargin: 5 }
        ListView {
            width: 320
            height: 320
            id: editorTypeRow
            model: root.editorType.length
            orientation: ListView.Horizontal
            cacheBuffer: 1000//Load the really expensive ones async if possible
            delegate: Item {
                id: delegate

                width: editorColumn.width
                height: editorColumn.height

                required property int index

                Column {
                    id: editorColumn
                    spacing: 5
                    width: textColumn.width+10
                    Text {
                        text: root.editorType[delegate.index]
                        font.pixelSize: 16
                        anchors.horizontalCenter: parent.horizontalCenter
                    }
                    Column {
                        id: textColumn
                        spacing: 5
                        anchors.horizontalCenter: parent.horizontalCenter
                        Repeater {
                            model: root.textComponents.length
                            delegate: root.textComponents[delegate.index]
                        }
                    }
                }
            }
        }
        Column {
            spacing: 2
            width: parent.width
            Rectangle {
                // button
                height: 50; width: parent.width
                color: mouseArea.pressed ? "black" : "lightgray"
                Column {
                    anchors.centerIn: parent
                    Text {
                        text: root.mirror ? "Mirrored" : "Not mirrored"
                        color: "white"
                        font.pixelSize: 16
                        anchors.horizontalCenter: parent.horizontalCenter
                    }
                    Text {
                        text: "(click here to toggle)"
                        color: "white"
                        font.pixelSize: 10
                        font.italic: true
                        anchors.horizontalCenter: parent.horizontalCenter
                    }
                }
                MouseArea {
                    id: mouseArea
                    property int index: 0
                    anchors.fill: parent
                    onClicked: root.mirror = !root.mirror
                }
            }
            Rectangle {
                // button
                height: 50; width: parent.width
                color: mouseArea2.pressed ? "black" : "gray"
                Column {
                    anchors.centerIn: parent
                    Text {
                        text: {
                            if (root.horizontalAlignment == undefined)
                                return "Implict alignment";
                            switch (root.horizontalAlignment) {
                            case Text.AlignLeft:
                                return "Left alignment";
                            case Text.AlignRight:
                                return "Right alignment";
                            case Text.AlignHCenter:
                                return "Center alignment";
                            case Text.AlignJustify:
                                return "Justify alignment";
                            }
                        }
                        color: "white"
                        font.pixelSize: 16
                        anchors.horizontalCenter: parent.horizontalCenter
                    }
                    Text {
                        text: "(click here to toggle)"
                        color: "white"
                        font.pixelSize: 10
                        font.italic: true
                        anchors.horizontalCenter: parent.horizontalCenter
                    }
                }
                MouseArea {
                    id: mouseArea2
                    property int index: 0
                    anchors.fill: parent
                    onClicked: {
                        if (index < 0) {
                            root.horizontalAlignment = undefined;
                        } else {
                            root.horizontalAlignment = Math.pow(2, index);
                        }
                        index = (index + 2) % 5 - 1;
                    }
                }
            }
        }
    }

    Component {
        id: plainTextComponent
        Text {
            required property int index

            width: 180
            text: root.text[index]
            font.pixelSize: root.pxSz
            wrapMode: Text.WordWrap
            horizontalAlignment: root.horizontalAlignment
            LayoutMirroring.enabled: root.mirror
            textFormat: Text.RichText
            Rectangle {
                z: -1
                color: Qt.rgba(0.8, 0.2, 0.2, 0.3)
                anchors.fill: parent
            }
            Text {
                text: root.description[parent.index]
                color: Qt.rgba(1,1,1,1.0)
                anchors.centerIn: parent
                font.pixelSize: root.pxSz - 2
                Rectangle {
                    z: -1
                    color: Qt.rgba(0.3, 0, 0, 0.3)
                    anchors { fill: parent; margins: -3 }
                }
            }
            Text {
                color: "white"
                text: root.shortText(parent.horizontalAlignment)
                anchors { top: parent.top; right: parent.right; margins: 2 }
            }
        }
    }

    Component {
        id: styledTextComponent
        Text {
            required property int index

            width: 180
            text: root.text[index]
            font.pixelSize: root.pxSz
            wrapMode: Text.WordWrap
            horizontalAlignment: root.horizontalAlignment
            LayoutMirroring.enabled: root.mirror
            textFormat: Text.RichText
            style: Text.Sunken
            styleColor: "white"
            Rectangle {
                z: -1
                color: Qt.rgba(0.8, 0.2, 0.2, 0.3)
                anchors.fill: parent
            }
            Text {
                text: root.description[parent.index]
                color: Qt.rgba(1,1,1,1.0)
                anchors.centerIn: parent
                font.pixelSize: root.pxSz - 2
                Rectangle {
                    z: -1
                    color: Qt.rgba(0.3, 0, 0, 0.3)
                    anchors { fill: parent; margins: -3 }
                }
            }
            Text {
                color: "white"
                text: root.shortText(parent.horizontalAlignment)
                anchors { top: parent.top; right: parent.right; margins: 2 }
            }
        }
    }

    Component {
        id: richTextComponent
        Text {
            required property int index

            width: 180
            text: root.text[index]
            font.pixelSize: root.pxSz
            wrapMode: Text.WordWrap
            horizontalAlignment: root.horizontalAlignment
            LayoutMirroring.enabled: root.mirror
            textFormat: Text.RichText
            Rectangle {
                z: -1
                color: Qt.rgba(0.8, 0.2, 0.2, 0.3)
                anchors.fill: parent
            }
            Text {
                text: root.description[parent.index]
                color: Qt.rgba(1,1,1,1.0)
                anchors.centerIn: parent
                font.pixelSize: root.pxSz - 2
                Rectangle {
                    z: -1
                    color: Qt.rgba(0.3, 0, 0, 0.3)
                    anchors { fill: parent; margins: -3 }
                }
            }
            Text {
                color: "white"
                text: root.shortText(parent.horizontalAlignment)
                anchors { top: parent.top; right: parent.right; margins: 2 }
            }
        }
    }

    Component {
        id: italicRichTextComponent
        Text {
            required property int index

            width: 180
            text: "<i>" + root.text[index] + "</i>"
            font.pixelSize: root.pxSz
            wrapMode: Text.WordWrap
            horizontalAlignment: root.horizontalAlignment
            LayoutMirroring.enabled: root.mirror
            textFormat: Text.RichText
            property variant backgroundColor: Qt.rgba(0.8, 0.2, 0.2, 0.3)
            Rectangle {
                z: -1
                color: parent.backgroundColor
                anchors.fill: parent
            }
            Text {
                text: root.description[parent.index]
                color: Qt.rgba(1,1,1,1.0)
                anchors.centerIn: parent
                font.pixelSize: root.pxSz - 2
                Rectangle {
                    z: -1
                    color: Qt.rgba(0.3, 0, 0, 0.3)
                    anchors { fill: parent; margins: -3 }
                }
            }
            Text {
                color: "white"
                text: root.shortText(parent.horizontalAlignment)
                anchors { top: parent.top; right: parent.right; margins: 2 }
            }
        }
    }

    Component {
        id: plainTextEdit
        TextEdit {
            required property int index

            width: 180
            text: root.text[index]
            font.pixelSize: root.pxSz
            cursorVisible: true
            wrapMode: TextEdit.WordWrap
            horizontalAlignment: root.horizontalAlignment
            LayoutMirroring.enabled: root.mirror
            Rectangle {
                z: -1
                color: Qt.rgba(0.5, 0.5, 0.2, 0.3)
                anchors.fill: parent
            }
            Text {
                text: root.description[parent.index]
                color: Qt.rgba(1,1,1,1.0)
                anchors.centerIn: parent
                font.pixelSize: root.pxSz - 2
                Rectangle {
                    z: -1
                    color: Qt.rgba(0.3, 0, 0, 0.3)
                    anchors { fill: parent; margins: -3 }
                }
            }
            Text {
                color: "white"
                text: root.shortText(parent.horizontalAlignment)
                anchors { top: parent.top; right: parent.right; margins: 2 }
            }
        }
    }

    Component {
        id: italicTextEdit
        TextEdit {
            required property int index

            width: 180
            text: "<i>" + root.text[index] + "<i>"
            font.pixelSize: root.pxSz
            cursorVisible: true
            wrapMode: TextEdit.WordWrap
            textFormat: TextEdit.RichText
            horizontalAlignment: root.horizontalAlignment
            LayoutMirroring.enabled: root.mirror
            Rectangle {
                z: -1
                color: Qt.rgba(0.5, 0.5, 0.2, 0.3)
                anchors.fill: parent
            }
            Text {
                text: root.description[parent.index]
                color: Qt.rgba(1,1,1,1.0)
                anchors.centerIn: parent
                font.pixelSize: root.pxSz - 2
                Rectangle {
                    z: -1
                    color: Qt.rgba(0.3, 0, 0, 0.3)
                    anchors { fill: parent; margins: -3 }
                }
            }
            Text {
                color: "white"
                text: root.shortText(parent.horizontalAlignment)
                anchors { top: parent.top; right: parent.right; margins: 2 }
            }
        }
    }

    Component {
        id: textInput
        Item {
            id: textDelegate
            required property int index
            width: 180
            height: textInput.text.length > 20 ? 3*textInput.height : textInput.height
            TextInput {
                id: textInput
                width: 180
                text: root.text[textDelegate.index]
                font.pixelSize: root.pxSz
                cursorVisible: true
                horizontalAlignment: root.horizontalAlignment
                LayoutMirroring.enabled: root.mirror
                Rectangle {
                    z: -1
                    color: Qt.rgba(0.6, 0.4, 0.2, 0.3)
                    anchors.fill: parent
                }
                Text {
                    text: root.description[textDelegate.index]
                    color: Qt.rgba(1,1,1,1.0)
                    anchors.centerIn: parent
                    font.pixelSize: root.pxSz - 2
                    Rectangle {
                        z: -1
                        color: Qt.rgba(0.3, 0, 0, 0.3)
                        anchors { fill: parent; margins: -3 }
                    }
                }
                Text {
                    color: "white"
                    text: root.shortText(parent.horizontalAlignment)
                    anchors { top: parent.top; right: parent.right; margins: 2 }
                }
            }
        }
    }
}

