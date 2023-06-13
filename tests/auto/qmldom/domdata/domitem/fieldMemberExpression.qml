// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

import QtQuick

Item {
    id: root

    property int p: 42

    property Item property1: Item {
        property bool p: 44
        property Item property2: Item {
            property string p: "Hello World"
            property Item property3: Item {
                property real p: 123
            }
        }
    }

    property var p1: p
    property var p1Qualified: root.p
    property var p1Bracket: root["p"]
    property var p1Index: root[42]
    property var p1Key: root[p]

    property var p2: property1.p
    property var p2Qualified: root.property1.p

    property var p3: property1.property2.p
    property var p3Qualified: root.property1.property2.p
}
