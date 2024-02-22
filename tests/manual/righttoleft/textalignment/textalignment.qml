// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick

pragma ComponentBehavior: Bound

Rectangle {
    id: root
    color: "white"
    width: 320
    height: 480

    property bool mirror: false
    property int pxSz: 18
    property variant horizontalAlignment: undefined

    readonly property variant editorType: [qsTr("Plain Text"), qsTr("Styled Text"), qsTr("Plain Rich Text"), qsTr("Italic Rich Text"), qsTr("Plain TextEdit"), qsTr("Italic TextEdit"), qsTr("TextInput")]
    readonly property variant text: ["", " ", "Hello world!", "مرحبا العالم!", "Hello world! Hello!\nHello world! Hello!", "مرحبا العالم! مرحبا! مرحبا العالم! مرحبا!" ,"مرحبا العالم! مرحبا! مرحبا Hello world!\nالعالم! مرحبا!"]
    readonly property variant description: [qsTr("empty text"), qsTr("white-space-only text"), qsTr("left-to-right text"), qsTr("right-to-left text"), qsTr("multi-line left-to-right text"), qsTr("multi-line right-to-left text"), qsTr("multi-line bidi text")]
    readonly property variant textComponents: [plainTextComponent, styledTextComponent, richTextComponent, italicRichTextComponent, plainTextEdit, italicTextEdit, textInput]

    function shortText(horizontalAlignment) {

        // all the different QML editors have
        // the same alignment values
        switch (horizontalAlignment) {
        case Text.AlignLeft:
            return qsTr("L");
        case Text.AlignRight:
            return qsTr("R");
        case Text.AlignHCenter:
            return qsTr("C");
        case Text.AlignJustify:
            return qsTr("J");
        default:
            return qsTr("Error");
        }
    }

    ListView {
        anchors {
            top: parent.top
            left: parent.left
            right: parent.right
            bottom: buttons.top
            topMargin: 5
        }
        model: root.editorType.length
        orientation: ListView.Horizontal
        cacheBuffer: 1000 //Load the really expensive ones async if possible
        delegate: Item {
            id: delegate

            required property int index

            width: ListView.view.width
            height: ListView.view.height

            Column {
                anchors.horizontalCenter: parent.horizontalCenter
                spacing: 5
                width: textColumn.width + 10
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
        id: buttons
        spacing: 2
        width: parent.width
        anchors.bottom: parent.bottom
        Rectangle {
            // button
            height: 50
            width: parent.width
            color: mouseArea.pressed ? "black" : "lightgray"
            Column {
                anchors.centerIn: parent
                Text {
                    text: root.mirror ? qsTr("Mirrored") : qsTr("Not mirrored")
                    color: "white"
                    font.pixelSize: 16
                    anchors.horizontalCenter: parent.horizontalCenter
                }
                Text {
                    text: qsTr("(click here to toggle)")
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
            height: 50
            width: parent.width
            color: mouseArea2.pressed ? "black" : "gray"
            Column {
                anchors.centerIn: parent
                Text {
                    anchors.horizontalCenter: parent.horizontalCenter
                    color: "white"
                    font.pixelSize: 16
                    text: {
                        if (root.horizontalAlignment == undefined)
                            return qsTr("Implict alignment");
                        switch (root.horizontalAlignment) {
                        case Text.AlignLeft:
                            return qsTr("Left alignment");
                        case Text.AlignRight:
                            return qsTr("Right alignment");
                        case Text.AlignHCenter:
                            return qsTr("Center alignment");
                        case Text.AlignJustify:
                            return qsTr("Justify alignment");
                        }
                    }
                }
                Text {
                    text: qsTr("(click here to toggle)")
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
                    root.horizontalAlignment = index < 0 ? undefined : Math.pow(2, index)
                    index = (index + 2) % 5 - 1;
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
                    anchors {
                        fill: parent
                        margins: -3
                    }
                }
            }
            Text {
                color: "white"
                text: root.shortText(parent.horizontalAlignment)
                anchors {
                    top: parent.top
                    right: parent.right
                    margins: 2
                }
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
                    anchors {
                        fill: parent
                        margins: -3
                    }
                }
            }
            Text {
                color: "white"
                text: root.shortText(parent.horizontalAlignment)
                anchors {
                    top: parent.top
                    right: parent.right
                    margins: 2
                }
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
                    anchors {
                        fill: parent
                        margins: -3
                    }
                }
            }
            Text {
                color: "white"
                text: root.shortText(parent.horizontalAlignment)
                anchors {
                    top: parent.top
                    right: parent.right
                    margins: 2
                }
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
                    anchors {
                        fill: parent
                        margins: -3
                    }
                }
            }
            Text {
                color: "white"
                text: root.shortText(parent.horizontalAlignment)
                anchors {
                    top: parent.top
                    right: parent.right
                    margins: 2
                }
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
                    anchors {
                        fill: parent
                        margins: -3
                    }
                }
            }
            Text {
                color: "white"
                text: root.shortText(parent.horizontalAlignment)
                anchors {
                    top: parent.top
                    right: parent.right
                    margins: 2
                }
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
                    anchors {
                        fill: parent
                        margins: -3
                    }
                }
            }
            Text {
                color: "white"
                text: root.shortText(parent.horizontalAlignment)
                anchors {
                    top: parent.top
                    right: parent.right
                    margins: 2
                }
            }
        }
    }

    Component {
        id: textInput
        TextInput {
            id: textDelegate

            required property int index

            width: 180
            text: root.text[textDelegate.index]
            font.pixelSize: root.pxSz
            cursorVisible: true
            wrapMode: Text.Wrap
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
                    anchors {
                        fill: parent
                        margins: -3
                    }
                }
            }
            Text {
                color: "white"
                text: root.shortText(parent.horizontalAlignment)
                anchors {
                    top: parent.top
                    right: parent.right
                    margins: 2
                }
            }
        }
    }
}

