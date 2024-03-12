// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
import QtQuick

Item {
    id: root
    property alias objRef1: root
    property alias objRef2: obj1
    property int p1: 1
    property alias a1i: root.p1
    property alias a2q: obj1.objectName
    property Item i1: Item{
        id: it1
        objectName: "it1"
        property QtObject i11: QtObject {
            id: it11
            objectName: "it11"
            property int p1: 42
        }
    }
    QtObject {
        id: obj1
        objectName:"obj1"
        property alias objRef2: obj2
        property real p1: 2.0
        property int p2: 33
        property alias a3i: root.p1
        property alias a5i: obj1.a4i
        property alias a4i: root.a1i
        property alias a6r: obj2.p2r
    }
    QtObject {
        id: obj2
        objectName:"obj2"
        property real p2r: 3.0
        property alias a8i: obj1.a3i
        property alias a9q: root.a2q
        property alias a11I: it1.objectName
        property alias a11Q: it1.i11.objectName
        property alias a12q: obj1.objectName
    }
    Rectangle {
        color: "red"
        Text{
            id:t1
            text: obj2.a11Q
        }
        Text{
            text: obj1.objRef2.p2r
            anchors.top: t1.bottom
        }
    }
}
