// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Templates as T
import Theme

T.Switch {
    id: control

    implicitWidth: indicator.implicitWidth
    implicitHeight: background.implicitHeight

    background: Rectangle {
        implicitWidth: 140
        implicitHeight: Theme.baseSize * 3.8
        color: Theme.lightGray
        border.color: Theme.gray
    }

    leftPadding: 4

    indicator: Rectangle {
        id: switchHandle
        implicitWidth: Theme.baseSize * 4.8
        implicitHeight: Theme.baseSize * 2.6
        x: control.leftPadding
        anchors.verticalCenter: parent.verticalCenter
        radius: Theme.baseSize * 1.3
        color: Theme.light
        border.color: Theme.lightGray

        Rectangle {
            id: nub

            width: Theme.baseSize * 2.6
            height: Theme.baseSize * 2.6
            radius: Theme.baseSize * 1.3
            color: Theme.light
            border.color: Theme.gray
        }

        states: [
            State {
                name: "off"
                when: !control.checked && !control.down
            },
            State {
                name: "on"
                when: control.checked && !control.down

                PropertyChanges {
                    switchHandle {
                        color: Theme.mainColor
                        border.color: Theme.mainColor
                    }
                    nub.x: switchHandle.width - nub.width

                }
            },
            State {
                name: "off_down"
                when: !control.checked && control.down

                PropertyChanges {
                    nub.color: Theme.light
                }

            },
            State {
                name: "on_down"
                extend: "off_down"
                when: control.checked && control.down

                PropertyChanges {
                    nub {
                        x: switchHandle.width - nub.width
                        color: Theme.light
                    }
                    switchHandle {
                        color: Theme.mainColorDarker
                        border.color: Theme.mainColorDarker
                    }
                }
            }
        ]
    }
}
