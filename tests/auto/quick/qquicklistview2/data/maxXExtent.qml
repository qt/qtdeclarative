// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick

Item {
    property alias view: view

    ListView {
        id: view
        model: 10
        width: 200
        height: 200

        Rectangle {
            anchors.fill: parent
            color: "transparent"
            border.color: "darkorange"
        }

        delegate: Rectangle {
            width: 100
            height: 100
            Text {
                text: modelData
            }
        }
    }
}
