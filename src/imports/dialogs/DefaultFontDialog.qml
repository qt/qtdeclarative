/*****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtQuick.Dialogs module of Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
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
**   * Neither the name of Digia Plc and its Subsidiary(-ies) nor the names
**     of its contributors may be used to endorse or promote products derived
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
*****************************************************************************/

import QtQuick 2.1
import QtQuick.Window 2.1
import QtQuick.Dialogs 1.1
import QtQuick.Dialogs.Private 1.1
import "qml"

AbstractFontDialog {
    id: root

    property alias font: content.externalFont

    Rectangle {
        id: content
        implicitWidth: Math.min(Screen.desktopAvailableWidth, implicitHeight * 1.2)
        implicitHeight: Math.min(Screen.desktopAvailableHeight, settingsBottom.implicitHeight * 3)
        color: palette.window
        focus: root.visible
        property real spacing: 6
        property real outerSpacing: 12
        property real listMargins: 4
        property real delegateHeightMultiplier: 1.5
        property real extraWidth: width > 400 ? width - 400 : 0
        property real extraHeight: height > initialHeight ? height - initialHeight : 0
        property real initialHeight: -1
        onHeightChanged: if (visible && initialHeight < 0) initialHeight = height

        property color borderColor: Qt.darker(palette.button, 1.5)

        property font font: Qt.font({ family: "Helvetica", pointSize: 24, weight: Font.Normal })
        property font externalFont
        property string writingSystem
        property string writingSystemSample
        property var pointSizes

        onFontChanged: externalFont = font

        onExternalFontChanged: {
            if (content.font != content.externalFont) {
                font = externalFont
                wsListView.reset()
                fontListView.reset()
                weightListView.reset()
            }
        }

        Keys.onPressed: {
            event.accepted = true
            switch (event.key) {
            case Qt.Key_Return:
            case Qt.Key_Select:
                root.font = content.font
                root.accept()
                break
            case Qt.Key_Escape:
            case Qt.Key_Back:
                reject()
                break
            default:
                // do nothing
                event.accepted = false
                break
            }
        }

        SystemPalette { id: palette }

        Column {
            id: contentColumn
            anchors.fill: parent
            anchors.margins: content.outerSpacing
            spacing: content.outerSpacing

            Grid {
                id: settingsTop
                columns: 3
                spacing: content.spacing
                width: parent.width
                height: parent.height - buttonRow.height - settingsBottom.height - parent.spacing * 2
                property real columnHeight: height - writingSystemLabel.height - spacing

                Text { id: writingSystemLabel; text: qsTr("Writing System"); font.bold: true }
                Text { id: fontNameLabel; text: qsTr("Font"); font.bold: true }
                Text { id: sizeLabel; text: qsTr("Size"); font.bold: true }
                Rectangle {
                    id: wsColumn
                    radius: 3
                    color: palette.window
                    border.color: content.borderColor
                    implicitWidth: Math.max(writingSystemLabel.implicitWidth, 100) + content.extraWidth / 5
                    height: parent.columnHeight
                    clip: true
                    ListView {
                        id: wsListView
                        anchors.fill: parent
                        anchors.margins: content.listMargins
                        anchors.topMargin: 2
                        highlightMoveDuration: 0
                        onHeightChanged: positionViewAtIndex(currentIndex, ListView.Contain)
                        function reset() {
                            if (wsModel.count > 0) {
                                content.writingSystem = wsModel.get(0).name;
                                fontModel.writingSystem = content.writingSystem;
                                content.writingSystemSample = wsModel.get(0).sample;
                            }
                        }

                        model: WritingSystemListModel {
                            id: wsModel
                            Component.onCompleted: wsListView.reset()
                        }
                        highlight: Rectangle {
                            color: palette.highlight
                            x: 2 - wsListView.anchors.margins
                            width: wsListView.parent.width - 4
                        }
                        delegate: Item {
                            width: parent.width
                            height: wsText.height * content.delegateHeightMultiplier
                            Text {
                                id: wsText
                                text: name
                                width: parent.width
                                elide: Text.ElideRight
                                color: index === wsListView.currentIndex ? palette.highlightedText : palette.windowText
                                anchors.verticalCenter: parent.verticalCenter
                            }
                            MouseArea {
                                anchors.fill: parent
                                onClicked: {
                                    wsListView.currentIndex = index;
                                    content.writingSystem = wsModel.get(wsListView.currentIndex).name;
                                    fontModel.writingSystem = content.writingSystem;
                                    content.writingSystemSample = wsModel.get(wsListView.currentIndex).sample;
                                }
                            }
                        }
                    }
                }
                Rectangle {
                    radius: 3
                    color: palette.window
                    border.color: content.borderColor
                    implicitWidth: Math.max(fontNameLabel.implicitWidth, parent.width - wsColumn.implicitWidth - pointSizesColumn.implicitWidth - parent.spacing * 2)
                    height: parent.columnHeight
                    clip: true
                    ListView {
                        id: fontListView
                        anchors.fill: parent
                        anchors.margins: content.listMargins
                        anchors.topMargin: 2
                        highlightMoveDuration: 0
                        onHeightChanged: positionViewAtIndex(currentIndex, ListView.Contain)
                        function reset() {
                            fontModel.findIndex()
                            content.pointSizes = fontModel.pointSizes()
                            fontModel.findPointSizesIndex()
                        }

                        model: FontListModel {
                            id: fontModel
                            scalableFonts: root.scalableFonts
                            nonScalableFonts: root.nonScalableFonts
                            monospacedFonts: root.monospacedFonts
                            proportionalFonts: root.proportionalFonts
                            Component.onCompleted: fontListView.reset()
                            onModelReset: { findIndex(); }
                            function findIndex() {
                                if (fontModel.count <= 0)
                                    return

                                if (content.font.family == "") {
                                    content.font.family = fontModel.get(0).family
                                    fontListView.currentIndex = 0
                                } else {
                                    var find = false
                                    for (var i = 0; i < fontModel.count; ++i) {
                                        if (content.font.family == fontModel.get(i).family) {
                                            find = true
                                            fontListView.currentIndex = i
                                            break
                                        }
                                    }
                                    if (find == false) {
                                        content.font.family = fontModel.get(0).family
                                        fontListView.currentIndex = 0
                                    }
                                }
                            }
                            function findPointSizesIndex() {
                                if (content.pointSizes.length <= 0)
                                    return

                                var find = false
                                for (var i = 0; i < content.pointSizes.length; ++i) {
                                    if (content.font.pointSize == content.pointSizes[i]) {
                                        find = true
                                        pointSizesListView.currentIndex = i
                                        break
                                    }
                                }
                                if (find == false) {
                                    content.font.pointSize = content.pointSizes[0]
                                    pointSizesListView.currentIndex = 0
                                }
                            }
                        }
                        highlight: Rectangle {
                            color: palette.highlight
                            x: 2 - fontListView.anchors.margins
                            width: fontListView.parent.width - 4
                        }
                        delegate: Item {
                            width: parent.width
                            height: fontText.height * content.delegateHeightMultiplier
                            Text {
                                id: fontText
                                text: family
                                width: parent.width
                                elide: Text.ElideRight
                                color: index === fontListView.currentIndex ? palette.highlightedText : palette.windowText
                                anchors.verticalCenter: parent.verticalCenter
                            }
                            MouseArea {
                                anchors.fill: parent
                                onClicked: {
                                    fontListView.currentIndex = index
                                    content.font.family = fontModel.get(fontListView.currentIndex).family
                                }
                            }
                        }
                    }
                }
                Rectangle {
                    id: pointSizesColumn
                    radius: 3
                    color: palette.window
                    border.color: content.borderColor
                    implicitWidth:sizeLabel.implicitWidth * 2
                    height: parent.columnHeight
                    clip: true
                    ListView {
                        id: pointSizesListView
                        anchors.fill: parent
                        anchors.margins: content.listMargins
                        anchors.topMargin: 2
                        highlightMoveDuration: 0
                        onHeightChanged: positionViewAtIndex(currentIndex, ListView.Contain)
                        model: content.pointSizes
                        highlight: Rectangle {
                            color: palette.highlight
                            x: 2 - pointSizesListView.anchors.margins
                            width: pointSizesListView.parent.width - 4
                        }
                        delegate: Item {
                            width: parent.width
                            height: pointSizesText.height * content.delegateHeightMultiplier
                            Text {
                                id: pointSizesText
                                text: content.pointSizes[index]
                                width: parent.width
                                elide: Text.ElideRight
                                color: index === pointSizesListView.currentIndex ? palette.highlightedText : palette.windowText
                                anchors.verticalCenter: parent.verticalCenter
                            }
                            MouseArea {
                                anchors.fill: parent
                                onClicked: {
                                    pointSizesListView.currentIndex = index
                                    content.font.pointSize = content.pointSizes[pointSizesListView.currentIndex]
                                }
                            }
                        }
                    }
                }
            }

            Grid {
                id: settingsBottom
                columns: 3
                spacing: content.spacing
                width: parent.width
                height: initialHeight + content.extraHeight / 4
                property real initialHeight
                property real secondRowHeight: height - weightLabel.height - spacing
                Component.onCompleted: initialHeight = implicitHeight

                Text { id: weightLabel; text: qsTr("Weight"); font.bold: true }
                Text { id: optionsLabel; text: qsTr("Style"); font.bold: true }
                Text { id: sampleLabel; text: qsTr("Sample"); font.bold: true }
                Rectangle {
                    id: weightColumn
                    radius: 3
                    color: palette.window
                    border.color: content.borderColor
                    implicitWidth: optionsColumn.implicitWidth
                    implicitHeight: optionsColumn.implicitHeight
                    height: parent.secondRowHeight
                    clip: true
                    ListView {
                        id: weightListView
                        anchors.fill: parent
                        anchors.margins: content.listMargins
                        anchors.topMargin: 2
                        highlightMoveDuration: 0
                        onHeightChanged: positionViewAtIndex(currentIndex, ListView.Contain)
                        function reset() {
                            weightModel.findIndex()
                        }

                        model: ListModel {
                            id: weightModel
                            ListElement {
                                name: "Light"
                                weight: Font.Light
                            }
                            ListElement {
                                name: "Normal"
                                weight: Font.Normal
                            }
                            ListElement {
                                name: "DemiBold"
                                weight: Font.DemiBold
                            }
                            ListElement {
                                name: "Bold"
                                weight: Font.Bold
                            }
                            ListElement {
                                name: "Black"
                                weight: Font.Black
                            }
                            Component.onCompleted: weightListView.reset()
                            function findIndex() {
                                var find = false
                                for (var i = 0; i < weightModel.count; ++i) {
                                    if (content.font.weight == weightModel.get(i).weight) {
                                        find = true
                                        weightListView.currentIndex = i
                                        break
                                    }
                                }
                                if (find == false) {
                                    content.font.weight = weightModel.get(1).family
                                    fontListView.currentIndex = 1
                                }
                            }
                        }
                        highlight: Rectangle {
                            color: palette.highlight
                            x: 2 - weightListView.anchors.margins
                            width: weightListView.parent.width - 4
                        }
                        delegate: Item {
                            width: parent.width
                            height: weightText.height * content.delegateHeightMultiplier
                            Text {
                                id: weightText
                                text: name
                                width: parent.width
                                elide: Text.ElideRight
                                color: index === weightListView.currentIndex ? palette.highlightedText : palette.windowText
                                anchors.verticalCenter: parent.verticalCenter
                            }
                            MouseArea {
                                anchors.fill: parent
                                onClicked: {
                                    weightListView.currentIndex = index
                                    content.font.weight = weightModel.get(weightListView.currentIndex).weight
                                }
                            }
                        }
                    }
                }
                Column {
                    id: optionsColumn
                    spacing: 4
                    CheckBox {
                        id: italicCheckBox
                        text: qsTr("Italic")
                        checked: content.font.italic
                        onClicked: { content.font.italic = italicCheckBox.checked }
                    }
                    CheckBox {
                        id: underlineCheckBox
                        text: qsTr("Underline")
                        checked: content.font.underline
                        onClicked: { content.font.underline = underlineCheckBox.checked }
                    }
                    CheckBox {
                        id: overlineCheckBox
                        text: qsTr("Overline")
                        checked: content.font.overline
                        onClicked: { content.font.overline = overlineCheckBox.checked }
                    }
                    CheckBox {
                        id: strikeoutCheckBox
                        text: qsTr("Strikeout")
                        checked: content.font.strikeout
                        onClicked: { content.font.strikeout = strikeoutCheckBox.checked }
                    }
                }
                Rectangle {
                    clip: true
                    implicitWidth: sample.implicitWidth + parent.spacing
                    implicitHeight: optionsColumn.implicitHeight
                    width: parent.width - weightColumn.width - optionsColumn.width - parent.spacing * 2
                    height: parent.secondRowHeight
                    color: palette.window
                    border.color: content.borderColor
                    Text {
                        id: sample
                        anchors.centerIn: parent
                        font: content.font
                        text: content.writingSystemSample
                    }
                }
            }

            Item {
                id: buttonRow
                height: buttonsOnly.height
                width: parent.width
                Row {
                    id: buttonsOnly
                    spacing: content.spacing
                    anchors.right: parent.right
                    Button {
                        text: qsTr("Cancel")
                        onClicked: root.reject()
                    }
                    Button {
                        text: qsTr("OK")
                        onClicked: {
                            root.font = content.font
                            root.accept()
                        }
                    }
                }
            }
        }
    }
}

