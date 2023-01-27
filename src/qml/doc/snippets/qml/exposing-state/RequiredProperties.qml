// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
//![0]
pragma ComponentBehavior: Bound

import QtQuick

Window {
    id: root
    visible: true

    required property int thing

    Text {
        anchors.fill: parent
        text: "The thing is " + root.thing
    }

    component Inner: QtObject {
        objectName: "I can see " + root.thing + " because I'm bound."
    }
}
//![0]
