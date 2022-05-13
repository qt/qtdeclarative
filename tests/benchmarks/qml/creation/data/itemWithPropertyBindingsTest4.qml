// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.0

Item {
    property int p1: foo.p1
    property real p2: foo.p2
    property bool p3: foo.p3
    property string p4: foo.p4

    property alias foo: obj

    QtObject {
        id: obj
        property int p1: 1
        property real p2: 1
        property bool p3: true
        property string p4: "foo"
    }
}
