// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import "components"

Rectangle {
    id: root
    width: 1280
    height: 960
    objectName: "root"
    color: "#222222"

    ListView {
        id: list
        objectName: "listView"
        anchors.fill: parent
        anchors.margins: 10
        orientation: Qt.Horizontal

        model: 20

        delegate: Item {
            objectName: "delegateItem" + index
            width: 154
            height: list.height

            Slider {
                anchors.fill: parent
                label: "Channel " + (index + 1)
            }
        }
    }
}
