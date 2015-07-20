/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
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

import QtQuick 2.6
import QtQuick.Window 2.2
import Qt.labs.templates 1.0 as T
import Qt.labs.controls 1.0

T.Panel {
    id: contextMenu

    property Item target
    property alias x: panelControl.x
    property alias y: panelControl.y

    contentItem: T.Frame {
        id: panelControl

        Theme.backgroundColor: "#444"
        Theme.frameColor: "#666"
        Theme.textColor: "#eee"
        Theme.disabledColor: "#777"
        Theme.pressColor: "#33ffffff"
        Theme.baseColor: "#444"

        padding: 6
        implicitWidth: leftPadding + contentWidth + rightPadding
        implicitHeight: topPadding + contentHeight + bottomPadding
        contentWidth: buttonRow.width
        contentHeight: buttonRow.height

        contentItem: Item {
            Row {
                id: buttonRow
                spacing: 12

                ToolButton {
                    text: "Cut"
                    onClicked: { target.cut(); contextMenu.hide() }
                    enabled: target && (target["selectedText"] !== "" && !target["readOnly"])
                }
                ToolButton {
                    text: "Copy"
                    onClicked: { target.copy(); contextMenu.hide() }
                    enabled: target && target["selectedText"] !== ""
                }
                ToolButton {
                    text: "Paste"
                    onClicked: { target.paste(); contextMenu.hide() }
                    enabled: target && target["canPaste"] === true
                }
                ToolButton {
                    text: "Select"
                    onClicked: { target.selectWord(); contextMenu.hide() }
                    enabled: target && target["text"] !== ""
                }
                ToolButton {
                    text: "Select All"
                    onClicked: { target.selectAll(); contextMenu.hide() }
                    enabled: target && target["text"] !== ""
                }
            }
        }

        frame: Rectangle {
            width: parent.width
            height: parent.height
            color: Theme.backgroundColor
            border.color: Theme.frameColor
            radius: 5
        }

        background: null

        Connections {
            target: contextMenu.target
            ignoreUnknownSignals: true

            onActiveFocusChanged: {
                if (!target.activeFocus && contextMenu.visible)
                    contextMenu.hide()
            }

            onPressAndHold: {
                var m = 3
                var pos = target.mapToItem(target.Window.contentItem, mouse.x - panelControl.width / 2, -panelControl.height - m)
                contextMenu.x = Math.max(m, Math.min(target.Window.width - panelControl.width - m, pos.x))
                contextMenu.y = pos.y
                contextMenu.show()
            }
        }
    }

    onAboutToShow: {
        if (target)
            target.cursorVisible = false
    }

    onAboutToHide: {
        if (target) {
            var t = target
            target.cursorVisible = Qt.binding(function() {
                return t.activeFocus && t.selectedText === ""
            })
        }
    }

    onPressedOutside: hide()
}
