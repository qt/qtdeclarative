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
    title: "Qt Quick Controls - Theme Example"

    header: ToolBar {
        ToolButton {
            label: Text {
                // \u25C0 (black left-pointing triangle is) missing in some fonts
                // => use a rotated \u25B2 (black up-pointing triangle) instead
                text: "\u25B2"
                rotation: -90
                color: enabled ? Theme.accentColor : Theme.frameColor
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
            Theme.roundness: roundedToggle.checked ? 3 : 0
            Theme.accentColor: Qt.hsla(colorSlider.position, 0.5, 0.5, 1.0)
            Theme.backgroundColor: darkButton.checked ? "#444" : "#fff"
            Theme.frameColor: darkButton.checked ? "#666" : "#ccc"
            Theme.textColor: darkButton.checked ? "#eee" : "#111"
            Theme.pressColor: darkButton.checked ? "#33ffffff" : "#33333333"
            Theme.baseColor: darkButton.checked ? "#444" : "#eee"
            background: Rectangle {
                color: Theme.backgroundColor
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
                        color: Theme.accentColor
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
                        color: Theme.frameColor
                    }

                    Column {
                        spacing: 6
                        width: parent.width
                        Label {
                            text: "Accent color"
                            color: Theme.textColor
                        }
                        Slider {
                            id: colorSlider
                            width: parent.width
                            value: 0.275
                        }
                    }

                    Rectangle {
                        width: parent.width
                        height: 1
                        color: Theme.frameColor
                    }

                    ExclusiveGroup {
                        id: themeGroup
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
                            ExclusiveGroup.group: themeGroup
                        }
                        RadioButton {
                            id: darkButton
                            text: "Dark"
                            width: parent.width
                            layoutDirection: Qt.RightToLeft
                            ExclusiveGroup.group: themeGroup
                        }
                    }

                    Rectangle {
                        width: parent.width
                        height: 1
                        color: Theme.frameColor
                    }

                    ToggleButton {
                        id: roundedToggle
                        width: parent.width
                        text: "Rounded corners"
                        layoutDirection: Qt.RightToLeft
                        checked: true
                    }

                    Rectangle {
                        width: parent.width
                        height: 1
                        color: Theme.frameColor
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
