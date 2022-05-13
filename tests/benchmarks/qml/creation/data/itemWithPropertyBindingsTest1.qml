// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.0

Item {
    property int p1: obj.p1
    property real p2: obj.p2
    property bool p3: obj.p3
    property string p4: obj.p4

    property QtObject obj: QtObject {
        property int p1: 1
        property real p2: 1
        property bool p3: true
        property string p4: "foo"
    }
}
