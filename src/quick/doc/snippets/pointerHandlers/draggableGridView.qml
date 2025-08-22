/****************************************************************************
**
** Copyright (C) 2024 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the documentation of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
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

pragma ComponentBehavior: Bound
import QtQml
import QtQuick
import QtQml.Models

//! [entire]
GridView {
    id: root
    width: 320
    height: 480
    cellWidth: 80
    cellHeight: 80
    interactive: false

    displaced: Transition {
        NumberAnimation {
            properties: "x,y"
            easing.type: Easing.OutQuad
        }
    }

    model: DelegateModel {
        id: visualModel
        model: 24
        property var dropTarget: undefined
        property bool copy: false
        delegate: DropArea {
            id: delegateRoot

            width: 80
            height: 80

            onEntered: drag => {
                if (visualModel.copy) {
                    if (drag.source !== icon)
                        visualModel.dropTarget = icon
                } else {
                    visualModel.items.move(drag.source.DelegateModel.itemsIndex, icon.DelegateModel.itemsIndex)
                }
            }

            Rectangle {
                id: icon
                objectName: DelegateModel.itemsIndex

                property string text
                Component.onCompleted: {
                    color = Qt.rgba(0.2 + (48 - DelegateModel.itemsIndex) * Math.random() / 48,
                                    0.3 + DelegateModel.itemsIndex * Math.random() / 48,
                                    0.4 * Math.random(),
                                    1.0)
                    text = DelegateModel.itemsIndex
                }
                border.color: visualModel.dropTarget === this ? "black" : "transparent"
                border.width: 2
                radius: 3
                width: 72
                height: 72
                anchors {
                    horizontalCenter: parent.horizontalCenter
                    verticalCenter: parent.verticalCenter
                }

                states: [
                    State {
                        when: dragHandler.active || controlDragHandler.active
                        ParentChange {
                            target: icon
                            parent: root
                        }

                        AnchorChanges {
                            target: icon
                            anchors {
                                horizontalCenter: undefined
                                verticalCenter: undefined
                            }
                        }
                    }
                ]

                Text {
                    anchors.centerIn: parent
                    color: "white"
                    font.pointSize: 14
                    text: controlDragHandler.active ? "+" : icon.text
                }

                //! [draghandlers]
                DragHandler {
                    id: dragHandler
                    acceptedModifiers: Qt.NoModifier
                    onActiveChanged: if (!active) visualModel.dropTarget = undefined
                }

                DragHandler {
                    id: controlDragHandler
                    acceptedModifiers: Qt.ControlModifier
                    onActiveChanged: {
                        visualModel.copy = active
                        if (!active) {
                            visualModel.dropTarget.text = icon.text
                            visualModel.dropTarget.color = icon.color
                            visualModel.dropTarget = undefined
                        }
                    }
                }
                //! [draghandlers]

                Drag.active: dragHandler.active || controlDragHandler.active
                Drag.source: icon
                Drag.hotSpot.x: 36
                Drag.hotSpot.y: 36
            }
        }
    }
}
//! [entire]
