// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Templates as T
import Theme

T.CheckBox {
    id: control

    font: Theme.font

    implicitWidth: Math.max(background ? background.implicitWidth : 0,
                                         contentItem.implicitWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(background ? background.implicitHeight : 0,
                                          Math.max(contentItem.implicitHeight,
                                                   indicator ? indicator.implicitHeight : 0) + topPadding + bottomPadding)
    leftPadding: 4
    indicator: Rectangle {
        id: checkboxHandle
        implicitWidth: Theme.baseSize * 2.6
        implicitHeight: Theme.baseSize * 2.6
        x: control.leftPadding
        anchors.verticalCenter: parent.verticalCenter
        radius: 2
        border.color: Theme.mainColor

        Rectangle {
            id: rectangle
            width: Theme.baseSize * 1.4
            height: Theme.baseSize * 1.4
            x: Theme.baseSize * 0.6
            y: Theme.baseSize * 0.6
            radius: Theme.baseSize * 0.4
            visible: false
            color: Theme.mainColor
        }

        states: [
            State {
                name: "unchecked"
                when: !control.checked && !control.down
            },
            State {
                name: "checked"
                when: control.checked && !control.down

                PropertyChanges {
                    rectangle.visible: true
                }
            },
            State {
                name: "unchecked_down"
                when: !control.checked && control.down

                PropertyChanges {
                    rectangle.color: Theme.mainColorDarker
                    checkboxHandle.border.color: Theme.mainColorDarker
                }
            },
            State {
                name: "checked_down"
                extend: "unchecked_down"
                when: control.checked && control.down

                PropertyChanges {
                    rectangle.visible: true
                }
            }
        ]
    }

    background: Rectangle {
        implicitWidth: 140
        implicitHeight: Theme.baseSize * 3.8
        color: Theme.lightGray
        border.color: Theme.gray
    }

    contentItem: Text {
        leftPadding: control.indicator.width + 4

        text: control.text
        font: control.font
        color: Theme.dark
        elide: Text.ElideRight
        visible: control.text
        horizontalAlignment: Text.AlignLeft
        verticalAlignment: Text.AlignVCenter
    }
}

