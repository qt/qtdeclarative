// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
import QtQuick

Item {
    id: root
    property alias loop1: root.loop1
    property alias loop2: obj1.loop3
    property int p1: 1
    property Item i1: Item{
        id: it1
        property Item i11: Item{
            id: it11
            property int p1: 42
        }
    }
    QtObject {
        id: obj1
        property alias objRef2: obj2
        property real p1: 2.0
        property alias loop3: root.loop2
        property alias loop4: root.loop1
        property alias a3i: root.p1
        property alias a7i: obj2.a8i // loop error
    }
    QtObject {
        id: obj2
        property alias a8i: obj1.a3i
        property alias tooDeep: root.i1.i11.p1
        property alias invalid1: noId
        property alias invalid2: noId.objectName
        property alias a13i: obj1.objRef2.a8i // invalid property a8i error
    }
    property alias tooDeepRef: obj2.tooDeep
    Rectangle {
        color: "red"
        Text{
            id:t1
            text: obj1.p1
        }
        Text{
            text: obj2.a8i
            anchors.top: t1.bottom
        }
    }
}
