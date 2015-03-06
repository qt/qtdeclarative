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
import QtQuick.Controls 2.0

ApplicationWindow {
    id: window
    width: 360
    height: 520
    visible: true
    title: "Qt Quick Controls - Styles Example"

    header: ToolBar {
        ToolButton {
            label: Text {
                text: "\u25C0"
                color: enabled ? window.style.accentColor : window.style.frameColor
                anchors.centerIn: parent
            }
            enabled: stackview.depth > 1
            onClicked: stackview.pop()
        }
    }

    StackView {
        id: stackview
        anchors.fill: parent
        initialItem: pageComponent
    }

    Component {
        id: pageComponent
        Control {
            id: page
            style: Style {
                padding: 6
                roundness: roundedToggle.checked ? 3 : 0
                accentColor: Qt.hsla(colorSlider.position, 0.5, 0.5, 1.0)
                backgroundColor: darkButton.checked ? "#444" : "#fff"
                frameColor: darkButton.checked ? "#666" : "#ccc"
                textColor: darkButton.checked ? "#eee" : "#111"
                pressColor: darkButton.checked ? "#33ffffff" : "#33333333"
                baseColor: darkButton.checked ? "#444" : "#eee"
            }
            background: Rectangle {
                color: style.backgroundColor
            }
            Flickable {
                anchors.fill: parent
                contentHeight: column.height + 48

                Column {
                    id: column

                    x: (window.width - width) / 2
                    y: 24
                    width: window.width / 2
                    spacing: 12

                    Label {
                        text: "Code less. Create more."
                        color: page.style.accentColor
                        width: parent.width
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                        wrapMode: Text.WordWrap
                        font.pointSize: 26
                    }

                    Item { width: 1; height: 48 }

                    Rectangle {
                        width: parent.width
                        height: 1
                        color: page.style.frameColor
                    }

                    Column {
                        spacing: 6
                        width: parent.width
                        Label {
                            text: "Accent color"
                            color: page.style.textColor
                        }
                        Slider {
                            id: colorSlider
                            width: parent.width
                            value: 0.275
                            //background: Rectangle { border.color: "red" }
                        }
                    }

                    Rectangle {
                        width: parent.width
                        height: 1
                        color: page.style.frameColor
                    }

                    ExclusiveGroup {
                        id: styleGroup
                    }

                    Column {
                        width: parent.width
                        spacing: 6
                        RadioButton {
                            id: lightButton
                            text: "Light"
                            width: parent.width
                            checked: true
                            layoutDirection: Qt.RightToLeft
                            Exclusive.group: styleGroup
                            //background: Rectangle { border.color: "red" }
                        }
                        RadioButton {
                            id: darkButton
                            text: "Dark"
                            width: parent.width
                            layoutDirection: Qt.RightToLeft
                            Exclusive.group: styleGroup
                            //background: Rectangle { border.color: "red" }
                        }
                    }

                    Rectangle {
                        width: parent.width
                        height: 1
                        color: page.style.frameColor
                    }

                    ToggleButton {
                        id: roundedToggle
                        width: parent.width
                        text: "Rounded corners"
                        layoutDirection: Qt.RightToLeft
                        checked: true
                        //background: Rectangle { border.color: "red" }
                    }

                    Rectangle {
                        width: parent.width
                        height: 1
                        color: page.style.frameColor
                    }

                    Button {
                        text: "Push"
                        anchors.right: parent.right
                        onClicked: stackview.push(pageComponent)
                    }
                }

                AbstractScrollIndicator.vertical: ScrollIndicator { }
            }
        }
    }
}
