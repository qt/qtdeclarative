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
import Qt.labs.controls 1.0

ApplicationWindow {
    id: window
    width: 360
    height: 520
    visible: true
    title: "Qt Labs Controls - Drawer Example"

    Rectangle {
        id: content
        anchors.fill: parent
        anchors.margins: -1
        border.color: Theme.frameColor

        Image {
            width: window.width / 2
            height: window.height / 2
            anchors.centerIn: parent
            anchors.horizontalCenterOffset: window.width > window.height ? width / 2 : 0
            anchors.verticalCenterOffset: window.width < window.height ? -height / 4 : 0
            fillMode: Image.PreserveAspectFit
            source: "qrc:/images/qt-logo.png"
        }

        Image {
            width: window.width / 2
            anchors.bottom: parent.bottom
            anchors.bottomMargin: height / 2
            fillMode: Image.PreserveAspectFit
            source: "qrc:/images/arrow.png"
        }

        transform: Translate {
            x: effect.current === uncover ? drawer.position * listview.width :
               effect.current === push ? drawer.position * listview.width * 0.5 : 0
        }

        z: effect.current === uncover ? 2 : 0
    }

    Drawer {
        id: drawer
        anchors.fill: parent

        ListView {
            id: listview

            width: window.width / 3 * 2
            height: window.height

            ExclusiveGroup {
                id: effect
            }

            model: VisualItemModel {
                Label {
                    text: "Settings"
                    x: 6
                    width: parent.width - 12
                    lineHeight: 2.0
                    color: Theme.accentColor
                    verticalAlignment: Text.AlignVCenter
                }
                Rectangle { width: parent.width; height: 1; color: Theme.frameColor }
                Switch {
                    id: dim
                    text: "Dim"
                    checked: true
                    width: parent.width
                    layoutDirection: Qt.RightToLeft
                    enabled: effect.current != uncover
                }
                Rectangle { width: parent.width; height: 1; color: Theme.frameColor }
                RadioButton {
                    id: overlay
                    text: "Overlay"
                    checked: true
                    width: parent.width
                    ExclusiveGroup.group: effect
                    layoutDirection: Qt.RightToLeft
                }
                RadioButton {
                    id: push
                    text: "Push"
                    width: parent.width
                    ExclusiveGroup.group: effect
                    layoutDirection: Qt.RightToLeft
                }
                RadioButton {
                    id: uncover
                    text: "Uncover"
                    width: parent.width
                    ExclusiveGroup.group: effect
                    layoutDirection: Qt.RightToLeft
                }
                Rectangle { width: parent.width; height: 1; color: Theme.frameColor }
            }
            Rectangle {
                z: -1
                anchors.fill: parent
                anchors.topMargin: -1
                anchors.bottomMargin: -1
                border.color: Theme.frameColor
            }

            transform: Translate {
                x: effect.current === uncover ? (1.0 - drawer.position) * listview.width : 0
            }
        }

        background.visible: dim.checked

        onClicked: close()
    }
}
