/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL3$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or later as published by the Free
** Software Foundation and appearing in the file LICENSE.GPL included in
** the packaging of this file. Please review the following information to
** ensure the GNU General Public License version 2.0 requirements will be
** met: http://www.gnu.org/licenses/gpl-2.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

import QtQuick 2.2
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
