/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtDeclarative module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

import QtQuick 2.1
import QtQuick.Dialogs 1.0
import Qt.labs.folderlistmodel 2.0
import "qml"

AbstractFileDialog {
    id: root
    onVisibleChanged: {
        if (visible) {
            selectedIndices = []
            lastClickedIdx = -1
            currentPathField.visible = false
        }
    }

    property bool showFocusHighlight: false
    property real textX: 28
    property SystemPalette palette
    property var selectedIndices: []
    property int lastClickedIdx: -1
    folder: urlToPath(view.model.folder)

    function dirDown(path) {
        view.model.folder = path
        lastClickedIdx = -1
        selectedIndices = []
    }
    function dirUp() {
        view.model.folder = view.model.parentFolder
        lastClickedIdx = -1
        selectedIndices = []
    }
    function up(extend) {
        if (view.currentIndex > 0)
            --view.currentIndex
        else
            view.currentIndex = 0
        if (extend) {
            if (selectedIndices.indexOf(view.currentIndex) < 0) {
                var selCopy = selectedIndices
                selCopy.push(view.currentIndex)
                selectedIndices = selCopy
            }
        } else
            selectedIndices = [view.currentIndex]
    }
    function down(extend) {
        if (view.currentIndex < view.model.count - 1)
            ++view.currentIndex
        else
            view.currentIndex = view.model.count - 1
        if (extend) {
            if (selectedIndices.indexOf(view.currentIndex) < 0) {
                var selCopy = selectedIndices
                selCopy.push(view.currentIndex)
                selectedIndices = selCopy
            }
        } else
            selectedIndices = [view.currentIndex]
    }
    function acceptSelection() {
        clearSelection()
        if (selectFolder && selectedIndices.length == 0)
            addSelection(folder)
        else {
            selectedIndices.map(function(idx) {
                if (view.model.isFolder(idx)) {
                    if (selectFolder)
                        addSelection(view.model.get(idx, "filePath"))
                } else {
                    if (!selectFolder)
                        addSelection(view.model.get(idx, "filePath"))
                }
            })
        }
        accept()
    }

    Rectangle {
        width: 480  // TODO: QTBUG-29817 geometry from AbstractFileDialog
        height: 320
        id: window
        color: palette.window
        anchors.centerIn: Qt.application.supportsMultipleWindows ? null : parent

        SystemPalette { id: palette }

        Component {
            id: folderDelegate
            Rectangle {
                id: wrapper
                function launch() {
                    if (view.model.isFolder(index)) {
                        dirDown(filePath)
                    } else {
                        root.acceptSelection()
                    }
                }
                width: window.width
                height: nameText.implicitHeight * 1.5
                color: "transparent"
                Rectangle {
                    id: itemHighlight
                    visible: root.selectedIndices.indexOf(index) >= 0
                    anchors.fill: parent
                    color: palette.highlight
                }
                Image {
                    id: icon
                    source: "images/folder.png"
                    height: wrapper.height - y * 2; width: height
                    x: (root.textX - width) / 2
                    y: 2
                    visible: view.model.isFolder(index)
                }
                Text {
                    id: nameText
                    anchors.fill: parent; verticalAlignment: Text.AlignVCenter
                    text: fileName
                    anchors.leftMargin: root.textX
                    color: itemHighlight.visible ? palette.highlightedText : palette.windowText
                    elide: Text.ElideRight
                }
                MouseArea {
                    id: mouseRegion
                    anchors.fill: parent
                    onDoubleClicked: {
                        selectedIndices = [index]
                        root.lastClickedIdx = index
                        launch()
                    }
                    onClicked: {
                        view.currentIndex = index
                        if (mouse.modifiers & Qt.ControlModifier && root.selectMultiple) {
                            // modifying the contents of selectedIndices doesn't notify,
                            // so we have to re-assign the variable
                            var selCopy = selectedIndices
                            var existingIdx = selCopy.indexOf(index)
                            if (existingIdx >= 0)
                                selCopy.splice(existingIdx, 1)
                            else
                                selCopy.push(index)
                            selectedIndices = selCopy
                        } else if (mouse.modifiers & Qt.ShiftModifier && root.selectMultiple) {
                            if (root.lastClickedIdx >= 0) {
                                var sel = []
                                if (index > lastClickedIdx) {
                                    for (var i = root.lastClickedIdx; i <= index; i++)
                                        sel.push(i)
                                } else {
                                    for (var i = root.lastClickedIdx; i >= index; i--)
                                        sel.push(i)
                                }
                                selectedIndices = sel
                            }
                        } else {
                            selectedIndices = [index]
                            root.lastClickedIdx = index
                        }
                    }
                }
            }
        }

        ListView {
            id: view
            anchors.top: titleBar.bottom
            anchors.bottom: bottomBar.top
            clip: true
            x: 0
            width: parent.width
            model: FolderListModel { }
            delegate: folderDelegate
            highlight: Rectangle {
                color: "transparent"
                border.color: palette.midlight
            }
            highlightMoveDuration: 0
            highlightMoveVelocity: -1
            focus: !currentPathField.visible
            Keys.onPressed: {
                event.accepted = true
                switch (event.key) {
                case Qt.Key_Up:
                    root.up(event.modifiers & Qt.ShiftModifier && root.selectMultiple)
                    break
                case Qt.Key_Down:
                    root.down(event.modifiers & Qt.ShiftModifier && root.selectMultiple)
                    break
                case Qt.Key_Left:
                    root.dirUp()
                    break
                case Qt.Key_Return:
                case Qt.Key_Select:
                case Qt.Key_Right:
                    if (view.currentItem)
                        view.currentItem.launch()
                    else
                        root.acceptSelection()
                    break
                default:
                    // do nothing
                    event.accepted = false
                    break
                }
            }
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
            BorderImage {
                source: "images/titlebar.sci"
                anchors.fill: parent
                anchors.topMargin: -7
                anchors.bottomMargin: -7
            }
            Rectangle {
                id: upButton
                width: root.textX
                height: titleBar.height
                color: "transparent"

                Image {
                    id: upButtonImage
                    anchors.centerIn: parent; source: "images/up.png"
                }
                MouseArea { id: upRegion; anchors.centerIn: parent
                    width: 56
                    height: parent.height
                    onClicked: if (view.model.parentFolder != "") dirUp()
                }
                states: [
                    State {
                        name: "pressed"
                        when: upRegion.pressed
                        PropertyChanges { target: upButton; color: palette.highlight }
                    }
                ]
            }
            Text {
                id: currentPathText
                anchors.left: parent.left; anchors.right: parent.right; anchors.verticalCenter: parent.verticalCenter
                anchors.leftMargin: textX; anchors.rightMargin: 4
                text: root.urlToPath(view.model.folder)
                color: "white"
                elide: Text.ElideLeft; horizontalAlignment: Text.AlignRight; verticalAlignment: Text.AlignVCenter
                MouseArea {
                    anchors.fill: parent
                    onClicked: currentPathField.visible = true
                }
            }
            TextField {
                id: currentPathField
                anchors.left: parent.left; anchors.right: parent.right; anchors.verticalCenter: parent.verticalCenter
                anchors.leftMargin: textX; anchors.rightMargin: 4
                visible: false
                focus: visible
                text: root.urlToPath(view.model.folder)
                onAccepted: {
                    root.clearSelection()
                    if (root.addSelection(text))
                        root.accept()
                    else
                        view.model.folder = root.pathFolder(text)
                }
                onDownPressed: currentPathField.visible = false
            }
        }
        Rectangle {
            id: bottomBar
            width: parent.width
            height: buttonRow.height + buttonRow.spacing * 2
            anchors.bottom: parent.bottom
            gradient: Gradient {
                GradientStop { position: 0.0; color: palette.dark }
                GradientStop { position: 0.3; color: palette.mid }
                GradientStop { position: 0.85; color: palette.mid }
                GradientStop { position: 1.0; color: palette.light }
            }

            Row {
                id: buttonRow
                anchors.right: parent.right
                anchors.rightMargin: spacing
                anchors.verticalCenter: parent.verticalCenter
                spacing: 4
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
                            dirDown(view.model.get(view.currentIndex, "filePath"))
                        else
                            root.acceptSelection()
                    }
                }
            }
        }
    }
}
