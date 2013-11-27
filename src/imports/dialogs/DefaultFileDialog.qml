/*****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtQuick.Dialogs module of the Qt Toolkit.
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
import QtQuick.Dialogs 1.0
import QtQuick.Window 2.1
import Qt.labs.folderlistmodel 2.0
import "qml"

AbstractFileDialog {
    id: root
    onVisibleChanged: {
        if (visible) {
            __selectedIndices = []
            __lastClickedIdx = -1
            currentPathField.visible = false
        }
    }
    onFolderChanged: view.model.folder = folder

    property real __textX: titleBar.height
    property SystemPalette __palette
    property var __selectedIndices: []
    property int __lastClickedIdx: -1

    function __dirDown(path) {
        view.model.folder = path
        __lastClickedIdx = -1
        __selectedIndices = []
    }
    function __dirUp() {
        view.model.folder = view.model.parentFolder
        __lastClickedIdx = -1
        __selectedIndices = []
    }
    function __up(extend) {
        if (view.currentIndex > 0)
            --view.currentIndex
        else
            view.currentIndex = 0
        if (extend) {
            if (__selectedIndices.indexOf(view.currentIndex) < 0) {
                var selCopy = __selectedIndices
                selCopy.push(view.currentIndex)
                __selectedIndices = selCopy
            }
        } else
            __selectedIndices = [view.currentIndex]
    }
    function __down(extend) {
        if (view.currentIndex < view.model.count - 1)
            ++view.currentIndex
        else
            view.currentIndex = view.model.count - 1
        if (extend) {
            if (__selectedIndices.indexOf(view.currentIndex) < 0) {
                var selCopy = __selectedIndices
                selCopy.push(view.currentIndex)
                __selectedIndices = selCopy
            }
        } else
            __selectedIndices = [view.currentIndex]
    }
    function __acceptSelection() {
        clearSelection()
        if (selectFolder && __selectedIndices.length == 0)
            addSelection(folder)
        else if (__selectedIndices.length > 0) {
            __selectedIndices.map(function(idx) {
                if (view.model.isFolder(idx)) {
                    if (selectFolder)
                        addSelection(view.model.get(idx, "fileURL"))
                } else {
                    if (!selectFolder)
                        addSelection(view.model.get(idx, "fileURL"))
                }
            })
        } else {
            addSelection(pathToUrl(currentPathField.text))
        }
        accept()
    }

    Rectangle {
        id: content
        property int maxSize: Math.min(Screen.desktopAvailableWidth, Screen.desktopAvailableHeight)
        // TODO: QTBUG-29817 geometry from AbstractFileDialog
        implicitWidth: Math.min(maxSize, Screen.pixelDensity * 100)
        implicitHeight: Math.min(maxSize, Screen.pixelDensity * 80)
        color: __palette.window
        focus: root.visible && !currentPathField.visible
        property real spacing: 6
        property real outerSpacing: 12
        SystemPalette { id: __palette }

        Component {
            id: folderDelegate
            Rectangle {
                id: wrapper
                function launch() {
                    if (view.model.isFolder(index)) {
                        __dirDown(filePath)
                    } else {
                        root.__acceptSelection()
                    }
                }
                width: content.width
                height: nameText.implicitHeight * 1.5
                color: "transparent"
                Rectangle {
                    id: itemHighlight
                    visible: root.__selectedIndices.indexOf(index) >= 0
                    anchors.fill: parent
                    color: __palette.highlight
                }
                Image {
                    id: icon
                    source: "images/folder.png"
                    height: wrapper.height - y * 2; width: height
                    x: (root.__textX - width) / 2
                    y: 2
                    visible: view.model.isFolder(index)
                }
                Text {
                    id: nameText
                    anchors.fill: parent; verticalAlignment: Text.AlignVCenter
                    text: fileName
                    anchors.leftMargin: root.__textX
                    color: itemHighlight.visible ? __palette.highlightedText : __palette.windowText
                    elide: Text.ElideRight
                }
                MouseArea {
                    id: mouseRegion
                    anchors.fill: parent
                    onDoubleClicked: {
                        __selectedIndices = [index]
                        root.__lastClickedIdx = index
                        launch()
                    }
                    onClicked: {
                        view.currentIndex = index
                        if (mouse.modifiers & Qt.ControlModifier && root.selectMultiple) {
                            // modifying the contents of __selectedIndices doesn't notify,
                            // so we have to re-assign the variable
                            var selCopy = __selectedIndices
                            var existingIdx = selCopy.indexOf(index)
                            if (existingIdx >= 0)
                                selCopy.splice(existingIdx, 1)
                            else
                                selCopy.push(index)
                            __selectedIndices = selCopy
                        } else if (mouse.modifiers & Qt.ShiftModifier && root.selectMultiple) {
                            if (root.__lastClickedIdx >= 0) {
                                var sel = []
                                if (index > __lastClickedIdx) {
                                    for (var i = root.__lastClickedIdx; i <= index; i++)
                                        sel.push(i)
                                } else {
                                    for (var i = root.__lastClickedIdx; i >= index; i--)
                                        sel.push(i)
                                }
                                __selectedIndices = sel
                            }
                        } else {
                            __selectedIndices = [index]
                            root.__lastClickedIdx = index
                        }
                    }
                }
            }
        }

        Keys.onPressed: {
            event.accepted = true
            switch (event.key) {
            case Qt.Key_Up:
                root.__up(event.modifiers & Qt.ShiftModifier && root.selectMultiple)
                break
            case Qt.Key_Down:
                root.__down(event.modifiers & Qt.ShiftModifier && root.selectMultiple)
                break
            case Qt.Key_Left:
                root.__dirUp()
                break
            case Qt.Key_Return:
            case Qt.Key_Select:
            case Qt.Key_Right:
                if (view.currentItem)
                    view.currentItem.launch()
                else
                    root.__acceptSelection()
                break
            case Qt.Key_Back:
            case Qt.Key_Escape:
                reject()
                break
            case Qt.Key_C:
                if (event.modifiers & Qt.ControlModifier)
                    currentPathField.copyAll()
                break
            case Qt.Key_V:
                if (event.modifiers & Qt.ControlModifier) {
                    currentPathField.visible = true
                    currentPathField.paste()
                }
                break
            default:
                // do nothing
                event.accepted = false
                break
            }
        }

        ListView {
            id: view
            anchors.top: titleBar.bottom
            anchors.bottom: bottomBar.top
            clip: true
            x: 0
            width: parent.width
            model: FolderListModel {
                onFolderChanged: {
                    root.folder = folder
                    currentPathField.text = root.urlToPath(view.model.folder)
                }
            }
            delegate: folderDelegate
            highlight: Rectangle {
                color: "transparent"
                border.color: Qt.darker(__palette.window, 1.3)
            }
            highlightMoveDuration: 0
            highlightMoveVelocity: -1
        }

        MouseArea {
            anchors.fill: view
            enabled: currentPathField.visible
            onClicked: currentPathField.visible = false
        }


        Item {
            id: titleBar
            width: parent.width
            height: currentPathField.height * 1.5
            Rectangle {
                anchors.fill: parent
                color: Qt.darker(__palette.window, 1.1)
                border.color: Qt.darker(__palette.window, 1.3)
            }
            Rectangle {
                id: upButton
                width: root.__textX
                height: titleBar.height
                color: "transparent"

                Image {
                    id: upButtonImage
                    anchors.centerIn: parent; source: "images/up.png"
                }
                MouseArea { id: upRegion; anchors.centerIn: parent
                    width: 56
                    height: parent.height
                    onClicked: if (view.model.parentFolder !== "") __dirUp()
                }
                states: [
                    State {
                        name: "pressed"
                        when: upRegion.pressed
                        PropertyChanges { target: upButton; color: __palette.highlight }
                    }
                ]
            }
            Text {
                id: currentPathText
                anchors.left: parent.left; anchors.right: parent.right; anchors.verticalCenter: parent.verticalCenter
                anchors.leftMargin: __textX; anchors.rightMargin: content.spacing
                text: root.urlToPath(view.model.folder)
                color: __palette.text
                elide: Text.ElideLeft; horizontalAlignment: Text.AlignRight; verticalAlignment: Text.AlignVCenter
                MouseArea {
                    anchors.fill: parent
                    onClicked: currentPathField.visible = true
                }
            }
            TextField {
                id: currentPathField
                anchors.left: parent.left; anchors.right: parent.right; anchors.verticalCenter: parent.verticalCenter
                anchors.leftMargin: __textX; anchors.rightMargin: content.spacing
                visible: false
                focus: visible
                onAccepted: {
                    root.clearSelection()
                    if (root.addSelection(root.pathToUrl(text)))
                        root.accept()
                    else
                        view.model.folder = root.pathFolder(text)
                }
                onDownPressed: currentPathField.visible = false
                onBackPressed: reject()
                onEscapePressed: reject()
            }
        }
        Rectangle {
            id: bottomBar
            width: parent.width
            height: buttonRow.height + buttonRow.spacing * 2
            anchors.bottom: parent.bottom
            color: Qt.darker(__palette.window, 1.1)
            border.color: Qt.darker(__palette.window, 1.3)

            Row {
                id: buttonRow
                anchors.right: parent.right
                anchors.rightMargin: spacing
                anchors.verticalCenter: parent.verticalCenter
                spacing: content.spacing
                TextField {
                    id: filterField
                    text: root.selectedNameFilter
                    visible: !selectFolder
                    width: bottomBar.width - cancelButton.width - okButton.width - parent.spacing * 5
                    anchors.verticalCenter: parent.verticalCenter
                    onAccepted: {
                        root.selectNameFilter(text)
                        view.model.nameFilters = text
                    }
                }
                Button {
                    id: cancelButton
                    text: "Cancel"
                    onClicked: root.reject()
                }
                Button {
                    id: okButton
                    text: "OK"
                    onClicked: {
                        if (view.model.isFolder(view.currentIndex) && !selectFolder)
                            __dirDown(view.model.get(view.currentIndex, "filePath"))
                        else
                            root.__acceptSelection()
                    }
                }
            }
        }
    }
}
