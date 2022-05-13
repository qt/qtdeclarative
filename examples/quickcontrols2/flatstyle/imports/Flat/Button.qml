// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Templates as T
import Theme

T.Button {
    id: control

    font: Theme.font

    implicitWidth: Math.max(background ? background.implicitWidth : 0,
                                         contentItem.implicitWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(background ? background.implicitHeight : 0,
                                          contentItem.implicitHeight + topPadding + bottomPadding)
    leftPadding: 4
    rightPadding: 4

    background: Rectangle {
        id: buttonBackground
        implicitWidth: 100
        implicitHeight: 40
        opacity: enabled ? 1 : 0.3
        border.color: Theme.mainColor
        border.width: 1
        radius: 2

        states: [
            State {
                name: "normal"
                when: !control.down
            },
            State {
                name: "down"
                when: control.down
                PropertyChanges {
                    buttonBackground.border.color: Theme.mainColorDarker
                }
            }
        ]
    }

    contentItem: Text {
        id: textItem
        text: control.text

        font: control.font
        opacity: enabled ? 1.0 : 0.3
        color: Theme.mainColor
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight

        states: [
            State {
                name: "normal"
                when: !control.down
            },
            State {
                name: "down"
                when: control.down
                PropertyChanges {
                    textItem.color: Theme.mainColorDarker
                }
            }
        ]
    }
}

