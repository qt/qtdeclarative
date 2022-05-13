// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.0

Item {
    property int p1: obj ? obj.p1 : 0
    property real p2: obj ? obj.p2 : 0
    property bool p3: obj ? obj.p3 : false
    property string p4: obj ? obj.p4 : "bar"

    property QtObject obj: QtObject {
        property int p1: 1
        property real p2: 1
        property bool p3: true
        property string p4: "foo"
    }
}
