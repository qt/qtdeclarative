// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0


import QtQuick 2.0
import "widgets"


Rectangle {
    id: page
    width: 640
    height: 480
    function col(str) {
        return Qt.hsla((str.charCodeAt(0)-65)/9, 1.0, 0.5, 1)
    }
    color: col(Accessible.name)
    Accessible.name: "A"
    Accessible.role: Accessible.StaticText

    Rectangle {
        id: b
        width: 20
        height: parent.height/2
        anchors.verticalCenter: parent.verticalCenter
        color: col(Accessible.name)
        Accessible.name: "B"
        Accessible.role: Accessible.StaticText
    }

    Rectangle {
        x: 20
        width: 80
        height: parent.height/2
        anchors.verticalCenter: parent.verticalCenter
        color: col(Accessible.name)
        Accessible.ignored: true
        Accessible.name: "C"
        Accessible.role: Accessible.StaticText

        Rectangle {
            width: 20
            color: col(Accessible.name)
            height: parent.height/2
            anchors.verticalCenter: parent.verticalCenter
            Accessible.name: "E"
            Accessible.role: Accessible.StaticText
        }

        Rectangle {
            x: 20
            width: 20
            color: col(Accessible.name)
            height: parent.height/2
            anchors.verticalCenter: parent.verticalCenter
            Accessible.name: "F"
            Accessible.role: Accessible.StaticText
        }

        Rectangle {
            x: 40
            width: 20
            height: parent.height/2
            anchors.verticalCenter: parent.verticalCenter
            color: col(Accessible.name)
            Accessible.ignored: true
            Accessible.name: "G"
            Accessible.role: Accessible.StaticText
            Rectangle {
                width: 20
                height: parent.height/2
                anchors.verticalCenter: parent.verticalCenter
                color: col(Accessible.name)
                Accessible.name: "I"
                Accessible.role: Accessible.StaticText
            }
        }
        Rectangle {
            x: 60
            width: 20
            height: parent.height/2
            anchors.verticalCenter: parent.verticalCenter
            color: col(Accessible.name)
            Accessible.name: "H"
            Accessible.role: Accessible.StaticText
        }
    }

    Rectangle {
        x: 100
        width: 20
        height: parent.height/2
        anchors.verticalCenter: parent.verticalCenter
        color: col(Accessible.name)
        Accessible.name: "D"
        Accessible.role: Accessible.StaticText
    }
}
