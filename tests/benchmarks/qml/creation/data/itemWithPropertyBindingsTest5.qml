// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick 2.0

Item {
    property alias p1: obj.p1
    property alias p2: obj.p2
    property alias p3: obj.p3
    property alias p4: obj.p4

    QtObject {
        id: obj
        property int p1: 1
        property real p2: 1
        property bool p3: true
        property string p4: "foo"
    }
}
