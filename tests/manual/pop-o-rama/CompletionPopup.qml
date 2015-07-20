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
import Qt.labs.templates 1.0 as T
import Qt.labs.controls 1.0
import QtQuick.Window 2.2


T.Panel {
    id: popup

    property Item input
    property var model

    modal: true

    contentItem: Frame {
        id: frame
        padding: 6
        width: input.width
        implicitHeight: topPadding + contentHeight + bottomPadding
        contentHeight: completionList.contentHeight
        Keys.forwardTo: [completionList]

        contentItem: Item {
            ListView {
                id: completionList
                model: popup.model
                height: contentHeight
                Keys.onReturnPressed: {
                    input.text = currentItem.text
                    event.accepted = false
                }
                Keys.onEscapePressed: popup.hide()
                delegate: Text {
                    text: modelData
                    elide: Text.ElideMiddle
                    color: ListView.isCurrentItem ? Theme.selectedTextColor : Theme.textColor
                    width: frame.width - 2 * frame.padding
                    MouseArea {
                        anchors.fill: parent
                        anchors.leftMargin: -frame.leftPadding
                        anchors.rightMargin: -frame.rightPadding
                        onClicked: frame.setInputTextAndDismiss(parent.text)
                    }
                }
                highlight: Rectangle {
                    x: -frame.leftPadding
                    y: completionList.currentItem.y
                    width: frame.width
                    height: completionList.currentItem.height
                    color: Theme.selectionColor
                }
                highlightFollowsCurrentItem: false
            }
        }

        frame: Rectangle {
            width: frame.width
            height: frame.height
            color: Theme.backgroundColor
            border.color: Theme.frameColor
        }

        background: null

        Connections {
            target: input
            onActiveFocusChanged: if (!input.activeFocus && popup.visible) frame.hide()
            onEditingFinished: if (popup.visible) popup.hide()
            onTextChanged: {
                if (input.text !== "" && !popup.visible) {
                    var pos = input.mapToItem(Window.contentItem, 0, input.height + 3)
                    frame.x = pos.x
                    frame.y = pos.y
                    popup.show()
                }
            }
        }

        function setInputTextAndDismiss(text) {
            input.text = text
            popup.hide()
        }
    }

    onPressedOutside: hide()
}
