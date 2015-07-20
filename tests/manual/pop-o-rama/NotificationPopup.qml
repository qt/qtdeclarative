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
import "qrc:/shared"

T.Panel {
    id: popup

    property alias sender: senderLabel.text
    property alias text: label.text

    contentItem: T.Frame {
        id: panelControl

        x: Window.width
        y: topPadding

        Theme.backgroundColor: "#444"
        Theme.frameColor: "#666"
        Theme.textColor: "#eee"
        Theme.disabledColor: "#777"
        Theme.pressColor: "#33ffffff"
        Theme.baseColor: "#444"

        padding: 12
        implicitWidth: leftPadding + contentWidth + rightPadding
        implicitHeight: topPadding + contentHeight + bottomPadding
        contentWidth: row.width
        contentHeight: label.font.pixelSize * 4

        contentItem: Item {
            Row {
                id: row
                spacing: panelControl.rightPadding
                FontAwesomeIcon {
                    y: -label.font.pixelSize / 4
                    iconId: FontAwesome.comment
                    size: 36
                    color: Theme.textColor
                }

                Column {
                    spacing: label.font.pixelSize / 2
                    Label {
                        id: senderLabel
                        width: label.width
                        elide: Text.ElideRight
                        font.bold: true
                    }

                    Label {
                        id: label
                        width: 200
                        wrapMode: Text.WordWrap
                        maximumLineCount: 2
                        elide: Text.ElideRight
                    }
                }
            }
        }

        frame: Rectangle {
            width: parent.width
            height: parent.height
            color: Theme.backgroundColor
            border.color: Theme.frameColor
            radius: 5
            opacity: 0.8
        }

        background: null
    }

    showTransition: Transition {
        OpacityAnimator {
            target: panelControl
            from: 0; to: 1
            duration: 200
        }

        XAnimator {
            target: panelControl
            from: panelControl.Window.width
            to: panelControl.Window.width - panelControl.width - panelControl.rightPadding
            duration: 300
        }
    }

    hideTransition: Transition {
        OpacityAnimator {
            target: panelControl
            from: 1; to: 0
            duration: 200
        }
    }
}
